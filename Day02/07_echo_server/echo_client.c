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

#define MAXLINE 1024

// 成功：返回套接字文件描述符
// 失败: -1
int tcp_connect(const char *hostname, const char *service)
{
    struct addrinfo hints, *res, *p;
    bzero(&hints, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(hostname, service, &hints, &res);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo %s\n", gai_strerror(err));
        return -1;
    }
    int sockfd = -1;
    for (p = res; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            // 创建失败则尝试下一个地址
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            // 连接失败，关闭socket并尝试下一个地址
            close(sockfd);
            continue;
        }
        break;
    }
    freeaddrinfo(res);
    return p == NULL ? -1 : sockfd;
}

void do_echo(int connfd)
{
    char sendline[MAXLINE];
    char recvline[MAXLINE];
    while (fgets(sendline, MAXLINE, stdin) != NULL)
    {
        // 发送数据
        int n;
        n = send(connfd, sendline, strlen(sendline), 0);
        if (n < 0)
        {
            perror("send error");
            exit(EXIT_FAILURE);
        }
        n = recv(connfd, recvline, MAXLINE, 0);
        if (n < 0)
        {
            perror("recv error");
            exit(EXIT_FAILURE);
        }
        else if (n == 0)
        {
            printf("server closed\n");
            break;
        }
        else
        {
            printf("%.*s", n, recvline);
        }
    }
}

int main()
{
    int connfd = tcp_connect("127.0.0.1", "22222");
    assert(connfd != -1 && "tcp_connect() failed!");

    do_echo(connfd);

    close(connfd);
    return 0;
}
