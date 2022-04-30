#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>	/* for printf etc. */
#include <string.h>	/* for strlen() etc. */
#include <stdlib.h> 	/* for malloc() */
#include <iostream>

#include <XmlRpc.H>

static const char *theClientName = NULL;
static int thePortNumber = -1;

static XmlRpc::XmlRpcClient& client() {
	static XmlRpc::XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client;
}

int usage(const char* s) {
	int i;
	printf("Usage: %s [-v] <cmd>\n", s);
	return -1;
}

int main(int argc, char* argv[]) {
	XmlRpc::XmlRpcValue	Args, result;
	char*			prog_name;
	char*			HOSTINFO = getenv("APGENHOST");
	char*			PORTINFO = getenv("APGENPORT");
	int			i = 1;

	if((!HOSTINFO) || (!PORTINFO)) {
		if(!HOSTINFO) {
			std::cerr << "host info not supplied; please set APGENHOST\n";
		}
		if(!PORTINFO) {
			std::cerr << "port info not supplied; please set APGENPORT\n";
		}
		exit(-1);
	}
	theClientName = HOSTINFO;
	thePortNumber = atoi(PORTINFO);
	prog_name = argv[0];
	if(argc == 1) {
		return usage(prog_name);
	}
	if(!strcmp(argv[1], "-v")) {
		i++;
		XmlRpc::setVerbosity(5);
	}
	if(argc == i) {
		return usage(prog_name);
	}
	if(!strcmp(argv[i], "next")) {
		client().execute("next", Args, result);
	}
	if(!client().isFault()) {
		std::cout << result << "\n";
		return 0;
	} else {
		std::cout << "\n Error in next: " << result << "\n";
		return -1;
	}
}
