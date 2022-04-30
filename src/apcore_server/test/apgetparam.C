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

static int usage(const char* Pname, const char *A) {
	std::cerr << "Usage: " << A << " [-v] [-t] [-log] -id <act ID> -param <param name>\n"
		  << "  or\n"
		  << "       " << A << " [-v] [-t] [-log] -all -type <type name>\n";
	return -1; }

int ap_getparam(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	int		hostarg = 1;
	bool		reportTime = false, Log = false, Verbose = false;
	double		startSeconds, endSeconds;
	bool		gotID = false, gotParam = false, all = false, gotType = false;
	int		argnum = 0;
	const char*	cmd = NULL;

	/* usage */
	if(argc == 1) {
		return usage(Pname, argv[0]); }
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-v")) {
			Verbose = true; }
		else if(!strcmp(argv[i], "-log")) {
			Log = true; }
		else if(!strcmp(argv[i], "-t")) {
			reportTime = true; }
		else if(!strcmp(argv[i], "-all")) {
			all = true; }
		else if(!strcmp(argv[i], "-id")) {
			if(argc < i + 2 || argnum != 0) {
				return usage(Pname, argv[0]); }
			gotID = true;
			i++;
			Args[argnum++] = argv[i]; }
		else if(!strcmp(argv[i], "-type")) {
			if(argc < i + 2 || argnum != 0) {
				return usage(Pname, argv[0]); }
			gotType = true;
			i++;
			Args[argnum++] = argv[i]; }
		else if(!strcmp(argv[i], "-param")) {
			if(argc < i + 2 || argnum != 1) {
				return usage(Pname, argv[0]); }
			gotParam = true;
			i++;
			Args[argnum++] = argv[i]; }
		else if(i == 1 && argc >= 3) {
			theClientName = argv[i];
			i++;
			thePortNumber = atoi(argv[i]); }
		else {
			return usage(Pname, argv[0]); } }

	if((gotID && (!gotParam)) || (all && (!gotType)) || (gotID && all) || !(gotID || all)) {
		return usage(Pname, argv[0]); }

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

	if(all) {
		cmd = "getActivityParameters"; }
	else {
		cmd = "getActivityInstanceParameterValue"; }

	if(Verbose) {
		std::cout << "sending this message:\n" << cmd << Args << "\n"; }
	if(reportTime) {
		struct timeb T;
		ftime(&T);
		startSeconds = (double) T.time;
		startSeconds += 0.001 * (double) T.millitm; }
	client().execute(cmd, Args, result);
	if(!client().isFault()) {
		if(reportTime) {
			struct timeb T;
			ftime(&T);
			endSeconds = (double) T.time;
			endSeconds += 0.001 * (double) T.millitm;
			std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
		std::cout << "\n" << cmd << "result:\n" << result << std::endl;}
	else {
		std::cout << "\n Error in " << cmd << ":\n" << result << std::endl;}

	if(Log) {
		if(client().execute("getLogMessage", noArgs, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl; }
		else {
			std::cout << "\nError calling 'getLogMessage'\n\n";
			return -1; } }
	return 0; }
