/*Generic string class from Stroustrop, section 6.9, with enhancements.*/

#ifndef C_STRING_H
#define C_STRING_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

#include <treeDir.H>

using namespace std;

class Cstring;
class TypedValue;

extern bool	 operator >=	( const Cstring &,	const char * ); 
extern bool	 operator >=	(const Cstring &,	const Cstring & ); 
extern bool	 operator &	(const Cstring &,	const char *);	// contains pattern
extern	ostream	&operator <<	(ostream & s,		const Cstring &x);
extern	istream	&operator >>	(istream &,		Cstring &);
extern	istream	&operator >	(istream &,		Cstring &);
extern	bool	 operator ==	(const Cstring &x,	const Cstring &y);
extern	bool	 operator ==	(const Cstring &x,	const char* y);
extern	bool	 operator !=	(const Cstring &x,	const Cstring &y);
extern	bool	 operator !=	(const Cstring &x,	const char *y);
extern	bool	 equal(const Cstring &a, const Cstring &b);
// extern	Cstring	operator +(const Cstring& x, const Cstring& y);
class TypedValue;

/* This structure moved out of Cstring class by PFM to make it
 * possible for TDnode to efficiently store string values
 * without copying */
class srep {
    public:
	srep() : perm { true }, s { NULL } {
	}
	srep(const srep&) = delete;
	~srep() {
	}

	const char*	s;

	//
	// true if C string is permanent (no need to allocate/free)
	// false if C string is our own (must allocate/free) 
	bool		perm;
};

namespace pEsys {
class PlusStringString;
class AdditiveExp;
template <class>
class StringOp;
};


class Cstring {
	srep		p;
public:

	static const Cstring& null();

	static bool		make_all_strings_permanent;

	//
	// create new srep from C string
	//
	static const char*	create(srep&, const char*);

	//
	// create new srep from existing srep
	//
	static const char*	create(srep&, const srep&);

	//
	// for the benefit of valgrind:
	//
	static void		delete_permanent_strings();

	Cstring();

	//
        // for easy conversion between string types
        //
        Cstring(const string &other); 

	Cstring(const Cstring &);

	//
	// Starting August 2019, the following two constructors differ
	// in essential ways. Given a constant C string, Cstring just
	// copies the pointer:
	//
	// 	CONST CHAR* OBJECTS ARE ASSUMED PERMANENT
	//
	// On the other hand,
	//
	// 	CHAR* OBJECTS ARE ASSUMED EPHEMERAL
	//
	// and the C string is duplicated in the Cstring object.
	//
	Cstring(const char *);

	//
	// Use this when the C string can be assumed permanent:
	//
	Cstring(const char *str, bool) {
		p.perm = true;
		assert(str);
		p.s = str;
	}

	Cstring(char *);

	Cstring(const int&);
	Cstring(const long&);
	Cstring(const long long&);
	Cstring(const unsigned long&);
	Cstring(const unsigned long long&);
	Cstring(const char);
	// Cstring(srep *);
	Cstring(const double, const int = 6); // 6 is default %f,%e precision
	Cstring(const void *);

	~Cstring();


	Cstring&	operator = (const Cstring &);

	Cstring&	operator = (const char *);
			// return ith character
	char		operator[](long i) const;

			//
			// returns address of string or of null
			//
	const char*	operator*() const {
		return p.s;
	}

			//
			// returns the underlying char*
			//
	char*		operator()() const;

			//
			// Used to check whether p.s was non-NULL.
			// But now, the equivalent check is whether
			// p.s[0] is not the null character.
			//
	bool		is_defined() const {
		return p.s[0] != '\0';
	}
			//  not null string
	bool		has_value() const {
		return p.s[0] != '\0';
	}
			//  make it undefined	
	void		undefine();
			//  returns length of string
	unsigned long	length() const {
		return strlen(p.s);
	}
			//  concatenate x onto this (see operator+ below )
	Cstring &	operator << (const Cstring &x);
  
	// added by BKL 9-26-97
	friend bool      operator >= (const Cstring &, const char *); 
	//added by BKL 9-26-97
	friend bool      operator >= (const Cstring &, const Cstring &); 

	// comparison operator so this can be used by STL
	bool		operator < (const Cstring& rhs) const {
		return !*p.s ?
				(!*rhs.p.s ?
					false :
					true
				) :
				(!*rhs.p.s ?
					false :
					strcmp(p.s, rhs.p.s) < 0
				);
	}

			// convert to lower case
	void		to_lower();
			// convert to upper case
	void		to_upper();
			// return the substring starting at 'strt' for length 'len'
			//	'strt' = 0 for first character
	Cstring		substr(long strt, long len);

	Cstring		operator()(long strt, long len) { return substr(strt, len); }

			// replace the underlying C string by the given one (ALL REFERENCES
			// TO THIS STRING WILL BE MODIFIED!)

			// (ALSO A MEMORY LEAK GENERATOR IF NOT CAREFUL)

	void		substitute(const char *C, const char **oldC) {
		*oldC = (const char *) p.s;
		p.s = C;
	}

			// for compatibility w/seq_review:
	long		OccurrencesOf(const char *) const;

	friend bool	::operator &	(const Cstring &,	const char *);	// contains pattern
	friend Cstring	operator /	(const Cstring &,	const char *);	// keep left of pattern
	friend Cstring	operator /	(const char *,		const Cstring&);// keep right of pattern
	friend Cstring	operator -	(const Cstring &,	const char *);	// trim right of pattern
	friend Cstring	operator -	(const char *,		const Cstring &);// trim left of pattern

			// insert into stream
	friend ostream	&::operator << (ostream & s , const Cstring &x);

			// extract from stream, white space delimiter
	friend istream	&::operator >> (istream &, Cstring &);

			// extract from stream, double quote delimiter
	friend istream	&::operator > (istream &, Cstring &);

			// return TRUE if equal (x,y must be defined)
	friend bool	operator == (const Cstring &x, const Cstring &y) {
		return !strcmp(*x, *y);
	}
			// return TRUE if equal (x must be defined)
	friend bool	operator == (const Cstring& x, const char* y) {
		return !strcmp(*x, y);
	}
			//return TRUE if unequal (x,y must be defined)
			// BUG: can return negative number, depending on string values!!
			//  Fixed 12/2/97 PFM
	friend bool	operator != (const Cstring &x, const Cstring &y) {
		return strcmp(*x, *y);
	}
			//return TRUE if unequal (x must be defined)
			// BUG: can return negative number, depending on string values!!
			//  Fixed 12/2/97 PFM
	friend bool	operator != (const Cstring &x , const char *y) {
		return strcmp(*x, y);
	}
			// compare strings (undefined is equal to null string)
	friend bool	equal(const Cstring &a, const Cstring &b) {
		return !strcmp(*a, *b);
	} 
			// concatenate x and y
	friend Cstring	operator +(const Cstring& x, const Cstring& y);
	friend Cstring	nonewlines(const Cstring& s);
	friend class	pEsys::PlusStringString;
	friend class	pEsys::AdditiveExp;
	friend class	pEsys::StringOp<pEsys::PlusStringString>;
	friend class	::TypedValue;
};

	    // replaces "&" by "&quot;" etc.
extern char *fix_for_XML(const Cstring &);

// class Node;

/*	The purpose of this class is to provide a mechanism for navigating a complete
	hierarchy of data structures (not just Lists and Nodes.) To enable the mechanism,
	all data members of a class must have pointers to their owners. Nodes already
	have pointers to their parent List. The listOwner class provides a convenient
	way to pass handles from a parent to its children. */
// OBSOLETE
// class listOwner {};

// use this as argument or return value
const Cstring NULLCS;

/* N.B., I have defined operator==(Cstring,Cstring), as well as
operator==(Cstring,char*) in spite of user-defined type
conversion (same for operator !=()) because "if (x == y || w == z)"
will fail in 2.0 cfront (also && or ?: -- see "Not implemented:
temporary of class ...", Reference Manual, App. D)

In the meantime, watch out for the likes of 

		Cstring x;
		if (x == 0) ...			

For the special case of 0, this will use operator==(const Cstring& x,
const char* y), which will bomb.  Need to say

		if (x == "0") 

*/

namespace listEnum {
// an enumeration to support the olist (OwnedList) subclass
typedef enum {
	BEFORE_NODE_INSERTION,
	AFTER_NODE_INSERTION,
	BEFORE_NODE_REMOVAL,
	AFTER_NODE_REMOVAL
	} listEvent;

// utility
extern const char* spellEvent(listEvent e);
} // namespace listEnum

//
// To help write structured (indented) debug statement
//

class dbgind {
public:
	static int count;
	dbgind() {
		count += 3;
	}
	~dbgind() {
		count -= 3;
	}
	static string indent() {
		stringstream s;
		for(int i = 0; i < count; i++) {
			s << " ";
		}
		return s.str();
	}
};

#endif

