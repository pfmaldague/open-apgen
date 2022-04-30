#ifndef _THREAD_INTFC_H_
#define _THREAD_INTFC_H_


#include <atomic>
#include <thread>
#include <mutex>

#include "APdata.H"


//
// thread_index should be set by all new threads
// as soon as they are started.
//
extern thread_local int thread_index;

class value_node;
class dual_purpose_iterator;

//
// An atomic pointer to an instance of this class will be
// shared between the modeling thread and slave threads
// that need the information. The information stored consists
// of two items:
// 	- a vector of pointers to resource histories (value_nodes)
// 	- an iterator used to store a safe pointer to the list of
// 	  activity ends. This iterator is needed by the (XML)TOL
// 	  threads to keep track of activity ends, which can be
// 	  modified by the modeling adaptation code.
//
class safe_vector_info {
public:
    safe_vector_info() = delete;
    safe_vector_info(const safe_vector_info& VI)
	: vec(new vector<value_node*>(*VI.vec)),
	    enditer(new slist<
			  alpha_time,
			  time_actptr,
			  ActivityInstance*>::iterator(*VI.enditer)),
	    Time(VI.Time) {}
    safe_vector_info(
	    vector<value_node*>* the_safe_vector,
	    slist<
	      alpha_time,
	        time_actptr,
	        ActivityInstance*>::iterator*
				safe_end_iterator,
	    CTime_base		the_time_at_which_it_was_captured)
	: vec(the_safe_vector),
	    enditer(safe_end_iterator),
	    Time(the_time_at_which_it_was_captured) {
    }
    ~safe_vector_info() {
	delete vec;
	delete enditer;
	vec = NULL;
	enditer = NULL;
    }

    CTime_base			Time;
    bool			is_null() const {
	return vec == NULL;
    }
    vector<value_node*>&	Vector() {
	return *vec;
    }
    const vector<value_node*>&	Vector() const {
	return *vec;
    }
    slist<  alpha_time,
	    time_actptr,
	    ActivityInstance*>::iterator&
		    		Iter() {
	return *enditer;
    }
    const slist<  alpha_time,
	    time_actptr,
	    ActivityInstance*>::iterator&
		    		Iter() const {
	return *enditer;
    }

private:
    vector<value_node*>*	vec;
    slist<  alpha_time,
	    time_actptr,
	    ActivityInstance*>::iterator*
	    			enditer;
};

//
// There is one instance of the following class for each
// value of thread_index.
//
class thread_intfc {
public:
    thread_intfc(int thread_to_follow);
    thread_intfc(const thread_intfc&) = delete;
    ~thread_intfc() {

#ifdef DO_THIS_IN_EVENT_LOOP
	//
	// All thread_intfc objects are owned by the
	// modeling thread:
	//
	if(Thread.joinable()) {
	    Thread.join();
	}
#endif /* DO_THIS_IN_EVENT_LOOP */
    }

    //
    // Not perfect, but MITER_THREADS is the maximum number
    // of threads running concurrently; it is also the number
    // of static integers defined here.
    //
    // NULL_THREAD is used as an index for a non-existent
    // thread; it is used by the constructor for the model
    // thread to mean "this thread is not trailing any other thread".
    //
    static const int MODEL_THREAD;
    static const int CONSTRAINT_THREAD;
    static const int XMLTOL_THREAD;
    static const int TOL_THREAD;
    static const int MITER_THREADS;
    static const int NULL_THREAD;

    std::thread			Thread;
    int				theThreadWeTrail;

    //
    // There is one of those for each THREAD defined
    // as a static const int aboce:
    //
    static vector<unique_ptr<thread_intfc> >&	threads();

    static void			sleep_for_ms(int i) {
	this_thread::sleep_for(chrono::milliseconds(i));
    }

    //
    // This static methods fills the threads() vector.
    // Note that the vector elements are unique_ptr wrapped
    // around a NULL pointer, so no real work is involved.
    //
    // The unique_ptr's will be fed substantial pointers
    // at modeling/scheduling time.
    //
    static void			create_threads();

    //
    // Updates SafeTime for this thread (used to be in Miterator_base).
    // The 'check' argument is a debugging aid which makes sure
    // that the modeling thread's SafeTime stays equal to
    // CurrentEvent's time tag at all times, except while
    // scheduling.
    //
    static void			update_current_time(
					const CTime_base&);

    //
    // To help the modeling thread get a suitable vector to
    // pass to threads that trail the constraint-checking
    // thread
    //
    static safe_vector_info*	get_copy_of_safe_vector_from(
					thread_intfc*);

    CTime_base			time_of_last_processed_vector;
    CTime_base			time_of_previous_processed_vector;


    //
    // Reports the SafeTime of thread with thread_index = i;
    // uses an atomic load():
    //
    static CTime_base		current_time(int i);

    //
    // By creating an instance of a Completion_signal_sender,
    // a thread function can guarantee that the Done flag of
    // the thread_intfc attached to it is set as the function
    // terminates execution.
    //
    class Completion_signal_sender {
    public:
	Completion_signal_sender() = delete;
	Completion_signal_sender(const Completion_signal_sender&) = delete;

	//
	// T may be NULL, in which case this class is
	// just a no-op
	//
	Completion_signal_sender(thread_intfc* T)
		: Thread(T) {
		if(Thread) {
			Thread->Done.store(false);
		}
	}
	~Completion_signal_sender() {
		if(Thread) {
			Thread->Done.store(true);
		}
	}
	void declare_task_completed() {
		if(Thread) {
			Thread->Done.store(true);
		}
	}

	//
	// May be NULL
	//
	thread_intfc* Thread;
    };

    //
    // The atomic integer/bool/pointer objects
    // below support the communication protocol
    // between the modeling thread(s) and any
    // trailing threads that do concurrent work.
    //
    // SafeMiterator and SafeTime are safe-to-use
    // snapshots of the actual potential_trigger
    // miterator and of the time tag of
    // CurrentEvent. They are safe to use because
    // they were copied from the originals, and
    // are not touched by the main thread after
    // creation. This implies that the trailing
    // thread(s) is/are responsible for deleting
    // the copies when they are no longer needed.
    //
    std::atomic<safe_vector_info*>	SafeVector;

    std::mutex				safe_vector_protector;
    unique_ptr<safe_vector_info>	copy_of_safe_vector;

    //
    // Set by the trailing thread(s), monitored
    // by the modeling thread:
    //
    std::atomic<bool>		theSafeVectorHasBeenCaptured;

    //
    // Set by the modeling thread and the constraint-checking
    // thread.
    //
    // The modeling thread SafeTime is monitored by its optional
    // trailing thread(s):
    //
    // 	- if there are no constraints, by the (XML)TOL output
    // 	  thread(s), if any
    //
    // 	- if there are constraints, by the constraint thread,
    // 	  if any
    //
    // The constraint_checking SafeTime is monitored by its
    // optional trailing threads:
    //
    // 	- the (XML)TOL output thread(s), if any
    //
    std::atomic<long int>	SafeTime;

    //
    // Flags that allow communications with the modeling thread
    // 
    std::atomic<bool>		Done;
    std::atomic<bool>		ErrorsFound;
    std::atomic<bool>		WaitingForData;

    //
    // For the leading thread to invoke just prior to launching
    // a trailing thread. prelude() is implemented in EventLoop.C
    // for the time being.
    //
    // Now (after removal of the vector argument) all it does
    // is initialize the flags used for communication with the
    // thread being followed.
    //
    void			prelude(CTime_base initial_time);

    //
    // Errors generated while the thread was running
    //
    Cstring			errors;
};

#endif /* _THREAD_INTFC_H_ */
