#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <thread>

#include "action_request.H"
#include "ActivityInstance.H"
#include "Constraint.H"
#include "EventLoop.H"
#include "RES_def.H"

// #define DEBUG_THIS

#ifdef DEBUG_THIS
extern coutx excl;
#endif /* DEBUG_THIS */

tlist<alpha_string, Constraint>&
		Constraint::allConstraints() {
	static tlist<alpha_string, Constraint>	A;
	return A;
}

int	con_violation::currentID = 0;

slist<alpha_void, smart_actptr>::iterator& con_violation_gui::dummy_iterator() {
	static slist<alpha_void, smart_actptr>			L;
	static slist<alpha_void, smart_actptr>::iterator	D(L);
	return D;
}

tlist<alpha_void, Cnode0<alpha_void, Rsource*> >&
		Constraint::resourcesSubjectToConstraints() {
	static tlist<alpha_void, Cnode0<alpha_void, Rsource*> > T;
	return T;
}

tlist<alpha_void, Cnode0<alpha_void, Rsource*> >&
		Constraint::resourcesRecentlyUpdated() {
	static tlist<alpha_void, Cnode0<alpha_void, Rsource*> > R;
	return R;
}

tlist<alpha_time, con_violation>&
		Constraint::violations() {
	static tlist<alpha_time, con_violation> A(true);
	return A;
}

std::atomic<con_violation*>	Constraint::last_safe_node;

con_violation::~con_violation() {
	Cnode0<alpha_void, con_violation_gui*>* dslineowner_ptr;

	//
	// Delete our representatives in the GUI
	//
	while((dslineowner_ptr = ds_line_owners.first_node())) {

		//
		// Do this first because the con_violation_gui
		// destructor will try to delete dslineowner_ptr
		// if it finds it in the list
		//
		ds_line_owners.remove_node(dslineowner_ptr);
		delete dslineowner_ptr->payload;
	}
}

long int	Constraint::release_count() {
	slist<alpha_time, con_violation>::iterator
		vioIter(Constraint::violations());
	con_violation*
		vionode;
	long int
		count = 0;

	while((vionode = vioIter())) {
		if(vionode->payload.severity == Violation_info::RELEASE) {
			count++;
		}
	}
	return count;
}


//
// The history of one relevant resource has changed. Update
// the list of changed resources, and update the list of
// constraints to check (because they have the changed resource
// as a dependency).
//
void Constraint::process_one_history_node(
		value_node*						res_event,
    		tlist<alpha_void, Cnode0<alpha_void, Rsource*> >&	updated_res,
    		tlist<alpha_void, Cnode0<alpha_void, Constraint*> >&	to_check) {

	Rsource*			  res = res_event->list->Owner;
	Cnode0<alpha_void, Constraint*>*  con;

	if(!updated_res.find(res)) {
	    updated_res << new Cnode0<alpha_void, Rsource*>((void*)res, res);

	    //
	    // Let's use RCsource::any_constraints to figure out
	    // which constraints to check
	    //
	    slist<alpha_void, Cnode0<alpha_void, Constraint*> >::iterator
		con_iter(res->get_container()->any_constraints);
	    while((con = con_iter())) {
		if(!to_check.find((void*)con->payload)) {
		    to_check << con->copy();
		}
	    }
	}
}


//
// Constraint::check_constraints() is called by EventLoop::ProcessEvents()
// via creation of a new thread. The thread is created in the initial
// section of ProcessEvents(), before the event loop per se.
//
// ProcessEvents() and check_constraints() then run in parallel:
//
// - ProcessEvents() goes through events in increasing time order, and
//   processes events by invoking their mEvent::do_model() method.
//
// - Meanwhile, check_constraints() runs in parallel to the event loop.
//   The only job of check_constraints() is to monitor the histories of
//   the various resources involved in constraints and check for violations
//   every time one of these resources changes.
//
// Synchronization between the event loop and the constraint checker occurs
// via a coarse-grained mechanism inside the thread_intfc class as described
// below.
//
// thread_intfc has two atomic data members and three atomic flags:
//
// Data Members
// ============
//
//    - SafeVector
//
//      Points to a vector of resource history nodes, one for
//      every resource the trailing thread cares about. These nodes
//      are 'safe' in the sense that the modeling thread is
//      guaranteed not to touch the piece of the history before
//	and including the safe node. The trailing thread can safely
//	iterate using the slist iterator.
//
//    - SafeTime
//
//      Is provided to the trailing thread(s) by the modeling
//      thread. Its semantics is "I have moved beyong SafeTime in my
//      processing and everyting up to and including SafeTime is safe
//      to consult." The time tags of history nodes in the SafeVector
//      are all less than or equal to SafeTime.
//
// Atomic Flags
// ============
//
//    - start_thread
//
//	Set by the modeling thread and consulted by the trailing
//	threads. The modeling thread always sets that flag to false
//	before starting a trailing thread, and the method that runs
//	a trailing thread always starts by waiting until start_thread
//	is set to true.
//
//    - theSafeVectorHasBeenCaptured
//
//      Set by a trailing thread and monitored by the modeling thread.
//      The initial value of this flag is always set to false by the
//      modeling thread just before it starts a trailing thread.
//
//      At periodic intervals that start just before the creation of a
//      trailing thread, the modeling thread puts together a vector
//      of resource history nodes that represent a snapshot of the
//      resource subsystem at the value of 'now' which the modeling
//      thread just finished processing. The trailing thread's pointer
//      is set to that vector, provided that theSafeVector... flag is
//      true, meaning that the trailing thread has captured the previous
//      flag and is ready for more work.
//
//      When a trailing thread is started, it grabs the vector, copies
//      it, deletes it and sets the pointer to it to NULL. It then
//      sets the value of theSafeVectorHasBeenCaptured to true.
//
//      When the trailing thread is done with its current workload, it
//      consults the SafeVector to see whether it has become non-NULL.
//      If so, it grabs it, copies it etc., just like before.
//
//    - Done
//
//      Set by the thread that owns the flag.
//
// The modeling thread uses a done_modeling_signal_sender object to send
// the signal that it's done; the constraint-checking thread does the same
// with a done_checking_constraints_signal_sender object.
//
void Constraint::check_constraints(thread_intfc* Thread) {

    //
    // Define global variables for this thread
    //
    thread_index = thread_intfc::MODEL_THREAD;
    global_behaving_object*& model_globals = behaving_object::GlobalObject();
    thread_index = thread_intfc::CONSTRAINT_THREAD;
    global_behaving_object*& thread_globals = behaving_object::GlobalObject();
    thread_globals = new global_behaving_object(*model_globals);

    //
    // The dual_purpose_iterator takes care of waiting until
    // this thread is fed a SafeVector. NOTE: make sure that
    // the dual_purpose_iterator constructor defines the copy
    // of the current vector before it returns!
    //
    dual_purpose_iterator	constraint_miter(Thread);

    // debug
#   ifdef DEBUG_THIS
    Cstring str;
    excl << "start; constructed constraints' trailing_miter.\n";
#   endif /* DEBUG_THIS */

    //
    // Initialize data
    //
    Constraint::violations().clear();
    last_safe_node.store(NULL);

    bool			there_is_at_least_one_constraint = false;

    //
    // Now we have a dual_purpose_iterator and a safe vector
    // is available. The safe vector references all modeled
    // resources and tells us how far we can iterate safely
    // along resource histories. But we also need to initialize
    // our own iterators for only those resources that we care
    // about. That's what we do here.
    //
    // Registering miterators with lists is not thread-safe,
    // so we lock the dual_purpose_iterator's Mutex.
    //
    try {
	lock_guard<mutex>	lock(dual_purpose_iterator::Mutex());

	tlist<alpha_string, Cnode0<alpha_string, Rsource*> >		all_res(false);
	slist<alpha_string, Cnode0<alpha_string, Rsource*> >::iterator	all_res_iter(all_res);
	Cnode0<alpha_string, Rsource*>*					ptr_to_res;

	Rsource::get_all_resources_in_dependency_order(all_res);


	while((ptr_to_res = all_res_iter.next())) {
	    Rsource*		resource = ptr_to_res->payload;
	    RES_state*		state_res = dynamic_cast<RES_state*>(resource);
	    RES_settable*	settable_res = dynamic_cast<RES_settable*>(resource);
	    RES_numeric*	numeric_res = dynamic_cast<RES_numeric*>(resource);

	    if(state_res) {
		if(Constraint::resourcesSubjectToConstraints().find(
							(void*)resource)) {
		    there_is_at_least_one_constraint = true;
		    state_res->get_history().addToIterator(
				*constraint_miter,
				1 + resource->evaluation_priority,
				/* store = */ true);
		}
	    } else if(settable_res) {
		if(Constraint::resourcesSubjectToConstraints().find(
							(void*)resource)) {
		    there_is_at_least_one_constraint = true;
		    settable_res->get_history().addToIterator(
				*constraint_miter,
				1 + resource->evaluation_priority,
				/* store = */ true);
		}
	    } else if(numeric_res) {
		if(Constraint::resourcesSubjectToConstraints().find(
							(void*)resource)) {
		    there_is_at_least_one_constraint = true;
		    numeric_res->get_history().addToIterator(
				*constraint_miter,
				1 + resource->evaluation_priority,
				/* store = */ true);
		}
	    }
	}
    } catch(eval_error Err) {
	Thread->errors
		<< "check_constraints: error - details:\n" << Err.msg;

	Thread->Done.store(true);

#ifdef DEBUG_THIS
	str.undefine();
	str << "error; setting done and returning.\n";
	excl << str;
#endif /* DEBUG_THIS */

	return;
    }

    if(!there_is_at_least_one_constraint) {

	Thread->Done.store(true);

	// debug
#	ifdef DEBUG_THIS
	str.undefine();
	str << "no constraints; setting done and returning.\n";
	excl << str;
#	endif /* DEBUG_THIS */

	return;
    }

    tlist<alpha_void, Cnode0<alpha_void, Rsource*> >&
		updated = Constraint::resourcesRecentlyUpdated();

    tlist<alpha_void, Cnode0<alpha_void, Constraint*> >
		constraints_to_check(true);

    updated.clear();

    try {

	// debug
#	ifdef DEBUG_THIS
	str.undefine();
	str << "starting to iterate - calling peek() to initialize.\n";
	excl << str;
#	endif /* DEBUG_THIS */
	

	//
	// Theorem
	// -------
	//
	// The constraint Miterator always returns a non-NULL
	// value_node the first time it is called, because the leading
	// thread has made sure that the initialization event has been
	// processed - number one - and, number two, the trailing Miterator
	// initialization guarantees that peek() will find the initial
	// resource history node.
	//
	// Programming note: constraint_miter->peek() returns the peek()
	// method of the internal Miterator, which is either a res_curval_miter
	// (if this is happening within the modeling thread) or a trailing_miter
	// (if this is happening in a trailing thread).
	//
	// constraint_miter.peek(), on the other hand, either
	//
	//   - returns res_curval_miter::peek() if the thread is the modeling thread
	//
	//   - executes logic around trailing_miter::peek() if the thread is a
	//     trailing thread, so that it looks transparent to the caller.
	//
	value_node* constraint_related_event = constraint_miter.peek();
	CTime_base curTime = constraint_related_event->getKey().getetime();

	//
	// Now we want to trail behind EventLoop::potential_triggers.
	// Unlike regular miterators, a trailing_iter is ready to use
	// as soon as it has been created. So, we don't need to call
	// next() to initialize them.
	//

	//
	// We iterate forever, until the modeling thread finishes
	// processing the event loop. The only way to exit this
	// (outer) loop is to find out that the modeling thread's done
	// flag has become true.
	//
	while((constraint_related_event = constraint_miter.peek())) {

	    // debug
#ifdef DEBUG_THIS
	    str.undefine();
	    str << "got one event at "
	    	<< constraint_related_event->getKey().getetime().to_string()
	    	<< "\n";
	    excl << str;
#endif /* DEBUG_THIS */

	    //
	    // The modeling thread is not finished, it sent us
	    // a snapshot of events to look at, and constraint_related_event
	    // points to the current node.
	    //
	    if(constraint_related_event->getKey().getetime() == curTime) {

		//
		// The history of one relevant resource has changed. Update
		// the list of changed resources, and update the list of
		// constraints to check (because they have the changed resource
		// as a dependency).
		//
		process_one_history_node(
				constraint_related_event,
				updated,
				constraints_to_check);

	    } else if(constraint_related_event->getKey().getetime() > curTime) {
		Cnode0<alpha_void, Constraint*>*	con;
		time_saver				save_the_time;

		//
		// OK, about to advance time - let's check constraints.
		//

		//
		// This call updates our SafeTime, which is used by
		// Rsource::evaluate().
		//
		// It is also used by any trailing (XML)TOL threads;
		// for these to work correctly, it is _vital_ that
		// SafeTime should proceed all the way to the end
		// of the modeling event queue.
		//
		thread_intfc::update_current_time(curTime);
		time_saver::set_now_to(curTime);

		slist<alpha_void, Cnode0<alpha_void, Constraint*> >::iterator
			con_iter(constraints_to_check);
		while((con = con_iter())) {

		    con->payload->evaluate();

		}
		updated.clear();
		constraints_to_check.clear();

		//
		// Update the "last safe node" that (XML)TOL threads
		// can access:
		//
		con_violation* vio = violations().last_node();
		con_violation* prev = last_safe_node.load();
		if(!prev || vio->getKey().getetime() > prev->getKey().getetime()) {
    			last_safe_node.store(vio);
		}

		curTime = constraint_related_event->getKey().getetime();

		//
		// The history of one relevant resource has changed. Update
		// the list of changed resources, and update the list of
		// constraints to check (because they have the changed resource
		// as a dependency).
		//
		process_one_history_node(
				constraint_related_event,
				updated,
				constraints_to_check);
	    }

	    constraint_miter->next();
	}

	// debug
#ifdef DEBUG_THIS
	str.undefine();
	str << "got NULL; we are done.\n";
	excl << str;
#endif /* DEBUG_THIS */

    } catch(eval_error Err) {
	Thread->errors
		<< "check_constraints: error - details:\n" << Err.msg;
	Thread->ErrorsFound.store(true);
	delete thread_globals;
	thread_globals = NULL;
	return;
    }
    Thread->Done.store(true);
    delete thread_globals;
    thread_globals = NULL;
}

void ForbiddenCondition::evaluate() {
    try {
	TypedValue val;

	payload->condition->eval_expression(
		beh_obj.object(),
		val);

	if(!condition_has_been_true && val.get_int()) {

		//
		// change the state of the automaton
		//
		condition_has_been_true = true;

		//
		// report a violation
		//
		TypedValue msg;
		payload->theMessage->eval_expression(
			beh_obj.object(),
			msg);

		con_violation* cv = new con_violation(
			thread_intfc::current_time(thread_index),
			this,
			msg.get_string(),
			severity);			
		violations() << cv;
		id_of_last_violation = cv->ID;
	} else if(condition_has_been_true && !val.get_int()) {

		//
		// change the state of the automaton
		//
		condition_has_been_true = false;

		//
		// report a release of violation
		//
		TypedValue msg;
		payload->theMessage->eval_expression(
			beh_obj.object(),
			msg);
		con_violation* cv = new con_violation(
			thread_intfc::current_time(thread_index),
			this,
			msg.get_string(),
			Violation_info::RELEASE,
			id_of_last_violation);			
		violations() << cv;
	}
    } catch(eval_error Err) {
	Cstring err;
	err << "File " << payload->condition->file << ", line "
		<< payload->condition->line
		<< ": error evaluating forbidden condition; details:\n"
		<< Err.msg;
	throw(eval_error(err));
    }
}

void ForbiddenCondition::initialize() {
	condition_has_been_true = false;
}

void MaximumDuration::initialize() {
	condition_has_been_true = false;
	since = CTime_base();
	in_violation = false;
}

void MaximumDuration::evaluate() {

    try {
	TypedValue val;

	payload->condition->eval_expression(
		beh_obj.object(),
		val);
	bool	new_condition = (bool) val.get_int();

	if(in_violation) {
		if(!new_condition) {

			//
			// We are no longer in violation; change
			// the state of the automaton
			//
			in_violation = false;
			condition_has_been_true = false;
		}
	} else if(!condition_has_been_true) {
		if(val.get_int()) {

			//
			// change the state of the automaton
			//
			condition_has_been_true = true;
			since = thread_intfc::current_time(thread_index);
		}
	} else {

		//
		// We were not in violation, but the condition has been true
		// for a while. For how long, exactly?
		//
		if(thread_intfc::current_time(thread_index) - since > payload->maximum_duration) {

			//
			// change the state of the automaton
			//
			if(val.get_int()) {
				in_violation = true;
			} else {
				condition_has_been_true = false;
			}

			//
			// report a violation
			//
			TypedValue msg;
			payload->theMessage->eval_expression(
				beh_obj.object(),
				msg);
			violations() << new con_violation(
				thread_intfc::current_time(thread_index),
				this,
				msg.get_string(),
				severity);			
		} else {

			//
			// change the state of the automaton
			//
			if(!val.get_int()) {
				condition_has_been_true = false;
			}
		}
	}
    } catch(eval_error Err) {
	Cstring err;
	err << "File " << payload->condition->file << ", line "
		<< payload->condition->line
		<< ": error evaluating forbidden condition; details:\n"
		<< Err.msg;
	throw(eval_error(err));
    }
}


