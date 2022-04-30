#ifndef _TYPED_VALUE_H_
#define _TYPED_VALUE_H_

#include	"timeptr.H"
#include	"smartptr.H"
#include	"C_list.H"
#include	"APbasic.H"
#include	"C_global.H"
#include	"RES_exceptions.H"
#include	"UTL_defs.H"
#include 	"json.h"
#include	"array_key.H"

#ifdef have_xmltol
namespace xmlpp {
	class Element;
}
#endif /* have_xmltol */

#include 		<BehavingElement.H>

class ActivityInstance;
class flexval;
class ArrayElement;
class ListOVal;
class Rsource;

union value_ptr {
	behaving_element*	INST;
	double			FLOATING;
	CTime_base*		TM;
	long			INTEGER;
	Cstring*		CST;
	ListOVal*		LI;
	void*			UN;
};

//
// Provides the fundamental "unit of data" for the APGEN core.
// Any data item that originates from the adaptation or that is
// stored in the Plan is expressed as one or more instance(s)
// of the TypedValue class.
//
// Be wary of the Array feature of this class.  It was implemented such that
// after
//
// 	A = [0,1]; B = A; B->append(2);
//
// the value of B is
//
//	B = [0,1,2]
//
// as expected. However, perhaps not as expected, A has changed too:
//
//	A = [0,1,2].
//
// This is because setting B equal to A makes it a reference, not a copy.
// When you change a reference, it changes the original.
//
// Since apgen depends on this behavior, it is documented and maintained.
//
class TypedValue {
public:

//
// DATA MEMBERS
//

	//
	// The value member (a union of possible data types)
	// It contains the raw data. Possible types:
	//
	//  double		if type == FLOATING
	//  CTime_base*		if type == DURATION/TIME
	//  long int		if type == INTEGER/BOOL_TYPE
	//  Cstring*		if type == STRING
	//  behaving_element*	if type == INSTANCE
	//  ListOVal*		if type == ARRAY
	//  NULL		if type == UNINITIALIZED
	//
	value_ptr		value;

	apgen::DATA_TYPE	declared_type;
private:
	apgen::DATA_TYPE	type;
	bool			Constant;
public:


	bool			value_is_a_pointer() const;

  	enum class arrayType: char {
		UNDEFINED,
		LIST_STYLE,
		STRUCT_STYLE
	};

	TypedValue();
	TypedValue(apgen::DATA_TYPE dec_type);
	TypedValue(const TypedValue& t);

	//
	// copy a raw value, convert to given type
	//
	TypedValue(	const value_ptr&	V,
			apgen::DATA_TYPE	T);

	//
	// Forces value to have the given type
	//
	TypedValue(	const TypedValue&	td,	
			apgen::DATA_TYPE	T);

	TypedValue(const char* s);
	TypedValue(const string& s);
	TypedValue(const Cstring& s);
	TypedValue(ListOVal& s);
	TypedValue(const CTime_base& s);
	TypedValue(const double& s);
	TypedValue(const bool& s);
	TypedValue(const unsigned long& s);
	TypedValue(const long long& s);
	TypedValue(const long& s);
	TypedValue(ActivityInstance& s);
	TypedValue(const behaving_element& s);
public:
	// deletes the value if != NULL. Made virtual to suppport subclasses.
	virtual ~TypedValue();

	static int		maxLineLength;
	static bool		tabs;

	void			generate_default_for_type(const Cstring&);
	void			generate_default_for_type(apgen::DATA_TYPE);
	void			dump(aoString& o) const;

				//
				// Used by the gtk activity editor:
				//
	void			extract_array_info(
						int				indent,
						const Cstring&			prefix,
						map<string, string>&		names,
						map<string, string>&		types,
						map<string, list<string> >&	ranges) const;

	void			print_to_XML(	aoString&,
						int sd = MAX_SIGNIFICANT_DIGITS,
						int indentation = 0) const;
				// to support early version of -C option (obsolete)
	void			print_to_C(aoString&, int) const;
				// to support xmlrpc_parser:
	void			print_to_xmlrpc(aoString&, int indentation) const;
	json_object*		print_to_json() const;
#ifdef have_xmltol
				// to support xml TOL:
	void			print_to_xmlpp(xmlpp::Element* parent_node) const;
#endif /* have_xmltol */
	json_object*		to_json() const;
	void			from_json(json_object*);
				// for long arrays
	void			print(	aoString&	a,
					int		indentation,
					int		significant_digits = MAX_SIGNIFICANT_DIGITS,
					bool		one_line = false
					) const;
	void			print_storage_allocation_to_C(
					const Cstring&	name,
					aoString&	a,
					int		s_d,
					int		indenta) const;
	Cstring			to_string() const {
		Cstring c;
		print(c, MAX_SIGNIFICANT_DIGITS);
		return c;
	}
				// 2nd arg is for precision
	void			print(Cstring&, int) const;
				// to simplify DBG macro usage
	const char*		print_to_stdout() const;
	const char*		serial() const {
		static Cstring tmp;
		tmp = to_string();
		return *tmp;
	}

				// for ARRAYs

	void			recursively_copy(
						const ListOVal& B);
	static TypedValue	flexval2typedval(
						flexval& F);
	void			clear();

				// Casting

				// casts to given type
	void			cast(
						apgen::DATA_TYPE);
				// casts to declared type if defined
	void			cast();
	void			make_constant();

private:
				// assumes that types are IDENTICAL
	virtual void		set_value(const value_ptr&, apgen::DATA_TYPE);
	virtual void		set_compatible_value(const value_ptr&);
				// assumes that types are COMPATIBLE
	void			adopt_value_from(const TypedValue&);
public:
				/* releases allocated memory and resets type
				 * to UNINITIALIZED */
	void			undefine();
				// returns 1 if ok to cast
	bool			is_compatible_with_type(apgen::DATA_TYPE) const;
	static bool		compatible_types(apgen::DATA_TYPE, apgen::DATA_TYPE);

				// convenience/utility functions
	bool			is_boolean() const;
	bool			is_int() const;
	bool			is_double() const;
	bool			is_time() const;
	bool			is_duration() const;
	bool			is_numeric() const;
	bool			is_string() const;
	bool			is_array() const;
	bool			is_list() const;
	bool			is_struct() const;
	bool			is_instance() const;
	bool			is_resource() const;

	long			get_int() const;

	double			get_double() const;

	bool			get_bool() const;

	CTime_base		get_time_or_duration() const;

				//
				// This method returns a reference to a CTime object, avoiding
				// the use of a constructor. Also gets duration; resulting
				// value has the details.
				//
	const CTime_base&	get_time_or_duration_ref() const;

	Cstring&		get_string() const;

				/*
				 The following function's 'const' label is somewhat misleading, because the
				 client can manipulate the returned reference without any restrictions
				 whatsoever; this really modifies the supposedly 'const' object. However,
				 this is really convenient, and we therefore exploit this little semantic
				 hole shamelessly.

				 A more rigorous implementation would return a reference to a const ListOVal,
				 but of course our little tricks would stop working and we would need a brand
				 new, non-const method anyway. */
	ListOVal&		get_array() const {
		return *value.LI;
	}
	behaving_element&	get_object() const;


				//
				// copies the supplied array into 'this'. Fails
				// if this is not an array, or if this array is
				// not of the same type as the array supplied.
				//
	void			merge(
					const ListOVal*);

				//
				// Similar to merge but does not modify the
				// operand.
				//
	TypedValue&		add(
					const ListOVal&);

	static void*		hasher(const char*);
	virtual operator void*() const {
		switch(type) {
			case apgen::DATA_TYPE::INSTANCE:
				return (void*)(*value.INST).object();
			case apgen::DATA_TYPE::FLOATING:

				//
				// The compiler won't let us cast from double to void*
				//
				return (void*)value.INTEGER;
			case apgen::DATA_TYPE::TIME:
			case apgen::DATA_TYPE::DURATION:
				return (void*)(value.TM->get_seconds() * 1000 + value.TM->get_milliseconds());
			case apgen::DATA_TYPE::INTEGER:
			case apgen::DATA_TYPE::BOOL_TYPE:
				return (void*)value.INTEGER;
			case apgen::DATA_TYPE::STRING:
				{
				return hasher(value.CST->p.s);
				}
			case apgen::DATA_TYPE::ARRAY:
			case apgen::DATA_TYPE::UNINITIALIZED:
			default:
				return value.UN;
		}
		return value.UN;
	}

				// = operator
	virtual TypedValue&	operator = (
					const TypedValue&);
	// virtual TypedValue&	operator = (
	// 				const double* s);
	virtual void		adopt(
					ActivityInstance&);
	virtual void		adopt(
					const behaving_element&);
	virtual void		adopt(
					const long&);
	virtual void		adopt(
					const long long&);
	virtual void		adopt(
					const unsigned long& ul){
		adopt((long) ul);
	}
	virtual void		adopt(
					const bool& B);
				// To create a single floating-point value
	virtual void		adopt(
					const double& );
	virtual void		adopt(
					const CTime_base&);
	virtual void		adopt(
					ListOVal&);
	virtual void		adopt(
					const Cstring&);
	virtual void		adopt(
					const char* str);
	virtual void		adopt(
					const string& str) {
		adopt(str.c_str());
	}

	virtual bool		operator == (
					const TypedValue& v) const {
		return is_equal_to(v);
	}
	bool			is_equal_to(
					const TypedValue& a,
					Rsource* res = NULL) const;

	const apgen::DATA_TYPE	get_type() const {
		return type;
	}
	const apgen::DATA_TYPE	get_declared_type() const {
		return declared_type;
	}
	const value_ptr		getValue() const {
		return value;
	}

	friend apgen::RETURN_STATUS exp_EXTRACT(
					Cstring&,
					TypedValue*,
					int);
	friend apgen::RETURN_STATUS exp_STRREL(
					Cstring&,
					TypedValue*,
					int);
	friend class time_saver;
};

//
// For experimenting with object size. See profile() in APmain.C,
// which reports on sizes. It is unfortunate that TypedValue
// takes 24 bytes while comparison_only only requires 16.
//
// class comparison_only {
// public:
// 	value_ptr	V;
// 	char		c1;
// 	char		c2;
// 	char		c3;
// 	bool		b;
// };

/* A TypedValue whose declared type 'sticks' to that of the
 * first object it is set equal to */
class StickyValue: public TypedValue {
public:
	// void constructor...
	StickyValue() {}
	// constructor for a strongly typed data element
	StickyValue(apgen::DATA_TYPE dec_type)
		: TypedValue(dec_type) {}
	// copy constructor
	StickyValue(const TypedValue& t)
		: TypedValue(t) {}
	// copy a raw value, convert to given type
	StickyValue(	const value_ptr&	V,
			apgen::DATA_TYPE	T)
		: TypedValue(V, T) {}
	// Forces value to have the given type
	StickyValue(	const TypedValue&	td,	
			apgen::DATA_TYPE	T)
		: TypedValue(td, T) {}
	~StickyValue() {}

	//
	// We need to be careful here, because this operator
	// is used by global_behaving_object::add_variable
	// to properly set the values in the global object.
	// The declared_type must be defined, even if the
	// type is not.
	//
	StickyValue&		operator = (
					const TypedValue& s) {
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = s.declared_type;
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}

	StickyValue&		operator = (
					ActivityInstance& s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
	StickyValue&		operator = (
					const behaving_element& s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
	StickyValue&		operator = (
					const long& s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
	StickyValue&		operator = (
					const long long& s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
	StickyValue&		operator = (
					const unsigned long& s){
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
	StickyValue&		operator = (
					const bool& s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
				// To create a single floating-point value
	StickyValue&		operator = (
					const double& s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}

	StickyValue&		operator = (
					const CTime_base& s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
	StickyValue&		operator = (
					ListOVal& s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
	StickyValue&		operator = (
					const Cstring& s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
	StickyValue&		operator = (
					const char* s) {
		TypedValue::operator=(s);
		if(declared_type == apgen::DATA_TYPE::UNINITIALIZED) declared_type = get_type();
		return *this;
	}
};

class ArrayElement {
public:
	array_string	key;
	TypedValue	payload;
	const Cstring&	get_key() const {
		return key.get_key();
	}
	unsigned long	get_2nd_key() const {
		return key.get_2nd_key();
	}

	ArrayElement() {}
	ArrayElement(const Cstring& name, apgen::DATA_TYPE dt)
		: key(name),
			payload(dt) {}
	ArrayElement(const Cstring& name, const TypedValue& V)
		: key(name),
			payload(V) {}
	ArrayElement(long int i, apgen::DATA_TYPE dt)
		: key(i),
			payload(dt) {}
	ArrayElement(long int i, const TypedValue& V)
		: key(i),
			payload(V) {}
	ArrayElement(long I)
		: key(I) {}
	ArrayElement(const Cstring& s)
		: key(s) {}
	ArrayElement(const array_string& ek)
		: key(ek) {}
	ArrayElement(const ArrayElement& ae)
		: key(ae.key),
			payload(ae.payload) {}
	~ArrayElement() {}

	ArrayElement*			copy() {
		return new ArrayElement(*this);
	}
	ArrayElement*			get_this() {
		return this;
	}
	const ArrayElement*		get_this() const {
		return this;
	}
	TypedValue::arrayType		get_array_type() const {
		return key.style_is_struct ?
			TypedValue::arrayType::STRUCT_STYLE
			: TypedValue::arrayType::LIST_STYLE;
	}

	TypedValue&			Val() {
		return payload;
	}
	void				SetVal(const TypedValue& v) {
		payload = v;
	}
	void				set_typed_val(
						const TypedValue& v) {
		payload = v;
		payload.cast();
	}
};

class ListOVal {
private:
	vector<ArrayElement*>	elements;
	map<Cstring, int>	keys;
	long			length;
	int			nref;

	//
	//possible types:
	// UNDEFINED,
	// LIST_STYLE,
	// STRUCT_STYLE
	//
	TypedValue::arrayType	lv_type;

	unsigned char		owner;
public:

	//
	// Setting the 'owner' member is predicated on the assumption
	// that while one thread is constructing an object, no other
	// thread can possibly consult it since a pointer to the
	// new object is not yet available.
	//
	// However, it is still possible that a thread consulting
	// e. g. the TypedValue owning this ListOVal would get
	// the wrong impression about value.LI. But this can only
	// be changed by the owning thread. Changing a value that
	// is accessible to other threads is forbidden, although
	// not currently enforced in the code.
	//
	ListOVal()
		: nref(0),
			elements(5),
			owner(thread_index),
			lv_type(TypedValue::arrayType::UNDEFINED),
			length(0) {
	}
	ListOVal(const ListOVal& lov)
		: nref(0),
			lv_type(lov.lv_type),
			elements(5),
			owner(thread_index),
			length(lov.length) {
	}
	ListOVal(unsigned long n)
		: nref(0),
			lv_type(TypedValue::arrayType::UNDEFINED),
			length(0),
			owner(thread_index),
			elements(n) {
	}

	~ListOVal() {
		clear();
	}

	ListOVal&	operator=(const ListOVal& lov) {
		assert(false); return *this;
	}

// private:

	//
	// We need to check array ownership before letting
	// the code change the reference count
	//
	void		ref() {
		if(thread_index == owner) {
			nref++;
		}
	}
	int		deref() {
		if(thread_index == owner) {
			return --nref;
		} else {
			return 1;
		}
	}
	int		num_refs() {
		if(thread_index == owner) {
			return nref;
		} else {
			return 1;
		}
	}
			//
			// Sets the type, which differentiates between numeric and string
			// indexing styles.
			//
	void		set_array_type(TypedValue::arrayType i) {
		lv_type = i;
	}
			// Access function
	TypedValue::arrayType	get_array_type() const {
		return lv_type;
	}

	ListOVal&	operator << (ArrayElement* ae) {
		return add(ae);
	}

	static const char* spell(TypedValue::arrayType T) {
	    switch(T) {
		case TypedValue::arrayType::LIST_STYLE:
			return "list style";
		case TypedValue::arrayType::STRUCT_STYLE:
			return "struct style";
		case TypedValue::arrayType::UNDEFINED:
		default:
			return "undefined";
	    }
	}

	//
	// Always adds the given array element to the struct
	//
	ListOVal&	add(ArrayElement* ae) {
		if(lv_type == TypedValue::arrayType::UNDEFINED) {
			lv_type = ae->get_array_type();
		} else if(lv_type != ae->get_array_type()) {
			Cstring tmp("Array type ");
			tmp << spell(lv_type)
				<< " conflicts with added element type "
				<< spell(ae->get_array_type())
				<< "\n";
			throw(eval_error(tmp));
		}
		if(lv_type == TypedValue::arrayType::STRUCT_STYLE) {
			keys[ae->get_key()] = length;
		}
		if(length >= elements.size()) {
			elements.resize(5 + 2 * elements.size());
		}
		elements[length++] = ae;
		return *this;
	}

	//
	// Adds an array element only if necessary
	//
	ListOVal&	add(long int i, const TypedValue& v, ArrayElement** aeptr = NULL) {
		ArrayElement* ae = find(i);
		if(ae) {
			ae->Val() = v;
		} else {
			for(long int k = length; k < i; k++) {
				ae = new ArrayElement(i, apgen::DATA_TYPE::UNINITIALIZED);
				add(ae);
			}
			ae = new ArrayElement(i, v);
			add(ae);
		}
		if(aeptr) {
			*aeptr = ae;
		}
		return *this;
	}

	//
	// Adds an array element only if necessary
	//
	ListOVal&	add(const Cstring& s, const TypedValue& v, ArrayElement** aeptr = NULL) {
		ArrayElement* ae = find(s);
		if(ae) {
			ae->Val() = v;
		} else {
			ae = new ArrayElement(s, v);
			add(ae);
		}
		if(aeptr) {
			*aeptr = ae;
		}
		return *this;
	}
	long		get_length() const {
		return length;
	}
	ArrayElement*	first_node() const {
		return length ? elements[0] : NULL;
	}
	ArrayElement*	earliest_node() const {
		if(lv_type == TypedValue::arrayType::STRUCT_STYLE) {
			map<Cstring, int>::const_iterator it = keys.begin();
			if(it != keys.end()) {
				return elements[it->second];
			}
		}
		return NULL;
	}
	ArrayElement*	last_node() const {
		return length > 0 ?
			elements[length - 1]
			: NULL;
	}
	ArrayElement*	find(const Cstring& s) const {
		if(lv_type == TypedValue::arrayType::STRUCT_STYLE) {
			map<Cstring, int>::const_iterator it = keys.find(s);
			if(it != keys.end()) {
				return elements[it->second];
			}
		}
		return NULL;
	}
	ArrayElement*	find(long int i) const {
		return (i >= 0 && length > i) ? elements[i] : NULL;
	}

	//
	// Never inserts anything; returns NULL if not found
	//
	ArrayElement*	operator[](const unsigned int i) const {
		return (i >= 0 && length > i) ? elements[i] : NULL;
	}

	//
	// Never inserts anything; returns NULL if not found
	//
	ArrayElement*	operator[](const Cstring& s) const {
		return find(s);
	}

	void		delete_el(long i) {
		if(i < 0 || i >= length) {
			Cstring err;
			err << "delete_el: index " << i << " is >= list size = " << length;
			throw(eval_error(err));
		}
		if(lv_type == TypedValue::arrayType::STRUCT_STYLE) {
			map<Cstring, int>::iterator it = keys.find(elements[i]->get_key());
			keys.erase(it);
			delete elements[i];
			for(int z = i; z < length - 1; z++) {
				ArrayElement* ae = elements[z + 1];
				elements[z] = ae;
				keys[ae->get_key()] = z;
			}
		} else {
			delete elements[i];
			for(int z = i; z < length - 1; z++) {
				elements[z] = elements[z + 1];
			}
		}
		length--;
	}
	void		delete_el(const Cstring& s) {
		if(lv_type != TypedValue::arrayType::STRUCT_STYLE) {
			Cstring err;
			err << "delete_el: array is a list, cannot find index " << s;
			throw(eval_error(err));
		}
		map<Cstring, int>::iterator it = keys.find(s);
		if(it == keys.end()) {
			Cstring err;
			err << "delete_el: string " << s << " not found in array";
			throw(eval_error(err));
		}
		int z = it->second;
		delete elements[z];
		keys.erase(it);
		for(; z < length - 1; z++) {
			ArrayElement* ae = elements[z + 1];
			elements[z] = elements[z + 1];
			keys[ae->get_key()] = z;
		}
		length--;
	}
	void		clear() {
		for(int i = 0; i < length; i++) {
			delete elements[i];
		}
		elements.clear();
		if(lv_type == TypedValue::arrayType::STRUCT_STYLE) {
			keys.clear();
		}
		length = 0;
		lv_type = TypedValue::arrayType::UNDEFINED;
	}
	long		get_index(
				ArrayElement* ae) {
		if(!ae->key.style_is_struct) {
			Cstring err;
			err << "get_index() does not work on array elements "
				"without a string key";
			throw(eval_error(err));
		}
		map<Cstring, int>::const_iterator it = keys.find(ae->get_key());
		if(it != keys.end()) {
			return it->second;
		}
		return -1;
	}

	//
	// To support tol2json, which turns TOL info into json.
	// When dealing with activity parameters, the json file
	// needs to know whether any parameters that are arrays
	// are lists or structs. It also needs to know whether
	// the elements in those lists and structs are themselves
	// arrays, and whether those arrays are lists or structs,
	// recursively.
	//
	// document_arrays_in() provides that information via
	// a flexval object that contains the required information
	// as a hierarchy of flexval values. See the code in
	// APdata.C for details.
	//
	void			document_arrays_in(
						flexval& F);
	friend class TypedValue;
};

typedef smart_ptr<ListOVal>	array_ptr;

#endif /* _TYPED_VALUE_H_ */
