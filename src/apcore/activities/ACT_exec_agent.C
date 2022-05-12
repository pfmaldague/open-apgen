#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <apDEBUG.H>
#include <aafReader.H>
#include <ACT_exec.H>
#include "ActivityInstance.H"
#include <apcoreWaiter.H>
#include <APmodel.H>
#include <APrequest_handler.H>
#include <EventRegistry.H>
#include <fileReader.H>
#include <IO_ApgenData.H>
#include <IO_ApgenDataOptions.H>
#include <RES_exec.H>
#include <UTL_time.H>
#include <memory>


#ifdef USE_AUTO_PTR
	#define AUTO_PTR_TEMPL auto_ptr
#else
	#define AUTO_PTR_TEMPL unique_ptr
#endif

// extern Cstring normalizeString(const Cstring &given_string, char c);

extern "C" {
#include "concat_util.h"
extern void (*activityInstanceSerializationHandle)(void *, buf_struct *);
extern void (*activityDefsSerializationHandle)(buf_struct *);
} // extern "C"

IO_TypedValue* APval2IOval(
		const TypedValue& val,
		Cstring &any_errors,
		int arrayAppend = 0) {
	switch(val.get_type()) {
		case apgen::DATA_TYPE::TIME:
			{
			return new IO_TimeExact(val.get_time_or_duration());
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			{
			return new IO_Duration(val.get_time_or_duration());
			}
			break;
		case apgen::DATA_TYPE::BOOL_TYPE:
			return new IO_TypedValBool(val);
		case apgen::DATA_TYPE::INTEGER:
			return new IO_TypedValInt(val.get_int());
			break;
		case apgen::DATA_TYPE::FLOATING:
			return new IO_TypedValDouble(val.get_double());
			break;
		case apgen::DATA_TYPE::STRING:
			return new IO_TypedValString(*val.get_string());
			break;
		case apgen::DATA_TYPE::ARRAY:
			{
			if(arrayAppend)
				return new IO_TypedValueArrayAppend(val);
			else
				return new IO_TypedValueArraySet(val);
			}
			break;
		case apgen::DATA_TYPE::INSTANCE:
			{
			// let's not bother. SASFs will never use APGEN instances.
			return new IO_TypedValInstance("(NULL)");
			}
			break;
		case apgen::DATA_TYPE::UNINITIALIZED:
			return new IO_TypedValUnInitialized();
		default:
			return NULL;
	}

	return NULL;
}

IO_Time* GetStartTime(ActivityInstance *act) {
	//get the start time
	CTime	ctime_start = act->getetime();
	CTime_base evStart(ctime_start);

	return new IO_TimeExact(evStart);
}

IO_TypedValue*	GetTypedValue(
		ActivityInstance *act,
		const Cstring &name,
		const TypedValue &val) {
	IO_TypedValue*	retVal = NULL;
	Cstring		Errs;

	//handle special cases first
	if (name == "\"Start\"") {
		retVal = GetStartTime(act);
	} else if (name == "\"Duration\"") {
		CTime_base span = act->get_timespan();

		CTime_base evSpan(span);
		retVal = new IO_Duration(evSpan);
	} else if (name == "\"SASF\"") {
		//the 1 means make any arrays append arrays
		retVal = APval2IOval(val, Errs, 1);
	} else {
		retVal = APval2IOval(val, Errs);
	}

	if(!retVal) {
		cerr << "GetTypedValue: error = " << Errs;
		return NULL;
	}

	if (name.is_defined()) {
		retVal->SetName(string(*name));
	}

	return retVal;
}

void exec_agent::clear_the_discrepancy_list() {
	tlist<alpha_void, Cntnr<alpha_void, CTime_base> >::iterator
		suspects(act_instances_with_discrepant_durations);
	Cntnr<alpha_void, CTime_base>*	ptp;

	// get rid of any previously detected discrepant instances...
	act_instances_with_discrepant_durations.clear();
}

void exec_agent::handle_typeless_activities() {
}

void exec_agent::dump() {
}

void exec_agent::purge() {
	clear_the_discrepancy_list();
}

void exec_agent::instantiate_all(
		tlist<alpha_string, instance_tag>& list_of_instance_nodes) {
	ActivityInstance*						act;
	apgen::RETURN_STATUS						ret = apgen::RETURN_STATUS::SUCCESS;
	instance_tag*							ptr;
	slist<alpha_string, instance_tag>::iterator			inst_list(list_of_instance_nodes);
	status_aware_iterator						brand_new_list(eval_intfc::get_act_lists().get_brand_new_iterator());
	smart_actptr*							dptrnode;
	tlist<alpha_void, Cntnr<alpha_void, CTime_base> >		temporary_discrepancy_list;
	slist<alpha_void, Cntnr<alpha_void, CTime_base> >::iterator	suspects(temporary_discrepancy_list);
	Cntnr<alpha_void, CTime_base>*					ptp;
	stringstream							errs;

	//
	// Instantiation is not a trivial problem for two main reasons:
	//
	// 1. an APF may contain partial hierarchies which need to be
	//    completed for activities with non-exclusive children
	//
	// 2. the decomposition section of an activity for which we
	//    need to create children may need the creation section
	//    to be executed first. However, the spirit of the creation
	//    section is that it should only be exercised if the
	//    activity is "active", i. e., linked to a visible activity
	//    through an uninterrupted chain of nonexclusive_decomposition
	//    relationships.
	//
	//    Does this mean that we only create children for activities
	//    that are active? This question cannot be resolved without
	//    consulting stakeholders, i. e., adapters.
	//

	while((ptr = inst_list())) {
	    act = ptr->payload.act;

	    if(act->agent().is_brand_new()) {
		try {
		    instance_state_changer	ISC(act);

		    ISC.set_desired_visibility_to(ptr->payload.invisibility);

		    //
		    // AP-604 fix: Instead of invoking
		    // instance_state_changer::do_it(),
		    // a new method only applicable to
		    // brand new activities is invoked.
		    // This new method never exercises
		    // creation sections and never exercises
		    // decomposition sections; it merely
		    // inserts the activities just read from
		    // the APF into the list that they
		    // belong to.
		    //
		    // A second stage below will invoke
		    // create() and also create missing
		    // hierarchies for activity instances
		    // whose type features a non-exclusive
		    // decomposition section.
		    //
		    ISC.instantiate_new_act();
		} catch(eval_error Err) {
		    Cstring errs;
		    errs << "Instantiate_all() ERROR:\n" << Err.msg << "\n";
		    throw(eval_error(errs));
		}
	    }
	}

	if(!mySource->recompute_durations) {

		//
		// restore the old value
		//
		CTime_base old_time;

		while((ptp = suspects())) {
			act = (ActivityInstance *) ptp->getKey().Num;
			old_time = ptp->payload;
			ptp->payload = act->get_timespan();
			act->obj()->set_timespan(old_time);
		}
	}

	act_instances_with_discrepant_durations << temporary_discrepancy_list;

	aoString aos;
	if(APcloptions::theCmdLineOptions().debug_execute) {
		aos << "instantiate all: documenting all instances\n";
	}
	if(ret == apgen::RETURN_STATUS::SUCCESS) {
	    while((ptr = inst_list())) {
		act = ptr->payload.act;
		bool active = act->agent().is_weakly_active();
		apgen::METHOD_TYPE mt;
		bool has_nonexcl_section = act->has_decomp_method(mt)
			&& mt == apgen::METHOD_TYPE::NONEXCLDECOMP;

		//
		// BUG 4/28/2008: the following line was missing
		//
		if(act->is_unscheduled()) {
			continue;
		}

		//
		// NOTE: if the activity is not weakly active and
		// has a nonexclusive decomposition section and
		// has no children, then we have a problem because
		// the decomposition section may rely on the creation
		// section having been executed, but the creation
		// section only gets executed when an instance
		// becomes weakly active.
		//
		if( !active
		    && has_nonexcl_section
		    && !act->hierarchy().children_count()) {
			Cstring errs;
			errs << "activity " << act->get_unique_id()
			    << " has a nonexclusive decomp. section "
			    << "which should be exercised, but it is "
			    << "not in the visible/active part of the "
			    << "activity hierarchy and therefore its "
			    << "creation section (if any) cannot be "
			    << "invoked. Clean up the APF, e. g. by making "
			    << "the activity visible or turning it into a child "
			    << "of a visible activity with nonexclusive "
			    << "children.";

			throw(errs);

		} else if(act->agent().is_weakly_active()
		   && !act->hierarchy().children_count()
		   && (!act->is_unscheduled())
		  ) {
			try {
	    		    instance_state_changer	ISC(act);
			    ISC.create_missing_nonexcl_children();
			}
			catch(eval_error Err) {
				errs << *Err.msg << "\n";
				ret = apgen::RETURN_STATUS::FAIL;
			}
		}
		if(APcloptions::theCmdLineOptions().debug_execute) {
			act->Object->to_stream(&aos, 2);
		}
	    }
	}
	if(APcloptions::theCmdLineOptions().debug_execute) {
		cerr << aos.str();
	}

	if(eval_intfc::get_act_lists().get_brand_new_length()) {
		errs << "\nBrand new list still has nodes in it, declaring FAILURE.\n";
		ret = apgen::RETURN_STATUS::FAIL;
	}
	while((act = (ActivityInstance *) brand_new_list())) {
		slist<alpha_void, smart_actptr>::iterator
				dobjlst(act->get_down_iterator());

		errs << "activity \"" << *act->theConstActAgent()->get_owner_name()
			<< "\", id \"" << *act->get_unique_id()
			<< "\" is referenced but never defined; "
			<< "will be deleted from plan file. #\n";
		while((dptrnode = dobjlst())) {
			act = dptrnode->BP;
			instance_state_changer	ISC(act);

			ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
			try{
				ISC.do_it(NULL);
			} catch(eval_error Err) {
				// ret is already set to FAIL...
				errs << *Err.msg;
			} catch(decomp_error Err) {
				// ret is already set to FAIL...
				errs << *Err.msg;
			}
		}
	}

	eval_intfc::get_act_lists().clear_brand_new_list();
	// It is safe to do this now:
	list_of_instance_nodes.clear();
	if(ret != apgen::RETURN_STATUS::SUCCESS) {
		Cstring err(errs.str().c_str());
		throw(eval_error(err));
	}
}

bool exec_agent::WriteActivitiesToStream(
		aoString&			fout,
		const stringslist&		listOfFiles,
		const CTime_base&		startTime,
		const CTime_base&		endTime,
		slist<alpha_void, dumb_actptr>&	bad_activities, // contains any instance(s) with illegal status
		long				top_level_chunk
		) {
	tlist<alpha_void, dumb_actptr>		Ups;
	tlist<alpha_void, dumb_actptr>		Downs;
	tlist<timeptr<ActivityInstance>, smart_actref, ActivityInstance*> Ordered_downs(true);
	slist<alpha_void, dumb_actptr>::iterator ups(Ups);
	slist<alpha_void, dumb_actptr>::iterator downs(Downs);
	slist<timeptr<ActivityInstance>, smart_actref, ActivityInstance*>::iterator ordered_downs(Ordered_downs);
	dumb_actptr*				b;
	smart_actref*				t;
	bool					wrote_something = false;
	ActivityInstance*			act_request;
	int					dum_prio = 0;
	aoString				hierarchy_section;
	long					childrenNode = 0, childNode = 0;

	// Aaah! Ooooh! Pierre is using the STL! :-)

	AUTO_PTR_TEMPL<status_aware_multiterator>	the_instances(eval_intfc::get_instance_multiterator(dum_prio, true, true));

	while((act_request = the_instances->next())) {
		if(	listOfFiles.find(act_request->get_APFplanname())
			&& (act_request->getetime() >= startTime)
			&& (act_request->getetime() < endTime)
			) {
			ActivityInstance* up = act_request->hierarchy().get_to_the_top();

			//
			// At some point we'll have to investigate the following message:
			//

			// if(!listOfFiles.find(up->get_APFplanname())) {
			// 	cerr << "Write acts to stream: PROBLEM with " << up->get_unique_id()
			// 		<< ", its plan = " << up->get_APFplanname()
			// 		<< " which is not in the list even though its child "
			// 		<< act_request->get_unique_id() << " has plan "
			// 		<< act_request->get_APFplanname() << "\n";
			// }

			if(up && !Ups.find((void *) up)) {
				Ups << new dumb_actptr(up);
			}
		}
	}

	while((b = ups())) {
		ActivityInstance*u = b->payload;

		u->hierarchy().recursively_get_ptrs_to_all_descendants(Downs);
		Downs << new dumb_actptr(u);
	}

	// We now have a set of activities that is closed under "belongs to the same hierarchy as"
	while((b = downs())) {
		ActivityInstance* d = b->payload;

		Ordered_downs << new smart_actref(d);
	}
	CTime_base	prevTime;
	long		itemElement = 0;
	while((t = ordered_downs())) {
		const char			*C, *D;
		Cstring				theName;
		apgen::act_visibility_state	V = apgen::act_visibility_state::VISIBILITY_REGULAR;
		static bool			first = true;
		long				parentNode = 0;

		act_request = t->getKey().mC;
		if(first) {
			first = false;
			prevTime = act_request->getetime();
		}
		if(act_request->const_agent().is_decomposed()) {
			V = apgen::act_visibility_state::VISIBILITY_DECOMPOSED;
		} else if(act_request->const_agent().is_abstracted()) {
			V = apgen::act_visibility_state::VISIBILITY_ABSTRACTED;
		} else if(!act_request->const_agent().is_active()) {
			bad_activities << new dumb_actptr(act_request);
		}
		if(!wrote_something) {
			wrote_something = true;
			fout << "\n";
		}
		act_request->theConstActAgent()->transfer_to_stream(
				fout,
				apgen::FileType::FT_APF,
				/* bounds = */ NULL);
	}

	if(bad_activities.get_length()) {
		return false;
	}
	return true;
}

void exec_agent::SetActivityInstance(
		ActivityInstance*			act,
		IO_ActivityInstance*			instance,
		const string&				context,
		const apgen::act_visibility_state	viz
		) {
	assert(act);
	assert(instance);
	instance->SetName(*(*act->Object)[ActivityInstance::NAME].get_string());
	instance->SetType(*(*act->Object)[ActivityInstance::TYPE].get_string());
	instance->SetID(*(*act->Object)[ActivityInstance::ID].get_string());
	instance->SetIsRequest(false);
	instance->SetIsChameleon(false);

	const task& constructor = act->Object->Task;

	//
	// Define all attributes
	//
	for(int i = 0; i < constructor.get_varinfo().size(); i++) {
		Cstring name = constructor.get_varinfo()[i].first;
		map<Cstring, int>::const_iterator iter = constructor.get_varindex().find(name);
		assert(iter != constructor.get_varindex().end());
		int index = iter->second;
		TypedValue& val = (*act->Object)[index];

		//
		// Note: the original SASF interface worked with
		// "official" attribute names. Here, we switch to
		// the "nickname". Update code in IO_SASFWrite.C
		// accordingly.
		//
		if(val.get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
			if(val.get_type() == apgen::DATA_TYPE::ARRAY) {
				if(val.get_array().get_length() > 0) {
					instance->AddAttribute(*name, val);
				}
			} else {
				instance->AddAttribute(*name, val);
			}
		}
	}

	//
	// Define all parameters
	//
	for(int i = 0; i < constructor.paramindex.size(); i++) {
		int index_of_param = constructor.paramindex[i];
		Cstring param_name = constructor.get_varinfo()[index_of_param].first;
		TypedValue& val = (*act->Object)[index_of_param];
		instance->AddParameter(*param_name, val);
	}

	//
	// Set parent and children
	//
	slist<alpha_void, smart_actptr>::iterator
				child_iter(act->get_down_iterator());

	smart_actptr*		ptr_to_child;
	ActivityInstance*	parent_act = act->get_parent();

	if(parent_act) {
		instance->SetParentID(*parent_act->get_unique_id());
	}
	while((ptr_to_child = child_iter())) {
		instance->AddChildID(*ptr_to_child->BP->get_unique_id());
	}
}

void model_intfc::doUnschedule(
		int		all_flag,
		time_saver&	ts) {
	apgen::RETURN_STATUS	ret = apgen::RETURN_STATUS::SUCCESS;
	ActivityInstance*	activityInstance;
	ActivityInstance*	a_parent;
	slist<alpha_void, dumb_actptr> CopyOfSelectionList;
	slist<alpha_void, dumb_actptr>::iterator copy_of_list(CopyOfSelectionList);
	smart_actptr*		ptr;
	dumb_actptr*		dumb;
	slist<alpha_void, smart_actptr>	selection_list;
	slist<alpha_void, smart_actptr>::iterator acts(selection_list);
	stringslist		list_of_parents_to_reschedule;
	stringslist::iterator	parents(list_of_parents_to_reschedule);
	stringstream		errs;
	int			task_index;
	behaving_element	parent_obj;
	parsedProg		P;

	eval_intfc::get_all_selections(selection_list);

	if(all_flag) {
		status_aware_iterator	acts1(eval_intfc::get_act_lists().get_scheduled_active_iterator());
		status_aware_iterator	acts2(eval_intfc::get_act_lists().get_decomposed_iterator());
		status_aware_iterator	acts3(eval_intfc::get_act_lists().get_abstracted_iterator());


		// UNSELECT
		while((ptr = acts())) {
			CopyOfSelectionList << new dumb_actptr(ptr->BP);
		}
		while((dumb = copy_of_list())) {
			dumb->payload->unselect();
		}

		// SELECT
		while((activityInstance = (ActivityInstance *) acts1())) {
			if(	activityInstance->get_parent()
				&& (parent_obj = activityInstance->get_parent()->Object)
				&& (task_index = parent_obj->type_has_task_named("decompose") >= 0)
				&& (P = parent_obj->Task.Type.tasks[task_index]->prog)
				&& P->section_of_origin() == apgen::METHOD_TYPE::CONCUREXP
			  ) {
				activityInstance->select();
			}
		}
		while((activityInstance = (ActivityInstance *) acts2())) {
			if(	activityInstance->get_parent()
				&& (parent_obj = activityInstance->get_parent()->Object)
				&& (task_index = parent_obj->type_has_task_named("decompose") >= 0)
				&& (P = parent_obj->Task.Type.tasks[task_index]->prog)
				&& P->section_of_origin() == apgen::METHOD_TYPE::CONCUREXP
			  ) {
				activityInstance->select();
			}
		}
		while((activityInstance = (ActivityInstance *) acts3())) {
			if(	activityInstance->get_parent()
				&& (parent_obj = activityInstance->get_parent()->Object)
				&& (task_index = parent_obj->type_has_task_named("decompose") >= 0)
				&& (P = parent_obj->Task.Type.tasks[task_index]->prog)
				&& P->section_of_origin() == apgen::METHOD_TYPE::CONCUREXP
			  ) {
				activityInstance->select();
			}
		}
	}

	//
	// Don't forget to do this... (I did, of course)
	//
	CopyOfSelectionList.clear();

	//
	// ditto...
	//
	eval_intfc::get_all_selections(selection_list);

	//
	// do this because clear_hierarchy changes
	// the contents of the selection list:
	//
	while((ret == apgen::RETURN_STATUS::SUCCESS) && (ptr = acts())) {
		activityInstance = ptr->BP;
		if(	(a_parent = activityInstance->get_parent())
			&& (parent_obj = activityInstance->get_parent()->Object)
			&& (task_index = parent_obj->type_has_task_named("decompose") >= 0)
			&& (P = parent_obj->Task.Type.tasks[task_index]->prog)
			&& P->section_of_origin() == apgen::METHOD_TYPE::CONCUREXP
		  ) {
			CopyOfSelectionList << new dumb_actptr(ptr->BP);
			if(!list_of_parents_to_reschedule.find(a_parent->get_unique_id())) {
				list_of_parents_to_reschedule << new emptySymbol(a_parent->get_unique_id());
			}
		}
	}

	while((ret == apgen::RETURN_STATUS::SUCCESS) && (dumb = copy_of_list())) {
		activityInstance = dumb->payload;
		if(activityInstance->agent().is_active()) {
			try {
				instance_state_changer		ISC(activityInstance);

				ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_ABSTRACTED);
				ISC.do_it(NULL);
			} catch(eval_error Err) {
				ret = apgen::RETURN_STATUS::FAIL;
				errs << "do_unschedule() got error(s):\n" << *Err.msg;
			} catch(decomp_error Err) {
				ret = apgen::RETURN_STATUS::FAIL;
				errs << "do_unschedule() got error(s):\n" << *Err.msg;
			}
		}
	}

	emptySymbol*					es;
	Cntnr<alpha_string, ActivityInstance*>*	ad;
	apgen::METHOD_TYPE				mt;

	while((ret == apgen::RETURN_STATUS::SUCCESS) && (es = parents())) {
	    if((ad = aaf_intfc::actIDs().find(es->get_key()))) {
		activityInstance = ad->payload;

		//
		// Let's be careful here - check that the activity has
		// the right type of decomposition method
		//
		if(	activityInstance->has_decomp_method(mt)
			&& mt == apgen::METHOD_TYPE::CONCUREXP
		  ) {
		    instance_state_changer	ISC(activityInstance);

		    ISC.set_desired_offspring_to(false);
		    ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
		    try {
			ISC.do_it(NULL);
		    } catch(eval_error Err) {
			ret = apgen::RETURN_STATUS::FAIL;
			errs << "do_unschedule() was unable to delete descendants."
				<< " Details:\n" << *Err.msg;
		    } catch(decomp_error Err) {
			ret = apgen::RETURN_STATUS::FAIL;
			errs << "do_unschedule() was unable to delete descendants."
				<< " Details:\n" << *Err.msg;
		    }
		}
	    }
	}

	if(ret != apgen::RETURN_STATUS::SUCCESS) {
		Cstring err(errs.str().c_str());
		throw(eval_error(err));
	}
}

LegendObject*	process_a_legend_height_pair(
			const Cstring&	given_name,
			int		theLegendHeight) {
	LegendObject	*the_legend;
	Cstring		theLegendName(given_name);

	// theLegendName = normalizeString(theLegendName , '_');

	/*
	 *	NOTE: QUOTES ARE NOW STRIPPED RIGHT HERE, AT THE SOURCE...
	 *
	 *	Also note: the constructor handles insertion into the master list of legends, as
	 *	well as removing any old legend with the same name and substituting this one instead.
	 */

	if(!(the_legend = (LegendObject *) Dsource::theLegends().find(theLegendName))) {
		the_legend = LegendObject::LegendObjectFactory(
					theLegendName,
					aafReader::current_file(),
					theLegendHeight);
	} else if(theLegendHeight >= ACTSQUISHED && theLegendHeight < 2000) {
		if(theLegendHeight < (ACTSQUISHED + ACTFLATTENED) / 2) {
			theLegendHeight = ACTFLATTENED;
		}
		if(theLegendHeight < (ACTVERUNIT + ACTSQUISHED) / 2) {
			theLegendHeight = ACTSQUISHED;
		} else if(theLegendHeight <= ACTVERUNIT) {
			theLegendHeight = ACTVERUNIT;
		}
		the_legend->adjust_height(theLegendHeight);
	}
	return the_legend;
}

void exec_agent::executeNewDirectives(
		slist<alpha_string, bsymbolnode>&	theTempList,
		int&					layout_directive_count) {
    slist<alpha_string, bsymbolnode>::iterator	dirs(theTempList);
    bsymbolnode*				theDirective;
    CTime_base					ADstart, ADduration;
    bool					ADstart_was_defined = false;
    bool					ADduration_was_defined = false;
    int						applicationHeight, applicationWidth;
    LegendObject*				the_legend;
    Cstring					theFile;
    int						theLine;

    try {
	while((theDirective = dirs())) {
	    Cstring	lhs(theDirective->get_key());

	    		//
			// NOTE: the Expression has been evaluated while parsing (in grammar_intfc.C)
			//
	    parsedExp	Expression(theDirective->payload);
	    TypedValue	val;

	    theFile = Expression->file;
	    theLine = Expression->line;
	    if(lhs == "Activity Display Start") {
		Expression->eval_expression(behaving_object::GlobalObject(), val);
		ADstart = val.get_time_or_duration();
		if(ADduration_was_defined) {
		    if(req_intfc::ACT_DISPLAY_HORIZONhandler) {
			req_intfc::ACT_DISPLAY_HORIZONhandler(ADstart, ADduration);
		    }
		    ADstart_was_defined = false;
		    ADduration_was_defined = false;
		} else {
			ADstart_was_defined = true;
		}
	    } else if(lhs == "Activity Display Duration") {
		Expression->eval_expression(behaving_object::GlobalObject(), val);
		ADduration = val.get_time_or_duration();
		if(ADstart_was_defined) {
		    if(req_intfc::ACT_DISPLAY_HORIZONhandler) {
			req_intfc::ACT_DISPLAY_HORIZONhandler(ADstart, ADduration);
		    }
		    ADstart_was_defined = false;
		    ADduration_was_defined = false;
		} else {
		    ADduration_was_defined = true;
		}
	    } else if(lhs == "Application Size") {
		Expression->eval_expression(behaving_object::GlobalObject(), val);
		sscanf (*val.get_string() , "%d x %d", &applicationWidth , &applicationHeight);
		if(req_intfc::DISPLAY_SIZEhandler) {
		    req_intfc::DISPLAY_SIZEhandler(applicationWidth, applicationHeight);
		}
	    } else if(lhs == "Register") {
		Expression->eval_expression(behaving_object::GlobalObject(), val);
		if(val.get_type() == apgen::DATA_TYPE::ARRAY) {
		    Cstring type = val.get_array()[0L]->Val().get_string();
		    Cstring name = val.get_array()[1L]->Val().get_string();
		    Cstring funcName = val.get_array()[2L]->Val().get_string();

		    UserFunctionEventClientManager::Manager().Create(*type, *name, *funcName);
		}
	    } else if(lhs == "Format") {
		Cstring any_errors;
		any_errors << "Format directive is no longer used. Please remove it.\n";
		throw(eval_error(any_errors));
	    } else if(lhs == "Legend") {
		Cstring		theLegendName;
		int		theLegendHeight = ACTVERUNIT;

		Expression->eval_expression(behaving_object::GlobalObject(), val);
		if(val.get_type() == apgen::DATA_TYPE::ARRAY) {
			theLegendName = val.get_array()[0L]->Val().get_string();
			theLegendHeight = val.get_array()[1L]->Val().get_int();
		}
		else {
			theLegendHeight = ACTVERUNIT;
			theLegendName = val.get_string();
		}

		//
		// Note: errors are not possible here - but we should check that. Example: theLegendName = ""
		//
		process_a_legend_height_pair(theLegendName, theLegendHeight);
	    } else if(lhs == "Activity Legend Layout") {
		Cstring		theLegendName;
		int		theLegendHeight = ACTVERUNIT;

		Expression->eval_expression(behaving_object::GlobalObject(), val);
		if(val.get_type() == apgen::DATA_TYPE::ARRAY
				&& val.get_array().get_length() > 0) {
			TypedValue		i_(apgen::DATA_TYPE::INTEGER), j_(apgen::DATA_TYPE::INTEGER);
			TypedValue		theTwoElementArray(apgen::DATA_TYPE::ARRAY);
			TypedValue		LayoutID(apgen::DATA_TYPE::STRING);
			Cstring			id;
			long			imax = val.get_array().get_length(), i;
			long			imin = 0L;
			List			theLegendsInTheRequestedOrder;

			// First level: get the individual legends
			if(val.get_array()[0L]->Val().get_type() == apgen::DATA_TYPE::STRING) {
				LayoutID = val.get_array()[0L]->Val();
				id = LayoutID.get_string();
				imin = 1L;
				// imax is still good: was set to total array size
			}
			for(i = imin; i < imax; i++) {
				theTwoElementArray = val.get_array()[(long)i]->Val();
				theLegendName = theTwoElementArray.get_array()[0L]->Val().get_string();
				theLegendHeight = theTwoElementArray.get_array()[1L]->Val().get_int();

				//
				// Note: errors are not possible here - but we
				// should check that. Example: theLegendName = ""
				//
				the_legend = process_a_legend_height_pair(theLegendName, theLegendHeight);
				theLegendsInTheRequestedOrder << new Pointer_node(the_legend, NULL);
			}
			ACT_exec::reorder_legends_as_per(id, theLegendsInTheRequestedOrder);
		} else {
			Cstring any_errors;
			any_errors = "Errors while processing activity legend layout directive: "
				"can't find array and/or its elements.\n";
			throw(eval_error(any_errors));
		}
		layout_directive_count++;
	    } else if(lhs == "Resource Legend Layout") {
		Cstring		theLegendName;
		long		theLegendHeight = RESVERUNIT;
		TypedValue	val;

		Expression->eval_expression(behaving_object::GlobalObject(), val);
		if(val.get_type() == apgen::DATA_TYPE::ARRAY
		   && val.get_array().get_length() > 0) {
			TypedValue		i_(apgen::DATA_TYPE::INTEGER), j_(apgen::DATA_TYPE::INTEGER);
			TypedValue		theTwoElementArray(apgen::DATA_TYPE::ARRAY);
			TypedValue		LayoutID(apgen::DATA_TYPE::STRING);
			Cstring			id;
			long			imax = val.get_array().get_length();
			long			i;
			List			theLegendsInTheRequestedOrder;
			vector<Cstring>		res_names;
			vector<int>		heights;

			//
			// get the individual resources
			//
			i_ = 0L;
			if(val.get_array()[0L]->Val().get_type() == apgen::DATA_TYPE::STRING) {
				LayoutID = val.get_array()[0L]->Val();
				id = LayoutID.get_string();

				for(i = 1L; i < imax; i++) {
				    theTwoElementArray = val.get_array()[(long)i]->Val();
				    theLegendName = theTwoElementArray.get_array()[0L]->Val().get_string();
				    theLegendHeight = theTwoElementArray.get_array()[1L]->Val().get_int();
				    // theLegendsInTheRequestedOrder << new Tag(theLegendName , (void *) theLegendHeight);
				    res_names.push_back(theLegendName);
				    heights.push_back(theLegendHeight);
				}
				if(req_intfc::RES_LAYOUThandler) {

				    //
				    // throws if error:
				    //
				    req_intfc::RES_LAYOUThandler(
						id,
						res_names,
						heights,
						theFile,
						theLine);
				}
			} else {
			    Cstring any_errors = "Errors while processing resource legend "
				"layout directive: missing layout ID.\n";
			    throw(eval_error(any_errors));
			}
		} else {
			Cstring errs;
			errs << "Errors while processing resource legend layout directive: "
				<< "can't find array and/or its elements.\n";
			throw(eval_error(errs));
		}
		layout_directive_count++;
	    // end of lhs == "Resource Legend Layout"
	    } else if(lhs == "Versions") {
			// we expect a list of versions; each version should have a label
			// and a description. Actually, we should provide for author name, too.
	    } // end of lhs == "Versions"
	} // while (theDirective)
    } catch(eval_error Err) {
	Cstring errs;
	errs << "File " << theFile << ", line " << theLine << ": Error in directive.\n";
	errs << Err.msg;
	throw(eval_error(errs));
    }

    // Stick into unused repository:
    while((theDirective = theTempList.first_node())) {
    	ACT_exec::Directives() << theDirective;
    }
}

void exec_agent::WriteDirectivesToStream(aoString &fout, IO_APFWriteOptions *options, long top_level_chunk) {
// GUTTED
}

void exec_agent::WriteTypedefs(aoString &fout, long theSectionNode) {
// GUTTED
}
