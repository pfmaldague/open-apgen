#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

using namespace std;

#include "apDEBUG.H"
#include <time.h>

#include "ACT_exec.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "APmodel.H"
#include "EventImpl.H"
#include "EventLoop.H"
#include "instruction_node.H"
#include "RES_def.H"
#include "Scheduler.H"

#include <assert.h>

extern mEvent*		CurrentEvent;

// #define DEBUG_THIS

bool window_finder::is_disabled = false;

value_node* window_finder::initializer(
		const slist<prio_time, value_node, Rsource*>&	sL,
		const CTime_base&				T
		) {
	const tlist<prio_time, value_node, Rsource*>* L
		= dynamic_cast<const tlist<prio_time, value_node, Rsource*>* >(&sL);
	if(!L) {
		Cstring err;
		err << "window_finder::initializer: expected history "
			"to be a tlist, but it's an slist";
		throw(eval_error(err));
	}

	//
	// follow the logic of tlist::find_at_or_before
	//
	CTime_base	f;
	value_node* current_node = L->root;
	value_node* last_earlier_node = NULL;
	value_node* tempnode;

	if(!current_node) return NULL;
	do {
		if (T < (f = current_node->Key.getetime())) {
			current_node = current_node->get_left();
		} else if(T == f) {
			while((tempnode = current_node->get_right())
				&& T == tempnode->Key.getetime()) {
				current_node = tempnode;
			}
			return current_node;
		} else {
			last_earlier_node = current_node;
			current_node = current_node->get_right();
		}
	} while(current_node);
	return last_earlier_node;
}

void CleanupSchedulingStructures() {
	slist<alpha_void, Cntnr<alpha_void, RCsource*> >::iterator RES(
			EventLoop::containersToWatchForWaitUntil());

	//
	// The following list is a tlist of Cntnr<alpha_void, RCsource*>;
	// no special action required.
	//
	EventLoop::containersToWatchForWaitUntil().clear();

	//
	// The following three lists contain pointers to
	// instruction_nodes which contain execStacks. Deleting
	// a stack does not completely get rid of execution
	// contexts because of internal smart pointers. You have to first
	// unwind the stack, i. e., pop all contexts out of it, before you can
	// delete the stack itself.
	//
	// This behavior is necessary because most of the time you want to
	// delete a stack but you do not want to delete the contexts it
	// references, since those contexts were passed on to other events in
	// the queue(s).
	//

	Cntnr<alpha_int, instruction_node*>* c;

	slist<alpha_int, Cntnr<alpha_int, instruction_node*> >::iterator
		iter_waiting1(eval_intfc::actsOrResWaitingOnSignals());
	while((c = iter_waiting1())) {
		c->payload->unwind();
		delete c->payload;
	}
	eval_intfc::actsOrResWaitingOnSignals().clear();

	slist<alpha_int, Cntnr<alpha_int, instruction_node*> >::iterator
		iter_waiting2(eval_intfc::actsOrResWaitingOnCond());
	while((c = iter_waiting2())) {
		c->payload->unwind();
		delete c->payload;
	}
	eval_intfc::actsOrResWaitingOnCond().clear();

	slist<alpha_int, instruction_node>::iterator iter4(where_to_go());
	instruction_node* in;
	while((in = iter4())) {
		in->unwind();
	}
	where_to_go().clear();
}

static void evaluate_expression(
		behaving_element&	syms,
		parsedExp&		exp,
		TypedValue&		val) {
	try {
		exp->eval_expression(syms.object(), val);
	} catch (eval_error ERR) {
		Cstring errors;
		errors << "evaluation error in expression ";
		exp->print(errors);
		errors << ":\n" << ERR.msg;
		throw(eval_error(errors));
	}
}


bool window_finder::evaluate_criterion( bool debug /* = false */ ) {

	//
	// Do this so that Rsource::evaluate_present_value()
	// knows that it's OK to use our Miterator to search
	// for the current history node:
	//  

#	ifdef DEBUG_THIS
	cerr << "window finder: updating safe time to "
		<< time_saver::get_now().to_string() << "\n";
#	endif /* DEBUG_THIS */

	thread_intfc::update_current_time(time_saver::get_now());

	TypedValue val;
	if(debug) {
		Rsource::evaluation_debug = true;
		evaluate_expression(Symbols, criterion_exp, val);
		bool ret = val.get_int() != 0;
		Rsource::evaluation_debug = false;
		return ret;
	}
	evaluate_expression(Symbols, criterion_exp, val);
	return val.get_int() != 0;
}

void	window_finder::get_search_parameters(
		TypedValue*&		windows,
		TypedValue*&		safe_windows,
		TypedValue*&		actual_duration,
		const CTime_base&	max_dur,
		bool&			max_dur_is_specified,
		behaving_element&	ambient,
		Cstring&		name_of_actual_dur_symbol) {
	Cstring	errors;

	windows = NULL;
	safe_windows = NULL;
	actual_duration = NULL;

	bool has_windows = ambient->defines_property("windows");
	bool has_safe_windows = ambient->defines_property("safe_windows");
	bool has_actual_duration = ambient->defines_property(name_of_actual_dur_symbol);

	if(has_windows) {
		windows = &(*ambient)["windows"];
	}
	if(has_safe_windows) {
		safe_windows = &(*ambient)["safe_windows"];
	}
	if(has_actual_duration) {
		actual_duration = &(*ambient)[name_of_actual_dur_symbol];
	}
	if(windows) {

#		ifdef DEBUG_THIS
		cerr << "window_finder::get_search_parameters: windows array was found\n";
#		endif /* DEBUG_THIS */

		if(!windows->is_array()) {
			errors << "get_search_params: \"windows\" symbol is "
				<< "defined but does not seem to be an array.";
			throw(eval_error(errors));
		}
		if(!max_dur_is_specified) {
			errors << "get_search_params: \"windows\" symbol is "
				<< "defined but a maximum duration is not specified.";
			throw(eval_error(errors));
		}
		windows->get_array().clear();
	}
	if(safe_windows) {

#		ifdef DEBUG_THIS
		cerr << "window_finder::get_search_parameters: safe_windows array was found\n";
#		endif /* DEBUG_THIS */

		if(!safe_windows->is_array()) {
			errors << "get_search_params: \"safe_windows\" symbol is "
				<< "defined but does not seem to be an array.";
			throw(eval_error(errors));
		}
		if(!max_dur_is_specified) {
			errors << "get_search_params: \"safe_windows\" symbol "
				<< "is defined but a maximum duration is not specified.";
			throw(eval_error(errors));
		}
		safe_windows->get_array().clear();
	}
}

bool window_finder::compute_windows() {
	Cstring			errors;
	value_node*		trigger;
	CTime_base		start_time_of_current_window, etime;
	CTime_base		latest_criterion_true_time;
				// Need to know this when evaluating windows of opportunity:
	bool			theCriterionIsTrueNow = false;
	bool			min_requirement_is_met = false;
	bool			max_requirement_is_exceeded = false;
	bool			we_ran_out_of_events = false;
	bool			there_is_a_max_time_limit = false;
	time_saver		makeSureNowIsRestored;
	CTime_base		actual_duration;

	start_time_of_search_window = time_saver::get_now();

	// 1. EVALUATE THE REQUESTED DURATION OVER WHICH WE SHOULD TRY SCHEDULING SOMETHING

	try {

		//
		// new scheme (Nov. 2013): we evaluate the requested duration only once
		//
		// Bug (April 2020): duration should be compared to duration, not time
		// if (max_duration > CTime_base(0,0,false))
		//
		// if (max_duration > CTime_base(0,0,true)) {
		//
		// Bug (AP-1499) the max duration may be specified as 0:0:0
		if (max_duration_is_specified) {
			there_is_a_max_time_limit = true;
			max_time_limit = start_time_of_search_window + max_duration;
		}
	} catch (eval_error ERR) {
		errors << "compute_windows: evaluation error in duration expression;\n"
				<< ERR.msg;
		throw(eval_error(errors));
	}

	// 1.5 if the criterion is not satisfied, find out whether there is a window


#	ifdef DEBUG_THIS
	cerr << "window_finder::compute_windows()\n\tcriterion satisfied at start = "
		<< criterion_is_satisfied_at_start << "\n";
	cerr << "\tcriterion: " << criterion_exp->to_string() << "\n";
#	endif /* DEBUG_THIS */


	if (criterion_is_satisfied_at_start) {
		start_time_of_current_window = time_saver::get_now();
		theCriterionIsTrueNow = true;
	} else {


		CTime_base prev_etime = start_time_of_search_window;

#		ifdef DEBUG_THIS
		int count = 0;
		cerr << "            Looking for start of window...\n";
#		endif /* DEBUG_THIS */

		while ((trigger = theTriggers.next())) {

		    etime = trigger->getKey().getetime();

		    //
		    // make sure you don't evaluate more often than you need to:
		    //
		    if (etime > prev_etime) {
			if(there_is_a_max_time_limit && etime > max_time_limit) {
				break;
			}

#			ifdef DEBUG_THIS
			if(++count < 4) {
			    cerr << "                looking at "
				<< etime.to_string() << "...";
			}
#			endif /* DEBUG_THIS */


			time_saver::set_now_to(etime);
			theCriterionIsTrueNow = evaluate_criterion();
			if (theCriterionIsTrueNow) {
				start_time_of_current_window = etime;

#				ifdef DEBUG_THIS
				if(count < 4) {
					cerr << " found a potential start!\n";
				} else {
					cerr << "            found a potential start at "
						<< etime.to_string() << " after "
						<< count << " iteration(s)\n";
				}
#				endif /* DEBUG_THIS */

				break;
			}
#			ifdef DEBUG_THIS
			else if(count < 4) {
				cerr << " no luck.\n";
			}
#			endif /* DEBUG_THIS */

			prev_etime = etime;
		    }
		}
		if (!theCriterionIsTrueNow) {

#		    ifdef DEBUG_THIS
		    cerr << "          no windows found. Returning.\n";
#		    endif /* DEBUG_THIS */

		    //
		    // no windows found
		    //
		    return false;
		}
	}

	assert(theCriterionIsTrueNow);
	latest_criterion_true_time = start_time_of_current_window;

	//
	// do this once the criterion is satisfied
	//
	min_time_limit = start_time_of_current_window + requested_duration;

	//
	// 2. ITERATE TO SEE WHETHER THE DURATION CRITERION CAN BE MET
	//
	while ((trigger = theTriggers.next())) {
	    etime = trigger->getKey().getetime();

	    //
	    // we don't look for windows that start prior to the start
	    // time associated with the scheduling or window-getting instruction:
	    //
	    if(etime > start_time_of_current_window) {

		//
		// 3. EVALUATE THE CRITERION
		//

		time_saver::set_now_to(etime);
		theCriterionIsTrueNow = evaluate_criterion();
		min_requirement_is_met = (
					    theSafeWindows != NULL
					    && latest_criterion_true_time >= min_time_limit
					 ) || (
					    theSafeWindows == NULL
					    && etime >= min_time_limit
					 );

		// 4. CHECK WHETHER THE CRITERION IS TRUE AND WHETHER
		//    WE ARE STILL WITHIN THE time_limit

		if (theCriterionIsTrueNow) {
			latest_criterion_true_time = etime;
		} else {
			break;
		}
		if (min_requirement_is_met) {
			break;
		} else if(there_is_a_max_time_limit && etime > max_time_limit) {
			max_requirement_is_exceeded = true;
			break;
		}
	    }

	    //
	    // save time on the next iteration
	    //
	    while ((trigger = theTriggers.peek())
			&& trigger->getKey().getetime() == etime) {
		theTriggers.next();
		assert(trigger != theTriggers.peek());
	    }
	}

	we_ran_out_of_events = trigger == NULL;

	/* Possibilities:
	 *
	 * 	- the criterion is false and !min_requirement_is_met
	 * 		if waiting, we failed; else, we should continue to look
	 * 	- the criterion is false and min_requirement_is_met
	 * 		we succeeded and we have the end of a window
	 * 	- the criterion is true and !min_requirement_is_met
	 * 		we ran out of events. Should we schedule something
	 * 		here? Probably yes... or declare an error, which
	 * 		I think is better. However, apgen 8 considers this
	 * 		case a success; so we will do the same.
	 * 	- the criterion is true and min_requirement_is_met
	 * 		we succeeded and may need to find out the end of
	 * 		the window
	 */

	// catch the interesting error condition first
	if (we_ran_out_of_events) {
		return true;
	}

	// next, easy case: we failed
	if (max_requirement_is_exceeded) {
		return false;
	}

	/* At this point we may or may not have passed the duration test.
	 * Let's figure out the actual duration of the first window... */
	Cstring errs;

	/* Next step: set actual duration to length of first window */
	if(min_requirement_is_met) {
	    if(theSafeWindows) {
		theDurationToRememberForTheActualDuration =
			latest_criterion_true_time - start_time_of_current_window;
	    } else {
		theDurationToRememberForTheActualDuration =
			etime - start_time_of_current_window;
	    }
	    if (!theCriterionIsTrueNow) {
		if (we_need_to_check_windows) {
		    assert(!twiceTheNumberOfWindows);
		    if (theWindows) {
			theWindows->get_array().add(twiceTheNumberOfWindows, start_time_of_current_window);
		    }
		    if (theSafeWindows) {
			theSafeWindows->get_array().add(twiceTheNumberOfWindows, start_time_of_current_window);
		    }
		    twiceTheNumberOfWindows++;
		    if (theWindows) {
			theWindows->get_array().add(twiceTheNumberOfWindows, etime);
		    }
		    if (theSafeWindows) {
			theSafeWindows->get_array().add(twiceTheNumberOfWindows, latest_criterion_true_time);
		    }
		    twiceTheNumberOfWindows++;
		}
	    }
	} else if(we_need_to_wait) {
	    return false;
	}

	if (	we_need_to_check_windows
		||
		(theCriterionIsTrueNow && theActualDuration)
	   ) {

	    //
	    // let's continue with our window investigation
	    //
	    while ((trigger = theTriggers.next())) {
		etime = trigger->getKey().getetime();
		if (there_is_a_max_time_limit && etime > max_time_limit) {

		    //
		    // make sure the last window does not spill beyond the specified duration
		    //
		    etime = max_time_limit;
		}

		time_saver::set_now_to(etime);
		bool crit_satisfied = evaluate_criterion();
		if (!crit_satisfied) {
		    min_requirement_is_met =
			(theSafeWindows != NULL
			    && latest_criterion_true_time - start_time_of_current_window
				>= requested_duration)
			|| (theSafeWindows == NULL
			    && etime - start_time_of_current_window
				>= requested_duration);
		    if (theCriterionIsTrueNow) {
			theCriterionIsTrueNow = false;
			if (!twiceTheNumberOfWindows) {

			    //
			    // the criterion has never been false
			    //
			    theDurationToRememberForTheActualDuration =
				etime - start_time_of_current_window;
			}

			//
			// At Steve Wissler's request:
			//
			if (min_requirement_is_met) {

			    //
			    // The second condition is consistent with the
			    // semantics of "finding a window": a window
			    // should be an open set, in a mathematical
			    // sense.  Therefore, a window of zero length
			    // does not make sense since its interior is empty.
			    //
			    if (we_need_to_check_windows
				&& etime > start_time_of_current_window) {
				if (theWindows) {
				    theWindows->get_array().add(twiceTheNumberOfWindows,
							start_time_of_current_window);
				    theWindows->get_array().add(twiceTheNumberOfWindows + 1,
							etime);
				}

				//
				// In principle we should have a separate
				// test for safe windows, but that would
				// make the two array dimensions different.
				// So, it could be that safe windows will
				// only contain one window of zero duration.
				//
				if (theSafeWindows) {
				    theSafeWindows->get_array().add(twiceTheNumberOfWindows,
							start_time_of_current_window);
				    theSafeWindows->get_array().add(twiceTheNumberOfWindows + 1,
							latest_criterion_true_time);
				}
				twiceTheNumberOfWindows += 2;
			    }
			}
		    } else {
			;
		    }
		    if (!we_need_to_check_windows) {
			break;
		    }
		} else {
		    // criterion is satisfied
		    latest_criterion_true_time = etime;
		    if (!theCriterionIsTrueNow) {
			theCriterionIsTrueNow = true;
			start_time_of_current_window = etime;
		    }
		    min_requirement_is_met =
			(theSafeWindows != NULL
			    && latest_criterion_true_time - start_time_of_current_window
				>= requested_duration)
			|| (theSafeWindows == NULL
			    && etime - start_time_of_current_window
				>= requested_duration);
		}
		if (there_is_a_max_time_limit && etime >= max_time_limit) {
		    break;
		}

		while ((trigger = theTriggers.peek())
				&& trigger->getKey().getetime() == etime) {
		    theTriggers.next();
		}
	    }
	    if (theCriterionIsTrueNow) {

		//
		// note: it could be that the criterion was always satisfied,
		// this would be our first window
		//
		if (min_requirement_is_met) {

		    //
		    // add a window
		    //
		    if (!twiceTheNumberOfWindows) {
			// the criterion has never been false
			if(theSafeWindows) {
				theDurationToRememberForTheActualDuration =
					latest_criterion_true_time
					- start_time_of_current_window;
			} else {
				theDurationToRememberForTheActualDuration =
					etime
					- start_time_of_current_window;
			}
		    }

		    //
		    // The second condition is consistent with the
		    // semantics of "finding a window": a window
		    // should be an open set, in a mathematical
		    // sense.  Therefore, a window of zero length
		    // does not make sense since its interior is empty.
		    //
		    if (    we_need_to_check_windows
			    && etime > start_time_of_current_window) {


			if (theWindows) {
				theWindows->get_array().add(twiceTheNumberOfWindows,
						start_time_of_current_window);
				theWindows->get_array().add(twiceTheNumberOfWindows + 1,
						etime);
			}
			if (theSafeWindows) {
				theSafeWindows->get_array().add(twiceTheNumberOfWindows,
						start_time_of_current_window);
				theSafeWindows->get_array().add(twiceTheNumberOfWindows + 1,
						etime);
			}
		    }
		}
	    }
	}
	if (theActualDuration) {
	    *theActualDuration = theDurationToRememberForTheActualDuration;
	}

	return true;
}

void window_finder::initialize_triggers_at_start() {
	Rsource*		resource;
	Rsource::iterator	iter;
	int			priority = 1;
	int			max_priority = 1;

	priority = 1;
	max_priority = 1;
	theTriggers.clear();

	//
	// NOTE: for the time being we cannot trigger scheduling based
	// on associative resources
	//
	while((resource = iter.next())) {
		RES_state*		state_res = dynamic_cast<RES_state*>(resource);
		RES_settable*		settable_res = dynamic_cast<RES_settable*>(resource);
		RES_numeric*		numeric_res = dynamic_cast<RES_numeric*>(resource);

		if(state_res) {
			state_res->get_history().addToIterator(
				theTriggers,
				priority + resource->evaluation_priority,
				/* store = */ true);
		} else if(settable_res) {
			settable_res->get_history().addToIterator(
				theTriggers,
				priority + resource->evaluation_priority,
				/* store = */ true);
		} else if(numeric_res) {
			numeric_res->get_history().addToIterator(
				theTriggers,
				priority + resource->evaluation_priority,
				/* store = */ true);
		}
		if(priority + resource->evaluation_priority > max_priority) {
			max_priority = priority + resource->evaluation_priority;
		}
	}

	//
	// Make sure the iterator is ready to use
	//
	theTriggers.first();
}

void pEsys::GetWindows::get_windows_of_opportunity(
		pEsys::execStack*	stack_to_use) {
	execution_context*	context = stack_to_use->get_context();
	behaving_element&	ambient = context->AmbientObject;

	instruction_node*	i_n;
	time_saver		theSaver;
	CTime_base		saved_current_time = thread_intfc::current_time(thread_index);
	parsedExp&		criterion = actual_arguments[0];

	CTime_base		max_dur(true);
	CTime_base		min_dur(true);
	bool			max_dur_is_specified = false;
	const char*		error_context;

	try {

	    //
	    // We use Array::eval_array and process the array explicitly.
	    //
	    TypedValue	the_options;

	    error_context = "evaluating options";
	    Options->eval_array(ambient.object(), the_options);
	    ListOVal&	options_list = the_options.get_array();
	    ArrayElement*	ae;
	    Cstring		actual_symbol_name;
	    CTime_base		desired_start;

	    if((ae = options_list.find("start"))) {
		desired_start = ae->Val().get_time_or_duration();
	    } else {
		desired_start = (*context->AmbientObject->level[1])[
				ActivityInstance::START].get_time_or_duration();
	    }
	    if(model_control::get_pass() != model_control::INACTIVE) {
		if(desired_start < thread_intfc::current_time(thread_index)) {
		    Cstring errs;
		    errs << "get_windows, file " << file << ", line " << line
			<< ": \"start\" argument (= " << desired_start.to_string()
			<< ") is earlier than modeling loop's 'now' (= "
			<< thread_intfc::current_time(thread_index).to_string()
		       	<< ")";
		    throw(eval_error(errs));
		}
	    }
	    time_saver::set_now_to(desired_start);

	    if((ae = options_list.find("actual"))) {

		//
		// This option is a little tricky because we don't
		// want the current value of the 'actual' symbol; we
		// need to know its name so we can store the actual
		// window duration there.
		//
		actual_symbol_name = ActualSymbol->getData();
	    }
	    if((ae = options_list.find("min"))) {
		min_dur = ae->Val().get_time_or_duration();
	    }
	    if((ae = options_list.find("max"))) {
		max_dur = ae->Val().get_time_or_duration();
		max_dur_is_specified = true;
	    }

	    //
	    // Evaluate the scheduling condition
	    //
	    error_context = "evaluating criterion";
	    TypedValue CriterionValue;
	    SchedulingCondition->eval_expression(ambient.object(), CriterionValue);

	    //
	    // NOTE: the caller has set now to the right value -
	    // so, we can use that inside compute_windows()
	    //
	    bool criterion_is_satisfied = CriterionValue.get_int() > 0;

	    //
	    // no waiting - this is get_windows
	    //
	    bool need_to_wait = false;


	    TypedValue*	windows;
	    TypedValue*	safe_windows;
	    TypedValue*	actual_win_dur;


	    //
	    // We don't need to do this!
	    // 
	    // EventLoop::theEventLoop().potential_triggers.clear();
	    // EventLoop::init_triggers();

	    error_context = "evaluating search parameters";
	    window_finder::get_search_parameters(
				windows,
				safe_windows,
				actual_win_dur,
				max_dur,
				max_dur_is_specified,
				ambient,
				actual_symbol_name);

	    error_context = "conducting the search";

	    smart_ptr<window_finder> DC;
	    if(EventLoop::CurrentEvent) {

		//
		// We are in the middle of modeling; it's OK to use
		// the event loop's triggers for initializing ours,
		// since we are only interested in events that are in
		// the future with respect to the event loop. We use
		// the window_finder constructor that was designed for
		// precisely that purpose.
		//
		DC.reference(new window_finder(
			EventLoop::theEventLoop().potential_triggers,
			SchedulingCondition,
			min_dur,
			max_dur,
			max_dur_is_specified,
			criterion_is_satisfied,
			need_to_wait,
			windows,
			safe_windows,
			actual_win_dur,
			ambient));
	    } else {

		//
		// We are not currently modeling; we will have to use
		// the resource profile computed during the last
		// modeling run. We need to use a constructor that
		// initializes our event iterator at the beginning
		// of the resource histories.
		//
		DC.reference(new window_finder(
			SchedulingCondition,
			min_dur,
			max_dur,
			max_dur_is_specified,
			criterion_is_satisfied,
			need_to_wait,
			windows,
			safe_windows,
			actual_win_dur,
			ambient));
	    }

	    //
	    // In all cases, thread_intfc::current_time(thread_index) has been set
	    // by the window_finder constructor to the initial time
	    // of the Miterator used to scan events. compute_windows()
	    // will continue to update thread_intfc::current_time(thread_index) so
	    // that Rsource::evaluate_present_value() will know to
	    // use the Miterator to get the current node, rather than
	    // conducting a brand new search for each new time point.
	    //
	    if (DC->compute_windows()) {

		//
		// success
		//

		//
		// Reset safe time to the value it had
		// before the call to get_windows:
		//

		//
		// debug
		//
#	ifdef DEBUG_THIS
#	endif /* DEBUG_THIS */

		thread_intfc::update_current_time(saved_current_time);
		return;
	    }
	} catch (eval_error Err) {
		Cstring errors;
		errors << "get_windows, file " << file << ", line " << line
			<< ": error " << error_context << ":\n"
			<< Err.msg << "\n";
		throw(eval_error(errors));
	}

	//
	// Reset safe time to the value it had
	// before the call to get_windows:
	//

	//
	// debug
	//
#	ifdef DEBUG_THIS
#	endif /* DEBUG_THIS */

	thread_intfc::update_current_time(saved_current_time);
}

void pEsys::GetInterpwins::get_windows_of_opportunity(
		pEsys::execStack*	stack_to_use) {
	Cstring errors;
	errors << "interpolated_windows: not implemented; use get_windows()\n"
		<< "with a criterion based on <resource>.interpval() instead.\n";
	throw(eval_error(errors));
}
