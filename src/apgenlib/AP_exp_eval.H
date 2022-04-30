#ifndef AP_EX_EVAL_H
#define AP_EX_EVAL_H

#include <sys/types.h>
#include <regex.h>

#include "APbasic.H"
#include "AbstractResource.H"
#include "GLOBdata.H"
#include "Multiterator.H"	// for status_aware_multiterator
#include "multilist.H"
#include "ParsedExpressionSystem.H"
#include "ValueNode.H"
#include "flexval.H"

class Rsource;
class RCsource;
class execution_context;
class Constraint;
class res_miterator;
class AbstractResource;

class signalInfo {
public:
	signalInfo() {
	}
	signalInfo(	behaving_element o,
			const TypedValue& val)
		: obj(o),
			info(val) {
	}
	signalInfo(const signalInfo& s)
		: obj(s.obj),
			info(s.info) {
	}
	~signalInfo() {
	}
	behaving_element	obj;
	TypedValue		info;
};

typedef BPnode<alpha_string, signalInfo> sigInfoNode;
typedef BPptr<alpha_string, sigInfoNode> sigInfoPtr;

class patternInfo {
public:
	patternInfo()
		: pattern(NULL),
			regexp_error(0) {
	}
	patternInfo(const Cstring& regexp)
		: pattern_string(regexp),
			pattern(NULL),
			regexp_error(0) {
	}
	patternInfo(const patternInfo& p)
		: pattern_string(p.pattern_string),
			pattern(NULL),
			regexp_error(0) {
	}
	~patternInfo() {
		if(pattern) {
			regfree(pattern);
			delete pattern;
		}
	}

	void	compile() {
		assert(pattern == NULL);
		pattern = new regex_t;
		regexp_error = regcomp(pattern, *pattern_string, REG_EXTENDED);
		if(regexp_error) {
			char buf[1000];
			regerror(regexp_error, pattern, buf, 1000);
			// just in case regerror does not null-terminate the string:
			buf[999] = '\0';
			throw(eval_error(buf));
		}
	}
	bool	matches(
			const Cstring& s) {
		regmatch_t	m[1];
		assert(pattern);
		int nomatch = regexec(pattern, *s, 1, m, 0);
		return nomatch == 0;
	}

	regex_t*			pattern;
	Cstring				pattern_string;
	int				regexp_error;
	//
	// list of smart pointers to sigInfoNodes that match the pattern
	//
	tlist<alpha_string, sigInfoPtr>	Matches;
};

typedef Cnode0<alpha_string, patternInfo> sigPatternNode;

class eval_intfc {
private:
	tlist<alpha_string, sigInfoNode>	recent_signals;
	tlist<alpha_string, sigPatternNode>	recent_patterns;
	eval_intfc() {}
	~eval_intfc() {}
public:

	static eval_intfc&	evalIntfc();

	void			push_miterator_data(
		Miterator_base<slist<alpha_time, mEvent, eval_intfc*>, mEvent>*,
		slist<alpha_time, mEvent, eval_intfc*>::iterator*) {
	}
	void			pop_miterator_data(
		Miterator_base<slist<alpha_time, mEvent, eval_intfc*>, mEvent>*) {
	}

	//
	// UUUUU should be in mEvent:
	//
	static Miterator<slist<alpha_time, mEvent, eval_intfc*>, mEvent>&	Events();

	static bool				something_happened(
							int& activeCount,
							int& decomposedCount,
							int& abstractedCount);

	static void				get_all_selections(
							slist<alpha_void, smart_actptr>& l);

	static back_pointer*			(*act_type_as_back_ptr_handler)(
							void *);
	static back_pointer*			act_type_as_back_ptr(
							void *);
	static void*				act_type_as_void_ptr(
							const Cstring &);
	static apgen::RETURN_STATUS		string_to_valid_time(
							const Cstring &given_time,
							CTime_base &result,
							Cstring &errors);
	static apgen::RETURN_STATUS		duration_to_timesystem_relative_string(
							const CTime_base&	given_dur,
							const Cstring&		EpochName,
							Cstring&		result,
							double&			multiplier,
							Cstring&		errors);
	static apgen::RETURN_STATUS		time_to_epoch_relative_string(
							const CTime_base&	given_time,
							const Cstring&		EpochName,
							Cstring&		result,
							Cstring&		errors);

	// activity instances
	static stringtlist&			ListOfAllFileNames();
	static status_aware_multiterator*	get_instance_multiterator(
							int& first_avail_priority,
							bool incl_scheduled,
							bool incl_unscheduled);
	static act_lists&			get_act_lists();
	static tlist<alpha_void, smart_actptr>&	sel_list();

	static tlist<alpha_int, Cnode0<alpha_int,
			instruction_node*> >&	actsOrResWaitingOnSignals();
	static tlist<alpha_int, Cnode0<alpha_int,
			instruction_node*> >&	actsOrResWaitingOnCond();

	// resources
	static void				get_list_of_ptrs_to_containers(
							tlist<alpha_void,
							Cnode0<alpha_void,
							RCsource*> >& l);

	static void				get_list_of_ptrs_to_resources(
							tlist<alpha_void,
							Cnode0<alpha_void, Rsource*> >& l);
	static void				get_resource_subsystems(
							flexval&);

	//
	// To find out the data type of a resource container, use
	// RES_container::get_datatype() which returns a apgen::DATA_TYPE that
	// can be passed to apgen::spell().
	//
	static Rsource*			FindResource(
						const Cstring&	nm);

					// invoking this will clear the event queues:
	static void			purge_queues_and_histories(
						model_control::modelingPass);

	// modeling
	static bool			there_is_a_last_event(
						CTime_base &theEventTime);
	static bool			(*last_event_handler)(
						CTime_base &);
	static bool			there_is_a_first_act(
						CTime_base &theEventTime,
						int the_print_flag);
	static bool			(*first_act_handler)(
						CTime_base &,
						int the_print_flag);

	static long			number_of_recent_signals();
	static bool			a_signal_matches(
						const Cstring&);
	static sigInfoNode*		find_a_signal(
						const Cstring&);
	static sigPatternNode*		find_a_pattern(
						const Cstring&);
	static void			get_all_signals(
						stringtlist&);
	static void			clear_signals();
	static void			clear_patterns();
	static void			add_a_signal(
						sigInfoNode*);
	static tlist<alpha_void,
		Cnode0<alpha_void,
		    Constraint*> >&	active_constraints();

	static bool			restore_previous_id_behavior;
	static bool			found_a_custom_attribute;
	static bool			MaxModelTimeExceededAt(
						const CTime_base &);
	static CTime_base&		MaxModelTime();
};

class res_info_for_constraintPLD {
public:
	res_info_for_constraintPLD() : theCons(NULL), type(apgen::DATA_TYPE::UNINITIALIZED) {}
	res_info_for_constraintPLD(apgen::DATA_TYPE t, Constraint* c) : theCons(c), type(t) {}
	res_info_for_constraintPLD(const res_info_for_constraintPLD& r) : theCons(r.theCons), type(r.type) {}
	~res_info_for_constraintPLD() {}

	Constraint*		theCons;
	apgen::DATA_TYPE	type;
};

typedef Cnode0<alpha_string, res_info_for_constraintPLD> Res_node;


			// built-in functions:
extern apgen::RETURN_STATUS exp_ABS(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_acos(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_ADDQUOTES(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_AMAX(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_AMIN(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_AND(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_asin(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_atan(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_atan2(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_CEILING(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_copy_array(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_cos(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_DELETE_ELEMENT(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_DELETE_SIGNAL(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_displayMessageToUser(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_divide(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_element_exists(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_end_debug(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_equal(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_ERROR(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_EVAL_ACT_CONSUMPTION(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_exp(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_exponent(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_EXTRACT(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_FABS(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_FIND_MATCHES(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_FIX(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_FMOD(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_FP(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_FROM_SECONDS(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GetEnv(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_get_all_globals(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_ALL_SIGNALS(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_CHILDREN_OF(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_FILE_INFO(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_get_global(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_HEADER(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_ID_OF(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_INSTANCE_STRING(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_MATCHING_SIGNALS(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_PARENT_OF(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_PLAN(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_RESOURCE_INFO(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_TYPE_OF(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_SELECTION(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_SELECTION_IDS(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_SENDER(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_SENDER_INFO(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
// extern apgen::RETURN_STATUS exp_get_threads(	  	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_UNLABELED_CHILDREN_OF(Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GET_VERSION(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_global_exists(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GTRTHAN(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_GTRTHANOREQ(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_has_substring(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_instance_with_id(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_int2ascii(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_interrupt(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_intervals(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_is_frozen(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_itostr(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_length_of(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_make_constant(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_LESSTHAN(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_LESSTHANOREQ(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_log(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_log10(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_MAX(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_MIN(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_MINUS(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_MOD(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_MULT(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_NEG(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_NOTEQUAL(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_notify_client(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_numeric_index_of(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_OR(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_PAREN(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_PLUS(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_printf(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_random(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_read_xmltol_as_incon(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_REMOVEQUOTES(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_REGISTER_CALLBACK(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_REQUIRE(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_RND(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_start_debug(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_SENDEVENT(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_SENDREQUEST(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_SORT_BY_INTEGER(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_SORT_BY_TIME(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_sin(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_sqrt(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_string_to_float(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_string_to_int(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_string_to_time(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_Strlen(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_strtoi(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_STOP(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_STRREL(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_string_index_of(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_strstr(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_tan(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
// extern apgen::RETURN_STATUS exp_time_series(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_TIME_OF_DAY(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_time_to_string(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_TMIN(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_TO_SECONDS(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_TO_STRING(		  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_TRIM(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_UNREGISTER_CALLBACK(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_write_to_stdout(	  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_XOR(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);
extern apgen::RETURN_STATUS exp_XR(			  Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args);

#endif /* AP_EX_EVAL_H */
