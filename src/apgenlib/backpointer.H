#ifndef _AP_BACK_POINTER_H_
#define _AP_BACK_POINTER_H_

#include <alphastring.H>
#include <templates.H>
#include <apDEBUG.H>

template <class BackP>
class backptr;

/* A base class for a smart pointer to a backpointer, i. e. an instance of class BackP.
 * Note that its destructor will destroy the entire smart pointer.
 *
 * The BackP class is assumed to contain a method Pointers() which returns a
 * pointer to a backptr<BackP> object; see the backptr template below. This object
 * contains pointers to all p2b's that point to it, so that appropriate notifications
 * can be sent when objects are created and destroyed.
 */
template <class BackP>
class p2b {
public:
  BackP*	BP;

// CONSTRUCTORS
  /* constructor */	p2b() : BP(NULL) {}		// don't use; p2b make no sense w/o ref to backptr
  /* constructor */	p2b(const p2b& P);		// do not inline: need to update the backptr's PointerNodes
  /* constructor */	p2b(BackP* const bp);		// do not inline: need to update the backptr's PointerNodes
  virtual		~p2b();				// do not inline: need to update the backptr's PointerNodes

  virtual bool			reacts_to_time_events() const = 0;
  virtual void			remove(bool shouldSave = true) = 0;
  virtual void			re_insert() = 0;

  BackP*	operator->() const {
	  return BP;
  }
  // basic introspection
  // virtual void*			voidThis() = 0;
};

/* Object that automatically deletes smart pointers to itself when destroyed. */
template <class BackP>
class backptr {
public:
	backptr() {}
	~backptr() { delete_pointers_to_this(); }

	class ptr2p2b: public Dnode0<alpha_void, ptr2p2b> {
	public:
		ptr2p2b() {}
		ptr2p2b(const ptr2p2b& P)
			: Dnode0<alpha_void, ptr2p2b>(P),
				payload(P.payload) {}
		ptr2p2b(p2b<BackP>* const bp)
			: Dnode0<alpha_void, ptr2p2b>((void*) bp),
				payload(bp) {}
		~ptr2p2b() {
			if(Dnode0<alpha_void, ptr2p2b>::list) {
				Dnode0<alpha_void, ptr2p2b>::list->remove_node(this);
			}
		}

					// implement pure virtual Dnode0 methods:
		ptr2p2b*		copy() { return new ptr2p2b(*this); }
  		ptr2p2b*		get_this() { return this; }
  		const ptr2p2b*		get_this() const { return this; }

		p2b<BackP>*		payload;
	};

	//
	// DATA
	//
	tlist<alpha_void, ptr2p2b>	pointers;

	//
	// METHODS
	//
	virtual void			delete_pointers_to_this();
	virtual void			temporarily_remove_pointers_to_this() {
		typename slist<alpha_void, ptr2p2b>::iterator	it(pointers);
		ptr2p2b*					n1;
		while((n1 = it())) {
			n1->payload->remove(true);
		}
	}
	virtual void			re_insert_pointers_to_this() {
		typename slist<alpha_void, ptr2p2b>::iterator	it(pointers);
		ptr2p2b*					n1;
		while((n1 = it())) {
			n1->payload->re_insert();
		}
	}
};

template <class BackP>
p2b<BackP>::p2b(const p2b<BackP>& P)
	: BP(P.BP) {

#	ifdef debug_templates
	DBG_NOINDENT("(creation) p2b(bottom level) adding self to list of PointerNodes ");
#	endif /* debug_templates */

	typename backptr<BackP>::ptr2p2b* p2 = new typename backptr<BackP>::ptr2p2b(this);
	BP->Pointers()->pointers << p2;

#	ifdef debug_templates
	DBG_NOPREFIX("which now has " << BP->Pointers()->pointers.get_length() << " node(s)\n");
	BP->Pointers()->pointers.check();
#	endif /* debug_templates */

	}

template <class BackP>
p2b<BackP>::p2b(BackP* const bp)
	: BP(bp) {

#	ifdef debug_templates
	DBG_NOINDENT("(creation) p2b(bottom level) adding self to list of PointerNodes ");
#	endif /* debug_templates */

	typename backptr<BackP>::ptr2p2b*	p2 = new typename backptr<BackP>::ptr2p2b(this);
	BP->Pointers()->pointers << p2;

#	ifdef debug_templates
	DBG_NOPREFIX("which now has " << BP->Pointers()->pointers.get_length() << " node(s)\n");
	BP->Pointers()->pointers.check();
#	endif /* debug_templates */

	}

template <class BackP>
p2b<BackP>::~p2b() {
	if(BP) {

#		ifdef debug_templates
		DBG_NOINDENT("(destruction) p2b(bottom level) looking for self in list of "
			<< BP->Pointers()->pointers.get_length() << " PointerNodes...\n");
#		endif /* debug_templates */

		typename backptr<BackP>::ptr2p2b*	p = BP->Pointers()->pointers.find((void*) this);
		if(p) {
			// assert(p->payload.smartPointer);
			// cout << "  found it!\n";
			// if(p->payload.smartPointer->get_node()) {
			// 	delete p->payload.smartPointer->get_node(); }
			delete p; }

#		ifdef debug_templates
		DBG_NOINDENT("    list of PointerNodes now has "
			<< BP->Pointers()->pointers.get_length() << " PointerNode(s) left.\n");
		BP->Pointers()->pointers.check();
#		endif /* debug_templates */

		}
	else {
#		ifdef debug_templates
		DBG_NOINDENT("(destruction) p2b has NULL BP - the BP must be disappearing.\n");
#		endif /* debug_templates */
	}
}

template <class BackP>
void backptr<BackP>::delete_pointers_to_this() {
	ptr2p2b* a;

#	ifdef debug_templates
	DBG_INDENT("BackP at " << ((void*)this) << " deleting pointers to itself...\n");
#	endif
	while((a = pointers.first_node())) {
		// do this before setting the BP to NULL:
#		ifdef debug_templates
		DBG_NOINDENT("ptr2p2b at " << ((void*)a) << ", payload at " << ((void*)a->payload) << " ...\n");
		pointers.check();
#		endif
		a->payload->remove(false);
		a->payload->BP = NULL;
		delete a->payload;
		delete a;
	}
#	ifdef debug_templates
	DBG_UNINDENT("BackP DONE deleting pointers to itself.\n");
#	endif
}

/* Backpointer that is also a Dnode. */
template <class keyClass, class PLD, class owner = short>
class BPnode: public Dnode0<keyClass, BPnode<keyClass, PLD, owner>, owner> {
public:

  typedef Dnode0<keyClass, BPnode, owner> nbase;

  BPnode*			copy() { return new BPnode(*this); }
  BPnode*			get_this() { return this; }
  const BPnode*			get_this() const { return this; }

  backptr<BPnode>		myBP;

				// method that is virtual in Dnode0:
  virtual backptr<BPnode>*	Pointers() { return &myBP; }

  /* main constructor */	BPnode(const keyClass& a, const PLD& p) : nbase(a), payload(p) {}
  /* auxiliary constructor */	BPnode(const PLD& p) : nbase(keyClass((void*) &p)), payload(p) {}
  /* comparison with strings */	BPnode(const keyClass& a) : nbase(a) {}
  /* copy constructor */	BPnode(const BPnode& Ap) : nbase(Ap), payload(Ap.payload) {}
  virtual			~BPnode() {
	myBP.delete_pointers_to_this();
	if(nbase::list) nbase::list->remove_node(this);
  }

  PLD				payload;
};

/* Template-based version of bpointernode. */
template <class keyClass, class masterObj, class owner = short>
class BPptr:	public Dnode0<keyClass, BPptr<keyClass, masterObj, owner>, owner>,
		public p2b<masterObj>
		{
public:

  typedef Dnode0<keyClass, BPptr<keyClass, masterObj, owner>, owner> nbase;
  typedef p2b<masterObj> bpbase;

  /* constructor */		BPptr() {}
  /* main constructor */	BPptr(const keyClass& a, masterObj* const p)
					: nbase(a), bpbase(p) {}
  /* auxiliary constructor */	BPptr(masterObj* const p)
					: nbase((void*) p), bpbase(p) {}
  /* copy constructor */	BPptr(const BPptr& Ap) : nbase(Ap), bpbase(Ap) {}
  virtual			~BPptr() { if(nbase::list) nbase::list->remove_node(this); }

  BPptr*			get_this() { return this; }
  const BPptr*			get_this() const { return this; }

  virtual BPptr*		copy() {
	  return new BPptr(*this);
  }

  // the pure virtual methods
  virtual void		remove(
				  bool should_save) {
#	ifdef debug_templates
	if(nbase::list) {
		nbase::list->check();
	}
#	endif /* debug_templates */

	if(should_save) {
		nbase::temporarily_remove_from_list();
	}
	else if(nbase::list) {
		nbase::list->remove_node(this);
	}

#	ifdef debug_templates
	if(nbase::list) {
		nbase::list->check();
	}
#	endif /* debug_templates */
}

  virtual bool		reacts_to_time_events() const {
	return nbase::getKey().reacts_to_time_events();
  }

  virtual void		re_insert() {
#	ifdef debug_templates
	if(nbase::list) {
		nbase::list->check();
	}
#	endif /* debug_templates */
	nbase::re_insert_in_list();
  }

  // virtual void*		voidThis() {
  // 				return this; }
};

/* Like BPptr but with room for a non-trivial payload */
template <class keyClass, class masterObj, class PLD>
class BPptr2:	public Dnode0<keyClass, BPptr2<keyClass, masterObj, PLD> >,
		public p2b<masterObj> {
public:

  typedef Dnode0<keyClass, BPptr2<keyClass, masterObj, PLD> > nbase;
  typedef p2b<masterObj> bpbase;

  /* void */		BPptr2() {}
  /* main */		BPptr2(const keyClass& a, masterObj* const p, const PLD& pld)
					: nbase(a), payload(pld), bpbase(p) {}
  /* auxiliary */	BPptr2(masterObj* const p)
					: nbase((void*) p), bpbase(p) {}
  /* copy */		BPptr2(const BPptr2& Ap) : nbase(Ap), payload(Ap.payload), bpbase(Ap) {}
  virtual		~BPptr2() { if(nbase::list) nbase::list->remove_node(this); }

  virtual BPptr2*	copy() { return new BPptr2(*this); }
  BPptr2*		get_this() { return this; }
  const BPptr2*		get_this() const { return this; }
  virtual bool		reacts_to_time_events() const {
				return nbase::getKey().reacts_to_time_events(); }

  // the pure virtual methods
  virtual void		remove(bool should_save) {
#				ifdef debug_templates
				if(nbase::list) {
					nbase::list->check(); }
#				endif /* debug_templates */
				if(should_save) {
					nbase::temporarily_remove_from_list(); }
				else if(nbase::list) {
					nbase::list->remove_node(this); } }
  virtual void		re_insert() {
				nbase::re_insert_in_list();
#				ifdef debug_templates
				if(nbase::list) {
					nbase::list->check(); }
#				endif /* debug_templates */
				}
  // virtual void*		voidThis() { return this; }
  PLD			payload;
};

#endif /* _AP_BACK_POINTER_H_ */
