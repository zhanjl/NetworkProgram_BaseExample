#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#define     SA  struct sockaddr
#define     MAXLEN  1024
int main(int argc, char *argv[])
{
    int sockfd, n, len;
    char buf[MAXLEN];
    struct sockaddr_in  serveraddr; //服务器地址
     
    if (argc != 2)
    {
        printf("we need an IP address");
        return -1;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket error\n");
        return -1;
    }

    //设置服务器地址
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port       = htons(9999);    //服务器进程端口号
    //把字符串形式的点分IP地址转换为二进制数值型的IP地址
    if (inet_pton(AF_INET, argv[1], &serveraddr.sin_addr) <= 0)
    {
        printf("inet_pton error");
        return -1;
    }

    //和服务器建立连接
    //客户端(sockfd)的IP地址和端口号自动分配
    if (connect(sockfd, (SA*)&serveraddr, sizeof(serveraddr)) < 0)
    {
        printf("connect error");
        return -1;
    }

    //处理数据
    while (fgets(buf, 1023, stdin) != NULL)
    {
        len = strlen(buf);
        //向服务器端写数据
        if (write(sockfd, buf, len) < 0)
        {
            perror("write error:");
            return -1;
        }

        //从服务器端读数据
        if ( (n = read(sockfd, buf, MAXLEN - 1)) < 0 )
        {
            perror("read error");
            return -1;
        }
        buf[n] = '\0';
        //在标准输出上打印出该数据
        fputs(buf, stdout);
    }
    //关闭连接
    close(sockfd);
    return 0;
}
