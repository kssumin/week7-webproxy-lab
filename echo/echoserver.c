#include "csapp.h"
#include <dirent.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include "echo.h"

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc != 2)
    {
        fprintf(stderr, "usage : %s <port>\n", argv[0]);
        exit(0);
    }
    
    // listening socket 생성
    listenfd = open_listenfd(argv[1]);

    while(1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        echo(connfd);
        Close(connfd);
    }
    exit(0);

}