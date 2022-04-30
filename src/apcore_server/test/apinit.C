// APcoreClient.C: based originally on HelloClient.cpp, A simple xmlrpc client.
// Usage: APcoreClient serverHost serverPort
// Link against xmlrpc lib
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>

#include <UTL_time_base.H>

extern "C" {
#include <concat_util.h>
} // extern "C"

using namespace XmlRpc;

static const char*	theClientName = NULL;
static int		thePortNumber = -1;

static XmlRpcClient &client() {
	static XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client; }

static bool shouldTerminate = true;

extern void terminate_server();

static int init_usage(const char* Pname, const char* A) {
	std::cerr << "Usage: " << Pname << A << " [-v] [-h] [-libudef <udef file>] [... any valid apgen option]\n";
	return -1; }

int ap_init(int argc, char* argv[], const char* prog_name) {
	int		i = 0;
	int		k = 0;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	bool		weHaveAlibudef = false;
	buf_struct	theLibudef = {NULL, 0, 0};
	bool		weHaveAtimeout = false;
	bool		Verbose = false;
	string		theTimeout;
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	int		hostarg = 1;

	// XmlRpc::setVerbosity(5);

	for(k = 1; k < argc; k++) {
		bool	skip = false;

		if(!strcmp(argv[k], "-h")) {
			return init_usage(prog_name, argv[0]); }
		else if(!strcmp(argv[k], "-v")) {
			skip = true;
			Verbose = true; }
		if(!strcmp(argv[k], "-libudef")) {
			weHaveAlibudef = true; }
		else if(!strcmp(argv[k], "-timeout")) {
			weHaveAtimeout = true; }
		if(!skip) {
			Args[i++] = argv[k]; } }
	if(!theClientName) {
		if(HOSTINFO && PORTINFO) {
			theClientName = HOSTINFO;
			thePortNumber = atoi(PORTINFO); }
		else {
			if(!HOSTINFO) {
				std::cerr << "host info not supplied; please set "
					"APCOREHOST or pass host/port as cmd line args\n"; }
			if(!PORTINFO) {
				std::cerr << "port info not supplied; please set "
					"APCOREPORT or pass host/port as cmd line args\n"; }
			exit(-1); } }
 
	/* Call the Init method */
	// Args[i++] = "-noconstraints";
	Args[i++] = "-noremodel";

	if(Verbose) {
		std::cout << "sending this message:\nAPcoreInit" << Args << std::endl; }
	client().execute("APcoreInit", Args, result);
	if(client().isFault()) {
		std::cout << "\nAPcoreInit error: " << result << "\n"; }
	else {
		std::cout << "\nAPcoreInit result: " << result << "\n"; }
	return 0; }
