#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#define MAXLINE 1024

// 服务器
// 1. 监听一个众所周知的端口
// 2. 监听通配符地址 (wildcard address)：0.0.0.0
int tcp_listen(const char *port)
{
    // 1. 获取服务器的地址信息
    struct addrinfo hints, *result, *p;
    // 初始化hints
    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;     // AI_PASSIVE + NULL: wildcard address
    hints.ai_family = AF_UNSPEC;     // IPv4, IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    int err = getaddrinfo(NULL, port, &hints, &result);
    if (err)
    {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(err));
        return -1;
    }

    int sockfd = -1;
    int flag = 1;
    for (p = result; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0)
        {
            continue;
        }
        // 设置地址复用
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            continue;
        }
        if (listen(sockfd, 1024) == -1)
        { // backlog: 已连接队列的大小
            close(sockfd);
            continue;
        }
        break;
    }

    freeaddrinfo(result); // 释放资源
    return sockfd;
}

void *ipaddr(struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET)
    {
        return &((struct sockaddr_in *)addr)->sin_addr;
    }
    else if (addr->sa_family == AF_INET6)
    {
        return &((struct sockaddr_in6 *)addr)->sin6_addr;
    }
    return NULL;
}

void do_echo(int connfd, const char *ipstr)
{
    char message[MAXLINE];
    int n;
    while ((n = recv(connfd, message, MAXLINE, 0)) > 0)
    {
        if (n < 0)
        {
            perror("recv()");
            exit(1);
        }
        else if (n == 0)
        {
            printf("Info: %s closed\n", ipstr);
        }
        else
        {
            send(connfd, message, n, 0);
        }
    }
}

void sighandler(int)
{
    // 回收子进程：wait, waitpid
    // wait 不可以：一直阻塞

    // 名言：细节是魔鬼
    int save = errno; // 保存以前的errno
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = save; // 恢复errno
}

int main()
{
    // 捕获 SIGCHLD 信号
    signal(SIGCHLD, sighandler);
    int listenfd = tcp_listen("22222");
    assert(listenfd != -1 && "tcp_listen() failed!");

    for (;;)
    {
        // 记录日志
        struct sockaddr_storage ss; // 传出参数
        socklen_t len = sizeof(ss); // 传入传出参数
        int connfd = accept(listenfd, (struct sockaddr *)&ss, &len);
        // 套接字 --> IP地址
        char ipstr[50];
        inet_ntop(ss.ss_family, ipaddr((struct sockaddr *)&ss), ipstr, sizeof(ipstr));
        

        pid_t pid;
        switch (pid = fork())
        {
        case -1:
            perror("fork()");
            break;
        case 0:
            // 子进程
            close(listenfd);
            do_echo(connfd, ipstr);
            close(connfd);
            exit(0);
        default:
            // 父进程
            printf("Info: %s connect\n", ipstr);
            close(connfd);
        }
    }
}