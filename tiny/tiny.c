/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor="
                  "ffffff"
                  ">\r\n",
            body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    // CRLF로 해당 헤더 필드가 마무리 되었음을 알 수 있다.
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf)); 
    // header와 body는 CRLF로 구분한다
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); 
    Rio_writen(fd, buf, strlen(buf));

    Rio_writen(fd, body, strlen(body));                          
}

// 클라이언트로부터 받은 데이터를 보관해놓은 rp 버퍼
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  // 첫 째줄은 start line
  Rio_readlineb(rp, buf, MAXLINE);
  // header와 body의 구분자
  while(strcmp(buf, "\r\n")){
    // startline이후에는 header다
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return ;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // 정적 콘텐츠를 요청하는 경우
  if(!strstr(uri, "cgi-bin")){
    strcpy(cgiargs,"");
    strcpy(filename, ".");
    strcat(filename, uri);
    
    if (uri[strlen(uri)-1] =='/') {
      strcat(filename, "home.html");
    }
    return 1;
  }

  // 동적 컨텐츠를 요청하는 경우
  else{
    // uri 에 ?에 있는 포인터 반환
    ptr = index(uri, '?');

    // 쿼리스트링이 있을 경우
    if (ptr) {
      strcpy(cgiargs, ptr+1);

      // ?를 문자열의 끝부분으로 설정
      *ptr = '\0';
    }

    // 쿼리스트링이 없을 경우
    else{
      strcpy(cgiargs, "");
    }

    // 현재 디렉터리 기준으로 파일
    strcpy(filename, ".");
    strcat(filename, uri);

    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    
    get_filetype(filename, filetype);     
    sprintf(buf, "HTTP/1.0 200 OK\r\n");   
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    printf("Response headers:\n");
    printf("%s", buf);

    // 읽기 전용으로 파일을 연다
    srcfd = Open(filename, O_RDONLY, 0);   
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);      
}

void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	    strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	    strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	    strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	    strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mpg"))
	    strcpy(filetype, "video/mp4");
    else if (strstr(filename, ".mp4"))
	    strcpy(filetype, "video/mp4");
    else if (strstr(filename, ".mov"))
	    strcpy(filetype, "video/mp4");
    else
	    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // 자식 프로세스의 경우
  if (Fork()==0) {
    setenv("QUERY_STRING", cgiargs, 1);
    // 소켓의 파일 디스크립터를 표준 출력으로 재지정한다.
    Dup2(fd, STDOUT_FILENO);
    // 파일 이름, 인수 목록, 환경 설정 목록
    Execve(filename, emptylist, environ);
  }
  // 자식 프로세스가 종료할 때까지 부모 프로세스의 종료를 막는다.
  Wait(NULL);
}

// connecting socket 식별자
void doit(int fd){
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);

    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    // tiny 서버는 GET 메서드만 지원
    if (strcasecmp(method, "GET")) {
      clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this methd");
      return ;
    }

    // GET 메서드 일 경우 헤더를 분석
    read_requesthdrs(&rio);

    // 정적 vs 동적 파일 분석
    is_static = parse_uri(uri, filename, cgiargs);

    if (stat(filename, &sbuf)<0)
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny coudn't read the file");
      return ;
    }

    if (is_static) {
      if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
        clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
        return ;
      }

      serve_static(fd, filename, sbuf.st_size);
    }
    else{
      if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
        clienterror(fd, filename, "403", "Forbidden", "Tiny coudln't run the GCI program");
        return;
      }
      serve_dynamic(fd, filename, cgiargs);
    }
    return ;
  }

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    Close(connfd);
  }
}
