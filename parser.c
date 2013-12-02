#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void parse_file(char* filename) {
	FILE* f = NULL;
	char* lineptr = NULL;
	char* temp;
	int tempi;
	int tempi2;
	f = fopen(filename, "r");

	if (f == NULL) {
		fprintf(stderr, "could not open input file: %s", filename);
		return;
	}

	while(getline(&lineptr, &tempi2, f) > 0) {
		temp = strtok(lineptr, "-");
		//addNode(temp);
		printf("node weight node: %s", temp);

		temp = strtok(NULL, "-");
		tempi = atoi(temp);		
		printf(" %d ", tempi);

		temp = strtok(NULL, "\n");
		printf("%s\n", temp);
	}
}

int main(int argc, char** argv)
{
	parse_file("test.top");

	return 0;
}