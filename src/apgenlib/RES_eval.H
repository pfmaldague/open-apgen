#ifndef RES_EVAL_H
#define RES_EVAL_H

#include	"ExecutionContext.H"
#include	"Rsource.H"
// #include	<Program.H>

		// in ACT_exec:
extern blist&	theStandardSASFelementNames();
extern void	is_a_valid_number(const Cstring &S, double &result);

class APreport {
public:
	class actor {
	public:
		actor() {}
		actor(execution_context* ec) {}
		actor(pEsys::executableExp* I) {}
		actor(const actor& A) {}
		~actor() {}
	Cstring		content;
	};
	class event {
	public:
		event(const char* s)
			: name(s) {
		}
		~event() {
		}
		event& operator << (const actor& A) {
			return *this;
		}
		Cstring		name;
	};
	static bool	active;
};


enum INS_TYPE {
	// handled by usage_clause_node::execute() and usage_clause_node::execute_decomposition()
	INS_ACTIVITY,
	INS_DECOMPOSITION,

	// handled by usage_clause_node::execute() and usage_clause_node::execute_usage()
	INS_USAGE,
	INS_ABS_USAGE,
	INS_SIGNAL_USAGE,

	// handled by wait_clause_node::execute()
	INS_WAIT_SIMPLE,
	INS_WAIT_UNTIL,
	INS_WAIT_UNTIL_REGEXP,
	INS_WAIT_OK2SCHED,
	INS_GET_WINDOWS,
	INS_INTERP_WINDOWS,

	// handled by executableExp::execute()
	INS_ASSIGNMENT,
	INS_DECLARATION,
	INS_PARAMETER,
	INS_DISABLE_SCHEDULING,
	INS_ENABLE_SCHEDULING,
	INS_IF,
	INS_ELSE,
	INS_ELSE_IF,
	INS_WHILE,
	INS_RETURN
};

class exec_status {
public:
	typedef enum {
		NOOP,
		WAITING,
		PUSH,
		POP,
		RESUMING,
		REENTERING_FROM_EVENT,
		MODELING_INTERRUPTED
	}	stack_state;

	exec_status()
		: state(NOOP) {
	}
	exec_status(const exec_status& es)
		: state(es.state) {
	}
	~exec_status() {
	}

	const char* spell() {
		switch(state) {
			case NOOP:
				return "NOOP";
			case WAITING:
				return "WAITING";
			case PUSH:
				return "PUSH";
			case POP:
				return "POP";
			case RESUMING:
				return "RESUMING";
			case REENTERING_FROM_EVENT:
				return "REENTERING_FROM_EVENT";
			case MODELING_INTERRUPTED:
				return "MODELING_INTERRUPTED";
			default:
				break;
		}
		return "UNKNOWN";
	}

	stack_state	state;
};

#include <vector>

// int Register_udef_function_filter(const char*	name_of_file_containing_filters);

	/* It would not make sense to provide a aaf_intfc::Strategy argument
	 * to a udef function, because such functions do not (normally) have
	 * access to resources.  Of course someone could always write a user-
	 * defined function that does access specific resources but that would
	 * be stretching the interface... */
void Execute_udef_function(	const string&		functionName,
				TypedValue*		result,
				slst<TypedValue*>&	args);



#endif
