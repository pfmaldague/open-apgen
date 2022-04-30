#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "apDEBUG.H"

#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory>

#ifdef USE_AUTO_PTR
	#define AUTO_PTR_TEMPL std::auto_ptr
#else
	#define AUTO_PTR_TEMPL std::unique_ptr
#endif

#ifndef OLDUDEF
#include <dlfcn.h>
#endif

#include "ACT_exec.H"
#include "action_request.H"
#include "aafReader.H"
#include "ActivityInstance.H"
#include "apcoreWaiter.H"
#include "APerrors.H"
#include "APrequest_handler.H"
#include "DB.H"
#include "fileReader.H"
#include "IO_ApgenDataOptions.H"
#include "Multiterator.H"
#include "RES_def.H"
#include "RES_exec.H"
#include "EventRegistry.H"
#include "UTL_time.H"
#include "ThreadIntfc.H"
#include "TOL_write.H"

#include "IO_SASFWriteImpl.H"
#include "IO_ApgenData.H"
#include "IO_SASFWrite.H"

extern IO_TypedValue*	APval2IOval(
				const TypedValue& val,
				Cstring& any_errors,
				int arrayAppend);
extern IO_Time*		GetStartTime(
				ActivityInstance* act);
extern IO_TypedValue*	GetTypedValue(
				ActivityInstance* act,
				const Cstring& name,
				const TypedValue& val);

#ifdef have_xml_reader
extern "C" {
#include "concat_util.h"
extern void (*activityInstanceSerializationHandle)(void *, buf_struct *);
extern void (*activityDefsSerializationHandle)(buf_struct *);
} // extern "C"
#endif

using namespace std;

// GLOBALS:

		/*
		 *	Note: a static object would be more elegant, but we
		 *	chose a pointer because that lets us control
		 *	WHEN the subsystem is created.
		 */

ACT_exec*&	ACT_exec::ACT_subsystem_ptr() {
	static ACT_exec*	ae = NULL;
	return ae;
}

ACT_exec&	ACT_exec::ACT_subsystem() {
	return *ACT_subsystem_ptr();
}

slist<alpha_string, bsymbolnode>&	ACT_exec::Directives() {
	static slist<alpha_string, bsymbolnode> d;
	return d;
}

slist<alpha_string, bsymbolnode>&	ACT_exec::theTemporaryListOfDirectives() {
	static slist<alpha_string, bsymbolnode> d;
	return d;
}

bool			ACT_exec::recompute_durations = false;
bool			ACT_exec::dur_discrepancy_popup_disabled = false;


blist&	theStandardSASFelementNames() {
	static blist B(compare_function(compare_bstringnodes, false));
	return B;
}

// EXTERNS:

extern void		Purge_model();
extern int		validateFlag;

extern Cstring		data_type_to_C_type_string(apgen::DATA_TYPE type);

		//
		// in validate.C: (may be obsolete)
		//
void		clear_the_documentation_lists();

class ordered_pointer_to_activity: public Time_node {
public:
	ActivityInstance*	request;
	CTime_base		event_time;
	ordered_pointer_to_activity(ActivityInstance * A)
		: event_time(A->getetime()) ,
		request(A)
		{}
	ordered_pointer_to_activity(const ordered_pointer_to_activity & C)
		: Time_node(C),
		event_time(C.event_time),
		request(C.request)
		{}
	~ordered_pointer_to_activity() {}

	virtual void		setetime(
					const CTime_base& new_time) {
		event_time = new_time;
	}
	virtual const CTime_base& getetime() const {
		return event_time;
	}

	Node*			copy() {
		return new ordered_pointer_to_activity(*this);
	}
	const Cstring&		get_key() const {
		return Cstring::null();
	}
	Node_type		node_type() const {
		return CONCRETE_TIME_NODE;
	}
};

// V_7_7 change 3 (part 1) START        
class instance_remover {
public:   
        instance_remover() : armed(true) {}
        ~instance_remover();
          
        bool    armed;
};
            
instance_remover::~instance_remover() {
        if(armed) {
                eval_intfc::get_act_lists().clear_brand_new_list();
                armed = false;
	}
}
// V_7_7 change 3 (part 1) END

CTime_base string_to_CTime_converter(const Cstring &s) {
	return CTime(s);
}

LegendObject* (*LegendObject::theLegendObjectConstructorHandle) (
	const Cstring	&ltext,		// text of the legend
	const Cstring	&filename,	// file in which legend was defined (or NEW)
	int		preferred_height) // defaults to 32
		= NULL;

LegendObject* LegendObject::theLegendObjectConstructor(
	const Cstring& ltext,		// text of the legend
	const Cstring& filename,	// file in which legend was defined (or NEW)
	int	       preferred_height // defaults to 32
	) {

	LegendObject* LO = new LegendObject(ltext, filename, preferred_height);

	ACT_exec::addALegend(LO);
	return LO;
}

LegendObject* LegendObject::LegendObjectFactory(
		const Cstring	&ltext,		// text of the legend
		const Cstring	&filename,	// file in which legend was defined (or NEW)
		int		preferred_height) { // defaults to 32
	if(theLegendObjectConstructorHandle) {
		return theLegendObjectConstructorHandle(ltext, filename, preferred_height);
	}
	return NULL;
}
	
void localDisplayMessage(
		const string&	theTitle,
		const string&	theText,
		bool		addToLog) {
	if(addToLog) {
		errors::Post(/* "Adaptation-Generated Message" */ theTitle.c_str(), theText.c_str());
	}
}

// in ACT_type.C:
// extern void		transfer_act_type_to_stream(
// 				void*,
// 				aoString&);


// in RES_exec.H:
extern void		(*DisplayMessage_handler)(
				const string& theTitle,
				const string& theText,
				bool addToLog);

static bool	subsystems_exist = false;

extern "C" {
void create_subsystems(const char* Cprefix) {
	if(subsystems_exist) {
		return;
	}
	Cstring thePrefix(Cprefix);
	Cstring	s = Cstring();

	pEsys::process_an_AAF_request_handler	= Action_request::process_an_AAF_request;

	// transfer_act_type_to_stream_handler	= transfer_act_type_to_stream;
	string_to_CTime_base_handler		= string_to_CTime_converter;
	DisplayMessage_handler			= localDisplayMessage;

	compiler_intfc::CompileExpression	= fileReader::CompileExpression;
	compiler_intfc::CompileStatement	= fileReader::CompileStatement;
	compiler_intfc::CompileExpressionList	= fileReader::CompileExpressionList;
	compiler_intfc::CompileAndEvaluate	= fileReader::CompileAndEvaluate;
	compiler_intfc::CompileAndEvaluateSelf	= fileReader::CompileAndEvaluateSelf;
	compiler_intfc::Evaluate		= fileReader::Evaluate;
	compiler_intfc::EvaluateSelf		= fileReader::EvaluateSelf;
	compiler_intfc::CompileAndEvaluate	= fileReader::CompileAndEvaluate;

	Behavior::initialize();

	//
	// create_subsystems() is always invoked from the main thread:
	//
	thread_index = thread_intfc::MODEL_THREAD;

	ACT_exec::ACT_subsystem_ptr()		= new ACT_exec;
	eval_intfc::last_event_handler		= model_intfc::LastEvent;
	eval_intfc::first_act_handler		= ACT_exec::FirstRequestTime;

	thread_intfc::create_threads();

	globalData::CreateReservedSymbols();

	//
	// will be overridden in GUI setup:
	//
	LegendObject::theLegendObjectConstructorHandle
					= LegendObject::theLegendObjectConstructor;

	subsystems_exist = true;
}

void delete_subsystems() {
	if(!subsystems_exist) {
		return;
	}

	model_intfc::PurgeSchedulingEvents();
	udef_intfc::unregister_all_functions(
		/* print = */ APcloptions::theCmdLineOptions().show_function_calls);
	delete ACT_exec::ACT_subsystem_ptr();
	delete EventLoop::theEventLoopPtr;
	globalData::destroy();
	Behavior::delete_subsystems();
	RES_exec::clear_all_lists();
	EventLoop::theEventLoopPtr = NULL;
	ACT_exec::ACT_subsystem_ptr() = NULL;
	fileReader::theFileReader().Tokens().clear();
	fileReader::theFileReader().CMD_Tokens().clear();
	aafReader::delete_global_objects();

	// do this to disable list documentation...
	Validating = 0;
	clear_the_documentation_lists();

	//
	// Do this last, in APmain; we still need the strings for final
	// reports etc.:
	//
	// Cstring::delete_permanent_strings();

	subsystems_exist = false;
}
}; // extern "C"

ACT_exec::ACT_exec()
		: myAgent(this),
			ActivityEnds(true) {

	if(!theStandardSASFelementNames().get_length()) { // first time only
		theStandardSASFelementNames() << new bstringnode("activity_name");
		theStandardSASFelementNames() << new bstringnode("command_name");
		theStandardSASFelementNames() << new bstringnode("cyclic");
		theStandardSASFelementNames() << new bstringnode("editgroup");
		theStandardSASFelementNames() << new bstringnode("file");
		theStandardSASFelementNames() << new bstringnode("genealogy");
		theStandardSASFelementNames() << new bstringnode("group_name");
		theStandardSASFelementNames() << new bstringnode("identifier");
		theStandardSASFelementNames() << new bstringnode("key");
		theStandardSASFelementNames() << new bstringnode("lower_label");
		theStandardSASFelementNames() << new bstringnode("parameterFormat");
		theStandardSASFelementNames() << new bstringnode("processor");
		theStandardSASFelementNames() << new bstringnode("request_state");
		theStandardSASFelementNames() << new bstringnode("requestor");
		theStandardSASFelementNames() << new bstringnode("status");
		theStandardSASFelementNames() << new bstringnode("step_label");
		theStandardSASFelementNames() << new bstringnode("step_name");
		theStandardSASFelementNames() << new bstringnode("TEXT");
		theStandardSASFelementNames() << new bstringnode("text");
		theStandardSASFelementNames() << new bstringnode("type");
		theStandardSASFelementNames() << new bstringnode("upper_label");
		theStandardSASFelementNames() << new bstringnode("workgroup");
		}

}

ACT_exec::~ACT_exec() {
	purge();
	eval_intfc::get_act_lists().delete_all_instances();
	execAgent()->clear_typeless_activities();
	LegendsWritten.clear();
}

//
// Recompute the state of the model:
//
// 	- resource histories
// 	- globals
// 	- (possibly) activity data members
// 
// If the adaptation is big, this can be a massive process
// that takes hours of CPU time to complete.
//
void ACT_exec::AAF_do_model() {
	const char*	errcontext = "";
	model_control	control(model_control::MODELING);

	try {
		errcontext = "calling model_intfc::init_model";
		model_intfc::init_model(
			ACT_exec::ACT_subsystem().theMasterTime);

		errcontext = "calling ACT_exec::generate_time_events";
		ACT_exec::ACT_subsystem().generate_time_events();

		errcontext = "calling model_intfc::do_model";
		model_intfc::doModel(ACT_exec::ACT_subsystem().theMasterTime);
	} catch(eval_error Err) {
		Cstring	errstr;
		errstr << "Modeling error while " << errcontext << ". Details from res. subsys.:\n"
			<< Err.msg << "#";
		throw(eval_error(errstr));
	}
}

void ACT_exec::AAF_do_unschedule(
		bool		all_flag) {
	const char*	errcontext;

	try {
		errcontext = "abstracting scheduled activity/ies";
		model_intfc::doUnschedule(all_flag, ACT_exec::ACT_subsystem().theMasterTime);
		errcontext = "re-modeling";
		AAF_do_model();
	} catch(eval_error Err) {
		Cstring errstr = "Error while ";
		errstr << errcontext;
		ACT_exec::displayMessage(
			"Unscheduling error",
			errstr,
			Err.msg);
		throw(eval_error(Err.msg));
	}
}

void ACT_exec::AAF_do_schedule(
		bool					use_selection_list,
		bool					incremental,
		slist<alpha_void, dumb_actptr>&		top_kids) {
	const char*	errcontext = "";
	model_control	control(model_control::SCHEDULING_1);

	try {
		errcontext = "calling model_intfc::doSchedule";
		model_intfc::doSchedule(
				use_selection_list,
				incremental,
				top_kids,
				ACT_exec::ACT_subsystem().theMasterTime);
	}
	catch(eval_error Err) {
		Cstring errstr;
		errstr << "Modeling error in " << errcontext << ":\n" << Err.msg << "#";
		throw(eval_error(errstr));
	}
}

void ACT_exec::get_all_type_names(stringtlist& inlist) {
	for(int i = 0; i < Behavior::ClassTypes().size(); i++) {
		Behavior& T = *Behavior::ClassTypes()[i];
		if(T.realm == "activity") {
			inlist << new emptySymbol(T.name);
		}
	}
}

void ACT_exec::dump() {
	execAgent()->dump();
}

apgen::RETURN_STATUS ACT_exec::WriteGlobalsToJsonStrings(
		map<string, string>&	result,
		Cstring&		errors) {
	globalData::WriteGlobalsToJson(result);
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS ACT_exec::WriteActInteractionsToJsonStrings(
		bool		singleFile,
		const Cstring&	Filename,
		stringslist&	result,
		Cstring&	errors) {
	return exec_agent::WriteActInteractionsToJsonStrings(
			singleFile,
			Filename,
			result,
			errors);
}

apgen::RETURN_STATUS ACT_exec::WriteActivitiesToJsonStrings(
		bool 		singleFile,
		const Cstring&	Namespace,
		const Cstring&	Directory,
		pairtlist&	result,
		Cstring&	errors) {
	return exec_agent::WriteActivitiesToJsonStrings(
			singleFile,
			Namespace,
			Directory,
			result,
			errors);
}

apgen::RETURN_STATUS ACT_exec::WriteResourceDefsToJsonString(
		const Cstring&	Namespace,
		Cstring& result,
		Cstring& errors) {
	return exec_agent::WriteResourceDefsToJsonString(Namespace, result, errors);
}

apgen::RETURN_STATUS ACT_exec::WriteResourcesToJsonStrings(
		bool singleFile,
		const Cstring& Namespace,
		stringslist& result,
		Cstring& errors) {
	return exec_agent::WriteResourcesToJsonStrings(singleFile, Namespace, result, errors);
}

//
// High-level static function that generates the master list of time events:
// only called from AAF_do_model() (this file) and model_intfc::doSchedule()
// (in Events.C).
//
void ACT_exec::generate_time_events() {
	status_aware_iterator visible_acts(eval_intfc::get_act_lists().get_scheduled_active_iterator());
	status_aware_iterator decomposed_acts(eval_intfc::get_act_lists().get_decomposed_iterator());
	tlist<alpha_time, Cnode0<alpha_time, ActivityInstance*> > AllActs(true);
	slist<alpha_time, Cnode0<alpha_time, ActivityInstance*> >::iterator all_acts(AllActs);
	Cnode0<alpha_time, ActivityInstance*>*	time_ptr;
	ActivityInstance*			request;
	int					number_of_c_instances_initialized = 0;
	Cstring					errs;
	CTime_base				origin_of_creation;


	eval_intfc::get_act_lists().order_active_list();
	eval_intfc::get_act_lists().order_decomposed_list();

	apgen::METHOD_TYPE	mt;

	while((request = visible_acts())) {
	    if(!request->is_unscheduled()) {
		if(request->theActAgent()->hasType()) {

#ifdef OBSOLETE
		    if(   request->has_decomp_method(mt)
		          && mt == apgen::METHOD_TYPE::MODELED_DECOMPOSITION) {
			instance_state_changer	ISC(request);

			ISC.set_desired_offspring_to(false);
			try {
			    ISC.do_it(NULL);
			} catch(eval_error Err) {
			    errs << "Error while deleting children of act. "
					"with modeled_decomposition section; details:\n"
					<< Err.msg;
			    throw(eval_error(errs));
			} catch(decomp_error Err) {
			    errs << "Error while deleting children of act. "
					"with modeled_decomposition section; details:\n"
					<< Err.msg;
			    throw(eval_error(errs));
			}
		    }
#endif /* OBSOLETE */

		}
		AllActs << new Cnode0<alpha_time, ActivityInstance*>(request->getetime(), request);
		request->hierarchy().recursively_get_time_ptrs_to_nonexclusive_descendants(AllActs);
	    }
	}
	while((request = decomposed_acts())) {
	    if(!request->is_unscheduled()) {
		if(request->theActAgent()->hasType()) {
#ifdef OBSOLETE
		    if(  request->has_decomp_method(mt)
		         && mt == apgen::METHOD_TYPE::MODELED_DECOMPOSITION) {
			instance_state_changer	ISC(request);

			ISC.set_desired_offspring_to(false);
			try {
			    ISC.do_it(NULL);
			} catch(eval_error Err) {
			    errs << "Error while deleting children of act. "
					"with modeled_decomposition section; details:\n"
					<< Err.msg;
			    throw(eval_error(errs));
			} catch(decomp_error Err) {
			    errs << "Error while deleting children of act. "
					"with modeled_decomposition section; details:\n"
					<< Err.msg;
			    throw(eval_error(errs));
			}
			AllActs << new Cnode0<alpha_time, ActivityInstance*>(
					request->getetime(),
						request);
		    } else
#endif /* OBSOLETE */

		    if(  request->has_decomp_method(mt) &&
		    	        mt == apgen::METHOD_TYPE::NONEXCLDECOMP) {

			//
			// NOTE: we are not doing this recursively, so there is no
			// duplication with the previous while loop -
			// these are UP activities!
			//
			AllActs << new Cnode0<alpha_time, ActivityInstance*>(
						request->getetime(),
						request);
		    }
		}
	    }
	}
	stringstream	errstream;
	bool		ran_into_an_error = false;
	while((time_ptr = all_acts())) {
	    request = time_ptr->payload;
	    execution_context::return_code Code = execution_context::REGULAR;

	    try {
		request->exercise_modeling(Code);
	    }
	    catch(eval_error Err) {
		ran_into_an_error = true;
		errstream << *Err.msg << "\n";
	    }
	}
	if(ran_into_an_error) {
	    Cstring temp("ACT_exec unable to generate events; details:\n");
	    temp << errstream.str().c_str();
	    throw(eval_error(temp));
	}

	if(model_intfc::FirstEvent(origin_of_creation)) {
	    bool	int_err = false;
	    mEvent*	theInitialEvent = mEvent::initializationQueue().first_node();

	    assert(theInitialEvent);
	    if(theInitialEvent->Key.getetime() > origin_of_creation) {

		// we must adjust the start time of the initial event
		//
		// Do not do this; it messes up the IDs of the events
		// in case they all occur at the same time:
		//
		// delete theInitialEvent;
		// theInitialEvent =
		//     mEvent::factory(eventPLD::INIT, origin_of_creation, int_err);
		// mEvent::initializationQueue() << theInitialEvent;
		//
		mEvent::initializationQueue().remove_node(theInitialEvent);
		theInitialEvent->Key.setetime(origin_of_creation);
		mEvent::initializationQueue() << theInitialEvent;
	    }
	} else {
		// there are no events and who cares what the time of the initial event is?
	}
}

bool act_lists::theFirstRequestTime(CTime_base& T, int print_flag) {
	ActivityInstance*	request;
	T = CTime_base(0, 0, false);
	if((request = (ActivityInstance *) someActiveInstances.earliest_node())) {
		T = request->getetime();
	}
	if((request = (ActivityInstance *) someDecomposedActivities.earliest_node())) {
		if(request->getetime() < T) {
			T = request->getetime();
		}
	}
	if((request = (ActivityInstance *) someAbstractedActivities.earliest_node())) {
		if(request->getetime() < T) {
			T = request->getetime();
		}
	}
	if(T > CTime_base(0, 0, false)) {
		return true;
	}
	T = CTime_base(time(0), 0, false);
	return false;
}

bool ACT_exec::FirstRequestTime(CTime_base& T, int print_flag) {
	return eval_intfc::get_act_lists().theFirstRequestTime(T, print_flag);
}

int ACT_exec::LastRequestTime(CTime_base& T) {
	if(!ActivityEnds.get_length()) {
		T = CTime_base(time(0), 0, false);
		return 0;
	}
	T = ActivityEnds.latest_node()->getKey().getetime();
	return 1;
}

CTime_base ACT_exec::default_req_start_time() {
	return CTime_base(ACTREQDEFSTIME, 0, false);
}

CTime_base ACT_exec::default_req_span() {
	return CTime_base(ACTREQDEFTIMESPAN , 0, true);
}

void act_lists::delete_all_instances() {
	// slist<alpha_void, dumb_actptr>		any_requests;
	// slist<alpha_void, dumb_actptr>::iterator acts(any_requests);

	someActiveInstances.disable_callbacks();
	someDecomposedActivities.disable_callbacks();
	someAbstractedActivities.disable_callbacks();
	somePendingActivities.disable_callbacks();
	someBrandNewActivities.disable_callbacks();

	someActiveInstances.clear();
	someDecomposedActivities.clear();
	someAbstractedActivities.clear();
	somePendingActivities.clear();
	someBrandNewActivities.clear();

	someActiveInstances.enable_callbacks();
	someDecomposedActivities.enable_callbacks();
	someAbstractedActivities.enable_callbacks();
	somePendingActivities.enable_callbacks();
	someBrandNewActivities.enable_callbacks();
}

void act_lists::clean_up_clipboard() {
	somePendingActivities.clear();
}

void act_lists::synchronize_all_lists() {

  someActiveInstances.synchronize_all();
  someDecomposedActivities.synchronize_all();
  someAbstractedActivities.synchronize_all();
  somePendingActivities.synchronize_all();
  someBrandNewActivities.synchronize_all();

  slist<alpha_time, time_actptr, ActivityInstance*>& endList(ACT_exec::ACT_subsystem().ActivityEnds);
  slist<alpha_time, time_actptr, ActivityInstance*>  temporaryList;
  time_actptr* ptr1;
  time_actptr* ptr2;
  for(ptr1 = endList.first_node(); ptr1; ptr1 = ptr2) {
    ptr2 = ptr1->next_node();
    CTime_base finish = (*ptr1->BP->Object)[ActivityInstance::FINISH].get_time_or_duration();
    if(ptr1->Key.getetime() != finish) {
	endList.remove_node(ptr1);
	ptr1->Key.setetime(finish);
	temporaryList << ptr1;
    }
  }
  for(ptr1 = temporaryList.first_node(); ptr1; ptr1 = ptr2) {
    ptr2 = ptr1->next_node();
    endList << ptr1;
  }
}

void ACT_exec::clean_up_clipboard() {
	eval_intfc::get_act_lists().clean_up_clipboard();
}

//purge all the activity requests and type definitions
void ACT_exec::purge() {
	Directives().clear();
	fileReader::theFileReader().listOfFilesReadThroughSeqReview().clear();

	/*	Do this first because of a nasty interference between Events and activity types
	 *	(the interp_event class contains (C-style, totally dumb) pointers to items stored 
	 *	inside activity types):
	 */
	model_intfc::PurgeSchedulingEvents();
	eval_intfc::get_act_lists().delete_all_instances();

	/*
	 *	Original Comment: "DO THIS LAST: activity constructors may need global resources..."
	 *
	 *	Well, that's a good idea but it doesn't work because deleting activity types will
	 *	make pointers to uage clauses illegal.	So, purge the model now.
	 */
	model_intfc::purge();

	// DO THIS AFTER CLEARING act_types; ACT_type destructor adds any lingering activity
	// requests (e. g. with errors in them) to the TypelessActivities list...
	// purge list of activity instances w/o type:
	execAgent()->clear_typeless_activities();
	Dsource::theLegends().clear();
	ActivityInstance::purge();
	eval_intfc::ListOfAllFileNames().clear();
	addNewItem();
}

void  Purge_model() {
}

// V_7_7 change 6 START (part 1)
//purge all the activity requests
void ACT_exec::purge_all_acts() {
	execAgent()->clear_the_discrepancy_list();
	eval_intfc::get_act_lists().delete_all_instances();
	// planFilename = "New";
	execAgent()->clear_typeless_activities();
	ActivityInstance::purge();
	eval_intfc::ListOfAllFileNames().clear();
	addNewItem();
	// planFilename = "";
}
// V_7_7 change 6 END (part 1)


void ACT_exec::TransferAllActivityTypes(aoString &Stream, long theTopLevelEl) {
	for(int i = 0; i < Behavior::ClassTypes().size(); i++) {
		Behavior& T = *Behavior::ClassTypes()[i];
		if(T.realm == "activity") {
			T.to_stream(&Stream, 0);
		}
	}
}

	/* This method merges all the entities from planfiles of the list
	 *
	 * At some point, it might be nice to have an "APF manager" that keeps track
	 * of where each activity instance came from.  This could become quite involved,
	 * though... */
void ACT_exec::merge_planfiles(const Cstring& newplanname, stringslist& list_of_files) {
	LegendObject*			legend_node;
	List_iterator			the_legends(Dsource::theLegends());
	int				dum_prio = 0;
	AUTO_PTR_TEMPL<status_aware_multiterator> the_instances(eval_intfc::get_instance_multiterator(dum_prio, true, true));
	ActivityInstance*			request;

	while((legend_node = (LegendObject *) the_legends())) {
		if(list_of_files.find(legend_node->apf)) {
			legend_node->apf = newplanname;
		}
	}
	while((request = the_instances->next())) {
		if(list_of_files.find(request->get_APFplanname())) {
			request->set_APFplanname(newplanname);
		}
	}
}

bool	ACT_exec::there_are_selections(){
	return eval_intfc::sel_list().get_length() > 0;
}

bool	ACT_exec::there_are_active_selections(){
	slist<alpha_void, smart_actptr>::iterator	li(eval_intfc::sel_list());
	smart_actptr*	b;

	while((b = li())) {
		ActivityInstance* a = b->BP;
		if(a->agent().is_active()) {
			return true;
		}
	}
	return false;
}

int	ACT_exec::number_of_selections(){
	return eval_intfc::sel_list().get_length();
}

int	ACT_exec::number_of_active_selections() {
	int		N = 0;
	slist<alpha_void, smart_actptr>::iterator	li(eval_intfc::sel_list());
	smart_actptr*	b;

	while((b = li())) {
		ActivityInstance* a = b->BP;
		if(a->agent().is_active()) {
			N++;
		}
	}
	return N;
}

void	ACT_exec::get_active_selections(slist<alpha_void, dumb_actptr>& l) {
	slist<alpha_void, smart_actptr>::iterator	li(eval_intfc::sel_list());
	smart_actptr*					b;

	while((b = li())) {
		ActivityInstance* a = b->BP;
		// debug
		// cout << "get_active_selections: considering " << a->get_unique_id() << "\n";
		if(a->agent().is_active()) {
			dumb_actptr*	p = new dumb_actptr(a);
			// cout << "get_active_selections: adding ptr to " << p->payload->get_unique_id() << "\n";
			l << p;
		}
	}
}

void ACT_exec::insert_instance(ActivityInstance* a) {
	if(!eval_intfc::sel_list().find((void *)a)) {
		eval_intfc::sel_list() << new smart_actptr(a);
	}
}

smart_actptr* ACT_exec::find_selection(ActivityInstance* a) {
	return eval_intfc::sel_list().find((void*) a);
}

void Dsource::select() {

	if(ACT_exec::find_selection((ActivityInstance *) this))
		return;	// already selected

	S_elected = true;

	ACT_exec::insert_instance((ActivityInstance *) this);
	if(dataForDerivedClasses) {
		dataForDerivedClasses->handle_selection(true);
	}
}

void Dsource::unselect() {
	smart_actptr* selnode;

	if((selnode = ACT_exec::find_selection((ActivityInstance *) this))) {
		delete selnode;
	} else {
		return;
	}

	S_elected = false;
	if(dataForDerivedClasses) {
		dataForDerivedClasses->handle_selection(false);
	}
}

CTime_base Dsource::get_timespan() const {
	return CTime_base(0, 0, true);
}

#ifdef TRIVIAL_VALIDATE
int	Dsource::validate() {
	int	result = 1;

	if(!Node::validate()) {
		result = 0; }
	return result; }
#endif

const char *get_state(ActivityInstance *a) {
	if(a->agent().is_brand_new()) {
		return "Brand New";
	} else if(a->agent().is_decomposed()) {
		return "Decomposed";
	} else if(a->agent().is_abstracted()) {
		return "Abstracted";
	} else if(a->agent().is_on_clipboard()) {
		return "On Clipboard";
	}
	return "Unknown";
}

/* The following function has 2 implementations: classic and C++.
 * The classic implementation is complicated; it allows for overriding
 * durations defined as a formula in the definition of the Duration
 * attribute, and it tries to salvage any activities that reference a
 * parent that is never defined.
 *
 * The C++ version will be much simpler: invoke create() on each
 * activity in the plan; it will be assumed that the hierarchy pointers
 * are consistent. This is the job of the APF parser.
 *
 * In view of the 2 distinct implementations, delegate the job to the
 * exec_agent class. */
void ACT_exec::instantiate_all(
	tlist<alpha_string, instance_tag>& list_of_instance_nodes) {
	execAgent()->instantiate_all(list_of_instance_nodes);
}

int ACT_exec::WriteDataToStream(
		aoString&		fout,
		const stringslist&	listOfFiles,
		const List&		legends,
		slist<alpha_void, dumb_actptr>& ptrs_to_bad_acts,
		IO_APFWriteOptions*	options,
		long			top_level_chunk) {
	CTime_base startTime, endTime;
	//will iterate through data
	int success = 1;


	/* In principle we should add the following call:
	 * WriteTypedefs(fout);
	 *
	 * However, you may get errors if you read something twice. I think that's
	 * why it was omitted; you should always read the AAF with the definitions
	 * first. */

	WriteVersion(fout, top_level_chunk);
	WriteLegendsToStream(fout, legends, options);
	globalData::WriteEpochsToStream(fout, options);
	globalData::WriteTimeSystemsToStream(fout, options);
	globalData::WriteGlobalsToStream(fout, options);
	WriteDirectivesToStream(fout, options);

	//initialize the start and end times
	model_intfc::FirstEvent(startTime);
	model_intfc::LastEvent(endTime);
	endTime = endTime + CTime_base(1, 0, true);
	success = WriteActivitiesToStream(fout, listOfFiles, startTime, endTime, ptrs_to_bad_acts, top_level_chunk);
	return success;
}

void ACT_exec::displayMessage(
		const Cstring &windowTitle,
		const Cstring &messageTitle,
		const Cstring &message) {
	errors::Post(windowTitle, messageTitle + Cstring("\n") + message); }

void ACT_exec::displayMessage(
		const Cstring &windowTitle,
		const Cstring &messageTitle,
		const stringslist& L) {
	emptySymbol* N;

	for(	N = L.first_node();
		N;
		N = N->next_node()) {
		errors::Post(windowTitle, messageTitle + Cstring("\n") + N->get_key());
	}
}


void    ACT_exec::addNewItem() {
	if(!eval_intfc::ListOfAllFileNames().find("New")) {
		eval_intfc::ListOfAllFileNames() << new emptySymbol("New");
	}
}


apgen::RETURN_STATUS string_to_valid_time(
		const Cstring&	given_time,
		CTime_base&	result,
		Cstring&	errors) {
	TypedValue val(apgen::DATA_TYPE::TIME);

	try {
		compiler_intfc::CompileAndEvaluateSelf(
					given_time,
					val);
	}
	catch(eval_error Err) {
		errors << "Expression " << given_time << " has errors:\n"
			<< Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	result = val.get_time_or_duration();
	return apgen::RETURN_STATUS::SUCCESS;
}

void
ACT_exec::WriteLegendsToStream(	aoString &fout,
				const List &legends,
				IO_APFWriteOptions *options,
				long top_level_chunk) {
	String_node	*legend_label;
	List_iterator	legs(legends);
	bool		wrote_something = false;

	while((legend_label = (String_node*) legs())) {
		LegendObject *theLeg = (LegendObject *) Dsource::theLegends().find(legend_label->get_key());

		if(!theLeg) {
			Cstring msg("Unable to write legend \"");
			msg << legend_label->get_key() << "\"; it doesn't seem to exist...";
			ACT_exec::displayMessage("APGEN DATA OUTPUT", "LEGEND ERROR", (char *) *msg);
			return;
		}

		if(options->GetLegendsOption() == Action_request::INCLUDE_NOT_AT_ALL) {
			;
		} else {
			if(!wrote_something) {
				// prepend a carriage return
				fout << "\n";
				wrote_something = true;
			}
			if(options->GetLegendsOption() == Action_request::INCLUDE_AS_COMMENTS) {
				fout << "# "; }
			fout << "directive \"Legend\" = [ \"" << legend_label->get_key() << "\", ";
			fout << theLeg->get_official_height() << " ];\n";
		}
	}
}

bool ACT_exec::WriteActivitiesToStream(
		aoString&		fout,
		const stringslist&	listOfFiles,
		const CTime_base&	startTime,
		const CTime_base&	endTime,
		slist<alpha_void, dumb_actptr>& bad_activities, // contains any instance(s) with illegal status
		long			top_level_chunk
		) {
	return ACT_exec::ACT_subsystem().execAgent()->WriteActivitiesToStream(fout, listOfFiles, startTime, endTime, bad_activities, top_level_chunk); }

void ACT_exec::TransferActivitiesToListOfSmartNodes(
		tlist<timeptr<ActivityInstance>, smart_actref, ActivityInstance*>& Ordered_downs) {
	tlist<alpha_void, dumb_actptr>			Ups;
	tlist<alpha_void, dumb_actptr>			Downs;
	slist<alpha_void, dumb_actptr>::iterator	ups(Ups);
	slist<alpha_void, dumb_actptr>::iterator	downs(Downs);
	slist<timeptr<ActivityInstance>, smart_actref, ActivityInstance*>::iterator ordered_downs(Ordered_downs);
	dumb_actptr*					b;
	smart_actref*					t;
	bool						wrote_something = false;
	ActivityInstance*				act_request;
	int						dum_prio = 0;

	AUTO_PTR_TEMPL<status_aware_multiterator>	the_instances(eval_intfc::get_instance_multiterator(dum_prio, true, false));

	// debug
	// cout << "ACT_exec: time limits " << startTime.to_string();
	// cout << ", " << endTime.to_string() << endl;
	while((act_request = the_instances->next())) {
		ActivityInstance*	up = act_request->hierarchy().get_to_the_top();

		if(up && !Ups.find((void *) up)) {
			Ups << new dumb_actptr(up); } }

	while((b = ups())) {
		ActivityInstance*	u = b->payload;

		u->hierarchy().recursively_get_ptrs_to_all_descendants(Downs);
		Downs << new dumb_actptr(u); }

	// We now have a set of activities that is closed under "belongs to the same hierarchy as"
	int idebug = 0;
	while((b = downs())) {
		ActivityInstance*	d = b->payload;

		Ordered_downs << new smart_actref(d);
		idebug++;
	}
	// debug
	// cout << "Transfer Acts to list: " << idebug << " activitie(s) transferred\n";
}

void ACT_exec::WriteVersion(aoString &fout, long theTopLevelEl) {

	fout << "apgen version \"" << get_apgen_version_build_platform()
		<< "\"" << "\n" << "\n" << "\n";

	//
	// This interface sucks but we'll take if for now
	//
	string			header_info(*get_tol_header_info(
					"",	// file name (for TOL output)
					false	// XMLTOL flag (for XMLTOL output)
					));
	string::size_type	idx = header_info.find('\n');

	fout << "# " << header_info.substr(0, idx) << "\n";
	idx++;
	while(1) {
		string::size_type idx2 = header_info.find('\n', idx);
		if(idx2 == header_info.npos) {
			break;
		}
		string chunk = header_info.substr(idx, idx2 - idx);
		fout << "# " << chunk.c_str() << "\n";
		idx = idx2 + 1;
	}
}

void	ACT_exec::WriteLegendLayoutToStream(aoString &Stream, const Cstring &theTag) {
	LegendObject		*curlegenddef;
	List_iterator 		all_legends(Dsource::theLegends());
	extern Cstring		addQuotes(const Cstring &);

	if(!Dsource::theLegends().get_length()) {
		return;
	}

	Stream << "directive \"Activity Legend Layout\" = [\n    \"" << *theTag << "\",\n";
	while((curlegenddef = (LegendObject *) all_legends())) {
		Stream << "    [ " << addQuotes(curlegenddef->get_key()) << " , "
			<< curlegenddef->get_official_height() << " ]";
		if(curlegenddef != (LegendObject *) Dsource::theLegends().last_node()) {
			Stream << ", ";
		}
		Stream << "\n";
	}
	Stream << "    ] ;\n";
}

void LegendObject::adjust_height(int new_value) {
	preferredHeight = new_value;
}

LegendObject::LegendObject(const Cstring &ltext, const Cstring &filename, int preferred_height)
	: bstringnode(ltext),
	apf(filename),
	preferredHeight(preferred_height) {
}

LegendObject::LegendObject(const LegendObject &LO)
	: bstringnode(LO),
	preferredHeight(LO.preferredHeight),
	apf(LO.apf) {}

LegendObject::~LegendObject() {
	ActivityPointers.clear();
}

void	ACT_exec::addALegend(LegendObject *LO) {
	LegendObject		*aPreviousLegendWithTheSameName;

	if((aPreviousLegendWithTheSameName = (LegendObject *) Dsource::theLegends().find(LO->get_key()))) {
		// smart pointers will hopefully do their magic here:
		delete aPreviousLegendWithTheSameName;
	}

	Dsource::theLegends() << LO;	// List order is used for vertical ordering; legend_objects are derived from
				// bstringnode, and so the blist is in alphanumeric order.
}

LegendObject *LegendObject::theGenericLegend(const Cstring &plan_name) {
	LegendObject	*genleg = (LegendObject *) Dsource::theLegends().find(GENERICLEGEND);

	if(!genleg) {
		genleg = LegendObjectFactory(GENERICLEGEND, plan_name, ACTVERUNIT);
	}
	return genleg;
}

int LegendObject::get_official_height() const {
	return preferredHeight;
}


void ACT_exec::reorder_legends_as_per(const Cstring &layoutID, List &thePointers) {
	blist		theWholeList(compare_function(compare_bpointernodes, false));
	List_iterator	ptrs(thePointers);
	Pointer_node	*ptr;
	bpointernode	*bptr;
	List_iterator	legends(Dsource::theLegends());
	LegendObject	*lo;

	// 1. create pointers to all existing legends (thePointers may be a partial list)
	while((lo = (LegendObject *) legends())) {
		theWholeList << new bpointernode(lo, NULL);
	}
	// 2. remove redundant pointers
	while((ptr = (Pointer_node *) ptrs())) {
		bptr = (bpointernode *) theWholeList.find(ptr->get_ptr());
		delete bptr;
	}
	// 3. append missing pointers
	while((bptr = (bpointernode *) theWholeList.first_node())) {
		thePointers << new Pointer_node(*bptr);
		delete bptr;
	}
	// 4. remove (but do not destroy) all legends from the master list
	while((ptr = (Pointer_node *) ptrs())) {
		lo = (LegendObject *) ptr->get_ptr();
		if(Dsource::theLegends().find(lo->get_key())) {
			Dsource::theLegends().remove_node(lo);
		}
	}
	// 4. re-insert all nodes in the order of the new list
	while((ptr = (Pointer_node *) thePointers.first_node())) {
		Dsource::theLegends().insert_node((LegendObject *) ptr->get_ptr());
		delete ptr;
	}
}

int
ACT_exec::SendDataToClient(
		slist<alpha_void, dumb_actptr>& ptr_to_bad_acts,
		Cstring& Errs) {
	return SendDataToClient(
			eval_intfc::ListOfAllFileNames(),
			Dsource::theLegends(),
			ptr_to_bad_acts,
			Errs);
}

int
ACT_exec::SendDataToClient(
		const stringslist&	listOfFiles,
		const List&		legends,
		slist<alpha_void, dumb_actptr>& ptrs_to_bad_acts,
		Cstring&		any_errors) {

	int success = 1;
	
	CTime_base startTime, endTime;

	//Epochs
	success = globalData::SendEpochsToClient();

	if(!success)
		return 0;

	//Globals
	success = globalData::SendGlobalsToClient(any_errors);
	
	if(!success)
		return 0;

	//initialize the start and end times
	model_intfc::FirstEvent( startTime );
	model_intfc::LastEvent( endTime );
	endTime = endTime + CTime_base(1, 0, true);
	
	success = SendActivitiesToClient(
			listOfFiles,
			startTime,
			endTime,
			ptrs_to_bad_acts);
	
	return success;
}

int
globalData::SendGlobalsToClient(Cstring &any_errors) {
	Cstring			Errs;

	int success = IO_SASFWriteImpl::SASFwriter().BeginGlobalsSection();

	if(!success)
		return 0;

	task* T = Behavior::GlobalType().tasks[0];
	for(int i = 0; i < T->get_varinfo().size() && (success); i++) {
		// tgs = dynamic_cast<TrueGlobalSymbol*>(tds);
		// assert(tgs);
		TypedValue&	val = (*behaving_object::GlobalObject())[i];
		const Cstring&	st = T->get_varinfo()[i].first;
		if(	(!isReserved(st)
			&& !isAnEpoch(st)
			&& !isATimeSystem(st))
		  ) {
			IO_SASFWriteImpl::SASFwriter().AddOneGlobal(*st, val);
		}
	}

	success = IO_SASFWriteImpl::SASFwriter().EndGlobalSection();
	return success;
}


int
globalData::SendEpochsToClient() {
	
	int success = IO_SASFWriteImpl::SASFwriter().BeginEpochsSection();

	if(!success) {
		return 0;
	}

	task* T = Behavior::GlobalType().tasks[0];
	for(int i = 0; i < T->get_varinfo().size() && (success); i++) {
		// tgs = dynamic_cast<TrueGlobalSymbol*>(tds);
		// assert(tgs);
		if(isAnEpoch(T->get_varinfo()[i].first)) {
			TypedValue&	val = (*behaving_object::GlobalObject())[i];
			IO_Epoch	epoch;
			string		epochName = *(T->get_varinfo()[i].first);

			CTime_base	time = val.get_time_or_duration();

			epoch.SetOrigin(time);
			epoch.SetName(epochName);
			success = IO_SASFWriteImpl::SASFwriter().AddOneEpoch(epoch);

			if(!success) {
				return 0;
			}
		}
	}

	success = IO_SASFWriteImpl::SASFwriter().EndEpochsSection();

	return success;
}

int ACT_exec::SendActivitiesToClient(
		const stringslist&	listOfFiles,
		const CTime_base&	startTime,
		const CTime_base&	endTime,
		
		//
		// contains any instance(s) with illegal status:
		// 
		slist<alpha_void, dumb_actptr>& bad_activities
		) {

    int						success = 1;

    tlist<alpha_void, dumb_actptr>		Ups;

    tlist<alpha_void, dumb_actptr>		Downs;

    tlist<timeptr<ActivityInstance>, smart_actref, ActivityInstance*>
						Ordered_downs(true);

    slist<alpha_void, dumb_actptr>::iterator	ups(Ups);

    slist<alpha_void, dumb_actptr>::iterator	downs(Downs);

    slist<timeptr<ActivityInstance>, smart_actref, ActivityInstance*>::iterator
						ordered_downs(Ordered_downs);

    dumb_actptr*				b;
    smart_actref*				t;
    int						dum_prio = 0;

    ActivityInstance*				act_request;

    AUTO_PTR_TEMPL<status_aware_multiterator> the_instances(
				eval_intfc::get_instance_multiterator(
							dum_prio,
							true,
							true));

    while((act_request = (ActivityInstance*) the_instances->next())) {
	if(	listOfFiles.find(act_request->get_APFplanname())
		&& (act_request->getetime() >= startTime)
		&& (act_request->getetime() < endTime)
		) {
		ActivityInstance*	up
			= act_request->hierarchy().get_to_the_top();

		if(up && !Ups.find((void*) up)) {
			Ups << new dumb_actptr(up, up);
		}
	}
    }
    while((b = ups())) {
	ActivityInstance*	u = b->payload;

	u->hierarchy().recursively_get_ptrs_to_all_descendants(Downs);
	Downs << new dumb_actptr(u, u);
    }

    //
    // We now have a set of activities that is closed under
    // "belongs to the same hierarchy as"
    //
    while((b = downs())) {
	ActivityInstance*	d = b->payload;
	CTime_base		ct;

	Ordered_downs << new smart_actref(d);
    }

    //
    // to obtain a predictable activity instance ordering, we will use the
    // activity instances' unique IDs as a secondary key
    //
    tlist<alpha_string, Cnode0<alpha_string, ActivityInstance*> >
					Consistent(true);
    tlist<alpha_string, Cnode0<alpha_string, ActivityInstance*> >::iterator
					consistent(Consistent);
    Cnode0<alpha_string, ActivityInstance*>*
					c;
    CTime_base				T;
    bool				first_time = true;

    while(true) {
	bool must_process_list = false;
	t = ordered_downs();
	if(t) {
	    act_request = t->getKey().mC;
	    if(first_time) {
		T = act_request->getetime();
		Consistent << new Cnode0<alpha_string, ActivityInstance*>(
			act_request->get_unique_id(), act_request);
		first_time = false;
	    } else if(act_request->getetime() > T) {
		must_process_list = true;
	    } else {
		Consistent << new Cnode0<alpha_string, ActivityInstance*>(
			act_request->get_unique_id(), act_request);
	    }
	} else {
	    must_process_list = true;
	}
	if(must_process_list) {
	    while((c = consistent())) {
		ActivityInstance*		a = c->payload;
		apgen::act_visibility_state	V = apgen::act_visibility_state::VISIBILITY_REGULAR;

		//
		// in the time range that we care about
		//
		string name;
		IO_SASFWriteImpl::SASFwriter().GetClientName(&name);

		IO_ActivityInstance instance;
		if(a->const_agent().is_decomposed()) {
			V = apgen::act_visibility_state::VISIBILITY_DECOMPOSED;
		} else if(a->const_agent().is_abstracted()) {
			V = apgen::act_visibility_state::VISIBILITY_ABSTRACTED;
		} else if(!a->const_agent().is_active()) {
			bad_activities << new dumb_actptr(a, a);
		}

		//
		// Translate a into the duplicate instance structure
		// on the IO_SASFWriteImpl side of the bridge. The
		// code is in ACT_exec_agent.C.
		//
		exec_agent::SetActivityInstance(a, &instance, name, V);

		//
		// Exercise the bridge to IO_SASFWriteImpl:
		//
		success = IO_SASFWriteImpl::SASFwriter().AddOneActivity(instance);

		if(!success) {
		    // how could this fail?
		    return 0;
		}
	    }
	    Consistent.clear();
	    if(t) {
		T = act_request->getetime();
		Consistent << new Cnode0<alpha_string, ActivityInstance*>(
				act_request->get_unique_id(),
				act_request);
	    } else {
		break;
	    }
	}
    }

    if(bad_activities.get_length()) {
	return 0;
    }
    return success;
}

	/*
	 * NOTE: As far as the user is concerned, the 'removed' activity is ALWAYS placed in
	 * the copy buffer. If sel_remove originates from a COPY request, then copyflg is set and
	 * it is a COPY of the activity that is placed in the buffer, not the activity itself.
	 * If sel_remove originates from a CUT request, then the activity itself is moved to the
	 * copy buffer.
	 *
	 * NOTE: the clearflg parameters is always 1. -- REMOVED 11/9/2000
	 *
	 * NOTE: BUG FIX -- selection_copy used to contain DUMB pointers. When trying to CUT an activity
	 * that is already in the Pending buffer (something only Scott Maxwell ever did!), the
	 * Pending Acts.clear() will delete the activities, but the DUMB pointer will point to the wrong
	 * thing...
	 *
	 * Solution: use SMART pointers.
	 */

apgen::RETURN_STATUS ACT_exec::sel_remove(
		const slist<alpha_void, dumb_actptr>&	selection_copy,
		const unsigned char			copyflg,
		Cstring&				theError) {
	ActivityInstance*				actreqnode;
	ActivityInstance					*lost_child, *parent_to_revive;
	slist<alpha_void, dumb_actptr>		ParentsWithDeletedOffspring;
	slist<alpha_void, dumb_actptr>::iterator	parents_to_reveal(ParentsWithDeletedOffspring);
	dumb_actptr					*ptr, *parent_ptr;
	Cnode0<alpha_void, dumb_actptr*>*		bptr;
	slist<alpha_void, dumb_actptr>::iterator	selected_activities(selection_copy);
			// NOTE: used to be bcopy, but linux objects to
			// this (also a function name):
	tlist<alpha_void, dumb_actptr>		b_copy;
	tlist<alpha_void, dumb_actptr>::iterator	theCopies(b_copy);
	tlist<alpha_void, Cnode0<alpha_void, dumb_actptr*> >	bactivities_with_a_selected_ancestor;
	tlist<alpha_void, Cnode0<alpha_void, dumb_actptr*> >::iterator activities_with_a_selected_ancestor(bactivities_with_a_selected_ancestor);
	tlist<alpha_void, dumb_actptr>		list_of_activities_with_their_descendants;

	eval_intfc::get_act_lists().clear_clipboard();

	/*
	 * STEP 1: make sure that no invisible activities are left childless when copyflg == 0.
	 */

	if(!copyflg) {
		slist<alpha_void, dumb_actptr>	copy2(selection_copy);

		while((ptr = copy2.first_node())) {
			lost_child = ptr->payload;
			// If the parent is visible or abstracted we have nothing to worry about
			if((parent_to_revive = lost_child->get_parent()) && parent_to_revive->agent().is_decomposed()) {
				/* This activity is a parent of a child to be deleted. Let's see if it
				 * has any children that are NOT to be deleted: */
				slist<alpha_void, smart_actptr>::iterator kids(parent_to_revive->get_down_iterator());
				int		better_worry = 1;
				smart_actptr*	child;

				while((child = kids())) {
					dumb_actptr*	ptr2;
					if(! (ptr2 = copy2.find(child->BP))) {
						/* Whew, we found one kid that is NOT in the selection
						 * to be deleted; stop worrying. */
						better_worry = 0;
					} else {
						delete ptr2;
					}
				}
				if(better_worry) {
					if(parent_to_revive->is_a_request()) {
						/* Uh-oh: we are trying to delete all the children
						 * of a request. Better not do that!!  */
						theError = "sel_remove: attempting to remove all steps of request ";
						theError << parent_to_revive->identify();
						return apgen::RETURN_STATUS::FAIL;
					}
					ParentsWithDeletedOffspring
						<< new dumb_actptr(parent_to_revive, parent_to_revive);
				}
			}
			// test that we haven't already deleted ptr as ptr2:
			if(copy2.first_node() == ptr) {
				delete ptr;
				assert(copy2.first_node() != ptr);
			}
		}
		/* We now revive the parents whose entire offspring is about to be deleted.
		 * Thanks to the above logic we know that the parent is invisible because
		 * it is in a decomposed state. */

		while((parent_ptr = parents_to_reveal())) {
			parent_to_revive = parent_ptr->payload;
			// FIXES PART OF FR 1.1-8:
			try {
				instance_state_changer	ISC(parent_to_revive);

				ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
				ISC.do_it(NULL);
				// parent_to_revive->hierarchy().make_active(/* please sever */ false);
			} catch(eval_error Err) {
				theError = "sel_remove: caught an error while trying to revive parent ";
				theError <<  parent_to_revive->identify() << "; details:\n";
				theError << Err.msg;
				return apgen::RETURN_STATUS::FAIL;
			} catch(decomp_error Err) {
				theError = "sel_remove: caught an error while trying to revive parent ";
				theError <<  parent_to_revive->identify() << "; details:\n";
				theError << Err.msg;
				return apgen::RETURN_STATUS::FAIL;
			}
		}
	}

	/* STEP 2: remove from the selection any activities with a selected ancestor.
	 * Do this in 3 steps:
	 *	(1) build list of all DESCENDANTS of selected activities,
	 * 	(2) flag all pointers to acts that are descendants of a selected activity, and
	 *	(3) delete them. */

	while((ptr = selected_activities())) {
		actreqnode = ptr->payload;
		actreqnode->hierarchy().recursively_get_ptrs_to_all_descendants(b_copy);
	}
	while((ptr = selected_activities())) {
		actreqnode = ptr->payload;
		if(	b_copy.find(ptr->payload)
			&& !bactivities_with_a_selected_ancestor.find((void *) ptr)
		  ) {
			bactivities_with_a_selected_ancestor << new Cnode0<alpha_void, dumb_actptr*>(actreqnode, ptr);
		}
	}
	while((bptr = activities_with_a_selected_ancestor())) {
		delete bptr->payload;
	}

	/* At this point, selection_copy (List iterator selected_activities) only
	 * contains top-level selected activities. */

	while((ptr = selected_activities())) {
		actreqnode = ptr->payload;

		if(copyflg) {	// we store a COPY of the selected activity
			// copies recursively, so only apply to top-level selected acts
			actreqnode->agent().put_a_copy_of_me_into_clipboard();
		} else {
			instance_state_changer		ISC(actreqnode);

			ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_ON_CLIPBOARD);
			ISC.set_desired_parent_to(NULL);
			try {
				ISC.do_it(NULL);
			} catch(eval_error Err) {
				theError = "sel_remove: caught an error while trying to move ";
				theError <<  actreqnode->identify() << " to the clipboard; details:\n";
				theError << Err.msg;
				return apgen::RETURN_STATUS::FAIL;
			} catch(decomp_error Err) {
				theError = "sel_remove: caught an error while trying to move ";
				theError <<  actreqnode->identify() << " to the clipboard; details:\n";
				theError << Err.msg;
				return apgen::RETURN_STATUS::FAIL;
			}
		}
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

Cstring ACT_exec::extractHopper(const Cstring &Given) {
	const char	*s = *Given;
	int		num = 0;
	static char	theNumber[40];

	while(isdigit(*s)) {
		s++;
		num++; }
	if(!num) {
		return Cstring();
	}
	if(strncmp(s, "Hopper:", strlen("Hopper:"))) {
		return Cstring();
	}
	strncpy(theNumber, *Given, num);
	strcpy(theNumber + num, "Hopper");
	return Cstring(theNumber);
}


	/*
	* Used to be a static method; now non-static.
	*
	* For Hopper design, we need to address a number of painful issues...
	*
	* 	1. consistency with old usage. When pasting a bunch of instances
	* 	   to a different legend we need to organize the new legends carefully.
	*
	* 	   What makes the most sense is to use a list ordered by increasing
	* 	   legend index; get min and max as cleanly as you can, then shift in
	* 	   a sensible way. This was never right in APGEN (old AR) so a cleanup
	* 	   would be welcome anyway.
	*
	* 	2. do we require an activity type definition when we stick something
	* 	   in the hopper? That seems sensible from a scheduling/AI planning
	* 	   perspective, but runs contrary to APGEN's laissez-faire philosophy.
	* 	   The only reason for requiring a type is that we would then know
	* 	   where to to send the activity being pasted. But we have a ready
	* 	   solution: paste it where the user clicked!
	*
	* I am inclined to making the following changes:
	*
	* 	1. No longer use the IDs, which are redundant. Always use a COPY first,
	* 	   which will populate the paste buffer.
	*
	* 	2. We are dealing with 3 types of ACT_sys'es now:
	*
	* 		- regular ACT_sys
	* 		- scheduling ACT_sys ('list of things to do'), also used by Europa.
	*		  It seems that we would want to have maximum compatibility between
	*		  the 2 cases (scheduling and Europa)
	* 		- defining ACT_sys, that can be used for cyclic (or general activity
	* 		  type definition)
	*
	* 	3. The legend-re-organizing business ONLY makes sense when pasting
	* 	   activities from one type of ACT_sys to one of a similar type
	* 	   (i. e. NOT a hopper). This suggests that we really need a subclass
	* 	   of ACT_sys with a "is_hopper()" method. The paste_activity() method
	* 	   needs to check whether the source and target ACT_sys'es are hopper(s)
	* 	   or not. For consistency we need the
	*
	* 	   	HOMOGENEOUS PASTE RULES:
	*
	* 	   	- all source activities must belong an ACT_sys
	* 	   	  of a given type (either regular or Hopper)
	*
	* 	   	- when the paste is homogeneous (from ACT_sys to ACT_sys or Hopper to Hopper)
	* 	   	  legend shifting is done based on 'geometry' (location). This might be
	* 	   	  rather confusing when shifting or attempting to shift activity instances
	* 	   	  that have highly structured rules associated with them, however; I am
	* 	   	  tempted to exclude those from legend shifts in all cases. Will do this
	* 	   	  as a later refinement.
	*
	* 	   	- when the paste is from an ACT_sys to a Scheduling/Planning Hopper (in
	* 	   	  the following I'll just say 'scheduling', which shall include the case
	* 	   	  of Europa timelines as well) to a regular ACT_sys, there shall be rules
	* 	   	  available for scheduling all activities in the Scheduling Hopper.
	*
	* 	   	  This goes together with the following rule: when the paste is TO a
	* 	   	  scheduling Hopper, all activities shall have a scheduling algorithm
	* 	   	  available (either through OKtoSchedule() or through Europa)
	*
	* 	   	- when the paste is from an ACT_sys to a Type Definition Hopper, anything goes,
	* 	   	  i. e., there is no restriction on the type of instance that can be pasted
	* 	   	  this way.
	*
	* As I am thinking about all this, it is now clear that we need more than one COPY buffer.
	* Among other things, this would allow us to indicate, as part of the buffer structure,
	* where the copied activities came from.
	*
	* SECOND ITERATION OF THESE COMMENTS:
	*
	* In an object-oriented design we should turn the static theLegends list into a class method, getLegend(),
	* which could return a reference to a static list or to a class member as appropriate.
	*
	* However in the interest of time we are reluctant to invest time into modifying the status of theLegend.
	* We will therefore use a shortcut: the 'message' indicating that the desired target is a Hopper shall be
	* that the legend name string starts with the pattern
	*
	* 	<legend name> = <hopper-name>:<normal-looking legend name>
	*
	* where for added consistency with Europa (ask Paul Morris about this) <hopper-name> has the form
	*
	* 	<hopper-name> = <number><nice-looking string such as "Hopper">
	*
	* Along the same lines, we will not subclass the Hopper right now; we'll use a method (isHopper()) that
	* returns a constant Cstring reference that is either "" or the hopper name.
	*
	* FOR THE SHORT TERM: if this is not a homogeneous move, force the paste to target only one legend at
	* a time.
	*/

void ACT_exec::paste_activity(
		const Cstring		&start_as_a_string,
		const Cstring		&rawDesiredLegend,
		bool			schedule_flag,
		bool			please_delete,
		List			&top_level_IDs
		) {
	LegendObject		*dslegdnode, *old_legend, *new_legend;
	int			index_of_target_legend_for_first_act = 0;
	long			index_of_source_legend_for_first_act, i, shift_in_legend_index = 0L;
	LegendObject		*ldef;
	Pointer_node		*ptr;
	ActivityInstance*		actreqnode;
	Cstring			legend_name = rawDesiredLegend;
	Cstring			target_hopper_name = ACT_exec::extractHopper(legend_name);
	status_aware_iterator	pending_acts(eval_intfc::get_act_lists().get_clipboard_iterator());
	List			PendingPointers;
	List_iterator		pending_pointers(PendingPointers);
	CTime_base		pastetime, deltatime;
	Cstring			errmsg;

	if(string_to_valid_time(start_as_a_string, pastetime, errmsg) != apgen::RETURN_STATUS::SUCCESS) {
		Cstring		errmsg("ACT_exec::paste_activity: error in start time expr. ");

		throw(eval_error(errmsg));
	}


	int			minvid = 0, maxvid = 0;

	// didn't use to be necessary in gui mode; however, when parsing erroneous commands
	// it may be that no activities were 'selected':
	if(!eval_intfc::get_act_lists().get_clipboard_length()) return;

	/* Hopper implementation (2): is this necessary? ACT_exec::PendingActvities
	 * is a blist built with compare_Time_nodes. The purpose of order() is to make
	 * sure that the linked list order agrees with chronological order, so that
	 * the 'head' of the list can be used to figure out timing and legend for
	 * every other activity in the clipboard. */
	eval_intfc::get_act_lists().order_clipboard();

	while((actreqnode = pending_acts())) {
		if(!actreqnode->get_parent()) {
			if(!PendingPointers.get_length()) {
				deltatime = pastetime - actreqnode->getetime();
				// Find index of source. NOTE: the clipboard is not empty (we checked.)
				ldef = actreqnode->legend().get_legend_object();
				index_of_source_legend_for_first_act = ldef->get_index();
			}
			PendingPointers << new Pointer_node(actreqnode, NULL);
		}
	}

	// compute minvid, maxvid
	bool first_iteration = true;
	while((ptr = (Pointer_node *) pending_pointers())) {
		actreqnode = (ActivityInstance *) ptr->get_ptr();
		ldef = actreqnode->legend().get_legend_object();
		if(first_iteration) {
			first_iteration = false;
			// Remember that (if I'm right) theLegends' linked list has the legends
			// in order of appearance in the activity display.
			minvid = ldef->get_index();
			maxvid = minvid;
		} else {
			// Remember that (if I'm right) theLegends' linked list has the legends
			// in order of appearance in the activity display.
			int		candidate = ldef->get_index();

			minvid = candidate > minvid ? minvid : candidate;
			maxvid = candidate < maxvid ? maxvid : candidate;
		}
	}

	// Find index of target
	if((dslegdnode = (LegendObject *) Dsource::theLegends().find(legend_name))) {
		index_of_target_legend_for_first_act = dslegdnode->get_index();
	} else {
		errmsg << "ACT_exec::paste_activity: legend ";
		errmsg << legend_name << " does not exist. \"This should not happen.\"";
		throw(eval_error(errmsg));
	}


	shift_in_legend_index = index_of_target_legend_for_first_act - index_of_source_legend_for_first_act;

	if(shift_in_legend_index < 0 && (minvid + shift_in_legend_index) < 0) {
		shift_in_legend_index = - minvid;
	} else if (shift_in_legend_index > 0 && (maxvid + shift_in_legend_index) >= Dsource::theLegends().get_length()) {
		shift_in_legend_index = Dsource::theLegends().get_length() - maxvid - 1;
	}

	// We don't have to worry about inter-sys transfer; in all cases, we will
	// use the type definition on an activity-by-activity basis

	/*
	* DON'T USE THIS: moving start time messes up the list
	* while((actreqnode = (ACT_req *) pending_acts()))
	*
	*/
	// apgenDB::ClearSelectedActivities();
	while((ptr = (Pointer_node *) pending_pointers())) {
		actreqnode = (ActivityInstance *) ptr->get_ptr();

		// Only paste the top-level activities in the clipboard
		if(!actreqnode->get_parent()) {
			ActivityInstance		*acteditnode;

			if(please_delete) {
				acteditnode = actreqnode;
			} else {
				acteditnode = actreqnode->agent().put_a_copy_of_me_into_clipboard();
			}
			old_legend = actreqnode->legend().get_legend_object();
			i = old_legend->get_index();

			new_legend = (LegendObject *) Dsource::theLegends()[i + shift_in_legend_index];
			if(new_legend != old_legend) {
				acteditnode->legend().switch_to_legend(new_legend);
			}

			acteditnode->obj()->move_starttime(deltatime, 1);

			acteditnode->set_scheduled(schedule_flag);

			instance_state_changer	ISC(acteditnode);

			ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
			try {
				ISC.do_it(NULL);
			} catch(eval_error Err) {
				Cstring err;
				err = "sel_remove: caught an error while trying to move ";
				err <<  actreqnode->identify() << " to the clipboard; details:\n";
				err << Err.msg;
				throw(eval_error(err));
			} catch(decomp_error Err) {
				Cstring err;
				err = "sel_remove: caught an error while trying to move ";
				err <<  actreqnode->identify() << " to the clipboard; details:\n";
				err << Err.msg;
				throw(eval_error(err));
			}
			top_level_IDs << new String_node(acteditnode->get_unique_id());
			acteditnode->select();
		}
	}
}

void ACT_exec::executeNewDirectives(
		slist<alpha_string, bsymbolnode>&	theTempList,
		int&					layout_directive_count) {
	exec_agent::executeNewDirectives(theTempList, layout_directive_count);
}

void ACT_exec::WriteDirectivesToStream(aoString &fout, IO_APFWriteOptions *options, long top_level_chunk) {
	exec_agent::WriteDirectivesToStream(fout, options, top_level_chunk);
}

void ACT_exec::WriteTypedefs(aoString &fout, long theSectionNode) {
	exec_agent::WriteTypedefs(fout, theSectionNode);
}

ActivityInstance* status_aware_iterator::next() {
	ActivityInstance*	a;
	bool		s;
	while((a = (ActivityInstance*) theIterator.next())) {
		s = a->is_unscheduled();
		if((s & includeUnscheduledActs) | ((!s) & includeScheduledActs)) {
			break;
		}
	}
	return a;
}

ActivityInstance* status_aware_multiterator::next() {
	ActivityInstance*	a;
	bool		s;
	while((a = (ActivityInstance*) theMulti.next())) {
		s = a->is_unscheduled();
		if((s & includeUnscheduledActs) | ((!s) & includeScheduledActs)) {
			break;
		}
	}
	return a;
}
