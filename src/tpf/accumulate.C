#include <iostream>
#include <sstream>
#include <string.h>
#include <stdio.h>

#include "config.h"

#include <stdlib.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif 

#include <map>
#include <string>

std::map<std::string, std::pair<long int, char> >& funcs() {
    static std::map<std::string, std::pair<long int, char> > M;
    return M;
}

bool udef = true;

const char* process_body_line(const char* c) {
    int state = 0;
    int chars = 0;
    std::stringstream fstream;
    std::string fname;
    std::stringstream cstream;
    std::string countstring;

    while(*c) {
	if(state == 0) {

	    // catch the blank line that separates AAF
	    // functions from udef functions
	    if(*c == '\n') {
		udef = false;
	    } else if(isblank(*c)) {
		if(!chars) {
		    fprintf(stderr, "no function name found in body line\n");
		    int error_code = -20;
		    throw(error_code);
		}
		fname = fstream.str();
		state = 1;
	    } else {
		chars++;
		fstream << *c;
	    }
	} else if(state == 1) {
	    if(isblank(*c)) {
		;
	    } else if(*c == '|') {
		state = 2;
		chars = 0;
	    } else {
		fprintf(stderr, "strange character %c found in body line\n", *c);
		int error_code = -21;
		throw(error_code);
	    }
	} else if(state == 2) {
	    if(isblank(*c)) {
		;
	    } else if(*c == '\n') {
		if(!chars) {
		    fprintf(stderr, "no function call number found in body line\n");
		    int error_code = -22;
		    throw(error_code);
		}
		countstring = cstream.str();
		long int num;
		sscanf(countstring.c_str(), "%ld", &num);
		std::map<std::string, std::pair<long int, char> >::const_iterator iter = funcs().find(fname);
		if(iter == funcs().end()) {
		    char w = 'u';
		    if(!udef) {
			w = 'a';
		    }
		    funcs()[fname] = std::pair<long int, char>(num, w);
		} else {
		    std::pair<long int, char> prev = iter->second;
		    funcs()[fname] = std::pair<long int, char>(num + prev.first, prev.second);
		}
		c++;
		break;
	    } else if(isdigit(*c)) {
		cstream << *c;
		chars++;
	    } else {
		fprintf(stderr, "strange character %c found in body line\n", *c);
		int error_code = -23;
		throw(error_code);
	    }
	}
	c++;
    }
    return c;
}
const char* process_second_line(const char* c) {
    int state = 0;
    int chars = 0;
    while(*c) {
	if(state == 0) {
	    if(*c == '|') {
		if(!chars) {
		    fprintf(stderr, "no leading heading found in second line\n");
		    int error_code = -20;
		    throw(error_code);
		}
		state = 1;
	    } else if(*c == '-') {
		chars++;
	    } else if(isblank(*c)) {
		;
	    } else {
		    fprintf(stderr, "strange character found in second line\n");
		    int error_code = -21;
		    throw(error_code);
	    }
	} else if(state == 1) {
	    if(*c == '_') {
		;
	    } else if(isblank(*c)) {
		;
	    } else if(*c == '\n') {
		state = 2;
	    }
	}
	c++;
	if(state == 2) {
	    break;
	}
    }
    if(state != 2) {
	fprintf(stderr, "incomplete second line\n");
	int error_code = -22;
	throw(error_code);
    }
    return c;
}

const char* process_first_line(const char* c) {
    int state = 0;
    int chars = 0;
    while(*c) {
	if(state == 0) {

	    // keep reading until '|'
	    if(*c == '|') {
		state = 1;
	    }
	} else if(state == 1) {
	    if(*c == '\n') {
		state = 2;
	    }
	}
	c++;
	if(state == 2) {
	    break;
	}
    }
    if(state != 2) {
	fprintf(stderr, "incomplete header line\n");
	int error_code = -11;
	throw(error_code);
    }
    return c;
}

int process_one_input_string(const char* c) {
    udef = true;
    try {
	const char* d = process_first_line(c);
	d = process_second_line(d);
	while((d = process_body_line(d)) && *d) {
	    ;
	}
    } catch(int k) {
	return k;
    }
    return 0;
}

int process_one_input_file(FILE* inF) {
    char c[101];
    std::stringstream S;
    while(true) {
	int num_read = fread(c, 1, 100, inF);
	c[num_read] = '\0';
	if(num_read > 0) {
	    S << std::string(c);
	}
	if(num_read < 100) {
	    break;
	}
    }

    std::string d_string = S.str();

    const char* d = d_string.c_str();

    return process_one_input_string(d);
}


int usage(char* s) {
    std::cerr << "Usage: " << s << " -output <outfile> <infile 1> [ <infile 2> [ <infile 3> ... ] ]\n";
    return -1;
}

int main(int argc, char* argv[]) {
    FILE	*outF, **inF;
    int		numfiles = 0;

    if(argc < 4) {
	return usage(argv[0]);
    }
    if(strcmp(argv[1], "-output")) {
	return usage(argv[0]);
    }
    numfiles = argc - 3;
    inF = (FILE**)malloc(sizeof(FILE*) * numfiles);
    for(int k = 0; k < numfiles; k++) {
	inF[k] = fopen(argv[k + 3], "r");
	if(!inF[k]) {
	    fprintf(stderr, "unable to open input file %s\n", argv[k+3]);
	    return -1;
	}
	if(process_one_input_file(inF[k])) {
	    fprintf(stderr, "error in input file %s\n", argv[k+3]);
	    return -2;
	}
    }
    free((char*)inF);

    // output what we got
    outF = fopen(argv[2], "w");
    if(!outF) {
	fprintf(stderr, "Cannot open output file \"%s\"\n", argv[2]);
	return -3;
    }
    const char* first = "function | calls | udef (u) or AAF (a)\n";
    fwrite(first, 1, strlen(first), outF);
    const char* second = "-------- | ----- | --------\n";
    fwrite(second, 1, strlen(second), outF);

    std::map<std::string, std::pair<long int, char> >::const_iterator iter;
    for(iter = funcs().begin(); iter != funcs().end(); iter++) {
	std::stringstream os;
	os << iter->first << " | " << iter->second.first << " | " << iter->second.second << "\n";
	std::string o = os.str();
	fwrite(o.c_str(), 1, o.size(), outF);
    }
    fclose(outF);
    return 0;
}
