#ifndef _ALPHA_STRING_H_
#define _ALPHA_STRING_H_

#include <ostream>

using namespace std;

/* The classes in this file are intended to be used as the class
 * argument K in the template apnode<K, PLD> (see apnode.H
 * for a list of requirements on K as a template argument).
 *
 * They come in 2 flavors, depending on whether or not the
 * class provides automatic disambiguation. Automatic dis-
 * ambiguation means that when inserting a new node with
 * key A into a tlist that already contains a node with key
 * A, a second "hidden" key will be defined so that the
 * combination of the visible and invisible keys is truly
 * unique.
 *
 * When disambiguation is not provided by the key class, an 
 * exception will be generated whenever one attempts to insert
 * a node with a duplicate key into a tlist. */


/* with some effort one could use std::string instead. That
 * is a well-defined conversion; will do it when I'll have
 * time... */
#include <C_string.H>

/** string-based key, with automatic disambiguation.

The main purpose of this class is to provide a convenient
way to index items that have unique or mostly unique names.
The main usage of this class is as a class template
argument of the apKey template (see slist.H). In a typical
example, the programmer needs to define a map-style list
of nodes indexed by a string. Suppose that the contents
of the node are described by a class called node_content.
The programmer can then create the desired map-style list
by using the following templates:

typedef apnode<alpha_string, node_content> node;
tlist<node> L;

Note that the apnode template will automatically create
a key class based on the apKey<alpha_string> template.
In hindsight this extra complication is superfluous;
apnode should be able to use the underlying concrete
class alpha_string with no problems. I should test this
idea. */
class alpha_string {

// DATA
  Cstring		str;
  unsigned long		secondaryKey;
public:

// CONSTRUCTORS
  /* constructor */	alpha_string() : secondaryKey(0) {}
  /* constructor */	alpha_string(const Cstring& s) : str(s), secondaryKey(0) {}
  /* constructor */	alpha_string(const char* s) : str(s), secondaryKey(0) {}
  /* constructor */	alpha_string(long d) : str(d), secondaryKey(0) {}
  /* constructor */	alpha_string(const void* v) : str(v), secondaryKey(0) {}
  /* constructor */	alpha_string(const alpha_string& as) : str(as.str), /* this is the correct behavior: */ secondaryKey(0) {}
  virtual		~alpha_string() {}

// METHODS
  bool			operator > (const alpha_string& as) const {
				int f = strcmp(*str, *as.str);
				if(!f) { return secondaryKey > as.secondaryKey; }
				return f > 0; }
  bool			operator < (const alpha_string& as) const {
				int f = strcmp(*str, *as.str);
				if(!f) { return secondaryKey < as.secondaryKey; }
				return f < 0; }
  bool			operator == (const alpha_string& as) const {
				int f = strcmp(*str, *as.str);
				if(!f) { return secondaryKey == as.secondaryKey; }
				return !f; }
  bool			operator != (const alpha_string& as) const {
				int f = strcmp(*str, *as.str);
				if(!f) { return secondaryKey != as.secondaryKey; }
				return f != 0; }
  int			compare_to(const alpha_string& that, bool ignoreSecKey = false) const {
				int f = strcmp(*str, *that.str);
				if(!f) {
					if(!ignoreSecKey) {
						if(secondaryKey > that.secondaryKey) {
							return 1; }
						else if(secondaryKey < that.secondaryKey) {
							return -1; }
						return 0; }
					else {
						return 0; } }
				return f; }
  static bool		disambiguates() { return true; }
  void			disambiguate(unsigned long& currMaxSecKey) { secondaryKey = ++currMaxSecKey; }
  static bool		reacts_to_time_events() { return false; }
  const Cstring&	get_key() const { return str; }
  void			set_key(const Cstring& s) { str = s; }
  unsigned long&	get_2nd_key() { return secondaryKey; }
  friend ostream& operator << (ostream& s, const alpha_string& x);
};

// integer-based key, with disambiguation
class alpha_int {
public:

// DATA
  long			Num;
  unsigned long		secondaryKey;

// CONSTRUCTORS
  /* constructor */	alpha_int(const Cstring& s) : Num(0), secondaryKey(0) { if(s.is_defined()) { Num = atol(*s); } }
  /* constructor */	alpha_int(const char* s) : Num(0), secondaryKey(0) { Num = atol(s); }
  /* constructor */	alpha_int(const long& L) : Num(L), secondaryKey(0) {}
  /* constructor */	alpha_int(const void* v) : Num((long) v), secondaryKey(0) {}
  /* constructor */	alpha_int(const alpha_int& as) : Num(as.Num), /* this is the correct behavior: */ secondaryKey(0) {}
  virtual		~alpha_int() {}

// METHODS
  bool operator > (const alpha_int& as) const {	long int f =  Num - as.Num;
						if(!f) { return secondaryKey > as.secondaryKey; }
						return f > 0; }
  bool operator < (const alpha_int& as) const { long int f =  Num - as.Num;
						if(!f) { return secondaryKey < as.secondaryKey; }
						return f < 0; }
  bool operator == (const alpha_int& as) const { long int f =  Num - as.Num;
						if(!f) { return secondaryKey == as.secondaryKey; }
						return false; }
  bool operator != (const alpha_int& as) const { long int f =  Num - as.Num;
						if(!f) { return secondaryKey != as.secondaryKey; }
						return true; }
  int compare_to(const alpha_int& that, bool ignoreSecKey = false) const {
						long int f = Num - that.Num;
						if(!f) {
							if(!ignoreSecKey) {
								if(secondaryKey > that.secondaryKey) {
									return 1; }
								else if(secondaryKey < that.secondaryKey) {
									return -1; }
								return 0; }
							else {
								return 0; } }
						if(f > 0) return 1;
						else if(f < 0) return -1;
						return 0; }
  static bool		disambiguates() { return true; } // yes, we can
  void			disambiguate(unsigned long& currMaxSecKey) { secondaryKey = ++currMaxSecKey; } // used while inserting nodes to remove ambiguity
  static bool		reacts_to_time_events() { return false; }
  const Cstring&	get_key() const { static Cstring s; s = Cstring(Num); return s; }
  unsigned long&	get_2nd_key() { return secondaryKey; }
  long			get_int() const { return Num; }
  friend ostream&	operator << (ostream& s, const alpha_int& x);
};

// string-based key, without disambiguation
class ualpha_string {
  Cstring		str;
public:

// CONSTRUCTORS
  // obvious constructor (1)
  ualpha_string(
		const Cstring& s)
	: str(s) {}

  /* constructor */	ualpha_string() {}
  /* constructor */	ualpha_string(const char* s) : str(s) {}
  /* constructor */	ualpha_string(long d) : str(d) {}
  /* constructor */	ualpha_string(const void* v) : str(v) {}
  /* constructor */	ualpha_string(const ualpha_string& as) : str(as.str) {}
  virtual		~ualpha_string() {}

// METHODS
  bool operator > (const ualpha_string& as) const { int f = strcmp(*str, *as.str); return f > 0; }
  bool operator < (const ualpha_string& as) const { int f = strcmp(*str, *as.str); return f < 0; }
  bool operator == (const ualpha_string& as) const { int f = strcmp(*str, *as.str); return !f; }
  bool operator != (const ualpha_string& as) const { int f = strcmp(*str, *as.str); return f != 0; }
  int			compare_to(const ualpha_string& that, bool = false) const { if(*this > that) return 1; if(*this == that) return 0; return -1; }
  static bool		disambiguates() { return false; } // no it does not
  void			disambiguate(unsigned long&) {}
  static bool		reacts_to_time_events() { return false; }
  const Cstring&	get_key() const { return str; }
  static unsigned long&	fake_secondaryKey() { static unsigned long k = 0; return k; }
  unsigned long&	get_2nd_key() { return fake_secondaryKey(); }
  friend ostream& operator << ( ostream& s, const ualpha_string& x);
};

// integer-based key, without disambiguation
class ualpha_int {
public:
// DATA
  long			Num;

// CONSTRUCTORS
  /* constructor */	ualpha_int(const Cstring& s) : Num(0) { if(s.is_defined()) { Num = atol(*s); } }
  /* constructor */	ualpha_int(const char* s) : Num(0) { Num = atol(s); } 
  /* constructor */	ualpha_int(const long& L) : Num(L) {}
  /* constructor */	ualpha_int(const void* v) : Num((long) v) {}
  /* constructor */	ualpha_int(const ualpha_int& as) : Num(as.Num) {}
  virtual		~ualpha_int() {}
  bool operator > (const ualpha_int& as) const { return Num > as.Num; }
  bool operator < (const ualpha_int& as) const { return Num < as.Num; }
  bool operator == (const ualpha_int& as) const { return Num == as.Num; }
  bool operator != (const ualpha_int& as) const { return Num != as.Num; }
  int			compare_to(const ualpha_int& that, bool = false) const { if(*this > that) return 1; if(*this == that) return 0; return -1; }
  static bool		disambiguates() { return false; }
  void			disambiguate(unsigned long&) {}
  static bool		reacts_to_time_events() { return false; }

// returned string will be static, hence short-lived
  const Cstring&	get_key() const;
  static unsigned long&	fake_secondaryKey() { static unsigned long k = 0; return k; }
  unsigned long&	get_2nd_key() { return fake_secondaryKey(); }
  long			get_int() const { return Num; }
  friend ostream& operator << ( ostream& s, const ualpha_int& x);
};

/* void*-based key, without disambiguation - usually, it's an error
 * to point multiple times to the same object. */
class alpha_void {
public:

// DATA
  const void*		Num;

// CONSTRUCTORS
  /* constructor */	alpha_void() : Num(0) {}
  /* constructor */	alpha_void(const Cstring& s) : Num(0) { if(s.is_defined()) { long k; sscanf(*s, "%lx", &k); Num = (const void*) k; } }
  /* constructor */	alpha_void(const char* s) : Num(0) { long k; sscanf(s, "%lx", &k); Num = (const void*) k; }
  /* constructor */	alpha_void(const long& L) : Num((const void*) L) {}
  /* constructor */	alpha_void(const void* v) : Num(v) {}
  /* constructor */	alpha_void(const alpha_void& as) : Num(as.Num) {}
  virtual		~alpha_void() {}

// METHODS
  bool operator > (const alpha_void& as) const { return Num > as.Num; }
  bool operator < (const alpha_void& as) const { return Num < as.Num; }
  bool operator == (const alpha_void& as) const { return Num == as.Num; }
  bool operator != (const alpha_void& as) const { return Num != as.Num; }
  int compare_to(const alpha_void& that, bool = false) const { if(*this > that) return 1; if(*this == that) return 0; return -1; }
  static bool		disambiguates() { return false; } // this class does not disambiguate
  void			disambiguate(unsigned long&) {}
  static bool		reacts_to_time_events() { return false; }
			// returned string will be static, hence short-lived
  const Cstring&	get_key() const { static char s[100]; static Cstring C; sprintf(s, "%lx", (long) Num); C = s; return C; }
  static unsigned long&	fake_secondaryKey() { static unsigned long k = 0; return k; }
  unsigned long&	get_2nd_key() { return fake_secondaryKey(); }
  long			get_int() const { return (long) Num; }
  friend ostream& operator << (ostream& s, const alpha_void& x);
};

#endif /* _ALPHA_STRING_H_ */
