#ifndef _SLIST_H_
#define _SLIST_H_


//
// Implementation header for the slist<> and related templates
//

#include <assert.h>
#include <sstream>
#include <vector>
#include <unistd.h>

#include "treeDir.H"
#include "C_string.H"
#include "RES_exceptions.H"
#include "Strategy.H"
#include "UTL_time_base.H"

#include "Miterator.H"

template <class keyClass, class nbase, class ownerData = short>
class slist {
public:

private:
  nbase*	head;
  nbase*	tail;
  long		length;

public:
  ownerData	Owner;

  slist();
  slist(const ownerData&);
  slist(const slist &);
  virtual ~slist();

  class iterator {
    public:

    //
    // DATA
    //
    const slist*	L;
    nbase*		iterNode;

    //
    // CONSTRUCTORS
    //
    iterator() = delete;
    iterator(const slist& l) {
	L = &l;
	iterNode = NULL;
    }
    iterator(const slist* l) {
	L = l;
	iterNode = NULL;
    }
    iterator(const iterator& li) {
	L = li.L;
	iterNode = li.iterNode;
    }
    virtual ~iterator() {}

    //
    // METHODS
    //
    virtual nbase*		getNode() const {
	return iterNode;
    }
    virtual nbase*		getPrev() const {
	return NULL;
    }
    virtual bool		is_empty() const {
	return iterNode == NULL;
    }
    virtual const void*	get_void_list() const {
	return (void*)L;
    }
    virtual iterator& operator=(const iterator& li) {
	L = li.L;
	iterNode = li.iterNode;
	return *this;
    }
    virtual nbase*	operator()() {
	return next();
    }
    virtual nbase*	const_first() const {
	return L->first_node();
    }

    //
    // this method is trivial for ordinary iterators; it isn't for
    // derived classes such as flexIterators.
    //
    virtual nbase*	first() {
	go_to_beginning();
	return iterNode;
    }
    virtual void	go_to_beginning() {
	iterNode = L->first_node();
    }
    virtual void	go_to_end() {
	iterNode = L->last_node();
    }
    virtual void	go_to(nbase* N) {
	iterNode = N;
    }
    virtual nbase*	last() {
	go_to_end();
	return iterNode;
    }
    virtual nbase*	next() {
	if(iterNode) {
	    if(!iterNode->next_node()) {
		return (iterNode = NULL);
	    } else {
	    	return (iterNode = iterNode->next_node());
	    }
	} else {

	    //
	    // sets iterNode to the first node of the list:
	    //
	    first();
	}
	return iterNode;
    }

    //
    // Provides the caller with a glimpse of what next()
    // will return, but does not change the content of
    // the iterator:
    //
    virtual nbase*	peek() const {
	return iterNode ?
	    iterNode->next_node()
	    : NULL;
    }
    virtual nbase*	previous() {
	if(iterNode) {
	    iterNode = iterNode->previous_node();
	} else {
	    iterNode = L->last_node();
	}
	return iterNode;
    }
    virtual void	undefine() {
	iterNode = NULL;
    }
  }; // end of iterator subclass

//
// 		INVARIANTS for curval_iter
// 		--------------------------
//
//	See the README.md document in the src directory.
//
  class curval_iter: public iterator {
    public:
    //
    // DATA
    //
    nbase*		prevNode;

    //
    // CONSTRUCTORS
    //
    curval_iter() = delete;
    curval_iter(const slist& l)
    	    : iterator(l) {
	prevNode = NULL;
    }
    curval_iter(const slist* l)
    	    : iterator(l) {
	prevNode = NULL;
    }
    curval_iter(const curval_iter& li)
    	    : iterator(li) {
	prevNode = li.prevNode;
    }
    virtual		~curval_iter() {}

    //
    // METHODS
    //
    virtual nbase*	getPrev() const override {
	return prevNode;
    }
    virtual curval_iter& operator=(const curval_iter& li) {
	iterator::L = li.L;
	iterator::iterNode = li.iterNode;
	prevNode = li.prevNode;
	return *this;
    }
    virtual void	go_to_beginning() override {
	iterator::iterNode = iterator::L->first_node();

	//
	// Not totally logical, but serves the purpose of resource
	// histories
	//
	prevNode = iterator::iterNode;
    }
    virtual void	go_to_end() override {
	iterator::iterNode = iterator::L->last_node();
	if(iterator::iterNode) {
		prevNode = iterator::iterNode->previous_node();
	} else {
		prevNode = NULL;
	}
    }
    virtual void	go_to(nbase* N) override {
	iterator::iterNode = N;
	if(N) {
		prevNode = iterator::iterNode->previous_node();
	} else {
		prevNode = NULL;
	}
    }
    virtual nbase*	next() override {
	if(iterator::iterNode) {
	    if(!iterator::iterNode->next_node()) {

		prevNode = iterator::iterNode;
		iterator::iterNode = NULL;
		return prevNode;
	    }

	    //
	    // Special considerations for resource history nodes
	    // =================================================
	    //
	    // When the iterator reaches the end of history, the iterNode
	    // returned will be NULL but the prevNode pointer points to
	    // the last event in the history list. This allows currentval()
	    // to work just the way it should.
	    //
	    // When potential_triggers() advances through the history list
	    // during the second scheduling pass, prevNode should always
	    // point to a "2nd generation" node, i. e., a node that is
	    // created during the scheduling pass proper, and not left
	    // over from the preceding modeling (or scheduling) pass.
	    //
	    prevNode = iterator::iterNode;
	    return (iterator::iterNode = iterator::iterNode->next_node());
	} else {
	    iterator::first();
	}
	return iterator::iterNode;
    }
    virtual nbase*	previous() override {
	if(iterator::iterNode) {
	    iterator::iterNode = iterator::iterNode->previous_node();
	} else {
	    iterator::iterNode = iterator::L->last_node();
	}
	if(iterator::iterNode) {
	    prevNode = iterator::iterNode->previous_node();
	}
	return iterator::iterNode;
    }
    virtual void		undefine() override {
	iterator::iterNode = NULL;
	prevNode = NULL;
    }
  }; // end of curval_iter subclass

//
// 		INVARIANTS for leading_iter
// 		---------------------------
//
// 	- safeNode has an event_id that is strictly
// 	  greater than mEvent::lastEventIDbeforeScheduling,
// 	  and will therefore never be deleted by the event
// 	  loop as long as it is active.
//
  class leading_iter : public iterator {
    public:

    //
    // DATA
    //


    //
    // Too slow:
    //
    // std::atomic<nbase*> safeNode;

    //
    // safeNode
    nbase*	safeNode;

    //
    // CONSTRUCTORS
    //
    leading_iter() = delete;
    leading_iter(const slist& l)
    	    : iterator(l) {
	safeNode = NULL;
    }
    leading_iter(const slist* l)
    	    : iterator(l) {
	safeNode = NULL;
    }
    leading_iter(const leading_iter& li)
   	    : iterator(li) {
	safeNode = li.safeNode;
    }
    virtual		~leading_iter() {}

    //
    // METHODS
    //
    virtual nbase*	getPrev() const override {
	return safeNode;
    }
    virtual leading_iter& operator=(const leading_iter& li) {

	//
	// We want the model_res_miter to have complete control
	//
	assert(false);
	return *this;
    }
    virtual void	go_to_beginning() override {

	//
	// We want the model_res_miter to have complete control
	//
	assert(false);
    }
    virtual void	go_to_end() override {

	//
	// We want the model_res_miter to have complete control
	//
	assert(false);
	iterator::iterNode = iterator::L->last_node();
	if(iterator::iterNode) {
		safeNode = iterator::iterNode->previous_node();
	} else {
		safeNode = NULL;
	}
    }
    virtual void	go_to(nbase* N) override {

	//
	// We want the model_res_miter to have complete control
	//
	assert(false);
    }
    virtual nbase*	last() override {

	//
	// We want the model_res_miter to have complete control
	//
	assert(false);
	return iterator::iterNode;
    }
    virtual nbase*	next() override {

	//
	// We want the model_res_miter to have complete control
	//
	assert(false);
	return iterator::iterNode;
    }
    virtual nbase*	previous() override {

	//
	// We want the model_res_miter to have complete control
	//
	assert(false);
    }
    virtual void	undefine() override {

	//
	// We want the model_res_miter to have complete control
	//
	assert(false);
    }
  }; // end of leading_iter subclass

//
// The following class requires a pointer to a leading_iter so it can
// navigate the list pointed to by the iterator. To do so, one has to
// follow what the modeling thread is doing, hence have access to the
// safeNode member of a leading_iter. The data member pointing to this
// leading_iter is called leader.
//
// Here are the invariants for this class:
//
// 			INVARIANTS
// 			----------
//
// 	- leader->safeNode has a time tag that is less than
// 	  or equal to the modeling thread's value of 'now'
//
// 	- leader->safeNode has an event_id that is strictly
// 	  greater than mEvent::lastEventIDbeforeScheduling,
// 	  and will therefore never be deleted by the event
// 	  loop as long as it is active
//
// As a result, if a node N was initialized to a safe node,
// and if the time tag of N is strictly less than 'now',
// and N is not equal to the current value of leader->safeNode,
// then it is safe to request N->next_node().
//
// The reason is that the modeling thread will only update
// leader->safeNode to nodes in the list which have no predecessor
// created by the previous modeling or scheduling pass. We know
// this, because the modeling thread only updates safeNode to
// nodes with a time tag less than or equal to now, and by the
// time nodes are being created with now having a certain value,
// old node processing has eliminated all the nodes with a time
// tag less than or equal to now. Therefore, it is safe to
// navigate the list of safe nodes as long as one keeps to the
// part of the list that is at or before leader->safeNode.
//
  class safe_iter: public curval_iter {
  public:

	safe_iter() = delete;
	safe_iter(const safe_iter&) = delete;
	safe_iter(const slist& L, nbase* I)
	    : curval_iter(L),
		safe_node(I) {
	    iterator::iterNode = I;
	    curval_iter::prevNode = I;
	}
	~safe_iter() {}

	//
	// There are no methods here; all interesting
	// methods are found in the trailing_miter class,
	// which manages safe_iter directly.
	//

	//
	// the lone data member:
	//
	nbase*	safe_node;

  }; // end of safe_iter subclass

  //
  // The following methods are provided for the benefit of lists
  // that are "under observation" by Miterators.
  //
  // The idea is that whenever the list is modified through the
  // addition or removal of nodes), it updates any Miterators
  // that are watching it so that the Miterators can update their
  // state. The purpose of these state updates is to make sure
  // that (1) no pointers are pointing to nodes that are about
  // to be deleted, and (2) if a node is inserted just before
  // the "next node" in an iterator, the iterator is modified
  // to point to the new node instead of the one it is currently
  // pointing to.
  //
  virtual void		notify_iterators_about_adding(nbase*) {}
  virtual void		notify_iterators_about_removing(nbase*) {}

  //
  // to help nodes that may be inserted in a tlist:
  //
  virtual void		make_root(nbase*) {}

  //
  // testing only
  //
  virtual void		test_iterators_for_refs_to(nbase*) {}

  //
  // The rationale for including the last two arguments is that
  // they are useful when the Miterator being registered is one
  // that should be used by a resource when asked to evaluate
  // its current value. Here is how a typical scenario plays out:
  //
  // 1. a new std::thread defines a miterator - for example,
  //    a trailing_miter, which it intends to use when evaluating
  //    the current (for that thread) value of some resources.
  //    Let's denote this miterator by M.
  //
  // 2. for each resource the std::thread is interested in, it
  //    adds M to the resource's history list, H.
  //
  // 3. H knows which resource owns it, thanks to the Owner
  //    template class argument. H is in fact a multilist,
  //	which overrides register_miterator(). In the overriding
  //	method, the resource is identified and its
  //	push_miterator_data() method is invoked, with the
  //	miterator and the iterator as arguments.
  //
  // 4. In its evaluate_present_value() method, the resource
  //    uses the miterator data as follows: the miterator
  //    provides the evaluation strategy, and the iterator
  //    provides the pointer to the history node that is
  //    the closest to the time at which currentval()
  //    needs to be evaluated.
  //
  // UPDATE
  // ======
  //
  // To enable concurrent modeling and TOL creation, it is
  // necessary to simplify the structure of state histories.
  // This means breaking up the single history list into
  // separate lists, one for set/reset events and one for
  // _each_ use event. As a result, the registration process
  // has become more complicated.
  //
  // Remember that part of the registration functionality is
  // to provide the resource that owns a list as part of its
  // history with means to evaluate its current value by
  // only querying the currently "active node" in its history.
  // The active node used to be an ordinary node belonging
  // to a well-defined list. But now, the active node belongs
  // to whatever 'use' list is in a Miterator's active threads
  // list; if there is no active 'use' list, then the active
  // node is the current node in the set/reset list, which
  // resembles the existing situation.
  //
  virtual void		register_miterator(
				Miterator_base<slist, nbase>&,
				iterator* = NULL,
				bool = false) {}
  virtual void		unregister_miterator(
				Miterator_base<slist, nbase>&) {}
  virtual void		unregister_miterators() {}
  virtual int		iterator_count() const {
	return 0;
  }

  virtual void		check() { return; }
  virtual bool		check_consumption(ostringstream& s) {
	return true;
  }
  virtual void		clear();
  bool			delete_node(nbase* aNode);
  virtual nbase*	find(const keyClass& s) const;

			//
			// In slist, this is the same as find(s); do not
			// use... the purpose is to have a virtual base for
			// the tlist version, which actually does what it says
			//
  virtual nbase*	find_at_or_before(const keyClass& s) const {
				Cstring err("Do not use slist<...>::find_at_or_before()");
				throw(eval_error(err));
			}

			/* 'after' means 'strictly after'. Key match must be
			 * exact, else return NULL. */
  virtual nbase*	find_match_after(const keyClass& s, nbase*) const;
  virtual nbase*	find_adjacent_right(const nbase*) const { return NULL; }
  virtual nbase*	find_adjacent_left(const nbase*) const { return NULL; }
  virtual nbase*	first_node() const { return head; }
  long			get_length() const { return length; }
  nbase*		insert_first_node_after_second(nbase* insert_this, nbase* after_that);
  virtual nbase*	insert_node(nbase* N);
  virtual nbase*	last_node() const { return tail; }
  slist&		operator = (const slist &);
  virtual slist&	operator << (nbase* N) { insert_node(N); return *this; }
  virtual slist&	operator << (slist& Arg);
  
  virtual nbase*	remove_node(nbase* aNode);
  virtual nbase*	remove_node_prior_to_deletion(nbase* N) {
	  return remove_node(N);
  }
};	// end of class slist

#endif /* _SLIST_H_ */
