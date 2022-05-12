#ifndef RES_DEF_H
#define RES_DEF_H

#include "aafReader.H"
#include "RES_eval.H"
#include "EventLoop.H"

class dummyHistory: public RES_history {
public:
	dummyHistory() : RES_history(NULL) {}
	~dummyHistory() {}
	virtual void		clear() override {}
	virtual bool		isFrozen() override { return false; }
	virtual long		get_length() override { return 0; }
	virtual void	get_numeric_values(
				const CTime_base&	a,
				const CTime_base&	b,
				tlist<alpha_time, unattached_value_node, Rsource*>& L,
				bool			all_vals,
				long&			theVersion
			  	) override {
	}

	virtual void	get_discrete_values(
				const CTime_base&	a,
				const CTime_base&	b,
				tlist<alpha_time, unattached_value_node, Rsource*>& L,
				bool			all_vals,
				long&			theVersion
			  	) override {
	}

	virtual void	get_settable_values(
				const CTime_base&	a,
				const CTime_base&	b,
				tlist<alpha_time, unattached_value_node, Rsource*>& L,
				bool			all_vals,
				long&			theVersion
			  	) override {
	}
};

class RES_resource;

/*
* Use value_nodes to hold resource values in 'ValueNodes' (a tlist).
* Also use them to store the 'history' of individual resources.
*
* As scheduling capabilities are added to apgen, it becomes necessary
* to modify the history-keeping mechanism. Originally, modeling was
* strictly a 2-pass algorithm: set up all the (potential) events first, 
* then scan the entire event queue and 'execute' events (i. e. exercise
* mEvent::do_model()) that are active (i. e. w/o a when clause
* or with a when clause that evaluates to 1).
* 
* Before jumping to scheduling issues, let's document the code a little
* bit.
* 
* When an event 'executes', it calls one of the following RES_resource
* methods:
* 
* 	METHOD			RES. TYPE		USAGE TYPE
* 	======			=========		==========
* 
* 	execute_set_clause()	STATE			SET
* 	execute_set_clause()	SETTABLE		SET
* 	execute_reset_clause()	STATE			RESET
* 	execute_use_clause()	CONSUMABLE,		USE
* 				NONCONSUMABLE,
* 				STATE
* 
* These methods in turn call append_to_histogram, which populates the
* RES_resource::resource_history.History clist with value_nodes.
* Loosely speaking, the history only stores ONE node per event
* (typically the new value of a state resource or the
* amount consumed for a continuous resource).
* 
* Note that there is one exception to this, namely when the 'interpolation'
* flag is used.
* 
* The precise meaning of the data stored in a value_node depends on the
* type of resource. For a state resource, value_nodes are introduced into
* the list whenever the state of the resource changes as a result of a
* 'SET', 'RESET' or 'USE' command. The new state of the resource
* is stored in the value_node.
* 
* For a settable resource, only SET is allowed.
* 
* (Note that state changes resulting from a time-varying profile are NOT
* documented in the history. The reason for this is that there is no
* general way of finding out where these state changes occur (the program
* for the time dependence of the profile can be arbitrarily complicated).
* One unfortunate side-effect is that state plots for state resources with
* a time-dependent profile contain unsightly diagonal lines. The only way to
* avoid this is to increase the resolution at which the resource is being
* plotted.)
* 
* For a continuous resource, only the 'amount consumed' is stored in the
* value_node if the value_node is to be inserted into the history. If the
* node is to be inserted into the ValueNodes tlist, then the value stored
* in it is the actual value of the resource.
* 
* Note that the 'value' associated with a value_node is stored in its
* 'val' data member.
* 
*
* As of 1/98, new RES_container class is defined, as unified way of implementing
* Arrayed Resources, and allowing them to coexist with non-arrayed Resources.
*
* References that formerly referred to RES_resource (or derived class) objects,
* now refer to RES_container objects.  RES_container has a flag of type
* RES_CONTAINER_TYPE to indicate whether it is an Array or not, and a List of the
* RES_resource (or derived) objects it contains (note that a Resource Array may
* contain 0*,1,2, or more such objects, whereas an ordinary Resource must contain
* exactly one).  RES_resource has a "contained-by" pointer to the 1 RES_container
* it belongs to.  RES_container now has the Resource (or Resource Array) name in
* its bstringnode; each RES_resource now has its FULL NAME in its bstringnode
* (in "canonical form":  for non-Array, same as Resource name; for Array, indexed
* name with the format: 'resource_name["index_string"]').
* 
* 0 members in an Array is a possible FUTURE capability (code is written to
* allow it, IF possible/easy); however, FOR NOW an Array must have 1+ members!
* Data members and methods were migrated to the RES_container class only if they
* were both invariant across Arrayed Resources, AND that migration made sense.
* 
* Attributes, States, and Profile may each be defined homogeneously (i.e. one
* shared definition) OR heterogeneously (i.e. separate definitions for subsets of
* the Array).  Parameters and [Resource] Usage MUST be defined homogeneously --
* the concept of Arrayed Resources permits reasonable variation but also demands
* adequate similarity.  Also, the Resource Type MUST be homogeneous within an
* Array, i.e. all members of a Resource Array must be of the same derived class.
* 
*
*
*   RES_resource derived classes:			get_class() returns:
*
*	class RES_abstract:	public RES_resource	ABSTRACT
*	class RES_state:	public RES_resource	STATE
*	class RES_settable:	public RES_resource	SETTABLE
*	class RES_associative:	public RES_resource	INTEGRAL or ASSOCIATIVE
*	class RES_numeric:	public RES_resource	
*	class RES_consumable:	public RES_numeric	CONSUMABLE
*	class RES_nonconsumable:public RES_numeric	NONCONSUMABLE
*
* Low-level note: The Cstring of the bstringnode (i. e., the key) is the
* Resource (if simple) or Array name (if not).
*/
class Rcontainer: public resContPLD {
public:
	Rcontainer(
			smart_ptr<res_container_object>&	obj,
			Behavior&				T,
			apgen::RES_CLASS			the_class,
			apgen::RES_CONTAINER_TYPE		single_or_array,
			apgen::DATA_TYPE			dt);
	~Rcontainer();

	RCsource*		mySource;
	virtual void		set_source_to(RCsource* r) {
		mySource = r;
	}

	//
	// *** DATA MEMBERS BELOW ARE SHAREABLE;
	//     98-01-29 MOVED from RES_resource ***
	//
	// For each member "foo", a "get_foo()" method is
	// now available in RES_resource
	//

	virtual void		clear_profiles();

	//
	// recursively identifies resources whose currentval is needed when evaluating the profile;
	// tlist ok_list is a list of bpointernodes to resources whose profiles have no loops;
	// tlist resource_chain is a list of all the resources in the recursion chain, and is used
	// to detect circular dependencies.
	//
	virtual apgen::RETURN_STATUS complete_dependency_list(
				tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& dependents,
				tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& ok_list,
				tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& resource_chain,
				Cstring &any_errors);
};

//
// This class is the main "working class" for non-associative resource objects.
// A resource has a name (which includes a string index enclosed in square
// brackets if the resource is not simple, i. e., a member of an arrayed
// resource 'container').
//
// Accordingly, the get_key() method evaluates to the <Resource Name> (if the
// resource is simple) or to <Resource Name>["index_string"] (if not).
//
class RES_resource: public Rsource {
public:

	// CONVENIENCE FUNCS.

	//
	// 98-01-29 DSG moved to RES_container:  usage_program, parameters,
	// declarations; see below for convenience get_* methods
	//

	virtual bool		is_hidden() const;
	RCsource*		get_res_container() {
		return &parent_container;
	}

	RES_resource(
		RCsource&	p,
		Behavior&	T,
		const Cstring&	namein);

	RES_resource(const RES_resource& Rr);

	//
	// Recall that the interest of a virtual destructor is
	// that the statement "delete (base_class *) b;"
	// invokes derived_class::~derivedclass() provided that
	// the ACTUAL type of b is derived_class
	//
	~RES_resource();

	const Cstring&		get_owner_name() const {
		return name;
	}

	bool			is_an_activity() const {
		return false;
	}
	bool			is_a_resource() const {
		return true;
	}
	bool			is_a_violation() const {
		return false;
	}
	long			get_history_size() {
		return get_history().get_length();
	}

	const pEsys::Program&	get_const_attribute_program() const {
			return *Object->Task.prog.object();
	}

	virtual Cstring		get_id() {
		return name;
	}

	RES_resource*		rsource_as_res_resource() {
		return this;
	}


	// GET-type functions

	//
	// get Resource [Array] name only; get_key() returns Resource name
	// (for non-Array) or formatted Resource["Index"] (for Array)
	//
	const Cstring&		get_name() {
		return parent_container.get_key();
	}

	void			addToTheProfile(
					tlist<alpha_time, Cntnr<alpha_time, parsedExp> >& l) {
		theProfile = l;
	}
	
	//
	// Used while building a resource from an AAF. Only state resources
	// can have a list of states, so this default implementation returns
	// an error.
	//
	void			add_state(
					tvslist*,
					bool) {
		Cstring err;
		err << "states don't exist for RES_resource " << name;
		throw(eval_error(err));
	}

	// USAGE CLAUSE PROCESSING

	//
	// The following three methods throw exceptions
	// because they only be used if overridden:
	//
	void			execute_set_clause(
					TypedValue&,
					mEvent*,
					bool	immediate);
	void			execute_reset_clause(
					mEvent*,
					bool	immediate);
	void			execute_use_clause(
					TypedValue&,
					mEvent*,
					bool	immediate);

	static dummyHistory&	emptyHist();

	// debug: dumps to stdout, using a specified number of spaces as indentation.
	void			dump(int prefix);

	void			abort() {}

	// for -C option.
	void			print_to_C(aoString &);
};


class RES_numeric: public RES_resource {
public:
	RES_numeric(
		RCsource&	p,
		Behavior&	T,
		const Cstring&	namein) 
		: RES_resource(p, T, namein),
			resHistory(this) {}
	~RES_numeric() {}

	virtual void		report_history() override {
		resHistory.report_history();
	}

	virtual TypedValue	nodeval2resval(
					value_node* node) override;

	RES_history resHistory;
	RES_history& get_history() override {
		return resHistory;
	}

				//
				// Used to set the "currentval" symbol to its
				// current value. Now, the method is invoked
				// whenever the current value is needed. Note
				// the ability to throw exceptions.
				//
	virtual void		evaluate_present_value(
					TypedValue&,
					const CTime_base& eval_time,
						// = EventLoop::current_time(),
					LimitContainer* lc = NULL) override;

				//
				// For currentval().
				//
	virtual void		evaluate_present_value(
					TypedValue&) override;

	void			eval_val_within_thread(
					TypedValue&) override;
	void			eval_val_within_thread(
					TypedValue&		returned_value,
					const CTime_base&	eval_time,
					LimitContainer*		lc) override;
};

class stateNodePLD {
public:
	stateNodePLD() : modelID(0) {}
	stateNodePLD(apgen::DATA_TYPE dt) : modelID(0), usedval(dt) {}
	stateNodePLD(long l, const TypedValue& v)
		: modelID(l), usedval(v) {}

	long		modelID;
	TypedValue	usedval;
};

class state_state {
public:
	/* Quote from C_global.H:
	 *
	 *	enum STATE_STATE {
	 *	STATE_STATE_RESET,
	 *	STATE_STATE_SET,
	 *	STATE_STATE_USE_SET_PENDING,
	 *	STATE_STATE_USE_RESET_PENDING };
	 */
	state_state(apgen::STATE_STATE s, apgen::DATA_TYPE dt)
		: currently(s), datatype(dt),
		currentval(dt), setval(dt) {}
	state_state(const state_state& s)
		: currently(s.currently),
		datatype(s.datatype),
		currentval(s.currentval),
		setval(s.setval) {}
	~state_state() {}

	apgen::STATE_STATE	currently;
	TypedValue		currentval;
	TypedValue		setval;	// only relevant if currently = STATE_STATE_SET
	apgen::DATA_TYPE	datatype;
	tlist<alpha_time, Cntnr<alpha_time, stateNodePLD> > end_uses;

	/* We need to capture the following data:
	 *
	 *    -	the last SET or RESET event - one always exists since
	 *	state histories start with a RESET
	 *
	 *    - the usage depth (my be implied, e. g., size of the list
	 *	described below)
	 *
	 *    - a list of pending end-of-usage events, if any; the number
	 *	of such events defines the "usage depth"
	 *
	 * What information is needed for each reference to a history node?
	 *
	 * Ideally we would like to trace nodes in the resource's history
	 * to the activities that caused these nodes to appear.
	 *
	 * This is probably not possible in the current version of APGen.
	 * Note that value_nodes (which make up the resource's history)
	 * do not have pointers to modeling events (instances of mEvent),
	 * but they do have a copy of the eventID of the mEvent that gave
	 * rise to them. Unfortunately, for immediate usages this eventID
	 * does not reflect the true "culprit". Fortunately, "use" clauses
	 * cannot be immediate!
	 *
	 * For the time being, let's capture the following:
	 *
	 *    -	the eventID
	 *
	 *    - the value associated with that usage
	 */
};

//
// Implements the state-valued kind of RES_resource object
//
class RES_state: public RES_resource {
public:
	RES_state(
		RCsource&	p,
		Behavior&	T,
		const Cstring&	namein);
	~RES_state();

	bool			requires_end_usage() const override {
		return true;
	}

	//
	// Prior to X.4.22, the history of a state resource reflected
	// the events that potentially affected the resource: 'set',
	// 'reset' and 'use'. However, because 'use' has priority over
	// 'set', it took non-trivial computations to compute the
	// current state from the history.
	//
	// Starting with X.4.22, the resource history only contains
	// nodes at which the resource value changes, unless the
	// "No Filtering" attribute is set, in which case a history
	// node is inserted for every single 'use', 'set' or 'reset'
	// event. In all cases, the payload of the history node is
	// the actual value at the time of the node's time key.
	//
	// To efficiently computate the current value of the resource,
	// a Miterator is used to keep track of 'use' events. The
	// Miterator is only used by the modeling thread; trailing
	// threads only need to consult the history, since it is no
	// longer necessary to carry out complex computations in order
	// to get the value of the resource at any time. Therefore, it
	// is sufficient to use a simple_res_miter to track 'use'
	// events.
	//
	// Note that each new 'use' event will have to create its
	// own list. The list will contain exactly two events, the
	// 'start use' event and the 'end use' event. These are the
	// head and tail of an slist<value_node>, respectively.
	// Fortunately, an slist only requires 4 64-bit words of
	// storage.
	//
	RES_history resHistory;

	RES_history& get_history() override {
		return resHistory;
	}

	simple_res_miter	usage_events;

	void			report_history() override {
		resHistory.report_history();
	}

	void			Add(value_node* tn, apgen::DATA_TYPE dt) {
		resHistory.Add(tn, dt);
	}

	//
	// for arrayed resources:
	//
	bool			theRangeIsTheDefault;

	vector<TypedValue>&	get_range() override {
		return rangevec;
	}
	map<void*, bool>	rangemap;
	vector<TypedValue>	rangevec;

	//
	// Fundamental "access method" that provides
	// the resource history to clients. The history
	// is returned as a List of value_nodes.
	// Typical client: RES_sys.
	//
	void	get_values(
			const CTime_base&	start_time,
			const CTime_base&	span,
			tlist<alpha_time, unattached_value_node, Rsource*>& result,
			bool			all_or_not,
			long&			returned_version
			) override {
		get_history().get_discrete_values(
			start_time,
			span,
			result,
			all_or_not,
			returned_version);
	}

	TypedValue		get_partial_history(
					CTime_base start_time,
					CTime_base span) override;

	state_state		get_resource_state_at(CTime_base given_time);

				//
				// Used to set the "value" symbol to its
				// current value. For currentval, use the
				// method without a time argument, below.
				//
	virtual void		evaluate_present_value(
					TypedValue&,
					const CTime_base& eval_time,
						// = EventLoop::current_time(),
					LimitContainer* lc = NULL) override;

				//
				// For currentval().
				//
	virtual void		evaluate_present_value(
					TypedValue&) override;

	//
	// Only used for TOL output of interpolated resources;
	// throw exception if used on a state resource.
	//
	virtual TypedValue	nodeval2resval(
					value_node* node) override {
		Cstring errs;
		errs << name << ": nodeval2resval() should only be used "
			"for numeric resources; this is a state resource.";
		throw(eval_error(errs));
	}

	void			add_state(
					tvslist* streamptr,
					bool isDefault) override;

				//
				// type returned should be one of:
				//
				// 	CONSUMABLE,
				// 	NONCONSUMABLE,
				// 	ABSTRACT,
				// 	STATE,
				// 	EXTERN
				//
	apgen::RES_CLASS	get_class() const {
		return apgen::RES_CLASS::STATE;
	}

	//
	// To support init_event::do_model():
	//
	void			initialize_value(
					init_event*) override;

	//
	// To support XMLTOL ingestion:
	//
	void			initialize_value(
					const CTime_base&) override;

				//
				// for SET
				// List contains TypedValues, one for each
				// parameter of the usage clause
				//
	void			execute_set_clause(
					TypedValue&,
					mEvent*,
					bool immediate = false) override;
	void			execute_reset_clause(
					mEvent*,
					bool immediate = false) override;

				//
				// for USE
				// List contains TypedValues, one for each
				// parameter of the usage clause
				//
	void			execute_use_clause(
					TypedValue&,
					mEvent*,
					bool immed = false) override;

	//
	// utype can be USE, SET, RESET:
	//
	void			append_to_history(
					TypedValue&		value,
					CTime_base		start_event_time,
					CTime_base		end_event_time,
					long			start_event_id,
					long			end_event_id,
					apgen::USAGE_TYPE	utype) override;

	//
	// utype can be SET, RESET (USE requires 2 events and is not allowed
	// in immediate mode; the method will throw an exception if utype is
	// USE):
	//
	void			immediately_append_to_history(
					TypedValue&		value,
					apgen::USAGE_TYPE	utype) override;

	void			eval_val_within_thread(
					TypedValue&) override;
	void			eval_val_within_thread(
					TypedValue&		returned_value,
					const CTime_base&	eval_time,
					LimitContainer*		lc) override;
};

//
// Implements the settable-valued kind of RES_resource object
//
class RES_settable: public RES_resource {
public:
	RES_settable(
		RCsource&	p,
		Behavior&	T,
		const Cstring&	namein)
		: RES_resource(p, T, namein),
			resHistory(this /* , new num_res_historyWizard */ ) {}
	~RES_settable() {}

	bool			requires_end_usage() const override {
		return false;
	}
	//
	// Essentially, resHistory is a tlist of value_nodes
	// that track cumulative usage (continuous) or
	// actual (settable, state) values of the resource. These
	// are used in evaluating currentval() via the
	// evaluate_present_value() method.
	//
	RES_history resHistory;
	RES_history& get_history() override {
		return resHistory;
	}

	virtual TypedValue	nodeval2resval(
					value_node* node) override {
		return node->get_value();
	}

	void		report_history() override {
		resHistory.report_history();
	}

	void		Add(
				const CTime_base&,
				const TypedValue& val,
				long event_id);
	void		UpdateLastSkippedNode(
				const CTime_base&,
				const TypedValue& val,
				long event_id);

	//
	// Fundamental "access method" that provides
	// the resource history to clients. The history
	// is returned as a List of value_nodes.
	// Typical client: RES_sys.
	//
	void	get_values(
			const CTime_base&	start_time,
			const CTime_base&	span,
			tlist<alpha_time, unattached_value_node, Rsource*>& result,
			bool			all_or_not,
			long&			returned_version
			) override {
		get_history().get_settable_values(
				start_time,
				span,
				result,
				all_or_not,
				returned_version);
	}

	TypedValue	get_partial_history(
				CTime_base start_time,
				CTime_base span) override;

	//
	// Used to set the "value" symbol to its
	// current value. For currentval, use the
	// method without a time argument, below.
	//
	virtual void	evaluate_present_value(
				TypedValue&,
				const CTime_base& eval_time,
				LimitContainer* lc = NULL) override;

	//
	// For currentval().
	//
	virtual void	evaluate_present_value(
				TypedValue&) override;

	//
	// type returned should be one of:
	//
	// 	CONSUMABLE,
	// 	NONCONSUMABLE,
	// 	ABSTRACT,
	// 	STATE,
	// 	SETTABLE,
	// 	EXTERN
	//
	apgen::RES_CLASS	get_class() const {
		return apgen::RES_CLASS::SETTABLE;
	}

	void		initialize_value(
				init_event*) override;
	void		initialize_value(
				const CTime_base&) override;

	//
	// for SET
	// List contains TypedValues, one for each
	// parameter of the usage clause
	//
	void		execute_set_clause(
				TypedValue&,
				mEvent*,
				bool immediate = false) override;

	//
	// utype must be SET:
	//
	void		append_to_history(
				TypedValue&		value,
				CTime_base		start_event_time,
				CTime_base		end_event_time,
				long			start_event_id,
				long			end_event_id,
				apgen::USAGE_TYPE	utype) override;

	//
	// utype can be SET, RESET (USE requires 2 events and is not allowed
	// in immediate mode; the method will throw an exception if utype is
	// USE):
	//
	void		immediately_append_to_history(
				TypedValue&		value,
				apgen::USAGE_TYPE	utype) override;

	virtual void	evaluate_profile(
				EvalTiming::Strategy,
				CTime_base,
				TypedValue&) override;
	virtual aafReader::single_precomp_res*	get_precomp_res() {
	    return NULL;
	}
};

class RES_precomputed: public RES_settable {
public:
    RES_precomputed(
		RCsource&	p,
		Behavior&	T,
		const Cstring&	namein,
		aafReader::precomp_container* pc)
    : RES_settable(p, T, namein) {

	//
	// We assume precomputed resources only have one level of indices
	//
	aafReader::single_precomp_res* pre_res = pc->payload.find(indices[0]);
	assert(pre_res);
	pc->payload.remove_node(pre_res);
	my_precomp_res_list << pre_res;
    }
    ~RES_precomputed() {}

    //
    // For adaptation-defined resources, this is an empy list.
    // For precomputed resources, the list has 1 element, which
    // is the 'precomputed resource' extracted from a
    // pre-computed AAF at parsing time. The single_precomp_res
    // class contains the entire list of resource values (and
    // resource derivative values) at all sampling times,
    // covering the entire range of SPICE-supplied SPK kernels.
    //
    aafReader::precomp_res_list	my_precomp_res_list;
    aafReader::single_precomp_res*	get_precomp_res() override {
	return my_precomp_res_list.first_node();
    }

    //
    // Used to set the "value" symbol to its
    // current value. For currentval, use the
    // method without a time argument, below.
    //
    virtual void	evaluate_present_value(
				TypedValue&,
				const CTime_base& eval_time,
				LimitContainer* lc = NULL) override;

    //
    // For currentval().
    //
    virtual void	evaluate_present_value(
				TypedValue&) override;
};


//
// First of 2 basic types of numeric (continuous) resources.
//
class RES_nonconsumable: public RES_numeric {
public:
	RES_nonconsumable(
			RCsource&	p,
			Behavior&	T,
			const Cstring&	namein)
		: RES_numeric(p, T, namein) {}
	~RES_nonconsumable() {}

	bool			requires_end_usage() const override {
		return true;
	}
	void			initialize_value(
					init_event*) override;
	void			initialize_value(
					const CTime_base&) override;
	void			execute_use_clause(
					TypedValue&,
					mEvent*,
					bool immed = false) override;


	//
	// Fundamental "access method" that provides the resource history to clients.
	// The history is returned as a List of value_nodes. Typical client: RES_sys.
	//
	void			get_values(
					const CTime_base&	start_time,
					const CTime_base&	span,
					tlist<alpha_time, unattached_value_node, Rsource*>& result,
					bool			all_or_not,
					long&			returned_version) override {
		get_history().get_numeric_values(
			start_time,
			span,
			result,
			all_or_not,
			returned_version);
	}

	TypedValue		get_partial_history(
					CTime_base start_time,
					CTime_base span) override;

	void			Add(
					value_node* tn,
					apgen::DATA_TYPE dt) {
		resHistory.Add(tn, dt);
	}

	//
	// Stores the freshly evaluated value into resource history.
	// In the old days, the event was propagated to dependent
	// resources as needed. Propagation consumed horrendous
	// amounts of memory. Now, propagation does not take place;
	// it is the responsibility of dependent resources to examine
	// their profile to find out any history events in resources
	// they depend on, and include such events in reports to clients.
	//
	// Since this is a nonconsumable resource, the only "legal"
	// type of usage is USE, and it requires 2 sets of event parameters:
	//
	void			append_to_history(
					TypedValue&		consumed_value,
					CTime_base		start_event_time,
					CTime_base		end_event_time,
					long			start_event_id,
					long			end_event_id,
					apgen::USAGE_TYPE	uType) override;


	//
	// Since this is a nonconsumable resource, the only "legal"
	// type of usage is USE, which requires 2 event. However,
	// immediate usages are restricted to single events, so
	// this method throws an exception for nonconsumable resources: 
	//
	void			immediately_append_to_history(
					TypedValue&		value,
					apgen::USAGE_TYPE	utype) override;

	//
	// type returned should be one of:
	// 	CONSUMABLE,
	// 	NONCONSUMABLE,
	// 	ABSTRACT,
	// 	STATE,
	// 	EXTERN
	//
	apgen::RES_CLASS	get_class() const {
		return apgen::RES_CLASS::NONCONSUMABLE;
	}
};

//
// Second of 2 basic types of numeric (continuous) resources.
//
class RES_consumable: public RES_numeric {
public:
	RES_consumable(RCsource& p, Behavior& T, const Cstring &namein)
		: RES_numeric(p, T, namein) {}
	~RES_consumable() {}

	bool		requires_end_usage() const override {
		return false;
	}

	//
	// Fundamental "access method" that provides the resource history to clients.
	// The history is returned as a List of value_nodes. Typical client: RES_sys.
	//
	void		get_values(	const CTime_base&	start_time,
					const CTime_base&	span,
					tlist<alpha_time, unattached_value_node, Rsource*>& result,
					bool			all_or_not,
					long&			returned_version
					) override {
		get_history().get_numeric_values(
				start_time,
				span,
				result,
				all_or_not,
				returned_version);
	}

	TypedValue	get_partial_history(
				CTime_base start_time,
				CTime_base span) override;

	void		Add(
					value_node* tn,
					apgen::DATA_TYPE dt) {
		resHistory.Add(tn, dt);
	}

	void		initialize_value(
					init_event*) override ;
	void		initialize_value(
					const CTime_base&) override ;
	void		execute_use_clause(
					TypedValue&,
					mEvent*,
					bool immed = false) override ;

	//
	// Stores the freshly evaluated value into resource history.
	// In the old days, the event was propagated to dependent
	// resources as needed. Propagation consumed horrendous
	// amounts of memory. Now, propagation does not take place;
	// it is the responsibility of dependent resources to examine
	// their profile to find out any history events in resources
	// they depend on, and include such events in reports to clients.
	//
	// Since this is a consumable resource, the only "legal"
	// type of usage is USE:
	//
	void		append_to_history(
					TypedValue&		consumed_value,
					CTime_base		start_event_time,
					CTime_base		end_event_time,
					long			start_event_id,
					long			end_event_id,
					apgen::USAGE_TYPE	uType) override;

	//
	// Since this is a consumable resource, the only "legal"
	// type of usage is USE:
	//
	void		immediately_append_to_history(
					TypedValue&		value,
					apgen::USAGE_TYPE	utype) override;

	//
	// type returned should be one of:
	// 	CONSUMABLE,
	// 	NONCONSUMABLE,
	// 	ABSTRACT,
	// 	STATE
	//
	apgen::RES_CLASS get_class() const {
		return apgen::RES_CLASS::CONSUMABLE;
	}
};

#endif /* RES_DEF_H */
