//
//  node.c
//  

#include "node.h"

struct list *nodelist;
int globalport = 3490;

/* add a node entry to our global list of nodes */
struct node *addnode(char name[])
{
    /* add it to the set of all nodes */
    /* add it to every existing nodes list of queues */
    /* give it queues for all existing nodes */
    /* append handles it all because append is magic */
    
    struct node *n = append(nodelist, name);
    
    spawnthread(n);
    
    return n;
}

/* given a node, and a name for our destination entry, add
 an RTE for the destination so our node knows about it */
struct routing_table_entry *rtappend(struct node *w, char name[], char through[], int weight)
{
    struct routing_table_entry *tmp;
    struct routing_table_entry *new;
    
    tmp = w->routing_table;
    
    /* construct the rt entry */    
    new = malloc(sizeof(struct routing_table_entry));
    new->weight = weight;
    strcpy(new->name, name);
    strcpy(new->through, through);

    new->sendq = malloc(sizeof(struct window));
    new->recvq = malloc(sizeof(struct window));
    new->delayq = malloc(sizeof(struct window));
    
    /* initialize the queues */
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
    
    new->delayq->head = NULL;
    new->delayq->tail = NULL;
    new->delayq->sz = 0;
    new->delayq->max = INT_MAX;
    new->delayq->first = 0;
    new->delayq->last = INT_MAX;
    new->delayq->cur = 0;
    
    /* do the linked list insertion */
    new->next = NULL;
    if(tmp == NULL) /* no entries */
    {
        w->routing_table = new;
            
        new->prev = NULL;
    }
    else
    {
        while(1)
        {
            if(strcmp(tmp->name, name) == 0)
            {
                printf("RTE already exists for node: %s\n", name);
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

/* Adds an edge to node a and node b's routing tables of weight weight */
void addedge(char* nodeaname, char* nodebname, int weight)
{
    struct node* nodea = getnodefromname(nodeaname);
    rtappend(nodea, nodebname, nodebname, weight);
    
    struct node* nodeb = getnodefromname(nodebname);
    rtappend(nodeb, nodeaname, nodeaname, weight);
}

/* given our (global) list of nodes, append a new node */
/* todo-remove it from handling RTE's */
struct node *append(struct list *l, char name[])
{
    /* fail if l is null */
    if(l == NULL)
    {
        printf("List is NULL!\n");
        return NULL;
    }
    
    /* check if name already exists */ 
    /* otherwise there is an el, append to the end */
    struct node *nodecheck = getnodefromname(name);
    if(nodecheck != NULL) 
    {   
        return nodecheck;
    }

    struct node *w = malloc(sizeof(struct node));
    strcpy(w->name, name);
    w->port = globalport++;
    w->next = NULL;
    w->dead=0;
    
    if(l->tail != NULL)
        l->tail->next = w;
    
    w->prev = l->tail;
    l->tail = w;
    
    /* then it is also the head */
    if((l->sz)++ == 0)
    {
        l->head = w;
    }
    
    w->socket = setupmyport(name);
     
    getaddr(w); /* port to make sense */
    
    return w;
}

/* given a node name, get it's corresponding port */
int getportfromname(char name[])
{
    struct node *w = getnodefromname(name);
    if (w == NULL)
        return -1;
    return w->port;
}

/* give them an ack number to use, and increment so we
 don't reuse. you would only call on sending. receiving
 acks are handled when you actually receive an ack (the
 ack number goes up */
int reqack(struct window *q)
{
    int tmp = q->cur;
    q->cur = plusone(q->cur);
    return tmp;
}

/* prints all the packets currently buffered in the
 window and prints out the window's current (true) size*/
void printwindow(struct window *q)
{
    if(q == NULL)
        return;
    
    struct packet *el = q->head;
    
    while(el != NULL)
    {
        printf("[%d]-%s\n", el->received, el->msg);
        el = el->next;
    }
    
    printf("Sz: %d\n", q->sz);
    
    return;
}

/* print out all nodes in a list, as well as all RTE's
 for each node */
void printlist(struct list *l)
{
    if(l == NULL)
        return;
    
    struct node *w = l->head;
    
    while(w != NULL)
    {
        printf("Node [%s]\n", w->name);
        struct routing_table_entry *ql = w->routing_table;
        
        while(ql != NULL)
        {
            printf("- RTE [%s]\n", ql->name);
            if(DEBUGWINDOWS)
            {
                printf("--------------------\nSendq sz [%d - %d]:\n",
                       ql->sendq->first, ql->sendq->last);
                printwindow(ql->sendq);
                printf("--------------------\nRecvq sz [%d - %d]:\n",
                       ql->recvq->first, ql->recvq->last);
                printwindow(ql->recvq);
                printf("--------------------\n");
            }
            
            ql = ql->next;
        }
        w = w->next;
    }
    
    return;
}

/* returns true if a comes before b in the sliding window */
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

/* buffer a message into the window passed in. enqueue
 it in order based on acknumber */
int enqueue(struct window *q, char* msg)
{
    struct packet *el;
    
    /* force the enqueue to be in order */
    
	if(q->sz >= q->max)
		return -1; // queue is full
	
	struct packet *e = malloc(sizeof(struct packet));
	
	if(e == NULL)
		return -1;
    
	strcpy(e->msg, msg);
    
    struct msgtok *tok = tokenmsg(msg);
    
    e->acknum = tok->acknum;
    e->received = 0;
    e->sent = 0;
    
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

/* remove and return the "head" packet in the window */
struct packet *dequeue(struct window *q)
{
    struct packet *temp;
    
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

/* remove and return a specific packet from the window */
/* not currently used */
struct packet *dequeue_el(struct window *q, struct packet *el)
{
    if(q == NULL || el == NULL)
        return NULL;
    
    struct packet *temp = q->head;
    
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

/* sets up the address a node will be using */
void getaddr(struct node *w)
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
    // TODO FIX MEMORY LEAK: call freeaddrinfo 
    w->addr = info->ai_addr;
    return;
}

/* create the socket for a node to send from */
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

/* get the node given it's name */
struct node *getnodefromname(char name[])
{
    struct node *w = nodelist->head;
    
    while(w != NULL)
    {
        if(strcmp(w->name, name) == 0)
        {
            return w;
        }
        
        w = w->next;
    }
    
    return NULL;
}

/* get the address to send to for a node from it's name */
struct sockaddr *getaddrfromname(char name[])
{
    struct node* w = getnodefromname(name);
    if (w == NULL)
        return NULL;
    return w->addr;
}

/* get the socket from the name */
int getsockfromname(char name[])
{
    struct node* w = getnodefromname(name);
    if (w == NULL)
        return -1;
    return w->socket;
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
    
    /* get the entry for what node it goes through and
     send to intermediate node */
    struct routing_table_entry *rte;
    rte = getroutingtableentry(getnodefromname(src), dest);
    
    if(rte == NULL)
    {
        fprintf(stderr, "Error, no connection to [%s] found!\n", dest);
        return;
    }
    
    struct sockaddr *to = getaddrfromname(rte->through);
    
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

struct routing_table_entry *getroutingtableentry(struct node *src, char *dest)
{
    struct routing_table_entry *iter = src->routing_table;
    while (iter != NULL)
    {
        if (strcmp(iter->name, dest) == 0)
            return iter;
    }
    return NULL;
}

/* check all of my packets for if they should be sent based off of their delay
 * or if they should be sent based off of the ack timing out (no confirmation) */
void checktimeouts(struct node *n)
{
    int doacks = 0;
	struct routing_table_entry *rte = n->routing_table;
    
    time_t curtime = time(NULL);
	
	/* check all RTEs for outstanding packets, either delay, or timeout */
	while(rte != NULL)
	{
		struct packet *p = rte->sendq->head;
		
		/* check all packets to see if outstanding, ensure
         we enter this once to check the ackq */
		while(p != NULL || !doacks)
		{
            /* once p is null, we've checked all packets, so
             switch to start doing acks */
            if(p == NULL && !doacks)
            {
                doacks = 1;
                p = rte->delayq->head;
                
                /* if there were no packets in the delayq, just
                 immediately break */
                if(p == NULL)
                    break;
            }
            
			/* check if the time is up, and if it hasn't already
			 * been confirmed received, and if this is the first
			 time sending it, ie send based on delay */
			if(p->enqT < curtime - rte->weight &&
				!(p->received) && !(p->sent))
			{
				/* re-send because timeout */
				printf("DELAY EXPIRED\n");
				/* mark the packet as having been sent */
				p->sent = 1;
				/* send it */
				sendudp(n->name, p->msg, rte->name);
				/* reset the timer */
				p->enqT = curtime;
                
                /* if we are just checking delays, once we've sent
                 it, since no reliable transfer, just remove it
                 from our queue */
                if(doacks)
                {
                    struct packet *t = p;
                    p = p->next;
                    /* free and dequeue the packet since we no
                     longer need it */
                    free(dequeue_el(rte->delayq, t));
                }
			}
			/* if it has been sent, then check the timeout on it
			 * and resend if timeout expires */
			else if(p->sent && p->enqT < curtime - TIMEOUT)
			{
				printf("TIMEOUT OCCURRED\n");
				p->sent = 0;
				p->enqT = curtime;
			}
            
            /* if we are only checking delays, then it is handled
             when we dequeue and free the packet so don't do anything
             if we aren't doing packets we need to move */
            if(!doacks)
                p = p->next;
		}
		rte = rte->next;
	}
	
	return;
}

/* main accept loop for every thread */
void *threadloop(void *arg)
{
	char *name = (char *)arg;
	
	struct node *me = getnodefromname(name);
	
	if(me == NULL)
	{
		fprintf(stderr, "Fatal error!\n");
		exit(1);
	}
	
	while(!me->dead)
	{
		checktimeouts(me);
		receivemsg(me->name);
	}
	
	return NULL;
}

/* given a node create a thread for it */
void spawnthread(struct node *n)
{
	if(pthread_create(&(n->thread), NULL, threadloop, n->name))
	{
		fprintf(stderr, "Error creating threads.\n");
		exit(1);
	}
	
	return;
}

/* signal a thread to stop execution */
void sigkillthread(char name[])
{
	struct node *n = getnodefromname(name);
	
	/* return error if node didn't exist */
	if(n == NULL)
		return;
	
	n->dead = 1;
	
	return;
}

/* kill all running threads */
void sigkillall()
{
	struct node *n = nodelist->head;
	
	/* loop through all nodes and kill their thread */
	while(n!=NULL)
	{
		n->dead = 1;
		
		n = n->next;
	}
	
	return;
}

void displaymenu()
{
	printf("\n\n\nEnter 0 to exit\n");
	printf("Enter 1 to add a node\n");
	printf("Enter 2 to remove a node\n");
	printf("Enter 3 to add an edge\n");
	printf("Enter 4 to modify an edge\n");
	printf("Enter 5\n");
	
	return;
}

void usersendmessage()
{
	char src[BUFSIZE], dest[BUFSIZE], msg[BUFSIZE];
	printf("Enter the name of the source node: ");
	scanf("%s", src);
	printf("Enter the name of the destination node: ");
	scanf("%s", dest);
	printf("Enter the message to send: ");
	scanf("%s", msg);
}

void userloop()
{
	char c;
	
	while(c != '0')
	{
		c = getchar();
		getchar();
	}
	
	return;
}

/* temp main function */
int main()
{
	/* initialize and create the nodes */
    initialize();
    addnode("NodeA");
    addnode("NodeB");
    addnode("NodeC");
    
    addedge("NodeA", "NodeB", 3);
    
    printf("Control in main!\n");
    
    char message0[BUFSIZE], message1[BUFSIZE];
    
    /* LETS SEND ON B TO A */
    struct routing_table_entry *ql = getroutingtableentry(getnodefromname("NodeA"), "NodeB");
    struct window *q = ql->sendq;
    
    sprintf(message0, "%d`NodeA`NodeB`Message0\n", reqack(q));
    sprintf(message1, "%d`NodeA`NodeB`Message1\n", reqack(q));
    
    /* enqueue it and send it */
    enqueue(q, message1);
    enqueue(q, message0);
    
    /* loop for user control */
    userloop();
    
    /* kill all threads and shut down */
    sigkillall();
    printf("Killed all threads\n");
    pthread_exit(NULL);
    
    //printlist(nodelist);
    
    
    sendudp("NodeA", message1, "NodeB");
    
    int i=0;
    
    while(i<100)
    {/*
        if(receivemsg("NodeA") > 0)
        {
            printf("\n\n\n\n\n\n\n\n");
            printlist(nodelist);
            printf("\n\n\n\n\n\n\n\n");
            
            i++;
        }
        
        if(receivemsg("NodeB") > 0)
        {
            printf("\n\n\n\n\n\n\n\n");
            printlist(nodelist);
            printf("\n\n\n\n\n\n\n\n");
            
            i++;
        }*/
        i++;
    }
    
    
    
    pthread_exit(NULL);
    
    return 0;
}
