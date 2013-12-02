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

#define DEBUGWINDOWS 0

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
struct packet
{
    char msg[BUFSIZE];
    time_t enqT;
    int acknum;
    int received;
    
    struct packet *next, *prev;
};

/*queue struct, first is the first item in the sliding window,
 last is the last item in sliding window */
struct window
{
	struct packet *head;
	struct packet *tail;
	int sz;
	int max;
	int first;
	int last;
    int cur;
};


/* holds a list of send and receive queues for every other node */
struct routing_table_entry
{
    struct window *sendq, *recvq;
    char name[BUFSIZE];
    //planning
    char through[BUFSIZE];
    int weight;
    
    // eplanning
    struct routing_table_entry *next, *prev;
};

/* is the window used for reliable transfer between nodes */
struct node
{
    struct routing_table_entry *qlist;
    char name[BUFSIZE];
    int port, socket;
    
    struct sockaddr *addr;
    
    struct node *next, *prev;
};

/* holds a nodes lists of buffers it can send with */
struct list
{
    struct node *head, *tail;
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
struct routing_table_entry *rtappend(struct node *w, char name[]);
struct node *append(struct list *l, char name[]);
void printwindow(struct window *q);
int enqueue(struct window *q, char* msg);
int comesfirst(int first, int a, int b);
struct packet *dequeue(struct window *q);
struct node *addnode(char name[]);
int reqack(struct window *q);
void getaddr(struct node *w);
int getportfromname(char name[]);
struct packet *dequeue_el(struct window *q, struct packet *el);
struct sockaddr *getaddrfromname(char name[]);
void sendudp(char *src, char *msg, char *dest);
int getsockfromname(char name[]);
struct node *getnodefromname(char name[]);
int setupmyport(char name[]);
struct routing_table_entry *get_routing_table_entry(char *src, char *dest);

/* for swind.c */
int plusone(int i);
int receivemsg(char *name);
int iamdest(char *name, char *msg);
int msginorder(struct window *q, int ack);
void sendnacks(struct window *q, char *msg);
void sendbacknack(char *msg, int out);
void sendbackack(char *msg);

void handleack(char *name, char *msg);
void handlenack(char *name, char *msg);
void handlemsg(char *name, char *msg);
int ihavemsg(struct window *q, int ack);
struct msgtok *tokenmsg(char *msg);
int interpret(char *msg);
void freetok(struct msgtok *tok);


#endif
