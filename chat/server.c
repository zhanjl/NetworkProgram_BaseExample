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
#define     MAXDATALEN  1024
#define     HEADLEN     4
#define     MAXFD       1000

int readn(int fd, char *buf, int size);

int main()
{
    int listenfd, connfd;
    struct sockaddr_in servaddr;
    int     sockfd[MAXFD];
    char    data[MAXDATALEN + HEADLEN];
    char    buf[MAXDATALEN+1];
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9999);    //端口号
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
     
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind error");
        return -1;
    }

    if (listen(listenfd, 5) < 0)
    {
        perror("listen error");
        return -1;
    }
    
    int i, j, n, len;
    for (i = 0; i < MAXFD; i++)
        sockfd[i] = 0;

    fd_set  rfds; 
    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(listenfd, &rfds);
        for (i = 0; i < MAXFD; i++)
        {
            if (sockfd[i] == 1)
                FD_SET(i, &rfds);
        }

        if (select(1024, &rfds, NULL, NULL, NULL) < 0)
        {
            perror("select error");
            return -1;
        }

        if (FD_ISSET(listenfd, &rfds))
        {
            connfd = accept(listenfd, NULL, NULL);
            sockfd[connfd] = 1;
            printf("client %d connected\n", connfd);
        }

        for (i = 0; i < MAXFD; i++)
        {
            if (sockfd[i] == 1 && FD_ISSET(i, &rfds))
            {
                n = readn(i, buf, HEADLEN);
                if (n < 0)
                {
                    perror("read error");
                    return -1;
                } 
                if (n == 0)   //客户端关闭
                {
                    printf("client %d closed\n", i);
                    close(i);
                    sockfd[i] = 0;
                    continue;
                }

                buf[HEADLEN] = '\0';
                len = atoi(buf);

                n = readn(i, buf, len);
                if (n < 0)
                {
                    perror("read error");
                    return -1;
                } 
                if (n == 0)   //客户端关闭
                {
                    close(i);
                    sockfd[i] = 0;
                    continue;
                }
                buf[len] = '\0';
                printf("client %d send %s", i, buf);
                sprintf(data, "%4d", len);
                strncat(data, buf, len);

                //向所有连接发送数据
                for (j = 0; j < MAXFD; j++)
                {
                    if (sockfd[j] == 1)
                        write(j, data, len + HEADLEN);
                }
            }
        }

    }
    return 0;
}

int readn(int fd, char *buf, int size)
{
    int n, left;
    left = size;
    while (left > 0)
    {
        if ((n = read(fd, buf, left)) <= 0)
            break;
        left -= n;
        buf += n;
    }
    if (n < 0)
        return -1;
    return size - left;
}
