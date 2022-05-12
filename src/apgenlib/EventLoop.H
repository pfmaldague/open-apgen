#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_

#include "ThreadIntfc.H"

#include "APmodel.H"
#include "Rsource.H"
#include "res_multiterator.H"
#include "ExecutionContext.H"

class WRITE_TOLrequest;
class WRITE_XMLTOLrequest;

//
// A dual_purpose_iterator is a thin wrapper around
// one of the following two Miterator objects, depending
// on the thread_intfc pointer passed to it:
//
//	- if the pointer is NULL, the internal object is
//	  a res_curval_miter
//
//	- if the pointer is a valid address, the internal
//	  object is a trailing_miter
//
// Using an instance of this wrapper class makes it possible
// for a high-level function that iterates over resource
// histories to operate either in single- or multi-threaded
// mode.
//
// Design requirement: the client of this class should not have
// to worry about whether the internal Miterator is a res_curval_miter
// or a trailing_miter; the interface should be the same for both.
// This is going to take some work...
//
// A good way to start is to retrofit Constraint::check_constraints(),
// which already uses the dual-purpose version, so it uses the new
// interface.
//
class dual_purpose_iterator {
public:
	dual_purpose_iterator() = delete;
	dual_purpose_iterator(const dual_purpose_iterator&) = delete;
	dual_purpose_iterator(thread_intfc* T);
	~dual_purpose_iterator();

	//
	// NOTE: the MiterPtr always references a non-NULL object;
	//
	smart_ptr<
	    Miterator_base<
		slist<prio_time, value_node, Rsource*>,
		value_node > >		MiterPtr;

	//
	// Adding threads to MiterPtr and removing threads
	// (e. g. while deleting it) cannot be done concurrently
	// in parallel threads, because both require the
	// multiterator to register themselves with resource
	// histories. Hence we need a static mutex to prevent
	// concurrency while performing those operations:
	//
	static mutex&			Mutex();

	thread_intfc*			Thread;

	Miterator_base<
		slist<prio_time, value_node, Rsource*>,
		value_node >&		operator*() {
	    return *MiterPtr;
	}
	Miterator_base<
		slist<prio_time, value_node, Rsource*>,
		value_node >*		operator->() {
	    return MiterPtr.object();
	}
	static void			sleep_for_ms(int i) {
	    this_thread::sleep_for(chrono::milliseconds(i));
	}

	//
	// This is used to efficiently initialize res_curval_miter
	// iterators to the start value of a TOL request. Only
	// used for standalone requests; when running TOL output
	// in parallel with modeling, a trailing_miter is used;
	// such an iterator is automatically initialized in such
	// a way that it is "ready to use" at the start time
	// of the TOL request.
	//
	void				initialize_at(CTime_base);

	
	//
	// In a simple Miterator, peek() returns NULL when the iterator
	// has reached the end of the list. We want a dual_purpose_iterator
	// to behave exactly the same way, and duplicate what the
	// constraint-checking thread does in its check_constraints()
	// method.
	//
	value_node*			peek();

	//
	// Returns the history node of R just before the current time
	//
	value_node*			current(Rsource* R);

	//
	// Returns an iterator for the list of activity ends
	// containing an iterNode that is the latest safe node
	// to use. If the iterNode is NULL, all nodes are safe;
	// this would be the case in a standalone scenario in
	// which the MiterPtr points to a res_curval_miter.
	//
	// In the case of a slave process, the following function
	// will get its information from the copy of the safe
	// vector held in the trailing_miter.
	//
};

class EventLoop {
public:

	EventLoop(time_saver&	master_time);
	~EventLoop() {}

	static EventLoop&	theEventLoop();
	static EventLoop*	theEventLoopPtr;
	static tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& containersToWatchForWaitUntil();
	static tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& containersToWatchForScheduling();
	static bool		try_scheduling(
					const CTime_base&	etime,
					pEsys::execStack&	found_this_thread);
	static bool		try_signals_first(
					pEsys::execStack&	found_this_thread);
	static bool		try_conditions_next(
					const CTime_base&	etime,
					pEsys::execStack&	found_this_thread);

	// vector<unique_ptr<thread_intfc::completion_signal_sender> > Senders;


	time_saver&		modelingTime;

	void			init_iterators();

	//
	// true return means errors were found; message
	// is appended to errs argument
	//
	bool			destroy_all_threads(Cstring& add_errors);

				//
				// for debugging optimization:
				//
	void			document_nodes(const Cstring&);

	bool			can_a_thread_be_unlocked(
					CTime_base		currTime,
					mEvent*			nextEvent,
					pEsys::execStack&	stack_for_old_thread);
	bool			one_thread_can_be_unblocked_now(
					CTime_base		currTime,
					pEsys::execStack&	found_this_thread);
	bool			one_thread_can_be_unblocked_in_the_near_future(
					CTime_base		currTime,
					mEvent*			nextEvent,
					CTime_base&		proposed_time,
					pEsys::execStack&	found_this_thread);

	void			ProcessEvents();

	void			advance_now_to(
					const CTime_base&	this_time,
					bool			tolerate_equal_times = false,
					bool			force_constraint_eval = false,
					bool			last_chance = false);

	//
	// To pass one more set of data value to the trailing
	// threads that are still active when the modeling
	// thread completes its task
	//
	void 			feed_last_data_to_trailing_threads();

	static mEvent*		CurrentEvent;
	static long int		time_advance_count;

	//
	// We will use this first Miterator to keep track of the boundary between
	// events ("modeling") pass - the "old events" - and those newly created
	// during the current ("scheduling") pass, the "new events".
	//
	// Needless to say, the boundary is moving. The precise way in which
	// the boundary should be managed is dictated by the following elementary
	// requirements:
	//
	//    - at the beginning of a step with time tag T, all old events whose
	// 	time tag is <= T should have been purged from resource histories;
	//
	//    - while the modeling/scheduling step at time T is in progress, events
	// 	result in the insertion of new nodes into resource histories;
	//
	//    - at the end of the modeling/scheduling process, the new events have
	//	been fully taken into account and the resource histories now reflect
	//	a fully consistent modeling run up to time T. Resource values beyond
	//	time T will be a mix of old events from the first pass and new events
	//	created by from ... to usage clauses in the modeling code.
	// 
	simple_res_miter	old_events;

	//
	// We will use this second Miterator to scan newly created resource value nodes and
	// detect any that have the potential of triggering a wake-up or a scheduling action.
	// A wake-up action consists in unblocking a thread that was blocked due to a
	// WAIT UNTIL Condition statement.
	//
	// In implementing the algorithm that keeps this Miterator current, we will have to
	// be aware of two complicating factors:
	//
	//    - new threads may be added as a result of processing brand new scheduling
	//	statements ("get_windows()")
	//
	//    - the timing of resource value nodes does not necessarily reflect that of
	//	events, since immediate clauses can create resource value nodes that have
	//	no counterparts in the event queues.
	//
	model_res_miter		potential_triggers;

};

#ifdef OBSOLETE
//
// Class for investigating currenval usage outside of standard case, i. e.,
// when now is not the current modeling time
//
class auditor {
public:
	typedef enum {
		get_windows,
		evaluate_profile,
		none } context;
	auditor(context C)
		: prev_context(global_context) {
		global_context = C;
	}
	~auditor() {
		global_context = prev_context;
	}
	context		prev_context;
	static context	global_context;
	static bool check(parsedExp e);
	static long total_count;
	static long bad_count;
	static long count_at_modeling_now;
};
#endif /* OBSOLETE */

#endif /* _EVENT_LOOP_H_ */
