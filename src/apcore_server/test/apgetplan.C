// APcoreClient.C: based originally on HelloClient.cpp, A simple xmlrpc client.
// Usage: APcoreClient serverHost serverPort
// Link against xmlrpc lib
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>
#include <sys/timeb.h>
// #include "APdata.H"

#include <UTL_time_base.H>
extern "C" {
#include <concat_util.h>
}

using namespace XmlRpc;

static const char *theClientName = NULL;
static int thePortNumber = -1;

static XmlRpcClient &client() {
	static XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client; }

static bool shouldTerminate = true;

static int usage(const char* Pname, const char *A) {
	std::cerr << "Usage: " << Pname << A << " [-v] [-t] [-log] [-full] [-IDs] [-start ID1 ID2 ... -end]\n";
	return -1; }

int ap_getplan(int argc, char* argv[], const char* Pname) {
	int		i = 0, n_acts = 0;
	XmlRpcValue	oneArg;
	XmlRpcValue	Args, result;
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	int		hostarg = 1;
	bool		reportTime = false, Log = false, Verbose = false, Full = false;
	bool		IDs = false, partial = false, Xml = false;
	double		startSeconds, endSeconds;
	const char*	cmd = NULL;

	while(1) {
		i++;
		if(i >= argc) break;
		if(!strcmp(argv[i], "-v")) {
			Verbose = true; }
		else if(!strcmp(argv[i], "-log")) {
			Log = true; }
		else if(!strcmp(argv[i], "-t")) {
			reportTime = true; }
		else if(!strcmp(argv[i], "-full")) {
			Full = true; }
		else if(!strcmp(argv[i], "-xml")) {
			Xml = true; }
		else if(!strcmp(argv[i], "-IDs")) {
			IDs = true; }
		else if(!strcmp(argv[i], "-start")) {
			i++;
			partial = true;
			while(1) {
				if(i >= argc) {
					return usage(Pname, argv[0]); }
				if(!strcmp(argv[i], "-end")) {
					break; }
				Args[n_acts] = argv[i];
				n_acts++;
				i++; }
			}
		else {
			return usage(Pname, argv[0]); } }

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

	if(n_acts) {
		XmlRpc::XmlRpcValue x(Args);

		Args.clear();
		Args[0] = x; }
	else {
		// redundant... defensive programming
		Args.clear(); }

	if(n_acts) {
		// Args already defined
		cmd = "getSomePlanActs"; }
	else if(IDs) {
		if(Full) {
			cmd = "getFullPlanIDs"; }
		else {
			cmd = "getPlanIDs"; } }
	else if(Xml) {
		if(Full) {
			Args[0] = "full";
			cmd = "getPlanAsXML"; }
		else {
			cmd = "getPlanAsXML"; } }
	else {
		if(Full) {
			Args[0] = "full";
			cmd = "getPlan"; }
		else {
			cmd = "getPlan"; } }

	if(Verbose) {
		std::cout << "sending this message:\n" << cmd << Args << "\n"; }

	if(n_acts) {
		client().execute(cmd, Args, result);
		if(!client().isFault()) {
			if(reportTime) {
				struct timeb T;
				ftime(&T);
				endSeconds = (double) T.time;
				endSeconds += 0.001 * (double) T.millitm;
				std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
			std::cout << "\ngetSomePlanActs result:\n" << result << std::endl; }
		else {
			std::cout << "\n Error in getSomePlanActs: " << result << "\n"; } }
	else if(IDs) {
		if(Full) {
		    client().execute(cmd, Args, result);
		    if(!client().isFault()) {
			if(reportTime) {
				struct timeb T;
				ftime(&T);
				endSeconds = (double) T.time;
				endSeconds += 0.001 * (double) T.millitm;
				std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
			std::cout << "\ngetFullPlanIDs result:\n" << result << std::endl; } }
		else {
		    client().execute(cmd, Args, result);
		    if(!client().isFault()) {
			if(reportTime) {
				struct timeb T;
				ftime(&T);
				endSeconds = (double) T.time;
				endSeconds += 0.001 * (double) T.millitm;
				std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
			std::cout << "\ngetPlanIDs result:\n" << result << std::endl; } } }
	else if(Xml) {
		if(Full) {
			client().execute(cmd, Args, result);
			if(!client().isFault()) {
				if(reportTime) {
					struct timeb T;
					ftime(&T);
					endSeconds = (double) T.time;
					endSeconds += 0.001 * (double) T.millitm;
					std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
				std::cout << "\ngetPlanAsXML(full) result:\n" << result << std::endl;}
			else {
				std::cout << "\n Error in getPlanAsXML(full): " << result << "\n"; } }
		else {
			client().execute(cmd, Args, result);
			if(!client().isFault()) {
				if(reportTime) {
					struct timeb T;
					ftime(&T);
					endSeconds = (double) T.time;
					endSeconds += 0.001 * (double) T.millitm;
					std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
				std::cout << "\ngetPlanAsXML result:\n" << result << std::endl;}
			else {
				std::cout << "\n Error in getPlanAsXML: " << result << "\n"; } } }
	else {
		if(Full) {
			client().execute(cmd, Args, result);
			if(!client().isFault()) {
				if(reportTime) {
					struct timeb T;
					ftime(&T);
					endSeconds = (double) T.time;
					endSeconds += 0.001 * (double) T.millitm;
					std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
				std::cout << "\ngetPlan(full) result:\n" << result << std::endl;}
			else {
				std::cout << "\n Error in getPlan(full): " << result << "\n"; } }
		else {
			client().execute(cmd, Args, result);
			if(!client().isFault()) {
				if(reportTime) {
					struct timeb T;
					ftime(&T);
					endSeconds = (double) T.time;
					endSeconds += 0.001 * (double) T.millitm;
					std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
				std::cout << "\ngetPlan result:\n" << result << std::endl;}
			else {
				std::cout << "\n Error in getPlan: " << result << "\n"; } } }
	
	if(Log) {
		if(client().execute("getLogMessage", Args, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl; }
		else {
			std::cout << "\nError calling 'getLogMessage'\n\n";
			return -1; } }
	return 0; }
