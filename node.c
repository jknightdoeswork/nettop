//
//  node.c
//  
// TODO : FIX A BETTER UI BUT SELECTIVE REPEAT SEEMS ALRIGHT
// NO REAL WAY TO TEST GIVEN WITH THE CURR UI YOU CAN'T
// REALLY SEND MULTIPLE MESSAGES

#include "node.h"

struct list *nodelist;
int globalport = 3490;
FILE *file;

/* frees a packet and the message in the packet */
void freepacket(struct packet* p)
{
    free(p->msg);
    free(p);
    p = NULL;
    
    return;
}

/* frees an entire window (doing all the packets in it first) */
void freewindow(struct window *w)
{
    if(w == NULL)
        return;
    
    struct packet *p = w->head;
    while(p != NULL)
    {
        /* clean the whole queue, just using dequeue and resetting p
         to be the head since dequeue takes it off the head */
        freepacket(dequeue(w));
        p = w->head;
    }
    
    free(w);
    w = NULL;
    
    return;
}

/* frees a specific routing table entry, freeing all of it's windows
 before freeing itself */
void freeroutingtableentry(struct routing_table_entry *rte)
{
    if(rte == NULL)
        return;
    
    /* free all of it's top level objects */
    free(rte->name);
    free(rte->through);
    rte->next = NULL;
    rte->prev = NULL;
    
    /* call other function to go deeper to free each window */
    freewindow(rte->sendq);
    freewindow(rte->recvq);
    freewindow(rte->ackq);
    
    free(rte);
    rte = NULL;
    
    return;
}

/* free an entire list of routing tables */
void freeroutingtable(struct node *n)
{
    /* point our rte to the head of the routing table */
    struct routing_table_entry *rte = n->routing_table;
    struct routing_table_entry *temp;
    
    /* call this free on every routing table entry */
    while(rte != NULL)
    {
        /* point temp to rte */
        temp = rte;
        /* move rte to point to the next one */
        rte = rte->next;
        /* then free the original one still held in temp */
        freeroutingtableentry(temp);
    }
    
    return;
}

/* free everything in a node, going deeper to free every sub-component */
void freenode(struct node *n)
{
    if(n == NULL)
        return;
    
    free(n->name);
    
    /* set to null ? better safe than sorry */
    n->next = NULL;
    n->addr = NULL;
    n->prev = NULL;
    
    /* free the routing table which goes deeper on itself */
    freeroutingtable(n);
    
    free(n);
    n = NULL;
    
    return;
}

/* pass in a name, then free and delete that node */
void deletenode(struct node *n)
{
    if(n == NULL)
    {
        fprintf(stderr, "Fatal error\n");
        exit(1);
    }
    
    /* remove the node from being in the list */
    if(n->prev == NULL)
    {
        /* then n is the head */
        nodelist->head = n->next;
    }
    else
    {
        /* n is not the head so remove itself from the linked
         list in the forward direction */
        n->prev->next = n->next;
    }
    
    if(n->next == NULL)
    {
        /* then n is the tail */
        nodelist->tail = n->prev;
    }
    else
    {
        /* n is not the tail so it removes itself from the linked
         list in the reverse direction */
        n->next->prev = n->prev;
    }
    
    /* now it is gone from the list so free the node */
    freenode(n);
    
    return;
}

/* add a node entry to our global list of nodes */
struct node *addnode(char* name)
{
    /* add it to the set of all nodes */
    /* add it to every existing nodes list of queues */
    /* give it queues for all existing nodes */
    /* append handles it all because append is magic */
    
    struct node *n = append(nodelist, name);
    fprintf(stderr, "spawning thread\n");
    
    return n;
}

/* given a node, and a name for our destination entry, add
 an RTE for the destination so our node knows about it */
struct routing_table_entry *rtappend(struct node *w, char* name,
		char* through, int delay, int drop, int weight)
{
    struct routing_table_entry *tmp;
    struct routing_table_entry *new;
    
    tmp = w->routing_table;
    
    /* construct the rt entry */    
    new = malloc(sizeof(struct routing_table_entry));
    new->name = malloc(BUFSIZE * sizeof(char));
    memset(new->name, 0, BUFSIZE);
    new->through = malloc(BUFSIZE * sizeof(char));
    memset(new->through, 0, BUFSIZE);
    new->delay = delay;
    new->drop = drop;
    new->weight = weight;
    strcpy(new->name, name);
    strcpy(new->through, through);

    new->sendq = malloc(sizeof(struct window));
    new->recvq = malloc(sizeof(struct window));
    new->ackq = malloc(sizeof(struct window));
    
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
    
    new->ackq->head = NULL;
    new->ackq->tail = NULL;
    new->ackq->sz = 0;
    new->ackq->max = INT_MAX;
    new->ackq->first = 0;
    new->ackq->last = INT_MAX;
    new->ackq->cur = 0;
    
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
                fprintf(stderr, "RTE already exists for node: %s\n", name);
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

/* Adds an edge to node a and node b's routing tables of delay delay */
void addedge(char* nodeaname, char* nodebname, int delay, int drop)
{

    /* calc weight and get both nodes */
    int weight = (100.0/(100.0-drop))*delay;
    struct node* nodea = getnodefromname(nodeaname);
    struct node* nodeb = getnodefromname(nodebname);
    
    /* if either don't exist, do not proceed */
    if(nodea == NULL)
    {
        printf("[%s]Node does not exist\n", nodeaname);
        return;
    }
    if(nodeb == NULL)
    {
        printf("[%s]Node does not exist\n", nodebname);
        return;
    }
    
    /* only proceed if both nodes exist */
    rtappend(nodea, nodebname, nodebname, delay, drop, weight);
    rtappend(nodeb, nodeaname, nodeaname, delay, drop, weight);
}

/* given our (global) list of nodes, append a new node */
/* todo-remove it from handling RTE's */
struct node *append(struct list *l, char* name)
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
    if (w == NULL) {
        fprintf(stderr, "OUT OF MEMORY\n");
        exit(1);
    }
    w->routing_table = NULL;
    w->name = malloc(BUFSIZE * sizeof(char));
    memset(w->name, 0, BUFSIZE);
    strcpy(w->name, name);
    w->port = globalport++;
    w->next = NULL;
    w->dead=0;
    w->logfile = createlogfile(name);
    
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

    spawnthread(w);
    
    return w;
}

/* given a node name, get it's corresponding port */
int getportfromname(char* name)
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
            printf("- RTE [%s] W:%d THROUGH:[%s]\n", ql->name, ql->weight, ql->through);
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
    
    e->msg = malloc(BUFSIZE * sizeof(char));
    memset(e->msg, 0, BUFSIZE);
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
        fprintf(stderr, "Error finding element to dequeue!\n");
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
int setupmyport(char* name)
{
    char portchar[8];
    memset(portchar, 0, 8);
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
struct node *getnodefromname(char* name)
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
struct sockaddr *getaddrfromname(char* name)
{
    struct node* w = getnodefromname(name);
    if (w == NULL)
        return NULL;
    return w->addr;
}

/* get the socket from the name */
int getsockfromname(char* name)
{
    struct node* w = getnodefromname(name);
    if (w == NULL)
        return -1;
    return w->socket;
}

/* initialize some basic features of the program */
void initialize()
{
    //createlogdir();
    nodelist = malloc(sizeof(struct list));
    if (nodelist == NULL) {
        fprintf(stderr, "OUT OF MEMORY\n");
    }
    memset(nodelist, 0, sizeof(struct list));
    srand(time(NULL));
    
    return;
}

/* send (or try to send) a packet to a destination */
void sendudp(char *src, char *msg, char *dest)
{
    int sock = getsockfromname(src);
    
    /* get the entry for what node it goes through and
     send to intermediate node */
    struct routing_table_entry *rte, *r;
    rte = getroutingtableentry(getnodefromname(src), dest);
    
    /* get the node we go through */
    r = getroutingtableentry(getnodefromname(src), rte->through);
    
    if(rte == NULL || r == NULL)
    {
        fprintf(stderr, "Error, no connection to [%s] found!\n", dest);
        return;
    }
    
    struct sockaddr *to = getaddrfromname(rte->through);
    
    /* run probability to drop packet */
    int rng = rand() % 100;
    if(rng >= r->drop)
    {
        /* send the packet, if it was less then it was dropped */
        sendto(sock, msg, strlen(msg), 0, to,
               sizeof(struct sockaddr_storage));
    }
    else
    {
	    if(msg[0] != '&')
		printf("Dropping message [%s]\n", msg);
    }
    
    return;
}

/* given a source node, get it's routing table entry for the dest */
struct routing_table_entry *getroutingtableentry(struct node *src, char *dest)
{
    struct routing_table_entry *iter = src->routing_table;
    
    /* loop on everything in our routing table */
    while (iter != NULL)
    {
        if (strcmp(iter->name, dest) == 0)
            return iter;
	
        iter = iter->next;
    }
    return NULL;
}

/* our ackq is anything we are blindly forwarding (acks, nacks,
 and messages not meant for us). if it's delay is up, send it
 and forget about it since i am not responsible for reliable
 delivery (handled elsewhere) */
void checkackdelays(struct node *n, struct routing_table_entry *rte)
{
    if(rte == NULL)
		return;

	time_t curtime = time(NULL);

    /* get appropriate entries */
	struct window *w = rte->ackq;
    
    if(w == NULL)
    {
        fprintf(stderr, "Window is null\n");
        exit(1);
    }
    
	struct packet *p = w->head;
	struct routing_table_entry *r = getroutingtableentry(n, rte->through);

	if(r == NULL)
		return;

    /* loop on all packets and send them if their delay is up */
	while(p != NULL)
	{
		if(p->enqT < curtime - r->delay)
		{
			sendudp(n->name, p->msg, r->name);
			struct packet *t = p;
			p = p->next;
			freepacket(dequeue_el(w, t));
		}
		else
			p = p->next;
	}
	
	return;
}

/* check if messages need to be sent from delay, or have timed out.
 either way it needs to be sent */
void checkmsgdelays(struct node *n, struct routing_table_entry *rte)
{
	if(rte == NULL)
		return;

	time_t curtime = time(NULL);

    /* get appropriate entries */
	struct window *w = rte->sendq;
    
    if(w == NULL)
    {
        fprintf(stderr, "Window is null\n");
        exit(1);
    }
    
	struct packet *p = w->head;
	struct routing_table_entry *r = getroutingtableentry(n, rte->through);

	if(r == NULL)
		return;

	while(p != NULL)
	{
		if(p->sent)
		{
			/* check timeouts */
			if(p->enqT < curtime - TIMEOUT)
			{
				/* only mark it as having been enqueued just now
				 * and say it hasn't been sent before so it knows
				 * to check for it's delay */
				printf("[%s]Packet timed out\n\n", p->msg);

				p->enqT = curtime;
				p->sent = 0;
			}
		}
		else
		{	/* check delays */
			if(p->enqT < curtime - r->delay && !p->received)
			{
				/* send the packet because it's delay is up */
				printf("[%s]Packet sent\n\n", p->msg);

				p->enqT = curtime;
				p->sent = 1;

				sendudp(n->name, p->msg, r->name);
			}
		}

		p = p->next;
	}
	
	return;
}

/* check all of my packets for if they should be sent based off of their delay
 * or if they should be sent based off of the ack timing out (no confirmation) */
void checktimeouts(struct node *n)
{
	struct routing_table_entry *rte = n->routing_table;
	
	/* check all RTEs for outstanding packets, either delay, or timeout */
	while(rte != NULL)
	{
		checkmsgdelays(n, rte);
		checkackdelays(n, rte);

		rte = rte->next;
	}
	
	return;
}

/* main accept loop for every thread */
void *threadloop(void *arg)
{
	char *name = (char *)arg;
	
	struct node *me = getnodefromname(name);
	time_t laststep = -1;
	
	if(me == NULL)
	{
		fprintf(stderr, "Fatal error!\n");
		exit(1);
	}
	
    /* main accept loop */
	while(!me->dead)
	{
		laststep = dvr_step(me, laststep);
		checktimeouts(me);
		receivemsg(me->name);
	}
    
    /* delete itself then exit */
    deletenode(me);

	pthread_exit(NULL);	
	return NULL;
}

/* given a node create a thread for it */
void spawnthread(struct node *n)
{
	if(pthread_create(&(n->thread), NULL, (void*)&threadloop, n->name))
	{
		fprintf(stderr, "Error creating threads.\n");
		exit(1);
	}
	
	return;
}

/* signal a thread to stop execution */
void sigkillthread(char* name)
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

/* prompt the user input to send a message between nodes */
void usersendmessage()
{
	char src[BUFSIZE], dest[BUFSIZE], msg[BUFSIZE];
	char send[BUFSIZE];
    char *saveptr;
    struct node *nsrc;
    
    /* prompt user for input */
	printf("Enter the name of the source node: ");
	fgets(src, BUFSIZE, stdin);
    /* remove trailing newline */
    strtok_r(src, "\n", &saveptr);
    
    if((nsrc = getnodefromname(src)) == NULL)
    {
        printf("[%s]Node does not exist\n", src);
        return;
    }
    
	printf("Enter the name of the destination node: ");
	fgets(dest, BUFSIZE, stdin);
    strtok_r(dest, "\n", &saveptr);
    
    if(getnodefromname(dest) == NULL)
    {
        printf("[%s]Node does not exist", dest);
        return;
    }
    
    /* see if there is an entry for this path, if there isn't, then
     no connection exists so we can't send a message */
    struct routing_table_entry *rte;
    rte = getroutingtableentry(nsrc, dest);
    
    if(rte == NULL)
    {
        printf("[%s]-[%s]No path between nodes exists\n", src, dest);
        return;
    }
    
	printf("Enter the message to send: ");
	fgets(msg, BUFSIZE, stdin);
    
    /* remove trailing newline from the input buffers */
    strtok_r(msg, "\n", &saveptr);
	
    /* ship off 3 versions */
	sprintf(send, "%d`%s`%s`%s", reqack(rte->sendq), src, dest, msg);
	enqueue(rte->sendq, send);
    sprintf(send, "%d`%s`%s`%s", reqack(rte->sendq), src, dest, msg);
	enqueue(rte->sendq, send);
    sprintf(send, "%d`%s`%s`%s", reqack(rte->sendq), src, dest, msg);
	enqueue(rte->sendq, send);
}

/* prompt the user input to add a node to the topology */
void useraddnode()
{
    char name[BUFSIZE];
    char *saveptr;
    printf("Enter the name of the node\n");
    fgets(name, BUFSIZE, stdin);
    
    /* parse off the newline */
    strtok_r(name, "\n", &saveptr);
    
    if(getnodefromname(name) != NULL)
    {
        printf("[%s]Node already exists\n", name);
        return;
    }
    
    addnode(name);
    
    printf("[%s]Node added\n", name);
    return;
}

// TODO: WE NEED A WAY TO REMOVE NODES SAFELY
/* prompt the users input to remove a node in the topology */
void userremovenode()
{
    char name[BUFSIZE];
    char *saveptr;
    printf("Enter the name of the node to remove\n");
    fgets(name, BUFSIZE, stdin);
    
    /* parse off the newline */
    strtok_r(name, "\n", &saveptr);
    
    if(getnodefromname(name) == NULL)
    {
        printf("[%s]Node does not exist\n", name);
        return;
    }
    
    sigkillthread(name);
    
    printf("[%s]Node deleted\n", name);
    
    return;
}

//TODO : Decide to create the nodes if they don't exist, or only
//accept edges on existing nodes
/* prompt the users input to add an edge to the topology */
void useraddedge()
{
    char name1[BUFSIZE], name2[BUFSIZE], other[BUFSIZE];
    int delay, drop;
    char *saveptr;
    
    struct node *nsrc;
    
    /* prompt for input */
    printf("Enter the name of the first node on the edge\n");
    fgets(name1, BUFSIZE, stdin);
    /* token off the newline */
    strtok_r(name1, "\n", &saveptr);
    
    if((nsrc = getnodefromname(name1)) == NULL)
    {
        printf("[%s]Node does not exist\n", name1);
        return;
    }
    
    printf("Enter the name of the second node on the edge\n");
    fgets(name2, BUFSIZE, stdin);
    strtok_r(name2, "\n", &saveptr);
    
    if(getnodefromname(name2) == NULL)
    {
        printf("[%s]Node does not exist\n", name2);
        return;
    }
    
    struct routing_table_entry *rte;
    rte = getroutingtableentry(nsrc, name2);
    
    if(rte != NULL)
    {
        printf("[%s]-[%s]Edge already exists\n", name1, name2);
        return;
    }
    
    /* get delay */
    printf("Enter the delay over the edge (integer in s)\n");
    fgets(other, BUFSIZE, stdin);
    delay = atoi(other);
    
    if(delay < 0)
    {
        printf("[%s]-[%s]Edge delay must be >= 0\n", name1, name2);
        return;
    }
    
    memset(other, 0, BUFSIZE);
    
    /* get drop prob */
    printf("Enter the drop probability (integer from 0-100)\n");
    fgets(other, BUFSIZE, stdin);
    drop = atoi(other);
    
    if(drop<0 || drop>100)
    {
        printf("[%s]-[%s]Edge drop probability must be between 0-100\n",
               name1, name2);
        return;
    }
    
    addedge(name1, name2, delay, drop);
    printf("[%s]-[%s]Edge added : (-delay:%ds -drop:%d%%)\n",
           name1, name2, delay, drop);
    
    return;
}

/* display the menu to the user */
void displaymenu()
{
	printf("\n\n----------------------\n");
	printf("Enter 0 to exit\n");
	printf("Enter 1 to send a message\n");
	printf("Enter 2 to add a node\n");
	printf("Enter 3 to remove a node\n");
	printf("Enter 4 to add an edge\n");
	printf("Enter 5 to modify an edge\n");
	printf("Enter 6 to print the routing table\n");
	printf("----------------------\n\n");
	
	return;
}

/* main accept loop for user input */
void userloop()
{
	char c, ch;
	
	printf("-------------INUSERLOOP------------\n");
	
	while(c != '0')
	{
		displaymenu();
		
		c = getchar();
		
		/* consume all the extra input */
		while ((ch = getchar() != '\n') && (ch != EOF));
		
        /* consider all cases */
		switch(c)
		{
			case '0':
				return;
				break;
			case '1':
				usersendmessage();
				break;
			case '2':
				useraddnode();
				break;
			case '3':
				userremovenode();
				break;
			case '4':
				useraddedge();
				break;
			case '5':
				//usermodedge();
				break;
			case '6':
				printlist(nodelist);
				break;
			default:
				break;
		}
	}
	
	return;
}

/* temp main function */
int main()
{
	/* initialize and create the nodes */
    initialize();
    createlogdir();
    
    file = fopen("readoutput", "w");
    parse_file("test.top");
    
    userloop();
    
    fclose(file);
    
    sigkillall();
    pthread_exit(NULL);
    
    return 0;
}

