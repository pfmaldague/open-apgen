#ifndef ACT_REQ_H
#define ACT_REQ_H

#define ACTREQSTART     "start"
#define ACTREQFINISH    "finish"


#include <accessorkey.H>
#include <ParsedExpressionSystem.H>

class ACT_sys; // unused but prepares for client override
class ActivityInstance;
class RES_resource;
class exec_status;

namespace pEsys {
class ActInstance;
class Decomp;
}


//
// Structures to hold parameter type information
// for those parameters that are arrays. Documenting
// such array is not a simple task, because a single
// parameter can be a struct whose elements are themselves
// structs or lists containing arrays. Therefore, the
// documentation process needs to be recursive.
//
// The recursive documentation process is implemented
// in actAgent::transfer_to_stream() (file actAgent.C).
// Look at the case where the file type is FT_TOL.
//
// The code that outputs the parameters of an activity
// instance contains documentation about the recursive
// documentation process.
//
// Since the information that needs to be stored in each
// node of a struct that is an array can be quite complex,
// we are using a flexval to store it. See actAgent.C
// for details.
//
typedef Cntnr<alpha_string,	flexval>	arrayNode;
typedef tlist<alpha_string,	arrayNode>	arraytypelist;
typedef Cntnr<alpha_string,	arraytypelist>	pinfoNode;
typedef tlist<alpha_string,	pinfoNode>	pinfotlist;
typedef slist<alpha_string,	pinfoNode>	pinfoslist;


extern int	destroy_an_act_req(ActivityInstance *, bool force = false);
extern blist&	theStandardSASFelementNames();

class decomp_error {
public:
	decomp_error(const Cstring& theError) : msg(theError) {}
	decomp_error(const decomp_error& ee) : msg(ee.msg) {}
	~decomp_error() {}
	Cstring		msg;
};

// Class used to express the (actual or desired) state of an activity instance
class act_instance_state {
private:
	// This constructor captures the state of the instance supplied as a parameter.
	act_instance_state(ActivityInstance *);
				// Sets the instance's visibility
	void			set_visibility_to(
					apgen::act_visibility_state A) {
		invisibility = A;
	}
				// Used for re-parenting an activity
	void			set_parent_to(
					ActivityInstance *a) {
		parent = a;
	}
				// Reparenting, from the parent's perspective
	void			set_child_to(
					ActivityInstance *a) {
		has_offspring = true;
		child = a;
	}
	void			set_resolution_to(int l) {
		resolution_case = l;
	}
				// use this to delete all children
	void			set_offspring_to(
					bool b) {
		has_offspring = b;
	}

				// desired/actual visibility
	apgen::act_visibility_state	invisibility;
				// desired/actual parent
	ActivityInstance*	parent;
				// desired/actual child
	ActivityInstance*	child;
				// whether we have offspring
	bool			has_offspring;
				// which case out of possible resolutions
	int			resolution_case;
	Cstring			to_string();
	friend class instance_state_changer;
};


	/* An instance_state_changer object is used to effect all
	non-symbol-table-driven transitions in the state of an activity instance */
class instance_state_changer {
private:
				// current state
	act_instance_state	current_state;
				// desired state
	act_instance_state	desired_state;
				// the instance whose state we want to change
	ActivityInstance*	ACT_req_changing;

	/* CONVENIENCE FUNCTIONS (private to this class only) */
				// turns instance into an orphan
	void			sever_from_parent(
					pEsys::execStack* stack_to_use);
				// we are about to become invisible
	void			undisplay_hierarchy();
public:
				// we adopt a new instance
	void			add_a_child(
					pEsys::execStack* stack_to_use,
					ActivityInstance* the_new_child);
	// Constructor, which captures the state of the given instance
	instance_state_changer(ActivityInstance *initial_act) :
		current_state(initial_act),
		desired_state(initial_act),
		ACT_req_changing(initial_act) {}

	/* Functions that set the desired state (used to request a state change) */
				// visibility state
	void			set_desired_visibility_to(
					apgen::act_visibility_state A) {
		desired_state.set_visibility_to(A);
	}
				// parent state
	void			set_desired_parent_to(
					ActivityInstance* a) {
		desired_state.set_parent_to(a);
	}
				// offspring state
	void			set_desired_offspring_to(
					bool B) {
		desired_state.set_offspring_to(B);
	}
				// child state
	void			set_desired_child_to(
					ActivityInstance* a) {
		desired_state.set_child_to(a);
	}
				// resolution state
	void			set_desired_resolution_to(int k) {
		desired_state.set_resolution_to(k);
	}

	//
	// MAIN METHOD OF THIS CLASS.
	//
	// The bool is optional; it tells do_it() whether
	// ActivityInstance::create() and ActivityInstance::destroy()
	// should be exercised while changing states. The only time
	// when we do not want to do this is when generating
	// a plan from an APF; the creation is done in one
	// fell swoop AFTER all the acts have been instantiated.
	// Reason for this: while individual instances are being
	// instantiated, the hierarchy is not in a consistent
	// state and one cannot tell whether the constructor/
	// destructors should be run.
	//
	void			do_it(
					pEsys::execStack* stack_to_use /* ,
					bool exercise_constr_destr = true */ );

	//
	// Special versions of do_it() that handles the case of
	// a brand new activity, i. e., one that was read from an
	// APF or one that was created via a NEWACTIVITY request.
	//
	void			instantiate_new_act();
	void			create_missing_nonexcl_children();

				//
				// for damage control prior to deleting a parent
				//
	void			fix_accelerator(
					ActivityInstance *);
};


//
// Class responsible for handling the hierarchy status of an activity instance
// (i. e., whether it's visible, abstracted, or decomposed)
//
class PR_manager {
private:
	void			move_to_abstracted_list();
	void			move_to_active_list();
	void			move_to_decomposed_list();
public:
	void			move_to_brand_new_list();

	PR_manager(ActivityInstance *p) :myboss(p) {}
	~PR_manager() {}

				// the parent ActivityInstance
	ActivityInstance*	myboss;
				// as it says
	ActivityInstance*	put_a_copy_of_me_into_clipboard() const;

	/* Public access methods */
	bool			is_active() const;
				// some ancestor is visible and we can connect to it through nonexclusive decomposition
	bool			is_weakly_active() const;
	bool			is_decomposed() const;
	bool			is_abstracted() const;
	bool			is_brand_new() const;
	bool			is_on_clipboard() const;

	/* client version has a method, DS_gc *get_tilepattern(DS_lndraw *) const,
	for which some type of overriding mechanism should be provided */
	const Cstring	&label_to_use_on_display() const;

	friend class instance_state_changer;
	friend class pEsys::Decomp;
};


	// Controls the data (not the state) associated with the instance hierarchy
class hierarchy_member {
private:
	// apgen::METHOD_TYPE	downtype;
	tlist<alpha_void, smart_actptr> superactivities;
	tlist<alpha_void, smart_actptr> subactivities;
	slist<alpha_void, smart_actptr>::iterator upIterator;
	slist<alpha_void, smart_actptr>::iterator downIterator;
	ActivityInstance*	up_accelerator;
				/* Used by add_a_child() and by
				 FixAllPointersToParentsAndChildren() in apf_intfc.C */
	void			update_up_accelerator(ActivityInstance*, bool Select = false);
public:
	int			attach_to_parent(hierarchy_member& newparent);

	hierarchy_member(apgen::METHOD_TYPE dt, ActivityInstance *P);
	hierarchy_member(const hierarchy_member& hm);
	~hierarchy_member() {}

	ActivityInstance*	myboss;

	/* Public methods that should perhaps be private */

				// used in apf_intfc.C
	void			move_all_descendants_by(
					CTime_base deltatime);

	/* @name Public methods that are OK in the public section */

				// selects the entire hierarchy this object belongs to
	void			select_hierarchy(
					bool down_only);

				// lets us keep superactivities private
	const slist<alpha_void, smart_actptr>::iterator& get_up_iterator() const {
		return upIterator;
	}
				// lets us keep subactivities private
	const slist<alpha_void, smart_actptr>::iterator& get_down_iterator() const {
		return downIterator;
	}
	int			children_count() const {
		return subactivities.get_length();
	}
	bool			can_be_decomposed_without_errors(
					bool redetail);
	void			recursively_get_ptrs_to_all_descendants(
					tlist<alpha_void, dumb_actptr>& l);
	void			recursively_get_time_ptrs_to_nonexclusive_descendants(
					tlist<alpha_time, Cntnr<alpha_time, ActivityInstance*> >&);
	ActivityInstance*		get_to_the_top() const {
		return up_accelerator;
	}
	friend class instance_state_changer;
};

	// Class that controls the legend that an instance sits on
class legend_member {
private:
				// Mark for vertical move
	int			vmoveflg;
				// Contains one smart pointer to LegendObject
	List			zero_or_one_legend;
	void            	attach_to_legend(LegendObject *legend_to_use);
	ActivityInstance*		myboss;
public:
	legend_member(ActivityInstance *p);
	~legend_member() {}

				// Provides access
	LegendObject*		get_legend_object() const;
	bool			legend_exists() const { return zero_or_one_legend.get_length() > 0; }
				// all public changes go through here
	void			switch_to_legend(LegendObject* new_legend);
};

// suitable for providing an activity instance object with usage methods
class actAgent /* : public UseMethods */ {
public:
	actAgent(ActivityInstance* s)
		: mySource(s) {}

	~actAgent() {}

	ActivityInstance*	mySource;

	// methods that are specific to actAgent:
	execution_context::return_code exercise_decomposition(
					bool skip_wait = false,
					pEsys::execStack* = NULL);
	virtual execution_context::return_code exercise_modeling(
					bool skip_wait = false,
					pEsys::execStack* = NULL);

	void			get_param_info(
					int		p_index_starting_at_0,
					Cstring&	p_name,
					Cstring&	p_descr,
					TypedValue&	p_value,
					List&		range_values,
					apgen::DATA_TYPE& p_type) const;
	void			get_struct_param_info(
					int				index_starting_at_0,
					const Cstring&			prefix,
					map<string, string>&		names,
					map<string, string>&		types,
					map<string, list<string> >&	ranges,
					map<string, string>&		descriptions,
					map<string, string>&		units,
					TypedValue&			actualvalue) const;
	bool			hasAssociates() const;
	bool			hasType() const {
		return true;
	}
	const Cstring&		get_owner_name() const;

	void			dump() const;
	void			export_symbols(
					Cstring&			id,
					Cstring&			type,
					Cstring&			name,
					CTime_base&			start,
					tvslist&			Atts_w_official_tag,
					tvslist&			Atts_w_nickname,
					tvslist&			Params,
					stringslist&			Parent,
					stringslist&			Children,
					apgen::act_visibility_state&	V,
					Cstring&			subsystem) const;
	void			transfer_to_stream(
					aoString&	Stream,
					apgen::FileType	file_type,
					CTime_base*	bounds = NULL,
					pinfotlist*	info = NULL) const;

#	ifdef have_xmltol
	void			transfer_to_xmlpp(
					xmlpp::Element*,
					bool all_acts_visible,
					CTime_base* bounds = NULL,
					const char* timesystem = NULL) const;
#	endif /* have_xmltol */

};

class actKey {
public:
	static tlist<alpha_string, Cntnr<alpha_string, bool> >& get_list();
};

class act_object: public behaving_object {
public:
	static const Cstring&	GenericType();
	// debug
	static int act_obj_count;

	smart_ptr<pEsys::ActInstance>	parsedThis;

	act_object(
			ActivityInstance* a,
			const Cstring& name,
			const Cstring& proposed_id,
			Behavior& act_type,
			pEsys::ActInstance& parsed_obj_which_this_came_from);
	act_object(
			ActivityInstance* a,
			const Cstring& name,
			const Cstring& proposed_id,
			Behavior& act_type);
	ActivityInstance*	act;

private:
	act_object(const act_object& EC);
public:
	~act_object();

	virtual void		assign_w_side_effects(
					int	index,
					const	TypedValue&,
					bool	notify = true) override;
	ActivityInstance*	get_req() const override {
		return act;
	}

	//
	// This method is used _only_ by pEsys::Usage methods that
	// seek time information from the parent object, i. e., the
	// definition of start and finish. This method tells where
	// to find these symbols.
	//
	int			level_where_start_is_defined() const override {
		return 1;
	}
	virtual bool		print(	aoString&,
					const char* prefix) const override;

	virtual behaving_element get_parent();
	virtual TypedValue	to_struct() const override;
	void			set_timespan(
					const CTime_base& S);
	virtual void		move_starttime(
					const CTime_base& deltatime,
					bool move_children);
	void			setetime(const CTime_base& T);
	const CTime_base&	getetime() const;
};


class ActivityInstance : public Dsource {
public:
	/* Synchronize this with activity::generic
	 * Type in Behavior::ClassTypes() (Behavior.C) */
	typedef enum {

		ENABLED = behaving_object::ENABLED,
		ID	= behaving_object::ID,
		TYPE	= behaving_object::TYPE,
		PARENT	= behaving_object::PARENT,
		START	= behaving_object::START,
		FINISH	= behaving_object::FINISH,
		SUBSYSTEM= behaving_object::SUBSYSTEM,

		THIS	= 7,
		cOLOR	= 8,
		COLOR	= 9,
		DECOMP_SUFFIX=10,
		DESCRIPTION= 11,
		LABEL	= 12,
		LEGEND	= 13,
		MISC	= 14,
		NAME	= 15,
		PATTERN	= 16,
		PLAN	= 17,
		SASF	= 18,
		SPAN	= 19,
		STATUS	= 20
		} varlocation;
	/* name SECTION 1 -- Constructor/Destructor */
	ActivityInstance(
			act_object*	obj);
	ActivityInstance(const ActivityInstance &ar);
	~ActivityInstance();

		// backpointer support
	backptr<ActivityInstance> ptrs;
	virtual backptr<ActivityInstance>* Pointers() {
		return &ptrs;
	}
	const backptr<ActivityInstance>* Pointers() const {
		return &ptrs;
	}
				// so the getKey().getetime() method works for timeptr<ActivityInstance>:
	ActivityInstance&	getKey() {
		return *this;
	}

	behaving_element	Object;
	act_object*		obj() {
		act_object* ao = dynamic_cast<act_object*>(Object.object());
		return ao;
	}
	const act_object*	obj() const {
		act_object* ao = dynamic_cast<act_object*>(Object.object());
		return ao;
	}

protected:
	/* SECTION 2 -- Helper objects */
	hierarchy_member	theHierarchyHandler;
	legend_member		theLegendHandler;
	PR_manager		thePRmanager;
public:
	actAgent		A;
	actAgent*		theActAgent() {
		return &A;
	}
	const actAgent*		theConstActAgent() const {
		return &A;
	}

	static bool verify_smart_pointers(
					ActivityInstance*, bool);
	// for debugging:
	Cstring			report_value_of_this();

	static apgen::RETURN_STATUS add_new_attributes_to_the_list(
					Cstring &errs);
	hierarchy_member&	hierarchy() {
		return theHierarchyHandler;
	}
	const hierarchy_member&	const_hierarchy() const {
		return theHierarchyHandler;
	}
	PR_manager		&agent() {
		return thePRmanager;
	}
	const PR_manager&	const_agent() const {
		return thePRmanager;
	}
	legend_member&		legend() {
		return theLegendHandler;
	}

	/* SECTION 3 -- Owner of adaptation language symbols */
				// for fast access
	// TypedValue*		start_value;
				// points to epoch
	Cstring			EpochIfApplicable;


	/* SECTION 4 -- Owner of symbols that can be changed */
	void			handle_epoch_in_start_time(
					const Cstring&	epoch_name,
					TypedValue*	epoch_value,
					bool		extend_to_children);
	bool			has_editable_duration() const;
				// changes 'status'
	void			set_scheduled(
					bool);
				// changes 'start'
	void			setetime(
					const CTime_base& T);
				// changes 'plan'
	void			set_APFplanname(
					const Cstring &newplanname);
				// resets global variables used to set unique IDs
	static void		purge();

	/* @name SECTION 5 -- Plan data with a unique tag */
protected:
	Cstring			generate_id(const Cstring& proposed_id);
	static blist&		NumbersInUse();
	static blist&		NumbersAvailable();
	static unsigned long long& abs_act_count();
	static unsigned long long& number_of_activities();

	/* SECTION 6 -- Owner of an ActivityType - OBSOLETE */

	/* SECTION 7 -- Node in a List */
	void			action_callback(
					void* theExecutiveOwner,
					list_event what_happened);
public:
	Node_type		node_type() const { return ACT_REQ; }

				// we don't ever, ever want to use this:
	Node*			copy() {
		return NULL;
	}

	virtual void		acknowledge_timing_changes() {}

	/* SECTION 8 -- Owner of modeling events */
public:
	void			create();
	void			destroy();

	execution_context::return_code exercise_modeling(
					bool skip_wait = false,
					pEsys::execStack* = NULL);
	execution_context::return_code exercise_decomposition(
					bool skip_wait = false,
					pEsys::execStack* = NULL);

	/* SECTION 9 -- GET (mostly const) methods */
public:
		/* object-related queries */
	time_actptr*		get_end_marker() const;
	ActivityInstance*	get_this() {
		return this;
	}

		/* list-of-symbols queries */
	const CTime_base&	getetime() const;
	alpha_time		Key;
	const alpha_time&	getKey() const {
		return Key;
	}
	virtual CTime_base	get_timespan() const;
	Cstring			get_APFplanname() const;
	void			get_optional_SASF_attributes(
					List &L) const;
	bool			is_SASF_enabled() const;
	int			get_SASF_attribute(
					const Cstring &keyword,
					Cstring &theValue) const;
	bool			is_unscheduled() const;

		/* name-related queries */
	const Cstring&		get_key() const;
	const Cstring&		get_unique_id() const {
		return (*Object)[behaving_object::ID].get_string();
	}
	Cstring			identify() const {
		return get_key() + " (id=" + get_unique_id() + ")"; }

		/* legend-related queries */
	const Cstring&		getTheLegend() const;
	int			getTheLegendIndex() const;

	bool			has_decomp_method(apgen::METHOD_TYPE& type) const;
	ActivityInstance*	get_parent() const {
		smart_actptr* p = theHierarchyHandler.get_up_iterator().const_first();
		return (p ? p->BP : NULL);
	}

		/* virtual class hierarchy-related queries + related data member */
				// KLUDGE -- to be removed as soon as we are 'clean' enough

	int			get_param_number() const;
	void			get_param_info(
					int		p_index_starting_at_0,
					Cstring&	p_name,
					Cstring&	p_descr,
					TypedValue&	p_value,
					List&		range_values,
					apgen::DATA_TYPE& p_type) const;

	/* SECTION 10 -- I/O */

	static bool&		we_are_writing_an_SASF();
	void			transfer_to_stream(
					aoString&	the_stream,
					apgen::FileType	file_type,
					CTime_base*	bounds = NULL,
					pinfotlist*	info = NULL) const;
#	ifdef have_xmltol
	void			transfer_to_xmlpp(
					xmlpp::Element*,
					bool all_acts_visible,
					CTime_base* bounds = NULL,
					const char* timesystem = NULL) const;
#	endif /* have_xmltol */

	void			transfer_step_to_stream(
					aoString&,
					int&			seq_num,
					const CTime_base&	the_request_time,
					const CTime_base&	the_previous_start,
			 		const CTime_base&	the_previous_end) const;
	void			transfer_request_to_stream(
					aoString&,
					blist*			list_of_legs_for_sasf) const;
				/* writes start time in epoch-relative style when appropriate
				(useful in APF and MIX file output) */
	Cstring			write_start_time(
					int epoch_but_not_time_system) const;
				// debugging aid
	void			dump() const;

				// useful in APF and APcore API-style output
	void			export_symbols(
					Cstring&			id,
					Cstring&			type,
					Cstring&			name,
					CTime_base&			start,
					tvslist&			Atts_w_official_tag,
					tvslist&			Atts_w_nickname,
					tvslist&			Params,
					stringslist&			Parent,
					stringslist&			Children,
					apgen::act_visibility_state&	V,
					Cstring&			subsystem) const;

	/* SECTION 11 -- overriding methods for Dsource API, including Graphics */

	bool			has_parent() const {
		return get_parent() != NULL;
	}
	bool			is_command() const;
	bool			is_on_clipboard() const {
		return const_agent().is_on_clipboard();
	}
	bool			is_active() const {
		return const_agent().is_active();
	}
	ActivityInstance*	get_to_the_top() const {
		return const_hierarchy().get_to_the_top();
	}

	//
	// Used to initialize iterators using the copy constructor:
	//
	const slist<alpha_void, smart_actptr>::iterator& get_down_iterator() const {
		return const_hierarchy().get_down_iterator();
	}
	LegendObject*		get_legend() {
		return legend().get_legend_object();
	}
	void			switch_to_legend(
					LegendObject* LO) {
		legend().switch_to_legend(LO);
	}
	int			children_count() {
		return hierarchy().children_count();
	}

	// void			destroy_this();

	bool			is_decomposed() const {
		return const_agent().is_decomposed();
	}
	bool			is_abstracted() const {
		return const_agent().is_abstracted();
	}

	/* SECTION 12 -- Friendship with constructor/destructor helper functions
	The factory method allows a derived class to impose its constructor as the "standard".
	This way, when e. g. an APF is read, the higher-level objects are automatically
	constructed.  */

				// used to keep track of earliest start time:
	static CTime_base&	getTimelineStart();
	static CTime_base&	getTimelineEnd();

			// wrapper around the destructor
	friend int	destroy_an_act_req(
					ActivityInstance*, bool force);
};

class dumb_actptr: public baseC<alpha_void, dumb_actptr> {
public:
	dumb_actptr() : payload(NULL) {}
	dumb_actptr(ActivityInstance* a)
		: baseC<alpha_void, dumb_actptr>((void*) a), payload(a) {}
	dumb_actptr(void* b, ActivityInstance* a)
		: baseC<alpha_void, dumb_actptr>(b), payload(a) {}
	dumb_actptr(const dumb_actptr& DA)
		: baseC<alpha_void, dumb_actptr>(DA), payload(DA.payload) {}
	~dumb_actptr() {
		if(list) list->remove_node(this);
	}

	dumb_actptr*			copy() {
		return new dumb_actptr(*this);
	}
  	dumb_actptr*			get_this() {
		return this;
	}
  	const dumb_actptr*		get_this() const {
		return this;
	}

	const Cstring&			get_key() const {
		static Cstring s("NULL");
		return payload ? payload->get_unique_id() : s;
		// return s;
	}
	ActivityInstance*		payload;
};


#endif /* ifndef ACT_REQ_H */
