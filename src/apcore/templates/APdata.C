#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define apDEBUG
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// debug
#include <assert.h>

#ifdef have_xmltol
#include <libxml++/libxml++.h>
#endif /* have_xmltol */

#include "apDEBUG.H"

#ifdef DBGEVENTS
	int event_indent = 0;
	bool inhibitDBGEVENTS = false;
	void eventindent() {
		event_indent++; }
	void eventunindent() {
		event_indent--; }
	void dbgindent() {
		int i = 0;
		if(inhibitDBGEVENTS) return;
		while(i < 2 * event_indent) {
			std::cout << ' ';
			i++; } }
#endif /* DBGEVENTS */

char	assoc_float_string[] = "AssociativeFloat";

using namespace std;

#include "C_string.H"
#include "ActivityInstance.H"
#include "AAFlibrary.H"
#include "APbasic.H"
#include "flexval.H"
#include "AbstractResource.H"

#define ARRAY_STRING_DEFAULT_ARRAY_SIZE	5

static unsigned long array_Size = ARRAY_STRING_DEFAULT_ARRAY_SIZE;

mutex&	streamx::mutx() {
	static mutex M;
	return M;
}

mutex&	coutx::mutx() {
	static mutex M;
	return M;
}

ofstream& coutx::outfile() {
	static ofstream F("apgen.RunLog");
	return F;
}

long&	abs_res_object::currentID() {
	static long 	l = 0L;
	return l;
}

void *&theAAFlibHandle() {
	static void *aaflib = NULL;
	return aaflib;
}

blist&	theAAFActivityFactories() {
	static blist	S(compare_function(compare_bstringnodes, false));
	return S;
}

extern "C" {
#include <concat_util.h>
} // extern "C"

const char* get_apgen_version() {
	return VERSION;
}
 
extern "C" {
	extern char APGEN_build_id[];
}

const char* get_build() {
	return APGEN_build_id;
}

// concatenates all useful info:
const char* get_apgen_version_build_platform() {
	static buf_struct B = {NULL, 0, 0};
	static bool initialized = false;

	if(!initialized) {
		initialized = true;
		concatenate(&B, PACKAGE);
		concatenate(&B, "-");
		concatenate(&B, get_apgen_version());
		concatenate(&B, "-");
		concatenate(&B, get_build());
	}
	return B.buf;
}

			// Handlers for installing core callbacks without dependency loops
// void			(*transfer_act_type_to_stream_handler)(void *, aoString &) = NULL;
CTime_base		(*string_to_CTime_base_handler)(const Cstring&) = NULL;

bool	debug_event_loop = false;

bool&	notify_control() {
	static bool a = false;
	return a;
}

#ifdef DEBUG_LOV
int	ListOVal::curId = 0;
lov_st*	ListOVal::lovState = NULL;
int	ListOVal::lsSize = 0;

void	ListOVal::register_id(int k) {
		if(lsSize <= k) {
			lsSize = 100 + 2 * lsSize;
			if(lovState == NULL) {
				lovState = (lov_st*) malloc(sizeof(lov_st) * lsSize); }
			else {
				lovState = (lov_st*) realloc(lovState, sizeof(lov_st) * lsSize); } }
		lovState[k].n = 0;
		lovState[k].s = 'c'; // created
		}
	ListOVal::unregister_id(int k) {
		lovState[k].s = 'd'; // destroyed
		}
#endif /* DEBUG_LOV */

void		(*Dsource::derivedDataConstructor)(Dsource*, Cstring&) = NULL;
void		(*Dsource::derivedDataDestructor)(Dsource *) = NULL;
void		(*Dsource::eventCatcher)(const char *act_id, const char *param_name) = NULL;


const char *udef_intfc::user_lib_version_string = NULL;

ListOVal&	list_of_all_typedefs() {
	static ListOVal s;
	return s;
}

long &udef_intfc::something_happened() {
	static long foo = 1L;
	return foo;
}

void udef_intfc::set_user_lib_version_to(const char *C) {
	user_lib_version_string = C;
}

const char *udef_intfc::get_user_lib_version() {
	if(user_lib_version_string) {
		return user_lib_version_string;
	}
	return "(not found)";
}

void*	TypedValue::hasher(const char* s) {
	// Don't use STL's hash; it does not exist in RHEL5
	// static hash<string>	H;
	static vector<string>	Vec;
	static map<string, int> Map;
	static long int		l;
	static mutex		mutx3;

	lock_guard<mutex>	lock(mutx3);

	string	st(s);
	map<string, int>::iterator iter = Map.find(st);
	if(iter == Map.end()) {
		Map[st] = l = Vec.size();
		Vec.push_back(st);
	} else {
		l = iter->second;
	}
	return (void*) l;
}

			// should be the same as the value used in simplify(), MADLutil.c:
int			TypedValue::maxLineLength = 180;
			// set by user using -tabs
bool			TypedValue::tabs = false;

TypedValue::TypedValue()
	: type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	value.UN = NULL;
}

TypedValue::TypedValue(apgen::DATA_TYPE dec_type)
	: type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(dec_type) {
	value.UN = NULL;
}

TypedValue::TypedValue(const TypedValue& t)
	: type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(t.Constant),
	  	declared_type(t.declared_type) {
	set_value(t.value, t.type);
}

TypedValue::TypedValue(const value_ptr&	R, apgen::DATA_TYPE t)
	: type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	set_value(R, t);
}

TypedValue::TypedValue(
		const TypedValue& tdn,
		apgen::DATA_TYPE T)
	: type(tdn.type),
		Constant(tdn.Constant),
		declared_type(T) {
	value.UN = NULL;
	if(is_compatible_with_type(tdn.type)) {
		adopt_value_from(tdn);
		cast();
	} else {
		type = apgen::DATA_TYPE::UNINITIALIZED;
	}
}

bool TypedValue::compatible_types(apgen::DATA_TYPE t1, apgen::DATA_TYPE t2) {
	switch(t1) {
		case apgen::DATA_TYPE::ARRAY:
			return t2 == apgen::DATA_TYPE::ARRAY;
		case apgen::DATA_TYPE::STRING:
			return t2 == apgen::DATA_TYPE::STRING;
		case apgen::DATA_TYPE::INSTANCE:
			return t2 == apgen::DATA_TYPE::INSTANCE;
		case apgen::DATA_TYPE::TIME:
			return t2 == apgen::DATA_TYPE::TIME;
		case apgen::DATA_TYPE::DURATION:
			return t2 == apgen::DATA_TYPE::DURATION;
		case apgen::DATA_TYPE::BOOL_TYPE:
			return t2 == apgen::DATA_TYPE::BOOL_TYPE;
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::FLOATING:
			switch(t2) {
				case apgen::DATA_TYPE::INTEGER:
				case apgen::DATA_TYPE::FLOATING:
					return true;
				default:
					return false;
			}
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return false;
	}
	return false;
}

bool TypedValue::is_compatible_with_type(apgen::DATA_TYPE t) const {
	return compatible_types(t, type);
}

// assumes that types are compatible:
void TypedValue::adopt_value_from(const TypedValue& lender) {

	switch(type) {
		case apgen::DATA_TYPE::ARRAY:
			value.LI = lender.value.LI;
			value.LI->ref();
			break;
		case apgen::DATA_TYPE::BOOL_TYPE:
			value.INTEGER = (lender.get_int() != 0L);
			break;
		case apgen::DATA_TYPE::STRING:
			if(lender.type == apgen::DATA_TYPE::STRING) {
				value.CST = new Cstring(*lender.value.CST);
			} else if(lender.type == apgen::DATA_TYPE::INSTANCE) {
				// we need something like this to make it easy to initialize an instance
				value.CST = new Cstring((*(*lender.value.INST))["id"].get_string());
			} else {
				break;
			}
			break;
		case apgen::DATA_TYPE::INSTANCE:
			value.INST = new behaving_element;
			if(lender.type == apgen::DATA_TYPE::INSTANCE) {
				(*value.INST).reference((*lender.value.INST).object());
				}
				break;
			case apgen::DATA_TYPE::INTEGER:
				value.INTEGER = lender.get_int();
				break;
			case apgen::DATA_TYPE::TIME:
			case apgen::DATA_TYPE::DURATION:
				value.TM = new CTime_base(lender.get_time_or_duration());
				break;
			case apgen::DATA_TYPE::FLOATING:
				value.FLOATING = lender.get_double();
				break;
			case apgen::DATA_TYPE::UNINITIALIZED:
			default:
				break;
	}
}

bool TypedValue::is_string() const {
	switch(type) {
		case apgen::DATA_TYPE::STRING:
			return true;
		case apgen::DATA_TYPE::ARRAY:
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return false;
	}
}

bool TypedValue::is_boolean() const {
	switch(type) {
		case apgen::DATA_TYPE::BOOL_TYPE:
			return true;
		case apgen::DATA_TYPE::ARRAY:
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return false;
	}
}

bool TypedValue::is_int() const {
	switch(type) {
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::BOOL_TYPE:
			return true;
		case apgen::DATA_TYPE::ARRAY:
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return false;
	}
}

bool TypedValue::is_duration() const {
	// don't do this: type may be UNINITIALIZED.
	// if(declared_type == apgen::DATA_TYPE::DURATION) {
	// 	return true; }
	switch(type) {
		case apgen::DATA_TYPE::DURATION:
			return true;
		case apgen::DATA_TYPE::TIME:
			// 30 million seconds = approx. 1 year
			// return get_time_or_duration().get_seconds() < 30000000;
		default:
			return false;
	}
}

bool TypedValue::is_time() const {
	switch(type) {
		case apgen::DATA_TYPE::TIME:
			return true;
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::ARRAY:
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return false;
	}
}

bool TypedValue::is_double() const {
	switch(type) {
		case apgen::DATA_TYPE::FLOATING:
			return true;
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::ARRAY:
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return false;
	}
}

bool TypedValue::is_list() const {
	return is_array()
		&& get_array().get_array_type() == TypedValue::arrayType::LIST_STYLE;
}

bool TypedValue::is_struct() const {
	return is_array()
		&& get_array().get_array_type() == TypedValue::arrayType::STRUCT_STYLE;
}

bool TypedValue::is_array() const {
	switch(type) {
		case apgen::DATA_TYPE::ARRAY:
			return true;
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return false;
	}
}

bool TypedValue::is_resource() const {
	switch(type) {
		default:
			return false;
	}
}
			
bool TypedValue::is_instance() const {
	switch(type) {
		case apgen::DATA_TYPE::INSTANCE:
			return true;
		case apgen::DATA_TYPE::ARRAY:
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return false;
	}
}

bool TypedValue::is_numeric() const {
	switch(type) {
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INTEGER:
			return true;
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::ARRAY:
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return false;
	}
}

void TypedValue::set_value(
		const value_ptr& new_val,
		apgen::DATA_TYPE new_type) {
	assert(type == apgen::DATA_TYPE::UNINITIALIZED);

	type = new_type;
	switch(type) {
		case apgen::DATA_TYPE::BOOL_TYPE:
			value.INTEGER = (new_val.INTEGER != 0);
			break;
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
			value.TM = new CTime_base(*new_val.TM);
			break;
		case apgen::DATA_TYPE::FLOATING:
			value.FLOATING = new_val.FLOATING;
			break;
		case apgen::DATA_TYPE::INSTANCE:
			value.INST = new behaving_element();

			//
			// let's be more tolerant:
			//
			// assert((void*)new_val.INST->obj);
			(*value.INST).reference((*new_val.INST).object());
			break;
		case apgen::DATA_TYPE::INTEGER:
			value.INTEGER = new_val.INTEGER;
			break;
		case apgen::DATA_TYPE::STRING:
			value.CST = new Cstring(*new_val.CST);
			break;
		case apgen::DATA_TYPE::ARRAY:

			//
			// SMART copy:
			//
			value.LI = new_val.LI;
			value.LI->ref();
			break;
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			value.UN = NULL;
			break;
	}
}

void TypedValue::set_compatible_value(const value_ptr& new_val) {

	switch(type) {
		case apgen::DATA_TYPE::BOOL_TYPE:
			value.INTEGER = (new_val.INTEGER != 0);
			break;
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
			*value.TM = *new_val.TM;
			break;
		case apgen::DATA_TYPE::FLOATING:
			value.FLOATING = new_val.FLOATING;
			break;
		case apgen::DATA_TYPE::INSTANCE:
			*value.INST = *new_val.INST;
			break;
		case apgen::DATA_TYPE::INTEGER:
			value.INTEGER = new_val.INTEGER;
			break;
		case apgen::DATA_TYPE::STRING:
			*value.CST = *new_val.CST;
			break;
		case apgen::DATA_TYPE::ARRAY:

			//
			// SMART copy. If this is not the
			// owning thread, deref() is
			// a no-op and num_refs() always
			// returns 1, so value.LI is never
			// deleted by a non-owning thread.
			//
			if(!value.LI->deref()) {
				delete value.LI;
			}

			//
			// The owner of this list is this
			// thread. This would be confusing
			// to the original owner if different
			// from this, but a thread is not
			// allowed to consult a value that
			// is changed by another thread.
			//
			value.LI = new_val.LI;
			value.LI->ref();
			break;
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			value.UN = NULL;
			break;
	}
}

void TypedValue::cast() {
	if(declared_type != apgen::DATA_TYPE::UNINITIALIZED) {
		cast(declared_type);
	}
}

/* The following method updates the type of the TypedValue to the
 * requested type if at all possible. */
void TypedValue::cast(apgen::DATA_TYPE new_type) {

    switch(type) {
	case apgen::DATA_TYPE::ARRAY:
	    switch(new_type) {
		case apgen::DATA_TYPE::ARRAY:
		    // Straightforward: trivial cast
		    break;
		case apgen::DATA_TYPE::INSTANCE:
		    {
		    /*
		    * This is a non-trivial case...
		    */
		    ArrayElement*	tds1 = NULL;
		    ArrayElement*	tds2 = NULL;

		    if(value.LI->get_length() != 2) {
			undefine();
			throw(eval_error("cannot cast as instance - type/id info is missing"));
		    }
		    tds1 = (*value.LI)[0];
		    tds2 = (*value.LI)[1];
		    if (	tds1->get_key() == "type"
				&& tds1->Val().is_string()
				&& tds2->get_key() == "id"
				&& tds2->Val().is_string()
		       ) {
			Cstring				instance_type(tds1->Val().get_string());
			Cstring				id(tds2->Val().get_string());
			Cntnr<alpha_string, ActivityInstance*>* act_tag;

			undefine();

			// first try activity instances
			if((act_tag = aaf_intfc::actIDs().find(id))) {
				ActivityInstance*	theActivity;
				type = new_type;
				theActivity = act_tag->payload;
				value.INST = new behaving_element();
				(*value.INST).reference(theActivity->Object.object());
			}

			//
			// no counterpart for abstract instances -
			// cannot save them as objects.
			// They have to be grabbed from the event queues.
			//
		    } else {
			undefine();
			throw(eval_error("cannot cast as instance - type/id info is missing"));
		    }
		    }
		    break;
		default:
		    undefine();
		    throw(eval_error(Cstring("cannot cast array as ") + apgen::spell(new_type)));
		    break;
	    }
	    break;
	case apgen::DATA_TYPE::INSTANCE:
	    if(new_type == apgen::DATA_TYPE::INSTANCE) {
		break;
	    } else if(new_type == apgen::DATA_TYPE::STRING) {
		Cstring temp;
		value_ptr	VP;

		if(value.INST->object()) {
			temp = (*(*value.INST))["id"].get_string();
		} else {
			temp = "generic";
		}
		undefine();
		VP.CST = &temp;
		set_value(VP, new_type);
	    } else {
		Cstring err;
		err << "cannot cast from instance to " << spell(new_type);
		undefine();
		throw(eval_error(err));
	    }
	    break;
	case apgen::DATA_TYPE::TIME:
	    switch(new_type) {
		case apgen::DATA_TYPE::TIME:
		    // already OK
		    // type = new_type;
		    break;
		default:
		    {
		    Cstring err;
		    err << "cannot cast from time to " << spell(new_type);
		    undefine();
		    throw(eval_error(err));
		    }
		    break;
	    }
	    break;
	case apgen::DATA_TYPE::DURATION:
	    switch(new_type) {
		case apgen::DATA_TYPE::DURATION:
		    // already OK
		    // type = new_type;
		    break;
		default:
		    {
		    Cstring err;
		    err << "cannot cast from duration to " << spell(new_type);
		    undefine();
		    throw(eval_error(err));
		    }
		    break;
	    }
	    break;
	case apgen::DATA_TYPE::FLOATING:
	    switch(new_type) {
		case apgen::DATA_TYPE::FLOATING:
		    // already OK
		    break;
		case apgen::DATA_TYPE::INTEGER:
		    {
		    double		D;
		    D = value.FLOATING;
		    type = new_type;
		    value.INTEGER = (long) D;
		    }
		    break;
		default:
		    {
		    Cstring err;
		    err << "cannot cast from float to " << spell(new_type);
		    undefine();
		    throw(eval_error(err));
		    }
		    break;
	    }
	    break;
	case apgen::DATA_TYPE::INTEGER:
	    switch(new_type) {
		case apgen::DATA_TYPE::FLOATING:
		    {
		    long		I;
		    I = value.INTEGER;
		    value.FLOATING = (double) I;
		    type = new_type;
		    }
		    break;
		case apgen::DATA_TYPE::INTEGER:
		    // already OK
		    break;
		default:
		    {
		    Cstring err;
		    err << "cannot cast from integer to " << spell(new_type);
		    undefine();
		    throw(eval_error(err));
		    }
		    break;
	    }
	    break;
	case apgen::DATA_TYPE::BOOL_TYPE:
	    switch(new_type) {
		case apgen::DATA_TYPE::BOOL_TYPE:
		    value.INTEGER = (value.INTEGER != 0);
		    type = new_type;
		    break;
		default:
		    {
		    Cstring err;
		    err << "cannot cast from boolean to " << spell(new_type);
		    undefine();
		    throw(eval_error(err));
		    }
		    break;
	    }
	    break;
	case apgen::DATA_TYPE::STRING:
	    if(new_type == apgen::DATA_TYPE::INSTANCE) {
		undefine();
		type = new_type;
		value.INST = new behaving_element();
	    } else if(new_type != apgen::DATA_TYPE::STRING) {
		Cstring err;
		err << "cannot cast from string to " << spell(new_type);
		undefine();
		throw(eval_error(err));
	    }
	    break;
	// typically, this occurs when a declared variable isn't initialized right away:
	case apgen::DATA_TYPE::UNINITIALIZED:
	    break;
		
	default:
	    value.UN = NULL;
	    break;
    }
}

long TypedValue::get_int() const {
	switch(type) {
		case apgen::DATA_TYPE::FLOATING:
			/* fixes an abominable bug in ACT_req::SetSymbol() -
			 * used to round instead of truncating: */
			return (long) value.FLOATING;
			break;
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::BOOL_TYPE:
			return value.INTEGER;
			break;
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return 0;
	}
}

const CTime_base& TypedValue::get_time_or_duration_ref() const {
	switch(type) {
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
			return *value.TM;
		default:
			throw(eval_error("TypedValue::get_time_or_duration_ref(): value is not a time"));
	}
}

CTime_base TypedValue::get_time_or_duration() const {
	switch(type) {
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
			return *value.TM;
		default:
			throw(eval_error("TypedValue::get_time_or_duration(): value is not a time"));
	}
}

double TypedValue::get_double() const {
	switch(type) {
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
			return value.TM->convert_to_double_use_with_caution();
		case apgen::DATA_TYPE::FLOATING:
			return value.FLOATING;
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::INTEGER:
			return (double) value.INTEGER;
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			return 0.;
	}
}

bool TypedValue::get_bool() const {
  return (bool) get_int();
}

Cstring& TypedValue::get_string() const {
	switch(type) {
		case apgen::DATA_TYPE::STRING:
			return *value.CST;
		case apgen::DATA_TYPE::INSTANCE:
		case apgen::DATA_TYPE::ARRAY:
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			{
			Cstring errstr;
			errstr << "get_string(): data type is "
				<< spell(type);
			throw(eval_error(errstr));
			}
	}
}

void TypedValue::undefine() {

	switch(type) {
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
			if(value.TM) {
				delete value.TM;
			}
			break;
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::FLOATING:
			break;
		case apgen::DATA_TYPE::STRING:
			if(value.CST) {
				delete value.CST;
			}
		    break;
		case apgen::DATA_TYPE::ARRAY:

			//
			// If this is not the owning
			// thread, deref() is a
			// no-op and num_refs() always
			// returns 1, so value.LI is never
			// deleted by a non-owning thread.
			//
			if(value.LI) {
			    if(!value.LI->deref()) {
				delete value.LI;
			    }
			}
			break;
		case apgen::DATA_TYPE::INSTANCE:
			if(value.INST) {
				delete value.INST;
			}
			break;
		case apgen::DATA_TYPE::UNINITIALIZED:
		default:
			break;
	}
	type = apgen::DATA_TYPE::UNINITIALIZED;
	value.UN = NULL;
}

TypedValue::~TypedValue() {
	if(value.UN) undefine();
}
TypedValue::TypedValue(const char* s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(const string& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(const Cstring& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(ListOVal& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(const CTime_base& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(const double& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(const bool& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(const unsigned long& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(const long long& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(const long& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(ActivityInstance& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}
TypedValue::TypedValue(const behaving_element& s) : type(apgen::DATA_TYPE::UNINITIALIZED),
		Constant(false),
		declared_type(apgen::DATA_TYPE::UNINITIALIZED) {
	adopt(s);
}

TypedValue& TypedValue::operator = (const TypedValue& td) {

	//
	// debug
	//
	// if(interesting && td.get_type() == apgen::DATA_TYPE::ARRAY) {
	// 	cerr << "interesting!\n";
	// }

	if(&td == this) {
	       return *this;
	}
	if(Constant) {
		Cstring err;
		err << "TypedValue is constant, cannot set it to a different value.";
		throw(eval_error(err));
	}


	/*
	* Q: does undefine() undefine declared_type?
	* A: no; only the type is undefined. If there is a declared type,
	*    and/or an expected type, they are still there. That is why we
	*    call cast.
	*/
	if(type == td.type) {
		set_compatible_value(td.value);
	} else {
		undefine();
		set_value(td.value, td.type);
		cast();
	}
	// if(is_array()) {
	// 	modifiers = td.modifiers;
	// 	users = td.users;
	// }
	return *this;
}

void TypedValue::adopt(
			const CTime_base& t) {
	value_ptr	VP;
	CTime_base	T(t);

	VP.TM = &T;

	if(type == apgen::DATA_TYPE::DURATION) {
		if(!t.is_duration()) {
			type = apgen::DATA_TYPE::TIME;
		}
		set_compatible_value(VP);
	} else if(type == apgen::DATA_TYPE::TIME) {
		if(t.is_duration()) {
			type = apgen::DATA_TYPE::DURATION;
		}
		set_compatible_value(VP);
	} else {
		apgen::DATA_TYPE new_type;
		undefine();
		if(t.is_duration()) {
			new_type = apgen::DATA_TYPE::DURATION;
		} else {
			new_type = apgen::DATA_TYPE::TIME;
		}
		set_value(VP, new_type);
		cast();
	}
}

void TypedValue::adopt(
			const double& D) {
	if(type != apgen::DATA_TYPE::FLOATING) {
	    undefine();
	    type = apgen::DATA_TYPE::FLOATING;
	    value.FLOATING = D;
	    cast();
	} else {
	    value.FLOATING = D;
	}
}

void TypedValue::adopt(
			const bool& B) {
	if(type != apgen::DATA_TYPE::BOOL_TYPE) {
	    undefine();
	    type = apgen::DATA_TYPE::BOOL_TYPE;
	    value.INTEGER = B;
	    cast();
	} else {
	    value.INTEGER = B;
	}
}

void TypedValue::adopt(
			ActivityInstance& a) {
	if(type != apgen::DATA_TYPE::INSTANCE) {
	    undefine();
	    type = apgen::DATA_TYPE::INSTANCE;
	    value.INST = new behaving_element();
	    (*value.INST) = a.Object;
	    cast();
	} else {
	    if((*value.INST).object() != a.Object.object()) {
		(*value.INST) = a.Object;
	    } // else there is nothing to do
	}
}

void TypedValue::adopt(
			const behaving_element& be) {
	if(type != apgen::DATA_TYPE::INSTANCE) {
		undefine();
		type = apgen::DATA_TYPE::INSTANCE;
		value.INST = new behaving_element();
		(*value.INST) = be;
		cast();
	} else {
		if((*value.INST).object() != be.object()) {
			(*value.INST) = be;
		} // else there is nothing to do
	}
}

void TypedValue::adopt(
			const long& I) {
	if(type != apgen::DATA_TYPE::INSTANCE) {
	    undefine();
	    value.INTEGER = I;
	    type = apgen::DATA_TYPE::INTEGER;
	    cast();
	} else {
	    value.INTEGER = I;
	}
}

void TypedValue::adopt(
			const long long& I) {
	if(type != apgen::DATA_TYPE::INTEGER) {
	    undefine();
	    type = apgen::DATA_TYPE::INTEGER;
	    value.INTEGER = I;
	    cast();
	} else {
	    value.INTEGER = I;
	}
}

void TypedValue::adopt(
			const Cstring& S) {
	if(type != apgen::DATA_TYPE::STRING) {
	    undefine();
	    type = apgen::DATA_TYPE::STRING;
	    value.CST = new Cstring(S);
	    cast();
	} else {
	    if(value.CST) {
		delete value.CST;
	    }
	    value.CST = new Cstring(S);
	}
}

void TypedValue::adopt(
			const char* S) {
	if(type != apgen::DATA_TYPE::STRING) {
	    undefine();
	    type = apgen::DATA_TYPE::STRING;
	    value.CST = new Cstring(S);
	    cast();
	} else {
	    if(value.CST) {
		delete value.CST;
	    }
	    value.CST = new Cstring(S);
	}
}

void TypedValue::adopt(ListOVal& L) {
	if(type != apgen::DATA_TYPE::ARRAY) {
	    undefine();
	    type = apgen::DATA_TYPE::ARRAY;
	    value.LI = &L;
	    value.LI->ref();
	    cast();
	} else {
	    if(value.LI) {
		value.LI->deref();
	    }
	    value.LI = &L;
	    value.LI->ref();
	}
}

void TypedValue::recursively_copy(
		const ListOVal& B) {
	ArrayElement*	tds;
	ListOVal*	L = new ListOVal(B.get_length());
	value_ptr	VP;

	VP.LI = L;
	L->set_array_type(B.get_array_type());
	set_value(VP, apgen::DATA_TYPE::ARRAY);
	for(int z = 0; z < B.get_length(); z++) {
		tds = B.elements[z];
		if(tds->Val().is_array()) {
			ArrayElement*	ae = new ArrayElement(tds->key);

			ae->Val().recursively_copy(tds->Val().get_array());
			L->add(ae);
		} else {
			ArrayElement* t2 = tds->copy();
			L->add(t2);
		}
	}
}

void TypedValue::make_constant() {
    Constant = true;
    if(type == apgen::DATA_TYPE::ARRAY) {
	ArrayElement* ae;
	for(int k = 0; k < get_array().get_length(); k++) {
	    ae = get_array().elements[k];
	    ae->Val().make_constant();
	}
    }
}

//
// The resource pointer is provided so that suitable
// tolerances can be applied in the case of floating-point
// values
//
bool	TypedValue::is_equal_to(
			const TypedValue&	tdn,
			Rsource*		res) const {
    switch(type) {
	case apgen::DATA_TYPE::INTEGER:
	case apgen::DATA_TYPE::BOOL_TYPE:
		if(tdn.is_int()) {
			return get_int() == tdn.get_int();
		} else {
			return false;
		}
	case apgen::DATA_TYPE::DURATION:
	case apgen::DATA_TYPE::TIME:
		if(tdn.is_time() || tdn.is_duration()) {
		    if(res) {
			double	absDelta = 0.0;
			double	relDelta = 0.0;
			bool	has_delta = false;
			TypedValue minAbsSym;
			if(res->properties[(int)Rsource::Property::has_min_abs_delta]) {
			    has_delta = true;
			    minAbsSym = (*res->Object)["min_abs_delta"];
			}
			if(res->properties[(int)Rsource::Property::has_min_rel_delta]) {
			    has_delta = true;
			    TypedValue minRelSym = (*res->Object)["min_rel_delta"];

			    //
			    // AP-1206 - filtering is applied twice, so halve the delta
			    //
			    relDelta = minRelSym.get_double()/2;
			}
			if(has_delta) {
			    CTime_base val1 = get_time_or_duration();
			    CTime_base val2 = tdn.get_time_or_duration();
			    long int v2 = val2.get_pseudo_millisec();
			    long int delta = labs(val1.get_pseudo_millisec() - v2);

			    //
			    // AP-1206 - filtering is applied twice, so halve the delta.
			    // Remember that for a positive duration, pseudo_millisec
			    // is actually a number of milliseconds.
			    //
			    long int absDelta = minAbsSym.get_time_or_duration().get_pseudo_millisec()/2;
			    if(	delta > absDelta
				&& ((double)delta) > relDelta * ((double)labs(v2))) {
				return false;
			    } else {
				return true;
			    }
			}
			// else we fall into the default case below
		    }
		    return get_time_or_duration() == tdn.get_time_or_duration();
		} else {
		    return false;
		}
	case apgen::DATA_TYPE::STRING:
		if(tdn.is_string()) {
			return *value.CST == *tdn.value.CST;
		} else {
			return false;
		}
	case apgen::DATA_TYPE::ARRAY:
		if(tdn.is_array()) {
			ArrayElement	*tds1, *tds2;
			if(value.LI->get_length() != tdn.value.LI->get_length())
				return false;
			int j = 0;
			for(int i = 0; i < value.LI->get_length(); i++) {
				if(!(value.LI->elements[i]->payload.is_equal_to(
					tdn.value.LI->elements[j]->payload)))
					return false;
				if(value.LI->get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
					if(value.LI->elements[i]->get_key()
						!= tdn.value.LI->elements[j]->get_key())
						return false;
				}
				j++;
			}
			return true;
		} else {
			return false;
		}
	case apgen::DATA_TYPE::FLOATING:
		if(tdn.is_double()) {
		    if(res) {
			double	absDelta = 0.0;
			double	relDelta = 0.0;
			bool	has_delta = false;
			TypedValue minAbsSym;
			if(res->properties[(int)Rsource::Property::has_min_abs_delta]) {
			    has_delta = true;
			    minAbsSym = (*res->Object)["min_abs_delta"];
			}
			if(res->properties[(int)Rsource::Property::has_min_rel_delta]) {
			    has_delta = true;
			    TypedValue minRelSym = (*res->Object)["min_rel_delta"];

			    //
			    // AP-1206 - filtering is applied twice, so halve the delta
			    //
			    relDelta = minRelSym.get_double()/2;
			}
			if(has_delta) {
			    double other = tdn.value.FLOATING;
			    double delta = fabs(value.FLOATING - other);

			    //
			    // AP-1206 - filtering is applied twice, so halve the delta
			    //
			    double absDelta = minAbsSym.get_double()/2;
			    if(	delta > absDelta
				&& delta > relDelta * fabs(other)) {
				return false;
			    } else {
				return true;
			    }
			}
			// else we fall into the default case below
		    }
		    double F1, F2; // fractional part
		    double I1, I2; // integral part
		    F1 = modf(value.FLOATING, &I1);
		    F2 = modf(tdn.value.FLOATING, &I2);

		    return I1 == I2 && fabs(value.FLOATING - tdn.value.FLOATING) < 1.0e-15;
		} else {
			return false;
		}
	default:

		//
		// We should probably do better, but checking
		// instances is tricky and the semantics of
		// comparing uninitialized values is murky.
		// For now, we just declare them all as
		// "unqual".
		//
		return false;
    }
    Cstring	errstr("is_equal_to: cannot deal with types ");
    errstr << spell(type) << " and " << spell(tdn.type);
    throw(eval_error(errstr));
}

const char *TypedValue::print_to_stdout() const {
	cout << to_string();
	return "";
}

void TypedValue::print(
		aoString&	aos,
		int		indentation,
		int		float_sd, /* default = MAX_SIGNIFICANT_DIGITS in the header file */
		bool		one_line /* = false */
		) const {
	apgen::DATA_TYPE	type_to_use = type;

	switch(type_to_use) {
		case apgen::DATA_TYPE::BOOL_TYPE:
			aos << (value.INTEGER ? "TRUE" : "FALSE");
			break;
		case apgen::DATA_TYPE::FLOATING:
			//96-08-19 DSG FR102:  use Cstring's formatting smarts
			//  instead; this makes it match other TypedValue::print();
			//  also FR065:  don't limit to default 6 digits
			aos << Cstring(value.FLOATING, float_sd);
			break;
		case apgen::DATA_TYPE::INSTANCE:
			if(*value.INST && value.INST->object()) {
				(*value.INST)->print(aos, "");
			} else {
				aos << "\"null\"";
			}
			break;
		case apgen::DATA_TYPE::INTEGER:
			if(declared_type == apgen::DATA_TYPE::BOOL_TYPE) {
				aos << (value.INTEGER ? "TRUE" : "FALSE");
			} else {
				aos << value.INTEGER;
			}
			break;
		case apgen::DATA_TYPE::STRING:

			//
			// addQuotes escapes characters that need it,
			// including newline characters. The output
			// string is guaranteed not to contain any
			// newlines.
			// 
			aos << addQuotes(*value.CST);
			break;
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::TIME:
			aos << CTime_base(*value.TM).to_string();
			break;
		case apgen::DATA_TYPE::ARRAY:
			// To fix alignment problem in SASF output:
			// print(s, float_sd, /* 4 */ 0);
			{
			ArrayElement	*tds;
			int		i;
			bool		one_at_a_time = false;

			if(indentation == 0) {
				// indentation is only used for continuation lines. We don't want 0.
				indentation = 4;
			}
			// for(i = 0; i < indentation; i++) aos << ' ';
			aos << "[ ";
			tds = value.LI->first_node();
			if(value.LI->get_length() > 10) {
				one_at_a_time = true;
			}
			for(int z = 0; z < value.LI->get_length(); z++) {
				tds = (*value.LI)[z];
				bool will_break = false;

				if(value.LI->get_array_type() == TypedValue::arrayType::LIST_STYLE) {
					if(tds->Val().type == apgen::DATA_TYPE::ARRAY) {
						tds->Val().print(aos, indentation + 4, float_sd, one_line);
					} else {
						tds->Val().print(aos, 0, float_sd, one_line);
					}
				} else {
					aos << addQuotes(tds->get_key());
					aos << " = ";
					if(tds->Val().type == apgen::DATA_TYPE::ARRAY) {
						tds->Val().print(aos, indentation + 4, float_sd, one_line);
					} else {
						tds->Val().print(aos, 0, float_sd, one_line);
					}
				}
				if(tds != value.LI->last_node()) {
					if(!one_line
					   && (one_at_a_time
					       || aos.LineLength > maxLineLength)
					  ) {
						aos << ",\n";
						aos.LineLength = 0;
						for(i = 0; i < indentation; i++) {
							aos << ' ';
						}
					} else if(tabs) {
						aos << ",\t";
					} else {
						aos << ", ";
					}
				}
			}
			aos << " ]";
			}
			break;

		default:
			// aos << "UNKNOWN";
			aos << "\"uninitialized\"";
			break;
	}
}

#ifdef have_xmltol
void	TypedValue::print_to_xmlpp(xmlpp::Element* parent_node) const {
    apgen::DATA_TYPE	type_to_use = type;
    xmlpp::Element*	element;

    if(type == apgen::DATA_TYPE::UNINITIALIZED) {
	element = parent_node->add_child_element("UninitializedValue");
	return;
    }
    if(value_is_a_pointer() && !value.UN) {
	element = parent_node->add_child_element("UninitializedValue");
	return;
    }
    if(declared_type != apgen::DATA_TYPE::UNINITIALIZED) {
	type_to_use = declared_type;
    }
    switch(type_to_use) {
	case apgen::DATA_TYPE::BOOL_TYPE:
		element = parent_node->add_child_element("BooleanValue");
		element->add_child_text(value.INTEGER ? "true" : "false");
		break;
	case apgen::DATA_TYPE::FLOATING:
		element = parent_node->add_child_element("DoubleValue");
		element->add_child_text(*Cstring(value.FLOATING, MAX_SIGNIFICANT_DIGITS));
		break;
	case apgen::DATA_TYPE::INSTANCE:
		element = parent_node->add_child_element("InstanceValue");
		if((*value.INST)) {
			element->add_child_text(*(*(*value.INST))["id"].get_string());
		} else {
			element->add_child_text("null");
		}
		break;
	case apgen::DATA_TYPE::INTEGER:
		element = parent_node->add_child_element("IntegerValue");
		element->add_child_text(*Cstring(value.INTEGER));
		break;
	case apgen::DATA_TYPE::STRING:
		element = parent_node->add_child_element("StringValue");
		element->add_child_text(**value.CST);
		break;
	case apgen::DATA_TYPE::DURATION:
		element = parent_node->add_child_element("DurationValue");
		element->add_child_text(*get_time_or_duration().to_string());
		element->set_attribute("milliseconds", *Cstring(
				1000 * get_time_or_duration().get_seconds()
				+ get_time_or_duration().get_milliseconds()));
		break;
	case apgen::DATA_TYPE::TIME:
		element = parent_node->add_child_element("TimeValue");
		element->add_child_text(*get_time_or_duration().to_string());
		element->set_attribute("milliseconds", *Cstring(
				1000 * get_time_or_duration().get_seconds()
				+ get_time_or_duration().get_milliseconds()));
		break;
	case apgen::DATA_TYPE::ARRAY:
		{
		ArrayElement*	tds;
		int		n, i;
		long		c = 0;

		if(value.LI->get_array_type() == TypedValue::arrayType::LIST_STYLE) {
			element = parent_node->add_child_element("ListValue"); }
		else if(value.LI->get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
			element = parent_node->add_child_element("StructValue"); }
		else if(value.LI->get_array_type() == TypedValue::arrayType::UNDEFINED) {
			element = parent_node->add_child_element("EmptyListValue"); }
		tds = value.LI->first_node();
		for(int z = 0; z < value.LI->get_length(); z++) {
			tds = (*value.LI)[z];
			xmlpp::Element*	subelement;
			if(value.LI->get_array_type() == TypedValue::arrayType::LIST_STYLE) {
				subelement = element->add_child_element("Element");
				subelement->set_attribute("index", *Cstring(c));
				c++;
			} else {
				subelement = element->add_child_element("Element");
				subelement->set_attribute("index", *tds->get_key());
			}
			tds->Val().print_to_xmlpp(subelement);
		}
		}
		break;
	default:
		parent_node->add_child_element("UnknownValue");
		break;
    }
}
#endif /* have_xmltol */

json_object*	TypedValue::print_to_json() const {
    apgen::DATA_TYPE	type_to_use = type;

    if(type == apgen::DATA_TYPE::UNINITIALIZED) {
	return json_object_new_string("invalid");
    }
    if(value_is_a_pointer() && !value.UN) {
	return json_object_new_string("invalid");
    }
    if(declared_type != apgen::DATA_TYPE::UNINITIALIZED) {
	type_to_use = declared_type;
    }
    switch(type_to_use) {
	case apgen::DATA_TYPE::BOOL_TYPE:
	    return json_object_new_int(value.INTEGER);
	case apgen::DATA_TYPE::FLOATING:
	    return json_object_new_double(value.FLOATING);
	case apgen::DATA_TYPE::INSTANCE:
	    if((*value.INST)) {
		return json_object_new_string(*(*(*value.INST))["id"].get_string());
	    } else {
		return json_object_new_string("null");
	    }
	case apgen::DATA_TYPE::INTEGER:
	    return json_object_new_int(value.INTEGER);
	case apgen::DATA_TYPE::STRING:
	    return json_object_new_string(**value.CST);
	case apgen::DATA_TYPE::DURATION:
	case apgen::DATA_TYPE::TIME:
	    return json_object_new_string(*get_time_or_duration().to_string());
	case apgen::DATA_TYPE::ARRAY:
	    {
	    json_object*	array_container = NULL;
	    ArrayElement*	tds;
	    int		n, i;
	    long		c = 0;

	    if(value.LI->get_array_type() == TypedValue::arrayType::LIST_STYLE) {
		array_container = json_object_new_array();
		for(int z = 0; z < value.LI->get_length(); z++) {
		    tds = (*value.LI)[z];
		    json_object_array_add(
			array_container,
			tds->payload.print_to_json());
		}
	    } else if(value.LI->get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
		array_container = json_object_new_object();
		for(int z = 0; z < value.LI->get_length(); z++) {
		    tds = (*value.LI)[z];
		    json_object_object_add(
			array_container,
			*tds->get_key(),
			tds->payload.print_to_json());
		}
	    } else {
		array_container = json_object_new_string("[]");
	    }
	    return array_container;
	    }
	default:
	    return json_object_new_string("invalid");
    }
}

void TypedValue::dump(aoString &s) const {
	if(type == apgen::DATA_TYPE::INSTANCE) {
		s << "instance:\n";
		if((*value.INST)) {
			s << (*value.INST)->Task.full_name() << "\n";
		} else {
			s << "    null\n";
		}
	}
	else
		print(s, 0);
}

bool TypedValue::value_is_a_pointer() const {
	switch(type) {
		case apgen::DATA_TYPE::BOOL_TYPE:
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::FLOATING:
			return false;
		default:
			return true; } }

//96-08-19 DSG FR065:  added significant-digit count (used for FLOAT case only)
void TypedValue::print(Cstring &s, int float_sd) const {
	apgen::DATA_TYPE	type_to_use = type;

	if(type == apgen::DATA_TYPE::UNINITIALIZED) {
		s << "UNKNOWN";
		return;
	}
	if(value_is_a_pointer() && !value.UN) {
		s << "(NULL)";
		return;
	}

	if(declared_type != apgen::DATA_TYPE::UNINITIALIZED)
		type_to_use = declared_type;
	switch(type_to_use) {
		case apgen::DATA_TYPE::BOOL_TYPE:
			s << (value.INTEGER ? "TRUE" : "FALSE");
			break;
		case apgen::DATA_TYPE::INSTANCE:
			if(*value.INST && value.INST->object()) {
				aoString	aos;
				(*value.INST)->print(aos, "");
				s << aos.str();
			} else {
				s << "\"null\"";
			}
			break;
		case apgen::DATA_TYPE::INTEGER:
			s << value.INTEGER;
			break;
		case apgen::DATA_TYPE::FLOATING:
			//96-08-19 DSG FR065:  don't limit to default 6 digits
			//
			// Added by PFM to fix the ambiguity between
			// 0 and -0 in TOL output:
			//
			if(fabs(value.FLOATING) < 1.0E-15) {
				s << "0.0";
			} else {
				s << Cstring(value.FLOATING, float_sd);
			}
			break;
		case apgen::DATA_TYPE::STRING:
			s << addQuotes(*value.CST);
			break;
		case apgen::DATA_TYPE::DURATION:
			s << CTime_base(*value.TM).to_string();
			break;
		case apgen::DATA_TYPE::TIME:
			s << CTime_base(*value.TM).to_string();
			break;
		case apgen::DATA_TYPE::ARRAY:
			s << '[';
			{
			ArrayElement*	tds;

			for(int z = 0; z < value.LI->get_length(); z++) {
				tds = value.LI->elements[z];
				if(value.LI->get_array_type() == TypedValue::arrayType::LIST_STYLE) {
					tds->Val().print(s, float_sd);
				} else {
					s << addQuotes(tds->get_key());
					s << " = ";
					tds->Val().print(s, float_sd);
				}
				if(tds != value.LI->last_node()) {
					s << ", ";
				}
			}
			}
			s << ']';
			break;
		default:
			s << "UNKNOWN";
			break;
	}
}

void TypedValue::clear() {
    if(type == apgen::DATA_TYPE::ARRAY) {

	//
	// A non-owning thread should never clear an array
	//
	assert(thread_index == value.LI->owner);

	if(thread_index == value.LI->owner) {
	    if(value.LI->num_refs() > 1) {
		value.LI->deref();
		value.LI = new ListOVal;
		value.LI->ref();
	    } else {
		value.LI->clear();
	    }
	    value.LI->set_array_type(TypedValue::arrayType::UNDEFINED);
	}
    }
}

TypedValue& TypedValue::add(const ListOVal& rhs) {

	ArrayElement*	tds1;
	ArrayElement*	tds2;

	if(type == apgen::DATA_TYPE::UNINITIALIZED) {
		value_ptr	VP;

		VP.LI = new ListOVal;
		set_value(VP, apgen::DATA_TYPE::ARRAY);
	}
	if(type != apgen::DATA_TYPE::ARRAY) {
		Cstring err;
		err << "add error: this is not an array. ";
		throw(eval_error(err));
	}
	if(value.LI->get_array_type() == TypedValue::arrayType::LIST_STYLE
	   && rhs.get_array_type() == TypedValue::arrayType::LIST_STYLE) {
		for(int i = 0; i < rhs.get_length(); i++) {
			value.LI->add(new ArrayElement(*rhs.elements[i]));
		}
	} else if(value.LI->get_array_type() == TypedValue::arrayType::LIST_STYLE
		  || rhs.get_array_type() == TypedValue::arrayType::LIST_STYLE) {
		Cstring err;
		err << "merge error: right and left arrays must use the same indexing style. ";
		throw(eval_error(err));
	} else {
		for(int i = 0; i < rhs.get_length(); i++) {
			tds1 = rhs.elements[i];
			if(!(tds2 = value.LI->find(tds1->get_key()))) {

				//
				// IMPORTANT: the old code would just insert tds1 into value.LI.
				// But this did not work correctly! One must be really careful with
				// ArrayElements, because of the implicit associativity between
				// integer and string indices for each node in the list.
				//
				// The base methods have not been properly overloaded to include
				// ArrayElements. As a result, each method must be evaluated on
				// its own merits... this is not good design, but as usual when
				// that is the case, it's the result of a compromise between
				// elegance, shortness of time and execution speed.
				//
				value.LI->add(new ArrayElement(*tds1));
			} else {
				tds2->SetVal(tds1->Val());
			}
		}
	}
	return *this;
}

void TypedValue::merge(const ListOVal* lov) {

	ArrayElement*	tds1;
	ArrayElement*	tds2;

	if(type == apgen::DATA_TYPE::UNINITIALIZED) {
		value_ptr	VP;

		VP.LI = new ListOVal;
		set_value(VP, apgen::DATA_TYPE::ARRAY); }
	if(type != apgen::DATA_TYPE::ARRAY) {
		Cstring err;
		err << "merge error: lhs is not an array. ";
		throw(eval_error(err));
	}
	if(value.LI->get_array_type() == TypedValue::arrayType::LIST_STYLE
	   && lov->get_array_type() == TypedValue::arrayType::LIST_STYLE) {
		for(int i = 0; i < lov->get_length(); i++) {
			value.LI->add(new ArrayElement(*lov->elements[i]));
		}
	} else if(value.LI->get_array_type() == TypedValue::arrayType::LIST_STYLE
		  || lov->get_array_type() == TypedValue::arrayType::LIST_STYLE) {
		Cstring err;
		err << "merge error: right and left arrays must use the same indexing style. ";
		throw(eval_error(err));
	} else {
		for(int i = 0; i < lov->get_length(); i++) {
			tds1 = lov->elements[i];
			if(!(tds2 = value.LI->find(tds1->get_key()))) {
				/*
				 * IMPORTANT: the old code would just insert tds1 into value.LI.
				 * But this did not work correctly! One must be really careful with
				 * ArrayElements, because of the implicit associativity between
				 * integer and string indices for each node in the list.
				 *
				 * The base methods have not been properly overloaded to include
				 * ArrayElements. As a result, each method must be evaluated on
				 * its own merits... this is not good design, but as usual when
				 * that is the case, it's the result of a compromise between
				 * elegance, shortness of time and execution speed.
				 */
				value.LI->add(new ArrayElement(*tds1));
			} else {
				tds2->SetVal(tds1->Val());
			}
		}
	}
}

behaving_element& TypedValue::get_object() const {
	if(!is_instance()) {
		throw(eval_error("get_object(): not an instance"));
	}
	return *value.INST;
}

//
// In this method, the typed value is the actual value of a
// certain activity instance parameter. The flexval F is
// intended to gather information about that parameter that
// is generic, i. e., not specific to this particular activity
// instance. It is expected that all instances are compatible,
// i. e., that arrays and sub-arrays have the same 'shape'
// (are defined by the same C-style struct).
//
// Initially, F is empty; it needs to be documented as much
// as possible.
//
void ListOVal::document_arrays_in(flexval& F) {
    // assert(type == apgen::DATA_TYPE::ARRAY);
    // ListOVal&		array = get_array();
    // assert(array.get_length() > 0);
    assert(get_length() > 0);
    TypedValue::arrayType array_type = get_array_type();
    assert(array_type != TypedValue::arrayType::UNDEFINED);

    //
    // The flexval needs to be set to a structure that contains
    // one of the following:
    //
    // 	- a string S if the values stored in the array
    // 	  being documented are all non-array objects;
    // 	  S will be either "list" or "struct".
    //
    // 	- a list of one element E if the array being
    // 	  documented is a list of arrays; E will document
    // 	  those arrays. Note that a list is always
    // 	  homogeneous, so a single element suffices. By
    // 	  the way, homogeneity is not mandated by APGenX;
    // 	  it's a feature of the adaptation.
    //
    // 	- a struct of N elements E_1, E_2, ..., E_N if
    // 	  the array being documented is a struct that
    // 	  contains N arrays. The key of E_i will be the
    // 	  key of the i-th array in the struct, and the
    // 	  value of E_i will document that array.
    //
    // If some of the nodes in F have already been documented,
    // we must be careful not to overwrite what has already
    // been collected; it may be that different activity
    // instances can be used to document different members
    // of a given struct.
    //
    ArrayElement*	ae;

    if(array_type == TypedValue::arrayType::LIST_STYLE) {
	ae = first_node();
	TypedValue& element = ae->Val();
	if(element.get_type() != apgen::DATA_TYPE::ARRAY) {
	    F[0] = string(*apgen::spell(element.get_type()));
	    return;
	} else {
	    flexval f;

	    //
	    // Let's make sure f already has the information
	    // previously collected, if any:
	    //
	    if(F.getType() != flexval::TypeInvalid) {
		assert(F.getType() == flexval::TypeArray);
		f = F[0];
	    }

	    ListOVal& subarray = element.get_array();
	    if(subarray.get_array_type() == TypedValue::arrayType::LIST_STYLE) {
		if(f.getType() != flexval::TypeInvalid) {
		    assert(f.getType() == flexval::TypeArray);
		}
		subarray.first_node()->Val().get_array().document_arrays_in(f);
		if(f.getType() != flexval::TypeInvalid) {
		    F[0] = f;
		}
		return;
	    } else if(subarray.get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
		if(f.getType() != flexval::TypeInvalid) {
		    assert(f.getType() == flexval::TypeStruct);
		}
		ArrayElement* ae2;
		for(int k = 0; k < subarray.get_length(); k++) {
		    ae2 = subarray[k];
		    if(ae2->Val().get_type() == apgen::DATA_TYPE::ARRAY
		       && ae2->Val().get_array().get_array_type() !=
			TypedValue::arrayType::UNDEFINED) {
			flexval g;
			ae2->Val().get_array().document_arrays_in(g);
			if(g.getType() != flexval::TypeInvalid) {
			    f[*ae2->get_key()] = g;
			}
		    }
		}
		if(f.getType() != flexval::TypeInvalid) {
		    F[0] = f;
		}
		return;
	    } else {

		//
		// the array is empty
		//
		return;
	    }
	}
    } else {

	//
	// For each element in the actual array, we need to document
	// that element in F
	//
	for(int k = 0; k < get_length(); k++) {

	    ae = operator[](k);
	    if(ae->Val().get_type() != apgen::DATA_TYPE::ARRAY) {
		F[*ae->get_key()] = string(*apgen::spell(ae->Val().get_type()));

		//
		// Don't return yet - there may be many other elements
		//
	    } else {

		//
		// f will document this particular array element
		//
		flexval f;

		//
		// Let's make sure f already has the information
		// previously collected, if any:
		//
		if(F.getType() != flexval::TypeInvalid) {
		    assert(F.getType() == flexval::TypeStruct);
		    map<string, flexval>& M = F.get_struct();
		    if(M.find(*ae->get_key()) == M.end()) {
			F[*ae->get_key()] = f;
		    } else {
			f = F[*ae->get_key()];
		    }
		}

		ListOVal& subarray = ae->Val().get_array();
		if(subarray.get_array_type() == TypedValue::arrayType::LIST_STYLE) {
		    if(f.getType() != flexval::TypeInvalid) {
			assert(f.getType() == flexval::TypeArray);
		    }
		    if(subarray.first_node()->Val().get_type() != apgen::DATA_TYPE::ARRAY) {
			f[0] = string(*apgen::spell(subarray.first_node()->Val().get_type()));
		    } else if(subarray.first_node()->Val().get_array().get_array_type()
				!= TypedValue::arrayType::UNDEFINED) {
			subarray.first_node()->Val().get_array().document_arrays_in(f);
		    }
		    if(f.getType() != flexval::TypeInvalid) {
			F[*ae->get_key()] = f;
		    }
		} else if(subarray.get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
		    if(f.getType() != flexval::TypeInvalid) {
			assert(f.getType() == flexval::TypeStruct);
		    }
		    ArrayElement* ae2;
		    for(int k = 0; k < subarray.get_length(); k++) {
			ae2 = subarray[k];
			if(ae2->Val().get_type() == apgen::DATA_TYPE::ARRAY) {
			    if(    ae2->Val().get_array().get_array_type() !=
					TypedValue::arrayType::UNDEFINED) {
				flexval g;
				ae2->Val().get_array().document_arrays_in(g);
				if(g.getType() != flexval::TypeInvalid) {
				    f[*ae2->get_key()] = g;
				}
			    }
			} else {
			    f[*ae2->get_key()] = string(*apgen::spell(ae2->Val().get_type()));
			}
		    }
		    if(f.getType() != flexval::TypeInvalid) {
			F[*ae->get_key()] = f;
		    }

		    //
		    // don't return yet
		    //

		} else {

		    //
		    // the subarray is empty
		    //

		} // if subarray is LIST_STYLE / STRUCT_STYLE / EMPTY
	    } // if this element is / is not an array
	} // for each item in the parameter array
    } // if the parameter array is LIST_STYLE / STRUCT_STYLE
}


Cstring addQuotes(const Cstring& s) {
	char		* t;
	int		i;
	int		j = -1;
	int		quote_count = 0;
	int		l = s.length();
	Cstring		S;


	for(i = 0; i < l; i++) {

		//
		// For every character that needs to
		// be escaped, add one to quote_count
		// to account for the escape character:
		//
		if(s[i] == '\"') {
			quote_count++;
		} else if(s[i] == '\\') {
			quote_count++;
		} else if(s[i] == '\n') {

#			ifdef REMOVE_THIS_BECAUSE_IT_MAKES_TOLs_HARD_TO_READ
			quote_count += 3;
#			endif /* REMOVE_THIS_BECAUSE_IT_MAKES_TOLs_HARD_TO_READ */

			quote_count++;
		}
	}

	t = (char *) malloc(s.length() + quote_count + 3);
	t[++j] = '\"';
	i = 0;
	while (s[i] != '\0') {
		if (s[i] == '\"') {
			t[++j] = '\\';
			t[++j] = '\"';
		} else if (s[i] == '\\') {
			t[++j] = '\\';
			t[++j] = '\\';
		} else if(s[i] == '\n') {
			t[++j] = '\\';
			t[++j] = 'n';

#			ifdef REMOVE_THIS_BECAUSE_IT_MAKES_TOLs_HARD_TO_READ
			// add an escaped newline for readability:
			t[++j] = '\\';
			t[++j] = '\n';
#			endif /* REMOVE_THIS_BECAUSE_IT_MAKES_TOLs_HARD_TO_READ */

		} else {
			t[++j] = s[i];
		}
		i++;
	}
	t[++j] = '\"';
	t[++j] = '\0';
	S = t;
	free((char *) t);
	return S;
}


void removeQuotes (Cstring & s) {
	char	* t = (char *) malloc(s.length() + 1) ;
	int	i = -1;
	int	j = -1;
	int	state = 0 ;

	do {
		++i;
		if(state == 0) {
			if(s[i] == '"')
				;
			else if(s[i] == '\\')
				state = 1;	// escape state
			else
				t[++j] = s[i];
		} else if(state == 1) {
			// escape state
			if(s[i] == '\"') t[++j] = '\"';
			else if(s[i] == 'n') t[++j] = '\n';
			else if(s[i] == 't') t[++j] = '\t';
			else if(s[i] == '\n');
			else t[++j] = s[i];
			state = 0;
		}
	} while (s[i] != '\0');
	// This step is redundant because we already processed the terminating '\0'!!
	t[j] = '\0';
	s = Cstring(t);
	free(t);
}

// Routines to convert from DATA_TYPE to Cstring and visa versa.
Cstring apgen::spell(apgen::DATA_TYPE type) {
	if(	   type == apgen::DATA_TYPE::ARRAY	) return "array";
	else	if(type == apgen::DATA_TYPE::BOOL_TYPE	) return "boolean";
	else	if(type == apgen::DATA_TYPE::DURATION	) return "duration";
	else	if(type == apgen::DATA_TYPE::FLOATING	) return "float";
	else	if(type == apgen::DATA_TYPE::INSTANCE	) return "instance";
	else	if(type == apgen::DATA_TYPE::INTEGER	) return "integer";
	else	if(type == apgen::DATA_TYPE::STRING	) return "string";
	else	if(type == apgen::DATA_TYPE::TIME	) return "time";
	else	if(type == apgen::DATA_TYPE::UNINITIALIZED) return "uninitialized";
	else				  		  return "unknown";
}

// Routines to convert from DATA_TYPE to Cstring and visa versa.
Cstring apgen::spellC(apgen::DATA_TYPE type) {
	if(	   type == apgen::DATA_TYPE::ARRAY	) return "array_ptr";
	else	if(type == apgen::DATA_TYPE::BOOL_TYPE	) return "bool";
	else	if(type == apgen::DATA_TYPE::DURATION	) return "CTime_base";
	else	if(type == apgen::DATA_TYPE::FLOATING	) return "double";
	else	if(type == apgen::DATA_TYPE::INSTANCE	) return "smart_actptr";
	else	if(type == apgen::DATA_TYPE::INTEGER	) return "long int";
	else	if(type == apgen::DATA_TYPE::STRING	) return "Cstring";
	else	if(type == apgen::DATA_TYPE::TIME       ) return "CTime_base";
	else	if(type == apgen::DATA_TYPE::UNINITIALIZED) return "void";
	else				  		  return "UNKNOWN";
}

apgen::DATA_TYPE string_to_data_type(const char* str) {
	if(	 !strcmp(str, "array"	))	return apgen::DATA_TYPE::ARRAY;
	else if(!strcmp(str, "boolean"	))	return apgen::DATA_TYPE::BOOL_TYPE;
	else if(!strcmp(str, "duration"))	return apgen::DATA_TYPE::DURATION;
	else if(!strcmp(str, "float"	))	return apgen::DATA_TYPE::FLOATING;
	else if(!strcmp(str, "instance"))	return apgen::DATA_TYPE::INSTANCE;
	else if(!strcmp(str, "integer"	))	return apgen::DATA_TYPE::INTEGER;
	else if(!strcmp(str, "string"	))	return apgen::DATA_TYPE::STRING;
	else if(!strcmp(str, "time"	))	return apgen::DATA_TYPE::TIME;
	else if(!strcmp(str,"uninitialized"))return apgen::DATA_TYPE::UNINITIALIZED;
	else return apgen::DATA_TYPE::UNINITIALIZED;
}

//
// spelling method types:
//
Cstring apgen::spell(apgen::METHOD_TYPE type) {
	switch(type) {
		case apgen::METHOD_TYPE::ATTRIBUTES:
			return Cstring("attributes");
		case apgen::METHOD_TYPE::CONCUREXP:
			return Cstring("scheduling");
		case apgen::METHOD_TYPE::CREATION:
			return Cstring("creation");
		case apgen::METHOD_TYPE::DECOMPOSITION:
			return Cstring("decomposition");
		case apgen::METHOD_TYPE::DESTRUCTION:
			return Cstring("destruction");
		case apgen::METHOD_TYPE::FUNCTION:
			return Cstring("function");
		case apgen::METHOD_TYPE::MODELING:
			return Cstring("modeling");
		case apgen::METHOD_TYPE::NONE:
			return Cstring("none");
		case apgen::METHOD_TYPE::NONEXCLDECOMP:
			return Cstring("nonexclusive decomposition");
		case apgen::METHOD_TYPE::PARAMETERS:
			return Cstring("parameters");
		case apgen::METHOD_TYPE::PROFILE:
			return Cstring("profile");
		case apgen::METHOD_TYPE::RESUSAGE:
			return Cstring("resource usage");
		case apgen::METHOD_TYPE::STATES:
			return Cstring("states");
		case apgen::METHOD_TYPE::USAGE:
			return Cstring("usage");
		default:
			break;
	}
	return Cstring("none");
}


void TypedValue::generate_default_for_type(
		const Cstring& theType) {
	ArrayElement* tds;
	if(	(theType == "structure")
		||  (theType == "list")
		||  (theType == "array")
	  ) {
		generate_default_for_type(apgen::DATA_TYPE::ARRAY);
	} else if(theType == "float") {
		generate_default_for_type(apgen::DATA_TYPE::FLOATING);
	} else if(theType == "integer") {
		generate_default_for_type(apgen::DATA_TYPE::INTEGER);
	} else if(theType == "boolean") {
		generate_default_for_type(apgen::DATA_TYPE::BOOL_TYPE);
	} else if(theType == "string") {
		generate_default_for_type(apgen::DATA_TYPE::STRING);
	} else if(theType == "time") {
		generate_default_for_type(apgen::DATA_TYPE::TIME);
	} else if(theType == "duration") {
		generate_default_for_type(apgen::DATA_TYPE::DURATION);
	} else if(theType == "instance") {
		generate_default_for_type(apgen::DATA_TYPE::INSTANCE);
	} else if((tds = list_of_all_typedefs().find(theType))) {
		recursively_copy(tds->Val().get_array());
		type = apgen::DATA_TYPE::ARRAY;
	}
}

void TypedValue::generate_default_for_type(
		apgen::DATA_TYPE theType) {
	undefine();
	if(theType == apgen::DATA_TYPE::ARRAY) {
		ListOVal* lov = new ListOVal;
		adopt(*lov);
	} else if(theType == apgen::DATA_TYPE::FLOATING) {
		adopt(0.0);
	} else if(theType == apgen::DATA_TYPE::INTEGER) {
		adopt(0L);
	} else if(theType == apgen::DATA_TYPE::BOOL_TYPE) {
		adopt(false);
	} else if(theType == apgen::DATA_TYPE::STRING) {
		adopt("");
	} else if(theType == apgen::DATA_TYPE::TIME) {
		adopt(CTime_base());
	} else if(theType == apgen::DATA_TYPE::DURATION) {
		adopt(CTime_base("00:00:00"));
	} else if(theType == apgen::DATA_TYPE::INSTANCE) {
		adopt(behaving_element());
	}
}

void TypedValue::extract_array_info(
			int			indent,
			const Cstring&		prefix,
			map<string, string>&	names,
			map<string, string>&	types,
			map<string, list<string> >& ranges) const {
	// we already know that the TypedValue is an array
	assert(type == apgen::DATA_TYPE::ARRAY);
	ArrayElement*					ae;
	int						elcount = 0;
	char						buf[80];
	Cstring						newprefix;
	int						newindent = indent;
	int						i;

	// do we need to do this? ... yes.
	if(get_array().get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
		types[*prefix] = "struct";
	} else {
		types[*prefix] = "list";
	}

	if(indent >= 0) {
		newindent = indent + 4;
		for(i = 0; i < indent; i++) {
			cerr << " ";
		}
		cerr << "types[" << prefix << "] = " << types[*prefix] << "\n";
	}

	for(int k = 0; k < get_array().get_length(); k++) {
		ae = get_array().elements[k];

		//
		// increment the prefix - remember, this is to support the GTK editor
		//
		sprintf(buf, ":%d", elcount);
		newprefix = prefix + buf;
		names[*newprefix] = *ae->get_key();
		
		if(indent >= 0) {
			for(i = 0; i < indent; i++) {
				cerr << " ";
			}
			cerr << "element " << ae->get_key() << ":\n";
		}
		if(ae->Val().get_type() == apgen::DATA_TYPE::ARRAY) {
			ae->Val().extract_array_info(newindent, newprefix, names, types, ranges);
		} else {
			if(indent >= 0) {
				for(i = 0; i < indent; i++) {
					cerr << " ";
				}
				cerr << "types[" << newprefix << "] = " <<
						spell(ae->Val().get_type()) << "\n";
			}
			types[*newprefix] = string(*spell(ae->Val().get_type()));
		}
		if(ae->Val().get_type() == apgen::DATA_TYPE::BOOL_TYPE) {
			list<string> L;
			L.push_back("TRUE");
			L.push_back("FALSE");
			ranges[*newprefix] = L;
		}
		elcount++;
	}
}

blist&	Dsource::theLegends() {
	static blist B(compare_function(compare_bstringnodes, false));
	return B;
}

// List_iterator *Dsource::getLegendListIterator() {
// 	return new List_iterator(theLegends());
// }

TypedValue	TypedValue::flexval2typedval(flexval& F) {
	flexval::Type		ftype(F.getType());
	TypedValue		val;
	if(ftype == flexval::TypeInvalid) {
		return val;
	}
	switch(ftype) {
		case flexval::TypeBool:
			val = (const bool) F;
			break;
		case flexval::TypeDouble:
			val = (const double) F;
			break;
		case flexval::TypeInt:
			val = (const long int) F;
			break;
		case flexval::TypeString:
			val = ((string) F).c_str();
			break;
		case flexval::TypeTime:
			{
			cppTime	T(F);
			val = T.T;
			}
			break;
		case flexval::TypeDuration:
			{
			cppDuration	T(F);
			val = T.T;
			}
			break;
		case flexval::TypeArray:
			{
			ListOVal*	lov = new ListOVal;
			int		i = 0;

			flexval::ValueArray::iterator iter;
			for(	iter = F.get_array().begin();
				iter != F.get_array().end();
				iter++
			   ) {
				ArrayElement*	ae = new ArrayElement(i);
				ae->SetVal(flexval2typedval(*iter));
				lov->add(ae);
				i++;
			}
			val = *lov;
			}
			break;
		case flexval::TypeStruct:
			{
			ListOVal*	lov = new ListOVal;

			flexval::ValueStruct::iterator iter;
			for(	iter = F.get_struct().begin();
				iter != F.get_struct().end();
				iter++
			   ) {
				ArrayElement*	ae = new ArrayElement(Cstring(iter->first.c_str()));
				ae->SetVal(flexval2typedval(iter->second));
				lov->add(ae);
			}
			}
			break;
		default:
			break;
	}
	return val;
}

//
// To support abstract resource methods (e. g. 'use'):
//
abs_res_object::abs_res_object(
		const Behavior&		Type,		// the type that defines the abs. res.
		int			task_index,	// the index of the task (1 for 'use')
		behaving_object*	parent_scope)	// where to look for variables if not local
	 	: behaving_object(*Type.tasks[task_index], parent_scope),
		    parent_constructor_obj(parent_scope) {

	//
	// Note: parent_scope is the object that supports the
	// abstract resource constructor.
	//
	assert(parent_scope);
}

//
// To support the abstract resource constructor, which
// contains class variables defined in the adaptation:
//
abs_res_object::abs_res_object(
		task&			T,
		behaving_element&	parent_context)	// the object that called us via 'use Foo(...)'
	 	: behaving_object(T) {
	//
	// Note that attributes are not defined for abstract
	// resources; this should probably be remedied.
	//
	Cstring	unique_id = Cstring("abstract-res::") + Type.name;
	unique_id << "_" << currentID()++;
	operator[](ID) = unique_id;
	operator[](TYPE) = Cstring("abstract-res::") + Type.name;
	operator[](PARENT) = parent_context;
	operator[](ENABLED) = true;
}

// accessor for description
TypedValue	abs_res_object::get_descr(
			const Cstring&,
			bool& found) const {
	aoString	s;
	Cstring		st;
	TypedValue	V;
	s << "<";
	if(print(s, "")) {
		s << ">";
	} else {
		s << "unknown object>";
	}
	st = s.str();
	V = nonewlines(st);
	found = true;
	return V;
}
