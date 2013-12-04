#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <time.h>

#define MAXFOLDERSIZE 32
char folderpath[MAXFOLDERSIZE];
void createlogdir()
{
    bzero(folderpath, MAXFOLDERSIZE);
    time_t curtime = time(NULL);
    strftime(folderpath, MAXFOLDERSIZE, "logs/%d-%m-%Y-%T", localtime(&curtime));
    fprintf(stderr, "about to create folder at path: %s\n", folderpath);
    mkdir(folderpath, S_IRWXU);
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

