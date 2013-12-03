#ifndef _NTDVR_H_
#define _NTDVR_H_

#include <time.h>
#include "node.h"

void set_interval(int seconds);

time_t dvr_step(struct node* a, time_t laststep);

void send_dvr_message(struct node* src, char* dest);

void handledvrmessage(struct node* nodea, struct msgtok* msg);

#endif
