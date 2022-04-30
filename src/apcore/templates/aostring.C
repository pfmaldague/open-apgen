#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include "C_global.H"	/* for DOUBLE_DIG */
#include "APbasic.H"	// for aoString

using namespace std;

aoString::aoString()
	: theBuffer(NULL),
	LineLength(0) {}

aoString::~aoString() {
	theList.clear();
	if(theBuffer) {
		free(theBuffer);
	}
}

aoString& aoString::operator << (void* V) {
	static char	holder[sizeof(void*)];

	sprintf(holder, "%p", V);
	return (*this) << holder;
}

aoString& aoString::operator << (const char* S) {
	int	k;

	if(S && (k = strlen(S))) {
		LineLength += k;
		theList << new emptySymbol(S);
	}
	return *this;
}

aoString& aoString::operator << (const Cstring& S) {
	int	k;

	if((k = S.length())) {
		LineLength += k;
		theList << new emptySymbol(S);
	}
	return *this;
}


void aoString::clear() {
	theList.clear();
	// Do NOT clear the buffer; ONLY clear the buffer inside str():
	// if( theBuffer ) {
	// 	free( theBuffer );
	// 	theBuffer = NULL; }
	LineLength = 0;
}

void aoString::write_to(FILE* f) {
	stringslist::iterator theNodes(theList);
	emptySymbol*	N;
	const char*	c;
	const char*	d;
	int		L;

	if(theBuffer) {
		free(theBuffer);
		theBuffer = NULL;
	}
	while((N = theNodes())) {
		N->get_key();
		fwrite(*N->get_key(), 1, N->get_key().length(), f);
	}
	clear();
}

char *aoString::str(bool append_newlines) {
	int			theLength = 0;
	stringslist::iterator	theNodes(theList);
	emptySymbol*		N;
	char*			c;
	int			nl = append_newlines ? 1 : 0;

	if(theBuffer) {
		free(theBuffer);
		theBuffer = NULL;
	}
	while((N = theNodes())) {
		theLength += N->get_key().length() + nl;
	}
	theBuffer = (char *) malloc(theLength + 1);
	c = theBuffer;
	*c = '\0'; // Required if buffers are empty
	while((N = theNodes())) {
		strcpy(c, *N->get_key());
		c += N->get_key().length();
		if(append_newlines) {
			*c++ = '\n';
			*c = '\0';
		}
	}
	clear();
	return theBuffer;
}

bool aoString::is_empty() const {
	return theList.get_length() == 0;
}
