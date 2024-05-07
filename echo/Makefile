CC = gcc
CFLAGS = -O2 -Wall -I .
LIB = -lpthread

all: echoserver echoclient

echoserver: echoserver.c csapp.o echo.o
	$(CC) $(CFLAGS) -o echoserver echoserver.c csapp.o echo.o $(LIB)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

echo.o: echo.c
	$(CC) $(CFLAGS) -c echo.c

echoclient: echoclient.c csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.c csapp.o $(LIB)

clean:
	rm -f *.o echoserver *~
	rm -f *.o echoclient *~
