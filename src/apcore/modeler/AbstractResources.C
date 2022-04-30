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
// In the templates implemented below, abs_resource
// is an adaptation-specific class which contains
// both ontological and algorithmic elements.
//
// The ontological elements supersede whose of the
// behaving_object class, while the algorithmic elements
// supersede those of the Behavior class. The mechanism
// used to supersede members and methods of those classes
// relies on templates which derive from and override
// both the behaving_element and the Behavior classes.
//
// To make things more efficient, it is convenient to
// also override the instruction which invokes the 'use'
// method of an abstract resource, i. e., the
// AbstractUsage executable expression. The following
// template does exactly that.
//
template <class abs_resource>
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
// The most important jobs of the AbstractUsage expression
// are
//
//	1. create an adaptation-specific instance of
//	   behaving_object that holds all the variables
//	   defined in the usage function of the abstract
//	   resource
//
// 	2. perform the execute() method, which is overridden
//	   by the following method of AbsUsageTemplate.
//
template <class abs_resource>
void AbsUsageTemplate::execute(
		execution_context*		context,
		execution_context::return_code& Code,
		execStack*			stack_to_use = NULL) {

    const Behavior&	targetType = abstract_resource->Type;

    //
    // NOTE: theTaskIndex is always 1 for an abstract resource
    //
    task*		use_task = targetType.tasks[theTaskIndex];

    behaving_base*	ambient_object = context->AmbientObject.object();
    Behavior&		sourceType = ambient_object->Task.Type;

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
	// of abs_resource
	//
	smart_ptr<abs_resource>	used_object;
	bool			from_to_style = usage_times.size() > 1;

	for(int k = 0; k < usage_times.size(); k++) {
	    ts.set_now_to(usage_times[k]);
	    if(CreatingEventsInThePast(this)) {
		return;
	    }

	    if(k == 0 || k < usage_times.size() - 1) {

		//
		// Here, abstract_resource refers to the
		// 'class scope' of the abstract resource,
		// which supports its constructor; used_object,
		// on the other hand, supports the 'method scope'
		// which is appropriate for the usage method.
		//
		used_object.reference(new abs_resource(
					targetType,
					abstract_resource,
					context->AmbientObject));

		//
		// We may not need to use actual_arguments[];
		// us adaptation-specific variables instead.
		// The overloaded Program can learn to make
		// use of them.
		//
		if(actual_arguments.size()) {
		    for(int j = 0; j < actual_arguments.size(); j++) {
			int param_index = use_task->paramindex[j];
			actual_arguments[j]->eval_expression(
					ambient_object,
					(*used_object)[param_index]);
		    }
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

		pEsys::Program*		program_to_execute
				= targetType.tasks[theTaskIndex]->prog.object();

		if(APcloptions::theCmdLineOptions().debug_execute) {
		    cerr << "executing method " << targetType.tasks[theTaskIndex]->name
			<< " of " << targetType.name << " at " << usage_times[k].to_string()
			<< "\n";
		}

		(*used_object)[abs_res_object::START] = usage_times[k];
		if(from_to_style) {
		    (*used_object)[abs_res_object::FINISH] = usage_times[k+1];
		}

		//
		// This will be replaced by the adaptation-specific template
		// provided below in this file:
		//
		execution_context* new_context
			= new abstract_context<abs_resource>(
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


		    // debug
		    // cerr << "execute abs. usage: case "
		    // 	<< sourceType.name << "(style = "
		    // 	<< context->counter->Prog->style << ") -> "
		    // 	<< targetType.name << "(style = "
		    // 	<< program_to_execute->style
		    // 	<< "); executing new stack\n";

		    pEsys::execStack stack_for_new_thread(new_context);
		    execution_context::return_code ThrowAwayCode
			    		= execution_context::REGULAR;
		    stack_for_new_thread.Execute(ThrowAwayCode);
		}
	    }
	}
    } catch(eval_error Err) {
	Cstring err;
	err << "File " << file << ", line " << line
		<< ": error trying to use resource:\n"
		<< Err.msg;
	throw(eval_error(err));
    }
}

//
// The following template overrides behaving_object by
// providing an adaptation-specific version. The class
// that contains the features found in the adaptation
// is the abs_resource template argument. It replaces
// the abs_res_object found in AbstractResource.H.
//
// Actually, there is no need to make this a template.
// What the code generation algorithm needs to do is
// simply override behaving_object with an adaptation-
// specific version for each abstract resource in the
// adaptation. Then, that version can be used as an
// adaptation-specific substitute for abs_res_object
// in the method that overrides AbstractUsage::execute().
//
template <class abs_resource>
class AbsUsageObjectTemplate: public behaving_object {
public:

    //
    // What exactly do we want to put in here?
    //
    // Since behaving_object is fully functional, we don't
    // _need_ to do anything, but we _could_ introduce
    // an explicit symbol table to supersede the array
    // of sticky values provided by the task class.
    //
    // To take advantage of this symbol table, it would be
    // necessary to overload execution_context::ExCon,
    // which is where the behaving_object from the context
    // is fed to the execute method of each program
    // statement. The overloaded version should feature
    // conventional-looking program statements with
    // variables that are conventionally named members of
    // a conventional class, as opposed to references to
    // a vector of TypedValues with a specific numeric
    // index.
    //
};

//
// This template overrides Behavior by providing an
// adaptation-specific version. Again, the adaptation-
// specific features come in via the abs_resource
// template argument.
//
// NOTE: we do not need this template. Its main value
// is that it holds the 'use' task of the abstract
// resource. But the right place for overloading the
// task is in the execution_context::ExCon() method,
// not in the class that holds the generic tasks.
//
template <class abs_resource>
class AbsResBehavior: public Behavior {
public:

    //
    // What exactly do we want to put in here?
    //
    // From Behavior, we inherit an array of tasks;
    // each of these has a vector of variables, a
    // vector of calling arguments and a Program. 
    //
    // Since Behavior is fully functional, we don't
    // _need_ to do anything, but we _could_ introduce
    // an explicit program to supersede the array
    // of executable expressions provided by the Program
    // in the task class.
    //
    // To take advantage of such a program would
    // require overloading execution_context::ExCon,
    // which is where the statements in the task's
    // Program are executed in a generic manner.
    //
};

//
// It is obvious from the above that we will need to
// overload execution_context::ExCon(). For that purpose
// we introduce an adaptation-specific template that
// is derived from execution_context:
//
template <class abs_resource>
class abstract_context: public execution_context {
public:
    //
    // We need to define a void constructor for
    // execution_context, since we won't need its
    // members - except for the prev smart pointer,
    // which is managed by pEsys::execStack.
    //

    void		ExCon(
			    execution_context::return_code& return_code,
			    pEsys::execStack* = NULL) override;
};
