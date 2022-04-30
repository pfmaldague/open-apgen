#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "APerrors.H"
#include <ActivityInstance.H>
#include "RES_exceptions.H"
#include "action_request.H" // Added by BKL 9-19-97

void	(*errors::clientShouldStoreMessage)() = NULL;
void	(*errors::clientShouldDisplayMessage)(const Cstring &title, const Cstring &msg) = NULL;

using namespace std;

bool	errors::shouldAccumulate = false;

strintslist& errors::theErrorStack() {
	static strintslist L;
	return L; }

// END OF STATICS

errors::errors() {}

errors::~errors() {}

void errors::accumulate_errors(bool c) {
	// debug
	// cout << "accumulate_errors set to " << c << endl;
	shouldAccumulate = c;
}

void errors::Post(const Cstring &title, const Cstring &body) {
	int S_length;
	Cstring	msg(title);

	msg << " -- " << body;
	// debug
	// cout << "errors::Post called\n";
	if(shouldAccumulate) {
		if(clientShouldStoreMessage) {
			// debug
			// cout << "errors::Post calling clientShouldStoreMessage...\n";
			clientShouldStoreMessage(); }
		errors::theErrorStack() << new strint_node(msg, COMBINED_ERROR | SERIOUS_ERROR | HAS_BEEN_SENT_TO_LOG); }
	else if(clientShouldDisplayMessage) {
		// debug
		// cout << "errors::Post calling clientShouldDisplayMessage...\n";
		clientShouldDisplayMessage(title, body); }
	// debug
	// else {
	// 	cout << "Hmmm... shouldAccumulate = false, clientShouldDisplayMessage = NULL... strange.\n"; }
	Log::LOG() << "\n" << *add_pound_sign(msg);
	S_length = msg.length();
	if(S_length && msg[S_length - 1] != '\n') Log::LOG() << "\n";
}
