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

node: node.o swind.o parser.o
	$(CC) $(CFLAGS) -o node node.o swind.o parser.o dvr.o

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c

dvr.o: dvr.c
	$(CC) $(CFLAGS) -c dvr.c

clean:
	rm -rf *.o node parser

