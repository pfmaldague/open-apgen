// apsystem.C: based originally on HelloClient.cpp, A simple xmlrpc client.
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
	std::cerr << "Usage: " << Pname << A << " [help <method-name>]\n";
	return -1; }

int ap_system(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, result;
	char*		HOSTINFO = getenv("APCOREHOST");
	char*		PORTINFO = getenv("APCOREPORT");
	bool		got_host = false;
	bool		got_port = false;
	bool		help_requested = false;
	const char*	method_name = NULL;

	/* usage */
	if(argc != 1 && argc != 3) {
		return usage(Pname, argv[0]); }
	if(argc == 3) {
		if(strcmp(argv[1], "help")) {
			return usage(Pname, argv[0]); }
		help_requested = true;
		method_name = argv[2]; }

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
 
	if(!help_requested) {
		// list all methods
		if(client().execute("system.listMethods", noArgs, result)) {
			std::cout << "Methods:\n" << result << "\n"; }
		else {
			std::cerr << "apsystem: error calling system.listMethod\n";
			return -2; } }
	else {
		// get help for specified method
		oneArg = method_name;
		if(client().execute("system.methodHelp", oneArg, result)) {
			std::cout << "Help for method " << method_name << ":\n" << result << "\n"; }
		else {
			std::cerr << "apsystem: error calling system.methodHelp" << method_name << "\n";
			return -2; } }
	return 0; }
