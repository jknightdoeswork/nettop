#ifndef _node_h
#define _node_h

/* INCLUDES */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <strings.h>
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
#include <pthread.h>
#include <limits.h>
#include <math.h>

/* DEFINES */

#define DEBUGWINDOWS 0

#define TIMEOUT 8
#define DROPPROB 0
#define QUEUESIZE 20
#define SLIDENSIZE 8
#define BUFSIZE 256
#define TIMEOUTVAL 1
#define LOCALHOST "127.0.0.1"
#define DELIM "`\n"
#define CONTROLCHARS "&+-"

/* GLOBALS */
extern struct list *nodelist;
extern int globalport;
extern int dvr_reset_interval;

/* STRUCTURES */

/* used for holding tokenized messages */
struct msgtok
{
    char *orig;
    char *ack, *src, *dest, *pay;
    int acknum;
    int type;
};

/* in this proxy enqT is for timeouts, acknum is the ack it is */
struct packet
{
    char *msg;
    time_t enqT;
    int acknum;
    int received;
    
    /* sent signifies if it has ever been attempted to send */
    int sent;
    
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

struct windowlist
{
    struct window *sendq, *recvq;
    struct window *ackq;
    char* name;

    struct windowlist *next, *prev;
};


/* holds a list of send and receive queues for every other node */
struct routing_table_entry
{
    char* name;
    char* through;

    float weight;
    int drop;
    int delay;
    
    struct routing_table_entry *next, *prev;
};

/* is the window used for reliable transfer between nodes */
struct node
{
    struct routing_table_entry *routing_table;
    struct routing_table_entry *connected_edges;

    struct windowlist *windows;

    char* name;
    int port, socket;
    
    pthread_t thread;
    int dead;
    
    struct sockaddr *addr;
    
    struct node *next, *prev;

    /* logfile for dvr logs, srlog for selective repeat logs */
    FILE* logfile, *srlog;
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
    enumnack = 1,
    enumpureack = 2,
    enumrealmsg = 3,
    enumdvrmsg = 4,
};

/* FUNCTIONS */

/* for node.c */
struct routing_table_entry *rtappend(struct node *w, char* name,
		char* through, int delay, int drop, float weight, int neighbourtable);
struct node *append(struct list *l, char* name);
int enqueue(struct window *q, char* msg);
int comesfirst(int first, int a, int b);
struct packet *dequeue(struct window *q);
struct node *addnode(char* name);
void addedge(char* nodeaname, char* nodebname, int delay, int drop);
int reqack(struct window *q);
void getaddr(struct node *w);
int getportfromname(char* name);
struct packet *dequeue_el(struct window *q, struct packet *el);
struct sockaddr *getaddrfromname(char* name);
void sendudp(char *src, char *msg, char *dest);
int getsockfromname(char* name);
struct node *getnodefromname(char* name);
int setupmyport(char* name);
void spawnthread(struct node *n);
void spawnallthreads();
void sigkillthread(char* name);
void sigkillall();
void *mainloop(void *arg);
void freepacket(struct packet* p);
void freewindowlist(struct windowlist* w);
void freeroutingtableentry(struct routing_table_entry *rte);
void createwindowlist(struct node* w, char* name);
struct windowlist* getwindowlist(struct node* w, char* name);
struct routing_table_entry *getroutingtableentry(struct node *src, char *dest, int master);
void deleterte(struct node *n, struct routing_table_entry *rte, int master);
void syncwait();

void printeverything();
void printnodewindows(struct node *n);
void printwindowlist(struct node *n, struct windowlist *w);
void printwindow(struct node *n, struct window *w);
void printdvr(struct node *n);


/* for swind.c */
int plusone(int i);
int receivemsg(char *name);
int iamdest(char *name, struct msgtok *tok);
int msginorder(struct window *q, int ack);
void sendnacks(struct window *q, struct msgtok *tok);
void sendbacknack(struct msgtok *tok, int out);
void sendbackack(struct msgtok *tok);

void handleack(char *name, struct msgtok *);
void handlenack(char *name, struct msgtok *);
void handlemsg(char *name, struct msgtok *);
int ihavemsg(struct window *q, int ack);
struct msgtok *tokenmsg(char *msg);
int interpret(char *msg);
void freetok(struct msgtok *tok);

/* for dvr.c */
/* set_interval
 * Updates the duration between dvr steps.
 * Default is 2 seconds.
 */
void set_interval(int seconds);

/* dvr_step
 * Send dvr information to neighbours
 * if current time - laststep > interval.
 * returns current time if it sent info, and last
 * step otherwise, so that it can be called in
 * succession using the return value as last step.
 * eg)
 *      time_t dvr_time = dvr_step(source_node, -1)
 *      ...
 *      dvr_time = dvr_step(source_node, dvr_time);
 */
time_t dvr_step(struct node* a, time_t laststep);

/* send_dvr_message
 * used by dvr_step to send routing table from src to dest.
 * sends each routing table entry as a seperate message
 */
void send_dvr_message(struct node* src, char* dest);

/* handledvrmessage
 * updates the routing table using the entry stored in msg.
 * msg->pay is the weight
 * msg->src is the neighbour
 * msg->dest is the destination node
 * node a is the routing table we will update
 */
void handledvrmessage(struct node* nodea, struct msgtok* msg);

time_t dvr_reset_step(struct node* a, time_t lastresetstep);
void set_dvr_reset_interval(int seconds);
void reset_dvr(struct node* a);


/* for log.c */
/* createlogfile
 * creates the log file for the node at logs/timestamp/nodename
 */
FILE* createlogfile(char* nodename);

/* createlogdir
 * creates the directory logs/timestamp
 */
void createlogdir();
void log_routing_table(struct node* a, int timestep, int resetstep);
void log_routing_table_to_fd(struct node* a, int timestep, int resetstep, FILE* logfile);
#endif
