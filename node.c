#include "node.h"

struct list *nodelist;
int globalport = 3490;

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
    
    free(rte);
    rte = NULL;
    
    return;
}

/* free an entire list of routing tables */
void freeroutingtables(struct node *n)
{
    /* point our rte to the head of the routing table */
    struct routing_table_entry *rte = n->routing_table;
    struct routing_table_entry *temp;
    
    int once = 0;
    
    /* call this free on every routing table entry */
    while(rte != NULL)
    {
        /* point temp to rte */
        temp = rte;
        /* move rte to point to the next one */
        rte = rte->next;
        /* then free the original one still held in temp */
        freeroutingtableentry(temp);
	
	/* do the routing table first, then the connected edges */
	if(rte == NULL && !once)
	{
		rte = n->connected_edges;
		once = 1;
	}
    }
    
    return;
}

/* deletes a nodes windowlist */
void deletewindowlist(struct node *n, struct windowlist *wl)
{
	if(n->windows == wl)
	{
		n->windows = wl->next;
	}
	if(wl->next != NULL)
	{
		wl->next->prev = wl->prev;
	}
	if(wl->prev != NULL)
	{
		wl->prev->next = wl->next;
	}
	
	freewindowlist(wl);
	wl = NULL;
	
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
    freeroutingtables(n);
    
    struct windowlist *wl = n->windows;
    
    while(wl != NULL)
    {
	    deletewindowlist(n, wl);
	    
	    wl = wl->next;
    }
    
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
    
    struct node *loopnode = nodelist->head;
    
    while(loopnode != NULL)
    {
	   struct routing_table_entry *rte;
	   rte = getroutingtableentry(loopnode, n->name, 1);
	   
	   /* if the master rte exists, delete it */
	   if(rte != NULL)
	   {
		deleterte(loopnode, rte, 1);
	   }
	  
	  loopnode = loopnode->next;
    }
    
    sleep(dvr_reset_interval+1);
    loopnode = nodelist->head;
    
    while(loopnode != NULL)
    {
	struct windowlist *wl = getwindowlist(loopnode, n->name);
	if(wl != NULL)
	{
		deletewindowlist(loopnode, wl);
	}
	
	loopnode = loopnode->next;
    }
    /* now it is gone from the list so free the node */
    freenode(n);
    
    return;
}

/* delete a nodes routing table entry */
void deleterte(struct node *n, struct routing_table_entry *rte, int master)
{
	if(n == NULL)
	{
		fprintf(stderr, "Node was null while trying to delete rte\n");
		return;
	}
	
	if(rte == NULL)
	{
		fprintf(stderr, "Error deleting rte: NULL\n");
		return;
	}
	
	/* check if first item */
	if(master)
	{
		if(n->connected_edges == rte)
			n->connected_edges = rte->next;
	}
	else
	{
		if(n->routing_table == rte)
			n->routing_table = rte->next;
	}
	
	/* adjust it's neighbours pointers to exclude it */
	if(rte->prev != NULL)
		rte->prev->next = rte->next;
	
	if(rte->next != NULL)
		rte->next->prev = rte->prev;
	
	freeroutingtableentry(rte);
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
    
    return n;
}

/** window list manipulation **/
struct windowlist* getwindowlist(struct node* w, char* name)
{
    if (w == NULL) {
        fprintf(stderr, "createwindowlist got null w\n");
        return NULL;
    }
    if (name == NULL) {
        fprintf(stderr, "createwindowlist got null name\n");
        return NULL;
    }

    /* if w->window list has an entry for name in it return */
    struct windowlist* tmp = w->windows;
    while(tmp != NULL)
    {
        if (strcmp(tmp->name, name) == 0)
            return tmp;
        tmp = tmp->next;
    }
    return NULL;
}

void freewindowlist(struct windowlist* w)
{
    freewindow(w->sendq);
    freewindow(w->recvq);
    freewindow(w->ackq);
    free(w->name);
    free(w);
    w = NULL;
}

void createwindowlist(struct node* w, char* name)
{
    if (w == NULL) {
        fprintf(stderr, "createwindowlist got null w\n");
        return;
    }
    if (name == NULL) {
        fprintf(stderr, "createwindowlist got null name\n");
        return;
    }

    /* if w->window list has an entry for name in it return*/
    struct windowlist* tmp = w->windows;
    while(tmp != NULL)
    {
        if (strcmp(tmp->name, name) == 0)
            return;
        if (tmp->next == NULL)
            break;
        tmp = tmp->next;
    }
    
    /* else create a w->windowlist entry for name */

    struct windowlist *new = malloc(sizeof(struct windowlist));
    memset(new, 0, sizeof(struct windowlist));
    new->next = NULL;
    new->prev = NULL;
    new->name = malloc(BUFSIZE * sizeof(char));
    memset(new->name, 0, BUFSIZE * sizeof(char));
    strcpy(new->name, name);
    new->sendq = malloc(sizeof(struct window));
    new->recvq = malloc(sizeof(struct window));
    new->ackq  = malloc(sizeof(struct window));
    
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
    
    if (tmp == NULL) // window list is empty
    {
        w->windows = new;
    }
    else
    {
        tmp->next = new;
        new->prev = tmp;
    }
}

/* given a node, and a name for our destination entry, add
 an RTE for the destination so our node knows about it */
struct routing_table_entry *rtappend(struct node *w, char* name,
		char* through, int delay, int drop, float weight, int neighbourtable)
{
    struct routing_table_entry *tmp;
    struct routing_table_entry *new;
    
    tmp = neighbourtable ? w->connected_edges : w->routing_table;
    
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

    /* create the window list */
    if (getwindowlist(w, name) == NULL)
        createwindowlist(w, name);
        
    /* do the linked list insertion */
    new->next = NULL;
    if(tmp == NULL) /* no entries */
    {
        if (neighbourtable)
            w->connected_edges = new;
        else
            w->routing_table = new;
        new->prev = NULL;
    }
    else
    {
        while(1)
        {
            if(strcmp(tmp->name, name) == 0)
            {
				/* suppress this output because it is perfectly acceptable
				 * to already exist, but can be useful for debugging */
                /*fprintf(stderr, "RTE already exists for node: %s\n", name);*/
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
    float weight = (100.0/(100.0-drop))*delay;
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
    rtappend(nodea, nodebname, nodebname, delay, drop, weight, 1);
    rtappend(nodeb, nodeaname, nodeaname, delay, drop, weight, 1);
    reset_dvr(nodea);
    reset_dvr(nodeb);
}

/* given our (global) list of nodes, append a new node */
/* todo-remove it from handling RTE's */
struct node *append(struct list *l, char* name)
{
	char lognames[BUFSIZE];
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
    w->connected_edges = NULL;
    w->windows = NULL;

    w->name = malloc(BUFSIZE * sizeof(char));
    memset(w->name, 0, BUFSIZE);
    strcpy(w->name, name);
    w->port = globalport++;
    w->next = NULL;
    w->dead=0;

	memset(lognames, 0, sizeof(lognames));
	sprintf(lognames, "%s-dvr.log", name);
    w->logfile = createlogfile(lognames);

	memset(lognames, 0, sizeof(lognames));
	sprintf(lognames, "%s-sr.log", name);
    w->srlog = createlogfile(lognames);
    
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
		exit(1);
    }
    memset(nodelist, 0, sizeof(struct list));
    srand(time(NULL));
    
    return;
}

/* send (or try to send) a packet to a destination */
void sendudp(char *src, char *msg, char *dest)
{
	struct node *n = getnodefromname(src);

	if(n == NULL)
	{
		fprintf(stderr, "Source node no longer exists\n");
		return;
	}

    int sock = getsockfromname(src);
    
    /* get the entry for what node it goes through and
     send to intermediate node */
    struct routing_table_entry *rte, *r;
    rte = getroutingtableentry(n, dest, 0);

	struct msgtok *tok = tokenmsg(msg);

    if (rte == NULL) {
        fprintf(n->srlog, "Error, no connection from [%s] to [%s] found for message [%d : %s]!\n",
				src, dest, tok->acknum, tok->pay);
		freetok(tok);
        return;
    }
    
    /* get the node we go through */
    r = getroutingtableentry(getnodefromname(src), rte->through, 0);
    
    if(rte == NULL || r == NULL)
    {
        fprintf(n->srlog, "Error, no connection from [%s] to [%s] found for message [%d : %s]!\n",
				src, dest, tok->acknum, tok->pay);
		freetok(tok);
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
		fprintf(n->srlog, "[%s]Dropping message [%d : %s] on it's way from [%s] to [%s]\n",
			   src, tok->acknum, tok->pay, tok->src, tok->dest);
    }
    
	freetok(tok);
    return;
}

/* given a source node, get it's routing table entry for the dest */
struct routing_table_entry *getroutingtableentry(struct node *src, char *dest, int master)
{
    struct routing_table_entry *iter;
    
    if(master)
	iter = src->connected_edges;
    else
	iter = src->routing_table;
    
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
void checkackdelays(struct node *n, struct windowlist *wl)
{
    struct routing_table_entry *rte = getroutingtableentry(n, wl->name, 0);
	if(rte == NULL)
		return;

	time_t curtime = time(NULL);

    /* get appropriate entries */
	struct window *w = wl->ackq;
    
    if(w == NULL)
    {
        fprintf(stderr, "Window is null\n");
        exit(1);
    }
    
	struct packet *p = w->head;
	struct routing_table_entry *r = getroutingtableentry(n, rte->through, 0);

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
void checkmsgdelays(struct node *n, struct windowlist* wl)
{
    if (wl == NULL)
    {
        fprintf(n->srlog, "Windowlist was null when checking for message delays\n");
        return;
    }
    
    struct routing_table_entry *rte = getroutingtableentry(n, wl->name, 0);
	if(rte == NULL)
		return;

	time_t curtime = time(NULL);

    /* get appropriate entries */
	struct window *w = wl->sendq;
    
    if(w == NULL)
    {
        fprintf(stderr, "Window is null\n");
        exit(1);
    }
    
	struct packet *p = w->head;
	struct routing_table_entry *r = getroutingtableentry(n, rte->through, 0);

	if(r == NULL)
		return;

	while(p != NULL)
	{
		struct msgtok *tok = tokenmsg(p->msg);

		if(p->sent)
		{
			/* check timeouts */
			if(p->enqT < curtime - TIMEOUT)
			{
				/* only mark it as having been enqueued just now
				 * and say it hasn't been sent before so it knows
				 * to check for it's delay */
				fprintf(n->srlog, "[%s]Packet [#%d : %s] timed out\n", n->name, tok->acknum, tok->pay);
				printf("[%s]Packet [#%d : %s] timed out...\n", n->name, tok->acknum, tok->pay);

				p->enqT = curtime;
				p->sent = 0;
			}
		}
		else
		{	/* check delays */
			if(p->enqT < curtime - r->delay && !p->received)
			{
				/* send the packet because it's delay is up */
				fprintf(n->srlog, "[%s]Packet [#%d : %s] is now in the network\n", n->name, tok->acknum, tok->pay);
				printf("[%s]Packet [#%d : %s] is now in the network\n", n->name, tok->acknum, tok->pay);

				p->enqT = curtime;
				p->sent = 1;

				sendudp(n->name, p->msg, r->name);
			}
		}

		freetok(tok);
		p = p->next;
	}
	
	return;
}

/* check all of my packets for if they should be sent based off of their delay
 * or if they should be sent based off of the ack timing out (no confirmation) */
void checktimeouts(struct node *n)
{
	struct windowlist *wl = n->windows;
	
	/* check all RTEs for outstanding packets, either delay, or timeout */
	while(wl != NULL)
	{
		checkmsgdelays(n, wl);
		checkackdelays(n, wl);

		wl = wl->next;
	}
	
	return;
}

/* main accept loop for every thread */
void *threadloop(void *arg)
{
	char *name = (char *)arg;
	
	struct node *me = getnodefromname(name);
	time_t laststep = -1;
    time_t lastdvrreset = -1;
	
	if(me == NULL)
	{
		fprintf(stderr, "Fatal error!\n");
		exit(1);
	}
	
    /* main accept loop */
	while(!me->dead)
	{
		laststep = dvr_step(me, laststep);
        lastdvrreset = dvr_reset_step(me, lastdvrreset);
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
		exit(1);
		n->dead = 1;
		
		n = n->next;
	}
	
	return;
}

/* print the dvr entries for node n */
void printnodedvr(struct node *n)
{
	if(n == NULL)
		return;
	
	log_routing_table_to_fd(n, time(NULL), 0, stdout);
	log_routing_table(n, time(NULL), 0);
	
	return;
}

/* prints all the packets currently buffered in the
 window and prints out the window's current (true) size*/
void printwindow(struct node *n, struct window *w)
{
    if(w == NULL)
        return;
    
    struct packet *el = w->head;
    struct msgtok *tok;
    
    while(el != NULL)
    {
	tok = tokenmsg(el->msg);
	if(tok->type == enumrealmsg)
	{
		printf("----[%s]-[%s] [#%d][%s]\n",
			tok->src, tok->dest, tok->acknum, tok->pay);
		fprintf(n->srlog, "----[%s]-[%s] [#%d][%s]\n",
			tok->src, tok->dest, tok->acknum, tok->pay);
	}
	else if(tok->type == enumpureack)
	{
		printf("----[%s]-[%s] [Ack#%d]\n",
		       tok->src, tok->dest, tok->acknum);
		fprintf(n->srlog, "----[%s]-[%s] [Ack#%d]\n",
		       tok->src, tok->dest, tok->acknum);
	}
	else if(tok->type == enumnack)
	{
		printf("----[%s]-[%s] [Nack#%d]\n",
		       tok->src, tok->dest, tok->acknum);
		fprintf(n->srlog, "----[%s]-[%s] [Nack#%d]\n",
		       tok->src, tok->dest, tok->acknum);
	}
	
	freetok(tok);
	tok = NULL;
	
        el = el->next;
    }
	
    fflush(n->srlog);
    
    return;
}

/* print all of the windows for a windowlist */
void printwindowlist(struct node *n, struct windowlist *w)
{
	/* print sendq, then recvq then ackq */
	if(w->sendq != NULL)
	{
		printf("--Send Buffer [Sz:%d]\n", w->sendq->sz);
		fprintf(n->srlog, "--Send Buffer [Sz:%d]\n", w->sendq->sz);
		printwindow(n, w->sendq);
	}
	/* print recvq */
	if(w->recvq != NULL)
	{
		printf("--Recv Buffer [Sz:%d]\n", w->recvq->sz);
		fprintf(n->srlog, "--Recv Buffer [Sz:%d]\n", w->recvq->sz);
		printwindow(n, w->recvq);
	}
	/* ackq */
	if(w->ackq != NULL)
	{
		printf("--Fwd Buffer [Sz:%d]\n", w->ackq->sz);
		fprintf(n->srlog, "--Fwd Buffer [Sz:%d]\n", w->ackq->sz);
		printwindow(n, w->ackq);
	}
	
	fflush(n->srlog);
	
	return;
}

/* print all of the windows for a node */
void printnodewindows(struct node *n)
{
	if(n == NULL)
		return;
	
	struct windowlist *wl = n->windows;
	
	printf("[%s] ==========================\n", n->name);
	fprintf(n->srlog, "[%s] ==========================\n", n->name);
	
	while(wl != NULL)
	{
		if(strcmp(wl->name, n->name) != 0)
		{
			printf("--[%s]\n", wl->name);
			fprintf(n->srlog, "--[%s]\n", wl->name);
			printwindowlist(n, wl);
			printf("--\n");
			fprintf(n->srlog, "--\n");
		}
		
		wl = wl->next;
	}
	
	fflush(n->srlog);
	
	return;
}

/* print all dvr and window information */
void printeverything()
{
	struct list *l = nodelist;
	
	if(l == NULL)
		return;
	
	struct node *n = l->head;
	
	/* iterate for all nodes */
	while(n != NULL)
	{
		printnodedvr(n);
		printnodewindows(n);
		
		n = n->next;
	}
	
	return;
}

/* parse which print they want to use, then print it */
void parseprint(char *msg)
{
    char nodea[BUFSIZE];
    char *tok;
    struct node *n;
    int type=0;
    
    /* if there is no backtick, it is a printall command */
    if((tok = strstr(msg, "`")) == NULL)
    {
        /* print all nodes information */
        printeverything();
    }
    else
    {
	/* get the node name, first token will be the rest of
	 "print" */
        if((tok = strtok(tok, DELIM)) == NULL)
        {
            fprintf(stderr, "Invalid format for print node\n");
            return;
        }
	
	/* mark the type if this token is for dvr */
	if(strcmp("dvr", tok) == 0)
		type = 1;
	
	/* the third will be the node name */
	if((tok = strtok(NULL, DELIM)) == NULL)
	{
		fprintf(stderr, "Invalid format for print node\n");
		return;
	}
        strcpy(nodea, tok);
        
        if((n = getnodefromname(nodea)) == NULL)
        {
            fprintf(stderr, "Error finding node to print\n");
            return;
        }
        
        /* type 1 means this was a dvr command, type 0 was window */
	if(type)
		printnodedvr(n);
	else
		printnodewindows(n);
    }
    
    return;
}

/* parse a message to remove the edge */
/*
 -e`A`B                 -remove edge between A and B
 */
void parseremoveedge(char *msg)
{
    char nodea[BUFSIZE], nodeb[BUFSIZE];
    char *tok;
    
    /* tokenize the remaining message */
    if((tok = strtok(msg, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for remove edge\n");
        return;
    }
    
    strcpy(nodea, tok);
    
    if((tok = strtok(NULL, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for remove edge\n");
        return;
    }
    
    strcpy(nodeb, tok);
    
    /* remove the edge from the master list now */
    struct node *na = getnodefromname(nodea);
    struct node *nb = getnodefromname(nodeb);
    
    if(na == NULL || nb == NULL)
    {
	    fprintf(stderr, "Could not find either nodea or nodeb to remove the edge\n");
	    return;
    }
    
    struct routing_table_entry *rte;
    
    /* get and delete both master entries */
    rte = getroutingtableentry(na, nodeb, 1);
    deleterte(na, rte, 1);
    rte = getroutingtableentry(nb, nodea, 1);
    deleterte(nb, rte, 1);
    
    printf("[%s]-[%s]Edge removed\n", nodea, nodeb);
    return;
}

/* parse adding an edge to the topology */
/*
 +e`A`B`delay`drop      +add edge from A to B to topology
 */
void parseaddedge(char *msg)
{
    char src[BUFSIZE], dest[BUFSIZE];
    int delay, drop;
    char *tok;
    
    struct node *nsrc;
    
    /* token off the newline */
    if((tok = strtok(msg, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for add edge\n");
        return;
    }
    strcpy(src, tok);
    
    
    if((tok = strtok(NULL, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for add edge\n");
        return;
    }
    
    strcpy(dest, tok);
    
    if((tok = strtok(NULL, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for add edge\n");
        return;
    }
    
    delay = atoi(tok);
    
    if((tok = strtok(NULL, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for add edge\n");
        return;
    }
    
    drop = atoi(tok);
    
    /* done tokenizing get values */
    if((nsrc = getnodefromname(src)) == NULL)
    {
        fprintf(stderr, "[%s]Node does not exist\n", src);
        return;
    }
    
    if(getnodefromname(dest) == NULL)
    {
        fprintf(stderr, "[%s]Node does not exist\n", dest);
        return;
    }
    
    struct routing_table_entry *rte;
    rte = getroutingtableentry(nsrc, dest, 0);
    
    if(rte != NULL)
    {
        fprintf(stderr, "[%s]-[%s]Edge already exists\n", src, dest);
        return;
    }
    
    if(delay < 0)
    {
        fprintf(stderr, "[%s]-[%s]Edge delay must be >= 0\n", src, dest);
        return;
    }
    
    if(drop<0 || drop>=100)
    {
        fprintf(stderr, "[%s]-[%s]Edge drop probability must be between 0-100\n",
               src, dest);
        return;
    }
    
    addedge(src, dest, delay, drop);
    printf("[%s]-[%s]Edge added : (-delay:%ds -drop:%d%%)\n",
           src, dest, delay, drop);
    
    return;
}

/* parse removing a node from the topology */
/*
 -n`A                   -remove node A from topology
 */
void parseremovenode(char *msg)
{
    char name[BUFSIZE];
    char *tok;
    
    /* parse off the newline */
    if((tok = strtok(msg, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for remove node\n");
        return;
    }
    strcpy(name, tok);
    
    if(getnodefromname(name) == NULL)
    {
        fprintf(stderr, "[%s]Node does not exist\n", name);
        return;
    }
    
    syncwait();
    sigkillthread(name);
    
    printf("[%s]Node deleted\n", name);
    
    return;
}

/* parse adding a node to the topology */
/*
 +n`A                   -add node A to topology
 */
void parseaddnode(char *msg)
{
    char name[BUFSIZE];
    char *tok;
    
    /* parse off the newline */
    if((tok = strtok(msg, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for add node\n");
        return;
    }
    strcpy(name, tok);
    
    if(getnodefromname(name) != NULL)
    {
        fprintf(stderr, "[%s]Node already exists\n", name);
        return;
    }
    
    addnode(name);
    
    printf("[%s]Node added\n", name);
    return;
}

/* parse sending a message between nodes */
/*
 +s`A`B`Message         -send message from A to B
 */
void parsesendmessage(char *msg)
{
    char src[BUFSIZE], dest[BUFSIZE], pay[BUFSIZE], send[BUFSIZE];
    char *tok;
    
    struct node *nsrc;
    
    /* parse the message */
    if((tok = strtok(msg, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for sending a message\n");
        return;
    }
    strcpy(src, tok);
    
    if((tok = strtok(NULL, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for sending a message\n");
        return;
    }
    strcpy(dest, tok);
    
    if((tok = strtok(NULL, DELIM)) == NULL)
    {
        fprintf(stderr, "Invalid format for sending a message\n");
        return;
    }
    
    strcpy(pay, tok);
    
    /* error check */
    if((nsrc = getnodefromname(src)) == NULL)
    {
        fprintf(stderr, "[%s]Node does not exist\n", src);
        return;
    }
    
    if(getnodefromname(dest) == NULL)
    {
        fprintf(stderr, "[%s]Node does not exist", dest);
        return;
    }
    
    /* see if there is an entry for this path, if there isn't, then
     no connection exists so we can't send a message */
    struct windowlist *wl;
    wl = getwindowlist(nsrc, dest);
    
    if(wl == NULL)
    {
        fprintf(stderr, "[%s]-[%s]No path between nodes exists\n", src, dest);
        return;
    }
    
    /* otherwise valid, so format the message and print */
    sprintf(send, "%d`%s`%s`%s", reqack(wl->sendq), src, dest, pay);
	enqueue(wl->sendq, send);
    
    return;
}

/* wait for everything in the network to finish up and be dequeued before proceeding with
 * input */
void syncwait()
{
	struct node *n;
	struct windowlist *w;

	int keepgoing = 1;

	printf("Waiting for buffers to clear...\n");
	/* wait until everybodies queues are emptied before proceeding */
	while(keepgoing)
	{
		/* set keepgoing to false, and if any queues are non-empty, put keepgoing
		 * back to true */
		keepgoing = 0;
		usleep(200000);
		
		n = nodelist->head;

		/* keepgoing will be false, and will be set to true the first time we see
		 * a non-empty queue, if we see that, no need to continue looking, we
		 * have to wait regardless */
		while(n != NULL && !keepgoing)
		{
			w = n->windows;

			while(w != NULL && !keepgoing)
			{
				/* check if any queues have packets in them */
				if((w->ackq != NULL && w->ackq->sz > 0)
				|| (w->sendq != NULL && w->sendq->sz > 0)
				|| (w->recvq != NULL && w->recvq->sz > 0))
				{
					keepgoing = 1;
					break;
				}

				w = w->next;
			}

			n = n->next;
		}
	}
	
	printf("Done waiting\n");

	return;
}

/* function formats. Use addedge to modify existing edges as well
 
 +s`A`B`Message         -send message from A to B
 +n`A                   +add node A to topology
 -n`A                   -remove node A from topology
 +e`A`B`delay`drop      +add edge from A to B to topology
 -e`A`B                 -remove edge between A and B from topology
 print`wind`A		print all windows for A
 print`dvr`A		print all DVR entries for A
 printall		print everything for all nodes
 wait			wait for everything in the network to be resolved

 */

/* parse the first two characters to see what time of command
 it is */
void determinetype(char *filename)
{
    FILE *file = fopen(filename, "r");
    char line[BUFSIZE];
    char remainder[BUFSIZE];
    char two[3];
    int stdone = 0;
    
    /* this means one of two things, either invalid file, or
     purposely skip straight to manual input. either way...
     skip straight to manual input */
    if(file == NULL)
    {
        file = stdin;
    }
    
    while(!feof(file) || file == stdin)
    {
        /* let the user know they can input */
        if(file == stdin && !stdone)
        {
            printf("--------------------\n");
            printf("Accepting user input\n");
            printf("Type exit or quit to exit\n");
            printf("--------------------\n\n");
            stdone = 1;
        }
        
        /* each line includes the newline at the end */
        if(fgets(line, BUFSIZE, file))
        {
            /* at least 2 chars need to be present for a
             control message so discard if smaller */
            if(strlen(line) < 2)
            {
                /* can't be a real message, so don't try
                 to tokenize it, just skip this line */
                continue;
            }
            
            memset(two, 0, sizeof(two));
            memset(remainder, 0, sizeof(remainder));
            
            strncpy(two, line, 2);
            two[2] = '\0';
            /* move over 2 to skip the control */
            strncpy(remainder, line+(2*sizeof(char)),
                    strlen(line)-2*(sizeof(char)));
            
            if(strcmp("+s", two) == 0)
            {
                /* call parsesend */
                parsesendmessage(remainder);
            }
            else if(strcmp("+n", two) == 0)
            {
                parseaddnode(remainder);
            }
            else if(strcmp("-n", two) == 0)
            {
                parseremovenode(remainder);
            }
            else if(strcmp("+e", two) == 0)
            {
                parseaddedge(remainder);
            }
            else if(strcmp("-e", two) == 0)
            {
                parseremoveedge(remainder);
            }
            else if(strcmp("pr", two) == 0)
            {
                parseprint(remainder);
            }
			else if(strcmp("wa", two) == 0)
			{
				syncwait();
			}
            else if(strcmp("ex", two) == 0 || strcmp("qu", two) == 0)
            {
                return;
            }
            else
            {
                fprintf(stderr, "Unrecognized command\n");
            }
        }
        else
        {
            /* we've reached the end of the input file
             so point to stdin */
            fclose(file);
            file = stdin;
        }
        
    }
    
    return;
}

/* temp main function */
int main(int argc, char *argv[])
{
	/* initialize and create the nodes */
    initialize();
    createlogdir();
    
    /* just go straight to userloop if they didn't specify
     an input file */
    if(argc>1)
        determinetype(argv[1]);
    else
        determinetype("\0");
    
    sigkillall();
    pthread_exit(NULL);
    
    return 0;
}

