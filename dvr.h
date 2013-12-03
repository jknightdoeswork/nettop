#ifndef _NTDVR_H_
#define _NTDVR_H_

#include <time.h>
#include "node.h"

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

#endif
