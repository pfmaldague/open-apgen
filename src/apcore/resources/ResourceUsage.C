#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <assert.h>

#include "action_request.H"
#include "ActivityInstance.H"
#include "apcoreWaiter.H"
#include "AbstractResource.H"
#include "EventImpl.H"
#include "EventLoop.H"
#include "RES_eval.H"
#include "RES_def.H"
#include "xmlrpc_api.H"

#ifdef have_xml_reader
extern "C" {
#	include <dom_chunky_xml_intfc.h>
} // extern "C"
#endif /* have_xml_reader */

using namespace std;

				// in ACT_edit.C:
extern void			initialize_all_schedulers();

void RES_resource::execute_set_clause(
		TypedValue&,
		mEvent*,
		bool) {
	Cstring errs = "SHOULD NOT HAPPEN";
	throw(eval_error(errs));
}

void RES_resource::execute_reset_clause(
		mEvent*,
		bool) {
	Cstring errs = "SHOULD NOT HAPPEN";
	throw(eval_error(errs));
}

void RES_resource::execute_use_clause(
		TypedValue&,
		mEvent*,
		bool) {
	Cstring errs = "SHOULD NOT HAPPEN";
	throw(eval_error(errs));
}

RES_state::RES_state(
		RCsource&	p,
		Behavior&	T,
		const Cstring&	namein)
	: RES_resource(p, T, namein),
		resHistory(this /* , new state_res_historyWizard */ ),
		theRangeIsTheDefault(true),
		usage_events("usage history") {
}

RES_state::~RES_state() {
}

void RES_state::add_state(tvslist* stateptr, bool is_default) {
	Cstring			retstr;
	TaggedValue*		N;

	theRangeIsTheDefault = is_default;
	for(	N = stateptr->first_node();
		N;
		N = N->next_node()) {
		if(N->payload.is_array()) {
			ArrayElement*	tds;

			for(int k = 0; k < N->payload.get_array().get_length(); k++) {
				tds = N->payload.get_array()[k];
				rangevec.push_back(tds->Val());
				rangemap[(void*)tds->Val()] = true;
				// range << new TaggedValue("", tds->Val());
			}
		} else {
			rangevec.push_back(N->payload);
			rangemap[(void*)N->payload] = true;
			// range << new TaggedValue("", N->payload);
		}
	}
}

//
// While not much happens here, it would be efficient if we knew ahead of time
// that 'not' agrees with Miterclass::current_time(thread_index), so that the
// potential_triggers Miterator could be use to determine the present value,
// instead of searching through a dense history of states.
//
// The search does not occur in execute_set_clause; it occurs in
// append_to_history(), where the code searches history starting at the root.
// We should be able to do better than that.
//
void RES_state::execute_set_clause(
		TypedValue&		evaluated_value,
		mEvent*			source,
		bool			immed) {

	//
	// check to make sure it is one of the possible values
	//
	map<void*, bool>::iterator iter = rangemap.find((void*) evaluated_value);
	if(iter == rangemap.end()) {
		Cstring errs;
		errs << "Resource " << name << " evaluated value "
			<< evaluated_value.to_string() << " is not one of the possible states #";
		throw(eval_error(errs));
	}

	if(immed) {
		immediately_append_to_history(
				evaluated_value,
				apgen::USAGE_TYPE::SET);
	} else {
		append_to_history(	evaluated_value,
					source->getetime(),
					source->getetime(),	// dummy arg
					source->get_event_id(),
					0,			// dummy arg
					apgen::USAGE_TYPE::SET);
	}
}

void RES_state::execute_reset_clause(
		mEvent* source,
		bool immed) {
	TypedValue	evaluated_value;

	try {
		evaluate_profile(
			EvalTiming::NORMAL,
			time_saver::get_now(),
			evaluated_value);
	}
	catch(eval_error Err) {
		Cstring errs;
		errs << "; this error occurred while executing reset clause "
			<< name << "\n";
		Err.msg = errs;
		throw(Err);
	}

	if(immed) {
		immediately_append_to_history(
				evaluated_value,
				apgen::USAGE_TYPE::RESET);
	} else {
		append_to_history(	evaluated_value,
					source->getetime(),
					source->getetime(),	// dummy arg
					source->get_event_id(),
					0,			// dummy arg
					apgen::USAGE_TYPE::RESET);
	}
}

void  RES_state::execute_use_clause(
		TypedValue&	val,
		mEvent*		E,
		bool		immed) {
	if(immed) {
		throw(eval_error("state resource cannot be used immediately"));
	}

	usage_event*	ue = dynamic_cast<usage_event*>(E->get_payload());
	assert(ue);


	//
	// check to make sure val is one of the possible values
	//
	map<void*, bool>::iterator iter = rangemap.find((void*) val);

	if(APcloptions::theCmdLineOptions().debug_execute) {
		cerr << name << ": using state ";
		cerr << val.to_string() << "\n";
	}
	if(iter == rangemap.end()) {
		Cstring errs;
		errs << "Resource " << name << " evaluated value "
			<< val.to_string() << " is not one of the possible states ";
		throw(eval_error(errs));
	}

	//
	// NOTE: for a state resource, append_to_history
	// stores BOTH start and end of use.
	//
	long id = E->get_event_id();
	append_to_history(	val,
				E->getetime(),
				ue->to_time,
				id,
				id + 1,
				/* the_clause->usage_type */ apgen::USAGE_TYPE::USE);
}

void RES_consumable::execute_use_clause(
		TypedValue&	amountconsumed,
		mEvent*		E,
		bool		immed /* = false */) {
	if(immed) {
		immediately_append_to_history(
				amountconsumed,
				apgen::USAGE_TYPE::USE);	// dummy arg
	} else {
		append_to_history(
				amountconsumed,
				E->getetime(),
				E->getetime(),	// dummy arg
				E->get_event_id(),
				0,		// dummy arg
				apgen::USAGE_TYPE::USE);	// dummy arg
	}
}

void RES_nonconsumable::execute_use_clause(
		TypedValue&	amountconsumed,
		mEvent*		E,
		bool		immed) {
	if(immed) {
		throw(eval_error("nonconsumable resource cannot be used immediately"));
	}
	usage_event*	ue = dynamic_cast<usage_event*>(E->get_payload());
	assert(ue);
	long id = E->get_event_id();
	append_to_history(
			amountconsumed,
			E->getetime(),
			ue->to_time,
			id,
			id + 1,	// this is the event ID the 'to event'
				// would have had if we had created it
			apgen::USAGE_TYPE::USE); // dummy arg
}

//setup for Rcontainer::dump() and RES_resource::dump() :
static int d_d;
#define DOUT for(d_d = 0; d_d < pre; d_d++) cout << " "; cout

//setup for RES_resource::dump() is above Rcontainer::dump() :
void RES_resource::dump(int pre) {
}

void RES_settable::execute_set_clause(
		TypedValue&	evaluated_value,
		mEvent*		source,
		bool		immed) {

	if(immed) {
		immediately_append_to_history(
				evaluated_value,
				apgen::USAGE_TYPE::SET);
	} else {
		append_to_history(	evaluated_value,
					source->getetime(),
					CTime_base(),	// dummy arg
					source->get_event_id(),
					0,		// dummy arg
					apgen::USAGE_TYPE::SET);
	}
}

void RES_settable::evaluate_profile(
			EvalTiming::Strategy,
			CTime_base,
			TypedValue&	evaluated_value) {

	//
	// settable resources only have one profile node:
	//
	Cnode0<alpha_time, parsedExp>*	N = theProfile.first_node();

	if(!N) {
		Cstring errs;
		errs << "Settable resource " << name
			<< " has no default; cannot evaluate.\n";
		throw(eval_error(errs));
	}

	N->payload->eval_expression(
				Object.object(),
				evaluated_value);
}

bool abs_res_object::print(
		aoString& S,
		const char* prefix) const {
	behaving_base* constr = level[1];
	S << prefix << "[type = "

		//
		// The TYPE member of the abstract resource already
		// has all the information we need
		//
		// << Task.Type.full_name()
		<< constr->operator[](ActivityInstance::TYPE).get_string()
		<< ", id = "
		<< constr->operator[](ActivityInstance::ID).get_string()
		<< "]";
	return true;
}

void Rsource::evaluate_profile(
			EvalTiming::Strategy	strategy,
			CTime_base		at_time,
			TypedValue&		evaluated_value) {
	time_saver			ts;
	EvalTiming::strategy_setter	setter(this, strategy);

	ts.set_now_to(at_time);

	Cnode0<alpha_time, parsedExp>*	N = theProfile.find_at_or_before(at_time);
	if(!N) N = theProfile.first_node();
	if(!N) {
		Cstring errs;
		errs << name << " has empty profile; cannot evaluate.\n";
		throw(eval_error(errs));
	}

	try {
		N->payload->eval_expression(
				Object.object(),
				evaluated_value);
	} catch(eval_error Err) {
		Cstring			errs;
		errs << name << " cannot evaluate profile at time "
			<< at_time.to_string() << ":\n" << Err.msg << "\n";
		throw(eval_error(errs));
	}
	if(evaluated_value.get_type() == apgen::DATA_TYPE::UNINITIALIZED) {
		Cstring errs;
		errs << name << " cannot evaluate profile at time "
			<< at_time.to_string() << " because the expression "
			<< " in file " << N->payload->file << ", line "
			<< N->payload->line << " evaluates to an undefined value.";
		throw(eval_error(errs));
	}
}


static long res_usage_object_count = 0;

res_usage_object::res_usage_object(
		Rsource*	r,
		task&		res_task)
	: concres(r),
		behaving_object(res_task, r->Object.object()) {
	operator[](ENABLED) = true;
	behaving_element	be;

#ifdef NO_SHENANIGANS
	values[ID] = res_usage_object_count++;
	values[THIS] = be;
	// we need to do this to avoid invoking the smart_ptr destructor:
	values[THIS].get_object().reference(this);
#endif /* NO_SHENANIGANS */
}	

bool res_usage_object::print(
		aoString& S,
		const char* prefix) const {
	Cstring t;
	if(concres) {
		S << "[\"type\" = " << addQuotes(Cstring("concrete-res-usage::") + concres->name)
//			<< ", \"id\" = " << addQuotes((*this)["id"].get_string()) << "]";
			<< ", \"id\" = \"TBD\"]";
	} else {
		return false;
	}
	return true;
}

static long act_method_object_count = 0;

//
// The first argument specifies the object on which the
// method is called (an instance of the activity Behavior)
// while the second argument specifies which method of
// that Behavior is to be executed:
//
act_method_object::act_method_object(
		ActivityInstance*	a,
		task&			act_task)
	: behaving_object(act_task,
			a->Object.object()) {
	
	// every activity method has to have enabled and id defined:

#ifdef NO_SHENANIGANS
	assert(act_task.varinfo.size() >= 2);
	values[ID] = Cstring("act-method_") + Cstring(act_method_object_count++)
		+ "::" + a->get_unique_id();
#endif /* NO_SHENANIGANS */
	operator[](ENABLED) = true;
}	

bool act_method_object::print(
		aoString& S,
		const char* prefix) const {
	behaving_base* constr = level[1];
	S << prefix << "[type = "
		<< constr->operator[](ActivityInstance::TYPE).get_string()
		<< ", id = "
		<< constr->operator[](ActivityInstance::ID).get_string()
		<< "]";
	return true;
}
