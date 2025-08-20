#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
// 功能：根据主机名和服务（端口/服务名），创建并连接到远端TCP服务器
// 返回：成功时返回已连接的socket文件描述符，失败返回-1
int tcp_connect(const char *hostname, const char *service)
{
    // 定义地址信息结构体和指针
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));

    // 指定不限定地址族（既支持IPv4又支持IPv6）
    hints.ai_family = AF_UNSPEC;
    // 指定流式套接字，即TCP
    hints.ai_socktype = SOCK_STREAM;

    // 根据主机名和服务（端口）获取地址链表
    int err = getaddrinfo(hostname, service, &hints, &res);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo %s\n", gai_strerror(err));
        return -1;
    }

    int sockfd = -1;
    for (p = res; p != NULL; p = p->ai_next)
    {
        // 创建socket，参数为当前地址的协议族、套接字类型和协议
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
        // 如果connect成功，直接break，sockfd即为所需描述符
        break;
    }

    freeaddrinfo(res);
    return p == NULL ? -1 : sockfd;
}

int main()
{
    const char *hostname = "www.baidu.com";
    const char *service = "http";
    int connfd = tcp_connect(hostname, service);
    assert(connfd != -1 && "tcp_connect() failed!");

    const char *request = "GET / HTTP/1.1\r\n"
                          "Host: www.baidu.com\r\n"
                          "Connection: close\r\n\r\n";
    send(connfd, request, strlen(request), 0);

    char buf[4096];
    int n;
    while ((n = recv(connfd, buf, sizeof(buf), 0)) > 0)
    {
        printf("%.*s", n, buf);
    }

    close(connfd);

    return 0;
}
