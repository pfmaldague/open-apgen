#ifndef _EXECUTION_CONTEXT_H_
#define _EXECUTION_CONTEXT_H_

#include <EventBase.H>
#include <BehavingElement.H>

class Cstack;

namespace pEsys {
class executableExp;
class Program;
}

/* Used to be an abstract class implemented by four derived classes, one
 * for each type of object able to store symbols and execute instructions:
 *
 * 	- simple object
 * 		(used by APGen to perform mundane tasks like evaluate a
 * 		time expression)
 * 	- function
 * 		(used to execute AAF functions)
 * 	- activity
 * 		(conceptually, instances of an activity type)
 * 	- resource
 * 		(conceptually, instances of an abstract resource)
 *
 * After refactoring, execution_context now stands on its own, but it
 * has an Element - a behaving_element which points to a behaving_object,
 * an abstract class with four derived classes, one for each one of
 * the above objects.  Instead of overloading execution_context, we
 * have overloaded behaving_object - which is a better design, without
 * any doubt.
 */
class execution_context {
public:

    //
    // internal classes/enums
    //
	typedef enum {
		REGULAR,
		WAITING,
		RETURNING,
		CONTINUING,
		FINISHED
	} return_code;

	//
	// to support if, else and while statements:
	//
	typedef enum {
		TRUE_CONDITION_FOUND,
		CONDITION_IS_FALSE
	} context_state;

	//
	// provides recursive support for 'if', 'else', 'else if' and 'while':
	//
	class mini_stack {
	public:
		mini_stack(pEsys::Program* p)
			: Prog(p),
				I(0),
	       			prev(NULL),
	       			next(NULL),
	       			state(TRUE_CONDITION_FOUND) {
		}
		~mini_stack() {
			if(next) {
				delete next;
			}
		}
		int depth() {
			if(prev) {
				return prev->depth() + 1;
			}
			return 0;
		}
		pEsys::Program*		Prog;
		int			I;
		context_state		state;
		mini_stack*		prev;
		mini_stack*		next;
	};

    //
    // Class members
    //
	behaving_element	AmbientObject;

	//
	// provides recursive support for WaitFor, WaitUntil,
	// Decomp, AbstractUsage etc.:
	//
	smart_ptr<execution_context> prev;

	apgen::METHOD_TYPE	theSection;
	mini_stack*		counter;

	//
	// smart_ptr support:
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

    //
    // Constructors etc.
    //
	execution_context(
			behaving_element	El,
			pEsys::Program*		p,
			apgen::METHOD_TYPE	ws = apgen::METHOD_TYPE::NONE,
			bool			skip_wait = false
			)
		 : ref_count(0),
			AmbientObject(El),
			theSection(ws) {
		if(!p) {
			Cstring errs;
			errs << "Attempting to create execution context w/o program.";
			throw(eval_error(errs));
		}
		counter = new mini_stack(p);
		if(skip_wait) {
			counter->I++;
		}
	}

	execution_context(const execution_context&) = delete;

	//
	// The challenge in defining a void constructor that
	// works is that the Program pointer should always
	// be valid. When overriding this class with an adaptation-
	// specific version, the Program will be replaced by
	// explicit C++ code. A solution is to have a dummy,
	// static Program available to serve as a valid object
	// that the overridden instance can point to.
	//
	// Alternatively: be careful about not using a mini_stack
	// if the Program pointer is NULL. Also need to be careful
	// if we set counter to NULL, which would be the logical
	// thing to do.
	//
	execution_context() = delete;
	execution_context(
			const execution_context& EC,
			pEsys::Program* p)
		 : ref_count(0),
			AmbientObject(EC.AmbientObject),
			theSection(EC.theSection) {
		if(!p) {
			Cstring errs;
			errs << "Attempting to create execution context w/o program.";
			throw(eval_error(errs));
		}
		counter = new mini_stack(p);
	}


	virtual ~execution_context() {
		while(counter->prev) {
			counter = counter->prev;
		}
		delete counter;
		counter = NULL;

		assert(ref_count == 0);
	}

	Cstring			spell();

	//
	// Challenge: why do we provide a stack?
	//
	// Answer: to help when executing a behavioral
	// method that is able to WAIT. A WAIT instruction
	// can only be executed in an environment where a
	// stack is provided.
	//
	// This method made virtual 9/9/2020 to allow overriding
	// by adaptation-specific instances.
	//
	virtual void		ExCon(
					return_code& return_code,
					Cstack* = NULL);

	//
	// Used when an if or while condition is met
	// and the attached block needs to be executed:
	//
	void			HandleNewBlock(
					pEsys::Program* P);

	int				depth() const {
		if(prev) {
			return prev->depth() + 1;
		}
		return 0;
	}

	Cstring				get_descr() const;

	pEsys::executableExp*		get_instruction() const;

	void				print(
						aoString&,
						const char* prefix);
	apgen::METHOD_TYPE		what_section() {
		return theSection;
	}

};

//
// Suitable for executing ordinary functions:
//
class function_object: public behaving_object {
public:
	typedef enum {
		ENABLED = 0,
		PARENT  = 1
	} varindex;

	//
	// for functions:
	//
	function_object(
			TypedValue&	V,
			task&		T);

	//
	// for methods:
	//
	function_object(
			TypedValue&	V,
			task&		T,
			behaving_base*	parent);
	function_object(const function_object&) = delete;
	~function_object() {
	}

	virtual bool		print(aoString& s, const char* prefix) {
		s << prefix << "function object";
		return true;
	}

	virtual Cstring	get_id() const {
		return Task.name;
	}

	// access these using dynamic_cast:
	TypedValue&		theReturnedValue;
};

//
// Suitable for executing global methods:
//
class method_object: public behaving_object {
public:
	typedef enum {
		ENABLED = 0,
		PARENT  = 1
	} varindex;

	method_object(
			task&		T,
			behaving_base*	parent_object);
	method_object(const method_object&) = delete;
	~method_object() {
	}
};

class signal_object: public behaving_object {
public:
	static task&		SignalTask();

	signal_object(const Cstring& s);
	signal_object(const signal_object& SBE) = delete;

	~signal_object() {
	}

	virtual behaving_element get_parent() {
		behaving_element b;
		return b;
	}
};

/* A convenience for defining resource usage tasks, which
 * should be stored in the tasks vector of the corresponding
 * using object type - i. e., the type of the object that is
 * creating this resource usage */
class res_usage_task: public task {
public:
	res_usage_task(
			const Cstring& nm,
			Behavior& T)
		: task(nm, T, 2) {
		// we want to define variables for id, this, and enabled:

		//
		// no we don't
		//

		// add_variable("id", apgen::STRING);
		// add_variable("this", apgen::INSTANCE);
	}
	~res_usage_task() {}
};

class res_usage_object: public behaving_object {
private:
	Rsource*	concres;
public:
	res_usage_object(
		Rsource*,
		task& // make sure the above vars are defined
		);
	res_usage_object(const res_usage_object&) = delete;

	~res_usage_object() {
	}

	Rsource*	get_concrete_res() const override {
		return concres;
	}

	bool		print(
				aoString&,
				const char* prefix) const override;
};

class act_method_object: public behaving_object {
public:
	act_method_object(
		ActivityInstance*,
		task& // make sure the above vars are defined
		);

	act_method_object(const act_method_object&) = delete;

	~act_method_object() {
	}

	//
	// This method helps Usage statements find timing information:
	//
	int	level_where_start_is_defined() const override {

		//
		// start and finish are stored in the activity instance's object,
		// which is the parent of this object:
		//

		return 1;
	}
	bool			print(
					aoString&,
					const char* prefix) const override;
};

#endif /* _EXECUTION_CONTEXT_H_ */
