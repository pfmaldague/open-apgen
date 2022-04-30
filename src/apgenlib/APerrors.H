#ifndef AP_ERRORS_H
#define AP_ERRORS_H

#include "APbasic.H"
#include "APdata.H"

#define SERIOUS_ERROR			1
#define WARNING_ONLY			2
#define COMBINED_ERROR			4
#define HAS_BEEN_SENT_TO_LOG		8
#define HAS_BEEN_DISPLAYED_TO_USER	16


class errors {
public:
	errors();
	~errors();

					// The following List should contain strint_node:
	static strintslist&		theErrorStack();
	// static void			Post(const Cstring &msg);
	static void			Post(const Cstring& title, const Cstring& msg);
	static void			accumulate_errors(bool);			
	static void			(*clientShouldStoreMessage)();
	static void			(*clientShouldDisplayMessage)(const Cstring& title, const Cstring& msg);
	static bool			shouldAccumulate;
	};

#endif /* AP_ERRORS_H */
