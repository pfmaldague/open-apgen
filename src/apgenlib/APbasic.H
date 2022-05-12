#ifndef _AP_BASIC_H_
#define _AP_BASIC_H_

#include "templates.H"
#include "alphastring.H"
#include "alphatime.H"

#include <thread>
#include <mutex>

extern thread_local int thread_index;

typedef keyNode<alpha_string> emptySymbol;
typedef slist<alpha_string, emptySymbol> stringslist;
typedef tlist<alpha_string, emptySymbol> stringtlist;

typedef Cntnr<alpha_string, int>		strint_node;
typedef slist<alpha_string, strint_node>	strintslist;
typedef tlist<alpha_string, strint_node>	strinttlist;

typedef Cntnr<alpha_string, stringslist> listNode;
typedef tlist<alpha_string, listNode> listtlist;

typedef Cntnr<alpha_string, Cstring> symNode;

typedef slist<alpha_string, symNode> pairslist;
typedef tlist<alpha_string, symNode> pairtlist;

class TypedSym; // for TypedSymbol
typedef slist<alpha_string, TypedSym> symslist;
typedef tlist<alpha_string, TypedSym> symtlist;

// APGEN ostrstream replacement:
class aoString {
// debug
public:
	stringslist	theList;
	char*		theBuffer;
public:
	aoString();
	~aoString();

	// for the benefit of clients that count from last newline char:
	long				LineLength;
	aoString&			operator << (void *);
	aoString&			operator << (const char *);
	aoString&			operator << (const Cstring &);
	char*				str(bool append_newlines = false);
	void				write_to(FILE *);
	void				clear();
	bool				is_empty() const;
};

//
// Use this class instead of cout to avoid trampling the output of
// other threads:
//
class coutx {
public:
	coutx() {}
	coutx(const coutx&) = default;
	~coutx() = default;

	static mutex&			mutx();
	static ofstream&		outfile();

	coutx operator << (const Cstring& s) {
		lock_guard<mutex>	lock(mutx());
		int			off = thread_index;

		//
		// Cosmetic... we want the TOL first, because
		// we use it more often!
		//
		if(off == 1) {
			// constraints
			off = 2;
		} else if(off == 2) {
			// XMLTOL
			off = 3;
		} else if(off == 3) {
			// TOL
			off = 1;
		}
		for(int i = 0; i < off; i++) {
			cerr << "\t\t";
		}
		cerr << s;
		return *this;
	}
	coutx operator << (const char* s) {
		lock_guard<mutex>	lock(mutx());
		int			off = thread_index;
		if(off == 1) {
			// constraints
			off = 2;
		} else if(off == 2) {
			// XMLTOL
			off = 3;
		} else if(off == 3) {
			// TOL
			off = 1;
		}
		for(int i = 0; i < off; i++) {
			cerr << "\t\t";
		}

		cerr << s;
		return *this;
	}
};


//
// Use this class instead of cout to avoid trampling the output of
// other threads:
//
class streamx {
public:
	streamx() {}
	streamx(const streamx&) = delete;
	~streamx() = default;

	static mutex&			mutx();

	streamx& operator << (const string& s) {
	    lock_guard<mutex>	lock(mutx());
	    for(int i = 0; i < thread_index; i++) {
		S << "\t\t";
	    }
	    S << s;
	    return *this;
	}
	streamx& operator << (const Cstring& s) {
	    lock_guard<mutex>	lock(mutx());
	    for(int i = 0; i < thread_index; i++) {
		S << "\t\t";
	    }
	    S << s;
	    return *this;
	}
	streamx& operator << (const char* s) {
	    lock_guard<mutex>	lock(mutx());
	    for(int i = 0; i < thread_index; i++) {
		S << "\t\t";
	    }

	    S << s;
	    return *this;
	}
	string str() {
	    return S.str();
	}
	stringstream	S;
};

#endif /* _AP_BASIC_H_ */
