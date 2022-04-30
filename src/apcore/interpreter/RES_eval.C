#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ActivityInstance.H"
#include "apcoreWaiter.H"
#include "CompilerIntfc.H"
#include "ExecStack.H"
#include "RES_eval.H"
#include "ParsedExpressionSystem.H"

extern const char* get_apgen_version_build_platform();
extern Cstring	nonewlines(const Cstring&);

void		(*compiler_intfc::CompileExpression)(
					const Cstring&	Text,
					parsedExp&	opaqueParsedObject);
void		(*compiler_intfc::CompileStatement)(
					const Cstring&	Text,
					pEsys::Program*	opaqueParsedObject);
void		(*compiler_intfc::CompileExpressionList)(
					const Cstring&	Text,
					slist<alpha_int, Cnode0<alpha_int, parsedExp> >&	opaqueParsedObject);
				/* we certainly need some context here
				 * so local symbols can be obtained. Assume
				 * a behaving element is available. The
				 * opaqueParsedObject should store its own
				 * value. */
void		(*compiler_intfc::Evaluate)(
					parsedExp&	opaqueParsedObject,
					behaving_element& MA,
					TypedValue&	result);
				// no local context provided
void		(*compiler_intfc::EvaluateSelf)(
					parsedExp&	opaqueParsedObject,
					TypedValue&	result);

				/* NOTE: typedef'd functions cannot throw... */
void		(*compiler_intfc::CompileAndEvaluate)(
					const Cstring&	exp_text,
					behaving_element& MA,
					TypedValue&	result);
				// no local context provided
void		(*compiler_intfc::CompileAndEvaluateSelf)(
					const Cstring&	exp_text,
					TypedValue&	result);

using namespace std;

static Cstring&	TS() {
	static Cstring s;
	return s;
}	// Temporary String

bool CONdebug = false;
int CONindent = 0;

void CONprefix() {
	int i;
	for(i = 0; i < CONindent; i++) {
		cout << ' ';
	}
}

void CONadjust(int delta) {
	if(delta >= 0) {
		CONindent += delta;
		CONprefix();
		cout << "{\n";
	} else {
		CONprefix();
		cout << "}\n";
		CONindent += delta;
	}
}


Cstring execution_context::spell() {
	Cstring s;
	if(AmbientObject) {
		s << AmbientObject->Task.full_name();
	}
	return s;
}

pEsys::executableExp* execution_context::get_instruction() const {
	return counter ? counter->Prog->statements[counter->I].object() : NULL;
}

void execution_context::print(aoString& S, const char* prefix) {
	if(AmbientObject->print(S, prefix)) {
		return;
	}
}

Cstring	execution_context::get_descr() const {
	Cstring S;
	if(AmbientObject) {
		S << "Instruction " << get_instruction()->spell()	
			<< " in " << AmbientObject->Task.full_name()
			<< ", file " << get_instruction()->file
			<< ", line " << get_instruction()->line;
	} else {
		S << "Instruction " << get_instruction()->spell() 
			<< ", file " << get_instruction()->file
			<< ", line " << get_instruction()->line;
	}
	return S;
}

//
// Patterned after execution_stack::ExCon(), which handles simple
// contexts - i. e., contexts that cannot pause execution due to a new thread
// getting started
//
void	Cstack::Execute(execution_context::return_code& Code) {

    //
    // debug
    //
    // dbgind	indenter;

    //
    // There must be a context present...
    //
    execution_context*		current_context = EC.object();
    assert(EC.object());
    do {

	//
	// Programs are of three types:
	//
	// 	- simple programs, like function calls, which neither
	// 	  call nor spawn new dynamic contexts. Note that
	// 	  functions can spawn new function contexts, but
	// 	  these are not dynamic (they do not involve now at
	// 	  all).
	//
	// 	- programs that can spawn usages and threads. Both are
	// 	  dynamic, but they are basically new threads. These
	// 	  new threads MUST be started by creating a new stack
	// 	  and invoking Cstack::Execute(). These new threads
	// 	  can be entirely handled by context::Execute(); they
	// 	  do not need to come back to _this_ level.
	//
	// 	- programs that can call usages and threads. Again,
	// 	  both of these are dynamic. Spawns are simpler; they
	// 	  can be "fired and forgotten". Calls, however, MUST
	// 	  come back to _this_ level of execution after pushing
	// 	  a new context onto the stack. Note that all calls
	// 	  result in the stack being stored into a new
	// 	  expansion event, so we can return (WAITING case);
	// 	  the new event will take care of continuing execution
	// 	  of the present thread.
	//


	//
	// debug
	//
	// cerr << dbgind::indent() << "stack::Execute() - context = "
	// 	<< current_context->spell() << "\n";

	current_context->ExCon(Code, this);
	switch(Code) {
	    case execution_context::RETURNING:

		//
		// error - only function calls can return!
		//
		assert(false);

	    case execution_context::REGULAR:

		//
		// error - context should continue executing!
		//
		assert(false);

	    case execution_context::WAITING:

		//
		// the stack was stored in an event; we are
		// done for the time being
		//

		//
		// debug
		//
		// cerr << dbgind::indent() << "stack::Execute() - return (WAITING)\n";

		return;

	    case execution_context::FINISHED:

		//
		// the modeling/decomp program finished
		// execution; we should move up the stack
		//
		current_context = pop_context().object();
		if(current_context) {
		    current_context->counter->I++;
		    Code = execution_context::REGULAR;
		}

		//
		// debug
		//
		// cerr << dbgind::indent() << "stack::Execute() - FINISHED, popping context\n";

		break;
	    default:
		assert(false);
	}
    } while(current_context);

    //
    // debug
    //
    // cerr << dbgind::indent() << "stack::Execute() - return (exhausted contexts)\n";
}

Cstring Cstack::trace() const {
    aoString aos;
    execution_context* con = EC.object();
    aos << "Stack trace:\n";
    while(con) {
       aos << con->get_descr() << "\n";	
       con = con->prev.object();
    }
    return aos.str();
}

execution_context* Cstack::get_context() const {
    return EC.object();
}

long	Cstack::get_count() {
    return EC->depth();
}

void	Cstack::push_context(execution_context* new_ec) {
    execution_context* current = EC.object();

    if(current) {

	// debug
	// cerr << "push_context: current "
	// 	<< current->AmbientObject->Task.full_name()
	// 	<< ", new " << new_ec->AmbientObject->Task.full_name()
	// 	<< "\n";

	// current->next.reference(new_ec);
	new_ec->prev.reference(current);
    }
    EC.reference(new_ec);
}

smart_ptr<execution_context> Cstack::pop_context() {
    execution_context* current = EC.object();
    if(current) {
	execution_context* previous = current->prev.object();
	if(previous) {

	    //
	    // be careful - current->prev might be the only
	    // reference to previous, so if we dereference it, it
	    // will disappear. So, create an extra reference to
	    // make sure that does not happen:
	    //

	    smart_ptr<execution_context> sm(previous);

	    //
	    // Now we can get rid of current (to which EC holds
	    // the only reference)
	    //

	    EC.reference(previous);
	} else {

	    //
	    // Will delete current:
	    //

	    EC.dereference();
	}
    }
    return EC;
}

Cstack::~Cstack() {
}

void Cstack::dump(aoString& S, int pre) {
}

static stringtlist& DSL_items_to_track() {
    static stringtlist s;
    return s;
}

//
// filename should be taken from command-line argument list
//
int
Read_list_of_DSL_items_to_track(const char*    filename) {
    stringstream	s;
    FILE*		f = fopen(filename, "r");
    int			c;
    int			length = 0;

    if(!f) return -1;

    while(fread(&c, 1, 1, f)) {
	if(c == '\n') {
	    length = 0;
	    string t = s.str();
	    if(!DSL_items_to_track().find(t.c_str())) {
		DSL_items_to_track() << new emptySymbol(t.c_str());
		// reset the stringstream
		s.str("");
	    }
	} else {
	    length++;
	    s << c;
	}
    }
    if(length) {
	string t = s.str();
	if(!DSL_items_to_track().find(t.c_str())) {
	    DSL_items_to_track() << new emptySymbol(t.c_str());
	}
    }
    return 0;
}

void Execute_udef_function(
		const string&		functionName,
		TypedValue*		result,
		slst<TypedValue*>&	args) {
	Cstring		error, funcName(functionName.c_str());

	// 1. find the udef_function

	bfuncnode*	bf = aaf_intfc::internalAndUdefFunctions().find(funcName);

	if(!bf) {
		//throw exception if we couldn't find the function
		Cstring errors = "Execute_udef_function: user-defined function \"";
		errors << funcName << "\" not found";
		throw(eval_error(errors));
	}

	// 2. execute function

	if(bf->payload.Func(
				error,
				result,
				args) != apgen::RETURN_STATUS::SUCCESS) {
		throw(eval_error(error));
	}
}

void addInfluentialResources(
		const Cstring& aContainer,
		stringtlist& containers_queried) {
    RCsource* the_container = RCsource::resource_containers().find(aContainer);
    if(!the_container) {
	return;
    }
    stringslist::iterator reslst(
		    the_container->payload->names_of_containers_used_in_profile);
    emptySymbol* ptr;
    while((ptr = reslst())) {
	RCsource* influential = RCsource::resource_containers().find(
							ptr->get_key());
	if(!influential) {
	    return;
	}
	if(!containers_queried.find(influential->get_key())) {
	    containers_queried << new emptySymbol(influential->get_key());
	}
    }
}

bool	pEsys::pE::is_epoch_relative(Cstring& epoch_name) const {

    return 0;
}

int	is_an_arithmetic_function(generic_function F) {
    if(F == exp_NEG || F == exp_PLUS || F == exp_MINUS)
	return 1;
    return 0;
}

void is_a_valid_time(
		const Cstring& S,
		CTime_base& result) {
	parsedExp	tentatively_a_correctly_formed_time_string;
	parsedExp	temp;

	try {
		compiler_intfc::CompileExpression(S, temp);
	}
	catch(eval_error Err) {
		Cstring		msg;
		msg << "is_a_valid_time: error\n" << Err.msg;

		throw(eval_error(msg));
	}
	tentatively_a_correctly_formed_time_string = temp;
	TypedValue tv;
	tentatively_a_correctly_formed_time_string->eval_expression(
		behaving_object::GlobalObject(), tv);
	if(!tv.is_time()) {
		Cstring errs;
		tentatively_a_correctly_formed_time_string->print(errs);
		errs << " does not evaluate to a time value.";
		throw(eval_error(errs));
	}
	
	result = tv.get_time_or_duration();
}

void is_a_valid_duration(const Cstring& S, CTime_base& result) {
	parsedExp	tentatively_a_correctly_formed_time_string;
	parsedExp	temp;

	try {
		compiler_intfc::CompileExpression(S, temp);
	}
	catch(eval_error Err) {
		Cstring		msg;
		msg << "is_a_valid_duration: error\n" << Err.msg;

		throw(eval_error(msg));
	}
	tentatively_a_correctly_formed_time_string = temp;
	TypedValue tv;
	tentatively_a_correctly_formed_time_string->eval_expression(
		behaving_object::GlobalObject(), tv);
	if(!tv.is_duration()) {
		Cstring errs;
		tentatively_a_correctly_formed_time_string->print(errs);
		errs << " does not evaluate to a duration value.";
		throw(eval_error(errs));
	}
	
	result = tv.get_time_or_duration();
}

void is_a_valid_number(const Cstring &S, double &result) {
	parsedExp	tentatively_a_correctly_formed_numeric_string;
	parsedExp	temp;

	try {
		compiler_intfc::CompileExpression(S, temp);
	}
	catch(eval_error Err) {
		Cstring		msg;
		msg << "is_a_valid_number: error\n" << Err.msg;

		throw(eval_error(msg));
	}
	tentatively_a_correctly_formed_numeric_string = temp;
	TypedValue tv;
	tentatively_a_correctly_formed_numeric_string->eval_expression(
		behaving_object::GlobalObject(), tv);
	if(!tv.is_numeric()) {
		Cstring errs;
		tentatively_a_correctly_formed_numeric_string->print(errs);
		errs << " does not evaluate to a numeric value.";
		throw(eval_error(errs));
	}
	result = tv.get_double();
}


void	eval_intfc::get_all_selections(slist<alpha_void, smart_actptr>& l) {
    slist<alpha_void, smart_actptr>::iterator	li(sel_list());
    smart_actptr*	b;

    while((b = li())) {
	ActivityInstance* a = b->BP;
	l << new smart_actptr(a);
    }
}
