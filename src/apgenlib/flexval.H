#ifndef _FLEX_VAL_H_
#define _FLEX_VAL_H_

#include <string>
#include <vector>
#include <map>
#include <ostream>
#include <sstream>

#ifdef have_xmltol
namespace xmlpp {
	class Element;
} // xmlpp
#endif

struct json_object;

#include "RES_exceptions.H"
#include "planning-time.H"

class flexval {
public:
    enum Type {
	TypeInvalid,
	TypeArray,
	TypeStruct,
	TypeBool,
	TypeInt,
	TypeDouble,
	TypeString,
	TypeTime,
	TypeDuration
    };

    static const char*				type2string(Type t);
    static string				addQuotes(const string&);
    static string				removeQuotes(const string&);

    typedef std::vector<flexval>		ValueArray;
    typedef std::map<std::string, flexval>	ValueStruct;

    flexval() : _type(TypeInvalid) { _value.asArray = NULL; }
    flexval(const bool& value) :_type(TypeBool) { _value.asBool = value; }
    flexval(const long int& value) :_type(TypeInt) { _value.asInt = value; }
    flexval(const int& value) :_type(TypeInt) { _value.asInt = (long)value; }
    flexval(const double& value) :_type(TypeDouble) { _value.asDouble = value; }
    flexval(const std::string& value) :_type(TypeString) { _value.asString = new std::string(value); }
    flexval(const cppTime& value) :_type(TypeTime) { _value.asTime = new cppTime(value); }
    flexval(const cppDuration& value) :_type(TypeDuration) { _value.asDuration = new cppDuration(value); }
    flexval(const flexval& A) : _type(TypeInvalid) { *this = A; }

    ~flexval() { invalidate(); }

    void clear() { invalidate(); }

    flexval&	operator=(const flexval& rhs);
    flexval&	operator=(const bool& rhs) { return operator=(flexval(rhs)); }
    flexval&	operator=(const long& rhs) { return operator=(flexval(rhs)); }
    flexval&	operator=(const int& rhs) { return operator=(flexval(rhs)); }
    flexval&	operator=(const double& rhs) { return operator=(flexval(rhs)); }
    flexval&	operator=(const std::string& rhs) { return operator=(flexval(rhs)); }
    flexval&	operator=(const char* rhs) { return operator=(flexval(std::string(rhs))); }
    flexval&	operator=(const cppTime& rhs) { return operator=(flexval(rhs)); }
    flexval&	operator=(const cppDuration& rhs) { return operator=(flexval(rhs)); }

    bool	operator == (const flexval& rhs) const;
    bool	operator != (const flexval& rhs) const;

    operator ValueStruct&() { assertStruct(); return *_value.asStruct; }
    operator ValueArray&()  { assertArray(1); return *_value.asArray; }
    operator bool&()  { assertTypeOrInvalid(TypeBool); return _value.asBool; }
    operator const bool&() const { assertTypeOrInvalid(TypeBool); return _value.asBool; }
    operator long int&()  { assertTypeOrInvalid(TypeInt); return _value.asInt; }
    operator const long int&() const { assertTypeOrInvalid(TypeInt); return _value.asInt; }
    operator double&()  { assertTypeOrInvalid(TypeDouble); return _value.asDouble; }
    operator const double&() const { assertTypeOrInvalid(TypeDouble); return _value.asDouble; }
    operator std::string&()  { assertTypeOrInvalid(TypeString); return *_value.asString; }
    operator const std::string&() const { assertTypeOrInvalid(TypeString); return *_value.asString; }
    operator cppTime&()  { assertTypeOrInvalid(TypeTime); return *_value.asTime; }
    operator const cppTime&() const { assertTypeOrInvalid(TypeTime); return *_value.asTime; }
    operator cppDuration&()  { assertTypeOrInvalid(TypeDuration); return *_value.asDuration; }
    operator const cppDuration&() const { assertTypeOrInvalid(TypeDuration); return *_value.asDuration; }

    ValueStruct& get_struct() { assertStruct(); return *_value.asStruct; }
    ValueArray&  get_array()  { assertArray(1); return *_value.asArray; }
    const ValueStruct& get_struct() const { assertStruct(); return *_value.asStruct; }
    const ValueArray&  get_array() const  { assertArray(1); return *_value.asArray; }

    flexval&	operator[](int i) const { assertArray(i + 1); return _value.asArray->at(i); }
    flexval&	operator[](int i) { assertArray(i + 1); return _value.asArray->at(i); }

    flexval&	operator[](const std::string& k) { assertStruct(); return (*_value.asStruct)[k]; }
    flexval&	operator[](const char* k) { assertStruct(); std::string s(k); return (*_value.asStruct)[s]; }

    bool	valid() const { return _type != TypeInvalid; }
    const Type&	getType() const { return _type; }
    int		size() const;
    void	setSize(int size) { assertArray(size); }
    bool	hasMember(const std::string& name) const;
    std::ostream&	write(
				std::ostream& os,
				bool pretty = false,
				int indent = 0) const;
    std::string		to_string() const;
    void		from_json(json_object*);
    json_object*	to_json() const;

    //
    // Same as to_json(), but replaces any invalid elements
    // by an empty list or an empty struct whenever possible.
    // The aux argument contains information that was extracted
    // from .tol.aux files.
    //
    json_object*	to_json(const flexval& aux) const;

    //
    // Merges structs in a way that supports tol2json:
    //
    void		merge(const flexval&);

    friend std::ostream& operator<<(std::ostream& os, const flexval& v);
    friend flexval operator+(const flexval& a, const flexval& b);
    friend flexval operator-(const flexval& a);
    friend flexval operator-(const flexval& a, const flexval& b);
    friend flexval operator*(const flexval& a, const flexval& b);
    friend flexval operator/(const flexval& a, const flexval& b);
    friend flexval operator+(const flexval& a, double b);
    friend flexval operator-(const flexval& a, double b);
    friend flexval operator*(const flexval& a, double b);
    friend flexval operator/(const flexval& a, double b);
    friend flexval operator+(double a, const flexval& b);
    friend flexval operator-(double a, const flexval& b);
    friend flexval operator*(double a, const flexval& b);
    friend flexval operator/(double a, const flexval& b);
    friend flexval operator+(const flexval& a, long int b);
    friend flexval operator-(const flexval& a, long int b);
    friend flexval operator*(const flexval& a, long int b);
    friend flexval operator/(const flexval& a, long int b);
    friend flexval operator+(long int a, const flexval& b);
    friend flexval operator-(long int a, const flexval& b);
    friend flexval operator*(long int a, const flexval& b);
    friend flexval operator/(long int a, const flexval& b);


#ifdef have_xmltol
    void	print_to_xmlpp(xmlpp::Element*);
#endif /* have_xmltol */

protected:
    void invalidate();

    void assertArray(int size) const;
    void assertArray(int size);
    void assertStruct() const;
    void assertStruct();
    void assertTypeOrInvalid(Type t);
    void assertTypeOrInvalid(Type t) const;

    Type		_type;

    union {
	ValueArray*	asArray;
	ValueStruct*	asStruct;
	bool		asBool;
	long int	asInt;
	double		asDouble;
	std::string*	asString;
	cppTime*	asTime;
	cppDuration*	asDuration;
    } _value;
};

#endif /* _FLEX_VAL_H_ */
