#ifndef _TNODE_TEMPLATE_H
#define _TNODE_TEMPLATE_H


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


template <class keyClass, class Node, class ownerData>
class slist;

template <class keyClass, class Node, class ownerData>
class tlist;

template <class Tnode>
class backptr;

class safe_vector_info;

//
// NOTE on iterators and multi-list iterators ("Miterators")
// =========================================================
//
// Prior to multithreading, iterators and Miterators were separate
// classes; there was no incentive to bring them close since Use
// Cases were different for the two.
//
// However, with the growing need for creating TOLs and XMLTOLs in
// a trailing thread, it has become important to bring the two
// classes together. The main reason for this is that evaluating
// state resources efficiently requires turning the resource history
// into a multi-list wich several 'threads'. A multi-resource
// Miterator must therefore be capable of handling threads that
// are implemented as Miterators themselves, instead of ordinary
// iterators as was previously the case.
//
// As a result, the Miterator_base class, described below, has to
// fulfill all the requirements necessary so they can be added to
// another Miterator_base instance as just another thread.
//
// The key to this requirement is the Miterator::add_thread()
// method.
//


//
// A base class template for objects that iterate over multiple lists
// ordered with respect to keys of the same type. These iterators
// are robust against insertion and deletion of nodes from/to
// the lists that they monitor, thanks to "handle_deletion" and
// "handle_addition" callbacks. These callbacks are not invoked
// by the base class templates slist and tlist below, but they
// are invoked by the multilist template; see multilist.H.
//
// If you want a multi-list iterator that is robust against
// node insertion and deletion, you therefore have to use
// lists whose type is derived from multilist.
//
template <class listClass, class nodeClass>
class Miterator_base {
public:
	Miterator_base()
		: strategy(EvalTiming::NORMAL),
			ref_count(0)
			{}
	virtual ~Miterator_base() {}

  //
  // The callbacks mentioned above:
  //
  //	callback to invoke before adding theNode:
  //
  virtual void		handle_addition(
				listClass* theList,
				nodeClass* theNode) = 0;

  //
  //	callback to invoke before removing theNode:
  //
  virtual void		handle_removal(
				listClass* theList,
				nodeClass* theNode) = 0;

  //
  // Tells the Miterator to monitor an additional list.
  // The Miterator treats the lists it monitors as if
  // they had been merged into one giant list, but it
  // does not actually merge anything and is therefore
  // much more efficient than merging the lists would
  // be.
  //
  virtual void		add_thread(
				listClass&,
				const Cstring& name,
				unsigned long priority,
				bool store = false) = 0;

  //
  // Returns the number of threads that were added
  //
  virtual int		thread_count() = 0;
				

  //
  // testing only
  //
  virtual void		test_for_refs_to(nodeClass*) = 0;

  //
  // Tells the Miterator to remove one
  // of the lists it is monitoring
  //
  virtual void		remove_thread(const listClass&) = 0;

  virtual const char*	get_name() const = 0;

  //
  // Removes all the monitoring data
  //
  virtual void		clear() = 0;

  //
  // Returns the next node in the virtual merged list
  //
  virtual nodeClass*	peek() = 0;

  //
  // Returns the node currently in a list owner's
  // representative in the internal list of iterators.
  // The void* is a pointer to the Owner member of
  // a listClass instance. Because that pointer is not
  // guaranteed to be non-NULL, this method is implemented
  // trivially in simple Miterators; res_miterators
  // have a non-trivial implementation, because resource
  // histories are guaranteed to have an Owner that
  // points to a valid Rsource object.
  //
  virtual nodeClass*	current(void*) {
	return NULL;
  }

  //
  // Same as peek() but updates the internal data so
  // that the next call will return the following
  // node
  //
  virtual nodeClass*	next() = 0;

  //
  // Does nothing for trailing_miter; otherwise, puts
  // the Miterator in its initial state
  //
  virtual void		first() = 0;

  //
  // Returns a vector of nodeClass pointers that are
  // safe to use by a trailing thread. This is a placeholder
  // for all miterators, except for res_model_miter and
  // trailing_miter.
  //
  virtual safe_vector_info* save_safe_nodes() {
	return NULL;
  }

  //
  // strategy support (evaluation of resources just before
  // or just after a given time tag)
  EvalTiming::Strategy	strategy;
  

  //
  // smart_ptr support - required by the lists of threads
  // found in derived classes
  //
  int			ref_count;

  int			ref() {
	ref_count++;
	return ref_count;
  }
  int			deref() {
	ref_count--;
	return ref_count;
  }

};

// nC: nodeClass
template <class nC>
class baseLinker {
public:
  baseLinker(nC* M) : me(M) {}
  baseLinker(const baseLinker& bl) : me(bl.me) {}
  virtual ~baseLinker() {}

  nC*			me;
  virtual void		add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal) = 0;
  virtual void		add_cons_to_ancestors() = 0;
  virtual void		attach(		TreeDir::Dir right_or_left,
					nC* node_to_attach,
					TreeDir::cons_adjust CA) = 0;
  virtual void		attach_left(	nC* node_to_attach,
					TreeDir::cons_adjust CA) = 0;
  virtual void		attach_right(	nC* node_to_attach,
					TreeDir::cons_adjust CA) = 0;
  virtual double	get_consumption() const = 0;
  virtual valueHolder	get_i_cons() const = 0;
  virtual double	get_integral() const = 0;
  virtual double	get_integral_left() const = 0;
  virtual double	get_integral_right() const = 0;
  virtual true_long	get_int_consumption() const = 0;
  virtual valueHolder	get_l_cons() const = 0;
  virtual valueHolder	get_r_cons() const = 0;
  virtual nC*		get_parent() const = 0;
  virtual void		init_cons() = 0;
  virtual bool		is_float() const { return false; }
  virtual void		set_i_cons(const valueHolder* vh) = 0;
  virtual void		set_parent(nC*) = 0;
  virtual void		subtract_cons_from_ancestors(nC* stop_here = NULL) = 0;
};

template <class nC>
class simpleLinker: public baseLinker<nC> {
public:

  simpleLinker(nC* bn) : baseLinker<nC>(bn) {}
  simpleLinker(const simpleLinker& sl) : baseLinker<nC>(sl) {}
  ~simpleLinker() {}

  void			add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal) {}
  void			add_cons_to_ancestors() {}
  void			attach(		TreeDir::Dir right_or_left,
					nC* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_left(	nC* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_right(	nC* node_to_attach,
					TreeDir::cons_adjust CA);
  double		get_consumption() const { return 0.0; }
  true_long		get_int_consumption() const { return 0; }
  valueHolder		get_i_cons() const { return valueHolder(0L); }
  double		get_integral() const { return 0.0; }
  double		get_integral_left() const { return 0.0; }
  double		get_integral_right() const { return 0.0; }
  valueHolder		get_l_cons() const { return valueHolder(0L); }
  valueHolder		get_r_cons() const { return valueHolder(0L); }
  nC*			get_parent() const { return NULL; }
  void			init_cons() {}
  void			set_i_cons(const valueHolder* vh) {}
  void			set_parent(nC*) {}
  void			subtract_cons_from_ancestors(nC* stop_here = NULL) {} };

template <class bN>
void
simpleLinker<bN>::attach_left(bN* N, TreeDir::cons_adjust CA) {
	attach(TreeDir::LEFT_DIR, N, TreeDir::UPDATE_CONS); }

template <class bN>
void
simpleLinker<bN>::attach_right(bN* N, TreeDir::cons_adjust CA) {
	attach(TreeDir::RIGHT_DIR, N, TreeDir::UPDATE_CONS); }

template <class bN>
void
simpleLinker<bN>::attach(TreeDir::Dir right_or_left, bN* N, TreeDir::cons_adjust CA) {
	baseLinker<bN>::me->Links[right_or_left] = N; }


template <class bN>
class intLinker: public baseLinker<bN> {
public:
  bN*			avlParent;
  valueHolder		myCount;
  valueHolder		nCount_right;
  valueHolder		nCount_left;

  intLinker(bN* bn) :	baseLinker<bN>(bn),
			avlParent(NULL),
			myCount(1L),
			nCount_right(0L),
			nCount_left(0L) {}
  intLinker(const intLinker& il) : baseLinker<bN>(il) ,
			avlParent(NULL),
			myCount(1L),
			nCount_right(0L),
			nCount_left(0L) {}
  ~intLinker() {}

  void			add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal);
  void			add_cons_to_ancestors();
  void			attach(		TreeDir::Dir right_or_left,
					bN* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_left(	bN* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_right(	bN* node_to_attach,
					TreeDir::cons_adjust CA);
  double		get_consumption() const { return (double) get_int_consumption(); }
  true_long		get_int_consumption() const;
  valueHolder		get_i_cons() const { return myCount; }
  double		get_integral() const { return 0.0; }
  double		get_integral_left() const { return 0.0; }
  double		get_integral_right() const { return 0.0; }
  valueHolder		get_l_cons() const { return nCount_left; }
  valueHolder		get_r_cons() const { return nCount_right; }
  bN*			get_parent() const { return avlParent; }
  virtual void		init_cons() {
				avlParent = NULL;
				nCount_left = valueHolder(0L);
				nCount_right = valueHolder(0L); }
  void			set_i_cons(const valueHolder* vh) { myCount = vh ? *vh : valueHolder(0L); }
  void			set_parent(bN* b) { avlParent = b; }
  void			subtract_cons_from_ancestors(bN* stop_here = NULL); };

template <class bN>
void
intLinker<bN>::add_cons(
		TreeDir::Dir which_dir,
		const valueHolder& the_amount,
		const valueHolder& /* integralVal */) {
	if(which_dir == TreeDir::RIGHT_DIR) {
		nCount_right += the_amount; }
	else {
		nCount_left += the_amount; } }

template <class bN>
void
intLinker<bN>::add_cons_to_ancestors() {
	bN* p = baseLinker<bN>::me->get_parent();
	bN* t = baseLinker<bN>::me;

	while(p) {
		if(t == p->Links[TreeDir::LEFT_DIR]) {
			p->add_cons(TreeDir::LEFT_DIR, myCount, 0.0); }
		else {
			p->add_cons(TreeDir::RIGHT_DIR, myCount, 0.0); }
		t = p;
		p = p->get_parent(); } }

template <class bN>
void
intLinker<bN>::subtract_cons_from_ancestors(
		bN* until_this) {
	bN* p = baseLinker<bN>::me->get_parent();
	bN* t = baseLinker<bN>::me;

	while(p) {
		if(t == p->Links[TreeDir::LEFT_DIR]) {
			p->add_cons(TreeDir::LEFT_DIR, valueHolder(-myCount.get_content()), 0L); }
		else {
			p->add_cons(TreeDir::RIGHT_DIR, valueHolder(-myCount.get_content()), 0L); }
		t = p;
		p = p->get_parent();
		if(p == until_this) {
			break; } } }

template <class bN>
void
intLinker<bN>::attach_left(
		bN* N,
		TreeDir::cons_adjust CA) {
	attach(TreeDir::LEFT_DIR, N, CA); }

template <class bN>
void
intLinker<bN>::attach_right(
		bN* N,
		TreeDir::cons_adjust CA) {
	attach(TreeDir::RIGHT_DIR, N, CA); }

template <class bN>
void
intLinker<bN>::attach(
		TreeDir::Dir right_or_left,
		bN* N,
		TreeDir::cons_adjust CA) {
	bN* an = baseLinker<bN>::me;
	an->Links[right_or_left] = N;
	if(N) {
		N->set_parent(baseLinker<bN>::me);
		if(CA == TreeDir::UPDATE_CONS) {
			valueHolder	t(N->get_i_cons());
			t += N->get_l_cons();
			t += N->get_r_cons();
			if(right_or_left == TreeDir::RIGHT_DIR) {
				nCount_right = t; }
			else {
				nCount_left = t; } } }
	else if(CA == TreeDir::UPDATE_CONS) {
		if(right_or_left == TreeDir::RIGHT_DIR) {
			nCount_right = valueHolder(0L); }
		else {
			nCount_left = valueHolder(0L); } } }

template <class bN>
true_long
intLinker<bN>::get_int_consumption() const {
	true_long val = get_i_cons().get_content() + get_l_cons().get_content();
	const bN* q = baseLinker<bN>::me;
	const bN* p = avlParent;
	while(p) {
		if(q == p->get_link(TreeDir::RIGHT_DIR)) {
			val += p->get_i_cons().get_content() + p->get_l_cons().get_content(); }
		q = p;
		p = p->get_parent(); }
	return val; }

template <class bN>
class floatLinker: public baseLinker<bN> {
public:
  bN*			avlParent;
  valueHolder		myCount;
  valueHolder		nCount_right;
  valueHolder		nCount_left;
  valueHolder		integral_right, integral_left, my_integral;

  floatLinker(bN* bn) : baseLinker<bN>(bn),
			avlParent(NULL), myCount(0.0), nCount_right(0.0),
			nCount_left(0.0), my_integral(0.0),
			integral_right(0.0), integral_left(0.0) {}
  floatLinker(const floatLinker& fl) : baseLinker<bN>(fl),
			avlParent(NULL), myCount(0.0), nCount_right(0.0),
			nCount_left(0.0), my_integral(0.0),
			integral_right(0.0), integral_left(0.0) {}
  ~floatLinker() {}

  void			add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal);
  void			add_cons_to_ancestors();
  void			attach(		TreeDir::Dir right_or_left,
					bN* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_left(	bN* node_to_attach,
					TreeDir::cons_adjust CA);
  void			attach_right(	bN* node_to_attach,
					TreeDir::cons_adjust CA);
  double		get_consumption() const;
  true_long		get_int_consumption() const { return 0; }
  double		get_integral() const;
  double		get_integral_left() const { return integral_left.get_double(); }
  double		get_integral_right() const { return integral_right.get_double(); }
  true_long		get_int_consumption() { return 0; }
  valueHolder		get_i_cons() const { return myCount; }
  valueHolder		get_l_cons() const { return nCount_left; }
  valueHolder		get_r_cons() const { return nCount_right; }
  bN*			get_parent() const { return avlParent; }
  virtual void		init_cons() {
				avlParent = NULL;
				nCount_left = valueHolder(0.0);
				nCount_right = valueHolder(0.0);
				// ??
				// myCount = valueHolder(0.0);
				my_integral = 0.0;
				integral_right = 0.0;
				integral_left = 0.0; }
  bool			is_float() const { return true; }
  void			set_i_cons(const valueHolder* vh) { myCount = vh ? *vh : valueHolder(0.0); }
  void			set_parent(bN* b) { avlParent = b; }
  void			subtract_cons_from_ancestors(bN* stop_here = NULL); };

template <class bN>
void
floatLinker<bN>::add_cons(
		TreeDir::Dir which_dir,
		const valueHolder &V,
		const valueHolder &I) {
	if(which_dir == TreeDir::RIGHT_DIR) {
		integral_right += I;
		nCount_right += V; }
	else {
		integral_left += I;
		nCount_left += V; } }

template <class bN>
void
floatLinker<bN>::add_cons_to_ancestors() {
	bN*		p = baseLinker<bN>::me->get_parent();
	bN*		t = baseLinker<bN>::me;
	CTime_base	t0(baseLinker<bN>::me->getKey().getetime());
	double		myTime = t0.convert_to_double_use_with_caution();
	double		deltaT;

	while(p) {
		CTime_base	t1(p->getKey().getetime());
		deltaT = t1.convert_to_double_use_with_caution() - myTime;
		if(t == p->Links[TreeDir::LEFT_DIR]) {
			p->add_cons(TreeDir::LEFT_DIR, myCount, valueHolder(myCount.get_double() * deltaT)); }
		else {
			p->add_cons(TreeDir::RIGHT_DIR, myCount, valueHolder(myCount.get_double() * deltaT)); }
		t = p;
		p = p->get_parent(); } }

template <class bN>
double
floatLinker<bN>::get_consumption() const {
	double val = get_i_cons().get_double() + get_l_cons().get_double();
	const bN* q = baseLinker<bN>::me;
	const bN* p = avlParent;
	while(p) {
		if(q == p->Links[TreeDir::RIGHT_DIR]) {
			val += p->get_i_cons().get_double() + p->get_l_cons().get_double(); }
		q = p;
		p = p->get_parent(); }
	return val; }

template <class bN>
double
floatLinker<bN>::get_integral() const {
	double val = integral_left.get_double();
	bN* q = baseLinker<bN>::me;
	bN* p = avlParent;
	CTime_base t1 = baseLinker<bN>::me->getKey().getetime(), t2;
	double deltaT;

	while(p) {
		if(q == p->Links[TreeDir::RIGHT_DIR]) {
			t2 = p->getKey().getetime();
			deltaT = t1.convert_to_double_use_with_caution() - t2.convert_to_double_use_with_caution();
			val += deltaT * (p->get_i_cons().get_double() + p->get_l_cons().get_double())
				+ p->get_integral_left(); }
		q = p;
		p = p->get_parent(); }
	return val; }

// keyClass'd better be alpha_time...
template <class bN>
void
floatLinker<bN>::subtract_cons_from_ancestors(
		bN* until_this) {
	bN*	p = get_parent();
	bN*	t = baseLinker<bN>::me;
	CTime_base t0 = baseLinker<bN>::me->getKey().getetime();
	double	deltaT;
	double	myTime = t0.convert_to_double_use_with_caution();

	while(p) {
		CTime_base t1(p->getKey().getetime());
		deltaT = t1.convert_to_double_use_with_caution() - myTime;
		if(t == p->Links[TreeDir::LEFT_DIR]) {
			p->add_cons(TreeDir::LEFT_DIR, valueHolder(-myCount.get_double()), valueHolder(-deltaT * myCount.get_double())); }
		else {
			p->add_cons(TreeDir::RIGHT_DIR, valueHolder(-myCount.get_double()), valueHolder(-deltaT * myCount.get_double())); }
		t = p;
		p = p->get_parent();
		if(p == until_this) {
			break; } } }

template <class bN>
void
floatLinker<bN>::attach(
		TreeDir::Dir right_or_left,
		bN* af,
		TreeDir::cons_adjust CA) {
	bN* an = baseLinker<bN>::me;
	an->Links[right_or_left] = af;
	if(af) {
		af->set_parent(baseLinker<bN>::me);
		if(CA == TreeDir::UPDATE_CONS) {
			valueHolder		t(af->get_i_cons());
			CTime_base		t1 = baseLinker<bN>::me->getKey().getetime(),
						t2 = af->getKey().getetime();
			double			deltaT;
			double			u = af->get_integral_left() + af->get_integral_right();

			deltaT = t1.convert_to_double_use_with_caution() - t2.convert_to_double_use_with_caution();
			t += af->get_l_cons();
			t += af->get_r_cons();
			u += t.get_double() * deltaT;
			if(right_or_left == TreeDir::RIGHT_DIR) {
				integral_right = u;
				nCount_right = t; }
			else {
				integral_left = u;
				nCount_left = t; } } }
	else if(CA == TreeDir::UPDATE_CONS) {
		if(right_or_left == TreeDir::RIGHT_DIR) {
			integral_right = valueHolder(0.0);
			nCount_right = valueHolder(0.0); }
		else {
			integral_left = valueHolder(0.0);
			nCount_left = valueHolder(0.0); } } }

template <class bN>
void
floatLinker<bN>::attach_left(
		bN* N,
		TreeDir::cons_adjust CA) {
	attach(TreeDir::LEFT_DIR, N, CA); }

template <class bN>
void
floatLinker<bN>::attach_right(
		bN* N,
		TreeDir::cons_adjust CA) {
	attach(TreeDir::RIGHT_DIR, N, CA); }

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

template <class keyClass, class nbase, class ownerData = short>
class tlist: public slist<keyClass, nbase, ownerData> {
public:

  typedef slist<keyClass, nbase, ownerData> lbase;

  bool			enabled;
  bool			synchronize_orders;
  unsigned long		ambiguityCount;
  nbase*		root;

			tlist(bool = false);
			tlist(const ownerData&, bool = false);
			tlist(const tlist& B);
  			~tlist();

  virtual void		notify_iterators_about_adding(nbase*) override {}
  virtual void		notify_iterators_about_removing(nbase*) override {}

  virtual void		make_root(nbase* N) override {
	root = N;
  }

  virtual void		register_miterator(
				Miterator_base<slist<keyClass, nbase, ownerData>, nbase>&,
				typename lbase::iterator* = NULL,
				bool = false) override {}
  virtual void		unregister_miterator(
				Miterator_base<slist<keyClass, nbase, ownerData>, nbase>&)
					override {}
  virtual void		unregister_miterators() override {}
  virtual int		iterator_count() const override { return 0; }

  void			add_without_duplication(lbase& L);
  virtual void		check() override;
  virtual bool		check_consumption(ostringstream& s) override;
  void			check(nbase*, int depth, const keyClass* smaller, const keyClass* bigger);
  virtual nbase*	find_node(nbase*) const;
  virtual nbase*	find(const keyClass& s) const override;
  nbase*		find_after(const keyClass&) const;		// strictly after
  nbase*		find_at_or_after(const keyClass&) const;
  nbase*		find_before(const keyClass&) const;		// strictly before
  nbase*		find_at_or_before(const keyClass&) const override;
  virtual nbase*	find_adjacent_right(const nbase*) const override;
  virtual nbase*	find_adjacent_left(const nbase*) const override;
  nbase*		insert_binary_node(nbase*&);
  virtual nbase*	insert_node(nbase* N) override;
  nbase*		insert_node(nbase* N1, nbase* N2);
  tlist&		operator = (const tlist &);
  virtual void		order();
  void			order(nbase*);
  nbase*		remove_node(nbase* N) override;

  virtual nbase*	earliest_node() const;
  virtual nbase*	latest_node() const;
  void			clear() override;
  tlist&		operator << (nbase* node) override;
  tlist&		operator << (lbase&) override;
  tlist&		operator << (tlist &);
  bool			operator <= (tlist &);

#ifdef TRY_WITHOUT
			// returns nonsense unless nLinker = intLinker
  virtual nbase*	operator[] (const unsigned int i) const;
#endif /* TRY_WITHOUT */

}; // end of class tlist

template <class keyClass, class nbase, class ownerData>
class ap_node_remover {
public:
	slist<keyClass, nbase, ownerData>* L;
	nbase* N;
	ap_node_remover(slist<keyClass, nbase, ownerData>* l, nbase* n) : L(l), N(n) {}
	/* Last condition inside if() was added because exceptions thrown
	 * while executing action_callbacks can leave a nbase in a somewhat
	 * unstable state. */
	~ap_node_remover() {
		if(N && L && N->list == L) {
			L->slist<keyClass, nbase, ownerData>::remove_node(N); } }
	nbase* remove() {
		slist<keyClass, nbase, ownerData>* l = L;
		L = NULL;
		return l->slist<keyClass, nbase, ownerData>::remove_node(N); }
};

template <class keyClass, class nbase, class ownerData>
// the void constructor
slist<keyClass, nbase, ownerData>::slist() {
	head = NULL;
	tail = NULL;
	length = 0L;
}

template <class keyClass, class nbase, class ownerData>
slist<keyClass, nbase, ownerData>::slist(const ownerData& od) : Owner(od) {
	head = NULL;
	tail = NULL;
	length = 0L;
}

/* This constructor copies the list and all of its nodes.  Each node
 * must have method, copy() defined to create a new like node. */
template <class keyClass, class nbase, class ownerData>
slist<keyClass, nbase, ownerData>::slist(
		const slist<keyClass, nbase, ownerData>& list) {
	nbase* node;

	head = NULL;
	tail = NULL;
	length = 0L;
	for(	node = list.first_node();			//for each node on right
		node;
		node = node->next_node()) { 
			insert_node(node->copy()); } }

/*
 *	This function empties the list.  This method (as well as
 *	delete_node()) make use of the virtual destructor of class nbase.  In
 *	order for this destructor to work properly, ALL DESCENDANTS OF NODE
 *	SHOULD HAVE A DESTRUCTOR DECLARED, or else their members' destructors
 *	will not be invoked.
 */
template <class keyClass, class nbase, class ownerData>
void
slist<keyClass, nbase, ownerData>::clear() {
	nbase* node;
	nbase* next;

	for(node = head; node; node = next) {
		next = node->next_node();
		if (node->list != this) {
			cerr << "nbase(" << (void*) node << ")'s list is not correct:\n";
			cerr << "   this slist = " << hex << this << "\n";
			cerr << "   node's list = " << hex << (long) node->list << "\n";
			cerr << "   Next = " << hex << (long) node->next << "\n";
			cerr << "   Length = " << length << "\n"; }
		delete node; }
	head = tail = NULL;
	length = 0; }

template <class keyClass, class nbase, class ownerData>
slist<keyClass, nbase, ownerData>::~slist() {
	clear();
	/* Crashes when owner no longer exists:
	 * LO->slists.remove_list(n_this); */
	}

template <class keyClass, class nbase, class ownerData>
nbase*
slist<keyClass, nbase, ownerData>::find(const keyClass& target) const {
	nbase* node;
	for(node = head; node; node = node->next) {
		if(! target.compare_to(node->getKey())) {
			break;
		}
	}
	return node;
}

template <class keyClass, class nbase, class ownerData>
nbase*
slist<keyClass, nbase, ownerData>::find_match_after(
		const keyClass& target,
		nbase* after) const {
	nbase* node;
	if(after) {
		if(after->list == this) {
			node = after->next;
		} else {
			return NULL;
		}
	} else {
		node = head;
	}
	for(; node; node = node->next) {
		if(! target.compare_to(node->getKey())) {
			break;
		}
	}
	return node;
}

/*
 *	This function inserts the specified node at the end of the list.
 */
template <class keyClass, class nbase, class ownerData>
nbase*
slist<keyClass, nbase, ownerData>::insert_node(nbase* node) {
	if(!node) {

		//
		// Nothing to do
		//
		return 0;
	}

	if(node->list) {

		//
		// This will result in additional notifications:
		//
		node->list->remove_node(node);
	}

	//
	// update node pointers
	//
	node->list = this;
	node->previous = tail;
	node->next = NULL;

	//
	// update list pointers
	//
	if(head) {

		//
		// there must be a tail!
		//
		tail->next = node;
	} else {

		//
		// the tail does not exist!
		//
		head = node;
	}
	tail = node;

	length++;

	//
	// Notify any Miterators watching this list that a
	// new node has been inserted:
	//
	notify_iterators_about_adding(node);

	return node;
}


/*
 *	This function adds new_node to the List immediately after old_node.
 *	If old_node is 0, then new_node will be inserted at the start of the
 *	list.
 */
template <class keyClass, class nbase, class ownerData>
nbase*
slist<keyClass, nbase, ownerData>::insert_first_node_after_second(
		nbase* new_node,
		nbase* old_node) {
	if(new_node == old_node) {
		return(new_node);
	}
	if(old_node && old_node->list != this) {
		return NULL;
	}
	if(!new_node) {
		return NULL;
	}
	if(new_node->list) {
		new_node->list->remove_node(new_node);
	}
	new_node->list = this;
	if(old_node == NULL) {

		//
		// insert in first place
		//
		if(head) {
			head->previous = new_node;
		} else {
			tail = new_node;
		}
		new_node->next = head;
		new_node->previous = 0;
		head = new_node;
	} else {

		//
		// insert after old node
		//
		if (old_node->next) {
			old_node->next->previous = new_node;
		} else {
			tail = new_node;
		}
		new_node->next = old_node->next;
		new_node->previous = old_node;
		old_node->next = new_node;
	}
	length++;
	notify_iterators_about_adding(new_node);
	return new_node;
}

/*
 *	This function deletes the specified node from the list and fixes the
 *	links (see nbase::nbase()).  It returns TRUE only if the specified node is on
 *	the specified list.  See clear().
 */

template <class keyClass, class nbase, class ownerData>
bool
slist<keyClass, nbase, ownerData>::delete_node(
		nbase* node) {
	if (!node || node->list != this) return false;
	delete node;
	return true; }

/*
 *	This function removes a node from the list but does not delete it. 
 *	It returns the specified node if it is on the specified list.  Otherwise
 *	it returns 0.
 */

template <class keyClass, class nbase, class ownerData>
nbase*
slist<keyClass, nbase, ownerData>::remove_node(
		nbase* node) {
	if(!node) {
		Cstring s;
		s << "APGEN INTERNAL ERROR -- attempt to remove foreign node.\n";
		throw(eval_error(s));
	}
	if(node->list != this) {
		Cstring s;
		s << "APGEN INTERNAL ERROR -- attempt to remove foreign node; it says its list is at "
			<< void_to_uint(node->list) << ", not this at " << void_to_uint(this)
			<< ".\n";
		throw(eval_error(s));
	}

	//
	// do this last: the call may throw; we don't want to leave a mess
	// behind... unfortunately, no, that does not work! We must notify
	// iterators while the node-to-be-removed is still there.
	//
	notify_iterators_about_removing(node);

	//
	// if not the first node:
	//
 	if (node->previous) {
		node->previous->next = node->next;
	} else {
		head = node->next;
	}

	//
	// if not the last node:
	//
	if (node->next) 	{
		node->next->previous = node->previous;
	} else {
		tail = node->previous;
	}
	length--;
	node->list = 0;
	node->next = 0;
	node->previous = 0;
	return node;
}

template <class keyClass, class nbase, class ownerData>
slist<keyClass, nbase, ownerData>&
slist<keyClass, nbase, ownerData>::operator = (const slist<keyClass, nbase, ownerData>& list) {
	nbase* node;

	clear();
	for (	node = list.first_node();		//for each node on right
		node;
		node = node->next) {
		insert_node(node->copy());		//insert a copy
	}
	return *this; }

template <class keyClass, class nbase, class ownerData>
slist<keyClass, nbase, ownerData>&
slist<keyClass, nbase, ownerData>::operator << (
		slist<keyClass, nbase, ownerData>& from) {
	nbase* node;

	if (&from == this) return *this;	//"x << x;"

	//
	// notifications about removal
	//
	for (node = from.head;
		 node;
		 node = node->next) {
		from.notify_iterators_about_removing(node);
	}

	if (!head) {
		head = from.head;
		tail = from.tail;
	} else if (from.head) {
		tail->next = from.head;
		from.head->previous = tail;
		tail = from.tail;
	}


	//
	// notifications about insertion
	//
	for (node = from.head;
		 node;
		 node = node->next) {
		node->list = this;
		notify_iterators_about_adding(node);
	}
	length += from.length;

	from.head = 0;
	from.tail = 0;
	from.length = 0;

	return *this;
}

template <class keyClass, class nbase, class ownerData>
tlist<keyClass, nbase, ownerData>&
tlist<keyClass, nbase, ownerData>::operator = (
		const tlist<keyClass, nbase, ownerData>& L) {
	nbase* node;

	lbase::Owner = L.Owner;
	clear();
	for (	node = L.first_node();
		node;
		node = node->next_node()) {
		insert_node(node->copy());
	}
	return *this;
}

template <class keyClass, class nbase, class ownerData>
tlist<keyClass, nbase, ownerData>::tlist(bool should_sync)
	: ambiguityCount(0),
		synchronize_orders(should_sync),
		enabled(true),
		root(NULL) {
}

template <class keyClass, class nbase, class ownerData>
tlist<keyClass, nbase, ownerData>::tlist(const ownerData& od, bool should_sync)
	: slist<keyClass, nbase, ownerData>(od),
		ambiguityCount(0),
		synchronize_orders(should_sync),
		enabled(true),
		root(NULL) {
}

template <class keyClass, class nbase, class ownerData>
tlist<keyClass, nbase, ownerData>::tlist(
		const tlist<keyClass, nbase, ownerData>& B)
	: slist<keyClass, nbase, ownerData>(B),
		enabled(true),
		synchronize_orders(B.synchronize_orders),
		ambiguityCount(0),
		root(NULL) {
	operator = (B);
}

template <class keyClass, class nbase, class ownerData>
tlist<keyClass, nbase, ownerData>::~tlist() {
	clear();
}
	
template <class keyClass, class nbase, class ownerData>
void
tlist<keyClass, nbase, ownerData>::order() {
	nbase* N = lbase::first_node();
	nbase* M;

	if(!root) return;
	while(N) {
		M = N->next_node();
		lbase::remove_node(N);
		N = M;
	}
	order(root);
}

template <class keyClass, class nbase, class ownerData>
void
tlist<keyClass, nbase, ownerData>::order(
		nbase* N) {
	nbase* M;

	if((M = N->get_left()))
		order(M);
	lbase::insert_node(N);
	if((M = N->get_right()))
		order(M);
}

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::earliest_node() const {
	nbase* N = root;
	nbase* M;

	if(! root) return NULL;
	while((M = N->get_left())) {
		N = M;
	}
	return N;
}

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::latest_node() const {
	nbase* N = root;
	nbase* M;

	if(! root) return NULL;
	while((M = N->get_right()))
		N = M;
	return N;
}

template <class keyClass, class nbase, class ownerData>
void
tlist<keyClass, nbase, ownerData>::clear() {
#		ifdef DEBUG_BLIST
		cout << "tlist::clear(" << (void*) this
			<< ") START...\n";
#		endif
#		ifdef TRACE
		cout << (void*) this << " clear start\n";
		cout.flush();
#		endif

	enabled = false;
	slist<keyClass, nbase, ownerData>::clear();
	make_root(NULL);
	ambiguityCount = 0;
	enabled = true;

#		ifdef TRACE
		cout << (void*) this << " clear end\n";
		cout.flush();
#		endif
#		ifdef DEBUG_BLIST
		cout << "tlist::clear(" << (void*) this
			<< ") DONE.\n";
#		endif
}

template <class keyClass, class nbase, class ownerData>
tlist<keyClass, nbase, ownerData>&
tlist<keyClass, nbase, ownerData>::operator << (nbase* N) {
	try {
		if (!insert_node(N)) {
			Cstring msg;
			msg << "APGEN Internal Error:  duplicate tlist node ignored by L"
				"ist&tlist::operator<<(nbase*N)\n";
			throw(eval_error(msg));
		}
	}
	catch(eval_error Err) {
		throw(Err);
	}
	return *this;
}

template <class keyClass, class nbase, class ownerData>
bool
tlist<keyClass, nbase, ownerData>::operator <= (tlist<keyClass, nbase, ownerData>& rhs) {
	nbase* N = lbase::first_node();

	while(N) {
		if(!rhs.find_node(N)) return false;
		N = N->next_node();
	}
	return true;
}

template <class keyClass, class nbase, class ownerData>
void
tlist<keyClass, nbase, ownerData>::add_without_duplication(
		slist<keyClass, nbase, ownerData> & l) {
	nbase* N = l.first_node();
	nbase* M;

	while(N) {
		M = N->next_node();
		if(!find_node(N)) insert_node(N);
		N = M;
	}
}

template <class keyClass, class nbase, class ownerData>
tlist<keyClass, nbase, ownerData>&
tlist<keyClass, nbase, ownerData>::operator << (
		slist<keyClass, nbase, ownerData>& l) {
	nbase* N = l.first_node();
	nbase* M;
#			ifdef DEBUG_BLIST
			cout << "operator tlist (" << lbase::get_length() << ") <- List(" <<
				l.get_length() << ") START...\n";
#			endif

	while(N) {
		M = N->next_node();
		if (! insert_node(N)) {
			cerr << "APGEN Internal Error:  duplicate tlist node ignored "
				"by List&tlist::operator<<(List&l)"
				<< endl;
			// a little too drastic:
			// exit(-2);
		}
		N = M;
	}
	return *this;
}

template <class keyClass, class nbase, class ownerData>
tlist<keyClass, nbase, ownerData>&
tlist<keyClass, nbase, ownerData>::operator << (
		tlist<keyClass, nbase, ownerData>& l) {
	nbase* N = l.first_node();
	nbase* M;

#			ifdef DEBUG_BLIST
			cout << "operator tlist (" << lbase::get_length() << ") <- tlist(" <<
				l.get_length() << ") START...\n";
#			endif
	while(N) {
		M = N->next_node();
		if (! insert_node(N)) {
			cerr << "APGEN Internal Error:  duplicate tlist node ignored "
				"by List&tlist::operator<<(tlist&l)"
				<< endl;
			// a little too drastic:
			// exit(-3);
		}
		N = M;
	}
	return *this;
}

/* does NOT assume that the list is ordered (links do NOT
 * have to agree with previous/next pointers) */
template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::find_adjacent_left(
		const nbase* new_node) const {
	int	f;
	nbase* current_node = root;
	nbase* last_earlier_node = NULL;

	if(!current_node) return NULL;
	do {
		if ((f = new_node->getKey().compare_to(current_node->getKey())) < 0)
			current_node = current_node->get_left();
		else if(!f) {
			if(current_node != new_node)
				return current_node;
			else
				current_node = current_node->get_left(); }
		else {
			last_earlier_node = current_node;
			current_node = current_node->get_right(); }
		} while(current_node);
	return last_earlier_node; }

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::find_at_or_after(
		const keyClass& compval) const {
	int	f;
	nbase* current_node = root;
	nbase* last_later_node = NULL;

	if(!current_node) return NULL;
	do {
		if(!current_node->list) {
			return NULL;
		}
		if ((f = compval.compare_to(current_node->getKey())) < 0) {
			last_later_node = current_node;
			current_node = current_node->get_left();
		} else if(!f) {
			while(current_node->get_left() && compval == current_node->get_left()->getKey()) {
				current_node = current_node->get_left();
			}
			return current_node;
		} else {
			current_node = current_node->get_right();
		}
	} while(current_node);
	return last_later_node;
}

#ifdef PREMATURE
//
// Attempt to throw an exception
//
template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::find_at_or_after(
		const CTime_base& compval) const {
	if(!keyClass::reacts_to_time_events()) {
		throw(eval_error("tlist::find_at_or_after(): key is not time-based.\n"));
	}

	nbase*		current_node = root;
	nbase*		last_later_node = NULL;
	CTime_base	T;

	if(!current_node) return NULL;

	do {
		if(!current_node->list) {
			return NULL;
		}
		if (compval < (T = (CTime_base) current_node->getKey())) {
			last_later_node = current_node;
			current_node = current_node->get_left();
		} else if(compval == T) {
			while(current_node->get_left() && compval == (CTime_base) current_node->get_left()->getKey()) {
				current_node = current_node->get_left();
			}
			return current_node;
		} else {
			current_node = current_node->get_right();
		}
	} while(current_node);
	return last_later_node;
}
#endif /* PREMATURE */

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::find_after(
		const keyClass& compval) const {
	nbase*	node = find_at_or_after(compval);

	if(!node) return NULL;
	if(node->Key == compval) {
		return node->following_node();
	}
	return node;
}

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::find_at_or_before(
		const keyClass& compval) const {
	int	f;
	nbase* current_node = root;
	nbase* last_earlier_node = NULL;
	nbase* tempnode;

	if(!current_node) return NULL;
	do {
		if ((f = compval.compare_to(current_node->getKey())) < 0) {
			current_node = current_node->get_left();
		} else if(!f) {
			while((tempnode = current_node->get_right())
				&& compval == tempnode->getKey()) {
				current_node = tempnode;
			}
			return current_node;
		} else {
			last_earlier_node = current_node;
			current_node = current_node->get_right();
		}
	} while(current_node);
	return last_earlier_node;
}

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::find_before(
		const keyClass& compval) const {
	nbase*	node = find_at_or_before(compval);

	if(!node) return NULL;
	if(node->Key == compval) {
		//
		// typo!!
		//
		// return node->previous_node();
		return node->preceding_node();
	}
	return node;
}

/* does NOT assume that the list is ordered (links do NOT
 * have to agree with previous/next pointers) */
template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::find_adjacent_right(
		const nbase* new_node) const {
	int	f;
	nbase* current_node = root;
	nbase* earliest_later_node = NULL;

	if(!current_node) return NULL;
	do {
		if(!current_node->list) {
			/* unreachable, I'd say - unless elements have been
			 * temporarily removed. But in that case it would be
			 * more correct to proceed... */
			return NULL;
		}
		if ((f = new_node->getKey().compare_to(current_node->getKey())) < 0) {
			earliest_later_node = current_node;
			current_node = current_node->get_left();
		} else if(!f) {
			if(current_node != new_node) {
				/* The logic is presumably that the client
				 * intends to insert new_node but hasn't done
				 * so quite yet. But it's not clear that
				 * returning the current node is correct... */
				return current_node;
			} else {
				/* new_node is in the list; we want to get the
				 * adjacent node to the right. */
				current_node = current_node->get_right();
			}
		} else {
			current_node = current_node->get_right();
		}
	} while(current_node);
	return earliest_later_node;
}

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::find(
		const keyClass& ref) const {
	int	f;
	nbase*	current_node = root;

	if(!current_node) return NULL;
	do {
		if(!current_node->list) {
			cerr << "tlist::find: node at " << (void*) this
				<< " has NULL list pointer" << endl;
			return NULL;
		}
		if ((f = ref.compare_to(current_node->getKey())) < 0) {
			current_node = current_node->get_left();
		} else if(!f) {
			break;
		} else {
			current_node = current_node->get_right();
		}
	} while(current_node);
	return current_node; // node already exists
}

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::find_node(
		nbase* ref) const {
	int	f;
	nbase* current_node = root;

	if(!current_node) return NULL;
	do {
		if(!current_node->list) {
			cerr << "tlist::find: node at " << (void*) this
				<< " has NULL list pointer" << endl;
			return NULL; }
		if ((f = ref->getKey().compare_to(current_node->getKey())) < 0)
			current_node = current_node->get_left();
		else if(!f)
			break;
		else
			current_node = current_node->get_right();
		} while(current_node);
	return current_node; } // node already exists

	/* The one place where the following function is used is in
	 * UI_ds_timeline, dealing with Legends. You need to
	 * search through Legends quickly, whence the tlist part;
	 * but you want to control the order also and
	 * the display uses the linked-list order for that purpose. */
template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::insert_node(
		nbase* N,
		nbase* after_this_one) {
	nbase* K = N;

	try { if(insert_binary_node(K)) {
		return lbase::insert_first_node_after_second(N, after_this_one); } }
	catch(eval_error Err) {
		throw(Err); }
	return NULL; }

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::insert_node(
		nbase* N) {
	nbase* K = N;

	try { if(insert_binary_node(K)) {
		// note: K is now the node preceding N in the tlist
		if(synchronize_orders) {
			return lbase::insert_first_node_after_second(N, K); }
		else {
			return lbase::insert_node(N); } } }
	catch(eval_error Err) {
		throw(Err); }
	return NULL; }

	/*

	IMPORTANT NOTE:
	---------------

	It would seem that the following function should call
	slist::insert_node(nbase*, nbase*) to insert the new node
	at the proper position.

	The reason this is NOT done is that it often happens that
	one wants to (i) search a tlist efficiently and (ii) maintain
	the (unsorted) order in which the nodes were inserted.

	If you want to insert a whole bunch of Nodes and end up
	with a sorted tlist, call tlist::order() to force agreement
	between the linked list and the btree.

	*/

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::insert_binary_node(nbase*& N) {
	long		n, i, f;
	TreeDir::Dir	dir, op_dir;
	nbase		*current_node, *new_node = N,
			*last_earlier_node = NULL;
	valueHolder*	save_new_cons;
	nbase*		path[MAXDEPTH];
	TreeDir::Dir	p_direction[MAXDEPTH];
	bool		ignore_secondary_key = true;

	/* Always do this first because we need to have a clean slate before
	 * detecting synchronization problems (Time_nodes only) */
	if(N->list) N->list->remove_node(N);
	if(N->Key.disambiguates()) {
		N->Key.get_2nd_key() = 0;
	}

	// AVL TreeDir::BALANCED TREE ALGORITHM (see e. g. Knuth)

	if(!root) {
		new_node->make_root_of(this);
		root->reset_links();
		N = last_earlier_node;
		// DONE (nothing to do)
		return root;
	}

	i = 0L;
	current_node = root;

	/* The following is not really appropriate:
		r_cons[0] = 0;
		l_cons[0] = 0;
	What we want to do instead is 2 things:
		- update the left and right consumptions as
		  we change the topology of the btree
		- update the path to the current node at the
		  very end so as to include its contribution
		  to all the relevant consumption values.
	*/

	do {
		if(!current_node->list) {
			cerr << "tlist::insert_binary_node: node at "
				<< (void *) this
				<< " has NULL list pointer" << endl;
			// DONE (error)
			return NULL;
		}
		path[i] = current_node;
		f = new_node->getKey().compare_to(current_node->getKey(), ignore_secondary_key);
		if(!f) {
		    // key already exists
		    if(new_node->getKey().disambiguates() && !new_node->list) {
			ignore_secondary_key = false;
			new_node->Key.disambiguate(ambiguityCount);
			f = 1;
		    } else {
			Cstring msg;
			msg << "tlist::insert_binary_node: attempting to "
				"insert node " << new_node->get_key()
				<< " @ " << ((void*) new_node)
				<< " into a spot already taken by "
				<< current_node->get_key() << " @ "
				<< ((void*) current_node) << "...\n";
			
			// DONE (error)
			throw(eval_error(msg)); 
		    }
		}
		if(f < 0) {
			/* See above comment; we'll do things differently.
			r_cons[i] +=	current_node->get_i_cons()
						+ current_node->get_r_cons();
			*/
			current_node = current_node->get_left();
			p_direction[i++] = TreeDir::LEFT_DIR; }
		else {
			last_earlier_node = current_node;
			/* See above comment; we'll do things differently.
			l_cons[i] +=	current_node->get_i_cons()
						+ current_node->get_l_cons();
			*/
			current_node = current_node->get_right();
			p_direction[i++] = TreeDir::RIGHT_DIR; }
		} while(current_node);
	N = last_earlier_node;

	// We know the path ends with a leaf. Attach the new node to it.
	path[i--] = new_node;

	new_node->reset_links();
	/* change in topology, but for the time being we 'pretend' that
	 * the current node consumes 0; we will adjust consumption values
	 * at the very end.  */
	save_new_cons = new_node->get_i_cons().copy();
	new_node->set_i_cons(NULL);
	path[i]->attach(p_direction[i], new_node, TreeDir::DO_NOT_UPDATE_CONS);

	// n is the number of items in the path to the new node

	n = i + 2;

	// look for a focal point

	f = n - 1;
	while(--f >= 0 && path[f]->indicator == TreeDir::BALANCED);

	// path[f] is the focal point

	// CASE 1

	if(f < 0) {	// focal point is above the root
		/* no change in topology, but new node needs to be accounted for
		 * in all nodes in the path that leads to it */
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		while(++f < n - 1) {
			path[f]->indicator = p_direction[f];
		}
		new_node->add_cons_to_ancestors();
		// DONE
		return new_node;
	}

	// CASE 2

	if(path[f]->indicator != p_direction[f]) {
		// no change in topology, but new node needs to be accounted for
		// in all nodes in the path that leads to it

		path[f]->indicator = TreeDir::BALANCED;
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		while(++f < n - 1) {
			path[f]->indicator = p_direction[f];
		}
		new_node->add_cons_to_ancestors();
		// DONE
		return new_node;
	}

	/*CASE 3

	       f - 1					f - 1 
	      /						  |
	     /						  |
	    /						  |
	    f			===>		    f	  |
	   / \					   /|\	  |
	  /   \ heavy				  / | \	  |
	 /     \				 /  |  \  |
	A	f + 1				A   |   f + 1
		/  \				    |      \
	       /    \				    |       \
	      /      \				    |        \
	     B        f + 2			    B	    f + 2

	*/

	dir = p_direction[f];
	op_dir = dir ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;

	if(dir == p_direction[f + 1]) {
		path[f]->attach(dir, path[f + 1]->Links[op_dir], TreeDir::UPDATE_CONS);
		path[f + 1]->attach(op_dir, path[f], TreeDir::UPDATE_CONS);

		// BUG: n - 1 here used to be n
		for(i = f + 2; i < n - 1; i++) {
			path[i]->indicator = p_direction[i];
		}

		path[f]->indicator = TreeDir::BALANCED;

		if(!f) {
			path[1]->make_root_of(this);
			new_node->set_i_cons(save_new_cons);
			delete save_new_cons;
			new_node->add_cons_to_ancestors();
			// DONE
			return new_node;
		} else {
			/* do not update consumption (didn't change -- remember
			 * the current node is assumed to have zero consumption */
			path[f - 1]->attach(p_direction[f - 1], path[f + 1], TreeDir::DO_NOT_UPDATE_CONS);
			new_node->set_i_cons(save_new_cons);
			delete save_new_cons;
			new_node->add_cons_to_ancestors();
			// DONE
			return new_node; 
		}
	}

	/*MODIFIED CASES 4 AND 5

		f - 1				f - 1
		 /				  |
		/				  |
	       /		===>		  |
	      f				f_	  |
	       \ heavy			  \	  |
		\			   \	  |
		 \			    \	  |
		f + 1			     \	  |    ___f + 1
		 /			      \   |   /
		/			       \  |  /
	       /				\ | /
	    f + 2				f + 2

	*/
	if((f + 3) == n) {
		path[f]->indicator = TreeDir::BALANCED;
		path[f]->attach(dir, NULL, TreeDir::UPDATE_CONS);
		path[f + 1]->attach(op_dir, NULL, TreeDir::UPDATE_CONS);
		path[f + 2]->attach(dir, path[f + 1], TreeDir::UPDATE_CONS);
		path[f + 2]->attach(op_dir, path[f], TreeDir::UPDATE_CONS);
		if(!f) {
			path[f + 2]->make_root_of(this);
			new_node->set_i_cons(save_new_cons);
			delete save_new_cons;
			new_node->add_cons_to_ancestors();
			// DONE
			return new_node; }
		// no adjustment
		path[f - 1]->attach(p_direction[f - 1], path[f + 2], TreeDir::UPDATE_CONS);
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		new_node->add_cons_to_ancestors();
		// DONE
		return new_node;
	}

	/*CASE 4

	      f - 1				    f - 1
	       /				      |
	      /				  	      |
	     /				  	      |
	    f				 f____ 	      |
	   /  \ heavy			/ \   \	      |
	  /    \		===>   /   |   \      |
	 /	\		      /    |	\     |
	A	f + 1 (B)	     A	   |	 \    |    ___f + 1
		 /			   |      \   |   /     /
		/			   |       \  |  /     /
	       /			   |	    \ | /     /
	    f + 2 (B)			   |	    f + 2    /
	   /    \			   \		    /
	  /      \			    \		   /
	 /        \			     \	          /
	B	 f + 3			      B	        f + 3
	*/

	if(p_direction[f + 2] == dir) {
		path[f]->indicator = op_dir;
		path[f]->attach(dir, path[f + 2]->Links[op_dir], TreeDir::UPDATE_CONS);
		path[f + 2]->attach(op_dir, path[f], TreeDir::UPDATE_CONS);
		path[f + 1]->attach(op_dir, path[f + 3], TreeDir::UPDATE_CONS);
		path[f + 2]->attach(dir, path[f + 1], TreeDir::UPDATE_CONS);
		for(i = f + 3; i < n - 1; i++)
			path[i]->indicator = p_direction[i];
		if(!f) {
			path[f + 2]->make_root_of(this);
			new_node->set_i_cons(save_new_cons);
			delete save_new_cons;
			new_node->add_cons_to_ancestors();
			// DONE
			return new_node; }
		// no update necessary
		// hmm... maybe...
		// path[f - 1]->attach(p_direction[f - 1], path[f + 2]);
		path[f - 1]->attach(p_direction[f - 1], path[f + 2], TreeDir::UPDATE_CONS);
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		new_node->add_cons_to_ancestors();
		// DONE
		return new_node;
	}

	/*CASE 5

	      f - 1				    f - 1
	       /				      |
	      /				  	      |
	     /				  	      |
	    f				 f____ 	      |
	   /  \ heavy			/ \   \	      |
	  /    \		===>   /   |   \      |
	 /	\		      /    |	\     |
	A	f + 1 (B)	     A	   |	 \    |    ___f + 1
		 / \			   |      \   |   /     / \
		/   \			   |       \  |  /     /   \ (heavy)
	       /    ...			   |	    \ | /     /     \
	    f + 2 (B)			   |	    f + 2    /      ...
	   /    \			   \		    /
   op_dir /      \ dir			    \		   /
	 /        \			     \	          /
	f + 3	   B			    f + 3        B
	*/

	path[f]->indicator = TreeDir::BALANCED;
	path[f + 1]->indicator = dir;
	path[f]->attach(dir, path[f + 3], TreeDir::UPDATE_CONS);
	path[f + 2]->attach(op_dir, path[f], TreeDir::UPDATE_CONS);
	path[f + 1]->attach(op_dir, path[f + 2]->Links[dir], TreeDir::UPDATE_CONS);
	path[f + 2]->attach(dir, path[f + 1], TreeDir::UPDATE_CONS);
	for(i = f + 3; i < n - 1; i++)
		path[i]->indicator = p_direction[i];
	if(!f) {
		path[f + 2]->make_root_of(this);
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		new_node->add_cons_to_ancestors();
		// DONE
		return new_node;
	}
	// no update necessary
	path[f - 1]->attach(p_direction[f - 1], path[f + 2], TreeDir::UPDATE_CONS);
	new_node->set_i_cons(save_new_cons);
	delete save_new_cons;
	new_node->add_cons_to_ancestors();
	// DONE
	return new_node;
}

template <class keyClass, class nbase, class ownerData>
bool
tlist<keyClass, nbase, ownerData>::check_consumption(ostringstream& s) {
	nbase*	vn;
	double	cons = 0.0;
	bool	problem = false;

	cons = 0.0;
	for(vn = earliest_node(); vn; vn = vn->following_node()) {
		double self = vn->get_i_cons().get_double();
		double left = vn->get_l_cons().get_double();
		double right = vn->get_r_cons().get_double();
		cons += self;
		s << *vn->get_key() << " cons " << cons << " reported " << vn->get_consumption()
			<< " self " << self << " left " << left << " right " << right << "\n";
		// assert(fabs(cons - vn->get_consumption()) < 0.01);
		if(!problem) {
			problem = fabs(cons - vn->get_consumption()) > 0.01;
		}
	}
	return !problem;
}

template <class keyClass, class nbase, class ownerData>
nbase*
tlist<keyClass, nbase, ownerData>::remove_node(nbase* N) {
	ap_node_remover<keyClass, nbase, ownerData> nr(this, N);
	long		i, n, f, i_old;
	TreeDir::Dir	heavy_direction, search_dir, P, Q;
	nbase		*old_node = N,
			*current_node, *end_node, *save,
			*a, *b, *newnode, *q_link_f, *z;
	nbase*		path[MAXDEPTH];
	TreeDir::Dir	p_direction[MAXDEPTH];

	if(!enabled) {
	    if((current_node = nr.remove())) {
		current_node->reset_links();
	    }
	    if(!lbase::get_length()) {
		root = NULL;
	    }
	    return current_node;
	}

	// PHASE 1: SEARCH

	if(!root)
		return NULL;
	i = 0;
	current_node = root;
	do {
		if(!current_node->list) {
			cerr << "tlist::remove_node: node at " << (void*) this
				<< " has NULL list pointer" << endl;
			return NULL; }
		path[i] = current_node;
		if((f = old_node->getKey().compare_to(current_node->getKey())) < 0) {
			current_node = current_node->get_left();
			p_direction[i++] = TreeDir::LEFT_DIR; }
		else if(!f)
			break;
		else {
			current_node = current_node->get_right();
			p_direction[i++] = TreeDir::RIGHT_DIR; }
		} while(current_node);


	/* change sign of incremental consumption and update
	 * ancestors. This will cancel the effect of the node
	 * being removed. */
	old_node->subtract_cons_from_ancestors(NULL);
	/* we do this so that we can use check_consumption() in debug mode
	 * without triggering asserts */
	old_node->set_i_cons(NULL);

#ifdef DESPERATE
	// debug
	int dbgk;
	ostringstream oss1;
	if(!check_consumption(oss1)) {
		cout << oss1.str();
		cout.flush();
		assert(0); }
#endif /* DESPERATE */

	if((!current_node) || (current_node != old_node)) {
	    stringstream s;
	    int		k = 0;
	    nbase*	n = lbase::first_node();

	    s << "Bad news: can't seem to remove node from binary tree. "
			"'old' node @" << ((void*) N) << " : " << N->get_key() << "\n";
	    s << "Complete list, size " << lbase::get_length() << ":\n";
	    while(n) {
		s << k++ << " @" << ((void*) n) << " : " << n->get_key();
		if(n == root) {
			s << " (root)\n"; }
		else if(n == current_node) {
			s << " (best guess)\n"; }
		else {
			s << endl; }
		if(n->get_left()) {
			s << "    left: "
				<< n->get_left()->get_key()
				<< endl; }
		else {
			s << "    left: NULL\n"; }
		if(n->get_right()) {
			s << "    right: "
				<< n->get_right()->get_key()
				<< endl; }
		else {
			s << "    right: NULL\n"; }
		if(N == n) {
			s << "This one matches!\n"; }
		n = n->next_node();
	    }

	    s << "Path:\n";
	    for(k = 0; k < i; k++) {
		s << "# " << (k + 1) << ": " << path[k]->get_key()
		  << endl;
	    }
	    throw(eval_error(s.str().c_str()));
	}

	/* DON'T FORGET TO REMOVE THE NODE: handled by the ap_node_remover now.
	 * List::remove_node(current_node);
	 */

	// PHASE 2: SWAP

	i_old = i;

	// NOTE: path[i_old] == old_node

	if((!current_node->get_left()) && (!current_node->get_right())) {
		// the old_node is a leaf; no swap necessary.
		if(!i) {
			root = NULL;
			return old_node; }
		// delete the leaf; note that we don't touch the indicator.
		// No need to update: current node has zero incremental consumption now.
		path[i - 1]->attach(p_direction[i - 1], NULL, TreeDir::DO_NOT_UPDATE_CONS);
		n = i - 1; }
	else {
		// the old node is not a leaf
		nbase* other;

		// swap the node to be deleted with a leaf

		if(	(current_node->indicator == TreeDir::LEFT_DIR)
			|| (current_node->indicator == TreeDir::BALANCED))
			heavy_direction = TreeDir::LEFT_DIR;
		else
			heavy_direction = TreeDir::RIGHT_DIR;
		search_dir = heavy_direction ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;

		// move once in the heavy direction

		current_node = current_node->Links[heavy_direction];
		p_direction[i] = heavy_direction;
		path[++i] = current_node;

		// then move in the direction that's not heavy

		current_node = current_node->Links[search_dir];

		if(current_node) {

			// swap sub-case (1) (nominal: found a branch containing swap candidate)

			do {
				p_direction[i] = search_dir;
				path[++i] = current_node;
				current_node =
					current_node->Links[search_dir]; }
			while(current_node);

			/* now see whether there is another node hanging from
			 * the last one in the path */

			end_node = path[i];

#ifdef DESPERATE
			// debug
			oss1 << "Path: (note: i_old = " << i_old << ")\n";
			for(dbgk = 0; dbgk <= i; dbgk++) {
				oss1 << "\t" << dbgk << ": " << *path[dbgk]->get_key() << "\n";
				oss1 << "\t\ti " << path[dbgk]->get_i_cons().get_double()
					<< " l " << path[dbgk]->get_l_cons().get_double()
					<< " r " << path[dbgk]->get_r_cons().get_double() << "\n"; }
			if(a = end_node->Links[heavy_direction]) {
				oss1 << "\t" << (i + 1) << "* " << *a->get_key() << "\n";
				oss1 << "\t\ti " << a->get_i_cons().get_double()
					<< " l " << a->get_l_cons().get_double()
					<< " r " << a->get_r_cons().get_double() << "\n"; }
#endif /* DESPERATE */

			/* disconnect the endnode. We update consumption up the tree
			 * until we reach the node to be deleted (== path[i_old]) */
			end_node->subtract_cons_from_ancestors(path[i_old]);
			/* we now attach any heavy link of the leaf node to the parent
			 * of the leaf node. No need to adjust consumption further;
			 * we've done exactly the right thing by subtracting the consumption
			 * of the leaf node up to the right point. */
			path[i - 1]->attach(
					search_dir,
					(a = end_node->Links[heavy_direction]), TreeDir::DO_NOT_UPDATE_CONS);
			// check_consumption();
			// can't check yet; we need to swap nodes...
			if(!a) {
				n = i - 1; }
			else {
				n = i;
				path[n] = a;
				p_direction[n] = search_dir;
				path[n]->indicator = search_dir; }
				// that's where the deleted node would have been

			// now we swap path[i_old] with the end_node

			save = path[i_old];
			path[i_old] = end_node;
			// adjust the end node itself; everything else is OK
			end_node->attach_left(save->get_left(), TreeDir::UPDATE_CONS);
			end_node->attach_right(save->get_right(), TreeDir::UPDATE_CONS);
			end_node->indicator = save->indicator; }
		else if((other = (a = path[i])->Links[heavy_direction])) {

			/* swap sub-case (2) (branch that was to contain
			 * swap candidate extends in the wrong direction) */

			/* The situation: we were able to move once (in the heavy
			 * direction) but there is no node in the search direction.
			 * We cheat a little bit and rearrange the tree in a way that
			 * does NOT respect the order... This does not matter,
			 * because we will remove the node at i_old shortly
			 * anyway. */

/*
				i_old-1			i_old-1
				   \			   \
				    \			    \
				     \			     \
				    i_old	==>	      a
				    /   \		     / \
			   (heavy) /     \		    /   \ 
				  /       \		   /     \
			    a = path[i]	   b  		other	  b
				/	  / \		  \	 / \
			heavy  /	 ... \		   \	/   \
			      /		     ...	    \  ...   \
			     other			  i_old      ...
*/

			// we mimic deleting path[i_old] in its new position
			// NOTE: only a needs an updated consumption value
			a->attach(	search_dir,
					(b = old_node->Links[search_dir]),
					TreeDir::UPDATE_CONS);
			a->indicator = old_node->indicator;
			other->indicator = search_dir;
			path[i_old] = a;
			p_direction[i_old] = heavy_direction;
			path[i] = other;
			p_direction[i] = search_dir;
			n = i;
			end_node = a; }
		else if((b = old_node->Links[search_dir])) {

			// swap sub-case (3) (must swap with short stub)

			// we cheat a little bit by pretending that old_node
			// was < a ...
			// we mimic deleting path[i_old] in its new position

			// NOTE: only a needs an updated consumption value
			a->attach(
				search_dir,
				(b = old_node->Links[search_dir]),
				TreeDir::UPDATE_CONS);
			path[i_old] = a;
			p_direction[i_old] = heavy_direction;
			n = i_old;
			end_node = a; }
		else {

			/* swap sub-case (4) (old node has single node under
			 * it in the heavy direction). No need to adjust
			 * consumptions: we are not changing topology. */

			path[i_old] = a;
			p_direction[i_old] = search_dir;
			a->indicator = search_dir;
			n = i_old;
			end_node = a; }

		/* DONE so far with consumption adjustment. No need to adjust
		 * path[i_old-1] since we've already cancelled the effect of
		 * the node to be deleted. */

		// attach the leaf we swapped with the to-be-deleted node to the upstream node
		if(! i_old) {
			end_node->make_root_of(this); }
		else {
			path[i_old - 1]->attach(
					p_direction[i_old - 1],
					end_node, TreeDir::UPDATE_CONS); }
		/* At this point the path is like this:
		 *
		 * 	path[0]
		 * 	path[1]
		 * 	...
		 * 	path[i_old - 1]
		 * 	end_node
		 * 	path[i_old + 1]
		 * 	...
		 * 	path[n]
		 */
		}
#ifdef DESPERATE
	ostringstream oss2;
	if(!check_consumption(oss2)) {
		cout << "Last good list:\n" << oss1.str();
		cout << "New path: (Note: n = " << n << ")\n";
		for(dbgk = 0; dbgk < i_old; dbgk++) {
			cout << "\t" << dbgk << ": " << path[dbgk]->get_key() << "\n";
			cout << "\t\ti " << path[dbgk]->get_i_cons().get_double()
				<< " l " << path[dbgk]->get_l_cons().get_double()
				<< " r " << path[dbgk]->get_r_cons().get_double() << "\n"; }
		cout << "\t" << i_old << ": " << end_node->get_key() << "\n";
		cout << "\t\ti " << end_node->get_i_cons().get_double()
			<< " l " << end_node->get_l_cons().get_double()
			<< " r " << end_node->get_r_cons().get_double() << "\n";
		for(dbgk = i_old + 1; dbgk <= n; dbgk++) {
			cout << "\t" << dbgk << ": " << path[dbgk]->get_key() << "\n";
			cout << "\t\ti " << path[dbgk]->get_i_cons().get_double()
				<< " l " << path[dbgk]->get_l_cons().get_double()
				<< " r " << path[dbgk]->get_r_cons().get_double() << "\n"; }
		cout << "Bad list:\n" << oss2.str();
		cout.flush();
		assert(0); }
#endif /* DESPERATE */

	// now we have to adjust the tree

	// step 1: initial adjustment

	if(path[n]->indicator == TreeDir::BALANCED) {
		// case 1
		path[n]->indicator = p_direction[n] ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;
		// DONE
		return old_node; }
	else if(path[n]->indicator == p_direction[n]) {
		// case 2
		path[n]->indicator = TreeDir::BALANCED;
		f = n - 1;
		if(f < 0) {
			// DONE
			return old_node; }
		// else go to step 2
		}
	else {
		/* case 3: path[n] is heavy in the q_direction. Since the
		 * deleted node is a leaf, a = path[n]->links[q_direction]
		 * has one descendant in the p_direction of path[n] (situation A),
		 * one descendant in the q-direction of path[n] (situation B),
		 * or two descendants (situation C).
		 */
		P = p_direction[n];
		Q = P ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;
		a = path[n]->Links[Q];
		if(a->indicator == P) // situation A
			{
			newnode = a->Links[P];
			path[n]->attach(Q, NULL, TreeDir::UPDATE_CONS);
			path[n]->indicator = TreeDir::BALANCED;
			a->attach(P, NULL, TreeDir::UPDATE_CONS);
			a->indicator = TreeDir::BALANCED;
			newnode->attach(P, path[n], TreeDir::UPDATE_CONS);
			newnode->attach(Q, a, TreeDir::UPDATE_CONS);
			// DONE so far
			}
		else if(a->indicator == Q) // situation B
			{
			newnode = a;
			path[n]->attach(Q, NULL, TreeDir::UPDATE_CONS);
			path[n]->indicator = TreeDir::BALANCED;
			newnode->attach(P, path[n], TreeDir::UPDATE_CONS);
			newnode->indicator = TreeDir::BALANCED;
			// DONE so far
			}
		else // situation C
			{

/*

			path[n]					a
			/     \				       / \
		   P   /       \ Q (heavy)	     (heavy)  /   \
		      /         \			     /     \
		(deleted)	 a		==>	    b	  other
				/ \			   /
			     P /   \ Q		  (heavy) /
			      /     \			 /
			     b       other	       path[n]

*/
			b = a->Links[P];
			path[n]->attach(Q, NULL, TreeDir::UPDATE_CONS);
			path[n]->indicator = TreeDir::BALANCED;
			b->attach(P, path[n], TreeDir::UPDATE_CONS);
			b->indicator = P;
			a->attach(P, b, TreeDir::UPDATE_CONS);
			a->indicator = P;
			if(!n) {
				a->make_root_of(this);
				// DONE
				return old_node; }
			// no adjustment needed
			path[n - 1]->attach(p_direction[n - 1], a, TreeDir::DO_NOT_UPDATE_CONS);
			// DONE
			return old_node; }
		// We only get here in situations A and B, both of which define newnode
		if(!n) {
			newnode->make_root_of(this);
			// DONE
			return old_node; }
		f = n - 1;
		// no adjustment needed
		path[f]->attach(p_direction[n - 1], newnode, TreeDir::DO_NOT_UPDATE_CONS); }
	// DONE so far

	// step 2: search for a prime position

	while(1) {
		if(f < 0) {
			// DONE
			return old_node;
		}
		if(path[f]->indicator == TreeDir::BALANCED) {
				// case 1: we are removing the node from
				//	   the branch in the p-direction;
				//	   just change the indicator
			path[f]->indicator = p_direction[f] ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;
			// We can return, because the weight of the
			// node at position f has not changed w.r.t. the
			// situation prior to deleting the node.
			// DONE
			return old_node;
		} else if(path[f]->indicator == p_direction[f]) {
				// case 2: even better; the p-branch was
				//	   heavy, so now it's balanced!

			// NOTE: we can't return yet, because we are
			// changing the weight of path[f] and the topology
			// of upstream nodes may have to be modified.
			path[f--]->indicator = TreeDir::BALANCED;
		} else {
			TreeDir::Dir temp_dir = p_direction[f] ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;
				// case 3: darn, not so easy. We want to
				//	   remove a fruit from a light branch
				//	   opposite a heavier one.

			// step 3: prime adjustment

			q_link_f = path[f]->Links[temp_dir];
			if(q_link_f->indicator == p_direction[f]) {
				// case a: the first node down the heavy (q)
				// 	   branch leans the same way as the
				//	   p-branch. Rotate recursively.

				path[f]->attach(
					temp_dir,
					// subtree consumption is the same; no adjustment
					q_link_f->rotate(),
					TreeDir::DO_NOT_UPDATE_CONS);
				/* Note that we don't do anything here; we just loop
				 * again with the same [f]. Since we rotated the tree,
				 * we are guaranteed to fall into a different case at
				 * the next iteration. */
			} else {
				// case b: the first node down the heavy (q)
				//	   branch leans in the q direction.
				if(!f) {
					q_link_f->make_root_of(this);
				} else {
					// no adjustment necessary
					path[f - 1]->attach(
						p_direction[f - 1],
						q_link_f, TreeDir::DO_NOT_UPDATE_CONS); 
				}
				z = q_link_f->Links[p_direction[f]];
				path[f]->attach(p_direction[f] ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR, z, TreeDir::UPDATE_CONS);
				q_link_f->attach(p_direction[f], path[f], TreeDir::UPDATE_CONS);
				// DONE with updates
				if(q_link_f->indicator != TreeDir::BALANCED) {
					path[f]->indicator = TreeDir::BALANCED;
					q_link_f->indicator = TreeDir::BALANCED;
					f--; 
				} else {
					path[f]->indicator = p_direction[f] ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;
					q_link_f->indicator = p_direction[f];
					return old_node;
				}
			} 
		} 
	}
}

template <class keyClass, class nbase, class ownerData>
void
tlist<keyClass, nbase, ownerData>::check() {
	cout << "check:\n";
	return check(root, 0, NULL, NULL);
}

template <class keyClass, class nbase, class ownerData>
void
tlist<keyClass, nbase, ownerData>::check(
		nbase*		n,
		int		depth,
		const keyClass*	smaller,
		const keyClass*	bigger) {
	if(depth > lbase::get_length()) {
		cout << "depth = " << depth << " > length = " << lbase::get_length() << ", nonsense!\n";
		cout.flush();
		assert(false);
		return;
	}
	if(!n) return;

	if(n->Links[0]) {
		check(n->Links[0], depth + 1, smaller, &n->getKey());
	} else if(smaller) {
		assert(smaller->compare_to(n->getKey()) < 0);
	}
	for(int k = 0; k <= depth; k++) {
		cout << " ";
	}
	cout << ((void*)n) << ", " << n->get_key() << "\n";
	if(n->Links[1]) {
		check(n->Links[1], depth + 1, &n->getKey(), bigger); 
	} else if(bigger) {
		assert(bigger->compare_to(n->getKey()) > 0);
	}
	return;
}

//
// NODE TEMPLATES
//
// Initially, there were Dnode, Cnode and Tnode class templates; each
// one featured a linker, which is nice but adds a fair amount of
// overhead.
//
// It turns out that only value_node (the kind of node used in a
// resource history) needs the linker capability. The generic
// class templates in this header were simplified by removing
// the linker-related capability; value_node has its own capability
// which is no longer derived from a template.
//
// During refactoring the original templates coexisted with the
// linker-free versions, which bear the same name with a '0'
// at the end. Eventually, the original templates were removed,
// but the '0' is still there in the names of the current class
// templates.
//

//
// Dnode0 is suitable as a base class for a Final class.
//
template <class keyClass, class Final, class ownerData = short>
class Dnode0 {
  public:

// DATA
  TreeDir::Dir				indicator;
  Final*				Links[2]; // index will always be LEFT_DIR or RIGHT_DIR
  slist<keyClass, Final, ownerData>*	list;
  Final*				next;
  Final*				previous;
  slist<keyClass, Final, ownerData>*	saved_list; // to support temporary removal and reinsertion
  keyClass				Key;

// PURE VIRTUAL METHODS
  virtual Final*			copy() = 0;
  virtual Final*			get_this() = 0;
  virtual const Final*			get_this() const = 0;
// ... and do not forget the virtual destructor implementation in the Final class!

// CONSTRUCTORS
  /* constructor */		Dnode0()
					: next(NULL),
					previous(NULL),
					list(NULL),
					saved_list(NULL),
					indicator(TreeDir::BALANCED) {
						Links[TreeDir::RIGHT_DIR] = NULL;
						Links[TreeDir::LEFT_DIR] = NULL; }
  /* comparison with Key */	Dnode0(const keyClass& a)
					: Key(a),
					next(NULL),
					previous(NULL),
					list(NULL),
					saved_list(NULL),
					indicator(TreeDir::BALANCED) {}
  /* copy constructor */	Dnode0(const Dnode0& Ap)
					: Key(Ap.Key),
					next(NULL),
					previous(NULL),
					list(NULL),
					saved_list(NULL),
					indicator(TreeDir::BALANCED) {}

  virtual			~Dnode0() {
					/* We can't do this - casting does not
					 * work as intended. Move the code to
					 * the Final class.
					 * if(list) {
					 * 	list->remove_node((Final*) this);
					 * 	list = NULL; } */
					}

// METHODS

  virtual Final*		following_node() const;
  TreeDir::Dir			get_indicator() const { return indicator; }
  virtual const Cstring&	get_key() const { return Key.get_key(); }
  virtual unsigned long&	get_2nd_key() { return Key.get_2nd_key(); }
  Final*			get_left() const { return Links[TreeDir::LEFT_DIR]; }
  Final*			get_link(TreeDir::Dir which) const { return Links[which]; }
  int				get_pathlength(int& length);
  Final*			get_right() const { return Links[TreeDir::RIGHT_DIR]; }
  virtual const keyClass&	getKey() const { return Key; }
  bool				is_in_list(const slist<keyClass, Final, ownerData>& l) const { return list == &l; }
  bool				has_a_list() const { return list != NULL; }
  virtual void			make_root_of(slist<keyClass, Final, ownerData>*);
  Final*			next_node() const { return next; }
  Dnode0&			operator = (const Dnode0&) { return *this; }
  virtual backptr<Final>*	Pointers() { return NULL; }
  virtual Final*		preceding_node() const;
  virtual Final*		previous_node() const { return previous;}
  virtual void			reset_links() {
					indicator = TreeDir::BALANCED;
					Links[TreeDir::RIGHT_DIR] = NULL;
					Links[TreeDir::LEFT_DIR] = NULL;
  }

  Final*			rotate();
				// made virtual so a derived class can disable by throwing
  virtual void			temporarily_remove_from_list() {
	if(saved_list != NULL || list == NULL) {
		Cstring errs(get_key());
		if(saved_list) {
			errs << ": saved_list != NULL - cannot remove temporarily";
		} else {
			errs << ": list == NULL - cannot remove temporarily";
		}
		throw(eval_error(errs));
	}

	saved_list = list;
	list->remove_node(get_this());
  }

				// made virtual so a derived class can disable by throwing
  virtual void			re_insert_in_list() {
	if(list != NULL || saved_list == NULL) {
		Cstring errs(get_key());
		if(saved_list) {
			errs << ": saved_list != NULL - cannot re-insert";
		} else {
			errs << ": list == NULL - cannot re-insert";
		}
		throw(eval_error(errs));
	}
	(*saved_list) << get_this();
	saved_list = NULL;
  }

  //
  // Methods intended to make get_linker() obsolete:
  //
  virtual void		add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal) {
  }
  virtual void		add_cons_to_ancestors() {
  }
  virtual void		attach(		TreeDir::Dir right_or_left,
					Final* node_to_attach,
					TreeDir::cons_adjust) {
	Links[right_or_left] = node_to_attach;
  }
  virtual void		attach_left(	Final* node_to_attach,
					TreeDir::cons_adjust CA) {
	attach(TreeDir::LEFT_DIR, node_to_attach, CA);
  }
  virtual void		attach_right(	Final* node_to_attach,
					TreeDir::cons_adjust CA) {
	attach(TreeDir::RIGHT_DIR, node_to_attach, CA);
  }
  virtual double	get_consumption() const {
	return 0.0;
  }
  virtual valueHolder	get_i_cons() const {
	return valueHolder(0L);
  }
  virtual double	get_integral() const {
	return 0.0;
  }
  virtual double	get_integral_left() const {
	return 0.0;
  }
  virtual double	get_integral_right() const {
	return 0.0;
  }
  virtual true_long	get_int_consumption() const {
	return 0;
  }
  virtual valueHolder	get_l_cons() const {
	return valueHolder(0L);
  }
  virtual valueHolder	get_r_cons() const {
	return valueHolder(0L);
  }
  virtual Final*	get_parent() const {
	return NULL;
  }
  virtual void		init_cons() {
  }
  virtual bool		is_float() const {
	return false;
  }
  virtual void		set_i_cons(const valueHolder* vh) {
  }
  virtual void		set_parent(Final* Par) {
  }
  virtual void		subtract_cons_from_ancestors(Final* stop_here = NULL) {
  }
};	// end of Dnode0 class

template <class keyClass, class Final, class ownerData>
void
Dnode0<keyClass, Final, ownerData>::make_root_of(slist<keyClass, Final, ownerData>* l) {
	l->make_root(get_this());
}

template <class keyClass, class Final, class ownerData>
Final*
Dnode0<keyClass, Final, ownerData>::preceding_node() const {
	slist<keyClass, Final, ownerData>* L = list;

	if(!L) return NULL;
	return L->find_adjacent_left(get_this());
}

template <class keyClass, class Final, class ownerData>
Final*
Dnode0<keyClass, Final, ownerData>::following_node() const {
	slist<keyClass, Final, ownerData>* L = list;

	if(!L) return NULL;
	return L->find_adjacent_right(get_this());
}

template <class keyClass, class Final, class ownerData>
Final*
Dnode0<keyClass, Final, ownerData>::rotate() {
	TreeDir::Dir	t_direction, u_direction;
	Final* new_root;

	if(indicator == TreeDir::BALANCED) {
		return NULL;
	}
	t_direction = indicator;
	u_direction = t_direction ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;

	new_root = Links[t_direction];
	if(new_root->indicator == u_direction) {
		new_root = new_root->rotate();
		attach(t_direction, new_root, TreeDir::UPDATE_CONS);
	}
	if(new_root->indicator == TreeDir::BALANCED)
		indicator = TreeDir::BALANCED;
	else
		indicator = u_direction;
	new_root->indicator = u_direction;
	attach(t_direction, new_root->Links[u_direction], TreeDir::UPDATE_CONS);
	new_root->attach(u_direction, get_this(), TreeDir::UPDATE_CONS);
	return new_root;
}

//
// a version of Tnode without a linker
//
template <class keyClass, class ownerData = short>
class Tnode0 {
  public:

// DATA
  TreeDir::Dir			indicator;
  Tnode0*			Links[2]; // index will always be LEFT_DIR or RIGHT_DIR
  slist<keyClass, Tnode0, ownerData>* list;
  Tnode0*			next;
  Tnode0*			previous;
  slist<keyClass, Tnode0, ownerData>* saved_list; // to support temporary removal and reinsertion
  keyClass			Key;

// CONSTRUCTORS
  /* constructor */		Tnode0()
					: next(NULL),
					previous(NULL),
					list(NULL),
					saved_list(NULL),
					indicator(TreeDir::BALANCED) {
	Links[TreeDir::RIGHT_DIR] = NULL;
	Links[TreeDir::LEFT_DIR] = NULL;
  }
  /* comparison with Key */	Tnode0(const keyClass& a)
					: Key(a),
					next(NULL),
					previous(NULL),
					list(NULL),
					saved_list(NULL),
					indicator(TreeDir::BALANCED) {}
  /* copy constructor */	Tnode0(const Tnode0& Ap)
					: Key(Ap.Key),
					next(NULL),
					previous(NULL),
					list(NULL),
					saved_list(NULL),
					indicator(TreeDir::BALANCED) {}

  virtual			~Tnode0() {
	if(list) {
		list->remove_node(this);
		list = NULL;
	}
  }

// METHODS

  virtual Tnode0*		copy() {
	return new Tnode0(*this);
  }
  virtual Tnode0*		following_node() const;
  TreeDir::Dir			get_indicator() const {
	return indicator;
  }
  virtual const Cstring&	get_key() const {
	return Key.get_key();
  }
  virtual unsigned long&	get_2nd_key() {
	return Key.get_2nd_key();
  }
  Tnode0*			get_left() const {
	return Links[TreeDir::LEFT_DIR];
  }
  Tnode0*			get_link(TreeDir::Dir which) const {
	return Links[which];
  }
  int				get_pathlength(int& length);
  Tnode0*			get_right() const {
	return Links[TreeDir::RIGHT_DIR];
  }
  virtual const keyClass&	getKey() const {
	return Key;
  }
  bool				is_in_list(const slist<keyClass, Tnode0, ownerData>& l) const {
	return list == &l;
  }
  bool				has_a_list() const {
	return list != NULL;
  }
  void				make_root_of(slist<keyClass, Tnode0, ownerData>*);
  Tnode0*			next_node() const {
	return next;
  }
  Tnode0&			operator = (const Tnode0&) { return *this;
  }
  virtual backptr<Tnode0>*	Pointers() {
	return NULL;
  }
  virtual Tnode0*		preceding_node() const;
  virtual Tnode0*		previous_node() const {
	return previous;}
  void				reset_links() {
	indicator = TreeDir::BALANCED;
	Links[TreeDir::RIGHT_DIR] = NULL;
	Links[TreeDir::LEFT_DIR] = NULL;
  }

  Tnode0*			rotate();
				// made virtual so a derived class can disable by throwing
  virtual void			temporarily_remove_from_list() {
	if(saved_list != NULL || list == NULL) {
		throw(eval_error("cannot remove temporarily: list ptrs messed up"));
	}
	saved_list = list;
	list->remove_node(this);
  }
				// made virtual so a derived class can disable by throwing
  virtual void			re_insert_in_list() {
	if(list != NULL || saved_list == NULL) {
		throw(eval_error("cannot re-insert: list ptrs messed up"));
	}
	(*saved_list) << this;
	saved_list = NULL;
  }

  //
  // Methods intended to make get_linker() obsolete:
  //
  virtual void		add_cons(	TreeDir::Dir which_dir,
					const valueHolder& countVal,
					const valueHolder& integralVal) {
  }
  virtual void		add_cons_to_ancestors() {
  }
  virtual void		attach(		TreeDir::Dir right_or_left,
					Tnode0* node_to_attach,
					TreeDir::cons_adjust) {
	Links[right_or_left] = node_to_attach;
  }
  virtual void		attach_left(	Tnode0* node_to_attach,
					TreeDir::cons_adjust CA) {
	attach(TreeDir::LEFT_DIR, node_to_attach, CA);
  }
  virtual void		attach_right(	Tnode0* node_to_attach,
					TreeDir::cons_adjust CA) {
	attach(TreeDir::RIGHT_DIR, node_to_attach, CA);
  }
  virtual double	get_consumption() const {
	return 0.0;
  }
  virtual valueHolder	get_i_cons() const {
	return valueHolder(0L);
  }
  virtual double	get_integral() const {
	return 0.0;
  }
  virtual double	get_integral_left() const {
	return 0.0;
  }
  virtual double	get_integral_right() const {
	return 0.0;
  }
  virtual true_long	get_int_consumption() const {
	return 0;
  }
  virtual valueHolder	get_l_cons() const {
	return valueHolder(0L);
  }
  virtual valueHolder	get_r_cons() const {
	return valueHolder(0L);
  }
  virtual Tnode0*		get_parent() const {
	return NULL;
  }
  virtual void		init_cons() {
  }
  virtual bool		is_float() const {
	return false;
  }
  virtual void		set_i_cons(const valueHolder* vh) {
  }
  virtual void		set_parent(Tnode0* Par) {
  }
  virtual void		subtract_cons_from_ancestors(Tnode0* stop_here = NULL) {
  }
};	// end of Tnode0 class

template <class keyClass, class ownerData>
void
Tnode0<keyClass, ownerData>::make_root_of(slist<keyClass, Tnode0<keyClass, ownerData>, ownerData>* l) {
	l->make_root(this);
}

template <class keyClass, class ownerData>
Tnode0<keyClass, ownerData>*
Tnode0<keyClass, ownerData>::preceding_node() const {
	slist<keyClass, Tnode0<keyClass, ownerData>, ownerData>* L = list;

	if(!L) return NULL;
	return L->find_adjacent_left(this);
}

template <class keyClass, class ownerData>
Tnode0<keyClass, ownerData>*
Tnode0<keyClass, ownerData>::following_node() const {
	slist<keyClass, Tnode0<keyClass, ownerData>, ownerData>* L = list;

	if(!L) return NULL;
	return L->find_adjacent_right(this);
}

template <class keyClass, class ownerData>
Tnode0<keyClass, ownerData>*
Tnode0<keyClass, ownerData>::rotate() {
	TreeDir::Dir	t_direction, u_direction;
	Tnode0* new_root;

	if(indicator == TreeDir::BALANCED) {
		return NULL;
	}
	t_direction = indicator;
	u_direction = t_direction ? TreeDir::LEFT_DIR : TreeDir::RIGHT_DIR;

	new_root = Links[t_direction];
	if(new_root->indicator == u_direction) {
		new_root = new_root->rotate();
		attach(t_direction, new_root, TreeDir::UPDATE_CONS);
	}
	if(new_root->indicator == TreeDir::BALANCED)
		indicator = TreeDir::BALANCED;
	else
		indicator = u_direction;
	new_root->indicator = u_direction;
	attach(t_direction, new_root->Links[u_direction], TreeDir::UPDATE_CONS);
	new_root->attach(u_direction, this, TreeDir::UPDATE_CONS);
	return new_root;
}

//
// A version of Cnode without a linker
//
template <class keyClass, class PLD, class ownerData = short>
class Cnode0: public Dnode0<keyClass, Cnode0<keyClass, PLD, ownerData>, ownerData> {
	typedef Dnode0<keyClass, Cnode0<keyClass, PLD, ownerData>, ownerData> dbase;
public:
	Cnode0() {}
	Cnode0(const keyClass& x) : dbase(x) {}
	Cnode0(const keyClass& x, const PLD& y) : dbase(x), payload(y) {}
	Cnode0(const Cnode0& c) : dbase(c), payload(c.payload) {}
	~Cnode0() {
		if(dbase::list) {
			dbase::list->remove_node(this);
			dbase::list = NULL;
		}
	}

	Cnode0*	copy() {
		return new Cnode0(*this);
	}
	Cnode0*	get_this() {
		return this;
	}
	const Cnode0*	get_this() const {
		return this;
	}

	PLD	payload;
};

#endif /* _TNODE_TEMPLATE_H */
