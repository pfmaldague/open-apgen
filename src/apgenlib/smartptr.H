#ifndef _SMART_PTR_H_
#define _SMART_PTR_H_

#include <iostream>
#include "RES_exceptions.H"

extern thread_local int thread_index;

// Change July 6 2017: assume the reference count is in the object

template <class obj>
class smart_ptr {
public:
	obj*	p;

	//
	// debug - owner should be set to thread_index
	//
	int	owner;

	smart_ptr()
	    : p(NULL),
		owner(thread_index) {}

	//
	// Casting as non-const object is ugly but I don't
	// know how to avoid it with 'this' symbols:
	//
	smart_ptr(const obj* X)
	    : p((obj*)X),
		owner(thread_index) {
		p->ref();
	}
	smart_ptr(const smart_ptr& sp)
	    : p(sp.p),
		owner(thread_index) {
		if(p) {
		    p->ref();
		    // cerr << "copy constr, n = " << p->n << " for " << p->s->desc() << "\n";
		}
	}
	virtual ~smart_ptr() {
		dereference();
	}

	smart_ptr& operator=(const smart_ptr& sp) {
		// if(p && p == sp.p) {
			/* NO! *this and sp already point (consistently) to the
			 * same object. Nothing needs to change! */
			// p->ref();
		// }
		if(!p || p != sp.p) {
			dereference();
			p = sp.p;
			if(p) {
				p->ref();
				// cerr << "assignment, n = " << p->n << " for " << p->s->desc() << "\n";
			}
		}
		return *this;
	}

	void dereference() {
	    if(owner == thread_index) {
		if(p) {
			if(!p->deref()) {
				// std::cerr << "    deleting obj " << p->s->desc() << "\n";
				delete p;
			}
			// do this in all cases!!! (we don't want to decrement p->n ever again)
			p = NULL;
		}
	    } else {

		//
		// Follow the same logic as arrays: if a thread creates
		// an instance that is shared with other threads, it is
		// the instance owner's responsibility to keep the
		// instance valid as long as there is any chance that
		// another thread will attempt to read it.
		//
		;
	    }
	}

	void reference(obj* O) {
	    if(owner == thread_index) {
		dereference();
		if(O) {
			p = O;
			p->ref();
		}
	    }
	}

	obj* operator->() const {
		return p;
	}

	obj& operator*() {
		return *p;
	}

	const obj& operator*() const {
		return *p;
	}

	obj* object() const {
		return p;
	}

	bool operator!() const {
		return p == NULL;
	}

	operator void*() const {
		return p;
	}

	obj*	operator[](int k) {
		return p ? p->level(k) : NULL;
	}

};
		

#endif /* _SMART_PTR_H_ */
