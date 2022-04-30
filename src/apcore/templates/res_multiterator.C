#include "Rsource.H"
#include "res_multiterator.H"
#include "apcoreWaiter.H"

extern streamx& tol_debug_stream();

#define ASSERT( x ) 


void
simple_res_miter::handle_removal(
		slist<prio_time, value_node, Rsource*>* theList,
		value_node* theNodeToBeDeleted) {
	threadptr*	ptr_to_concerned_thread = pointers_to_threads.find(theList);
	ASSERT(ptr_to_concerned_thread);
	Tthread*	concerned_thread = ptr_to_concerned_thread->payload;
	ASSERT(concerned_thread);
	iterClass*	the_iterator = concerned_thread->payload;

	if(concerned_thread->list == &active_threads) {
		if(theNodeToBeDeleted == the_iterator->getNode()) {
			active_threads.remove_node(concerned_thread);
			the_iterator->iterNode = the_iterator->iterNode->next_node();
			if(!concerned_thread->payload->is_empty()) {
				active_threads << concerned_thread;
			} else {
				dead_threads << concerned_thread;
			}
			return;
		}
	}
}

//
// this is actually for testing the simple_res_miter template - make sure there are
// no references to a node that is about to be deleted. It would make sense
// to perform this test after a call to handle_removal()...
//
void
simple_res_miter::test_for_refs_to(value_node* theNodeToTest) {
	typename Tthreadslist::iterator	threads(active_threads);
	Tthread*			T;

	while((T = threads())) {
		if(T->payload->getNode() == theNodeToTest) {
		    Cstring errs;

		    errs << "test_for_refs_to("
			<< theNodeToTest->get_key() << "): found a ref in list.";
		    throw(eval_error(errs));
		}
	}
}

void
simple_res_miter::clear() {
	typename threadptrslist::iterator	iter(pointers_to_threads);
	threadptr*				v;

	//
	// for each iterClass object (usually an iterator or a class derived from an iterator),
	//
	while((v = iter())) {

		if(v->payload->list) {

			//
			// do this first because v->payload
			// needs the iterClass object
			// v->payload->payload to extract
			// itself from its list
			//
			v->payload->list->remove_node(v->payload);
		}

		//
		// v->payload->payload is an iterator for the list behind the thread.
		//
		baseListClass* ptr_to_list = (baseListClass*)
			dynamic_cast<const baseListClass*>(v->payload->payload->L);
		ASSERT(ptr_to_list);

		ptr_to_list->unregister_miterator(*this);

		//
		// delete the iterClass object
		//
		delete v->payload->payload;

		//
		// delete the Tthread pointing to the iterClass object
		//
		delete v->payload;
	}

	pointers_to_threads.clear();
	thread_names.clear();
}

simple_res_miter::simple_res_miter(
			const simple_res_miter& mit)
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
	((listClass*) ic->L)->register_miterator(*this, ic);
	tt = new Tthread(relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
	pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
	if(tptr->payload->list == &mit.dead_threads) {
		dead_threads << tt;
	} else {
		active_threads << tt;
	}
    }
}

simple_res_miter::simple_res_miter(
			const char*		P,
			const simple_res_miter&	mit)
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
	((listClass*) ic->L)->register_miterator(*this, ic);
	tt = new Tthread(relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
	pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
	if(tptr->payload->list == &mit.dead_threads) {
		dead_threads << tt;
	} else {
		active_threads << tt;
	}
    }
}

simple_res_miter::simple_res_miter(
			const char*		P,
			const simple_res_miter&	mit,
			value_node* (*initFunc)(const slist<prio_time, value_node, Rsource*>&,
				const CTime_base&),
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
    // typedef Cnode0<relptr<iterClass>, iterClass*>	Tthread;
    // typedef Cnode0<alpha_void, Tthread*>		threadptr;
    //
    threadptr*				tptr;

    Tthread*				tt;

    while((tptr = l())) {

	//
	// This iterator points to the same list element as the
	// original simple_res_miter's current pointer for this thread:
	//
	iterClass*	ic = new iterClass(*tptr->payload->payload);

	//
	// NOTE: casting v->payload->payload is unpleasant, but I don't
	// know of any way to let the template know the precise relationship
	// between iterClass and listClass... maybe the template parameters
	// could be structured differently?
	//
	listClass*	lc = (listClass*) ic->L;

	//
	// The trick here is to get the correct key priority. Priority
	// is stored in each node and also in the list, I believe...
	//
	ic->go_to(initializationFunction(*lc, initializationTime));

	lc->register_miterator(*this, ic, true);
	tt = new Tthread(relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
	pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
	if(tptr->payload->list == &mit.dead_threads) {
		dead_threads << tt;
	} else {
		active_threads << tt;
	}
    }
}

void simple_res_miter::dump_one(
		Cnode0<relptr<slist<prio_time, value_node, Rsource*>::iterator>,
			slist<prio_time, value_node, Rsource*>::iterator*>* n,
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

void simple_res_miter::dump(std::stringstream& s, int k) {
	dump_one(active_threads.root, s, k);
}

void
simple_res_miter::first() {
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

value_node*
simple_res_miter::peek() {
	Tthread*	tt = active_threads.first_node();
	return tt ? tt->payload->getNode() : NULL;
}

slist<prio_time, value_node, Rsource*>::iterator*
simple_res_miter::peek_at_iter() {
	Tthread*	tt = active_threads.first_node();
	return tt->payload;
}

value_node*
simple_res_miter::next() {
	Tthread*	tt = active_threads.first_node();
	Tthread*	new_current_node;
	value_node*	is_there_a_future = NULL;
	value_node*	to_return = NULL;

	if(!tt) {
		first();
		return NULL;
	}
	to_return = tt->payload->getNode();
	active_threads.remove_node(tt);

	is_there_a_future = tt->payload->next();
	if(is_there_a_future) {
		active_threads << tt;
	} else {
		dead_threads << tt;
	}
	return to_return;
}

//
// Only used for testing:
//
slist<prio_time, value_node, Rsource*>::iterator*
simple_res_miter::there_is_a_current_event(Cstring& name, unsigned long& prio) {
	Tthread*	tt = active_threads.first_node();

	if(!tt) {
		return NULL;
	}
	ASSERT(thread_names.find(tt->payload->get_void_list()));
	name = thread_names.find(tt->payload->get_void_list())->payload;
	prio = tt->Key.get_priority();
	return tt->payload;
}

void
simple_res_miter::add_thread(
		slist<prio_time, value_node, Rsource*>& lc,
		const Cstring& thread_name,
		unsigned long prio,
		bool store) {
	iterClass*	ic = new iterClass(lc);
	Tthread*	new_thread = new Tthread(relptr<iterClass>(ic, prio), ic);


	thread_names << new Cnode0<alpha_void, Cstring>(
		new_thread->payload->get_void_list(), thread_name);

	ic->go_to_beginning();
	pointers_to_threads << new Cnode0<alpha_void, Tthread*>(
					new_thread->payload->get_void_list(),
					new_thread);
	lc.register_miterator(*this, ic, store);
}

void
simple_res_miter::remove_thread(const slist<prio_time, value_node, Rsource*>& lc) {
	Cnode0<alpha_void, Tthread*>*	old_ptr = pointers_to_threads.find(&lc);
	iterClass*			iter_to_delete = old_ptr->payload->payload;
	Tthread*			old_thread = old_ptr->payload;
	Cnode0<alpha_void, Cstring>*	nameptr
		= thread_names.find(old_thread->payload->get_void_list());

	delete nameptr;
	delete old_ptr;
	delete old_thread;
	delete iter_to_delete;
}

void
simple_res_miter::handle_addition(
		slist<prio_time, value_node, Rsource*>* theList,
		value_node* node_to_adopt) {
    threadptr*	ptr_to_adoptive_thread
				= pointers_to_threads.find(theList);

    Tthread*	adoptive_thread = ptr_to_adoptive_thread->payload;

    baseListClass::iterator* It = adoptive_thread->payload;
    Tthread*		 current_thread = active_threads.first_node();
    CTime_base		 adoptive_time(node_to_adopt->getKey().getetime());

    //
    // NOTE: In principle, we should check that the node to adopt occurs
    // in the future - but in reality, this is difficult to do because
    // simple_res_miter do not track "now". The latest node strictly before
    // the current active node is not necessarily the node with a time tag
    // of "now", which might reside in another simple_res_miter.
    //
    if(adoptive_thread->list == &active_threads) {
	int		compresult;
	value_node*	current_node_in_adoptive_thread = It->getNode();
	value_node*	one_before;

	if((compresult = node_to_adopt->getKey().compare_to(
				current_node_in_adoptive_thread->getKey())) < 0) {
	    if((one_before = current_node_in_adoptive_thread->previous_node())) {
    		Rsource* res = It->L->Owner;

		if(node_to_adopt->getKey().getetime() < one_before->getKey().getetime()) {
			Cstring errs;
			errs << "handle_addition(" << res->name
				<< "): attempting to adopt a node (key = "
				<< node_to_adopt->getKey().get_key();

			errs << ") earlier than the node before the current node (key = "
				<< one_before->getKey().get_key() << ").";
			throw(eval_error(errs));
		}
	    }
	    active_threads.remove_node(adoptive_thread);

	    //
	    // No need to update the iterator's "previous node",
	    // although it may trail by a little bit
	    //
	    It->iterNode = node_to_adopt;
	    active_threads << adoptive_thread;
	} else if(!compresult) {
    		Rsource* res = It->L->Owner;
		Cstring errs;
		errs << "handle_addition(" << res->name
				<< "): trying to add a node "
				<< "identical to the current node";
		throw(eval_error(errs));
	}

	//
	// else, the node to adopt will come up naturally as we move on.
	// There is nothing special we should do.
	//
    } else if(adoptive_thread->list == &dead_threads) {

	//
	// NOTE: multilist calls handle_addition AFTER inserting a
	// new node; most of the time during modeling, addition occurs
	// at the end of the list
	//
	dead_threads.remove_node(adoptive_thread);
	It->iterNode = node_to_adopt;
	active_threads << adoptive_thread;
    } else if(!adoptive_thread->list) {

	//
	// the list was empty and the thread was not put anywhere
	//
	adoptive_thread->payload->go_to_beginning();
	if(!adoptive_thread->payload->is_empty()) {
		active_threads << adoptive_thread;
	} else {
		dead_threads << adoptive_thread;
	}
    }
}

//	====================== END OF SIMPLE_RES_MITER =========================


//
// Calls to this method originate in slist::remove_node, which invokes
// slist::notify_iterators_about_removing before actually removing a node from
// itself. The notify--- method is only a stub in slist, but it is overridden
// in the multilist class template which is the base class for resource
// histories.
//
// The purpose of notification is to provide multi-iterators (such as
// num_res_iter) with an opportunity to adjust their internal data as a result
// of node removal and addition. The task is made easier by the fact that
// during modeling, the main multi-iterator is EventLoop::potential_triggers,
// and the modeling loop design ensures that every Miterator thread is in one
// of the following possible states:
//
//	- the current thread is active, its iterator points to a time that
//	  is equal to or greater than modeling time, and the iterator's
//	  "previous node" points to a time less than or equal to modeling time
//
//	- the current thread is dead, its iterator points to a time that is
//	  earlier than modeling time, and the iterator's "previous node"
//	  points to a time less than or equal to the time of the current node
//
// Furthermore, when the modeling loop removes a node (usually to delete it),
// the removed node has a time tag that is at least equal to modeling time.
//
void
res_curval_miter::handle_removal(
		slist<prio_time, value_node, Rsource*>* theList,
		value_node* theNodeToBeDeleted) {
	threadptr*	ptr_to_concerned_thread = pointers_to_threads.find(theList);
	ASSERT(ptr_to_concerned_thread);
	Tthread*	concerned_thread = ptr_to_concerned_thread->payload;
	ASSERT(concerned_thread);
	iterClass*	the_iterator = concerned_thread->payload;
	value_node* the_previous_one = the_iterator->getPrev();

	if(concerned_thread->list == &active_threads) {
		if(theNodeToBeDeleted == the_iterator->getNode()) {
			int relative_pos = theNodeToBeDeleted->Key.compare_to(
				the_previous_one->Key);

			active_threads.remove_node(concerned_thread);

			switch(theNodeToBeDeleted->Key.compare_to(the_previous_one->Key)) {
				case 1:
					//
					// normal case; the previous node sits
					// before the node to be deleted. We
					// will leave the previous node
					// untouched. Don't call next()!!
					//
					the_iterator->iterNode
						= the_iterator->iterNode->next_node();
					break;
				case 0:
					//
					// not so normal case: the previous
					// node coincides with the current
					// node. Could be that both are at
					// the beginning of the list. We keep
					// things that way, but both now point
					// to the next node.
					//
					the_iterator->iterNode
						= the_iterator->iterNode->next_node();
					the_iterator->prevNode = the_iterator->iterNode;
					break;
				case -1:
				default:
					//
					// should never happen
					//
					assert(false);
			}
			// concerned_thread->payload->next();
			if(!concerned_thread->payload->is_empty()) {
				active_threads << concerned_thread;
			} else {
				dead_threads << concerned_thread;
			}
			return;
		}
		if(theNodeToBeDeleted == the_previous_one) {
			value_node* try_this_one = theNodeToBeDeleted->previous_node();
			if(try_this_one) {
				concerned_thread->payload->prevNode = try_this_one;
			} else {

				//
				// Both are now at the beginning of the list
				//
				concerned_thread->payload->prevNode
					= concerned_thread->payload->iterNode;
			}
		}
	} else {

		//
		// The iterator's previous node may not be NULL!
		//
		if(theNodeToBeDeleted == the_previous_one) {
			value_node* try_this_one = theNodeToBeDeleted->previous_node();
			if(try_this_one) {
				concerned_thread->payload->prevNode = try_this_one;
			} else {

				//
				// Not much we can do...
				//
				concerned_thread->payload->prevNode
					= NULL;
			}
		}
	}
}

//
// this is actually for testing the res_curval_miter template - make sure there are
// no references to a node that is about to be deleted. It would make sense
// to perform this test after a call to handle_removal()...
//
void
res_curval_miter::test_for_refs_to(value_node* theNodeToTest) {
	typename Tthreadslist::iterator	threads(active_threads);
	Tthread*			T;

	while((T = threads())) {
		if(T->payload->getNode() == theNodeToTest) {
			Cstring errs;

			errs << "test_for_refs_to(" << theNodeToTest->get_key() << "): found a ref in list.";
			throw(eval_error(errs));
		}
	}
}

void
res_curval_miter::clear() {
	typename threadptrslist::iterator	iter(pointers_to_threads);
	threadptr*				v;

	//
	// for each iterClass object (usually an iterator or a class derived from an iterator),
	//
	while((v = iter())) {

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
		((baseListClass*)v->payload->payload->L)->unregister_miterator(*this);

		//
		// delete the iterClass object
		//
		delete v->payload->payload;

		//
		// delete the Tthread pointing to the iterClass object
		//
		delete v->payload;
	}

	pointers_to_threads.clear();
	threads_by_res.clear();
	thread_names.clear();
}

res_curval_miter::res_curval_miter(
			const char*		P,
			const model_res_miter&	mit,
			value_node* (*initFunc)(const slist<prio_time, value_node, Rsource*>&,
				const CTime_base&),
			const CTime_base& initTime
			)
	: active_threads(true),
		name(P),
		initializationFunction(initFunc),
		initializationTime(initTime),
		thread_names(mit.thread_names) {
    typename model_res_miter::threadptrslist::iterator	l(mit.pointers_to_threads);

    //
    // Reminder:
    //
    // typedef Cnode0<relptr<iterClass>, iterClass*>	Tthread;
    // typedef Cnode0<alpha_void, Tthread*>		threadptr;
    //
    model_res_miter::threadptr*		tptr;

    Tthread*				tt;

    while((tptr = l())) {

	//
	// This iterator points to the same list element as the
	// original model_res_miter's current pointer for this thread.
	// Passing the payload works because leading_iter is derived
	// from curval_iter, which is what the iterClass constructor
	// expects.
	//
	model_res_miter::leadingClass* model_ic = tptr->payload->payload;
	iterClass*	ic = new iterClass(model_ic->L);
	ic->iterNode = model_ic->iterNode;
	ic->prevNode = model_ic->safeNode;

	//
	// NOTE: casting v->payload->payload is unpleasant, but I don't
	// know of any way to let the template know the precise relationship
	// between iterClass and listClass... maybe the template parameters
	// could be structured differently?
	//
	listClass*	lc = (listClass*) ic->L;

	//
	// The trick here is to get the correct key priority. Priority
	// is stored in each node and also in the list, I believe...
	//
	ic->go_to(initializationFunction(*lc, initializationTime));

	lc->register_miterator(*this, ic, true);
	tt = new Tthread(relptr<iterClass>(ic, tptr->payload->Key.get_priority()), ic);
	pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
	threads_by_res << new threadptr(tt->payload->L->Owner, tt);
	if(tptr->payload->list == &mit.dead_threads) {
		dead_threads << tt;
	} else {
		active_threads << tt;
	}
    }
}

//
// This method is a combination of two things:
//
// 	- window_finder::initializer for quickly locating resource
// 	  history nodes close to the given start time
//
// 	- first() for initializing the internal structures so they
// 	  ready for use with code that expects next() to return the
// 	  earliest node with a time tag >= current time.
//
// It is assumed that the required lists have already been added to
// the miterator.
//
void res_curval_miter::prepare_for_use_at(
		value_node* (*init_func)(
				const slist<
				    prio_time,
				    value_node,
				    Rsource*>&,
				const CTime_base&
			    ),
		CTime_base  init_time
		) {
    Tthread*				tt;
    threadptr*				tptr;
    typename threadptrslist::iterator	l(pointers_to_threads);

    //
    // Remember that active_threads is a clist, so all threads are in time order!
    //
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

	//
	// Reminder:
	//
	// typedef Cnode0<relptr<iterClass>, iterClass*>	Tthread;
	// typedef Cnode0<alpha_void, Tthread*>			threadptr;
	//

	iterClass*	ic = tt->payload;

	//
	// NOTE: casting v->payload->payload is unpleasant, but I don't
	// know of any way to let the template know the precise relationship
	// between iterClass and listClass... maybe the template parameters
	// could be structured differently?
	//
	listClass*	lc = (listClass*) ic->L;

	//
	// Similar to first(), which uses ic->go_to_beginning() instead:
	//
	ic->go_to(init_func(*lc, init_time));

	if(!tt->payload->is_empty()) {
		active_threads << tt;
	} else {
		dead_threads << tt;
	}
    }
}

void
res_curval_miter::first() {
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

value_node*
res_curval_miter::peek() {
	Tthread*	tt = active_threads.first_node();

	if(	APcloptions::theCmdLineOptions().TOLdebug
		&& tt
		&& tt->payload->getNode()
		&& tt->payload->getNode()->list
		&& tt->payload->getNode()->list->Owner->name ==
			APcloptions::theCmdLineOptions().theResourceToDebug) {
		Cstring str;

		str << "curval iter(peek): "
			<< tt->payload->getNode()->list->Owner->name
			<< " at " << tt->payload->getNode()->Key.getetime().to_string()
			<< ", val " << tt->payload->getNode()->get_value().to_string()
			<< " returned by iterator.\n";
		tol_debug_stream() << str;
	}
	
	return tt ? tt->payload->getNode() : NULL;
}

value_node*
res_curval_miter::current(void* res) {
    threadptr* N = threads_by_res.find(res);
    ASSERT(N);
    Tthread*	tt = N->payload;
    ASSERT(tt);

    //
    // We want the node suitable for computing the current
    // value of a resource
    //
    if(tt->payload->getPrev()) {

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

	return tt->payload->getPrev();
    } else {

	//
	// The iterator may be sitting at the beginning of history:
	//
	return tt->payload->getNode();
    }
}

slist<prio_time, value_node, Rsource*>::curval_iter*
res_curval_miter::peek_at_iter() {
	Tthread*	tt = active_threads.first_node();
	return tt->payload;
}

value_node*
res_curval_miter::next() {
	Tthread*	tt = active_threads.first_node();
	Tthread*	new_current_node;
	value_node*	is_there_a_future = NULL;
	value_node*	to_return = NULL;

	if(!tt) {
		first();
		return NULL;
	}
	to_return = tt->payload->getNode();
	ASSERT(tt->Key.mC == tt->payload);
	active_threads.remove_node(tt);
	ASSERT(!tt->list);

	is_there_a_future = tt->payload->next();
	if(is_there_a_future) {
		active_threads << tt;
	} else {
		tt->payload->prevNode = tt->payload->L->last_node();
		dead_threads << tt;
	}

	if(	APcloptions::theCmdLineOptions().TOLdebug
		&& tt
		&& tt->payload->getNode()
		&& tt->payload->getNode()->list
		&& tt->payload->getNode()->list->Owner->name ==
			APcloptions::theCmdLineOptions().theResourceToDebug) {
		Cstring str;

		str << "curval iter(next): "
			<< to_return->list->Owner->name
			<< " at " << to_return->Key.getetime().to_string()
			<< ", val " << to_return->get_value().to_string()
			<< " returned by iterator.\n";
		tol_debug_stream() << str;
	}
	return to_return;
}

void
res_curval_miter::add_thread(
		slist<prio_time, value_node, Rsource*>& lc,
		const Cstring& thread_name,
		unsigned long prio,
		bool store) {
	iterClass*	ic = new iterClass(lc);
	Tthread*	new_thread = new Tthread(relptr<iterClass>(ic, prio), ic);


	thread_names << new Cnode0<alpha_void, Cstring>(
		(void*)&lc, thread_name);

	//
	// If this is a regular modeling pass, this is OK. However, if this is
	// a 2nd scheduling pass, then we need to make sure that ic points to
	// a safe node, and not one that is about to be deleted.
	//
	if(lc.Owner->isFrozen()) {

		//
		// We'll be safe no matter what - nobody touches
		// this resources' history
		//
		value_node*	first = ic->L->first_node();
		ic->prevNode = first;
		ic->iterNode = first;
	} else if(model_control::get_pass() == model_control::SCHEDULING_2
		  && mEvent::lastEventIDbeforeScheduling != 0) {

		//
		// First let's do some basic checking. We want to make sure
		// that the first node in the history list has an event id
		// number that is 1 (the id of the first initialization node):
		//
		value_node*	first = ic->L->first_node();
		bool		first_comes_from_modeling_pass
					= first->Key.event_id == mEvent::firstEventIDbeforeScheduling;
		bool		first_comes_from_scheduling_pass
					= first->Key.event_id == mEvent::lastEventIDbeforeScheduling + 1;
		ASSERT(first_comes_from_modeling_pass || first_comes_from_scheduling_pass);
		value_node*	node_to_use = NULL;
		if(first_comes_from_modeling_pass) {

			//
			// This node is about to get destroyed when the
			// event loop advances now:
			//
			value_node*	node_to_use = first->next_node();

			//
			// We look for the node just created by the
			// initial event; shouldn't be too far down
			// the list:
			//
			while(node_to_use && node_to_use->Key.event_id <= mEvent::lastEventIDbeforeScheduling) {
				node_to_use = node_to_use->next_node();
			}
			ASSERT(node_to_use && node_to_use->Key.event_id == mEvent::lastEventIDbeforeScheduling + 1);
		} else {

			//
			// safe to use this node; it was just created by
			// the initial event and won't get destroyed while
			// the scheduling loop goes on:
			//
			node_to_use = first;
		}

		ic->iterNode = node_to_use;
		ic->prevNode = node_to_use;
	} else {

		//
		// Either we are modeling, or we are scheduling
		// incrementally with no previous pass
		//
		ic->go_to_beginning();
	}
	pointers_to_threads << new threadptr(
						new_thread->payload->get_void_list(),
						new_thread);

	//
	// Allows an efficient implementation of current():
	//
	threads_by_res << new threadptr(
						new_thread->payload->L->Owner,
						new_thread);
	lc.register_miterator(*this, ic, store);
}

void
res_curval_miter::remove_thread(const slist<prio_time, value_node, Rsource*>& lc) {
	threadptr*			old_ptr = pointers_to_threads.find(&lc);
	ASSERT(old_ptr);
	iterClass*			iter_to_delete = old_ptr->payload->payload;
	Tthread*			old_thread = old_ptr->payload;
	ASSERT(old_thread);
	Cnode0<alpha_void, Cstring>*	nameptr
		= thread_names.find(old_thread->payload->get_void_list());
	ASSERT(nameptr);

	delete nameptr;
	delete old_ptr;
	delete old_thread;
	delete iter_to_delete;
}

void
res_curval_miter::handle_addition(
		slist<prio_time, value_node, Rsource*>* theList,
		value_node* node_to_adopt) {

    //
    // Among all the lists that were 'added' to this miterator,
    // find the one that contains the node to be added
    //
    threadptr*	ptr_to_adoptive_thread = pointers_to_threads.find(theList);

    //
    // Now let's get the node that stores the information about
    // that list within either the active_threads list or the
    // dead_threads list, depending on whether they contain
    // 'future' nodes
    //
    Tthread*	adoptive_thread = ptr_to_adoptive_thread->payload;

    baseListClass::curval_iter* It = adoptive_thread->payload;
    Tthread*		 current_thread = active_threads.first_node();
    CTime_base		 adoptive_time(node_to_adopt->getKey().getetime());

    //
    // NOTE: In principle, we should check that the node to adopt occurs
    // in the future - but in reality, this is difficult to do because
    // res_curval_miter do not track "now". The latest node strictly before
    // the current active node is not necessarily the node with a time tag
    // of "now", which might reside in another res_curval_miter.
    //
    if(adoptive_thread->list == &active_threads) {
	int		compresult;
	value_node*	current_node_in_adoptive_thread = It->getNode();
	value_node*	one_before;

	//
	// We are going to compare the new node to the current one
	// in the trailing thread.
	//
	if((compresult = node_to_adopt->getKey().compare_to(
				current_node_in_adoptive_thread->getKey())) < 0) {

	    //
	    // Uh Oh - the new node is earlier than the node we thought
	    // was current. So, what do we do?
	    //
	    if((one_before = current_node_in_adoptive_thread->previous_node())) {
		if(node_to_adopt->getKey().getetime() < one_before->getKey().getetime()) {
		    Rsource* res = It->L->Owner;
		    Cstring errs;
		    errs << "handle_addition(" << res->name
			<< "): attempting to adopt a node (key = "
			<< node_to_adopt->getKey().get_key()
		        << ") earlier than the node before the current node (key = "
			<< one_before->getKey().get_key() << ").";
		    throw(eval_error(errs));
		}
	    }
	    active_threads.remove_node(adoptive_thread);

	    //
	    // No need to update the iterator's "previous node",
	    // although it may trail by a little bit
	    //
	    It->iterNode = node_to_adopt;
	    active_threads << adoptive_thread;
	} else if(!compresult) {
	    Rsource* res = It->L->Owner;
	    Cstring errs;
	    errs << "handle_addition(" << res->name
		<< "): attempting to add a node identical to the current node";
	    throw(eval_error(errs));
	}

	//
	// else, the node to adopt will come up naturally as we move on.
	// There is nothing special we should do.
	//
    } else if(adoptive_thread->list == &dead_threads) {

	//
	// NOTE: multilist calls handle_addition AFTER inserting a
	// new node; most of the time during modeling, addition occurs
	// at the end of the list
	//
	dead_threads.remove_node(adoptive_thread);
	It->iterNode = node_to_adopt;
	value_node* P = node_to_adopt->previous_node();
	if(P) {
		It->prevNode = P;
	} else {
		It->prevNode = node_to_adopt;
	}
	active_threads << adoptive_thread;
    } else if(!adoptive_thread->list) {

	//
	// the list was empty and the thread was not put anywhere
	//
	adoptive_thread->payload->go_to_beginning();
	if(!adoptive_thread->payload->is_empty()) {
		active_threads << adoptive_thread;
	} else {
		dead_threads << adoptive_thread;
	}
    }
}
