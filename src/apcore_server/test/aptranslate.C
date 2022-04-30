// APcoreClient.C: based originally on HelloClient.cpp, A simple xmlrpc client.
// Usage: APcoreClient serverHost serverPort
// Link against xmlrpc lib
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>
#include <sys/timeb.h>

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
	// debug
	// std::cout << "return from gmtime: Y " << theTime.tm_year
	 // 	<< " M " << theTime.tm_mon
	 // 	<< " D " << theTime.tm_mday
	 // 	<< " H " << theTime.tm_hour
	 // 	<< " M " << theTime.tm_min
	 // 	<< " S " << theTime.tm_sec
	 // 	<< std::endl;
	return XmlRpcValue(&theTime); }

static int usage(const char*Pname, const char* A) {
		std::cerr << "Usage: " << Pname << A
			<< " [-t] [-v] [-log] [-aaf] <input file name> <mission name>\n";
		return -1; }

int ap_translate(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	buf_struct	Buf = {NULL, 0, 0};
	bool		weHaveAnAaf = false;
	bool		weHaveAMissionName = false;
	const char*	theMissionName = NULL;
	char		c, b[2];
	int		seconds = -1;
	FILE		*theAaf = NULL, *theApf = NULL;
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	bool		Verbose = false, Log = false;
	bool		reportTime = false;
	double		startSeconds, endSeconds;

	b[1] = '\0';

	if(HOSTINFO && PORTINFO) {
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

	/* 1. usage */
	if(argc < 3) {
		return usage(Pname, argv[0]); }

	/* 2. process arguments */
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-aaf")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-aaf option requires file name\n";
				return -1; }
			i++;
			theAaf = fopen(argv[i], "r");
			if(!theAaf) {
				std::cerr << "cannot open input file " << argv[i] << "\n";
				return -1; }
			weHaveAnAaf = true; }
		else if(!strcmp(argv[i], "-v")) {
			Verbose = true; }
		else if(!strcmp(argv[i], "-log")) {
			Log = true; }
		else if(!strcmp(argv[i], "-t")) {
			reportTime = true; }
		else if(!weHaveAnAaf) {
			// assume it's a file name
			theAaf = fopen(argv[i], "r");
			if(!theAaf) {
				std::cerr << "cannot open input file " << argv[i] << "\n";
				return -1; }
			weHaveAnAaf = true; }
		else {
			// assume it's a mission name
			weHaveAMissionName = true;
			theMissionName = argv[i]; } }

	// XmlRpc::setVerbosity(5);

	if(!weHaveAnAaf) {
		return usage(Pname, argv[0]); }
	if(!weHaveAMissionName) {
		return usage(Pname, argv[0]); }
	initialize_buf_struct(&Buf);
	while(fread(&c, 1, 1, theAaf)) {
		b[0] = c;
		concatenate(&Buf, b); }
	fclose(theAaf);
	oneArg[0] = Buf.buf;
	oneArg[1] = theMissionName;

	if(Verbose) {
		std::cout << "sending this message:\ncreateDictionary" << oneArg << std::endl; }
	if(reportTime) {
		struct timeb T;
		ftime(&T);
		startSeconds = (double) T.time;
		startSeconds += 0.001 * (double) T.millitm; }
	client().execute("createDictionary", oneArg, result);
	if(client().isFault()) {
		std::cout << "\nError calling 'createDictionary': " << result << "\n"; }
	else {
		if(reportTime) {
			struct timeb T;
			ftime(&T);
			endSeconds = (double) T.time;
			endSeconds += 0.001 * (double) T.millitm;
			std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
		std::cout << "\ncreateDictionary result: " << result << std::endl; }

	if(Log) {
		if(client().execute("getLogMessage", noArgs, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl; }
		else {
			std::cout << "\nError calling 'getLogMessage'\n\n"; } }
	return 0; }
