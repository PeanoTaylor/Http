#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void *ipaddr(struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if (addr->sa_family == AF_INET6)
    {
        return &(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <hostname>\n", argv[0]);
        return -1;
    }
    // 根据域名获取ip地址信息
    struct addrinfo hints, *res, *p;
    bzero(&hints, sizeof(hints));
    // 初始化hints结构体
    hints.ai_family = AF_UNSPEC;     // 不限制协议族
    hints.ai_socktype = SOCK_STREAM; // TCP;

    int err = getaddrinfo(argv[1], NULL, &hints, &res); // hints指明期望的协议、套接字类型等条件
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo %s\n", gai_strerror(err));
        return -1;
    }
    // 遍历结果，打印IP地址
    printf("IP addresses for %s:\n", argv[1]);
    for (p = res; p != NULL; p = p->ai_next)
    {
        char ipstr[INET6_ADDRSTRLEN]; // 用于存储IP地址字符串
        char *ipver;
        if (p->ai_family == AF_INET)
        {
            ipver = "IPv4";
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            // 将套接字转为IP
            inet_ntop(p->ai_family, ipaddr(p->ai_addr), ipstr, sizeof(ipstr));
        }
        else if (p->ai_family == AF_INET)
        {
            ipver = "IPv6";
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            // 将套接字转为IP
            inet_ntop(p->ai_family, ipaddr(p->ai_addr), ipstr, sizeof(ipstr));
        }
        printf("%s: %s\n", ipver, ipstr);
    }
    // 释放地址信息
    freeaddrinfo(res);
    return 0;
}