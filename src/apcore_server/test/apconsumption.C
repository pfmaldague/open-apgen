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
} // extern "C"

using namespace XmlRpc;

static const char *theClientName = NULL;
static int thePortNumber = -1;

static XmlRpcClient &client() {
	static XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client; }

static XmlRpcValue xmlTime(int sec) {
	struct tm	theTime;

	gmtime_r((const time_t *) &sec, &theTime);
	return XmlRpcValue(&theTime); }

static bool shouldTerminate = true;

static int usage(const char* Pname, const char *A) {
		std::cerr << "Usage: " << Pname << A << " [serverHost serverPort] [-t] [-v] [-log] -id <act id> -res <resource>\n";
		return -1; }

int ap_consumption(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	buf_struct	theStart = {NULL, 0, 0};
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	bool		gotanid = false, gotres = false;
	bool		Verbose = false, Log = false, reportTime = false;
	double		startSeconds, endSeconds;

	/* process arguments */
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-id")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-id option requires value\n";
				return -1; }
			i++;
			gotanid = true;
			Args[0] = argv[i]; }
		else if(!strcmp(argv[i], "-res")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-res option requires value\n";
				return -1; }
			i++;
			gotres = true;
			Args[1] = argv[i]; }
		else if(!strcmp(argv[i], "-v")) {
			Verbose = true; }
		else if(!strcmp(argv[i], "-log")) {
			Log = true; }
		else if(!strcmp(argv[i], "-t")) {
			reportTime = true; }
		else if(i == 1 && argc > 3) {
			theClientName = argv[i];
			i++;
			thePortNumber = atoi(argv[i]); }
		else if(!strcmp(argv[i], "-h")) {
			return usage(Pname, argv[0]); }
		else {
			return usage(Pname, argv[0]); } }
	if(!gotanid) {
		std::cerr << "No ID supplied; cannot query activity.\n";
		return -1; }
	if(!gotres) {
		std::cerr << "No resource supplied; cannot query activity.\n";
		return -1; }

	if(Verbose) {
		std::cout << "sending this message:\ngetConsumption" << Args << "\n"; }

	// XmlRpc::setVerbosity(5);

	if(!theClientName) {
		if((!HOSTINFO) || (!PORTINFO)) {
			if(!HOSTINFO) {
				std::cerr << "host info not supplied; please set "
					"APCOREHOST or pass host/port as cmd line args\n"; }
			if(!PORTINFO) {
				std::cerr << "port info not supplied; please set "
					"APCOREPORT or pass host/port as cmd line args\n"; }
			exit(-1); }
		theClientName = HOSTINFO;
		thePortNumber = atoi(PORTINFO); }

	if(reportTime) {
		struct timeb T;
		ftime(&T);
		startSeconds = (double) T.time;
		startSeconds += 0.001 * (double) T.millitm; }
	client().execute("getConsumption", Args, result);
	if(client().isFault()) {
		std::cout << "\nError calling 'getConsumption': " << result << "\n"; }
	else {
		if(reportTime) {
			struct timeb T;
			ftime(&T);
			endSeconds = (double) T.time;
			endSeconds += 0.001 * (double) T.millitm;
			std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
		std::cout << "\ngetConsumption result: " << result << std::endl; }

	if(Log) {
		if(client().execute("getLogMessage", noArgs, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl; }
		else {
			std::cout << "\nError calling 'getLogMessage'\n\n"; } }
	return 0; }
