#ifndef _EXEC_STACK_H_
#define _EXEC_STACK_H_

#include "ExecutionContext.H"

class instruction_node;
class EventLoop;

//
// Cstack = (execution) context stack
//
// Captures the state of a stack of calls to
// methods of specific instances
//
class Cstack {
public:

	//
	// The only data member of this class.
	// It is the top of a singly-linked
	// stack.
	//
	smart_ptr<execution_context>	EC;

	Cstack() {}
	Cstack(const Cstack&) = delete;
	Cstack(execution_context* ec) {
		EC.reference(ec);
	}

	~Cstack();

	// PUBLIC METHODS:
	void			push_context(execution_context* new_ec);
	smart_ptr<execution_context> pop_context();

	execution_context*	get_context() const;
	long			get_count();

	void			Execute(execution_context::return_code&);

	//
	// To produce stack traces as part of anomaly
	// documentation when an exception is thrown:
	//
	Cstring			trace() const;

	//
	// for debugging:
	//
	void			dump(aoString&, int prefix_size);
};

namespace pEsys {

//
// execStack is basically a singly-linked list of
// execution contexts. execution_context has all
// the data it needs to be a node; in particular,
// execution_context::prev points to the previous
// node in the list.
//
// The main functionality of the stack consists in
//
// 	- pushing a new context onto the stack
// 	- popping the current context off the stack
// 	- executing the stack, starting with the
// 	  current context (the one most recently
// 	  pushed.)
//
typedef Cstack execStack;

} // namespace pEsys


#endif /* _EXEC_STACK_H_ */
