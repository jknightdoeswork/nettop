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
    int weight;
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
		weight = atoi(temp);		

		temp = strtok(NULL, "\n");
        nodebname = malloc(strlen(temp)+1);
        bzero(nodebname, strlen(temp)+1);
        strcpy(nodebname, temp);
		printf("node-weight-node: %s-%d-%s\n", nodeaname, weight, nodebname);

        addnode(nodeaname);
        addnode(nodebname);
        // TODO CALCULATE WEIGHTS
        addedge(nodeaname, nodebname, weight, 0);
        free(lineptr);
        lineptr = NULL;
	}
}

