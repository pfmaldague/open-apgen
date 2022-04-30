// APcoreClient.C: based originally on HelloClient.cpp, A simple xmlrpc client.
// Usage: APcoreClient serverHost serverPort
// Link against xmlrpc lib
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>
#include "UTL_time_base.H"

extern "C" {
#include <concat_util.h>
}

using namespace XmlRpc;

static const char*	theClientName = NULL;
static int		thePortNumber = -1;

static XmlRpcClient&	client() {
	static XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client; }

static bool shouldTerminate = true;

static int usage(const char* Pname, const char *A) {
	std::cerr << "Usage: " << Pname << A << " [-v]\n";
	return -1; }

int ap_spice(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	bool		weHaveAlibudef = false;
	buf_struct	theLibudef = {NULL, 0, 0};
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	bool		Verbose = false;
	int		start_here = 1;

	/* usage */
	if(argc == 1) {
		; /* no args */ }
	else if(argc == 2 && (!strcmp(argv[1], "-h"))) {
		// help request
		return usage(Pname, argv[0]); }
	else if(argc >= 2 && (!strcmp(argv[1], "-v"))) {
		Verbose = true;
		start_here = 2;}

	// XmlRpc::setVerbosity(5);

	if(!HOSTINFO) {
		std::cerr << "host info not supplied; please set APCOREHOST\n";
		exit(-1); }
	if(!PORTINFO) {
		std::cerr << "port info not supplied; please set APCOREPORT\n";
		exit(-1); }
	theClientName = HOSTINFO;
	thePortNumber = atoi(PORTINFO);
 
	if(Verbose) {
		std::cout << "sending this message:\n" << argv[0] << "\n"; }

	// need to convert all args

	/* Call the checkPlan method */
	client().execute("checkPlan", noArgs, result);
	if(client().isFault()) {
		std::cout << "\nError calling 'checkPlan': " << result << "\n";
		return -1; }
	else {
		std::cout << "\ncheckPlan result: " << result << "\n"; }
	return 0; }
