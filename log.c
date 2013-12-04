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

    //strftime(folderpath, MAXFOLDERSIZE, "logs/%d-%m-%Y-%T", localtime(&curtime));

    sprintf(folderpath, "logs/%d/", (int)curtime);
    fprintf(stderr, "about to create folder at path: %s\n", folderpath);
    if ((error = mkdir(folderpath, S_IREAD | S_IEXEC | S_IWRITE)) != 0)
    {
        fprintf(stderr, "error creating directory: %s", strerror(error));
    }
    fprintf(stderr, "done\n");
}

FILE* createlogfile(char* nodename)
{

    fprintf(stderr, "about to create logfile at name: %s\n", nodename);
    char namepath[MAXFOLDERSIZE];
    bzero(namepath, MAXFOLDERSIZE);
    sprintf(namepath, "%s/%s", folderpath, nodename);

    FILE* to_return = NULL;

    to_return = fopen(namepath, "w");
    
    return to_return;
}

void log_routing_table(struct node* a, int timestep)
{
    FILE* logfile = a->logfile;
    struct routing_table_entry* rte = a->routing_table;
    fprintf(logfile, "=====================\n%s Routing Table At Timestep %d\n", a->name, timestep);
    fprintf(logfile, "dest\tnexthop\tweight\n");

    while (rte != NULL)
    {
        fprintf(logfile, "%s\t%s\t%d\n", rte->name, rte->through, rte->weight);
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
