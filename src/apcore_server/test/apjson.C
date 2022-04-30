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
	std::cerr << "Usage: " << Pname << A << " <method-name> [<argument 1> [<argument 2>]]\n"
		<< "where <method-name> is one of the following:\n"
		<< "\tGetAllAttributesInJson\n"
		<< "\tGetAttrHistoryInJson\n"
		<< "\tGetActLegendsInJson\n"
		<< "\tGetEventHistoryInJson\n";
	std::cerr << "If method-name is GetAttrHistoryInJson, multiple pairs of arguments are OK.\n";
	return -1; }

int ap_json(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	Args, result;
	char*		HOSTINFO = getenv("APCOREHOST");
	char*		PORTINFO = getenv("APCOREPORT");
	bool		got_host = false;
	bool		got_port = false;
	const char*	method_name = NULL;

	/* usage */
	if(argc < 2) {
		return usage(Pname, argv[0]); }
	method_name = argv[1];
	if(argc > 4 && strcmp(method_name, "GetAttrHistoryInJson")) {
		return usage(Pname, argv[0]); }

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

	if(!strcmp(method_name, "GetAllAttributesInJson")) {
		if(argc != 2) {
			return usage(Pname, argv[0]); }
		// no arguments
		}
	else if(!strcmp(method_name, "GetActLegendsInJson")) {
		if(argc != 2) {
			return usage(Pname, argv[0]); }
		// no arguments
		}
	else if(!strcmp(method_name, "GetAttrHistoryInJson")) {
		for(int j = 1; j < argc/2; j++) {
			int n = 2 * j;
			// each pair of arguments: subsystem, resource name (watch out for arrayed resources!
			Args[n - 2] = argv[n];
			if(++n == argc) {
				return usage(Pname, argv[0]); }
			Args[n - 2] = argv[n]; } }
	else if(!strcmp(method_name, "GetEventHistoryInJson")) {
		if(argc != 3) {
			return usage(Pname, argv[0]); }
		// one argument: legend
		Args[0] = argv[2]; }
	else {
		return usage(Pname, argv[0]); }
	

	try {
		if(client().execute(method_name, Args, result)) {
			std::cout << "Result:\n" << result << "\n"; }
		else {
			throw(eval_error(((std::string) result).c_str())); } }
	catch(eval_error Err) {
		std::cerr << "ap_json: error calling " << method_name << ": " << result << "\n";
		return -2; }
 
	return 0; }
