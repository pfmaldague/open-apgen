#ifndef _AP_DATA_H
#define _AP_DATA_H

#include <map>
#include <list>
#include <mutex>
#include <fstream>

// #define DEBUG_CONSTRAINTS

#include "TypedValue.H"

class DS_time_scroll_graph; // unused but paves the way for client implementation
class Rsource;
class RCsource;
class mEvent;
class flexval;
class actAgent;
class RES_abstract;

typedef Cnode0<alpha_string, TypedValue> TaggedValue;
typedef slist<alpha_string, TaggedValue> tvslist;
typedef tlist<alpha_string, TaggedValue> tvtlist;

extern thread_local int thread_index;

#include <BehavingElement.H>

			// in APdata.C:
extern ListOVal&	list_of_all_typedefs();

class ActivityInstance;

			// general string utilities:
extern void		removeQuotes(Cstring & s);
extern Cstring		addQuotes(const Cstring &s);

			// general utilities for s/w version and build info:
extern const char*	get_apgen_version_build_platform();
extern const char*	get_apgen_version();
extern const char*	get_build();

	 		// Handles that provide access to higher-level capabilities:  

			// handle for CTime constructor (derived from CTime_base):
extern CTime_base	(*string_to_CTime_base_handler)(const Cstring&);

			// handle for activity type serialization:
extern void		(*transfer_act_type_to_stream_handler)(void *, aoString &);

//
// simple list. Its purpose is to provide a simple, efficient structure in
// which to store the arguments of function calls. Early versions of APGenX
// used a static stack for that purpose, but that design is incompatible with
// multithreading requirements; slst is the thread-safe version.
//
// Note: making slst a template is overkill; in practive, the class template
// argument T is always TypedValue*.
//
template<class T>
class slst {
  public:
	class node {
	  public:
		node() = delete;
		node(const node&) = delete;
		node& operator=(const node&) = delete;
		node(T s)
			: obj(s), next(NULL) {}
		~node() {

			//
			// We cannot do this, because
			// of pop_front() which deletes
			// the node but returns the object.
			//
			// This means that whoever creates
			// an slst object should explicitly
			// delete the objects which the
			// slst object points to after the
			// slst has finished its job.
			//
			// delete obj;
		}
		T	obj;
		node*	next;
	};

	slst() : head(NULL), length(0) {}
	~slst() {
		while(head) {
			node* N = head->next;
			delete head;
			head = N;
		}
	}
	void	push_front(T s) {
		node* N = new node(s);
		N->next = head;
		head = N;
		length++;
	}
	T pop_front() {
		node* N = head;
		if(N) {
			head = N->next;
			T ret = N->obj;
			delete N;
			length--;
			return ret;
		} else {
			throw(eval_error("empty list"));
		}
	}
	int	size() const {
		return length;
	}
	node*	head;
	int	length;
};

//
// template for APGEN functions (built-in as well as user-defined):
//
typedef apgen::RETURN_STATUS	(*generic_function) (
					Cstring&	   err,
					TypedValue*	   result,
					slst<TypedValue*>& args);
extern "C" {
	// to support c_option: (OBSOLETE)
	extern int		(*initialize_all_resources)(void*, char**);
	extern const char	*(*getSourceFileInfo)(void);
}


class triplet {
public:
	triplet() : Func(NULL), returntype(apgen::DATA_TYPE::UNINITIALIZED), count(0) {}
	triplet(const triplet& T) : Func(T.Func), Origin(T.Origin), returntype(T.returntype), count(0) {}
	triplet(generic_function F, const Cstring& s, apgen::DATA_TYPE dt)
		: Func(F), Origin(s), returntype(dt), count(0) {}
	~triplet() {}
	generic_function	Func;
	Cstring			Origin;
	apgen::DATA_TYPE	returntype;
	long int		count;
};

typedef BPnode<alpha_string, triplet> bfuncnode;

//
// Now that APGenX is multi-threaded, we need one instance of time_saver
// for each thread that is performing evaluation (not necessarily modeling.)
//
class time_saver {
private:
	//
	// Each thread has its own value of 'now'; this is where it
	// is stored. The argument should be thread_index.
	//
	// static CTime_base&	current_time(int);

	//
	// under local thread control:
	//
	CTime_base		save_now;
	static int		now_index;
public:
	time_saver() {
	    if(now_index < 0) {
		const task& T = behaving_object::GlobalObject()->Task;
	        map<Cstring, int>::const_iterator iter = T.get_varindex().find("now");
		if(iter == T.get_varindex().end()) {
		    Behavior::GlobalType().tasks[0]->add_variable("now", apgen::DATA_TYPE::TIME);
		    iter = T.get_varindex().find("now");
		    now_index = iter->second;
		    (*behaving_object::GlobalObject())[now_index]
			= CTime_base("2000-001T00:00:00.000");
		} else {
		    now_index = iter->second;
		}
	    }
	    // save_now = current_time(thread_index);
	    save_now = (*behaving_object::GlobalObject())[now_index].get_time_or_duration();
	}
	~time_saver() {
	    (*behaving_object::GlobalObject())[now_index] = save_now;
	}

	CTime_base		get_saved_time() const {
	    return save_now;
	}

	static void		set_now_to(const CTime_base& nowtime) {
	    // current_time(thread_index) = nowtime;

	    //
	    // Note that global_behaving_object is now
	    // thread-specific, so we will have no
	    // inteference
	    //
	    (*behaving_object::GlobalObject())["now"] = nowtime;
	}
	static CTime_base get_now() {
	    return (*behaving_object::GlobalObject())[now_index].get_time_or_duration();
	    // return current_time(thread_index);
	}
};

//
// Initially, this class provided an interface to the AAF compiler subsystem.
// As it evolved, the class became smaller and smaller, until it reached
// its current state where it is an inhomogeneous container of a few
// global objects. It should be eliminated.
//
class aaf_intfc {
public:
	aaf_intfc() {}
	~aaf_intfc() {}

	//
	// list of unique activity instance IDs
	//
	static tlist<alpha_string, Cnode0<alpha_string, ActivityInstance*> >& actIDs();

	//
	// Internal and library-defined functions. Note that AAF-defined functions
	// are stored in completely different containers:
	//
	// 	- aafReader::functions() for AAF-defined functions
	// 	- aafReader::methods() for AAF-defined methods
	// 	- Behavior::GlobalConstructor().tasks for both
	//
	static tlist<alpha_string, bfuncnode>&	internalAndUdefFunctions();

	//
	// to support the '-showlib' command-line option
	//
	static void show_all_funcs();
};

class model_control {
public:

	typedef enum {
		INACTIVE,
		MODELING,
		SCHEDULING_1,
		SCHEDULING_2
	} modelingPass;

	static const char* spell_pass(modelingPass p) {
		const char* s;
		switch(p) {
			case INACTIVE:
				s = "INACTIVE";
				break;
			case MODELING:
				s = "MODELING";
				break;
			case SCHEDULING_1:
				s = "SCHEDULING_1";
				break;
			case SCHEDULING_2:
				s = "SCHEDULING_2";
				break;
			default:
				s = "UNKNOWN";
				break;
		}
		return s;
	}
private:
	static modelingPass&	current_pass();
	modelingPass		save_pass;
public:
	model_control() {
		save_pass = current_pass();
	}
	model_control(modelingPass mp) {
		save_pass = current_pass();
		set_pass_to(mp);
	}
	~model_control() {
		set_pass_to(save_pass);
	}

	void			set_pass_to(
					modelingPass thePass);
	static modelingPass	get_pass() {
		return current_pass();
	}
};

// Class that provides most of the functionality needed by user-defined functions
class udef_intfc {
public:
	udef_intfc() {}
	~udef_intfc() {}

				// user-defined library interface:
	static void		add_to_Func_list(const char *, generic_function, apgen::DATA_TYPE);
	static void		add_to_Func_list(const char *, generic_function, const Cstring&, apgen::DATA_TYPE);
	static void		unregister_all_functions(bool print = false);
	static const char*	get_user_lib_version();
	static void		set_user_lib_version_to(const char *C);
	static const char*	user_lib_version_string;
				// hint to the GUI that it should refresh itself:
	static long&		something_happened();

				// some primitive info about modeling:
	// static bool		we_are_modeling() {
	// 	return modeling_flag;
	// }
	// static bool		modeling_flag;
};


class Dsource;
class LegendObject;

//
// Class that allows derived classes of Dsource to store their data
// in the lower-level object. It supports the APGenX GUI.
//
class derivedData {
public:
	derivedData(Dsource* s) : source(s) {}
	derivedData(const derivedData &dd) : source(NULL) {}
	virtual ~derivedData() {}
	Dsource* source;

	virtual void	handle_legend(LegendObject *new_legend) = 0;
	virtual void	handle_position() = 0;
	virtual void	handle_appearance() = 0;
	virtual void	handle_selection(bool selected) = 0;
	virtual void	handle_instantiation(bool which_way, bool active_only) = 0;
};

typedef BPptr<alpha_void, ActivityInstance>			smart_actptr;
typedef BPptr<alpha_time, ActivityInstance, ActivityInstance*>	time_actptr;
typedef Tptr<ActivityInstance, ActivityInstance*>		smart_actref;

class dumb_actptr;

//
// Dsource - Interface class for ActivityInstance
//
// NOTE: this is one of the few classes based on the old APGen List class,
// 	 which has been there from the very beginning. It should be replaced
// 	 by one of the container templates defined in templates.H, but this
// 	 should be done very, very carefully because of restrictions on what
// 	 can be done to the time-ordering of activities.
//
// NOTE: the Time_node class is abstract because of pure virtual methods
//	 setetime() and getetime(); these are provided in the ActivityInstance
//	 and Violation_info classes (i. e., the 2 superclasses of Dsource),
//	 but not by Dsource which is therefore an abstract class also.
//
class Dsource: public Time_node {
protected:
	Dsource()
		: S_elected(0),
		dataForDerivedClasses(NULL),
		insertion_state(unattached),
		saved_list(NULL)
		{}
	Dsource(const Dsource &ds)
		: Time_node(ds),
		S_elected(0),
		dataForDerivedClasses(NULL),
		insertion_state(unattached),
		saved_list(NULL)
		{}
public:
	virtual ~Dsource()
		{}

	typedef enum {
		unattached,			// native state
		temporarily_unattached,		// after being removed from a list; pointers are unattached
		need_to_reattach_pointers,	// just before insertion, if pointers were removed
		in_a_list,			// stable "inserted" state
		is_about_to_be_removed,		// decision was made to remove, need to remove smart pointers from their list
		} insertion_status;
	insertion_status	insertion_state;

protected:

	  			/* set to false by constructors. Private because several things
				 * need to be done when selecting an activity, which should
				 * always be done through the select() and unselect() methods below:
				 */
	bool			S_elected;

public:
	const bool		is_selected() const {
		return S_elected;
	}
	void			select();
	void			unselect();
	virtual const alpha_time& getKey() const = 0;

				// accelerator for status attribute; default = yes
	bool			scheduled;
				/* Internal handle to objects managed by derived classes.
				 * For instance, a GUI client can override the base derivedData
				 * class and insert an object of the derived type here.  */
	derivedData*		dataForDerivedClasses;
				// Used by the constraint violation code
	virtual void		set_y_to(int, int) {
	}

				/* Overridden in ACT_request. Depends on whether the
				 * instance is meant to appear in the hopper or not. */
	virtual CTime_base	get_timespan() const;

		/* Virtual methods that are overridden in ActivityInstance
		 * Purpose is to decouple the GUI code from the core
		 * by providing an API. */

	virtual bool		has_parent() const {
		return false;
	}
	virtual bool		is_on_clipboard() const {
		return false;
	}
	virtual bool		is_unscheduled() const {
		return false;
	}
	virtual bool		is_active() const {
		return true;
	}
	virtual bool		is_decomposed() const = 0;
	virtual bool		is_abstracted() const = 0;
	virtual bool		is_a_request() const { return false; }
	virtual Dsource*	get_to_the_top() const { return NULL; }
	virtual Dsource*	get_parent() const { return NULL; }

	virtual const Cstring&	get_unique_id() const {
		return Cstring::null();
	}
	virtual Cstring		identify() const {
		return Cstring();
	}
	virtual Cstring		get_APFplanname() const {
		return Cstring();
	}
#ifdef OBSOLETE
	virtual void 		transfer_to_stream(
					aoString&	the_stream,
					blist*		list_of_legs_for_sasf,
					apgen::FileType	file_type,
					) const {
	}
#endif /* OBSOLETE */

	virtual const slist<alpha_void, smart_actptr>::iterator& get_down_iterator() const = 0;
	virtual LegendObject*	get_legend() = 0;
	virtual int		children_count() = 0;
	virtual int		getTheLegendIndex() const = 0;

	virtual void		get_integrated_consumption_for(
					const Cstring&	assocRes,
					double&		result,
					bool&		is_used) {
	}
	virtual void		get_consumption_for(
					const Cstring&	assocRes,
					double&		result,
					bool&		is_used) {
	}

	virtual void		switch_to_legend(
					LegendObject*) {
		;
	}
	virtual void		recursively_get_ptrs_to_all_descendants(
					tlist<alpha_void, dumb_actptr>&) {
		return;
	}
	virtual void		select_hierarchy(bool) {
	}
	virtual void		handle_epoch_in_start_time(
					const Cstring&	epoch_name,
					TypedValue*	epoch_value,
					bool		extend_to_children) {
	}
	virtual Cstring		write_start_time(
					int) const {
		return Cstring(getetime().to_string());
	}

	virtual bool		is_an_activity() const {
		return true;
	}
	virtual bool		is_a_resource() const {
		return false;
	}
	virtual bool		is_a_violation() const {
		return false;
	}

	static blist&		theLegends();

	static void		(*derivedDataConstructor)(
					Dsource*,
					Cstring&);
	static void		(*derivedDataDestructor)(
					Dsource*);

				//
				// method optionally supplied by APcore; normally, NULL
				//
	static void		(*eventCatcher)(
					const char*	act_id,
					const char*	param);


	virtual List*		get_saved_list() {
		return saved_list;
	}
	virtual void		set_saved_list(List* L) {
		saved_list = L;
	}
	virtual void		temporarily_remove_from_list() {
		if(list) {
			saved_list = list;
			list->remove_node(this);
		}
	}
	virtual void		re_insert_in_list() {
		if(saved_list) {
			saved_list->insert_node(this);
			saved_list = NULL;
		}
	}
	List*			saved_list;
};


#include <vector>
class Cstack;


class res_val_type_owner {
public:
	res_val_type_owner(apgen::RES_VAL_TYPE t) : type(t) {}
	~res_val_type_owner() {}

	apgen::RES_VAL_TYPE	type;
	apgen::RES_VAL_TYPE	get_res_val_type() const { return type; }
};

typedef void (*Cplusplus_function) ();

class tokenPLD {
public:
	tokenPLD() : tokenval(0), tokenproc(NULL) {}
	tokenPLD(const tokenPLD& t) : tokenval(t.tokenval), tokenproc(t.tokenproc) {}
	tokenPLD(int i, void (*f)()) : tokenval(i), tokenproc(f) {}
	~tokenPLD() {}

	int	tokenval;
	void	(*tokenproc)();
};

class btokennode: public Dnode0<alpha_string, btokennode> {
public:
	btokennode() {}
	btokennode(const Cstring& s, int i, void (*f)()) : Dnode0<alpha_string, btokennode>(s), payload(tokenPLD(i, f)) {}
	virtual ~btokennode() {}

	btokennode*			copy() { return new btokennode(*this); }
	btokennode*			get_this() { return this; }
	const btokennode*		get_this() const { return this; }

	tokenPLD			payload;
};

namespace pEsys {
class pE;
class Program;
}

typedef smart_ptr<pEsys::pE> parsedExp;

apgen::DATA_TYPE string_to_data_type(const char* str);

namespace synchro {

class detector {
public:
	detector() {}
	virtual ~detector() {}

	virtual const Cstring& detectorName() const = 0;
};

class fixer {
public:
	fixer() {}
	virtual ~fixer() {}

	virtual const Cstring& fixerName() const = 0;
};

class problem;

class problemPLD {
public:
	problemPLD(
		detector*			D,
		problem*			(*detect)(detector*),
		fixer*				F,
		void				(*fix)(problem*))
		: the_detector(D),
			detect_action(detect),
			the_fixer(F),
			fix_action(fix) {}
	problemPLD(const problemPLD& P)
		: the_detector(P.the_detector),
			detect_action(P.detect_action),
			the_fixer(P.the_fixer),
			fix_action(P.fix_action) {}
	~problemPLD() {}

	detector*	the_detector;
	problem*	(*detect_action)(detector*);
	fixer*		the_fixer;
	void		(*fix_action)(problem*);
};

// NOTE: should turn the following into a template parameterized by payload class
class problem: public Dnode0<alpha_string, problem> {
public:
	problem() : Dnode0<alpha_string, problem>(Cstring()), payload(NULL, NULL, NULL, NULL) {}
	problem(
		const Cstring&			descr,
		detector*			D,
		problem*			(*detect)(detector*),
		fixer*				F,
		void				(*fix)(problem*),
		problem*			lower_level = NULL
		) :	Dnode0<alpha_string, problem>(descr),
			payload(D, detect, F, fix) {
		if(lower_level) {
			problems << lower_level; } }
	problem(const problem& P)
		: Dnode0<alpha_string, problem>(P), payload(P.payload), problems(P.problems) {}
	~problem()  {}

	problem*			copy() { return new problem(*this); }
	problem*			get_this() { return this; }
	const problem*			get_this() const { return this; }
	problemPLD			payload;
	Cstring				action_taken;
	slist<alpha_string, problem>	problems;
};

} /* synchro */

#endif /* _AP_DATA_H */
