#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "ParsedExpressionSystem.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "RES_def.H"
#include "EventImpl.H"
#include "apcoreWaiter.H"
#include "APmodel.H"

void pEsys::SetSignal::refresh_args_and_call_usage_method(
			usage_event&	event) {
	TypedValue	val, information;
	actual_arguments[0]->eval_expression(event.Cause.object(), val);
	Cstring		signal_name(val.get_string());
	if(actual_arguments.size() == 2) {
		actual_arguments[1]->eval_array(event.Cause.object(), information);
	}
	eval_intfc::add_a_signal(
			new sigInfoNode(
				signal_name,
				signalInfo(event.Cause, information)));
}

void pEsys::Usage::get_time_from_now(
			behaving_base* parent_obj) {
	usage_times.clear();
	usage_times.push_back(time_saver::get_now());
}

void pEsys::Usage::get_time_from_start(
			behaving_base* parent_obj) {
	usage_times.clear();
	usage_times.push_back(
		(*parent_obj->level[parent_obj->level_where_start_is_defined()])[ActivityInstance::START].get_time_or_duration());
}

void pEsys::Usage::get_time_from_start_and_finish(
			behaving_base* parent_obj) {
	usage_times.clear();
	usage_times.push_back(
		(*parent_obj->level[parent_obj->level_where_start_is_defined()])[ActivityInstance::START].get_time_or_duration());
	usage_times.push_back(
		(*parent_obj->level[parent_obj->level_where_start_is_defined()])[ActivityInstance::FINISH].get_time_or_duration());
}

void pEsys::Usage::get_time_from_one_expression(
			behaving_base* parent_obj) {
	usage_times.clear();
	TypedValue tempval;
	at_expression->eval_expression(parent_obj, tempval);
	usage_times.push_back(tempval.get_time_or_duration());
}

void pEsys::Usage::get_time_from_two_expressions(
			behaving_base* parent_obj) {
	usage_times.clear();
	TypedValue tempval;
	from_expression->eval_expression(parent_obj, tempval);
	usage_times.push_back(tempval.get_time_or_duration());
	to_expression->eval_expression(parent_obj, tempval);
	usage_times.push_back(tempval.get_time_or_duration());
}

//
// Returns a behaving_element which encapsulates
// either a res_usage_object or an abstract_res_object.
// Will throw if error:
//
RES_resource* pEsys::Usage::get_resource_used_by_this(
		behaving_element&	ambient) const {
    RES_resource*		res_used_by_THIS = NULL;
    resContPLD&		pld = *theContainer->payload;
    Rsource*		res;
    if(pld.cardinality == apgen::RES_CONTAINER_TYPE::SIMPLE) {
	res = pld.Object->array_elements[0];
	assert(res);
	res_used_by_THIS = dynamic_cast<RES_resource*>(res);

	assert(res_used_by_THIS);

    } else if(pld.cardinality
		    	== apgen::RES_CONTAINER_TYPE::ARRAY) {
	int flat_index = indices->evaluate_flat_index_for(
					pld.Type,
					ambient.object());
	res = pld.Object->array_elements[flat_index];
	res_used_by_THIS = dynamic_cast<RES_resource*>(res);

	assert(res_used_by_THIS);

    } else if(pld.cardinality
		    	== apgen::RES_CONTAINER_TYPE::SIGNAL) {
	assert(false);
    }
    return res_used_by_THIS;
}

void pEsys::AbstractUsage::refresh_args_and_call_usage_method(
			usage_event&	event) {
    behaving_base*	usage_object = event.Effect.object();
    const task&	useTask = usage_object->Task;

    //
    // process call arguments:
    //
    if(actual_arguments.size()) {

	//
	// we already checked - no need to assert
	//
	// assert(useTask.paramindex.size() == args->expressions.size());
	for(int i = 0; i < actual_arguments.size(); i++) {
	    behaving_object::temporary_storage
		    		= event.by_value_storage.object();

	    int param_index = useTask.paramindex[i];

	    actual_arguments[i]->eval_expression(
			    		event.Cause.object(),
					(*usage_object)[param_index]);

	    behaving_object::temporary_storage = NULL;
	}
    }

    //
    // That's all we do! The rest will be handled by the threads created
    // by AbstractUsage::execute()
    //
}

void pEsys::Usage::refresh_args_and_call_usage_method(
			usage_event&	event) {
	behaving_base*	usage_object = event.Effect.object();
	const task&	useTask = usage_object->Task;

	//
	// process call arguments:
	//
	if(actual_arguments.size()) {

		//
		// we already checked - no need to assert
		//
		// assert(useTask.paramindex.size() == args->expressions.size());
		for(int i = 0; i < actual_arguments.size(); i++) {
			behaving_object::temporary_storage = event.by_value_storage.object();
			int param_index = useTask.paramindex[i];

			actual_arguments[i]->eval_expression(event.Cause.object(), (*usage_object)[param_index]);


			behaving_object::temporary_storage = NULL;
		}
	}
	execution_context::return_code Code = execution_context::REGULAR;
	execution_context* ec = new execution_context(
					usage_object,
					useTask.prog.object());

	//
	// The useTask evaluates an internal resource variable called
	// "consumption"; the index of that variable in the resource's
	// symbol table is consumptionIndex. See
	// Resource::consolidate_implementation_phase(), the part dealing
	// with consumable/nonconsumable resources.
	//
	ec->ExCon(Code);
	delete ec;
	try {
		usage_object->get_concrete_res()->execute_use_clause(
			(*usage_object)[consumptionIndex],
			event.source);
	} catch(eval_error Err) {
		Cstring errs;
		errs << "Error in refresh_args_and_call_usage_method():\n"
			<< Err.msg << " - aborting REMODEL/SCHEDULE.\n";
		throw(eval_error(errs));
	}
}

void pEsys::SetUsage::refresh_args_and_call_usage_method(
				usage_event&	event) {
	behaving_base*	usage_object = event.Effect.object();
	const task&	setTask = usage_object->Task;
	Rsource*	res = usage_object->get_concrete_res();

	if(res->get_class() == apgen::RES_CLASS::STATE) {

	    //
	    // process call argument - we already checked that
	    // that this is a state resource, hence there is
	    // only one argument (the value set to)
	    //
	    behaving_object::temporary_storage = event.by_value_storage.object();
	    actual_arguments[0]->eval_expression(event.Cause.object(), (*usage_object)[1]);
	    behaving_object::temporary_storage = NULL;
	    res->execute_set_clause(
			(*usage_object)[1],
			event.source);
	} else {

	    //
	    // Settable resource. Process call arguments:
	    //
	    for(int i = 0; i < actual_arguments.size(); i++) {
		behaving_object::temporary_storage = event.by_value_storage.object();
		int param_index = setTask.paramindex[i];

		actual_arguments[i]->eval_expression(event.Cause.object(), (*usage_object)[param_index]);

		behaving_object::temporary_storage = NULL;
	    }

	    //
	    // If the resource is precomputed, set() takes no
	    // argument. There is an implied argument which is
	    // the time at which the set method was invoked.
	    // It can be obtained from event.source which is
	    // a mEvent*, whose Key has a getetime() method.
	    //
	    // If setTask has room for 1 parameter, we should
	    // stick the time value into it.
	    //

	    execution_context::return_code Code = execution_context::REGULAR;
	    execution_context* ec = new execution_context(
					usage_object,
					setTask.prog.object());

	    //
	    // The setTask evaluates an internal resource variable called
	    // "set_value"; the index of that variable in the resource's
	    // symbol table is consumptionIndex. See RES_consolidate.C,
	    // Resource::consolidate_implementation_phase(), the part dealing
	    // with settable resources.
	    //
	    ec->ExCon(Code);
	    delete ec;
	    res->execute_set_clause(
		(*usage_object)[consumptionIndex],
	    	event.source);
	}
}

void pEsys::ResetUsage::refresh_args_and_call_usage_method(
			usage_event&	event) {
	behaving_base* usage_object = event.Effect.object();
	usage_object->get_concrete_res()->execute_reset_clause(
				event.source);
}
