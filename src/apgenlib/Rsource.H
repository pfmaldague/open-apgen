#ifndef _R_SOURCE_H_
#define _R_SOURCE_H_

#include <vector>

#include "ThreadIntfc.H"
#include "AP_exp_eval.H"
#include "accessorkey.H"
#include "res_multiterator.H"

class RCsource;
class RES_resource;
class init_event;
class res_container_object;
class Constraint;


namespace pEsys {
class Program;
class Usage;
}

extern thread_local int thread_index;

//
// NOTE: could use BPnode instead of defining methods explicitly
//
class RESptr: public baseC<alpha_void, RESptr>, public p2b<Rsource> {
public:
	long		_i;
	Rsource*	payload;
	RESptr() : baseC<alpha_void, RESptr>(0L), payload(NULL), _i(0L) {}
	RESptr(const RESptr& r) : baseC<alpha_void, RESptr>(r), payload(r.payload), _i(0L), p2b<Rsource>(r) {}
	RESptr(Rsource* r, long k) : baseC<alpha_void, RESptr>(r), payload(r), _i(k), p2b<Rsource>(r) {}
	~RESptr();

	RESptr*				copy() { return new RESptr(*this); }
	RESptr*				get_this() { return this; }
	const RESptr*			get_this() const { return this; }
	const Cstring&			get_key() const; // { return payload->get_key(); }
	long				get_res_index() const { return _i; }
	virtual void			remove(bool S) { if(S) baseC<alpha_void, RESptr>::temporarily_remove_from_list();
							else if(list) list->remove_node(this); }
	virtual bool			reacts_to_time_events() const { return false; }
	virtual void			re_insert() { baseC<alpha_void, RESptr>::re_insert_in_list(); }
	virtual const char*		className() const { return "RESptr"; }
	virtual void*			voidThis() { return this; }
};

//
// "Resource container" base class - payload for RCsource
//
class resContPLD {
public:

	//
	// Constructor section
	//
	// Two constructors are provided: one for arrays of resources, one for
	// simple resources. The copy constructor is mandated by copy() but not
	// used. The destructor is boilerplate.
	//

	//
	// For arrays
	//
	resContPLD(
		smart_ptr<res_container_object>&	obj,
		Behavior&				T,
		apgen::RES_CLASS			c,
		apgen::RES_CONTAINER_TYPE		simple_or_array,
		apgen::DATA_TYPE			dt)
		: Object(obj),
			myClass(c),
			Type(T),
			datatype(dt),
			cardinality(simple_or_array)
		{
	}

	//
	// For simple resources
	//
	resContPLD(
		const resContPLD&	r,
		Behavior&		T,
		apgen::DATA_TYPE	dt)
		: Object(r.Object),
			Type(T),
			myClass(r.myClass),
			datatype(dt),
			cardinality(r.cardinality)
		{
	}
	virtual ~resContPLD() {
	}

	//
	// General client-oriented methods
	//
	// These methods allow clients to 'discover' e. g. how to
	// use the resource, whether it is simple, what the indices
	// are etc.
	//

	apgen::RES_CLASS		myClass;
	smart_ptr<res_container_object>	Object;
	Behavior&			Type;
	virtual void			clear_profiles() {
	}

	//
	// Used in consolidation
	//
	virtual apgen::RETURN_STATUS	complete_dependency_list(
						tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& dependents,
						tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& ok_list,
						tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& resource_chain,
						Cstring& any_errors) = 0;

	bool				is_associative() {
		return myClass == apgen::RES_CLASS::ASSOCIATIVE
			|| myClass == apgen::RES_CLASS::ASSOCIATIVE_CONSUMABLE
			|| myClass == apgen::RES_CLASS::INTEGRAL;
	}

	// parsing and building support
	virtual void			set_source_to(RCsource*) = 0;

	//
	// General client-oriented data members
	// These members provide the basic objects needed by
	// native resources. Not clear that external resources need them.
	//
	apgen::RES_CONTAINER_TYPE	cardinality;

	//
	// Dependency documentation
	//
	tlist<alpha_void, Cntnr<alpha_void, RCsource*> >	ptrs_to_containers_used_in_profile;
	stringtlist						names_of_containers_used_in_profile;
	tlist<alpha_void, Cntnr<alpha_void, RCsource*> >	containers_whose_profile_invokes_this;

	apgen::DATA_TYPE		datatype;

	friend class Rsource;
};

//
// The "resource container": Contains one (scalar)
// or several (arrayed) resource(s)
//
class RCsource: public baseC<alpha_string, RCsource> {
public:
	typedef Cntnr<alpha_string, RCsource*>	container_tag;
	typedef tlist<alpha_string, container_tag> container_tags;
	typedef slist<alpha_string, container_tag> container_tagss;

	typedef Cntnr<alpha_void, RCsource*>	container_ptr;
	typedef tlist<alpha_void, container_ptr> container_ptrs;
	typedef slist<alpha_void, container_ptr> container_ptrss;

	RCsource(
			const Cstring& A,
			resContPLD* rcp)
		: baseC<alpha_string,RCsource>(A),
			use_method_count(0),
			payload(rcp) {
		if(payload) {
			payload->set_source_to(this);
		}
	}
	RCsource(const RCsource& rcs); // offline because it needs a concrete resContPLD factory
	~RCsource() {
		if(list) {
			list->remove_node(this);
		}
		delete payload;
		payload = NULL;
	}

	resContPLD*			payload;
	int				use_method_count;

	virtual RCsource*		copy() {
		return new RCsource(*this);
	}
	virtual RCsource*		get_this() {
		return this;
	}
	virtual const RCsource*		get_this() const {
		return this;
	}
	virtual void			clear_profiles() {
		payload->clear_profiles();
	}

	//
	// consolidation support
	//
	virtual apgen::RETURN_STATUS	complete_dependency_list(
			tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& dependents,
			tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& ok_list,
			tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& resource_chain,
			Cstring&	any_errors) {
		return payload->complete_dependency_list(
				dependents,
				ok_list,
				resource_chain,
				any_errors);
	}

	virtual bool		is_associative() {
		return payload->is_associative();
	}

				// self-explanatory
	bool			is_simple() {
		return  payload->cardinality == apgen::RES_CONTAINER_TYPE::SIMPLE ? true : false;
	}
				// self-explanatory
	bool			is_array() {
		return  payload->cardinality == apgen::RES_CONTAINER_TYPE::ARRAY ? true : false;
	}
	apgen::DATA_TYPE&	get_datatype() const {
		return payload->datatype;
	}
	long int		get_resource_count() const {
		return payload->Type.SubclassTypes.size();
	}
	long int		get_index_count() const;


	//
	// From C_global.H, RES_CLASS is an enum containing
	// the following items:
	//
	// 	UNDEFINED,			(does not belong here)
	//	CONSUMABLE,
	//	NONCONSUMABLE,
	//	SETTABLE,
	//	STATE,
	//	EXTERNAL,			(obsolete)
	//	ASSOCIATIVE,			(obsolete)
	//	ASSOCIATIVE_CONSUMABLE,		(obsolete)
	//	INTEGRAL,			(obsolete)
	//	SIGNAL
	//
	apgen::RES_CLASS	get_class() const {
		return payload->myClass;
	}

	//
	// Constraint support
	//
	void			add_constraint(Constraint* cons);
	tlist<alpha_void, Cntnr<alpha_void, Constraint*> > any_constraints;

	//
	// Static / Factory section
	//
	static tlist<alpha_string, RCsource>&	resource_containers();
	static tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& initialization_list();
};

//
// The purpose of this class is to make it possible for the symbol-changing
// functions in the ACT_req class to inform the resource displays in the GUI
// (via the notify_clients method) that the displays of associative resources
// need refreshing. This takes place in ACT_req::notify_assoc_res_if_any().
// Activities that affect associative resources know what these resources
// are because they hold AssociativeFloat nodes that point back to them.
//
class rsource_ext {
public:
	rsource_ext() {}
	virtual ~rsource_ext() {}

	virtual void notify_clients(void *) = 0;
	virtual CTime_base& get_resolution() = 0;
};

class res_container_object: public behaving_object {
public:
	RCsource*	res;
	res_container_object(
					RCsource*	r,
					Cstring		integrand = Cstring());

	res_container_object(const res_container_object& c) = delete;

	~res_container_object();

	//
	// The items in this vector are indexed in a manner
	// consistent with the SubclassTypes of the container's
	// behavior type
	//
	vector<Rsource*>	array_elements;
	Rsource*		get_concrete_res() const {
		return NULL;
	}
	RCsource*		get_res_container() const {
		return res;
	}
	virtual Cstring		get_id() const {
		return res->get_key();
	}
	const Cstring&		get_key() const;
	bool			print(
					aoString& s,
					const char* prefix) const {
		bool b;
		s << prefix << "[\"type\" = " << addQuotes(Task.full_name())
			<< "]";
                return true;
        }
};

class concrete_res_object: public behaving_object {
private:
	Rsource*	res;
public:
	//
	// do we need an enumeration of always-present
	// attributes? Let's start with only a few
	// essential ones.
	//
	// Coordinate with the following (in Behavior.C):
	//
	//	Behavior::add_subtypes()		
	//	concrete_res_object::concrete_res_object()
	//

	typedef enum {
		ENABLED = 0,
		THIS = 1,
		ID = 2,
		INDICES = 3
	} variable_name;

	concrete_res_object(
		Rsource*	r);

	concrete_res_object(const concrete_res_object& c) = delete;

	~concrete_res_object() {
	}

	Rsource*	get_concrete_res() const {
		return res;
	}
	const Cstring&	get_key() const;
	virtual Cstring		get_id() const {
		return get_key();
	}

	virtual int	deref() {
		ref_count--;
		if(ref_count == 1) {

			//
			// will prevent ref_count from dropping to zero prematurely:
			//
			ref_count = 3;
			operator[](THIS).get_object().dereference();
			ref_count = 0;
		}
		return ref_count;
	}


	bool		print(
				aoString& s,
				const char* prefix) const {
		bool b;
		s << prefix << "[\"type\" = " << addQuotes(Task.full_name()) << "]";
                return true;
        }

};

//
// For numeric and state resources
//
class RES_history {
public:
	typedef prio_time	timeKey;
	typedef value_node	nodeClass;
	typedef slist<timeKey, nodeClass, Rsource*> Rslist;

	RES_history(Rsource* r);
	RES_history(const RES_history& RH);
	~RES_history() {}

	//
	// An option for resource read from a file or
	// inherited from a previous REMODEL or SCHEDULE
	// command. The Frozen flag is set by a FREEZE
	// script command. The history of a frozen resource
	// can _never_ be touched by the adaptation - not
	// in modeling, not in scheduling.
	//
	bool					Frozen;

	//
	// The Imported flag indicates a resource that
	// was read from an XMLTOL file. If the flag is
	// set and a scheduling command is issued with
	// the INCREMENTAL flag set, the resource history
	// will not be cleared at the beginning of the
	// run, so it can be used for scheduling purposes.
	// The flag will be cleared at the end of the
	// modeling run, at which point the resource
	// becomes an ordinary resource.
	//
	bool					Imported;
	Rsource*				resource_parent;

	//
	// Because a resource may depend on other resources
	// through a complex profile, and because the histories
	// of these dependencies are _not_ including in this
	// resource's history, iterating over a history requires
	// a multiterator that combines events affecting this
	// resource directly as well as events that affect all
	// the resources on which this one depends.  The
	// multiterator stored in RES_history is basically the
	// one that provides that capabability.
	//
	simple_res_miter*			get_the_multiterator();
	virtual void				report_history();
	apgen::DATA_TYPE&			get_datatype();
	const long				get_res_version() {
		return theVersion().get_res_version();
	}
	Rsource*				get_resource() const {
		return resource_parent;
	}
	version_changer&			theVersion();
	long					change_count;
	virtual bool				isImported() {
		return Imported;
	}
	virtual void				setImported(bool s) {
		Imported = s;
	}
	virtual bool				isFrozen() {
		return Frozen;
	}
	void					Freeze() {
		Frozen = true;
	}
	void					Unfreeze() {
		Frozen = false;
	}
	void					change_version() {
		theVersion().increment();
	}

			//
			// finds the latest node whose CTime is at or
			// before the given time
			//
	value_node*	latestHistNodeAtOrBefore(
				const CTime_base& event_time) {
		prio_time	temptime(event_time, mEvent::currentEventID + 1);
		return historyList.find_at_or_before(temptime);
	}

			//
			// finds the latest node whose CTime is before given time
			//
	value_node*	latestHistNodeBefore(
				const CTime_base& event_time) {
		prio_time temptime(event_time, 0);
		return historyList.find_before(temptime);
	}

			//
			// Same as the above two methods, but knowing
			// that we have a list iterator at almost the
			// right time thanks to the modeling loop
			//
	value_node*	latestHistNodeAtOrBeforeModelTime(
					const CTime_base& event_time,
					Rslist::iterator* iter_at_model_time);
	value_node*	latestHistNodeBeforeModelTime(
					const CTime_base& event_time,
					Rslist::iterator* iter_at_model_time);
	value_node*	firstNode() {
		return historyList.first_node();
	}

	//
	// For the convenience of code that verifies that 
	// history::Add() does not interfere with the
	// constraint-modeling thread:
	//
	void*		history_list_as_void_ptr() {
		return (void*)&historyList;
	}

	//
	// for making plots of state resources:
	//
	virtual void	get_discrete_values(
				const CTime_base&		start,
				const CTime_base&		span,
				tlist<	alpha_time, 
					unattached_value_node,
					Rsource*>&		l,
				bool				all_vals,
				long&				theVersion
				);

	//
	// for exploring state resources histories as arrays:
	//
	void		get_partial_state_history(
				const CTime_base&		start,
				const CTime_base&		span,
				tlist<	alpha_time, 
					unattached_value_node,
					Rsource*>&		l,
				bool				all_vals
				);

	//
	// for settable resources:
	//
	virtual void	get_settable_values(
				const CTime_base&		start,
				const CTime_base&		span,
				tlist<	alpha_time, 
					unattached_value_node,
					Rsource*>&		l,
				bool				all_vals,
				long&				theVersion
				);

	//
	// for exploring settable resources histories as arrays:
	//
	void		get_partial_settable_history(
				const CTime_base&		start,
				const CTime_base&		span,
				tlist<	alpha_time, 
					unattached_value_node,
					Rsource*>&		l,
				bool				all_vals
				);

	//
	// for numeric resources:
	//
	virtual void	get_numeric_values(
				const CTime_base&		start,
				const CTime_base&		span,
				tlist<	alpha_time, 
					unattached_value_node,
					Rsource*>&		l,
				bool				all_vals,
				long&				theVersion
				);

	//
	// for exploring state resources histories as arrays:
	//
	void		get_partial_numeric_history(
				const CTime_base&		start,
				const CTime_base&		span,
				tlist<	alpha_time, 
					unattached_value_node,
					Rsource*>&		l,
				bool				all_vals
				);

	void		Add(value_node*, apgen::DATA_TYPE);
	virtual long	get_length() {
		return historyList.get_length();
	}

	virtual void	clear() {
		historyList.clear();
		lastSkippedNodeContainer.clear();
		theVersion().increment();
	}

	virtual void	clear_skipped_nodes_list() {
		lastSkippedNodeContainer.clear();
	}

	//
	// This method is used whenever a Miterator is loaded with this
	// specific resource history:
	//
	void		addToIterator(
				Miterator_base<Rslist, value_node>&	iter,
				int					prio,
				bool					store);

	//
	// For settable resources only:
	//
	// This list contains at most a single element, which is the last
	// history node that would have been added to the history if
	// there was no filtering. The reason for keep this node around is
	// that it needs to be added to the history _before_ a new node
	// representing a significant jump is added to the history.
	//
	slist<prio_time, value_node, Rsource*>		lastSkippedNodeContainer;
private:
	multilist<prio_time, value_node, Rsource*>	historyList;
	simple_res_miter				the_multiterator;
};

//
// Resource base class
//
class Rsource {
public:

	//
	// constructor/destructor section
	//
	// The constructor (there is only one) is pretty "bland": it
	// initializes tlists, pointers and nothing else. In the code
	// that parses adaptation files, resource objects are created,
	// then the data members (lists etc.) are filled in as necessary.
	//

	Rsource() = delete;

	//
	// constructor that takes the name of the resource
	//
	Rsource(
		RCsource&		rc,
		Behavior&		T,
		const Cstring&		s,
		apgen::RES_CLASS	rclass)
			: name(s),
				myClass(rclass),
				Type(T),
				theProfile(true),
				parent_container(rc),
				derivedResource(NULL),
				evaluation_priority(0) {
		if(parent_container.is_array()) {
			assert(T.parent);
			map<Cstring, int>::iterator kk = T.parent->SubclassFlatMap.find(s);
			assert(kk != T.parent->SubclassFlatMap.end());
			indices = T.parent->SubclassIndices[kk->second];
		}

		//
		// Make sure you create this object AFTER setting indices!!
		//
		Object.reference(new concrete_res_object(this));

		for(int i = 0; i < thread_intfc::MITER_THREADS; i++) {
		    active_miterators[i]
			= new slist<alpha_void, Cntnr<alpha_void, miterator_data> >;
		}
		properties.resize((int)Property::how_many_properties);
	}
	Rsource(const Rsource& r) = delete;

	//
	// Do-nothing (almost) destructor
	//
	virtual ~Rsource() {
 		if(derivedResource) {
			delete derivedResource;
		}
		for(int i = 0; i < thread_intfc::MITER_THREADS; i++) {
			delete active_miterators[i];
		}
	}


	//
	// "get" methods
	//
	tlist<alpha_time, Cntnr<alpha_time, parsedExp> >& getTheProfile() {
		return theProfile;
	}

	RCsource*		get_container() {
		return &parent_container;
	}

	apgen::DATA_TYPE&	get_datatype() {
		return parent_container.get_datatype();
	}

	int&			get_evaluation_priority() {
		return evaluation_priority;
	}

	const parsedProg&	get_attribute_program() const {
		return Object->Task.prog;
	}

	//
	// Action programs are exercised whenever the resource
	// is used. There are up to 4 of them: one for begin of use,
	// one for end of use, one for setting, one for resetting.
	// As of now they only work with state resources. Added
	// to facilitate the task of adapting APGEN to MER.
	//
	// NOTE: I believe that as of June, 2019 this
	// has not been implemented in APGenX.
	//
	// virtual vector<smart_ptr<pEsys::Program> >* get_action_program() {
	// 	return NULL;
	// }

	//
	// overridden in RES_state
	//
	virtual vector<TypedValue>&	get_range() {
		return default_range();
	}

	EvalTiming::Strategy		get_strategy() {
		Cntnr<alpha_void, miterator_data>* N;
		if(!(N = active_miterators[thread_index]->last_node())) {

			//
			// Not an error
			//
			// Cstring errs;
			// errs << "Resource " << name
			// 	<< " cannot get strategy - modeling "
			// 	<< "is not in progress.";
			// throw(eval_error(errs));
			//
			return EvalTiming::NORMAL;
		}
		return N->payload.miter.strategy;
	}

	int				get_usage_count() const {
		return parent_container.use_method_count;
	}

	//
	// "set" methods
	//
	void			set_strategy(EvalTiming::Strategy s) {
		Cntnr<alpha_void, miterator_data>* N;
		if(!(N = active_miterators[thread_index]->last_node())) {
			Cstring errs;
			errs << "Resource " << name
				<< " cannot set strategy - modeling "
				<< "is not in progress.";
			throw(eval_error(errs));
		}
		N->payload.miter.strategy = s;
	}

	//
	// "query" methods
	//
	virtual bool		requires_end_usage() const = 0;

	bool			is_associative() {
		return parent_container.is_associative();
	}

	virtual bool		is_hidden() const = 0;

	//
	// Useful for TOL writing
	//
	virtual TypedValue	nodeval2resval(
					value_node* node) = 0;

	//
	// "compute" methods
	//

	//
	// overridden by RES_settable, which has a trivial profile:
	//
	virtual void		evaluate_profile(
					EvalTiming::Strategy,
					CTime_base,
					TypedValue&);


	virtual void		append_to_history(
					TypedValue&	val,
					CTime_base	start_event_time,
					CTime_base	end_event_time,
					long		start_event_id,
					long		end_event_id,
					apgen::USAGE_TYPE utype) = 0;

	//
	// utype can only be SET or RESET. A time
	// argument is not supplied because 'now'
	// is always used.
	//
	virtual void		immediately_append_to_history(
					TypedValue&	val,
					apgen::USAGE_TYPE utype) = 0;

	virtual void		eval_immediate_resusage(
					pEsys::Usage*	UsageClause,
					RES_resource*	nextres,
					behaving_element c) {
		Cstring s("This object cannot support the "
			"eval_immediate_resusage method.");
		throw(eval_error(s));
	}

    class miterator_data {
	public:
	    miterator_data() = delete;
	    miterator_data(
			Miterator_base<slist<prio_time, value_node, Rsource*>, value_node>& R, 
			slist<prio_time, value_node, Rsource*>::iterator* I)
		: miter(R), iter(I) {}
	    miterator_data(const miterator_data&) = default;
	    ~miterator_data() = default;

	    Miterator_base<slist<prio_time, value_node, Rsource*>, value_node>&	miter;
	    slist<prio_time, value_node, Rsource*>::iterator*			iter;
    };

    class iterator {
        public:
	    iterator();
	    iterator(const iterator&) = delete;
	    ~iterator() {}

	    Rsource*					next(bool reset = false);

	    RCsource*					container;
	    int						res_index;
	    slist<alpha_string, RCsource>::iterator	iter;
    };

	void				push_miterator_data(
			Miterator_base<
				slist<alpha_time, mEvent, Rsource*>,
				mEvent>*,
			slist<alpha_time, mEvent, Rsource*>::iterator*) {
	}

	void				push_miterator_data(
			Miterator_base<
				slist<prio_time, value_node, Rsource*>,
				value_node>* M,
			slist<prio_time, value_node, Rsource*>::iterator* I) {

		(*active_miterators[thread_index])
			<< new Cntnr<alpha_void, miterator_data>(
				(void*)M,
				miterator_data(*M, I));
	}

	miterator_data*			peek_at_miterator_data() {
		return active_miterators[thread_index]->last_node() ?
			&active_miterators[thread_index]->last_node()->payload :
			NULL;
	}

	void				pop_miterator_data(
			Miterator_base<
				slist<alpha_time, mEvent, Rsource*>,
				mEvent>*) {
	}
	void				pop_miterator_data(
			Miterator_base<
				slist<prio_time, value_node, Rsource*>,
				value_node>* N) {

		Cntnr<alpha_void, miterator_data>* last
			= active_miterators[thread_index]->last_node();
		if(N != &last->payload.miter) {
			Cstring errs;
			errs << "Resource " << name << " is told that iterator "
				<< N->get_name() << " is going out of scope, but "
				<< "current miterator is "
				<< (last ? last->payload.miter.get_name() : "NULL")
				<< "\n";
			throw(eval_error(errs));
		}
		if(last) {
			active_miterators[thread_index]->remove_node(last);
			delete last;
		}
	}

	//
	// These functions support modeling. For example, methods
	// are provided for setting, resetting, and using state
	// resources.
	//

	//
	// method that executes a "set foo()" for
	// state or settable resource foo
	//
	virtual void			execute_set_clause(
						TypedValue&,
						mEvent*,
						bool immediately = false) = 0;
	virtual void			execute_reset_clause(
						mEvent*,
						bool immediately = false) = 0;
	virtual void			execute_use_clause(
						TypedValue&,
						mEvent*,
						bool immediately = false) = 0;

	//
	// for modeling/scheduling:
	//
	virtual void			initialize_value(
						init_event*) = 0;

	//
	// for XML TOL ingestion:
	//
	virtual void			initialize_value(
						const CTime_base&) = 0;

	//
	// Parsing support
	//
	virtual void			add_state(
						tvslist*,
						bool is_default) = 0;
	virtual void			addToTheProfile(
						tlist<	alpha_time,
							Cntnr<alpha_time, parsedExp> >& prof) = 0;

	//
	// Debugging support
	//
	virtual void			dump(int prefix) = 0;

	virtual void			report_history() = 0;

	virtual void		evaluate_present_value(
						TypedValue&		returned_value,
						const CTime_base&	eval_time,
						LimitContainer*		container = NULL) = 0;
	virtual void		evaluate_present_value(
						TypedValue&		returned_value) = 0;
	virtual void		eval_val_within_thread(
						TypedValue&		returned_value,
						const CTime_base&	eval_time,
						LimitContainer*		container = NULL) {
		evaluate_present_value(
				returned_value,
				eval_time,
				container);
	}
	virtual void		eval_val_within_thread(
						TypedValue&		returned_value) {
		evaluate_present_value(
				returned_value);
	}

				//
				// Returns the number of nodes in the resource history
				//
	virtual long		get_history_size() = 0;
	virtual RES_history&	get_history() = 0;

	virtual const long	get_res_version() {
		return get_history().get_res_version();
	}
	virtual bool		isFrozen() {
		return get_history().isFrozen();
	}
	virtual bool		isImported() {
		return get_history().isImported();
	}

	//
	// get_values() is the main access function used by
	// resource display classes. Parameters:
	//
	//    param a		start_time
	//    param b		time_span
	//    param L		list of value_nodes
	//    param all_vals	want all values?
	//    param theVersion	helps client figure out whether its values are stale
	//
	virtual void		get_values(
					const CTime_base&	a,
					const CTime_base&	b,
					tlist<alpha_time, unattached_value_node, Rsource*>& L,
					bool			all_vals,
					long&			theVersion
			  		) = 0;

	virtual TypedValue	get_partial_history(
					CTime_base	start,
					CTime_base	span) = 0;

	// NON-PURE VIRTUAL

	//
	// This query must be answered by derived classes
	//
	apgen::RES_CLASS	get_class() const {
		return myClass;
	}
	virtual bool		is_consumable() {
		return false;
	}

	//
	// DATA MEMBERS
	//

	Cstring				name;

	//
	// The "class" of this resource
	//
	Behavior&			Type;

	//
	// The "instance" of this resource (it's a singleton)
	//
	behaving_element		Object;	

	//
	// reference to the parent container; always present
	//
	RCsource&			parent_container;

	vector<bool>			properties;

	//
	// For efficient evaluation of resource attributes and other variables.
	// Attributes are evaluated during the consolidation phase. The Boolean
	// has_xxxx in which xxxx is an attribute means "attribute xxxx is defined
	// and its value is 'true'". So, if the attribute is defined but its value
	// is 'false', has_xxxx is set to false.
	//
	// For variables such as oldval, the Boolean has_oldval means that the
	// variable has been declared, and it can therefore be used in a resource
	// program.
	//
	enum class Property: int {
		has_interpolation	= 0,
		has_oldval		= 1,
		has_newval		= 2,
		has_hidden		= 3,
		has_subsystem		= 4,
		has_min_abs_delta	= 5,
		has_min_rel_delta	= 6,
		has_maximum		= 7,
		has_minimum		= 8,
		has_nofiltering		= 9,
		has_allowed_users	= 10,
		has_modeling_only	= 11,

		//
		// make sure that
		//   (1) this is the last item, and
		//   (2) the value is correct
		//
		how_many_properties	= 12
	};

	tlist<alpha_time, Cntnr<alpha_time, parsedExp> > theProfile;

	//
	// Convenient; this vector is made visible to adapters via the
	// adaptation language.
	//
	vector<Cstring>			indices;

	//
	// We need one copy of this list for each parallel thread:
	//
	// 	- the modeling thread
	// 	- the constraint-checking thread
	// 	- the TOL output thread
	// 	- the XMLTOL output thread
	//
	// Each thread can, in principle, have more than one active
	// miterator. In practice, the only one that currently does
	// so is the modeling thread, which uses EventLoop::potential_triggers
	// to keep track of current history nodes while modeling, and
	// window_finder::theTriggers while searching for opportunities
	// while executing get_windows(). Within a given thread, the
	// miterator created last is the only active ones. When get_windows()
	// is done, theTriggers is unregistered from the various resource
	// histories, and potential_triggers resumes its role.
	//
	slist<alpha_void, Cntnr<alpha_void, miterator_data> >*
					active_miterators[4];

	//
	// GUI support
	//
	rsource_ext*			derivedResource;

	//
	// backpointer support - this enables the use of smart RESptr pointers
	//
	backptr<Rsource>		ptrs;
	virtual backptr<Rsource>*	Pointers() {
		return &ptrs;
	}

	//
	// multithreading support
	//
	mutex				res_mutex;

	//
	// highest for resources with no dependencies; can be used to
	// evaluate current values in a consistent manner, e. g. when
	// computing TOLs:
	//
	int				evaluation_priority;

	apgen::RES_CLASS		myClass;

	//
	// Index set and used while creating (XML)TOL files. There
	// is a separate value for each thread, in case output is
	// handled by independent, parallel execution threads.
	// Use thread_index as the index into the array.
	//
	int				output_index[10];

	//
	// Static data and methods
	//

	//
	// For state resources
	//
	static vector<TypedValue>&	default_range();

	//
	// for debugging
	//
	static bool			evaluation_debug;

	//
	// For easy traversal of the list of all resources. Resources are listed in
	// "dependencies first" order. Thus, standalone resources (with no dependencies)
	// are listed first; resources that only depend in standalone resources are listed
	// next, and so on.
	//
	static void			get_all_resources_in_dependency_order(
			tlist<alpha_string, Cntnr<alpha_string, Rsource*> >& here);

	//
	// static "query" methods
	//
	static void			get_all_resource_names(stringslist&);

	//
	// Same for containers
	//
	static void			get_all_resource_container_names(stringslist&);

	//
	// transform list of names into list of pointers to resources
	//
	static void			expand_list_of_resource_names(
	    vector<string>					   resource_names,
	    tlist<alpha_string, Cntnr<alpha_string, Rsource*> >&  all_included_res);
};

class simple_res_iter {
public:
	vector<Rsource*>&	vec;
	int			i0;
	simple_res_iter(RCsource* rc)
		: i0(0),
			vec(rc->payload->Object->array_elements) {}
	~simple_res_iter() {}
	Rsource*	next() {
		if(i0 >= vec.size()) {
			i0 = 0;
			return NULL;
		}
		return vec[i0++];
	}
};

#endif /* _R_SOURCE_H_ */
