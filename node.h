//
//  node.h
//  

#ifndef _node_h
#define _node_h

/* MESSAGE FORMAT: 15`NODEA`NODEB`MESSAGE
                Ack`Src`Dest`Payload */

/* INCLUDES */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

/* DEFINES */

#define DROPPROB 0
#define QUEUESIZE 20
#define SLIDENSIZE 8
#define BUFSIZE 256
#define TIMEOUTVAL 1
#define LOCALHOST "127.0.0.1"
#define DELIM "`"

/* GLOBALS */
extern struct list *nodelist;
extern int globalport;

/* STRUCTURES */

/* used for holding tokenized messages */
struct msgtok
{
    char *ack, *src, *dest, *pay;
    int acknum;
};

/* in this proxy enqT is for timeouts, acknum is the ack it is */
struct qel
{
    char msg[BUFSIZE];
    time_t enqT;
    int acknum;
    int received;
    
    struct qel *next, *prev;
};

/*queue struct, first is the first item in the sliding window,
 last is the last item in sliding window */
struct queue
{
	struct qel *head;
	struct qel *tail;
	int sz;
	int max;
	int first;
	int last;
    int cur;
};

/* holds a list of send and receive queues for every other node */
struct queuelist
{
    struct queue *sendq, *recvq;
    char name[BUFSIZE];
    
    struct queuelist *next, *prev;
};

/* is the window used for reliable transfer between nodes */
struct window
{
    struct queuelist *qlist;
    char name[BUFSIZE];
    int port, socket;
    
    struct sockaddr *addr;
    
    struct window *next, *prev;
};

/* holds a nodes lists of buffers it can send with */
struct list
{
    struct window *head, *tail;
    int sz;
};

/* enum for the type of queue we are looking at */
enum globalenums
{
    enumnack = -1,
    enumpureack = 0,
    enumrealmsg = 1,
};

/* FUNCTIONS */

/* for node.c */
struct queuelist *qlappend(struct window *w, char name[]);
struct window *append(struct list *l, char name[]);
void printqueue(struct queue *q);
int enqueue(struct queue *q, char* msg);
int comesfirst(int first, int a, int b);
struct qel *dequeue(struct queue *q);
struct window *addnode(char name[]);
int reqack(struct queue *q);
void getaddr(struct window *w);
int getportfromname(char name[]);
struct qel *dequeue_el(struct queue *q, struct qel *el);
struct sockaddr *getaddrfromname(char name[]);
void sendudp(char *src, char *msg, char *dest);
int getsockfromname(char name[]);
struct window *getwindowfromname(char name[]);
int setupmyport(char name[]);
struct queuelist *getql(char *src, char *dest);

/* for swind.c */
int plusone(int i);
int receivemsg(char *name);
int iamdest(char *name, char *msg);
int msginorder(struct queue *q, int ack);
void sendnacks(struct queue *q, char *msg);
void sendbacknack(char *msg, int out);
void sendbackack(char *msg);

void handleack(char *name, char *msg);
void handlenack(char *name, char *msg);
void handlemsg(char *name, char *msg);
int ihavemsg(struct queue *q, int ack);
struct msgtok *tokenmsg(char *msg);
int interpret(char *msg);
void freetok(struct msgtok *tok);


#endif
