// APcoreClient.C: based originally on HelloClient.cpp, A simple xmlrpc client.
// Usage: APcoreClient serverHost serverPort
// Link against xmlrpc lib
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>
#include <sys/timeb.h>
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

static XmlRpcValue xmlTime(int sec) {
	struct tm	theTime;

	gmtime_r((const time_t *) &sec, &theTime);
	// debug
	// std::cout << "return from gmtime: Y " << theTime.tm_year
	// 	<< " M " << (theTime.tm_mon + 1) // NOTE: UNIX time starts month at 0...
	// 	<< " D " << theTime.tm_mday
	// 	<< " H " << theTime.tm_hour
	// 	<< " M " << theTime.tm_min
	// 	<< " S " << theTime.tm_sec
	// 	<< std::endl;
	return XmlRpcValue(&theTime); }

static bool shouldTerminate = true;

static int usage(const char* Pname, const char *A) {
		std::cerr << "Usage: " << Pname << A << " [serverHost serverPort] -type <act type> "
			"-name <act name> -id <act id> -start <SCET start time> [-log] [-t] [-v]\n";
		return -1; }

int ap_crash(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");

	if((!HOSTINFO) || (!PORTINFO)) {
		if(!HOSTINFO) {
			std::cerr << "host info not supplied; please set "
				"APCOREHOST or pass host/port as cmd line args\n"; }
		if(!PORTINFO) {
			std::cerr << "port info not supplied; please set "
				"APCOREPORT or pass host/port as cmd line args\n"; }
		exit(-1); }
	theClientName = HOSTINFO;
	thePortNumber = atoi(PORTINFO);

	client().execute("exit", noArgs, result);
	if(client().isFault()) {
		std::cout << "\nOops... we got a fault; error no. and string: " << result << std::endl; }
	else {
		std::cout << "\nexit result: " << result << std::endl; }

	return 0; }
