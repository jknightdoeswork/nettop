#COMPILER
CC=gcc
#FLAGS
CFLAGS=-Wall -Wextra

all: nodes

udp: proxy server

nodes: node.c swind.c
	$(CC) $(CFLAGS) node.c swind.c -o node; chmod +x node

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c

parser: parser.o
	$(CC) -o parser parser.o

clean:
	rm -rf *.o node parser
