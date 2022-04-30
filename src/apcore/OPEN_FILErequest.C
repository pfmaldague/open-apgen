#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <dlfcn.h>
#include <aafReader.H>
#include <ActivityInstance.H>
#include "action_request.H"
#include "action_request_client.H"
#include <apcoreWaiter.H>
#include "APrequest_handler.H"
#include "apDEBUG.H"
#include "ACT_exec.H"
#include "AP_exp_eval.H"
#include "APmodel.H"
#include "DB.H"
#include "EventRegistry.H"
#include "fileReader.H"

req_handler	req_intfc::OPEN_FILEhandler = NULL;
req_handler	req_intfc::PURGEhandler = NULL;

stringtlist&	FileNamesWithInstancesAndOrNonLayoutDirectives() {
	static stringtlist v;
	return v; }

stringtlist&	FileNamesWithLayoutDirectives() {
	static stringtlist v;
	return v; }

stringtlist&	FileNamesWithAdaptationData() {
	static stringtlist v;
	return v; }

OPEN_FILErequest::OPEN_FILErequest(compiler_intfc::source_type S, const Cstring& text)
	: Action_request("OPENFILE"),
		the_data_source(S),
		ListOfCommands(this),
		the_file_has_commands(false) {
	if(the_data_source == compiler_intfc::FILE_NAME) {
		filename = text;
	} else if(the_data_source == compiler_intfc::CONTENT_TEXT) {
		file_contents = text;
	}
}

OPEN_FILErequest::OPEN_FILErequest(Cstring& errors, ListOVal* args)
	: Action_request("OPENFILE"),
		ListOfCommands(this),
		the_file_has_commands(false) {
	ArrayElement* name_ae = NULL;
	ArrayElement* content_ae = NULL;

	if((name_ae = args->find("name"))) {
		the_data_source = compiler_intfc::FILE_NAME;
		filename = name_ae->Val().get_string();
	} else if((content_ae = args->find("content"))) {
		the_data_source = compiler_intfc::CONTENT_TEXT;
		file_contents = content_ae->Val().get_string();
	} else {
		errors << "An element named either \"name\" or \"content\" is missing from the args list. ";
	}
}

const Cstring& OPEN_FILErequest::get_command_text() {
	if (commandText.length() > 0) return commandText;

	if(the_data_source == compiler_intfc::CONTENT_TEXT) {
		char C[1005];
		commandText = commandType + " DATA_FROM_STRING\n";
		if(file_contents.length() > 1000) {
			strncpy(C, *file_contents, 1000);
			C[1000] = '\0';
			strcat(C, "...\n");
			commandText << addQuotes(C);
		} else {
			commandText << addQuotes(file_contents);
		}
	} else if(the_data_source == compiler_intfc::FILE_NAME) {
		commandText = commandType + " " + filename;
	}
	return commandText;
}

void OPEN_FILErequest::process_middle(TypedValue*) {

	/* THINGS TO DO:
	 *
	 *   1.	initializeReader() should be renamed because it
	 * 	does a lot of interesting things besides initi-
	 * 	alization.
	 *
	 *   2.	initializeReader() (or another method) should
	 *	return information about the content of the file
	 *	so we don't have to guess later on based on weird
	 *	criteria.
	 *
	 *   3.	Consolidation should only be called after a full
	 *	set of AAFs have been read. Actually, the default
	 *	is that consolidation should NEVER take place -
	 *	except under one circumstance: when an action
	 *	request that exercises the adaptation is about
	 *	to start executing, and the adaptation has been
	 *	updated since the last consolidation.
	 */

	if(req_intfc::OPEN_FILEhandler) {
		req_intfc::OPEN_FILEhandler(this, 0);
	}

	Cstring		charsToParse;
	Cstring		fileToParse;
	compiler_intfc::content_type ContentType = compiler_intfc::UNKNOWN;

	try {
		fileParser::initializeTheReader(
			(the_data_source == compiler_intfc::FILE_NAME)
				? filename : file_contents,
			the_data_source,
			ContentType,
			charsToParse);
		aafReader::current_file() = fileReader::handle_environment(filename);

		Cstring::make_all_strings_permanent = true;
		fileParser::exerciseTheReader(
			the_data_source,
			ContentType,
			ListOfCommands,
			charsToParse);
	} catch(eval_error Err) {
		set_status(apgen::RETURN_STATUS::FAIL);
		add_error(Err.msg);
		Cstring::make_all_strings_permanent = false;
		return;
	}
	Cstring::make_all_strings_permanent = false;

	if(compiler_intfc::CompilationWarnings().get_length()) {
		Cstring		tmp("\n(warnings occurred in ");

		tmp << get_command_text_with_modifier() << ")\n";
		add_error(compiler_intfc::CompilationWarnings());
		add_error(tmp);
	}

	if(fileReader::theFileReader().the_file_had_commands_in_it()) {
		try {
			executeRequests(ListOfCommands); 
		} catch(eval_error(Err)) {
			if(theStatus != apgen::RETURN_STATUS::FAIL) {
				set_status(apgen::RETURN_STATUS::FAIL);
				add_error(Err.msg);
			}
		}
	}

	//
	// This is where the activity displays are updated,
	// assuming the user never did any scrolling:
	//
	if(req_intfc::OPEN_FILEhandler) {
		req_intfc::OPEN_FILEhandler(this, 1);
	}
	Action_request::unconsolidated_files_exist() = true;
	return;
}

FILE_CONSOLIDATErequest::FILE_CONSOLIDATErequest(
		const stringslist&	flist, 
		const Cstring&		cname)
	: Action_request("CONSOLIDATE"),
	filelist(flist),
	name(cname)
	{}

FILE_CONSOLIDATErequest::FILE_CONSOLIDATErequest(Cstring& errors, ListOVal* args)
	: Action_request("CONSOLIDATE") {
	ArrayElement*	name_ae;
	ArrayElement*	list_ae;

	if((name_ae = args->find("name"))) {
		name = name_ae->Val().get_string();
	} else {
		errors << "Element named \"name\" is missing from arg. list. ";
	}
	if((list_ae = args->find("files"))) {
		if(!list_ae->Val().is_array()) {
			errors = "files element is not a list of string values";
		} else {
			ListOVal&		theFiles = list_ae->Val().get_array();
			ArrayElement*		one_file;

			for(int i = 0; i < theFiles.get_length(); i++) {
				one_file = theFiles[i];
				Cstring theValue;
				theValue = one_file->Val().get_string();
				filelist << new emptySymbol(theValue);
			}
		}
	} else {
		errors << "Element named \"files\" is missing from arg. list. ";
	}
}
	
const Cstring& FILE_CONSOLIDATErequest::get_command_text() {
	if(commandText.length() > 0) return commandText;
	commandText = commandType;

	emptySymbol* act;
	act = filelist.first_node();
	if (act) {
		commandText << " ";
		while (act) {
			commandText << act->get_key();
			act = act->next_node();
			if (act) commandText << ","; } }
	commandText << " " << name;
	return commandText; }

void FILE_CONSOLIDATErequest::to_script(Cstring& s) const {
	s << "xcmd(\"CONSOLIDATE\", [\"name\" = " << addQuotes(name)
		<< ", \"files\" = [";
	emptySymbol* act;
	stringslist::iterator iter(filelist);
	bool first = true;
	while((act = iter())) {
		if(first) {
			first = false; }
		else {
			s << ", "; }
		s << addQuotes(act->get_key()); }
	s << "]]);\n"; }

void FILE_CONSOLIDATErequest::process_middle(TypedValue*) {
	if(!filelist.get_length()) {
		add_error("FILE_CONSOLIDATE error: no input files specified.\n");
		set_status(apgen::RETURN_STATUS::FAIL);
		return; }
	ACT_exec::ACT_subsystem().merge_planfiles(
		name,
		filelist);
	if(!FileNamesWithInstancesAndOrNonLayoutDirectives().find(name)) {
		FileNamesWithInstancesAndOrNonLayoutDirectives() << new emptySymbol(name); }
	if(!eval_intfc::ListOfAllFileNames().find(name)) {
		eval_intfc::ListOfAllFileNames() << new emptySymbol(name); }
	return; }

PURGErequest::PURGErequest(bool planOnly)
	: PlanOnly(planOnly),
	  Action_request("PURGE") 
	{}

const Cstring & PURGErequest::get_command_text() {
	if (commandText.length() > 0) return commandText;
	commandText = commandType;

	if(PlanOnly)
		commandText << " \"PLAN\"";

	return commandText; }

void PURGErequest::process_middle(TypedValue*) {
	if(req_intfc::PURGEhandler) {
		req_intfc::PURGEhandler(this, 0); }
	//send and event to the EventRegistry:
	TypedValuePtrVect vect;
	TypedValue *purgePlanOnly = new TypedValue;
	if(PlanOnly)
		*purgePlanOnly = "PLAN";
	else
		*purgePlanOnly = "ALL";

	vect.push_back(purgePlanOnly);
	EventRegistry::Register().PropagateEvent("PURGE", vect);
	//free newed memory
	delete purgePlanOnly;

	if(PlanOnly) {
		//get activity ids
		// StringVect ids;
		// apgenDB::GetActivityIDs(&ids);

		FileNamesWithInstancesAndOrNonLayoutDirectives().clear();
		FileNamesWithLayoutDirectives().clear();
		
		//delete them all
		// for_each(ids.begin(), ids.end(), apgenDB::DeleteActivity);
		ACT_exec::ACT_subsystem().purge_all_acts();
		
		//get all the legends
		// StringVect legNames;
		// apgenDB::GetLegendNames(&legNames);
		//delete them (they should be empty now)
		// for_each(legNames.begin(), legNames.end(), apgenDB::DeleteEmptyLegend);

		// re-initialize globals
		globalData::CreateReservedSymbols();
	} else {
		// do a few extra things:
		ACT_exec::ACT_subsystem().purge();
		eval_intfc::ListOfAllFileNames().clear();
		FileNamesWithInstancesAndOrNonLayoutDirectives().clear();
		FileNamesWithLayoutDirectives().clear();
		FileNamesWithAdaptationData().clear();
	}
	if(req_intfc::PURGEhandler) {
		req_intfc::PURGEhandler(this, 1);
	}
	return;
}
