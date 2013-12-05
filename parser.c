#include "parser.h"
#include "node.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void parse_file(char* filename) {
	FILE* f = NULL;
	char* lineptr = NULL;
	char* temp;
    char* nodeaname;
    char* nodebname;
    int delay;
    int drop;
    size_t tempi2;
    
	f = fopen(filename, "r");

	if (f == NULL) {
		fprintf(stderr, "could not open input file: %s", filename);
		return;
	}

	while(getline(&lineptr, &tempi2, f) > 0) {
		temp = strtok(lineptr, "-");
        nodeaname = malloc(strlen(temp)+1);
        bzero(nodeaname, strlen(temp)+1);
        strcpy(nodeaname, temp);

		temp = strtok(NULL, "-");
		delay = atoi(temp);		

		temp = strtok(NULL, "%");
        nodebname = malloc(strlen(temp)+1);
        bzero(nodebname, strlen(temp)+1);
        strcpy(nodebname, temp);
	
		temp = strtok(NULL, "\n");
	    drop = atoi(temp);
	
		//printf("node-delay-node-drop: %s-%d-%s-%d\n",
		//       nodeaname, delay, nodebname, drop);

        addnode(nodeaname);
        addnode(nodebname);
	
        addedge(nodeaname, nodebname, delay, drop);
        free(lineptr);
        lineptr = NULL;
	}
}

