// APcoreClient.C: based originally on HelloClient.cpp, A simple xmlrpc client.
// Usage: APcoreClient serverHost serverPort
// Link against xmlrpc lib
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>

#include "UTL_time_base.H"

extern "C" {
#include <concat_util.h>
} // extern "C"

using namespace XmlRpc;

static const char *theClientName = NULL;
static int thePortNumber = -1;

static XmlRpcClient &client() {
	static XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client; }

static bool shouldTerminate = true;

void terminate_server() {
	if(!shouldTerminate) {
		return; }
	XmlRpcValue	noArgs, result;
	client().execute("APcoreTerminate", noArgs, result);
	if(client().isFault()) {
		std::cout << "\nError calling 'APcoreTerminate': " << result << "\n"; }
	else {
		std::cout << "\nAPcoreTerminate result: " << result << "\n"; } }

static int usage(const char* Pname, const char* A) {
	std::cerr << "Usage: " << Pname << A << " [serverHost serverPort]\n";
	return -1; }

int ap_terminate(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	int		hostarg = 1;

	/* usage */
	if(argc != 1 && argc != 3) {
		return usage(Pname, argv[0]); }

	// XmlRpc::setVerbosity(5);

	if(argc > hostarg) {
		theClientName = argv[hostarg];
		thePortNumber = atoi(argv[hostarg + 1]); }
	else if(HOSTINFO && PORTINFO) {
		theClientName = HOSTINFO;
		thePortNumber = atoi(PORTINFO); }
	else {
		if(!HOSTINFO) {
			std::cerr << "host info not supplied; please set "
				"APCOREHOST or pass host/port as cmd line args\n"; }
		if(!PORTINFO) {
			std::cerr << "port info not supplied; please set "
				"APCOREPORT or pass host/port as cmd line args\n"; }
		exit(-1); }
 
	terminate_server();
	return 0; }
