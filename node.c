//
//  node.c
//  

#include "node.h"

struct list *nodelist;
int globalport = 3490;

/* add a node to our nodelist */
struct window *addnode(char name[])
{
    /* add it to the set of all nodes */
    /* add it to every existing nodes list of queues */
    /* give it queues for all existing nodes */
    /* append handles it all because append is magic */
    return append(nodelist, name);
}

/* append a queuelist to a window */
struct queuelist *qlappend(struct window *w, char name[])
{
    struct queuelist *tmp;
    struct queuelist *new;
    
    tmp = w->qlist;
        
    new = malloc(sizeof(struct queuelist));
    
    new->sendq = malloc(sizeof(struct queue));
    new->recvq = malloc(sizeof(struct queue));
    
    /* initialize values for the queue */
    new->sendq->head = NULL;
    new->sendq->tail = NULL;
    new->sendq->sz = 0;
    new->sendq->max = SLIDENSIZE;
    new->sendq->first = 0;
    new->sendq->last = SLIDENSIZE-1;
    new->sendq->cur = 0;
    
    new->recvq->head = NULL;
    new->recvq->tail = NULL;
    new->recvq->sz = 0;
    new->recvq->max = SLIDENSIZE;
    new->recvq->first = 0;
    new->recvq->last = SLIDENSIZE-1;
    new->recvq->cur = 0;
    
    new->next = NULL;
    strcpy(new->name, name);
    
    if(tmp == NULL) /* no entries */
    {
        w->qlist = new;
            
        new->prev = NULL;
    }
    else
    {
        while(1)
        {
            if(strcmp(tmp->name, name) == 0)
            {
                printf("Qlist already exists for node: %s\n", name);
                return NULL;
            }
            
            if(tmp->next == NULL)
                break;
            
            tmp = tmp->next;
        }
        
        /* now tmp will point to the last element */
        tmp->next = new;
        new->prev = tmp;
    }
    
    return new;
}

/* append a window to the list */
struct window *append(struct list *l, char name[])
{
    /* fail if l is null */
    if(l == NULL)
    {
        printf("List is NULL!\n");
        return NULL;
    }
    
    struct window *tmp = l->head;
    
    /* otherwise there is an el, append to the end */
    while(tmp != NULL)
    {
        if(strcmp(tmp->name, name) == 0)
        {
            printf("Already have an entry for %s\n", name);
            return NULL;
        }
        
        tmp = tmp->next;
    }
    
    tmp = l->head;
    
    while(tmp != NULL)
    {
        /* for every node, append an entry for this new node */
        qlappend(tmp, name);
        
        tmp = tmp->next;
    }
    
    struct window *w = malloc(sizeof(struct window));
    
    w->next = NULL;
    
    if(l->tail != NULL)
        l->tail->next = w;
    
    w->prev = l->tail;
    l->tail = w;
    
    /* then it is also the head */
    if((l->sz)++ == 0)
    {
        l->head = w;
    }
    
    tmp = l->head;
    
    /* build the new window's list to have entries for all
     existing nodes and add an entry with all existing nodes
     for the new node */
    while(tmp != NULL)
    {
        /* for the new node, add entries for all existing nodes */
        /* skip the last one because it is the one we are adding as
         we speak */
        if(tmp->next != NULL)
            qlappend(w, tmp->name);
        
        tmp = tmp->next;
    }
    
    strcpy(w->name, name);
    
    w->port = globalport++;
    
    getaddr(w); /* port to make sense */
    
    w->socket = setupmyport(name);
    
    return w;
}

/* given a node name, get it's corresponding port */
int getportfromname(char name[])
{
    struct window *w = nodelist->head;
    
    while(w != NULL)
    {
        if(strcmp(w->name, name) == 0)
        {
            return w->port;
        }
        
        w = w->next;
    }
    
    /* ERROR GETTING PORT */
    printf("Error getting port\n");
    return -1;
}

/* give them an ack number to use, and increment so we
 don't reuse */
int reqack(struct queue *q)
{
    int tmp = q->cur;
    q->cur = plusone(q->cur);
    return tmp;
}

/* prints all the queue elements then the size */
void printqueue(struct queue *q)
{
    if(q == NULL)
        return;
    
    struct qel *el = q->head;
    
    while(el != NULL)
    {
        printf("[%d]-%s\n", el->received, el->msg);
        el = el->next;
    }
    
    printf("Sz: %d\n", q->sz);
    
    return;
}

/* print a list values */
void printlist(struct list *l)
{
    if(l == NULL)
        return;
    
    struct window *w = l->head;
    
    while(w != NULL)
    {
        printf("Window [%s]\n", w->name);
        struct queuelist *ql = w->qlist;
        
        while(ql != NULL)
        {
            printf("- QL [%s]\n", ql->name);
            printf("--------------------\nSendq sz [%d - %d]:\n",
                   ql->sendq->first, ql->sendq->last);
            printqueue(ql->sendq);
            printf("--------------------\nRecvq sz [%d - %d]:\n",
                   ql->recvq->first, ql->recvq->last);
            printqueue(ql->recvq);
            printf("--------------------\n");
            
            ql = ql->next;
        }
        w = w->next;
    }
    
    return;
}

/* does a come before b wrt first */
int comesfirst(int first, int a, int b)
{
    int count = 0;
    
    while(count < QUEUESIZE)
    {
        if(first == a)
            return 1;
        else if(first == b)
            return 0;
        
        first = plusone(first);
        count++;
    }
    
    printf("I counted to queuesize and never found 'a' OR 'b'!\n");
    exit(1);
}

//enqueue function, manages all the little things
int enqueue(struct queue *q, char* msg)
{
    struct qel *el;
    
    /* force the enqueue to be in order */
    
	if(q->sz >= q->max)
		return -1; // queue is full
	
	struct qel *e = malloc(sizeof(struct qel));
	
	if(e == NULL)
		return -1;
    
	strcpy(e->msg, msg);
    
    struct msgtok *tok = tokenmsg(msg);
    
    e->acknum = tok->acknum;
    e->received = 0;
    
    freetok(tok);
    
    /* adjust to put it in order */
    el = q->head;
    
    while(el != NULL)
    {
        if(el->next == NULL ||
           comesfirst(q->first, e->acknum, el->next->acknum))
            break;
        
        el = el->next;
    }
    
    /* if el is null then there were no elements in the list */
    if(el == NULL)
    {
        e->next = NULL;
        e->prev = NULL;
        q->sz++;
        q->head = e;
        q->tail = e;
        
        e->enqT = time(NULL);
        
        return 0;
    }
    /* this will cover the case of inserting into the head */
    else if(comesfirst(q->first, e->acknum, el->acknum))
    {
        e->prev = NULL;
        e->next = el;
        el->prev = e;
        q->head = e;
        q->sz++;
        
        e->enqT = time(NULL);
        
        return 0;
    }
    
    /* el now points to the element before where we insert */
    /* if el is not null then it points to the element before
     where we want to insert */
    e->next = el->next;
	e->prev = el;
    el->next = e;
    
    /* if null it is the last element so set the tail */
    if(e->next == NULL)
        q->tail = e;
    else /* there is an el after, so set it's prev pointer to us */
        e->next->prev = e;
	
	e->enqT = time(NULL);
    
    q->sz++;
	
	return 0;
}

/* dequeue takes an el out and frees it, but returns the string of the element
 in the queue */
struct qel *dequeue(struct queue *q)
{
    struct qel *temp;
    
	if(q == NULL || q->head == NULL)
		return NULL;
	else
	{
		//remove from list
		temp = q->head;
        
		q->head = q->head->next;
		
		if(--q->sz == 0)
			q->tail = NULL;
	}
	
	return temp;
}

/* dequeue an element in specific */
struct qel *dequeue_el(struct queue *q, struct qel *el)
{
    if(q == NULL || el == NULL)
        return NULL;
    
    struct qel *temp = q->head;
    
    /* validate the element is in the queue */
    while(temp != NULL && temp != el)
    {
        temp = temp->next;
    }
    
    if(temp == NULL)
    {
        printf("Error finding element to dequeue!\n");
        exit(1);
    }
    
    /* actually dequeue the element */
    
    /* make sure it isn't the first el */
    if(el->prev != NULL)
    {
        el->prev->next = el->next;
    }
    else /* if it was, adjust the head of the queue */
        q->head = el->next;
    
    /* make sure it isn't the tail */
    if(el->next != NULL)
    {
        el->next->prev = el->prev;
    }
    else /* if it was adjust the tail */
        q->tail = el->prev;
    
    q->sz--;
    
    return el;
}

/* gets the address for a given udp port to send to */
void getaddr(struct window *w)
{
    char portchar[8];
    struct addrinfo hints, *info;
    
    if(w == NULL)
    {
        printf("Error getting addr, window is null\n");
        exit(1);
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    sprintf(portchar, "%d", getportfromname(w->name));
    
    if(getaddrinfo(LOCALHOST, portchar, &hints, &info) != 0)
    {
        printf("Error getting address info on port: %s\n", portchar);
        exit(1);
    };
    
    w->addr = info->ai_addr;
    return;
}

/* call from a thread to bind to it's designated port */
int setupmyport(char name[])
{
    char portchar[8];
    int sock;
    struct addrinfo hints, *info;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    sprintf(portchar, "%d", getportfromname(name));
    
    if(getaddrinfo(LOCALHOST, portchar, &hints, &info) != 0)
    {
        printf("Error getting address info on port: %s\n", portchar);
        exit(1);
    };
    
    if ((sock = socket(info->ai_family, info->ai_socktype,
                            info->ai_protocol)) == -1)
    {
        printf("Error creating socket: %s\n", portchar);
        exit(1);
    }
    
    if (bind(sock, info->ai_addr, info->ai_addrlen) == -1)
	{
		printf("Error binding on socket: %s\n", portchar);
		exit(1);
	}
    
	fcntl(sock, F_SETFL, O_NONBLOCK);
    
    return sock;
}

/* get the whole window given a nodes name */
struct window *getwindowfromname(char name[])
{
    struct window *w = nodelist->head;
    
    while(w != NULL)
    {
        if(strcmp(w->name, name) == 0)
        {
            return w;
        }
        
        w = w->next;
    }
    
    printf("Error getting window!\n");
    exit(1);
}

/* get the address to send to for a node from it's name */
struct sockaddr *getaddrfromname(char name[])
{
    struct window *w = nodelist->head;
    
    while(w != NULL)
    {
        if(strcmp(w->name, name) == 0)
        {
            return w->addr;
        }
        
        w = w->next;
    }
    
    printf("Getting address failed!\n");
    exit(1);
}

/* get the socket from the name */
int getsockfromname(char name[])
{
    struct window *w = nodelist->head;
    
    while(w != NULL)
    {
        if(strcmp(w->name, name) == 0)
        {
            return w->socket;
        }
        
        w = w->next;
    }
    
    printf("Getting socket failed!\n");
    exit(1);
}

/* initialize some basic features of the program */
void initialize()
{
    nodelist = malloc(sizeof(struct list));
    srand(time(NULL));
    
    return;
}

/* send (or try to send) a packet to a destination */
void sendudp(char *src, char *msg, char *dest)
{
    int sock = getsockfromname(src);
    struct sockaddr *to = getaddrfromname(dest);
    /* run probability to drop packet */
    int rng = rand() % 100;
    if(rng >= DROPPROB)
    {
        /* send the packet, if it was less then it was dropped */
        sendto(sock, msg, strlen(msg), 0, to,
               sizeof(struct sockaddr_storage));
    }
    
    return;
}

/* retreive a queuelist from giving the src and dest */
struct queuelist *getql(char *src, char *dest)
{
    struct window *w = nodelist->head;
    
    while(w != NULL)
    {
        if(strcmp(w->name, src) == 0)
        {
            struct queuelist *ql = w->qlist;
            
            while(ql != NULL)
            {
                if(strcmp(ql->name, dest) == 0)
                {
                    return ql;
                }
                
                ql = ql->next;
            }
            
            printf("No node found for dest!\n");
            return NULL;
        }
        w = w->next;
    }
    
    printf("Node node found for source!\n");
    return NULL;
}

void fakeoutside(int first, int last, int acknum)
{
    if(last > first)
    {
        if(acknum < first || acknum > last)
            printf("[%d - %d] (%d) OUTSIDE!\n", first, last, acknum);
        else
            printf("[%d - %d] (%d) INSIDE!\n", first, last, acknum);
    }
    else
    {
        if(acknum < first && acknum > last)
            printf("[%d - %d] (%d) OUTSIDE!\n", first, last, acknum);
        else
            printf("[%d - %d] (%d) INSIDE!\n", first, last, acknum);
    }
}

/* temp main function */
int main()
{
    initialize();
    addnode("NodeA");
    addnode("NodeB");
    
    //printlist(nodelist);
    
    char message0[BUFSIZE], message1[BUFSIZE];
    
    /* LETS SEND ON B TO A */
    struct queuelist *ql = getql("NodeA", "NodeB");
    struct queue *q = ql->sendq;
    
    sprintf(message0, "%d`NodeA`NodeB`Message0\n", reqack(q));
    sprintf(message1, "%d`NodeA`NodeB`Message1\n", reqack(q));
    
    /* enqueue it and send it */
    enqueue(q, message1);
    enqueue(q, message0);
    sendudp("NodeA", message1, "NodeB");
    
    int i=0;
    
    while(i<100)
    {
        if(receivemsg("NodeA") > 0)
        {
            printf("\n\n\n\n\n\n\n\n");
            printlist(nodelist);
            printf("\n\n\n\n\n\n\n\n");
            
            i++;
        }
        
        if(receivemsg("NodeB") > 0)
        {
            receivemsg("NodeB");
            printf("\n\n\n\n\n\n\n\n");
            printlist(nodelist);
            printf("\n\n\n\n\n\n\n\n");
            
            i++;
        }
    }
    
    return 0;
}