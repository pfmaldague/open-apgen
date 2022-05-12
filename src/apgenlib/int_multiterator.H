#ifndef _INT_MULTITEMPLATE_H_
#define _INT_MULTITEMPLATE_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <sstream>

#include <timeptr.H>

/* Interator - a robust multiple iterator. It behaves like an iterator, except
 * that it is able to scan multiple lists simultaneously. The lists to be
 * scanned should be added through the add_thread() method.
 *
 * Interator requirements:
 *
 * 	- listClass and nodeClass should be "as expected" (obvious)
 *
 * 	- iterClass must be suitable as an argument for the int_relptr template, i. e.:
 * 		- have a getNode() method
 * 		- the object to which getNode() returns a pointer should have a
 * 		  getKey() method that returns a Key suitable for slist and tlist.
 *
 *
 * Some design notes: 
 *
 *    - A Interator is always is a well-defined state
 *
 *    - the state of a Interator is characterized by
 *
 *    		its list of threads; each thread corresponds to one list
 *
 *    		its list of active_threads
 *
 *    		its list of dead_threads
 *
 * NOTE: a Interator may itself be watched by "super-Interators" which need to
 * be informed whenever the current node of a Interator changes. We need a
 * mechanism for this.
 */

template <class listClass, class nodeClass, class keyClass>
class Interator: public Miterator_base<listClass, nodeClass> {
public:
  typedef typename listClass::iterator		iterClass;
  typedef Cntnr<int_relptr<iterClass>, iterClass*>	Tthread;
  typedef tlist<int_relptr<iterClass>, Tthread>	Tthreadtlist;
  typedef slist<int_relptr<iterClass>, Tthread>	Tthreadslist;
  typedef Cntnr<alpha_void, Tthread*>		threadptr;
  typedef slist<alpha_void, threadptr>		threadptrslist;
  typedef tlist<alpha_void, threadptr>		threadptrtlist;

/* NOTE: what used to be called 'tol_thread' is now a Tthread. It's important
 * to keep the Tnode's key, because the int_relptr<iterClass> class contains the
 * priority information. That's why the master list of threads must contain
 * Tthreads and not iterClasses.
 *
 * On the other hand, we still need to access threads based on the address
 * of the corresponding time-based tlist. That's why we have pointers_to_threads. */

// for general-purpose use, in particular the modeling loop:
  Interator(const char* p)
	: name(p),
		active_threads(true),
		initializationFunction(NULL) {}
  Interator(const char* p, const Interator& mit);
  Interator(	const char* p,
		const Interator& mit,
		nodeClass* (*initFunc)(const listClass&, const long int&),
		const long int& initInt);
private:
  Interator(const Interator& mit);
public:
  ~Interator() { clear(); }

// METHODS

  //
  // debug methods
  //
  void		dump(std::stringstream&, int);
  void		dump_one(Tthread*, std::stringstream&, int);
  void		test_for_refs_to(nodeClass*) override;

  //
  // other methods
  //
  nodeClass*	(*initializationFunction)(const listClass&, const long int&);

  const char*	get_name() const override {
	return name;
  }

		//
		// adds one source of events.
		// Final state: a pointer to the new list has been added to
		// 		dead_lists (if the new list is empty) or to
		// 		active_lists (if it is not)
		// Note: the last argument is for compatibility with other
		// iterators that deal with resource histories.
		//
  virtual void	add_thread(	listClass& aList,
				const Cstring& thread_name,
				unsigned long itsPriority,
				bool = false) override;

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
		// Final state: active_threads is populated with pointers
		// to the earliest elements of each added list.
		//
  void		first() override;

  long int	get_int() {
	long int	T0;
	if(is_empty()) {
		return T0;
	}
	/* we know the Key of a Tthread is a int_relptr<iterator>,
	 * so it must have an mC member that is an iterator */
	return active_threads.first_node()->getKey().mC->getNode()->getKey().get_int();
  }

  void		handle_addition(listClass* theList, nodeClass* theNode) override;
  void		handle_removal(listClass* theList, nodeClass* theNode) override;

  bool		is_empty() {
	return active_threads.get_length() == 0;
  } // we ran out of events (no state change)


		// identifies the current event and returns it after moving on (changing state) to the next event
  nodeClass*	next() override;

		// same as next() but does not move on
  nodeClass*	peek() override;

  void		remove_thread(const listClass& listToRemove) override;

  //
  // Only used in testing:
  //
  iterClass*	there_is_a_current_event(Cstring& name, unsigned long& priority); // returns the current event (no state change)

// DATA

  const char*	name;
  long int	initializationInt;

  threadptrtlist					pointers_to_threads;	// need to delete pointee when clearing
  tlist<alpha_void, Cntnr<alpha_void, Cstring> >	thread_names;		// used to store the strings associated with each thread

  // made public for debugging:
  // private:

  /* The following 2 lists provide a partition of all
   * threads into 2 groups: */
  Tthreadtlist			active_threads;		// Don't delete any nodes!
  Tthreadslist			dead_threads;		// Ditto!
};

template <class listClass, class nodeClass, class keyClass>
void
Interator<listClass, nodeClass, keyClass>::handle_removal(listClass* theList, nodeClass* theNode) {
	threadptr*			ptr_to_concerned_thread = pointers_to_threads.find(theList);
	assert(ptr_to_concerned_thread);
	Tthread*			concerned_thread = ptr_to_concerned_thread->payload;
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

/* this is actually for testing the Interator template - make sure there are
 * no references to a node that is about to be deleted. It would make sense
 * to perform this test after a call to handle_removal()... */
template <class listClass, class nodeClass, class keyClass>
void
Interator<listClass, nodeClass, keyClass>::test_for_refs_to(nodeClass* theNodeToTest) {
	typename Tthreadslist::iterator	threads(active_threads);
	Tthread*			T;

	while((T = threads())) {
		if(T->payload->getNode() == theNodeToTest) {
			Cstring msg;

			msg << "test_for_refs_to(" << theNodeToTest->get_key() << "): found a ref in list.";
			throw(eval_error(msg));
		}
	}
}

template <class listClass, class nodeClass, class keyClass>
void
Interator<listClass, nodeClass, keyClass>::clear() {
	typename threadptrslist::iterator	iter(pointers_to_threads);
	threadptr*				v;

#	ifdef debug_templates
	cerr << "Interator clearing " << pointers_to_threads.get_length() << " threads...\n";
#	endif
	while((v = iter())) {		// for each iterClass object (usually an iterator or a class derived from an iterator),
#		ifdef debug_templates
		cerr << "    deleting iterator...\n";
#		endif
		if(v->payload->list) {
			/* do this first because v->payload
			 * needs the iterClass object
			 * v->payload->payload to extract
			 * itself from its list */
			v->payload->list->remove_node(v->payload);
		}
		/* v->payload->payload is an iterator for the list behind the thread.
		 *
		 * NOTE: casting v->payload->payload is unpleasant, but I don't
		 * know of any way to let the template know the precise relationship
		 * between iterClass and listClass... maybe the template parameters
		 * could be structured differently? */
		((listClass*) v->payload->payload->L)->unregister_miterator(*this);
		// delete the iterClass object
		delete v->payload->payload;
#		ifdef debug_templates
		cerr << "    deleting Tthread...\n";
#		endif
		// delete the Tthread pointing to the iterClass object
		delete v->payload;
	}
#	ifdef debug_templates
	cerr << "    now clearing the actual list...\n";
#	endif
	pointers_to_threads.clear();	// Then delete each pointer to a Tthread pointing to an iterClass object.
	thread_names.clear();
}

template <class listClass, class nodeClass, class keyClass>
Interator<listClass, nodeClass, keyClass>::Interator(
			const Interator<listClass, nodeClass, keyClass>& mit)
		: active_threads(true),
			name(mit.name),
			initializationFunction(NULL),
			thread_names(mit.thread_names) {
	typename threadptrslist::iterator	l(mit.pointers_to_threads);
	threadptr*				tptr;
	Tthread*				tt;

	while((tptr = l())) {
		iterClass*	ic = new iterClass(*tptr->payload->payload);

		/* NOTE: casting v->payload->payload is unpleasant, but I don't
		 * know of any way to let the template know the precise relationship
		 * between iterClass and listClass... maybe the template parameters
		 * could be structured differently? */
		((listClass*) ic->L)->register_miterator(*this);
		tt = new Tthread(int_relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
		pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
		if(tptr->payload->list == &mit.dead_threads) {
			dead_threads << tt;
		} else {
			active_threads << tt;
		}
	}
}

template <class listClass, class nodeClass, class keyClass>
Interator<listClass, nodeClass, keyClass>::Interator(
			const char*				P,
			const Interator<listClass, nodeClass, keyClass>&	mit)
		: active_threads(true),
			name(P),
			initializationFunction(NULL),
			thread_names(mit.thread_names) {
	typename threadptrslist::iterator	l(mit.pointers_to_threads);
	threadptr*				tptr;
	Tthread*				tt;

	while((tptr = l())) {
		iterClass*	ic = new iterClass(*tptr->payload->payload);

		/* NOTE: casting v->payload->payload is unpleasant, but I don't
		 * know of any way to let the template know the precise relationship
		 * between iterClass and listClass... maybe the template parameters
		 * could be structured differently? */
		((listClass*) ic->L)->register_miterator(*this);
		tt = new Tthread(int_relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
		pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
		if(tptr->payload->list == &mit.dead_threads) {
			dead_threads << tt;
		} else {
			active_threads << tt;
		}
	}
}

template <class listClass, class nodeClass, class keyClass>
Interator<listClass, nodeClass, keyClass>::Interator(
			const char*						P,
			const Interator<listClass, nodeClass, keyClass>&	mit,
			nodeClass* (*initFunc)(const listClass&, const long int&),
			const long int& initInt
			)
		: active_threads(true),
			name(P),
			initializationFunction(initFunc),
			initializationInt(initInt),
			thread_names(mit.thread_names) {
	typename threadptrslist::iterator	l(mit.pointers_to_threads);

	//
	// Reminder:
	//
  	// typedef Cntnr<int_relptr<iterClass>, iterClass*>	Tthread;
  	// typedef Cntnr<alpha_void, Tthread*>		threadptr;
  	//
	threadptr*				tptr;

	Tthread*				tt;

	while((tptr = l())) {
		//
		// This iterator points to the same list element as the
		// original Interator's current pointer for this thread:
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
		ic->go_to(initializationFunction(*lc, initializationInt));
		
		lc->register_miterator(*this);
		tt = new Tthread(int_relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
		pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
		if(tptr->payload->list == &mit.dead_threads) {
			dead_threads << tt;
		} else {
			active_threads << tt;
		}
	}
}

template <class listClass, class nodeClass, class keyClass>
void Interator<listClass, nodeClass, keyClass>::dump_one(
		Cntnr<int_relptr<iterClass>, iterClass*>* n,
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

template <class listClass, class nodeClass, class keyClass>
void Interator<listClass, nodeClass, keyClass>::dump(std::stringstream& s, int k) {
	dump_one(active_threads.root, s, k);
}

template <class listClass, class nodeClass, class keyClass>
void
Interator<listClass, nodeClass, keyClass>::first() {
	Tthread*				tt;
	threadptr*				tptr;
	typename threadptrslist::iterator	l(pointers_to_threads);

	// Remember that active_threads is a clist, so all threads are in time order!
	while((tt = active_threads.first_node())) {
		active_threads.remove_node(tt);
	}
	while((tt = dead_threads.first_node())) {
		dead_threads.remove_node(tt);
	}
	// DO NOT DO THIS! It would delete the nodes; we still need them.
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

template <class listClass, class nodeClass, class keyClass>
nodeClass*
Interator<listClass, nodeClass, keyClass>::peek() {
	Tthread*	tt = active_threads.first_node();
	/* we got to look at 2 things: the next node within the active thread, and
	 * the second node in the list of threads */

	return tt ? tt->payload->getNode() : NULL;
}

template <class listClass, class nodeClass, class keyClass>
nodeClass*
Interator<listClass, nodeClass, keyClass>::next() {
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
template <class listClass, class nodeClass, class keyClass>
typename listClass::iterator*
Interator<listClass, nodeClass, keyClass>::there_is_a_current_event(Cstring& name, unsigned long& prio) {
	Tthread*	tt = active_threads.first_node();

	if(!tt) {
		return NULL;
	}
	assert(thread_names.find(tt->payload->get_void_list()));
	name = thread_names.find(tt->payload->get_void_list())->payload;
	prio = tt->Key.get_priority();
	return tt->payload;
}

template <class listClass, class nodeClass, class keyClass>
void
Interator<listClass, nodeClass, keyClass>::add_thread(
		listClass& lc,
		const Cstring& thread_name,
		unsigned long prio,
		bool) {
	iterClass*	ic = new iterClass(lc);
	Tthread*	new_thread = new Tthread(int_relptr<iterClass>(ic, prio), ic);

	lc.register_miterator(*this);

	thread_names << new Cntnr<alpha_void, Cstring>(new_thread->payload->get_void_list(), thread_name);
	ic->go_to_beginning();
	pointers_to_threads << new Cntnr<alpha_void, Tthread*>(
						new_thread->payload->get_void_list(),
						new_thread);
}

template <class listClass, class nodeClass, class keyClass>
void
Interator<listClass, nodeClass, keyClass>::remove_thread(const listClass& lc) {
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

template <class listClass, class nodeClass, class keyClass>
void
Interator<listClass, nodeClass, keyClass>::handle_addition(listClass* theList, nodeClass* node_to_adopt) {
	threadptr*			ptr_to_adoptive_thread = pointers_to_threads.find(theList);
	assert(ptr_to_adoptive_thread);
	Tthread*			adoptive_thread = ptr_to_adoptive_thread->payload;
	assert(adoptive_thread);
	Tthread*			current_thread = active_threads.first_node();
	long int			adoptive_time(node_to_adopt->getKey().get_int());

	/* NOTE: In principle, we should check that the node to adopt occurs in the future - but
	 *	 in reality, this is difficult to do because Interators do not
	 *	 track "now". The latest node strictly before the current
	 *	 active node is not necessarily the node with a time tag of
	 *	 "now", which might reside in another Interator. */
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
				 * Interator as far as it can while the time tag stays the same in the
				 * at_least_1_thread_can_be_unblocked() function in ACT_edit.C.
				 *
				 * Still, it would be good to investigate - see ScheduleObservations
				 * test run_script_2-apgen9. */
				if(node_to_adopt->getKey().get_int() < one_before->getKey().get_int()) {
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

#endif /* _INT_MULTITEMPLATE_H_ */
