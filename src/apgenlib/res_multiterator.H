#ifndef _NUM_RES_MITER_H_
#define _NUM_RES_MITER_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <sstream>

#include "timeptr.H"
#include "ValueNode.H"


class safe_vector_info;

//
// Old apgen had a class based on List and blist for iterating over a
// number of time-ordered lists without merging them. This class was
// useful for
//
// 	- iterating over events belonging to several queues
//
// 	- iterating over activity starts and ends and over resource and
// 	  violation events when creating time-ordered listings
//
// 	- iterating over events while looking for scheduling windows
//
// As APGen was refactored to introduce templates for most applications
// of the old List and blist classes, a general Miterator template was
// introduced as a replacement of the old Miterator. Note that in one place
// in the code, which is the template used for TOL and XMLTOL output, the
// old-style Miterator (modified to handle status in a logically consistent
// way) is still used.
//
// The new Miterator template was used by a number of clients:
//
// 	- eval_intfc::Events()
//
// 		This is a Miterator for traversing the various event queues
// 		in time order.
//
// 	- EventLoop::potential_triggers
//
// 		This Miterator is used just before advancing time in the
// 		event loop, to find out whether one or more activities or
// 		abstract resources currently on hold could be unblocked.
//
// 	- EventLoop::old_events
//
// 		This Miterator is used while advancing time; its purpose
// 		is to delete old events in the resource histories during
// 		the 2nd scheduling loop. Old events with time tags greater
// 		than now are used to estimate the behavior of resources in
// 		the future; new events replace them as the modeling thread
// 		proceeds through the time-ordered list of events.
//
// 	- window_finder::theTriggers
//
// 		This Miterator is used while searching for windows via
// 		the get_windows() method in a decomposition or scheduling
// 		activity type section.
//
//	- writeTOLtemplate in output/IO_write.C
//
//		The Miterator was used for tracking resource histories,
//		including associative resources.
//
//	- Rsource::get_the_multiterator()
//
//		This is used when the GUI requests the history of a specific
//		resource. The Miterator keeps track of any other histories
//		on which the queried resource depends via its profile
//		definition.
//
// Then, more recently (May 2019), a new requirement made its appearance -
// enable multithreading in APGenX. This is not an absolute requirement,
// but it was introduced as part of a general undercurrent of "design for
// efficiency" while constraint processing was re-introduced into APGenX
// (it had been dropped during the main refactoring effort that took old
// apgen into APGenX.)
//
// The initial applications of multithreading are going to be
//
// 	- constraint evaluation
//
// 	- TOL and XMLTOL output
//
// In the future, multiple modeling threads will be introduced to implement
// a "multiple wave" approach to modeling; this will have to wait until we
// have a consistent method for ascertaining dependencies between activity
// and resource subsystems.
//
// Independently of multithreading, another requirement arose from a detailed
// study of performance profiles for Europa simulations. It was discovered
// that a significant time hog was the evaluation of a resource's currentval()
// method, which uses the general find() method of a tlist. While find()
// returns a value in logarithmic time, the logarithm is nontrivial for
// resources with histories containing thousands of nodes. In many cases,
// it would be good enough to start looking from the end of the history
// list, but that does not quite work because APGenX lets adapters insert
// events with future time tags into the events list.
//
// In order to enable efficient evaluation of currentval() and simultaneously
// enable multithreading, the Miterator template was partially replaced
// by the res_miterator class, which was specifically redesigned to
// handle resource histories in a way that would allow a "trailing miterator"
// to handle other tasks with a time tag strictly less than the modeling
// thread's "now".
//
// The first element of the res_miterator design was to add a new data
// member to slist::iterator, which already included a value_node pointer
// called iterNode and now also features a prevNode object of type
// std::atomic<value_node*>. Making this pointer atomic allows the
// modeling thread to set it while the trailing thread reads it and
// waits for it to change if necessary.
//
// The introduction of prevNode in iterators also makes it possible to
// evaluate currentval() efficiently, since the search for the best
// node can start right at the node pointed to by prevNode.
//
// The "invariant" underlying the design is the safety of the prevNode
// member of the iterators in potential_triggers. In a nutshell, the
// time tag of prevNode is less than or equal to now; since APGenX
// never creates history nodes in the past, it is guaranteed that no
// node will ever be inserted before prevNode. As a result, the iterators
// used by trailing threads can safely rely on prevNode to evaluate
// resource values at times less than now.
//
// This design works well in all cases, except in the 2nd scheduling pass.
// The problem in this case is that prevNode is safe against the introduction
// of nodes before it by the modeling thread, but it is not safe because it
// might point to a node that was created in the 1st modeling pass and is
// about to be destroyed by the processing of old events that takes place in
// the EventLoop::advance_now_to() method.
//
// As a result, it is necessary to introduce a third pointer in
// slist::iterator, namely safeNode. The three pointers have well defined
// roles:
//
// 	- iterNode points to the next node in the resource history,
// 	  i. e., the one that will be returned by the peek() and
// 	  next() methods of res_miterator.
//
// 	- prevNode points to a node that is safe for evaluating
//	  currentval() within the modeling thread. It is not safe
//	  for use by any other, trailing threads because it might
//	  be destroyed when the modeling thread advances now.
//
//	  It is also safe to use by get_windows(), since get_windows()
//	  is only used within the modeling thread, at least for now.
//
//	- safeNode points to a node that is safe for use by trailing
//	  threads. It is safe against insertion of earlier nodes and
//	  also against deletion by old event processing inside
//	  EventLoop::advance_now_to().
//
// As the complexity of slist::iterator increases, so does that of the
// algorithms for updating iterators within the res_miterator and
// trailing_miter code. As a result, it may be a good idea to keep a
// simple version of res_miterator around, for customers who do not
// need the fancy capabilities. Let's have a little matrix.
//
//  Miterator			    uses currentval	support trailers
//  =========			    ===============	================
//
//  eval_intfc::Events()		no(*)			no(*)
//
//  EventLoop::potential_triggers	yes			yes
//
//  EventLoop::old_events		no			no
//
//  EventLoop::check_constraints	yes			no
//
//  window_finder::theTriggers		yes			no
//
//  writeTOLtemplate			yes			no
//
//  Rsource::get_the_multiterator()	yes			no
//
//
//  (*): not relevant because it's based on the Miterator template,
//       not res_miterator
//
// Onward Strategy
// ---------------
//
// The current Miterator template will be left the way it is, since it works
// fine for lists like Events() which are not shared between threads.
//
// The current res_iterator will be split into a hierarchy as follows:
//
//			Class					  usage
//			-----					  -----
//
// Miterator_base<slist<prio_time, value_node, Rsource*>,	(abstract)
// 		    value_node>
//
//			|
// 			|
//
//		simple_res_miter				 old_events
//
// 			|
// 			|
//
//		res_curval_miter				theTriggers
//							    get_the_multiterator()
// 		/	      \
// 	       /	       \
//
//	model_res_miter	/  trailing_miter	potential_triggers / checking_constraints()
//								     writeTOLtemplate()
//
// Note that in order to support this design, iterators will have to have
// their own hierarchy:
//
// 		   Miterator			slist::iterator
// 		   ---------			---------------
//
// 		simple_res_miter		    iterator
//
// 		res_curval_miter		   curval_iter
//
//		/	       \		   /	    \
//	       /		\		  /	     \
//
// 	model_res_miter	  trailing_miter   leading_iter	  safe_iter
//
// NOTE: we may not want to derive leading_iter and safe_iter from
// curval_iter, because the former need to declare prevNode std::atomic<>,
// while curval_iter is used within the modeling thread exclusively and
// can declare prevNode as a regular pointer.
//

class model_res_miter;

class thread_intfc;

//
// Simplest case - iterators contain iterNode, nothing else
//
class simple_res_miter: public Miterator_base<
				slist<	prio_time,
					value_node,
					Rsource*>,
				value_node> {
public:

  typedef multilist<prio_time, value_node, Rsource*> listClass;
  typedef slist<prio_time, value_node, Rsource*> baseListClass;
  typedef baseListClass::iterator		iterClass;
  typedef Cnode0<relptr<iterClass>, iterClass*>	Tthread;
  typedef tlist<relptr<iterClass>, Tthread>	Tthreadtlist;
  typedef slist<relptr<iterClass>, Tthread>	Tthreadslist;
  typedef Cnode0<alpha_void, Tthread*>		threadptr;
  typedef slist<alpha_void, threadptr>		threadptrslist;
  typedef tlist<alpha_void, threadptr>		threadptrtlist;

//
// for general-purpose use, in particular the modeling loop:
//
  simple_res_miter(const char* p)
	: name(p),
		active_threads(true),
		initializationFunction(NULL) {}
  simple_res_miter(
		const char* p,
		const simple_res_miter& mit);
  simple_res_miter(
		const char* p,
		const simple_res_miter& mit,
		value_node* (*initFunc)(
				const baseListClass&,
				const CTime_base&),
		const CTime_base& initTime);
  simple_res_miter(const simple_res_miter& mit);

  virtual ~simple_res_miter() { clear(); }

// METHODS

  // debug methods
  void		dump(std::stringstream&, int);
  void		dump_one(Tthread*, std::stringstream&, int);
  void		test_for_refs_to(
			value_node*) override;

  // other methods
 
  value_node*	(*initializationFunction)(const baseListClass&, const CTime_base&);

  const char*	get_name() const override {
	return name;
  }
 
	//
	// adds one source of events.
	//
	// Final state: a pointer to the new list has been added to
	// 		dead_lists (if the new list is empty) or to
	// 		active_lists (if it is not)
	//
	// Note: default values are not supposed to be used for this
	// base class, but they are OK for derived class trailing_miter,
	// which does not care about previously specified parameters:
	//
  virtual void		add_thread(
		baseListClass& aList,
		const Cstring& thread_name = Cstring::null(),
		unsigned long itsPriority = 0,
		bool store = false) override;

  virtual int		thread_count() override {
	return pointers_to_threads.get_length();
  }

	//
	// 'forgets' all the threads that were added.
	//
	// Final state: the state is the same as just after construction.
	//
  virtual void		clear() override;

	//
	// resets threads to their list's beginning.
	//
	// Final state: active_threads is populated with pointers to
	// the earliest elements of each added list.
	//
  virtual void		first() override;

  CTime_base	getetime() {
	CTime_base	T0;
	if(is_empty()) {
		return T0;
	}
	//
	// we know the Key of a Tthread is a relptr<iterator>,
	// so it must have an mC member that is an iterator
	//
	return active_threads.first_node()->getKey().mC->getNode()->getKey().getetime();
  }

  virtual void	handle_addition(baseListClass* theList, value_node* theNode) override;
  virtual void	handle_removal(baseListClass* theList, value_node* theNode) override;

	//
	// indicates that we ran out of events (no state change)
	//
  bool		is_empty() {
	return active_threads.get_length() == 0;
  }


	//
	// identifies the current event and returns it after
	// moving on (changing state) to the next event
	//
  virtual value_node*	next() override;

	//
	// same as next() but does not move on
	//
  virtual value_node*	peek() override;

	//
	// same as peek() but returns the iterator
	//
  virtual iterClass*	peek_at_iter();

  void			remove_thread(
		  		const baseListClass& listToRemove) override;

	//
	// returns the current event (no state change)
	//
  virtual iterClass*	there_is_a_current_event(
		  		Cstring& name,
				unsigned long& priority);

// DATA

  const char*		name;
  CTime_base		initializationTime;

	//
	// need to delete pointee when clearing
	//
  threadptrtlist	 pointers_to_threads;

	//
	// used to store the strings associated with each thread
	//
  tlist<alpha_void, Cnode0<alpha_void, Cstring> > thread_names;

  // made public for debugging:
  // private:

	//
	// The following 2 lists provide a partition of all
	// threads into 2 groups:
	//
  Tthreadtlist		active_threads;	// Don't delete any nodes!
  Tthreadslist		dead_threads;	// Ditto!
};

//
// Important note - as of June 6th, this kind of Miterator
// keeps track of the last returned node, in addition to the next node to
// return.
//
class res_curval_miter: public Miterator_base<
				slist<	prio_time,
					value_node,
					Rsource*>,
				value_node> {
public:

  typedef multilist<prio_time, value_node, Rsource*> listClass;
  typedef slist<prio_time, value_node, Rsource*> baseListClass;

  //
  // In this class, we use the curval_iter:
  //
  typedef baseListClass::curval_iter		iterClass;

  typedef Cnode0<relptr<iterClass>, iterClass*>	Tthread;
  typedef tlist<relptr<iterClass>, Tthread>	Tthreadtlist;
  typedef slist<relptr<iterClass>, Tthread>	Tthreadslist;
  typedef Cnode0<alpha_void, Tthread*>		threadptr;
  typedef slist<alpha_void, threadptr>		threadptrslist;
  typedef tlist<alpha_void, threadptr>		threadptrtlist;

  //
  // Suitable for derived classes (e. g. trailing_miter). Also
  // suitable for decomposition code, i. e., when a modeling
  // thread is not available; the window_finder object then has
  // to initialize each internal iterator to the start of its
  // resource history.
  //
  res_curval_miter(const char* p)
	: name(p),
		active_threads(true),
		initializationFunction(NULL) {}

  //
  // Suitable for use by scheduling code; a set of 'live' resource
  // history iterators is available from the modeling loop, and we
  // can use that set to initialize the res_curval_miter.
  //
  res_curval_miter(
		const char* p,
		const model_res_miter& mit,
		value_node* (*initFunc)(
				const baseListClass&,
				const CTime_base&),
		const CTime_base& initTime);

  //
  // Suitable for use by a single-threaded TOL output process. We
  // want to initialize the miterator so it is "ready for use",
  // i. e., the internal iterators are all set to a time just
  // before the start time of the TOL output request.
  //
  // We can use the same initialization function as the
  // window_finder class; the use case is similar (basic
  // single-threaded case).
  //
  res_curval_miter(
		const char* p,
		value_node* (*initFunc)(
				const baseListClass&,
				const CTime_base&),
		const CTime_base& initTime);
		
  res_curval_miter(const res_curval_miter& mit) = delete;

  virtual ~res_curval_miter() {
	clear();
  }

// METHODS

  // debug methods
  void		dump(std::stringstream&, int);
  void		dump_one(Tthread*, std::stringstream&, int);
  void		test_for_refs_to(value_node*) override;

  // other methods
 
  value_node*	(*initializationFunction)(const baseListClass&, const CTime_base&);

  const char*	get_name() const override {
	return name;
  }
 
	//
	// adds one source of events.
	//
	// Final state: a pointer to the new list has been added to
	// 		dead_lists (if the new list is empty) or to
	// 		active_lists (if it is not)
	//
	// Note: default values are not supposed to be used for this
	// base class, but they are OK for derived class trailing_miter,
	// which does not care about previously specified parameters:
	//
  virtual void		add_thread(
		baseListClass& aList,
		const Cstring& thread_name = Cstring::null(),
		unsigned long itsPriority = 0,
		bool store = false) override;

  virtual int		thread_count() override {
	return pointers_to_threads.get_length();
  }

	//
	// 'forgets' all the threads that were added.
	//
	// Final state: the state is the same as just after construction.
	//
  virtual void		clear() override;

	//
	// resets threads to their list's beginning.
	//
	// Final state: active_threads is populated with pointers to
	// the earliest elements of each added list.
	//
  virtual void		first() override;

  void			prepare_for_use_at(
  				value_node*	(*func)(
					const baseListClass&,
					const CTime_base&),
				CTime_base	T);

  CTime_base		getetime() {
	CTime_base	T0;
	if(is_empty()) {
		return T0;
	}
	//
	// we know the Key of a Tthread is a relptr<iterator>,
	// so it must have an mC member that is an iterator
	//
	return active_threads.first_node()->getKey().mC
				->getNode()->getKey().getetime();
  }

  virtual void		handle_addition(
		  		baseListClass* theList,
				value_node* theNode) override;
  virtual void		handle_removal(
		  		baseListClass* theList,
				value_node* theNode) override;

	//
	// indicates that we ran out of events (no state change)
	//
  bool			is_empty() {
	return active_threads.get_length() == 0;
  }


	//
	// identifies the current event and returns it after
	// moving on (changing state) to the next event
	//
  virtual value_node*	next() override;

	//
	// same as next() but does not move on
	//
  virtual value_node*	peek() override;

	//
	// Returns the node currently in a resource's
	// representative in the internal list of iterators.
	// This method can be implemented reliably because
	// resource histories are guaranteed to have an
	// Owner that points to a valid Rsource object.
	//
  virtual value_node*	current(void*) override;

	//
	// same as peek() but returns the iterator
	//
  virtual iterClass*	peek_at_iter();

  void			remove_thread(
		  	  const baseListClass& listToRemove) override;

// DATA

  const char*		name;
  CTime_base		initializationTime;

	//
	// need to delete pointee when clearing
	//
  threadptrtlist	 pointers_to_threads;

	//
	// ditto
	//
  threadptrtlist	 threads_by_res;

	//
	// used to store the strings associated with each thread
	//
  tlist<alpha_void, Cnode0<alpha_void, Cstring> > thread_names;

  // made public for debugging:
  // private:

	//
	// The following 2 lists provide a partition of all
	// threads into 2 groups:
	//
  Tthreadtlist		active_threads;	// Don't delete any nodes!
  Tthreadslist		dead_threads;	// Ditto!
};

//
// Most complex case - iterators contain iterNode, prevNode, safeNode
//
class model_res_miter: public Miterator_base<
				slist<	prio_time,
					value_node,
					Rsource*>,
				value_node> {
public:

  typedef multilist<prio_time, value_node, Rsource*>	listClass;
  typedef slist<prio_time, value_node, Rsource*>	baseListClass;
  typedef baseListClass::leading_iter			leadingClass;
  typedef Cnode0<relptr<leadingClass>, leadingClass*>	Tthread;
  typedef tlist<relptr<leadingClass>, Tthread>		Tthreadtlist;
  typedef slist<relptr<leadingClass>, Tthread>		Tthreadslist;
  typedef Cnode0<alpha_void, Tthread*>			threadptr;
  typedef slist<alpha_void, threadptr>			threadptrslist;
  typedef tlist<alpha_void, threadptr>			threadptrtlist;

//
// for general-purpose use, in particular the modeling loop:
//
  model_res_miter(const char* p)
	: name(p),
		active_threads(true) {
  }

  model_res_miter(const model_res_miter& mit);

  virtual ~model_res_miter() { clear(); }

  const char*	name;

// METHODS
//
  virtual const char*	get_name() const override {
	return name;
  }
  void			test_for_refs_to(
				value_node*) override {}

  virtual void		clear() override;

	//
	// adds one source of events.
	//
	// Final state: a pointer to the new list has been added to
	// 		dead_lists (if the new list is empty) or to
	// 		active_lists (if it is not)
	//
	// Note: default values are not supposed to be used for this
	// base class, but they are OK for derived class trailing_miter,
	// which does not care about previously specified parameters:
	//
  virtual void		add_thread(
		baseListClass& aList,
		const Cstring& thread_name = Cstring::null(),
		unsigned long itsPriority = 0,
		bool store = false) override;

  virtual int		thread_count() override {
	return pointers_to_threads.get_length();
  }

	//
	// resets threads to their list's beginning.
	//
	// Final state: active_threads is populated with pointers to
	// the earliest elements of each added list.
	//
  virtual void		first() override;

  virtual void	handle_addition(baseListClass* theList, value_node* theNode) override;
  virtual void	handle_removal(baseListClass* theList, value_node* theNode) override;

	//
	// identifies the current event and returns it after
	// moving on (changing state) to the next event
	//
  virtual value_node*	next() override;

	//
	// same as next() but does not move on
	//
  virtual value_node*	peek() override;

	//
	// same as peek() but returns the iterator
	//
  virtual leadingClass*	peek_at_iter();

  void			remove_thread(const baseListClass& listToRemove) override;

  void			initialize_iterator(leadingClass& ic);

  //
  // Used to provide vectors to any trailing miterators:
  //
  safe_vector_info*	save_safe_nodes() override;

  //
  // DATA
  //
  tlist<alpha_void, Cnode0<alpha_void, Cstring> > thread_names;

  			//
			// Given (void*)baseListClass, provides Tthread*
			//
  threadptrtlist	pointers_to_threads;

  tlist<alpha_void, Cnode0<alpha_void, int> > thread_indices;

  			//
			// tlist of Tthreads
			//
  Tthreadtlist		active_threads;	// Don't delete any nodes!

  			//
			// slist of Tthreads
			//
  Tthreadslist		dead_threads;	// Ditto!
};

class trailing_miter: public res_curval_miter {
public:

  //
  // Remember: iterClass is curval_iter. However, we are trailing a
  // model_res_miter which uses a leading_iter, derived from
  // curval_iter. Since we need to access the safeNode member of
  // the leading_iter (it is the only chunk of data that is safe
  // to access in a multithreading situation), we will use a
  // safe_iter; a safe_iter contains a pointer to the safe
  // node taken from the leading_iter that it is trailing.
  //

  typedef baseListClass::leading_iter	leadingClass;
  typedef baseListClass::safe_iter	safeClass;

  typedef Cnode0<relptr<safeClass>, safeClass*>	trailer;
  typedef tlist<relptr<safeClass>, trailer>	trailertlist;
  typedef slist<relptr<safeClass>, trailer>	trailerslist;
  typedef Cnode0<alpha_void, trailer*>		trailerptr;
  typedef slist<alpha_void, trailerptr>		trailerptrslist;
  typedef tlist<alpha_void, trailerptr>		trailerptrtlist;

  trailing_miter(
					// name of this iterator
	    const char*			p,

					// interface to the thread
					// in which we are running
	    thread_intfc&		T,

					// "safe to use" snapshot of
					// the modeling thread behind
					// which this iterator trails
	    const safe_vector_info*	M,

					// ID of the thread we are trailing
	    int				thread_to_follow
		);

  trailing_miter(const trailing_miter& mit) = delete;
  trailing_miter() = delete;

  ~trailing_miter();

  //
  // adds one source of events.
  //
  // Final state: a pointer to the new list has been added to
  // 		dead_lists (if the new list is empty) or to
  // 		active_lists (if it is not)
  //
  virtual void			add_thread(
				  baseListClass& aList,
				  const Cstring& thread_name = Cstring::null(),
				  unsigned long itsPriority = 0,
				  bool store = false) override;

  virtual int			thread_count() override {
	return pointers_to_trailers.get_length();
  }

  value_node*	next() override;
  value_node*	peek() override;
  value_node*	current(void*) override;

  //
  // Unlike the base class, this one does nothing because
  // the only available constructor puts the Miterator in
  // its initial state, and the Miterator should not be used
  // more than once.
  //
  virtual void	first() override {}

  //
  // This function is called by both peek() and next()
  // before they return anything. The function makes sure
  // that there is a node in active_trailers that is
  // ready to be returned. That is to say, its time tag
  // is less than or equal to Thread.time_of_last_processed_vector.
  //
  // If there is no such node, then get_new_data() iterates
  // until there is one.
  //
  // If get_new_data() returns NULL, that means that all
  // sources of new nodes have been exhausted, i. e., all
  // nodes have been processed and it's OK to set Done
  // to true.
  // 
  trailer*	get_new_data();

  //
  // Makes sure the current safe vector is not NULL
  //
  void		check_safe_vector();

  //
  // To assist (XML)TOL trailing threads in keeping track
  // of activity end nodes
  //
  time_actptr*	get_safe_endnode();

  //
  // Makes sure the current trailing event is not NULL
  //
  void		check_current_trailing_event(value_node*);

  //
  // true return: OK to use this node
  // false return: not OK to use this node; its time tag
  // 		   is ahead of the current time of the
  // 		   constraint-checking thread
  //
  bool		check_event_timing(value_node*);
  iterClass*	peek_at_iter() override;
  virtual void	clear() override;
  void	handle_addition(baseListClass* theList, value_node* theNode) override;
  void	handle_removal(baseListClass* theList, value_node* theNode) override;

  void	adopt_a_new_model_iterator(safe_vector_info*);

  //
  // Data members
  //
  int				the_thread_we_trail;

  //
  // debugging
  //
  value_node*			the_previous_node;

  //
  // To ensure that nodes returned by peek() and next() have
  // time tags before the smallest possible time tag of nodes
  // to be retrieved in the future
  //
  CTime_base			holding_time;

  thread_intfc&			Thread;
  const safe_vector_info*	model_vector;
  trailerptrtlist	 	pointers_to_trailers;
  trailerptrtlist	 	trailers_by_res;
  trailertlist			active_trailers;	// Don't delete any nodes!
  trailerslist			trailers_on_hold;	// Ditto!
};

#endif /* _NUM_RES_MITER_H_ */
