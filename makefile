#COMPILER
CC=gcc
#FLAGS
CFLAGS=-Wall -Wextra -g

all: node

udp: proxy server

node.o: node.c
	$(CC) $(CFLAGS) -c node.c

swind.o: swind.c
	$(CC) $(CFLAGS) -c swind.c

node: node.o swind.o dvr.o log.o
	$(CC) $(CFLAGS) -o node node.o log.o swind.o dvr.o -lpthread

dvr.o: dvr.c
	$(CC) $(CFLAGS) -c dvr.c

log.o: log.c
	$(CC) $(CFLAGS) -c log.c

clean:
	rm -rf *.o node

