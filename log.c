#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <errno.h>

#define MAXFOLDERSIZE 64
char folderpath[MAXFOLDERSIZE];
void createlogdir()
{
    int error;
    bzero(folderpath, MAXFOLDERSIZE);
    time_t curtime;
    time(&curtime);
    strftime(folderpath, MAXFOLDERSIZE, "logs/%d-%m-%Y-%T", localtime(&curtime));
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

/*int main() {
    createlogdir();
    FILE* lf = createlogfile("testnodename");
    fprintf(lf, "testing!\n");
    fclose(lf);
    return 0;
}*/
