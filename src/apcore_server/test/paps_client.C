// APcoreClient.C: based originally on HelloClient.cpp, A simple xmlrpc client.
// Usage: APcoreClient serverHost serverPort
// Link against xmlrpc lib
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>
#include "UTL_time_base.H"
using namespace XmlRpc;

static const char *theHostName = NULL;
static int thePortNumber = -1;

XmlRpcClient &client() {
	static XmlRpcClient a_client(theHostName, thePortNumber);
	return a_client; }

// simple string-handling utilities

typedef struct _buf_struct {
	char	*buf;
	int	bufsize;
	int	stringL; } buf_struct;

int	is_empty(buf_struct *buffer) {
	if((!buffer) || (!buffer->buf) || (!buffer->buf[0])) {
		return 1; }
	return 0; }

void initialize_buf_struct(buf_struct *B) {
	if(B->bufsize) {
		B->buf[0] = '\0';
		B->stringL = 0; } }

void destroy_buf_struct(buf_struct *B) {
	if(B->buf) {
		free(B->buf);
		B->buf = NULL; }
	B->bufsize = 0;
	B->stringL = 0; }

void concatenate(buf_struct *B, const char *addendum) {
	char	*buffer = B->buf;
	int	l;

	if(!buffer) {
		B->bufsize = 120;
		B->buf = (char *) malloc(120);
		B->stringL = 0;
		B->buf[0] = '\0'; }
	if(!addendum) {
		return; }
	l = strlen(addendum);
	if(B->bufsize < B->stringL + l + 1) {
		B->bufsize = 2 * B->bufsize + l + 1;
		B->buf = (char *) realloc(B->buf, B->bufsize); }
	strcat(B->buf + B->stringL, addendum);
	B->stringL += l; }


void test_numbers(XmlRpcClient &c) {
	XmlRpcValue	numbers;
	XmlRpcValue	result;

	// Add up an array of numbers
	numbers[0] = 33.33;
	numbers[1] = 112.57;
	numbers[2] = 76.1;
	std::cout << "Sum test: numbers.size() is " << numbers.size() << std::endl;
	if(c.execute("Sum", numbers, result)) {
		std::cout << "Sum = " << double(result) << "\n\n"; }
	else {
		std::cout << "Error calling 'Sum'\n\n"; } }

static bool shouldTerminate = true;

void terminate_server() {
	if(!shouldTerminate) {
		return; }
	XmlRpcValue	noArgs, result;
	if(client().execute("APcoreTerminate", noArgs, result)) {
		std::cout << "\n<last>. APcoreTerminate result: " << result << "\n"; }
	else {
		std::cout << "\n<last>. Error calling 'APcoreTerminate'\n\n"; } }

int usage(const char *A) {
	std::cerr << "Usage: " << A << " serverHost serverPort [APcoreSessions.]method numparam [param0 [param1 [...]]]\n";
	return -1; }

int main(int argc, char* argv[]) {
	int		i = 0;
	int		k = 0;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	const char	*theMethod, *theParameter;
	int		numparam = 0;
	buf_struct	B = {NULL, 0, 0};

	/* usage */
	// XmlRpc::setVerbosity(5);
	if(argc >= 5) {
		theHostName = argv[1];
		thePortNumber = atoi(argv[2]);
		theMethod = argv[3];
		numparam = atoi(argv[4]);
		for(k = 0; k < numparam; k++) {
			int l;
			bool is_number = true;
			const char *c = argv[5 + k];
			for(l = 0; l < strlen(c); l++) {
				if(!isdigit(c[l])) {
					is_number = false;
					break; } }
			if(is_number) {
				Args[k] = atol(argv[5 + k]); }
			else {
				Args[k] = argv[5 + k]; } } }
	else {
		return usage(argv[0]); }
 
	/* Call the Init method */
	// concatenate(&B, "APcoreSessions.");
	concatenate(&B, theMethod);
	std::cout << "sending APcoreServer the command \"" << B.buf << "(" << Args << ")\"..." << std::endl;
	if(client().execute(B.buf, Args, result)) {
		std::cout << "\n" << theMethod << " result: " << result << "\n"; }
	else {
		std::cout << "\nError calling '" << B.buf << Args << std::endl;
		return -1; }
	return 0; }
