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

static const char *theClientName = NULL;
static int thePortNumber = -1;

static XmlRpcClient &client() {
	static XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client; }

static bool shouldTerminate = true;

static int usage(const char* Pname, const char *A) {
	std::cerr << "Usage: " << Pname << A << " [-v]\n";
	return -1; }

int ap_checkplan(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	bool		weHaveAlibudef = false;
	buf_struct	theLibudef = {NULL, 0, 0};
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	int		hostarg = 1;
	bool		Verbose = false;

	/* usage */
	if(argc == 2 && (!strcmp(argv[1], "-h"))) {
		return usage(Pname, argv[0]); }
	else if(argc == 2 && (!strcmp(argv[1], "-v"))) {
		Verbose = true; }
	else if(argc != 1) {
		return usage(Pname, argv[0]); }

	// XmlRpc::setVerbosity(5);

	if(argc > 3) {
		return usage(Pname, argv[0]); }
	if(argc > hostarg + 1) {
		theClientName = argv[hostarg];
		thePortNumber = atoi(argv[hostarg + 1]); }
	
	if(!HOSTINFO) {
		std::cerr << "host info not supplied; please set "
			"APCOREHOST or pass host/port as cmd line args\n";
		exit(-1); }
	if(!PORTINFO) {
		std::cerr << "port info not supplied; please set "
			"APCOREPORT or pass host/port as cmd line args\n";
		exit(-1); }
	theClientName = HOSTINFO;
	thePortNumber = atoi(PORTINFO);
 
	if(Verbose) {
		std::cout << "sending this message:\ncheckPlan" << std::endl; }

	/* Call the checkPlan method */
	client().execute("checkPlan", noArgs, result);
	if(client().isFault()) {
		std::cout << "\nError calling 'checkPlan': " << result << "\n";
		return -1; }
	else {
		std::cout << "\ncheckPlan result: " << result << "\n"; }
	return 0; }
