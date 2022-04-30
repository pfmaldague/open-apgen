#ifndef _ARRAY_KEY_STRING_H_
#define _ARRAY_KEY_STRING_H_

#include <C_string.H>
#include <vector>

/* This class is not intended to provide a 'real' key,
 * i. e., a key for use with the slist and tlist templates
 * in templates.H.  Instead, it is intended to support a
 * new approach to TypedValue arrays in which we use STL
 * vectors to provide access in constant time.
 *
 * In nutshell: for elements in a LIST_STYLE array, use
 * secondaryKey as the key; for elements in a STRUCT_STYLE
 * array, use str. */
class array_string {

// DATA
  Cstring		str;
  unsigned long		secondaryKey;
public:
  bool			style_is_struct;

// CONSTRUCTORS
  /* constructor */	array_string()
	  			: secondaryKey(0),
					str(Cstring::null()),
       					style_is_struct(false)	{}
  /* constructor */	array_string(const Cstring& s)
	  			: str(s),
					style_is_struct(true),
					secondaryKey(0) {}
  /* constructor */	array_string(const char* s)
	  			: str(s),
					style_is_struct(true),
					secondaryKey(0) {}
  /* constructor */	array_string(long d)
	  			: secondaryKey((unsigned long)d),
					style_is_struct(false)
					{}
  /* constructor */	array_string(const void* v)
	  			: str(v),
					style_is_struct(true),
					secondaryKey(0) {}
  /* constructor */	array_string(const array_string& as)
	  			: str(as.str),
					style_is_struct(as.style_is_struct),
					secondaryKey(0) {}
  virtual		~array_string() {}

// METHODS
  bool			operator > (const array_string& as) const {
				int f = strcmp(*str, *as.str);
				if(!f) { return secondaryKey > as.secondaryKey; }
				return f > 0; }
  bool			operator < (const array_string& as) const {
	int f = strcmp(*str, *as.str);
	if(!f) {
		return secondaryKey < as.secondaryKey;
	}
	return f < 0;
  }
  bool			operator == (const array_string& as) const {
	int f = strcmp(*str, *as.str);
	if(!f) {
		return secondaryKey == as.secondaryKey;
	}
	return !f;
  }
  bool			operator != (const array_string& as) const {
	int f = strcmp(*str, *as.str);
	if(!f) {
		return secondaryKey != as.secondaryKey;
	}
	return f != 0;
  }
  int			compare_to(const array_string& that, bool ignoreSecKey = false) const {
	int f = strcmp(*str, *that.str);
	if(!f) {
	    if(!ignoreSecKey) {
		if(secondaryKey > that.secondaryKey) {
		    return 1;
		} else if(secondaryKey < that.secondaryKey) {
		    return -1;
		}
		return 0;
	    } else {
		return 0;
	    }
	}
	return f;
  }
  static bool		disambiguates() {
	return true;
  }
  void			disambiguate(unsigned long& currMaxSecKey) {
	secondaryKey = ++currMaxSecKey;
  }
  static bool		reacts_to_time_events() {
	return false;
  }
  const Cstring&	get_key() const {
	return str;
  }

  void			set_key(const Cstring& s) {
	assert(false);
  }
  unsigned long&	get_2nd_key() {
	return secondaryKey;
  }
  const unsigned long&	get_2nd_key() const {
	return secondaryKey;
  }
  friend ostream& operator << (ostream& s, const array_string& x);
};
#endif /* _ARRAY_KEY_STRING_H_ */
