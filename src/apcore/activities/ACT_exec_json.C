#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <ACT_exec.H>
#include "ActivityInstance.H"
#include "aafReader.H"

extern "C" {
#include <concat_util.h>
} // extern "C"

#include <memory>	// for unique_ptr


#ifdef USE_AUTO_PTR
	#define AUTO_PTR_TEMPL auto_ptr
#else
	#define AUTO_PTR_TEMPL unique_ptr
#endif

typedef Cnode0<alpha_string, json_object*>	subsysNode;

apgen::RETURN_STATUS
exec_agent::WriteActInteractionsToJsonStrings(
		bool		singleFile,
		const Cstring&	Filename,
		stringslist&	result,
		Cstring&	errors) {

	json_object*	all_data = json_object_new_object();
	json_object*	act_data = json_object_new_object();
	json_object*	fun_data = json_object_new_object();
	json_object*	abs_res_data = json_object_new_object();
	json_object*	res_data = json_object_new_object();

	map<Cstring, vector<set<Cstring> > >::iterator iter;

	//
	// FOR EACH FILE
	//
	for(int i = 0; i < aafReader::consolidated_files().size(); i++) {
	    parsedExp& one_file = aafReader::consolidated_files()[i];
	    Cstring theFilename = one_file->file;

	    //
	    // First do activities
	    //
	    {

		pEsys::decomp_finder DF(pEsys::decomp_finder::Activities);

		one_file->recursively_apply(DF);

		//
		// FOR EACH PARENT ACTIVITY
		//
		for(	iter = DF.properties.begin();
			iter != DF.properties.end();
			iter++) {
			json_object*	one_act_data = json_object_new_object();

			json_object_object_add(
					one_act_data,
					"file",
					json_object_new_string(*theFilename));

			json_object*	one_act_children = json_object_new_array();
			json_object*	one_act_resources_set = json_object_new_array();
			json_object*	one_act_resources_read = json_object_new_array();
			json_object*	one_act_abs_resources_set = json_object_new_array();
			json_object*	one_act_globals_set = json_object_new_array();
			json_object*	one_act_globals_read = json_object_new_array();
			json_object*	one_act_udef_functions_called = json_object_new_array();
			json_object*	one_act_aaf_functions_called = json_object_new_array();

			set<Cstring>::iterator iter2;
			//
			// FOR EACH CHILD ACTIVITY
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::CHILDREN].begin();
				iter2 != iter->second[pEsys::decomp_finder::CHILDREN].end();
				iter2++) {
				json_object_array_add(
						one_act_children,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH RESOURCE SET
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::RESOURCES_SET].begin();
				iter2 != iter->second[pEsys::decomp_finder::RESOURCES_SET].end();
				iter2++) {
				json_object_array_add(
						one_act_resources_set,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH ABSTRACT RESOURCE CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::ABS_RESOURCES_SET].begin();
				iter2 != iter->second[pEsys::decomp_finder::ABS_RESOURCES_SET].end();
				iter2++) {
				json_object_array_add(
						one_act_abs_resources_set,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH RESOURCE READ
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::RESOURCES_READ].begin();
				iter2 != iter->second[pEsys::decomp_finder::RESOURCES_READ].end();
				iter2++) {
				json_object_array_add(
						one_act_resources_read,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH GLOBAL SET
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::GLOBALS_SET].begin();
				iter2 != iter->second[pEsys::decomp_finder::GLOBALS_SET].end();
				iter2++) {
				json_object_array_add(
						one_act_globals_set,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH GLOBAL READ
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::GLOBALS_READ].begin();
				iter2 != iter->second[pEsys::decomp_finder::GLOBALS_READ].end();
				iter2++) {
				json_object_array_add(
						one_act_globals_read,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH UDEF FUNCTION CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::UDEF_FUNCTIONS_CALLED].begin();
				iter2 != iter->second[pEsys::decomp_finder::UDEF_FUNCTIONS_CALLED].end();
				iter2++) {
				json_object_array_add(
						one_act_udef_functions_called,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH AAF FUNCTION CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::AAF_FUNCTIONS_CALLED].begin();
				iter2 != iter->second[pEsys::decomp_finder::AAF_FUNCTIONS_CALLED].end();
				iter2++) {
				json_object_array_add(
						one_act_aaf_functions_called,
						json_object_new_string(**iter2));
			}


			json_object_object_add(
					one_act_data,
					"children",
					one_act_children);
			json_object_object_add(
					one_act_data,
					"resources set",
					one_act_resources_set);
			json_object_object_add(
					one_act_data,
					"abstract resources called",
					one_act_abs_resources_set);
			json_object_object_add(
					one_act_data,
					"resources read",
					one_act_resources_read);
			json_object_object_add(
					one_act_data,
					"globals set",
					one_act_globals_set);
			json_object_object_add(
					one_act_data,
					"globals read",
					one_act_globals_read);
			json_object_object_add(
					one_act_data,
					"udef functions called",
					one_act_udef_functions_called);
			json_object_object_add(
					one_act_data,
					"AAF functions called",
					one_act_aaf_functions_called);

			json_object_object_add(
					act_data,
					*iter->first,
					one_act_data);
		}
	    }

	    //
	    // Next do functions
	    //
	    {

		pEsys::decomp_finder DF(pEsys::decomp_finder::Functions);

		one_file->recursively_apply(DF);

		//
		// FOR EACH FUNCTION
		//
		for(	iter = DF.properties.begin();
			iter != DF.properties.end();
			iter++) {
			json_object*	one_fun_data = json_object_new_object();

			json_object_object_add(
					one_fun_data,
					"file",
					json_object_new_string(*theFilename));

			json_object*	one_fun_resources_read = json_object_new_array();
			json_object*	one_fun_globals_set = json_object_new_array();
			json_object*	one_fun_globals_read = json_object_new_array();
			json_object*	one_fun_udef_functions_called = json_object_new_array();
			json_object*	one_fun_aaf_functions_called = json_object_new_array();

			set<Cstring>::iterator iter2;

			//
			// FOR EACH RESOURCE READ
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::RESOURCES_READ].begin();
				iter2 != iter->second[pEsys::decomp_finder::RESOURCES_READ].end();
				iter2++) {
				json_object_array_add(
						one_fun_resources_read,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH GLOBAL SET
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::GLOBALS_SET].begin();
				iter2 != iter->second[pEsys::decomp_finder::GLOBALS_SET].end();
				iter2++) {
				json_object_array_add(
						one_fun_globals_set,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH GLOBAL READ
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::GLOBALS_READ].begin();
				iter2 != iter->second[pEsys::decomp_finder::GLOBALS_READ].end();
				iter2++) {
				json_object_array_add(
						one_fun_globals_read,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH UDEF FUNCTION CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::UDEF_FUNCTIONS_CALLED].begin();
				iter2 != iter->second[pEsys::decomp_finder::UDEF_FUNCTIONS_CALLED].end();
				iter2++) {
				json_object_array_add(
						one_fun_udef_functions_called,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH AAF FUNCTION CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::AAF_FUNCTIONS_CALLED].begin();
				iter2 != iter->second[pEsys::decomp_finder::AAF_FUNCTIONS_CALLED].end();
				iter2++) {
				json_object_array_add(
						one_fun_aaf_functions_called,
						json_object_new_string(**iter2));
			}

			json_object_object_add(
					one_fun_data,
					"resources read",
					one_fun_resources_read);
			json_object_object_add(
					one_fun_data,
					"globals set",
					one_fun_globals_set);
			json_object_object_add(
					one_fun_data,
					"globals read",
					one_fun_globals_read);
			json_object_object_add(
					one_fun_data,
					"udef functions called",
					one_fun_udef_functions_called);
			json_object_object_add(
					one_fun_data,
					"AAF functions called",
					one_fun_aaf_functions_called);

			json_object_object_add(
					fun_data,
					*iter->first,
					one_fun_data);
		}
	    }

	    //
	    // Next do abstract resources
	    //
	    {

		pEsys::decomp_finder DF(pEsys::decomp_finder::AbstractResources);

		one_file->recursively_apply(DF);

		//
		// FOR EACH ABSTRACT RESOURCE
		//
		for(	iter = DF.properties.begin();
			iter != DF.properties.end();
			iter++) {
			json_object*	one_abs_res_data = json_object_new_object();

			json_object_object_add(
					one_abs_res_data,
					"file",
					json_object_new_string(*theFilename));

			json_object*	one_abs_res_abs_resources_set = json_object_new_array();
			json_object*	one_abs_res_resources_set = json_object_new_array();
			json_object*	one_abs_res_resources_read = json_object_new_array();
			json_object*	one_abs_res_globals_set = json_object_new_array();
			json_object*	one_abs_res_globals_read = json_object_new_array();
			json_object*	one_abs_res_udef_functions_called = json_object_new_array();
			json_object*	one_abs_res_aaf_functions_called = json_object_new_array();

			set<Cstring>::iterator iter2;

			//
			// FOR EACH RESOURCE SET
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::RESOURCES_SET].begin();
				iter2 != iter->second[pEsys::decomp_finder::RESOURCES_SET].end();
				iter2++) {
				json_object_array_add(
						one_abs_res_resources_set,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH ABSTRACT RESOURCE CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::ABS_RESOURCES_SET].begin();
				iter2 != iter->second[pEsys::decomp_finder::ABS_RESOURCES_SET].end();
				iter2++) {
				json_object_array_add(
						one_abs_res_abs_resources_set,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH RESOURCE READ
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::RESOURCES_READ].begin();
				iter2 != iter->second[pEsys::decomp_finder::RESOURCES_READ].end();
				iter2++) {
				json_object_array_add(
						one_abs_res_resources_read,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH UDEF FUNCTION CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::UDEF_FUNCTIONS_CALLED].begin();
				iter2 != iter->second[pEsys::decomp_finder::UDEF_FUNCTIONS_CALLED].end();
				iter2++) {
				json_object_array_add(
						one_abs_res_udef_functions_called,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH AAF FUNCTION CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::AAF_FUNCTIONS_CALLED].begin();
				iter2 != iter->second[pEsys::decomp_finder::AAF_FUNCTIONS_CALLED].end();
				iter2++) {
				json_object_array_add(
						one_abs_res_aaf_functions_called,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH GLOBAL SET
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::GLOBALS_SET].begin();
				iter2 != iter->second[pEsys::decomp_finder::GLOBALS_SET].end();
				iter2++) {
				json_object_array_add(
						one_abs_res_globals_set,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH GLOBAL READ
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::GLOBALS_READ].begin();
				iter2 != iter->second[pEsys::decomp_finder::GLOBALS_READ].end();
				iter2++) {
				json_object_array_add(
						one_abs_res_globals_read,
						json_object_new_string(**iter2));
			}

			json_object_object_add(
					one_abs_res_data,
					"resources set",
					one_abs_res_resources_set);
			json_object_object_add(
					one_abs_res_data,
					"abstract resources called",
					one_abs_res_abs_resources_set);
			json_object_object_add(
					one_abs_res_data,
					"resources read",
					one_abs_res_resources_read);
			json_object_object_add(
					one_abs_res_data,
					"udef functions called",
					one_abs_res_udef_functions_called);
			json_object_object_add(
					one_abs_res_data,
					"AAF functions called",
					one_abs_res_aaf_functions_called);
			json_object_object_add(
					one_abs_res_data,
					"globals set",
					one_abs_res_globals_set);
			json_object_object_add(
					one_abs_res_data,
					"globals read",
					one_abs_res_globals_read);

			json_object_object_add(
					abs_res_data,
					*iter->first,
					one_abs_res_data);
		}
	    }

	    //
	    // Next do resources
	    //
	    {

		pEsys::decomp_finder DF(pEsys::decomp_finder::Resources);

		one_file->recursively_apply(DF);

		//
		// FOR EACH RESOURCE
		//
		for(	iter = DF.properties.begin();
			iter != DF.properties.end();
			iter++) {
			json_object*	one_res_data = json_object_new_object();

			json_object_object_add(
					one_res_data,
					"file",
					json_object_new_string(*theFilename));

			json_object*	one_res_resources_read = json_object_new_array();
			json_object*	one_res_globals_set = json_object_new_array();
			json_object*	one_res_globals_read = json_object_new_array();
			json_object*	one_res_udef_functions_called = json_object_new_array();
			json_object*	one_res_aaf_functions_called = json_object_new_array();

			set<Cstring>::iterator iter2;

			//
			// FOR EACH RESOURCE READ
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::RESOURCES_READ].begin();
				iter2 != iter->second[pEsys::decomp_finder::RESOURCES_READ].end();
				iter2++) {
				json_object_array_add(
						one_res_resources_read,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH UDEF FUNCTION CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::UDEF_FUNCTIONS_CALLED].begin();
				iter2 != iter->second[pEsys::decomp_finder::UDEF_FUNCTIONS_CALLED].end();
				iter2++) {
				json_object_array_add(
						one_res_udef_functions_called,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH AAF FUNCTION CALLED
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::AAF_FUNCTIONS_CALLED].begin();
				iter2 != iter->second[pEsys::decomp_finder::AAF_FUNCTIONS_CALLED].end();
				iter2++) {
				json_object_array_add(
						one_res_aaf_functions_called,
						json_object_new_string(**iter2));
			}

			//
			// FOR EACH GLOBAL READ
			//
			for(	iter2 = iter->second[pEsys::decomp_finder::GLOBALS_READ].begin();
				iter2 != iter->second[pEsys::decomp_finder::GLOBALS_READ].end();
				iter2++) {
				json_object_array_add(
						one_res_globals_read,
						json_object_new_string(**iter2));
			}

			json_object_object_add(
					one_res_data,
					"resources read",
					one_res_resources_read);
			json_object_object_add(
					one_res_data,
					"udef functions called",
					one_res_udef_functions_called);
			json_object_object_add(
					one_res_data,
					"AAF functions called",
					one_res_aaf_functions_called);
			json_object_object_add(
					one_res_data,
					"globals set",
					one_res_globals_set);
			json_object_object_add(
					one_res_data,
					"globals read",
					one_res_globals_read);

			json_object_object_add(
					res_data,
					*iter->first,
					one_res_data);
		}
	    }
	}

	json_object_object_add(
			all_data,
			"Activities",
			act_data);
	json_object_object_add(
			all_data,
			"Functions",
			fun_data);
	json_object_object_add(
			all_data,
			"Abstract Resources",
			abs_res_data);
	json_object_object_add(
			all_data,
			"Resources",
			res_data);

	const char* the_data_to_output
		= json_object_to_json_string_ext(all_data, JSON_C_TO_STRING_PRETTY);
	result << new emptySymbol(the_data_to_output);
	json_object_put(all_data);

	return apgen::RETURN_STATUS::SUCCESS;
}

void TransferActToJson(
	json_object*	new_act,
	bool		act_is_visible,
	const Cstring&	namestring,
	const Cstring&	parentID,
	const Cstring&	ID,
	const Cstring&	type,
	tvslist&	the_params,
	tvslist&	attributes) {
    tvslist::iterator		atts(attributes), params(the_params);
    json_object*		act_metadata = json_object_new_array();
    json_object*		keyword_value_pair;
    json_object*		kwd_value_pair = NULL;

    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"Visible",
		json_object_new_boolean(act_is_visible));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);

    json_object_object_add(
		new_act,
		"Activity Name",
		json_object_new_string(*namestring));
    kwd_value_pair = json_object_new_object();
    json_object_object_add(
			kwd_value_pair,
			"Parent",
			json_object_new_string(*parentID));
    json_object_array_add(
			act_metadata,
			kwd_value_pair);

    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"ID",
		json_object_new_string(*ID));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);

    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"Type",
		json_object_new_string(*type));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);

    TaggedValue*	tds;
    while((tds = atts())) {
	keyword_value_pair = json_object_new_object();

	//
	// Instead of writing the subsystem attribute,
	// we send its value to the caller, who can
	// then organize the activities into files that
	// are subsystem-specific. There will be one JSON
	// file per subsystem. This helps make sure that
	// the JSON files don't get huge.
	//
	if(tds->get_key() == "subsystem") {
	    continue;
	}

	json_object_object_add(
		keyword_value_pair,
		*tds->get_key(),
		tds->payload.print_to_json()
		);
	json_object_array_add(
		act_metadata,
		keyword_value_pair);
    }

    json_object* act_params = json_object_new_array();
    while((tds = params())) {
	TypedValue&	paramValue(tds->payload);
	Cstring		paramName(tds->get_key());
	json_object*	keyword_value_pair = json_object_new_object();

	json_object_object_add(
		keyword_value_pair,
		*paramName,
		paramValue.print_to_json());

	json_object_array_add(
		act_params,
		keyword_value_pair);
    }

    json_object_object_add(new_act, "Activity Parameters", act_params);
    json_object_object_add(new_act, "Metadata", act_metadata);
}

apgen::RETURN_STATUS
exec_agent::WriteOneActToJsonString(
    ActivityInstance*	act_request,
    Cstring&		Subsystem,
    json_object*	new_act) {
    apgen::act_visibility_state	V;
    bool				act_is_visible = true;

    if(act_request->const_agent().is_decomposed()) {
		act_is_visible = false;
    } else if(act_request->const_agent().is_abstracted()) {
		act_is_visible = false;
    }

    Cstring		namestring = act_request->get_key();

    tvslist		Atts_off, Atts_nick, Params;
    stringslist		Parent, Children;
    Cstring		ID, Type, Name, Legend, theValue;
    CTime_base		temp_time;
    CTime_base		start_value, duration_value, finish_value;

    act_request->export_symbols(
				ID,
				Type,
				Name,
				temp_time,
				Atts_off,
				Atts_nick,
				Params,
				Parent,
				Children,
				V,
				Subsystem);
    Cstring parentID;
    ActivityInstance*	theParent = act_request->get_parent();
    if(theParent) {
	parentID = theParent->get_unique_id();
    } else {
	parentID = "none";
    }

    if(!Subsystem.is_defined()) {
	Subsystem = "Generic";
    }

    TransferActToJson(
	new_act,
	act_is_visible,
	namestring,
	parentID,
	act_request->get_unique_id(),
	Type,
	Params,
	Atts_nick);
    return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS
exec_agent::WriteActivitiesToJsonStrings(
		bool		singleFile,
		const Cstring&	Namespace,
		const Cstring&	Directory,
		pairtlist&	result,
		Cstring&	errors) {
    ActivityInstance*	act_request;
    int			dum_prio = 0;
    AUTO_PTR_TEMPL<status_aware_multiterator>
			the_instances(eval_intfc::get_instance_multiterator(dum_prio, true, false));
    json_object*	global_wrapper;			// = json_object_new_object();
    json_object*	timeline_data;			// = json_object_new_array();
    json_object*	new_act = NULL;
    tlist<alpha_string, subsysNode> subsystem_based_list; // one node for each subsystem
    subsysNode*		acts_for_subsystem = NULL;	// payload is a json array

    if(singleFile) {
	timeline_data = json_object_new_array();
    }
    while((act_request = (ActivityInstance *) the_instances->next())) {
	Cstring		Subsystem;

	new_act = json_object_new_object();

	// what could go wrong?
	WriteOneActToJsonString(act_request, Subsystem, new_act);

	if(singleFile) {
	    json_object_array_add(timeline_data, new_act);
	} else {
	    if(!(acts_for_subsystem = subsystem_based_list.find(Subsystem))) {
		acts_for_subsystem = new subsysNode(Subsystem, json_object_new_array());
		subsystem_based_list << acts_for_subsystem;
	    }
	    json_object_array_add(acts_for_subsystem->payload, new_act);
	}
    }

    /* do not do this:
     * printf("JSON string:\n%s\n", json_object_to_json_string_ext(global_wrapper, JSON_C_TO_STRING_PRETTY));
     * fflush(stdout);
     * write to a file instead: */

    if(singleFile) {
	global_wrapper = json_object_new_object();
	json_object*		metadata_wrapper = json_object_new_object();
	metadata_wrapper = json_object_new_object();

	// TO DO: fill in the metadata_wrapper correctly
	json_object_object_add(metadata_wrapper, "hasObjectName", json_object_new_string(*Directory));
	json_object_object_add(metadata_wrapper, "hasObjectNamespace", json_object_new_string(*Namespace));

	json_object_object_add(global_wrapper, "Timeline Metadata", metadata_wrapper);
	json_object_object_add(global_wrapper, "Timeline Data", timeline_data);
	const char* the_data_to_output = json_object_to_json_string_ext(global_wrapper, JSON_C_TO_STRING_PRETTY);
	result << new symNode("everything", the_data_to_output);
	json_object_put(global_wrapper);
    } else {
	slist<alpha_string, subsysNode>::iterator	subsys_iter(subsystem_based_list);
	while((acts_for_subsystem = subsys_iter())) {
	    global_wrapper = json_object_new_object();

	    json_object_object_add(global_wrapper, "Timeline Data", acts_for_subsystem->payload);
	    const char* the_data_to_output = json_object_to_json_string_ext(global_wrapper, JSON_C_TO_STRING_PRETTY);
	    result << new symNode(acts_for_subsystem->Key.get_key(), the_data_to_output);
	    json_object_put(global_wrapper);
	}
    }
    return apgen::RETURN_STATUS::SUCCESS;
}

/* NOTE: Handling the indexing of resource names inside arrayed resources is
 * tricky because arrayed resource can have more than one dimension.
 *
 * See RCsource::generate_arrayed_resource_names() in AP_exp_eval.C and how it
 * is used inside apcore/res_intfc.C.
 */

/* NOTE: A second tricky issue has to do with the fact that timelines need to
 * be created before they can be inserted. This is not a problem for
 * activities, because activity timelines are native to TMS - the type is
 * already known.
 *
 * For now we will have 2 separate functions, one to generate a JSON string
 * suitable for creating timelines, one suitable for inserting them. */

apgen::RETURN_STATUS
exec_agent::WriteResourceDefsToJsonString(
		const Cstring&	Namespace,
		Cstring& 	result,
		Cstring& 	errors) {
#ifdef GUTTED
	RCsource*				res_cont;
	slist<alpha_string, RCsource>::iterator	rlst(RCsource::resource_containers());
	json_object*				global_wrapper = json_object_new_array();

	while((res_cont = rlst.next())) {
		slist<alpha_void, RESptr>::iterator	iter(res_cont->payload->simple_resources);
		RESptr*					theResTag;
		Cstring					containerName = res_cont->get_key();

		while((theResTag = iter())) {
			Rsource*	res = theResTag->payload;
			Cstring		resourceName = res->name;

			tlist<alpha_time, unattached_value_node, Rsource*>		list_of_values;
			slist<alpha_time, unattached_value_node, Rsource*>::iterator	vals(list_of_values);

			CTime_base		theStart, theSpan;
			bool			get_all_values = true;
			long			the_version;
			apgen::DATA_TYPE	resType = res->get_datatype();
			TypedValue		subsys_tdv;
			TypedValue		interp_tdv;
			Cstring			subsystemName = "GenericSubsystem";
			long			L = 0;
			// char*		userName = getenv("LOGNAME");
			buf_struct		name_space = {NULL, 0, 0};
			stringslist::iterator	index_iterator(res->listOfIndices);
			emptySymbol*		es;

			try {
				interp_tdv = (*res->Object)["interpolation"];
			}
			catch(eval_error) {
			}
			try {
				subsys_tdv = (*res->Object)["subsystem"];
			}
			catch(eval_error) {
			}

			// concatenate(&name_space, "/");
			// concatenate(&name_space, userName);
			// concatenate(&name_space, "/test/");
			concatenate(&name_space, *Namespace);
			if(Namespace[Namespace.length() - 1] != '/') {
				concatenate(&name_space, "/"); }
			if(subsys_tdv.get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
				Cstring	quoted = subsys_tdv.get_string();
				removeQuotes(quoted);
				subsystemName = quoted;
			}
			concatenate(&name_space, *subsystemName);
			while((es = index_iterator())) {
				concatenate(&name_space, "/");
				concatenate(&name_space, *es->get_key());
			}

			// we ignore transitions across possible warning/error levels by passing NULL:
			res->get_values(theStart, theSpan, list_of_values, get_all_values, the_version);
			if((L = list_of_values.get_length())) {
				json_object*		metadata = json_object_new_object();
				json_object*		oneTimeline = json_object_new_object();

				// define resource metadata
				if(resType == apgen::DATA_TYPE::FLOATING) {
					json_object_object_add(
						metadata,
						"hasValueType",
						json_object_new_string("float64_xdr"));
				} else if(resType == apgen::DATA_TYPE::INTEGER || resType == apgen::DATA_TYPE::BOOL_TYPE) {
					json_object_object_add(
						metadata,
						"hasValueType",
						json_object_new_string("integer64_xdr"));
				} else if(resType == apgen::DATA_TYPE::STRING) {
					json_object_object_add(
						metadata,
						"hasValueType",
						json_object_new_string("string_xdr"));
				} else if(resType == apgen::DATA_TYPE::TIME) {
					json_object_object_add(
						metadata,
						"hasValueType",
						json_object_new_string("integer64_xdr"));
					json_object_object_add(
						metadata,
						"hasUnits",
						json_object_new_string("milliseconds since 1970"));
				} else if(resType == apgen::DATA_TYPE::DURATION) {
					json_object_object_add(
						metadata,
						"hasValueType",
						json_object_new_string("integer64_xdr"));
					json_object_object_add(
						metadata,
						"hasUnits",
						json_object_new_string("milliseconds"));
				}
				json_object_object_add(
					metadata,
					"hasTimelineType",
					json_object_new_string("state"));
				json_object_object_add(
					metadata,
					"hasTimeSystem",
					json_object_new_string("UTC"));
				if(interp_tdv.get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
					json_object_object_add(
						metadata,
						"hasInterpolatorType",
						json_object_new_string("linear")); }
				else {
					json_object_object_add(
						metadata,
						"hasInterpolatorType",
						json_object_new_string("constant"));
				}
				json_object_object_add(
					metadata,
					"hasObjectName",
					json_object_new_string(*containerName));
				json_object_object_add(
					metadata,
					"hasObjectNamespace",
					json_object_new_string(name_space.buf));
				json_object_object_add(oneTimeline, "Timeline Metadata", metadata);

				json_object*	timeline_wrapper = json_object_new_object();
				json_object_object_add(timeline_wrapper, "gltl", oneTimeline);
				// add the timeline to the global string
				json_object_array_add(global_wrapper, timeline_wrapper);
			}
			destroy_buf_struct(&name_space);
		}
	}
	/* write to file instead:
	 * printf("JSON string:\n%s\n", json_object_to_json_string_ext(global_wrapper, JSON_C_TO_STRING_PRETTY));
	 * fflush(stdout); */

	const char* the_data_to_output = json_object_to_json_string_ext(global_wrapper, JSON_C_TO_STRING_PRETTY);
	result = the_data_to_output;

	json_object_put(global_wrapper);
#endif /* GUTTED */
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS
exec_agent::WriteLegendToJsonString(
		const Cstring&	Namespace,
		const Cstring&	Legend,		// use this as directory name
		json_object*	legendActs	// json array
		) {
	// find the legend
	LegendObject*	lo = (LegendObject*) Dsource::theLegends().find(Legend);

	if(!lo) {
		// only possible error: nonexistent legend
		return apgen::RETURN_STATUS::FAIL; }
	// grab acts from pointers
	slist<alpha_void, smart_actptr>::iterator	actptrs(lo->ActivityPointers);
	smart_actptr*					bp;
	ActivityInstance*				req;

	while((bp = actptrs())) {
		Cstring		Subsystem;
		json_object*	one_act = json_object_new_object();

		req = bp->BP;
		WriteOneActToJsonString(req, Subsystem, one_act);
		json_object_array_add(legendActs, one_act);
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS
exec_agent::WriteOneResourceToJsonString(
	const Cstring&	Namespace,
	const Cstring&	containerName,
	Cstring&	subsystemName,
	Rsource*	res,
	json_object*	oneTimeline,
	bool		mimic_tms_output
	) {
	Cstring		resourceName = res->name;

	// debug
	// cout << "handling resource " << resourceName << "\n";
	tlist<alpha_time, unattached_value_node, Rsource*>			list_of_values;
	slist<alpha_time, unattached_value_node, Rsource*>::iterator	vals(list_of_values);

	CTime_base		theStart, theSpan;
	bool			get_all_values = true;
	long			the_version;
	apgen::DATA_TYPE	resType = res->get_datatype();
	TypedValue		subsys_tdv;
	long			L = 0;
	// char*		userName = getenv("LOGNAME");
	buf_struct		name_space = {NULL, 0, 0};
	emptySymbol*		es;

	try {
		subsys_tdv = (*res->Object)["subsystem"];
	}
	catch(eval_error) {
	}


	subsystemName = "GenericSubsystem";
	// concatenate(&name_space, "/");
	// concatenate(&name_space, userName);
	// concatenate(&name_space, "/test");
	concatenate(&name_space, *Namespace);
	if(Namespace[Namespace.length() - 1] != '/') {
		concatenate(&name_space, "/");
	}
	if(subsys_tdv.get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
		Cstring	quoted = subsys_tdv.get_string();
		removeQuotes(quoted);
		subsystemName = quoted;
	}
	concatenate(&name_space, *subsystemName);
	for(int z = 0; z < res->indices.size(); z++) {
		concatenate(&name_space, "/");
		concatenate(&name_space, *res->indices[z]);
	}
	concatenate(&name_space, "/");

	// NOTE: we know that res is not an abstract resource
	// we ignore transitions across possible warning/error levels by passing NULL:
	res->get_values(theStart, theSpan, list_of_values, get_all_values, the_version);
	if((L = list_of_values.get_length())) {
		bool*			active = (bool*) malloc(sizeof(bool) * L);
		bool			first = true;
		unattached_value_node*	uvn;
		unattached_value_node*	prev_uvn;
		long			j = 0;
		json_object*		timelineArray = json_object_new_array();
		json_object*		metadata = json_object_new_object();

		// define resource metadata
		json_object_object_add(
			metadata,
			"hasObjectNamespace",
			json_object_new_string(name_space.buf));
		// ALWAYS use the container name. Avoid funny characters which TMS cannot handle.
		json_object_object_add(
			metadata,
			"hasObjectName",
			json_object_new_string(*containerName));

		if(mimic_tms_output) {
			// Additional items requested by Taifun for Timely / MPS Server interface
			TypedValue		interp_tdv;
			try {
				interp_tdv = (*res->Object)["interpolation"];
			}
			catch(eval_error) {
			}
			if(resType == apgen::DATA_TYPE::FLOATING) {
				json_object_object_add(
					metadata,
					"hasValueType",
					json_object_new_string("float64_xdr"));
			}
			else if(resType == apgen::DATA_TYPE::TIME || resType == apgen::DATA_TYPE::DURATION) {
				json_object_object_add(
					metadata,
					"hasValueType",
					json_object_new_string("integer64_xdr"));
			}
			else if(resType == apgen::DATA_TYPE::INTEGER || resType == apgen::DATA_TYPE::BOOL_TYPE) {
				json_object_object_add(
					metadata,
					"hasValueType",
					json_object_new_string("integer64_xdr"));
			}
			else if(resType == apgen::DATA_TYPE::STRING) {
				json_object_object_add(
					metadata,
					"hasValueType",
					json_object_new_string("string_xdr"));
			}
			if(interp_tdv.get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
				json_object_object_add(
					metadata,
					"hasInterpolatorType",
					json_object_new_string("linear"));
			}
			else {
				json_object_object_add(
					metadata,
					"hasInterpolatorType",
					json_object_new_string("constant"));
			}
			json_object_object_add(
				metadata,
				"hasObjectType",
				json_object_new_string("measurement timeline"));
			json_object_object_add(
				metadata,
				"hasTimeSystem",
				json_object_new_string("UTC"));
			json_object_object_add(
				metadata,
				"hasTimelineType",
				json_object_new_string("measurement"));
		}

		json_object_object_add(oneTimeline, "Timeline Metadata", metadata);

		// figure out which time/value pairs should be included ("active")
		while((uvn = vals())) {
			const CTime_base&	timeTag(uvn->Key.getetime());

			active[j] = false;
			if(first) {
				first = false;
			} else {
				if(timeTag != prev_uvn->Key.getetime()) {
					active[j - 1] = true;
				}
			}
			j++;
			if(j == L) {
				active[j - 1] = true;
			}
			prev_uvn = uvn; }
		// now we are ready to create a timeline of unique time/value pairs
		j = 0;
		while((uvn = vals())) {
			if(active[j]) {
				json_object*	insertnode = NULL;
				json_object*	timepoint = json_object_new_object();

				json_object_object_add(
					timepoint,
					"Data Timestamp",
					json_object_new_string(*uvn->Key.getetime().to_string()));
				if(resType == apgen::DATA_TYPE::FLOATING) {
					json_object_object_add(
						timepoint,
						"Data Value",
						json_object_new_double(uvn->get_value().get_double()));
				} else if(resType == apgen::DATA_TYPE::TIME || resType == apgen::DATA_TYPE::DURATION) {
					CTime_base	T(uvn->get_value().get_time_or_duration());
					long		timeval = T.get_seconds() * 1000
								+ T.get_milliseconds();
					json_object_object_add(
						timepoint,
						"Data Value",
						json_object_new_int(timeval));
				} else if(resType == apgen::DATA_TYPE::INTEGER || resType == apgen::DATA_TYPE::BOOL_TYPE) {
					json_object_object_add(
						timepoint,
						"Data Value",
						json_object_new_int(uvn->get_value().get_int()));
				} else if(resType == apgen::DATA_TYPE::STRING) {
					Cstring	quoted = uvn->get_value().get_string();
					// removeQuotes(quoted);
					json_object_object_add(
						timepoint,
						"Data Value",
						json_object_new_string(*addQuotes(quoted)));
				}
				if(!mimic_tms_output) {
					json_object*	insertnode = json_object_new_object();
					json_object_object_add(insertnode, "Insert", timepoint);
					json_object_array_add(timelineArray, insertnode);
				} else {
					json_object_array_add(timelineArray, timepoint);
				}
			}
			j++;
		}
		json_object_object_add(oneTimeline, "Timeline Data", timelineArray);
	}
	destroy_buf_struct(&name_space);
    return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS
exec_agent::WriteResourcesToJsonStrings(
		bool		singleFile,
		const Cstring&	Namespace,
		stringslist&	result,
		Cstring&	errors) {
	return apgen::RETURN_STATUS::SUCCESS;
}
