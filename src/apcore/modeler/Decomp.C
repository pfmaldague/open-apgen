#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "ParsedExpressionSystem.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "RES_def.H"
#include "apcoreWaiter.H"
#include "APmodel.H"
#include "EventImpl.H"
#include "EventLoop.H"


void pEsys::Decomp::get_time_from_start(
			behaving_base* parent_obj) {
	decomp_times.clear();

		//
		// we need the class object, not the decomposition method
		// object; that's why we use ->level[1] in the expression
		// below
		//

	decomp_times.push_back(
		(*parent_obj->level[1])[ActivityInstance::START].get_time_or_duration()
		);
}

void pEsys::Decomp::get_time_from_now(
			behaving_base* parent_obj) {
	decomp_times.clear();

		//
		// we need the class object, not the decomposition method
		// object; that's why we use ->level[1] in the expression
		// below
		//

	decomp_times.push_back(
		time_saver::get_now()
		);
}

void pEsys::Decomp::get_time_from_one_expression(
			behaving_base* parent_obj) {
	decomp_times.clear();
	TypedValue tempval;
	at_expression->eval_expression(parent_obj, tempval);

	if(tempval.is_duration()) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line
		<< ": expression " << at_expression->to_string()
		<< " should be a time, not a duration";
	    throw(eval_error(errs));
	}

	decomp_times.push_back(tempval.get_time_or_duration());
}

int dbgind::count = 3;

void pEsys::Decomp::execute_one_decomp(
			pEsys::execStack*		stack_to_use,
			behaving_base*			parent_obj,
			const CTime_base&		start_time,
			execution_context::return_code&	Code) {

    //
    // Local symbols for parent activity:
    //
    behaving_base*	parent_class_obj = NULL;
    ActivityInstance*	the_parent_act = NULL;
    bool		we_are_scheduling = false;

    Cstring		instance_id;
    Cstring		instance_type;
    act_object*		child_obj = NULL;
    task*		ChildConstructor = NULL;
    TypedValue*		where_to_store_the_child = NULL;

    //
    // We need to distinguish between 2 contexts for the decomp statement:
    //
    // 	1. an activity type's decomposition or scheduling section
    //
    // 	2. a global method
    //
    // The realm of the parent_obj will tell us which case we
    // are dealing with.
    //
    if(parent_obj->Type.realm == "activity") {

	//
	// local symbols for parent activity. This is
	// also equal to parent_obj->level[1]:
	//
	parent_class_obj = parent_obj->level[1];
	the_parent_act = parent_class_obj->get_req();
	apgen::METHOD_TYPE	parent_mt;
	we_are_scheduling = the_parent_act->has_decomp_method(parent_mt) &&
				parent_mt == apgen::METHOD_TYPE::CONCUREXP;
    } else if(parent_obj->Type.realm == "global") {

	//
	// Grab the activity parent from the parent variable.
	// Actually, this task requires solving a non-trivial
	// issue: in a cascade of calls to global methods, what
	// does 'parent' mean at each level?
	//
	// A reasonable answer: the caller.  But in a decomposition,
	// we need to know the parent activity, not the method that
	// called us. So, there is a distinction.
	//
	// parent_class_obj = parent_obj->level[1];

	//
	// Recursive search for an activity ancestor:
	//
	parent_class_obj = parent_obj;
	while(parent_class_obj->Type.realm != "activity") {
	    parent_class_obj
		= parent_class_obj->operator[](function_object::PARENT).
			get_object().object();
	    assert(parent_class_obj);
	}

	//
	// parent_class_obj supports the decomposition task,
	// but we want the constructor
	//
	parent_class_obj = parent_class_obj->level[1];
	the_parent_act = parent_class_obj->get_req();
	apgen::METHOD_TYPE	parent_mt;
	we_are_scheduling = the_parent_act->has_decomp_method(parent_mt) &&
				parent_mt == apgen::METHOD_TYPE::CONCUREXP;
    }

    bool call_or_spawn_requested = call_or_spawn_arguments;

    //
    // Main task is to figure out the child type and to evaluate the
    // arguments to pass to the child constructor. The Decomp
    // constructor has already pre-determinded the tuple containing
    // call and spawn arguments, if applicable; else, any arguments
    // are contained in right():
    //
    if(call_or_spawn_requested) {

	    //
	    // child activity type. We know from grammar.y, call_or_spawn_arguments,
	    // that there are at least 2 arguments
	    //
	    parsedExp& childTypeExp = ActualArguments[0];
	    TypedValue child_val;
	    childTypeExp->eval_expression(parent_obj, child_val);
	    if(!child_val.is_string()) {
			Cstring errs;
			errs << "File " << file << ", line " << line
				<< ": first argument of call/spawn is not a string";
			throw(eval_error(errs));
	    }
	    instance_type = child_val.get_string();

	    map<Cstring, int>::const_iterator iter = Behavior::ClassIndices().find(instance_type);
	    if(iter == Behavior::ClassIndices().end()) {
			Cstring errs;
			errs << "File " << file << ", line " << line
				<< ": call or spawn error - " << instance_type
				<< " could not be found among activity types";
			throw(eval_error(errs));
	    }
	    Type = Behavior::ClassTypes()[iter->second];
	    instance_id = Type->name;
	    child_obj = new act_object(
				/* ActivityInstance* = */ NULL,
				instance_type,
				instance_id,
				*Type);
	    parsedExp& childParams = ActualArguments[1];
	    TypedValue param_val;
	    childParams->eval_expression(parent_obj, param_val);
	    if(!param_val.is_array()) {
			Cstring errs;
			errs << "File " << file << ", line " << line
				<< ": call or spawn error - second arg is not an array";
			throw(eval_error(errs));
	    }
	    ChildConstructor = Type->tasks[0];
	    ListOVal& Array = param_val.get_array();
	    if(Array.get_length() != ChildConstructor->paramindex.size()) {
			Cstring errs;
			errs << "File " << file << ", line " << line
				<< ": Decomposition clause has " << Array.get_length()
				<< " parameter(s) but type "
				<< Type->name
				<< " has " << ChildConstructor->paramindex.size();
			throw(eval_error(errs));
	    }

	    ArrayElement*	ae;
	    for(int i = 0; i < Array.get_length(); i++) {
			ae = Array[i];
			if(APcloptions::theCmdLineOptions().debug_execute) {
				cerr << "execute_decomp(): handling expression " << ae->Val().to_string() << "\n";
				cerr << "param[" << i << "] = "
					<< ChildConstructor->get_varinfo()[ChildConstructor->paramindex[i]].first
					<< " has type "
					<< apgen::spell((*child_obj)[ChildConstructor->paramindex[i]].declared_type)
					<< "\n";
			}
			(*child_obj)[ChildConstructor->paramindex[i]] = ae->Val();
	    }

	    //
	    // finally, we need to handle the third argument in which we should store the child
	    //
	    if(ActualArguments.size() > 2) {
		parsedExp& childSymbol = ActualArguments[2];
		where_to_store_the_child = &childSymbol->get_val_ref(parent_obj);
	    }

    } else {
	    if(!Type) {
		Cstring errs;
		errs << "File " << file << ", line " << line
		     << ": Decomposition clause has undefined child type "
		     << getData();
		throw(eval_error(errs));
	    }

	    ChildConstructor = Type->tasks[0];
	    instance_id = Type->name;
	    child_obj = new act_object(
			/* ActivityInstance* = */ NULL,
			getData(),
			instance_id,
			*Type);
	    for(int k = 0; k < ActualArguments.size(); k++) {
		pE* obj = ActualArguments[k].object();
		int pk = ChildConstructor->paramindex[k];
		obj->eval_expression(parent_obj, (*child_obj)[pk]);
	    }
    }


    if(APcloptions::theCmdLineOptions().debug_execute) {
	cerr << "execute_decomp(): creating act object, name = "
	     << getData() << ", type = " << Type->name << "\n";
    }

    //
    // set the name, subjet to changing it later if the child's attributes
    // include Decomposition Suffix:
    //
    (*child_obj)[ActivityInstance::NAME]
		= (*parent_class_obj)[ActivityInstance::NAME].get_string()
			+ "_d" + Cstring(the_parent_act->hierarchy().children_count() + 1);


    (*child_obj)[ActivityInstance::START] = start_time;

    //
    // Only do this if scheduling:
    //
    if(we_are_scheduling) {
		(*child_obj)[ActivityInstance::PLAN] = "New";
    } else {
		(*child_obj)[ActivityInstance::PLAN]
			= (*parent_class_obj)[ActivityInstance::PLAN];
    }

    time_saver	ts;

    ts.set_now_to(start_time);

    if(EventLoop::CurrentEvent && EventLoop::CurrentEvent->getetime() > time_saver::get_now()) {
	if(CreatingEventsInThePast(this)) {

	    //
		// we continue anyway (tolerant behavior)
		//
		;
	}
    }

    ActivityInstance* theChild = NULL;
    theChild = new ActivityInstance(child_obj);
    // Don't forget the set the activity pointer if necessary
    if(where_to_store_the_child) {
		*where_to_store_the_child = *theChild;
    }

    theChild->setetime(time_saver::get_now());

    if(APcloptions::theCmdLineOptions().debug_execute) {
		cerr << "after creating child instance - parent:\n";
		aoString aos;
		parent_obj->to_stream(&aos, 2);
		cerr << aos.str();
		cerr << "                              - child:\n";
		child_obj->to_stream(&aos, 2);
		cerr << aos.str();
    }

    //
    // Exercise the child type constructor. This will take
    // care of any class variables and any attributes.
    //
    if(ChildConstructor->prog) {
	  	execution_context* ec = new execution_context(
						child_obj,
						ChildConstructor->prog.object());
		exec_status		eStatus;
		execution_context::return_code localCode = execution_context::REGULAR;

		if(APcloptions::theCmdLineOptions().debug_execute) {
			cerr << "Calling Execute() with EC based on task "
				<< child_obj->Task.full_name() << "\n";
		}
		ec->ExCon(localCode);
		delete ec;
    }
    if(APcloptions::theCmdLineOptions().debug_execute) {
		cerr << "after exercising child attributes - parent:\n";
		aoString aos;
		parent_obj->to_stream(&aos, 2);
		cerr << aos.str();
		cerr << "                                  - child:\n";
		child_obj->to_stream(&aos, 2);
		cerr << aos.str();
    }

    //
    // update the child's name if appropriate
    //
    if((*child_obj)[ActivityInstance::DECOMP_SUFFIX].get_string().length()) {
		(*child_obj)[ActivityInstance::NAME]
			= (*child_obj)[ActivityInstance::DECOMP_SUFFIX].get_string();
    }

    instance_state_changer	ParentStateChanger(the_parent_act);
    ParentStateChanger.add_a_child(stack_to_use, theChild);
    if(!the_parent_act->agent().is_decomposed()) {
		the_parent_act->unselect();
		if(the_parent_act->dataForDerivedClasses) {
			the_parent_act->dataForDerivedClasses->handle_instantiation(false, true);
		}
		the_parent_act->agent().move_to_decomposed_list();
		apgen::METHOD_TYPE	mt;
		the_parent_act->has_decomp_method(mt);
		if(mt != apgen::METHOD_TYPE::NONEXCLDECOMP) {
			the_parent_act->destroy();
		}
    }
	

    if(APcloptions::theCmdLineOptions().debug_execute) {
		cerr << "after hooking up child to parent - parent:\n";
		aoString aos;
		parent_obj->to_stream(&aos, 2);
		cerr << aos.str();
		cerr << "                                 - child:\n";
		child_obj->to_stream(&aos, 2);
		cerr << aos.str();
    }


    //
    // debug
    //

    // cout << time_saver::get_now().to_string() << " - Decomp(" << the_parent_act->get_unique_id()
    // 	<< "->" << Type->name << "): activating the child\n";

    try {

	//
	// Things we need to do:
	// 	changing child visibility to REGULAR
	// 	attach child to parent
	// 	stick child into someActiveInstances list
	// 	ACT_req_changing->dataForDerivedClasses->handle_instantiation(true, true);
	//	create child
	//	select child
	//

	theChild->hierarchy().attach_to_parent(the_parent_act->hierarchy());
	theChild->agent().move_to_active_list();
	if(theChild->dataForDerivedClasses) {
		theChild->dataForDerivedClasses->handle_instantiation(true, true);
	}
	theChild->create();
	apgen::METHOD_TYPE	child_mt;
	if(  theChild->has_decomp_method(child_mt) &&
	     (child_mt == apgen::METHOD_TYPE::NONEXCLDECOMP)
	  ) {
		execution_context::return_code localCode = execution_context::REGULAR;
		theChild->exercise_decomposition(localCode);
		theChild->select();
	}

    } catch(eval_error Err) {
	Cstring errs;
	errs << "File " << file << ", line " << line
	     << ": Request \"" << the_parent_act->get_unique_id()
	     << "\" generates error when decomposed"
	     << " into " << Type->name << ":\n" << Err.msg
	     << ". Decomposition may not be correct.";
	throw(eval_error(errs));
    } catch(decomp_error Err) {
	Cstring errs;
	errs << "File " << file << ", line " << line
	     << ": Request \"" << the_parent_act->get_unique_id()
	     << "\" generates error when decomposed"
	     << " into " << Type->name << ":\n" << Err.msg
	     << ". Decomposition may not be correct.";
	throw(eval_error(errs));
    }

    if(APcloptions::theCmdLineOptions().debug_execute) {
	cerr << "after activating the child - parent:\n";
	aoString aos;
	parent_obj->to_stream(&aos, 2);
	cerr << aos.str();
	cerr << "                           - child:\n";
	child_obj->to_stream(&aos, 2);
	cerr << aos.str();
    }

    //
    // ?? Modeling should never be the case here
    //

    if(model_control::get_pass() == model_control::MODELING) {
	Cstring err;
	err << "Decomposition cannot take place in a modeling section.";
	throw(eval_error(err));
    }

    if( model_control::get_pass() == model_control::SCHEDULING_2
	    && theChild->agent().is_weakly_active()
      ) {

	//
	// We are in the middle of scheduling (an ancestor of) THIS.
	// We need to generate modeling events or to initiate scheduling
	// (which ever is appropriate) and break if appropriate.
	//


	//
	// debug
	//

	// cerr << time_saver::get_now().to_string() << " - Decomp(" << the_parent_act->get_unique_id()
	// 	<< "->" << Type->name << "): exercising child modeling / scheduling\n";

	try {

	    //
	    // This is the right behavior in all cases; the current
	    // modeling/scheduling process should not be put
	    // on hold because a child is waiting.
	    //

#ifdef REVERTED
	    //
	    // DEFAULT_IS_SPAWN the same now applies to decomposition
	    //
#endif /* REVERTED */

	    task* task_to_use = NULL;
	    Program* program_to_use = NULL;
	    bool call_expansion_section = false;
	    map<Cstring, int>::iterator method_iter;

#ifdef REVERTED
	    //
	    // Previously:
	    //
#endif /* REVERTED */

	    bool put_current_thread_on_hold = true;
	    if(tok_spawn) {
	    	put_current_thread_on_hold = false;
	    }

#ifdef REVERTED
	    //
	    // DEFAULT_IS_SPAWN:
	    //
	    bool put_current_thread_on_hold = false;
	    if(call_or_spawn_requested && tok_call) {
	    	put_current_thread_on_hold = true;
	    }
#endif /* REVERTED */

	    for(  method_iter = Type->taskindex.begin();
		  method_iter != Type->taskindex.end();
		  method_iter++) {
		if(method_iter->first == "modeling"
		   || method_iter->first == "resource usage") {
		    task_to_use = Type->tasks[method_iter->second];
		    program_to_use = task_to_use->prog.object();
		    call_expansion_section = false;
		    break;
		} else if( model_control::get_pass() == model_control::SCHEDULING_2
			   && method_iter->first == "decompose") {
		    task_to_use = Type->tasks[method_iter->second];
		    program_to_use = task_to_use->prog.object();
		    if(program_to_use->orig_section == apgen::METHOD_TYPE::CONCUREXP) {
			call_expansion_section = true;
			break;
		    }
		}
	    }
	    if(program_to_use) {
		if(call_expansion_section) {
		    if(put_current_thread_on_hold) {
			Code = theChild->theActAgent()->exercise_decomposition(
				/* skip_wait = */ true,
				stack_to_use);
		    } else {
			theChild->theActAgent()->exercise_decomposition();
		    }
		} else {
		    if(put_current_thread_on_hold) {
			Code = theChild->theActAgent()->exercise_modeling(
				/* skip_wait = */ true,
				stack_to_use);
		    } else {
			theChild->theActAgent()->exercise_modeling();
		    }
		}
	    }

	    //
	    // If event generation was interrupted, the child put itself on hold
	    // and put a new WAIT event in the queue; it will continue on its merry
	    // way. OUR stack is unaffected; we continue on OUR merry way!
	    //
	} catch(eval_error Err) {
	    Cstring errstring;
	    errstring << "Activity " << theChild->get_unique_id()
		<< " having trouble generating events; details:\n"
		<< Err.msg << "\n";
	    throw(eval_error(errstring));
	} catch(decomp_error Err) {
	    Cstring errstring;
	    errstring << "Activity " << theChild->get_unique_id()
		<< " having trouble generating events; details:\n"
		<< Err.msg << "\n";
	    throw(eval_error(errstring));
	}
    }
}
