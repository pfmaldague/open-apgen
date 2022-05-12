#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "Rsource.H"
#include "res_multiterator.H"
#include "EventLoop.H"
#include "ActivityInstance.H"
#include "ACT_exec.H"
#include "action_request.H"
#include "apcoreWaiter.H"

// #define ASSERT( x )
#define ASSERT( x ) assert(x)

// #define DEBUG_THIS

#ifdef DEBUG_THIS
extern coutx excl;
extern atomic<int> feed_flag;
#endif /* DEBUG_THIS */

extern streamx& tol_debug_stream();


//
// Important note: the model_vector _only_ contains safe node information
// from the modeling thread. It's up to the trailing_miter to initialize
// its own iterators correctly.
//
trailing_miter::trailing_miter(
	    const char* p,		// name of this iterator

	    thread_intfc& T,		// thread info for the thread this runs in

	    const safe_vector_info* M,	// "safe to use" snapshot of
					// the modeling thread behind
					// which this iterator trails
	    int thread_to_follow
	    )
	: res_curval_miter(p),
		active_trailers(true),
		Thread(T),
		the_previous_node(NULL),
		the_thread_we_trail(thread_to_follow),
		model_vector(M) {
	lock_guard<mutex>	lock(Thread.safe_vector_protector);

	// 
	// Define a new data structure holding both the vector of history
	// nodes and the time at which the thread that provides that node
	// (i. e. the modeling thread) currently sits ("Safe Time")
	//
	Thread.copy_of_safe_vector.reset(new safe_vector_info(*model_vector));

	Thread.time_of_last_processed_vector = model_vector->Time;

	//
	// Launchers should always wait for this to become true:
	//
	Thread.theSafeVectorHasBeenCaptured.store(true);
}


trailing_miter::~trailing_miter() {
	if(model_vector) {
		delete model_vector;
	}
	clear();
}

time_actptr* trailing_miter::get_safe_endnode() {
	lock_guard<mutex>	lock(Thread.safe_vector_protector);
	thread_intfc*		model_thread
		= thread_intfc::threads()[thread_intfc::MODEL_THREAD].get();
	if(model_thread->Done.load()) {
		return NULL;
	}
	return Thread.copy_of_safe_vector.get()->Iter().iterNode;
}

void trailing_miter::adopt_a_new_model_iterator(
		safe_vector_info* new_model_vector) {

	if(model_vector) {
		delete model_vector;
	}
	model_vector = new_model_vector;
	{
	    lock_guard<mutex>	lock(Thread.safe_vector_protector);
	    Thread.copy_of_safe_vector.reset(new safe_vector_info(*model_vector));
	}

	//
	// This lets the modeling thread override the snapshot again;
	// it's safe to let it know, because we've captured what we
	// needed already.
	//
	Thread.theSafeVectorHasBeenCaptured.store(true);

	//
	// If the new vector contains new data (because our master is done)
	// we have nothing to update:
	//
	if(!model_vector || model_vector->is_null()) {
		return;
	}

	//
	// Update our trailer pointers to reflect the new data
	//
	trailerptr*			ptr;
	trailerptrslist::iterator	iter(pointers_to_trailers);
	model_res_miter*		model_miter
		= &EventLoop::theEventLoop().potential_triggers;
	while((ptr = iter())) {
		trailer*			T = ptr->payload;
		baseListClass::safe_iter*	ic = T->payload;
		Cntnr<alpha_void, int>*	m_index
			= model_miter->thread_indices.find(ic->L);
		if(!m_index) {
			Cstring errs;
			errs << "trailing iterator " << name
				<< " cannot find the history list of "
				<< ic->L->Owner->name
				<< " in the modeling thread.";
			throw(eval_error(errs));
		}

		ic->safe_node = model_vector->Vector()[m_index->payload];
	}
}

//
// This method is invoked by the modeling thread, while processing the
// initial event in the Event Queue.
//
// When modeling, the logic is relatively straightforward.
//
// When scheduling (2nd pass), one needs to be careful that all prevNode
// members of iterators managed by the modeling thread should point to
// nodes that are safe, i. e., are not about to be destroyed by the
// modeling thread when scanning the old_events miterator. The logic
// used by the modeling thread should enforce this; it should not call
// slist::iterator::next(), which might inadvertently set prevNode to
// an about-to-be-destroyed node. Instead, it should set prevNode explicitly
// to a node that is known to be safe. This is handled by the logic below,
// when initializing the trailing iterator.
//
void trailing_miter::add_thread(
		baseListClass&	new_thread,
		const Cstring&	its_name,
		unsigned long	its_priority,
		bool		store
		) {

	thread_names << new Cntnr<alpha_void, Cstring>(
		(void*)&new_thread, its_name);

	//
	// Event if this trailing miterator gets its safe vectors
	// from another trailing thread, the iterators to watch
	// are still pointing to the history lists in the master
	// miterator, which is potential_triggers:
	//
	model_res_miter* model_miter = &EventLoop::theEventLoop().potential_triggers;

	//
	// First, we look for the new thread in model_miter. It is safe to
	// do this as long as model_miter was fully constructed (i. e., all
	// the important lists had been added) before the trailing thread
	// was started.
	//
	Cntnr<alpha_void, int>* m_index
		= model_miter->thread_indices.find((void*)&new_thread);
	if(!m_index) {
		Cstring errs;
		errs << "trailing iterator " << name
			<< " cannot find the history list of "
			<< new_thread.Owner->name
			<< " in the modeling thread.";
		throw(eval_error(errs));
	}

	//
	// Make sure you grab the safe_node from the previously saved
	// model_vector. The modeling thread (a.k.a.
	// EventLoop::ProcessEvents()) has made sure that a model vector
	// was supplied to the trailing_miter constructor.
	//
	// Question: what are the prevNode and iterNode members of the
	// trailing_iter we are creating? Ah, interesting: both are
	// initialized to the node passed as the second argument of
	// the safeClass (a safe_iter for the appropriate list, actually).
	// And that argument is the safe node identified by the modeling
	// thread!
	//
	// Thus, the first time peek() is invoked, it will return the
	// iterNode, which is the safe node just identified. OK! As
	// soon as next() is invoked, iterNode will point to the next
	// node, while prevNode is still at its previous location.
	//
	// Now comes the crucial part: when history nodes are added to
	// the list by the modeling thread, will iterNode always move
	// forward in time? If iterNode's time tag is at most the safe
	// time of the modeling thread just before advancing time, then
	// I would say yes. This needs to be checked.
	//
	//
	safeClass* trailing_iter = new safeClass(
					new_thread,
					model_vector->Vector()[m_index->payload]);

	trailer* Trailing = new trailer(
				relptr<safeClass>(trailing_iter, its_priority),
				trailing_iter);

	pointers_to_trailers << new trailerptr((void*)&new_thread, Trailing);
	trailers_by_res << new trailerptr(new_thread.Owner, Trailing);

	new_thread.register_miterator(*this, trailing_iter, store);

	//
	// The trailer is pointing to the initialization node in the
	// resource history, so it's always active:
	//
	active_trailers << Trailing;
}

value_node*
trailing_miter::current(void* res) {
    trailerptr*	N = trailers_by_res.find(res);
    ASSERT(N);
    trailer*	tt = N->payload;
    ASSERT(tt);

    if(APcloptions::theCmdLineOptions().TOLdebug) {
	Cstring resname = APcloptions::theCmdLineOptions().theResourceToDebug;
	value_node* prev = tt->payload->getPrev();
	value_node* Next = tt->payload->getNode();
	Rsource* res = tt->payload->getPrev()->list->Owner;
	if(res->name == resname) {
	    Cstring str;
	    str << resname << "->current(): returning Prev(tag = "
		<< prev->Key.getetime().to_string() << ", val = "
		<< prev->get_value().to_string() << "); next = (";
	    if(Next) {
		str << "tag = " << Next->Key.getetime().to_string() << ", val = "
		    << Next->get_value().to_string() << ")";
	    } else {
		str << "NULL)";
	    }
	    str << "\n";
	    tol_debug_stream() << str;
	}
    }

    //
    // We want the node suitable for evaluating a current value.
    // Normally, that would be <current iterator>->getPrev().
    // However, it could be that the current iterator has been
    // put on hold by next(), in which case the best node to
    // use for a current value is <current iterator>->getNode()
    // and not getPrev().
    //
    if(tt->list == &active_trailers) {
	return tt->payload->getPrev();
    } else if(tt->list == &trailers_on_hold) {
	return tt->payload->getNode();
    } else {
	assert(false);
    }
}

void trailing_miter::check_safe_vector() {

    //
    // This method is called from get_new_data(). Can we derive a
    // little "theorem" to the effect that the following boolean should
    // always be false?
    //
    // Yes:
    //
    //	     1.	trailing_miter::get_new_data() calls this function
    //		if and only if EventLoop::SafeVector contains a new value
    //
    //	     2.	EventLoop only stores new values in SafeVector in three
    //		places, and just before each one, theSafeVectorHasBeenCaptured
    //		is set to false.
    //
    bool to_check = Thread.theSafeVectorHasBeenCaptured.load();
    if(to_check) {

	//
	// Provide post-mortem analyzers with some idea of what happened:
	//
	cerr << "update nodes on hold: assertion failed - "
	     << "safe vector has been captured = true\n";
	ASSERT(!to_check);
    }
}

void trailing_miter::check_current_trailing_event(
		value_node* current_trailing_event) {

    //
    // This method is called from get_new_data(). Can we derive a
    // little "theorem" to the effect that the following value_node* should
    // always be non-NULL?
    //
    // Let's try. First, recall the definition of current_modeling_event:
    //
    // 1. baseListClass is slist<prio_time, value_node, Rsource*>; this is
    //    the class to which resource histories belong
    //
    // 2. safeClass is baseListClass::safe_iter, a "safe iterator" for a
    //    resource history list. safe_iter consists of the following:
    //
    //        - the iterator base class, which contains pointers to
    //		the list that is iterated on and to the "current node"
    //		of that list. The "current node" of an iterator is the
    //		node returned by the last call to next(), or NULL if
    //		next() was never called.
    //
    //	      - the curval_iter class, a special-purpose iterator tailored
    //  	to the needs of currentval calculations. In addition to
    //  	the members of iterator, it has a prevNode pointer.
    //
    //
    if(!current_trailing_event) {
	//
	// Provide post-mortem analyzers with some idea of what happened:
	//
	cerr << "update nodes on hold: assertion failed; current_trailing_event = NULL\n";
	ASSERT(current_trailing_event);
    }
}

bool trailing_miter::check_event_timing(value_node* current_trailing_event) {
    if(current_trailing_event->Key.getetime().get_pseudo_millisec()
			   > thread_intfc::threads()[the_thread_we_trail]->SafeTime) {

	if(the_thread_we_trail != thread_intfc::MODEL_THREAD) {

		//
		// semantics: do _not_ use this node; it's ahead
		// of where the constraint-checking thread is.
		//
		return false;
	}

	//
	// Provide post-mortem analyzers with some idea of what happened:
	//
	cerr << "update nodes on hold: assertion failed;\n";
	cerr << "Trailing time "
		<< current_trailing_event->Key.getetime().to_string()
		<< " is greater than event loop time "
		<< CTime_base(thread_intfc::threads()[the_thread_we_trail]->SafeTime).to_string()
		<< ".\n";
	ASSERT(current_trailing_event->Key.getetime().get_pseudo_millisec()
		<= thread_intfc::threads()[the_thread_we_trail]->SafeTime);
    }

    //
    // OK to use this node
    //
    return true;
}

//
// Returns true if the active_trailers list is not empty
//
trailing_miter::trailer* trailing_miter::get_new_data() {

    //
    // If there are no constraints, we are following the
    // modeling thread directly. So, there may not be a
    // "super master".  The safest thing to do is to define
    // the super master as the modeling thread. It may be
    // identical to our_master but that's OK.
    //
    thread_intfc& modeling_thread = *thread_intfc::threads()[thread_intfc::MODEL_THREAD].get();
    thread_intfc& our_master = *thread_intfc::threads()[the_thread_we_trail].get();
    CTime_base curTime = Thread.current_time(thread_index);
    CTime_base our_master_s_current_time;

#   ifdef DEBUG_THIS
    Cstring tstr;
#   endif /* DEBUG_THIS */

    bool dbg = APcloptions::theCmdLineOptions().TOLdebug;
    Cstring& resdbg = APcloptions::theCmdLineOptions().theResourceToDebug;

    //
    // Any Miterator contains a list of iterators, each one
    // pointing to a 'future node' within the history of a
    // resource. In a normal iterator, these iterators are
    // the payloads of the "Tthreads" stored in the active
    // list. The time tag of a Tthread is that of the future
    // node pointed to by the iterator in the Tthread's payload.
    //
    // An important time is the time tag of the earliest Tthread
    // in the active list; it is the time revealed by a "peek"
    // or "next" call to the Miterator. Let's refer to it as
    // the "next time".
    //
    // Another important time is the last time returned by a
    // previous call to next(). That time, now forgotten, can
    // be thought of as the "current time" of the Miterator.
    //
    // The state of the Miterator can be viewed as a coordinated
    // state of all resources. In this state, there is a time
    // interval from "current time" to "next time". Because several
    // resources may have nodes with the same time tag, there is
    // a need to disambiguate such resource history nodes; the
    // disambiguation mechanism is based on a priority value which
    // is associated with each resource, and guarantees that the
    // set of nodes in a Miterator is always well-ordered. Thus,
    // the "next time" may be equal to the "current time" for a while.
    // As long as that is the case, calls to the Miterator's next()
    // method will return nodes with the same tag but increasing
    // priority numbers (i. e., decreasing priority; a low priority
    // number means "higher priority").
    //
    // Each call to next() results in a Tthread being removed from
    // the active list, its payload updated via a call to its own
    // next() method (which advances it within a resource history),
    // and the node being re-inserted into the active list. However,
    // it may be that the payload in question was pointed at the
    // last node in the history of the resource, in which case it
    // cannot really be "advanced" beyond where it is. When that
    // happens, the Tthread is simply removed from the active list
    // and inserted into the "dead list". This list contains Tthreads
    // whose payloads are stuck on the last history node of their
    // resource. No further nodes can be extracted from the
    // corresponding resource histories.
    //
    // As time goes on, more and more Tthreads end up in the dead
    // list, until eventually they all do, at which point the
    // Miterator has reached the end of its iteration loop.
    //

    //
    // The above was for ordinary Miterators. Now let us talk about
    // curval miterators. Instead of Tthreads, we talk about trailers,
    // which follow the same logic as Tthreads. However, the payloads
    // of trailers are more sophisticated than those of Tthreads:
    // they are so-called "safe iterators", which in addition to the
    // iterNode familiar to Tthreads also contain a pointer to a
    // prevNode, which is a node that precedes the iterNode. Just as
    // before, that does not mean that the two have different time
    // tags; the time tags may be the same, in which case some priority
    // scheme makes the ordering unique.
    //
    // Note: in the res_curval_miter class, trailers are still called
    // Tthreads. However, for the purposes of this discussion it is
    // easier to assume that Tthreads have been given the new name
    // "trailer".
    //
    // A trailer is a time-based node whose payload is a safe
    // iterator. The safe iterator has an iterNode which points
    // to the next node it would return, and a safeNode (a.k.a.
    // prevNode) which represents the latest node at or before
    // the current time.
    //
    // Now let's revisit the notion of the "state" of the curval
    // miterator. As before, this state is characterized by a time
    // interval from "current time" to "next time", where the two
    // may actually be equal. Unlike a Tthread, a trailer remembers
    // the last node that it returned: that is the prevNode of the
    // safe iterator in its payload. That prevNode is basically the
    // correct node to invoke when attempting to determine the
    // current value of a resource (ignoring issues related to the
    // resource's profile, which may be complex). This property of
    // safe iterators explains in a nutshell why they are useful
    // in evaluating resource values, be it inside a call to get
    // windows() or in a loop that outputs resource values to a TOL.
    //

    //
    // Finally, let us look at trailing miterators, which add yet
    // another level of complexity beyond that of curval miterators
    // because they are used inside execution threads that lag
    // behind the modeling thread, whence their name. Trailing
    // miterators, and in particular their trailers, do not advance
    // their payloads freely, the way curval trailers do. The reason
    // is that they can only advance up to a certain point which is
    // "safely" earlier than the modeling thread. If a trailing
    // miterator tried to advance any of its nodes beyond the time
    // that is currently being worked on by the modeling thread,
    // all hell would break loose.
    //
    // How do trailing miterators stay safe? The key concept here
    // is that of a "safe vector". A safe vector is created by the
    // modeling thread from time to time, when it is about to advance
    // modeling time ("now") from its current value T0 to the next
    // time value in the event queue, T1. The modeling thread
    // maintains a special miterator called _potential triggers_.
    // The state of potential triggers, like the state of any
    // miterator, consists of a time interval from "current time"
    // to "next time". But in this case, there is an additional
    // guarantee, which is maintained by the modeling thread:
    // "current time" is <= T0 while "next time" is > T0 or
    // undefined because the iterators inside the Tthreads don't
    // have a next node (the history terminates at T0 or before).
    // The _safe vector_ is a cross section through _all_ resources.
    // For each resource, the safe vector contains a pointer to
    // the latest history node whose time tag is <= T0. This
    // history node is called the _safe node_ for that resource.
    // By construction and by virtue of a basic "contract" with
    // the modeling thread, either a safe node has no successor,
    // meaning that it is the last node in its resource's history,
    // or it has a successor whose time tag is strictly later
    // than T0.
    //
    // The time tag T0 at which a safe vector was captured by the
    // modeling thread is stored as a data member of the safe
    // vector.
    //

    //
    // From time to time, the modeling thread feeds a new safe
    // vector to each trailing thread. Such a safe vector allows
    // a trailing thread to iterate safely, using the algorithm
    // explained below.
    //
    // At each point of its operation, a trailing miterator has
    // a safe vector which it received from the modeling thread;
    // that safe vector has a capture time Ts which is strictly
    // greater than the _current time_ of the trailing miterator.
    // Like a curval miterator, a trailing miterator has two
    // lists of trailers, one of which is known as the _active
    // list_. While a curval miterator has a _dead list_, a
    // trailing miterator has a list of _trailers on hold_.
    // Because a trailing miterator operates before the modeling
    // thread completes, it has no way of knowing whether a
    // history list is complete or not. What is knows, thanks
    // to the _safe vector_ provided by the modeling thread,
    // is that a history node can be safely advanced as long
    // as it is not equal to the _safe node_ stored in the safe
    // vector for that resource. When a iterNode in the payload
    // of a trailer reaches the _safe node_ of its resource,
    // the trailer is removed from the _active list_ and moved
    // to the list of _trailers on hold_. This takes place
    // inside the trailing miterator's next() method; it does
    // return a pointer to the iterNode in the trailer's
    // payload, but the trailer is put on hold because the
    // iterNode is equal to the safe node for that resource.
    //
    // Whenever peek() or next() is called after a trailer is
    // put on hold, the first thing the trailing miterator
    // does is invoke its get_new_data() method, which checks
    // whether a new _safe vector_ might be available.
    //
    // UUUUU
    //

    //
    // There are two kinds of trailers, those in active_trailers
    // and those in trailers_on_hold. Important: both of them
    // have a valid safeNode! However, those in trailers_on_hold
    // cannot be advanced, because if they have a next_node, that
    // next_node lies beyond the "safe zone" in potential_triggers.
    //

    while(true) {

	our_master_s_current_time = our_master.SafeTime.load();

	//
	// If our master is the constraint-checking thread and
	// it is done, we should use the modeling thread's
	// safe time instead:
	//
	if(our_master.Done.load()) {
	    our_master_s_current_time = modeling_thread.SafeTime.load();
	}

	//
	// First: check whether a new vector has become available,
	// and then attempt to revive trailers on hold if possible.
	//
	// Our master will push a new vector our way when it's
	// ready. All we have to do is check the capture flag.
	//
	if(Thread.theSafeVectorHasBeenCaptured.load() == false) {

#	    ifdef DEBUG_THIS
	    if(feed_flag.load()) {
		tstr << "capture flag set to false; reading new vector\n";
		excl << tstr;
		tstr.undefine();
	    }
#	    endif /* DEBUG_THIS */

	    vector<trailer*>	to_revive;

	    //
	    // OK, we have a new vector. Before grabbing it, we
	    // should keep track of the capture time of the
	    // previous safe vector - OK, the capture time
	    // was saved as Thread.time_of_previous_processed_vector
	    //

	    //
	    // This will delete model_vector and replace it with
	    // the new one:
	    //
	    adopt_a_new_model_iterator(Thread.SafeVector);

	    if(Thread.WaitingForData.load()) {
		Thread.WaitingForData.store(false);
	    }

	    //
	    // We now have enough data to update our nodes on hold,
	    // reflecting the fact that our master is further ahead
	    // than last time we checked.
	    //
	    trailer*			on_hold;
	    trailerslist::iterator	iter(trailers_on_hold);

	    //
	    // Try to revive as many threads as we can:
	    //
	    while((on_hold = iter())) {

		//
		// on_hold is the trailer that was on hold
		//

		//
		// E is the safe iterator for the trailer's resource
		//
		safeClass*	E = on_hold->payload;

		//
		// current_trailing_event is the event that was
		// returned by next() at the time the event was
		// put on hold
		//
		value_node*	current_trailing_event = E->iterNode;
		assert(current_trailing_event);

		value_node*	current_safe_event = E->safe_node;
		assert(current_safe_event);

		//
		// debug
		//
		Rsource* resource = current_trailing_event->list->Owner;

		if(current_trailing_event != current_safe_event) {

		    //
		    // new_trailing_event is the node immediately
		    // following the node that used to be safe.
		    // One of two things must be true:
		    //
		    //	1. either this node has a time tag that is
		    //	   beyond the time at which the previous
		    //	   safe vector was captured,
		    //
		    //	2. or this node was added retroactively
		    //	   at a time strictly earlier than the previous
		    //	   capture time.
		    //

		    //
		    // We must now increment the iterator; this is
		    // what we were not able to do from within next(),
		    // when we put the trailer on hold.
		    //
		    value_node* new_trailing_event = E->next();
		    assert(new_trailing_event);
		    bool new_event_was_added_retroactively
			= new_trailing_event->Key.getetime()
				< Thread.time_of_last_processed_vector;

		    //
		    // We skip over retroactive nodes; there is no
		    // way we can report them, since they are out
		    // of order
		    //
		    if(new_event_was_added_retroactively) {

			if(dbg && resource->name == resdbg) {
			    Cstring str;
			    str << "trailing iter: " << resdbg << " node was on hold at "
				<< current_trailing_event->Key.getetime().to_string()
				<< ", captured at "
		    		<< Thread.time_of_last_processed_vector.to_string()
				<< ", now advanced to "
				<< new_trailing_event->Key.getetime().to_string();
			    if(new_event_was_added_retroactively) {
				str << " - added retroactively\n";
			    } else {
				str << "\n"; 
			    }
			tol_debug_stream() << str;
			}
			while(new_event_was_added_retroactively) {
			    new_trailing_event = E->next();
			    assert(new_trailing_event);
			    new_event_was_added_retroactively
				= new_trailing_event->Key.getetime()
					< Thread.time_of_last_processed_vector;
			}
		    }

		    assert(E->iterNode->Key.getetime() >= the_previous_node->Key.getetime());

		    if(dbg && resource->name == resdbg) {
			    Cstring str;
			    str << "trailing iter: " << resdbg << " node at "
				<< current_trailing_event->Key.getetime().to_string()
				<< " will be revived\n"; 
			    tol_debug_stream() << str;
		    }
		    to_revive.push_back(on_hold);
		} else {

		    //
		    // We can't use this node yet; we have to wait
		    // until our master has moved forward.
		    //
		    if(dbg && resource->name == resdbg) {
			Cstring str;
			str << "trailing iter: " << resdbg << " node at "
			    << current_trailing_event->Key.getetime().to_string()
			    << " is at the start of history - cannot use it.\n"; 
			tol_debug_stream() << str;
		    }
		}
	    }

	    //
	    // We have tried to get new data. Let's revive
	    // any nodes we found.
	    //
	    for(int i = 0; i < to_revive.size(); i++) {
		trailer*	on_hold = to_revive[i];
		safeClass*	E = on_hold->payload;
		assert(E->iterNode);
		if(the_previous_node && E->iterNode->Key.getetime()
				< the_previous_node->Key.getetime()) {
		    assert(E->iterNode->Key.getetime() >=
				    Thread.time_of_previous_processed_vector);
		}
		if(dbg && E->iterNode->list->Owner->name == resdbg) {
		    Cstring str;
		    str << "trailing iter: " << resdbg << " node at "
			<< E->iterNode->Key.getetime().to_string()
			<< " actually revived\n"; 
		    tol_debug_stream() << str;
		}

	        //
	        // Will remove the trailer from trailers_on_hold:
	        //
	        active_trailers << on_hold;
	    }
	    Thread.time_of_previous_processed_vector = Thread.time_of_last_processed_vector;
	    Thread.time_of_last_processed_vector = model_vector->Time;
	}

	trailer*	Trailing = active_trailers.first_node();
	if(Trailing) {

	    //
	    // We must test the time tag of the candidate returned
	    // node against our master's current time. We may not
	    // return the candidate if that is not the case; we
	    // will have to wait until our master has moved on.
	    //
	    if(Trailing->payload->iterNode->Key.getetime()
			<= our_master_s_current_time
	       && Trailing->payload->iterNode->Key.getetime()
			<= Thread.time_of_last_processed_vector) {
		if(dbg && Trailing->payload->iterNode->list->Owner->name == resdbg) {
		    Cstring str;
		    str << Trailing->payload->iterNode->Key.getetime().to_string()
			<< " - " << resdbg << " = "
			<< Trailing->payload->iterNode->get_value().to_string()
			<< " (get_data)\n";
		    tol_debug_stream() << str;
		}
		return Trailing;
	    } else {

		//
		// else, we revive the nodes on hold and/or wait for
		// our master to move ahead far enough.
		//
	    }
	}

	if(!active_trailers.get_length() && our_master.Done.load()) {

	    //
	    // OK, our master is done; since we got here, we have already processed
	    // all the data there was to get. No more data will be forthcoming
	    // from the modeling thread. From this point on, peek() will always
	    // return NULL; we are done too.
	    //
	    Thread.Done.store(true);
	    Thread.WaitingForData.store(false);
	    return NULL;
	}
	if(our_master.ErrorsFound.load()) {

	    //
	    // Error situation, let's bail out
	    //
	    Thread.Done.store(true);

	    //
	    // Signal any thread trailing us that things are bad
	    //
	    Thread.ErrorsFound.store(true);

	    Thread.WaitingForData.store(false);
	    return NULL;
	}

	//
	// OK, the modeling thread is busy... must be patient.
	//
	if(!Thread.WaitingForData.load()) {
	    Thread.WaitingForData.store(true);
	}
	thread_intfc::sleep_for_ms(10);
    }
}

//
// The behavior of this method is similar to that of a regular res_miterator,
// except that next() only changes the state of the trailing_miter if the
// new iterNode is "safe." In order to probe for this condition in a safe
// manner, it is sufficient to check whether
//
// 	[trailing thread]->iterNode == [modeling thread]->safeNode.
//
// This is because we are scanning the "safe" part of the list, which always
// ends at the modeling thread's safeNode.
//
// If the above condition is satisfied, then
//
// 	[trailing thread]->iterNode
//
// is set to NULL while
//
// 	[trailing thread]->prevNode
//
// is set to
//
// 	[modeling thread]->safeNode.
//
// Thus, prevNode can still be safely accessed as "current()". But we need
// to do something to indicate that this node cannot be moved forward, at
// least not until its next node appears and/or becomes safe. The
// mechanism used to do this is to put the safe_iter in a "holding" list.
//
// We don't want the trailer to block the miterator, because it is possible
// that a new history node will _never_ be created. What needs to be done
// instead is that whenever next() is invoked, the threads in the holding list
// need to check whether a new event may have become "safe" in the history
// list that they are watching. If that is the case, the iterator in the
// holding list needs to update its pointer to the new safe event, and it
// needs to be re-inserted in the list of active_trailers. Then, the next()
// algorithm can be run.
//
// The above algorithm implies that next() performance will be linear in
// the number of nodes in the holding list. Since the amount of processing
// required for each such node is modest, that should not be an issue.
//
value_node*
trailing_miter::next() {

	//
	// get_new_data() will _not_ return until it finds a node
	// that satisfies a number of conditions, stated below.
	//
	trailer*	Trailing = get_new_data();
	if(!Trailing) {

		//
		// we are done and should exit. The calling program,
		// which is the one passed to std::thread, will take
		// care of that.
		//
		return NULL;
	}

	safeClass*	D = Trailing->payload;
	value_node*	to_return = D->iterNode;
	CTime_base	return_time = to_return->Key.getetime();
	bool		put_trailer_on_hold = false;
	assert(	

		//
		// In all cases, the time tag of the returned node
		// must be less than or equal to the time of the
		// last processed vector. This is common sense. If
		// the time tag were greater, there could be nodes
		// we haven't checked yet, with a time tag greater
		// than the last processed time, that should come
		// before the one we found.
		//
		return_time <= Thread.time_of_last_processed_vector);
	assert(

		//
		// Second, whatever we return should be in time order.
		//
		!the_previous_node
		    || return_time >= the_previous_node->Key.getetime());
	assert(

		//
		// Finally, either the returned node is earlier than
		// the safe node, or we must put it on hold.
		//
			(to_return != D->safe_node
			 && to_return->next_node()
			)
			|| (put_trailer_on_hold = true));

	bool dbg = APcloptions::theCmdLineOptions().TOLdebug;
	Cstring& resdbg = APcloptions::theCmdLineOptions().theResourceToDebug;
	if(put_trailer_on_hold) {

	    if(dbg && to_return->list->Owner->name == resdbg) {
		Cstring str;
		str << return_time.to_string()
			<< " - " << resdbg << " = "
			<< to_return->get_value().to_string()
			<< ", captured at " << model_vector->Time.to_string()
			<< ", next() setting this as the_previous_node (on hold)\n";
		tol_debug_stream() << str;
	    }
	    active_trailers.remove_node(Trailing);
	    trailers_on_hold << Trailing;
	} else {

	    //
	    // The model iterator has advanced; we can safely
	    // call next(), because there is a safe_node ahead
	    //
	    active_trailers.remove_node(Trailing);
	    D->next();
	    active_trailers << Trailing;
	    if(dbg && to_return->list->Owner->name == resdbg) {
		Cstring str;
		str << return_time.to_string()
			<< " - " << resdbg << " = "
			<< to_return->get_value().to_string()
			<< ", captured at " << model_vector->Time.to_string()
			<< ", next() setting this as the_previous_node (active)\n";
		tol_debug_stream() << str;
	    }
	}

	if(to_return && the_previous_node) {
		assert(return_time >= the_previous_node->Key.getetime());
	}

	the_previous_node = to_return;

	//
	// Check that the returned node has a tag less than
	// the time at which it was captured, provided this is not
	// node at the beginning of the history
	//
	if(return_time > to_return->list->first_node()->Key.getetime()
	   && return_time > model_vector->Time) {
	    cerr << "ANOMALY - trailing iterator for "
		<< to_return->list->Owner->name
		<< " returning node with tag "
		<< to_return->Key.getetime().to_string() << " >= capture time "
		<< model_vector->Time.to_string() << "\n";
	    assert(false);
	}
	return to_return;
}

//
// The behavior of this method is the same as in a regular res_miterator.
// It is a "theorem" that the node returned by peek() has a tag that is
// equal to or earlier than the prevNode element of the modeling thread's
// iterator for the same list. This guarantees that the peeked-at node is
// "safe."
//
value_node* trailing_miter::peek() {

	//
	// get_new_data() will _not_ return until it finds a node
	// that satisfies a number of conditions, stated below.
	//
	trailer*	Trailing = get_new_data();
	if(!Trailing) {

		//
		// we are done and should exit. The calling program,
		// which is the one passed to std::thread, will take
		// care of that.
		//
		return NULL;
	}

	safeClass*	D = Trailing->payload;
	value_node*	to_return = D->iterNode;
	assert(	

		//
		// In all cases, the time tag of the returned node
		// must be less than or equal to the time of the
		// last processed vector. This is common sense. If
		// the time tag were greater, there could be nodes
		// we haven't checked yet, with a time tag greater
		// than the last processed time, that should come
		// before the one we found.
		//
		to_return->Key.getetime() <= Thread.time_of_last_processed_vector);
	assert(

		//
		// Second, whatever we return should be in time order.
		//
		!the_previous_node || to_return->Key.getetime()
			>= the_previous_node->Key.getetime());

#ifdef ONLY_DO_THIS_IN_NEXT
	bool		put_trailer_on_hold = false;
	assert(
		//
		// Finally, either we are ahead of a safe node, or
		// we have been put in the list of trailers on hold.
		//
		to_return != D->safe_node
		|| (put_trailer_on_hold = true)
	      );

	prev_node = to_return;
#endif /* ONLY_DO_THIS_IN_NEXT */

	return to_return;
}

slist<prio_time, value_node, Rsource*>::curval_iter* trailing_miter::peek_at_iter() {
	return NULL;
}


	//
	// We do nothing in the next two methods because we
	// manage our iterators ourselves.
	//
void	trailing_miter::handle_addition(
		slist<prio_time, value_node, Rsource*>*,
		value_node*) {
}
void	trailing_miter::handle_removal(
		slist<prio_time, value_node, Rsource*>*,
		value_node*){
}

void	trailing_miter::clear() {
	typename trailerptrslist::iterator	iter(pointers_to_trailers);
	trailerptr*				ptr;

	while((ptr = iter())) {
		if(ptr->payload->list) {
			ptr->payload->list->remove_node(ptr->payload);
		}

		((baseListClass*)ptr->payload->payload->L)->unregister_miterator(*this);
		delete ptr->payload->payload;
		delete ptr->payload;
	}

	//
	// No need to clear the active_trailers and trailers_on_hold lists;
	// we've just removed all their nodes.
	//
	pointers_to_trailers.clear();
	trailers_by_res.clear();
	thread_names.clear();
}
