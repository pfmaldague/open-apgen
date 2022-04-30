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

static XmlRpcValue xmlTime(time_t sec) {
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
		std::cerr << Pname << "Usage: " << A << " [serverHost serverPort] -type <act type> "
			"-name <act name> -id <act id> -start <SCET start time> [-log] [-t] [-v]\n";
		return -1; }

int ap_create(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	buf_struct	Buf = {NULL, 0, 0};
	buf_struct	theType = {NULL, 0, 0};
	buf_struct	theName = {NULL, 0, 0};
	buf_struct	theId = {NULL, 0, 0};
	buf_struct	theStart = {NULL, 0, 0};
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	bool		Verbose = false, Log = false, reportTime = false;
	double		startSeconds, endSeconds;

	/* process arguments */
	if(argc == 1) {
		return usage(Pname, argv[0]); }
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-type")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-type option requires file name\n";
				return -1; }
			i++;
			concatenate(&theType, argv[i]); }
		else if(!strcmp(argv[i], "-v")) {
			Verbose = true; }
		else if(!strcmp(argv[i], "-h")) {
			return usage(Pname, argv[0]); }
		else if(!strcmp(argv[i], "-log")) {
			Log = true; }
		else if(!strcmp(argv[i], "-t")) {
			reportTime = true; }
		else if(!strcmp(argv[i], "-name")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-name option requires file name\n";
				return -1; }
			i++;
			concatenate(&theName, argv[i]); }
		else if(!strcmp(argv[i], "-id")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-id option requires file name\n";
				return -1; }
			i++;
			concatenate(&theId, argv[i]); }
		else if(!strcmp(argv[i], "-start")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-start option requires file name\n";
				return -1; }
			i++;
			concatenate(&theStart, argv[i]); }
		else if(i == 1 && argc > 2) {
			int j;
			theClientName = argv[i];
			i++;
			for(j = 0; j < strlen(argv[i]); j++) {
				if(!isdigit(argv[i][j])) {
					return usage(Pname, argv[0]); } }
			thePortNumber = atoi(argv[i]); }
		else {
			return usage(Pname, argv[0]); } }
	if(bfs_is_empty(&theName)) {
		std::cerr << "no name supplied.\n";
		return -1; }
	if(bfs_is_empty(&theType)) {
		std::cerr << "no type supplied.\n";
		return -1; }
	if(bfs_is_empty(&theId)) {
		std::cerr << "no Id supplied.\n";
		return -1; }
	if(bfs_is_empty(&theStart)) {
		std::cerr << "no start supplied.\n";
		return -1; }

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

	CTime_base start_time(theStart.buf);
	Args["type"] = theType.buf;
	Args["name"] = theName.buf;
	Args["start"] = xmlTime((time_t) start_time.get_seconds());
	Args["id"] = theId.buf;
	oneArg[0] = Args;
	if(Verbose) {
		std::cout << "sending this message:\ncreateActivity" << oneArg << "\n"; }
	if(reportTime) {
		struct timeb T;
		ftime(&T);
		startSeconds = (double) T.time;
		startSeconds += 0.001 * (double) T.millitm; }
	client().execute("createActivity", oneArg, result);
	if(reportTime) {
		struct timeb T;
		ftime(&T);
		endSeconds = (double) T.time;
		endSeconds += 0.001 * (double) T.millitm;
		std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
	if(client().isFault()) {
		std::cout << "\nOops... we got a fault; error no. and error string: " << result << "\n"; }
	else {
		std::cout << "\ncreateActivity result: " << result << std::endl; }

	if(Log) {
		if(client().execute("getLogMessage", noArgs, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl; }
		else {
			std::cout << "\nError calling 'getLogMessage'\n\n"; } }
	return 0; }
