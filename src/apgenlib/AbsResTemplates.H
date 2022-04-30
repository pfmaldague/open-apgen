#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <assert.h>

#include "action_request.H"
#include "ActivityInstance.H"
#include "APmodel.H"
#include "apcoreWaiter.H"
#include "RES_eval.H"
#include "RES_def.H" // for create/destroy programs
#include "EventImpl.H"
#include "Scheduler.H"

using namespace std;
using namespace pEsys;

//
// Note on the structure of the "system" based on compiled
// adaptation code.
//
// The obvious minimum requirement for incorporating adaptation-
// specific abstract resources (and other things as well) is
// that Usage expressions should invoke the compiled AAF methods
// instead of the pE-based method execution_context::ExCon().
//

//
// In the templates implemented below, use_abs_res_obj
// is an adaptation-specific behaving_object which supports
// the 'use' task of the abstract resource.
//
template <class use_abs_res_obj>
class abstract_context: public execution_context {
public:
    //
    // We need to define a void constructor for
    // execution_context, since we won't need its
    // members - except for the prev smart pointer,
    // which is managed by pEsys::execStack.
    //
    abstract_context(	smart_ptr<use_abs_res_obj>&	ptr,
			Program*			P,
			apgen::METHOD_TYPE		mt)
	: execution_context(behaving_element(ptr.object()),
			    P,
			    mt) {}

    void		ExCon(
			    execution_context::return_code& return_code,
			    pEsys::execStack* S = NULL) override {
	AmbientObject->Task.start_timer();

	//
	// No! This grows an infinite stack!
	// You need to import the code from ExCon.
	//
	// ExCon(return_code, S);
	execution_context::ExCon(return_code, S);
	AmbientObject->Task.stop_timer();
    }
};

template <class use_abs_res_obj>
class AbsUsageTemplate: public AbstractUsage {
public:
	AbsUsageTemplate(const Usage& u) : AbstractUsage(u) {}
	AbsUsageTemplate(bool, const Usage& u) : AbstractUsage(true, u) {}
	~AbsUsageTemplate() {}

	virtual void execute(
		execution_context*		context,
		execution_context::return_code& Code,
		execStack*			stack_to_use = NULL) override;
};

//
// The most important jobs of the AbstractUsage expression are
//
//	1. create an adaptation-specific instance of
//	   behaving_object that holds all the variables
//	   defined in the usage function of the abstract
//	   resource
//
// 	2. perform the execute() method, which is overridden
//	   by the following method of AbsUsageTemplate.
//
// The execute() method is invoked from execution_context::ExCon(),
// and nowhere else. That method is overridden in the abstract
// context template, defined above in this header file.
//
// In terms of tracking wall clock time, this method handles
// the handover from the ambient abstract resource to  the
// one being called. It is still the responsibility of
// execution_context::ExCon() to start and stop time counters
// at the beginning and end of execution, respectively.
//
template <class use_abs_res_obj>
void AbsUsageTemplate<use_abs_res_obj>::execute(
		execution_context*		context,
		execution_context::return_code& Code,
		execStack*			stack_to_use) {

    //
    // static_abstract_object is declared as a pointer to a
    // behaving_object in the generated header file
    // gen-parsedExp.H. In reality, it is an instance
    // of use_abs_res_obj<...>:
    //
    //		behaving_base
    //		    \
    //		 behaving_object
    //		      \
    //		   abs_res_object
    //		        \
    //		     use_abs_res_obj<adaptation>
    //

    //
    // ambient_object is the object that called us:
    //
    behaving_base*	ambient_object = context->AmbientObject.object();

    const Behavior&	targetType = *abs_type;

    //
    // NOTE: theTaskIndex is always 1 for the 'use' task of an abstract resource
    //
    task*		use_task = targetType.tasks[theTaskIndex];

    //
    // If we are currently executing a higher-level abstract
    // resource, we should stop its wall clock before we
    // start this one
    //
    ambient_object->Task.stop_timer();
    use_task->called_once();
    use_task->start_timer();

    Behavior&		sourceType = ambient_object->Task.Type;
    task*		storage_task = (theStorageTaskIndex >= 0) ?
				sourceType.tasks[theStorageTaskIndex] : NULL;

    try {

	//
	// This function pointer is based on the type of timing
	// clause found in the statement.  The function it points
	// to sets an array that is 'cast in stone', i. e., the
	// (CTime_base) usage_times[] member of the Usage expression.
	//
	// See the constructor-specifics file in apcore/parser.
	//
	((*this).*eval_usage_times)(ambient_object);

	// NOTE: should define finish if available from temporal spec

	time_saver		ts;

	//
	// In our case, used_object will encapsulate an instance
	// of use_abs_res_obj
	//
	smart_ptr<use_abs_res_obj>	used_object;
	behaving_base*			storage = NULL;
	bool				from_to_style = usage_times.size() > 1;

	//
	// This loop was meant to support the 'every ... from ... to'
	// timing specification, which is obsolete.
	//
	// for(int k = 0; k < usage_times.size(); k++)
	int k = 0;
	ts.set_now_to(usage_times[k]);
	if(CreatingEventsInThePast(this)) {
	    return;
	}

	//
	// Here, static_abstract_resource refers to a global instance
	// of the abstract resource which contains the 'class scope'
	// variables (static variables, in C++ terminology.)
	//
	// The used_object variable, which is basically being
	// allocated here, supports the 'method scope'
	// which is appropriate for the usage method.
	//
	// Among other things, it should contain C++ references
	// to all the local variables defined in the behaving_object
	// base class.
	//
	abs_res_object* the_constructor_obj = new abs_res_object(
			*abs_type->tasks[0],
			context->AmbientObject);
	used_object.reference(new use_abs_res_obj(
					targetType,
					the_constructor_obj,
					context->AmbientObject));

	//
	// We may not need to use actual_arguments[];
	// us adaptation-specific variables instead.
	// The overloaded Program can learn to make
	// use of them.
	//
	if(actual_arguments.size()) {
	    if(storage_task) {
		storage = new behaving_object(*storage_task);
		behaving_object::var_should_be_stored = true;
		behaving_object::temporary_storage = storage;
	    }
	    for(int j = 0; j < actual_arguments.size(); j++) {
		int param_index = use_task->paramindex[j];
		actual_arguments[j]->eval_expression(
				ambient_object,
				(*used_object)[param_index]);
	    }
	    if(storage_task) {
		delete storage;
		behaving_object::temporary_storage = NULL;
		behaving_object::var_should_be_stored = false;
	    }
	} else {
	    storage = NULL;
	}

	//
	// now create a new thread event. If we are spawning
	// events (meaning that we are in the midst of
	// executing a resource usage-style program), we must
	// create a new stack (see documentation of
	// execStack::Execute() in RES_eval.C) and let it rip
	// while we continue on our merry way.
	//

	//
	// first we collect the basics:
	//
	// 	- the program to run
	// 	- the finish value if appropriate
	// 	- the new context
	//
	pEsys::Program*	program_to_execute
			= targetType.tasks[theTaskIndex]->prog.object();

	if(APcloptions::theCmdLineOptions().debug_execute) {
	    cerr << "executing method " << targetType.tasks[theTaskIndex]->name
		 << " of " << targetType.name << " at " << usage_times[k].to_string()
		 << "\n";
	}

	(*used_object->parent_constructor_obj)[abs_res_object::START] = usage_times[k];
	if(from_to_style) {
	    (*used_object->parent_constructor_obj)[abs_res_object::FINISH] = usage_times[k+1];
	}

	//
	// Adaptation-specific execution context:
	//
	execution_context* new_context
		= new abstract_context<use_abs_res_obj>(
				used_object,
				program_to_execute,
				program_to_execute->section_of_origin());

	//
	// Now we determine whether we are calling or
	// spawning. Possible cases:
	//
	// 	current program		new program 	call/spawn
	// 	===============		===========	==========
	//
	// 	resource usage		resource usage	SPAWN
	// 	resource usage		modeling	SPAWN
	// 	modeling		resource usage	SPAWN
	// 	modeling		modeling	CALL
	//
	if(context->counter->Prog->get_style()
				== Program::SYNCHRONOUS_STYLE
	   && program_to_execute->get_style()
	   			== Program::SYNCHRONOUS_STYLE) {

	    //
	    // modeling style called from modeling style: CALL.
	    // The current thread is put on hold. We MUST return
	    // after pushing the new context onto the stack.
	    //
	    stack_to_use->push_context(new_context);

	    //
	    // Execute() should run into the initial wait statement
	    // and return with a code set to WAITING
	    //
	    stack_to_use->Execute(Code);

	} else {

	    //
	    // SPAWN style: we create a new thread and we call
	    // execStack::Execute(). Note that if this is
	    // a resource usage program calling a modeling
	    // program, the WAIT instruction in the latter
	    // will cause it to wait in a new event; but
	    // we don't care: we continue executing as if
	    // nothing happened (SPAWN paradigm.)
	    //
	    pEsys::execStack stack_for_new_thread(new_context);
	    execution_context::return_code ThrowAwayCode
		    		= execution_context::REGULAR;
	    stack_for_new_thread.Execute(ThrowAwayCode);
	}
    } catch(eval_error Err) {
	Cstring err;
	err << "File " << file << ", line " << line
	    << ": error trying to use resource:\n"
	    << Err.msg;
	throw(eval_error(err));
    }

    //
    // At this point we should stop accumulating wall clock
    // time and we should make sure to restart the wall
    // clock of the object that called us
    //
    use_task->stop_timer();
    ambient_object->Task.start_timer();
}
