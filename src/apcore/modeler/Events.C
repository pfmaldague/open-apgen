#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define SCHED_VERBOSE

// #define apDEBUG
#include <time.h>
#include <assert.h>

#include "apDEBUG.H"
#include "action_request.H"
#include "ACT_exec.H"
#include "ActivityInstance.H"
#include "apcoreWaiter.H"
#include "APmodel.H"
#include "RES_def.H"
#include "RES_exec.H"
#include "EventImpl.H"

using namespace std;

Cstring describe_object(behaving_element obj) {
	if(obj) {
		return obj->Type.name;
	} else {
		return Cstring("empty object");
	}
}

tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& eventPLD::resources_with_no_dependencies() {
	static tlist<alpha_void, Cntnr<alpha_void, RCsource*> > C;
	return C;
}

Cstring get_owner_name(execution_context* context) {
	Cstring	owner_name;
	ActivityInstance* source_act = context->AmbientObject->get_req();
	behaving_base* source_res = context->AmbientObject.object();
	if(source_act) {
		owner_name = source_act->get_key();
	} else if(source_res) {
		owner_name = source_res->Type.name;
	} else {
		owner_name = "unknown object";
	}
	return owner_name;
}

long eventPLD::event_count = 0;

//
// do not use outside of factory (null payload), except for comparison
//
mEvent::mEvent( /* eventPLD::event_type et, */ const CTime_base& T)
	: baseC<alpha_time, mEvent, eval_intfc*>(T),
		/* EventType(et), */
		eventID(++currentEventID),
		Payload(NULL) {

	//
	// Only the modeling thread belongs here
	//
	assert(thread_index == thread_intfc::MODEL_THREAD);

	//
	// Never create an event in the past, _except_ in the
	// initial phase of remodeling, when ACT_exec asks each
	// activity in the plan to exercise its modeling or
	// resource usage section. It may be that resource usage
	// statements result in events prior to the start of the
	// first activity in the plan, which is the value at which
	// 'now' is set initially.
	//
	// That is not really a problem, since EventLoop::ProcessEvents()
	// will adjust the value of 'now' to reflect the earliest event
	// in the queue.  So, we allow this possibility as long as
	// CurrentEvent has not been defined.
	//
	if(EventLoop::CurrentEvent && T < thread_intfc::current_time(thread_index)) {

	    //
	    // Output a message to stderr so the user has some idea
	    // of what happened:
	    //
	    cerr << "assertion will fail - event time "
			<< T.to_string() << " is less than current time "
			<< thread_intfc::current_time(thread_index).to_string()
			<< "\n";
	    assert(T >= thread_intfc::current_time(thread_index));
	}
}

//
// for the initial event:
//
init_eventPLD::init_eventPLD(
		mEvent* parent) {
	// STATISTICS
	event_count++;

	parent->set_payload(this);
	source = parent;
}

int	dummy_int;

usage_event::usage_event(
		mEvent*			parent,
		execution_context*	c,
		behaving_element	obj_used,
		behaving_base*		storage_obj)
	: // Enabled(true),
		UsageClause(NULL),
		// OtherNode(NULL),
		from_to(false),
		Effect(obj_used),

		//
		// if this event is created by an abstract resource,
		// copying the AmbientObject ensures survival of the
		// list of symbols:
		//
		Cause(c->AmbientObject) {
	pEsys::Usage*	u = dynamic_cast<pEsys::Usage*>(c->get_instruction());
	assert(u);
	UsageClause = u;

	if(storage_obj) {
		by_value_storage.reference(storage_obj);
	}

	// STATISTICS
	event_count++;
	source = parent;
	source->set_payload(this);
}

usage_event::usage_event(
		mEvent*			parent,
		execution_context*	c,
		behaving_element	obj_used,
		bool			, // always true
		CTime_base		ToTime,
		behaving_base*		storage_obj)
	: // Enabled(true),
		UsageClause(NULL),
		// OtherNode(NULL),
		from_to(true),
		to_time(ToTime),
		Effect(obj_used),

		//
		// if this event is created by an abstract resource,
		// copying the AmbientObject ensures survival of the
		// list of symbols:
		//
		Cause(c->AmbientObject) {
	pEsys::Usage*	u = dynamic_cast<pEsys::Usage*>(c->get_instruction());
	assert(u);
	UsageClause = u;

	if(storage_obj) {
		by_value_storage.reference(storage_obj);
	}

	// STATISTICS
	event_count++;
	source = parent;
	source->set_payload(this);
}

const Cstring& init_eventPLD::get_key() const {
	static Cstring			S;

	S = Cstring("<mEvent ");
	S = S + Cstring(source->eventID) + " at " + source->getetime().to_string() + "|"
		+ Cstring(source->getKey().get_secondary_key());
	S << " INIT>\n";
	return S;
}

const Cstring& usage_event::get_key() const {
	static Cstring			S;

	S = Cstring("<mEvent ");
	S = S + Cstring(source->eventID) + " at " + source->getetime().to_string() + "|"
		+ Cstring(source->getKey().get_secondary_key());
	if(get_usage_clause()) {
		S << " created by {";
		get_usage_clause()->print(S);
		S << "}";
	}
	if(Cause) {
		S << " for " << Cause->Type.name;
	} else if(Cause->get_req()) {
		ActivityInstance*	act = Cause->get_req();
		S << " for " << act->identify();
	} else {
		S << " for no one";
	}
	S << " -- object used: "
//		<< (*Effect)[res_usage_object::ID].get_int() << ">";
		<< "TBD>";

	return S;
}

// For 'stealing' the stack, or more precisely the stack contents.
simple_wait_event::simple_wait_event(
		mEvent*			parent,
		execution_context*	EC) {

	//
	// Note: hiddenStack is empty; this causes it to reference the
	// context.
	//

	hiddenStack.push_context(EC);

	// STATISTICS
	event_count++;

	source = parent;
	source->set_payload(this);

	/* make sure the whole stack is re-created when execution resumes.
	 * Resumption is done in execStack::resume_execution().
	 * (current_context is normally the same as stack.last_node()) */

}

simple_wait_event::~simple_wait_event() {
}

const Cstring& simple_wait_event::get_key() const {
	static Cstring			S;
	execution_context*		context = getStack().get_context();

	S = Cstring("<mEvent ");
	S = S + Cstring(source->eventID) + " at " + source->getetime().to_string() + "|"
		+ Cstring(source->getKey().get_secondary_key());
	// if(get_wait_clause()) {
	// 	S << " created by {";
	// 	get_wait_clause()->print(S);
	// 	S << "}";
	// }
	if(context) {
		S << " CTXT" << " - "
			<< context->AmbientObject->Task.full_name();
	} else {
		S << " CTXT-1";
	}
	S << ">";

	return S;
}

execution_context* simple_wait_event::get_context() const {
	return getStack().get_context();
}

Cstring simple_wait_event::get_info() const {
	static Cstring			S;
	execution_context*		context = getStack().get_context();

	S = Cstring("<MODEL_EVENT  ");
	// if(get_wait_clause()) {
	// 	S << " created by {";
	// 	get_wait_clause()->print(S);
	// 	S << "}";
	// }
	if(context && context->AmbientObject->get_req()) {
		ActivityInstance*	act = context->AmbientObject->get_req();
		S << " for " << act->identify();
	} else if(context && context->AmbientObject) {
		S << " for " << context->AmbientObject->Type.name;
	} else {
		S << " for no one";
	}
	S << ">";

	return nonewlines(S);
}

void simple_wait_event::do_model() {
	pEsys::execStack*		stack = &getStack();
	execution_context*		context = stack->get_context();

	if(!(*context->AmbientObject)[0].get_int()) {
		return;
	}
	pEsys::WaitFor* wait_instr = dynamic_cast<pEsys::WaitFor*>(
					context->counter->Prog->statements[
						context->counter->I].object());

	assert(wait_instr);
	execution_context::return_code Code = execution_context::REGULAR;

	//
	// move to the next instruction
	//
	context->counter->I++;
	try {
		stack->Execute(Code);
	} catch(eval_error Err) {
		Cstring errs;
		errs << Err.msg;
		errs << "\nThis error occurred while processing "
			<< hiddenStack.get_context()->get_descr() << " at time " <<
			source->getetime().to_string() << "\n";
		throw(eval_error(errs));
	}
}

// For 'stealing' the stack, or more precisely the stack contents.
wait_until_event::wait_until_event(
		mEvent*			parent,
		execution_context*	EC) {

	//
	// Note: hiddenStack is empty; this causes it to reference the
	// context.
	//

	hiddenStack.push_context(EC);

	// STATISTICS
	event_count++;

	source = parent;
	source->set_payload(this);

	/* make sure the whole stack is re-created when execution resumes.
	 * Resumption is done in execStack::resume_execution().
	 * (current_context is normally the same as stack.last_node()) */

}

wait_until_event::~wait_until_event() {
}

const Cstring& wait_until_event::get_key() const {
	static Cstring			S;
	execution_context*		context = getStack().get_context();

	S = Cstring("<mEvent ");
	S = S + Cstring(source->eventID) + " at " + source->getetime().to_string() + "|"
		+ Cstring(source->getKey().get_secondary_key());
	// if(get_wait_clause()) {
	// 	S << " created by {";
	// 	get_wait_clause()->print(S);
	// 	S << "}";
	// }
	if(context) {
		S << " CTXT" << " - "
			<< context->AmbientObject->Task.full_name();
	} else {
		S << " CTXT-1";
	}
	S << ">";

	return S;
}

execution_context* wait_until_event::get_context() const {
	return getStack().get_context();
}

Cstring wait_until_event::get_info() const {
	static Cstring			S;
	execution_context*		context = getStack().get_context();

	S = Cstring("<MODEL_EVENT  ");
	// if(get_wait_clause()) {
	// 	S << " created by {";
	// 	get_wait_clause()->print(S);
	// 	S << "}";
	// }
	if(context && context->AmbientObject->get_req()) {
		ActivityInstance*	act = context->AmbientObject->get_req();
		S << " for " << act->identify();
	} else if(context && context->AmbientObject) {
		S << " for " << context->AmbientObject->Type.name;
	} else {
		S << " for no one";
	}
	S << ">";

	return nonewlines(S);
}

void wait_until_event::do_model() {
	pEsys::execStack*		stack = &getStack();
	execution_context*		context = stack->get_context();

	if(!(*context->AmbientObject)[0].get_int()) {
		return;
	}
	pEsys::WaitUntil* wait_instr = dynamic_cast<pEsys::WaitUntil*>(
					context->counter->Prog->statements[
						context->counter->I].object());
	assert(wait_instr);

	//
	// first execute the reenter method
	//
	execution_context::return_code Code = execution_context::REGULAR;
	if(wait_instr->reenter(context, Code, stack)) {

		//
		// then proceed with execution if OK
		//
		execution_context::return_code Code = execution_context::REGULAR;
		try {
			context->counter->I++;
			stack->Execute(Code);
		} catch(eval_error Err) {
			Cstring errs;
			errs << Err.msg;
			errs << "\nThis error occurred while processing "
				<< context->get_descr() << " at time " <<
				source->getetime().to_string() << "\n";
			throw(eval_error(errs));
		}
	}
}

// For 'stealing' the stack, or more precisely the stack contents.
wait_regexp_event::wait_regexp_event(
		mEvent*			parent,
		execution_context*	EC) {

	//
	// Note: hiddenStack is empty; this causes it to reference the
	// context.
	//
	hiddenStack.push_context(EC);

	// STATISTICS
	event_count++;

	source = parent;
	source->set_payload(this);

	/* make sure the whole stack is re-created when execution resumes.
	 * Resumption is done in execStack::resume_execution().
	 * (current_context is normally the same as stack.last_node()) */

}

wait_regexp_event::~wait_regexp_event() {
}

const Cstring& wait_regexp_event::get_key() const {
	static Cstring			S;
	execution_context*		context = getStack().get_context();

	S = Cstring("<mEvent ");
	S = S + Cstring(source->eventID) + " at " + source->getetime().to_string() + "|"
		+ Cstring(source->getKey().get_secondary_key());
	// if(get_wait_clause()) {
	// 	S << " created by {";
	// 	get_wait_clause()->print(S);
	// 	S << "}";
	// }
	if(context) {
		S << " CTXT" << " - "
			<< context->AmbientObject->Task.full_name();
	} else {
		S << " CTXT-1";
	}
	S << ">";

	return S;
}

execution_context* wait_regexp_event::get_context() const {
	return getStack().get_context();
}

Cstring wait_regexp_event::get_info() const {
	static Cstring			S;
	execution_context*		context = getStack().get_context();

	S = Cstring("<MODEL_EVENT  ");
	// if(get_wait_clause()) {
	// 	S << " created by {";
	// 	get_wait_clause()->print(S);
	// 	S << "}";
	// }
	if(context && context->AmbientObject->get_req()) {
		ActivityInstance*	act = context->AmbientObject->get_req();
		S << " for " << act->identify();
	} else if(context && context->AmbientObject) {
		S << " for " << context->AmbientObject->Type.name;
	} else {
		S << " for no one";
	}
	S << ">";

	return nonewlines(S);
}

void wait_regexp_event::do_model() {
	pEsys::execStack*		stack = &getStack();
	execution_context*		context = stack->get_context();

	if(!(*context->AmbientObject)[0].get_int()) {
		return;
	}
	pEsys::WaitUntilRegexp* wait_instr = dynamic_cast<pEsys::WaitUntilRegexp*>(
					context->counter->Prog->statements[
						context->counter->I].object());
	assert(wait_instr);

	//
	// first execute the reenter method
	//
	execution_context::return_code Code = execution_context::REGULAR;
	if(wait_instr->reenter(context, Code, stack)) {

		//
		// then proceed with execution if OK
		//
		execution_context::return_code Code = execution_context::REGULAR;
		try {
			context->counter->I++;
			stack->Execute(Code);
		} catch(eval_error Err) {
			Cstring errs;
			errs << Err.msg;
			errs << "\nThis error occurred while processing "
				<< context->get_descr() << " at time " <<
				source->getetime().to_string() << "\n";
			throw(eval_error(errs));
		}
	}
}

Cstring eventPLD::get_descr() const {
	Cstring S(source->getetime().to_string());

	S = S + " " + Cstring(source->eventID) + "|" + Cstring(source->getKey().get_secondary_key()) + " " + get_info();
	return S;
}

init_eventPLD::~init_eventPLD() {
	event_count--;
}

Cstring init_eventPLD::get_info() const {
	static Cstring			S;

	S = Cstring("<INIT_EVENT  ");
	S << " -- no resource used.>";
	return nonewlines(S);
}

Cstring usage_event::get_info() const {
	static Cstring			S;

	S = Cstring("<MODEL_EVENT  ");
	if(get_usage_clause()) {
		S << " created by {";
		get_usage_clause()->print(S);
		S << "}";
	}
	if(Cause->get_req()) {
		ActivityInstance*	act = Cause->get_req();
		S << " for " << act->identify();
	} else if(Cause) {
		S << " for " << Cause->Type.name;
	} else {
		S << " for no one";
	}
	S << " -- object used: "
//		<< (*getEffect())[res_usage_object::ID].get_int() << ">";
		<< "TBD>";

	return nonewlines(S);
}

usage_event::~usage_event() {
}

//Assume the node is in modeling window
void usage_event::do_model() {
	if(!(*Cause)[0].get_int() || !(*Effect)[0].get_int()) {
		return;
	}

	assert(model_control::get_pass() != model_control::INACTIVE);

	try {

		//
		// This call can rely on the fact that 'now'
		// is in sync with the potential_triggers
		// Miterator.
		//
		UsageClause->refresh_args_and_call_usage_method(*this);
	} catch(eval_error Err) {
		Cstring errs;
		errs	<< "File " << UsageClause->file << ", line " << UsageClause->line
			<< ": usage_event::do_model ERROR. Details:\n" << Err.msg;
		throw(eval_error(errs));
	}
}

resume_event::resume_event(
		mEvent*			parent,
		execution_context*	ec)
	: hiddenStack(ec) {

	event_count++;

	source = parent;
	source->set_payload(this);
}

void resume_event::do_model() {
	pEsys::execStack*		stack = &getStack();
	execution_context*		context = stack->get_context();

	if(!((*context->AmbientObject)[/* enabled = */ 0].get_int())) {
	    return;
	}
	assert(model_control::get_pass() != model_control::INACTIVE);

	if(APcloptions::theCmdLineOptions().debug_execute) {
	    cerr << "  do_model(): starting " << describe_object(context->AmbientObject)
		<< "-><execute expansion>()\n";
	    cerr.flush();
	}
	try {
	    execution_context::return_code Code = execution_context::REGULAR;
	    hiddenStack.Execute(Code);
	}
	catch(eval_error Err) {
	    Cstring errs;
	    errs << Err.msg;
	    errs << "\nThis error occurred while processing "
		<< hiddenStack.get_context()->get_descr()
		<< " at time " << source->getetime().to_string() << "\n";
	    throw(eval_error(errs));
	}
	if(APcloptions::theCmdLineOptions().debug_execute) {
	    cerr << "  do_model(): processed event of type 'expansion'\n";
	}
}

void init_eventPLD::do_model() {
	Rsource*	resource;
	init_event*	ie = dynamic_cast<init_event*>(source);
	int		res_priority = 0;

	assert(ie);

	tlist<alpha_string, Cntnr<alpha_string, Rsource*> > all_res(false);
	slist<alpha_string, Cntnr<alpha_string, Rsource*> >::iterator
						all_res_iter(all_res);
	Cntnr<alpha_string, Rsource*>*	ptr_to_res;

	EventLoop::theEventLoop().potential_triggers.clear();

	Rsource::get_all_resources_in_dependency_order(all_res);
	while((ptr_to_res = all_res_iter.next())) {
		resource = ptr_to_res->payload;
		if(!resource->isFrozen()) {
			try {
				resource->initialize_value(ie);
			} catch(eval_error Err) {
				Cstring any_errors;
				any_errors << Err.msg;
				throw(eval_error(any_errors));
			}
		}
		resource->evaluation_priority = res_priority++;

		RES_state*	state_res = dynamic_cast<RES_state*>(resource);
		RES_settable*	settable_res = dynamic_cast<RES_settable*>(resource);
		RES_numeric*	numeric_res = dynamic_cast<RES_numeric*>(resource);
		if(state_res) {
			state_res->get_history().addToIterator(
				EventLoop::theEventLoop().potential_triggers,
				1 + resource->evaluation_priority,
				/* store = */ true);
		} else if(settable_res) {
			settable_res->get_history().addToIterator(
				EventLoop::theEventLoop().potential_triggers,
				1 + resource->evaluation_priority,
				/* store = */ true);
		} else if(numeric_res) {
			numeric_res->get_history().addToIterator(
				EventLoop::theEventLoop().potential_triggers,
				1 + resource->evaluation_priority,
				/* store = */ true);
		}
	}
}

const Cstring& resume_event::get_key() const {
	static Cstring			S;
	execution_context*		context = getStack().get_context();

	S = Cstring("<mEvent ");
	S = S + Cstring(source->eventID) + " resume at " + source->getetime().to_string() + "|"
		+ Cstring(source->getKey().get_secondary_key());
	if(context) {
		S << " CTXT" << " - "
			<< context->AmbientObject->Task.full_name();
	} else {
		S << " CTXT-1";
	}
	S << ">";

	return S;
}

Cstring resume_event::get_info() const {
	return get_key();
}

execution_context* resume_event::get_context() const {
	return getStack().get_context();
}

void model_intfc::doModel(time_saver& masterTime) {
	CTime_base			t1;
	TypedValue			v;
	slist<alpha_time, mEvent>::iterator* event_iter;
	CTime_base			to_time;

	assert(model_control::get_pass() != model_control::INACTIVE);

	eval_intfc::Events().clear();
	eval_intfc::Events().add_thread(mEvent::initializationQueue(), "initialization events", 0);
	eval_intfc::Events().add_thread(mEvent::eventQueue(), "regular events", 1);
	eval_intfc::Events().add_thread(mEvent::currentQueue(), "current events", 2);
	eval_intfc::Events().add_thread(mEvent::expansionQueue(), "expansion events", 3);
	eval_intfc::Events().add_thread(mEvent::schedulingQueue(), "scheduling events", 4);

	try {
		EventLoop::theEventLoop().ProcessEvents();
	}
	catch(eval_error Err) {
		PurgeSchedulingEvents();
		eval_intfc::Events().clear();
		throw(Err);
	}
	if(LastEvent(t1)) {
		masterTime.set_now_to(t1);
	}

	eval_intfc::Events().clear();
	PurgeSchedulingEvents();
}


void model_intfc::doSchedule(
			bool				use_selection_list,
			bool				incremental,
			slist<alpha_void, dumb_actptr>&	top_kids,
			time_saver&			masterTime) {
    status_aware_iterator		active_acts(
			eval_intfc::get_act_lists().get_scheduled_active_iterator());
    slist<alpha_void, dumb_actptr>	pointers_to_activities;
    slist<alpha_void, dumb_actptr>::iterator copies_of_acts(pointers_to_activities);
    slist<alpha_void, smart_actptr>	copy_of_selection_list;
    slist<alpha_void, smart_actptr>::iterator selected_acts(copy_of_selection_list);
    dumb_actptr*			ptr;
    ActivityInstance*			req;
    CTime_base				t1;
    const char*				errcontext = "";
    TypedValue				v;

    try {
	if(!incremental) {
	    model_control control(model_control::SCHEDULING_1);

	    //
	    // STEP 1. We do a modeling pass to paint the background, so to speak
	    //

	    errcontext = "Step 1.1: ask model_intfc::init_model() to initialize everything\n";
	    model_intfc::init_model(masterTime);

	    errcontext = "Step 1.2: regenerate time events from all activities\n";
	    ACT_exec::ACT_subsystem().generate_time_events();

	    errcontext = "Step 1.3: call model_intfc::do_model\n";
	    model_intfc::doModel(masterTime);

	    //
	    // We know this because we just completed modeling
	    // from scratch
	    //
	    mEvent::firstEventIDbeforeScheduling = 1;

	    //
	    // We know this because we just finished modeling
	    //
	    mEvent::lastEventIDbeforeScheduling = mEvent::currentEventID;
	} else {

	    //
	    // This is the first event of the previous
	    // incremental modeling pass:
	    //
	    mEvent::firstEventIDbeforeScheduling = mEvent::lastEventIDbeforeScheduling + 1;

	    //
	    // We know this because we just finished scheduling
	    //
	    mEvent::lastEventIDbeforeScheduling = mEvent::currentEventID;
	}

	model_control control2(model_control::SCHEDULING_2);

	//
	// STEP 2. get rid of old events but keep resource histories
	//

	errcontext = "Step 2: ask model_intfc::init_model() to get rid of old events "
		"but keep resource histories\n";

	model_intfc::init_model(masterTime);

	//
	// STEP 3. regenerate events from all activities
	//

	errcontext = "Step 3: regenerate time events from all activities\n";

	ACT_exec::ACT_subsystem().generate_time_events();

	//
	// STEP 4. Store dumb pointers to scheduling activities in a List.
	//
	// Do this because some activities might get deleted during
	// scheduling, and the list of active requests could change
	// from under the feet of the iterator. Come to think of it,
	// this also means that if a scheduling activity is deleted
	// by someone else, we'll get a segm vio. Oh well. Scheduler
	// beware.
	//

	errcontext = "Step 4: Store pointers to selected acts in pointers_to_activities\n";
	if(use_selection_list) {
	    smart_actptr*	Ptrn;
	    /* this call gets selections that are not necessarily active,
	     * but are scheduled: */
	    eval_intfc::get_all_selections(copy_of_selection_list);
	    while((Ptrn = selected_acts())) {
		req = Ptrn->BP;
		pointers_to_activities << new dumb_actptr(req);
	    }
	} else {
	    // use all activities
	    while((req = active_acts())) {
		pointers_to_activities << new dumb_actptr(req);
	    }
	}

	//
	// STEP 5. We get the scheduling activities started.
	//
	// This used not to be mandatory but now it is.
	//
	// The activities' expansion section starts with a
	// wait for 0:0:0 statement which puts execution on
	// hold till now == start.
	//

	errcontext = "Step 5: Execute expansion programs of chosen scheduling activities\n";
	while((ptr = copies_of_acts())) {
	    req = ptr->payload;
	    act_object*	ao = dynamic_cast<act_object*>(req->Object.object());
	    assert(ao);
	    const Behavior& req_type = ao->Type;
	    map<Cstring, int>::const_iterator meth_iterator
				= req_type.taskindex.find("decompose");
	    if(meth_iterator != req_type.taskindex.end()) {
		task* decomp_task = req_type.tasks[meth_iterator->second];
		if(decomp_task->prog
		   && decomp_task->prog->section_of_origin()
					== apgen::METHOD_TYPE::CONCUREXP
		   && !req->hierarchy().children_count()) {
		    act_method_object*   amo = new act_method_object(req, *decomp_task);
		    behaving_element     be(amo);
		    execution_context*   AC = new execution_context(
							be,
							decomp_task->prog.object(),
							apgen::METHOD_TYPE::CONCUREXP
							);

		    masterTime.set_now_to(req->getetime());
		    pEsys::execStack*	new_stack = new pEsys::execStack(AC);
		    execution_context::return_code Code = execution_context::REGULAR;
		    new_stack->Execute(Code);
		    delete new_stack;
		}
	    }
	}

	//
	// STEP 6. Run the second pass of the modeling loop (a.k.a. scheduling loop)
	//

	if(model_intfc::FirstEvent(t1)) {
	    masterTime.set_now_to(t1);
	} else {
	    masterTime.set_now_to(CTime_base(time(0), 0, false));
	}

	if(model_intfc::HasEvents()) {
	    errcontext = "Step 6. emulate AAF_do_model() for the 2nd modeling pass\n";
	    model_intfc::doModel(masterTime);
	}

	//
	// Note that scheduling activities were only processed if they had no children.
	// This provides us with an easy way to grab the top-level children.
	//
	while((ptr = copies_of_acts())) {
	    req = ptr->payload;
	    // anything scheduled by req?
	    if(req->hierarchy().children_count()) {
		// yes
		slist<alpha_void, smart_actptr>::iterator li(req->get_down_iterator());
		smart_actptr*				  p;

		while((p = li())) {
		    ActivityInstance*	a = p->BP;
		    top_kids << new dumb_actptr(a);
		}
	    }
	}
    }
    catch(eval_error Err) {
	Cstring errors(errcontext);
	model_intfc::PurgeSchedulingEvents();
	errors << " -\n" << Err.msg;
	throw(eval_error(errors));
    }
}

//
// EVENT FACTORIES
//

simple_wait_event*	simple_wait_event::simpleWaitEventFactory(
				execution_context*	current_context) {
	if(EventLoop::CurrentEvent && time_saver::get_now() < EventLoop::CurrentEvent->getetime()) {
		// anticipate problems rather than trying to fix them later
		return NULL;
	}
	mEvent*		newtimenode = new mEvent(
						/* eventPLD::EXPANSION, */
						time_saver::get_now());
			// grabs the context:
	simple_wait_event*	event_payload = new simple_wait_event(
		       				newtimenode,
						current_context);

	return event_payload;
}

wait_regexp_event*	wait_regexp_event::waitRegexpEventFactory(
				execution_context*	current_context) {
	if(EventLoop::CurrentEvent && time_saver::get_now() < EventLoop::CurrentEvent->getetime()) {
		// anticipate problems rather than trying to fix them later
		return NULL;
	}
	mEvent*		newtimenode = new mEvent(
						/* eventPLD::EXPANSION, */
						time_saver::get_now());
			// grabs the context:
	wait_regexp_event*	event_payload = new wait_regexp_event(
		       				newtimenode,
						current_context);

	return event_payload;
}

wait_until_event*	wait_until_event::waitUntilEventFactory(
				execution_context*	current_context) {
	if(EventLoop::CurrentEvent && time_saver::get_now() < EventLoop::CurrentEvent->getetime()) {
		// anticipate problems rather than trying to fix them later
		return NULL;
	}
	mEvent*		newtimenode = new mEvent(
						/* eventPLD::EXPANSION, */
						time_saver::get_now());
			// grabs the context:
	wait_until_event*	event_payload = new wait_until_event(
		       				newtimenode,
						current_context);

	return event_payload;
}

resume_event*	resume_event::resumeEventFactory(
				execution_context*	current_stack) {
	if(EventLoop::CurrentEvent && time_saver::get_now() < EventLoop::CurrentEvent->getetime()) {
		// anticipate problems rather than trying to fix them later
		return NULL;
	}
	mEvent*		newtimenode = new mEvent(
						/* eventPLD::EXPANSION, */
						time_saver::get_now());
			// steals the stack:
	resume_event*	event_payload = new resume_event(
		       				newtimenode,
						current_stack);

	//
	// We come from a WAIT instruction pointed at by the program counter.
	// We need to increment the counter so we can move on.
	//

	current_stack->counter->I++;

	return event_payload;
}

usage_event*	usage_event::resEventFactory(
				execution_context*	context,
				behaving_element	object_used,
				behaving_base*		storage) {
	
		/* T is one of the following two:
		 *	eventPLD::EVAL
		 *	eventPLD::START_USAGE */

	if(EventLoop::CurrentEvent
	   && time_saver::get_now() < EventLoop::CurrentEvent->getetime()) {

		//
		// anticipate problems rather than trying to fix them later
		//
		return NULL;
	}
	mEvent*		newtimenode = new mEvent(
						time_saver::get_now());
	usage_event*	event_payload = new usage_event(
						newtimenode,
						context,
						object_used,
						storage);
	return event_payload;
}

usage_event*	usage_event::resEventFactory(
				execution_context*	context,
				behaving_element	object_used,
				bool			FromTo,
				CTime_base		ToTime,
				behaving_base*		storage) {
	
		/* T is one of the following two:
		 *	eventPLD::EVAL
		 *	eventPLD::START_USAGE */

	if(EventLoop::CurrentEvent
	   && time_saver::get_now() < EventLoop::CurrentEvent->getetime()) {

		//
		// anticipate problems rather than trying to fix them later
		//
		return NULL;
	}
	mEvent*		newtimenode = new mEvent(
						time_saver::get_now());

	//
	// Each mEvent has a unique eventID which is set to
	// ++currentEventID in the constructor. This ID is
	// later used to index history nodes. Since a from-to
	// usage results in two history nodes instead of one,
	// we increment currentEventID _twice_ to make sure
	// that the hsitory node indices are all distinct.
	// This means that there will be no mEvent with the
	// such IDs, but that should be of no consequence.
	//
	mEvent::currentEventID++;

	usage_event*	event_payload = new usage_event(
						newtimenode,
						context,
						object_used,
						FromTo,
						ToTime,
						storage);
	return event_payload;
}

usage_event*	usage_event::signalEventFactory(
				execution_context*	context) {
	if(EventLoop::CurrentEvent && time_saver::get_now() < EventLoop::CurrentEvent->getetime()) {
		// anticipate problems rather than trying to fix them later
		return NULL;
	}
	behaving_element	SO(new signal_object("signal"));
	mEvent*		newtimenode = new mEvent(
						/* ACT_SIGNAL, */
						time_saver::get_now());
	usage_event*	event_payload = new usage_event(
						newtimenode,
						context,
						SO);

	return event_payload;
}

init_eventPLD*	init_event::initialEventFactory(
				CTime_base		init_time) {
	mEvent*		newtimenode = new init_event(init_time);
	init_eventPLD*	event_payload = new init_eventPLD(newtimenode);

	return event_payload;
}
