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

#ifdef DEBUG_THIS
extern coutx excl;
atomic<int> feed_flag;
#endif /* DEBUG_THIS */


//
// This seems OK. By Europa Clipper standards,
// 1000 events is a drop in the Ocean (which one? :-)
//
#define TIME_ADVANCES_BETWEEN_TRAILING_THREAD_UPDATES 1000

//
// Or maybe not...
//
// #define TIME_ADVANCES_BETWEEN_TRAILING_THREAD_UPDATES 100


const int  thread_intfc::MODEL_THREAD	= 0;
const int  thread_intfc::CONSTRAINT_THREAD = 1;
const int  thread_intfc::XMLTOL_THREAD	= 2;
const int  thread_intfc::TOL_THREAD	= 3;
const int  thread_intfc::MITER_THREADS	= 4;
const int  thread_intfc::NULL_THREAD	= -1;

vector<unique_ptr<thread_intfc> >&	thread_intfc::threads() {
	static vector<unique_ptr<thread_intfc> > V;
	return V;
}

thread_intfc::thread_intfc(int thread_to_follow)
    : theThreadWeTrail(thread_to_follow) {

	//
	// Actual initialization occurs in prelude()
	//
}

//
// Invoked from create_subsystems() in ACT_exec.C
//
void	thread_intfc::create_threads() {
    for(int i = 0; i < MITER_THREADS; i++) {
	if(i == MODEL_THREAD) {

	    //
	    // This creates a valid thread_intfc for the
	    // modeling thread. The std::thread inside
	    // the new thread_intfc is unused and not
	    // joinable. The NULL_THREAD argument means
	    // that the modeling thread is not trailing
	    // anything else.
	    //
	    threads().push_back(NULL);
	    threads()[i].reset(new thread_intfc(NULL_THREAD));
	} else {

	    //
	    // No thread here. Question, though: who is
	    // going to destroy the thread after we
	    // use it? I think the logic is at the end
	    // of the ProcessEvents() loop.
	    //
	    threads().push_back(NULL);
	}
    }
}

//
// The terminology "safe_time" comes from the basic requirement on it:
//
//			Requirement
//			-----------
//
// Safe time shall denote an event time T with the property that
//
// 	"Any trailing thread can safely evaluate resource
// 	values at times up to but not including T."
//
void	thread_intfc::update_current_time(const CTime_base& T) {
	threads()[thread_index]->SafeTime.store(T.get_pseudo_millisec());
}

CTime_base thread_intfc::current_time(int index /* default: thread_index */ ) {

	//
	// Beware: this is NOT the same as 'now', which can be set by
	// programs executing at some time in the future:
	//
	return CTime_base(threads()[index]->SafeTime.load());
}

//
// This method is used by a leading thread
// before launching a trailing thread
//
void thread_intfc::prelude(CTime_base initial_time) {

	//
	// Initialize variables used for inter-thread communications.
	//
	Done.store(false);
	ErrorsFound.store(false);
	WaitingForData.store(false);
	theSafeVectorHasBeenCaptured.store(false);

	//
	// When the thread starts, it will sleep until SafeVector
	// is no longer NULL:
	//
	SafeVector.store(NULL);
	SafeTime.store(initial_time.get_pseudo_millisec());
}

//
// Upon return:
//
//    - safe_vector_info->Vector() contains a snapshot of the
//	current resource history nodes processed by the modeling
//	thread
//
//    - safe_vector_info->Time is the time at which the modeling
//	thread captured the data
//
safe_vector_info* thread_intfc::get_copy_of_safe_vector_from(
			thread_intfc* followed_thread) {

	//
	// We know a safe copy exists, but we need to ensure
	// that the pointer to it is valid. The only way is
	// to have a guard:
	//
	lock_guard<mutex>	lock(
		followed_thread->safe_vector_protector);

	//
	// Now we can get the pointer and make a copy while
	// preventing the constraint thread to destroy
	// the object pointed to:
	//
	safe_vector_info*	new_vector
		= followed_thread->copy_of_safe_vector.get();
	assert(new_vector);
	return new safe_vector_info(*new_vector);
}

//
// This method is only called when the modeling thread
// is done, and wants to update the safe vector one
// last time for those trailing threads that are
// still waiting.
//
void EventLoop::feed_last_data_to_trailing_threads() {

    thread_intfc* modeling_thread
	= thread_intfc::threads()[thread_intfc::MODEL_THREAD].get();

#ifdef DEBUG_THIS
    Cstring estr;
    feed_flag.store(1);
    estr << "feed last data START\n";
    excl << estr;
    estr.undefine();
#endif /* DEBUG_THIS */

    //
    // It's possible that we _never_ started the (XML)TOL threads.
    // In that case, they should be fed data _before_ we declare that
    // we are done.
    //
    for(int k = 0; k < thread_intfc::MITER_THREADS; k++) {
	if(k == thread_intfc::MODEL_THREAD) {
	    continue;
	}
	thread_intfc*	trailing_thread = thread_intfc::threads()[k].get();

	if(trailing_thread && trailing_thread->Thread.joinable()) {

#	    ifdef DEBUG_THIS
	    estr << "feed last data: waiting for thread " << k
		    << " to capture the last data\n";
	    excl << estr;
	    estr.undefine();
#	    endif /* DEBUG_THIS */

	    safe_vector_info* new_vector = NULL;

	    //
	    // Wait for the thread to finish processing the last data
	    // vector it got
	    //
	    bool the_thread_is_done = false;
	    while(!trailing_thread->theSafeVectorHasBeenCaptured.load()) {

		//
		// WaitingForData is set by the thread when it has finished processing
		// all the data it's got and it has noticed that the modeling thread
		// has not advanced. That basically means that it's done, and we
		// should join it.
		//
		if(trailing_thread->Done.load()) {

#		    ifdef DEBUG_THIS
		    estr << "thread " << k
			    << " is done; breaking out of thread loop\n";
		    excl << estr;
		    estr.undefine();
#		    endif /* DEBUG_THIS */

		    the_thread_is_done = true;
		    break;
		}
		if(trailing_thread->ErrorsFound.load()) {

#		    ifdef DEBUG_THIS
		    estr << "thread " << k
			    << " found errors; breaking out of thread loop\n";
		    excl << estr;
		    estr.undefine();
#		    endif /* DEBUG_THIS */

		    the_thread_is_done = true;
		    break;
		}
		if(trailing_thread->WaitingForData.load()) {

#		    ifdef DEBUG_THIS
		    estr << "thread " << k
			    << " is waiting for data; breaking out of thread loop\n";
		    excl << estr;
		    estr.undefine();
#		    endif /* DEBUG_THIS */

		    //
		    // The thread has finished crunching on its data
		    // and is waiting for more. We are going to give
		    // it more below.
		    //
		    break;
		}
		dual_purpose_iterator::sleep_for_ms(10);
	    }

	    if(the_thread_is_done) {

#		ifdef DEBUG_THIS
		estr << "thread " << k
			<< " is done; moving on.\n";
		excl << estr;
		estr.undefine();
#		endif /* DEBUG_THIS */

		//
		// Nothing to do
		//
		continue;
	    }

	    //
	    // No need to delete the old vector; the trailing thread
	    // will take care of it
	    //
	    lock_guard<mutex>	lock(trailing_thread->safe_vector_protector);

	    //
	    // One thing we need to make sure of: the activity end node
	    // has no constraints. That means the iterNode should be
	    // NULL inside the iterator.
	    //
	    new_vector = potential_triggers.save_safe_nodes();
	    trailing_thread->theSafeVectorHasBeenCaptured.store(false);
	    trailing_thread->SafeVector.store(new_vector);

	    //
	    // We no longer need to check on this thread; it process
	    // the new data, then notice that we are done and exit
	    // so we can join it after this call returns.
	    //
	}
    }

#   ifdef DEBUG_THIS
    estr << "feed data: done; setting Done flag.\n ";
    excl << estr;
    estr.undefine();
#   endif /* DEBUG_THIS */

    //
    // Now we tell the threads that we are done; they will exit,
    // so we can join them after returning from this call.
    //
    modeling_thread->Done.store(true);
}

//
// time advance detector
//
// This method is called when one or more events have just been processed
// at time time_of_last_event, and either
//
//	- there is no next event in the event queue
//
// 	- the next event in the queue has a time tag that is greater than
// 	  time_of_last_event.
//
// This method will try to find a waiting thread which can either
//
// 	- proceed right now, so that it is not necessary to advance time.
// 	  Such a thread was put on indefinite hold, because it hit a
// 	  conditional wait and it was not clear whether the thread
// 	  could ever be revived. Therefore, the current event
// 	  queue does not contain any events related to this thread.
// 	  Since the execution stack required for thread execution is
// 	  normally stored in the event at which the thread should execute
// 	  or resume, this means that some spot had to be found where to
// 	  store the stack of the sleeping thread. This spot was inside
// 	  the node which keeps information about the waiting thread.
// 	  Before returning, the method will grab the execution context
// 	  of the waiting thread, stick it in the execution stack provided
// 	  by the caller, and delete the node containig the thread information.
//
// 	  Note that threads that are put on hold as a result of a
// 	  "predictable wait" are going to be woken up automatically, since
// 	  wait processing resulted in a new event being inserted into the
// 	  event queue with a time tag equal to the wake-up time. Such
// 	  threads are never returned by the method.
//
// 	- proceed at a time that is greater than now, but less than the
// 	  time of the next event in the queue if it exists
//
// The method will return true in either case; it will return false if it
// cannot find a waiting thread that is ready to proceed.
//
// When the method returns true, it has put the previously saved context
// for execution of the waiting thread into the stack provided by the caller.
//
bool EventLoop::can_a_thread_be_unlocked(
		CTime_base		time_of_last_event,
		mEvent*			next_event_in_queue,
		pEsys::execStack&	thread_to_wake_up
		) {
	bool	a_waiting_thread_can_go_forward = false;

	try {

		//
		// we first try to find a waiting thread that's ready to go now
		//
		a_waiting_thread_can_go_forward = one_thread_can_be_unblocked_now(
				time_of_last_event,
				thread_to_wake_up);

		if(a_waiting_thread_can_go_forward) {
			return true;
		}
	}
	catch(eval_error Err) {
		Cstring s;
		s << "EventLoop::can_a_thread_be_unlocked: error in one_thread_can_be_unblocked_now(); "
			"details:\n" << Err.msg;
		throw(eval_error(s));
	}

	//
	// OK, we could not unblock anything, so we will have to really advance time.
	// First, try to find a waiting activity that could be unlocked at a time
	// strictly between time_of_last_event and next_event_time if it exists:
	//
	CTime_base proposed_new_time = time_of_last_event;

	try {
		a_waiting_thread_can_go_forward = one_thread_can_be_unblocked_in_the_near_future(
				time_of_last_event,
				next_event_in_queue,
				proposed_new_time,
				thread_to_wake_up);

		//
		// let's verify that the call did its job:
		//
		if(a_waiting_thread_can_go_forward) {
			assert(proposed_new_time > time_of_last_event);
			if(next_event_in_queue) {
				assert(proposed_new_time < next_event_in_queue->getetime());
			}
		}
	}
	catch(eval_error Err) {
		Cstring s;
		s << "EventLoop::can_a_thread_be_unlocked: error in "
			"one_thread_can_be_unblocked_in_the_near_future(); details:\n"
			<< Err.msg;
		throw(eval_error(s));
	}

	try {
		if(a_waiting_thread_can_go_forward) {

			//
			// may throw:
			//
			advance_now_to(proposed_new_time);
		} else if(!next_event_in_queue) {

			//
			// let's check constraints one last time
			//
			advance_now_to(	time_of_last_event,
					/* tolerate_equal_times = */	true,
					/* force_constraint_eval = */	true,
					/* last_chance = */		true);
		}
	} catch(eval_error Err) {
		Cstring s;
		s << "EventLoop::can_a_thread_be_unlocked: error in advance_now_to(); details:\n"
			<< Err.msg;
	}

	if(a_waiting_thread_can_go_forward) {
		return true;
	}
	return false;
}

// #define HELPFUL_MESSAGES

bool	EventLoop::try_signals_first(
		pEsys::execStack&	found_this_one) {
	instruction_node*			i_n;
	static const char*			error_id;
	Cntnr<alpha_int, instruction_node*>*	aptr;
	Cntnr<alpha_int, instruction_node*>*	aptr2;
	bool					something_happened = false;

	for(	aptr = eval_intfc::actsOrResWaitingOnSignals().first_node();
			aptr;
			aptr = aptr2) {
		Cstring			identifier;
		behaving_element&	ambient = aptr->payload->Calls.get_context()->AmbientObject;

		something_happened = false;
		aptr2 = aptr->next_node();

		i_n = where_to_go().find(aptr->getKey().get_int());
		if(!i_n) {
			Cstring errors;
			errors << "try_signals_first: Hey! nowhere to go for " << ambient->get_id() << "\n";
			throw(eval_error(errors));
		}
		TypedValue CriterVal;
		try {
			error_id = "Criterion";
			i_n->Criterion->eval_expression(i_n->theSymbols.object(), CriterVal);
		} catch(eval_error Err) {
			throw(eval_error(Err));
		}
		assert(CriterVal.is_string());
		if(i_n->cond_is_a_reg_exp) {

			//
			// NOTE: best would be to store regexp structures in eval_intfc. Add methods to
			//	 retrieve signals based on a string, and based on a pattern.
			//
			if(eval_intfc::a_signal_matches(CriterVal.get_string())) {
				something_happened = true;
			}
		} else if(eval_intfc::find_a_signal(CriterVal.get_string())) {
			something_happened = true;
		}
		if(something_happened) {
			Cstring errors;
			delete aptr;

			//
			// we steal the stack:
			//
			found_this_one.push_context(i_n->Calls.get_context());
			delete i_n;
			return true;
		}
	}
	return false;
}

//
// Looks for activities and resources that are waiting on a condition
// which has suddenly become true.
//
bool EventLoop::try_conditions_next(
		const CTime_base&	eval_time,
		pEsys::execStack&	found_this_one) {
	instruction_node*			i_n;
	static const char*			error_id;
	Cntnr<alpha_int, instruction_node*>*	aptr;
	Cntnr<alpha_int, instruction_node*>*	aptr2;
	CTime_base				requested_duration(0, 0, true);
	bool					something_happened = false;
	time_saver				ts;

	ts.set_now_to(eval_time);

	for(	aptr = eval_intfc::actsOrResWaitingOnCond().first_node();
			aptr;
			aptr = aptr2) {
		behaving_element&	ambient = aptr->payload->Calls.get_context()->AmbientObject;

		something_happened = false;
		aptr2 = aptr->next_node();

		i_n = where_to_go().find(aptr->getKey().get_int());
		if(!i_n) {
			Cstring errors;
			errors << "try_conditions_next: Hey! nowhere to go for activity "
				<< ambient->get_id() << "\n";
			throw(eval_error(errors));
		}
		try {
			error_id = "Criterion";
			TypedValue condition_waited_upon;
			i_n->Criterion->eval_expression(i_n->theSymbols.object(), condition_waited_upon);

			//
			// We are waiting for a bool expression to become true
			//
			assert(condition_waited_upon.is_boolean());
			if(condition_waited_upon.get_int() > 0) {
				something_happened = true;
			}
		}
		catch(eval_error Err) {
			throw(eval_error(Err));
		}
		if(something_happened) {
			delete aptr;

			//
			// we steal the stack:
			//
			found_this_one.push_context(i_n->Calls.get_context());
			delete i_n;
			return true;
		}
	}

	return false;
}


//
// Tries to unblock threads (activities and resources) that are
// waiting on a signal, a condition, or a scheduling opportunity.
// 
// The current time Tcurrent is the time tag of the last event
// just processed.  The method will look for a trigger (typically
// a change in a resource value or a signal) with a time tag T
// equal to Tcurrent.
//
bool EventLoop::one_thread_can_be_unblocked_now(
		const CTime_base	Tcurrent,		// won't be touched
		pEsys::execStack&	found_this_thread) {	// will contain the thread if we found it
	bool		result = false;
	value_node*	trigger;
	dbgindenter	Indenter;

	// try signals first
	if(eval_intfc::number_of_recent_signals()) {
		result = try_signals_first(found_this_thread);
		if(result) {

			//
			// never changes Tcurrent, no need to update
			//
			return true;
		}
	}

	bool we_tried_conditions = false;
	CTime_base last_attempt_for_cond;
	while((trigger = potential_triggers.peek())) {
		CTime_base	trigger_time = trigger->getKey().getetime();
		RCsource*	the_container = trigger->list->Owner->get_container();

		if(trigger_time < Tcurrent) {

			//
			// we are catching up to the current time - keep going
			//
			potential_triggers.next();

			continue;
		}

		if(trigger_time > Tcurrent) {

			//
			// we are not within the time limits
			//
			break;
		}

		//
		// we know that trigger_time == Tcurrent
		//
		if((!we_tried_conditions)
				&& containersToWatchForWaitUntil().find((void*) the_container)) {

			/* NOTE: this algorithm does not match blocked threads with the
			 * actual resources within a container that could unblock them.
			 * May want to revisit at some point. */

			result = try_conditions_next(
					trigger_time,
					found_this_thread);
			we_tried_conditions = true;
			if(result) {

				//
				// we commit to advancing the potential_triggers Miterator
				// 
				assert(trigger == potential_triggers.next());

				return true;
			} else {

				//
				// no luck
				//
			}
		}

		//
		// DO NOT DO THIS! Different triggers operating at the same
		// time will in general involve different resources; we don't
		// know which one(s) matter(s).
		//
		// while(	(trigger = potential_triggers.peek())
		//            && trigger->getKey().getetime() == trigger_time
		//   ) {
		//       potential_triggers.next(); }
		//

		//
		// keep looking for a successful trigger
		//
		assert(trigger == potential_triggers.next());
	}

	//
	// Following that, we should check any associative triggers. Note that
	// they may not be synchronized with the timing of events in the main
	// loop, i. e., the time tag of an associative resource event may well
	// fall before that of the next event in the loop. We still have to
	// try and unblock any thread waiting on the associative resource,
	// even though the time is not the tag of the current event.
	//

	return false;
}

//
// Tries to unblock threads (activities and resources) that are
// waiting on a signal or a condition
// 
// The current time Tcurrent is the time tag of the last event
// just processed.  The method will look for a trigger (typically
// a change in a resource value or a signal) with a time tag T
// equal to or greater than Tcurrent, but less than the time
// tag of the next event (new_event) in the event queue, if it
// exists; new_event will be NULL if we have reached the end of
// the event queue.
//
bool EventLoop::one_thread_can_be_unblocked_in_the_near_future(
		//
		// will be set to the new event
		// time T if we find one:
		//
		CTime_base		Tcurrent,
		mEvent*			new_event,
		CTime_base&		proposed_time,
		pEsys::execStack&	found_this_thread) {
	bool		result = false;
	value_node*	trigger;

	assert((!new_event) || new_event->getKey().getetime() > Tcurrent);

	bool we_tried_conditions;
	CTime_base last_attempt_for_cond;
	while((trigger = potential_triggers.peek())) {
		CTime_base	trigger_time = trigger->getKey().getetime();
		RCsource*	the_container = trigger->list->Owner->get_container();

		if(trigger_time <= Tcurrent) {

			//
			// we are catching up to the current time - keep going
			//
			potential_triggers.next();
			continue;
		}

		if(new_event && trigger_time >= new_event->getKey().getetime()) {

			// we are not within the time limits

			break;
		}

		if(trigger_time > last_attempt_for_cond) {
			we_tried_conditions = false;
		}

		if((!we_tried_conditions)
				&& containersToWatchForWaitUntil().find((void*) the_container)) {

			/* NOTE: this algorithm does not even try to match
			 * blocked threads with the resources that could
			 * unblock them. May want to revisit at some point. */

			result = try_conditions_next(
					trigger_time,
					found_this_thread);
			we_tried_conditions = true;
			last_attempt_for_cond = trigger_time;

			if(result) {

				//
				// we commit to advancing the potential_triggers Miterator.
				// Note that in this one case, trigger time is ahead of
				// current time, which has not yet been updated.
				//
				assert(trigger == potential_triggers.next());
				proposed_time = trigger_time;
				return true;
			} else {
				// no luck
			}
		}

		//
		// DO NOT DO THIS! Different triggers operating at the same
		// time will in general involve different resources; we don't
		// know which one(s) matter(s).
		// while(	(trigger = potential_triggers.peek())
		// && trigger->getKey().getetime() == trigger_time
		// ) {
		// potential_triggers.next();
		// }
		//
		assert(trigger == potential_triggers.next());
	}

	return false;
}

void EventLoop::advance_now_to(
		const CTime_base&	new_time,
		bool 			tolerate_equality,
		bool 			force_constraint_eval,
		bool			last_chance) {
    CTime_base	the_current_time = time_saver::get_now();
    bool	advancing_time = new_time != the_current_time;

    if((!advancing_time) && (!tolerate_equality)) {

	//
	// inconsistent choice of options:
	//
	Cstring t("advance_now: equal times ");
	t << new_time.to_string();
	throw(eval_error(t));
    }

    if(model_control::get_pass() == model_control::SCHEDULING_2) {

	//
	// We should only execute the code below when we are absolutely
	// certain that we have nothing else to do NOW
	//
	value_node*				old_event;
	tlist<alpha_void, Cntnr<alpha_void, value_node*> > ptrs;
	slist<alpha_void, Cntnr<alpha_void, value_node*> >::iterator piter(ptrs);
	Cntnr<alpha_void, value_node*>*	ptr;
	value_node*				on; // other node
	TypedValue*				modeling_only_value;

	while((old_event = old_events.peek())) {
	    if(old_event->getKey().getetime() <= new_time) {
		assert(old_event == old_events.next());
		Rsource*	rs = old_event->list->Owner;
		if(	old_event->Key.get_event_id() <= mEvent::lastEventIDbeforeScheduling
			&& (!old_event->list->Owner->isFrozen())
			&& !rs->properties[(int)Rsource::Property::has_modeling_only]) {

		    if(!ptrs.find((void*)old_event)) {

			ptrs << new Cntnr<alpha_void, value_node*>(old_event, old_event);
			if((on = old_event->get_other())) {
				assert(!ptrs.find(on));
				ptrs << new Cntnr<alpha_void, value_node*>(on, on);
			}
		    }
		}
	    } else {
		break;
	    }
	}

	while((ptr = piter())) {
	    old_event = ptr->payload;

	    try {

		//
		// Prevent conflict with resource access methods such
		// as evaluate_present_value(), which are used by
		// slave threads
		//
		lock_guard<mutex> Lock(old_event->list->Owner->res_mutex);
		delete old_event;
	    } catch(eval_error Err) {
		throw(Err);
	    }
	}
    }

    //
    // do this last
    //
    if(advancing_time) {

	//
	// Check whether it's time to let established
	// trailing threads move forward
	//
	time_advance_count++;
	if(time_advance_count > TIME_ADVANCES_BETWEEN_TRAILING_THREAD_UPDATES) {

	    //
	    // Reset the time advance counter
	    //
	    time_advance_count = 0;

#	    ifdef DEBUG_THIS
	    Cstring str;
	    str << "\n\tmodeling thread hit "
		<< TIME_ADVANCES_BETWEEN_TRAILING_THREAD_UPDATES
		<< " time advances; scanning thread to feed them data...\n";
	    excl << str;
#	    endif /* DEBUG_THIS */

	    //
	    // Scan all trailing threads
	    //
	    for(int k = 0; k < thread_intfc::MITER_THREADS; k++) {

		//
		// The threads were created in create_subsystems are
		// NULL except for the modeling thread.
		//
		thread_intfc* the_thread = thread_intfc::threads()[k].get();
		if(	the_thread

			//
			// No, we do not check this; all trailing threads
			// need vectors of safe nodes. Dependencies within
			// trailing threads are handled by trailing_miter.
			//
			/* && the_thread->theThreadWeTrail == thread_intfc::MODEL_THREAD */
		  ) {

#		    ifdef DEBUG_THIS
		    str.undefine();
		    str << "\tfound a thread following us; "
			    << "checking that it has read its vector...\n";
		    excl << str;
#		    endif /* DEBUG_THIS */

		    //
		    // Make sure the trailing thread is done with its current
		    // task and is waiting for more data
		    //
		    if(the_thread->theSafeVectorHasBeenCaptured.load()) {

#			ifdef DEBUG_THIS
			str.undefine();
			str << "\tit has.\n";
			excl << str;
#			endif /* DEBUG_THIS */

			//
			// This has already been done by ProcessEvents()
			// just before calling this method:
			//
			// thread_intfc::update_current_time(new_time);

			//
			// Prepare a new vector of "safe nodes" for the
			// thread's exclusive use
			//
			safe_vector_info* new_vector
			    = potential_triggers.save_safe_nodes();

#			ifdef DEBUG_THIS
			str.undefine();
			str << "\tmodeling thread storing a new vector in thread " << k << "\n";
			excl << str;
#			endif /* DEBUG_THIS */

			//
			// We can safely do this because the trailing
			// thread has already captured the previous
			// vector:
			//
			the_thread->SafeVector.store(new_vector);

			//
			// Now we set the flag to false, which will kick
			// the waiting thread into action:
			//
			the_thread->theSafeVectorHasBeenCaptured.store(false);
		    } else {

			#ifdef DEBUG_THIS
			str.undefine();
			str << "\tit has not.\n";
			excl << str;
#			endif /* DEBUG_THIS */
		    }
		}
	    }
	}

	//
	// Launch the TOL thread(s) if necessary... but not before
	// the requested start time:
	//

	//
	// We request that new_time be strictly greater than the
	// start time of the TOL, because that is precisely the
	// situation where the current time point has been fully
	// processed and resources have reached their final value.
	//
	WRITE_TOLrequest*	tol_req;
	WRITE_XMLTOLrequest*	xmltol_req;
	if(	(tol_req = WRITE_TOLrequest::in_process())
		&& !WRITE_TOLrequest::in_process()->running
		&& new_time > tol_req->get_start()) {
	    WRITE_TOLrequest::in_process()->running = true;

	    thread_intfc* TT = thread_intfc::threads()[thread_intfc::TOL_THREAD].get();
	    TT->prelude(the_current_time);

#	    ifdef DEBUG_THIS
	    Cstring str;
	    str << "\tadvance time: starting TOL thread\n";
	    excl << str;
#	    endif /* DEBUG_THIS */

	    TT->Thread = std::thread(WRITE_TOLrequest::execute, TT);

	    //
	    // The new thread will sleep until fed a fresh vector.
	    // If there is a constraint-checking thread, we should
	    // grab the new thread from it; if there isn't, we
	    // should compute the thread ourselves.
	    //
	    // The trick is to retrieve the last vector received from
	    // the constraint-checking thread in a thread-safe manner.
	    // Each thread_intfc instance is equipped with a mutex
	    // that protects access to the last safe vector processed.
	    // We lock the mutex, retrieve the safe vector, pass a
	    // copy of it to the TOL thread, and _then_ release the
	    // mutex.
	    //
	    safe_vector_info* new_vector = NULL;
	    thread_intfc* CT = 
	    	thread_intfc::threads()[thread_intfc::CONSTRAINT_THREAD].get();
	    if(CT) {

#		ifdef DEBUG_THIS
		excl << "  getting vector from constraints...\n";
#		endif /* DEBUG_THIS */

		new_vector = thread_intfc::get_copy_of_safe_vector_from(CT);
		if(!new_vector) {

#		    ifdef DEBUG_THIS
		    excl << "  no luck; creating our own...\n";
#		    endif /* DEBUG_THIS */

		    //
		    // We pass our own safe nodes. Hopefully everything
		    // will be all right.
		    //
		    new_vector = potential_triggers.save_safe_nodes();
		}
	    } else {

#		ifdef DEBUG_THIS
		excl << "  getting vector from our triggers...\n";
#		endif /* DEBUG_THIS */

		new_vector = potential_triggers.save_safe_nodes();
	    }

#	    ifdef DEBUG_THIS
	    excl << "  model thread storing vector in TOL thread\n";
#	    endif /* DEBUG_THIS */

	    TT->SafeVector.store(new_vector);

	    //
	    // Wait until the new thread has set up its flags:
	    //
	    while(!TT->theSafeVectorHasBeenCaptured) {
		thread_intfc::sleep_for_ms(10);
	    }
	}

	//
	// We request that new_time be strictly greater than the
	// start time of the TOL, because that is precisely the
	// situation where the current time point has been fully
	// processed and resources have reached their final value.
	//
	if(	(xmltol_req = WRITE_XMLTOLrequest::in_process())
		&& !WRITE_XMLTOLrequest::in_process()->running
		&& new_time > xmltol_req->get_start()) {
	    WRITE_XMLTOLrequest::in_process()->running = true;

	    thread_intfc* XT = thread_intfc::threads()[thread_intfc::XMLTOL_THREAD].get();
	    XT->prelude(the_current_time);

#	    ifdef DEBUG_THIS
	    excl << "    advance time: starting XMLTOL thread\n";
#	    endif /* DEBUG_THIS */

	    XT->Thread = std::thread(WRITE_XMLTOLrequest::execute, XT);

	    //
	    // The new thread will sleep until fed a fresh vector; see
	    // comments above for the TOL case.
	    //
	    safe_vector_info* new_vector = NULL;
	    thread_intfc* CT = 
	    	thread_intfc::threads()[thread_intfc::CONSTRAINT_THREAD].get();
	    if(CT) {

#		ifdef DEBUG_THIS
		excl << "  getting vector from constraints...\n";
#		endif /* DEBUG_THIS */

		new_vector = thread_intfc::get_copy_of_safe_vector_from(CT);
		if(!new_vector) {

#		    ifdef DEBUG_THIS
		    excl << "  no luck; creating our own...\n";
#		    endif /* DEBUG_THIS */

		    //
		    // We pass our own safe nodes. Hopefully everything
		    // will be all right.
		    //
		    new_vector = potential_triggers.save_safe_nodes();
		}
	    } else {

#		ifdef DEBUG_THIS
		excl << "  getting vector from our triggers...\n";
#		endif /* DEBUG_THIS */

		new_vector = potential_triggers.save_safe_nodes();
	    }

#	    ifdef DEBUG_THIS
	    excl << "  model thread storing vector in TOL thread\n";
#	    endif /* DEBUG_THIS */

	    XT->SafeVector.store(new_vector);

	    //
	    // Wait until the new thread has set up its flags:
	    //
	    while(!XT->theSafeVectorHasBeenCaptured) {
		thread_intfc::sleep_for_ms(10);
	    }
	}
	modelingTime.set_now_to(new_time);
    }
}

bool EventLoop::destroy_all_threads(Cstring& append_errors) {

    bool found_errors = false;

    //
    // IMPORTANT NOTE
    // ==============
    // For this loop to work correctly, the threads
    // must be ordered such that a thread can only trail
    // another thread with a smaller index.
    //
    for(int k = thread_intfc::MITER_THREADS - 1; k > 0; k--) {
	thread_intfc* the_thread = thread_intfc::threads()[k].get();
	if(the_thread && the_thread->Thread.joinable()) {
	    the_thread->Thread.join();
	    if(the_thread->ErrorsFound.load()) {
		append_errors << "\n" << the_thread->errors;
		found_errors = true;
	    }
	    thread_intfc::threads()[k].reset();
	}
    }
    return found_errors;
}
