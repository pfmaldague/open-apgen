#ifndef ACT_EXEC_H
#define ACT_EXEC_H

#include <APbasic.H>
#include <Legends.H>
#include <Rsource.H>


#include 	"json.h"

// Oops... GUI stuff! Well, I need the ACTSQUISHED etc. definitions.
// I wish I could separate that stuff better, but that would require
// providing generic ways to encapsulate derived data in base class
// objects.
#include "UI_dsconfig.h"

void process_activity_instance_name_type_and_id();

class ACT_exec;
class Action_request;
class SCHEDULE_ACTrequest;
class OPEN_FILErequest;
class ActivityInstance;
class Dsource;
class eval_error;
class LegendObject;
class IO_APFWriteOptions;
class IO_ActivityInstance;
class instance_tag_PLD;
typedef Cnode0<alpha_string, instance_tag_PLD> instance_tag;

// several implementations will be provided in the future:
class exec_agent {
public:
	exec_agent(ACT_exec* a)
		: mySource(a) {}
	~exec_agent() {}

	// START OF INTERFACE SPEC

				// utility
	void			clear_the_discrepancy_list();
	void			clear_typeless_activities() { TypelessActivities.clear(); }
	void			dump();
	static void		executeNewDirectives(
					slist<alpha_string, bsymbolnode>&	theTempList,
					int&					layout_directive_count);
	void			handle_typeless_activities();
	void			instantiate_all(
					tlist<alpha_string, instance_tag>& pointers_to_instances);
	void			purge();

	static void		SetActivityInstance(
					ActivityInstance*		act,
					IO_ActivityInstance*		instance,
					const string&			context,
					const apgen::act_visibility_state viz
					);
	bool			WriteActivitiesToStream(
					aoString&			fout,
					const stringslist&		listOfFiles,
					const CTime_base&		startTime,
					const CTime_base&		endTime,
					// contains any instance(s) with illegal status:
					slist<alpha_void, dumb_actptr>&	bad_activities,
					long				top_level_chunk
					);
	static void		WriteDirectivesToStream(
					aoString&		fout,
					IO_APFWriteOptions*	options,
					long			top_level_chunk);

	static apgen::RETURN_STATUS WriteOneActToJsonString(
					ActivityInstance*	act_request,
					Cstring&		subsystem,
					json_object*		new_act);
	static apgen::RETURN_STATUS WriteActInteractionsToJsonStrings(
					bool			singleFile,
					const Cstring&		Filename,
					stringslist&		result,
					Cstring&		errors);
	static apgen::RETURN_STATUS WriteActivitiesToJsonStrings(
					bool			singleFile,
					const Cstring&		Namespace,
					const Cstring&		Directory,
					pairtlist&		result,
					Cstring&		errors);
	static apgen::RETURN_STATUS WriteLegendToJsonString(
					const Cstring&		Namespace,
					const Cstring&		legend,	// used as the Directory
					json_object*		legendActs);
	static apgen::RETURN_STATUS WriteOneResourceToJsonString(
					const Cstring&		Namespace,
					const Cstring&		containerName,
					Cstring&		subsystemName,
					Rsource*		res,
					json_object*		oneTimeline,
					bool			mimic_tms_output = false);
	static apgen::RETURN_STATUS WriteResourceDefsToJsonString(
					const Cstring&		Namespace,
					Cstring&		result,
					Cstring&		errors);
	static apgen::RETURN_STATUS WriteResourcesToJsonStrings(
					bool			singleFile,
					const Cstring&		Namespace,
					stringslist&		result,
					Cstring&		errors);

	static void		WriteTypedefs(
					aoString&		fout,
					long			theSectionNode);
	// END OF INTERFACE SPEC

				//
				// blist of pointer_to_pointers with copy of old durations for instances
				// whose duration does not agree with type formula
				//
	tlist<alpha_void, Cnode0<alpha_void, CTime_base> > act_instances_with_discrepant_durations;

	ACT_exec*		mySource;
	tlist<alpha_void, smart_actptr>	TypelessActivities;
};

//
// The "master" or "executive" class of the Core Activity Subsystem.
//
class ACT_exec {
public:
	static ACT_exec&	ACT_subsystem();
	static ACT_exec*&	ACT_subsystem_ptr();
	static bool		recompute_durations;
	static bool		dur_discrepancy_popup_disabled;
	static slist<alpha_string, bsymbolnode>& Directives();
	static slist<alpha_string, bsymbolnode>& theTemporaryListOfDirectives();

	static void		executeNewDirectives(
					slist<alpha_string, bsymbolnode>&	theList,
					int&					layout_count);

	static void		writeDirectives(int		timeparams,
						int		win_size,
						int		execute_directives,
						aoString	&S );

public:
	ACT_exec();
	~ACT_exec();

	exec_agent	myAgent;
	exec_agent	*execAgent() {
		return &myAgent;
	}

	static void		WriteVersion(
					aoString &,
					long top_level_chunk = 0);
	static void		WriteTypedefs(
					aoString &,
					long top_level_chunk = 0);
        static void		WriteLegendsToStream(
					aoString &, const List &legs, IO_APFWriteOptions *,
					long top_level_chunk = 0);
        static void		WriteDirectivesToStream(
					aoString &,
					IO_APFWriteOptions *,
					long top_level_chunk = 0);
        static bool		WriteActivitiesToStream(
					aoString &fout,
					const stringslist&		listOfFiles,
					const CTime_base&		startTime,
					const CTime_base&		endTime,
					slist<alpha_void, dumb_actptr>&	bad_acts,
					long				top_level_chunk = 0);
	static void		TransferActivitiesToListOfSmartNodes(
					tlist<timeptr<ActivityInstance>,
					smart_actref, ActivityInstance*>& Ordered_downs);

        static int		WriteDataToStream(
					aoString&			as,
					const stringslist&		listOfFiles,
					const List&			legends,
					slist<alpha_void, dumb_actptr>&	bad_acts,
					IO_APFWriteOptions*		options,
					long				top_level_chunk = 0);
        static void		WriteVersionToStream(
					aoString&,
					const char*);
	static void		WriteLegendLayoutToStream(
					aoString&,
					const Cstring &);

	static apgen::RETURN_STATUS WriteActInteractionsToJsonStrings(
					bool			singleFile,
					const Cstring&		Filename,
					stringslist&		result,
					Cstring&		errors);
	static apgen::RETURN_STATUS WriteActivitiesToJsonStrings(
					bool			singleFile,
					const Cstring&		Namespace,
					const Cstring&		Directory,
					pairtlist&		result,
					Cstring&		errors);
	static apgen::RETURN_STATUS WriteGlobalsToJsonStrings(
					map<string, string>&	result,
					Cstring&		errors);
	static apgen::RETURN_STATUS WriteResourceDefsToJsonString(
					const Cstring&		Namespace,
					Cstring&		result,
					Cstring&		errors);
	static apgen::RETURN_STATUS WriteResourcesToJsonStrings(
					bool			singleFile,
					const Cstring&		Namespace,
					stringslist&		result,
					Cstring&		errors);

	// SASF support
        static int		ReleaseClient();
        static int		SendDataToClient(
					slist<alpha_void, dumb_actptr>& bad_acts,
					Cstring &Errs);
        static int		SendDataToClient(
					const stringslist&		listOfFiles,
					const List&			legends,
					slist<alpha_void, dumb_actptr>&	bad_acts,
					Cstring&			Errs);
        static int		SendActivitiesToClient(	const stringslist&	listOfFiles,
					const CTime_base&		startTime,
					const CTime_base&		endTime,
					slist<alpha_void, dumb_actptr>&	bad_acts);

	static Cstring		extractHopper(
					const Cstring &HID);


	// ACT_sys-like interface methods

	static void		addALegend(
					LegendObject *LO);
	static void		reorder_legends_as_per(
					const Cstring &layout_id,
					List &pointers_to_legends);

	// activities
				    // Remove the selected activity requests to a CUT/COPY buffer 
	static apgen::RETURN_STATUS sel_remove(
					const slist<alpha_void, dumb_actptr>& selection_copy,
					const unsigned char		copyflg,
					Cstring&			any_errors);

	//
	// This has been turned into a non-static method so we can use different kinds
	// of ACT-sisses. See notes in ACT_edit.C about this method.
	//
	// Modified so that is now takes an int flag, = 1 to schedule an act, = -1 to unschedule.
	//
	static void		paste_activity(
					const Cstring&			start_time_as_a_string,
					const Cstring&			legend_name,
		       			bool				change_status_to,
					bool				no_copy_in_clipboard,
					List&				list_of_created_IDs);

	static int		number_of_active_selections();
	static int		number_of_selections();
	static bool		there_are_active_selections();
	static bool		there_are_selections();
	static smart_actptr*	find_selection(ActivityInstance* a);

	//
	// inserts dumb act ptrs pointing to active instances
	//
	static void		get_active_selections(
					slist<alpha_void, dumb_actptr>&);

	//
	// inserts a smart act ptr into the selection list
	//
	static void		insert_instance(ActivityInstance* a);

	static void		classinit();
	static void		generate_time_events();

	//
	// returns true if there is one, false otherwise
	//
	static bool		FirstRequestTime(
					CTime_base&,
					int the_print_flag = 0);

	//
	// imported from UI_exec.C:
	//
	static void		enable_scheduling();
	static void		disable_scheduling();
	static void		addNewItem();
	static void		addNewActivity(
					const Cstring &);

	//
	// imported from UI_mainwindow.C:
	//
	static void		displayMessage(
					const Cstring&		windowTitle,
					const Cstring&		messageTitle,
					const Cstring&		message);

	//
	// For inserting a bunch of strings
	//
	static void		displayMessage(
					const Cstring&		windowTitle,
					const Cstring&		messageTitle,
					const stringslist&	messages);

	//
	// Non-static methods
	//
	void			paste_one_instance(
					ActivityInstance*	above,
					ActivityInstance*	a,
					int			leg_shift,
					const CTime_base&	paste_start,
					bool			scheduleflag,
					int&			updateflg);

	void			AAF_do_schedule(
					bool	use_selection_list,
					bool	incremental,
		                       	slist<alpha_void, dumb_actptr>&	top_kids);
	void			AAF_do_unschedule(
					bool	all_flag);

	//
	// The list argument contains optional TOL or XMLTOL output request(s):
	//
	void			AAF_do_model();

	void			purge();
	void			purge_all_acts();
	static void		clean_up_clipboard();
	void			instantiate_all(
				    tlist<alpha_string, instance_tag>& pointers_to_instances);

	void			TransferAllActivityTypes(
				    aoString&,
				    long theSectionNode);
	void			merge_planfiles(
				    const Cstring& newplanname,
				    stringslist& list_of_files);

	//
	// returns 1 if there is one, 0 otherwise
	//
	int			LastRequestTime(
				    CTime_base &);

	//
	// Fills the List with emptySymbols, one per type.
	//
	void			get_all_type_names(
				    stringtlist&);

	void			dump();
	CTime_base		default_req_start_time();
	CTime_base		default_req_span();

	//
	// Data members
	//
protected:
				// List of String_nodes
	List			LegendsWritten;
public:
	tlist<alpha_time, time_actptr, ActivityInstance*> ActivityEnds;
	time_saver		theMasterTime;


	//
	// not very good practice... improve later:
	//
	friend		class UI_mainwindow;
	friend		void Purge_model();
	friend		class ActivityInstance;
	friend		class actAgent;
	friend		class ACT_req_local;
	friend		class EventLoop;
};

#endif
