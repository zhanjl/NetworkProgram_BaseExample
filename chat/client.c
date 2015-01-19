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

inline int max(int num1, int num2)
{
    return num1 > num2 ? num1 : num2;
}

int readn(int fd, char *buf, int size);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("you must input an IP address\n");
        return -1;
    }

    int sockfd, n, datalen;
    char data[HEADLEN + MAXDATALEN];
    char buf[MAXDATALEN+1];
    struct sockaddr_in  servaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9999);
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
    {
        perror("inet_pton error");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect error");
        return -1;
    }

    fd_set  rfds;
    
    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);

        if (select(max(sockfd, STDIN_FILENO) + 1, &rfds, NULL, NULL, NULL) < 0)
        {
            perror("select error");
            return -1;
        }
        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            if (fgets(buf, MAXDATALEN, stdin) == NULL)  //读到EOF
            {
                printf("input EOF\n");
                break;
            }
            n = strlen(buf);
            sprintf(data, "%4d", n);
            strncat(data, buf, n);
            write(sockfd, data, n+HEADLEN);
        }
        if (FD_ISSET(sockfd, &rfds))
        {
            if ((n = readn(sockfd, buf, 4) ) < 0)  
            {
                perror("read error");
                return -1;
            }
            if (n == 0) 
            {
                printf("server colsed\n");
                break;
            }
            buf[4] = '\0';
            datalen = atoi(buf);
            if ((n = readn(sockfd, buf, datalen)) < 0)
            {
                perror("read error");
                return -1;
            }
            buf[datalen] = '\0';
            printf("receive %s", buf);
        }
    }
    close(sockfd);
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
