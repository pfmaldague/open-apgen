#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <unistd.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <memory>
#include <stdlib.h>


#include "AP_exp_eval.H"
#include "APdata.H"
#include "C_string.H"
#include "C_list.H"
#include "RES_eval.H"
#include "apDEBUG.H"

#include "ActivityInstance.H"
#include "spiceIntfc.H"

#define ARG_POINTER args.size()
#define ARG_POP args.pop_front()
#define ARG_STACK slst<TypedValue*>& args
#define WHERE_TO_STOP 0

#define list_array_type 1
#define struct_array_type 2
#define SET_SYMBOL( x , y , z ) x->set_typed_val(y);

using namespace apgen;

#define  MAXWIN  10000

extern void register_spice_subsystem_functions();
extern void last_call_to_spice();
extern void register_vector_subsystem_functions();
extern void register_file_subsystem_functions();
extern void clear_spice_cache();
extern void clean_up_spice_cache();

extern void	removeQuotes( Cstring & s ) ;
extern void	add_to_Func_list( const char * , generic_function P ) ;

extern char      *get_ET_time(double time);
extern char      *get_Current_UTC_time();
extern char      *ConvertMPFTime( char *MPFTime );


apgen::RETURN_STATUS
get_double_array(Cstring& errs, const TypedValue& given, double returned[], int returned_size) {
	const ListOVal&					els(given.get_array());
	ArrayElement*					ae;

	if(els.get_length() > returned_size) {
		errs << "get_double_array error: given array has " << els.get_length()
			<< " elements, while returned array size is " << returned_size << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	for(int index = 0; index < els.get_length(); index++) {
		ae = els[index];
		if(!ae->Val().is_numeric()) {
			errs << "get_double_array error: expected given array to contain floats\n";
			return apgen::RETURN_STATUS::FAIL;
		}
		returned[index] = ae->Val().get_double();
	}
	return apgen::RETURN_STATUS::SUCCESS;
}


using namespace std;

double get_pos( int i );
double Time_to_ET( double time );

TypedValue
create_float_list(int n, double* d) {
	ListOVal* L = new ListOVal;

	for(int i = 0; i < n; i++) {
		L->add(i, d[i]);
	}
	TypedValue aValue(*L);
	return aValue;
}

TypedValue
create_float_struct(int n, const char* indices[], double* d) {
	ListOVal* L = new ListOVal;

	for(int i = 0; i < n; i++) {
		L->add(indices[i], d[i]);
	}
	TypedValue aValue(*L);
	return aValue;
}


RETURN_STATUS
keyset(Cstring & errs , TypedValue* result, ARG_STACK ) {
	TypedValue*	n1;
	ListOVal*	L = new ListOVal;

	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
	        errs << "  wrong number of parameters passed to user-defined function keyset; expected 1, got "
		    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n1 = ARG_POP ;
	if(! n1->is_array()) {
	        errs << "keyset: first argument is not a struct\n" ;
	        return RETURN_STATUS::FAIL ;
	}
	ListOVal& passedInStruct = n1->get_array();
	for(int i = 0; i < passedInStruct.get_length(); i++) {
	    L->add(i, passedInStruct[i]->get_key());
	}
	*result = *L;
	return RETURN_STATUS::SUCCESS;
}

/* Returns a sub-map of the input map.
 * inMap = [obj1 = ["size" = "1", "color" = "red"], obj2 = ["size = "1", "color" = "blue"]]
 * subset(inMap,"size","1") -> [obj1 = ["size" = "1", "color" = "red"], obj2 = ["size = "1", "color" = "blue"]]
 * subset(inMap,"color","red") -> [obj1 = ["size" = "1", "color" = "red"]]
 */
RETURN_STATUS
subset(Cstring & errs , TypedValue* result, ARG_STACK ) {
	TypedValue	* n1, // "array"
	                * n2, // "key"
	                * n3; // "value"
	TypedValue	propertyMap;
	ListOVal*	L = new ListOVal;
	ArrayElement*	tds;

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 3 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function subset; expected 3, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(! n1->is_array() ){
		errs << "subset: first argument is not an array\n" ;
		return RETURN_STATUS::FAIL ;
	}
	else if(! n2->is_string() ){
		errs << "subset: second argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
	}
	else if( ! n3->is_string() ){
		errs << "subset: third argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
	}
	ListOVal& passedInStruct = n1->get_array();
	for(int i = 0; i < passedInStruct.get_length(); i++) {
		tds = passedInStruct[i];
		propertyMap = tds->Val();
		//cout << propertyMap.get_array()[n2->get_string()]->Val().get_string() << "\n";
		//cout <<  n3->get_string() << "\n";
		if(propertyMap.is_array() && propertyMap.get_array()[n2->get_string()]->Val().get_string()
				== n3->get_string()) {
			L->add(tds->get_key(), tds->Val());
		}
		else if(!propertyMap.is_array()){
			errs << "subset: one of the top-level values is not itself a map of properties to values\n" ;
			return RETURN_STATUS::FAIL ;
		}
	}

	*result = *L;
	return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS getCurrentTimeInMilliseconds ( Cstring & errs , TypedValue * result , ARG_STACK ) {
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
	errs << "  wrong number of parameters passed to user-defined "
	    << "|function getCurrentTimeInMilliseconds; expected 0, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    //printf("milliseconds: %lld\n", milliseconds);
    *result = (double)milliseconds ;
    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS   formatCPCM( Cstring & errs , TypedValue *result , ARG_STACK ) {
        TypedValue      *n1 ; // "float"
	char tempString[80];
        errs.undefine() ;
        if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
                errs << "  wrong number of parameters passed to user-defined function formatCPCM; expected 1, got "
                        << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
                return RETURN_STATUS::FAIL ;
        }
        n1 = ARG_POP ;
        if( n1->get_type() != DATA_TYPE::FLOATING ) {
                errs << "formatCPCM: argument is not a float\n" ;
                return RETURN_STATUS::FAIL ;
        }
	(void)sprintf(tempString,"%d_%1.1d",(int)n1->get_double(),(int)(10.0*(n1->get_double()-(int)n1->get_double())));
        *result = tempString ;
        return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS   formatCart( Cstring & errs , TypedValue *result , ARG_STACK ) {
        TypedValue      *n1 ; // "float"
	char tempString[80];
        errs.undefine() ;
        if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
                errs << "  wrong number of parameters passed to user-defined function formatCart; expected 1, got "
                        << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
                return RETURN_STATUS::FAIL ;
        }
        n1 = ARG_POP ;
        if( n1->get_type() != DATA_TYPE::FLOATING ) {
                errs << "formatCart: argument is not a float\n" ;
                return RETURN_STATUS::FAIL ;
        }
	(void)sprintf(tempString,"%9.6f",n1->get_double());
        *result = tempString ;
        return RETURN_STATUS::SUCCESS ;
}


RETURN_STATUS ConvertMPFTime_ap ( Cstring & errs , TypedValue * result , ARG_STACK ) {
	TypedValue	* n1; // "MPFTime"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
		errs << "  wrong number of parameters passed to user-defined function ConvertMPFTime_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ; }
	n1 = ARG_POP ;
	if( ! n1->is_string() ) {
		errs << "ConvertMPFTime_ap: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ; }
	try {
	*result = ConvertMPFTime( (char *)*n1->get_string() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ; }

RETURN_STATUS  get_ET_time_ap ( Cstring & errs , TypedValue * result , ARG_STACK ) {
	TypedValue	* n1; // "MPFTime"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
		errs << "  wrong number of parameters passed to user-defined function  get_ET_time_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ; }
	n1 = ARG_POP ;
	if( ! n1->is_numeric() ) {
		errs << " get_ET_time_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ; }
	try {
	*result =  get_ET_time(  n1->get_double() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ; }

RETURN_STATUS  get_Current_UTC_time_ap ( Cstring & errs , TypedValue * result , ARG_STACK ) {
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
		errs << "  wrong number of parameters passed to user-defined function  get_Current_UTC_time_ap; expected 0, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ; }
	try {
	*result =  get_Current_UTC_time( ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ; }



RETURN_STATUS Ceil ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1;  //"string"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Ceil; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "GetEnv: first argument is not an float\n" ;
		return RETURN_STATUS::FAIL ;
		}
	*result = ceil( n1->get_double() ) ;
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS   normalize_string_ap(Cstring &errs, TypedValue *result, ARG_STACK) {
	TypedValue      *n1; // "string 1"

	errs.undefine();
	if(ARG_POINTER != (WHERE_TO_STOP + 1)) {
		errs << "  wrong number of parameters passed to user-defined function normalize_string; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
			return RETURN_STATUS::FAIL; }
	n1 = ARG_POP;
	if(!n1->is_string()) {
		errs << "arg of normalize_string() is not a string\n";
		return RETURN_STATUS::FAIL; }
	char* t = strdup(*n1->get_string());
	char* u = t;
	while(*u) {
		if(*u == ' ' || *u == '-' || *u == '"' || *u == '&' || *u == '$' || *u == '/') {
			*u = '_'; }
		u++; }
	*result = t;
	free(t);
	return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS   is_an_array_ap(Cstring &errs, TypedValue *result, ARG_STACK) {
	TypedValue      *n1; // "string 1"

	errs.undefine();
	if(ARG_POINTER != (WHERE_TO_STOP + 1)) {
		errs << "  wrong number of parameters passed to user-defined function is_an_array; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
			return RETURN_STATUS::FAIL; }
	n1 = ARG_POP;
	*result = n1->is_array() != 0;
	return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS   is_an_act_type_ap(Cstring &errs, TypedValue *result, ARG_STACK) {
	TypedValue      *n1; // "string 1"

	errs.undefine();
	if(ARG_POINTER != (WHERE_TO_STOP + 1)) {
		errs << "  wrong number of parameters passed to user-defined function is_an_act_type; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
			return RETURN_STATUS::FAIL; }
	n1 = ARG_POP;
	if(!n1->is_string()) {
		errs << "is_an_act_type: argument 1 is not a string\n";
		return RETURN_STATUS::FAIL; }
	map<Cstring, int>::const_iterator iter = Behavior::ClassIndices().find(n1->get_string());

	//
	// return true if Type exists and it's an activity type:
	//

	*result = iter != Behavior::ClassIndices().end() &&
		Behavior::ClassTypes()[iter->second]->realm == "activity";
	return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS	get_all_act_types(Cstring &errs, TypedValue *result, ARG_STACK) {
	// no args
	errs.undefine();
	if(ARG_POINTER != WHERE_TO_STOP) {
		errs << "  wrong number of parameters passed to user-defined function get_all_act_types; "
			"expected 0, got " << ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
		return RETURN_STATUS::FAIL;
	}
	ListOVal*	L = new ListOVal;
	int		count = 0;

	vector<Behavior*>& types = Behavior::ClassTypes();
	for(int i = 0; i < types.size(); i++) {
		if(types[i]->realm == "activity") {
			L->add(count++, types[i]->name);
		}
	}
	*result = *L;
	return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS	check_delete_signal(Cstring &errs, TypedValue *result, ARG_STACK) {
	TypedValue*		n1; // string signal_id
	sigInfoNode*		tag;

	errs.undefine();
	if(ARG_POINTER != (WHERE_TO_STOP + 1)) {
		errs << "  wrong number of parameters passed to user-defined function check_delete_signal; "
			"expected 1, got " << ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
                return RETURN_STATUS::FAIL;
        }
	n1 = ARG_POP;
	if(!n1->is_string()) {
		errs << "check_delete_signal: arg is not a string\n";
		return RETURN_STATUS::FAIL;
        }
	if(!(tag = eval_intfc::find_a_signal(n1->get_string()))) {
		*result = (long) 0;
		return RETURN_STATUS::SUCCESS;
        }
	delete tag;
	*result = (long) 1;
	return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS exp_invcos(Cstring& errs, TypedValue* res, ARG_STACK) {
	TypedValue	*n1; // float cosine_value

	errs.undefine();
	if(ARG_POINTER != (WHERE_TO_STOP + 1)) {
		errs << "  wrong number of parameters passed to user-defined function invcos; "
			"expected 1, got " << ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
                return RETURN_STATUS::FAIL;
        }
	n1 = ARG_POP;
        if(!n1->is_numeric() ) {
                errs = "non-float arg. in invcos()" ;
                return RETURN_STATUS::FAIL ;
        }
        *res = acos(n1->get_double());
        return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS delete_signals(Cstring& errs, TypedValue* res, ARG_STACK) {
	TypedValue	*n1; // float cosine_value
	List		to_delete;
	Pointer_node*	p;
	sigInfoNode*	signode;

	errs.undefine();
	if(ARG_POINTER != (WHERE_TO_STOP + 1)) {
		errs << "  wrong number of parameters passed to user-defined function delete_signals; "
			"expected 1, got " << ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
                return RETURN_STATUS::FAIL;
        }
	n1 = ARG_POP;
        if(!n1->is_string() ) {
                errs = "non-string arg. in delete_signals()" ;
                return RETURN_STATUS::FAIL ; }
	Cstring signalName = n1->get_string();
	signode = eval_intfc::find_a_signal(signalName);
	if(signode) {
		*res = 1L;
		delete signode;
        } else {
		*res = 0L;
        }
	return RETURN_STATUS::SUCCESS;
}


RETURN_STATUS start_debug_events_ap(Cstring& errs, TypedValue* res, ARG_STACK) {
#ifdef DBGEVENTS
	inhibitDBGEVENTS = false;
#endif /* DBGEVENTS */
	return RETURN_STATUS::SUCCESS; }

RETURN_STATUS stop_debug_events_ap(Cstring& errs, TypedValue* res, ARG_STACK) {
#ifdef DBGEVENTS
	inhibitDBGEVENTS = true;
#endif /* DBGEVENTS */
	return RETURN_STATUS::SUCCESS; }

#define UDEF_MSG "EHM udef library; build date: "

//
// In User_Lib_Title.C, created by Makefile at build time:
//
extern char*	User_lib_Title ;

extern char*	User_lib_build_date ;

extern "C" {
void register_all_user_extensions() {}

void	register_all_user_defined_functions() {
	ostringstream s;
	s << "EHM udef library version " << VERSION << " - build date: "
		<< User_lib_build_date;
	string version_string = s.str();
	//
	// UDEF version added May 25, 2004
	// UDEF VERSION from configure.ac added July 22nd, 2019
	//
	User_lib_Title = (char*) malloc(strlen(version_string.c_str()) + 1);
	strcpy(User_lib_Title, version_string.c_str());

	//
	// output to stdout removed July 22ndm 2019
	//
	// cout << User_lib_Title << endl ;

	udef_intfc::add_to_Func_list("is_an_act_type",			is_an_act_type_ap,		apgen::DATA_TYPE::BOOL_TYPE);
	udef_intfc::add_to_Func_list("is_an_array",			is_an_array_ap,			apgen::DATA_TYPE::BOOL_TYPE);
	udef_intfc::add_to_Func_list("normalize_string",		normalize_string_ap,		apgen::DATA_TYPE::STRING);

	udef_intfc::add_to_Func_list( "invcos" ,			exp_invcos,			apgen::DATA_TYPE::FLOATING);

	udef_intfc::add_to_Func_list( "keyset" ,			keyset,				apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "subset" ,			subset,				apgen::DATA_TYPE::ARRAY);

	register_spice_subsystem_functions();
	register_file_subsystem_functions();

	udef_intfc::add_to_Func_list( "formatCPCM",			formatCPCM,			apgen::DATA_TYPE::STRING ) ;
	udef_intfc::add_to_Func_list( "formatCart",			formatCart,			apgen::DATA_TYPE::STRING ) ;
        udef_intfc::add_to_Func_list( "Ceil",				Ceil,				apgen::DATA_TYPE::FLOATING) ;


	udef_intfc::add_to_Func_list("get_all_act_types",		get_all_act_types,		apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list("check_delete_signal",		check_delete_signal,		apgen::DATA_TYPE::INTEGER);
	udef_intfc::add_to_Func_list("delete_signals",			check_delete_signal,		apgen::DATA_TYPE::INTEGER);

	udef_intfc::add_to_Func_list( "getCurrentTimeInMilliseconds",	getCurrentTimeInMilliseconds,	apgen::DATA_TYPE::FLOATING);

	udef_intfc::add_to_Func_list( "ConvertMPFTime",			ConvertMPFTime_ap,		apgen::DATA_TYPE::STRING);
	udef_intfc::add_to_Func_list( "get_ET_time",			get_ET_time_ap,			apgen::DATA_TYPE::STRING);

	udef_intfc::add_to_Func_list( "get_Current_UTC_time",		get_Current_UTC_time_ap,	apgen::DATA_TYPE::STRING);

	register_vector_subsystem_functions();

	udef_intfc::add_to_Func_list( "start_debug_events",		start_debug_events_ap,		apgen::DATA_TYPE::UNINITIALIZED);
	udef_intfc::add_to_Func_list( "stop_debug_events",		stop_debug_events_ap,		apgen::DATA_TYPE::UNINITIALIZED);

}

void last_call() {
	last_call_to_spice();
}

//
// Invoked at the beginning of the modeling process
//
void clear_all_user_caches() {
	clear_spice_cache();
}

//
// Invoked at the end of the modeling process
//
void clean_up_all_user_caches() {
	clean_up_spice_cache();
}

} // extern "C"

Cstring SCET_PRINT(double time)
{
  double D , F ;

  F = modf( time , &D ) ;
    return CTime_base((long) D, (long) (1000 * F), false).to_string();
}

int debug_is_on()
{
    if ( getenv("DEBUG") != NULL )
        if ( strcmp(getenv("DEBUG"),"ON") == 0 )
            return true ;
        else
            return false ;
    else
        return false ;
}

extern "C" {
char *get_udef_version() {
	return User_lib_Title ; }
}
