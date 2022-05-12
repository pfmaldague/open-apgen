#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <assert.h>

#include "aafReader.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "apcoreWaiter.H"
#include "APmodel.H"
#include "EventImpl.H"
#include "RES_def.H" // for create/destroy programs
#include "RES_eval.H"
#include "Scheduler.H"

using namespace std;
using namespace pEsys;

// in Behavior.C:
extern pEsys::pE* copy_and_modify(
				pEsys::pE*	p,
				task*		T);

// in RES_exec.C:
extern int		scheduling_disabled;

extern bool		debug_event_loop;

// in RES_exec.C:
Cstring			spell_event_type(unsigned char et);

// in grammar_intfc.C:
extern Cstring          addQuotes(const Cstring &);

// in RES_model.C:
extern Cstring get_owner_name(execution_context* context);

//
// (also remove extra white space)
//
Cstring removeNewlines(const Cstring &s) {
	int		count = 0;
	const char	*t = *s;
	char		*u, *v;

	while(*t) {
		int	skip = 0;
		char	sub = *t;

		if(*t == '\\') {
			if(t[1] == 'n') {
				skip = 2;
			}
		} else if(*t == '\n') {
			skip = 1;
		} else if(*t == ' ') {
			if(t[1] == ' ' || t[1] == '\t') {
				skip = 1;
			}
		} else if(*t == '\t') {
			if(t[1] == ' ' || t[1] == '\t') {
				skip = 1;
			}
			sub = ' ';
		}
		if(skip) {
			t += skip;
		} else {
			count++;
			t++;
		}
	}
	t = *s;
	u = (char *) malloc(count + 1);
	v = u;
	while(*t) {
		int	skip = 0;
		char	sub = *t;

		if(*t == '\\') {
			if(t[1] == 'n') {
				skip = 2;
			}
		} else if(*t == '\n') {
			skip = 1;
		} else if(*t == ' ') {
			if(t[1] == ' ' || t[1] == '\t') {
				skip = 1;
			}
		} else if(*t == '\t') {
			if(t[1] == ' ' || t[1] == '\t') {
				skip = 1;
			}
			sub = ' ';
		}
		if(skip) {
			t += skip;
		} else {
			*v++ = sub;
			t++;
		}
	}
	*v = '\0';
	return Cstring(u);
}

bool act_object::print(
		aoString& S,
		const char* prefix) const {
	S << prefix << "[\"type\" = " << addQuotes(Type.name)
		<< ", \"id\" = " << operator[](ActivityInstance::ID).get_string()
		<< "]";
	return true;
}

void pEsys::Declaration::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	Symbol* lhs = dynamic_cast<Symbol*>(tok_id.object());
	behaving_base*	 obj = context->AmbientObject.object();
	assert(lhs);
	pE* rhs = Expression.object();
	TypedValue& cval = lhs->get_val_ref(obj);

	//
	// sets both declared_type and type:
	//
	cval = val;
	rhs->eval_expression(obj, cval);
}

void pEsys::Decomp::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	behaving_base*	 obj = context->AmbientObject.object();
	try {
		((*this).*eval_decomp_times)(obj);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " <<  line
			<< ": error trying to evaluate start time of new activity:\n"
			<< Err.msg;
		throw(eval_error(err));
	}
	try {
		for(int k = 0; k < decomp_times.size(); k++) {
			execute_one_decomp(
				stack_to_use,
				obj,
				decomp_times[k],
				Code);
		}
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " <<  line << ": error trying to decompose:\n"
			<< Err.msg;
		throw(eval_error(err));
	}
}

void pEsys::AbstractUsage::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
    const Behavior&	targetType = *abs_type;

    //
    // theTaskIndex is always 1 for an abstract resource;
    // 0 is the index of the constructor
    //
    task*		use_task = targetType.tasks[/* theTaskIndex = */ 1];
    behaving_base*	ambient_object = context->AmbientObject.object();
    Behavior&		sourceType = ambient_object->Task.Type;
    task*		storage_task = (theStorageTaskIndex >= 0) ?
				sourceType.tasks[theStorageTaskIndex] : NULL;

    try {
	((*this).*eval_usage_times)(ambient_object);

	// NOTE: will define finish if available from temporal spec.
	//       See below.

	time_saver		ts;
	behaving_element	used_object;
	behaving_base*		storage = NULL;
	bool			from_to_style = usage_times.size() > 1;

	for(int k = 0; k < usage_times.size(); k++) {
	    ts.set_now_to(usage_times[k]);
	    if(CreatingEventsInThePast(this)) {
		return;
	    }

	    if(k == 0 || k < usage_times.size() - 1) {

		//
		// We need to create a 'constructor' object to hold the class
		// variables, and a 'method' object to support execution of
		// the 'use' method.
		//
		// Question: who owns the constructor object? How does it get
		// deleted? These questions do not arise in the case of an
		// activity instance, which is permanent. But abstract
		// resource instances are created on the fly. We need to
		// manage them.
		//
		// Solution: abs_res_object has a behaving element which
		// contains the constructor.
		//
		abs_res_object* the_constructor_obj = new abs_res_object(
				*targetType.tasks[0],
				context->AmbientObject);
		used_object.reference(new abs_res_object(
					targetType,
					1,	// task index for 'use'
					the_constructor_obj));

		//
		// process call arguments. NOTE: need different logic for
		// set, reset... but that is not an issue for abstract
		// resources.
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

		pEsys::Program*		program_to_execute
				= targetType.tasks[theTaskIndex]->prog.object();

		if(APcloptions::theCmdLineOptions().debug_execute) {
		    cerr << "executing method " << targetType.tasks[theTaskIndex]->name
			<< " of " << targetType.name << " at " << usage_times[k].to_string()
			<< "\n";
		}

		(*the_constructor_obj)[abs_res_object::START] = usage_times[k];
		if(from_to_style) {
		    (*the_constructor_obj)[abs_res_object::FINISH] = usage_times[k+1];
		}

		execution_context* new_context = new execution_context(
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
		    // The current thread is put on hold.
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
// we know the action is use, not set nor reset
//

void pEsys::Usage::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	behaving_base*		obj = context->AmbientObject.object();
	RES_resource*		res_used_by_THIS
					= get_resource_used_by_this(context->AmbientObject);
	Behavior&		sourceType = context->AmbientObject->Task.Type;
	Behavior&		targetType = res_used_by_THIS->Type;
	task*			storage_task = (theStorageTaskIndex >= 0) ?
					sourceType.tasks[theStorageTaskIndex]
					: NULL;
	task*			use_task = targetType.tasks[theTaskIndex];

	try {

		//
		// Sets usage_times. In the old days, an
		// 'every ... from ... to ...' time specification
		// was allowed, resulting in possibly large number
		// of usages. Nowadays, the number of usages is
		// 1 (for 'use at ...')  or 2 (for 'use from ... to ...').
		//
		((*this).*eval_usage_times)(obj);
		bool	end_usage_needed = res_used_by_THIS->requires_end_usage();

		usage_event*		event_for_this_usage;
		usage_event*		end_event;
		time_saver		ts;
		behaving_element	used_object;
		behaving_base*		storage = NULL;

		//
		// number of usages is always 1 or 2:
		//
		int			number_of_usages = usage_times.size();

		assert(number_of_usages & 3);

		ts.set_now_to(usage_times[0]);
		if(CreatingEventsInThePast(this)) {
			return;
		}

		//
		// We need to check that the end time (if it exists) is
		// later than the start time. If this test fails, it's
		// always a hard error (unlike creating an event in the
		// 'past' relative to EventLoop::CurrentEvent)
		//
		if(number_of_usages == 2 && usage_times[0] > usage_times[1]) {
		    Cstring err;
		    err << "File " << file << ", line " << line
			<< ": usage clause starts at "
			<< usage_times[0].to_string()
			<< " and ends at the earlier time "
			<< usage_times[1].to_string();
		    throw(eval_error(err));
		}
		used_object.reference(new res_usage_object(
				res_used_by_THIS,
				*use_task));

		if(actual_arguments.size()) {
			if(storage_task) {
				storage = new behaving_object(*storage_task);
				behaving_object::var_should_be_stored = true;
				behaving_object::temporary_storage = storage;
			}
			for(int j = 0; j < actual_arguments.size(); j++) {
				int param_index = use_task->paramindex[j];
				actual_arguments[j]->eval_expression(
						context->AmbientObject.object(),
						(*used_object)[param_index]);
			}
			if(storage_task) {
				behaving_object::temporary_storage = NULL;
				behaving_object::var_should_be_stored = false;
			}
		} else {
			storage = NULL;
		}
		if(number_of_usages == 1) {
		    event_for_this_usage = usage_event::resEventFactory(
				/* eventPLD::START_USAGE, */
				context,
				used_object,
				storage);
		} else {
		    event_for_this_usage = usage_event::resEventFactory(
				/* eventPLD::START_USAGE, */
				context,
				used_object,
				/* from_to = */ true,
				usage_times[1],
				storage);
		}
		model_intfc::add_timevent(
				event_for_this_usage->source,
				apgen::METHOD_TYPE::USAGE);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " <<  line
		    << ": error trying to use resource:\n"
		    << Err.msg;
		throw(eval_error(err));
	}
}

void pEsys::ImmediateUsage::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
    behaving_base*	obj = context->AmbientObject.object();
    RES_resource*	res_used_by_THIS
    			 = get_resource_used_by_this(context->AmbientObject);
    Behavior&		sourceType = context->AmbientObject->Task.Type;
    Behavior&		targetType = res_used_by_THIS->Type;
    task*		use_task = targetType.tasks[theTaskIndex];
    behaving_element	used_object;

    used_object.reference(new res_usage_object(
			  res_used_by_THIS,
			  *use_task));
	

    //
    // process call arguments. NOTE: need different logic for
    // set, reset...
    //

    try {
	for(int j = 0; j < actual_arguments.size(); j++) {
	    int param_index = use_task->paramindex[j];
	    actual_arguments[j]->eval_expression(
			context->AmbientObject.object(),
			(*used_object)[param_index]);
	}
    } catch(eval_error Err) {
	Cstring err;
	err << "File " << file << ", line " << line
	    << ": error trying to use resource:\n"
	    << Err.msg;
	throw(eval_error(err));
    }
    execution_context ec(	used_object.object(),
				use_task->prog.object());
    ec.ExCon(Code);
    res_used_by_THIS->execute_use_clause(
			(*used_object)[consumptionIndex],
			EventLoop::CurrentEvent,
			/* immed = */ true);
}

void pEsys::SetSignal::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
    behaving_base*	 obj = context->AmbientObject.object();

    try {
	((*this).*eval_usage_times)(obj);
	usage_event*		event_for_this_usage;

	time_saver		ts;
	behaving_base*		storage = NULL;

	for(int k = 0; k < usage_times.size(); k++) {
	    ts.set_now_to(usage_times[k]);
	    if(CreatingEventsInThePast(this)) {
		return;
	    }

	    event_for_this_usage
		    = usage_event::signalEventFactory(context);
	    model_intfc::add_timevent(
			event_for_this_usage->source,
			apgen::METHOD_TYPE::USAGE);
	}
    } catch(eval_error Err) {
	Cstring err;
	err << "File " << file << ", line " << line
	    << ": error trying to set signal:\n"
	    << Err.msg;
	throw(eval_error(err));
    }
}

void pEsys::SetUsage::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
    behaving_base*		obj = context->AmbientObject.object();
    RES_resource*		res_used_by_THIS
					= get_resource_used_by_this(context->AmbientObject);
    Behavior&		sourceType = context->AmbientObject->Task.Type;
    Behavior&		targetType = res_used_by_THIS->Type;
    task*			storage_task = (theStorageTaskIndex >= 0) ?
					sourceType.tasks[theStorageTaskIndex]
					: NULL;
    task*			use_task = targetType.tasks[theTaskIndex];

    try {
	((*this).*eval_usage_times)(obj);
	usage_event*		event_for_this_usage;
	usage_event*		end_event;
	time_saver		ts;
	behaving_element	used_object;
	behaving_base*		storage = NULL;

	for(int k = 0; k < usage_times.size(); k++) {
	    ts.set_now_to(usage_times[k]);
	    if(CreatingEventsInThePast(this)) {
		return;
	    }
	    if(k == 0 || k < usage_times.size() - 1) {
		used_object.reference(new res_usage_object(
					res_used_by_THIS,
					*use_task));
	    }

	    //
	    // process call arguments. During consolidation, we
	    // already checked that there was only one.
	    //

	    if(actual_arguments.size()) {
		if(storage_task) {
		    storage = new behaving_object(*storage_task);
		    behaving_object::var_should_be_stored = true;
		    behaving_object::temporary_storage = storage;
		}
		actual_arguments[0]->eval_expression(
				context->AmbientObject.object(),
				(*used_object)[1]);

		if(storage_task) {
		    behaving_object::temporary_storage = NULL;
		    behaving_object::var_should_be_stored = false;
		}
	    } else {
		storage = NULL;
	    }
	    event_for_this_usage = usage_event::resEventFactory(
			/* eventPLD::START_USAGE, */
			context,
			used_object,
			storage);
	    model_intfc::add_timevent(
			event_for_this_usage->source,
			apgen::METHOD_TYPE::USAGE);
	}
    } catch(eval_error Err) {
	Cstring err;
	err << "File " << file << ", line " <<  line
	    << ": error trying to set resource:\n"
	    << Err.msg;
	throw(eval_error(err));
    }
}

void pEsys::ImmediateSetUsage::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
    RES_resource* res_used_by_THIS = get_resource_used_by_this(
					context->AmbientObject);
    Behavior&	sourceType = context->AmbientObject->Task.Type;
    Behavior&	targetType = res_used_by_THIS->Type;
    task*	storage_task = (theStorageTaskIndex >= 0) ?
				sourceType.tasks[theStorageTaskIndex]
				: NULL;
    task&		setTask = *targetType.tasks[theTaskIndex];
    behaving_base*	obj = context->AmbientObject.object();
    behaving_element setting_object;

    setting_object.reference(new res_usage_object(
					res_used_by_THIS,
					setTask));

    try {
	behaving_base*	storage = NULL;

	if(res_used_by_THIS->get_class() == apgen::RES_CLASS::STATE) {

	    //
	    // State resource.
	    //

	    //
	    // Process call arguments. During consolidation, we
	    // already checked that there was only one.
	    //
	    assert(actual_arguments.size() == 1);
	    actual_arguments[0]->eval_expression(
					obj,
					(*setting_object)[1]);
	    res_used_by_THIS->execute_set_clause(
					(*setting_object)[1],
					NULL,
					/* immed = */ true);
	} else {

	    //
	    // Settable resource.
	    //
	    assert(res_used_by_THIS->get_class() == apgen::RES_CLASS::SETTABLE);
	    RES_precomputed* precomp_res = dynamic_cast<RES_precomputed*>(res_used_by_THIS);

	    //
	    // Process call arguments:
	    //
	    for(int i = 0; i < actual_arguments.size(); i++) {
		behaving_object::temporary_storage = obj;

		//
		// Make sure resource is not precomputed!
		//
		if(precomp_res) {
		    throw(eval_error("The set() method of a precomputed resource should have no parameters\n"));
		}
		int param_index = setTask.paramindex[i];
	
		actual_arguments[i]->eval_expression(
					obj,
					(*setting_object)[param_index]);
	
		behaving_object::temporary_storage = NULL;
	    }

	    execution_context ec(	setting_object.object(),
					setTask.prog.object());
	    ec.ExCon(Code);

	    res_used_by_THIS->execute_set_clause(
				(*setting_object)[consumptionIndex],
				NULL,
				/* immed = */ true);
	 }
    } catch(eval_error Err) {
	Cstring err;
	err << "File " << file << ", line " << line
	    << ": error trying to set resource:\n"
	    << Err.msg;
	throw(eval_error(err));
    }
}

bool pEsys::CreatingEventsInThePast(pE* here) {
	if(EventLoop::CurrentEvent && EventLoop::CurrentEvent->getetime() > time_saver::get_now()) {
		/* we are creating an activity in the past. Per
		 * @@@agreement with Steve Wissler@@@ we will report
		 * to stdout and skip modeling. */
		if(APcloptions::theCmdLineOptions().hard_timing_errors) {
			Cstring errs;
			errs << "File " << here->file << ", line " << here->line
				<< ": Trying to create event in the past at "
				<< time_saver::get_now().to_string()
				<< " earlier than current modeling time "
				<< EventLoop::CurrentEvent->getetime().to_string() << "\n";
			throw(eval_error(errs));
		} else if(APcloptions::theCmdLineOptions().ModelWarnings) {
			cout << "File " << here->file << ", line " << here->line
				<< ": WARNING - Trying to create new usage event at "
				<< time_saver::get_now().to_string()
				<< " earlier than current modeling time "
				<< EventLoop::CurrentEvent->getetime().to_string() << "\n";
		}
		return true;
	}
	return false;
}

void pEsys::ResetUsage::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	behaving_base*		obj = context->AmbientObject.object();
	RES_resource*		res_used_by_THIS
					= get_resource_used_by_this(context->AmbientObject);
	Behavior&		sourceType = context->AmbientObject->Task.Type;
	Behavior&		targetType = res_used_by_THIS->Type;
	task*			storage_task = (theStorageTaskIndex >= 0) ?
					sourceType.tasks[theStorageTaskIndex]
					: NULL;
	task*			use_task = targetType.tasks[theTaskIndex];

	try {
		((*this).*eval_usage_times)(obj);
		usage_event*		event_for_this_usage;
		usage_event*		end_event;
		time_saver		ts;
		behaving_element	used_object;
		behaving_base*		storage = NULL;

		for(int k = 0; k < usage_times.size(); k++) {
		    ts.set_now_to(usage_times[k]);
		    if(CreatingEventsInThePast(this)) {
			return;
		    }
		    if(k == 0 || k < usage_times.size() - 1) {
			used_object.reference(new res_usage_object(
				res_used_by_THIS,
				*use_task));
		    }

		    //
		    // process call arguments. During consolidation, we
		    // already checked that there weren't any.
		    //
		    event_for_this_usage = usage_event::resEventFactory(
				/* eventPLD::START_USAGE, */
				context,
				used_object,
				storage);
		    model_intfc::add_timevent(
				event_for_this_usage->source,
				apgen::METHOD_TYPE::USAGE);
		}
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line
			<< ": error trying to reset resource:\n"
			<< Err.msg;
		throw(eval_error(err));
	}
}

void pEsys::ImmediateResetUsage::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	behaving_base*		obj = context->AmbientObject.object();
	RES_resource*		res_used_by_THIS
					= get_resource_used_by_this(context->AmbientObject);
	Behavior&		sourceType = context->AmbientObject->Task.Type;
	Behavior&		targetType = res_used_by_THIS->Type;
	task*			storage_task = (theStorageTaskIndex >= 0) ?
					sourceType.tasks[theStorageTaskIndex]
					: NULL;
	task*			use_task = targetType.tasks[theTaskIndex];

	try {
		behaving_element	used_object;
		used_object.reference(new res_usage_object(
					res_used_by_THIS,
					*use_task));
		res_used_by_THIS->execute_reset_clause(
			EventLoop::CurrentEvent,
			/* immed = */ true);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " <<  line << ": error trying to reset resource:\n"
			<< Err.msg;
		throw(eval_error(err));
	}
}

void pEsys::Assignment::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	behaving_base*	 obj = context->AmbientObject.object();
	try {
		Expression->eval_expression(obj, lhs_of_assign->get_val_ref(obj));
	} catch(eval_error Err) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": error in assignment -\n" << Err.msg;
		throw(eval_error(errs));
	}
}

void pEsys::Continue::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	Code = execution_context::CONTINUING;
}

void pEsys::Return::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	function_object*	fo = dynamic_cast<function_object*>(
						context->AmbientObject.object());
	Expression->eval_expression(context->AmbientObject.object(), fo->theReturnedValue);
	Code = execution_context::RETURNING;
}

void If::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	((*this).*execute_method)(context, Code, stack_to_use);
}

void If::execute_if(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
}

void NonRecursiveIf::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	execution_context::mini_stack* current_counter = context->counter;
	TypedValue A;

	//
	// We should not assume that Expression was optimized
	//
	Expression->eval_expression(context->AmbientObject.object(), A);
	bool	condition = A.get_bool();
	if(!condition) {
		context->counter->state = execution_context::CONDITION_IS_FALSE;
		return;
	}
	context->counter->state = execution_context::TRUE_CONDITION_FOUND;
	context->HandleNewBlock(P);
}

void If::execute_elseif(
		execution_context*		context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
}

void NonRecursiveElseIf::execute(
		execution_context*		context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	execution_context::mini_stack* mini = context->counter;
	if(mini->state == execution_context::TRUE_CONDITION_FOUND) {
		return;
	}
	TypedValue A;
	Expression->eval_expression(context->AmbientObject.object(), A);
	bool	condition = A.get_bool();
	if(!condition) {
		mini->state = execution_context::CONDITION_IS_FALSE;
		return;
	}
	mini->state = execution_context::TRUE_CONDITION_FOUND;
	context->HandleNewBlock(P);
}

void If::execute_else(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
}

void NonRecursiveElse::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	execution_context::mini_stack* mini = context->counter;
	if(mini->state == execution_context::TRUE_CONDITION_FOUND) {
		return;
	}
	context->HandleNewBlock(P);
}

void pEsys::While::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
	TypedValue V;
	while_statement_header->eval_expression(context->AmbientObject.object(), V);
	if(!V.get_int()) {
		return;
	}
	execution_context::mini_stack* mini = context->counter;
	Program* pr = dynamic_cast<Program*>(program.object());
	execution_context::mini_stack* ms;
	if(!(ms = mini->next)) {
		ms = new execution_context::mini_stack(pr);
		ms->prev = mini;
		mini->next = ms;
	} else {
		ms->Prog = pr;
		ms->state = execution_context::TRUE_CONDITION_FOUND;
	}
	ms->I = -1;
	context->counter = ms;
}

void pEsys::FunctionCall::eval_precomputed_expression(
		behaving_base*	local_object,
		TypedValue&	returned_value) {

    //
    // There are no parameters. We need to grab time from the 'at' clause,
    // which must be the same as 'now'. 
    //

    Rsource* res = local_object->get_concrete_res();
    RES_precomputed* settable_res = dynamic_cast<RES_precomputed*>(res);
    assert(settable_res);
    aafReader::single_precomp_res* precomp_res
	    = settable_res->get_precomp_res();

    if(!precomp_res) {
	Cstring err;
	err << "File " << file << ", line " << line
		<< ": no precomputed resource values found named "
		<< TaskInvoked->name;
	throw(eval_error(err));
    }

    double result[2];
    try {
	returned_value = precomp_res->Evaluate(time_saver::get_now(), result);
    } catch(eval_error Err) {
	Cstring err;
	err << "File " << file << ", line " << line
	    << ": evaluation error " << Err.msg;
	throw(eval_error(err));
    }
}

void pEsys::FunctionCall::execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		pEsys::execStack*	stack_to_use) {
    behaving_base*	local_object = context->AmbientObject.object();

    if(	  TaskInvoked &&
	  !TaskInvoked->prog) {

	//
	// Precomputed resource. We don't execute a DSL
	// program, we know what to do.
	//
	TypedValue val;
	eval_precomputed_expression(
			local_object,
			val);
    } else

    if(	  TaskInvoked &&
		  TaskInvoked->prog->orig_section != apgen::METHOD_TYPE::FUNCTION) {

	//
	// The eval_expression() method of FunctionCall handles
	// the case in which the function being called is an
	// ordinary function. Here, we deal with the case when
	// the function is a global method, which really needs
	// the stack to function properly.
	//

	//
	// We have a method. The first difference with
	// ordinary functions is that the 'parent' variable
	// is defined and set to the object calling this
	//
	behaving_element called_object;
	method_object* called_obj = new method_object(
				*TaskInvoked,
				local_object);
	called_object.reference(called_obj);

	//
	// The variables of a method object start with
	// ENABLED, followed by PARENT. We initialize the
	// parent to the instance that called the method:
	//
	called_obj->operator[](function_object::PARENT) = behaving_element(local_object);

	//
	// Set the call parameter values in the new object to
	// the values
	//
	for(int k = 0; k < actual_arguments.size(); k++) {
	    TypedValue val;
	    pE* obj = actual_arguments[k].object();
	    int pk = TaskInvoked->paramindex[k];
	    obj->eval_expression(local_object, val);
	    (*called_obj)[pk] = val;
	}

	//
	// We now follow essentially the same pattern as
	// AbstractUsage::execute().
	//
	execution_context* new_context
	    = new execution_context(
		    called_object,
		    TaskInvoked->prog.object(),
		    TaskInvoked->prog->section_of_origin());

	try {
	    //
	    // We have to decide whether to call or spawn
	    // the execution process.
	    //
	    // The default is call. If the keyword 'spawn'
	    // was detected, then that is what we do.
	    //
    	    if(tok_spawn) {
		execution_context::return_code
		    ThrowAwayCode = execution_context::REGULAR;
		execStack	stack_for_new_thread(new_context);
		stack_for_new_thread.Execute(ThrowAwayCode);
	    } else {

		//
		// Default: call. Code will be interpreted by
		// the program that contains this instruction.
		//
		stack_to_use->push_context(new_context);
		stack_to_use->Execute(Code);
	    }
	} catch(eval_error& Err) {

	    Cstring err;
	    err << "File " << file << ", line " << line << ", "
	        << "in global method \"" << getData() << "\":\n"
	        << Err.msg;
	    throw (eval_error(err));
	}
    } else {

	//
	// Ordinary functions. We do not need a stack.
	//
	TypedValue val;
	eval_expression(local_object, val);
    }
}

const char *print_type_of_resource(apgen::RES_CLASS i) {
	switch(i) {
		case apgen::RES_CLASS::CONSUMABLE:
			return "CONSUMABLE";
		case apgen::RES_CLASS::NONCONSUMABLE:
			return "NONCONSUMABLE";
		case apgen::RES_CLASS::STATE:
			return "STATE";
		case apgen::RES_CLASS::EXTERNAL:
			return "EXTERNAL";
		case apgen::RES_CLASS::SIGNAL:
			return "SIGNAL";
		case apgen::RES_CLASS::ASSOCIATIVE:
			return "ASSOCIATIVE";
		case apgen::RES_CLASS::INTEGRAL:
			return "INTEGRAL";
		default:
			return "UNDEFINED";
	}
	return "UNDEFINED";
}

const char *print_usage_type(apgen::USAGE_TYPE i) {
	switch(i) {
		case apgen::USAGE_TYPE::USE:
			return "USE";
		case apgen::USAGE_TYPE::SET:
			return "SET";
		case apgen::USAGE_TYPE::RESET:
			return "RESET";
		default:
			return "UNKNOWN";
	}
	return "UNDEFINED";
}

void payloadToStream(aoString &Stream, const char *prefix, const char *keyword, bsymbolnode* bs){
	Stream << "\n" << prefix << keyword;
	bs->payload->to_stream(&Stream, 0);
	Stream << " ";
}

template class slist<alpha_void, Cntnr<alpha_void, usage_event*, short>, short>;
