#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <errno.h>
#include "node.h"

#define MAXFOLDERSIZE 64
char folderpath[MAXFOLDERSIZE];
void createlogdir()
{
    int error;
    bzero(folderpath, MAXFOLDERSIZE);
    time_t curtime;
    time(&curtime);

    mkdir("logs", S_IREAD | S_IEXEC | S_IWRITE);

    strftime(folderpath, MAXFOLDERSIZE, "logs/%d-%m-%Y-%T", localtime(&curtime));
    
    if ((error = mkdir(folderpath, S_IREAD | S_IEXEC | S_IWRITE)) != 0)
    {
        fprintf(stderr, "error creating directory: %s", strerror(error));
    }
}

FILE* createlogfile(char* nodename)
{
    char namepath[MAXFOLDERSIZE];
    bzero(namepath, MAXFOLDERSIZE);
    sprintf(namepath, "%s/%s", folderpath, nodename);

    FILE* to_return = NULL;

    to_return = fopen(namepath, "w");
    
    return to_return;
}

void log_routing_table(struct node* a, int timestep, int resetstep)
{
    if (a == NULL) {
        fprintf(stderr, "ERROR: log routing table got null node\n");
        return;
    }
    FILE* logfile = a->logfile;
    if (logfile == NULL) {
        fprintf(stderr, "ERROR: log routing table got node [%s] with null fd\n", a->name);
        return;
    }
    struct routing_table_entry* rte = a->routing_table;
    if (resetstep)
        fprintf(logfile, "=====================\n%s RESET Routing Table At Timestep %d\n", a->name, timestep);
    else
        fprintf(logfile, "=====================\n%s Routing Table At Timestep %d\n", a->name, timestep);
    fprintf(logfile, "dest\tnexthop\tweight\n");

    while (rte != NULL)
    {
        fprintf(logfile, "%s\t%s\t%.2f\n", rte->name, rte->through, rte->weight);
        rte = rte->next;
    }
    fprintf(logfile, "=====================\n");
    fflush(logfile);
}

/*int main() {
    createlogdir();
    FILE* lf = createlogfile("testnodename");
    fprintf(lf, "testing!\n");
    fclose(lf);
    return 0;
}*/
