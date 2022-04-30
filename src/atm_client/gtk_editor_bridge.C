#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "AP_exp_eval.H"	// for most instance-related data and pE_extra
#include "ActivityInstance.H"	// for actAgent
// #include "ActivityType.H"	// for bsymbolnode
#include "ACT_sys.H"		// for legends
#include "action_request.H"	// for EDIT_ACTIVITYrequest
#include "UI_mainwindow.H"
#include "UI_dsconfig.h"	// for NUM_OF_COLORS etc.
#include "APerrors.H"		// for error::Post()
#include "apcoreWaiter.H"	// for error::Post()
#include "fileReader.H"
#include "aafReader.H"

#ifdef GTK_EDITOR

#include	"gtk_bridge.H"

bool		eS::panel_active = false;
bool		eS::panel_requested = false;
bool		glS::panel_active = false;
bool		glS::panel_requested = false;
bool		xS::panel_active = false;
bool		xS::panel_requested = false;

bool		eS::gtk_editor_data_available = false;
bool		eS::gtk_editor_requests_redetailing = false;
bool		eS::gtk_editor_requests_error_display = false;

thread*		gS::theGtkThread = NULL;

// The purpose of this file is to establish an interface bridge between
// the APGEN core, which uses traditional APGEN core classes, and the
// gtk editor, which exclusively uses the STL and the GTK-- toolkit.
//
// The rule for this file is that we take an ACT_req (the one to be edited),
// we extract all items of interest, and we then stuff these same items into
// generic STL structures. The design constraint is that the gtk editor needs
// to know nothing about the traditional APGEN core classes, thus decoupling
// editor development completely from core development.
//

			// in main.C:
extern RGBvaluedef	Patterncolors[];

			// in ActivityType.C:
extern void		print_one_instruction(
				const char*	theInstructionType,
				pEsys::executableExp* theInstruction,
				aoString&	Stream,
				const char*	leading_blanks);

extern "C" {

	//
	// implemented below:
	//
	int recompute_duration_from_pars(char **new_value, char **any_errors);
	int parameter_list_parser(const char*, char**);
}


//
// in ACT_req.C:
//
extern apgen::DATA_TYPE	consult_act_req_for_param_type(
					const ActivityInstance* ,
					int which_param );
extern int		consult_act_req_for_attribute_value(
					const ActivityInstance* ,
					const Cstring& attr_name ,
					TypedValue &val );
extern int		consult_act_req_for_range(
					const ActivityInstance* theActivityToEdit ,
					int which_param ,
					List& list_of_range_values );

#endif /* ifdef GTK_EDITOR */



//
// Special purpose function to support InSight
//
void		update_parameters_from_panel(
			slist<alpha_int, Cnode0<alpha_int, TypedValue> >& new_params);


#ifdef OBSOLETE
	//
	// Implemented below. This function is called by the
	// real-time synchronization functions in IO_Seqtalk.C.
	// Its purpose is to pull the data from the gtk editor
	// and make it available to the core in the form of an
	// action request.
	//
bool	the_gtk_editor_has_a_request();
#endif /* OBSOLETE */

#ifdef GTK_EDITOR

		//
		// The entry point from the core. Starts up a new pthread with
		// gS::display_gtk_window() as the entry point, after setting up the
		// mutex that will be used as a semaphore to coordinate access
		// to global gtk editor data by the gtk editor thread and by
		// the core thread (when the core thread calls fire_up_some_editor)
		//
void		start_gtk_editor(bool &ok_to_proceed);

		/* Provides assistance to the gtk editor when the user requests
		 * an epoch- or time system-relative view of start time and/or
		 * duration.
		 *
		 * The basic method is "time_to_epoch_relative_string" in UI_activityeditor.C.
		 * That method is not suitable for inclusion in APedit, because it is based on
		 * core headers such as C_string.H, C_list.H and C_time_base.H. We also do not
		 * want to include back-references from APedit to the APGEN core. Our solution
		 * is to implement a C linkage function (not really necessary, but at some point
		 * this might become a sort of low-level standard), a pointer to which is
		 * supplied by the initialization code to the gtk implementation in APedit.
		 * This way, there is no library dependency, and yet the core is able lend its
		 * time-conversion capability to the APedit library.
		 *
		 * Returns 1 if successful, 0 otherwise; call with any_errors set to the address
		 * of a 'const char *' that will point to the error message. */
extern "C" {
	int		time_string_to_epoch_rel_string(
				const char *the_orig_time_string,
				const char *the_epoch_or_time_system,
				const char **the_converted_time,
				const char **any_errors );
	const char*	name_of_color_number(int i);
	const char*	name_of_color_number_no_mutex(int i);

	char*		default_time_system = NULL;
	int duration_string_to_timesystem_rel_string(
			const char *the_orig_time_string,
			const char *the_epoch_or_time_system,
			const char **the_converted_duration,
			double	*the_multiplier,
			const char **any_errors);

	extern char*	gtk_error_to_report;
	extern char*	get_type_def();
	extern char*	get_param_def(int first_time);
} /* extern "C" */

const char* theColorNames[NUM_OF_COLORS];

// We will probably want to access the following from weShouldWork:
static slist<alpha_void, smart_actptr>&	zero_or_one_instance() {
	static slist<alpha_void, smart_actptr>	z;
	return z;
}

static char* theFullTypeDef = NULL;

void print_one_instruction(
	const char* theInstructionType,
	pEsys::executableExp *theInstruction,
	aoString&	Stream,
	const char*	leading_blanks) {
	theInstruction->to_stream(&Stream, strlen(leading_blanks));
}

char* compute_param_definitions() {

	//
	// we grab actual values from the editor's current values, and the definitions from the act. type
	//
	gS::gtk_editor_array&	modified_items(editor_intfc::get_interface().theCurrentValues);
	gS::gtk_editor_array	parms = modified_items[1]->get_array();
	gS::gtk_editor_array::iterator par_it;

	//
	// Note: the constructor doesn't care what the string says.
	// There used to be a typo here, but it didn't matter.
	//
	gS::compact_int		inf("infinity");
	buf_struct		out_string = {NULL, 0, 0};
	aoString		aos;
	vector<string>		paramdefs;
	int			max_val_length = 0;

	//
	// get activity instance
	//
	smart_actptr*		ptr = zero_or_one_instance().first_node();

	if(!ptr) {
		Cstring err = "Activity Instance seems to have been deleted.";
		throw(eval_error(err));
	}

	ActivityInstance*	instance = (ActivityInstance*) ptr->BP;

	//
	// get activity type
	//
	Behavior&	ActType = instance->Object->Task.Type;
	task*		act_task = ActType.tasks[0];

	pEsys::executableExp*	par;
	Cstring tmp;
	for(int i = 0; i < act_task->parameters->statements.size(); i++) {
		par = act_task->parameters->statements[i].object();
		print_one_instruction("parameter", par, aos, "");
		tmp = aos.str();
		paramdefs.push_back(*tmp);
		aos.clear();
	}

	int i_param_max = act_task->parameters->statements.size();
	par_it = parms.begin();
	for(int i = 0; i < i_param_max; i++) {
		Cstring		tmp;
		if(par_it != parms.end()) {
			initialize_buf_struct(&out_string);
			(*par_it)->transfer_to_stream(&out_string, inf);
			tmp << out_string.buf;
			par_it++;
		} else {
			tmp << "???";
		}

		if(tmp.length() > max_val_length) {
			max_val_length = tmp.length();
		}
	}
	par_it = parms.begin();
	for(int i = 0; i < i_param_max; i++) {
		Cstring tmp;
		if(par_it != parms.end()) {
			initialize_buf_struct(&out_string);
			(*par_it)->transfer_to_stream(&out_string, inf);
			tmp << out_string.buf;
			par_it++;
		} else {
			tmp << "???";
		}
		aos << tmp;
		if(i != i_param_max - 1) {
			aos << ",";
		} else {
			aos << " ";
		}
		for(int k = 0; k < max_val_length - tmp.length(); k++) {
		       aos << " ";
		}	       
	}
	return strdup(aos.str());
}

char*
get_type_def() {
	return theFullTypeDef;
}


char*
get_param_def() {
	static char* ret_char = NULL;

	if(ret_char) free(ret_char);
	ret_char = compute_param_definitions();
	return ret_char;
}

#endif /* ifdef GTK_EDITOR */

void start_gtk_editor() {

#ifdef GTK_EDITOR

    static gS::WinObj	editorWinObj = gS::WinObj::EDITOR;


    if(!gS::theGtkThread) {

	// debug
	// cerr << "start_gtk_editor(): creating theGtkThread\n";

	//
	// This is the very first time a GTK panel is requested.
	//
	gS::theGtkThread = new thread(&gS::display_gtk_window, editorWinObj);
    } else {

	//
	// The GTK thread already exists, so we start by locking
	// the gtk mutex:
	//

	// debug
	// cerr << "start_gtk_editor(): locking the gtk mutex...";

	bool must_restart_gtk = false;

	{
	    lock_guard<mutex> lock(*gS::get_gtk_mutex());
	

	    // debug
	    // cerr << " got it.\n";
	    must_restart_gtk = gS::gtk_subsystem_inactive;

	    //
	    // Prevent another request from trying to join:
	    //
	    if(must_restart_gtk) {
		gS::gtk_subsystem_inactive = false;
	    }
	}

	if(must_restart_gtk) {
	    gS::theGtkThread->join();
	    delete gS::theGtkThread;
	    gS::theGtkThread
		= new thread(&gS::display_gtk_window, editorWinObj);
	} else {

	    // debug
	    // cerr << "setting eS::panel_requested\n";

	    eS::panel_requested = true;
	}
    }
    // cerr << "start_gtk_editor(): released the lock.\n";

#endif /* ifdef GTK_EDITOR */

    // debug
    // cerr << "start_gtk_editor(): done.\n";

}

#ifdef GTK_EDITOR

//
// Recursive procedure for building a generic (STL-based) tree
// structure from the lists and arrays in the activity instances'
// list of local symbols.
//
gS::de_facto_array*
gS::handle_array_value(
		const string&		theKey,
		const string&		theParentDescr,
		const list<string>&	theParentRange,
		ListOVal&		AParray,
		bool			describe_children) {
	ArrayElement*	tds;
	list<string>	empty_list;
	bool		the_array_is_a_list = AParray.get_array_type() == TypedValue::arrayType::LIST_STYLE;
	gS::de_facto_array* A = NULL;

	// create a de_facto_array with the given key; we'll fill in the values below
	if(the_array_is_a_list) {
		string	top_descr("(For each element of the list:) ");
		top_descr = top_descr + theParentDescr;
		A = new gS::de_facto_array(
					theKey,
					top_descr,
					empty_list,
					false);
	} else {
		A = new gS::de_facto_array(
					theKey,
					theParentDescr,
					theParentRange,
					AParray.get_array_type() == TypedValue::arrayType::STRUCT_STYLE);
	}

	for(int z = 0; z < AParray.get_length(); z++) {
		tds = AParray[z];
				// key of array element is always shown as a label.
		string		elementLabel(*tds->get_key());
		string		theDescr;

		if(tds->Val().is_array()) {
			A->get_array().push_back(
				handle_array_value( 
					elementLabel,
					"",
					empty_list,
					tds->Val().get_array()));
		} else if(tds->Val().is_string()) {
			if(describe_children) {
				// let's document sasf items
				if(elementLabel == "file") {
					theDescr = "Determines the name of the output SASF file";
				}
			} else if(the_array_is_a_list) {
				A->get_array().push_back(
					new gS::de_facto_string(
						elementLabel,
						theParentDescr,
						theParentRange,
						/* get_string() produces the 'raw' string contents. We need
						 * to encapsulate the contents into the canonical output format. */
						gS::add_quotes_to(string(*tds->Val().get_string()))
					)
				);
			} else {
				A->get_array().push_back(
					new gS::de_facto_string(
						elementLabel,
						theDescr,
						empty_list,
						// get_string() produces the 'raw' string contents. We need
						// to encapsulate the contents into the canonical output format.
						gS::add_quotes_to(string(*tds->Val().get_string()))
					)
				);
			}
		} else {
			Cstring		serialized_value = tds->Val().to_string();
			if(the_array_is_a_list) {
				A->get_array().push_back(
					new gS::de_facto_string(
						elementLabel,
						theParentDescr,
						theParentRange,
						*serialized_value));
			} else {
				A->get_array().push_back(
					new gS::de_facto_string(
						elementLabel,
						theDescr,
						empty_list,
						*serialized_value));
			}
		}
	}
	return A;
}

//
// Given a TypedValue array, provide the information listed below.
// All arguments are computed and returned, except for the last one
// (the prefix) which is provided by the caller.
//
// Arguments:
//
//	name		description
//	----		-----------
//
// 	names		map from path (in the sense of Gtk Tables) to item name
//
// 	types		map from path to array type (only applies to elements 
// 				that are arrays)
//
// 	descriptions	map from path to description
//
// 	ranges		map from path to ranges
//
// 	AParray		the TypedValue array to document
//
// 	prefix		path to the array being documented; this path is a
// 			prefix to those of all the elements it contains
//
gS::de_facto_array*
gS::handle_documented_array(
		map<string, string>&		names,
		map<string, string>&		types,
		map<string, string>&		descriptions,
		map<string, list<string> >&	ranges,
		ListOVal&			AParray,
		const string&			prefix) {
	ArrayElement*			tds;
	list<string>			empty_list;
	gS::de_facto_array*		A = NULL;
	int				elcount = 0;
	char				buf[10];
	string				Key;
	string				Descr;
	list<string>			Range;
	string				Type;
	map<string, string>::iterator	name_iter;
	map<string, string>::iterator	type_iter;
	map<string, string>::iterator	descr_iter;
	map<string, list<string> >::iterator range_iter;

	name_iter = names.find(prefix);
	assert(name_iter != names.end());
	Key = name_iter->second;
	if((descr_iter = descriptions.find(prefix)) != descriptions.end()) {
		Descr = descr_iter->second;
	}
	if((range_iter = ranges.find(prefix)) != ranges.end()) {
		Range = range_iter->second;
	}

	// create a de_facto_array with the given key; we'll fill in the values below
	A = new gS::de_facto_array(
				Key,
				Descr,
				Range,
				true);

	for(int z = 0; z < AParray.get_length(); z++) {
		tds = AParray[z];
				// key of array element is always shown as a label.
		string		elementLabel(*tds->get_key());
		string		theDescr;
		string		newprefix(prefix);
		string		elKey;
		string		elDescr;
		list<string>	elRange;

		sprintf(buf, ":%d", elcount);
		newprefix = newprefix + buf;
		elcount++;
		type_iter = types.find(newprefix);
		// assert(type_iter != types.end());
		if(type_iter == types.end()) {
			continue;
		}
		Type = type_iter->second;
		if(Type == "struct") {
			A->get_array().push_back(
				handle_documented_array(
					names,
					types,
					descriptions,
					ranges,
					tds->Val().get_array(),
					newprefix));
		} else {
			if((name_iter = names.find(newprefix)) != names.end()) {
				elKey = name_iter->second;
			}
			if((descr_iter = descriptions.find(newprefix)) != descriptions.end()) {
				elDescr = descr_iter->second;
			}
			if((range_iter = ranges.find(newprefix)) != ranges.end()) {
				elRange = range_iter->second;
			}
			if(Type == "list") {
				A->get_array().push_back(
					handle_documented_list(
						elKey,
						elDescr,
						elRange,
						tds->Val().get_array()));
			} else if(tds->Val().is_string()) {
				A->get_array().push_back(
					new gS::de_facto_string(
						elKey,
						elDescr,
						elRange,

						//
						// get_string() produces the 'raw' string contents. We need
						// to encapsulate the contents into the canonical output format.
						//
						gS::add_quotes_to(string(*tds->Val().get_string()))));
			} else {
				Cstring		serialized_value = tds->Val().to_string();
				A->get_array().push_back(
					new gS::de_facto_string(
						elKey,
						elDescr,
						elRange,
						*serialized_value));
			}
		}
	}
	return A;
}

gS::de_facto_array*
gS::handle_documented_list(
		const string&		theKey,
		const string&		theParentDescr,
		const list<string>&	theParentRange,
		ListOVal&		AParray) {
	ArrayElement*		tds;
	list<string>		empty_list;
	gS::de_facto_array*	A = NULL;

	//
	// create a de_facto_array with the given key;
	// we'll fill in the values below
	//
	string	top_descr("(For each element of the list:) ");
	top_descr = top_descr + theParentDescr;
	A = new gS::de_facto_array(
				theKey,
				top_descr,
				empty_list,
				false);

	for(int z = 0; z < AParray.get_length(); z++) {
		tds = AParray[z];

		//
		// key of array element is always shown as a label.
		//
		char		buf[40];
		sprintf(buf, "%lu", tds->get_2nd_key());
		string		elementLabel(buf);
		string		theDescr;

		if(tds->Val().is_array()) {
			A->get_array().push_back(
				handle_array_value(
					elementLabel,
					theParentDescr,
					theParentRange,
					tds->Val().get_array()));
		} else if(tds->Val().is_string()) {
			A->get_array().push_back(
				new gS::de_facto_string(
					elementLabel,
					theParentDescr,
					theParentRange,

					//
					// get_string() produces the 'raw' string contents. We need
					// to encapsulate the contents into the canonical output format.
					//
					gS::add_quotes_to(string(*tds->Val().get_string()))));
		} else {
			Cstring		serialized_value = tds->Val().to_string();
			A->get_array().push_back(
				new gS::de_facto_string(
					elementLabel,
					theParentDescr,
					theParentRange,
					*serialized_value));
		}
	}
	return A;
}

extern bool is_a_simple_duration(pEsys::executableExp*);

//
// Used by the core. Fills not only instance-specific structures,
// but also generic components of the gtk editor such as the list
// of epochs/time systems and the list of legends.
//
apgen::RETURN_STATUS
eS::fire_up_gtk_editor(
		Cstring&	any_errors) {
	extern blist		master_id_list;
	blist			set_of_legends(compare_function(
							compare_bstringnodes,
							false));
	List_iterator		the_legends(set_of_legends);
	ListOVal		globs;
	ArrayElement*		tds;
	TypedValue		theValue;
	smart_actptr*		ptr;
	blist			list_of_types(compare_function(
							compare_bstringnodes,
							false));
	List_iterator		list_of_types_iterator(list_of_types);
	bstringnode*		bstrnode;
	list<string>		empty_list;
	slist<alpha_void, smart_actptr>	selection_list;
	int			there_is_a_time_system = 0;
	Cstring			theDefaultTimeSystem("UTC");
	int			k;
	aoString		aos;

	globalData::copy_symbols(globs);
	for(k = 0; k < NUM_OF_COLORS; k++) {
		theColorNames[k] = Patterncolors[k].colorname;
	}

	//
	// We do not check whether the GTK editor is already up;
	// that will be done by the call to start_gtk_editor(),
	// down below. All we do here is populate the interface
	// data we are going to need.
	//

	//
	// need interface function to get time system in use in first selected AD
	//
	there_is_a_time_system = UI_mainwindow::get_default_timesystem(
			theDefaultTimeSystem);

	//
	// Provide APedit code with the handles necessary to perform conversion:
	//
	TimeConversionFunc = time_string_to_epoch_rel_string;
	DurConversionFunc = duration_string_to_timesystem_rel_string;
	DurUpdateFunc = recompute_duration_from_pars;
	ColorFunc = name_of_color_number;

	//
	// To support apgen_param, a gtk panel introduced at the
	// request of the InSight mission
	//
	ParamParsingFunc = parameter_list_parser;

	TypeDefFunc = get_type_def;
	ParamDefFunc = get_param_def;
	if(default_time_system) free(default_time_system);
	default_time_system = strdup(*theDefaultTimeSystem);

	/************************************************************************
	 * STEP 1. Detect gross errors.						*
	 ************************************************************************/

	eval_intfc::get_all_selections(selection_list);
	if(selection_list.get_length() > 1) {
		any_errors = "More than one activity selected." ;
		return apgen::RETURN_STATUS::FAIL;
	} else if(!selection_list.get_length()) {
		any_errors = "No activity selected." ;
		return apgen::RETURN_STATUS::FAIL;
	}


	/************************************************************************
	 * STEP 2. Grab the activity instance.					*
	 ************************************************************************/
	ptr = selection_list.first_node();

	ActivityInstance*	instance = ptr->BP;

	//
	// Need to ascertain that a lingering pointer isn't already present.
	// Oh well, that's what our mutexes are for.
	//
	zero_or_one_instance().clear();
	zero_or_one_instance() << new smart_actptr(instance);

	instance->Object->Type.to_stream(&aos, 0);
	if(theFullTypeDef) {
		free(theFullTypeDef);
	}
	theFullTypeDef = strdup(aos.str());


	/************************************************************************
	 * STEP 3. Load available legends and the actual instance legend	*
	 ************************************************************************/

	ACT_sys::transferAllLegends(set_of_legends);

	//
	// how do you clear a vector? Try:
	//
	editor_intfc::get_interface().theLegends.erase(
		editor_intfc::get_interface().theLegends.begin(),
		editor_intfc::get_interface().theLegends.end());

	//
	// Note that the iterator traverses the list in linked order,
	// not alphabetic order. The legends will therefore be listed
	// in the order that they appear in in the activity display.
	//
	while((bstrnode = (bstringnode *) the_legends())) {
		editor_intfc::get_interface().theLegends.push_back(
			string(*bstrnode->get_key()));
	}
	editor_intfc::get_interface().instance_legend = *instance->get_legend()->get_key();

	/************************************************************************
	 * STEP 3A. Ditto for actual instance Color (note the uppercase C!!!)	*
	 ************************************************************************/

	//
	// Note that the iterator traverses the list in linked order,
	// not alphabetic order. The legends will therefore be listed
	// in the order that they appear in in the activity display.
	//
	Cstring theColor("Dodger Blue");

	try {
		TypedValue	colorval((*instance->Object)["Color"]);
		theColor = colorval.get_string();
	}
	catch(eval_error) {}

	editor_intfc::get_interface().instance_color = *theColor;

	/************************************************************************
	 * STEP 4. Define the type of instance and the instance type.		*
	 ************************************************************************/

	editor_intfc::get_interface().type_label = "Type:";

	editor_intfc::get_interface().type_name = *instance->Object->Task.Type.name;
	pEsys::executableExp*	attr_in_type = NULL;

#ifdef PREMATURE
	if(instance->Object->Task.Type.tasks[0]->prog) {
		attr_in_type = instance->Object->Task.Type.tasks[0]->prog->find("span");
	}

	if(attr_in_type && (!is_a_simple_duration(attr_in_type))) {
		editor_intfc::get_interface().instance_duration_formula = *attr_in_type->get_rhs();
	} else {
		editor_intfc::get_interface().instance_duration_formula = "";
	}
#endif /* PREMATURE */

	editor_intfc::get_interface().instance_duration_formula = "";

	apgen::METHOD_TYPE mt;
	bool		redetail_is_needed = instance->has_decomp_method(mt) &&
					mt == apgen::METHOD_TYPE::NONEXCLDECOMP;
	std::map<std::string, editor_config>::iterator editor_config_map_it
				= editor_intfc::get_default_choices().find(
						editor_intfc::get_interface().type_name);
	if(editor_config_map_it != editor_intfc::get_default_choices().end()) {
		editor_config_map_it->second.redetail_needed = redetail_is_needed;
	} else {
		editor_config	EC(redetail_is_needed, editor_intfc::get_interface().type_name);
		editor_intfc::get_default_choices()[editor_intfc::get_interface().type_name] = EC;
	}


	/************************************************************************
	 * STEP 5. Load the editor with the available activity types (sort of disabled)
	 ************************************************************************/
#ifdef OBSOLETE
	ACT_exec::ACT_subsystem().get_all_type_names(list_of_types);
	editor_intfc::get_interface().theTypes.erase(
		editor_intfc::get_interface().theTypes.begin(),
		editor_intfc::get_interface().theTypes.end());
	while ((bstrnode = (bstringnode *) list_of_types_iterator())) {
		editor_intfc::get_interface().theTypes.push_back(string(*bstrnode->get_key()));
	}
#endif /* OBSOLETE */
	/************************************************************************
	 * STEP 6. Get name, ID, duration, duration formula, description	*
	 ************************************************************************/

	editor_intfc::get_interface().instance_name = *instance->get_key();
	editor_intfc::get_interface().instance_id = *instance->get_unique_id();
							// the '0' argument is to write
							// time systems if applicable:
	editor_intfc::get_interface().instance_start = *instance->write_start_time(0);

	//
	// We need to make this dependent on the choice of a time system:
	//
	editor_intfc::get_interface().instance_duration = *instance->get_timespan().to_string();

	//
	// Get the list of epochs
	//
	editor_intfc::get_interface().theEpochs.erase(
		editor_intfc::get_interface().theEpochs.begin(),
		editor_intfc::get_interface().theEpochs.end());
	editor_intfc::get_interface().theEpochs.push_back("UTC");

	ArrayElement*		epoch_ptr;
	for(int i = 0; i < globs.get_length(); i++) {
		epoch_ptr = globs[i];
		if(globalData::isAnEpoch(epoch_ptr->get_key())) {
			if(!(epoch_ptr->get_key() & "Hopper:")) {
				editor_intfc::get_interface().theEpochs.push_back(*epoch_ptr->get_key());
			}
		}
	}
	for(int i = 0; i < globs.get_length(); i++) {
		epoch_ptr = globs[i];
		if(globalData::isATimeSystem(epoch_ptr->get_key())) {
			if(!(epoch_ptr->get_key() & "Hopper:")) {
				editor_intfc::get_interface().theEpochs.push_back(*epoch_ptr->get_key());
			}
		}
	}

	//
	// get description
	//
	if((*instance->Object)[ActivityInstance::DESCRIPTION].get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
		editor_intfc::get_interface().instance_description =
			*(*instance->Object)[ActivityInstance::DESCRIPTION].get_string();
	} else {
		editor_intfc::get_interface().instance_description = "";
	}

	/************************************************************************
	 * STEP 7. Get remaining symbols in the symbol table of the instance.	*
	 ************************************************************************/

	/* The core editor starts from the official list of attributes and from the parameter list.
	 * We will start from the instance's list of symbols, which also include any local variables
	 * defined in creation, decomposition, modeling, scheduling and destruction sections.
	 *
	 * We will store all symbols (other than duration etc. which have already been captured) as
	 * keyword-value pairs.
	 *
	 * We have a little bit of a dilemma: do we use the nickname or the actual name for
	 * attributes? Tentatively: let's use the actual name.
	 *
	 * Also note that we face the task of constructing parameters of arbitray complexity...
	 */

	//
	// First we clear the gtk_editor_arrays:
	//
	gS::gtk_editor_array& all_items = editor_intfc::get_interface().Everything;
	all_items.erase(all_items.begin(), all_items.end());

	gS::de_facto_array *the_attrs, *the_params, *the_vars;
	gS::ISL* oc;

	the_attrs = new gS::de_facto_array(
					"Attributes",
					"",
					empty_list,
					false);
	oc = new gS::ISL(the_attrs);

	//
	// stl will use a copy constructor but no destructor:
	//
	all_items.push_back(*oc);
	delete oc;

	the_params = new gS::de_facto_array(
					"Parameters",
					"",
					empty_list,
					false);
	oc = new gS::ISL(the_params);
	all_items.push_back(*oc);
	delete oc;

	the_vars = new gS::de_facto_array(
					"Local Variables",
					"",
					empty_list,
					false);
	oc = new gS::ISL(the_vars);
	all_items.push_back(*oc);
	delete oc;
	oc = NULL;

	gS::gtk_editor_array&	attrs(the_attrs->get_array());
	gS::gtk_editor_array&	parms(the_params->get_array());
	gS::gtk_editor_array&	vars(the_vars->get_array());
					
	map<string, int>	already_processed;

	already_processed["span"]	 = 0;
	already_processed["start"]	 = 0;
	already_processed["name"]	 = 0;
	already_processed["color"]	 = 1;	// we don't want "color" in the new symbol table
	already_processed["Color"]	 = 0;
	already_processed["pattern"]	 = 0;
	already_processed["this"]	 = 0;
	already_processed["finish"]	 = 0;
	already_processed["legend"]	 = 0;
	already_processed["description"] = 0;

	//
	// ATTRIBUTES
	//
	// We traverse the instance's list of local symbols and
	// fish out attributes. Note that we need to avoid parameters
	// and internal variables, which will be processed in
	// subsequent 'for' loops.
	//
	ListOVal		copy_of_symbols;


	behaving_base*	obj = instance->Object.object();
	task&	constructor = *instance->Object->Task.Type.tasks[0];

	for(int isym = 0; isym < constructor.get_varinfo().size(); isym++) {
		const Cstring the_key = constructor.get_varinfo()[isym].first;
		bsymbolnode*	bsn;

		//
		// decide what to ignore
		//
		if(	already_processed.find(*the_key) != already_processed.end()) {
			;
		} else {
		    map<Cstring, Cstring>::iterator iter
			    		= aafReader::nickname_to_activity_string().find(the_key);
		    if(iter != aafReader::nickname_to_activity_string().end()
		       && (*obj)[isym].get_type() != apgen::DATA_TYPE::UNINITIALIZED) {

			//
			// We show the attribute name as a label (no quotes),
			// not as a string value.
			//
			string	attrNameAsALabel(*iter->first);
			string	attrDescription;
			gS::indexed_string_or_list* dfs;

			already_processed[*the_key] = 0;
			if(the_key == "plan") {
				attrDescription = "Indicates which plan file this activity belongs to "
						"(\"New\" if newly created)";
			} else if(the_key == "sasf") {
				attrDescription = "Determines how the activity is output to an SASF";
			} else if(the_key == "misc") {
				attrDescription = "Anything goes in here";
			}
			if((*obj)[isym].is_string()) {
				if(the_key == "status") {
					list<string>	statuses;
					statuses.push_back("\"scheduled\"");
					statuses.push_back("\"unscheduled\"");
					dfs = new gS::de_facto_string(
							attrNameAsALabel,
							attrDescription,
							statuses,
							gS::add_quotes_to(
								*(*obj)[isym].get_string()));
					attrs.push_back(dfs);
				} else {
					dfs = new gS::de_facto_string(
							attrNameAsALabel,
							attrDescription,
							empty_list,
							gS::add_quotes_to(
								*(*obj)[isym].get_string()));
					attrs.push_back(dfs);
				}
			} else if((*obj)[isym].is_string()) {
				dfs = gS::handle_array_value(
						attrNameAsALabel,
						attrDescription,
						empty_list,
						(*obj)[isym].get_array());
				attrs.push_back(dfs);
			} else {
				Cstring	temp = (*obj)[isym].to_string();
				dfs = new gS::de_facto_string(
						attrNameAsALabel,
						attrDescription,
						empty_list,
						*temp);
				attrs.push_back(dfs);
			}
		    }
		}
	}

	//
	// PARAMETERS
	//

	int	numParameters = constructor.paramindex.size();
	int	ip;

	for( ip = 0; ip < numParameters; ip++) {
		const Cstring	paramName = constructor.get_varinfo()[constructor.paramindex[ip]].first;
		apgen::DATA_TYPE paramType = constructor.get_varinfo()[constructor.paramindex[ip]].second;
		TypedValue	paramValue = (*obj)[constructor.paramindex[ip]];
		Cstring		paramDesc;
		list<string>	the_range;
		List		list_of_range_values;
		String_node*	N;

		//
		// Parameters are inserted into a Table whose rows can
		// be expanded by the user. Every element in the table
		// has a unique path; the path is a sequence of numbers
		// separater by a colon. The first element in the parameter
		// part of the table is the word "Parameters", which follows
		// the word "Attributes"; the path of Attributes is "0",
		// the path of Parameters is "1". Items one level below
		// parameters, displayed when Parameters is expanded by
		// the user, have a path of the form "1:n" with n is the
		// index of the parameter, starting at 0. For parameters
		// that are arrays, the same method is used for array
		// elements, recursively if needed.
		//
		Cstring		prefix("1:");
		char		buf[10];
		string		string_prefix;

		sprintf(buf, "%d", ip);
		prefix << buf;
		string_prefix = *prefix;

		//
		// We have a parameter. We show the parameter name as
		// a label (no quotes), not as a string value.
		//
		string		paramNameAsALabel(*paramName);
		string		paramDescription(*paramDesc);

		for(	N = (String_node*) list_of_range_values.first_node();
			N;
			N = (String_node*) N->next_node() ) {
			the_range.push_back(string(*N->get_key()));
		}
		already_processed[*paramName] = 0;

		if(paramValue.is_string()) {
			parms.push_back(new gS::de_facto_string(
							paramNameAsALabel,
							paramDescription,
							the_range,
							gS::add_quotes_to(*paramValue.get_string())));
		} else if(paramValue.is_array()) {
			Cstring par_type;

			if(paramValue.get_array().get_array_type() == TypedValue::arrayType::LIST_STYLE) {
				par_type = "list";
			} else if(paramValue.get_array().get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
				par_type = "struct";
			} else {
				// empty list...
				par_type = "empty";
			}

			if(par_type == "struct") {
				map<string, string>		names;
				map<string, string>		types;
				map<string, list<string> >	ranges;
				map<string, string>		descriptions;
				map<string, string>		units;

				instance->theActAgent()->get_struct_param_info(
								ip,
								prefix,
								names,
								types,
								ranges,
								descriptions,
								units,
								paramValue);

#ifdef DEBUG_ONLY
				map<string, string>::iterator names_iter;
				map<string, string>::iterator types_iter;
				map<string, list<string> >::iterator ranges_iter;
				map<string, string>::iterator descriptions_iter;
				map<string, string>::iterator units_iter;

				for(names_iter = names.begin(); names_iter != names.end(); names_iter++) {
					string sindex(names_iter->first);
					cerr << sindex << " -\n";
					cerr << "\tname " << names_iter->second << "\n";
					if((types_iter = types.find(sindex)) != types.end()) {
						cerr << "\ttype " << types_iter->second << "\n";
					}
					if((descriptions_iter = descriptions.find(sindex)) != descriptions.end()) {
						cerr << "\tdescr " << descriptions_iter->second << "\n";
					}
					if((ranges_iter = ranges.find(sindex)) != ranges.end()) {
						list<string>::iterator	it;
						cerr << "\trange:\n";
						for(it = ranges_iter->second.begin(); it != ranges_iter->second.end(); it++) {
							cerr << "\t\t" << *it << "\n";
						}
					}
					if((units_iter = units.find(sindex)) != units.end()) {
						cerr << "\tdescr " << units_iter->second << "\n";
					}
				}
#endif /* DEBUG_ONLY */

				parms.push_back(gS::handle_documented_array(
							names,
							types,
							descriptions,
							ranges,
							paramValue.get_array(),
							string_prefix));
			} else if(par_type == "list") {

				/* we need to create a structure that can hold range
				 * info for the entire array. */
				parms.push_back(gS::handle_documented_list(
							paramNameAsALabel,
							paramDescription,
							the_range,
							paramValue.get_array()));
			} else if(par_type == "empty") {

				/* we need to create a structure that can hold range
				 * info for the entire array. */
				parms.push_back(gS::handle_documented_list(
							paramNameAsALabel,
							paramDescription,
							the_range,
							paramValue.get_array()));
			}
		} else if(paramValue.is_boolean()) {
			Cstring	temp = paramValue.to_string();

			the_range.push_back(string("TRUE"));
			the_range.push_back(string("FALSE"));
			parms.push_back(new gS::de_facto_string(
							paramNameAsALabel,
							paramDescription,
							the_range,
							string(*temp)));
		} else {
			Cstring	temp = paramValue.to_string();
			// debug
			// cerr << "fire_up_gtk_editor: adding param " << paramNameAsALabel
			// 	<< ", value = " << temp << endl;
			parms.push_back(new gS::de_facto_string(
							paramNameAsALabel,
							paramDescription,
							the_range,
							string(*temp)));
		}
	}

#ifdef DISABLED
	// Now fill local variables
	for(	tds = copy_of_symbols.first_node();
		tds;
		tds = tds->next_node() ) {
		// decide what to ignore
		if(	already_processed.find(*tds->get_key()) != already_processed.end()) {
			;
		}
		else {
			// We show the attribute name as a label (no quotes), not as a string value.
			string	varNameAsALabel(*tds->get_key());
			// Need a way to extract descriptions from the type if available...
			string	varDescr;

			already_processed[*tds->get_key()] = 0;
			if(tds->Val().is_string()) {
				vars.push_back(
					new gS::de_facto_string(
						varNameAsALabel,
						varDescr,
						empty_list,
						gS::add_quotes_to( *tds->Val().get_string() ) ) );
			} else if(tds->Val().is_array()) {
				vars.push_back(
					gS::handle_array_value(
						varNameAsALabel,
						varDescr,
						empty_list,
						tds->Val().get_array()));
			} else {
				Cstring	temp = tds->Val().to_string();
				vars.push_back(
					new gS::de_facto_string(
						varNameAsALabel,
						varDescr,
						empty_list,
						*temp));
			}
		}
	}
#endif /* DISABLED */


	// debug
	// cerr << "fire_up_gtk_editor(): prepared data; calling start_gtk_editor.\n";

	//
	// finally
	//
	start_gtk_editor();

	// debug
	// cerr << "fire_up_gtk_editor(): done.\n";
	return apgen::RETURN_STATUS::SUCCESS;
}

extern "C" {

int
recompute_duration_from_pars(
		char **new_value,
		char **any_errors) {
	gS::gtk_editor_array&	modified_items = editor_intfc::get_interface().theCurrentValues ;
	gS::gtk_editor_array	parms = modified_items[1]->get_array();
	gS::gtk_editor_array::iterator i;
	ActivityInstance*		instance ;
	// ActivityType*			activityType;
	smart_actptr*			ptr;
	Cstring				temp;

	// normal case:
	*any_errors = NULL;
	// just in case... Be sure to check!
	*new_value = NULL;
	ptr = zero_or_one_instance().first_node();
	if(!ptr) {
		temp = "Activity Instance seems to have been deleted.";
		*any_errors = (char *) malloc(temp.length() + 1);
		strcpy(*any_errors, *temp);
		return 0;
	}
	instance = ptr->BP;
	// we also need to find the appropriate constructor task
	// NOTE: can't use Task itself, which is const - silly
	task&	constructor = *instance->Object->Task.Type.tasks[0];

	pEsys::executableExp*	attribute_in_type = NULL;
#ifdef PREMATURE
	if(instance->Object->Task.Type.tasks[0]->prog) {
		attribute_in_type = (pEsys::instruction *)
			instance->Object->Task.Type.tasks[0]->prog->find("span");
	}

	int			numParameters = constructor.paramindex.size();
	int			ip;
	parsedExp		Expression;
				// we will store temporary activity values here, not
				// in the original instance since we may throw away
				// our edits:
	behaving_element	be(new behaving_object(constructor));
	be->parent_scope = be.object();

	for(	ip = 0, i = parms.begin();
		ip < numParameters && i != parms.end();
		ip++, i++) {
		const Cstring	paramName = constructor.get_varinfo()[constructor.paramindex[ip]].first;
		const Cstring	paramDesc;
		TypedValue	paramValue = (*instance->Object)[constructor.paramindex[ip]];

		//
		// Note: the constructor doesn't care what the string says.
		// There used to be a typo here, but it didn't matter.
		//
		gS::compact_int	ml("infinity");
		Cstring		edited_value;
		static buf_struct out_string = {NULL, 0, 0};
		List		rangelist;
		parsedExp	vptr;
		apgen::DATA_TYPE paramType = constructor.get_varinfo()[constructor.paramindex[ip]].second;

		initialize_buf_struct(&out_string);
		(*i)->transfer_to_stream(&out_string, ml);
		edited_value = out_string.buf;
		try {
			compiler_intfc::CompileExpression(edited_value, vptr);
			Expression = vptr;
		}
		catch(eval_error Err) {
			temp = "Gtk Editor Error: syntax error in expr. ";
			temp << Expression->to_string() << "; details:\n" << Err.msg << "\n";
			*any_errors = (char *) malloc(temp.length() + 1);
			strcpy(*any_errors, *temp);
			return 0;
		}
		try {
			Expression->eval_expression(instance->Object);
		}
		catch(eval_error Err) {
			temp = "Gtk Editor Error in Evaluation of expression ";
			temp << Expression->to_string() << "; details:\n" << Err.msg << "\n";
			*any_errors = (char *) malloc(temp.length() + 1);
			strcpy(*any_errors, *temp);
			return 0;
		}
		(*be)[constructor.paramindex[ip]] = Expression->get_val();
	}
	/* At this point the symbol table has been updated
	 * to reflect the new parameter values. We should
	 * now be able to compute the updated duration: */
	if(attribute_in_type && !attribute_in_type->get_tree()->is_a_simple_duration()) {
		pEsys::execStack	theStack(
						new execution_context(
							aaf_intfc::EVAL_STRATEGY_NORMAL,
							be,
							constructor.prog.object()),
						false);
		exec_status		eStatus;
		try {
			attribute_in_type->execute(theStack, eStatus);
		}
		catch(eval_error Err) {
			temp << Err.msg;
			*any_errors = (char *) malloc(temp.length() + 1);
			strcpy(*any_errors, *temp);
			return 0;
		}
		// extract time value
		temp = attribute_in_type->get_tree()->get_val().to_string();
		*new_value = (char *) malloc(temp.length() + 1);
		strcpy(*new_value, *temp);
		return 1;
	} else {
		// debug
		// cerr << "!? no span attribute\n";
	}
#endif /* PREMATURE */
	return 0;
}
} /* extern "C" */

void gS::send_unblocking_event_to_motif() {
	UI_mainwindow::send_a_dummy_event_to_unblock_XtAppNextEvent();
}

//
// grab_gtk_editor_data() extracts all data from the generic (STL)
// structures filled by the gtk editor and formats them in the
// form of a list of keyword/value pairs suitable for creating
// an EDIT_ACTIVITYrequest. The call results in such a request
// being created and processed.
//
void eS::grab_gtk_editor_data() {
	gS::gtk_editor_array&		modified_items(editor_intfc::get_interface().theCurrentValues);
	gS::gtk_editor_array&		original_items(editor_intfc::get_interface().Everything);
	gS::gtk_editor_array		attrs = modified_items[0]->get_array();
	gS::gtk_editor_array		parms = modified_items[1]->get_array();
	gS::gtk_editor_array		vars =  modified_items[2]->get_array();
	gS::gtk_editor_array		orig_attrs = original_items[0]->get_array();
	gS::gtk_editor_array		orig_parms = original_items[1]->get_array();
	gS::gtk_editor_array		orig_vars =  original_items[2]->get_array();
	gS::gtk_editor_array::iterator	i, k;
	ActivityInstance*		instance;
	// ActivityType*			activityType;
	smart_actptr*			ptr;
	ArrayElement*			tds;
	pairslist			name_value;
	pEsys::executableExp*		attribute_in_type;
	bool				ok_to_edit_span = true;

	// debug
	// cerr << "grab_gtk_editor_data() START\n";

	ptr = zero_or_one_instance().first_node();
	if(!ptr) {
		// should report an error condition...
		cerr << "?! ATM INTERNAL ERROR in gtk_bridge.C: No pointer in zero_or_one_instance list\n";
		return;
	}
	instance = ptr->BP;
#ifdef PREMATURE
	activityType = ActivityType::activity_types().find(instance->get_typename());
	if(instance->get_typename() != "generic") {
		if(!activityType) {
			// debug
			// cout << "activity type " << instance->get_typename() << " not found; cannot edit type.\n";
			ok_to_edit_span = false;
		}
		else if((attribute_in_type = activityType->attribute_program.find("span"))) {
			if(!attribute_in_type->get_tree()->is_a_simple_duration()) {
				// debug
				// cout << "activity type " << instance->get_typename()
				// 	<< " defined duration attribute as non-simple; cannot edit type.\n";
				ok_to_edit_span = false;
			}
		}
	}
	// debug
	// else {
	// 	cout << "activity type " << instance->get_typename() << " is generic; OK to edit span.\n";
	// 	}

	// debug
	// cout << "ok_to_edit_span: " << ok_to_edit_span << endl;

#endif /* PREMATURE */

	if(editor_intfc::get_interface().instance_name !=  *instance->get_key()) {
		name_value << new symNode("name", editor_intfc::get_interface().instance_name.c_str());
	}
	if(editor_intfc::get_interface().instance_start != *instance->write_start_time(0)) {
		// debug
		// cerr << "Start times are different: orig = " << *instance->write_start_time(0)
		// 	<< "; new = " << editor_intfc::get_interface().instance_start << endl;
		name_value << new symNode("start", editor_intfc::get_interface().instance_start.c_str());
	}
	if(	editor_intfc::get_interface().instance_duration != *instance->get_timespan().to_string()
		&& ok_to_edit_span) {
		// debug
		// cerr << "Durations are different: orig = " << *instance->get_timespan().to_string()
		// 	<< "; new = " << editor_intfc::get_interface().instance_duration << endl;
		name_value << new symNode("span", editor_intfc::get_interface().instance_duration.c_str());
	}
	// debug
	else {
		// debug
		// cerr << "Durations are the same: orig = " << *instance->get_timespan().to_string()
		// 	<< "; new = " << editor_intfc::get_interface().instance_duration << endl;
		;
	}

	//
	// Should do color, pattern here
	//
	TypedValue	theValue;
	Cstring		InstanceDescription;

	if(consult_act_req_for_attribute_value(instance, "description", theValue)) {
		InstanceDescription = theValue.get_string();
	}
	if(editor_intfc::get_interface().instance_description != *InstanceDescription) {
		name_value << new symNode(
					"descr",
					editor_intfc::get_interface().instance_description.c_str() );
	}
	if(editor_intfc::get_interface().instance_legend != *instance->get_legend()->get_key()) {
		name_value << new symNode(
					"legend",
					editor_intfc::get_interface().instance_legend.c_str());
	}
	Cstring		theColor;
	TypedValue	theColorValue;

	try {
		theColorValue = (*instance->Object)["Color"];
	}
	catch(eval_error Err) {

		//
		// debug
		//
		cerr << "?! no color\n";

		theColorValue = "Dodger Blue";
	}
	theColor = theColorValue.get_string();
	if(editor_intfc::get_interface().instance_color != *theColor) {
		int	c_index;

		for(c_index = 0; c_index < NUM_OF_COLORS; c_index++) {
			if(editor_intfc::get_interface().instance_color
				== name_of_color_number_no_mutex(c_index)) {
				break;
			}
		}
		name_value << new symNode(
					"color",
					Cstring(c_index));
	}

	// all we want to do at this point is print the value (without truncation) of
	// each attribute/parameter and pass those values to an action request.

	// For attributes, we need to iterate over local instances in exactly the same way
	// as when we grabbed symbols from the activity.
					
	map<string, int>	already_processed ;

	//
	// NOTE: the r.h.s. values don't matter... All that matters is that each element is defined.
	//
	already_processed["span"] = 1;
	already_processed["start"] = 1;
	already_processed["name"] = 1;
	already_processed["color"] = 1;
	already_processed["Color"] = 0;
	already_processed["pattern"] = 0;
	already_processed["this"] = 0;
	already_processed["finish"] = 1;
	already_processed["legend"] = 1;
	already_processed["description"] = 1;

	//
	// Next we traverse the instance's list of local symbols and only retain whatever
	// qualifies as an attribute:
	//

	behaving_base* obj = instance->Object.object();
	
	i = attrs.begin();
	k = orig_attrs.begin();
	for(int isym = 0; isym < obj->Task.get_varinfo().size(); isym++) {
		const Cstring the_key = obj->Task.get_varinfo()[isym].first;
		// decide what to ignore
		if(already_processed.find(*the_key) != already_processed.end()) {
			;
		} else {
		    map<Cstring, Cstring>::iterator iter
		    		= aafReader::nickname_to_activity_string().find(the_key);
		    if(iter != aafReader::nickname_to_activity_string().end()
		       && (*obj)[isym].get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
			string			attrNameAsAnOfficialName(*iter->second);
			string			edited_value, orig_value;

			//
			// Note: the constructor doesn't care what the string says.
			//
			gS::compact_int	ml("infinity");
			static buf_struct	out_string = {NULL, 0, 0}, out_string2 = {NULL, 0, 0};

			initialize_buf_struct(&out_string);
			initialize_buf_struct(&out_string2);
			// Now we should be able to compare this against the value stored in the gtk interface.
			// (*i) points to an indexed_string_or_list object. Use the transfer_to_stream method
			// to get the ASCII representation after editing. Compare key with that of attribute.
			    already_processed[*the_key] = 1;
			// debug
			// cerr << "grab_gtk_editor_data(): handling attribute " << attrNameAsAnOfficialName
			// 	<< "/" << (*i)->get_key() << endl;
			(*i)->transfer_to_stream(&out_string, ml);
			edited_value = out_string.buf;
			(*k)->transfer_to_stream(&out_string2, ml);
			orig_value = out_string2.buf;
			// debug
			// cerr << "comparing " << orig_value << " vs. " << edited_value << "...\n";
			if(edited_value != orig_value) {
				name_value << new symNode(the_key, edited_value.c_str());
			}
			i++;
			k++;
		    }
		}
	}

	task&	constructor = *instance->Object->Task.Type.tasks[0];
	int	numParameters = constructor.paramindex.size();
	int	ip;

	for(	ip = 0, i = parms.begin(), k = orig_parms.begin();
		ip < numParameters && i != parms.end() && k != orig_parms.end();
		ip++, i++, k++) {
		const Cstring	paramName = constructor.get_varinfo()[constructor.paramindex[ip]].first;
		apgen::DATA_TYPE paramType = constructor.get_varinfo()[constructor.paramindex[ip]].second;
		TypedValue	paramValue = (*obj)[constructor.paramindex[ip]];

		//
		// Note: the constructor doesn't care what the string says.
		//
		gS::compact_int ml("infinity");
		string			edited_value, orig_value;
		static buf_struct	out_string = {NULL, 0, 0}, out_string2 = {NULL, 0, 0};
		List			rangelist;

		initialize_buf_struct(&out_string);
		initialize_buf_struct(&out_string2);

		//
		// We show the parameter name as a label (no quotes), not as a string value.
		//
		string		paramNameAsALabel(*paramName);
		already_processed[*paramName] = 0;
		(*i)->transfer_to_stream(&out_string, ml);
		edited_value = out_string.buf;
		(*k)->transfer_to_stream(&out_string2, ml);
		orig_value = out_string2.buf;
		if(edited_value != orig_value) {
			name_value << new symNode(paramName, edited_value);
		}
	}

	// NOTE: should compute integral values instead

	// Now fill local variables
#ifdef DISABLED_FOR_NOW
	for(	tds = copy_of_symbols.first_node(),
		i = vars.begin();
		tds && i != vars.end();
		tds = tds->next_node(), i++ ) {
		// decide what to ignore
		if(	already_processed.find( *tds->get_key() ) != already_processed.end() ) {
			;
		} else {
				// We show the attribute name as a label (no quotes), not as a string value.
			string	varName( *tds->get_key() );
			already_processed[*tds->get_key()] = 0;
			// debug
			// cerr << "grab_gtk_editor_data(): handling var "
			//      << varName << "/" << (*i)->get_key() << endl ;
		}
	}
#endif /* DISABLED_FOR_NOW */

	if(name_value.get_length()) {

		// debug
		// symNode*		N;
		// pairslist::iterator	nv(name_value);
		// cerr << "Name value pairs:\n";
		// while((N = nv())) {
		// 	cerr << "    " << N->get_key() << " = " << N->payload << endl;
		// }

		EDIT_ACTIVITYrequest*	request = new EDIT_ACTIVITYrequest(
				name_value,
				/* carry_children */ 1,
				instance->get_unique_id() );

		//
		// request will pop up error panel if necessary:
		//
		if(	request->process() == apgen::RETURN_STATUS::SUCCESS
			&& eS::gtk_editor_requests_redetailing ) {
			stringslist	the_list_of_one_name;

			the_list_of_one_name << new emptySymbol( instance->get_unique_id() );
			REGEN_CHILDRENrequest*	other_request;

			other_request = new REGEN_CHILDRENrequest(
						the_list_of_one_name,
						0);  // all
			other_request->process();
		}
	}


	// debug
	// else {
	// 	cerr << "No name-value pairs that represent any kind of change... "
	// 		<< "NOT generating an action request.\n";
	// }

	Cstring msg;
	msg << "Done editing " << instance->get_unique_id()
		<< " with the gtk editor.\n";
	
	UI_mainwindow::setStatusBarMessage(msg);
}

extern "C" {

	// ColorFunc = name_of_color_number;
const char	*name_of_color_number(int i) {
	return name_of_color_number_no_mutex(i);
}

const char	*name_of_color_number_no_mutex(int i) {
#ifdef GUI
	if(GUI_Flag) {
				// in APconfig.C:
	  //extern RGBvaluedef	Patterncolors[];
	smart_actptr*	p = zero_or_one_instance().first_node();
	ActivityInstance*	inst;

	if(	p
		&& (inst = p->BP)
		// && (!inst->is_composite())
		&& i >= 0
		&& i < NUM_OF_COLORS) {
		return Patterncolors[i].colorname;
	}
}
#endif
	return NULL;
}

	// TimeConversionFunc = time_string_to_epoch_rel_string;
int		time_string_to_epoch_rel_string(
			const char *the_orig_time_string,
			const char *the_epoch_or_time_system,
			const char **the_converted_time,
			const char **any_errors ) {
	// make sure these two are persistent:
	static Cstring	convertedTime;
	static Cstring	errorsThatWereFound;

	Cstring		given_time(the_orig_time_string);
	Cstring		epochName(the_epoch_or_time_system);
	int		seconds, milliseconds;
	CTime_base	T;

	/* fist we convert the given string containing time info into a CTime object. */
	if(eval_intfc::string_to_valid_time(given_time, T, errorsThatWereFound) != apgen::RETURN_STATUS::SUCCESS) {
		*any_errors = *errorsThatWereFound;
		return 0;
	}
	if(eval_intfc::time_to_epoch_relative_string(
			T,
			epochName,
			convertedTime,
			errorsThatWereFound) != apgen::RETURN_STATUS::SUCCESS ) {
		*any_errors = *errorsThatWereFound;
		return 0;
	}
	*the_converted_time = *convertedTime;
	return 1;
}

	// DurConversionFunc = duration_string_to_timesystem_rel_string;
int		duration_string_to_timesystem_rel_string(
			const char *the_orig_dur_string,
			const char *the_epoch_or_time_system,
			const char **the_converted_duration,
			double	*the_multiplier,
			const char **any_errors ) {
	// make sure these two are persistent:
	static Cstring	convertedDuration;
	static Cstring	errorsThatWereFound;

	Cstring		given_dur(the_orig_dur_string);
	Cstring		epochName(the_epoch_or_time_system);
	int		seconds, milliseconds;
	CTime_base	T;
	const char	*c = the_orig_dur_string;
	bool		minus_sign = false;

	// 1. detect minus sign
	while(c && *c == ' ') {
		c++;
	}
	if(*c == '-') {
		minus_sign = true;
		c++;
	}
	if(*c == '"') {
		// we have a timesystem
		c++;
		while(c && *c != '"') {
			c++;
		}
		if(*c != '"') {
			errorsThatWereFound = "Incorrect Duration Format.";
			*any_errors = *errorsThatWereFound;
			return 0;
		}
		c++;
		T = CTime_base(c);
		T = T * (*the_multiplier);
		if(minus_sign) {
			T = -T;
		}
	} else {
		T = CTime_base(c);
		if(minus_sign) {
			T = -T;
		}
	}
	if(eval_intfc::duration_to_timesystem_relative_string(
					T,
					epochName,
					convertedDuration,
					*the_multiplier,
					errorsThatWereFound ) != apgen::RETURN_STATUS::SUCCESS )  {
		*any_errors = *errorsThatWereFound;
		return 0;
	}
	*the_converted_duration = *convertedDuration;
	return 1;
}


//
// To support apgen_param, a gtk panel introduced at the
// request of the InSight mission
//
int	parameter_list_parser(
		const char* paramlist,
		char** any_errors) {
	Cstring		text(paramlist);
	slist<alpha_int, Cnode0<alpha_int, parsedExp> > expressions;

	*any_errors = NULL;
	try {
		fileReader::CompileExpressionList(text, expressions);
		/* now we need to evaluate... */
	}
	catch(eval_error Err) {
		Cstring tmp;
		tmp << "parameter_list_parser: compilation error:\n" <<  Err.msg << "\n";
		*any_errors = strdup(*tmp);
		return -1;
	}
	slist<alpha_int, Cnode0<alpha_int, parsedExp> >::iterator	iter(expressions);
	Cnode0<alpha_int, parsedExp>*					c;
	int								i = 0;
	slist<alpha_int, Cnode0<alpha_int, TypedValue> >			values;
	while((c = iter())) {
		try {
			TypedValue	V;
			fileReader::EvaluateSelf(c->payload, V);
			values << new Cnode0<alpha_int, TypedValue>(i, V);
		}
		catch(eval_error Err) {
			Cstring tmp("error parsing parameter ");
			tmp << i << "; error:\n" << Err.msg;
			*any_errors = strdup(*tmp);
			return -1;
		}
		i++;
	}
	try {
		update_parameters_from_panel(values);
	}
	catch(eval_error Err) {
		Cstring tmp;
		tmp << "error evaluating parameters; error:\n" << Err.msg;
		*any_errors = strdup(*tmp);
		return -1;
	}
	return 0;
}

} /* extern "C" */


//
// To support apgen_param, a gtk panel introduced at the
// request of the InSight mission
//
void update_parameters_from_panel(
		slist<alpha_int, Cnode0<alpha_int, TypedValue> >& new_params) {

    //
    // reference to a vector<ISL>
    //
    gS::gtk_editor_array&	all_items = editor_intfc::get_interface().theCurrentValues;
    list<string>		empty_list;

    // get the previous parameters - smart container, so the obj_rep inside points to the same ISL
    gS::ISL			att_container = all_items[0];
    gS::ISL			param_container = all_items[1];
    gS::ISL			var_container = all_items[2];

    all_items.erase(all_items.begin() + 2);
    all_items.erase(all_items.begin() + 1);

    //
    // define a new structure to hold the new parameters
    //
    gS::de_facto_array*	the_params = new gS::de_facto_array(
						"Parameters",
						"",
						empty_list,
						false);
    param_container.reference(the_params);

    //
    // all_items.push_back(att_container);
    //
    all_items.push_back(param_container);
    all_items.push_back(var_container);


    //
    // get_array() returns a reference to a vector<ISL>
    //
    gS::gtk_editor_array&		parms(the_params->get_array());

	//
	// get activity instance
	//
	smart_actptr*	ptr = zero_or_one_instance().first_node();
	if(!ptr) {
		Cstring temp = "Activity Instance seems to have been deleted.";
		throw(eval_error(temp));
	}
	ActivityInstance*	instance = (ActivityInstance*) ptr->BP;

	// get activity type
	// ActivityType*	activityType = ActivityType::activity_types().find(instance->get_typename());
	// if(!activityType) {
	// 	Cstring	err("Activity type ");
	// 	err << instance->get_typename() << " is not defined.";
	// 	throw(eval_error(err));
	// }
	
	task&	constructor = *instance->Object->Task.Type.tasks[0];
	// iterate over parameters
	int	numParameters = constructor.paramindex.size();
	if(numParameters != new_params.get_length()) {
		Cstring	err;
		err << new_params.get_length() << " parameters given, but activity expects "
			<< numParameters;
		throw(eval_error(err));
	}

	slist<alpha_int, Cnode0<alpha_int, TypedValue> >::iterator	new_iter(new_params);
	Cnode0<alpha_int, TypedValue>*					c;

	for(int ip = 0; ip < numParameters; ip++) {
		const Cstring	paramName = constructor.get_varinfo()[constructor.paramindex[ip]].first;
		Cstring		paramDesc;
		TypedValue	paramValue = (*instance->Object)[constructor.paramindex[ip]];
		apgen::DATA_TYPE paramType = constructor.get_varinfo()[constructor.paramindex[ip]].second;
		list<string>	the_range;
		List		list_of_range_values;
		String_node*	N;
		Cstring		prefix("1:");
		char		buf[10];
		string		string_prefix;

		c = new_iter();
		sprintf(buf, "%d", ip);
		prefix << buf;
		string_prefix = *prefix;

		// override activity param values
		paramValue = c->payload;

		/* We have a parameter. We show the parameter name as a label (no quotes),
		 * not as a string value. */

		string		paramNameAsALabel(*paramName);
		string		paramDescription(*paramDesc);

		for(	N = (String_node*) list_of_range_values.first_node();
			N;
			N = (String_node*) N->next_node() ) {
			the_range.push_back(string(*N->get_key()));
		}
		if(paramValue.is_string()) {
			parms.push_back(new gS::de_facto_string(
						paramNameAsALabel,
						paramDescription,
						the_range,
						gS::add_quotes_to(*paramValue.get_string())));
		} else if(paramValue.is_array()) {
			Cstring par_type;

			if(paramValue.get_array().get_array_type() == TypedValue::arrayType::LIST_STYLE) {
				par_type = "list";
			} else if(paramValue.get_array().get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
				par_type = "struct";
			} else {
				// empty list...
				par_type = "empty";
			}
			if(par_type == "struct") {
				map<string, string>		names;
				map<string, string>		types;
				map<string, list<string> >	ranges;
				map<string, string>		descriptions;
				map<string, string>		units;

				instance->theActAgent()->get_struct_param_info(
							ip,
							prefix,
							names,
							types,
							ranges,
							descriptions,
							units,
							paramValue);

				parms.push_back(gS::handle_documented_array(
						names,
						types,
						descriptions,
						ranges,
						paramValue.get_array(),
						string_prefix));
			} else if(par_type == "list") {

				/* we need to create a structure that can hold range
				 * info for the entire array. */
				parms.push_back(gS::handle_documented_list(
						paramNameAsALabel,
						paramDescription,
						the_range,
						paramValue.get_array()));
			} else if(par_type == "empty") {

				/* we need to create a structure that can hold range
				 * info for the entire array. */
				parms.push_back(gS::handle_documented_list(
						paramNameAsALabel,
						paramDescription,
						the_range,
						paramValue.get_array()));
			}
		} else if(paramValue.is_boolean()) {
			Cstring	temp = paramValue.to_string();

			the_range.push_back(string("TRUE"));
			the_range.push_back(string("FALSE"));
			parms.push_back(new gS::de_facto_string(
						paramNameAsALabel,
						paramDescription,
						the_range,
						string(*temp)));
		} else {
			Cstring	temp = paramValue.to_string();
			// debug
			// cout << "fire_up_gtk_editor: adding param " << paramNameAsALabel
			// 	<< ", value = " << temp << endl;
			parms.push_back(new gS::de_facto_string(
						paramNameAsALabel,
						paramDescription,
						the_range,
						string(*temp)));
		}
	}
}

#endif /* ifdef GTK_EDITOR */
