#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>

#include "AbstractResource.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "C_list.H"
#include "EventImpl.H"
#include "EventLoop.H"
#include "RES_def.H"
#include "apcoreWaiter.H"

#include <assert.h>

extern coutx excl;
extern streamx& tol_debug_stream();

using namespace std;

bool Rsource::evaluation_debug = false;

extern thread_local int thread_index;

TypedValue convert_from_double_to_typed_value(double in_val, apgen::DATA_TYPE T) {
    TypedValue V;
    switch(T) {
	case apgen::DATA_TYPE::FLOATING:
	    V = in_val;
	    break;
	case apgen::DATA_TYPE::INTEGER:
	    V = (long int)in_val;
	    break;
	case apgen::DATA_TYPE::TIME:
	    V = CTime_base::convert_from_double_use_with_caution(in_val, false);
	    break;
	case apgen::DATA_TYPE::DURATION:
	    V = CTime_base::convert_from_double_use_with_caution(in_val, true);
	    break;
	default:
	    assert(false);
    }
    return V;
}


dummyHistory& RES_resource::emptyHist() {
	static dummyHistory h;
	return h;
}

RES_resource::RES_resource(
		RCsource&	p,
		Behavior&	T,
		const Cstring&	namein)
	: Rsource(p, T, namein, p.get_class()) {
}

RES_resource::RES_resource(const RES_resource& Rr)
	: Rsource(Rr.parent_container, Rr.Type, Rr.name, Rr.get_class()) {
}

RES_resource::~RES_resource() {}

bool RES_resource::is_hidden() const {
	return properties[(int)Property::has_hidden];
}

//
// In AP_exp_eval.C:
//
extern void recursively_add_indices(
		flexval& l,
		vector<string>& it,
		int);

void eval_intfc::get_resource_subsystems(flexval& F) {
	flexval					subsystems;
	slist<alpha_string, RCsource>::iterator	cont_iter(RCsource::resource_containers());
	RCsource*				container;

	if(!RCsource::resource_containers().get_length()) {
		flexval&	subvar(subsystems["<Default>"]);
		flexval&	level(subvar["None"]);
		level = true;
	}
	while((container = cont_iter())) {
		vector<Rsource*>& vec = container->payload->Object->array_elements;

		for(int i = 0; i < vec.size(); i++) {
			Rsource*	rsource = vec[i];
			RES_resource*	res = dynamic_cast<RES_resource*>(rsource);
			assert(res);

			//
			// NOTE: the various elements of a resource array
			//	 do not have to share the same subsystem
			//
			Cstring		subName("<Default>");
			bool		subsystem_is_defined = res->properties[(int)Rsource::Property::has_subsystem];

			if(subsystem_is_defined) {
				subName = (*res->Object)["subsystem"].get_string();
			}

			flexval&	subvar(subsystems[*subName]);
			flexval&	level(subvar[*container->get_key()]);
			vector<string>	full_indices;
			for(int j = 0; j < res->indices.size(); j++) {
				stringstream s;
				s << "[\"" << *res->indices[j] << "\"]";
				full_indices.push_back(s.str());
			}
			if(container->is_array()) {
				recursively_add_indices(level, full_indices, 0);
			} else {
				level = true;
			}
		}
	}
	F["subsystems"] = subsystems;
}

map<Cstring, int>& STATS_settable() {
	static map<Cstring, int> M;
	return M;
}

map<Cstring, int>& STATS_set() {
	static map<Cstring, int> M;
	return M;
}

void RES_state::append_to_history(
		TypedValue&		new_value,
		CTime_base		start_event_time,
		CTime_base		end_event_time,
		long			start_event_id,
		long			end_event_id,
		apgen::USAGE_TYPE	uType) {
	value_node*			vn;
	value_node*			closest_node;
	int				depth_at_first = 0;

	miterator_data*	res_miter_data = peek_at_miterator_data();

	if(APcloptions::theCmdLineOptions().TOLdebug
	   && name == APcloptions::theCmdLineOptions().theResourceToDebug) {
	    Cstring str;
	    str << name << "->append(T=" << start_event_time.to_string()
		<< "), val = " << new_value.to_string() << "\n";
	    tol_debug_stream() << str;
	}

	if(res_miter_data && start_event_time
			     == thread_intfc::current_time(thread_index)) {
	    assert(time_saver::get_now() == start_event_time);

	    closest_node = resHistory.latestHistNodeAtOrBeforeModelTime(
					start_event_time,
					res_miter_data->iter);
	} else {
	    closest_node = get_history().latestHistNodeBefore(time_saver::get_now());
	}

	if(closest_node) depth_at_first = closest_node->get_usage_depth();

	{

	    //
	    // We need this because evaluating currentval()
	    // requires scanning the history list, and we
	    // are going to mess up that list when we insert
	    // history nodes, below
	    //
	    lock_guard<mutex>		lock(res_mutex);

	    if(uType == apgen::USAGE_TYPE::USE) {
		value_node*			end_use;
		int				depth_at_end = 0;

		Add(	vn = value_node::stateFactory(
					start_event_time,
					new_value,
					start_event_id,
					depth_at_first,
					apgen::RES_VAL_TYPE::START_USING,
					*this),
			get_datatype());

		closest_node = get_history().latestHistNodeBefore(end_event_time);
		if(	closest_node
			&& closest_node != vn) {
			depth_at_end = closest_node->get_usage_depth();
		} else {
			depth_at_end = depth_at_first;
		}

		Add(	end_use = value_node::stateFactory(
					end_event_time,
					vn,
					end_event_id,
					depth_at_end,
					apgen::RES_VAL_TYPE::END_USING,
					*this),
			get_datatype());

		while(vn && vn != end_use) {
			vn->get_usage_depth()++;
			vn = vn->following_node();
		}
	    } else if(uType == apgen::USAGE_TYPE::SET) {

		//
		// New behavior required by JIRA AP-745
		//
		// ... modified: until a correct fix is
		// available for AP-814, the no_filtering
		// attribute is always honored, even for
		// state resources.
		//
		if(	!properties[(int)Property::has_nofiltering] // mandated by AP-814
			&& closest_node
			&& closest_node->get_res_val_type() == apgen::RES_VAL_TYPE::SET
			&& closest_node->get_value() == new_value) {
#ifdef NO_STATISTICS
			//
			// collect statistics for now
			//
			map<Cstring, int>::iterator it9 = STATS_set().find(name);
			if(it9 == STATS_set().end()) {
				STATS_set()[name] = 1;
			} else {
				STATS_set()[name]++;
			}
#endif /* NO_STATISTICS */

			return;
		}

		Add(	vn = value_node::stateFactory(
					start_event_time,
					new_value,
					start_event_id,
					depth_at_first,
					apgen::RES_VAL_TYPE::SET,
					*this),
			get_datatype());
	    } else if(uType == apgen::USAGE_TYPE::RESET) {
		Add(	vn = value_node::stateFactory(
					start_event_time,
					new_value,
					start_event_id,
					depth_at_first,
					apgen::RES_VAL_TYPE::RESET,
					*this),
			get_datatype());
	    }
	}
}

void RES_state::immediately_append_to_history(
		TypedValue&		new_value,
		apgen::USAGE_TYPE	uType) {
	if(uType == apgen::USAGE_TYPE::USE) {
		Cstring errs;
		errs << "State resource " << name
			<< " is of type state; it can only be set or reset immediately, not used.";
		throw(eval_error(errs));
	}

	append_to_history(
		new_value,
		time_saver::get_now(),
		CTime_base(),
		++mEvent::currentEventID,
		0,
		uType);
}

//
// Note a difference between this version of initialize_value() and
// the next one, which has a time argument: this version only clears
// the history if the current model control pass is NOT the second
// scheduling pass.
//
// However, we can always clear the usage events Miterator.
//
void RES_state::initialize_value(
		init_event*		source) {

	if(isFrozen()) {
		Cstring rmessage;
		rmessage << "initialize_value: Resource " << name << " is frozen.";
		throw(eval_error(rmessage));
	}
	if(isImported()) {

		//
		// We want to use the history now, but not next time.
		//
		get_history().setImported(false);
		return;
	}

	if(model_control::get_pass() != model_control::SCHEDULING_2) {
		get_history().clear();
	}
	usage_events.clear();

	TypedValue	result_of_evaluation;
	try {
		evaluate_profile(
				EvalTiming::NORMAL,
				time_saver::get_now(),
				result_of_evaluation);
	}
	catch(eval_error Err) {
		Cstring rmessage;
		rmessage << "RES_state::initialize_value ERROR:\n" << Err.msg;
		throw(eval_error(rmessage));
	}

	try {
		resHistory.Add(
			value_node::stateFactory(
				source->getetime(),
				result_of_evaluation,
				source->eventID,
				0,
				apgen::RES_VAL_TYPE::RESET,
				*this),
			get_datatype());
	} catch(eval_error Err) {
		throw(Err);
	}
}

//
// for XML TOL ingestion:
//
void RES_state::initialize_value(const CTime_base& T) {
	TypedValue	result_of_evaluation;

	// do this because we are going to use history::Add()
	get_history().Unfreeze();
	get_history().setImported(true);

	get_history().clear();
	usage_events.clear();
	try {
		evaluate_profile(EvalTiming::NORMAL, T, result_of_evaluation);
	}
	catch(eval_error Err) {
		Cstring rmessage = Err.msg;
		throw(Err);
	}
	try {
		resHistory.Add(
			value_node::stateFactory(
				T,
				result_of_evaluation,
				0,
				0,
				apgen::RES_VAL_TYPE::RESET,
				*this),
			get_datatype());
	}
	catch(eval_error Err) {
		throw(Err);
	}
}

void RES_state::evaluate_present_value(
			TypedValue&		returned_value,
			const CTime_base&	eval_time,
			LimitContainer*		lc) {
	if(thread_index == thread_intfc::MODEL_THREAD) {
		eval_val_within_thread(returned_value, eval_time, lc);
	} else {

		//
		// This is a slave thread, using a trailing miterator
		//
		lock_guard<mutex>	Lock(res_mutex);
		eval_val_within_thread(returned_value, eval_time, lc);
	}
}

void RES_state::eval_val_within_thread(
			TypedValue&		returned_value,
			const CTime_base&	eval_time,
			LimitContainer*		lc) {
	value_node*	last_set = NULL;

	//
	//  1 - first guess at a node close to given_time
	//
	value_node* found;

	found = resHistory.latestHistNodeAtOrBefore(eval_time);

	//
	//  2 - if strategy is to find an event before given_time, back off
	//

	//
	// Get the strategy from the miterator_data
	//
	miterator_data* res_miter_data = peek_at_miterator_data();
	EvalTiming::Strategy strategy = res_miter_data ?
		res_miter_data->miter.strategy :
		EvalTiming::NORMAL;

	if(strategy == EvalTiming::INTERPOLATED) {
		throw(eval_error("Interpolated evaluation is not allowed for state resources"));
	} else if(strategy == EvalTiming::BEFORE
		  && found && found->Key.getetime() == eval_time) {

		while(found->previous_node()) {
			found = found->previous_node();
 			if(found && found->Key.getetime() < eval_time)
				break;
		}
	}

	//
	//  3 - if no event found, resort to profile in desperation
	//
	if(!found) {

		//
		// not an error (??) - unclear, because initialization should
		// have inserted at least one node in the history
		//
		try {
			evaluate_profile(strategy, eval_time, returned_value);
		} catch(eval_error Err) {
			throw(eval_error(Err.msg));
		}

		if(evaluation_debug) {
			cerr << name << ": no history node at " << eval_time.to_string()
				<< "; returning profile = " << returned_value.to_string()
				<< "\n";
		}
		return;
	}

	//
	//  4 - OK, we have found an event. First easy case: not an end-of-usage, usage depth = 0
	//
	else if((found->get_res_val_type() != apgen::RES_VAL_TYPE::END_USING) && (!found->get_usage_depth())) {

		//
		// OK if node time is too early
		//
		if(	found->get_res_val_type() == apgen::RES_VAL_TYPE::RESET
			|| found->get_res_val_type() == apgen::RES_VAL_TYPE::DON_T_CARE) {
			try {
				evaluate_profile(strategy, eval_time, returned_value);
			} catch(eval_error Err) {
				throw(eval_error(Err.msg));
			}
		} else {
			returned_value = found->get_value();
		}
	}

	//
	//  5 - Second case, not so easy: we found an end-of-usage, and/or usage depth > 0
	//
	else {
		tlist<alpha_void, Cnode0<alpha_void, value_node*> >	old_usages;

		//
		//  6 -	First sub-case: look for a start-of-usage node that has not expired.
		//	And, while we are at it, also look for any SET events.
		//
		while(found) {
			if(found->get_res_val_type() == apgen::RES_VAL_TYPE::END_USING) {
				if(!found->get_other()) {
					throw(eval_error("st_value_node error"));
				}
				if(old_usages.find((void *) found->get_other())) {
					Cstring rmessage;
					rmessage << name << "->evaluate_present_value: end node at "
						<< found->Key.getetime().to_string()
						<< " points to a start node at "
						<< found->get_other()->Key.getetime().to_string()
						<< " that comes earlier. ";
					throw(eval_error(rmessage));
				}
				old_usages << new Cnode0<alpha_void, value_node*>(
								(void *) found->get_other(),
								found->get_other());
			} else if(found->get_res_val_type() == apgen::RES_VAL_TYPE::START_USING
					&& !old_usages.find((void *) found)) {
				break;
			} else if((! last_set)
					&& (found->get_res_val_type() == apgen::RES_VAL_TYPE::SET
					    || found->get_res_val_type() == apgen::RES_VAL_TYPE::RESET)) {
				last_set = found;
				if(! last_set->get_usage_depth()) {
					break;
				}
			}
			found = found->previous_node();
		}

		//
		//  7 - OK, we found an unexpired usage event! We know what to return
		//
		if(found &&	(found->get_res_val_type() == apgen::RES_VAL_TYPE::START_USING
				|| found->get_res_val_type() == apgen::RES_VAL_TYPE::SET)) {
			returned_value = found->get_value();
		}

		//
		//  8 - OK, all usage events have expired, but we have a last
		//  SET event! We know what to return
		//
		else if(last_set && last_set->get_res_val_type() == apgen::RES_VAL_TYPE::SET) {
			returned_value = last_set->get_value();
		}

		//
		//  9 - OK, found nothing really interesting. Back to the profile,
		//  and that's our last resort.
		//
		else {
			try {
				evaluate_profile(strategy, eval_time, returned_value);
			} catch(eval_error Err) {
				throw(eval_error(Err.msg));
			}
		}
	}

	if(evaluation_debug) {
		cerr << name << ": no history node at " << eval_time.to_string()
			<< "; returning profile = " << returned_value.to_string()
			<< "\n";
	}
}

//
// For numeric and state values, evaluate_present_value
// is a thin wrapper that calls eval_val_within_thread()
// after locking a mutex if it is not the modeling thread.
//
void RES_state::evaluate_present_value(
			TypedValue&		returned_value) {
	if(thread_index == thread_intfc::MODEL_THREAD) {
		eval_val_within_thread(returned_value);
	} else {

		//
		// This is a slave thread, using a trailing miterator
		//
		lock_guard<mutex>	Lock(res_mutex);
		eval_val_within_thread(returned_value);
	}
}

void RES_state::eval_val_within_thread(
			TypedValue&		returned_value) {
	value_node*	last_set = NULL;

	//
	//  1 - first, identify a history node close to given_time
	//
	value_node*	found;

	miterator_data*	res_miter_data = peek_at_miterator_data();
	EvalTiming::Strategy strategy = res_miter_data ?
		res_miter_data->miter.strategy :
		EvalTiming::NORMAL;

	CTime_base	eval_time;
	bool		use_miterator;

	//
	// Make a sharp distinction between the modeling thread and
	// trailing threads. The modeling thread may query values
	// at values of (global) 'now' that are not in sync with
	// the miterator attached to the modeling process. If that
	// is the case, this miterator cannot be used and we have
	// to do a search over history nodes.
	//
	// The story is further complicated by the fact that we
	// may be running a standalone TOL request, in which case
	// we are in the so-called modeling thread (it's the main
	// APGenX engine thread, after all) but we are not really
	// modeling; all our attention is devoted to the TOL output
	// process. In that case, we do want to use the miterator
	// after all.
	//
	// The logic below suggests that if we are in this last
	// situation, we should make sure to keep the thread's
	// current time in sync with 'now'. The miterator will
	// then be used, which is exactly what we want.
	//
	if(thread_index == thread_intfc::MODEL_THREAD) {

		//
		// thread_intfc::current_time(thread_index) reports
		// the time of the current event in the event loop;
		// it is NOT the same as 'now'.
		//
		// Be careful to make the distinction.
		//
		if(res_miter_data) {
			if((eval_time = time_saver::get_now())
				== thread_intfc::current_time(thread_index)) {

				//
				// We are synchronized with the modeling loop
				//
				use_miterator = true;
			} else {

				//
				// We are operating in the future or requesting
				// a value in the past
				//
				use_miterator = false;
			}
		} else {

			//
			// We are not modeling; maybe just carrying out a decomposition
			//
			eval_time = time_saver::get_now();
			use_miterator = false;
		}
	} else {

		//
		// Any thread other than the modeling thread has its own 'safe time'
		//
		eval_time = thread_intfc::current_time(thread_index);
		use_miterator = true;
	}

	if(use_miterator) {

		found = resHistory.latestHistNodeAtOrBeforeModelTime(
					eval_time,
					res_miter_data->iter);
	} else {

		found = resHistory.latestHistNodeAtOrBefore(eval_time);
	}

	//
	//  2 - if strategy is to find an event before given_time, back off
	//
	if(strategy == EvalTiming::INTERPOLATED) {
		throw(eval_error("Interpolated evaluation is not allowed for state resources"));
	} else if(strategy == EvalTiming::BEFORE
		  && found && found->Key.getetime() == eval_time) {
		// clist!!
		while(found->previous_node()) {
			found = found->previous_node();
 			if(found && found->Key.getetime() < eval_time)
				break;
		}
	}

	//
	//  3 - if no event found, resort to profile in desperation
	//
	if(!found) {

		//
		// not an error (??) - unclear, because initialization should
		// have inserted at least one node in the history
		//
		try {
			evaluate_profile(strategy, eval_time, returned_value);
		} catch(eval_error Err) {
			throw(eval_error(Err.msg));
		}

		if(evaluation_debug) {
			cerr << name << ": no history node at " << eval_time.to_string()
				<< "; returning profile = " << returned_value.to_string()
				<< "\n";
		}
		return;
	}

	//
	//  4 - OK, we have found an event. First easy case: not an end-of-usage, usage depth = 0
	//
	else if((found->get_res_val_type() != apgen::RES_VAL_TYPE::END_USING) && (! found->get_usage_depth())) {
		//
		// OK if node time is too early
		//
		if(	found->get_res_val_type() == apgen::RES_VAL_TYPE::RESET
			|| found->get_res_val_type() == apgen::RES_VAL_TYPE::DON_T_CARE) {
			try {
				evaluate_profile(strategy, eval_time, returned_value);
			} catch(eval_error Err) {
				throw(eval_error(Err.msg));
			}
		} else {
			returned_value = found->get_value();
		}
	}

	//
	//  5 - Second case, not so easy: we found an end-of-usage, and/or usage depth > 0
	//
	else {
		tlist<alpha_void, Cnode0<alpha_void, value_node*> >	old_usages;

		//
		//  6 -	First sub-case: look for a start-of-usage node that has not expired.
		//	And, while we are at it, also look for any SET events.
		//
		while(found) {
			if(found->get_res_val_type() == apgen::RES_VAL_TYPE::END_USING) {
				if(!found->get_other()) {
					throw(eval_error("st_value_node error"));
				}

				if(old_usages.find((void *) found->get_other())) {
					Cstring rmessage;
					rmessage << name << "->evaluate_present_value: end node at "
						<< found->Key.getetime().to_string()
						<< " points to a start node at "
						<< found->get_other()->Key.getetime().to_string()
						<< " that comes earlier. ";
					throw(eval_error(rmessage));
				}
				old_usages << new Cnode0<alpha_void, value_node*>(
								(void *) found->get_other(),
								found->get_other());
			} else if(found->get_res_val_type() == apgen::RES_VAL_TYPE::START_USING
					&& !old_usages.find((void *) found)) {
				break;
			} else if((! last_set)
					&& (found->get_res_val_type() == apgen::RES_VAL_TYPE::SET
					    || found->get_res_val_type() == apgen::RES_VAL_TYPE::RESET)) {
				last_set = found;
				if(! last_set->get_usage_depth()) {
					break;
				}
			}
			found = found->previous_node();
		}

		//
		//  7 - OK, we found an unexpired usage event! We know what to return
		//
		if(found &&	(found->get_res_val_type() == apgen::RES_VAL_TYPE::START_USING
				|| found->get_res_val_type() == apgen::RES_VAL_TYPE::SET)) {
			returned_value = found->get_value();
		}

		//
		//  8 - OK, all usage events have expired, but we have a last
		//  SET event! We know what to return
		//
		else if(last_set && last_set->get_res_val_type() == apgen::RES_VAL_TYPE::SET) {
			returned_value = last_set->get_value();
		}

		//
		//  9 - OK, found nothing really interesting. Back to the profile,
		//  and that's our last resort.
		//
		else {
			try {
				evaluate_profile(strategy, eval_time, returned_value);
			} catch(eval_error Err) {
				throw(eval_error(Err.msg));
			}
		}
	}

	if(evaluation_debug) {
		cerr << name << ": no history node at " << eval_time.to_string()
			<< "; returning profile = " << returned_value.to_string()
			<< "\n";
	}
}

state_state RES_state::get_resource_state_at(CTime_base given_time) {
	value_node*			found = NULL;
	value_node*			last_set_or_reset = NULL;
	state_state			ret(apgen::STATE_STATE::RESET, get_datatype());
	bool				looking_for_unexpired_usages = false;
	bool				looking_for_the_active_node = true;
	bool				last_set_or_reset_is_set = false;

	//
	//  0 -	we define a list to hold pointers to expired start-of-usage nodes
	//
	tlist<alpha_void, Cnode0<alpha_void, value_node*> >	old_usages;

	//
	//  1 - first guess at a node close to given_time
	//
	found = resHistory.latestHistNodeAtOrBefore(given_time);

	if(found && found->get_usage_depth() > 0) {
		looking_for_unexpired_usages = true;
	}

	//
	// Does found always exist? At the very least, we should have a RESET
	// node from the initialization event. However, if the given time precedes
	// the initialization time, it's possible that nothing will be found.
	// In that case, we should report the earliest value in the profile.
	//

	//
	//  2 - We are looking for 3 things:
	//
	//  (1)	the active node, i. e., the one that determines the current
	//	value of the resource
	//  (2) the list of all pending usages, if any; we will list all
	//	the future END_USING nodes for these pending usages
	//  (3)	the latest set or reset node before the given time
	//
	//  To find the active node:
	//
	//	- if the usage depth is zero, it's the latest SET or RESET node before now
	//	- if the usage depth is > 0, it's the latest unexpired START_USING node before now
	//
	//  To find all pending usages:
	//
	//	- collect a list of all unexpired START_USING nodes.
	//	  Only do this if the usage depth is > 0.
	//
	//  To find the latest set or RESET node:
	//
	//	- scan the history (DUH)
	//
	//
	// NOTE: get_usage_depth() reports the depth immediately AFTER
	//	 a node in history. See the code at the end of the USE
	//	 case in RES_state::append_to_history().
	//

	//
	// we step through the history list, back in time
	//
	while(found) {

		if(looking_for_unexpired_usages) {
			if(found->get_usage_depth() == 0) {
				looking_for_unexpired_usages = false;
			} else if(found->get_res_val_type() == apgen::RES_VAL_TYPE::END_USING) {
				assert(found->get_other());
				assert(!old_usages.find((void *) found->get_other()));

				//
				// the corresponding START_USING node has expired
				//
				old_usages << new Cnode0<alpha_void, value_node*>(
							(void *) found->get_other(),
							found->get_other());
			} else if(found->get_res_val_type() == apgen::RES_VAL_TYPE::START_USING) {
				if(!old_usages.find((void *) found)) {

					// this START_USING node is unexpired

					if(looking_for_the_active_node) {
						looking_for_the_active_node = false;
						ret.currentval = found->get_value();
					}
					assert(found->get_other());
					ret.end_uses << new Cnode0<alpha_time, stateNodePLD>(
						found->get_other()->getKey().getetime(),
						stateNodePLD(found->getKey().get_event_id(),
						found->get_value()));
				}
			}
		}

		if(!last_set_or_reset) {
			if(	(last_set_or_reset_is_set = found->get_res_val_type() == apgen::RES_VAL_TYPE::SET)
				|| found->get_res_val_type() == apgen::RES_VAL_TYPE::RESET) {
				last_set_or_reset = found;
			}
		}

		if(last_set_or_reset && !looking_for_unexpired_usages) {

			//
			// we are all done
			//
			break;
		}

		found = found->previous_node();
	}

	//
	// set currentval if it has not been set by an unexpired START_USE node
	//
	if(looking_for_the_active_node) {
		if(last_set_or_reset_is_set) {
			ret.currentval = last_set_or_reset->get_value();
		} else {
			try {
				evaluate_profile(
						EvalTiming::NORMAL,
						given_time,
						ret.currentval);
			}
			catch(eval_error Err) {
				throw(eval_error(Err.msg));
			}
		}
	}

	//
	// set setval and currently members of the state_state
	//
	if(last_set_or_reset_is_set) {
		if(ret.end_uses.get_length()) {
			ret.currently = apgen::STATE_STATE::USE_SET_PENDING;
		} else {
			ret.currently = apgen::STATE_STATE::SET;
		}
		ret.setval = last_set_or_reset->get_value();
	}
	else {
		if(ret.end_uses.get_length()) {
			ret.currently = apgen::STATE_STATE::USE_RESET_PENDING;
		} else {
			ret.currently = apgen::STATE_STATE::RESET;
		}
	}
	return ret;
}

void RES_settable::append_to_history(
		TypedValue&		new_value,
		CTime_base		start_event_time,
		CTime_base		end_event_time,
		long			start_event_id,
		long			end_event_id,
		apgen::USAGE_TYPE) {
	value_node*			vn;
	value_node*			closest_node;

	//
	// For the time being, parse_act_plan() does not use Miterators:
	//
	miterator_data*	res_miter_data = peek_at_miterator_data();

	if(APcloptions::theCmdLineOptions().TOLdebug
	   && name == APcloptions::theCmdLineOptions().theResourceToDebug) {
	    Cstring str;
	    str << name << "->append(T=" << start_event_time.to_string()
		<< "), val = " << new_value.to_string() << "\n";
	    tol_debug_stream() << str;
	}

	//
	// The code below was tragically wrong if immed == true, because
	// in that case start_event_time could be completely wrong - it was
	// stolen from current event, while the actual usage is set in
	// the future and should be taken from time_saver::get_now().
	//
	// That was corrected and verified by the assert statement below.
	//
	if(res_miter_data && start_event_time
			     == thread_intfc::current_time(thread_index)) {
		assert(time_saver::get_now() == start_event_time);

		closest_node = resHistory.latestHistNodeAtOrBeforeModelTime(
					start_event_time,
					res_miter_data->iter);
	} else {
		closest_node = get_history().latestHistNodeBefore(time_saver::get_now());
	}


#	ifndef DISABLE_FILTERING_WHILE_MODELING
	//
	// Handle filtering
	//
	bool	skip = false;

	if(closest_node) {
		TypedValue	prev_value = closest_node->get_value();
		double		absDelta = 0.0;
		double		relDelta = 0.0;
		bool		has_delta = false;
		bool		has_no_filtering = properties[(int)Property::has_nofiltering];
		TypedValue	minAbsSym;


		//
		// If has_no_filtering is defined, has_delta is always false.
		//
		if(!has_no_filtering) {
		    if(properties[(int)Property::has_min_abs_delta]) {
			has_delta = true;
			minAbsSym = (*Object)["min_abs_delta"];
		    }
		    if(properties[(int)Property::has_min_rel_delta]) {
			has_delta = true;
			TypedValue	minRelSym = (*Object)["min_rel_delta"];

			//
			// AP-1206 - filtering is applied twice, so halve the given delta
			//
			relDelta = minRelSym.get_double()/2;
		    }

		    if(has_delta) {

			switch(prev_value.get_type()) {
			    case apgen::DATA_TYPE::FLOATING:
				{
				double prev = prev_value.get_double();
				double delta = fabs(new_value.get_double() - prev);

				//
				// AP-1206 - filtering is applied twice,
				// so halve the given delta
				//
				double absDelta = minAbsSym.get_double()/2;

				if(	delta > absDelta
					&& delta > relDelta * fabs(prev)) {

					skip = false;

				} else {

					skip = true;

				}

				if(APcloptions::theCmdLineOptions().TOLdebug
				   && name == APcloptions::theCmdLineOptions().theResourceToDebug) {
				    Cstring str;
				    str << "    delta = " << delta
					<< ", absDelta = " << absDelta
					<< ", relDelta = " << relDelta
					<< ", skip = " << skip << "\n";
				    tol_debug_stream() << str;
				}
				}
				break;
			    case apgen::DATA_TYPE::TIME:
			    case apgen::DATA_TYPE::DURATION:
				{
				CTime_base prevT = prev_value.get_time_or_duration();
				CTime_base newT = new_value.get_time_or_duration();
				long int prev = prevT.get_pseudo_millisec();
				long int delta = labs(newT.get_pseudo_millisec() - prev);

				//
				// AP-1206 - filtering is applied twice,
				// so halve the given delta. Remember that
				// for a positive duration, pseudo_millisec is
				// actually a number of milliseconds.
				//
				long int absDelta = minAbsSym.get_time_or_duration().get_pseudo_millisec()/2;

				if(	delta > absDelta
					&& ((double)delta) > relDelta * ((double)labs(prev))) {

					skip = false;

				} else {

					skip = true;

				}

				if(APcloptions::theCmdLineOptions().TOLdebug
				   && name == APcloptions::theCmdLineOptions().theResourceToDebug) {
				    Cstring str;
				    str << "    delta = " << delta
					<< ", absDelta = " << absDelta
					<< ", relDelta = " << relDelta
					<< ", skip = " << skip << "\n";
				    tol_debug_stream() << str;
				}
				}
				break;
			    case apgen::DATA_TYPE::INTEGER:
				{
				long int prev = prev_value.get_int()
					+ prev_value.get_int();
				long int delta = labs(new_value.get_int() - prev);

				//
				// AP-1206 - filtering is applied twice,
				// so halve the given delta
				//
				double absDelta = minAbsSym.get_int()/2;

				if(	((double)delta) > absDelta
					&& ((double)delta) > relDelta * ((double)labs(prev))) {

					skip = false;

				} else {

					skip = true;

				}

				if(APcloptions::theCmdLineOptions().TOLdebug
				   && name == APcloptions::theCmdLineOptions().theResourceToDebug) {
				    Cstring str;
				    str << "    delta = " << delta
					<< ", absDelta = " << absDelta
					<< ", relDelta = " << relDelta
					<< ", skip = " << skip << "\n";
				    tol_debug_stream() << str;
				}
				}
				break;

			    //
			    // pacify the compiler - handle all other types, even though
			    // they are not applicable to numeric resources
			    //
			    default:

				skip = false;

				break;
			}
		    } else if(closest_node->get_value() == new_value) {

			skip = true;

			if(APcloptions::theCmdLineOptions().TOLdebug
			   && name == APcloptions::theCmdLineOptions().theResourceToDebug) {
			    tol_debug_stream() << "    same value, skipping\n";
			}
		    }
		} // if(!has_no_filtering)
	}

	//
	// New behavior required by JIRA AP-745
	//
	if(skip) {

#ifdef NO_STATISTICS
		//
		// collect statistics
		//
		map<Cstring, int>::iterator it9 = STATS_settable().find(name);
		if(it9 == STATS_settable().end()) {
			STATS_settable()[name] = 1;
		} else {
			STATS_settable()[name]++;
		}
#endif /* NO_STATISTICS */

		//
		// Disabled until a correct fix is implemented for AP-814
		//
		// Re-enabled since the code was right after all. But this
		// time, we are going to check with assert statements that
		// the new node is not earlier than any safeNode that was
		// passed to the constraint-checking thread.
		//
		UpdateLastSkippedNode(start_event_time, new_value, start_event_id);

		return;
	} else {
#	endif /* DISABLE_FILTERING_WHILE_MODELING */
		Add(start_event_time, new_value, start_event_id);
#	ifndef DISABLE_FILTERING_WHILE_MODELING
	}
#	endif /* DISABLE_FILTERING_WHILE_MODELING */
}

void RES_settable::Add(
		const CTime_base&	event_time,
		const TypedValue&	new_value,
		long			event_id) {
    value_node* vn = resHistory.lastSkippedNodeContainer.first_node();

    //
    // We first do this so that sudden jumps don't have the undesirable
    // side effect of lifting the values of all previous nodes by some
    // amount larger than the filtering tolerance.
    //
    if(vn) {
	resHistory.lastSkippedNodeContainer.remove_node(vn);

#ifdef ONLY_INCLUDE_FOR_VERIFICATION
	//
	// We need to check that adding this node does not
	// jeopardize the safety of the last safeNode provided
	// to the constraint-checking thread.
	//
	// Verification Algorithm
	// ----------------------
	//
	// First, we are going to need a copy of the safe node vector
	// that was passed by the EventLoop to the constraint-checking
	// thread. This can be conveniently done by
	// model_res_miter::save_safe_nodes() in trailing_miterator.C,
	// which can also delete the previous copy since resource usage
	// events are never handled by threads other than the modeling
	// thread.
	//
	// Second, we are going to need an easy way for each resource
	// (e. g. this) to find its resource list iterator within the
	// safe node vector. But the code in model_res_miter::save_safe_nodes()
	// shows that the iterators in the vector are ordered according to
	// the slist order of
	//
	// 	potential_triggers::pointer_to_threads
	//
	// which is a tlist using the address of the resource history list
	// as its key. Let's test that theory.
	//
	void*	hist_key = resHistory.history_list_as_void_ptr();
	model_res_miter* triggers = &EventLoop::theEventLoop().potential_triggers;
	model_res_miter::threadptr*	ptr1
		= triggers->pointers_to_threads.find(hist_key);
	if(ptr1) {
	    model_res_miter::Tthread*	ptr2 = ptr1->payload;
	    multilist<prio_time, value_node, Rsource*>::leading_iter*
						ptr3 = ptr2->payload;
	    assert(ptr3);

	    //
	    // OK. Now, we need to verify that the time tag of the
	    // skipped node (which we are about to insert) is _later_
	    // than the safeNode time in the leading_iter. This is
	    // obviously true, because the safeNode is _already_ in
	    // the history list, and its time tag is strictly earlier
	    // than the current modeling time.
	    //
	    if(vn->getKey().getetime() < ptr3->safeNode->getKey().getetime()) {
		cerr << name << "->Add(): last skipped node @ "
		    << vn->getKey().getetime().to_string()
		    << ", safe node @ "
		    << ptr3->safeNode->getKey().getetime().to_string()
		    << "\n";
	    }
	    assert(vn->getKey().getetime() >= ptr3->safeNode->getKey().getetime());
	}
#endif /* ONLY_INCLUDE_FOR_VERIFICATION */


	resHistory.Add(vn, get_datatype());
	if(APcloptions::theCmdLineOptions().TOLdebug
	   && name == APcloptions::theCmdLineOptions().theResourceToDebug) {
	    Cstring str;
	    str << name << "->append retroactively(T=" << vn->Key.getetime().to_string()
		<< "), val = " << vn->get_value().to_string() << "\n";
	    tol_debug_stream() << str;
	}
    }

    //
    // Now we add the node the client really wanted
    //
    switch(get_datatype()) {
	case apgen::DATA_TYPE::FLOATING:
		resHistory.Add(	value_node::settableFloatFactory(
					event_time,
					new_value.get_double(),
					event_id,
					*this),
				get_datatype());
		break;
	case apgen::DATA_TYPE::INTEGER:
	case apgen::DATA_TYPE::BOOL_TYPE:
		resHistory.Add(	value_node::settableIntFactory(
					event_time,
					new_value.get_int(),
					event_id,
					*this),
				get_datatype());
		break;
	case apgen::DATA_TYPE::STRING:
		resHistory.Add(	value_node::settableStringFactory(
					event_time,
					new_value.get_string(),
					event_id,
					*this),
				get_datatype());
		break;
	case apgen::DATA_TYPE::TIME:
	case apgen::DATA_TYPE::DURATION:
		{
		resHistory.Add(	value_node::settableTimeFactory(
					event_time,
					new_value.get_time_or_duration(),
					event_id,
					*this),
				get_datatype());
		}
		break;
	default:
		;
    }
}

void RES_settable::UpdateLastSkippedNode(
		const CTime_base&	event_time,
		const TypedValue&	new_value,
		long			event_id) {
	resHistory.lastSkippedNodeContainer.clear();
	switch(get_datatype()) {
	    case apgen::DATA_TYPE::FLOATING:
		resHistory.lastSkippedNodeContainer << value_node::settableFloatFactory(
					event_time,
					new_value.get_double(),
					event_id,
					*this);
		break;
	    case apgen::DATA_TYPE::INTEGER:
	    case apgen::DATA_TYPE::BOOL_TYPE:
		resHistory.lastSkippedNodeContainer << value_node::settableIntFactory(
					event_time,
					new_value.get_int(),
					event_id,
					*this);
		break;
	    case apgen::DATA_TYPE::STRING:
		resHistory.lastSkippedNodeContainer << value_node::settableStringFactory(
					event_time,
					new_value.get_string(),
					event_id,
					*this);
		break;
	    case apgen::DATA_TYPE::TIME:
	    case apgen::DATA_TYPE::DURATION:
		{
		resHistory.lastSkippedNodeContainer << value_node::settableTimeFactory(
					event_time,
					new_value.get_time_or_duration(),
					event_id,
					*this);
		}
		break;
	    default:
		;
	}
}

void RES_settable::immediately_append_to_history(
		TypedValue&		new_value,
		apgen::USAGE_TYPE) {
	append_to_history(
		new_value,
		time_saver::get_now(),
		CTime_base(),
		++mEvent::currentEventID,
		0,
		apgen::USAGE_TYPE::SET);
}

void RES_settable::initialize_value(
		init_event*		source) {
	TypedValue	result_of_evaluation;

	if(isFrozen()) {
		Cstring rmessage;
		rmessage << "initialize_value: Resource "
			<< name << " is frozen.";
		throw(eval_error(rmessage));
	}
	if(isImported()) {

		//
		// We want to use the history now, but not next time.
		//
		get_history().setImported(false);
		return;
	}
	if(model_control::get_pass() != model_control::SCHEDULING_2) {
		get_history().clear();
	}
	try {
		evaluate_profile(
				EvalTiming::NORMAL,
				time_saver::get_now(),
				result_of_evaluation);
	}
	catch(eval_error Err) {
		Cstring rmessage;
		rmessage << "RES_settable::initialize_value ERROR:\n"
			<< Err.msg;
		throw(eval_error(rmessage));
	}

	Add(source->getetime(), result_of_evaluation, source->eventID);
}

//
// for XML TOL ingestion:
//
void RES_settable::initialize_value(const CTime_base& T) {
	TypedValue	result_of_evaluation;

	// do this because we are going to use history::Add()
	get_history().Unfreeze();
	get_history().setImported(true);
	get_history().clear();
	try {
		evaluate_profile(EvalTiming::NORMAL, T, result_of_evaluation);
	}
	catch(eval_error Err) {
		Cstring rmessage = Err.msg;
		throw(Err);
	}
	Add(T, result_of_evaluation, 0);
}

void RES_consumable::append_to_history(
		TypedValue&		amountconsumed,
		CTime_base		start_event_time,
		CTime_base,
		long			start_event_id,
		long,
		apgen::USAGE_TYPE) {

	//
	// this method is necessarily called by the
	// modeling thread. For (non-)consumable and
	// state resources, adding history nodes
	// interferes with getting history nodes,
	// so we need a semaphore. The same mutex
	// is locked by evaluate_present_value().
	//
	lock_guard<mutex>		Lock(res_mutex);

	apgen::DATA_TYPE		datatype = get_datatype();

	value_node* new_node;
	if(get_datatype() == apgen::DATA_TYPE::FLOATING) {
		resHistory.Add(	(new_node = value_node::numericFloatFactory(
					start_event_time,
					amountconsumed.get_double(),
					start_event_id,
					*this,
					apgen::RES_VAL_TYPE::START_USING)),
				datatype);
	} else if(get_datatype() == apgen::DATA_TYPE::INTEGER) {
		resHistory.Add( (new_node = value_node::numericIntFactory(
					start_event_time,
					amountconsumed.get_int(),
					start_event_id,
					*this,
					apgen::RES_VAL_TYPE::START_USING)),
				  datatype);
	} else if(get_datatype() == apgen::DATA_TYPE::TIME
		  || get_datatype() == apgen::DATA_TYPE::DURATION) {

		//
		// this encoding is undone when calling get_value():
		//
		CTime_base		t0(amountconsumed.get_time_or_duration());
		// true_long		I = t0.get_milliseconds() + 1000L * (true_long) t0.get_seconds();
		true_long		I = t0.get_pseudo_millisec();

		resHistory.Add(	(new_node = value_node::numericIntFactory(
					start_event_time,
					I,
					start_event_id,
					*this,
					apgen::RES_VAL_TYPE::START_USING)),
				datatype);
	}

	if(APcloptions::theCmdLineOptions().TOLdebug
	   && name == APcloptions::theCmdLineOptions().theResourceToDebug) {
	    Cstring str;
	    str << name << "->append(T=" << start_event_time.to_string()
		<< "), val = " << new_node->get_value().to_string() << "\n";
	    tol_debug_stream() << str;
	}
}

void RES_consumable::immediately_append_to_history(
		TypedValue&		amountconsumed,
		apgen::USAGE_TYPE	uType) {
	append_to_history(
		amountconsumed,
		time_saver::get_now(),
		CTime_base(),
		++mEvent::currentEventID,
		0,
		uType);
}

void RES_nonconsumable::immediately_append_to_history(
		TypedValue&,
		apgen::USAGE_TYPE) {
	throw(eval_error("nonconsumable resource cannot process immediate events"));
}

void RES_nonconsumable::append_to_history(
		TypedValue&		amountconsumed,
		CTime_base		start_event_time,
		CTime_base		end_event_time,
		long			start_event_id,
		long			end_event_id,
		apgen::USAGE_TYPE) {
	apgen::DATA_TYPE	datatype = get_datatype();
	value_node*		ncv;
	value_node*		dbgdbg;

	//
	// We need this because evaluating currentval()
	// requires scanning the history list, and we
	// are going to mess up that list when we insert
	// history nodes, below
	//
	static mutex			mutx;
	lock_guard<mutex>		lock(res_mutex);


	if(datatype == apgen::DATA_TYPE::FLOATING) {
		resHistory.Add(	ncv = value_node::numericFloatFactory(
					start_event_time,
					amountconsumed.get_double(),
					start_event_id,
					*this,
					apgen::RES_VAL_TYPE::START_USING),
				datatype);

		if(end_event_time > CTime_base(0,0,false)) {
			resHistory.Add(	dbgdbg = value_node::numericFloatFactory(
						end_event_time,
						-amountconsumed.get_double(),
						ncv,
						end_event_id,
						*this,
						apgen::RES_VAL_TYPE::END_USING),
				  	datatype);
		}
	} else if(datatype == apgen::DATA_TYPE::INTEGER) {
		resHistory.Add(	ncv = value_node::numericIntFactory(
					start_event_time,
					amountconsumed.get_int(),
					start_event_id,
					*this,
					apgen::RES_VAL_TYPE::START_USING),
				datatype);
		if(end_event_time > CTime_base(0,0,false)) {
			resHistory.Add(	dbgdbg = value_node::numericIntFactory(
						end_event_time,
						-amountconsumed.get_int(),
						ncv,
						end_event_id,
						*this,
						apgen::RES_VAL_TYPE::END_USING),
					datatype);
		}
	} else if(datatype == apgen::DATA_TYPE::TIME || datatype == apgen::DATA_TYPE::DURATION) {

		//
		// this encoding is undone when calling get_value():
		//
		CTime_base		t0(amountconsumed.get_time_or_duration());
		true_long		I = t0.get_pseudo_millisec();

		resHistory.Add(	ncv = value_node::numericIntFactory(
					start_event_time,
					I,
					start_event_id,
					*this,
					apgen::RES_VAL_TYPE::START_USING),
				datatype);
		if(end_event_time > CTime_base(0,0,false)) {
			resHistory.Add(	dbgdbg = value_node::numericIntFactory(
					    end_event_time,
					    -I,
					    ncv,
					    end_event_id,
					    *this,
					    apgen::RES_VAL_TYPE::END_USING),
					datatype);
		}
	}

	if(APcloptions::theCmdLineOptions().TOLdebug
	   && name == APcloptions::theCmdLineOptions().theResourceToDebug) {
	    Cstring str;
	    str << name << "->append(T=" << start_event_time.to_string()
		<< "), val = " << ncv->get_value().to_string() << "\n";
	    tol_debug_stream() << str;
	}
}

void RES_consumable::initialize_value(
		init_event*		source) {

	if(isFrozen()) {
		Cstring rmessage;
		rmessage << "initialize_value: Resource " << name << " is frozen.";
		throw(eval_error(rmessage));
	}
	if(isImported()) {

		//
		// We want to use the history now, but not next time.
		//
		get_history().setImported(false);
		return;
	}
	if(model_control::get_pass() != model_control::SCHEDULING_2) {
		resHistory.clear();
	}
	try {
		apgen::DATA_TYPE	datatype = get_datatype();

		if(datatype == apgen::DATA_TYPE::FLOATING) {
			resHistory.Add(	value_node::numericFloatFactory(
					    source->getetime(),
					    0.0,
					    source->eventID,
					    *this,
					    apgen::RES_VAL_TYPE::DON_T_CARE),
					datatype);
		} else if(datatype == apgen::DATA_TYPE::INTEGER
			  || datatype == apgen::DATA_TYPE::TIME
			  || datatype == apgen::DATA_TYPE::DURATION) {
			resHistory.Add(	value_node::numericIntFactory(
					    source->getetime(),
					    0L,
					    source->eventID,
					    *this,
					    apgen::RES_VAL_TYPE::DON_T_CARE),
					datatype);
		}
	} catch(eval_error Err) {
		Cstring rmessage;
		rmessage = Err.msg;
		throw(eval_error(rmessage));
	}
}

void RES_consumable::initialize_value(
		const CTime_base&	T) {

	get_history().Unfreeze();
	get_history().setImported(true);
	resHistory.clear();
	try {
		apgen::DATA_TYPE	datatype = get_datatype();

		if(datatype == apgen::DATA_TYPE::FLOATING) {
			resHistory.Add(	value_node::numericFloatFactory(
					    T,
					    0.0,
					    0,
					    *this,
					    apgen::RES_VAL_TYPE::DON_T_CARE),
					datatype);
		} else if(datatype == apgen::DATA_TYPE::INTEGER
			  || datatype == apgen::DATA_TYPE::TIME
			  || datatype == apgen::DATA_TYPE::DURATION) {
			resHistory.Add(	value_node::numericIntFactory(
					    T,
					    0L,
					    0,
					    *this,
					    apgen::RES_VAL_TYPE::DON_T_CARE),
					datatype);
		}
	}
	catch(eval_error Err) {
		Cstring rmessage = Err.msg;
		throw(Err);
	}
}

void RES_nonconsumable::initialize_value(
		init_event* source) {

	if(isFrozen()) {
		Cstring rmessage;
		rmessage << "initialize_value: Resource " << name << " is frozen.";
		throw(eval_error(rmessage));
	}
	if(isImported()) {

		//
		// We want to use the history now, but not next time.
		//
		get_history().setImported(false);
		return;
	}
	if(model_control::get_pass() != model_control::SCHEDULING_2) {
		resHistory.clear();
	}
	try {
		apgen::DATA_TYPE		datatype = get_datatype();

		if(datatype == apgen::DATA_TYPE::FLOATING) {
			resHistory.Add(	value_node::numericFloatFactory(
						    source->getetime(),
						    0.0,
						    source->eventID,
						    *this,
						    apgen::RES_VAL_TYPE::DON_T_CARE),
						datatype);
		} else if(datatype == apgen::DATA_TYPE::INTEGER
			  || datatype == apgen::DATA_TYPE::TIME
			  || datatype == apgen::DATA_TYPE::DURATION) {
			resHistory.Add(	value_node::numericIntFactory(
						    source->getetime(),
						    0L,
						    source->eventID,
						    *this,
						    apgen::RES_VAL_TYPE::DON_T_CARE),
						datatype);
		}
	} catch(eval_error Err) {
		Cstring rmessage = Err.msg;
		throw(Err);
	}
}

void RES_nonconsumable::initialize_value(
		const CTime_base& T) {

	get_history().Unfreeze();
	get_history().setImported(true);
	get_history().clear();
	try {
		apgen::DATA_TYPE	datatype = get_datatype();

		if(datatype == apgen::DATA_TYPE::FLOATING) {
			resHistory.Add(	value_node::numericFloatFactory(
						    T,
						    0.0,
						    0,
						    *this,
						    apgen::RES_VAL_TYPE::DON_T_CARE),
						datatype);
		} else if(datatype == apgen::DATA_TYPE::INTEGER
			  || datatype == apgen::DATA_TYPE::TIME
			  || datatype == apgen::DATA_TYPE::DURATION) {
			resHistory.Add(	value_node::numericIntFactory(
						    T,
						    0L,
						    0,
						    *this,
						    apgen::RES_VAL_TYPE::DON_T_CARE),
						datatype);
		}
	} catch(eval_error Err) {
		Cstring err;
		err << name << "->initialize_value error:\n" << Err.msg;
		throw(eval_error(err));
	}
}


//
// Useful in TOL output of interpolated resources
//
TypedValue RES_numeric::nodeval2resval(value_node* node) {

	//
	// prevent thread conflicts
	//
	lock_guard<mutex>	Lock(res_mutex);

	TypedValue val;
	CTime_base eval_time = node->getKey().getetime();

	try {
	    evaluate_profile(EvalTiming::NORMAL, eval_time, val);
	} catch(eval_error Err) {
	    Cstring ErrMsg(name);
	    ErrMsg << ": error evaluating profile -\n" << Err.msg;
	    throw(eval_error(ErrMsg));
	}

	bool dependent_resource = node->list->Owner != this;

	if(!dependent_resource) {
	    TypedValue nodeval = node->get_value();
	    switch(get_datatype()) {
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
		    val = val.get_time_or_duration() - nodeval.get_time_or_duration();
		    break;
		case apgen::DATA_TYPE::FLOATING:
		    val = val.get_double() - nodeval.get_double();
		    break;
		case apgen::DATA_TYPE::INTEGER:
		    val = val.get_int() - nodeval.get_int();
		    break;

		//
		// pacify the compiler - handle all other types, even though
		// they are not applicable to numeric resources
		//
		default:
		    break;
	    }
	}

	//
	// else, we just return the profile value
	//
	return val;
}

void RES_numeric::evaluate_present_value(
			TypedValue&		returned_value,
			const CTime_base&	eval_time,
			LimitContainer*		lc) {
    if(thread_index == thread_intfc::MODEL_THREAD) {
		eval_val_within_thread(returned_value, eval_time, lc);
    } else if(!lc && !get_usage_count()) {

	//
	// The hope is that this will make parallel TOL output
	// significantly faster - no mutex needed
	//
	miterator_data* res_miter_data = peek_at_miterator_data();
	EvalTiming::Strategy strategy = res_miter_data ?
		res_miter_data->miter.strategy :
		EvalTiming::NORMAL;
	evaluate_profile(strategy, eval_time, returned_value);
    } else {

	//
	// This is a slave thread, using a trailing miterator
	//
	lock_guard<mutex>	Lock(res_mutex);
	eval_val_within_thread(returned_value, eval_time, lc);
    }
}

void RES_numeric::eval_val_within_thread(
			TypedValue&		returned_value,
			const CTime_base&	eval_time,
			LimitContainer*		lc) {
	value_node*			found = NULL;
	value_node*			also_found = NULL;
	bool				interpolate = false;
	value_node*			future_node = NULL;

	miterator_data* res_miter_data = peek_at_miterator_data();
	EvalTiming::Strategy strategy = res_miter_data ?
		res_miter_data->miter.strategy :
		EvalTiming::NORMAL;

	//
	// NORMAL case (the most frequently used)
	//
	if(strategy == EvalTiming::NORMAL) {
	    found = get_history().latestHistNodeAtOrBefore(eval_time);

	//
	// BEFORE case
	//
	} else if(strategy == EvalTiming::BEFORE) {
	    found = get_history().latestHistNodeBefore(eval_time);

	//
	// INTERPOLATED case
	//
	} else if(strategy == EvalTiming::INTERPOLATED) {

	    //
	    // will need to interpolate. Note that consolidate() has
	    // already verified that this resource has its Interpolation
	    // attribute set to true.
	    //
	    found = get_history().latestHistNodeAtOrBefore(eval_time);
	    if(found) {
		value_node*	another_node = NULL;
		another_node = found;
		while((	future_node = another_node->following_node())
			&& (future_node->Key.getetime() == found->Key.getetime())) {
			another_node = future_node;
		}
		if(future_node) {
			// we have a positive time interval over which to interpolate
			interpolate = true;
		}
	    } else {
		assert(false);
	    }
	}


	//
	// If a Limit Container is presents, compute appropriate limits and
	// store them in the lc object.
	//
	if(lc) {
		bool				returning = false;
		time_saver			ts;
		const parsedProg&		attrProgPtr(get_attribute_program());
		map<Cstring, pEsys::Assignment*>::const_iterator	errorHighIter;
		map<Cstring, pEsys::Assignment*>::const_iterator	errorLowIter;
		map<Cstring, pEsys::Assignment*>::const_iterator	warningHighIter;
		map<Cstring, pEsys::Assignment*>::const_iterator	warningLowIter;
		pEsys::Assignment*	errorHighInstr = NULL;
		pEsys::Assignment*	errorLowInstr = NULL;
		pEsys::Assignment*	warningHighInstr = NULL;
		pEsys::Assignment*	warningLowInstr = NULL;

		if(attrProgPtr) {
			pEsys::Program& theAttrProg = *attrProgPtr.object();
			errorHighIter = theAttrProg.symbols.find("errorhigh");
			if(errorHighIter != theAttrProg.symbols.end()) {
				errorHighInstr = errorHighIter->second;
			}
			errorLowIter = theAttrProg.symbols.find("errorlow");
			if(errorLowIter != theAttrProg.symbols.end()) {
				errorLowInstr = errorLowIter->second;
			}
			warningHighIter = theAttrProg.symbols.find("warninghigh");
			if(warningHighIter != theAttrProg.symbols.end()) {
				warningHighInstr = warningHighIter->second;
			}
			warningLowIter = theAttrProg.symbols.find("warninglow");
			if(warningLowIter != theAttrProg.symbols.end()) {
				warningLowInstr = warningLowIter->second;
			}
		}

		ts.set_now_to(eval_time);
		try {
			TypedValue V;
			if(errorHighInstr) {
				errorHighInstr->Expression->eval_expression(Object.object(), V);
				lc->error_high_val = V.get_double();
			}
			if(warningHighInstr) {
				warningHighInstr->Expression->eval_expression(Object.object(), V);
				lc->warn_high_val = V.get_double();
			}
			if(warningLowInstr) {
				warningLowInstr->Expression->eval_expression(Object.object(), V);
				lc->warn_low_val = V.get_double();
			}
			if(errorLowInstr) {
				errorLowInstr->Expression->eval_expression(Object.object(), V);
				lc->error_low_val = V.get_double();
			}
		}
		catch(eval_error Err) {
			Cstring	error = name;
			error << ": error while evaluating error/warning limit - details:\n"
				<< Err.msg;
			throw(eval_error(error));
		}
	}

	//
	// Straightforward case: no history node is applicable.
	//
	// Use the resource's profile.
	//
	if(!found) {
		try {
			evaluate_profile(strategy, eval_time, returned_value);
		}
		catch(eval_error Err) {
			Cstring ErrMsg(name);
			ErrMsg << ": evaluate present value: error computing profile -\n";
			ErrMsg << Err.msg;
			throw(eval_error(ErrMsg));
		}

		if(evaluation_debug) {
			cerr << name << ": no history node at " << eval_time.to_string()
				<< "; returning profile = " << returned_value.to_string()
				<< "\n";
		}
		return;
	}

	//
	// In all cases, we need to evaluate the profile and correct it
	// by the amount stored in the history node we found.
	//
	try {
		evaluate_profile(strategy, eval_time, returned_value);
		returned_value.cast(get_datatype());
	} catch(eval_error Err) {
		Cstring ErrMsg(name);
		ErrMsg << ": error evaluating profile -\n" << Err.msg;
		throw(eval_error(ErrMsg));
	}

	//
	// num_value_node_i::get_value() correctly decodes time and
	// duration values encoded as true_long integers
	//
	TypedValue	temptv = found->get_value();

	//
	// F2 is the weight we should give the future node in case we need
	// to evaluate an interpolated resource.
	//

	double F2;
	if(interpolate) {
		CTime_base delta = future_node->Key.getetime() - found->Key.getetime();
		F2 = (eval_time - found->Key.getetime()) / delta;
	}
	switch(returned_value.get_type()) {
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
			returned_value = returned_value.get_time_or_duration() - temptv.get_time_or_duration();
			if(interpolate) {
				returned_value = returned_value.get_time_or_duration()
					- (future_node->get_value().get_time_or_duration()
							- found->get_value().get_time_or_duration()) * F2;
			}
			break;
		case apgen::DATA_TYPE::FLOATING:
			returned_value = returned_value.get_double() - temptv.get_double();
			if(interpolate) {
				returned_value = returned_value.get_double()
					- (future_node->get_value().get_double()
							- found->get_value().get_double()) * F2;
			}
			break;
		case apgen::DATA_TYPE::INTEGER:
			returned_value = returned_value.get_int() - temptv.get_int();
			if(interpolate) {
				returned_value = returned_value.get_int()
					- (long)((future_node->get_value().get_int()
							- found->get_value().get_int()) * F2);
			}
			break;

		//
		// pacify the compiler - handle all other types, even though
		// they are not applicable to numeric resources
		//
		default:
			break;
	}

	if(evaluation_debug) {
	    cerr << name << ": found history node at " << eval_time.to_string()
		<< "; returning value = " << returned_value.to_string()
		<< "\n";
	}
}

//
// For numeric and state values, evaluate_present_value
// is a thin wrapper that calls eval_val_within_thread()
// after locking a mutex if it is not the modeling thread.
//
void RES_numeric::evaluate_present_value(
			TypedValue&		returned_value) {
	if(thread_index == thread_intfc::MODEL_THREAD) {
		eval_val_within_thread(returned_value);
	} else {

		//
		// This is a slave thread, using a trailing miterator
		//
		lock_guard<mutex>	Lock(res_mutex);
		eval_val_within_thread(returned_value);
	}
}

void RES_numeric::eval_val_within_thread(
			TypedValue&		returned_value) {
	value_node*			found = NULL;
	value_node*			also_found = NULL;
	bool				interpolate = false;
	value_node*			future_node = NULL;

	//
	// If this is the modeling thead, the current modeling
	// time to use is always 'now', even if we are in the
	// middle of get_windows() processing.
	//
	// If this is the constraint-checking thread, then we
	// don't want to use peek() data, because the thread evaluates
	// constraints just _before_ jumping to the new time.
	// But luckily for us, the thread stores its current time
	// (just before peek() time) in a thread-local variable,
	// so that's what we use.
	//
	// thread_intfc::current_time(thread_index) reports the time of the current
	// event in the event loop (either the constraint-checking
	// loop or the event loop); it is NOT the same as 'now'.
	// Be careful to make the distinction.
	//
	miterator_data* res_miter_data = peek_at_miterator_data();
	EvalTiming::Strategy strategy = res_miter_data ?
		res_miter_data->miter.strategy :
		EvalTiming::NORMAL;

	CTime_base	eval_time;
	bool		use_miterator;

	if(thread_index == thread_intfc::CONSTRAINT_THREAD) {
		if(!res_miter_data) {
		    coutx excl_stream;
		    Cstring str;
		    str << "\neval_present_value: res " << name
			<< " is missing an iterator...\n";
		    excl_stream << str;
		    assert(false);
		}

		//
		// This will report the constraint-checking
		// loop's current time
		//
		eval_time = thread_intfc::current_time(thread_index);
		use_miterator = true;
	} else {
		if(res_miter_data) {
			if((eval_time = time_saver::get_now())
				== thread_intfc::current_time(thread_index)) {

				//
				// We are synchronous with the modeling loop
				//
				use_miterator = true;
			} else {

				//
				// We are operating in the future or requesting
				// a value in the past
				//
				use_miterator = false;
			}
		} else {

			//
			// We are not modeling; maybe just carrying out a decomposition
			//
			eval_time = time_saver::get_now();
			use_miterator = false;
		}
	}

	//
	// NORMAL case (the most frequently used)
	//
	if(strategy == EvalTiming::NORMAL) {

	    if(use_miterator) {
		    found = resHistory.latestHistNodeAtOrBeforeModelTime(
						eval_time,
						res_miter_data->iter);

		    // coutx excl_stream;
		    // Cstring str;
		    // str << "eval_present_value: time = " << eval_time.to_string() << "\n";
		    // excl_stream << str;
	    } else {

		    // coutx excl_stream;
		    // Cstring str;
		    // str << "eval_present_value: thread_index " << thread_index
		    // 	    << ", CONSTRAINT_THREAD = " << thread_intfc::CONSTRAINT_THREAD << "\n";
		    // excl_stream << str;

		    found = get_history().latestHistNodeAtOrBefore(eval_time);
	    }

	//
	// BEFORE case
	//
	} else if(strategy == EvalTiming::BEFORE) {

	    if(use_miterator) {
		    found = resHistory.latestHistNodeBeforeModelTime(
						eval_time,
						res_miter_data->iter);
	    } else {
		    found = get_history().latestHistNodeBefore(eval_time);
	    }

	//
	// INTERPOLATED case
	//
	} else if(strategy == EvalTiming::INTERPOLATED) {

	    //
	    // will need to interpolate. Note that consolidate() has
	    // already verified that this resource has its Interpolation
	    // attribute set to true.
	    //
	    // Note also that this should only be evaluated in the
	    // modeling thread; invoking following_node() is not
	    // safe from the constraint-checking thread.
	    //
	    if(use_miterator) {
		    found = resHistory.latestHistNodeAtOrBeforeModelTime(
						eval_time,
						res_miter_data->iter);
	    } else {
		    found = get_history().latestHistNodeAtOrBefore(eval_time);
	    }

	    if(found) {
			value_node*	another_node = NULL;
			another_node = found;
			while((	future_node = another_node->following_node())
				&& (future_node->Key.getetime() == found->Key.getetime())) {
				another_node = future_node;
			}
			if(future_node) {
				// we have a positive time interval over which to interpolate
				interpolate = true;
			}
	    } else {
		assert(false);
	    }
	}

	if(!found) {
		try {
			evaluate_profile(strategy, eval_time, returned_value);
		}
		catch(eval_error Err) {
			Cstring ErrMsg(name);
			ErrMsg << ": evaluate present value: error computing profile -\n";
			ErrMsg << Err.msg;
			throw(eval_error(ErrMsg));
		}

		if(evaluation_debug) {
			cerr << name << ": no history node at " << eval_time.to_string()
				<< "; returning profile = " << returned_value.to_string()
				<< "\n";
		}
		return;
	}

	try {
		evaluate_profile(strategy, eval_time, returned_value);
		returned_value.cast(get_datatype());
	} catch(eval_error Err) {
		Cstring ErrMsg(name);
		ErrMsg << ": error evaluating profile -\n" << Err.msg;
		throw(eval_error(ErrMsg));
	}

	//
	// num_value_node_i::get_value() correctly decodes time and
	// duration values encoded as true_long integers
	//
	TypedValue	temptv = found->get_value();

	//
	// F2 is the weight we should give the future node
	//

	double F2;
	if(interpolate) {
		CTime_base delta = future_node->Key.getetime() - found->Key.getetime();
		F2 = (eval_time - found->Key.getetime()) / delta;
	}
	switch(returned_value.get_type()) {
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
			returned_value = returned_value.get_time_or_duration() - temptv.get_time_or_duration();
			if(interpolate) {
				returned_value = returned_value.get_time_or_duration()
					- (future_node->get_value().get_time_or_duration()
							- found->get_value().get_time_or_duration()) * F2;
			}
			break;
		case apgen::DATA_TYPE::FLOATING:
			returned_value = returned_value.get_double() - temptv.get_double();
			if(interpolate) {
				returned_value = returned_value.get_double()
					- (future_node->get_value().get_double()
							- found->get_value().get_double()) * F2;
			}
			break;
		case apgen::DATA_TYPE::INTEGER:
			returned_value = returned_value.get_int() - temptv.get_int();
			if(interpolate) {
				returned_value = returned_value.get_int()
					- (long)((future_node->get_value().get_int()
							- found->get_value().get_int()) * F2);
			}
			break;

		//
		// pacify the compiler - handle all other types, even though
		// they are not applicable to numeric resources
		//
		default:
			break;
	}

	// debug
	// if(name == "SpacecraftGanymedeAltitude") {
	//     cerr << "    " << name << eval_time.to_string()
	// 	<< "; returning value = " << returned_value.to_string() << "\n";
	// }

	if(evaluation_debug) {
	    cerr << name << ": found history node at " << eval_time.to_string()
		<< "; returning value = " << returned_value.to_string()
		<< "\n";
	}
}

void RES_precomputed::evaluate_present_value(
			TypedValue&		returned_value,
			const CTime_base&	eval_time,
			LimitContainer*		lc) {
    aafReader::single_precomp_res* precomp_res = get_precomp_res();

    double result[2];

    try {
	returned_value = precomp_res->Evaluate(eval_time, result);
    } catch(eval_error Err) {
	Cstring err;
	err << name << " evaluation error " << Err.msg;
	throw(eval_error(err));
    }

    if(lc) {
	bool			returning = false;
	time_saver		ts;
	const parsedProg&	attrProgPtr(get_attribute_program());
	map<Cstring, pEsys::Assignment*>::const_iterator	errorHighIter;
	map<Cstring, pEsys::Assignment*>::const_iterator	errorLowIter;
	map<Cstring, pEsys::Assignment*>::const_iterator	warningHighIter;
	map<Cstring, pEsys::Assignment*>::const_iterator	warningLowIter;
	pEsys::Assignment*	errorHighInstr = NULL;
	pEsys::Assignment*	errorLowInstr = NULL;
	pEsys::Assignment*	warningHighInstr = NULL;
	pEsys::Assignment*	warningLowInstr = NULL;

	if(attrProgPtr) {
	    pEsys::Program& theAttrProg = *attrProgPtr.object();
	    errorHighIter = theAttrProg.symbols.find("errorhigh");
	    if(errorHighIter != theAttrProg.symbols.end()) {
		errorHighInstr = errorHighIter->second;
	    }
	    errorLowIter = theAttrProg.symbols.find("errorlow");
	    if(errorLowIter != theAttrProg.symbols.end()) {
		errorLowInstr = errorLowIter->second;
	    }
	    warningHighIter = theAttrProg.symbols.find("warninghigh");
	    if(warningHighIter != theAttrProg.symbols.end()) {
		warningHighInstr = warningHighIter->second;
	    }
	    warningLowIter = theAttrProg.symbols.find("warninglow");
	    if(warningLowIter != theAttrProg.symbols.end()) {
		warningLowInstr = warningLowIter->second;
	    }
	}

	ts.set_now_to(eval_time);
	try {
	    TypedValue V;
	    if(errorHighInstr) {
		errorHighInstr->Expression->eval_expression(Object.object(), V);
		lc->error_high_val = V.get_double();
	    }
	    if(warningHighInstr) {
		warningHighInstr->Expression->eval_expression(Object.object(), V);
		lc->warn_high_val = V.get_double();
	    }
	    if(warningLowInstr) {
		warningLowInstr->Expression->eval_expression(Object.object(), V);
		lc->warn_low_val = V.get_double();
	    }
	    if(errorLowInstr) {
		errorLowInstr->Expression->eval_expression(Object.object(), V);
		lc->error_low_val = V.get_double();
	    }
	}
	catch(eval_error Err) {
	    Cstring	error = name;
	    error << ": error while evaluating error/warning limit - details:\n"
		  << Err.msg;
	    throw(eval_error(error));
	}
    }
    
}

void RES_settable::evaluate_present_value(
			TypedValue&		returned_value,
			const CTime_base&	eval_time,
			LimitContainer*		lc) {

    value_node*			found = NULL;
    value_node*			also_found = NULL;
    bool			interpolate = false;
    value_node*			future_node = NULL;

    miterator_data* res_miter_data = peek_at_miterator_data();
    EvalTiming::Strategy strategy = res_miter_data ?
		res_miter_data->miter.strategy :
		EvalTiming::NORMAL;

    //
    // NORMAL case (the most frequently used)
    //
    if(strategy == EvalTiming::NORMAL) {
	found = get_history().latestHistNodeAtOrBefore(eval_time);

    //
    // BEFORE case
    //
    } else if(strategy == EvalTiming::BEFORE) {
	found = get_history().latestHistNodeBefore(eval_time);

    //
    // INTERPOLATED case
    //
    } else if(strategy == EvalTiming::INTERPOLATED) {

	//
	// will need to interpolate. Note that consolidate() has
	// already verified that this resource has its Interpolation
	// attribute set to true.
	//
	found = get_history().latestHistNodeAtOrBefore(eval_time);
	if(found) {
	    value_node*	another_node = NULL;
	    another_node = found;
	    while((	future_node = another_node->following_node())
			&& (future_node->Key.getetime() == found->Key.getetime())) {
			another_node = future_node;
	    }
	    if(future_node) {

		//
		// we have a positive time interval over which to interpolate
		//
		interpolate = true;
	    }
	} else {
	    assert(false);
	}
    }


    if(lc) {
	bool			returning = false;
	time_saver		ts;
	const parsedProg&	attrProgPtr(get_attribute_program());
	map<Cstring, pEsys::Assignment*>::const_iterator	errorHighIter;
	map<Cstring, pEsys::Assignment*>::const_iterator	errorLowIter;
	map<Cstring, pEsys::Assignment*>::const_iterator	warningHighIter;
	map<Cstring, pEsys::Assignment*>::const_iterator	warningLowIter;
	pEsys::Assignment*	errorHighInstr = NULL;
	pEsys::Assignment*	errorLowInstr = NULL;
	pEsys::Assignment*	warningHighInstr = NULL;
	pEsys::Assignment*	warningLowInstr = NULL;

	if(attrProgPtr) {
	    pEsys::Program& theAttrProg = *attrProgPtr.object();
	    errorHighIter = theAttrProg.symbols.find("errorhigh");
	    if(errorHighIter != theAttrProg.symbols.end()) {
		errorHighInstr = errorHighIter->second;
	    }
	    errorLowIter = theAttrProg.symbols.find("errorlow");
	    if(errorLowIter != theAttrProg.symbols.end()) {
		errorLowInstr = errorLowIter->second;
	    }
	    warningHighIter = theAttrProg.symbols.find("warninghigh");
	    if(warningHighIter != theAttrProg.symbols.end()) {
		warningHighInstr = warningHighIter->second;
	    }
	    warningLowIter = theAttrProg.symbols.find("warninglow");
	    if(warningLowIter != theAttrProg.symbols.end()) {
		warningLowInstr = warningLowIter->second;
	    }
	}

	ts.set_now_to(eval_time);
	try {
	    TypedValue V;
	    if(errorHighInstr) {
		errorHighInstr->Expression->eval_expression(Object.object(), V);
		lc->error_high_val = V.get_double();
	    }
	    if(warningHighInstr) {
		warningHighInstr->Expression->eval_expression(Object.object(), V);
		lc->warn_high_val = V.get_double();
	    }
	    if(warningLowInstr) {
		warningLowInstr->Expression->eval_expression(Object.object(), V);
		lc->warn_low_val = V.get_double();
	    }
	    if(errorLowInstr) {
		errorLowInstr->Expression->eval_expression(Object.object(), V);
		lc->error_low_val = V.get_double();
	    }
	}
	catch(eval_error Err) {
	    Cstring	error = name;
	    error << ": error while evaluating error/warning limit - details:\n"
		  << Err.msg;
	    throw(eval_error(error));
	}
    }

    //
    // If no node was found in the history, return the profile value
    //
    if(!found) {
	try {
	    evaluate_profile(strategy, eval_time, returned_value);
	}
	catch(eval_error Err) {
	    Cstring ErrMsg(name);
	    ErrMsg << ": evaluate present value: error computing profile -\n";
	    ErrMsg << Err.msg;
	    throw(eval_error(ErrMsg));
	}

	if(evaluation_debug) {
	    cerr << name << ": no history node at " << eval_time.to_string()
		 << "; returning profile = " << returned_value.to_string()
		 << "\n";
	}
	return;
    }

    //
    // num_value_node_i::get_value() correctly decodes time and
    // duration values encoded as true_long integers
    //
    returned_value = found->get_value();

    //
    // Last, handle interpolation
    //
    if(interpolate) {

	//
	// F2 is the weight we should give the future node
	//
	double F2;
	CTime_base delta = future_node->Key.getetime() - found->Key.getetime();
	F2 = (eval_time - found->Key.getetime()) / delta;
	switch(returned_value.get_type()) {
	    case apgen::DATA_TYPE::TIME:
	    case apgen::DATA_TYPE::DURATION:
		returned_value = returned_value.get_time_or_duration()
				+ (future_node->get_value().get_time_or_duration()
				- found->get_value().get_time_or_duration()) * F2;
		break;
	    case apgen::DATA_TYPE::FLOATING:
		returned_value = returned_value.get_double()
				+ (future_node->get_value().get_double()
				- found->get_value().get_double()) * F2;
		break;
	    case apgen::DATA_TYPE::INTEGER:
		returned_value = returned_value.get_int()
				+ (long)((future_node->get_value().get_int()
						- found->get_value().get_int()) * F2);
		break;

	    //
	    // pacify the compiler - handle all other types, even though
	    // they are not applicable to numeric resources
	    //
	    default:
		break;
	}
    }

    if(evaluation_debug) {
	cerr << name << ": found history node at " << eval_time.to_string()
	     << "; returning value = " << returned_value.to_string()
	     << "\n";
    }
}

void RES_precomputed::evaluate_present_value(
			TypedValue&		returned_value) {
    CTime_base	eval_time;

    if(thread_index == thread_intfc::CONSTRAINT_THREAD) {

	//
	// This will report the constraint-checking
	// loop's current time
	//
	eval_time = thread_intfc::current_time(thread_index);
    } else {
	eval_time = time_saver::get_now();
    }
    evaluate_present_value(returned_value, eval_time);
}

void RES_settable::evaluate_present_value(
			TypedValue&		returned_value) {
    value_node*			found = NULL;
    value_node*			also_found = NULL;
    bool			interpolate = false;
    value_node*			future_node = NULL;

    //
    // If this is the modeling thead, the current modeling
    // time to use is always 'now', even if we are in the
    // middle of get_windows() processing.
    //
    // If this is the constraint-checking thread, then we
    // don't want to use peek() data, because the thread evaluates
    // constraints just _before_ jumping to the new time.
    // But luckily for us, the thread stores its current time
    // (just before peek() time) in a thread-local variable,
    // so that's what we use.
    //
    // thread_intfc::current_time(thread_index) reports the time of the current
    // event in the event loop (either the constraint-checking
    // loop or the event loop); it is NOT the same as 'now'.
    // Be careful to make the distinction.
    //
    miterator_data* res_miter_data = peek_at_miterator_data();
    EvalTiming::Strategy strategy = res_miter_data ?
		res_miter_data->miter.strategy :
		EvalTiming::NORMAL;

    CTime_base	eval_time;
    bool	use_miterator;

    if(thread_index == thread_intfc::CONSTRAINT_THREAD) {
	if(!res_miter_data) {
	    coutx excl_stream;
	    Cstring str;
	    str << "\neval_present_value: res " << name
		<< " is missing an iterator...\n";
	    excl_stream << str;
	    assert(false);
	}

	//
	// This will report the constraint-checking
	// loop's current time
	//
	eval_time = thread_intfc::current_time(thread_index);
	use_miterator = true;
    } else {
	if(res_miter_data) {
	    if((eval_time = time_saver::get_now())
		== thread_intfc::current_time(thread_index)) {

		//
		// We are synchronous with the modeling loop
		//
		use_miterator = true;
	    } else {

		//
		// We are operating in the future or requesting
		// a value in the past
		//
		use_miterator = false;
	    }
	} else {

	    //
	    // We are not modeling; maybe just carrying out a decomposition
	    //
	    eval_time = time_saver::get_now();
	    use_miterator = false;
	}
    }

    //
    // NORMAL case (the most frequently used)
    //
    if(strategy == EvalTiming::NORMAL) {

	if(use_miterator) {
	    found = resHistory.latestHistNodeAtOrBeforeModelTime(
						eval_time,
						res_miter_data->iter);
	} else {
	    found = get_history().latestHistNodeAtOrBefore(eval_time);
	}

    //
    // BEFORE case
    //
    } else if(strategy == EvalTiming::BEFORE) {

	if(use_miterator) {
	    found = resHistory.latestHistNodeBeforeModelTime(
						eval_time,
						res_miter_data->iter);
	} else {
	    found = get_history().latestHistNodeBefore(eval_time);
	}

    //
    // INTERPOLATED case
    //
    } else if(strategy == EvalTiming::INTERPOLATED) {

	//
	// will need to interpolate. Note that consolidate() has
	// already verified that this resource has its Interpolation
	// attribute set to true.
	//
	// Note also that this should only be evaluated in the
	// modeling thread; invoking following_node() is not
	// safe from the constraint-checking thread.
	//
	if(use_miterator) {
		found = resHistory.latestHistNodeAtOrBeforeModelTime(
						eval_time,
						res_miter_data->iter);
	} else {
		found = get_history().latestHistNodeAtOrBefore(eval_time);
	}

	if(found) {
		value_node*	another_node = NULL;
		another_node = found;
		while((	future_node = another_node->following_node())
			&& (future_node->Key.getetime() == found->Key.getetime())) {
			another_node = future_node;
		}
		if(future_node) {
		    // we have a positive time interval over which to interpolate
		    interpolate = true;
		}
	} else {
		assert(false);
	}
    }

    if(!found) {
	    try {
		evaluate_profile(strategy, eval_time, returned_value);
	    }
	    catch(eval_error Err) {
		Cstring ErrMsg(name);
		ErrMsg << ": evaluate present value: error computing profile -\n";
		ErrMsg << Err.msg;
		throw(eval_error(ErrMsg));
	    }

	    if(evaluation_debug) {
		cerr << name << ": no history node at " << eval_time.to_string()
		    << "; returning profile = " << returned_value.to_string()
		    << "\n";
	    }
	    return;
    }

    //
    // num_value_node_i::get_value() correctly decodes time and
    // duration values encoded as true_long integers
    //
    returned_value = found->get_value();

    //
    // Finally, handle interpolation
    //
    if(interpolate) {

	    //
	    // F2 is the weight we should give the future node
	    //
	    double F2;
	    CTime_base delta = future_node->Key.getetime() - found->Key.getetime();
	    F2 = (eval_time - found->Key.getetime()) / delta;
	    switch(returned_value.get_type()) {
		case apgen::DATA_TYPE::TIME:
		case apgen::DATA_TYPE::DURATION:
		    returned_value = returned_value.get_time_or_duration()
					+ (future_node->get_value().get_time_or_duration()
						- found->get_value().get_time_or_duration()) * F2;
		    break;
		case apgen::DATA_TYPE::FLOATING:
		    returned_value = returned_value.get_double()
					+ (future_node->get_value().get_double()
						- found->get_value().get_double()) * F2;
		    break;
		case apgen::DATA_TYPE::INTEGER:
		    returned_value = returned_value.get_int()
					+ (long)((future_node->get_value().get_int()
						- found->get_value().get_int()) * F2);
		    break;

		//
		// pacify the compiler - handle all other types, even though
		// they are not applicable to numeric resources
		//
		default:
		    break;
	    }
    }

    if(evaluation_debug) {
	cerr << name << ": found history node at " << eval_time.to_string()
	     << "; returning value = " << returned_value.to_string()
	     << "\n";
    }
}

apgen::DATA_TYPE& RES_history::get_datatype() {
	return resource_parent->get_datatype();
}

//
// NOTES
// =====
//
// 1. One should not call this method before consolidating resources.
//    In fact, the multiterator should be built precisely at the end of the
//    consolidation process. See model_intfc::consolidate() in RES_exec.C.
//
// 2. The only customers of this method are
//
// 		RES_history::get_discrete_values(),
//    		RES_history::get_settable_values() and
//    		RES_history::get_numeric_values(),
//
//    which are used by the GUI to populate its resource displays.
//
// 3. The simple_res_miter class registers itself with the Rsource history
//    list, so that we can use the miterator_data object to retrieve Strategy
//    information instead of passing it explicitly as we used to.
//
simple_res_miter*
RES_history::get_the_multiterator() {
	slist<alpha_void, Cnode0<alpha_void, RCsource*> >::iterator
		iter(resource_parent->parent_container.payload->ptrs_to_containers_used_in_profile);
	Cnode0<alpha_void, RCsource*>*		ptr_to_prerequisite;

	the_multiterator.clear();
	the_multiterator.add_thread(
				historyList,
				resource_parent->name,
				1 /* priority */ ,
				true /* store */ );
	while((ptr_to_prerequisite = iter())) {
		RCsource*	prerequisite = ptr_to_prerequisite->payload;
		simple_res_iter	sri(prerequisite);
		Rsource*	resource;

		while((resource = sri.next())) {
			RES_history* hist_ptr = &resource->get_history();

			assert(hist_ptr);

			the_multiterator.add_thread(
				hist_ptr->historyList,
				resource->name,
				2 /* priority */,
				true /* store */ );
		}
	}
	return &the_multiterator;
}

void RES_history::get_discrete_values(
		const CTime_base&					start_time,
		const CTime_base&					span,
		tlist<alpha_time, unattached_value_node, Rsource*>&	result,
		bool							all_nodes_flag,
		long&							returned_version
		) {
	multilist<prio_time, value_node, Rsource*>& h = historyList;
	simple_res_miter*		multi = get_the_multiterator();
	Rsource*			This = get_resource();

	assert(This);

	TypedValue			evaluated_val, profile_value, current_value, previous_value;
	value_node*			current_node;
	bool				first_node_occurs_after_start;
	bool				last_iteration = false;

	tlist<alpha_void, Cnode0<alpha_void, value_node*> > stack;
	Cnode0<alpha_void, value_node*>* ptr;
	value_node*			last_set_or_reset = NULL;
	bool				last_set_or_reset_is_set = false;
	TypedValue			value_to_use;
	TypedValue			value_last_used;
	bool				value_last_used_is_set = false;
	CTime_base			time_last_stored;

	multi->next();
	while((current_node = multi->next())) {

		CTime_base etime = current_node->Key.getetime();

#ifdef IT_SEEMS_TO_ME_THIS_IS_NOT_NECESSARY
		//
		// UUUU This is dangerous - who's going to reset
		// the current time to its previous value?
		//
		thread_intfc::update_current_time(etime);
#endif /*  IT_SEEMS_TO_ME_THIS_IS_NOT_NECESSARY */

		if(current_node->list == &h) {

			//
			// this is one of 'our' nodes
			//
			if(current_node->get_res_val_type() == apgen::RES_VAL_TYPE::START_USING) {
				stack << new Cnode0<alpha_void, value_node*>((void *) current_node, current_node);
				value_to_use = current_node->get_value();
			} else if(current_node->get_res_val_type() == apgen::RES_VAL_TYPE::END_USING) {
				if((ptr = stack.find((void *) current_node->get_other()))) {
					delete ptr;
				}
				if((ptr = stack.last_node())) {
					value_to_use = ptr->payload->get_value();
					current_node->set_value(value_to_use);
				} else if(last_set_or_reset && last_set_or_reset_is_set) {

					//
					// no usages left; Was there a set clause?
					//
					value_to_use = last_set_or_reset->get_value();
					current_node->set_value(value_to_use);
				} else {

					//
					// no 'set' usage; use profile
					//
					try {
						This->evaluate_profile(
								EvalTiming::NORMAL,
								etime,
								value_to_use);
					}
					catch(eval_error Err) {
						Cstring		e("get_vals_from_miterator error for resource ");
	
						e << This->name << "; details:\n" << Err.msg;
						throw(eval_error(e));
					}
				}
			} else if(	current_node->get_res_val_type() == apgen::RES_VAL_TYPE::SET
					|| current_node->get_res_val_type() == apgen::RES_VAL_TYPE::RESET
			       ) {

				if(current_node->get_res_val_type() == apgen::RES_VAL_TYPE::SET) {
					last_set_or_reset = current_node;
					last_set_or_reset_is_set = true;
				} else {
					last_set_or_reset = current_node;
					last_set_or_reset_is_set = false;
				}

				if(stack.get_length() == 0) {
					value_to_use = current_node->get_value();
				}
			}
			
		} else {

			//
			// this is a profile node.
			//
			if(	stack.get_length() == 0
				&& (	!last_set_or_reset
					|| !last_set_or_reset_is_set
				)
			  ) {

				This->evaluate_present_value(
						value_to_use,
						etime);
			}

			//
			// else, the profile node is ignored - has no effect
			//
		}
	

		if(value_to_use.get_type() != apgen::DATA_TYPE::UNINITIALIZED) {

			//
			// very first thing to do: repeat previous value if it exists
			//
			if(	value_last_used_is_set
				&& time_last_stored < etime) {

				result << new unattached_value_node(
						value_last_used,
						This->get_datatype(),
						etime);
			}

			result << new unattached_value_node(
					value_to_use,
					This->get_datatype(),
					etime);
			value_last_used_is_set = true;
			value_last_used = value_to_use;
			time_last_stored = etime;
		}

		if(etime >= start_time + span && !all_nodes_flag) {
			last_iteration = true;
			break;
		}
	}
	if(!last_iteration) {

		//
		// we haven't explicitly taken care of the endpoint; time to do it now.
		//
		if(current_node) {
		    	TypedValue		V;
    			CTime_base		last_time(current_node->Key.getetime());
			EvalTiming::strategy_setter setter(This, EvalTiming::BEFORE);

    			This->evaluate_present_value(
				V,
				last_time);

    			result << new unattached_value_node(V, This->get_datatype(), last_time);
		}

		//
		// must be careful here: History could be empty... (defensive programming)
		//
		else if(result.get_length()) {
    			CTime_base		last_time;
    			unattached_value_node*	uvn;

    			uvn = result.last_node();
    			if(	eval_intfc::there_is_a_last_event(last_time)
    				&& last_time > uvn->Key.getetime()) {
    				TypedValue		V;

    				This->evaluate_present_value(
					V,
					last_time);

    				result << new unattached_value_node(V, This->get_datatype(), last_time);
			}
		}
	}
	returned_version = This->get_res_version();
	multi->clear();
}

TypedValue RES_state::get_partial_history(
		CTime_base			start_time,
		CTime_base			span) {
	tlist<alpha_time, unattached_value_node, Rsource*>	l;
	get_history().get_partial_state_history(
					start_time,
					span,
					l,
					false);
	TypedValue	retval;
	ListOVal*	ret = new ListOVal;
	ListOVal*	times = new ListOVal;
	ListOVal*	values = new ListOVal;
	unattached_value_node* vn;
	long int	i = 0;
	ArrayElement*	ae;

	for(vn = l.first_node(); vn; vn = vn->next_node()) {
		TypedValue T;
		T = vn->getKey().getetime();
		ae = new ArrayElement(i);
		ae->SetVal(T);
		(*times) << ae;
		ae = new ArrayElement(i);
		ae->SetVal(vn->get_value());
		(*values) << ae;
		i++;
	}
	TypedValue A;
	A = *times;
	ae = new ArrayElement(Cstring("times"));
	ae->SetVal(A);
	(*ret) << ae;
	A = *values;
	ae = new ArrayElement(Cstring("values"));
	ae->SetVal(A);
	(*ret) << ae;
	retval = *ret;
	return retval;
}

void RES_history::get_partial_state_history(
		const CTime_base&					start_time,
		const CTime_base&					span,
		tlist<alpha_time, unattached_value_node, Rsource*>&	result,
		bool							all_nodes_flag
		) {
	multilist<prio_time, value_node, Rsource*>& h = historyList;
	simple_res_miter*		multi = get_the_multiterator();
	Rsource*			This = get_resource();
	CTime_base			end_time = start_time + span;

	assert(This);

	TypedValue			evaluated_val, profile_value, current_value, previous_value;
	value_node*			current_node;

	tlist<alpha_void, Cnode0<alpha_void, value_node*> > stack;
	Cnode0<alpha_void, value_node*>* ptr;
	value_node*			last_set_or_reset = NULL;
	bool				last_set_or_reset_is_set = false;
	TypedValue			value_to_use;
	TypedValue			value_last_used;
	bool				value_last_used_is_set = false;
	CTime_base			time_last_stored;

	multi->next();
	while((current_node = multi->next())) {

		CTime_base etime = current_node->Key.getetime();

#ifdef IT_SEEMS_TO_ME_THIS_IS_NOT_NECESSARY

		//
		// UUUU This is dangerous - who's going to reset
		// the current time to its previous value?
		//
		thread_intfc::update_current_time(etime);

#endif /* IT_SEEMS_TO_ME_THIS_IS_NOT_NECESSARY */

		if(current_node->list == &h) {

			//
			// this is one of 'our' nodes
			//
			if(current_node->get_res_val_type() == apgen::RES_VAL_TYPE::START_USING) {
				stack << new Cnode0<alpha_void, value_node*>((void *) current_node, current_node);
				value_to_use = current_node->get_value();
			} else if(current_node->get_res_val_type() == apgen::RES_VAL_TYPE::END_USING) {
				if((ptr = stack.find((void *) current_node->get_other()))) {
					delete ptr;
				}
				if((ptr = stack.last_node())) {
					value_to_use = ptr->payload->get_value();
					current_node->set_value(value_to_use);
				} else if(last_set_or_reset && last_set_or_reset_is_set) {

					//
					// no usages left; Was there a set clause?
					//
					value_to_use = last_set_or_reset->get_value();
					current_node->set_value(value_to_use);
				} else {

					//
					// no 'set' usage; use profile
					//
					try {
						This->evaluate_profile(
								EvalTiming::NORMAL,
								etime,
								value_to_use);
					}
					catch(eval_error Err) {
						Cstring		e("get_vals_from_miterator error for resource ");
	
						e << This->name << "; details:\n" << Err.msg;
						throw(eval_error(e));
					}
				}
			} else if(	current_node->get_res_val_type() == apgen::RES_VAL_TYPE::SET
					|| current_node->get_res_val_type() == apgen::RES_VAL_TYPE::RESET
			       ) {

				if(current_node->get_res_val_type() == apgen::RES_VAL_TYPE::SET) {
					last_set_or_reset = current_node;
					last_set_or_reset_is_set = true;
				} else {
					last_set_or_reset = current_node;
					last_set_or_reset_is_set = false;
				}

				if(stack.get_length() == 0) {
					value_to_use = current_node->get_value();
				}
			}
			
		} else {

			//
			// this is a profile node.
			//
			if(	stack.get_length() == 0
				&& (	!last_set_or_reset
					|| !last_set_or_reset_is_set
				)
			  ) {

				This->evaluate_present_value(
						value_to_use,
						etime);
			}

			//
			// else, the profile node is ignored - has no effect
			//
		}
	

		if(value_to_use.get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
			if(all_nodes_flag || (etime >= start_time && etime <= end_time)) {

				//
				// We do not repeat previous value if it exists
				//
				result << new unattached_value_node(
					value_to_use,
					This->get_datatype(),
					etime);
			}
			value_last_used_is_set = true;
			value_last_used = value_to_use;
			time_last_stored = etime;
		}

		if(etime >= start_time + span && !all_nodes_flag) {
			break;
		}
	}
	multi->clear();
}

static bool	check_for_crossing(
			const TypedValue& currentval, double currentref,
			const TypedValue& previousval, double previousref,
			const CTime_base& currenttime, const CTime_base& previoustime,
			CTime_base& newtime, double& newval) {
	double delta2 = currentval.get_double() - currentref;
	double delta1 = previousval.get_double() - previousref;

	if(delta1 > 0.0 && delta2 <= 0.0) {
		double		delta = delta1 - delta2;
		CTime_base	deltaT = currenttime - previoustime;
		double		fraction = delta1 / delta;

		newtime = previoustime + fraction * deltaT;
		newval = previousval.get_double() + fraction * (currentval.get_double() - previousval.get_double());

		return true;
	} else if(delta1 <= 0.0 && delta2 > 0.0) {
		double		delta = delta2 - delta1;
		CTime_base	deltaT = currenttime - previoustime;
		double		fraction = -delta1 / delta;

		newtime = previoustime + (fraction * deltaT);
		newval = previousval.get_double() + fraction * (currentval.get_double() - previousval.get_double());

		return true;
	}

	return false;
}

void RES_history::get_numeric_values(
		const CTime_base&					start_time,
		const CTime_base&					span,
		tlist<alpha_time, unattached_value_node, Rsource*>&	result,
		bool							all_nodes_flag,
		long&							returned_version
		) {
	Rsource*					This = get_resource();
	multilist<prio_time, value_node, Rsource*>&	multiHist = historyList;
	simple_res_miter*				multi = get_the_multiterator();
	

	RES_numeric*	checkThis = dynamic_cast<RES_numeric*>(multiHist.Owner);

	assert(checkThis);

	TypedValue	evaluated_val, profile_value, current_value, previous_value;
	value_node*	current_node, *previous_consuming_node = NULL;
	bool		first_node_occurs_after_start;
	bool		show_discontinuities = true;
	bool		last_iteration = false;

	//
	// NUMERIC RESOURCE CASE
	//
	bool		maxSym_exists	= This->properties[(int)Rsource::Property::has_maximum];
	bool		minSym_exists	= This->properties[(int)Rsource::Property::has_minimum];

	map<Cstring, pEsys::Assignment*>::const_iterator	errorHighIter;
	map<Cstring, pEsys::Assignment*>::const_iterator	errorLowIter;
	map<Cstring, pEsys::Assignment*>::const_iterator	warningHighIter;
	map<Cstring, pEsys::Assignment*>::const_iterator	warningLowIter;
	pEsys::Assignment*	errorHighInstr = NULL;
	pEsys::Assignment*	errorLowInstr = NULL;
	pEsys::Assignment*	warningHighInstr = NULL;
	pEsys::Assignment*	warningLowInstr = NULL;
	double			old_value;
	TypedValue		ival;
	double			lval;
	CTime_base		itime(false);
	value_node*		prev_node = NULL;
	value_node*		peeked_at;
	unattached_value_node*	new_node;
	unattached_value_node*	old_node = NULL;
	double errorHigh = 0.0, errorLow = 0.0, warnHigh = 0.0, warnLow = 0.0;

	//
	// Only show discontinuities if interpolation is set to false
	//
	show_discontinuities = !This->properties[(int)Rsource::Property::has_interpolation];

	const parsedProg&	attrProgPtr(This->get_attribute_program());

	if(attrProgPtr) {
		pEsys::Program&	theAttrProg = *attrProgPtr.object();
		errorHighIter = theAttrProg.symbols.find("errorhigh");
		if(errorHighIter != theAttrProg.symbols.end()) {
			errorHighInstr = errorHighIter->second;
		}
		errorLowIter = theAttrProg.symbols.find("errorlow");
		if(errorLowIter != theAttrProg.symbols.end()) {
			errorLowInstr = errorLowIter->second;
		}
		warningHighIter = theAttrProg.symbols.find("warninghigh");
		if(warningHighIter != theAttrProg.symbols.end()) {
			warningHighInstr = warningHighIter->second;
		}
		warningLowIter = theAttrProg.symbols.find("warninglow");
		if(warningLowIter != theAttrProg.symbols.end()) {
			warningLowInstr = warningLowIter->second;
		}
	}
	if(errorHighInstr || errorLowInstr || warningHighInstr || warningLowInstr) {
		if(maxSym_exists || minSym_exists) {
			Cstring errs = This->name;
			errs << ": error trying to get resource values - both Max/Min and "
				"Warning/Error attributes are used. Please use either "
				"Maximum/Minimum, or (Warning High/Low) / (Error High/Low).";
			throw(eval_error(errs));
		}
		if(show_discontinuities) {
			Cstring errs = This->name;
			errs << ": error trying to get resource values - Error/Warning limits are given, "
				"but the \"Interpolation\" attribute is not set or it is set to false. Please "
				"set the \"Interpolation\" attribute to true.";
			throw(eval_error(errs));
		}

	}

	multi->next();
	current_node = multi->next();
	if(!current_node) {
		first_node_occurs_after_start = false;
		multi->clear();
		return;
	}

	// while(current_node = (value_node *) localator.next())
	if(!all_nodes_flag) {
		if(current_node->Key.getetime() < start_time ) {
			while((peeked_at = multi->peek()) && (peeked_at->Key.getetime() < start_time)) {
				current_node = multi->next();
			}
		}
	}

	// set the flag indicating whether the first endpoint is covered:
	if(current_node->Key.getetime() > start_time) {
		first_node_occurs_after_start = true;
	} else {
		first_node_occurs_after_start = false;
	}

	while(current_node) {

		//
		// The profile value may originate from our own history,
		// but it can also originate from the history of a
		// resource we depend on. This Boolean variable keeps track of
		// which kind we are looking at:
		//
		bool	node_is_local;

		if(current_node->Key.getetime() > start_time + span && (!all_nodes_flag)) {
		    last_iteration = true;
		}

		//
		// find the first local node with time = current time,
		// if possible. We determine locality by checking the
		// list that current_node belongs to.
		//
		while(!(node_is_local = (current_node->list == &multiHist))) {
		    peeked_at = multi->peek();
		    if(peeked_at && peeked_at->Key.getetime() == current_node->Key.getetime()) {
			current_node = multi->next();
		    } else {
			break;
		    }
		}

		if(show_discontinuities) {

		    //
		    // Evaluate profile and store the result in profile_value
		    //
		    try {
			This->evaluate_profile(
				EvalTiming::BEFORE,
				current_node->Key.getetime(),
				profile_value);
		    }
		    catch(eval_error Err) {
			Cstring		e("get_vals_from_miterator error for resource ");

			e << This->name << "; details:\n" << Err.msg;
			throw(eval_error(e));
		    }

		    if(previous_consuming_node) {
			TypedValue	tv = previous_consuming_node->get_value();

			switch(get_datatype()) {
			    case apgen::DATA_TYPE::FLOATING:
				current_value = profile_value.get_double() - tv.get_double();
				break;
			    case apgen::DATA_TYPE::TIME:
				current_value = profile_value.get_time_or_duration()
					- CTime_base::convert_from_double_use_with_caution(tv.get_double(), true);
				break;
			    case apgen::DATA_TYPE::DURATION:
				current_value = profile_value.get_time_or_duration()
					- CTime_base::convert_from_double_use_with_caution(tv.get_double(), true);
				break;
			    case apgen::DATA_TYPE::INTEGER:
				current_value = profile_value.get_int()
					- (long) tv.get_double();
				break;

			    //
			    // pacify the compiler - handle all other types, even though
			    // they are not applicable to numeric resources
			    //
			    default:
				break;
			}

		    } else {
			current_value = profile_value;
		    }

		    result << new unattached_value_node(
					current_value,
					This->get_datatype(),
					current_node->Key.getetime());
		    if(last_iteration) {
			break;
		    }
		}


		try {
		    This->evaluate_profile(
					EvalTiming::NORMAL,
					current_node->Key.getetime(),
					profile_value);
		    profile_value.cast(This->get_datatype());

		} catch(eval_error Err) {
		    Cstring		e("get_vals_from_miterator error for resource ");

		    e << This->name << "; details:\n" << Err.msg;
		    throw(eval_error(e));
		}

		if(node_is_local) {
		    const TypedValue&	current_node_val = current_node->get_value();

		    switch(get_datatype()) {
			case apgen::DATA_TYPE::TIME:
			    current_value = profile_value.get_time_or_duration()
			    	- CTime_base::convert_from_double_use_with_caution(
							current_node_val.get_double(),
							true);
			    break;
			case apgen::DATA_TYPE::DURATION:
			    current_value = profile_value.get_time_or_duration()
			    	- CTime_base::convert_from_double_use_with_caution(
							current_node_val.get_double(),
							true);
			    break;
			case apgen::DATA_TYPE::FLOATING:
			    current_value = profile_value.get_double() - current_node_val.get_double();
			    break;
			case apgen::DATA_TYPE::INTEGER:
			    current_value = profile_value.get_int() - (long) current_node_val.get_double();
			    break;

			//
			// pacify the compiler - handle all other types, even though
			// they are not applicable to numeric resources
			//
			default:
			    break;
		    }

		} else {
		    if(previous_consuming_node) {
			const TypedValue&	tv = previous_consuming_node->get_value();

			switch(get_datatype()) {
			    case apgen::DATA_TYPE::FLOATING:
				current_value = profile_value.get_double() - tv.get_double();
				break;
			    case apgen::DATA_TYPE::TIME:
			    case apgen::DATA_TYPE::DURATION:
				current_value = profile_value.get_time_or_duration() - tv.get_time_or_duration();
				break;
			    case apgen::DATA_TYPE::INTEGER:
				current_value = profile_value.get_int() - (long) tv.get_double();
				break;

			    //
			    // pacify the compiler - handle all other types, even though
			    // they are not applicable to numeric resources
			    //
			    default:
				break;
			}

		    } else {

			current_value = profile_value;
		    }
		}

		if(errorHighInstr || errorLowInstr || warningHighInstr || warningLowInstr) {
		    time_saver			ts;

		    ts.set_now_to(current_node->Key.getetime());
		    try {
			TypedValue V;
			if(errorHighInstr) {
			    errorHighInstr->Expression->eval_expression(This->Object.object(), V);
			    errorHigh = V.get_double();
			}
			if(warningHighInstr) {
			    warningHighInstr->Expression->eval_expression(This->Object.object(), V);
			    warnHigh = V.get_double();
			}
			if(warningLowInstr) {
			    warningLowInstr->Expression->eval_expression(This->Object.object(), V);
			    warnLow = V.get_double();
			}
			if(errorLowInstr) {
			    errorLowInstr->Expression->eval_expression(This->Object.object(), V);
			    errorLow = V.get_double();
			}
		    } catch(eval_error Err) {
			Cstring	error = This->name;
			error << ": error while evaluating error/warning limit - details:\n"
			    << Err.msg;
			throw(eval_error(error));
		    }
		    new_node = new unattached_node_w_limits(
					current_value,
					This->get_datatype(),
					current_node->Key.getetime(),
					errorHigh,
					warnHigh,
					warnLow,
					errorLow);
		} else {
		    new_node = new unattached_value_node(
					current_value,
					This->get_datatype(),
					current_node->Key.getetime());
		}

		bool discard_the_new_node = false;
		if(	    (!show_discontinuities) && prev_node
			    && current_node->Key.getetime() > prev_node->Key.getetime()) {
		    if(maxSym_exists) {
			TypedValue	maxSym = (*This->Object)["maximum"];
			double	theMax = maxSym.get_double();
			if(previous_value.get_double() > theMax && current_value.get_double() <= theMax) {
				double		delta = previous_value.get_double() - current_value.get_double();
				CTime_base	deltaT = current_node->Key.getetime() - prev_node->Key.getetime();
				double		fraction = (previous_value.get_double() - theMax) / delta;
				CTime_base	itime = prev_node->Key.getetime() + (fraction * deltaT);

				ival = maxSym;
				discard_the_new_node = true;
				result << new unattached_value_node(ival, This->get_datatype(), itime);
			} else if(previous_value.get_double() <= theMax && current_value.get_double() > theMax) {
				double		delta = current_value.get_double() - previous_value.get_double();
				CTime_base	deltaT = current_node->Key.getetime() - prev_node->Key.getetime();
				double		fraction = (theMax - previous_value.get_double()) / delta;
				CTime_base	itime = prev_node->Key.getetime() + (fraction * deltaT);

				ival = maxSym;
				discard_the_new_node = true;
				result << new unattached_value_node(ival, This->get_datatype(), itime);
			}
		    }
		    if(minSym_exists) {
			TypedValue	minSym = (*This->Object)["minimum"];
			double	theMin = minSym.get_double();
			if(previous_value.get_double() >= theMin && current_value.get_double() < theMin) {
				double		delta = previous_value.get_double() - current_value.get_double();
				CTime_base	deltaT = current_node->Key.getetime() - prev_node->Key.getetime();
				double		fraction = (previous_value.get_double() - theMin) / delta;
				CTime_base	itime = prev_node->Key.getetime() + (fraction * deltaT);

				ival = minSym;
				discard_the_new_node = true;
				result << new unattached_value_node(ival, This->get_datatype(), itime);
			} else if(previous_value.get_double() < theMin && current_value.get_double() >= theMin) {
				double		delta = current_value.get_double() - previous_value.get_double();
				CTime_base	deltaT = current_node->Key.getetime() - prev_node->Key.getetime();
				double		fraction = (theMin - previous_value.get_double()) / delta;
				CTime_base	itime = prev_node->Key.getetime() + (fraction * deltaT);

				ival = minSym;
				discard_the_new_node = true;
				result << new unattached_value_node(ival, This->get_datatype(), itime);
			}
		    }

		    if(errorHighInstr) {
			if(check_for_crossing(
					current_value, errorHigh,
					previous_value, old_node->get_error_high(),
					current_node->Key.getetime(), prev_node->Key.getetime(),
					itime, lval)) {
			    discard_the_new_node = true;
			    ival = convert_from_double_to_typed_value(lval, This->get_datatype());
			    result << new unattached_node_w_limits(
						ival, This->get_datatype(), itime,
						lval,
						warnHigh,
						warnLow,
						errorLow);
			}
		    }
		    if(warningHighInstr) {
			if(check_for_crossing(
					current_value, warnHigh,
					previous_value, old_node->get_warn_high(),
					current_node->Key.getetime(), prev_node->Key.getetime(),
					itime, lval)) {
				discard_the_new_node = true;
				ival = convert_from_double_to_typed_value(lval, This->get_datatype());
				result << new unattached_node_w_limits(
						ival, This->get_datatype(), itime,
						errorHigh,
						lval,
						warnLow,
						errorLow);
			}
		    }
		    if(warningLowInstr) {
			if(check_for_crossing(
					current_value, warnLow,
					previous_value, old_node->get_warn_low(),
					current_node->Key.getetime(), prev_node->Key.getetime(),
					itime, lval)) {
				discard_the_new_node = true;
				ival = convert_from_double_to_typed_value(lval, This->get_datatype());
				result << new unattached_node_w_limits(
						ival, This->get_datatype(), itime,
						errorHigh,
						warnHigh,
						lval,
						errorLow);
			}
		    }
		    if(errorLowInstr) {
			if(check_for_crossing(
					current_value, errorLow,
					previous_value, old_node->get_error_low(),
					current_node->Key.getetime(), prev_node->Key.getetime(),
					itime, lval)) {
				discard_the_new_node = true;
				ival = convert_from_double_to_typed_value(lval, This->get_datatype());
				result << new unattached_node_w_limits(
						ival, This->get_datatype(), itime,
						errorHigh,
						warnHigh,
						warnLow,
						lval);
			}
		    }
		}
		if(last_iteration) {
		    if(discard_the_new_node) {
			delete new_node;
		    } else {
			result << new_node;
		    }
		    break;
		}

		result << new_node;

		previous_value = current_value;
		if(node_is_local) {
		    previous_consuming_node = current_node;
		}
		prev_node = current_node;
		old_node = new_node;
		if(last_iteration) {
		    break;
		}
		current_node = multi->next();
	}

	if(!last_iteration) {
		// we haven't explicitly taken care of the endpoint; time to do it now.
		if(current_node) {
		    	TypedValue		V;
			CTime_base		last_time(current_node->Key.getetime());
			LimitContainer		lc;

			// can throw:
			if(show_discontinuities) {
				EvalTiming::strategy_setter setter(This, EvalTiming::BEFORE);
				This->evaluate_present_value(
					V,
					last_time);
			} else {
				This->evaluate_present_value(
					V,
					last_time,
					&lc);
			}
			if(errorHighInstr || errorLowInstr || warningHighInstr || warningLowInstr) {
				result << new unattached_node_w_limits(V, This->get_datatype(), last_time,
						lc.error_high_val,
						lc.warn_high_val,
						lc.warn_low_val,
						lc.error_low_val);
			} else {
				result << new unattached_value_node(V, This->get_datatype(), last_time);
			}
		// must be careful here: History could be empty... (defensive programming)
		} else if(result.get_length()) {
			CTime_base		last_time;
			unattached_value_node*	uvn;
			LimitContainer		lc;

			uvn = result.last_node();
			if(	eval_intfc::there_is_a_last_event(last_time)
				&& last_time > uvn->Key.getetime()) {
				TypedValue		V;

				// can throw:
				This->evaluate_present_value(
					V,
					last_time,
					&lc);
				if(errorHighInstr || errorLowInstr || warningHighInstr || warningLowInstr) {
					result << new unattached_node_w_limits(V, This->get_datatype(), last_time,
							lc.error_high_val,
							lc.warn_high_val,
							lc.warn_low_val,
							lc.error_low_val);
				}
				else {
					result << new unattached_value_node(V, This->get_datatype(), last_time);
				}
			}
		}
	}
	multi->clear();
	returned_version = This->get_res_version();
}

TypedValue RES_consumable::get_partial_history(
		CTime_base			start_time,
		CTime_base			span) {
	tlist<alpha_time, unattached_value_node, Rsource*>	l;
	get_history().get_partial_numeric_history(
					start_time,
					span,
					l,
					false);
	TypedValue	retval;
	ListOVal*	ret = new ListOVal;
	ListOVal*	times = new ListOVal;
	ListOVal*	values = new ListOVal;
	unattached_value_node* vn;
	long int	i = 0;
	ArrayElement*	ae;

	for(vn = l.first_node(); vn; vn = vn->next_node()) {
		TypedValue T;
		T = vn->getKey().getetime();
		ae = new ArrayElement(i);
		ae->SetVal(T);
		(*times) << ae;
		ae = new ArrayElement(i);
		ae->SetVal(vn->get_value());
		(*values) << ae;
		i++;
	}
	TypedValue A;
	A = *times;
	ae = new ArrayElement(Cstring("times"));
	ae->SetVal(A);
	(*ret) << ae;
	A = *values;
	ae = new ArrayElement(Cstring("values"));
	ae->SetVal(A);
	(*ret) << ae;
	retval = *ret;
	return retval;
}

TypedValue RES_nonconsumable::get_partial_history(
		CTime_base			start_time,
		CTime_base			span) {
	tlist<alpha_time, unattached_value_node, Rsource*>	l;
	get_history().get_partial_numeric_history(
					start_time,
					span,
					l,
					false);
	TypedValue	retval;
	ListOVal*	ret = new ListOVal;
	ListOVal*	times = new ListOVal;
	ListOVal*	values = new ListOVal;
	unattached_value_node* vn;
	long int	i = 0;
	ArrayElement*	ae;

	for(vn = l.first_node(); vn; vn = vn->next_node()) {
		TypedValue T;
		T = vn->getKey().getetime();
		ae = new ArrayElement(i);
		ae->SetVal(T);
		(*times) << ae;
		ae = new ArrayElement(i);
		ae->SetVal(vn->get_value());
		(*values) << ae;
		i++;
	}
	TypedValue A;
	A = *times;
	ae = new ArrayElement(Cstring("times"));
	ae->SetVal(A);
	(*ret) << ae;
	A = *values;
	ae = new ArrayElement(Cstring("values"));
	ae->SetVal(A);
	(*ret) << ae;
	retval = *ret;
	return retval;
}

void RES_history::get_partial_numeric_history(
		const CTime_base&					start_time,
		const CTime_base&					span,
		tlist<alpha_time, unattached_value_node, Rsource*>&	result,
		bool							all_nodes_flag
		) {
	Rsource*					This = get_resource();
	multilist<prio_time, value_node, Rsource*>&	multiHist = historyList;
	simple_res_miter*				multi = get_the_multiterator();
	

	RES_numeric*	checkThis = dynamic_cast<RES_numeric*>(multiHist.Owner);

	assert(checkThis);

	TypedValue	evaluated_val, profile_value, current_value, previous_value;
	value_node*	current_node, *previous_consuming_node = NULL;

	//
	// NUMERIC RESOURCE CASE
	//

	double			old_value;
	TypedValue		ival;
	double			lval;
	CTime_base		itime(false);
	value_node*		prev_node = NULL;
	value_node*		peeked_at;
	unattached_value_node*	new_node;
	unattached_value_node*	old_node = NULL;

	multi->next();
	current_node = multi->next();
	if(!current_node) {
		multi->clear();
		return;
	}

	if(!all_nodes_flag) {
		if(current_node->Key.getetime() < start_time ) {
			while((peeked_at = multi->peek()) && (peeked_at->Key.getetime() < start_time)) {
				current_node = multi->next();
			}
		}
	}
	current_node = multi->next();
	if(!current_node) {
	    multi->clear();
	    return;
	}

	while(current_node) {

		//
		// The profile value may originate from our own history,
		// but it can also originate from the history of a
		// resource we depend on. This Boolean variable keeps track of
		// which kind we are looking at:
		//
		bool	node_is_local;

		if(current_node->Key.getetime() > start_time + span && (!all_nodes_flag)) {
		    break;
		}

		//
		// find the first local node with time = current time,
		// if possible. We determine locality by checking the
		// list that current_node belongs to.
		//
		while(!(node_is_local = (current_node->list == &multiHist))) {
		    peeked_at = multi->peek();
		    if(peeked_at && peeked_at->Key.getetime() == current_node->Key.getetime()) {
			current_node = multi->next();
		    } else {
			break;
		    }
		}

		try {
		    This->evaluate_profile(
					EvalTiming::NORMAL,
					current_node->Key.getetime(),
					profile_value);
		    profile_value.cast(This->get_datatype());

		} catch(eval_error Err) {
		    Cstring		e("get_vals_from_miterator error for resource ");

		    e << This->name << "; details:\n" << Err.msg;
		    throw(eval_error(e));
		}

		if(node_is_local) {
		    const TypedValue&	current_node_val = current_node->get_value();

		    switch(get_datatype()) {
			case apgen::DATA_TYPE::TIME:
			    current_value = profile_value.get_time_or_duration()
			    	- CTime_base::convert_from_double_use_with_caution(
							current_node_val.get_double(),
							true);
			    break;
			case apgen::DATA_TYPE::DURATION:
			    current_value = profile_value.get_time_or_duration()
			    	- CTime_base::convert_from_double_use_with_caution(
							current_node_val.get_double(),
							true);
			    break;
			case apgen::DATA_TYPE::FLOATING:
			    current_value = profile_value.get_double() - current_node_val.get_double();
			    break;
			case apgen::DATA_TYPE::INTEGER:
			    current_value = profile_value.get_int() - (long) current_node_val.get_double();
			    break;

			//
			// pacify the compiler - handle all other types, even though
			// they are not applicable to numeric resources
			//
			default:
			    break;
		    }

		} else {
		    if(previous_consuming_node) {
			const TypedValue&	tv = previous_consuming_node->get_value();

			switch(get_datatype()) {
			    case apgen::DATA_TYPE::FLOATING:
				current_value = profile_value.get_double() - tv.get_double();
				break;
			    case apgen::DATA_TYPE::TIME:
			    case apgen::DATA_TYPE::DURATION:
				current_value = profile_value.get_time_or_duration() - tv.get_time_or_duration();
				break;
			    case apgen::DATA_TYPE::INTEGER:
				current_value = profile_value.get_int() - (long) tv.get_double();
				break;

			    //
			    // pacify the compiler - handle all other types, even though
			    // they are not applicable to numeric resources
			    //
			    default:
				break;
			}

		    } else {

			current_value = profile_value;
		    }
		}

		new_node = new unattached_value_node(
					current_value,
					This->get_datatype(),
					current_node->Key.getetime());

		result << new_node;

		previous_value = current_value;
		if(node_is_local) {
		    previous_consuming_node = current_node;
		}
		prev_node = current_node;
		old_node = new_node;
		current_node = multi->next();
	}

	multi->clear();
}

TypedValue RES_settable::get_partial_history(
		CTime_base			start_time,
		CTime_base			span) {
	tlist<alpha_time, unattached_value_node, Rsource*>	l;
	get_history().get_partial_settable_history(
					start_time,
					span,
					l,
					false);
	TypedValue	retval;
	ListOVal*	ret = new ListOVal;
	ListOVal*	times = new ListOVal;
	ListOVal*	values = new ListOVal;
	unattached_value_node* vn;
	long int	i = 0;
	ArrayElement*	ae;

	for(vn = l.first_node(); vn; vn = vn->next_node()) {
		TypedValue T;
		T = vn->getKey().getetime();
		ae = new ArrayElement(i);
		ae->SetVal(T);
		(*times) << ae;
		ae = new ArrayElement(i);
		ae->SetVal(vn->get_value());
		(*values) << ae;
		i++;
	}
	TypedValue A;
	A = *times;
	ae = new ArrayElement(Cstring("times"));
	ae->SetVal(A);
	(*ret) << ae;
	A = *values;
	ae = new ArrayElement(Cstring("values"));
	ae->SetVal(A);
	(*ret) << ae;
	retval = *ret;
	return retval;
}

void RES_history::get_settable_values(
		const CTime_base&					start_time,
		const CTime_base&					span,
		tlist<alpha_time, unattached_value_node, Rsource*>&	result,
		bool							all_nodes_flag,
		long&							returned_version
		) {
    Rsource*		This = get_resource();
    multilist<prio_time, value_node, Rsource*>&	multiHist = historyList;

    //
    // NOTE: using a Multiterator for a settable resource is overkill,
    // because the profile of a settable resource must be trivial.
    // However we keep things this way for the time being since we cut and
    // pasted this code from RES_numeric.
    //
    simple_res_miter*	multi = get_the_multiterator();
	

    RES_settable*	checkThis = dynamic_cast<RES_settable*>(multiHist.Owner);

    assert(checkThis);

    TypedValue		evaluated_val, profile_value, current_value, previous_value;
    value_node*		current_node;
    bool		first_node_occurs_after_start;
    bool		show_discontinuities = true;
    bool		last_iteration = false;

    bool		maxSym_exists	= This->properties[(int)Rsource::Property::has_maximum];
    bool		minSym_exists	= This->properties[(int)Rsource::Property::has_minimum];

    map<Cstring, pEsys::Assignment*>::const_iterator	errorHighIter;
    map<Cstring, pEsys::Assignment*>::const_iterator	errorLowIter;
    map<Cstring, pEsys::Assignment*>::const_iterator	warningHighIter;
    map<Cstring, pEsys::Assignment*>::const_iterator	warningLowIter;
    pEsys::Assignment*	errorHighInstr = NULL;
    pEsys::Assignment*	errorLowInstr = NULL;
    pEsys::Assignment*	warningHighInstr = NULL;
    pEsys::Assignment*	warningLowInstr = NULL;
    double		old_value;
    TypedValue		ival;
    double		lval;
    CTime_base		itime(false);
    value_node*		prev_node = NULL;
    value_node*		peeked_at;
    unattached_value_node* new_node;
    unattached_value_node* old_node = NULL;
    double		errorHigh = 0.0, errorLow = 0.0, warnHigh = 0.0, warnLow = 0.0;
    
    //
    // Only show discontinuities if interpolation is set to false
    //
    show_discontinuities = !This->properties[(int)Rsource::Property::has_interpolation];

    const parsedProg&	attrProgPtr(This->get_attribute_program());

    if(attrProgPtr) {
	pEsys::Program&	theAttrProg = *attrProgPtr.object();
	errorHighIter = theAttrProg.symbols.find("errorhigh");
	if(errorHighIter != theAttrProg.symbols.end()) {
		errorHighInstr = errorHighIter->second;
	}
	errorLowIter = theAttrProg.symbols.find("errorlow");
	if(errorLowIter != theAttrProg.symbols.end()) {
		errorLowInstr = errorLowIter->second;
	}
	warningHighIter = theAttrProg.symbols.find("warninghigh");
	if(warningHighIter != theAttrProg.symbols.end()) {
		warningHighInstr = warningHighIter->second;
	}
	warningLowIter = theAttrProg.symbols.find("warninglow");
	if(warningLowIter != theAttrProg.symbols.end()) {
		warningLowInstr = warningLowIter->second;
	}
    }
    if(errorHighInstr || errorLowInstr || warningHighInstr || warningLowInstr) {
	if(maxSym_exists || minSym_exists) {
		Cstring errs = This->name;
		errs << ": error trying to get resource values - both Max/Min and "
			"Warning/Error attributes are used. Please use either "
			"Maximum/Minimum, or (Warning High/Low) / (Error High/Low).";
		throw(eval_error(errs));
	}
	if(show_discontinuities) {
		Cstring errs = This->name;
		errs << ": error trying to get resource values - Error/Warning limits are given, "
			"but the \"Interpolation\" attribute is not set or it is set to false. Please "
			"set the \"Interpolation\" attribute to true.";
		throw(eval_error(errs));
	}

    }

    multi->next();
    current_node = multi->next();
    if(!current_node) {
	first_node_occurs_after_start = false;
	multi->clear();
	return;
    }

    if(!all_nodes_flag) {
	if(current_node->Key.getetime() < start_time ) {
		while((peeked_at = multi->peek()) && (peeked_at->Key.getetime() < start_time)) {
			current_node = multi->next();
		}
	}
    }

    //
    // set the flag indicating whether the first endpoint is covered:
    //
    if(current_node->Key.getetime() > start_time) {
	first_node_occurs_after_start = true;
    } else {
	first_node_occurs_after_start = false;
    }

    while(current_node) {

	if(current_node->Key.getetime() > start_time + span && (!all_nodes_flag)) {
	    last_iteration = true;
	}

	//
	// We don't check whether the node belongs to us,
	// because the profile of a settable resource is
	// always trivial.
	//
	assert(current_node->list == &multiHist);

	//
	// if we need to show discontinuities,
	// duplicate the previous value
	//
	if(show_discontinuities) {
	    if(prev_node) {
		current_value = prev_node->get_value();
	    } else {
		try {
			This->evaluate_profile(
				EvalTiming::BEFORE,
				current_node->Key.getetime(),
				current_value);
		} catch(eval_error Err) {
			Cstring		e("get_vals_from_miterator error for resource ");

			e << This->name << "; details:\n" << Err.msg;
			throw(eval_error(e));
		}
	    }

	    result << new unattached_value_node(
				current_value,
				This->get_datatype(),
				current_node->Key.getetime());
	    if(last_iteration) {
		break;
	    }
	}

	//
	// Now let's switch our attention to the current node's value
	//
	current_value = current_node->get_value();


	//
	// Evaluate limits if appropriate
	//
	if(errorHighInstr || errorLowInstr || warningHighInstr || warningLowInstr) {
	    time_saver	ts;

	    ts.set_now_to(current_node->Key.getetime());
	    try {
		TypedValue V;
		if(errorHighInstr) {
		    errorHighInstr->Expression->eval_expression(This->Object.object(), V);
		    errorHigh = V.get_double();
		}
		if(warningHighInstr) {
		    warningHighInstr->Expression->eval_expression(This->Object.object(), V);
		    warnHigh = V.get_double();
		}
		if(warningLowInstr) {
		    warningLowInstr->Expression->eval_expression(This->Object.object(), V);
		    warnLow = V.get_double();
		}
		if(errorLowInstr) {
		    errorLowInstr->Expression->eval_expression(This->Object.object(), V);
		    errorLow = V.get_double();
		}
	    } catch(eval_error Err) {
		Cstring	error = This->name;
		error << ": error while evaluating error/warning limit - details:\n"
		    << Err.msg;
		throw(eval_error(error));
	    }
	    new_node = new unattached_node_w_limits(
				current_value,
				This->get_datatype(),
				current_node->Key.getetime(),
				errorHigh,
				warnHigh,
				warnLow,
				errorLow);
	} else {
	    new_node = new unattached_value_node(
				current_value,
				This->get_datatype(),
				current_node->Key.getetime());
	}

	bool discard_the_new_node = false;
	if(    !show_discontinuities && prev_node
		&& current_node->Key.getetime() > prev_node->Key.getetime()) {
	    if(maxSym_exists) {
		TypedValue	maxSym = (*This->Object)["maximum"];
		double	theMax = maxSym.get_double();
		if(previous_value.get_double() > theMax && current_value.get_double() <= theMax) {
			double		delta = previous_value.get_double() - current_value.get_double();
			CTime_base	deltaT = current_node->Key.getetime() - prev_node->Key.getetime();
			double		fraction = (previous_value.get_double() - theMax) / delta;
			CTime_base	itime = prev_node->Key.getetime() + (fraction * deltaT);

			ival = maxSym;
			discard_the_new_node = true;
			result << new unattached_value_node(ival, This->get_datatype(), itime);
		} else if(previous_value.get_double() <= theMax && current_value.get_double() > theMax) {
			double		delta = current_value.get_double() - previous_value.get_double();
			CTime_base	deltaT = current_node->Key.getetime() - prev_node->Key.getetime();
			double		fraction = (theMax - previous_value.get_double()) / delta;
			CTime_base	itime = prev_node->Key.getetime() + (fraction * deltaT);

			ival = maxSym;
			discard_the_new_node = true;
			result << new unattached_value_node(ival, This->get_datatype(), itime);
		}
	    }
	    if(minSym_exists) {
		TypedValue	minSym = (*This->Object)["minimum"];
		double	theMin = minSym.get_double();
		if(previous_value.get_double() >= theMin && current_value.get_double() < theMin) {
			double		delta = previous_value.get_double() - current_value.get_double();
			CTime_base	deltaT = current_node->Key.getetime() - prev_node->Key.getetime();
			double		fraction = (previous_value.get_double() - theMin) / delta;
			CTime_base	itime = prev_node->Key.getetime() + (fraction * deltaT);

			ival = minSym;
			discard_the_new_node = true;
			result << new unattached_value_node(ival, This->get_datatype(), itime);
		} else if(previous_value.get_double() < theMin && current_value.get_double() >= theMin) {
			double		delta = current_value.get_double() - previous_value.get_double();
			CTime_base	deltaT = current_node->Key.getetime() - prev_node->Key.getetime();
			double		fraction = (theMin - previous_value.get_double()) / delta;
			CTime_base	itime = prev_node->Key.getetime() + (fraction * deltaT);

			ival = minSym;
			discard_the_new_node = true;
			result << new unattached_value_node(ival, This->get_datatype(), itime);
		}
	    }

	    if(errorHighInstr) {
		if(check_for_crossing(
				current_value, errorHigh,
				previous_value, old_node->get_error_high(),
				current_node->Key.getetime(), prev_node->Key.getetime(),
				itime, lval)) {
		    discard_the_new_node = true;
		    ival = convert_from_double_to_typed_value(lval, This->get_datatype());
		    result << new unattached_node_w_limits(
					ival, This->get_datatype(), itime,
					lval,
					warnHigh,
					warnLow,
					errorLow);
		}
	    }
	    if(warningHighInstr) {
		if(check_for_crossing(
				current_value, warnHigh,
				previous_value, old_node->get_warn_high(),
				current_node->Key.getetime(), prev_node->Key.getetime(),
				itime, lval)) {
			discard_the_new_node = true;
			ival = convert_from_double_to_typed_value(lval, This->get_datatype());
			result << new unattached_node_w_limits(
					ival, This->get_datatype(), itime,
					errorHigh,
					lval,
					warnLow,
					errorLow);
		}
	    }
	    if(warningLowInstr) {
		if(check_for_crossing(
				current_value, warnLow,
				previous_value, old_node->get_warn_low(),
				current_node->Key.getetime(), prev_node->Key.getetime(),
				itime, lval)) {
			discard_the_new_node = true;
			ival = convert_from_double_to_typed_value(lval, This->get_datatype());
			result << new unattached_node_w_limits(
					ival, This->get_datatype(), itime,
					errorHigh,
					warnHigh,
					lval,
					errorLow);
		}
	    }
	    if(errorLowInstr) {
		if(check_for_crossing(
				current_value, errorLow,
				previous_value, old_node->get_error_low(),
				current_node->Key.getetime(), prev_node->Key.getetime(),
				itime, lval)) {
			discard_the_new_node = true;
			ival = convert_from_double_to_typed_value(lval, This->get_datatype());
			result << new unattached_node_w_limits(
					ival, This->get_datatype(), itime,
					errorHigh,
					warnHigh,
					warnLow,
					lval);
		}
	    }
	}
	if(last_iteration) {
	    if(discard_the_new_node) {
		delete new_node;
	    } else {
		result << new_node;
	    }
	    break;
	}

	result << new_node;

	previous_value = current_value;
	prev_node = current_node;
	old_node = new_node;
	if(last_iteration) {
	    break;
	}
	current_node = multi->next();
    }

    if(!last_iteration) {

	//
	// we haven't explicitly taken care of the endpoint;
	// time to do it now.
	//
	if(current_node) {
	    	TypedValue		V;
		CTime_base		last_time(current_node->Key.getetime());
		LimitContainer		lc;

		// can throw:
		if(show_discontinuities) {
			EvalTiming::strategy_setter setter(This, EvalTiming::BEFORE);
			This->evaluate_present_value(
				V,
				last_time);
		} else {
			This->evaluate_present_value(
				V,
				last_time,
				&lc);
		}
		if(errorHighInstr || errorLowInstr || warningHighInstr || warningLowInstr) {
			result << new unattached_node_w_limits(
					V,
					This->get_datatype(),
					last_time,
					lc.error_high_val,
					lc.warn_high_val,
					lc.warn_low_val,
					lc.error_low_val);
		} else {
			result << new unattached_value_node(
					V,
				       	This->get_datatype(),
				       	last_time);
		}

	//
	// must be careful here: History could be empty... (defensive programming)
	//
	} else if(result.get_length()) {
		CTime_base		last_time;
		unattached_value_node*	uvn;
		LimitContainer		lc;

		uvn = result.last_node();
		if(	eval_intfc::there_is_a_last_event(last_time)
			&& last_time > uvn->Key.getetime()) {
			TypedValue		V;

			// can throw:
			This->evaluate_present_value(
				V,
				last_time,
				&lc);
			if( errorHighInstr
			    || errorLowInstr
			    || warningHighInstr
			    || warningLowInstr) {
				result << new unattached_node_w_limits(
						V,
						This->get_datatype(),
						last_time,
						lc.error_high_val,
						lc.warn_high_val,
						lc.warn_low_val,
						lc.error_low_val);
			}
			else {
				result << new unattached_value_node(
						V,
						This->get_datatype(),
						last_time);
			}
		}
	}
    }
    multi->clear();
    returned_version = This->get_res_version();
}

void RES_history::get_partial_settable_history(
		const CTime_base&					start_time,
		const CTime_base&					span,
		tlist<alpha_time, unattached_value_node, Rsource*>&	result,
		bool							all_nodes_flag
		) {
    Rsource*		This = get_resource();
    multilist<prio_time, value_node, Rsource*>&	multiHist = historyList;

    //
    // NOTE: using a Multiterator for a settable resource is overkill,
    // because the profile of a settable resource must be trivial.
    // However we keep things this way for the time being since we cut and
    // pasted this code from RES_numeric.
    //
    simple_res_miter*	multi = get_the_multiterator();
	

    RES_settable*	checkThis = dynamic_cast<RES_settable*>(multiHist.Owner);

    assert(checkThis);

    TypedValue		evaluated_val, profile_value, current_value, previous_value;
    value_node*		current_node;

    double		old_value;
    TypedValue		ival;
    double		lval;
    CTime_base		itime(false);
    value_node*		prev_node = NULL;
    value_node*		peeked_at;
    unattached_value_node* new_node;
    unattached_value_node* old_node = NULL;
    
    multi->next();
    current_node = multi->next();
    if(!current_node) {
	multi->clear();
	return;
    }

    if(!all_nodes_flag) {
	if(current_node->Key.getetime() < start_time ) {
		while((peeked_at = multi->peek()) && (peeked_at->Key.getetime() < start_time)) {
			current_node = multi->next();
		}
	}
    }
    current_node = multi->next();
    if(!current_node) {
	multi->clear();
	return;
    }

    while(current_node) {

	if(current_node->Key.getetime() > start_time + span && (!all_nodes_flag)) {
	    break;
	}

	//
	// We don't check whether the node belongs to us,
	// because the profile of a settable resource is
	// always trivial.
	//
	assert(current_node->list == &multiHist);

	//
	// Now let's switch our attention to the current node's value
	//
	current_value = current_node->get_value();


	new_node = new unattached_value_node(
				current_value,
				This->get_datatype(),
				current_node->Key.getetime());

	result << new_node;

	previous_value = current_value;
	prev_node = current_node;
	old_node = new_node;
	current_node = multi->next();
    }

    multi->clear();
}


void
RES_history::Add(
		value_node*		vn,
		apgen::DATA_TYPE	dt) {
	Rsource* res = historyList.Owner;
	rsource_ext*	theExtension = res->derivedResource;

	if(theExtension) {
		theExtension->notify_clients(NULL);
	}

	if(res->isFrozen()) {
		Cstring rmessage;
		rmessage << "history.Add(): Resource " << res->name << " is frozen.";
		throw(eval_error(rmessage));
	}

	try {
		historyList << vn;
	}
	catch(eval_error Err) {
		cout << "RES_history: error in Add():\n"
			<< Err.msg << "\n";
	}

#ifdef TOO_RESTRICTIVE
	if(res->properties[(int)Rsource::Property::has_interpolation]) {
		value_node* prev = vn->previous_node();
		if(prev && prev->getKey().getetime() == vn->getKey().getetime()) {
			Cstring err;
			err << res->name << ": this resource is interpolated; "
				<< "it cannot be used or set more than once "
				<< "at time "
				<< vn->getKey().getetime().to_string()
				<< ".";
			throw(eval_error(err));
		}
	}
#endif /* TOO_RESTRICTIVE */

	theVersion().increment();
}

void RES_history::report_history() {
	nodeClass*						V;
	typename slist<timeKey, nodeClass, Rsource*>::iterator	hi(historyList);

	while((V = hi())) {
		CTime_base	t(V->Key.getetime());
		long		id = V->Key.event_id;
		aoString	CO;

		value_node*	trueV = dynamic_cast<value_node*>(V);
		if(trueV) {
			cerr << "    " << t.to_string()
				<< " (prio = " << id << "): depth = " << trueV->get_usage_depth();
			switch(trueV->get_res_val_type()) {
				case apgen::RES_VAL_TYPE::START_USING :
					cerr << ", case START";
					{
					cerr << ", other at prio " << trueV->get_other()->Key.event_id;
					}
					break;
				case apgen::RES_VAL_TYPE::END_USING :
					cerr << ", case END";
					{
					cerr << ", other at prio " << trueV->get_other()->Key.event_id;
					}
					break;
				case apgen::RES_VAL_TYPE::SET :
					cerr << ", case SET";
					break;
				case apgen::RES_VAL_TYPE::RESET :
					cerr << ", case RESET";
					break;
				case apgen::RES_VAL_TYPE::DON_T_CARE :
					cerr << ", case DON'T CARE";
					break;
			}
			cerr << ", val = ";
			// THIS IS INCORRECT; we should really evaluate the formula at this point.
			TypedValue x = trueV->get_value();
			x.print(CO, 0, MAX_SIGNIFICANT_DIGITS);
			cerr << CO.str() << endl;
		}
	}
}

RES_history::RES_history(Rsource* r)
		: change_count(0),
			resource_parent(r),
			historyList(r, true),
			Frozen(false),
			Imported(false),
			the_multiterator(*r->name) {}
RES_history::RES_history(const RES_history& RH)
		: change_count(0),
			resource_parent(RH.resource_parent),
			historyList(RH.historyList),
			Imported(RH.Imported),
			Frozen(RH.Frozen),
			the_multiterator(RH.the_multiterator) {}

value_node* RES_history::latestHistNodeAtOrBeforeModelTime(
					const CTime_base& event_time,
					Rslist::iterator* iter_at_model_time) {

    if(!iter_at_model_time) {
		Cstring errs;
		errs << "Resource " << historyList.Owner->name
			<< " does not have a pointer to its trigger.";
		throw(eval_error(errs));
    } 

    value_node* M;
    value_node* N;

    value_node* safe = iter_at_model_time->getPrev();

    if(!safe) {
	return latestHistNodeAtOrBefore(
	    event_time);
    }

// #define THIS_IS_THE_DEBUG_VERSION

    if(thread_index == thread_intfc::MODEL_THREAD) {

#	ifdef THIS_IS_THE_DEBUG_VERSION

	static long int total_iter_steps = 0;
	static long int total_safe_steps = 0;
	static long int debug_count = 0;


	//
	// There is no reason to "go safe" if
	// this is the modeling thread.
	//
	N = iter_at_model_time->iterNode;
	if(!N) {
		N = iter_at_model_time->L->last_node();
	}
	assert(N);

	bool iter_is_late = N->getKey().getetime() >= event_time;
	bool please_print = (++debug_count % 100000) == 0;
	if(please_print && iter_is_late) {
	    cerr << "latest: iter LATE  at " << N->getKey().getetime().to_string()
		<< ", event time " << event_time.to_string()
		<< " for " << N->list->Owner->name << "\n";
	} else if(please_print) {
	    cerr << "latest: iter EARLY at " << N->getKey().getetime().to_string()
		<< ", event time " << event_time.to_string()
		<< " for " << N->list->Owner->name << "\n";
	}

	int safe_steps = 0;

	//
	// Walk forward from the safe node
	//
	M = safe->next_node();
	while(M && M->getKey().getetime() <= event_time) {
		safe = M;
		safe_steps++;
		M = safe->next_node();
	}

	bool they_agree = N == safe;
	int iter_steps = 0;

	//
	// Try matching nodes; try both directions!
	//
	if(N->getKey().getetime() >= safe->getKey().getetime()) {

	    //
	    // Walk backward from the forward-looking node
	    //
	    M = N->previous_node();
	    int count = 0;
	    value_node* P = N;
	    while(M && M->getKey().getetime() >= safe->getKey().getetime()) {
		P = M;
		count++;
		if(safe == P) {
			they_agree = true;
			iter_steps = count;
			break;
		}
		M = P->previous_node();
	    }
	}
	if(N->getKey().getetime() <= safe->getKey().getetime()) {

	    //
	    // Walk forward from the early node
	    //
	    M = N->next_node();
	    int count = 0;
	    value_node* P = N;
	    while(M && M->getKey().getetime() <= event_time) {
		P = M;
		count++;
		if(safe == P) {
			they_agree = true;
			iter_steps = count;
			break;
		}
		M = P->next_node();
	    }
	}

	total_iter_steps += iter_steps;
	total_safe_steps += safe_steps;
	if(please_print) {
	    cerr << "\titer_steps " << iter_steps << " (total " << total_iter_steps
		<< ") - safe_steps " << safe_steps << " (total " << total_safe_steps << ")\n";
	}
	if(!they_agree) {
	    cerr << "\tDISAGREE - safe at " << safe->getKey().getetime().to_string()
		<< ", iter at " << N->getKey().getetime().to_string() << "\n";
	}
	return safe;

#	else /* PRODUCTION_VERSION_BELOW */

	//
	// There is no reason to "go safe" if
	// this is the modeling thread.
	//
	N = iter_at_model_time->iterNode;
	if(!N) {
		N = iter_at_model_time->L->last_node();
	}
	assert(N);

	//
	// Walk forward if the node is early
	//
	M = N;
	while(M && M->getKey().getetime() <= event_time) {
	    N = M;
	    M = N->next_node();
	}

	//
	// Walk backward if the node is late. Note that if
	// we went through the previous loop at least once,
	// this loop will be skipped.
	//
	M = N;
	while(M && M->getKey().getetime() > event_time) {
	    if((M = N->previous_node())) {
		N = M;
	    }
	}

	return N;

#	endif /* PRODUCTION_VERSION_ABOVE */

    } else {

	//
	// getPrev() returns the safe node,
	// which may be significantly behind
	// the event loop.
	//
	N = iter_at_model_time->getPrev();
    }
    if(!N) {
	return latestHistNodeAtOrBefore(
	    event_time);
    }

    M = N->next_node();
    while(M && M->getKey().getetime() <= event_time) {
	N = M;
	M = N->next_node();
    }

    return N;
}

value_node* RES_history::latestHistNodeBeforeModelTime(
					const CTime_base& event_time,
					Rslist::iterator* iter_at_model_time) {
	if(!iter_at_model_time) {
		Cstring errs;
		errs << "Resource " << historyList.Owner->name
			<< " does not have a pointer to its trigger.";
		throw(eval_error(errs));
	} 

	value_node* M;
	value_node* N;

	if(thread_index == thread_intfc::MODEL_THREAD) {

		//
		// There is no reason to "go safe" if
		// this is the modeling thread.
		//
		N = iter_at_model_time->iterNode;
	} else {
		N = iter_at_model_time->getPrev();
	}

	if(!N) {
		return latestHistNodeAtOrBefore(
			event_time);
	}
	M = N;
	while(M->getKey().getetime() >= event_time) {
		M = M->previous_node();
		if(!M) {
			return NULL;
		}
	}
	while(M && M->getKey().getetime() < event_time) {
		N = M;
		M = N->next_node();
	}
	return N;
}

void RES_history::addToIterator(
				Miterator_base<Rslist, value_node>&	iter,
				int					prio,
				bool					store) {
	iter.add_thread(historyList, historyList.Owner->name, prio, store);
}


EvalTiming::strategy_setter::strategy_setter(
					Rsource* res,
					Strategy strat)
    : my_resource(res), new_strategy(strat) {
    old_strategy = my_resource->get_strategy();
    if(new_strategy != old_strategy) {
	    my_resource->set_strategy(new_strategy);
    }
}

EvalTiming::strategy_setter::~strategy_setter() {
    if(new_strategy != old_strategy) {
	    my_resource->set_strategy(old_strategy);
    }
}

const char* EvalTiming::spell_strategy(EvalTiming::Strategy s) {
	const char* t;
	switch(s) {
		case NORMAL:
			t = "NORMAL";
			break;
		case BEFORE:
			t = "JUST_BEFORE";
			break;
		case INTERPOLATED:
			t = "INTERPOLATED";
			break;
		default:
			t = "BIZARRE";
			break;
	}
	return t;
}
