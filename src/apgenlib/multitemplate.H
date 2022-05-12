#ifndef _MULTITEMPLATE_H_
#define _MULTITEMPLATE_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <sstream>

#include <timeptr.H>

//
// Miterator - a robust multiple iterator. It behaves like an iterator, except
// that it is able to scan multiple lists simultaneously. The lists to be
// scanned should be added through the add_thread() method.
//
// Miterator requirements:
//
// 	- listClass and nodeClass should be "as expected" (obvious)
//
// 	- iterClass must be suitable as an argument for the relptr template,
// 	  i. e.:
// 	    - have a getNode() method
// 	    - the object to which getNode() returns a pointer should have a
// 	      getKey() method that returns a Key suitable for slist and tlist.
//
//
// Some design notes: 
//
//    - A Miterator is always is a well-defined state; as long as at least one
//      thread is active, it has a well-defined notion of "now" which is the
//      time tag of the last returned node
//
//    - the state of a Miterator is characterized by
//    	  - its list of threads; each thread corresponds to one list
//    	  - its list of active threads, whose lists contain a "next node" in
//    	    the future
//    	  - its list of dead threads, whose lists are completely in the past
//
//    - if two or more lists have nodes with exactly the same time tag, the
//      priority of the list containing the node acts as a secondary index;
//      this priority is passed as an argument by the add_thread method, which
//      is the mechanism used to register a new list with the Miterator
//
//    - if the priorities are the same (which should not happen, but is not
//      ruled out by the add_thread method), the built-in secondary index from
//      the list of pointers to threads (see below) is used as the final
//      arbiter
//
// Some notes about multi-threading:
//
//    - Here we discuss what happens when EventLoop::ProcessEvents() is being
//      run in a std::thread called the "modeling thread", and other threads
//      called "trailing threads" follow the modeling thread to perform
//      various tasks that do not alter the state of the APGen engine.
//
//    - An instance of the Miterator class is used by EventLoop to keep track
//      of all resource history nodes; this is necessary in case an execution
//      thread needs to be woken up because the condition it was blocking on
//      just became true.
//
//    - A "trailing thread" can iterate over resource events at a "safe
//      distance" from 'now', i. e., the time tag of the event being processed
//      by EventLoop. "Safe distance" means that the history nodes queried by
//      the trailing thread should all have a time tag strictly less than now.
//      As long as that is the case, the trailing thread can consult the node
//      without fear of the node being deleted by the modeling thread.
//
//    - However, the forward pointer of a 'safe' node can still be affected by
//      the modeling thread; the trailing threads should not access it until
//      now has advanced some more.
//      
//    - To make it possible for trailing threads to stay in the "safe zone",
//      a special kind of iterator called "safe_iterator" will be used; in
//      addition to the data members of a regular iterator, it contains a
//      pointer to the "latest safe node" in the history list it is tracking.
//      By definition, the latest safe node in a history list has a time
//      tag strictly less than now, and its forward pointer will be either
//      NULL or point to a node whose time tag is equal to or greater
//      than now. Therefore, a trailing thread should not attempt to
//      dereference the forward pointer of the latest safe node; it should
//      wait until the latest safe node points to a pointer that occurs later
//      in the list. In any case, a trailing thread will always have the
//      information it needs to evaluate the current value of a resource,
//      since the next node in the list, if it exists, could never have a time
//      tag strictly less than now.
//

template <class listClass, class nodeClass, class iterClass = typename listClass::iterator>
class Miterator: public Miterator_base<listClass, nodeClass> {
public:

  typedef Cntnr<relptr<iterClass>, iterClass*>	Tthread;
  typedef tlist<relptr<iterClass>, Tthread>	Tthreadtlist;
  typedef slist<relptr<iterClass>, Tthread>	Tthreadslist;
  typedef Cntnr<alpha_void, Tthread*>		threadptr;
  typedef slist<alpha_void, threadptr>		threadptrslist;
  typedef tlist<alpha_void, threadptr>		threadptrtlist;

  //
  // for general-purpose use, in particular the modeling loop:
  //
  Miterator(const char* p)
	: name(p),
		active_threads(true),
		initializationFunction(NULL) {}
  Miterator(const char* p, const Miterator& mit);
  Miterator(	const char* p,
		const Miterator& mit,
		nodeClass* (*initFunc)(const listClass&, const CTime_base&),
		const CTime_base& initTime);
private:
  Miterator(const Miterator& mit);
public:
  ~Miterator() { clear(); }

// METHODS

  // debug methods
  void		dump(std::stringstream&, int);
  void		dump_one(Tthread*, std::stringstream&, int);
  void		test_for_refs_to(nodeClass*) override;


  // other methods
 
  nodeClass*	(*initializationFunction)(const listClass&, const CTime_base&);

  const char* get_name() const override {
	return name;
  }

  //
  // adds one source of events.
  //
  // Final state: a pointer to the new list has been added to
  // 		dead_lists (if the new list is empty) or to
  // 		active_lists (if it is not)
  //
  virtual void		add_thread(	listClass& aList,
					const Cstring& thread_name,
					unsigned long itsPriority,
					bool B = false) override;

  virtual int		thread_count() override {
	return pointers_to_threads.get_length();
  }


  //
  // 'forgets' all the threads that were added.
  //
  // Final state: the state is the same as just after construction.
  //
  void		clear() override;

  //
  // resets threads to their list's beginning.
  //
  // Final state: active_threads is populated with pointers to
  // the earliest elements of each added list.
  //
  void		first() override;

  void		handle_addition(
			listClass* theList,
			nodeClass* theNode) override;
  void		handle_removal(
			listClass* theList,
			nodeClass* theNode) override;

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
  nodeClass*	next() override;

  //
  // same as next() but does not move on
  //
  nodeClass*	peek() override;

  //
  // same as peek() but returns the iterator
  //
  iterClass*	peek_at_iter();

  void		remove_thread(const listClass& listToRemove) override;

  //
  // Only used in testing:
  //
  iterClass*	there_is_a_current_event(Cstring& name, unsigned long& priority);

// DATA

  const char*	name;
  CTime_base	initializationTime;

  //
  // need to delete pointee when clearing
  //
  threadptrtlist	 	pointers_to_threads;

  //
  // used to store the strings associated with each thread
  //
  tlist<alpha_void, Cntnr<alpha_void, Cstring> > thread_names;

  // made public for debugging:
  // private:

  //
  // The following 2 lists provide a partition of all
  // threads into 2 groups:
  //
  Tthreadtlist			active_threads;		// Don't delete any nodes!
  Tthreadslist			dead_threads;		// Ditto!
};

template <class listClass, class nodeClass, class iterClass>
void
Miterator<listClass, nodeClass, iterClass>::handle_removal(listClass* theList, nodeClass* theNode) {
	threadptr*	ptr_to_concerned_thread = pointers_to_threads.find(theList);
	assert(ptr_to_concerned_thread);
	Tthread*	concerned_thread = ptr_to_concerned_thread->payload;
	assert(concerned_thread);

	if(concerned_thread->list == &active_threads) {
		if(theNode == concerned_thread->payload->getNode()) {
			active_threads.remove_node(concerned_thread);
			concerned_thread->payload->next();
			if(!concerned_thread->payload->is_empty()) {
				active_threads << concerned_thread;
			} else {
				dead_threads << concerned_thread;
			}
		}
	}
	// there is nothing else to check.
}

//
// This is actually for testing the Miterator template - make sure there are
// no references to a node that is about to be deleted. It would make sense
// to perform this test after a call to handle_removal()...
//
template <class listClass, class nodeClass, class iterClass>
void
Miterator<listClass, nodeClass, iterClass>::test_for_refs_to(nodeClass* theNodeToTest) {
	typename Tthreadslist::iterator	threads(active_threads);
	Tthread*			T;

	while((T = threads())) {
		if(T->payload->getNode() == theNodeToTest) {
			Cstring msg;

			msg << "test_for_refs_to(" << theNodeToTest->get_key()
				<< "): found a ref in list.";
			throw(eval_error(msg));
		}
	}
}

template <class listClass, class nodeClass, class iterClass>
void
Miterator<listClass, nodeClass, iterClass>::clear() {
	typename threadptrslist::iterator	iter(pointers_to_threads);
	threadptr*				v;

#	ifdef debug_templates
	cerr << "Miterator clearing " << pointers_to_threads.get_length()
		<< " threads...\n";
#	endif

	//
	// for each iterClass object (usually an iterator or a class derived from an iterator),
	//
	while((v = iter())) {	

#		ifdef debug_templates
		cerr << "    deleting iterator...\n";
#		endif

		if(v->payload->list) {

			//
			// do this first because v->payload
			// needs the iterClass object
			// v->payload->payload to extract
			// itself from its list */
			//
			v->payload->list->remove_node(v->payload);
		}

		//
		// v->payload->payload is an iterator for the list behind the thread.
		//
		// NOTE: casting v->payload->payload is unpleasant, but I don't
		// know of any way to let the template know the precise relationship
		// between iterClass and listClass... maybe the template parameters
		// could be structured differently?
		//
		((listClass*) v->payload->payload->L)->unregister_miterator(*this);

		//
		// delete the iterClass object
		//
		delete v->payload->payload;

#		ifdef debug_templates
		cerr << "    deleting Tthread...\n";
#		endif

		//
		// delete the Tthread pointing to the iterClass object
		//
		delete v->payload;
	}

#	ifdef debug_templates
	cerr << "    now clearing the actual list...\n";
#	endif

	pointers_to_threads.clear();

	//
	// Then delete each pointer to a Tthread pointing to an iterClass object.
	//
	thread_names.clear();
}

template <class listClass, class nodeClass, class iterClass>
Miterator<listClass, nodeClass, iterClass>::Miterator(
			const Miterator<listClass, nodeClass, iterClass>& mit)
		: active_threads(true),
			name(mit.name),
			initializationFunction(NULL),
			thread_names(mit.thread_names) {
	typename threadptrslist::iterator	l(mit.pointers_to_threads);
	threadptr*				tptr;
	Tthread*				tt;

	while((tptr = l())) {
		iterClass*	ic = new iterClass(*tptr->payload->payload);

		//
		// NOTE: casting v->payload->payload is unpleasant, but I don't
		// know of any way to let the template know the precise relationship
		// between iterClass and listClass... maybe the template parameters
		// could be structured differently?
		//
		((listClass*) ic->L)->register_miterator(*this);
		tt = new Tthread(relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
		pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
		if(tptr->payload->list == &mit.dead_threads) {
			dead_threads << tt;
		} else {
			active_threads << tt;
		}
	}
}

template <class listClass, class nodeClass, class iterClass>
Miterator<listClass, nodeClass, iterClass>::Miterator(
			const char*				P,
			const Miterator<listClass, nodeClass, iterClass>&	mit)
		: active_threads(true),
			name(P),
			initializationFunction(NULL),
			thread_names(mit.thread_names) {
	typename threadptrslist::iterator	l(mit.pointers_to_threads);
	threadptr*				tptr;
	Tthread*				tt;

	while((tptr = l())) {
		iterClass*	ic = new iterClass(*tptr->payload->payload);

		//
		// NOTE: casting v->payload->payload is unpleasant, but I don't
		// know of any way to let the template know the precise relationship
		// between iterClass and listClass... maybe the template parameters
		// could be structured differently?
		//
		((listClass*) ic->L)->register_miterator(*this);
		tt = new Tthread(relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
		pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
		if(tptr->payload->list == &mit.dead_threads) {
			dead_threads << tt;
		} else {
			active_threads << tt;
		}
	}
}

template <class listClass, class nodeClass, class iterClass>
Miterator<listClass, nodeClass, iterClass>::Miterator(
			const char*				P,
			const Miterator<listClass, nodeClass, iterClass>&	mit,
			nodeClass* (*initFunc)(const listClass&, const CTime_base&),
			const CTime_base& initTime
			)
		: active_threads(true),
			name(P),
			initializationFunction(initFunc),
			initializationTime(initTime),
			thread_names(mit.thread_names) {
	typename threadptrslist::iterator	l(mit.pointers_to_threads);

	//
	// Reminder:
	//
  	// typedef Cntnr<relptr<iterClass>, iterClass*>	Tthread;
  	// typedef Cntnr<alpha_void, Tthread*>			threadptr;
  	//
	threadptr*				tptr;

	Tthread*				tt;

	while((tptr = l())) {
		//
		// This iterator points to the same list element as the
		// original Miterator's current pointer for this thread:
		//
		iterClass*	ic = new iterClass(*tptr->payload->payload);

		//
		// NOTE: casting v->payload->payload is unpleasant, but I don't
		// know of any way to let the template know the precise relationship
		// between iterClass and listClass... maybe the template parameters
		// could be structured differently?
		listClass*	lc = (listClass*) ic->L;

		//
		// The trick here is to get the correct key priority. Priority
		// is stored in each node and also in the list, I believe...
		//
		ic->go_to(initializationFunction(*lc, initializationTime));
		
		lc->register_miterator(*this);
		tt = new Tthread(relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
		pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
		if(tptr->payload->list == &mit.dead_threads) {
			dead_threads << tt;
		} else {
			active_threads << tt;
		}
	}
}

template <class listClass, class nodeClass, class iterClass>
void Miterator<listClass, nodeClass, iterClass>::dump_one(
		Cntnr<relptr<iterClass>, iterClass*>* n,
		stringstream& s,
		int k) {
	int i;
	if(!n) return;
	dump_one(n->Links[0], s, k);
	for(i = 0; i < 2 * k; i++) {
		s << " ";
	}
	s << n->payload->getNode()->get_descr() << " - " << n->Key.get_priority() << "\n";
	dump_one(n->Links[1], s, k);
}

template <class listClass, class nodeClass, class iterClass>
void Miterator<listClass, nodeClass, iterClass>::dump(std::stringstream& s, int k) {
	dump_one(active_threads.root, s, k);
}

template <class listClass, class nodeClass, class iterClass>
void
Miterator<listClass, nodeClass, iterClass>::first() {
	Tthread*				tt;
	threadptr*				tptr;
	typename threadptrslist::iterator	l(pointers_to_threads);

	//
	// Remember that active_threads is a clist, so all
	// threads are in time order!
	//
	while((tt = active_threads.first_node())) {
		active_threads.remove_node(tt);
	}
	while((tt = dead_threads.first_node())) {
		dead_threads.remove_node(tt);
	}

	//
	// DO NOT DO THIS! It would delete the nodes; we
	// still need them.
	//
	// active_threads.clear();
	// dead_threads.clear();

	while((tptr = l())) {
		tt = tptr->payload;
		tt->payload->go_to_beginning();
		if(!tt->payload->is_empty()) {
			active_threads << tt;
		} else {
			dead_threads << tt;
		}
	}
}

template <class listClass, class nodeClass, class iterClass>
nodeClass*
Miterator<listClass, nodeClass, iterClass>::peek() {
	Tthread*	tt = active_threads.first_node();
	return tt ? tt->payload->getNode() : NULL;
}

template <class listClass, class nodeClass, class iterClass>
iterClass*
Miterator<listClass, nodeClass, iterClass>::peek_at_iter() {
	Tthread*	tt = active_threads.first_node();
	return tt->payload;
}

template <class listClass, class nodeClass, class iterClass>
nodeClass*
Miterator<listClass, nodeClass, iterClass>::next() {
	Tthread*	tt = active_threads.first_node();
	Tthread*	new_current_node;
	nodeClass*	is_there_a_future = NULL;
	nodeClass*	to_return = NULL;

	if(!tt) {
		first();
		return NULL;
	}
	to_return = tt->payload->getNode();
	assert(tt->Key.mC == tt->payload);
	active_threads.remove_node(tt);
	assert(!tt->list);

	is_there_a_future = tt->payload->next();
	if(is_there_a_future) {
		active_threads << tt;
	} else {
		dead_threads << tt;
	}
	return to_return;
}

//
// Only used in testing:
//
template <class listClass, class nodeClass, class iterClass>
iterClass*
Miterator<listClass, nodeClass, iterClass>::there_is_a_current_event(
		Cstring& name,
		unsigned long& prio) {
	Tthread*	tt = active_threads.first_node();

	if(!tt) {
		return NULL;
	}
	assert(thread_names.find(tt->payload->get_void_list()));
	name = thread_names.find(tt->payload->get_void_list())->payload;
	prio = tt->Key.get_priority();
	return tt->payload;
}

template <class listClass, class nodeClass, class iterClass>
void
Miterator<listClass, nodeClass, iterClass>::add_thread(
				listClass&	lc,
				const Cstring&	thread_name,
				unsigned long	prio,
				bool) {
	iterClass*	ic = new iterClass(lc);
	Tthread*	new_thread = new Tthread(relptr<iterClass>(ic, prio), ic);

	lc.register_miterator(*this);

	thread_names << new Cntnr<alpha_void, Cstring>(
				(void*)&lc,
				thread_name);
	ic->go_to_beginning();
	pointers_to_threads << new Cntnr<alpha_void, Tthread*>(
						(void*)&lc,
						new_thread);
}

template <class listClass, class nodeClass, class iterClass>
void
Miterator<listClass, nodeClass, iterClass>::remove_thread(const listClass& lc) {
	Cntnr<alpha_void, Tthread*>*	old_ptr = pointers_to_threads.find(&lc);
	assert(old_ptr);
	iterClass*			iter_to_delete = old_ptr->payload->payload;
	Tthread*			old_thread = old_ptr->payload;
	assert(old_thread);
	Cntnr<alpha_void, Cstring>*	nameptr = thread_names.find(old_thread->payload->get_void_list());
	assert(nameptr);

	delete nameptr;
	delete old_ptr;
	delete old_thread;
	delete iter_to_delete;
}

template <class listClass, class nodeClass, class iterClass>
void
Miterator<listClass, nodeClass, iterClass>::handle_addition(listClass* theList, nodeClass* node_to_adopt) {
	threadptr*			ptr_to_adoptive_thread = pointers_to_threads.find(theList);
	assert(ptr_to_adoptive_thread);
	Tthread*			adoptive_thread = ptr_to_adoptive_thread->payload;
	assert(adoptive_thread);
	Tthread*			current_thread = active_threads.first_node();
	CTime_base			adoptive_time(node_to_adopt->getKey().getetime());

	/* NOTE: In principle, we should check that the node to adopt occurs in the future - but
	 *	 in reality, this is difficult to do because Miterators do not
	 *	 track "now". The latest node strictly before the current
	 *	 active node is not necessarily the node with a time tag of
	 *	 "now", which might reside in another Miterator. */
	if(adoptive_thread->list == &active_threads) {
		int		compresult;
		nodeClass*	current_node_in_adoptive_thread = adoptive_thread->payload->getNode();
		nodeClass*	one_before;
		if((compresult = node_to_adopt->getKey().compare_to(current_node_in_adoptive_thread->getKey())) < 0) {
			if((one_before = current_node_in_adoptive_thread->previous_node())) {
				/* Original version:
				 *
				 * 	if(node_to_adopt->getKey().compare_to(one_before->getKey()) < 0)
				 *
				 * but this is too stringent - after all, we run the potential_triggers
				 * Miterator as far as it can while the time tag stays the same in the
				 * at_least_1_thread_can_be_unblocked() function in ACT_edit.C.
				 *
				 * Still, it would be good to investigate - see ScheduleObservations
				 * test run_script_2-apgen9. */
				if(node_to_adopt->getKey().getetime() < one_before->getKey().getetime()) {
					Cstring msg;
					msg << "handle_addition: attempting to adopt a node (key = " << node_to_adopt->getKey().get_key();
					// get_key() may involve time_to_SCET_string()...
					msg << ") earlier than the node before the current node (key = " << one_before->getKey().get_key() << ").";
					throw(eval_error(msg));
				}
			}
			active_threads.remove_node(adoptive_thread);
			adoptive_thread->payload->iterNode = node_to_adopt;
			active_threads << adoptive_thread;
		} else if(!compresult) {
			throw(eval_error("handle_addition: trying to add a node identical to the current node"));
		}
		/* else, the node to adopt will come up naturally as we move on.
		 * There is nothing special we should do. */
	} else if(adoptive_thread->list == &dead_threads) {
		dead_threads.remove_node(adoptive_thread);
		adoptive_thread->payload->iterNode = node_to_adopt;
		active_threads << adoptive_thread;
	} else if(!adoptive_thread->list) {
		// the list was empty and the thread was not put anywhere
		adoptive_thread->payload->go_to_beginning();
		if(!adoptive_thread->payload->is_empty()) {
			active_threads << adoptive_thread;
		} else {
			dead_threads << adoptive_thread;
		}
	}
}

#endif /* _MULTITEMPLATE_H_ */
