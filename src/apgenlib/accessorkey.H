#ifndef _ACCESSOR_KEY_H_
#define _ACCESSOR_KEY_H_

#include <templates.H>
#include <alphastring.H>


/* A "weird" string-based key which recognizes a finite
 * set of special strings as distinct keys and lumps all
 * other strings into a single generic key.  This class
 * was designed to support accessors for objects which
 * have a fixed set of special accessors (for things
 * like parent, children, and various things that need
 * to be computed rather than stored) and also an
 * arbitrary number of non-special keys which must all
 * be accessed through the generic accessor.
 *
 * The template parameter SpecialKeys is a class on
 * which the only requirement is that it should have
 * a static method get_list() which returns a tlist
 * containing one node for each special string. */
template <class SpecialKeys>
class accesskey {
  Cstring		str;
public:
  static tlist<alpha_string, Cntnr<alpha_string, bool> >& special_keys() {
	  static tlist<alpha_string, Cntnr<alpha_string, bool> > T(SpecialKeys::get_list());
	  return T;
  }

  accesskey(const Cstring& s) {
	if(special_keys().find(s)) {
		str = s;
	} else {
		str = "generic";
	}
  }
  accesskey() : str("generic") {}
  accesskey(const char* s) {
	if(special_keys().find(s)) {
		str = s;
	} else {
		str = "generic";
	}
  }
  accesskey(long d) : str("generic") {}
  accesskey(const void* v) : str("generic") {}
  accesskey(const accesskey& as) : str(as.str) {}
  virtual ~accesskey() {}

// METHODS
  bool operator > (const accesskey& as) const {
	  int f = strcmp(*str, *as.str); return f > 0;
  }
  bool operator < (const accesskey& as) const {
	  int f = strcmp(*str, *as.str); return f < 0;
  }
  bool operator == (const accesskey& as) const {
	  int f = strcmp(*str, *as.str); return !f;
  }
  bool operator != (const accesskey& as) const {
	  int f = strcmp(*str, *as.str); return f != 0;
  }
  int  		compare_to(const accesskey& that, bool = false) const {
	  if(*this > that) return 1; if(*this == that) return 0; return -1;
  }
  static bool	disambiguates() {
	  return false; // no it does not
  }
  void		disambiguate(unsigned long&) {}
  static bool	reacts_to_time_events() {
	  return false;
  }
  const Cstring& get_key() const {
	  return str;
  }
  static unsigned long&	fake_secondaryKey() {
	  static unsigned long k = 0; return k;
  }
  unsigned long& get_2nd_key() {
	  return fake_secondaryKey();
  }
};

#endif /* _ACCESSOR_KEY_H_ */
