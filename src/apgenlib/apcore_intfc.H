#ifndef _AP_CORE_INTFC_H
#define _AP_CORE_INTFC_H

// UNIX includes:
#include <string.h>
#include <time.h>

// STL includes:
#include <list>
#include <map>
#include <string>
#include <utility>	// for std::pair
#include <vector>

using namespace std;

class apcoreValue;

					// Quoting and Unquoting utilities
extern string				add_quotes(const string &s);
extern void 				remove_quotes(string &s);

typedef vector<apcoreValue *>		apcoreArray;


extern "C" {
#include "concat_util.h"
} // extern "C"

class compact_int {
public:
	long		i;
	bool		is_infinite;
	compact_int(int j) :i(j), is_infinite(false) {}
	/* Note: any string will do; suggestion: compact_int("infinity")
	 */
	compact_int(const char *) :i(0L), is_infinite(true) {}
	compact_int(const compact_int &C)
		:i(C.i), is_infinite(C.is_infinite) {}
	~compact_int() {}
	operator int() {return i;}
	bool operator < (long k) { return (is_infinite ? false : i < k); }
	bool operator <= (long k) { return (is_infinite ? false : i <= k); }
	bool operator == (long k) { return (is_infinite ? false : i == k); }
	bool operator != (long k) { return (is_infinite ? true : i != k); }
	bool operator > (long k) { return (is_infinite ? true : i > k); }
	bool operator >= (long k) { return (is_infinite ? true : i >= k); }
	compact_int &operator -=(long K) { if(!is_infinite) i -= K; return *this; }
		    // pre-increment
	compact_int &operator --() { if(!is_infinite) i--; return *this; }
		    // post-increment (arg is unused...)
	compact_int &operator --(int) { if(!is_infinite) i--; return *this; }
	compact_int &operator = (long L) { i=L; is_infinite=false; return *this; } };

class apcoreValue {
public:
	static string			&get_default_string();
	static apcoreArray		&get_default_apcoreArray();
	static std::list<std::string>	&emptyList();

	string				Key;
	string				Description;
	std::list<string>		Range;

	apcoreValue() {}
	apcoreValue(		const string &theKey,
				const string &theDescription,
				const std::list<string> &theRange)
		: Key(theKey),
		Description(theDescription),
		Range(theRange) {}
	apcoreValue(const apcoreValue &a)
		: Key(a.Key),
		Description(a.Description),
		Range(a.Range) {}
	virtual ~apcoreValue() {}
	virtual bool			is_string() const { return false; }
	virtual bool			is_int() const { return false; }
	virtual bool			is_double() const { return false; }
	virtual bool			is_time() const { return false; }
	virtual bool			is_duration() const { return false; }
	virtual bool			is_list() const { return false; }
	virtual bool			has_labels() const { return false; }
	virtual const string		&get_string() const { return get_default_string(); }
	const string			&get_key() const { return Key; }
	const string			&get_descr() const { return Description; }
	virtual apcoreArray		&get_array() { return get_default_apcoreArray(); }
	virtual void			set_string_to(const string &) {}
	virtual long			get_int() { return 0; }
	virtual double			get_double() { return 0.0; }

	// pure virtual methods
	virtual apcoreValue		*copy() const { return NULL; }
	virtual const std::string	&get_type() const;
	virtual long			length() { return 0; }
					// returns 0 if OK, 1 if truncation took place
	virtual long			transfer_to_stream(buf_struct *s,
						compact_int &max_length) { return 0L; } };

class de_facto_string: public apcoreValue {
	string			contents;
	string			type;
public:
	de_facto_string(	const string		&the_key,
				const string		&the_description,
				const list<string>	&the_range,
				const string		&the_contents,
				const string		&the_type)
		: apcoreValue(the_key, the_description, the_range),
		contents(the_contents),
		type(the_type) {}
	de_facto_string(const string &s)
		: apcoreValue("", "", emptyList()),
		contents(s),
		type("string") {}
	de_facto_string(const de_facto_string &d)
		: apcoreValue(d),
		contents(d.contents),
		type(d.type) {}
	~de_facto_string() {}
				// NOTE: this method is somewhat misleading; it really
				// means that the value represented by this object is
				// a single value (representable as a string) and not
				// an array of values. The actual data type of the value
				// can be obtained by invoking get_type(); see below.
	bool			is_string() const { return true; }
	const string		&get_string() const { return contents; }
	void			set_string_to(const string &S) { contents = S; }

	// pure virtual methods
	apcoreValue		*copy() const;
				// returns the actual APGEN data type as a string
	const std::string	&get_type() const;
	long			length() { return contents.length(); }
	long			transfer_to_stream(buf_struct *s, compact_int &max_length); };

class de_facto_int: public apcoreValue {
	long	contents;
public:
	de_facto_int(long K) : apcoreValue("", "", emptyList()),
		contents(K) {}
	de_facto_int(const de_facto_int &d) : contents(d.contents) {}
	~de_facto_int() {}

	bool	is_int() const { return true; }
	long	get_int() { return contents; }
	apcoreValue		*copy() const { return new de_facto_int(contents); }
	const std::string	&get_type() const;
	long			length() { return 0; }
	long			transfer_to_stream(buf_struct *s, compact_int &max_length); };


typedef std::pair<long, long> apcoreTime;

class de_facto_time: public apcoreValue {
	long	seconds, milliseconds;
public:
	de_facto_time(apcoreTime K) : apcoreValue("", "", emptyList()),
		seconds(K.first), milliseconds(K.second) {}
	de_facto_time(const de_facto_time &d)
		: seconds(d.seconds), milliseconds(d.milliseconds) {}
	~de_facto_time() {}

	bool	is_time() const { return true; }
	apcoreTime		get_time() { return std::pair<long, long>(seconds, milliseconds); }
	apcoreValue		*copy() const { return new de_facto_time(
					std::pair<long, long>(seconds, milliseconds)); }
	const std::string	&get_type() const;
	long			length() { return 0; }
	long			transfer_to_stream(buf_struct *s, compact_int &max_length); };

class de_facto_duration: public apcoreValue {
	long	seconds, milliseconds;
public:
	de_facto_duration(apcoreTime K) : apcoreValue("", "", emptyList()),
		seconds(K.first), milliseconds(K.second) {}
	de_facto_duration(const de_facto_duration &d)
		: seconds(d.seconds), milliseconds(d.milliseconds) {}
	~de_facto_duration() {}

	bool	is_duration() const { return true; }
	apcoreTime		get_time() { return std::pair<long, long>(seconds, milliseconds); }
	apcoreValue		*copy() const { return new de_facto_duration(
					std::pair<long, long>(seconds, milliseconds)); }
	const std::string&	get_type() const;
	long			length() { return 0; }
	long			transfer_to_stream(buf_struct *s, compact_int &max_length); };

class de_facto_double: public apcoreValue {
	double	contents;
public:
	de_facto_double(double K) : apcoreValue("", "", emptyList()),
		contents(K) {}
	~de_facto_double() {}

	bool			is_double() const { return true; }
	double			get_double() { return contents; }
	apcoreValue		*copy() const { return new de_facto_double(contents); }
	const std::string	&get_type() const;
	long			length() { return 0; }
	long			transfer_to_stream(buf_struct *s, compact_int &max_length); };

class de_facto_array: public apcoreValue {
	apcoreArray	contents;
	bool		HasLabels;
public:
	de_facto_array(	const string &k,
			const string &descr,
			const list<string> &range,
			bool label_flag)
		: apcoreValue(k, descr, range),
		HasLabels(label_flag) {}
	~de_facto_array(); // memory leak... {}
	bool			is_list() const { return true; }
	bool			has_labels() const { return HasLabels; }
				// Not const, so client is allowed to push_back on contents.
	apcoreArray	&get_array() { return contents; }

	// pure virtual methods
	apcoreValue		*copy() const;
	const std::string	&get_type() const;
	long			transfer_to_stream(buf_struct *s, compact_int &max_length);
	long			length(); };

class apcoreTimeval: public std::pair<apcoreTime, apcoreValue*> {
public:
	apcoreTimeval(apcoreTime t, apcoreValue *av)
		: std::pair<apcoreTime, apcoreValue *>(t, av) {}
	apcoreTimeval(const apcoreTimeval &tv)
		: std::pair<apcoreTime, apcoreValue *>(tv.first, tv.second->copy()) {}
	~apcoreTimeval(); };

typedef std::vector<apcoreTimeval> apcorePlan;

	// Internal (as opposed to XMLRPC) APcore interface specification
namespace APcoreIntfc {

	//
	// as a rule, normal return is 0
	//
	int		initialize(const vector<string> &args, string &errors);

	int		terminate();

	int		FILE_OPEN(	const std::string	&theStringToParse,
					std::string		&theErrorsFound);

	std::string	getLogMessage();

	std::vector<std::string> getLegends();

			// returns params w/o descriptions; time values omit milliseconds
	int		getActivityParameters(
				const std::string	&actTypeName,
				apcoreArray		&params,
				string			&any_errors);

	std::vector<apcoreTimeval>
			getResource(
				const std::string	&resource_name,
				std::string		&resource_type,
				std::string		&any_errors);

	std::vector<apcoreTimeval>
			getResourceElement(
				const std::string	&resource_name,
				const std::vector<string> &indices);

	int		deleteActivity(const string &act_id, string &errors);
			/* note: it would make sense to implement keyword/value
			 * pairs as maps indexed by keyword. However, the apcoreArray
			 * is a vector whose elements are apcoreValues, which have
			 * room for both a Key and a Value. So use just use the
			 * apcoreArray class to hold the kwd-value pairs.
			 */
	int		editActivity(
				const std::string	&the_act_id,
				const apcoreArray	&kwd_value_pairs);
	int		editActivitySymbol(
				const string		&act_id,
				const string		&symbolname,
				const string		&symbolvalue,

							// to handle Hoppers:
				const string		&current_legend,
				const string		&current_start,

				string			&anyErrors);
	int		getConsumption(
				const std::string	&the_act_id,
				const std::string	&the_resource,
				double			&total,
				bool			&is_used,
				std::string		&any_errors);
	int		getIntegratedConsumption(
				const std::string	&the_act_id,
				const std::string	&the_resource,
				double			&total,
				bool			&is_used,
				std::string		&any_errors);
	int		regenChildren(
				const std::string	&the_act_id,
				std::string		&any_errors);
	} // namespace APcoreIntfc

	// ad hoc class to hold information regarding e. g. standard attributes
class infotriplet {
public:
			/* Semantics:
			 * data[0] = name
			 * data[1] = type
			 * data[2] = description
			 */
	std::string	data[3];
	infotriplet() {}
	infotriplet(const infotriplet &i) {
		data[0] = i.data[0];
		data[1] = i.data[1];
		data[2] = i.data[2]; }
	~infotriplet() {}
	std::string &operator[](long k) {
		return data[k]; } };

				/* this vector contains info for all legal
				 * activity attributes:
				 */
std::vector<infotriplet>	&stdActAtts();

				/* search this map based on attribute name to
				 * get the attribute's index into stdActAtts():
				 */
std::map<std::string, long>	&mapOfStdActAtts();

#endif /* _AP_CORE_INTFC_H */
