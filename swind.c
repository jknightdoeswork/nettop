//
//  swind.c
//  

#include "node.h"

/* increment the sliding window range for a given window */
void incr(struct window *q)
{
    if(++(q->first) == QUEUESIZE)
        q->first = 0;
    
    if(++(q->last) == QUEUESIZE)
        q->last = 0;
    
    return;
}

/* get the next value in a sliding window (wrap on QUEUESIZE) */
int plusone(int i)
{
    if(i<QUEUESIZE-1)
        return i+1;
    else return 0;
}

/* check if an acknum is outside of the range of the
 sliding window */
int outside(struct window *q, int acknum)
{
    int first = q->first, last = q->last;
    
    /* the window is not wrapped so just check if in between */
    if(last > first)
    {
        if(acknum < first || acknum > last)
            return 1;
        else
            return 0;
    }
    else
    {
        if(acknum < first && acknum > last)
            return 1;
        else
            return 0;
    }
}

/* general function for a node to receive a message */
int receivemsg(char *name)
{
    char recvbuf[BUFSIZE];
    struct sockaddr_storage their;
    socklen_t addr_size;
    
    int sock = getsockfromname(name);
    
    int bytes = recvfrom(sock, recvbuf, BUFSIZE-1, 0,
                         (struct sockaddr*)&their, &addr_size);
    
    /* since we are using non-blocking recvfrom dont proceed
     if we didn't actually receive anything */
    if(bytes <= 0)
        return bytes;
    
    recvbuf[bytes] = '\0';
    
    struct msgtok *tok = tokenmsg(recvbuf);
    
    if(tok->type == enumdvrmsg)
         handledvrmessage(getnodefromname(name), tok);
    else if(tok->type == enumpureack)
        handleack(name, tok);
    else if(tok->type == enumnack)
        handlenack(name, tok);
    else
        handlemsg(name, tok);
    
    freetok(tok);
    return bytes;
}

/* read the payload to see what type it is */
int interpret(char *msg)
{
    if(msg[0] == '+')
        return enumpureack;
    else if(msg[0] == '-')
        return enumnack;
    else if (msg[0] == '&')
        return enumdvrmsg;
    else
        return enumrealmsg;
}

/* clear all packets in a window that are now in order */
void clearwindow(struct routing_table_entry *ql, int type)
{
    struct window *q;
    
    switch(type)
    {
        case enumpureack:
            q= ql->sendq;
            break;
            
        case enumrealmsg:
            q = ql->recvq;
            break;
    }
    
    struct packet *el = q->head;
    
    while(el != NULL)
    {
        /* check if in order */
        if(el->acknum == q->first && (el->received || type == enumrealmsg))
        {
            /* then it is in order so increment our window
             and dequeue the element*/
            incr(q);
            
            /* if it was just an ack then just advance the window
             and remove it from our buffer */
            if(type == enumpureack)
                free(dequeue(q));
            else
            {
                /* a true message so we need to handle it */
                struct packet *el = dequeue(q);
                
                struct msgtok *tok = tokenmsg(el->msg);
                struct routing_table_entry *ql = getroutingtableentry(getnodefromname(tok->dest), tok->src);
		
		if(ql == NULL)
			return;
                
                char returnmsg[BUFSIZE];
                
                sprintf(returnmsg, "%d`%s`%s`Message %d received!\n",
                        reqack(ql->sendq), tok->dest, tok->src, tok->acknum);
                
		//temp do not send message back
                /* enqueue it in our sendq to the destination */
                //enqueue(ql->sendq, returnmsg);
                
                //sendudp(tok->dest, returnmsg, tok->src);
                
                /* free the memory we were using */
                freetok(tok);
                free(el);
            }
            
            
            /* set it to the new head to see if it also should be
             dequeued */
            el = q->head;
        }
        else
            break;
    }
    
    return;
}

/* check if the message is a duplicate (already exists
 in our window) */
int ihavemsg(struct window *q, int ack)
{
    struct packet *el = q->head;
    
    while(el != NULL)
    {
        if(el->acknum == ack)
            return 1;
        
        el = el->next;
    }
    
    return 0;
}

/* check if the message is in order for our window
 (the next one we are expecting, or if something was lost) */
int msginorder(struct window *q, int ack)
{
    if(ack == q->first)
        return 1;
    else return 0;
}

/* properly frees a tokenized message */
void freetok(struct msgtok *tok)
{
    if(tok == NULL)
        return;
    else
    {
        /* free all of the msgtok buffers then free
         the msgtok itself */
	free(tok->orig);
        free(tok->ack);
        free(tok->src);
        free(tok->dest);
        free(tok->pay);
        free(tok);
    }
    
    return;
}

/* parse the message and store all of it's info in a
 msgtok structure and return the structure */
struct msgtok *tokenmsg(char *msg)
{
    char *tok, *saveptr;
    char *temp = malloc(BUFSIZE);
    
    struct msgtok *msgtok = malloc(sizeof(struct msgtok));
    
    msgtok->orig = malloc(BUFSIZE);
    msgtok->ack = malloc(BUFSIZE);
    msgtok->src = malloc(BUFSIZE);
    msgtok->dest = malloc(BUFSIZE);
    msgtok->pay = malloc(BUFSIZE);
    msgtok->type = interpret(msg);
    
    /* because we do a lot of forwarding, store the original
     * message in the token as well */
    strcpy(msgtok->orig, msg);
    /* use temp so we don't break the original message */
    strcpy(temp, msg);
   
    /* interpretable character */
    if (msgtok->type != enumrealmsg) {
        tok = strtok_r(temp, CONTROLCHARS, &saveptr);
        temp = tok; // only call strtok_r(temp,...) once
    }
    

    /* ack */
    if(msgtok->type != enumdvrmsg)
    {
	tok = strtok_r(temp, DELIM, &saveptr);
	if(tok == NULL)
	{
		printf("Error tokenizing ack!\n");
		exit(1);
	}
	/* store the ack in both character and integer format so we
	can easily reference it's value, as well as add it to messages
	without relying heavily on temporary variables and sprintf */
	strcpy(msgtok->ack, tok);
	msgtok->acknum = atoi(msgtok->ack);
	
	temp = NULL;
    }
    
    /* src */
    tok = strtok_r(temp, DELIM, &saveptr);
    if(tok == NULL)
    {
        printf("Error tokenizing src!\n");
        exit(1);
    }
    strcpy(msgtok->src, tok);
    
    /* dest */
    tok = strtok_r(NULL, DELIM, &saveptr);
    if(tok == NULL)
    {
        printf("Error tokenizing dest!\n");
        exit(1);
    }
    strcpy(msgtok->dest, tok);
    
    /*payload */
    if (msgtok->type == enumrealmsg || msgtok->type == enumdvrmsg)
    {
	tok = strtok_r(NULL, DELIM, &saveptr);
	if(tok == NULL)
	{
	    printf("[%s]\nError tokenizing payload!\n", msgtok->orig);
	    exit(1);
	}
	else
	    strcpy(msgtok->pay, tok);
    }
    
    return msgtok;
}

/* return true if I (given my name and a message)
 am the intended recipient of the message */
int iamdest(char *name, struct msgtok *tok)
{
    if(strcmp(tok->dest, name) == 0)
        return 1;
    else
        return 0;
}

/* send back an ack given a message */
void sendbackack(struct msgtok *tok)
{
    char ackmsg[BUFSIZE];
    
    /* reverse dest and src because we are sending back to them */
    sprintf(ackmsg, "+%d`%s`%s`", tok->acknum, tok->dest, tok->src);
    
    struct routing_table_entry *rte;
    rte = getroutingtableentry(getnodefromname(tok->dest), tok->src);
    
    if(rte == NULL)
	    return;
    
    /* enqueue it to my delay queue so it waits
     the appropriate length of time before being sent */
    enqueue(rte->ackq, ackmsg);
    //sendudp(tok->dest, ackmsg, tok->src);
    
    return;
}

/* send back a nack given a message */
void sendbacknack(struct msgtok *tok, int out)
{
    char nackmsg[BUFSIZE];
    
    /* reverse dest and src */
    sprintf(nackmsg, "-%d`%s`%s`", out, tok->dest, tok->src);
    
    struct routing_table_entry *rte;
    rte = getroutingtableentry(getnodefromname(tok->dest), tok->src);
    
    if(rte == NULL)
	    return;
    
    /* enqueue it to my delay queue so it waits
     the appropriate length of time before being sent */
    enqueue(rte->ackq, nackmsg);
    //sendudp(tok->dest, nackmsg, tok->src);
    
    return;
}

/* send back nacks for all messages outstanding
 (out of order) */
void sendnacks(struct window *q, struct msgtok *tok)
{
    struct packet *el = q->head;
    int outack = q->first;
    
    while(el != NULL)
    {
        while(el->acknum != outack)
        {
            /* then outack is an outstanding ack */
            sendbacknack(tok, outack);
            outack++;
            
            /* stop if we reach the end; should only happen
             if el is the last element */
            if(outack >= tok->acknum)
                break;
        }
        
        outack++;
        el = el->next;
    }
    
    return;
}

/* handle the message if it is an ack (ie marking the packets
 as having been successfully received) */
void handleack(char *name, struct msgtok *tok)
{
    /* forward it if it isn't intended for me */
    if(!iamdest(name, tok))
    {
	    printf("Forwarding ack!\n");
        /* i am not the destination so forward it towards the
         real destination. the msg will still contain the original
         sender, so we won't modify the message */
	struct routing_table_entry *rte;
	rte = getroutingtableentry(getnodefromname(name), tok->dest);
	
	/* put it in queue to handle delays, and then let
	 * our thread handle it when the delay comes up */
	enqueue(rte->ackq, tok->orig);
        //sendudp(name, tok->orig, tok->dest);
        
        return;
    }
    
    /* I just received a pure ack confirming a message I sent */
    printf("Received an ack: [#%d]:[%s]-[%s]\n-----------\n",
	    tok->acknum, tok->dest, tok->src);
    
    /* the source of the message is the "dest" of my queue */
    struct routing_table_entry *ql = getroutingtableentry(getnodefromname(name), tok->src);
    
    if(ql == NULL)
	    return;
    
    /* now it's an ack confirming our message, so we want
     the sendq */
    struct window *q = ql->sendq;
    
    struct packet *el = q->head;
    int ack = tok->acknum;
    
    /* find the message and mark it received */
    while(el != NULL)
    {
        if(el->acknum == ack)
        {
            el->received = 1;
            break;
        }
        
        el = el->next;
    }
    
    if(el == NULL)
    {
	    printf("Received duplicate ack!\n");
	    // duplicate acks are okay if we blindly re-send, but
	    // they did actually receive it ie if timeouts are small
	    // and it 'timed out' but is still in the network
    }
    
    clearwindow(ql, enumpureack);
    
    return;
}

/* handle if we received a nack. (ie a packet was
 lost or delayed) */
void handlenack(char *name, struct msgtok *tok)
{
    /* forward it if it isn't intended for me, delays were calculated
     by the main node */
    if(!iamdest(name, tok))
    {
	    printf("Forwarding nack\n");
        /* i am not the destination so forward it towards the
         real destination. the msg will still contain the original
         sender, so we won't modify the message */
	struct routing_table_entry *rte;
	rte = getroutingtableentry(getnodefromname(name), tok->dest);
	
	/* put it in queue to handle delays, and then let
	 * our thread handle it when the delay comes up */
	enqueue(rte->ackq, tok->orig);
        //sendudp(name, tok->orig, tok->dest);
        
        return;
    }
    
    printf("Received a nack: [#%d]:[%s]-[%s]\n-----------\n",
	    tok->acknum, tok->dest, tok->src);
    
    struct routing_table_entry *ql = getroutingtableentry(getnodefromname(tok->dest), tok->src);
    
    if(ql == NULL)
	    return;
    
    /* it is a nack so I want to re-send outstanding message which will
     be in my sendq */
    struct window *q = ql->sendq;
    
    struct packet *el = q->head;
    
    while(el != NULL)
    {
        /* if the message corresponds to the nack, resend it */
        if(el->acknum == tok->acknum)
        {
            /* the message is already in our queue, just
             adjust a few values to prompt the thread
             to resend it when delay is up */
            el->enqT = time(NULL);
            el->sent = 0;
            break;
        }
        
        el = el->next;
    }
    
    return;
}

/* given a real message, interpret it and send back an ack
 saying that I've successfully received the message */
void handlemsg(char *name, struct msgtok *tok)
{
    /* delays were calculated by the main node */
    if(!iamdest(name, tok))
    {
	    printf("Forwarding message!\n");
        /* i am not the destination so forward it towards the
         real destination. the msg will still contain the original
         sender, so we won't modify the message */
	struct routing_table_entry *rte;
	rte = getroutingtableentry(getnodefromname(name), tok->dest);
	
	/* put it in queue to handle delays, and then let
	 * our thread handle it when the delay comes up */
	enqueue(rte->ackq, tok->orig);
        //sendudp(name, tok->orig, tok->dest);
        
        return;
    }
    
    printf("Received a message:\n[#%d][%s]-[%s]\n%s-----------\n",
           tok->acknum, tok->src, tok->dest, tok->pay);
    
    /* if we get here then I was the intended recipient so handle
     it properly */
    
    /* reverse dest and src because we are receiving from them so
     we want my buffer */
    struct routing_table_entry *ql = getroutingtableentry(getnodefromname(tok->dest), tok->src);
    
    if(ql == NULL)
	    return;
    
    /* this is a real message so we want to receive it */
    struct window *q = ql->recvq;
    
    int ack = tok->acknum;
    
    /* if I've already received it, or it is outside
     my buffer then it must be a retransmission that I've
     already attempted to okay, so just okay it again */
    if(ihavemsg(q, ack) || outside(q, ack))
    {
        /* duplicate message retransmit ack */
        sendbackack(tok);
    }
    else
    {
        /* but it is still a valid message, so buffer it
         in the meantime */
        enqueue(q, tok->orig);
        
        /* and send back an ack for the message */
        sendbackack(tok);
        
        /* also send nacks if message out of order */
        if(!msginorder(q, ack))
            sendnacks(q, tok);
        
        /* and clear the queue */
        clearwindow(ql, enumrealmsg);
    }
    
    return;
}
