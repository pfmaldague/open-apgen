#ifndef _MULTI_LIST_H_
#define _MULTI_LIST_H_

#include <templates.H>

//
// A tlist that keeps pointers to Miterators that reference
// them. Whenever a multilist Moo is added to a Miterator
// IterFoo through a call to add_thread, like this:
//
//	IterFoo.add_thread(Moo, ...),
//
// Moo adds a pointer to IterFoo in its list of referencing
// multiterators. Note that this pointer will be automatically
// deleted if IterFoo is destroyed. It also gets destroyed if
// Moo is destroyed.
//
// The rationale for introducing multilists is that they make
// it possible to automatically update miterators that
// reference them when a node is added to or deleted from the
// multilist. This capability is essential in building
// multiterators that are robust with respect to concurrent
// updates, i. e., updates that occur while iterating.
//
// UPDATE
// ======
//
// Until now, a resource history was a multilist, i. e., a
// tlist with extra notification capabilities as explained 
// above. However, the need for a better implementation of
// state resource leads to history objects that are more
// complicated than a single list. Basically, the history
// of a state resource is the union of a set/reset list,
// which captures 'set' and 'reset' modeling events, and a
// number of two-element lists, one for each active 'use'
// modeling event. This is necessary for efficiently computing
// state history values, as required for multi-threaded
// creation of TOLs.
//
template <class keyClass, class nbase, class ownerData = short>
class multilist: public tlist<keyClass, nbase, ownerData> {
public:
	typedef slist<keyClass, nbase, ownerData>		iterlist;
	typedef Miterator_base<iterlist, nbase>			miterclass;
	typedef Cnode0<alpha_void, pair<miterclass*, bool> >	miterptr;
	typedef tlist<alpha_void, miterptr>			mitertlist;
	typedef slist<alpha_void, miterptr>			miterslist;

	multilist(const ownerData& y, bool x = false)
		: tlist<keyClass, nbase, ownerData>(y, x) {}
	multilist(bool x = false)
		: tlist<keyClass, nbase, ownerData>(x) {}
	multilist(const multilist& B)
		: tlist<keyClass, nbase, ownerData>(B) {}

	~multilist() {

		//
		// Do this first; model_res_miter expects that the only
		// deletions are those that delete old nodes. Deleting
		// a node created during the second scheduling pass would
		// cause an assert() to fail.
		//
		clear();
	}

	virtual void		clear() override;

	//
	// register_miterator() is invoked by a miterator when it
	// is being to add a new list to the set of lists it is
	// monitoring. The purpose of registration is to provide
	// the list (this) with means to notify all the miterators
	// that reference it when a new node is added to the list
	// or when an existing node is deleted.
	//
	// Notifications are sent by means of the two "notify"
	// methods discussed below.
	//
	virtual void		register_miterator(
					miterclass&,
					typename iterlist::iterator* = NULL,
					bool store = false) override;

	//
	// Called by a previously registered miterator or by this,
	// whichever disappears first.
	//
	virtual void		unregister_miterator(
					miterclass&) override;
	virtual void		unregister_miterators() override;

	//
	// Send the notifications discussed above.
	//
	virtual void		notify_iterators_about_adding(nbase*) override;
	virtual void		notify_iterators_about_removing(nbase*) override;
	virtual int		iterator_count() const override;
	virtual void		test_iterators_for_refs_to(nbase*) override;

	mitertlist		miter_ptrs;
	mitertlist		iterators;
};

template <class keyClass, class nbase, class ownerData>
void
multilist<keyClass, nbase, ownerData>::clear() {
	nbase*	N;
	slist<alpha_void, Cnode0<alpha_void, nbase*> > L;
	Cnode0<alpha_void, nbase*>*		      p;

	//
	// No - you don't want to do that; deleting an iterator should
	// unregister it; so should deleting the list - nothing else should.
	// See ~multilist() above.
	//
	// Well, yes, you do want to do this, because the dynamics of
	// iterators is different while modeling and after modeling.
	// After modeling, the feedback mechanisms are no longer needed.
	//
	unregister_miterators();

	//
	// tlist<keyClass, nbase, ownerData>::check();
	//

	while((N = slist<keyClass, nbase, ownerData>::first_node())) {

		tlist<keyClass, nbase, ownerData>::remove_node(N);
		L << new Cnode0<alpha_void, nbase*>(N, N);
	}
	while((p = L.first_node())) {
		delete p->payload;
		delete p;
	}
}

template <class keyClass, class nbase, class ownerData>
void
multilist<keyClass, nbase, ownerData>::unregister_miterators() {
  	typename miterslist::iterator	it(iterators);
  	miterptr*	N;

	while((N = it())) {
		if(N->payload.second) {
			slist<	keyClass,
				nbase,
				ownerData>::Owner->pop_miterator_data(
							N->payload.first);
		}
		N->payload.first->remove_thread(*this);
	}
	iterators.clear();
}

template <class keyClass, class nbase, class ownerData>
void
multilist<keyClass, nbase, ownerData>::test_iterators_for_refs_to(
						nbase* theNodeToTest) {
  	typename miterslist::iterator	it(iterators);
  	miterptr*			N;

	while((N = it())) {
		N->payload.first->test_for_refs_to(theNodeToTest);
	}
}

template <class keyClass, class nbase, class ownerData>
int
multilist<keyClass, nbase, ownerData>::iterator_count() const {
	return iterators.get_length();
}

template <class keyClass, class nbase, class ownerData>
void
multilist<keyClass, nbase, ownerData>::register_miterator(
		multilist<	keyClass,
				nbase,
				ownerData>::miterclass&		Mit,
		typename slist<	keyClass,
				nbase,
				ownerData>::iterator*		iter,
		bool						store) {
	iterators << new miterptr(&Mit, pair<miterclass*, bool>(
							&Mit, store));
	if(store) {
		slist<	keyClass,
			nbase,
			ownerData>::Owner->push_miterator_data(
							&Mit, iter);
	}
}

template <class keyClass, class nbase, class ownerData>
void
multilist<keyClass, nbase, ownerData>::unregister_miterator(
		multilist<keyClass, nbase, ownerData>::miterclass& Mit) {
	miterptr*	N = iterators.find(&Mit);
	assert(N);
	if(N->payload.second) {
		slist<keyClass, nbase, ownerData>::Owner->pop_miterator_data(N->payload.first);
	}
	delete N;
}

template <class keyClass, class nbase, class ownerData>
void
multilist<keyClass, nbase, ownerData>::notify_iterators_about_adding(nbase* node) {
  	typename miterslist::iterator	it(iterators);
  	miterptr*			N;

	while((N = it())) {
		miterclass*	iter = N->payload.first;

		//
		// We don't want to get into the private members of the
		// Miterators. Therefore, we will delegate to each Miterator
		// the task of updating itself following the addition of a new
		// node.
		//
		try {
			iter->handle_addition(this, node);
		}
		catch(eval_error Err) {
			throw(Err);
		}
	}
}

template <class keyClass, class nbase, class ownerData>
void
multilist<keyClass, nbase, ownerData>::notify_iterators_about_removing(nbase* node) {
  	typename miterslist::iterator	it(iterators);
  	miterptr*			N;

	while((N = it())) {
		miterclass*	iter = N->payload.first;

		//
		// We don't want to get into the private members of the
		// Miterators. Therefore, we will delegate to each Miterator
		// the task of updating itself following the addition of a new
		// node.
		//
		iter->handle_removal(this, node);
	}
}

#endif /* _MULTI_LIST_H_ */
