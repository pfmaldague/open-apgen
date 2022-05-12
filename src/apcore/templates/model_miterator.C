#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "Rsource.H"
#include "res_multiterator.H"
#include "EventLoop.H"
#include "ActivityInstance.H"
#include "ACT_exec.H"
#include "action_request.H"

// #define ASSERT( x )
#define ASSERT( x ) assert(x)

// #define DEBUG_THIS

#ifdef DEBUG_THIS
extern coutx excl;
#endif /* DEBUG_THIS */

void
model_res_miter::clear() {
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
	thread_names.clear();
	thread_indices.clear();
}

//
// Calls to this method originate in slist::remove_node, which invokes
// slist::notify_iterators_about_removing before actually removing a node from
// itself. The notify--- method is only a stub in slist, but it is overridden
// in the multilist class template which is the base class for resource
// histories.
//
// The purpose of notification is to provide multi-iterators (such as
// this one) with an opportunity to adjust their internal data as a result
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
// When the modeling loop removes a node (usually to delete it),
// the removed node has a time tag that is at least equal to the modeling time
// just prior to advancing it. The only case where future nodes are deleted
// are pairs of nodes involved in a non-consumable resource.
//
void
model_res_miter::handle_removal(
		slist<prio_time, value_node, Rsource*>* theList,
		value_node* theNodeToBeDeleted) {
	threadptr*	ptr_to_concerned_thread = pointers_to_threads.find(theList);
	Tthread*	concerned_thread = ptr_to_concerned_thread->payload;
	leadingClass*	the_iterator = concerned_thread->payload;
	value_node*	the_safe_one = the_iterator->getPrev();

	//
	// Check our assumptions
	//
	bool	deleted_node_has_old_id = theNodeToBeDeleted->Key.event_id <= 
			mEvent::lastEventIDbeforeScheduling;
	bool	safe_node_has_new_id = the_safe_one->Key.event_id >
			mEvent::lastEventIDbeforeScheduling;

	ASSERT(deleted_node_has_old_id);
	ASSERT(safe_node_has_new_id);

	if(concerned_thread->list == &active_threads) {
	    if(theNodeToBeDeleted == the_iterator->getNode()) {
		int relative_pos = theNodeToBeDeleted->Key.compare_to(
			the_safe_one->Key);

		active_threads.remove_node(concerned_thread);

		switch(relative_pos) {
		    case 1:
			//
			// normal case; the safe node sits before the node
			// to be deleted. We will leave the safe node
			// untouched. Don't call next()!!
			//
			// We update iterNode to point at the next node,
			// whatever that is (or isn't)
			//
			the_iterator->iterNode = the_iterator->iterNode->next_node();
			break;
		    case 0:
			//
			// not so normal case: the safe
			// node coincides with the current
			// node.
			//
			// This should never happen!
			//
			assert(false);
			break;
		    case -1:
		    default:
			//
			// should never happen
			//
			assert(false);
		}
		if(!concerned_thread->payload->is_empty()) {
		    active_threads << concerned_thread;
		} else {
		    dead_threads << concerned_thread;
		}
		return;
	    }
	    if(theNodeToBeDeleted == the_safe_one) {

		//
		// This should never happen
		//
		assert(false);

	    }
	} else {

	    //
	    // The iterator's previous node may not be NULL!
	    //
	    if(theNodeToBeDeleted == the_safe_one) {
		value_node* try_this_one = theNodeToBeDeleted->previous_node();
		if(try_this_one) {
		    concerned_thread->payload->safeNode = try_this_one;
		} else {

		    //
		    // Not much we can do...
		    //
		    concerned_thread->payload->safeNode = NULL;
		}
	    }
	}
}

model_res_miter::model_res_miter(
			const model_res_miter&	mit)
		: name(mit.name),
			thread_names(mit.thread_names) {
    typename threadptrslist::iterator	l(mit.pointers_to_threads);
    threadptr*				tptr;
    Tthread*				tt;

    while((tptr = l())) {
	leadingClass*	ic = new leadingClass(*tptr->payload->payload);

	//
	// NOTE: casting v->payload->payload is unpleasant, but I don't
	// know of any way to let the template know the precise relationship
	// between leadingClass and listClass... maybe the template parameters
	// could be structured differently?
	//
	// Other note: strictly speaking, we don't need to worry about
	// node addition/deletion since none will be added. But the
	// infrastructure expects us to register, so we have do.
	//
	((listClass*) ic->L)->register_miterator(*this, ic);

	tt = new Tthread(relptr<leadingClass>(ic, tptr->payload->Key.get_priority()), ic);
	pointers_to_threads << new threadptr(tt->payload->get_void_list(), tt);
	if(tptr->payload->list == &mit.dead_threads) {
		dead_threads << tt;
	} else {
		active_threads << tt;
	}
    }
}

void
model_res_miter::first() {
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
		initialize_iterator(*tt->payload);
		if(!tt->payload->is_empty()) {
			active_threads << tt;
		} else {
			dead_threads << tt;
		}
	}
}

value_node*
model_res_miter::peek() {
	Tthread*	tt = active_threads.first_node();
	return tt ? tt->payload->getNode() : NULL;
}

slist<prio_time, value_node, Rsource*>::leading_iter*
model_res_miter::peek_at_iter() {
	Tthread*	tt = active_threads.first_node();
	return tt->payload;
}

//
// The use case for this method is not firmly estabished.
// It needs to be. Apparently, this method is extremely
// costly during modeling, not scheduling... strange.
//
value_node*
model_res_miter::next() {
	Tthread*	tt = active_threads.first_node();
	value_node*	to_return = NULL;

	if(!tt) {
		first();
		return NULL;
	}
	leadingClass*	ic = tt->payload;
	to_return = ic->getNode();
	active_threads.remove_node(tt);

	if(ic->L->Owner->isFrozen()) {

	    //
	    // to_return is safe
	    //
	    ic->safeNode = to_return;
	} else if(model_control::get_pass() != model_control::SCHEDULING_2
		  || to_return->Key.event_id > mEvent::lastEventIDbeforeScheduling) {

	    //
	    // to_return is safe
	    //
	    ic->safeNode = to_return;
	} else {

	    //
	    // We look for a safe node somewhere between its current
	    // position and to_return.
	    //
	    value_node* safe_node = ic->safeNode;
	    safe_node = safe_node->next_node();
	    while(safe_node) {
		if(safe_node == to_return) {

		    //
		    // We know to_return is not safe
		    //
		    safe_node = NULL;
		    break;
		}
		if(model_control::get_pass() != model_control::SCHEDULING_2
		   || safe_node->Key.event_id > mEvent::lastEventIDbeforeScheduling) {

		    //
		    // OK, we can move here
		    //
		    ic->safeNode = safe_node;
		    break;
		}

		//
		// Keep going
		//
		safe_node = safe_node->next_node();
	    }

	    //
	    // If we get here because safe_node = NULL and the while
	    // condition fails, we don't want to touch ic->safeNode;
	    // it's too close to where the modeling thread is actively
	    // changing things. We will only advance ic->safeNode
	    // when we get to a safe place; otherwise, it will end
	    // up being the latest safe node in the list, and there
	    // is nothing wrong with that.
	    //
	}

	ic->iterNode = ic->iterNode->next_node();
	if(ic->iterNode) {
	    active_threads << tt;
	} else {

	    //
	    // We don't want to do this; we already updated the safe
	    // node to the extent that it was possible
	    //
	    // tt->payload->safeNode = tt->payload->L->last_node();

	    dead_threads << tt;
	}

	return to_return;
}

void model_res_miter::initialize_iterator(
		slist<prio_time, value_node, Rsource*>::leading_iter& ic) {

	value_node*	first = ic.L->first_node();

	//
	// If this is a regular modeling pass, this is OK. However, if this is
	// a 2nd scheduling pass, then we need to make sure that ic points to
	// a safe node, and not one that is about to be deleted.
	//
	if(ic.L->Owner->isFrozen()) {

	    //
	    // We'll be safe no matter what - nobody is going
	    // to touch the history of this resource
	    //
	    ic.iterNode = first;
	    ic.safeNode = first;
	} else if(model_control::get_pass() == model_control::SCHEDULING_2
		  && mEvent::lastEventIDbeforeScheduling != 0) {

	    //
	    // First let's do some basic checking. We want to make sure
	    // that the first node in the history list has an event id
	    // number that is 1 (the id of the first initialization node):
	    //

	    bool	first_comes_from_modeling_pass
				= first->Key.event_id == mEvent::firstEventIDbeforeScheduling;

	    bool	first_comes_from_scheduling_pass
				= first->Key.event_id == mEvent::lastEventIDbeforeScheduling + 1;

	    ASSERT(first_comes_from_modeling_pass || first_comes_from_scheduling_pass);

	    value_node*	node_to_use = NULL;

	    if(first_comes_from_modeling_pass) {

		//
		// This node is about to get destroyed when the
		// event loop advances now:
		//
		node_to_use = first->next_node();

		//
		// We look for the node just created by the
		// initial event; it shouldn't be too far down
		// the list:
		//
		while(node_to_use && node_to_use->Key.event_id
			<= mEvent::lastEventIDbeforeScheduling) {
				node_to_use = node_to_use->next_node();
		}
	    } else {

		//
		// The first node comes from the new scheduling pass,
		// it's not about to be deleted and is therefore
		// safe to use.
		//
		node_to_use = first;
	    }
	    ASSERT(node_to_use && node_to_use->Key.event_id
		== mEvent::lastEventIDbeforeScheduling + 1);

	    ic.iterNode = node_to_use;
	    ic.safeNode = node_to_use;
	} else {

	    //
	    // Either we are modeling or we are scheduling incrementally
	    // with no previous pass
	    //
	    ic.iterNode = first;
	    ic.safeNode = first;
	}
}

void model_res_miter::add_thread(
		slist<prio_time, value_node, Rsource*>& lc,
		const Cstring& thread_name,
		unsigned long prio,
		bool store) {
	leadingClass*	ic = new leadingClass(lc);
	Tthread*	new_thread = new Tthread(relptr<leadingClass>(ic, prio), ic);


	thread_names << new Cntnr<alpha_void, Cstring>(
		new_thread->payload->get_void_list(), thread_name);

	initialize_iterator(*ic);

	thread_indices << new Cntnr<alpha_void, int>(
					(void*)&lc,
					pointers_to_threads.get_length());
	pointers_to_threads << new threadptr((void*)&lc, new_thread);
	lc.register_miterator(*this, ic, store);
}

//
// Returns safe nodes for ALL resources
//
safe_vector_info* model_res_miter::save_safe_nodes() {

	slist<alpha_void, Cntnr<alpha_void, Tthread*> >::iterator
						iter(pointers_to_threads);
	Cntnr<alpha_void, Tthread*>*	ptr;
	vector<value_node*>*		vec = new vector<value_node*>;
	slist<
	  alpha_time,
	  time_actptr,
	  ActivityInstance*>::iterator* enditer = new slist<
							  alpha_time,
							  time_actptr,
							  ActivityInstance*>::iterator(
							ACT_exec::ACT_subsystem().ActivityEnds);
	thread_intfc*			model_thread
		= thread_intfc::threads()[thread_intfc::MODEL_THREAD].get();
	CTime_base	     		time_of_capture
		= model_thread->SafeTime.load();

	//
	// We need to set up enditer so that its iterNode points
	// to the latest node in ActivityEnds with a time tag
	// strictly less that time_of_capture.
	//
	// If that is not possible, then we can set iterNode to
	// NULL since no activity has ended yet, as of the time
	// of this safe node capture. Therefore the (XML)TOL
	// threads don't need to track activity ends (yet).
	//
	alpha_time			alpha(time_of_capture);

	//
	// Investigate special cases:
	//	- alpha is the time tag of the first end node, or
	//	  even before that: returned node is NULL. That's OK.
	//
	//	- alpha is the very last time that was modeled. You
	//	  will never get the last node in the list if that's
	//	  its time tag. That's OK; see end processing.
	//
	time_actptr*			endptr
		= ACT_exec::ACT_subsystem().ActivityEnds.find_before(alpha);

	if(!endptr) {
	    endptr = ACT_exec::ACT_subsystem().ActivityEnds.first_node();
	}

	if(model_thread->Done.load()) {
		endptr = NULL;
	}

	//
	// could be NULL; it's OK, the (XML)TOL thread will check
	//
	enditer->iterNode = endptr;

	//
	// Reminder:
	//
	//  typedef slist<prio_time, value_node, Rsource*>	baseListClass;
	//  typedef baseListClass::leading_iter			leadingClass;
	//  typedef Cntnr<relptr<leadingClass>, leadingClass*>	Tthread;
	// 
	while((ptr = iter())) {

		//
		// Notes:
		//
		//	- ptr->payload is a pointer to a Tthread
		//
		//	- ptr->payload->payload is a pointer to an
		//	  iterator over a resource history
		//
		// The safeNode of such an iterator is strictly before
		// the SafeTime at the time this method is called
		//
		(*vec).push_back(ptr->payload->payload->safeNode);
	}
	safe_vector_info* info = new safe_vector_info(vec, enditer, time_of_capture);

	{
	    lock_guard<mutex>	lock(model_thread->safe_vector_protector);
	    model_thread->copy_of_safe_vector.reset(new safe_vector_info(*info));
	}

	//
	// client will manage:
	//
	return info;
}

void
model_res_miter::remove_thread(const slist<prio_time, value_node, Rsource*>& lc) {
	Cntnr<alpha_void, Tthread*>*	old_ptr = pointers_to_threads.find(&lc);
	leadingClass*			iter_to_delete = old_ptr->payload->payload;
	Tthread*			old_thread = old_ptr->payload;
	Cntnr<alpha_void, Cstring>*	nameptr
		= thread_names.find(old_thread->payload->get_void_list());

	delete nameptr;
	delete old_ptr;
	delete old_thread;
	delete iter_to_delete;
}

void
model_res_miter::handle_addition(
		slist<prio_time, value_node, Rsource*>* theList,
		value_node* node_to_adopt) {
    threadptr*	ptr_to_adoptive_thread
				= pointers_to_threads.find(theList);
    Tthread*	adoptive_thread = ptr_to_adoptive_thread->payload;

    baseListClass::leading_iter* It = adoptive_thread->payload;
    Tthread*		 current_thread = active_threads.first_node();
    CTime_base		 adoptive_time(node_to_adopt->getKey().getetime());

    //
    // NOTE: In principle, we should check that the node to adopt occurs
    // in the future - but in reality, this is difficult to do because
    // model_res_miter do not track "now". The latest node strictly before
    // the current active node is not necessarily the node with a time tag
    // of "now", which might reside in another model_res_miter.
    //
    if(adoptive_thread->list == &active_threads) {
	int		compresult;
	value_node*	current_node_in_adoptive_thread = It->getNode();
	value_node*	one_before;
	if((compresult = node_to_adopt->getKey().compare_to(
				current_node_in_adoptive_thread->getKey())) < 0) {
		if((one_before = current_node_in_adoptive_thread->previous_node())) {

			//
			// Make sure the node to adopt comes at the same time as
			// or after the last node that was processed
			//
			if(node_to_adopt->getKey().getetime() < one_before->getKey().getetime()) {
				Cstring msg;
				msg << "handle_addition: attempting to adopt a node (key = "
					<< node_to_adopt->getKey().get_key();

				msg << ") earlier than the node before the current node (key = "
					<< one_before->getKey().get_key() << ").";
				throw(eval_error(msg));
			}
		}

		//
		// Adjust the current iterNode; it was pointing
		// to a node after the one being adopted
		//
		active_threads.remove_node(adoptive_thread);
		It->iterNode = node_to_adopt;

		//
		// No need to update the iterator's safe node,
		// although it may trail by a little bit
		//
		ASSERT(It->safeNode);

		active_threads << adoptive_thread;
	} else if(!compresult) {
		throw(eval_error("handle_addition: trying to add a node identical to the current node"));
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
	value_node* safe_node = It->safeNode;

	//
	// We don't do anything for the safe node; it should
	// be valid. Let's make sure:
	//
	ASSERT(It->safeNode);
	active_threads << adoptive_thread;
    } else if(!adoptive_thread->list) {

	//
	// should never happen
	//
	assert(false);
    }
}
