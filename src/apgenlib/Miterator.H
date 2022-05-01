#ifndef _MITERATOR_TEMPLATE_H_
#define _MITERATOR_TEMPLATE_H_


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

#endif /* _MITERATOR_TEMPLATE_H_ */
