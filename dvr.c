#include "node.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define msgformat "&%s`%s`%f`" //&me`dest`distance`
int dvr_interval = 2;
int is_neighbour(struct routing_table_entry* rte);

void set_interval(int seconds)
{
    dvr_interval = seconds;
}

time_t dvr_step(struct node* a, time_t laststep)
{
    time_t curtime = time(NULL);
    if (laststep != -1 && difftime(curtime, laststep) < dvr_interval)
        return laststep;
    
    log_routing_table(a, curtime);
    // send my datas to all neighbours
    struct routing_table_entry *rte = a->routing_table;
    
    while (rte != NULL)
    {
        if (is_neighbour(rte))
        {
            send_dvr_message(a, rte->name);
        }
        rte = rte->next;
    }
    return curtime;
}

void send_dvr_message(struct node* src, char* dest)
{
    // create msg
    char message[BUFSIZE];

    struct routing_table_entry *rte = src->routing_table;
    while (rte != NULL)
    {
        if (strcmp(rte->name, dest) != 0)
        {
            bzero(message, BUFSIZE);
            sprintf(message, msgformat, src->name, rte->name, rte->weight);
            sendudp(src->name, message, dest);
        }
        rte = rte->next;
    }
}

void handledvrmessage(struct node* nodea, struct msgtok* msg)
{
    // i am node a
    char* nodebname = msg->src;
    char* nodecname = msg->dest;
    int bcweight = atoi(msg->pay);
    int abweight;
    int abcweight;
    struct routing_table_entry* abrte;
    struct routing_table_entry* acrte;
    
    // find distance between a, b
    abrte = getroutingtableentry(nodea, nodebname);
    if (abrte == NULL) {
        fprintf(stderr, "ERROR handledvrmessage. %s is not in %s's routing table\n", nodebname, nodea->name);
        return;
    }
    
    abweight = abrte->weight;
    abcweight = abweight + bcweight;

    // compare abcweight against current routing table entry for a to c
    acrte = getroutingtableentry(nodea, nodecname);
    if (acrte == NULL)
    {
        // no existing entry
        rtappend(nodea, nodecname, nodebname, 0, 0, abcweight);
    }
    else if (acrte->weight > abcweight) 
    {
	    acrte->weight = abcweight;
	    strcpy(acrte->through, nodebname);
    }
}

int is_neighbour(struct routing_table_entry* rte)
{
    return (strcmp(rte->through, rte->name) == 0);
}

