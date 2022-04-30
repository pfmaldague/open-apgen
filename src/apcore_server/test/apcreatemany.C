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
	// 	<< " M " << theTime.tm_mon
	// 	<< " D " << theTime.tm_mday
	// 	<< " H " << theTime.tm_hour
	// 	<< " M " << theTime.tm_min
	// 	<< " S " << theTime.tm_sec
	// 	<< std::endl;
	return XmlRpcValue(&theTime); }

static bool shouldTerminate = true;

static int usage(const char* Pname, const char *A) {
		std::cerr << "Usage: " << Pname << A << " [serverHost serverPort] -type <act type> "
			"-name <act name> -id <act id> -start <SCET start time> -delay <duration> -n <number>\n";
		return -1; }

int ap_createmany(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, result;
	buf_struct	Buf = {NULL, 0, 0};
	buf_struct	theType = {NULL, 0, 0};
	buf_struct	theName = {NULL, 0, 0};
	buf_struct	theId = {NULL, 0, 0};
	buf_struct	theStart = {NULL, 0, 0};
	buf_struct	theDelay = {NULL, 0, 0};
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	bool		Verbose = false, Log = false, reportTime = false;
	double		startSeconds, endSeconds;
	int		N, num = 0;

	/* process arguments */
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-type")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-type option requires file name\n";
				return -1; }
			i++;
			concatenate(&theType, argv[i]); }
		else if(!strcmp(argv[i], "-v")) {
			Verbose = true; }
		else if(!strcmp(argv[i], "-log")) {
			Log = true; }
		else if(!strcmp(argv[i], "-t")) {
			reportTime = true; }
		else if(!strcmp(argv[i], "-name")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-name option requires name string\n";
				return -1; }
			i++;
			concatenate(&theName, argv[i]); }
		else if(!strcmp(argv[i], "-id")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-id option requires ID string\n";
				return -1; }
			i++;
			concatenate(&theId, argv[i]); }
		else if(!strcmp(argv[i], "-start")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-start option requires time string\n";
				return -1; }
			i++;
			concatenate(&theStart, argv[i]); }
		else if(!strcmp(argv[i], "-delay")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-delay option requires duration string\n";
				return -1; }
			i++;
			concatenate(&theDelay, argv[i]); }
		else if(!strcmp(argv[i], "-n")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-n option requires integer argument\n";
				return -1; }
			i++;
			sscanf(argv[i], "%d", &num); }
		else if(i == 1) {
			theClientName = argv[i];
			i++;
			thePortNumber = atoi(argv[i]); }
		else {
			return usage(Pname, argv[0]); } }
	if(bfs_is_empty(&theName)) {
		std::cerr << "no name supplied.\n";
		return usage(Pname, argv[0]); }
	if(bfs_is_empty(&theType)) {
		std::cerr << "no type supplied.\n";
		return usage(Pname, argv[0]); }
	if(bfs_is_empty(&theId)) {
		std::cerr << "no Id supplied.\n";
		return usage(Pname, argv[0]); }
	if(bfs_is_empty(&theStart)) {
		std::cerr << "no start supplied.\n";
		return usage(Pname, argv[0]); }
	if(bfs_is_empty(&theDelay)) {
		std::cerr << "no delay supplied.\n";
		return usage(Pname, argv[0]); }
	if(!num) {
		std::cerr << "no number supplied.\n";
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

	if(reportTime) {
		struct timeb T;
		ftime(&T);
		startSeconds = (double) T.time;
		startSeconds += 0.001 * (double) T.millitm; }
	for(N = 0; N < num; N++) {
		XmlRpcValue	Args;
		static buf_struct	theFullId = {NULL, 0, 0};
		static buf_struct	theFullName = {NULL, 0, 0};
		CTime_base		start_time(theStart.buf);
		CTime_base		delay_time(theDelay.buf);
		char			theCount[100];

		sprintf(theCount, "%d", N);
		initialize_buf_struct(&theFullId);
		concatenate(&theFullId, theId.buf);
		concatenate(&theFullId, "_");
		concatenate(&theFullId, theCount);
		initialize_buf_struct(&theFullName);
		concatenate(&theFullName, theName.buf);
		concatenate(&theFullName, "_");
		concatenate(&theFullName, theCount);
		Args["type"] = theType.buf;
		Args["name"] = theFullName.buf;
		Args["start"] = xmlTime(start_time.get_seconds() + N * delay_time.get_seconds());
		Args["id"] = theFullId.buf;
		if(Verbose) {
			std::cout << "sending this message: " << oneArg << std::endl; }
		client().execute("createActivity", Args, result);
		if(client().isFault()) {
			std::cout << "\nError calling 'createActivity': " << result << "\n"; }
		else {
			std::cout << "\ncreateActivity result: " << result << std::endl; } }
	if(reportTime) {
		struct timeb T;
		ftime(&T);
		endSeconds = (double) T.time;
		endSeconds += 0.001 * (double) T.millitm;
		std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }

	if(Log) {
		if(client().execute("getLogMessage", noArgs, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl; }
		else {
			std::cout << "\nError calling 'getLogMessage'\n\n"; } }
	return 0; }
