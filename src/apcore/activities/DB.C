#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "aafReader.H"
#include "DB.H"
#include "apDEBUG.H"
#include "ACT_exec.H"
#include "ActivityInstance.H"
#include "CompilerIntfc.H"
#include "C_list.H"
#include "EventImpl.H"
#include "RES_exec.H"
#include "action_request.H"

#include <set>
#include <iterator>

using std::set;
using std::string;
using std::insert_iterator;
using std::endl;

typedef set<string> StringSet;


namespace apgenDB
{

ActivityInstance*
CreateActivity(
		const Cstring&		type,
		const Cstring&		name,
		const Cstring&		legend,
		const CTime_base&	time,
		const pairslist&	opt_pairs,
		Cstring&		id) {

	// we emulate the syntax rule for activity_instance in grammar.y:

	aafReader::current_file() = "New";
	if(!eval_intfc::ListOfAllFileNames().find("New")) {
		eval_intfc::ListOfAllFileNames() << new emptySymbol("New");
	}

	// first, get the type
	map<Cstring, int>::const_iterator iter = Behavior::ClassIndices().find(type);
	if(iter == Behavior::ClassIndices().end()) {
		Cstring err;
		err << "Cannot construct activity of unknown type \"" << type << "\"";
		throw(eval_error(err));
	}
	Behavior*	act_type = Behavior::ClassTypes()[iter->second];
	task*		act_task = act_type->tasks[0];
	act_object*	ao = new act_object(
				/* ActivityInstance* = */ NULL,
				name,
				id,
				*act_type);

	(*ao)[ActivityInstance::PLAN] = "New";

	behaving_element actObj(ao);

	int	type_params =
		    act_task->parameters ? act_task->parameters->statements.size() : 0;

	//
	// Create parameters.
	//
	if(opt_pairs.get_length()) {
		if(opt_pairs.get_length() != type_params) {
			Cstring err;
			err << "Activity instance has " << opt_pairs.get_length()
				<< " parameter(s) but type "
				<< act_task->name
				<< " has " << type_params;
			throw(eval_error(err));
		}
		parsedExp		Expression;
		pairslist::iterator	it(opt_pairs);
		symNode*		N;
		int			i = 0;

		while((N = it())) {
			try {
				compiler_intfc::CompileExpression(N->payload, Expression);
			}
			catch(eval_error Err) {
				Cstring errors;
				errors << "Error while creating specified parameters for new activity "
					<< "of type " << type << "; details:\n"
					<< Err.msg;
				throw eval_error(errors);
			}

			try {
				Expression->eval_expression(
						actObj.object(),
						(*ao)[act_task->paramindex[i++]]);
			}
			catch(eval_error Err) {
				Cstring errors;
				errors << "Error while evaluating specified parameters for new "
					"activity of type " << type << "; details:\n";
				errors << Err.msg;
				throw eval_error(errors);
			}
		}
	} else {

		//
		// We have to give parameters their default values. Cannot
		// use Execute() because we don't have a task whose prog
		// is the parameters of the act_task.  So, we execute
		// parameters definitions one by one.
		//
		if(act_task->parameters) {
			pEsys::Program*		P = act_task->parameters.object();
	  		execution_context* ec = new execution_context(
					actObj,
					P);
			execution_context::return_code	Code = execution_context::REGULAR;
	
			ec->ExCon(Code);
			delete ec;
		}
	}

	//
	// Create the activity instance.
	//
	ActivityInstance* activityInstance = NULL;
	try {
		activityInstance = new ActivityInstance(ao);
	} catch(eval_error Err) {
		throw(Err);
	}

	//
	// Execute the activity type constructor.
	//
	// This will take care of
	//
	// 	- class variables, if present in the type definition
	// 	- attributes, if present in the type definition
	//
	// That should be it.
	//
	if(act_task->prog) {
	  	execution_context* ec = new execution_context(
					actObj,
					act_task->prog.object());
		pEsys::execStack	stack(ec);

		execution_context::return_code	Code = execution_context::REGULAR;
		ec->ExCon(Code);
	}
	id = (*ao)[ActivityInstance::ID].get_string();
	(*ao)[ActivityInstance::LEGEND] = legend;
	activityInstance->setetime(time);
	return activityInstance;
}

void
CreateActivityInPlan(
		const Cstring&		type,
		const Cstring&		name,
		const Cstring&		legend,
		const CTime_base&	time,
		const pairslist&	opt_pairs,
		Cstring&		id) {
	ActivityInstance* activityInstance = CreateActivity(
						type, name, legend,
						time, opt_pairs, id);
	// Instantiate the activity instance
	try {
		instance_state_changer	ISC(activityInstance);

		ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
		ISC.do_it(NULL);

		// NO! instance_state_changer already did it:
		// activityInstance->create();
	}
	catch(eval_error Err) {
		Cstring errors;
		errors = "New Activity Error: Error in Activity Instantiation; details:\n";
		errors << Err.msg << "\n";
    
		destroy_an_act_req(activityInstance);
		throw eval_error(errors);
	}

	// Handle the case where the instance was created while modeling/scheduling
	if(EventLoop::CurrentEvent) {

		//
		// NOTE: the ability to add an activity to the plan while
		// modeling or scheduling is ongoing is OBSOLETE.
		//
		// Let's test that theory:
		//
		assert(false);

		/* We are in the middle of modeling/scheduling. Note that we are here not
		 * because we have been asked to help model children of activities in the
		 * plan, but because the user (or the socket) has manually added an
		 * activity to the plan while we were modeling. Therefore, this is always
		 * a "spawn" type process. */
		execution_context::return_code Code = execution_context::REGULAR;
		activityInstance->exercise_modeling( /* NULL, */ Code);
	}

	ACT_exec::addNewItem();
}

} // namespace apgenDB


