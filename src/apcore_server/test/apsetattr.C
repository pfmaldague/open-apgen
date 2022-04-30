#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>
#include <sys/timeb.h>

#include "UTL_time_base.H"
extern "C" {
#include <concat_util.h>
extern int xmparse();
}

extern bool ap_verbose;

// extern void initialize_xml_system(buf_struct*, GdomeDocument**, GdomeElement**);
// extern void build_xmlrpc_object(GdomeDocument*, GdomeElement*, XmlRpc::XmlRpcValue&);

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
	// print_tm(theTime);
	// cout << endl;
	return XmlRpcValue(&theTime); }


static int usage(const char* Pname, const char *A) {
	std::cerr << "Usage: " << Pname << A << " [serverHost serverPort] [-v] [-t] [-log] -id <act ID> "
		"-attr <attr name> -val attvalue\n"
		"    where attvalue is a string that can be parsed into an XmlRpc structure\n";
	return -1; }

int ap_setattr(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg, theXmlRpcExpression;
	XmlRpcValue	noArgs, Args, result;
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	bool		reportTime = false, Log = false, Verbose = false;
	double		startSeconds, endSeconds;
	bool		gotID = false, gotAttr = false, gotValue = false;
	bool		isFloat = false, isTime = false, isDuration = false, isList = false,
				isStruct = false, isInteger = false, isBoolean = false;
	bool		typeSpec = false;
	int		argnum = 0;
	buf_struct	In = {NULL, 0, 0};
	// GdomeDocument*	theDoc;
	// GdomeElement*	theEl;
	int		ret = 0;

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
		else if(!strcmp(argv[i], "-id")) {
			if(argc < i + 2 || argnum != 0) {
				return usage(Pname, argv[0]); }
			gotID = true;
			i++;
			Args[argnum++] = argv[i]; }
		else if(!strcmp(argv[i], "-attr")) {
			if(argc < i + 2 || argnum != 1) {
				return usage(Pname, argv[0]); }
			gotAttr = true;
			i++;
			Args[argnum++] = argv[i]; }
		else if(!strcmp(argv[i], "-val")) {
			if(argc < i + 1) {
				return usage(Pname, argv[0]); }
			gotValue = true;
			i++;
			concatenate(&In, argv[i]); }
		else {
			return usage(Pname, argv[0]); } }
	if((!gotID) || (!gotAttr) || (!gotValue)) {
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

	// initialize_xml_system(&In, &theDoc, &theEl);
	// xmparse builds an XML DOM object as a tree with theEl as its root
	// ret = xmparse();

	// if(ap_verbose) {
	// 	printf("Return from xmparse() = %d\n", ret); }
	// destroy_buf_struct(&In);
	// if(ret) {
	// 	std::cerr << "Error found while parsing arguments.\n";
	// 	return -1; }
	/* no errors */

	// debug:
	// saveAPDOM("XmlRpcValue.xml", theDoc);

	// converts the XML DOM tree into an XmlRpc structure
	// build_xmlrpc_object(theDoc, theEl, theXmlRpcExpression);

	// deletes all the XML DOC structures
	// cleanupDocument(&theDoc, &theEl);
	// cleanupImplementation();

	// getting ready to send the structure to the server
	Args[argnum++] = theXmlRpcExpression;

	if(Verbose) {
		std::cout << "sending this message: " << Args << std::endl; }
	if(reportTime) {
		struct timeb T;
		ftime(&T);
		startSeconds = (double) T.time;
		startSeconds += 0.001 * (double) T.millitm; }
	client().execute("setActivityInstanceAttributeValue", Args, result);
	if(!client().isFault()) {
		if(reportTime) {
			struct timeb T;
			ftime(&T);
			endSeconds = (double) T.time;
			endSeconds += 0.001 * (double) T.millitm;
			std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl; }
		std::cout << "\nsetActivityInstanceAttributeValue result:\n" << result << std::endl;}
	else {
		std::cout << "\n Error in setActivityInstanceAttributeValue:\n" << result << std::endl;}

	if(Log) {
		if(client().execute("getLogMessage", noArgs, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl; }
		else {
			std::cout << "\nError calling 'getLogMessage'\n\n";
			return -1; } }
	return 0; }
