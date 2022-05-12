#ifndef _Constraint_H_
#define _Constraint_H_

#include "ParsedExpressionSystem.H"

class RCsource;
class con_violation_gui;
class thread_intfc;

class con_violation: public baseC<alpha_time, con_violation> {
public:
	con_violation() : baseC<alpha_time, con_violation>(CTime_base()) {
		ID = currentID++;
	}
	con_violation(const con_violation& VE)
	    : baseC<alpha_time, con_violation>(CTime_base()),
		payload(VE.payload) {
		ID = currentID++;
	}
	con_violation(
		CTime_base			etime,
		Constraint*			the_constraint,
		const Cstring&			msg,
		Violation_info::Severity	s,
		int				id_to_use = -1)
	    : baseC<alpha_time, con_violation>(etime),
			payload(the_constraint, msg, s) {
		if(id_to_use >= 0) {
			ID = id_to_use;
		} else {
			ID = currentID++;
		}
	}
	~con_violation();

	Violation_info			payload;
	con_violation*			get_this() {
		return this;
	}
	const con_violation*		get_this() const {
		return this;
	}
	con_violation*			copy() {
		return new con_violation(*this);
	}
	CTime_base			getetime() const {
		return getKey().getetime();
	}

	int		ID;
	tlist<alpha_void, Cntnr<alpha_void, con_violation_gui*> > ds_line_owners;

	static int	currentID;
};

//
// The inheritance from Dsource is cumbersome but makes the
// bridge to the GUI much, much simpler
//
class con_violation_gui: public Dsource {
public:
	con_violation_gui() = delete;
	con_violation_gui(con_violation* cv)
		: ref(cv) {
	    if(derivedDataConstructor) {
		Cstring any_errors;
		derivedDataConstructor(this, any_errors);

		//
		// Let the con_violation know that we exist, so
		// it can delete us first when it goes away
		//
		cv->ds_line_owners << new Cntnr<alpha_void, con_violation_gui*>(this, this);
	    }
	}
	virtual ~con_violation_gui() {
		if(derivedDataDestructor) {

			//
			// Note that Dsource inheritance provides
			// a back-pointer mechanism which will
			// cause the derived data destructor to
			// delete any DS_line objects in the GUI
			//
			derivedDataDestructor(this);
		}
		Cntnr<alpha_void, con_violation_gui*>* ptr;

		//
		// Let our master know that we are going away
		// and no longer need to be deleted when it is
		// itself deleted. If this deletion is the result
		// of our master being deleted, the master will
		// have removed our representative in its list
		// of ds_line_owners, so that we wont find
		// ourselves there and no harmful recursion
		// will take place.
		//
		if((ptr = ref->ds_line_owners.find(this))) {
			delete ptr;
		}
	}

	con_violation* ref;
	int ystart;
	int yend;

	//
	// Dummy up pure virtual methods of Dsource as much as possible
	//
	Node* copy() override { return NULL; }
	const Cstring& get_key() const override {
		static Cstring s("con_violation_gui");
		return s;
	}
	Node_type node_type() const override {
		return UNKNOWN_EXTENSION;
	}
	void setetime(const CTime_base&) override {}
	const CTime_base& getetime() const override {
		return ref->getKey().getetime();
	}
	const alpha_time& getKey() const override {
		return ref->getKey();
	}
	bool is_decomposed() const override { return false; }
	bool is_abstracted() const override { return false; }
	bool is_a_violation() const override { return true; }
	static slist<alpha_void, smart_actptr>::iterator& dummy_iterator();
	const slist<alpha_void, smart_actptr>::iterator& get_down_iterator() const override {
		return dummy_iterator();
	}
	LegendObject* get_legend() override {
		return NULL;
	}
	int children_count() override {
		return 0;
	}
	int getTheLegendIndex() const override {
		return 0;
	}
	void set_y_to(int a, int b) override {
		ystart = a;
		yend = b;
	}
};

class Constraint: public baseC<alpha_string, Constraint> {
public:

    typedef Miterator_base<slist<alpha_time, value_node>, value_node> Miterclass;

    Constraint(	const Cstring&		consName,
		pEsys::PassiveCons*	consDef,
		task*			the_constructor
	  ) : baseC<alpha_string, Constraint>(consName),
		beh_obj(new behaving_object(*the_constructor)),
		payload(consDef) { 
	if(consDef->severity == "WARNING") {
	    severity = Violation_info::WARNING;
	} else {
	    severity = Violation_info::ERROR;
	}
    }
    Constraint( const Constraint& ) = default;
    virtual ~Constraint() = default;

    Constraint*			get_this() {
		return this;
    }
    const Constraint*		get_this() const {
		return this;
    }

    smart_ptr<pEsys::PassiveCons> payload;
    behaving_element		beh_obj;

    Violation_info::Severity	severity;

    //
    // Evaluate the condition(s) that affect the state of the constraint's
    // finite automaton
    //
    virtual void evaluate() = 0;

    //
    // Initialize the constraint's finite automaton at the beginning of a
    // modeling run
    //
    virtual void initialize() = 0;

    //
    // the main "engines" of this class
    //
    static void			check_constraints(thread_intfc*);
    static void			process_one_history_node(
				    value_node*			res_event,
			    	    tlist<alpha_void, Cntnr<alpha_void, Rsource*> >&
								updated_res,
			    	    tlist<alpha_void, Cntnr<alpha_void, Constraint*> >&
								to_check);

    //
    // Methods for accessing the data of the Constraint subsystem
    //

    static tlist<alpha_string, Constraint>&
				allConstraints();

    static tlist<alpha_void, Cntnr<alpha_void, Rsource*> >&
				resourcesSubjectToConstraints();

    static tlist<alpha_void, Cntnr<alpha_void, Rsource*> >&
				resourcesRecentlyUpdated();

    static tlist<alpha_time, con_violation>&
				violations();

    //
    // To assist (XML)TOL threads in trailing us. This node is
    // static, but once it has been computed by the constraint-checking
    // thread it won't be touched again, and so it is safe for the
    // trailing threads to consult it.
    //
    static std::atomic<con_violation*>	last_safe_node;

    //
    // For the convenience of atm_client:
    //
    static long int		release_count();
};

class ForbiddenCondition: public Constraint {
public:
	ForbiddenCondition(	const Cstring&		consName,
				pEsys::PassiveCons*	consDef,
				task*			the_constructor
		  ) : Constraint(consName, consDef, the_constructor),
			condition_has_been_true(false) {}
	ForbiddenCondition( const ForbiddenCondition& ) = default;
	virtual ~ForbiddenCondition() = default;

	ForbiddenCondition* copy() {
		return new ForbiddenCondition(*this);
	}

	void evaluate();
	void initialize();

	//
	// Finite automaton
	//
	bool	condition_has_been_true;
	int	id_of_last_violation;
};

class MaximumDuration: public Constraint {
public:
	MaximumDuration(	const Cstring&		consName,
				pEsys::PassiveCons*	consDef,
				task*			the_constructor
		  ) : Constraint(consName, consDef, the_constructor),
			condition_has_been_true(false),
			in_violation(false),
			since(CTime_base( /* duration? */ true)) {}
	MaximumDuration( const MaximumDuration& ) = default;
	virtual ~MaximumDuration() = default;

	MaximumDuration* copy() {
		return new MaximumDuration(*this);
	}

	void evaluate();
	void initialize();

	//
	// Finite automaton
	//
	bool		condition_has_been_true;
	bool		in_violation;
	CTime_base	since;
};


#endif /* _Constraint_H_ */
