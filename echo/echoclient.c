#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if(argc !=3)
    {
        fprintf(stderr, "useage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    // 클라이언트 소켓 생성(connect까지 완료)
    clientfd = Open_clientfd(host, port);
    // 해당 소켓과 읽기버퍼를 연결
    Rio_readinitb(&rio, clientfd);

    // stdin에서 MAXLINE만큼 읽어서 buffer에 저장한다.
    while (Fgets(buf, MAXLINE, stdin)!=NULL) {
        // buffer에 있는 내용을 client socket의 버퍼에 write한다
        // client socket의 버퍼에 write를 했지만 connecting socket과 connect된 상태이므로
        // connecting socket에게 데이터를 보낸다.
        Rio_writen(clientfd, buf, strlen(buf));

        // 서버로부터 받은 데이터를 사용자가 제공한 buffer로 옮긴다.
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }

    Close(clientfd);
    exit(0);
}