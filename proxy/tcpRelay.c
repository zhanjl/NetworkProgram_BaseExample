//每个客户端只连接到一个server，是个n:1结构
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>

#define MAXCLIENT   1000    //能连接的最大客户端数
#define MAXDATA     1024    //一次能传送的最大数据数
//每个代理结构体，对应一个客户端和一个服务器描述符
struct proxy {
    int clifd;
    int servfd;
};

int main(int argc, char *argv[])
{
    struct proxy connected[MAXCLIENT] = {0};   //保存已建立的连接  
    if (argc != 2)
    {
        printf("ERROR: please input server addr\n");
        return -1;
    }

    int listenfd, servfd, clifd;    //代理服务的监听套接字，与服务器和客户端连接套接字
    struct sockaddr_in  servaddr;   //唯一的服务器地址
    //初始化服务器地址
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9999);    //服务器端口
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
    {
        perror("inet_pton error");
        return -1;
    }

    struct sockaddr_in proxyaddr;   //代理服务地址（也就是本机）
    //初始化代理服务器地址
    bzero(&proxyaddr, sizeof(proxyaddr));
    proxyaddr.sin_family = AF_INET;
    proxyaddr.sin_port = htons(8888);    //代理服务端口
    proxyaddr.sin_addr.s_addr = htonl(INADDR_ANY);


    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("listen socket error");
        return -1;
    }

    if (bind(listenfd, (struct sockaddr*)&proxyaddr, sizeof(proxyaddr)) < 0)
    {
        perror("bind error");
        return -1;    
    }

    if (listen(listenfd, 5) < 0)
    {
        perror("listen error");
        return -1;
    }

    int maxfd;
    maxfd = listenfd;
    fd_set readfds;
    char buf[MAXDATA];
    int i, n;
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);
         
        for (i = 0; i < MAXCLIENT; i++)
        {
            if (connected[i].clifd != 0)
            {
                FD_SET(connected[i].clifd, &readfds);
            }

            if (connected[i].servfd != 0)
            {
                FD_SET(connected[i].servfd, &readfds);
            }
        }

        if (select(maxfd+1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select error");
            return -1;
        }

        if (FD_ISSET(listenfd, &readfds))
        {
            if ((clifd = accept(listenfd, NULL, NULL)) < 0)
            {
                perror("accept error");
                return -1;
            }
            if (clifd > maxfd)
                maxfd = clifd;

            for (i = 0; i < MAXCLIENT; i++)
            {
                if (connected[i].clifd == 0 && connected[i].servfd == 0)
                    break;
            }
             
            if (i == MAXCLIENT)
            {
                printf("too many client, we must close %d\n", clifd);
                close(clifd);
            }
            
            //有一个客户端连接，则新建一个和服务器的连接
            if ((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("socket error");
                return -1;
            }
            
            if (connect(servfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
            {
                perror("connect server error");
                return -1;
            }
            if (servfd > maxfd)
                maxfd = servfd;
            connected[i].clifd = clifd;
            connected[i].servfd = servfd;
            printf("client %d connect to server %d\n", clifd, servfd);
        }

        //查看已建立的连接中是否有数据交换
        for (i = 0; i < MAXCLIENT; i++)
        {
            if (connected[i].clifd == 0 && connected[i].servfd == 0)
                continue;
            
            if (FD_ISSET(connected[i].clifd, &readfds))
            {
                if ((n = read(connected[i].clifd, buf, MAXDATA)) < 0)
                {
                    perror("read error");
                    return -1;
                }
                //客户端关闭
                else if (n == 0)
                {
                    if (connected[i].servfd != 0)
                        shutdown(connected[i].servfd, SHUT_WR); //关闭服务器端连接
                    close(connected[i].clifd);
                    connected[i].clifd = 0;     //不可能再从客户端读数据
                }
                else
                {
                    write(connected[i].servfd, buf, n); //否则向服务器写数据
                    buf[n] = '\0';
                    printf("%d write to %d: %s\n", connected[i].clifd, connected[i].servfd, buf);
                }
            }

            if (FD_ISSET(connected[i].servfd, &readfds))
            {
                if ((n = read(connected[i].servfd, buf, MAXDATA)) < 0)
                {
                    perror("read error");
                    return -1;
                }
                //服务器端关闭
                else if (n == 0)
                {
                    if (connected[i].clifd)
                        shutdown(connected[i].clifd, SHUT_WR); //关闭服务器端连接
                    close(connected[i].servfd);
                    connected[i].servfd = 0;
                }
                else
                {
                    write(connected[i].clifd, buf, n); //否则向服务器写数据
                    buf[n] = '\0';
                    printf("%d write to %d: %s\n", connected[i].servfd, connected[i].clifd, buf);
                }
            }
        }
    }
    return 0;
}
