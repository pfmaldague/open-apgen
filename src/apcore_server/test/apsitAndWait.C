
// apsitAndWait.C: based originally on HelloClient.cpp, A simple xmlrpc client.
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>

#include "UTL_time_base.H"

extern "C" {
#include <concat_util.h>
} // extern "C"

using namespace XmlRpc;

static const char*	theClientName = NULL;
static int		thePortNumber = -1;

static XmlRpcClient &client() {
	static XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client; }

static int usage(const char* Pname, const char* A) {
	std::cerr << "Usage: " << Pname << A << " <seconds-to-wait>]\n";
	return -1; }

int ap_sitAndWait(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, result;
	char*		HOSTINFO = getenv("APCOREHOST");
	char*		PORTINFO = getenv("APCOREPORT");
	bool		got_host = false;
	bool		got_port = false;
	long		num_seconds = 0;

	/* usage */
	if(argc != 2) {
		return usage(Pname, argv[0]); }
	num_seconds = atoi(argv[1]);

	// XmlRpc::setVerbosity(5);

	if(!HOSTINFO) {
		std::cerr << "host info not supplied; please set "
			"APCOREHOST or pass host/port as cmd line args\n"; }
	else {
		got_host = true; }
	if(!PORTINFO) {
		std::cerr << "port info not supplied; please set "
			"APCOREPORT or pass host/port as cmd line args\n"; }
	else {
		got_port = true; }
	if((!got_port) || (!got_host)) {
		return -1; }

	theClientName = HOSTINFO;
	thePortNumber = atoi(PORTINFO);
 
	oneArg = num_seconds;
	if(client().execute("sitAndWait", oneArg, result)) {
		std::cout << "Result of sitAndWait: " << result << "\n"; }
	else {
		std::cerr << "apsitAndWait: error calling sitAndWait " << num_seconds << "\n";
		return -2; }
	return 0; }
