#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <assert.h>
#include <sstream>

#include "apDEBUG.H"

#include "action_request.H"
#include "ActivityInstance.H"
#include "ACT_exec.H"
#include "Constraint.H"
#include "instruction_node.H"
#include "RES_def.H"
#include "RES_exec.H"
#include "EventImpl.H"
#include "Scheduler.H"
#include "TOL_write.H"
#include "apcoreWaiter.H"

// #define DEBUG_THIS
coutx excl;

#ifdef DEBUG_THIS
extern atomic<int> feed_flag;
#endif /* DEBUG_THIS */


//
// Defined in APmain.C; points to C functions
// in the user-defined library:
//
extern "C" {
    extern void (*clear_user_cache)(void);
    extern void (*clean_up_user_cache)(void);
}

		//
		// At the present time, we implement CurrentEvent as a
		// dumb, static pointer. It is not intended for use by
		// trailing threads. What will be consulted by trailing
		// threads is the time tag of the current event, which
		// is of the utmost importance.
		//
mEvent*		EventLoop::CurrentEvent = NULL;
long int	EventLoop::time_advance_count = 0;

tlist<alpha_void, Cntnr<alpha_void, RCsource*> >&
		EventLoop::containersToWatchForWaitUntil() {
	static tlist<alpha_void, Cntnr<alpha_void, RCsource*> > R;
	return R;
}

EventLoop::EventLoop(
		time_saver&	ts)
	: modelingTime(ts),

		//
		// This initializes Senders to a vector
		// containing MITER_THREADS unique_ptr's
		// referencing NULL.
		//
		// Senders(thread_intfc::MITER_THREADS),
		old_events("old_events"),
		potential_triggers("potential_triggers") {

    //
    // the MODEL_THREAD item in threads() was created
    // by thread_intfc::create_threads(), invoked from
    // create_subsystems() in APmain. So we can safely
    // do this.
    //
    // Senders[thread_intfc::MODEL_THREAD].reset(
    //	new thread_intfc::completion_signal_sender(
    //	    thread_intfc::threads()[thread_intfc::MODEL_THREAD].get()
    //	)
    // );
}

EventLoop*	EventLoop::theEventLoopPtr = NULL;

EventLoop&	EventLoop::theEventLoop() {
	static bool initialized = false;
	if(!initialized) {
		initialized = true;
		theEventLoopPtr = new EventLoop(ACT_exec::ACT_subsystem().theMasterTime);
	}
	return *theEventLoopPtr;
}

apgen::RETURN_STATUS first_time_waiting_for_signal_or_condition(
		pEsys::executableExp&	source_clause,
		pEsys::execStack&	stack,
		Cstring&		errors,
		exec_status&		eStatus) {
	instruction_node*	i_n;
	RCsource*		the_container;
	execution_context*	context = stack.get_context();
	behaving_element&	ambient = context->AmbientObject;

	//
	// Possible types of wait statements:
	//     WaitUntil with string-type condition
	//     WaitUntil with boolean-type condition
	//     WaitUntilRegexp
	//

	pEsys::WaitUntil* until = dynamic_cast<pEsys::WaitUntil*>(&source_clause);
	pEsys::WaitUntilRegexp* until_regexp = dynamic_cast<pEsys::WaitUntilRegexp*>(&source_clause);

	assert(until || until_regexp);

	//
	// case 'wait for signal' or 'wait for condition'
	//
	stringtlist		ContainersAppearingThroughCurrentval;
	stringslist::iterator	currentval_cont_list(ContainersAppearingThroughCurrentval);
	bool			waiting_on_resource = false;
	bool			waiting_on_signal = false;
	bool			string_is_reg_exp = false;

	parsedExp&		theCondition = until ? until->Expression : until_regexp->Expression;
	TypedValue		evaluated_condition;

	try {
		theCondition->eval_expression(ambient.object(), evaluated_condition);
	}
	catch(eval_error Err) {
		errors << "first_time_waiting: evaluation error in condition;\n" << Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}

	if(evaluated_condition.is_string()) {
		waiting_on_signal = true;
		if(until_regexp) {
			string_is_reg_exp = true;
		}
	} else if(evaluated_condition.is_int()) {
		waiting_on_resource = true;
	} else {
		errors << "first_time_waiting: WAIT_UNTIL condition is neither a string nor a boolean.";
		return apgen::RETURN_STATUS::FAIL;
	}

	//
	// handle the case "wait for a condition to become true"
	//
	if(waiting_on_resource) {
	    if(evaluated_condition.get_int() > 0) {
		eStatus.state = exec_status::NOOP;
		return apgen::RETURN_STATUS::SUCCESS;
	    } else {
		pEsys::currentval_finder	CF;
		theCondition->recursively_apply(CF);
		stringslist::iterator		iter(CF.container_names);
		emptySymbol*			es;

		while((es = iter())) {

		    //
		    // NOTE: all resources found are concrete, no need to look for abstract ones
		    //
		    if(!(the_container = RCsource::resource_containers().find(es->get_key()))) {
		    	errors << "first_time_waiting: cannot find resource container \""
		    		<< es->get_key() << "\" ";
		    	return apgen::RETURN_STATUS::FAIL;
		    }

		    //
		    // The purpose of containersToWatchForWaitUntil is to help
		    // filter resource value nodes in the loop for unblocking
		    // threads that wait for a condition to become true
		    //
		    if(!EventLoop::containersToWatchForWaitUntil().find(
						(void *) the_container)) {
			EventLoop::containersToWatchForWaitUntil()
				<< new Cntnr<alpha_void, RCsource*>(
						the_container,
						the_container);
		    }

		    //
		    // We need to add all the containers that the_container
		    // might have a dependency on
		    //
		    slist<alpha_void, Cntnr<alpha_void, RCsource*> >::iterator
				dependencies(the_container->payload->ptrs_to_containers_used_in_profile);
		    Cntnr<alpha_void, RCsource*>*		a_ptr;
		    while((a_ptr = dependencies())) {
			RCsource*	the_dependency = a_ptr->payload;
			if(!EventLoop::containersToWatchForWaitUntil().find(
						(void *) the_dependency)) {
			    EventLoop::containersToWatchForWaitUntil()
				<< new Cntnr<alpha_void, RCsource*>(
						the_dependency,
						the_dependency);
			}
		    }
		}
	    }
	}

	//
	// else, if waiting on a signal we put the thread on hold
	// no matter what, so: nothing special to do
	//
	eStatus.state = exec_status::WAITING;

#ifdef HELPFUL_MESSAGES
	cout << "first_time_waiting_for_signal_or_condition: putting the following instruction on hold:\n";
	Cstring tmp;
	context->get_instruction()->print(tmp, true);
	cout << tmp;
#endif /* HELPFUL_MESSAGES */

	//
	// We will put the thread on hold; since the current event contains
	// the stack necessary for resuming execution of the the thread, we
	// need to capture the stack as part of the information saved in
	// the new instruction_node.
	//
	where_to_go() <<	(i_n = new instruction_node(
					stack,
					ambient,
					theCondition,
					string_is_reg_exp)
				);

	Cntnr<alpha_int, instruction_node*>*	ptr;

	//
	// park the thread in the appropriate queue
	//
	if(waiting_on_signal) {
	    if(!(ptr = eval_intfc::actsOrResWaitingOnSignals().find(
					EventLoop::CurrentEvent->eventID))) {
		eval_intfc::actsOrResWaitingOnSignals()
			<< new Cntnr<alpha_int, instruction_node*>(
				EventLoop::CurrentEvent->eventID,
				i_n);
	    }
	} else {
	    if(!(ptr = eval_intfc::actsOrResWaitingOnCond().find(
					EventLoop::CurrentEvent->eventID))) {
		eval_intfc::actsOrResWaitingOnCond()
			<< new Cntnr<alpha_int, instruction_node*>(
				EventLoop::CurrentEvent->eventID,
				i_n);
	    }
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void EventLoop::document_nodes(const Cstring& s) {
    Rsource*		resource;
    Rsource::iterator	iter;

    cout << s << ":\n";
    while((resource = iter.next())) {
	RES_history* hist_ptr = &resource->get_history();

	cout << "    res. " << resource->name << ": saved node = ";
	if(resource->peek_at_miterator_data()) {
	    cout << ((void*)&resource->peek_at_miterator_data()->miter) << "\n";
	    value_node* ptr;
	    if((ptr = resource->peek_at_miterator_data()->iter->getPrev())) {
		cout << "         time = "
		    << ptr->getKey().getetime().to_string()
		    << "\n";
	    }
	} else {
	    cout << "NULL";
	}
    }
}


//
// This method is only called in one place: in EventLoop::ProcessEvents(),
// just _after_ the initial event has been processed.
//
// There is a potential problem with this: if the Miterators are not in
// their "pristine" state when the initial event is processed, strange
// things might happen.
//
void EventLoop::init_iterators() {
	Rsource*	resource;

	old_events.clear();


	tlist<alpha_string, Cntnr<alpha_string, Rsource*> > all_res(false);
	slist<alpha_string, Cntnr<alpha_string, Rsource*> >::iterator
						all_res_iter(all_res);
	Cntnr<alpha_string, Rsource*>*	ptr_to_res;
	
	Rsource::get_all_resources_in_dependency_order(all_res);
	while((ptr_to_res = all_res_iter.next())) {
	    resource = ptr_to_res->payload;
	    RES_state*		state_res = dynamic_cast<RES_state*>(resource);
	    RES_settable*	settable_res = dynamic_cast<RES_settable*>(resource);
	    RES_numeric*	numeric_res = dynamic_cast<RES_numeric*>(resource);

	    if(state_res) {
		if(model_control::get_pass() == model_control::SCHEDULING_2) {
		    if(!resource->isFrozen()) {

			//
			// We do not set the store flag; this means that the
			// old_events Miterator will not register itself with
			// the history list. That is OK because this Miterator
			// will never be used for evaluation, and so it does
			// not need to convey Strategy information to the
			// execution engine.
			//
			state_res->get_history().addToIterator(
				old_events,
				1 + resource->evaluation_priority,
				/* store = */ false);
		    }
		}
	    } else if(settable_res) {
		if(model_control::get_pass() == model_control::SCHEDULING_2) {
		    if(!resource->isFrozen()) {

			//
			// See above remark concerning store = false
			//
			settable_res->get_history().addToIterator(
				old_events,
				1 + resource->evaluation_priority,
				/* store = */ false);
		    }
		}
	    } else if(numeric_res) {
		if(model_control::get_pass() == model_control::SCHEDULING_2) {
		    if(!resource->isFrozen()) {

			//
			// See above remark concerning store = false
			//
			numeric_res->get_history().addToIterator(
				old_events,
				1 + resource->evaluation_priority,
				/* store = */ false);
		    }
		}
	    }
	}

	if(model_control::get_pass() == model_control::SCHEDULING_2) {
	    assert(old_events.next() == NULL);
	}
	assert(potential_triggers.next() == NULL);

	// assert(events_involved_in_constraints.next() == NULL);
	// assert(pef_record::OtherEvents().next() == NULL);
}

//
// Special-purpose class used to clear the user cache
// (in the user-defined library) at the beginning of
// the modeling process and clean it up at the end.
// This is used e. g. for clearing the spkez_api cache
// in the udef library.
//
class user_data_manager {
public:
    user_data_manager() {
	if(clear_user_cache) {
	    clear_user_cache();
	}
    }
    ~user_data_manager() {
	if(clean_up_user_cache) {
	    clean_up_user_cache();
	}
    }
};

void	EventLoop::ProcessEvents() {
    mEvent*			nextEvent;

    thread_intfc*		model_thread =
    	thread_intfc::threads()[thread_intfc::MODEL_THREAD].get();

    thread_intfc*		CT = NULL;
    thread_intfc*		TT = NULL;
    thread_intfc*		XT = NULL;
    CTime_base&			time_last_processed =
				    	model_thread->time_of_last_processed_vector;

    user_data_manager		clear_and_clean_up_libudef_data;

#   ifdef DEBUG_THIS
    feed_flag.store(0);
#   endif /* DEBUG_THIS */

    try {

	//
	// basic checks
	//
	assert((CurrentEvent = eval_intfc::Events().next()) == NULL);
	assert(eval_intfc::actsOrResWaitingOnSignals().get_length() == 0);
	assert(eval_intfc::actsOrResWaitingOnCond().get_length() == 0);
	assert(where_to_go().get_length() == 0);


	//
	// Initialize the Event Miterator. At the very least, Events() contains
	// the initialization node, so we know that the queues are not empty.
	//
	nextEvent = eval_intfc::Events().peek();
	assert(nextEvent);
	assert(nextEvent == eval_intfc::Events().next());
	CurrentEvent = nextEvent;

	//
	// It is too soon to initialize the resource history iterators,
	// because the resource priorities - necessary when defining
	// e. g. the Miterator of potential triggers - have not
	// yet been computed. They will only be computed when invoking
	// do_model() on the initialization event - which we do now.
	//

	CurrentEvent->do_model();

	//
	// Design pattern: update this time variable every time
	// an event is processed.
	//
	time_last_processed = CurrentEvent->getKey().getetime();

	//
	// See above note - NOW we can initialize the resource
	// history Miterators.
	//
	init_iterators();


	//
	// Get ready to communicate with the constraint-checking
	// (and, in the near future, TOL and XMLTOL) thread(s).
	// new_vector will contain a snapshot of iterators
	// containing pointers to "safe to use" history nodes.
	//
	thread_intfc::update_current_time(time_last_processed);

	//
	// Initializes flags like Done and ErrorsFound:
	//
	thread_intfc::threads()[thread_intfc::MODEL_THREAD].get()->prelude(time_last_processed);


#	ifdef DEBUG_THIS
	excl << "process events START\n";
#	endif /* DEBUG_THIS */

	//
	// Launch the constraint-checking thread if appropriate
	//
	Cntnr<alpha_string, thread_intfc*>* thread_node;
	if(APcloptions::theCmdLineOptions().constraints_active
	   && Constraint::allConstraints().get_length()) {
	    thread_intfc::threads()[thread_intfc::CONSTRAINT_THREAD].reset(
		(CT = new thread_intfc(
			thread_intfc::MODEL_THREAD // thread to trail
			)));

	    //
	    // Initializes the thread's essential flags:
	    //
	    CT->prelude(time_last_processed);

#	    ifdef DEBUG_THIS
	    excl << "process events: launching constraint-checking thread.\n";
#	    endif /* DEBUG_THIS */

	    //
	    // Now we can start the trailing thread
	    //
	    CT->Thread = std::thread(
			Constraint::check_constraints,
			CT);

	    //
	    // The constraint checker will create a dual_purpose_iterator
	    // which is waiting until fed a vector of safe nodes, so
	    // we feed it what it wants:
	    //
	    safe_vector_info* new_vector = potential_triggers.save_safe_nodes();

#	    ifdef DEBUG_THIS
	    excl << "process events: about to store CT::safe_vector...\n";
#	    endif /* DEBUG_THIS */

	    //
	    // Release the trailing thread from its hold by storing a
	    // valid pointer in SafeVector:
	    //
	    CT->SafeVector.store(new_vector);

#	    ifdef DEBUG_THIS
	    excl << "process events: stored CT::safe_vector; waiting for it to be loaded....\n";
#	    endif /* DEBUG_THIS */

	    while(!CT->theSafeVectorHasBeenCaptured) {
		thread_intfc::sleep_for_ms(10);
	    }

#	    ifdef DEBUG_THIS
	    excl << "process events: safe_vector has been read.\n";
#	    endif /* DEBUG_THIS */

	}

	advance_now_to(	time_last_processed,
			/* tolerate_equal_times  = */	true,
			/* force_constraint_eval = */	false,
			/* last_chance           = */	false);

	//
	// Loop over events
	//
	while(true) {

	    //
	    // Take a peek at the next thing in the event loop
	    //
	    nextEvent = eval_intfc::Events().peek();

	    if(!nextEvent) {

		//
		// We have run out of events, but we may still
		// be able to advance time to a new value.
		// To find out, we need to check two things:
		//
		// 	1. A waiting thread may be able to move
		// 	   forward right now;
		//
		// 	2. A waiting thread may be able to move
		// 	   forward at a time T which is greater
		// 	   than now
		//
		// This is exactly what can_a_thread_be_unlocked()
		// will do for us.
		//
		pEsys::execStack new_stack_for_old_thread;
		if(can_a_thread_be_unlocked(
				time_last_processed,
				nextEvent,
				new_stack_for_old_thread)) {

		    //
		    // The factory will only create a resume_event
		    // if the stack contains a context, and that
		    // will only be the case if can_a_thread_be_unlocked()
		    // identified a sleeping thread that is ready to
		    // proceed:
		    //
		    resume_event*	revived_thread =
			resume_event::resumeEventFactory(
				new_stack_for_old_thread.get_context());
		    if(revived_thread) {
			model_intfc::add_timevent(
				revived_thread->source,
				revived_thread->getStack().get_context()->what_section());
		    }

		    //
		    // else, can_a_thread_be_unlocked() identified a
		    // thread which is going to be woken up by an event
		    // that is already in the queue.
		    //
		    continue;
		} else {

		    //
		    // we exit the loop, since there is no next event
		    //
		    break;
		}

		//
		// no path leads to here since we continued, broke out, or
		// returned with an error
		//
		assert(false);
	    }

	    //
	    // Question: is the time tag of the next event greater than that
	    // of the event we just processed?
	    //
	    CTime_base	newTime = nextEvent->getKey().getetime();

	    //
	    // If it is, then there is some checking and housekeeping we need
	    // to do before we really advance time
	    //
	    if(newTime > time_last_processed) {

		//
		// We may possibly advance time to newTime.
		// However, before doing that, we need to check
		// two things:
		//
		// 	1. A waiting thread may be able to move
		// 	   forward right now;
		//
		// 	2. A waiting thread may be able to move
		// 	   forward at a time T which is greater
		// 	   than now but less than the proposed
		// 	   time value, newTime
		//
		pEsys::execStack	new_stack_for_old_thread;

		//
		// The call to can_a_thread_be_unlocked() will
		// change proposed_new_time (third argument)
		// if a new opportunity can be found at time T
		// with Tcurrent < T < Tnext.
		//
		// A side effect of this call is to update the
		// potential_triggers Miterator so that it has
		// pointers to the latest resource history nodes
		// just before 'now'. Having these pointers makes
		// it very easy to evaluate current values; it is
		// therefore important to take advantage of this
		// fact.
		//
		if(can_a_thread_be_unlocked(
					time_last_processed,
					nextEvent,
					new_stack_for_old_thread)) {

			//
			// The factory will only create a resume_event
			// if the stack contains a context, and that
			// will only be the case if can_a_thread_be_unlocked()
			// identified a sleeping thread that is ready to
			// proceed:
			//
			resume_event* revived_thread
					    = resume_event::resumeEventFactory(
						new_stack_for_old_thread.get_context());
			if(revived_thread) {
			    model_intfc::add_timevent(
				revived_thread->source,
				revived_thread->getStack().get_context()->what_section());
			}

			//
			// else, can_a_thread_be_unlocked() identified a
			// thread which is going to be woken up by an event
			// that is already in the queue.
			//
			continue;
		} else {

			//
			// we were NOT able to unblock a waiting thread.
			//
			eval_intfc::clear_signals();
		}
		if(nextEvent != eval_intfc::Events().peek()) {

		    //
		    // New events have been created, presumably by a thread
		    // that just woke up; let's process these events before we
		    // do anything else.
		    //
		    continue;
		} else {

		    //
		    // There is no more work to do for the present value of
		    // now; we can truly advance time.
		    //
		    delete CurrentEvent;
		    CurrentEvent = nextEvent;
		}
	    } else {

		//
		// The time tag of the new event is the same as that of the
		// previous event. Let's keep processing events until we are
		// all done with the present value of now.
		//
		delete CurrentEvent;
		CurrentEvent = nextEvent;
	    }
	    assert(CurrentEvent == eval_intfc::Events().next());


	    //
	    // Finally, we process the event! But first, we advance 'now' to
	    // the time tag of the event. This used to be done by do_model(),
	    // but they all did the same thing, so we might just as well do it
	    // here.
	    //
	    // Note that the potential_triggers Miterator is in sync with
	    // 'now' thanks to the processing that took place near the top
	    // of the loop. As a result, resources can rely on the fact that
	    // the current Miterator can be used to compute currentval().
	    //
	    // Note: we anticipate a little; technically we should set
	    // time_last_processed just after the call to do_model(),
	    // but it's not used globally so it's OK:
	    //
	    time_last_processed = CurrentEvent->getKey().getetime();

	    thread_intfc::update_current_time(time_last_processed);
	    advance_now_to(time_last_processed, true);

#	    ifdef DEBUG_THIS
	    Cstring str;
	    str << "process event: do model at " << CurrentEvent->getetime().to_string() << "\n";
	    excl << str;
#	    endif /* DEBUG_THIS */

	    CurrentEvent->do_model();
	}

#	ifdef DEBUG_THIS
	excl << "process events DONE; feeding data to trailing threads...\n";
#	endif /* DEBUG_THIS */

	//
	// Modeling is done, but the threads are most likely
	// still going. Feed them data as needed.
	//
	feed_last_data_to_trailing_threads();

	//
	// Catch any errors that occurred within a thread
	//
	for(int k = 0; k < thread_intfc::MITER_THREADS; k++) {

	    //
	    // The modeling thread takes care of its own errors
	    //
	    if(k == thread_intfc::MODEL_THREAD) {
		continue;
	    }
	    thread_intfc* the_thread = thread_intfc::threads()[k].get();
	    if(the_thread && the_thread->errors.length()) {
		throw(eval_error(the_thread->errors));
	    }
	}
    } catch(eval_error Err) {
    	Cstring		errs;

	errs = "ProcessEvents: failure in CurrentEvent->do_model:\n";
	errs << Err.msg;

	//
	// Make sure trailing threads know not to wait
	//
	thread_intfc::threads()[thread_intfc::MODEL_THREAD].get()->Done = true;
	thread_intfc::threads()[thread_intfc::MODEL_THREAD].get()->ErrorsFound = true;

	destroy_all_threads(errs);

	eval_intfc::clear_signals();
	eval_intfc::clear_patterns();
	throw(eval_error(errs));
    }

    eval_intfc::clear_signals();
    eval_intfc::clear_patterns();
    Cstring errs;
    if(destroy_all_threads(errs)) {
	throw(eval_error(errs));
    }
}

mutex&	dual_purpose_iterator::Mutex() {
	static mutex M;
	return M;
}

dual_purpose_iterator::dual_purpose_iterator(thread_intfc* T)
		: Thread(T) /* , Sender(T) */ {
	if(Thread) {

		//
		// Multi-threaded mode. We wait until the modeling
		// thread releases us. Initially I had a special
		// start flag, but in reality it's enough to wait
		// for the new vector from the thread we are
		// following.
		//
		while(!Thread->SafeVector.load()) {
			this_thread::sleep_for(chrono::milliseconds(10));
    		}

		//
		// ... then, we grab the "safe vector" pointing to
		// iterators that are safe for the trailing thread
		// (that's us) to use.
		//
		MiterPtr.reference(new trailing_miter(
					"threaded events",
					*Thread,
					Thread->SafeVector.load(),
					Thread->theThreadWeTrail));
	} else {

		//
		// Single-thread operations; we don't need to be
		// concerned with inter-thread communications.
		// However, we would like the miterator to be
		// initialized at the start time of the TOL output
		// request.
		//
		MiterPtr.reference(new res_curval_miter(
					"regular events"));
	}
}

dual_purpose_iterator::~dual_purpose_iterator() {

	//
	// unregistering Miterators is thread-unsafe
	//
	lock_guard<mutex>	lock(Mutex());
	MiterPtr.dereference();
}

void dual_purpose_iterator::initialize_at(CTime_base init_time) {
	if(Thread) {

		//
		// Nothing to do; the internal trailing miterator
		// was initialized with a vector from the modeling
		// thread.
		//
	} else {
		res_curval_miter* curiter
			= dynamic_cast<res_curval_miter*>(MiterPtr.object());
		assert(curiter);
		curiter->prepare_for_use_at(
			window_finder::initializer,
			init_time);
	}
}

value_node* dual_purpose_iterator::peek() {
    return MiterPtr->peek();
}
