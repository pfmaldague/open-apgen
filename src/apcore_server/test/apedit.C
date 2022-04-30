// APcoreClient.C: based originally on HelloClient.cpp, A simple xmlrpc client.
// Usage: APcoreClient serverHost serverPort
// Link against xmlrpc lib
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>
#include <sys/timeb.h>

#include "UTL_time_base.H"
extern "C" {
// #include <gdome_wrapper.h>
// extern int xmparse();
#include <concat_util.h>
}

extern bool ap_verbose;

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
	std::cerr << "Usage: " << Pname << A << " [-t] [-log] -id <act id> [ - | -arg <xmlrpc struct>]\n";
	std::cerr << "       If option '-' is used, command line should be followed by ^D-terminated expression.\n";
	std::cerr << "       NOTE: the struct must contain kwd-value pairs where the values are string\n";
	std::cerr << "             representations of the argument values.\n";
	return -1; }


int ap_edit(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	theStruct, noArgs, Args, result;
	buf_struct	theStart = {NULL, 0, 0};
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	int		noargs = 0;
	int		ret;
	bool		gotanid = false;
	bool		Log = false, reportTime = false;
	double		startSeconds, endSeconds;
	bool		std_input = false;
	buf_struct	In = {NULL, 0, 0};
	// GdomeDocument*	theDoc;
	// GdomeElement*	theEl;

	/* process arguments */
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-id")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-id option requires value\n";
				return -1; }
			i++;
			gotanid = true;
			Args[0] = argv[i]; }
		else if(!strcmp(argv[i], "-v")) {
			ap_verbose = true; }
		else if(!strcmp(argv[i], "-h")) {
			return usage(Pname, argv[0]); }
		else if(!strcmp(argv[i], "-log")) {
			Log = true; }
		else if(!strcmp(argv[i], "-t")) {
			reportTime = true; }
		else if(!strcmp(argv[i], "-arg")) {
			if(i == argc - 1) {
				return usage(Pname, argv[0]); }
			noargs++;
			concatenate(&In, argv[++i]); }
		else if(!strcmp(argv[i], "-")) {
			noargs++;
			std_input = true; }
		else
			return usage(Pname, argv[0]); }
	if(!gotanid) {
		std::cerr << "No ID supplied; cannot edit activity.\n";
		return -1; }
	if(!noargs) {
		std::cerr << "No keyword-value pairs supplied; cannot edit activity.\n";
		return -1; }
	if(std_input) {
		char byte[2];
		int c;
		byte[1] = '\0';
		while((c = getchar()) != EOF) {
			byte[0] = c;
			concatenate(&In, byte); } }

	// initialize_xml_system(&In, &theDoc, &theEl);
	// xmparse builds an XML DOM object as a tree with theEl as its root
	// ret = xmparse();

	// if(ap_verbose) {
	// 	printf("Return from xmparse() = %d\n", ret); }
	destroy_buf_struct(&In);
	if(ret) {
		std::cerr << "Error found while parsing arguments.\n";
		return -1; }
	/* no errors */

	// debug:
	// saveAPDOM("XmlRpcValue.xml", theDoc);

	// converts the XML DOM tree into an XmlRpc structure
	// build_xmlrpc_object(theDoc, theEl, theStruct);

	// deletes all the XML DOC structures
	// cleanupDocument(&theDoc, &theEl);
	// cleanupImplementation();

	// getting ready to send the structure to the server
	Args[1] = theStruct;

	if(ap_verbose) {
		std::cout << "sending this message:\neditActivity" << Args << "\n"; }

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
	client().execute("editActivity", Args, result);
	if(!client().isFault()) {
		if(reportTime) {
			struct timeb T;
			ftime(&T);
			endSeconds = (double) T.time;
			endSeconds += 0.001 * (double) T.millitm;
			std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
		std::cout << "\neditActivity result: " << result << std::endl; }
	else {
		std::cout << "\nError calling 'editActivity': " << result << "\n"; }

	if(Log) {
		if(client().execute("getLogMessage", noArgs, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl; }
		else {
			std::cout << "\nError calling 'getLogMessage'\n\n"; } }
	return 0; }

static int usage2(const char* Pname, const char *A) {
	std::cerr << "Usage: " << Pname << A << " [-t] [-log] -id <act id>\n";
	return -1; }

int ap_editableduration(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	theStruct, noArgs, Args, result;
	buf_struct	theStart = {NULL, 0, 0};
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	int		ret;
	bool		gotanid = false;
	bool		Log = false, reportTime = false;
	double		startSeconds, endSeconds;
	// GdomeDocument*	theDoc;
	// GdomeElement*	theEl;

	/* process arguments */
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-id")) {
			if((i == argc - 1) || (argv[i+1][0] == '-')) {
				std::cerr << "-id option requires value\n";
				return -1; }
			i++;
			gotanid = true;
			Args[0] = argv[i]; }
		else if(!strcmp(argv[i], "-v")) {
			ap_verbose = true; }
		else if(!strcmp(argv[i], "-log")) {
			Log = true; }
		else if(!strcmp(argv[i], "-t")) {
			reportTime = true; }
		else
			return usage2(Pname, argv[0]); }
	if(!gotanid) {
		std::cerr << "No ID supplied; cannot edit activity.\n";
		return -1; }

	if(ap_verbose) {
		std::cout << "sending this message:\nactDurIsEditable" << Args << std::endl; }

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
	client().execute("actDurIsEditable", Args, result);
	if(!client().isFault()) {
		if(reportTime) {
			struct timeb T;
			ftime(&T);
			endSeconds = (double) T.time;
			endSeconds += 0.001 * (double) T.millitm;
			std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
		std::cout << "\neditActivity result: " << result << std::endl; }
	else {
		std::cout << "\nError calling 'editActivity': " << result << "\n"; }

	if(Log) {
		if(client().execute("getLogMessage", noArgs, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl; }
		else {
			std::cout << "\nError calling 'getLogMessage'\n\n"; } }
	return 0; }
