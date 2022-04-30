#if HAVE_CONFIG_H
#include <config.h>
#endif

extern "C" {
#include <concat_util.h>
} // extern "C"

#include <assert.h>

// #define apDEBUG
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// for cputime():
#include <ctime>
#include <ratio>
#include <chrono>

#include "aafReader.H"
#include "apDEBUG.H"
#include "ActivityInstance.H"
#include "AP_exp_eval.H"
#include "act_plan_parser.H"
#include "xml_incon_parser.H"
#include "EventLoop.H"
#include "EventRegistry.H"
#include "ParsedExpressionSystem.H"
#include "EventImpl.H"
#include "instruction_node.H"
#include "apcoreWaiter.H"
#include "int_multiterator.H"

using namespace pEsys;

extern int	abdebug;

// MACROS for built-in functions:

#define RETRIEVE3( XX , YY , UU , ZZ ) \
		if( args.size() > 3 ) { \
				rmessage = Cstring( "too many args. in " ) + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		else if( args.size() < 3 ) { \
				rmessage = Cstring( "too few args. in " ) + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		TypedValue	*UU = args.pop_front() , \
				*YY = args.pop_front() , \
				*XX = args.pop_front() ;

#define RETRIEVE2( XX , YY , ZZ ) \
		if( args.size() > 2 ) \
				{ rmessage = Cstring( "too many args. in " ) + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		else if( args.size() < 2 ) \
				{ rmessage = Cstring( "too few args. in " ) + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		TypedValue	*YY = args.pop_front() , \
				*XX = args.pop_front() ;

#define RETRIEVE1( XX , ZZ ) \
		if( args.size() > 1 ) \
				{ rmessage = Cstring( "too many args. in " ) + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		else if( args.size() < 1 ) \
				{ rmessage = Cstring("too few args. in ") + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		TypedValue	*XX = args.pop_front() ;

#define RETRIEVE0( ZZ ) \
		if( args.size() > 0 ) \
				{ rmessage = Cstring( "too many args. in " ) + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; }

// needed to output errors
void			(*DisplayMessage_handler)(	const string &theTitle,
							const string &theText,
							bool addToLog) = NULL;

class sorted_time_node: public Time_node {
public:
	long	ind;
	CTime_base event_time;
	sorted_time_node(CTime_base t, long k) : event_time(t), ind(k) {}
	sorted_time_node(const sorted_time_node& stn)
		: event_time(stn.event_time), ind(stn.ind) {}
	~sorted_time_node() {}
		virtual void		setetime(
					const CTime_base& new_time) {
		event_time = new_time;
	}
	virtual const CTime_base& getetime() const {
		return event_time;
	}

	Node*			copy() {
		return new sorted_time_node(*this);
	}
	Node_type		node_type() const {
		return CONCRETE_TIME_NODE;
	}
	const Cstring&		get_key() const {
		return Cstring::null();
	}
};

class sorted_int_node: public int_node {
public:
	long	ind;
	sorted_int_node(long t, int k) : int_node(t), ind(k) {}
	~sorted_int_node() {}
};

apgen::RETURN_STATUS new_exp_AMIN(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	/*
	* STRICT TYPING:	both arguments have identical type; result has same type. Error if
	* 			cannot be cast as integer.
	* PERMISSIVE TYPING:	both args "loosely numeric". Dangerous (with times) but
	* 			we DO need to be backward-compatible. Result has same type
	* 			as first arg (arbitrary but that's what we choose). Error if
	* 			result is not an integer (strict rules). If the user wants to
	* 			convert between floats and integers, explicit conversion functions
	* 			(truncate, round) should be used.
	*/
	TypedValue	*b = args.pop_front();
	TypedValue	*a = NULL;
	TypedValue	R;

	if(!b) {
		rmessage = Cstring("too few args. in AMIN function");
		return apgen::RETURN_STATUS::FAIL;
	} else if(args.size() == 0) {
		rmessage = Cstring("too few args. in AMIN function");
		return apgen::RETURN_STATUS::FAIL;
	}
	while(args.size() > 0) {
		a = args.pop_front();
		if((!a->is_numeric()) || (!b->is_numeric())) {
			rmessage = "non-numeric args. in AMIN()";
			return apgen::RETURN_STATUS::FAIL;
		} else if(a->get_double() < b->get_double()) {
			R = *a;
		} else {
			R = *b;
		}
		b = &R;
	}
	*res = R;
	try {
		res->cast();
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS new_exp_TMIN(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	//
	// STRICT TYPING:	both arguments have identical type; result has same type. Error if
	// 			cannot be cast as time.
	//

	TypedValue	*b = args.pop_front();
	TypedValue	*a = NULL;
	TypedValue	R;

	if(!b) {
		rmessage = Cstring("too few args. in TMIN function");
		return apgen::RETURN_STATUS::FAIL;
	} else if(args.size() == 0) {
		rmessage = Cstring("too few args. in TMIN function");
		return apgen::RETURN_STATUS::FAIL;
	}
	while(args.size() > 0) {
		a = args.pop_front();
		if((!a->is_time()) || (!b->is_time())) {
			rmessage = "non-time args. in TMIN()";
			return apgen::RETURN_STATUS::FAIL;
		} else if(a->get_time_or_duration() < b->get_time_or_duration()) {
			R = *a;
		} else {
			R = *b;
		}
		b = &R;
	}
	*res = R;
	try {
		res->cast();
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS new_exp_DMIN(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	/*
	* STRICT TYPING:	both arguments have identical type; result has same type. Error if
	* 			cannot be cast as duration.
	*/
	TypedValue	*b = args.pop_front();
	TypedValue	*a = NULL;
	TypedValue	R;

	if(!b) {
		rmessage = Cstring("too few args. in DMIN function");
		return apgen::RETURN_STATUS::FAIL;
	} else if(args.size() == 0) {
		rmessage = Cstring("too few args. in DMIN function");
		return apgen::RETURN_STATUS::FAIL;
	}
	while(args.size() > 0) {
		a = args.pop_front();
		if((!a->is_duration()) || (!b->is_duration())) {
			rmessage = "non-duration args. in DMIN()";
			return apgen::RETURN_STATUS::FAIL;
		} else if(a->get_time_or_duration() < b->get_time_or_duration()) {
			R = *a;
		} else {
			R = *b;
		}
		b = &R;
	}
	*res = R;
	try {
		res->cast();
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS new_exp_AMAX(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	//
	// STRICT TYPING:	both arguments are floats; result is float.
	//

	TypedValue*	b = args.pop_front();
	TypedValue*	a = NULL;
	TypedValue	R;

	if(!b) {
		rmessage = Cstring("too few args. in AMAX function");
		return apgen::RETURN_STATUS::FAIL;
	} else if(args.size() == 0) {
		rmessage = Cstring("too few args. in AMAX function");
		return apgen::RETURN_STATUS::FAIL;
	}
	while(args.size() > 0) {
		a = args.pop_front();
		if((!a->is_numeric()) || (!b->is_numeric())) {
			rmessage = "non-numeric args. in AMAX()";
			return apgen::RETURN_STATUS::FAIL;
		} else if(a->get_double() > b->get_double()) {
			R = *a;
		} else {
			R = *b;
		}
		b = &R;
	}
	*res = R;
	try {
		res->cast();
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}

	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS new_exp_TMAX(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	//
	// STRICT TYPING:	both arguments are times; result is time.
	//

	TypedValue*	b = args.pop_front();
	TypedValue*	a = NULL;
	TypedValue	R;

	if(!b) {
		rmessage = Cstring("too few args. in TMAX function");
		return apgen::RETURN_STATUS::FAIL;
	} else if(args.size() == 0) {
		rmessage = Cstring("too few args. in TMAX function");
		return apgen::RETURN_STATUS::FAIL;
	}
	while(args.size() > 0) {
		a = args.pop_front();
		if((!a->is_time()) || (!b->is_time())) {
			rmessage = "non-time args. in TMAX()";
			return apgen::RETURN_STATUS::FAIL;
		} else if(a->get_time_or_duration() > b->get_time_or_duration()) {
			R = *a;
		} else {
			R = *b;
		}
		b = &R;
	}
	*res = R;
	try {
		res->cast();
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}

	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS new_exp_DMAX(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	//
	// STRICT TYPING:	both arguments are durations; result is duration.
	//

	TypedValue*	b = args.pop_front();
	TypedValue*	a = NULL;
	TypedValue	R;

	if(!b) {
		rmessage = Cstring("too few args. in DMAX function");
		return apgen::RETURN_STATUS::FAIL;
	} else if(args.size() == 0) {
		rmessage = Cstring("too few args. in DMAX function");
		return apgen::RETURN_STATUS::FAIL;
	}
	while(args.size() > 0) {
		a = args.pop_front();
		if((!a->is_duration()) || (!b->is_duration())) {
			rmessage = "non-duration args. in DMAX()";
			return apgen::RETURN_STATUS::FAIL;
		} else if(a->get_time_or_duration() > b->get_time_or_duration()) {
			R = *a;
		} else {
			R = *b;
		}
		b = &R;
	}
	*res = R;
	try {
		res->cast();
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}

	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_TO_SECONDS(Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args) {
	if(args.size() != 1) {
		rmessage = "TO_SECONDS() needs one time or duration argument";
		return apgen::RETURN_STATUS::FAIL;
	}
	TypedValue*	b = args.pop_front();
	if(!(b->is_duration() || b->is_time())) {
		rmessage = "TO_SECONDS() needs one time or duration argument";
		return apgen::RETURN_STATUS::FAIL;
	}
	*res = b->get_time_or_duration().convert_to_double_use_with_caution();
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_FROM_SECONDS(Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args) {
	if(args.size() != 1) {
		rmessage = "FROM_SECONDS() needs one numeric argument";
		return apgen::RETURN_STATUS::FAIL;
	}
	TypedValue*	b = args.pop_front();
	if(!(b->is_double() || b->is_int())) {
		rmessage = "FROM_SECONDS() needs one numeric argument";
		return apgen::RETURN_STATUS::FAIL;
	}
	*res = CTime_base::convert_from_double_use_with_caution(b->get_double(), /* is_a_duration = */ true);
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_MAX(Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args) {
	/*
	* STRICT TYPING:	both arguments have identical type; result has same type.
	* PERMISSIVE TYPING:	both args "loosely numeric". Dangerous (with times) but
	* 			we DO need to be backward-compatible. Result has same type
	* 			as first arg (arbitrary but that's what we choose).
	*/
	TypedValue	*b = args.pop_front();
	TypedValue	*a = args.pop_front();
	TypedValue	*c = NULL;
	if(!a) {
		rmessage = Cstring("too few args. in MAX function");
		return apgen::RETURN_STATUS::FAIL;
	}
	if(args.size() == 1) {
		c = b;
		b = a;
		a = args.pop_front();
		if(!c->is_numeric()) {
			rmessage = "non-numeric 3rd arg. in MAX()";
			return apgen::RETURN_STATUS::FAIL;
		}
	} else if(args.size() != 0) {
		rmessage = Cstring("wrong number of args. in MAX function");
		return apgen::RETURN_STATUS::FAIL;
	}
	if((! a->is_numeric()) || (!b->is_numeric())) {
		rmessage = "non-numeric args. in MAX()";
		return apgen::RETURN_STATUS::FAIL;
	}
	if(a->get_double() > b->get_double()) {
	       	*res = *a;
	} else {
		*res = *b;
	}
	if(c && (c->get_double() > res->get_double())) *res = *c;
	res->cast(apgen::DATA_TYPE::INTEGER);
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_MIN(Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_MIN")
	if((!a->is_numeric()) || (!b->is_numeric())) {
		rmessage = "non-numeric args. in MIN()";
		return apgen::RETURN_STATUS::FAIL;
	}
	if(a->get_double() < b->get_double()) {
		*res = *a;
	} else {
		*res = *b;
	}
	res->cast(apgen::DATA_TYPE::INTEGER);
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_FMOD(Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args) {
	/*
	* STRICT TYPING:	Seems that you want to use the same rules as the C function. We should
	* 			cast the result (which is the remainder of the division) as a float.
	*/
	RETRIEVE2(a, b, "exp_FMOD")
	double A, B;
	// NOTE: is_double() accepts time/duration in the permissive (default) case.
	if(a->is_time() && b->is_duration()) {
		A = a->get_time_or_duration().convert_to_double_use_with_caution();
		B = b->get_time_or_duration().convert_to_double_use_with_caution();
	} else if((!a->is_double()) || (!b->is_double())) {
		rmessage = "non-float args. in FMOD()";
		return apgen::RETURN_STATUS::FAIL;
	} else {
		A = a->get_double();
		B = b->get_double();
	}
	*res = fmod(A, B);
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_FP(Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_FP")
	if(!a->is_numeric()) {
		rmessage = "non-integer arg. in FP()";
		return apgen::RETURN_STATUS::FAIL;
	}
	*res = a->get_double();
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_FABS(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_FABS")
	if(!a->is_double()) {
		rmessage = "non-float arg. in FABS()";
		return apgen::RETURN_STATUS::FAIL; }
	double x = a->get_double();
	*res = (x < 0.) ? -1.*x : x;
	return apgen::RETURN_STATUS::SUCCESS; }


apgen::RETURN_STATUS exp_FIX(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_FIX")
	if(!a->is_double()) {
		rmessage = "non-float arg. in FIX()";
		return apgen::RETURN_STATUS::FAIL; }
	*res = a->get_double() - fmod(a->get_double(), 1.0);
	return apgen::RETURN_STATUS::SUCCESS; }


apgen::RETURN_STATUS exp_CEILING(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	TypedValue	*a = args.pop_front();
	TypedValue	*b = NULL;
	double		second_arg = 1.0;
	if(!a) {
		rmessage = Cstring("too few args. in CEILING function");
		return apgen::RETURN_STATUS::FAIL;
	}
	// get 2nd argument if necessary
	if(args.size() == 1) {
		b = a;
		a = args.pop_front();
		if(!b->is_numeric()) {
			rmessage = "non-numeric 2nd arg. in CEILING()";
			return apgen::RETURN_STATUS::FAIL; }
		second_arg = b->get_double();
	} else if(args.size() != 0) {
		rmessage = Cstring("wrong number of args. in CEILING function");
		return apgen::RETURN_STATUS::FAIL;
	}
	if(!a->is_numeric()) {
		rmessage = "non-numeric arg. in CEILING()";
		return apgen::RETURN_STATUS::FAIL;
	}
	double D = a->get_double();
	double F = fmod(a->get_double(), second_arg);
	long i = (long) D;
	if(F > 0) i++;
	*res = i;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_RND(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_RND")
	if(! a->is_numeric()) {
		rmessage = "non-numeric arg. in RND()";
		return apgen::RETURN_STATUS::FAIL; }
	double D = a->get_double();
	if(D >= 0L) *res = (long) (D + 0.5);
	else *res = (long) (D - 0.5);
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_REQUIRE(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_REQUIRE")
	if(! a->is_string()) {
		rmessage = "non-string arg. in REQUIRE()";
		return apgen::RETURN_STATUS::FAIL; }
	Cstring thefunc(a->get_string());
	*res = 1L;
	if(!aaf_intfc::internalAndUdefFunctions().find(thefunc)) {
		rmessage = "REQUIRE: Function ";
		rmessage << thefunc << " has not been defined. ";
		return apgen::RETURN_STATUS::FAIL; }
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_ABS(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_ABS")
	if(! a->is_numeric()) {
		rmessage = "non-numeric arg. in ABS()";
		return apgen::RETURN_STATUS::FAIL;
	}
	long I = (long) a->get_double();
	*res = I < 0 ? -1*I : I;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_ADDQUOTES(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_ADDQUOTES")
	if(! a->is_string()) {
		rmessage = "non-string arg in ADDQUOTES";
		return apgen::RETURN_STATUS::FAIL;
	}

	*res = addQuotes(a->get_string());
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_AND(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_AND")
	if((! a->is_int()) || (! b->is_int())) {
		rmessage = "non-integer arg. in AND()";
		return apgen::RETURN_STATUS::FAIL; }
	long I = a->get_int();
	long J = b->get_int();
	*res = I & J;
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_OR(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_OR")
	if((! a->is_int()) || (! b->is_int())) {
		rmessage = "non-integer arg. in OR()";
		return apgen::RETURN_STATUS::FAIL; }
	long I = a->get_int();
	long J = b->get_int();
	*res = I | J;
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_XOR(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_XOR")
	if((! a->is_int()) || (!b->is_int())) {
		rmessage = "non-integer arg. in XOR()";
		return apgen::RETURN_STATUS::FAIL; }
	long I = a->get_int();
	long J = b->get_int();
	*res = I ^ J;
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_XR(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_XR")
	if(!a->is_string()) {
		rmessage = "first arg is not a string in xcmd()";
		return apgen::RETURN_STATUS::FAIL;
	}
	if(!b->is_array()) {
		rmessage = "second arg is not an array in xcmd()";
		return apgen::RETURN_STATUS::FAIL;
	}
	Cstring request = a->get_string();
	ListOVal& Args = b->get_array();
	try {
		*res = process_an_AAF_request_handler(request, Args);
	} catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_TO_STRING(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_TO_STRING")
	Cstring	t;
	if(a->get_type() == apgen::DATA_TYPE::STRING) {
		t = a->get_string();
	} else {
		t = a->to_string();
	}
	*res = t;
	return apgen::RETURN_STATUS::SUCCESS;
}

	/*
	 * trim blanks from ends of string
	 *
	 * option = 1, trim leading blanks
	 * option = 2, trim trailing blanks
	 * option = 3, trim both leading and trailing
	 */
apgen::RETURN_STATUS exp_TRIM(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_TRIM")
	if((! a->is_string()) || (! b->is_int())) {
		rmessage = "bad arg. in TRIM()";
		return apgen::RETURN_STATUS::FAIL; }
	Cstring cs(a->get_string());
	long	option = b->get_int();

	if(option == 1L)
		*res = " " - cs;
	else if(option == 2L)
		*res = cs - " ";
	else if(option == 3L)
		// 'Cstring' didn't use to be there but Solaris wants it:
		/*

		NOTE!!!!!!!

		Solaris does NOT do what is expected here:

		*/
		*res = " " - (cs - " ");
	return apgen::RETURN_STATUS::SUCCESS;
}

	/*
	 *extract a substring from the specified string
	 *
	 * char *s;                string from which chars to be extracted
	 * int  inx;               index of position of 1st char
	 * int  count;             how many chars to extract
	 */
apgen::RETURN_STATUS exp_EXTRACT(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE3(a, b, c, "exp_EXTRACT")
	if((!a->is_string()) || (!b->is_int()) || (!c->is_int())) {
		rmessage = "bad arg. in EXTRACT()";
		return apgen::RETURN_STATUS::FAIL;
	}
	long	i = b->get_int(), count = c->get_int();
	char	*strp;
	long	len = a->value.CST->length();

	if(count < 0) {
		rmessage = "bad char. count argument ";
		rmessage << count << " in EXTRACT()";
		return apgen::RETURN_STATUS::FAIL;
	} else if(i + count > len) {
		rmessage = "error in EXTRACT(): attempting to extract chars. ";
		rmessage << (i + count) << " through " << i + count << " while string \"";
		rmessage << *a->value.CST << "\" only contains " << len << " ";
		return apgen::RETURN_STATUS::FAIL;
	}

	strp = (char *) malloc(count + 1);
	strncpy(strp, **a->value.CST + i, count);
	strp[count] = '\0';
	*res = strp;
	free(strp);
	return apgen::RETURN_STATUS::SUCCESS;
}

// dummy function (work is done in instruction::execute()):
apgen::RETURN_STATUS exp_STOP(Cstring &, TypedValue *, slst<TypedValue*>&) {
	return apgen::RETURN_STATUS::SUCCESS;
}

/******************************************************** STRREL *******/

	/*
	 * int rel;              s1 < s2, rel = 1; s1 == s2, rel = 2; s1 > s2, rel = 3
	 * char *s1, *s2;
	 */
apgen::RETURN_STATUS exp_STRREL(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE3(a, b, c, "exp_STRREL")
	long	intres;

	if((!a->is_string()) || (!b->is_string()) || (!c->is_int())) {
		rmessage = "bad arg. in STRREL()";
		return apgen::RETURN_STATUS::FAIL;
	}
	if(!a->value.CST->length()) {
		if(c->value.INTEGER == 1L && b->value.CST->length())
			*res = 1L;
		else
			*res = 0L;
		return apgen::RETURN_STATUS::SUCCESS;
	}
	if(!b->value.CST->length()) {
		if(c->value.INTEGER == 3L  && a->value.CST->length())
			*res = 1L;
		else
			*res = 0L;
		return apgen::RETURN_STATUS::SUCCESS;
	}
	intres = strcmp(**a->value.CST, **b->value.CST);

	switch (c->value.INTEGER) {
		case 1:     /*                   is s1 < s2 ?            */
		   if(intres < 0) *res = 1L;
		   else *res = 0L;
		   break;
		case 2:    /*                    is s1 == s2 ?           */
		   if(intres == 0) *res = 1L;
		   else *res = 0L;
		   break;
		case 3:   /*                      is s1 > s2 ?           */
		   if(intres > 0) *res = 1L;
		   else *res = 0L;
		   break;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_sin(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_sin")
	if(!a->is_numeric()) {
		rmessage = "non-numeric arg. in sin()";
		return apgen::RETURN_STATUS::FAIL;
	}
	*res = sin(a->get_double());
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_cos(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_cos")
	if(!a->is_numeric()) {
		rmessage = "non-float arg. in cos()";
		return apgen::RETURN_STATUS::FAIL;
	}
	*res = cos(a->get_double());
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_cputime(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	if(args.size() != 0) {
		rmessage = Cstring("too few args. in cputime function; ");
		rmessage << "expected 0, got " << 
			(args.size() != 0);
		return apgen::RETURN_STATUS::FAIL;
	}
	static std::chrono::high_resolution_clock::time_point t1
		= std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point t2
		= std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span
		= std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1);
	double cpu_duration = time_span.count();
	long int seconds = (long int)cpu_duration;
	long int milliseconds = (long int)((1000.0 * cpu_duration) - (1000 * seconds));
	CTime_base T(seconds, milliseconds, true);
	*res = T;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_tan(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
    RETRIEVE1(a, "exp_tan")
    if(!a->is_numeric()) {
	rmessage = "non-float arg. in tan()";
	return apgen::RETURN_STATUS::FAIL;
    }
    *res = tan(a->get_double());
    return apgen::RETURN_STATUS::SUCCESS;
}

#ifdef OBSOLETE
apgen::RETURN_STATUS exp_time_series(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
    RETRIEVE2(a, b, "exp_time_series")
    if(!a->is_array() || !b->is_array()) {
	rmessage = "both args should be arrays in time_series()";
	return apgen::RETURN_STATUS::FAIL;
    }

    // debug
    cerr << "exp_time_series: first arg = " << a->to_string() << "\n";

    ListOVal&	key_lov = a->get_array();
    ArrayElement* key_ae;
    aoString the_key;
    for(int w = 0; w < key_lov.get_length(); w++) {
	TypedValue& vv = key_lov[w]->Val();
	switch(vv.get_type()) {
	    case apgen::DATA_TYPE::STRING:
		the_key << vv.get_string();
		break;
	    case apgen::DATA_TYPE::INTEGER:
		the_key << Cstring(vv.get_int());
		break;
	    case apgen::DATA_TYPE::BOOL_TYPE:
		if(vv.get_int()) {
		    the_key << "T";
		} else {
		    the_key << "F";
		}
		break;
	    case apgen::DATA_TYPE::TIME:
	    case apgen::DATA_TYPE::DURATION:
		the_key << vv.get_time_or_duration().to_string();
		break;
	    default:
		{
		Cstring err;
		err << "exp_time_series: key " << vv.to_string()
		    << " is of a type not suitable for use as a key";
		throw(eval_error(err));
		}
	}
    }

    //
    // Insert a node with the key just computed into the list
    // that is the payload of the resource UnderConsolidation
    //
    Cstring combined_key = the_key.str();
    Cnode0<alpha_string, aafReader::state_series>*	coeff_node =
	new Cnode0<alpha_string, aafReader::state_series>(combined_key);
    tlist<alpha_string, Cnode0<alpha_string, aafReader::state_series> >& coeff
	= aafReader::precomputed_resource::UnderConsolidation->payload.interp_coeff;
    coeff << coeff_node;

    //
    // Get ready to stuff time nodes into the payload of the node just inserted
    //
    tlist<alpha_time, Cnode0<alpha_time, aafReader::state6> >& coeff_series = coeff_node->payload;

    ArrayElement* ae;
    ListOVal&	time_coeff_pairs = b->get_array();
    for(int k = 0; k < time_coeff_pairs.get_length(); k++) {
	ae = time_coeff_pairs[k];
	CTime_base	time_stamp(ae->get_key());
	ListOVal&	lov = ae->Val().get_array();
	Cnode0<alpha_time, aafReader::state6>* state_node
		= new Cnode0<alpha_time, aafReader::state6>(time_stamp);
	for(int l = 0; l < 6; l++) {
	    ArrayElement* ae2 = lov[l];
	    state_node->payload.s[l] = ae2->Val().get_double();
	}
	coeff_series << state_node;
    }

    //
    // debug
    //
    cerr << "    node(" << combined_key << ") had " << coeff_series.get_length() << " time nodes\n";

    return apgen::RETURN_STATUS::SUCCESS;
}
#endif /* OBSOLETE */

apgen::RETURN_STATUS exp_atan(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_atan")
	if(!a->is_numeric()) {
		rmessage = "non-numeric arg. in atan()";
		return apgen::RETURN_STATUS::FAIL; }
	*res = atan(a->get_double());
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_log(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_log")
	if(!a->is_numeric()) {
		rmessage = "non-numeric arg. in log()" ;
		return apgen::RETURN_STATUS::FAIL ; }
	if(a->get_double() < 0.) {
		rmessage = "negative arg. in log()." ;
		return apgen::RETURN_STATUS::FAIL ; }
	*res = log(a->get_double()) ;
	return apgen::RETURN_STATUS::SUCCESS ; }

apgen::RETURN_STATUS exp_log10(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_log10")
	if(!a->is_numeric()) {
		rmessage = "non-numeric arg. in log10()" ;
		return apgen::RETURN_STATUS::FAIL ; }
	if(a->get_double() < 0.) {
		rmessage = "negative arg. in log10()." ;
		return apgen::RETURN_STATUS::FAIL ; }
	*res = log10(a->get_double()) ;
	return apgen::RETURN_STATUS::SUCCESS ; }
apgen::RETURN_STATUS exp_exp(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_exp")
	if(!a->is_numeric()) {
		rmessage = "non-numeric arg. in exp()" ;
		return apgen::RETURN_STATUS::FAIL ; }
	if(a->get_double() > 200.) {
		rmessage = "arg. of exponential function exceeds 200." ;
		return apgen::RETURN_STATUS::FAIL ; }
	*res = exp(a->get_double()) ;
	return apgen::RETURN_STATUS::SUCCESS ; }

apgen::RETURN_STATUS exp_sqrt(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_sqrt")
	if(!a->is_numeric()) {
		rmessage = "non-numeric arg. in sqrt()" ;
		return apgen::RETURN_STATUS::FAIL ; }
	if(a->get_double() < 0.) {
		rmessage = "sqrt(negative number)" ;
		return apgen::RETURN_STATUS::FAIL ; }
	*res = sqrt(a->get_double()) ;
	return apgen::RETURN_STATUS::SUCCESS ; }

apgen::RETURN_STATUS exp_strtoi(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_strtoi")

	if(!(a->is_string()) || !(b->is_int())) {
		rmessage = "strtoi takes a string and an integer base";
		return apgen::RETURN_STATUS::FAIL; }

	*res = (long) strtol(*(a->get_string()), NULL, b->get_int());
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_atan2(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_atan2")
	if((!a->is_numeric()) || (! b->is_numeric())) {
		rmessage = "non-float arg. in atan2()";
		return apgen::RETURN_STATUS::FAIL; }
	*res = atan2(a->get_double(), b->get_double());
	return apgen::RETURN_STATUS::SUCCESS; }


apgen::RETURN_STATUS exp_ERROR(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_ERROR")

	if(!a->is_string()) {
		rmessage = "non-string arg in error";
		return apgen::RETURN_STATUS::FAIL; }

	rmessage = a->get_string();
	return apgen::RETURN_STATUS::FAIL;
}

Cstring getCharForNum(int val) {
	//out of our range.. give it an 'x'
	if((val < 0) || (val > 15))
		return "x";

	//"0" -- "9"
	if(val < 10) {
		char t[2];
		t[0] = '0' + val;
		t[1] = '\0';
		return Cstring(t); }

	//"a" -- "f"
	else {
		char t[2];
		t[0] = 'a' + val - 10;
		t[1] = '\0';
		return Cstring(t); } }

apgen::RETURN_STATUS exp_itostr(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_itostr");

	if(!a->is_int() || !b->is_int()) {
		rmessage = "itostr takes a number(int) and a base(int)";
		return apgen::RETURN_STATUS::FAIL; }

	long accum = a->get_int();
	long base = b->get_int();
	Cstring straccum;

	while(accum > 0) {
		long placeMem = accum % base;
		Cstring c = getCharForNum(placeMem);
		straccum = c + straccum;
		accum = accum / base; }

	*res = straccum;
	return apgen::RETURN_STATUS::SUCCESS; }

// The following functions used to be part of the "standard" user-defined library:

apgen::RETURN_STATUS	exp_GET_FILE_INFO(Cstring& errs, TypedValue* res, slst<TypedValue*>& args) {
	TypedValue*	n1; // string fname

	errs.undefine();
	if(args.size() != (1)) {
		errs << "  wrong number of parameters passed to function get_file_info; "
			"expected 1, got " << (long) (args.size() ) << "\n";
                return apgen::RETURN_STATUS::FAIL;
	}
	n1 = args.pop_front();
	if(!n1->is_string()) {
		errs = "non-array value passed as first arg to get_file_info";
		return apgen::RETURN_STATUS::FAIL;
	}
	const char*	fname = *n1->get_string();
	struct stat	fileStats;
	if(stat(fname, &fileStats) < 0) {
		*res = "nosuchfile"; }
	else if(S_ISREG(fileStats.st_mode)) {
		*res = "file";
	} else if(S_ISDIR(fileStats.st_mode)) {
		*res = "directory";
	} else {
		*res = "other";
	}
	return apgen::RETURN_STATUS::SUCCESS;
}


/*
 * The ingredients of the following function are
 *
 *    - parse_incon<parser class>(), a template defined in APGen
 *	header file act_plan_parser.H
 *
 *    - xml_incon_parser, a parser class defined in APGen
 *	header file xml_incon_parser.H
 */
void	parse_incon_xml(
				const char*	start_here,
				CTime_base&	finalTime) {
	apgen::RETURN_STATUS	ret;

	xml_incon_parser	parser(start_here);

	try {
		parser.parse();
	} catch(eval_error Err) {
		throw(Err);
	}

	parse_incon<xml_incon_parser>(parser);
	finalTime = parser.final_time();
}

apgen::RETURN_STATUS	exp_read_xmltol_as_incon(Cstring& errs, TypedValue* res, slst<TypedValue*>& args) {
	TypedValue*		n1; // file name
	apgen::RETURN_STATUS	ret;

	errs.undefine();
	if(args.size() != (1)) {
		errs << "  wrong number of parameters passed to function read_xmltol_as_incon; "
			"expected 1, got " << (args.size() ) << "\n";
                return apgen::RETURN_STATUS::FAIL;
	}
	n1 = args.pop_front();
        if(!n1->is_string() ) {
                errs = "non-string arg. 1 in read_xmltol_as_incon()";
                return apgen::RETURN_STATUS::FAIL;
	}
	FILE*	f = fopen(*n1->get_string(), "r");
	if(!f) {
		errs << " read_xmltol_as_incon cannot find file \"" << n1->get_string() << "\"";
		return apgen::RETURN_STATUS::FAIL;
	}
	buf_struct	bs = {NULL, 0, 0};
	char		c[2];
	c[1] = '\0';
	while(fread(c, 1, 1, f)) {
		concatenate(&bs, c);
	}
	CTime_base final_time;
	try {
		ret = apgen::RETURN_STATUS::SUCCESS;
		parse_incon_xml(bs.buf, final_time);
		*res = final_time;
	} catch(eval_error Err) {
		ret = apgen::RETURN_STATUS::FAIL;
		errs = Err.msg;
	}
	destroy_buf_struct(&bs);
	fclose(f);
	return ret;
}

apgen::RETURN_STATUS	exp_write_behavior(Cstring &errs, TypedValue* ret, slst<TypedValue*>& args) {
	TypedValue*	n1; // string fname

	errs.undefine();
	if(args.size() != (1)) {
		errs << "  wrong number of parameters passed to function write_behavior; "
			"expected 1, got "
			<< (long) (args.size()
					) << "\n";
                return apgen::RETURN_STATUS::FAIL;
	}
	n1 = args.pop_front();
	if(!n1->is_string()) {
		errs = "non-array value passed as first arg to write_behavior";
		return apgen::RETURN_STATUS::FAIL;
	}
	FILE*	f = fopen(*n1->get_string(), "w");
	if(!f) {
		errs << " write_behavior cannot open file \"" << n1->get_string() << "\"";
		return apgen::RETURN_STATUS::FAIL;
	}
	bool save_flag = APcloptions::theCmdLineOptions().debug_execute;
	APcloptions::theCmdLineOptions().debug_execute = true;
	aoString s;

	int save_debug = abdebug;
	abdebug = 1;
	for(int i = 0; i < Behavior::ClassTypes().size(); i++) {
		Behavior& be = *Behavior::ClassTypes()[i];
		be.to_stream(&s, 0);
	}
	abdebug = save_debug;

	Cstring t = s.str();
	int n = t.length();
	fwrite(*t, n, 1, f);
	fclose(f);
	*ret = 1L;
	APcloptions::theCmdLineOptions().debug_execute = save_flag;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS	exp_write_to_stdout(Cstring &errs, TypedValue *, slst<TypedValue*>& args) {
	TypedValue*	n1; // "string"
	Cstring		temp;
	char*		C;
	int		C_length = 20;
	const char*	t;
	char*		s;
	int		state = 0;
	List		fragments;
	String_node*	N;

	//
	// Prevent multiple thread to mangle each other's output:
	//
	static	mutex		mutx;
	lock_guard<mutex>	lock(mutx);

	errs.undefine();
	if(args.size() < (1)) {
		errs << "  wrong number of parameters passed to function "
			<< "write_to_stdout; expected at least 1, got "
			<< (long) (args.size() )
			<< "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	C = (char *) malloc(C_length + 1);
	while(args.size() > 0) {
		n1 = args.pop_front();
		temp = n1->to_string();
		if(n1->get_type() == apgen::DATA_TYPE::STRING) removeQuotes(temp);
		if(C_length < temp.length()) {
			C_length = 2 * temp.length();
			C = (char *) realloc(C, C_length + 1); }
		s = C;
		state = 0;
		for(t = *temp; *t; t++) {
			if(!state) {
				if(*t == '\\')
					state = 1;
				else
					*s++ = *t; }
			else {
				if(*t == 'n')
					*s++ = '\n';
				else if(*t == 't')
					*s++ = '\t';
				else if(*t == '\"')
					*s++ = '\"';
				else if(*t == '\\')
					*s++ = '\\';
				else
					*s++ = *t;
				state = 0; } }
		*s = '\0';
		fragments << new String_node(C);
	}
	for(	N = (String_node *) fragments.last_node();
		N;
		N = (String_node *) N->previous_node()) {
		cout << N->get_key();
	}
	cout.flush();
	free(C);

	return apgen::RETURN_STATUS::SUCCESS;
}


apgen::RETURN_STATUS	exp_start_debug(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
	errs.undefine();
	if(args.size() != 0) {
		errs << " incorrect # parameters passed to function start_debug; expected 0, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL; }
#	ifdef DBGEVENTS
	inhibitDBGEVENTS = false;
#	endif /* DBGEVENTS */
	*result = 1L;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS	exp_stop_debug(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
	errs.undefine();
	if(args.size() != 0) {
		errs << " incorrect # parameters passed to function stop_debug; expected 0, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
#	ifdef DBGEVENTS
	inhibitDBGEVENTS = true;
#	endif /* DBGEVENTS */
	*result = 1L;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS	exp_random(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
	errs.undefine();
	if(args.size() != 0) {
		errs << " incorrect # parameters passed to function random; expected 0, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	// according to the man pages, rand returns an int between 0 and RAND_MAX ('man rand') :
	*result = ((double) rand()) / (double) RAND_MAX;
	return apgen::RETURN_STATUS::SUCCESS;
}


apgen::RETURN_STATUS exp_strstr(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
	TypedValue	* n1,  //"string"
	              * n2;  //"string"
        char *temp = NULL;
	errs.undefine();
	if(args.size() != (2)) {
		errs << "  wrong number of parameters passed to function strstr_ap; expected 2, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	n2 = args.pop_front();
	n1 = args.pop_front();
	if(!n1->is_string()) {
	      errs << "strstr_ap: first argument is not an string\n";
	      return apgen::RETURN_STATUS::FAIL;
	}
	if(!n2->is_string()) {
	      errs << "strstr_ap: second argument is not an string\n";
	      return apgen::RETURN_STATUS::FAIL;
	}

	temp = (char *) strstr(*n1->get_string(), *n2->get_string());
	if(temp == NULL) {
		*result = "";
	} else{
		*result = temp;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_int2ascii(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
	  TypedValue	* n1;  //"integer"
          Cstring	temp;

	errs.undefine();
	if(args.size() != (1))
		{
		errs << "  wrong number of parameters passed to function int2ascii; expected 1, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL;
		}
	n1 = args.pop_front();
	if(n1->get_type() != apgen::DATA_TYPE::INTEGER) {
		errs << "int2ascii: first argument is not an integer\n";
		return apgen::RETURN_STATUS::FAIL; }
	temp << n1->get_int();
	*result = temp;
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_GetEnv(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
	  TypedValue	* n1;  //"string"

	errs.undefine();
	if(args.size() != (1)) {
		errs << "  wrong number of parameters passed to function GetEnv; expected 1, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL; }
	n1 = args.pop_front();
        if(n1->get_type() != apgen::DATA_TYPE::STRING) {
		errs << "GetEnv: first argument is not an string\n";
		return apgen::RETURN_STATUS::FAIL; }
	*result = getenv(*n1->get_string());
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS	exp_asin(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
	  TypedValue	* n1; // "x"


	errs.undefine() ;
	if(args.size() != (1))
		{
		errs << "  wrong number of parameters passed to function asin_ap; expected 1, got "
			<< (args.size() ) << "\n" ;
		return apgen::RETURN_STATUS::FAIL ;
		}
	n1 = args.pop_front() ;
	if(n1->get_type() != apgen::DATA_TYPE::FLOATING)
		{
		errs << "asin_ap: first argument is not a float\n" ;
		return apgen::RETURN_STATUS::FAIL ;
		}
	*result = asin(n1->get_double()) ;
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS	exp_acos(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
	  TypedValue	* n1; // "x"

	errs.undefine();
	if(args.size() != (1)) {
		errs << "  wrong number of parameters passed to function acos; expected 1, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL; }
	n1 = args.pop_front();
	if(n1->get_type() != apgen::DATA_TYPE::FLOATING) {
		errs << "acos_ap: first argument is not a float\n";
		return apgen::RETURN_STATUS::FAIL; }
	*result = acos(n1->get_double());
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS   exp_string_to_int(Cstring & errs, TypedValue *result, slst<TypedValue*>& args) {
        TypedValue      *n1; // "string"

        errs.undefine();
        if(args.size() != (1)) {
                errs << "  wrong number of parameters passed to function string_to_int; expected 1, got "
                        << (args.size() ) << "\n";
                return apgen::RETURN_STATUS::FAIL; }
        n1 = args.pop_front();
        if(n1->get_type() != apgen::DATA_TYPE::STRING) {
                errs << "string_to_int: argument is not a string\n";
                return apgen::RETURN_STATUS::FAIL; }
        *result = atol(*n1->get_string());
        return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS   exp_time_to_string(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
        TypedValue      *n1; // "time"

        errs.undefine();
        if(args.size() != (1)) {
                errs << "  wrong number of parameters passed to function time_to_string; expected 1, got "
                        << (args.size() ) << "\n";
                return apgen::RETURN_STATUS::FAIL; }
        n1 = args.pop_front();
        if((!n1->is_time()) && !(n1->is_duration())) {
                errs << "time_to_string: argument is not a time/duration\n";
                return apgen::RETURN_STATUS::FAIL; }
        *result = n1->get_time_or_duration().to_string();
        return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS   exp_string_to_time(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
        TypedValue      *n1; // "string"

        errs.undefine();
        if(args.size() != (1)) {
                errs << "  wrong number of parameters passed to function string_to_time; expected 1, got "
                        << (args.size() ) << "\n";
                return apgen::RETURN_STATUS::FAIL; }
        n1 = args.pop_front();
        if(n1->get_type() != apgen::DATA_TYPE::STRING) {
                errs << "string_to_duration: argument is not a string\n";
                return apgen::RETURN_STATUS::FAIL; }
        *result = CTime_base(n1->get_string());
        return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS   exp_string_to_float(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
        TypedValue      *n1; // "string"

        errs.undefine();
        if(args.size() != (1)) {
                errs << "  wrong number of parameters passed to function string_to_float; expected 1, got "
                        << (args.size() ) << "\n";
                return apgen::RETURN_STATUS::FAIL; }
        n1 = args.pop_front();
        if(n1->get_type() != apgen::DATA_TYPE::STRING) {
                errs << "string_to_duration: argument is not a string\n";
                return apgen::RETURN_STATUS::FAIL; }
        *result = atof(*n1->get_string());
        return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS   exp_Strlen(Cstring &errs, TypedValue *result, slst<TypedValue*>& args) {
        TypedValue      *n1; // "string"

        errs.undefine();
        if(args.size() != (1)) {
                errs << "  wrong number of parameters passed to function Strlen; expected 1, got "
                        << (args.size() ) << "\n";
                return apgen::RETURN_STATUS::FAIL; }
        n1 = args.pop_front();
        if(n1->get_type() != apgen::DATA_TYPE::STRING) {
                errs << "Strlen: argument is not a string\n";
                return apgen::RETURN_STATUS::FAIL; }
        *result = n1->get_string().length();
        return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_displayMessageToUser(Cstring &errs, TypedValue *results, slst<TypedValue*>& args) {
        TypedValue      *n1; // "title"
        TypedValue      *n2; // "text"

	errs.undefine();

        if(args.size() != (2)) {
                errs << "  wrong number of parameters passed to function displayMessageToUser; expected 2, got "
                        << (args.size() ) << "\n";
                return apgen::RETURN_STATUS::FAIL; }
        n2 = args.pop_front();
        if(!n2->is_string()) {
                errs << "displayMessageToUser: argument 2 is not a string\n";
		return apgen::RETURN_STATUS::FAIL; }
        n1 = args.pop_front();
        if(!n1->is_string()) {
                errs << "displayMessageToUser: argument 1 is not a string\n";
		return apgen::RETURN_STATUS::FAIL; }
	if(DisplayMessage_handler) {
		DisplayMessage_handler(string(*n1->get_string()), string(*n2->get_string()), true); }
	return apgen::RETURN_STATUS::SUCCESS; }


apgen::RETURN_STATUS exp_has_substring(Cstring & errs, TypedValue *result, slst<TypedValue*>& args) {
        TypedValue      *n1; // "string"
        TypedValue      *n2; // "pattern"

	errs.undefine();

        if(args.size() != (2)) {
                errs << "  wrong number of parameters passed to function has_substring; expected 2, got "
                        << (args.size() ) << "\n";
                return apgen::RETURN_STATUS::FAIL;
	}
        n2 = args.pop_front();
        if(!n2->is_string()) {
                errs << "has_substring: argument 2 is not a string\n";
		return apgen::RETURN_STATUS::FAIL;
	}
        n1 = args.pop_front();
        if(!n1->is_string()) {
                errs << "has_substring: argument 1 is not a string\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	*result = (n1->get_string() & *n2->get_string());
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS	exp_element_exists(Cstring & errs, TypedValue * result, slst<TypedValue*>& args) {
	TypedValue	*n1, // "array"
			*n2; // "index (string)"

	errs.undefine();
	if(args.size() != (2)) {
		errs << "  wrong number of parameters passed to function element_exists; expected 2, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	n2 = args.pop_front();
	n1 = args.pop_front();
	if(!n2->is_string()) {
		errs << "element_exists: second argument is not a string\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	if(n1->get_type() == apgen::DATA_TYPE::INSTANCE) {
		behaving_element	ds = n1->get_object();
		TypedValue*		tdv;

		if(!ds) {
			errs << "element_exists: instance does not currently exist";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(ds->defines_property(n2->get_string())) {
			*result = true;
		} else {
			*result =  false;
		}
	} else if(n1->get_type() != apgen::DATA_TYPE::ARRAY) {
		errs << "element_exists: first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	} else {
		*result = (n1->get_array()[n2->get_string()] != NULL);
	}
	return apgen::RETURN_STATUS::SUCCESS;
}
// End of functions that used to be part of the "standard" user-defined library

apgen::RETURN_STATUS	exp_global_exists(Cstring& errs, TypedValue* result, slst<TypedValue*>& args) {
	TypedValue*	n1; // "globalname"

	errs.undefine();
	if(args.size() != (1)) {
		errs << "  wrong number of parameters passed to function global_exists; expected 1, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	n1 = args.pop_front();
	if(n1->get_type() != apgen::DATA_TYPE::STRING) {
		errs << "global_exists: argument is not a string\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	try {
		globalData::get_symbol(n1->get_string());
		*result = true;
	} catch(eval_error Err) {
		*result = false;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS	exp_get_global(Cstring& errs, TypedValue* result, slst<TypedValue*>& args) {
	TypedValue*	n1; // "globalname"

	errs.undefine();
	if(args.size() != (1)) {
		errs << "  wrong number of parameters passed to function get_global; expected 1, got "
			<< (args.size() ) << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	n1 = args.pop_front();
	if(n1->get_type() != apgen::DATA_TYPE::STRING) {
		errs << "global_exists: argument is not a string\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	try {
		*result = globalData::get_symbol(n1->get_string());
	} catch(eval_error Err) {
		errs = "Global ";
		errs << n1->get_string() << " does not exist. ";
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_get_all_globals(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	ArrayElement*		ae;
	ListOVal*		L = new ListOVal;
	ListOVal		copy_of_globals;

	globalData::copy_symbols(copy_of_globals);
	L->set_array_type(TypedValue::arrayType::STRUCT_STYLE);
	for(int i = 0; i < copy_of_globals.get_length(); i++) {
		ArrayElement*	tds = copy_of_globals[i];
		TypedValue V = tds->Val();
		ae = new ArrayElement(tds->get_key(), V.get_type());
		ae->set_typed_val(V);
		L->add(ae);
	}
	*res = *L;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_SELECTION(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	slist<alpha_void, smart_actptr>			selection_list;
	slist<alpha_void, smart_actptr>::iterator	li(selection_list);
	ArrayElement*		ae;
	TypedValue		V;
	Cstring			temp_error;
	ListOVal*		L = new ListOVal;
	ActivityInstance*	req;
	smart_actptr*		ptr;

	eval_intfc::get_all_selections(selection_list);

	while((ptr = li())) {
		if(L->get_array_type() == TypedValue::arrayType::UNDEFINED) {
			L->set_array_type(TypedValue::arrayType::STRUCT_STYLE);
		}
		req = ptr->BP;
		V = *req;
		ae = new ArrayElement(req->get_unique_id(), apgen::DATA_TYPE::INSTANCE);
		ae->set_typed_val(V);
		L->add(ae);
	}
	*res = *L;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_SELECTION_IDS(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	slist<alpha_void, smart_actptr>			selection_list;
	slist<alpha_void, smart_actptr>::iterator	li(selection_list);
	ArrayElement*					ae;
	TypedValue					V;
	Cstring						temp_error;
	ListOVal*					L = new ListOVal;
	ActivityInstance*				req;
	smart_actptr*					ptr;
	long						i = 0;

	eval_intfc::get_all_selections(selection_list);

	while((ptr = li())) {
		if(L->get_array_type() == TypedValue::arrayType::UNDEFINED) {
			L->set_array_type(TypedValue::arrayType::LIST_STYLE);
		}
		req = ptr->BP;
		V = req->get_unique_id();
		ae = new ArrayElement(i);
		ae->set_typed_val(V);
		L->add(ae);
	}
	*res = *L;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_VERSION(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	if(args.size() != 0) {
		rmessage = "wrong number of args to exp_GET_VERSION.  Expected 0";
		return apgen::RETURN_STATUS::FAIL;
	}
	Cstring versionInfo = get_apgen_version_build_platform();
	*res = versionInfo;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_last_event(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE0("exp_last_event")
	if(EventLoop::CurrentEvent) {
		*res = EventLoop::CurrentEvent->getetime();
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_length_of(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_length_of")
	if(a->is_array()) {
		*res = a->get_array().get_length();
	} else if(a->is_string()) {
		RCsource		*rcont;
		Rsource			*resou;
		bool			ResourceIsArray;

		if((rcont = RCsource::resource_containers().find(a->get_string()))) {
			*res = rcont->get_resource_count();
		} else if((resou = eval_intfc::FindResource(a->get_string()))) {
			rcont = &resou->parent_container;
			*res = rcont->get_resource_count();
		} else {
			rmessage.undefine();
			rmessage << "undefined resource name \"" << a->get_string() << "\" in length_of";
			return apgen::RETURN_STATUS::FAIL;
		}
	} else {
		rmessage.undefine();
		rmessage << "length_of cannot deal with argument \""
	       		<< a->to_string() << "\" ";
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_make_constant(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {

	//
	// this function is never executed - it is handled by FunctionCall::execute()
	//
	RETRIEVE1(a, "exp_make_constant")
	if(a->is_array()) {
		;
	} else {
		rmessage.undefine();
		rmessage << "make_constant needs an array, but got a(n) "
	       		<< apgen::spell(a->get_type()) << " ";
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_notify_client(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	int		i;
	const TypedValue *tdv = NULL;
	TypedValue*		b = args.pop_front();
	TypedValue*		a = args.pop_front();
	behaving_element	A;

	if(!a) {
		rmessage = Cstring("too few args. in ") + "notify_client";
		return apgen::RETURN_STATUS::FAIL; }
	if(args.size() > 0) {
		rmessage = Cstring( "too many args. in " ) + "notify_client";
		return apgen::RETURN_STATUS::FAIL; }
	else if(args.size() < 0) {
		rmessage = Cstring( "too few args. in " ) + "notify_client";
		return apgen::RETURN_STATUS::FAIL; }

	if(!Dsource::eventCatcher) return apgen::RETURN_STATUS::SUCCESS;
	// expect activity instance, string (= parameter name)
	if(!a->is_instance()) {
		rmessage.undefine();
		rmessage << "first arg of notify_client must be an instance ";
		return apgen::RETURN_STATUS::FAIL; }
	A = a->get_object();
	if(!b->is_string()) {
		rmessage.undefine();
		rmessage << "second arg of notify_client must be a string referring to a parameter name ";
		return apgen::RETURN_STATUS::FAIL;
	}
	bool hasSymbol = true;
	if(A->defines_property(b->get_string())) {
		hasSymbol = true;
	} else {
		hasSymbol = false;
	}
	if(!hasSymbol) {
		rmessage.undefine();
		rmessage << "second arg of notify_client is " << b->get_string()
			<< " which does not refer to a param/attribute of object "
			<< (*A)["id"].get_string();
		return apgen::RETURN_STATUS::FAIL;
	}
	// we checked earlier that the method exists
	Dsource::eventCatcher(*(*A)["id"].get_string(), *b->get_string());
	return apgen::RETURN_STATUS::SUCCESS;
}


//
// If the first argument is an array, the function expects a second argument that evaluates to a string. It
// assumes that the string is a valid string index for the array (returns error otherwise) and returns
// the numeric index of the corresponding array element.
//
// If the second argument is a string, the function assumes that EITHER the string is a resource name, in which
// case it expects only 1 argument, or the string is a resource container name, in which case it expects a
// first string argument, which it will interpret as a valid index string for that resource container. In either
// case, the function will return the numeric index of the resource in its container, and will return -1 if the
// resource is not part of an array. It will return an error if the resource does not exist.
//
apgen::RETURN_STATUS exp_numeric_index_of(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {

	//
	// We don't know how many arguments we got, so let's not use the macros.
	//
	int	how_many_args =  args.size() ;

	if(how_many_args <= 0) {
		rmessage = Cstring("too few args. in numeric_index_of");
		return apgen::RETURN_STATUS::FAIL;
	} else if(how_many_args > 2) {
		rmessage = Cstring("too many args. in numeric_index_of; can have 2 at most.");
		return apgen::RETURN_STATUS::FAIL;
	}
	if(how_many_args == 2) {
		TypedValue* second_arg = args.pop_front();
		TypedValue* first_arg = args.pop_front();
		if(second_arg->is_array()) {

		    //
		    // First case: the second arg is an array. The first arg should be a string index.
		    //
		    if(!second_arg->is_struct()) {
			rmessage << "second arg. is a list, not a struct. "
					<< "numeric_index_of() only works with struct-style arrays.";
			return apgen::RETURN_STATUS::FAIL;
		    } else {
			ArrayElement* bn;

			if(!first_arg->is_string()) {
				rmessage.undefine();
				rmessage << "non-string index name \""
					<< second_arg->to_string()
					<< "\" in numeric_index_of";
				return apgen::RETURN_STATUS::FAIL;
			}
			bn = second_arg->get_array().find(first_arg->get_string());
			if(!bn) {
				*res = -1L;
			} else {
				*res = second_arg->get_array().get_index(bn);
			}
			return apgen::RETURN_STATUS::SUCCESS;
		    }
		} else if(second_arg->is_string()) {

			//
			// Second case: the second arg is a string. This is always wrong.
			//
			if(first_arg->is_string()) {
				rmessage = "numeric_index_of() no longer supports use with two string arguments.";
				return apgen::RETURN_STATUS::FAIL;
			} else if(first_arg->is_array()) {
				rmessage = "numeric_index_of() expects a string first, then an array. "
					"Sounds like you got them in reverse order.";
				return apgen::RETURN_STATUS::FAIL;
			} else {
				rmessage = "numeric_index_of() expects a string first, ";
				rmessage << "not a(n) " << spell(first_arg->get_type());
				return apgen::RETURN_STATUS::FAIL;
			}
		} else {
			rmessage = Cstring("First arg. in numeric_index_of is not an array.");
			return apgen::RETURN_STATUS::FAIL;
		}
	} else {

		//
		// how many arguments = 1
		//
		TypedValue*		tv = args.pop_front();

		if(tv->is_instance()) {
			if(!tv->get_object().object()) {
				rmessage.undefine();
				rmessage << "numeric_index_of: argument is not an object";
				return apgen::RETURN_STATUS::FAIL;
			}
			behaving_element obj = tv->get_object();
			Rsource*	rs;

			// debug
			// cout << "numeric_index_of: argument = " << obj->serial() << "\n";

			if((rs = obj->get_concrete_res())) {
				map<Cstring, int>::iterator iter = rs->Type.parent->SubclassFlatMap.find(rs->name);
				*res = (long) iter->second;
				return apgen::RETURN_STATUS::SUCCESS;
			} else {
				rmessage = Cstring("Single arg. in numeric_index_of is not a concrete resource.");
				return apgen::RETURN_STATUS::FAIL;
			}
		} else {
			rmessage = Cstring("Single arg. in numeric_index_of is not an object.");
			return apgen::RETURN_STATUS::FAIL;
		}
	}
	rmessage = "numeric_index_of: strange case";
	return apgen::RETURN_STATUS::FAIL;
}

// placeholder for Steve's printf-like function
apgen::RETURN_STATUS exp_printf(Cstring &message, TypedValue *res, slst<TypedValue*>& args) {
	// args[0] is the "format string"
	Cstring		format_string;
	TypedValue*	format_value = args.pop_front();
	char		*C, *D;
	char		*t;
	char		*s;
	int		state = 0;
	List		fragments;
	String_node	*N;
	int		argno = args.size() ;
	int		i = argno;

	message.undefine();
	if(argno < 1) {
		message << "  wrong number of parameters passed to printf; expected at least 1, got "
			<< argno << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	TypedValue** arguments = (TypedValue **) malloc(sizeof(TypedValue *) * argno);
	while(args.size() > 0) {
		arguments[--i] = args.pop_front();
	}
	if(!format_value->is_string()) {
		message << "  printf: first argument is not a string\n";
		free((char *) arguments);
		return apgen::RETURN_STATUS::FAIL;
	}
	// Should not be enclosed in quotes:
	format_string = format_value->get_string();
	if(argno == 1) {
		cout << format_string;
		cout.flush();
		free((char *) arguments);
		return apgen::RETURN_STATUS::SUCCESS; }
	C = (char *) malloc(format_string.length() + 1);
	D = (char *) malloc(format_string.length() + 1);
	strcpy(C, *format_string);
	s = C;
	t = D;
	while(*s) {
		if(*s == '%') {
			if(state == 0) {
				state = 1; }
			else if(state == 1) {
				state = 0;
				*t++ = *s; }
			else if(state == 2) {
				// should invoke printf with what we've got so far
				} }
		else if(state == 1) {
			// format specification
			state = 2;
			// make sure we have an argument to go with this
			}
		else {
			*t++ = *s; }
		s++; }
	*t = '\0';
	free(D);
	free(C);
	free((char *) arguments);
	return apgen::RETURN_STATUS::SUCCESS; }

apgen::RETURN_STATUS exp_copy_array(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(b, "exp_copy_array")
	if(!b->is_array()) {
		rmessage = "argument of copy_array is not an array.";
		return apgen::RETURN_STATUS::FAIL;
	}
	// probably unnecessary
	res->undefine();
	res->recursively_copy(b->get_array());
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_interrupt(
		Cstring&	rmessage,
		TypedValue*	res,
		slst<TypedValue*>& args) {
	RETRIEVE1(options, "exp_interrupt")

	//
	// For now
	//
	rmessage = "interrupt() is not implemented yet";
	return apgen::RETURN_STATUS::FAIL;

	if(!options->is_array()) {
		rmessage = "argument of interrupt() is not an array.";
		return apgen::RETURN_STATUS::FAIL;
	}
	Cstring Enabled("enabled");
	ArrayElement*	ae = NULL;
	try {
		ae = options->get_array()[Enabled];
		if(!ae) {
			throw(eval_error(Cstring("element 'enabled' does not exist")));
		}
		// ae exists, or we just threw
		if(ae->Val().get_int()) {

			//
			// not yet
			//
			// modeling_interruptor::interrupt();
			ae->Val() = false;
		}
	}
	catch(eval_error Err) {
		rmessage << "interrupt(): " << Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

#ifdef NEEDS_TO_BE_REWORKED
apgen::RETURN_STATUS exp_get_threads(
		Cstring&	rmessage,
		TypedValue*	res,
		slst<TypedValue*>& args) {
	RETRIEVE1(options, "exp_get_threads")
	if(!options->is_array()) {
		rmessage = "argument of get_threads() is not an array.";
		return apgen::RETURN_STATUS::FAIL;
	}
	ArrayElement*	ae = NULL;
	*res = *options;
	try {
		Cstring current_pass = globalData::modeling_pass().get_string();
		if(current_pass == "INACTIVE") {
			/* not an error; we may want to have get_threads()
			 * inside a decomposition section */
			return apgen::RETURN_STATUS::SUCCESS;
		}

		/* Next, we insert an array of 'active threads' into the options array.
		 * First, determine whether the options array has a "time scope" element that
		 * tells us the range of events we need to scan... */
		ArrayElement*	time_scope_el = options->get_array().find("time scope");
		ArrayElement*	scheduled_threads = options->get_array().find("threads");
		ArrayElement*	objects = options->get_array().find("objects");
		CTime_base	time_scope(0,0,true);

		if(time_scope_el) {
			if(!time_scope_el->Val().is_duration()) {
				throw(eval_error("get_threads(): time scope element is not a duration"));
			}
			time_scope = time_scope_el->Val().get_time_or_duration();
		} else {
			// can't provide threads without a time scope
			throw(eval_error("get_threads(): time scope needed in order to provide threads"));
		}

		if(scheduled_threads) {
			if(!scheduled_threads->Val().is_array()) {
				throw(eval_error("\"threads\" element of get_threads() arg. is not an array"));
			}
			scheduled_threads->Val().get_array().clear();
		} else {
			throw(eval_error("arg. of get_threads() should have a \"threads\" element"));
		}

		if(objects) {
			if(!objects->Val().is_array()) {
				throw(eval_error("\"objects\" element of get_threads() arg. is not an array"));
			}
			objects->Val().get_array().clear();
		} else {
			throw(eval_error("arg. of get_threads() should have an \"objects\" element"));
		}

		/* Now let's fill the thread array. We would like to copy the
		 * Events() multiterator, but first we'd better check that
		 * we are in a modeling or scheduling loop */

		Miterator<slist<alpha_time, mEvent, eval_intfc*>, mEvent>
				event_copy("get_threads()", eval_intfc::Events());

		/* Now we can traverse events, starting at the current event. */
		mEvent*	event = event_copy.peek();
		// assert(event);
		CTime_base	threads_start = time_saver::get_now();
		CTime_base	threads_end;
		bool		threads_end_defined = false;

		/* First we scan events, then we scan 
		 * eval_intfc::actsOrResWaitingOnSignals() and
		 * eval_intfc::actsOrResWaitingOnCond() */

		while((event = event_copy.next())) {
			ArrayElement*	new_ae;
			ArrayElement*	array_item;
			ArrayElement*	type_item;
			ListOVal*	one_el;

			Cstring		obj_key;

			if(!threads_end_defined) {
				threads_end = threads_start + time_scope;
				threads_end_defined = true;
				Cnode0<alpha_int, instruction_node*>*	aptr;
				slist<alpha_int, Cnode0<alpha_int, instruction_node*> >::iterator
							instr_node_iter(eval_intfc::actsOrResWaitingOnSignals());
				// we insert one array for each thread waiting on a signal:
				while((aptr = instr_node_iter())) {
					one_el = new ListOVal;
					type_item = new ArrayElement(Cstring("event_type"));
					type_item->Val() = "waiting on signal";
					one_el->add(type_item);

					behaving_element	el(aptr->payload->Calls.get_context()->AmbientObject);
					array_item = new ArrayElement(Cstring("thread"));
					array_item->Val() = el->to_struct();
					one_el->add(array_item);

					new_ae = new ArrayElement(event->getetime().to_string());
					new_ae->Val() = *one_el;
					scheduled_threads->Val().get_array().add(new_ae);

					new_ae = new ArrayElement((*el)["id"].get_string());
					new_ae->Val() = el;
					objects->Val().get_array().add(new_ae);
				}

				slist<alpha_int, Cnode0<alpha_int, instruction_node*> >::iterator
					instr_node_iter2(eval_intfc::actsOrResWaitingOnCond());
				// we insert one array for each thread waiting on a condition:
				while((aptr = instr_node_iter2())) {
					one_el = new ListOVal;
					type_item = new ArrayElement(Cstring("event_type"));
					type_item->Val() = "waiting on condition";
					one_el->add(type_item);

					behaving_element	el(aptr->payload->Calls.get_context()->AmbientObject);
					array_item = new ArrayElement(Cstring("thread"));
					array_item->Val() = el->to_struct();
					one_el->add(array_item);

					new_ae = new ArrayElement(event->getetime().to_string());
					new_ae->Val() = *one_el;
					scheduled_threads->Val().get_array().add(new_ae);

					new_ae = new ArrayElement((*el)["id"].get_string());
					new_ae->Val() = el;
					objects->Val().get_array().add(new_ae);
				}
			}
			if(event->getetime() > threads_end) {
				break;
			}

			// we insert one array for each event in the queue:
			one_el = new ListOVal;

			usage_event*	ue = dynamic_cast<usage_event*>(event->get_payload());
			wait_event*	te = dynamic_cast<wait_event*>(event->get_payload());
			resume_event*	re = dynamic_cast<resume_event*>(event->get_payload());

			if(re) {
				// should not happen
				cerr << "get_threads: strange - we are being interrupted while resuming\n";
				continue;
			} else if(ue) {
				type_item = new ArrayElement(Cstring("event_type"));
				type_item->Val() = "usage event";
				one_el->add(type_item);
				// array_item = new ArrayElement(Cstring("cause"));
				// array_item->Val() = ue->Cause->to_struct();
				// one_el->add(array_item);
				array_item = new ArrayElement(Cstring("thread"));
				array_item->Val() = ue->Effect->to_struct();
				one_el->add(array_item);
				obj_key = (*ue->Effect)["id"].get_string();
				if(!objects->Val().get_array().find(obj_key)) {
					array_item = new ArrayElement(obj_key);
					array_item->Val() = ue->Effect;
					objects->Val().get_array().add(array_item);
				}
			} else if(te) {
				type_item = new ArrayElement(Cstring("event_type"));
				type_item->Val() = "thread event";
				one_el->add(type_item);
				execution_context* context = te->getStack().get_context();
				array_item = new ArrayElement(Cstring("thread"));
				array_item->Val() = context->AmbientObject->to_struct();
				one_el->add(array_item);
				obj_key = (*context->AmbientObject)["id"].get_string();
				if(!objects->Val().get_array().find(obj_key)) {
					array_item = new ArrayElement(obj_key);
					array_item->Val() = context->AmbientObject;
					objects->Val().get_array().add(array_item);
				}
			}

			new_ae = new ArrayElement(event->getetime().to_string());
			new_ae->Val() = *one_el;
			scheduled_threads->Val().get_array().add(new_ae);
		}
	}
	catch(eval_error Err) {
		rmessage << "get_threads(): " << Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}
#endif /* NEEDS_TO_BE_REWORKED */

apgen::RETURN_STATUS exp_instance_with_id(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	// We don't know how many arguments we got, so let's not use the macros.
	int					how_many_args =  args.size();
	Cnode0<alpha_string, ActivityInstance*>*	theTag;

	if(how_many_args <= 0) {
		rmessage = Cstring("too few args. in instance_with_id");
		return apgen::RETURN_STATUS::FAIL;
	} else if(how_many_args > 1) {
		rmessage = Cstring("too many args. in instance_with_id; must have exactly one.");
		return apgen::RETURN_STATUS::FAIL;
	}

	TypedValue		*id = args.pop_front();

	if(!id->is_string()) {
		rmessage.undefine();
		rmessage << "non-string index \""
			<< id->to_string()
			<< "\" in instance_with_id";
		return apgen::RETURN_STATUS::FAIL;
	}
	theTag = aaf_intfc::actIDs().find(id->get_string());
	if(!theTag) {
		rmessage = "non-existent instance ";
		rmessage << id->get_string();
		return apgen::RETURN_STATUS::FAIL;
	}
	ActivityInstance* act = theTag->payload;
	*res = *act;
	return apgen::RETURN_STATUS::SUCCESS;
}

//
// If the second argument is an array, the function expects a first argument that evaluates to an integer. It
// assumes that the integer is a valid index for the array (returns error otherwise) and returns
// the string index of the corresponding array element.
//
// If the second argument is a string, the function assumes that the string is a resource container name,
// and it expects a first argument of type integer, which it will interpret as a valid index for that
// resource container.
//
// The function will return the string index of the resource in its container, and will return "" if the
// resource is not part of an array. It will return an error if the resource does not exist.
//
apgen::RETURN_STATUS exp_string_index_of(Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args) {
	// We don't know how many arguments we got, so let's not use the macros.
	int			how_many_args =  args.size() ;

	if(how_many_args <= 0) {
		rmessage = Cstring("too few args. in string_index_of");
		return apgen::RETURN_STATUS::FAIL;
	} else if(how_many_args > 2) {
		rmessage = Cstring("too many args. in string_index_of; can have 2 at most.");
		return apgen::RETURN_STATUS::FAIL;
	}

	if(how_many_args == 2) {
		// Watch out: array/resource container name comes SECOND, so should be popped FIRST!
		TypedValue*	tv = args.pop_front();
		TypedValue*	tv2 = args.pop_front();

		// First case: the second arg is an array. Expect a first arg of type integer.
		if(tv->is_array()) {
			ArrayElement* bn;

			if(!tv2->is_numeric()) {
				rmessage.undefine();
				rmessage << "non-integer index \""
					<< tv2->to_string() << "\" in string_index_of";
				return apgen::RETURN_STATUS::FAIL;
			}
			bn = tv->get_array()[tv2->get_int()];
			if(!bn) {
				rmessage = "array element ";
				rmessage << tv2->to_string() << " not found.";
				return apgen::RETURN_STATUS::FAIL;
			}
			*res = bn->get_key();
			return apgen::RETURN_STATUS::SUCCESS;
		}
		/*
		 * Second case: the second arg is a string. Expect 2 args and the first
		 * arg must be of type integer.
		 */
		else if(tv->is_string()) {
			RCsource*		cont;
			Rsource*		resou;

			//
			// Look for a resource
			//
			resou = eval_intfc::FindResource(tv->get_string());
			if(!resou) {
				// maybe we were given a container name
				bool				is_an_array;
				BPnode<alpha_string, Rsource*>*	theTagPointingToTheSimpleResource;
				Cstring				stripped;

				/*
				 * Let's look for a container instead
				 */
				cont = RCsource::resource_containers().find(tv->get_string());
				// First let's make sure the container exists...
				if(!cont) {
					rmessage.undefine();
					rmessage << "undefined resource name \"" << tv->get_string()
						<< "\" in string_index_of";
					return apgen::RETURN_STATUS::FAIL;
				}
				// Then let's check that the second argument is a number...
				if(!tv2->is_numeric()) {
					rmessage = Cstring("second arg. in string_index_of is not a number.");
					return apgen::RETURN_STATUS::FAIL;
				}
				if(!cont->is_array()) {
					*res = Cstring();
					return apgen::RETURN_STATUS::SUCCESS;
				}

				if(tv2->get_int() > cont->payload->Object->array_elements.size()) {
					rmessage = Cstring("string_index_of: index ");
					rmessage << tv2->get_int() << " is greater than the number of resources "
						<< cont->payload->Object->array_elements.size()
						<< " in " << cont->get_key();
					return apgen::RETURN_STATUS::FAIL;
				}
				Rsource* rs = cont->payload->Object->array_elements[tv2->get_int()];
				stripped = ("[" / rs->name) / "]";
				removeQuotes(stripped);
				*res = stripped;
				return apgen::RETURN_STATUS::SUCCESS;
			} else  {
				// simple resource; no index to speak of. Not an error, though;
				// we just return -1.
				*res = Cstring();
				return apgen::RETURN_STATUS::SUCCESS;
			}
		} else {
			rmessage.undefine();
			rmessage << "Second argument \"" << tv->to_string()
				<< "\" in string_index_of is neither a string nor an array. "
				"The second argument should be either a resource name or an array of type struct, i. e., containing keyword-value pairs. The first argument should always be an integer N. The function will return the string index value for the N-th element of the arrayed resource or the array, respectively.";
			return apgen::RETURN_STATUS::FAIL;
		}
	} else {
		// how many arguments = 1
		TypedValue*		tv = args.pop_front();

		if(tv->is_instance()) {
			rmessage.undefine();
			rmessage << "string_index_of(<resource name>) is obsolete.\nIf you are using it inside "
				<< "a resource 'usage' statement, note that now you can invoke "
				<< "currentval without prefix, like this:\n"
				<< "  usage\n"
				<< "    currentval() - x;\n\n"
				<< "If you are using it elsewhere, note that you can use built-in variable id if you want the full name\n"
				<< "of the resource, and you can use built-in variable indices if you want the individual indices of\n"
				<< "the resource. Note that indices is an array; array[i] contains the i-th level index of an N-dimensional\n"
				<< "resource array (0 <= i < N).\n";
				
			return apgen::RETURN_STATUS::FAIL;
		}
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_is_frozen(Cstring&
			rmessage,
			TypedValue*	res,
			slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_is_frozen");
	if(a->is_string()) {
		Rsource*		resou;

		// Look for a resource
		resou = eval_intfc::FindResource(a->get_string());
		if(!resou) {
			rmessage = "exp_is_frozen: resource ";
			rmessage << a->get_string() << " not found.";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(resou->isFrozen()) {
			*res = true;
		} else {
			*res = false;
		}
	} else {
		rmessage = "exp_is_frozen: expected one string argument";
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_DELETE_ELEMENT(
			Cstring&	rmessage,
			TypedValue*	res,
			slst<TypedValue*>& args) {
	ArrayElement*	N;
	RETRIEVE2(a , b , "exp_DELETE_ELEMENT")

	if(!a->is_array()) {
		rmessage << "delete_element error: non-array type. ";
		return apgen::RETURN_STATUS::FAIL;
	}
	if(b->is_numeric()) {
		N = a->get_array()[b->get_int()];
		if(!N) {
			rmessage << "delete_element error: index = " << b->get_int() <<
				", list size " << a->get_array().get_length() << ". ";
			return apgen::RETURN_STATUS::FAIL;
		}
		a->get_array().delete_el(b->get_int());
	} else if(b->is_string()) {
		N = a->get_array().find(b->get_string());
		if(!N) {
			rmessage << "delete_element error: index \""
				<< b->get_string()
				<< "\" not found #";
			return apgen::RETURN_STATUS::FAIL; }
		a->get_array().delete_el(b->get_string());
	} else {
		rmessage = "delete_element error: unintelligible index ";
		rmessage << b->to_string();
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_DELETE_SIGNAL(	Cstring& rmessage,
					TypedValue* res,
					slst<TypedValue*>& args) {
	RETRIEVE1(a , "exp_DELETE_SIGNAL")
	if(a->is_string()) {
		sigInfoNode*	N = eval_intfc::find_a_signal(a->get_string());
		// Node*		M = eval_intfc::recent_cxx_signals().find(a->get_string());

		if(!N) {
			rmessage << "trying to delete non-existent signal";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(N) delete N;
	} else {
		rmessage << "trying to delete signal of non-string type " << spell(a->get_type());
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_ID_OF(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
    RETRIEVE1(a , "exp_GET_ID_OF")
    if(a->is_instance()) {
	behaving_element	req = a->get_object();

	if(!req) {
		rmessage << "trying to get id of non-existent instance";
		return apgen::RETURN_STATUS::FAIL;
	}

	//
	// This is only right if the symbol is defined
	// at the same level as the instance variable.
	// For a general method, emulate
	// aafReader::find_symbol_in_task().
	//
	// *res = (*req)["id"].get_string();

	//
	// First, figure out the level of this object.
	//
	int object_level = -1;
	if(req.object() == req->level[0]) {
	    object_level = 0;
	} else if(req.object() == req->level[1]) {
	    object_level = 1;
	} else if(req.object() == req->level[2]) {
	    object_level = 2;
	}
	for(int k = object_level; k >= 0; k--) {
	    const task* curtask = &req->level[k]->Task;
	    map<Cstring, int>::const_iterator index_of_var_in_table;
	    if(((index_of_var_in_table = curtask->get_varindex().find("id"))
		!= curtask->get_varindex().end())) {

		//
		// level to use is k, index of ID is index_of_var_in_table->second
		//
		*res = req->level[k]->operator[](index_of_var_in_table->second);
		return apgen::RETURN_STATUS::SUCCESS;
	    }
	}
	rmessage << "Cannot find variable 'id' in this instance variable.";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	rmessage << "trying to get id of non-instance type " << spell(a->get_type());
	return apgen::RETURN_STATUS::FAIL;
    }
    return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_SENDER_INFO(Cstring& rmessage , TypedValue* res , slst<TypedValue*>& args) {
	RETRIEVE1(a , "exp_GET_SENDER_INFO")
	if(a->is_string()) {
		sigInfoNode*		theTag = eval_intfc::find_a_signal(a->get_string());
		behaving_element	be;
		ListOVal*		lov = new ListOVal;
		Cstring			actid;
		ArrayElement*		tds;

		if(!theTag) {
			rmessage << "trying to get senders of non-existent signal";
			return apgen::RETURN_STATUS::FAIL;
		}
		be = theTag->payload.obj;
		if(be) {

			// debug
			aoString aos;
			be->to_stream(&aos, 0);
			// cerr << "get_sender_info: object = " << aos.str() << "\n";
			// cerr << "    type = " << be->Task.full_name() << "\n";

			TypedValue	V;
			ListOVal*	a = new ListOVal;
			ArrayElement*	b;

			b = new ArrayElement("type", apgen::DATA_TYPE::STRING);
			// V = (*be)["type"];
			V = be->Task.Type.name;
			b->set_typed_val(V);
			a->add(b);

			//
			// Search for an ID - unfortunately, act instances and
			// abstract resources behave differently in this
			// regard
			//
			b = new ArrayElement("id", apgen::DATA_TYPE::STRING);
			if(be->defines_property("id")) {
				b->set_typed_val((*be)["id"]);
			} else if(be->level[1]->defines_property("id")) {
				b->set_typed_val((*be->level[1])["id"]);
			} else {
				TypedValue Unknown;
				Unknown = "unknown";
				b->set_typed_val(Unknown);
			}

			// V = (*be)["id"];
			// b->set_typed_val(V);

			a->add(b);

			V = *a;
			tds = new ArrayElement("sender", apgen::DATA_TYPE::ARRAY);
			tds->set_typed_val(V);
		}
		lov->add(tds);
		tds = new ArrayElement(Cstring("info"));
		tds->set_typed_val(theTag->payload.info);
		lov->add(tds);
		*res = *lov;
	} else {
		rmessage << "trying to get sender info of signal of non-string type " << spell(a->get_type());
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_ALL_SIGNALS(Cstring& rmessage , TypedValue* res , slst<TypedValue*>& args) {
	RETRIEVE0("exp_GET_ALL_SIGNALS")

	stringtlist		A;
	eval_intfc::get_all_signals(A);
	stringslist::iterator	iter(A);
	emptySymbol*		e;
	ListOVal*		lov = new ListOVal;
	int			count = 0;
	while((e = iter())) {
		ArrayElement*	ae = new ArrayElement(count++);
		TypedValue	v;
		v = e->get_key();
		ae->set_typed_val(v);
		lov->add(ae);
	}
	*res = *lov;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_MATCHING_SIGNALS(Cstring& rmessage , TypedValue* res , slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_GET_MATCHING_SIGNALS")

	if(a->is_string()) {
		sigPatternNode*	p = eval_intfc::find_a_pattern(a->get_string());

		if(!p) {
			rmessage << "get_matching_signals: pattern " << a->get_string()
				<< " was not used in a WAIT UNTIL REGEXP statement. \n";
			return apgen::RETURN_STATUS::FAIL;
		}
		stringtlist		sigs;
		stringslist::iterator	iter(sigs);
		emptySymbol*		s;
		ListOVal*		lov = new ListOVal;
		ArrayElement*		ae;
		long			count = 0;
		TypedValue		V;

		eval_intfc::get_all_signals(sigs);
		while((s = iter())) {
			if(p->payload.matches(s->get_key())) {
				ae = new ArrayElement(count++);
				V = s->get_key();
				ae->set_typed_val(V);
				lov->add(ae);
			}
		}
		*res = *lov;
	} else {
		Cstring tmp("get_matching_signals: arg ");
		tmp << a->to_string() << " is not a string.\n";
		rmessage = tmp;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_FIND_MATCHES(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	RETRIEVE2(a , b , "exp_FIND_MATCHES")
	if(a->is_array() && b->is_string()) {
		regex_t		patt;
		int		reg_err = regcomp(&patt, *b->get_string(), REG_EXTENDED);

		if(reg_err) {
			char		errbuf[1000];
			regerror(reg_err, &patt, errbuf, 1000);
			rmessage << "find_matches: error compiling pattern " << b->get_string() << "; details: " << errbuf << "\n";
			return apgen::RETURN_STATUS::FAIL;
		}
		ArrayElement*					ae;
		ListOVal*					lov = new ListOVal;
		long						count = 0;
		for(int k = 0; k < a->get_array().get_length(); k++) {
			ae = a->get_array()[k];
			regmatch_t	m[1];
			TypedValue	el_value = ae->payload;
			if(!el_value.is_string()) {
				rmessage << "element " << ae->get_key() << " of supplied array is not a string.\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			if(!regexec(&patt, *el_value.get_string(), 1, m, 0)) {
				ArrayElement*	ae2 = new ArrayElement(count++);
				TypedValue	v;
				v = el_value;
				ae2->set_typed_val(v);
				lov->add(ae2);
			}
		}
		regfree(&patt);
		*res = *lov;
	} else {
		Cstring tmp("find_matches(");
		tmp << a->to_string() << ", " << b->to_string()
			<< "): first arg should be an array of strings, second arg a pattern.\n";
		rmessage = tmp;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_SENDER(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	RETRIEVE1(a , "exp_GET_SENDER")
	if(a->is_string()) {
		sigInfoNode*		theTag = eval_intfc::find_a_signal(a->get_string());
		behaving_element	be;

		if(!theTag) {
			rmessage << "trying to get sender of non-existent signal";
			return apgen::RETURN_STATUS::FAIL;
		}
		be = theTag->payload.obj;
		if(!be) {
			rmessage << "trying to get sender of signal with no parent.";
			return apgen::RETURN_STATUS::FAIL;
		}
		*res = be;
	}
	else {
		rmessage << "trying to get sender of signal of non-string type " << spell(a->get_type());
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_CHILDREN_OF(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	RETRIEVE1(a , "exp_GET_CHILDREN_OF")
	if(a->is_instance()) {
		behaving_element	be = a->get_object();

		if(!be) {
			rmessage << "trying to get children of non-existent activity";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(!be->get_req()) {
			rmessage << "trying to get children of non-existent activity";
			return apgen::RETURN_STATUS::FAIL;
		}
		ActivityInstance*	req = be->get_req();
		slist<alpha_void, smart_actptr>::iterator theKids(req->get_down_iterator());
		smart_actptr*		ptr;
		ActivityInstance*	child;
		ListOVal*		L = new ListOVal;
		ArrayElement*		tds;
		TypedValue		V;

		if(req->children_count()) {
			// We set the type to 2, meaning that the array elements are string-indexed:
			L->set_array_type(TypedValue::arrayType::STRUCT_STYLE);
		}
		while((ptr = theKids())) {
			child = ptr->BP;
			tds = new ArrayElement(child->get_unique_id(), apgen::DATA_TYPE::INSTANCE);
			V = *child;
			tds->set_typed_val(V);
			L->add(tds);
		}
		*res = *L;
	} else {
		rmessage << "trying to get children of non-instance type " << spell(a->get_type());
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_PLAN(
		Cstring&	rmessage,
		TypedValue*	res,
		slst<TypedValue*>& args) {
	RETRIEVE1(a , "exp_GET_PLAN")
	int		prio = 0;
	unique_ptr<status_aware_multiterator>	the_instances(
						    eval_intfc::get_instance_multiterator(
							prio,
							/* incl_scheduled */   true,
							/* incl_unscheduled */ false));
	bool			Full;
	ListOVal*		L;
	bool			first = true;
	ActivityInstance*	req;

	if(!a->is_string()) {
		rmessage << "get_plan requires a string argument";
		return apgen::RETURN_STATUS::FAIL; }
	Full = !strcasecmp(*a->get_string(), "full");
	L = new ListOVal;
	while((req = the_instances->next())) {
		if(Full || !req->has_parent()) {
			ArrayElement*	tds;
			TypedValue	V;

			if(first) {
				first = false;
				L->set_array_type(TypedValue::arrayType::STRUCT_STYLE); }
			tds = new ArrayElement(req->get_unique_id(), apgen::DATA_TYPE::INSTANCE);
			V = *req;
			tds->set_typed_val(V);
			L->add(tds);
		}
	}
	*res = *L;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_RESOURCE_NAMES(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	RETRIEVE1(a, "get_resource_names")
	if(!a->is_string()) {
		rmessage << "get_resource_names: Argument must be a string";
		return apgen::RETURN_STATUS::FAIL;
	}
	RCsource* container = RCsource::resource_containers().find(a->get_string());
	if(!container) {
		rmessage << "get_resource_names: resource container " << a->get_string() << " does not exist";
		return apgen::RETURN_STATUS::FAIL;
	}
	vector<Rsource*>& simple_res = container->payload->Object->array_elements;
	ListOVal* lov = new ListOVal;
	ArrayElement* ae;
	for(int i = 0; i < simple_res.size(); i++) {
		Rsource* R = simple_res[i];
		lov->add(ae = new ArrayElement(i));
		ae->Val() = R->name;
	}
	*res = *lov;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_RESOURCE_HISTORY(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	RETRIEVE1(a, "get_resource_history")
	if(!a->is_string()) {
		rmessage << "get_resource_history: Argument must be a string";
		return apgen::RETURN_STATUS::FAIL;
	}
	Rsource* R = eval_intfc::FindResource(a->get_string());
	if(!R) {
		rmessage << "get_resource_history: resource " << a->get_string() << " does not exist";
		return apgen::RETURN_STATUS::FAIL;
	}
	tlist<alpha_time, unattached_value_node, Rsource*> L(R);
	long int version = 0;
	R->get_values(CTime_base(0,0,false), CTime_base(0,0,false), L, true, version);
	ListOVal* lov = new ListOVal;
	tlist<alpha_time, unattached_value_node, Rsource*>::iterator Liter(L);
	unattached_value_node* v;
	ArrayElement* ae;
	while((v = Liter())) {
		lov->add(ae = new ArrayElement(v->Key.getetime().to_string(), v->val));
	}
	*res = *lov;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_SORT_BY_INTEGER(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_SORT")
	if(!a->is_array()) {
		rmessage << "Argument must be an array";
		return apgen::RETURN_STATUS::FAIL; }
	long		N = a->get_array().get_length();
	long		i;
	blist		X(compare_function(compare_int_nodes, false));
	sorted_int_node* sn;
	ListOVal*	L;
	ArrayElement*	ae;
	bool		first = true;

	for(i = 0L; i < N; i++) {
		ae = a->get_array()[i];
		if(!ae->Val().is_int()) {
			rmessage << "array must contain integers";
			return apgen::RETURN_STATUS::FAIL; }
		X << new sorted_int_node(ae->Val().get_int(), i);
	}
	i = 0L;
	L = new ListOVal;
	for(sn = (sorted_int_node *) X.earliest_node(); sn; sn = (sorted_int_node *) sn->following_node()) {
		TypedValue		V;
		V = sn->ind;
		ae = new ArrayElement(i++);
		ae->set_typed_val(V);
		if(first) {
			first = false;
			L->set_array_type(TypedValue::arrayType::LIST_STYLE);
		}
		L->add(ae);
	}
	*res = *L;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_SORT_BY_TIME(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_SORT")
	if(!a->is_array()) {
		rmessage << "Argument must be an array";
		return apgen::RETURN_STATUS::FAIL; }
	long		N = a->get_array().get_length();
	long		i;
	blist		X(compare_function(compare_Time_nodes, true));
	sorted_time_node *sn;
	ListOVal*	L;
	ArrayElement*	ae;
	bool		first = true;

	for(i = 0L; i < N; i++) {
		ae = a->get_array()[i];
		if(!ae->Val().is_time()) {
			rmessage << "array must contain times";
			return apgen::RETURN_STATUS::FAIL;
		}
		X << new sorted_time_node(ae->Val().get_time_or_duration(), i);
	}
	i = 0L;
	L = new ListOVal;
	for(sn = (sorted_time_node *) X.earliest_node(); sn; sn = (sorted_time_node *) sn->following_node()) {
		TypedValue		V;
		V = sn->ind;
		ae = new ArrayElement(i++);
		ae->set_typed_val(V);
		if(first) {
			first = false;
			L->set_array_type(TypedValue::arrayType::LIST_STYLE);
		}
		L->add(ae);
	}
	*res = *L;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_UNLABELED_CHILDREN_OF(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	RETRIEVE1(a , "exp_GET_UNLABELED_CHILDREN_OF")
	if(a->is_instance()) {
		behaving_element	be = a->get_object();

		if(!be) {
			rmessage << "trying to get unlabeled children of non-existent activity";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(!be->get_req()) {
			rmessage << "trying to get unlabeled children of non-existent activity";
			return apgen::RETURN_STATUS::FAIL;
		}
		ActivityInstance*	req = be->get_req();

		if(!req) {
			rmessage << "trying to get unlabeled children of non-existent activity";
			return apgen::RETURN_STATUS::FAIL;
		}
		slist<alpha_void, smart_actptr>::iterator theKids(req->get_down_iterator());
		smart_actptr*		ptr;
		ActivityInstance*	child;
		ListOVal*		L = new ListOVal;
		ArrayElement*		tds;
		TypedValue		V;
		int			k = 0;

		if(req->children_count()) {
			// We set the type to 1, meaning that the array elements are integer-indexed:
			L->set_array_type(TypedValue::arrayType::LIST_STYLE);
		}
		while((ptr = theKids())) {
			child = ptr->BP;
			// tds = new ArrayElement(child->get_unique_id(), apgen::DATA_TYPE::INSTANCE);
			tds = new ArrayElement(k++);
			V = *child;
			tds->set_typed_val(V);
			L->add(tds);
		}
		*res = *L;
	} else {
		rmessage << "trying to get children of non-instance type " << spell(a->get_type());
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

// Formerly in ACT_io.C; converted to CTime_base method:
apgen::RETURN_STATUS TimeOfDay(
		CTime_base&	theTime,
		const Cstring&	given_timesystem,
		long&		sols,
		long&		hours,
		long&		minutes,
		double&		seconds) {
	CTime_base	orig(0, 0, false);
	double		Scale = 1.0;
	CTime_base	delta(0, 0, true);

	if(given_timesystem.length()) {
		/* emulate activity_display::adTimeSystemChangeCallback(). This could be
		 * added as a built-in to the CTime_base class, but we would then increase
		 * inter-dependencies which is not so good. */
		try {
			TypedValue&	tdv = globalData::get_symbol(given_timesystem);
			if(globalData::isAnEpoch(given_timesystem)) {
				orig = tdv.get_time_or_duration();
			} else if(globalData::isATimeSystem(given_timesystem)) {
				ListOVal*	LV = &tdv.get_array();
				ArrayElement*	element = LV->find("origin");

				orig = element->Val().get_time_or_duration();
				element = LV->find("scale");
				Scale = element->Val().get_double();
			} else {
				return apgen::RETURN_STATUS::FAIL;
			}
		} catch(eval_error) {
			return apgen::RETURN_STATUS::FAIL;
		}
	}
	delta = (theTime - orig) / Scale;
	sols = (long) (delta.get_seconds() / SECS_PER_DAY);

	delta -= CTime_base(sols * SECS_PER_DAY, 0L, true);
	if(delta < CTime_base(0, 0, true)) {
		delta += CTime_base(SECS_PER_DAY, 0L, true);
	}
	hours = (delta.get_seconds() / 60L) / 60L;
	minutes = (delta.get_seconds() / 60L) % 60L;
	seconds = (double) (delta.get_seconds() % 60L);
	seconds += 0.001 * delta.get_milliseconds();
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_TIME_OF_DAY(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_TIME_OF_DAY")
	if(a->is_time() && b->is_string()) {
		long			hours, minutes, sols;
		double			seconds;
		ListOVal		*L;
		ArrayElement		*tds;
		TypedValue		V;
		CTime_base		theTime(a->get_time_or_duration());

		if(TimeOfDay(
				theTime,
				b->get_string(),
				sols,
				hours,
				minutes,
				seconds) != apgen::RETURN_STATUS::SUCCESS) {
			rmessage << "TIME_OF_DAY: epoch/time system \"" << b->get_string()
				<< "\" doesn't seem to exist. ";
			return apgen::RETURN_STATUS::FAIL; }
		L = new ListOVal;
		// We set the type to 2, meaning that the array elements are string-indexed:
		L->set_array_type(TypedValue::arrayType::STRUCT_STYLE);
		tds = new ArrayElement("hours", apgen::DATA_TYPE::INTEGER);
		V = hours;
		// should check errors in principle... what could go wrong!?
		tds->set_typed_val(V);
		L->add(tds);
		tds = new ArrayElement("minutes", apgen::DATA_TYPE::INTEGER);
		V = minutes;
		// should check errors in principle... what could go wrong!?
		tds->set_typed_val(V);
		L->add(tds);
		tds = new ArrayElement("seconds", apgen::DATA_TYPE::FLOATING);
		V = seconds;
		// should check errors in principle... what could go wrong!?
		tds->set_typed_val(V);
		L->add(tds);
		tds = new ArrayElement("sols", apgen::DATA_TYPE::INTEGER);
		V = sols;
		// should check errors in principle... what could go wrong!?
		tds->set_typed_val(V);
		L->add(tds);
		*res = *L;
	} else {
		rmessage << "time-of-day(";
		a->print(rmessage, MAX_SIGNIFICANT_DIGITS);
		rmessage << " , ";
		b->print(rmessage, MAX_SIGNIFICANT_DIGITS);
		rmessage << "): argument type error; non-time type " << spell(a->get_type())
			<< " and/or non-string type " << spell(b->get_type()) << ". ";
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_PARENT_OF(
		Cstring&	rmessage,
		TypedValue*	res,
		slst<TypedValue*>& args) {
	RETRIEVE1(a , "exp_GET_PARENT_OF")
	if(a->is_instance()) {
		behaving_element	be = a->get_object();
		behaving_element	parent;

		if(!be) {
			rmessage << "trying to get parent of non-existent object";
			return apgen::RETURN_STATUS::FAIL;
		}
		try {
			parent = (*be)["parent"].get_object();
			*res = parent;
		} catch(eval_error) {
			// Steve Wissler's ISA Z92966
			*res = be;
		}
	} else {
		rmessage << "trying to get parent of non-instance type " << spell(a->get_type());
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS exp_GET_TYPE_OF(Cstring &rmessage , TypedValue *res , slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_GET_TYPE_OF")
	if(a->is_instance()) {
		behaving_element	be = a->get_object();

		if(!be) {
			rmessage << "trying to get type of non-existent activity";
			return apgen::RETURN_STATUS::FAIL;
		}
		*res = (*be)["type"].get_string();
	} else {
		rmessage << "trying to get type of non-instance type " << spell(a->get_type());
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

/*
  arguments

  1 string headerString
  2 string message

*/

apgen::RETURN_STATUS exp_SENDREQUEST(Cstring &rmessage, TypedValue *result, slst<TypedValue*>& args) {
  RETRIEVE2(a, b, "exp_SENDREQUEST");

  if((!a->is_string()) || (!b->is_string())) {
    rmessage = "bad arg in SENDREQUEST";
    return apgen::RETURN_STATUS::FAIL; }

    rmessage = "SENDREQUEST is not available in apcore. ";
    return apgen::RETURN_STATUS::FAIL;
}

/*
  There are 2 arguments

  string headerString
  string message
*/

apgen::RETURN_STATUS exp_SENDEVENT(Cstring &rmessage, TypedValue *result, slst<TypedValue*>& args) {
    rmessage = "SENDEVENT is not available in apcore. ";
    return apgen::RETURN_STATUS::FAIL; }

/*
  the arguments to this function are:

  none
  returns a headerString

*/

apgen::RETURN_STATUS exp_GET_HEADER(Cstring &rmessage, TypedValue* result, slst<TypedValue*>& args) {
    rmessage = "GET_HEADER is not available in apcore. ";
    return apgen::RETURN_STATUS::FAIL; }


apgen::RETURN_STATUS
exp_GET_INSTANCE_DATA(Cstring &rmessage, TypedValue* result, slst<TypedValue*>& args) {
	RETRIEVE1(a , "exp_GET_INSTANCE_DATA");

	if(!a->is_instance()) {
		rmessage = "bad arg in GET_INSTANCE_DATA";
		return apgen::RETURN_STATUS::FAIL;
	}


	behaving_element	be = a->get_object();
	if(!be || !be->get_req()) {
		rmessage = "1st arg is not an activity instance in get_instance_string";
		return apgen::RETURN_STATUS::FAIL;
	}
	ActivityInstance* req = be->get_req();
	ListOVal* lov = new ListOVal;
	ArrayElement* ae;

	const Behavior& T = req->Object->Type;
	task* constructor = T.tasks[0];
	for(int i = 0; i < req->Object->size(); i++) {
		lov->add(ae = new ArrayElement(constructor->get_varinfo()[i].first));
		ae->Val() = (*req->Object)[i];
	}
	*result = *lov;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS
exp_GET_INSTANCE_STRING(Cstring &rmessage, TypedValue* result, slst<TypedValue*>& args) {
	RETRIEVE1(a , "exp_GET_INSTANCE_STRING");

	if(!a->is_instance()) {
		rmessage = "bad arg in GET_INSTANCE_STRING";
		return apgen::RETURN_STATUS::FAIL;
	}

	//build a string here
	aoString buildString;

	behaving_element	be = a->get_object();
	if(!be || !be->get_req()) {
		rmessage = "1st arg is not an activity instance in get_instance_string";
		return apgen::RETURN_STATUS::FAIL;
	}
	ActivityInstance* req = be->get_req();

	//
	//send the instance to the writer
	//
	req->transfer_to_stream(
			buildString,
			apgen::FileType::FT_APF,
			/* bounds = */ NULL);
	*result = buildString.str();
	return apgen::RETURN_STATUS::SUCCESS;
}


apgen::RETURN_STATUS exp_REMOVEQUOTES(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "remove_quotes")
	if(! a->is_string()) {
		rmessage = "non-string arg. in removequotes";
		return apgen::RETURN_STATUS::FAIL;
	}
	Cstring myStr = a->get_string();
	removeQuotes(myStr);
	*res = myStr;
	return apgen::RETURN_STATUS::SUCCESS;
}


/*
  The arguments to this function are
  apgen::DATA_TYPE::STRING: callbackName (type of function to be registered)
  apgen::DATA_TYPE::STRING: nameString (name of the callback)
  apgen::DATA_TYPE::STRING: functionName (name of function to call with arguments later)
*/

apgen::RETURN_STATUS exp_REGISTER_CALLBACK(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
  RETRIEVE3(a, b, c, "exp_REGISTER_CALLBACK");

  if((!a->is_string()) || (!b->is_string()) || (!c->is_string())) {
    rmessage = "bad arg in REGISTER_CALLBACK";
    return apgen::RETURN_STATUS::FAIL; }

  string callbackName(*(a->get_string()));
  string nameString(*(b->get_string()));
  string functionName(*(c->get_string()));
  UserFunctionEventClient* client = UserFunctionEventClientManager::Manager().Create(callbackName, nameString, functionName);

  return apgen::RETURN_STATUS::SUCCESS; }

/*
  The arguments to this function are
  apgen::DATA_TYPE::STRING: callbackName (type of function to be unregistered)
  apgen::DATA_TYPE::STRING: nameString (name of the callback)
*/

apgen::RETURN_STATUS exp_UNREGISTER_CALLBACK(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
  RETRIEVE2(a, b, "exp_UNREGISTER_CALLBACK");

  if((!a->is_string()) || (!b->is_string())) {
    rmessage = "bad arg in UNREGISTER_CALLBACK";
    return apgen::RETURN_STATUS::FAIL; }

  string callbackName(*(a->get_string()));
  string nameString(*(b->get_string()));

  //delete
  UserFunctionEventClientManager::Manager().Delete(callbackName, nameString);
  return apgen::RETURN_STATUS::SUCCESS; }

static bool interval_union(bool a, bool b) {
       return a || b;
}

static bool interval_intersection(bool a, bool b) {
       return a && b;
}

static bool interval_minus(bool a, bool b) {
       return a && !b;
}

static bool interval_xor(bool a, bool b) {
       return a != b;
}

apgen::RETURN_STATUS	exp_intervals(Cstring& rmessage, TypedValue* result, slst<TypedValue*>& args) {
	int		numberOfArgs = -1;

	rmessage.undefine();
	numberOfArgs = args.size() ;
	if(numberOfArgs < 2 || numberOfArgs > 3) {
		rmessage << "  wrong number of parameters passed to intervals(); expected 2 or 3, got "
			<< numberOfArgs << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	TypedValue*	a = NULL;
	TypedValue*	b = NULL;
	TypedValue*	c = NULL;
	ListOVal*	lov = NULL;
	if(numberOfArgs == 3) {
		c = args.pop_front();
		b = args.pop_front();
		a = args.pop_front();
		if((!a->is_string()) || (!b->is_array()) || (!c->is_array())) {
			rmessage = "bad arg. in intervals() - expect (string, array, array)";
			return apgen::RETURN_STATUS::FAIL;
		}

		bool (*membership_evaluator)(bool, bool) = NULL;

		if(a->get_string() == "and") {
			membership_evaluator = interval_intersection;
		} else if(a->get_string() == "or") {
			membership_evaluator = interval_union;
		} else if(a->get_string() == "minus") {
			membership_evaluator = interval_minus;
		} else if(a->get_string() == "xor") {
			membership_evaluator = interval_xor;
		} else {
			rmessage << "first arg of intervals() must be 'and', 'or', 'minus' or 'xor', not '"
				<< a->get_string() << "'\n";
			return apgen::RETURN_STATUS::FAIL;
		}

		ArrayElement*					ae;
		slist<alpha_time, Cnode0<alpha_time, int> >	A;
		slist<alpha_time, Cnode0<alpha_time, int> >	B;
		bool						first = true;
		CTime_base					start_time, end_time, prev_end_time;

		if(A.get_length() % 2) {
			rmessage = "first list passed to intervals() should contain an even number of times\n";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(B.get_length() % 2) {
			rmessage = "second list passed to intervals() should contain an even number of times\n";
			return apgen::RETURN_STATUS::FAIL;
		}
		for(int iA = 0; iA < b->get_array().get_length(); iA++) {
			ae = b->get_array()[iA];
			if(!ae->Val().is_time()) {
				rmessage = "first list in intervals() does not contain time values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			start_time = ae->Val().get_time_or_duration();

			ae = b->get_array()[++iA];

			if(!ae->Val().is_time()) {
				rmessage = "first list in intervals() does not contain time values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			end_time = ae->Val().get_time_or_duration();

			if(first) {
				first = false;
				A << new Cnode0<alpha_time, int>(start_time, 0);
			} else if(start_time < prev_end_time) {
				rmessage = "First list of intervals() contains overlapping intervals.\n";
				rmessage << "First offending interval ends at " << prev_end_time.to_string()
					<< ", second starts at " << start_time.to_string() << "\n";
				return apgen::RETURN_STATUS::FAIL;
			} else if(start_time > prev_end_time) {
				A << new Cnode0<alpha_time, int>(prev_end_time, 0);
				A << new Cnode0<alpha_time, int>(start_time, 0);
			}
			prev_end_time = end_time;
		}
		if(!first) {
			// A ends with the last end time
			A << new Cnode0<alpha_time, int>(prev_end_time, 0);
		}

		first = true;
		for(int iB = 0; iB < c->get_array().get_length(); iB++) {
			ae = c->get_array()[iB];
			if(!ae->Val().is_time()) {
				rmessage = "second list in intervals() does not contain time values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			start_time = ae->Val().get_time_or_duration();

			ae = c->get_array()[++iB];

			if(!ae->Val().is_time()) {
				rmessage = "second list in intervals() does not contain time values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			end_time = ae->Val().get_time_or_duration();

			if(first) {
				first = false;
				B << new Cnode0<alpha_time, int>(start_time, 0);
			} else if(start_time < prev_end_time) {
				rmessage = "Second list of intervals() contains overlapping intervals\n";
				rmessage << "First offending interval ends at " << prev_end_time.to_string()
					<< ", second starts at " << start_time.to_string() << "\n";
				return apgen::RETURN_STATUS::FAIL;
			} else if(start_time > prev_end_time) {
				B << new Cnode0<alpha_time, int>(prev_end_time, 0);
				B << new Cnode0<alpha_time, int>(start_time, 0);
			}
			prev_end_time = end_time;
		}
		if(!first) {
			// B ends with the last end time
			B << new Cnode0<alpha_time, int>(prev_end_time, 0);
		}

		lov = new ListOVal;
		Miterator<slist<alpha_time, Cnode0<alpha_time, int> >, Cnode0<alpha_time, int> >
			miter("miterator");

		long int	Index = 0;

		miter.add_thread(A, "A", 0);
		miter.add_thread(B, "B", 1);
		miter.first();

		Cnode0<alpha_time, int>*		curnode = NULL;
		bool				isInA = false;
		bool				isInB = false;
		bool				isInC = false;
		bool				wasInC = false;
		while((curnode = miter.next())) {
			if(curnode->list == &A) {
				isInA = !isInA;
			} else if(curnode->list == &B) {
				isInB = !isInB;
			}
			if(miter.peek() && miter.peek()->getKey().getetime() == curnode->getKey().getetime()) {
				curnode = miter.next();
				if(curnode->list == &A) {
					isInA = !isInA;
				} else if(curnode->list == &B) {
					isInB = !isInB;
				}
			}
			CTime_base	itime = curnode->getKey().getetime();
			wasInC = isInC;
			isInC = membership_evaluator(isInA, isInB);
			if(isInC != wasInC) {
				lov->add(ae = new ArrayElement(Index++));
				ae->Val() = itime;
			}
		}
	} else {
		b = args.pop_front();
		a = args.pop_front();
		if((!a->is_string()) || (!b->is_array())) {
			rmessage = "bad arg. in intervals() - expect (string, array)";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(a->get_string() != "sort") {
			rmessage = "bad arg. 1 in intervals() - expect 'sort'";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(b->get_array().get_length() % 2) {
			rmessage = "bad arg. 2 in intervals() - array should contain an even number of time values";
			return apgen::RETURN_STATUS::FAIL;
		}

		ArrayElement*						ae;
		tlist<alpha_time, Cnode0<alpha_time, CTime_base> >	A(true);
		CTime_base						itime, old_time;
		long int						Index = 0;

		for(int iA = 0; iA < b->get_array().get_length(); iA++) {
			ae = b->get_array()[iA];
			if(!ae->Val().is_time()) {
				rmessage = "first list in intervals() does not contain time values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			itime = ae->Val().get_time_or_duration();
			if(Index % 2) {

				//
				// endpoint
				//
				if(itime < old_time) {
					rmessage = "list should contain time intervals() of positive duration\n";
					return apgen::RETURN_STATUS::FAIL;
				} else {
					A << new Cnode0<alpha_time, CTime_base>(old_time, itime);
				}
			}
			old_time = itime;
			Index++;
		}

		//
		// A is a t-list with consistent linked list order
		//
		tlist<alpha_time, Cnode0<alpha_time, CTime_base> >::iterator	iter2(A);
		Cnode0<alpha_time, CTime_base>*					curnode = NULL;
		Index = 0;

		lov = new ListOVal;
		while((curnode = iter2())) {
			old_time = curnode->getKey().getetime();
			itime = curnode->payload;

			lov->add(ae = new ArrayElement(Index++));
			ae->Val() = old_time;

			lov->add(ae = new ArrayElement(Index++));
			ae->Val() = itime;
		}
	}
	*result = *lov;
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS	exp_int_intervals(Cstring& rmessage, TypedValue* result, slst<TypedValue*>& args) {
	int		numberOfArgs = -1;

	rmessage.undefine();
	numberOfArgs = args.size();
	if(numberOfArgs < 2 || numberOfArgs > 3) {
		rmessage << "  wrong number of parameters passed to int_intervals(); expected 2 or 3, got "
			<< numberOfArgs << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	TypedValue*	a = NULL;
	TypedValue*	b = NULL;
	TypedValue*	c = NULL;
	ListOVal*	lov = NULL;
	if(numberOfArgs == 3) {
		c = args.pop_front();
		b = args.pop_front();
		a = args.pop_front();
		if((!a->is_string()) || (!b->is_array()) || (!c->is_array())) {
			rmessage = "bad arg. in int_intervals() - expect (string, array, array)";
			return apgen::RETURN_STATUS::FAIL;
		}

		bool (*membership_evaluator)(bool, bool) = NULL;

		if(a->get_string() == "and") {
			membership_evaluator = interval_intersection;
		} else if(a->get_string() == "or") {
			membership_evaluator = interval_union;
		} else if(a->get_string() == "minus") {
			membership_evaluator = interval_minus;
		} else if(a->get_string() == "xor") {
			membership_evaluator = interval_xor;
		} else {
			rmessage << "first arg of int_intervals() must be 'and', 'or', 'minus' or 'xor', not '"
				<< a->get_string() << "'\n";
			return apgen::RETURN_STATUS::FAIL;
		}

		ArrayElement*					ae;
		slist<alpha_int, Cnode0<alpha_int, int> >	A;
		slist<alpha_int, Cnode0<alpha_int, int> >	B;
		bool						first = true;
		long int					start_int, end_int, prev_end_int;

		if(A.get_length() % 2) {
			rmessage = "first list passed to int_intervals() should contain "
				"an even number of integers\n";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(B.get_length() % 2) {
			rmessage = "second list passed to int_intervals() should contain "
				"an even number of integers\n";
			return apgen::RETURN_STATUS::FAIL;
		}
		for(int iA = 0; iA < b->get_array().get_length(); iA++) {
			ae = b->get_array()[iA];
			if(!ae->Val().is_int()) {
				rmessage = "first list in int_intervals() does not contain "
					"integer values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			start_int = ae->Val().get_int();

			ae = b->get_array()[++iA];

			if(!ae->Val().is_int()) {
				rmessage = "first list in int_intervals() does not contain "
					"integer values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			end_int = ae->Val().get_int();

			if(first) {
				first = false;
				A << new Cnode0<alpha_int, int>(start_int, 0);
			} else if(start_int < prev_end_int) {
				rmessage = "First list of int_intervals() contains "
					"overlapping intervals\n";
				rmessage << "First offending interval ends at " << prev_end_int
					<< ", second starts at " << start_int << "\n";
				return apgen::RETURN_STATUS::FAIL;
			} else if(start_int > prev_end_int) {
				A << new Cnode0<alpha_int, int>(prev_end_int, 0);
				A << new Cnode0<alpha_int, int>(start_int, 0);
			}
			prev_end_int = end_int;
		}
		if(!first) {

			//
			// A ends with the last end int
			//
			A << new Cnode0<alpha_int, int>(prev_end_int, 0);
		}

		first = true;
		for(int iB = 0; iB < c->get_array().get_length(); iB++) {
			ae = c->get_array()[iB];
			if(!ae->Val().is_int()) {
				rmessage = "second list in int_intervals() does not "
					"contain integer values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			start_int = ae->Val().get_int();

			ae = c->get_array()[++iB];

			if(!ae->Val().is_int()) {
				rmessage = "second list in int_intervals() does not "
					"contain integer values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			end_int = ae->Val().get_int();

			if(first) {
				first = false;
				B << new Cnode0<alpha_int, int>(start_int, 0);
			} else if(start_int < prev_end_int) {
				rmessage = "Second list of int_intervals() contains "
					"overlapping intervals\n";
				rmessage << "First offending interval ends at " << prev_end_int
					<< ", second starts at " << start_int << "\n";
				return apgen::RETURN_STATUS::FAIL;
			} else if(start_int > prev_end_int) {
				B << new Cnode0<alpha_int, int>(prev_end_int, 0);
				B << new Cnode0<alpha_int, int>(start_int, 0);
			}
			prev_end_int = end_int;
		}
		if(!first) {
			// B ends with the last end int
			B << new Cnode0<alpha_int, int>(prev_end_int, 0);
		}

		lov = new ListOVal;
		Interator<slist<alpha_int, Cnode0<alpha_int, int> >, Cnode0<alpha_int, int>, alpha_int>
			miter("interator");

		long int	Index = 0;

		miter.add_thread(A, "A", 0);
		miter.add_thread(B, "B", 1);
		miter.first();

		Cnode0<alpha_int, int>*		curnode = NULL;
		bool				isInA = false;
		bool				isInB = false;
		bool				isInC = false;
		bool				wasInC = false;
		while((curnode = miter.next())) {
			if(curnode->list == &A) {
				isInA = !isInA;
			} else if(curnode->list == &B) {
				isInB = !isInB;
			}
			if(miter.peek() && miter.peek()->getKey().get_int() == curnode->getKey().get_int()) {
				curnode = miter.next();
				if(curnode->list == &A) {
					isInA = !isInA;
				} else if(curnode->list == &B) {
					isInB = !isInB;
				}
			}
			long int	interval_bound = curnode->getKey().get_int();
			wasInC = isInC;
			isInC = membership_evaluator(isInA, isInB);
			if(isInC != wasInC) {
				lov->add(ae = new ArrayElement(Index++));
				ae->Val() = interval_bound;
			}
		}
	} else {
		b = args.pop_front();
		a = args.pop_front();
		if((!a->is_string()) || (!b->is_array())) {
			rmessage = "bad arg. in int_intervals() - expect (string, array)";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(a->get_string() != "sort") {
			rmessage = "bad arg. 1 in int_intervals() - expect 'sort'";
			return apgen::RETURN_STATUS::FAIL;
		}
		if(b->get_array().get_length() % 2) {
			rmessage = "bad arg. 2 in int_intervals() - "
				"array should contain an even number of integer values";
			return apgen::RETURN_STATUS::FAIL;
		}

		ArrayElement*						ae;
		tlist<alpha_int, Cnode0<alpha_int, long int> >		A(true);
		long int						interval_bound, prev_bound;
		long int						Index = 0;

		for(int iA = 0; iA < b->get_array().get_length(); iA++) {
			ae = b->get_array()[iA];
			if(!ae->Val().is_int()) {
				rmessage = "first list in int_intervals() does not "
					"contain integer values\n";
				return apgen::RETURN_STATUS::FAIL;
			}
			interval_bound = ae->Val().get_int();
			if(Index % 2) {

				//
				// endpoint
				//
				if(interval_bound < prev_bound) {
					rmessage = "list should contain integer "
						"intervals of positive duration\n";
					return apgen::RETURN_STATUS::FAIL;
				} else {
					A << new Cnode0<alpha_int, long int>(prev_bound, interval_bound);
				}
			}
			prev_bound = interval_bound;
			Index++;
		}

		//
		// A is a tlist with consistent linked list order
		//
		tlist<alpha_int, Cnode0<alpha_int, long int> >::iterator	iter2(A);
		Cnode0<alpha_int, long int>*					curnode = NULL;
		Index = 0;

		//
		// Stick the intervals in order of increasing interval starts
		// into a new array
		//
		long int							interval_start;
		long int							interval_end;
		lov = new ListOVal;
		while((curnode = iter2())) {
			interval_start = curnode->getKey().get_int();
			interval_end = curnode->payload;

			lov->add(ae = new ArrayElement(Index++));
			ae->Val() = interval_start;

			lov->add(ae = new ArrayElement(Index++));
			ae->Val() = interval_end;
		}
	}
	*result = *lov;
	return apgen::RETURN_STATUS::SUCCESS;
}

void add_internal_function(const char *n, generic_function q, apgen::DATA_TYPE dt) {
	Cstring		Int("internal");

	udef_intfc::add_to_Func_list(n, q, Int, dt);
}

void register_all_internal_functions() {
	add_internal_function("abs",			exp_ABS			, apgen::DATA_TYPE::INTEGER);
	add_internal_function("add_quotes",		exp_ADDQUOTES		, apgen::DATA_TYPE::STRING);
	add_internal_function("amin",			new_exp_AMIN		, apgen::DATA_TYPE::FLOATING);
	add_internal_function("tmin",			new_exp_TMIN		, apgen::DATA_TYPE::TIME);
	add_internal_function("dmin",			new_exp_DMIN		, apgen::DATA_TYPE::DURATION);
	add_internal_function("amax",			new_exp_AMAX		, apgen::DATA_TYPE::FLOATING);
	add_internal_function("tmax",			new_exp_TMAX		, apgen::DATA_TYPE::TIME);
	add_internal_function("dmax",			new_exp_DMAX		, apgen::DATA_TYPE::DURATION);
	add_internal_function("and",			exp_AND			, apgen::DATA_TYPE::BOOL_TYPE);
	add_internal_function("atan",			exp_atan		, apgen::DATA_TYPE::FLOATING);
	add_internal_function("atan2",			exp_atan2		, apgen::DATA_TYPE::FLOATING);
	add_internal_function("ceiling",		exp_CEILING		, apgen::DATA_TYPE::INTEGER);
	add_internal_function("copy_array",		exp_copy_array		, apgen::DATA_TYPE::ARRAY);
	add_internal_function("cos",			exp_cos			, apgen::DATA_TYPE::FLOATING);
	add_internal_function("cputime",		exp_cputime		, apgen::DATA_TYPE::TIME);
	add_internal_function("exp",			exp_exp			, apgen::DATA_TYPE::FLOATING);
	add_internal_function("delete_element",		exp_DELETE_ELEMENT	, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("delete_signal",		exp_DELETE_SIGNAL	, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("stop_debug",		exp_stop_debug		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("generate_error",		exp_ERROR		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("extract",		exp_EXTRACT		, apgen::DATA_TYPE::STRING);
	add_internal_function("fabs",			exp_FABS		, apgen::DATA_TYPE::FLOATING);
	add_internal_function("find_matches",		exp_FIND_MATCHES	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("fix",			exp_FIX			, apgen::DATA_TYPE::FLOATING);
	add_internal_function("fmod",			exp_FMOD		, apgen::DATA_TYPE::FLOATING);
	add_internal_function("fp",			exp_FP			, apgen::DATA_TYPE::FLOATING);
	add_internal_function("from_seconds",		exp_FROM_SECONDS	, apgen::DATA_TYPE::DURATION);
	add_internal_function("get_all_globals",	exp_get_all_globals	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_all_signals",	exp_GET_ALL_SIGNALS	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_children_of",	exp_GET_CHILDREN_OF	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_file_info",		exp_GET_FILE_INFO	, apgen::DATA_TYPE::STRING);
	add_internal_function("get_header",		exp_GET_HEADER		, apgen::DATA_TYPE::STRING);
	add_internal_function("get_id_of",		exp_GET_ID_OF		, apgen::DATA_TYPE::STRING);
	add_internal_function("get_instance_data",	exp_GET_INSTANCE_DATA	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_instance_string",	exp_GET_INSTANCE_STRING	, apgen::DATA_TYPE::STRING);
	add_internal_function("get_matching_signals",	exp_GET_MATCHING_SIGNALS, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_parent_of",		exp_GET_PARENT_OF	, apgen::DATA_TYPE::INSTANCE);
	add_internal_function("get_plan",		exp_GET_PLAN		, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_resource_history",	exp_GET_RESOURCE_HISTORY, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_resource_names",	exp_GET_RESOURCE_NAMES	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_selection",		exp_GET_SELECTION	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_selection_ids",	exp_GET_SELECTION_IDS	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_sender",		exp_GET_SENDER		, apgen::DATA_TYPE::INSTANCE);
	add_internal_function("get_sender_info",	exp_GET_SENDER_INFO	, apgen::DATA_TYPE::ARRAY);
	// add_internal_function("get_threads",		exp_get_threads		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("get_type_of",		exp_GET_TYPE_OF		, apgen::DATA_TYPE::STRING);
	add_internal_function("get_unlabeled_children_of",exp_GET_UNLABELED_CHILDREN_OF	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("get_version",		exp_GET_VERSION		, apgen::DATA_TYPE::STRING);
	add_internal_function("interrupt",		exp_interrupt		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("intervals",		exp_intervals		, apgen::DATA_TYPE::ARRAY);
	add_internal_function("int_intervals",		exp_int_intervals	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("is_frozen",		exp_is_frozen		, apgen::DATA_TYPE::BOOL_TYPE);
	add_internal_function("itostr",			exp_itostr		, apgen::DATA_TYPE::STRING);
	add_internal_function("log",			exp_log			, apgen::DATA_TYPE::FLOATING);
	add_internal_function("log10",			exp_log10		, apgen::DATA_TYPE::FLOATING);
	add_internal_function("length_of",		exp_length_of		, apgen::DATA_TYPE::INTEGER);
	add_internal_function("make_constant",		exp_make_constant	, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("max",			exp_MAX			, apgen::DATA_TYPE::INTEGER);
	add_internal_function("min",			exp_MIN			, apgen::DATA_TYPE::INTEGER);
	add_internal_function("notify_client",		exp_notify_client	, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("numeric_index_of",	exp_numeric_index_of	, apgen::DATA_TYPE::INTEGER);
	add_internal_function("or",			exp_OR			, apgen::DATA_TYPE::BOOL_TYPE);
	add_internal_function("printf",			exp_printf		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("require",		exp_REQUIRE		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("rnd",			exp_RND			, apgen::DATA_TYPE::INTEGER);
	add_internal_function("send_request",		exp_SENDREQUEST		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("send_event",		exp_SENDEVENT		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("remove_quotes",		exp_REMOVEQUOTES	, apgen::DATA_TYPE::STRING);
	add_internal_function("register_callback",	exp_REGISTER_CALLBACK	, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("start_debug",		exp_start_debug		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("sin",			exp_sin			, apgen::DATA_TYPE::FLOATING);
	add_internal_function("sort_by_integer",	exp_SORT_BY_INTEGER	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("sort_by_time",		exp_SORT_BY_TIME	, apgen::DATA_TYPE::ARRAY);
	add_internal_function("sqrt",			exp_sqrt		, apgen::DATA_TYPE::FLOATING);
	add_internal_function("strtoi",			exp_strtoi		, apgen::DATA_TYPE::INTEGER);
	add_internal_function("stop",			exp_STOP		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("string_index_of",	exp_string_index_of	, apgen::DATA_TYPE::STRING);
	add_internal_function("strrel",			exp_STRREL		, apgen::DATA_TYPE::STRING);
	// add_internal_function("time_series",		exp_time_series		, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("tan",			exp_tan			, apgen::DATA_TYPE::FLOATING);
	add_internal_function("time_of_day",		exp_TIME_OF_DAY		, apgen::DATA_TYPE::ARRAY);
	add_internal_function("to_seconds",		exp_TO_SECONDS		, apgen::DATA_TYPE::FLOATING);
	add_internal_function("to_string",		exp_TO_STRING		, apgen::DATA_TYPE::STRING);
	add_internal_function("trim",			exp_TRIM		, apgen::DATA_TYPE::STRING);
	add_internal_function("unregister_callback",	exp_UNREGISTER_CALLBACK	, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("xcmd",			exp_XR			, apgen::DATA_TYPE::ARRAY);
	add_internal_function("xor",			exp_XOR			, apgen::DATA_TYPE::BOOL_TYPE);

	// the following functions used to be part of the user-defined library:

	add_internal_function("acos",			exp_acos, apgen::DATA_TYPE::FLOATING);
	add_internal_function("asin",			exp_asin, apgen::DATA_TYPE::FLOATING);
	add_internal_function("display_message_to_user",exp_displayMessageToUser, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("element_exists",		exp_element_exists, apgen::DATA_TYPE::BOOL_TYPE);
	add_internal_function("GetEnv",			exp_GetEnv, apgen::DATA_TYPE::STRING);
	add_internal_function("get_global",		exp_get_global, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("global_exists",		exp_global_exists, apgen::DATA_TYPE::BOOL_TYPE);
	add_internal_function("has_substring",		exp_has_substring, apgen::DATA_TYPE::BOOL_TYPE);
	add_internal_function("int2ascii",		exp_int2ascii, apgen::DATA_TYPE::STRING);
	add_internal_function("random",			exp_random, apgen::DATA_TYPE::FLOATING);
	add_internal_function("read_xmltol_as_incon",	exp_read_xmltol_as_incon, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("string_to_float",	exp_string_to_float, apgen::DATA_TYPE::FLOATING);
	add_internal_function("string_to_int",		exp_string_to_int, apgen::DATA_TYPE::INTEGER);
	add_internal_function("string_to_time",		exp_string_to_time, apgen::DATA_TYPE::TIME);
	add_internal_function("Strlen",			exp_Strlen, apgen::DATA_TYPE::INTEGER);
	add_internal_function("strstr",			exp_strstr, apgen::DATA_TYPE::STRING);
	add_internal_function("time_to_string",		exp_time_to_string, apgen::DATA_TYPE::STRING);
	add_internal_function("write_behavior",		exp_write_behavior, apgen::DATA_TYPE::UNINITIALIZED);
	add_internal_function("write_to_stdout",	exp_write_to_stdout, apgen::DATA_TYPE::UNINITIALIZED);
}

Cstring	 pEsys::print_func(generic_function func) {
	if(func == exp_ABS) return Cstring("abs");
	else if(func == exp_ADDQUOTES) return Cstring("add_quotes");
	else if(func == new_exp_AMAX) return Cstring("amax");
	else if(func == new_exp_TMAX) return Cstring("tmax");
	else if(func == new_exp_DMAX) return Cstring("dmax");
	else if(func == new_exp_AMIN) return Cstring("amin");
	else if(func == new_exp_TMIN) return Cstring("tmin");
	else if(func == new_exp_DMIN) return Cstring("dmin");
	else if(func == exp_AND) return Cstring("and");
	else if(func == exp_atan2) return Cstring("atan2");
	else if(func == exp_CEILING) return Cstring("ceiling");
	else if(func == exp_copy_array) return Cstring("copy_array");
	else if(func == exp_cos) return Cstring("cos");
	else if(func == exp_DELETE_ELEMENT) return Cstring("delete_element");
	else if(func == exp_DELETE_SIGNAL) return Cstring("delete signal");
	else if(func == exp_divide) return Cstring("divide");
	else if(func == exp_ERROR) return Cstring("generate_error");
	else if(func == exp_equal) return Cstring("equal");
	else if(func == exp_exponent) return Cstring("exponent");
	else if(func == exp_exp) return Cstring("exp");
	else if(func == exp_EXTRACT) return Cstring("extract");
	else if(func == exp_FABS) return Cstring("Fabs");
	else if(func == exp_FIND_MATCHES) return Cstring("find matches");
	else if(func == exp_FIX) return Cstring("fix");
	else if(func == exp_FMOD) return Cstring("fmod");
	else if(func == exp_FP) return Cstring("FP");
	else if(func == exp_FROM_SECONDS) return Cstring("from_seconds");
	else if(func == exp_get_all_globals) return Cstring("get all globals");
	else if(func == exp_get_global) return Cstring("get all globals");
	else if(func == exp_global_exists) return Cstring("global exists");
	else if(func == exp_GET_ALL_SIGNALS) return Cstring("get all signals");
	else if(func == exp_GET_CHILDREN_OF) return Cstring("get children of");
	else if(func == exp_GET_HEADER) return Cstring("get_header");
	else if(func == exp_GET_ID_OF) return Cstring("get id of");
	else if(func == exp_GET_MATCHING_SIGNALS) return Cstring("get matching signals");
	else if(func == exp_GET_PLAN) return Cstring("get plan");
	else if(func == exp_GET_RESOURCE_HISTORY) return Cstring("get resource history");
	else if(func == exp_GET_RESOURCE_NAMES) return Cstring("get resource names");
	else if(func == exp_GET_SELECTION) return Cstring("get selection");
	else if(func == exp_GET_SELECTION_IDS) return Cstring("get selection ids");
	else if(func == exp_GET_SENDER) return Cstring("get sender");
	else if(func == exp_GET_SENDER_INFO) return Cstring("get senders");
	// else if(func == exp_get_threads) return Cstring("get_threads");
	else if(func == exp_GET_UNLABELED_CHILDREN_OF) return Cstring("get unlabeled children of");
	else if(func == exp_GET_VERSION) return Cstring("get_version");
	else if(func == exp_GTRTHAN) return Cstring(" > ");
	else if(func == exp_GTRTHANOREQ) return Cstring(" >= ");
	else if(func == exp_has_substring) return Cstring("has substring");
	else if(func == exp_instance_with_id) return Cstring("instance_with_id");
	else if(func == exp_interrupt) return Cstring("interrupt");
	else if(func == exp_itostr) return Cstring("itostr");
	else if(func == exp_last_event) return Cstring("last_event");
	else if(func == exp_length_of) return Cstring("length_of");
	else if(func == exp_make_constant) return Cstring("make_constant");
	else if(func == exp_LESSTHAN) return Cstring(" < ");
	else if(func == exp_LESSTHANOREQ) return Cstring(" <= ");
	else if(func == exp_log) return Cstring("log");
	else if(func == exp_log10) return Cstring("log10");
	else if(func == exp_MAX) return Cstring("max");
	else if(func == exp_MIN) return Cstring("min");
	else if(func == exp_MINUS) return Cstring(" - ");
	else if(func == exp_MOD) return Cstring(" % ");
	else if(func == exp_MULT) return Cstring(" * ");
	else if(func == exp_NEG) return Cstring(" - ");
	else if(func == exp_NOTEQUAL) return Cstring(" != ");
	else if(func == exp_notify_client) return Cstring("notify_client");
	else if(func == exp_numeric_index_of) return Cstring("numeric_index_of");
	else if(func == exp_PAREN) return Cstring(" open paren. ");
	else if(func == exp_PLUS) return Cstring(" + ");
	else if(func == exp_printf) return Cstring("printf");
	else if(func == exp_OR) return Cstring("or");
	else if(func == exp_REMOVEQUOTES) return Cstring("remove_quotes");
	else if(func == exp_REGISTER_CALLBACK) return Cstring("register_callback");
	else if(func == exp_REQUIRE) return Cstring("require");
	else if(func == exp_RND) return Cstring("rnd");
	else if(func == exp_sin) return Cstring("sin");
	else if(func == exp_SORT_BY_INTEGER) return Cstring("sort by integer");
	else if(func == exp_SORT_BY_TIME) return Cstring("sort by time");
	else if(func == exp_sqrt) return Cstring("sqrt");
	else if(func == exp_strtoi) return Cstring("strtoi");
	else if(func == exp_string_index_of) return Cstring("string_index_of");
	else if(func == exp_tan) return Cstring("tan");
	// else if(func == exp_time_series) return Cstring("time_series");
	else if(func == exp_TIME_OF_DAY) return Cstring("time_of_day");
	else if(func == exp_TO_SECONDS) return Cstring("to_seconds");
	else if(func == exp_TO_STRING) return Cstring("to_string");
	else if(func == exp_TRIM) return Cstring("trim");
	else if(func == exp_write_behavior) return Cstring("write_behavior");
	else if(func == exp_write_to_stdout) return Cstring("write_to_stdout");
	else if(func == exp_XOR) return Cstring("Xor");
	else if(func == exp_XR) return Cstring("xcmd");
	else return Cstring("unknown func");
}

