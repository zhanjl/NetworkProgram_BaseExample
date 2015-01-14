#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#define     SA  struct sockaddr
#define     MAXLEN  1024

int main()
{
    int listenfd, connfd;
    struct sockaddr_in  servaddr;   //服务器端地址
    struct sockaddr_in  cliaddr;   //客户端地址
    pid_t   pid;
    char    buf[MAXLEN];
    int     n, addrlen; 
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return -1;
    }
    
    //设置服务器端地址
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(9999);

    //绑定服务器端地址
    if (bind(listenfd, (SA*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind error");
        return -1;
    }
    //把该套接字设为监听套接字
    if (listen(listenfd, 5) < 0)
    {
        perror("listen error");
        return -1;
    }

    for ( ; ; )
    {
        addrlen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (SA*)&cliaddr, &addrlen)) < 0)
        {
            perror("accept error");
            return -1;
        }

        if ((pid = fork()) == 0)    //子进程
        {
            close(listenfd);    //关闭监听套接字
            //处理从客户端发来的数据
            while ((n = read(connfd, buf, MAXLEN)) > 0)
            {
                //在这里可以对客户端发来的数据进程任意处理
                //比如打包和拆包，比如可以写成简单的http服务
                if (write(connfd, buf, n) < 0)
                {
                    perror("write error");
                    exit(-1);
                }
            }
            //关闭连接套接字 
            close(connfd);
            if (n < 0)
            {
                perror("read error");
                exit(-1);
            }
            printf("server process exit\n");
            exit(0);    //退出进程
        }
        //父进程
        close(connfd);      //关闭已连接套接字
    }
    return 0;
}
