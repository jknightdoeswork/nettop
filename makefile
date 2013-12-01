#COMPILER
CC=gcc
#FLAGS
CFLAGS=-Wall -Wextra

all: nodes

udp: proxy server

nodes: node.c swind.c
	$(CC) $(CFLAGS) node.c swind.c -o node; chmod +x node

clean:
	rm -rf node
