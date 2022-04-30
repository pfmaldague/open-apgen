#if HAVE_CONFIG_H
#include <config.h>
#endif
// #define apDEBUG

#include <unistd.h>
#include <assert.h>

#include "apDEBUG.H"

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

extern apgen::RETURN_STATUS	first_time_waiting_for_signal_or_condition(
					pEsys::executableExp&	source_clause,
					pEsys::execStack&	stack,
					Cstring&		errors,
					exec_status&		eStatus);

// in RES_exec.C:
extern int		scheduling_disabled;

void pEsys::WaitFor::execute(
		execution_context*	context,
		execution_context::return_code& Code,
		execStack*		stack_to_use) {

	// simple wait

	CTime_base		evaluated_duration(true);
	bool			we_need_to_wait = true;
	TypedValue		durval;

	Expression->eval_expression(context->AmbientObject.object(), durval);
	if(durval.is_time()) {
		CTime_base	desired_time = durval.get_time_or_duration();
		if(time_saver::get_now() >= desired_time) {
			we_need_to_wait = false;
		} else {
			evaluated_duration = desired_time - time_saver::get_now();
		}
	} else if(!durval.is_duration()) {
		Cstring err;
		err << "File " << file << ", line " << line << ": executing WAIT_SIMPLE"
			<< "; invalid duration expression " << Expression->to_string();
		throw(eval_error(err));
	} else {
		evaluated_duration = durval.get_time_or_duration();
	}

	if(we_need_to_wait) {
		if(evaluated_duration < CTime_base(0, 0, true)) {
			Cstring err;
			err << "File " << file << ", line " << line << ": executing WAIT_SIMPLE"
				<< "; Found negative duration: expression "
				<< Expression->to_string() << " evaluates to "
				<< evaluated_duration.to_string();
			throw(eval_error(err));
		}

		/* the mEvent constructor will
		 *	- grab the current Stack, undefine it and set its status
		 *	  to EXEC_INTERRUPTED
		 *	- set the mEvent's EventType to EXPANSION_EVENT
		 *	- set the status returned by the NEW stack
		 *	  to REENTERING_FROM_EVENT.
		 */
		time_saver		TS;
		simple_wait_event*	new_thread;

		TS.set_now_to(time_saver::get_now() + evaluated_duration);
		if((new_thread = simple_wait_event::simpleWaitEventFactory(context))) {
			model_intfc::add_timevent(
				new_thread->source,
				context->what_section());
		} else {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": 'WAIT FOR' error: trying to create an event at time "
				<< time_saver::get_now().to_string()
				<< " earlier than Current Event time ";
			if(EventLoop::CurrentEvent) {
				err << EventLoop::CurrentEvent->getetime().to_string() << "\n";
			} else {
				err << "unknown\n";
			}
			throw(eval_error(err));
		}

		//
		// execution_context::Execute() will increment the instruction
		// pointer, so the new thread event will pick up right after
		// this instruction
		//

		Code = execution_context::WAITING;
	} else {
		/* SIMPLE WAIT for a time in the past - moving right along */
	}
}

bool pEsys::WaitUntil::reenter(
		execution_context*	context,
		execution_context::return_code& Code,
		execStack*		stack_to_use) {

	// case WAIT UNTIL signal or WAIT UNTIL condition
	Cstring	errors;
	exec_status eStatus;

	/* NOTE: better make sure that the start-time is properly set through the
	 * context when calling execute... */
	if(first_time_waiting_for_signal_or_condition(
		*this,
		*stack_to_use,
		errors,
		eStatus) != apgen::RETURN_STATUS::SUCCESS) {
		throw(eval_error(errors));
	}
	//
	// We need some kind of signal from first_time_waiting that we may
	// proceed
	//

	//
	// the event will continue processing the program if no wait is
	// required
	//

	return eStatus.state == exec_status::NOOP;
}

void pEsys::WaitUntil::execute(
		execution_context*	context,
		execution_context::return_code& Code,
		execStack*		stack_to_use) {

	/* first time we encounter this WAIT instruction; create a wait_event
	 * that will be inserted in the expansion queue.  This will guarantee
	 * that the WAIT is executed after all pending usage events have been
	 * executed first. */

	wait_until_event*	new_thread = wait_until_event::waitUntilEventFactory(context);
	if(new_thread) {
		model_intfc::add_timevent(
				new_thread->source,
				context->what_section());
	}

	Code = execution_context::WAITING;
}

bool pEsys::WaitUntilRegexp::reenter(
		execution_context*	context,
		execution_context::return_code& Code,
		execStack*		stack_to_use) {
	// case WAIT UNTIL signal
	Cstring	errors;
	exec_status eStatus;

	/* NOTE: better make sure that the start-time is properly set through the
	 * context when calling execute... */
	if(first_time_waiting_for_signal_or_condition(
		*this,
		*stack_to_use,
		errors,
		eStatus) != apgen::RETURN_STATUS::SUCCESS) {
		throw(eval_error(errors));
	}
	return eStatus.state == exec_status::NOOP;
}

void pEsys::WaitUntilRegexp::execute(
		execution_context*	context,
		execution_context::return_code& Code,
		execStack*		stack_to_use) {

	/* first time we encounter this WAIT instruction; create a wait_event
	 * that will be inserted in the expansion queue.  This will guarantee
	 * that the WAIT is executed after all pending usage events have been
	 * executed first. */
	wait_regexp_event*	new_thread = wait_regexp_event::waitRegexpEventFactory(
									context);
	if(new_thread) {
		model_intfc::add_timevent(
				new_thread->source,
				new_thread->getStack().get_context()->what_section());
		Code = execution_context::WAITING;
	}
}

void pEsys::GetWindows::execute(
		execution_context*	context,
		execution_context::return_code& Code,
		execStack*		stack_to_use) {
	try {
		get_windows_of_opportunity(
			stack_to_use);
	} catch(eval_error Err) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": error evaluating windows; details:\n"
			<< Err.msg;
		throw(eval_error(errs));
	}
}


void pEsys::GetInterpwins::execute(
		execution_context*	context,
		execution_context::return_code& Code,
		execStack*		stack_to_use) {
	try {
		get_windows_of_opportunity(
			stack_to_use);
	} catch(eval_error Err) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": error evaluating interpolated windows; details:\n"
			<< Err.msg;
		throw(eval_error(errs));
	}
}
