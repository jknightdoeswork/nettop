#include "node.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define msgformat "&%s`%s`%f`" //&me`dest`distance`
int dvr_interval = 1;
int dvr_reset_interval = 10;

int is_neighbour(struct routing_table_entry* rte);

void clear_dvr(struct node* a)
{
    struct routing_table_entry *rte = a->routing_table;
    struct routing_table_entry *tmp;
    while(rte != NULL)
    {
        tmp = rte;
        rte = rte->next;
        freeroutingtableentry(tmp);
    }
    a->routing_table = NULL;
}

void reset_dvr(struct node* a)
{
    struct routing_table_entry *rte = a->connected_edges;
    while(rte != NULL)
    {
        rtappend(a, rte->name, rte->through, rte->delay, rte->drop, rte->weight, 0);
        rte = rte->next;
    }
    rtappend(a, a->name, a->name, 0, 0, 0, 0);
}

void set_interval(int seconds)
{
    dvr_interval = seconds;
}

void set_dvr_reset_interval(int seconds)
{
    dvr_reset_interval = seconds;
}

time_t dvr_reset_step(struct node* a, time_t lastresetstep)
{
    time_t curtime = time(NULL);
    if (lastresetstep != -1 && difftime(curtime, lastresetstep) < dvr_interval)
        return lastresetstep;
    clear_dvr(a);
    reset_dvr(a);
    log_routing_table(a, curtime, 1);
    return curtime;
}

time_t dvr_step(struct node* a, time_t laststep)
{
    time_t curtime = time(NULL);
    if (laststep != -1 && difftime(curtime, laststep) < dvr_interval)
        return laststep;
    
    
    // send my datas to all neighbours
    struct routing_table_entry *rte = a->connected_edges;
    
    while (rte != NULL)
    {
        send_dvr_message(a, rte->name);
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
        if (strcmp(rte->through, dest) != 0)
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
    float bcweight = (float)atof(msg->pay);
    float abweight;
    float abcweight;
    int routingtablechanged = 0;
    struct routing_table_entry* abrte;
    struct routing_table_entry* acrte;
    time_t curtime = time(NULL);
    
    // find distance between a, b
    abrte = getroutingtableentry(nodea, nodebname, 0);
    if (abrte == NULL) {
        return;
    }
    
    abweight = abrte->weight;
    abcweight = abweight + bcweight;

    // compare abcweight against current routing table entry for a to c
    acrte = getroutingtableentry(nodea, nodecname, 0);
    if (acrte == NULL)
    {
        rtappend(nodea, nodecname, nodebname, 0, 0, abcweight, 0);
        routingtablechanged = 1;
    }
    else if (acrte->weight > abcweight) 
    {
	    acrte->weight = abcweight;
	    strcpy(acrte->through, nodebname);
        routingtablechanged = 1;
    }

    if (routingtablechanged)
        log_routing_table(nodea, curtime, 0);
}

int is_neighbour(struct routing_table_entry* rte)
{
    return (strcmp(rte->through, rte->name) == 0);
}

