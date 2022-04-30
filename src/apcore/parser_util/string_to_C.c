#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <string.h>

int usage(char* s) {
	fprintf(stderr, "Usage: %s <file name>\n", s);
	return -1;
}

char*	in = NULL;
int	size_in = 0;
int	actual_in = 0;
char*	out = NULL;
int	size_out = 0;
int	actual_out = 0;

void concat_in(char s) {
	// after reading, actual_in must be at most size_in - 1
	if(actual_in > size_in - 2) {
		size_in = 2 * size_in + 10;
		if(!in) {
			in = (char*) malloc(size_in);
		} else {
			in = (char*) realloc(in, size_in);
		}
	}
	in[actual_in++] = s;
}

int main(int argc, char* argv[]) {
	FILE*	inchar = fopen(argv[1], "r");
	int	k;

	if(argc != 2) {
		return usage(argv[0]);
	}
	if(!inchar) {
		fprintf(stderr, "File %s not found\n", argv[1]);
	}

	char c;
	while(fread(&c, 1, 1, inchar)) {
		concat_in(c);
	}
	fclose(inchar);

	// write prelude
	printf("char c[] = \"");
	for(k = 0; k < actual_in; k++) {
		switch(in[k]) {
			case '\"':
				printf("\\\"");
				break;
			case '\t':
				printf("\\t");
				break;
			case '\\':
				printf("\\\\");
				break;
			case '\n':
				if(k == actual_in - 1) {
				    printf("\\n\";\n");
				} else {
				    // terminate current line and start new one
				    printf("\\n\"\n\t\"");
				}
				break;
			default:
				printf("%c", in[k]);
				break;
		}
	}
	return 0;
}
