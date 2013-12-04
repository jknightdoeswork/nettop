#COMPILER
CC=gcc
#FLAGS
CFLAGS=-Wall -Wextra

all: node

udp: proxy server

node.o: node.c
	$(CC) $(CFLAGS) -c node.c

swind.o: swind.c
	$(CC) $(CFLAGS) -c swind.c

node: node.o swind.o parser.o dvr.o log.o
	$(CC) $(CFLAGS) -o node node.o swind.o parser.o dvr.o log.o -lpthread

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c

dvr.o: dvr.c
	$(CC) $(CFLAGS) -c dvr.c

log.o: log.c
	$(CC) $(CFLAGS) -c log.c

clean:
	rm -rf *.o node parser

