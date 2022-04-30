#ifndef _TIMEPTR_H_
#define _TIMEPTR_H_

#include <apDEBUG.H>
#include <templates.H>
#include <alphatime.H>	// includes backpointer.H which includes slist.H and apDEBUG.H

//
// first we define a pointer-based time class suitable as the key of a node that
// gets its time from something else. Should this be a template? Well, yes - we
// will want one template class parameter, the most fully derived time node that
// we get time from. Let's call that class the masterClass.
//
// The only assumption about masterClass is that it has a getKey().getetime() method.
//
// Make sure to allow for a time offset.
//
template <class masterClass>
class timeptr {
// DATA
  unsigned long		secondaryKey;
  CTime_base		offset;
public:
  masterClass*		mC; // provides time through its key

//
// CONSTRUCTORS
//
 
  //
  // timeptr-specific constructor
  //
  timeptr(masterClass* M) : mC(M), secondaryKey(0), offset(CTime_base(0, 0, true)) { assert(mC); }

  // the remaining constructors (all trivial) provide what Tnode expects of its keyClass:

  timeptr(const Cstring& s) : mC(NULL), secondaryKey(0), offset(CTime_base(0, 0, true)) { assert(mC); }
  timeptr(const char* s) : mC(NULL), secondaryKey(0), offset(CTime_base(0, 0, true)) { assert(mC); }
  timeptr(long d) : mC(NULL), secondaryKey(0), offset(CTime_base(0, 0, true)) { assert(mC); }
  timeptr(int s, int m) : mC(NULL), secondaryKey(0), offset(CTime_base(0, 0, true)) { assert(mC); }
  timeptr(void* v) : mC(NULL), secondaryKey(0), offset(CTime_base(0, 0, true)) { assert(mC); }
  timeptr(const timeptr& tp) : mC(tp.mC), /* this is the correct behavior: */ secondaryKey(0), offset(CTime_base(0, 0, true)) { assert(mC); }
  virtual		~timeptr() { assert(mC); }

//
// METHODS
//

  int			relTime(const CTime_base& t1, const CTime_base& t2) const {
				if(t1 > t2) return 1;
				else if(t1 == t2) return 0;
				return -1; }
  bool			operator > (const timeptr& as) const {
				assert(mC);
				int f = relTime(getetime(), as.getetime());
				if(!f) { return secondaryKey > as.secondaryKey; }
				return f > 0; }
  bool			operator < (const timeptr& as) const {
				assert(mC);
				int f = relTime(getetime(), as.getetime());
				if(!f) { return secondaryKey < as.secondaryKey; }

				return f < 0; }
  bool			operator == (const timeptr& as) const {
				assert(mC);
				int f = relTime(getetime(), as.getetime());
				if(!f) { return secondaryKey == as.secondaryKey; }
				return !f; }
  bool			operator != (const timeptr& as) const {
				assert(mC);
				int f = relTime(getetime(), as.getetime());
				if(!f) { return secondaryKey != as.secondaryKey; }
				return f != 0; }
  int			compare_to(const timeptr& that, bool ignoreSecKey = false) const {
				assert(mC);
				int f = relTime(getetime(), that.getetime());
				if(!f) {
					if(!ignoreSecKey) {
						if(secondaryKey > that.secondaryKey) {
							return 1; }
						else if(secondaryKey < that.secondaryKey) {
							return -1; }
						return 0; }
					else {
						return 0; } }
				return f; }
  static bool		disambiguates() { return true; }
  void			disambiguate(unsigned long& currMaxSecKey) { secondaryKey = ++currMaxSecKey; }
  const Cstring&	get_key() const { static Cstring s; s = getetime().to_string(); return s; }
  unsigned long&	get_2nd_key() { return secondaryKey; }
  Cstring		get_id() const {
	return Cstring(getetime().to_string());
  }
  CTime_base		getetime() const {
	return mC->getKey().getetime() + offset;
  }
			/* probably should remove/re-insert from list - that method cannot be put in the Tnode
			 * that uses this key (the "time pointer node"), unless we make it a universal method
			 * in Tnode... ugly.
			 *
			 * Better solution: the offset depends on a Symbol Node at some level. That's the node
			 * that 'owns' the offset. It should 'know' that kind of nodes depend on its time
			 * value, and take appropriate action. I like that.
			 *
			 * But then, the time pointer node is a pointer to two back pointers: the master time
			 * node, and the master offset symbol. Can we support multiple dependencies of this type?
			 * 
			 * Answer: it's not necessary; in reality, the main case is that of pointer nodes pointing
			 * to activity instances. Instances 'know' to take care of pointers to them, presumably...
			 * Actually, they would need to know whether or not a given pointer has an offset; so yes,
			 * it might be better to have double dependencies after all. */
  void			setoffset(const CTime_base& s) { assert(s.is_duration()); offset = s; }
  const CTime_base&	getoffset() const { return offset; }
  static bool		reacts_to_time_events() { return true; }
};

/* the following class is similar to BPptr but uses the time of the Obj to define its own
 *
 * Assumptions: masterObj has a const method called Key.getetime() that returns a time
 * 		masterObj has a const method called get_key() that returns a const Cstring reference
 */
template <class masterObj, class PLD, class owner = short>
class TMptr:	public Dnode0<timeptr<masterObj>, TMptr<masterObj, PLD, owner>, owner>,
		public p2b<masterObj> {
public:

  typedef Dnode0<timeptr<masterObj>, TMptr<masterObj, PLD, owner>, owner> nbase;
  typedef p2b<masterObj> bpbase;

  // don't use
  TMptr() = delete;
  TMptr(const TMptr& t) : nbase(t), bpbase(t), payload(new PLD(*t.payload)), offsetRef(t.offsetRef) {}
  TMptr(masterObj* p, const CTime_base& off, PLD* q) : nbase(timeptr<masterObj>(p)), payload(q), bpbase(p), offsetRef(off) {
			nbase::Key.setoffset(offsetRef); }
  ~TMptr() {
			if(nbase::list) {
				nbase::list->remove_node(this); }
			if(payload) {
				delete payload;
				payload = NULL; }
			}

  TMptr*		copy() { return new TMptr(*this); }
  TMptr*		get_this() { return this; }
  const TMptr*		get_this() const { return this; }

  const Cstring&	get_key() const {
	if(bpbase::BP) {
		static Cstring s;
		s.undefine();
		s << nbase::getKey().get_key() << " - " << bpbase::BP->get_key();
		return s;
	} else {
		return Cstring::null();
	}
  }
  void			remove(bool should_save) {
				if(should_save) {
					temporarily_remove_from_list(); }
				else if(nbase::list){
					nbase::list->remove_node(this); } }
  void			temporarily_remove_from_list() {
				assert(nbase::saved_list == NULL);
#				ifdef debug_templates
				slist<timeptr<masterObj>, TMptr, owner>* save_the_list = nbase::list;
				if(nbase::list) {
					nbase::list->check(); }
#				endif /* debug_templates */

				nbase::temporarily_remove_from_list();
#				ifdef debug_templates
				if(save_the_list) {
					save_the_list->check();
				}
#				endif /* debug_templates */
			}
  bool			reacts_to_time_events() const {
				return nbase::Key.reacts_to_time_events(); }
  void			re_insert() {
				re_insert_in_list(); }
  void			re_insert_in_list() {
#				ifdef debug_templates
				if(nbase::list) {
					nbase::list->check(); }
#				endif /* debug_templates */
				// the master may have changed the offset value
				nbase::Key.setoffset(offsetRef); 
				nbase::re_insert_in_list();
				assert(nbase::saved_list == NULL);
	 		}
  void*			voidThis() { return this; }
  const CTime_base&	offsetRef;
  PLD*			payload;
};

/* the following class is similar to TMptr but has no payload
 *
 * Assumptions: masterObj has a const method called Key.getetime() that returns a time
 * 		masterObj has a const method called get_key() that returns a const Cstring reference
 */
template <class masterObj, class owner = short>
class Tptr:	public Dnode0<timeptr<masterObj>, Tptr<masterObj, owner>, owner>,
		public p2b<masterObj> {
public:

  typedef Dnode0<timeptr<masterObj>, Tptr<masterObj, owner>, owner> nbase;
  typedef p2b<masterObj> bpbase;

  // don't use
  Tptr() = delete;
  Tptr(const Tptr& t)
	: nbase(t),
		bpbase(t),
		offsetRef(t.offsetRef) {}
  Tptr(masterObj* p, const CTime_base& off)
	: nbase(timeptr<masterObj>(p)),
		bpbase(p),
		offsetRef(off) {
	nbase::Key.setoffset(offsetRef);
  }
  Tptr(masterObj* p)
	: nbase(timeptr<masterObj>(p)),
		bpbase(p),
		offsetRef(nullOffset()) {
	nbase::Key.setoffset(offsetRef);
  }
  ~Tptr() {
	if(nbase::list) {
		nbase::list->remove_node(this);
	}
  }

  Tptr*			copy() { return new Tptr(*this); }
  Tptr*			get_this() { return this; }
  const Tptr*		get_this() const { return this; }

  const Cstring&	get_key() const {
				if(bpbase::BP) {
					static Cstring s;
					s.undefine();
					s << nbase::getKey().get_key() << " - " << bpbase::BP->get_key();
					return s; }
				else {
					return Cstring::null(); } }
  void			remove(bool should_save) {
				if(should_save) {
					temporarily_remove_from_list(); }
				else if(nbase::list){
					nbase::list->remove_node(this); } }
  void			temporarily_remove_from_list() {
				assert(nbase::saved_list == NULL);
#				ifdef debug_templates
				slist<timeptr<masterObj>, Tptr, owner>* save_the_list = nbase::list;
				if(nbase::list) {
					nbase::list->check(); }
#				endif /* debug_templates */

				nbase::temporarily_remove_from_list();
#				ifdef debug_templates
				if(save_the_list) {
					save_the_list->check(); }
#				endif /* debug_templates */
			}
  bool			reacts_to_time_events() const {
				return nbase::Key.reacts_to_time_events(); }
  void			re_insert() {
				re_insert_in_list(); }
  void			re_insert_in_list() {
				assert(nbase::list == NULL);
#				ifdef debug_templates
				if(nbase::saved_list) {
					nbase::saved_list->check(); }
#				endif /* debug_templates */
				// the master may have changed the offset value
				nbase::Key.setoffset(offsetRef); 
				nbase::re_insert_in_list();
#				ifdef debug_templates
				nbase::list->check();
#				endif /* debug_templates */
				assert(nbase::saved_list == NULL);
	 		}
  void*			voidThis() { return this; }
  const CTime_base&	offsetRef;
  const CTime_base&	nullOffset() const { static CTime_base s(0, 0, true); return s; }
};

/* similar to timeptr but works for any time-based Key; no offset possible.
 * Suitable for multiterator threads. It would have been nice if the
 * secondaryKey could have played the role of a priority, but unfortunately
 * slist makes special assumptions about it and so we have to define a new
 * priority field.
 *
 * NOTE: the only requirements on masterClass is that it should have a
 * getNode() method, and the latter should return a Key that meets the
 * requirements of the slist and tlist classes. */
template <class masterClass>
class relptr {
// DATA
  unsigned long		secondaryKey;
  unsigned long		priority;
public:
  masterClass*		mC; // provides key

//
// CONSTRUCTORS
//
  relptr(masterClass* M, unsigned long p) : mC(M), priority(p), secondaryKey(0) {}

  //
  // the remaining constructors (all trivial) provide what Tnode expects of its keyClass:
  //
  relptr(const Cstring& s) : priority(0), mC(NULL), secondaryKey(0) {}
  relptr(const char* s) : priority(0), mC(NULL), secondaryKey(0) {}
  relptr(long d) : priority(0), mC(NULL), secondaryKey(0) {}
  relptr(int s, int m) : priority(0), mC(NULL), secondaryKey(0) {}
  relptr(void* v) : priority(0), mC(NULL), secondaryKey(0) {}
  relptr(const relptr& tp) : priority(tp.priority), mC(tp.mC), secondaryKey(0) {}
  relptr() : priority(0), mC(NULL), secondaryKey(0) {}

  virtual		~relptr() {}

//
// METHODS
//

  // way too specialized:
  // const CTime_base	getetime() const { return mC->getNode()->getKey().getetime(); }

  int			relTime(masterClass* const t1, masterClass* const t2) const {
	return ((void*)t1->L) == ((void*)t2->L) ?

	    //
	    // They belong to the same list
	    //
	    (t1->iterNode ?
	        (t2->iterNode ?
		    (t1->iterNode->Key > t2->iterNode->Key ?
			1
			: (t2->iterNode->Key == t2->iterNode->Key ?
				0
				: -1
			)
		    )

		    //
		    // exhausted iterator is later
		    //
		    : -1
		)
		: (t2->iterNode ?
		    1

		    //
		    // both exhausted
		    //
		    : 0
		)
	    )

	    //
	    // They belong to different lists
	    //
	    : (t1->iterNode ?
	        (t2->iterNode ?
		    (t1->iterNode->Key.getetime() > t2->iterNode->Key.getetime() ?
			1
		        : (t1->iterNode->Key.getetime() == t2->iterNode->Key.getetime() ?
			    0
			    : -1
			)
		    )

		    //
		    // exhausted iterator is later
		    //
		    : -1
		)
		: (t2->iterNode ?
		    1

		    //
		    // both exhausted
		    //
		    : 0
		)
	    );
  }
  bool			operator > (const relptr& as) const {
				assert(mC);
				int f = relTime(mC, as.mC);
				if(!f) {
					if(priority == as.priority) {
						return secondaryKey > as.secondaryKey; }
					else {
						return priority > as.priority; } }
				return f > 0; }
  bool			operator < (const relptr& as) const {
				assert(mC);
				int f = relTime(mC, as.mC);
				if(!f) {
					if(priority == as.priority) {
						return secondaryKey < as.secondaryKey; }
					else {
						return priority < as.priority; } }

				return f < 0; }
  bool			operator == (const relptr& as) const {
				assert(mC);
				int f = relTime(mC, as.mC);
				if(!f) {
					if(priority == as.priority) {
						return secondaryKey == as.secondaryKey; }
					else {
						return false; } }
				return false; }
  bool			operator != (const relptr& as) const {
				assert(mC);
				int f = relTime(mC, as.mC);
				if(!f) {
					if(priority == as.priority) {
						return secondaryKey != as.secondaryKey; }
					else {
						return true; } }
				return true; }
  int			compare_to(const relptr& that, bool ignoreSecKey = false) const {
				assert(mC);
				int f = relTime(mC, that.mC);
				if(!f) {
					if(priority == that.priority) {
						if(!ignoreSecKey) {
							if(secondaryKey < that.secondaryKey) {
								return -1; }
							else if(secondaryKey > that.secondaryKey) {
								return 1; }
							return 0; }
						else {
							return 0; } }
					else if(priority > that.priority) {
						return 1; }
					else if(priority < that.priority) {
						return -1; }
					return 0; }
				return f; }
  static bool		disambiguates() { return true; }
  void			disambiguate(unsigned long& currMaxSecKey) { secondaryKey = ++currMaxSecKey; }
  const Cstring&	get_key() const {
	return mC->getNode() ?
		mC->getNode()->get_key()
		: Cstring::null();
  }
  unsigned long&	get_2nd_key() { return secondaryKey; }
  unsigned long		get_priority() const { return priority; }
  void			set_priority(const long& p) { priority = p; }
  Cstring		get_id() const {
	return mC->getNode() ?
		mC->getNode()->get_key()
		: Cstring();
  }
  static bool		reacts_to_time_events() { return true; }
};

/* similar to relptr but works for any int-based Key; no offset possible.
 * Suitable for Interator threads. It would have been nice if the
 * secondaryKey could have played the role of a priority, but unfortunately
 * slist makes special assumptions about it and so we have to define a new
 * priority field.
 *
 * NOTE: the only requirements on masterClass is that it should have a
 * getNode() method, and the latter should return a Key that meets the
 * requirements of the slist and tlist classes. */
template <class masterClass>
class int_relptr {
// DATA
  unsigned long		secondaryKey;
  unsigned long		priority;
public:
  masterClass*		mC; // provides key

//
// CONSTRUCTORS
//
  int_relptr(masterClass* M, unsigned long p) : mC(M), priority(p), secondaryKey(0) {}

  //
  // the remaining constructors (all trivial) provide what Tnode expects of its keyClass:
  //
  int_relptr(const Cstring& s) : priority(0), mC(NULL), secondaryKey(0) {}
  int_relptr(const char* s) : priority(0), mC(NULL), secondaryKey(0) {}
  int_relptr(long d) : priority(0), mC(NULL), secondaryKey(0) {}
  int_relptr(int s, int m) : priority(0), mC(NULL), secondaryKey(0) {}
  int_relptr(void* v) : priority(0), mC(NULL), secondaryKey(0) {}
  int_relptr(const int_relptr& tp) : priority(tp.priority), mC(tp.mC), secondaryKey(0) {}
  int_relptr() : priority(0), mC(NULL), secondaryKey(0) {}

  virtual		~int_relptr() {}

// METHODS

  int		relInteger(masterClass* const t1, masterClass* const t2) const {
	if(t1->get_void_list() == t2->get_void_list()) {
	    if(t1->getNode()) {
		if(t2->getNode()) {
			if(t1->getNode()->getKey() > t2->getNode()->getKey()) {
				return 1;
			} else if(t1->getNode()->getKey() == t2->getNode()->getKey()) {
				return 0;
			}
			return -1;
		}
		// exhausted iterator is later
		return -1;
	    }
	    if(t2->getNode()) {
		return 1;
	    }
	    // both exhausted
	    return 0;
	} else {
	    if(t1->getNode()) {
		if(t2->getNode()) {
			if(t1->getNode()->getKey().get_int() > t2->getNode()->getKey().get_int()) {
				return 1;
			} else if(t1->getNode()->getKey().get_int() == t2->getNode()->getKey().get_int()) {
				return 0;
			}
			return -1;
		}
		// exhausted iterator is later
		return -1;
	    }
	    if(t2->getNode()) {
		return 1;
	    }
	    // both exhausted
	    return 0;
	}
	return -1;
  }
  bool		operator > (const int_relptr& as) const {
	assert(mC);
	int f = relInteger(mC, as.mC);
	if(!f) {
		if(priority == as.priority) {
			return secondaryKey > as.secondaryKey;
		} else {
			return priority > as.priority;
		}
	}
	return f > 0;
  }
  bool	operator < (const int_relptr& as) const {
	assert(mC);
	int f = relInteger(mC, as.mC);
	if(!f) {
		if(priority == as.priority) {
			return secondaryKey < as.secondaryKey;
		} else {
			return priority < as.priority;
		}
	}

	return f < 0;
  }
  bool			operator == (const int_relptr& as) const {
	assert(mC);
	int f = relInteger(mC, as.mC);
	if(!f) {
		if(priority == as.priority) {
			return secondaryKey == as.secondaryKey;
		} else {
			return false;
		}
	}
	return false;
  }
  bool		operator != (const int_relptr& as) const {
	assert(mC);
	int f = relInteger(mC, as.mC);
	if(!f) {
		if(priority == as.priority) {
			return secondaryKey != as.secondaryKey;
		} else {
			return true;
		}
	}
	return true;
  }
  int		compare_to(const int_relptr& that, bool ignoreSecKey = false) const {
	assert(mC);
	int f = relInteger(mC, that.mC);
	if(!f) {
		if(priority == that.priority) {
			if(!ignoreSecKey) {
				if(secondaryKey < that.secondaryKey) {
					return -1;
				} else if(secondaryKey > that.secondaryKey) {
					return 1;
				}
				return 0;
			} else {
				return 0;
			}
		} else if(priority > that.priority) {
			return 1;
		} else if(priority < that.priority) {
			return -1;
		}
		return 0;
	}
	return f;
  }
  static bool		disambiguates() {
	  return true;
  }
  void			disambiguate(unsigned long& currMaxSecKey) {
	  secondaryKey = ++currMaxSecKey;
  }
  const Cstring&	get_key() const {
	  return mC->getNode() ?
		mC->getNode()->get_key()
		: Cstring::null();
  }
  unsigned long&	get_2nd_key() {
	  return secondaryKey;
  }
  unsigned long		get_priority() const {
	  return priority;
  }
  void			set_priority(const long& p) {
	  priority = p;
  }
  Cstring		get_id() const {
	  return mC->getNode() ?
		mC->getNode()->get_key()
		: Cstring();
  }
  static bool		reacts_to_time_events() { return true; }
};

#endif /* _TIMEPTR_H_ */
