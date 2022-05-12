#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>


#include <iostream>
#include <algorithm>
#include <sys/stat.h>

#include "apDEBUG.H"
#include "aafReader.H"
#include "action_request_client.H"
#include "ActivityInstance.H"
#include "APcolors.H"
#include "APrequest_handler.H"
#include "apcoreWaiter.H"
#include "CMD_exec.H"		/* includes "action_request.H" */
#include "DB.H"
#include "EventRegistry.H"
#include "ExecStack.H"
#include "fileReader.H"
#include "IO_write.H"
#include "Prefs.H"
#include "RES_def.H"

using namespace std;

// GLOBALS:


//97-12-04 DSG:  (char *) silliness keeps HP10 aCC quiet
RGBvaluedef Patterncolors[ NUM_OF_COLORS ] = {
    {(char *)"Spring Green",   0, 255, 127}, 
    {(char *)"Aquamarine"  , 127, 255, 212}, 
    {(char *)"Cadet Blue"  ,  95, 158, 160}, 
    {(char *)"Sky Blue"    , 135, 206, 235}, 
    {(char *)"Dodger Blue" ,  30, 144, 255}, 
    {(char *)"Purple"      , 160,  32, 240}, 
    {(char *)"Plum"        , 221, 160, 221}, 
    {(char *)"Lavender"    , 230, 230, 250}, 
    {(char *)"Pink"        , 255, 192, 203}, 
    {(char *)"Hot Pink"    , 255, 105, 180}, 
    {(char *)"Violet Red"  , 208,  32, 144}, 
    {(char *)"Orange Red"  , 255,  69,   0}, 
    {(char *)"Salmon"      , 250, 128, 114}, 
    {(char *)"Orange"      , 255, 165,   0}, 
    {(char *)"Khaki"       , 240, 230, 140}, 
    {(char *)"Yellow"      , 255, 255,   0}
};

map<Cstring, int>& standard_colors() {
	static map<Cstring, int> M;
	static bool initialized = false;

	if(!initialized) {
		initialized = true;
    		for ( int loop = 0; loop < NUM_OF_COLORS; loop++ ) {
			M[Patterncolors[loop].colorname] = loop;
		}
	}
	return M;
}

long int find_color_id( const Cstring &colorname ) {
	map<Cstring, int>::const_iterator iter = standard_colors().find(colorname);
	if(iter != standard_colors().end()) {
		return iter->second + 1;
	}
	return -1;
}

bool& Action_request::unconsolidated_files_exist() {
	static bool u = false;
	return u;
}

bool& Action_request::persistent_scripts_enabled() {
	static bool b = false;
	return b;
}

			// in grammar_intfc.C:
extern Cstring		addQuotes(const Cstring &);
extern void		removeQuotes(Cstring & s);

			// in APGEN_Title.C (created by makefile):
extern  char *		APGEN_Title;

			// in validate.C:
extern long		Validating;

bool			Action_request::RAMreport = false;
stringslist		list_of_errors;

req_handler	req_intfc::ABSTRACT_ALLhandler = NULL;
req_handler	req_intfc::ADD_RESOURCEhandler = NULL;
req_handler	req_intfc::CLOSE_ACT_DISPLAYhandler = NULL;
req_handler	req_intfc::CLOSE_RES_DISPLAYhandler = NULL;
req_handler	req_intfc::COMMONhandler = NULL;
req_handler	req_intfc::DELETE_ALL_DESCENDANTShandler = NULL;
req_handler	req_intfc::DETAIL_ALLhandler = NULL;
req_handler	req_intfc::FIND_RESOURCEhandler = NULL;
req_handler	req_intfc::NEW_ACT_DISPLAYhandler = NULL;
req_handler	req_intfc::NEW_HORIZONhandler = NULL;
req_handler	req_intfc::NEW_RES_DISPLAYhandler = NULL;
req_handler	req_intfc::PAUSEhandler = NULL;
req_handler	req_intfc::REMODELhandler = NULL;
req_handler	req_intfc::SCHEDULEhandler = NULL;
req_handler	req_intfc::UNSCHEDULEhandler = NULL;
req_handler	req_intfc::ENABLE_SCHEDULINGhandler = NULL;
req_handler	req_intfc::REMOVE_RESOURCEhandler = NULL;
req_handler	req_intfc::RESOURCE_SCROLLhandler = NULL;
req_handler	req_intfc::SAVE_FILEhandler = NULL;
req_handler	req_intfc::SELECT_RES_LEGENDhandler = NULL;
res_layout_handler		req_intfc::RES_LAYOUThandler = NULL;
act_display_horizon_handler	req_intfc::ACT_DISPLAY_HORIZONhandler = NULL;
display_size_handler		req_intfc::DISPLAY_SIZEhandler = NULL;

			// implemented below
extern apgen::RETURN_STATUS	evaluate_string_arg(
				const Cstring &s,
				TypedValue &result,
				apgen::DATA_TYPE dType,
				Cstring &err_msg);

void Action_request::add_id_to_list(
		const Cstring& x) {
	OPEN_FILErequest*	of = list ? list->Owner : NULL;
	if(of) {
		of->IDs << new emptySymbol(x);
	}
}

level_handler::level_handler() {
	Action_request::script_execution_level++;
}

level_handler::~level_handler() {
	Action_request::script_execution_level--;
}

Cntnr<alpha_string, ActivityInstance*>*	Action_request::find_tag_from_real_or_symbolic_id(
		const Cstring& s) {
	Cstring				id_to_use(s);
	Cntnr<alpha_string, ActivityInstance*>*	the_tag;

	if(id_to_use[0] == '@') {
		// symbolic ID
		OPEN_FILErequest*	of = list ? list->Owner : NULL;
		symNode*		theTag;

		if(!of) {
			Cstring any_errors;
			any_errors << "invalid symbolic ID \"" << id_to_use << "\": not in parsing mode.";
			throw(eval_error(any_errors));
		} else if(!(theTag = of->id_variables.find(id_to_use))) {
			Cstring any_errors;
			any_errors << "invalid symbolic ID \"" << id_to_use << "\": symbol not found.";
			throw(eval_error(any_errors));
		}
		id_to_use = theTag->payload;
	}
	the_tag = aaf_intfc::actIDs().find(id_to_use);
	if(!the_tag) {
		Cstring any_errors;
		any_errors << "no instance found with id = " << id_to_use;
		throw(eval_error(any_errors));
	}
	return the_tag;
}

// second arg defaults to default client.
Action_request::Action_request(const Cstring& cmd)
		: commandType(cmd), 
			  theStatus(apgen::RETURN_STATUS::SUCCESS), 
			  shouldCaptureErrors(false),
			  captured(NULL),
			  CommentInLog(script_execution_level > 0) {
}

Action_request::Action_request(const Action_request& AA)
		: commandType(AA.commandType),
			  commandText(AA.commandText), 
			  theStatus(apgen::RETURN_STATUS::SUCCESS), 
			  shouldCaptureErrors(false),
			  captured(NULL),
			  CommentInLog(script_execution_level > 0) {}

void Action_request::add_error(
		const Cstring& err) {
	local_errors << new emptySymbol(err);
	APcloptions::theCmdLineOptions().ModelErrorsFound = true;
}

void Action_request::add_error(
		stringslist& E) {
	local_errors << E;
	APcloptions::theCmdLineOptions().ModelErrorsFound = true;
}

const Cstring & Action_request::get_command_text() {
	return commandText; }

Cstring Action_request::get_command_text_with_modifier() {
	Cstring commandWithModifier = get_command_text();

	if(get_modifier().length())
		commandWithModifier << " MODIFIER = " << "\"" << get_modifier() << "\"";

	return commandWithModifier; }

void
Action_request::executeRequests(
		slist<	alpha_string,
			Action_request,
			OPEN_FILErequest*>&
			    list_of_commands) {
    Action_request*	AR;
    Action_request*	nextAR;
    int			the_level = 0;
    aoString aos;

    for(    AR = list_of_commands.first_node();
	    AR;
	    AR = nextAR) {
	nextAR = AR->next_node();

	WRITE_TOLrequest*	tolAR    = dynamic_cast<WRITE_TOLrequest*>(   AR);
	WRITE_XMLTOLrequest*    xmltolAR = dynamic_cast<WRITE_XMLTOLrequest*>(AR);

	if(tolAR && !tolAR->xmltol_requests.get_length()) {
	    if(nextAR) {
		WRITE_XMLTOLrequest*    next_xmltolAR = dynamic_cast<WRITE_XMLTOLrequest*>(nextAR);
		if(next_xmltolAR) {

		    //
		    // Skip the XMLTOL request
		    //
		    nextAR = next_xmltolAR->next_node();

		    //
		    // This removes next_xmltolAR from its original list:
		    //
		    tolAR->xmltol_requests << next_xmltolAR;
		} else {

		    //
		    // No XMLTOL request is following
		    //
		    ;
		}
	    }
	} else if(xmltolAR) {
	    if(nextAR) {
		WRITE_TOLrequest*    next_tolAR = dynamic_cast<WRITE_TOLrequest*>(nextAR);
		if(next_tolAR) {

		    //
		    // This removes xmltolAR from its original list:
		    //
		    next_tolAR->xmltol_requests << xmltolAR;

		    //
		    // Skip the XMLTOL request
		    //
		    continue;
		} else {

		    //
		    // No XMLTOL request is following
		    //
		    ;
		}
	    }
	}

	try {

	    if(AR->process() != apgen::RETURN_STATUS::SUCCESS) {
		aos << AR->get_error_message();

		if(!persistent_scripts_enabled()) {
		    // Steve Wissler ISA Z92457
		    break;
		}

	    }
	} catch(eval_error(Err)) {
	    aos << Err.msg;

	    if(!persistent_scripts_enabled()) {
		// Steve Wissler ISA Z92457
		break;
	    }

	}
    }
    list_of_commands.clear();
    Cstring errs = aos.str();
    if(errs.length()) {
	throw(eval_error(errs));
    }
}

void Action_request::set_error_string_to(Cstring* E) {
	shouldCaptureErrors = true;
	captured = E;
}

apgen::RETURN_STATUS Action_request::process() {
	level_handler level;

	try {
	    //
	    // NOTE: common_process_first indents the debug text.
	    //
	    common_process_first( /* keep_going */ );

	} catch(eval_error Err) {
	    Cstring s;
	    s << "Consolidation error:\n" << Err.msg;
	    add_error(s);
	    theStatus = apgen::RETURN_STATUS::FAIL;
	}

	if(theStatus == apgen::RETURN_STATUS::SUCCESS) {
	    if(get_pointers() == apgen::RETURN_STATUS::SUCCESS) {
		TypedValue val;
		try {

		    //
		    // This is the "core" of the proces
		    //
		    process_middle(&val);

		} catch(eval_error Err) {
		    theStatus = apgen::RETURN_STATUS::FAIL;
		    add_error(Err.msg);
		}
	    }
	}

	//
	// This is where errors get logged:
	//
	common_process_last();

	//
	// NOTE: captured was set to error_string,
	// so any errors were captured.
	//
	return theStatus;
}


void Action_request::common_process_first() {

	OPEN_FILErequest*	ofr = dynamic_cast<OPEN_FILErequest*>(this);
	QUITrequest*		qr = dynamic_cast<QUITrequest*>(this);
	REMODELrequest*		rem = dynamic_cast<REMODELrequest*>(this);
	SCHEDULE_ACTrequest*	sch = dynamic_cast<SCHEDULE_ACTrequest*>(this);
	UNSCHEDULE_ACTrequest*	unsch = dynamic_cast<UNSCHEDULE_ACTrequest*>(this);
	BUILDrequest*		build = dynamic_cast<BUILDrequest*>(this);

	bool			consolidation_error = false;
	Cstring			consolidation_msg("\n");
	if(ofr || qr || build) {

	    //
	    // We quit if requested, or keep reading files; we will
	    // consolidate when we are done. Unless the -noremodel
	    // flag was specified, consolidate() will remodel after
	    // consolidation.
	    //
	    // If build, then process_middle() will do the consolidation.
	    // We have no business doing it here.
	    //

	} else if(unconsolidated_files_exist()) {

	    try {

		//
		// If the request calls for remodeling, obviouly we should
		// consolidate if necessary but not remodel right away. That's
		// why we provide the rem/sch/unsch flags to consolidate().
		//
		aafReader::consolidate( /* skip remodel = */ rem || sch || unsch);
	    } catch(eval_error(Err)) {
		consolidation_error = true;

		//
		// Add the newline char because there may be many such messages:
		//
		consolidation_msg = Err.msg + "\n";
	    }
	    unconsolidated_files_exist() = false;
	}
	//
	// Else, we don't want to consolidate just for fun!
	//

	if(CommentInLog) {

		//
		// This instruction came for a script; it should be written to the log as a
		// comment, since the instruction to read the script is the only one that
		// should be executed again
		//
		if(APcloptions::theCmdLineOptions().aafLog) {
			Cstring s;
			to_script(s);
			Log::LOG() << "    " << *add_pound_sign(s);
		} else {
			Log::LOG() << *add_pound_sign(get_command_text_with_modifier()) << "\n";
		}
	} else {

		//
		// This instruction is at the top level: either it is an OPENFILE command to
		// read a script, or it came from the GUI or an external source. It should
		// appear as such in the log, so it can be played back when the log is read
		// into APGen.
		//


		//
		// The list_of_errors is where errors from lower levels will be accumulating:
		//

		list_of_errors.clear();
		if(APcloptions::theCmdLineOptions().aafLog) {
			Cstring s;
			to_script(s);
			Log::LOG() << "    " << *s;
		} else {
			Log::LOG() << *get_command_text_with_modifier() << "\n";
		}
	}

	if(req_intfc::COMMONhandler) {
		req_intfc::COMMONhandler(this, 0);
	}

	//
	// Make sure that action requests from external sources
	// are followed by GUI synchronization:
	//
	udef_intfc::something_happened() = 1;

	if(consolidation_error) {
		add_error(consolidation_msg);
		set_status(apgen::RETURN_STATUS::FAIL);
	}
}


//
// Note on TOL and XMLTOL requests "hidden" inside REMODEL or
// SCHEDULE / UNSCHEDULE requests: If errors occur, they will
// be logged as part of the rem/sch/unsch request logging. If
// successful, we need to log the "hidden" requests as well
// as the rem/sch/unsch request.
//
void 
Action_request::common_process_last() {
	if(theStatus != apgen::RETURN_STATUS::SUCCESS) {

		//
		// hard error
		//

		Cstring		postfix("(error occurred in \"");
		Cstring		command_and_modif(get_command_text_with_modifier());

		postfix << command_and_modif << "\")";
		if(shouldCaptureErrors) {
			stringslist::iterator	err(local_errors);
			emptySymbol*		N;
			aoString		a;

			while((N = err())) {
				a << N->get_key();
			}
			*captured = a.str();
		}

		if(script_execution_level == 1) {
			list_of_errors << local_errors;
			list_of_errors << new emptySymbol(postfix);
			Cstring error_text(command_and_modif);
			error_text << " - Execution Error";
			// NOTE: UI_mainwindow will write this to the LOG as well; no need to do it ourselves.
			ACT_exec::displayMessage("APGEN Error", error_text, list_of_errors);
		} else {

			//
			// Add some space for readability:
			//

			list_of_errors << new emptySymbol("\nAPGEN Error - ");
			list_of_errors << local_errors;
			list_of_errors << new emptySymbol(postfix);
		}
	} else if(list_of_errors.get_length()) {

		//
		// warning
		//

		if(script_execution_level == 1) {
			Cstring		postfix("(warning occurred in \"");
			Cstring		command_and_modif(get_command_text_with_modifier());
			Cstring error_text(command_and_modif);
			error_text << " - Execution Warning";
    
			if(command_and_modif.length() > 256) {
				char	truncated[260];

				strncpy(truncated, *command_and_modif, 256);
				truncated[256] = '.';
				truncated[257] = '.';
				truncated[258] = '.';
				truncated[259] = '\0';
				command_and_modif = truncated; }
			postfix << command_and_modif << "\")";
			list_of_errors << new emptySymbol(postfix);
			/*
			 * NOTE: UI_mainwindow will write this to the LOG as well; no need to do it ourselves.
			 */
			ACT_exec::displayMessage("APGEN Warning", error_text, list_of_errors);
		}
	}
	eval_intfc::get_act_lists().synchronize_all_lists();
	if(req_intfc::COMMONhandler) {
		req_intfc::COMMONhandler(this, 1);
	}

	//
	// Add performance information. Note: the script_execution_level
	// global will automatically provide the indentation.
	//
	if(CommentInLog) {
	    char buf[200];
	    sprintf(buf, "CPU: %.2lf sec. Mem: %.2lf Gbyte",
			perf_report::last_cpu_duration(),
			((double)perf_report::MemoryUsed()) / 1000000000.0);
	    Log::LOG() << *add_pound_sign(buf) << "\n";
	}
	REMODELrequest* remodel = dynamic_cast<REMODELrequest*>(this);

	//
	// debug
	//
	// cerr << "    common_process_last: processing AR at " << ((void*)this)
	// 	<< " (" << get_command_text() << ")\n";

	if(remodel) {

	    //
	    // debug
	    //
	    // cerr << "           found remodel; tol requests: "
	    //	<< remodel->tol_requests.get_length() << "\n";

	    if(remodel->tol_requests.get_length()) {

		//
		// We need to log and eelete any (XML)TOL requests
		// that were executed concurrently
		//
		Action_request* req;
		while((req = remodel->tol_requests.first_node())) {

		    //
		    // debug
		    //
		    // cerr << "         deleting tol request at " << ((void*)req) << "\n";

		    if(CommentInLog) {
			Log::LOG() << *add_pound_sign(req->get_command_text_with_modifier())
				<< "\n";
		    } else {
			Log::LOG() << *req->get_command_text_with_modifier()
				<< "\n";
		    }
		    delete req;
		}
	    }
	}
}

Cstring Action_request::get_local_errors() {
	aoString aos;
	for(emptySymbol* es = local_errors.first_node();
	    es != local_errors.last_node();
	    es = es->next_node()) {
		aos << es->get_key();
	}
	local_errors.clear();
	return aos.str();
}

Cstring Action_request::get_error_message() {
	Cstring		complete_message;
	int		length_of_complete_message = 0;
	char		*C_version_of_complete_message, *char_pointer;
	emptySymbol*	N;
	stringslist::iterator	errs(list_of_errors);

	while((N = errs()))
		length_of_complete_message += (N->get_key().length() + 1);
	C_version_of_complete_message = (char *) malloc(length_of_complete_message + 1);
	C_version_of_complete_message[0] = '\0'; // BKL added to fix ABR purify bug
	char_pointer = C_version_of_complete_message;
	while((N = errs())) {
		strcpy(char_pointer, *N->get_key());
		char_pointer += N->get_key().length();
		*char_pointer++ = '\n';
	}
	*char_pointer = '\0';
	list_of_errors.clear();
	complete_message = C_version_of_complete_message;
	free(C_version_of_complete_message);
	return complete_message;
}

SAVE_FILErequest::SAVE_FILErequest(	const Cstring&				fn,
					const apgen::FileType&			aft,
					const stringslist&			excl,
					tlist<alpha_string, optionNode>&	theOptions)
	: Action_request("SAVEFILE"),
	file_name(fn),
	theFileType(aft),
	exclude(excl),
	saveOptions(theOptions) {}

SAVE_FILErequest::SAVE_FILErequest(Cstring& errors, ListOVal* args)
	: Action_request("SAVEFILE") {
	ArrayElement* file_type_ae;
	ArrayElement* options_ae;
	ArrayElement* name_ae;
	ArrayElement* exclude_ae;
	Cstring ft;

	if((name_ae =args->find("name"))) {
		file_name = name_ae->Val().get_string();
	} else {
		errors << "Element named \"name\" is missing from args. ";
	}
	if((file_type_ae = args->find("type"))) {
		ft = file_type_ae->Val().get_string();
		if(ft == "AAF") {
			theFileType = apgen::FileType::FT_AAF;
		} else if(ft == "APF") {
			theFileType = apgen::FileType::FT_APF;
		} else if(ft == "MIX") {
			theFileType = apgen::FileType::FT_MIXED;
		} else if(ft == "LAYOUT") {
			theFileType = apgen::FileType::FT_LAYOUT;
		} else if(ft == "XML") {
			theFileType = apgen::FileType::FT_XML;
		} else {
			errors << "Element named \"type\" is not a valid apgen file type. ";
		}
	} else {
		errors << "Element named \"type\" not found in args. ";
	}

	// ugly but that's the way it is:
	if(theFileType == apgen::FileType::FT_APF) {
		if(file_name[0] != '"') {
			file_name = addQuotes(file_name);
		}
	}

	if(file_type_ae && (options_ae = args->find("options"))) {
		if(!options_ae->Val().is_array()) {
			errors << "Element named \"options\" is not an array. "; }
		else {
			ListOVal&					options(options_ae->Val().get_array());
			ArrayElement*					ae;

			for(int k = 0; k < options.get_length(); k++) {
				ae = options[k];
				Cstring		which_option(ae->get_key());
				Cstring		option_value(ae->Val().get_string());

				if(	which_option == "GLOBALS"
					|| which_option == "EPOCHS"
					|| which_option == "TIME_SYSTEMS"
					|| which_option == "LEGENDS"
					|| which_option == "TIME_PARAMS"
					|| which_option == "WIN_SIZE") {
					if(option_value == "NOTHING") {
						saveOptions << new optionNode(ft + " " + which_option, INCLUDE_NOT_AT_ALL);
					} else if(option_value == "COMMENTS") {
						saveOptions << new optionNode(ft + " " + which_option, INCLUDE_AS_COMMENTS);
					} else if(option_value == "CODE") {
						saveOptions << new optionNode(ft + " " + which_option, INCLUDE_AS_CODE);
					} else {
						errors << "Element named \"options\" contains "
							"an invalid option value. ";
					}
				} else  {
					errors << "Element named \"options\" contains "
					"an invalid option name. ";
				}
			}
		}
	}
	if(!saveOptions.find(ft + " " + "GLOBALS")) {
		saveOptions << new optionNode(ft + " " + "GLOBALS", INCLUDE_AS_COMMENTS);
	}
	if(!saveOptions.find(ft + " " + "EPOCHS")) {
		saveOptions << new optionNode(ft + " " + "EPOCHS", INCLUDE_AS_COMMENTS);
	}
	if(!saveOptions.find(ft + " " + "TIME_SYSTEMS")) {
		saveOptions << new optionNode(ft + " " + "TIME_SYSTEMS", INCLUDE_AS_COMMENTS);
	}
	if(!saveOptions.find(ft + " " + "LEGENDS")) {
		saveOptions << new optionNode(ft + " " + "LEGENDS", INCLUDE_AS_COMMENTS);
	}
	if(!saveOptions.find(ft + " " + "TIME_PARAMS")) {
		saveOptions << new optionNode(ft + " " + "TIME_PARAMS", INCLUDE_AS_COMMENTS);
	}
	if(!saveOptions.find(ft + " " + "WIN_SIZE")) {
		saveOptions << new optionNode(ft + " " + "WIN_SIZE", INCLUDE_AS_COMMENTS);
	}
	if((exclude_ae = args->find("exclude"))) {
		if(!exclude_ae->Val().is_array()) {
			errors << "Element named \"exclude\" is not an array. ";
		} else {
			ListOVal&					excluded(exclude_ae->Val().get_array());
			ArrayElement*					ae;

			for(int k = 0; k > excluded.get_length(); k++) {
				ae = excluded[k];
				exclude << new emptySymbol(ae->Val().get_string());
			}
		}
	}
}

const Cstring &SAVE_FILErequest::get_command_text() {
	emptySymbol*		act;
	optionNode		*bn_globals, *bn_epochs, *bn_timesystems, *bn_legends, *bn_timeparams, *bn_winsize;
	Cstring			Suffix, theFileTypeAsAString, theFileTypeForPreferences;

	if (commandText.length() > 0) return commandText;

	if(theFileType == apgen::FileType::FT_AAF) {
		theFileTypeForPreferences = "AAF";
		theFileTypeAsAString = "AAF";
	} else if(theFileType == apgen::FileType::FT_APF) {
		theFileTypeForPreferences = "APF";
		theFileTypeAsAString = "APF";
	} else if(theFileType == apgen::FileType::FT_MIXED) {
		theFileTypeForPreferences = "MIX";
		theFileTypeAsAString = "MIX";
	} else if(theFileType == apgen::FileType::FT_LAYOUT) {
		theFileTypeForPreferences = "APF";
		/* We do this because we don't have specific preferences
		 * for LAYOUT */
		theFileTypeAsAString = "APF";
	} else if(theFileType == apgen::FileType::FT_XML) {
		theFileTypeForPreferences = "APF";
		/* We do this because we don't have specific preferences
		 * for XML and most likely, they would be identical to the
		 * APF preferences if we did have them */
		theFileTypeAsAString = "XML";
	}

	commandText = commandType + " " + theFileTypeAsAString + " " + file_name;

	act = exclude.first_node();

	if(theFileType == apgen::FileType::FT_LAYOUT) {
		if(act) {
			commandText << " LAYOUT_TAG ";
			commandText << addQuotes(act->get_key());
		}
	}
	bn_globals	= saveOptions.find(theFileTypeForPreferences + " GLOBALS"),
	bn_epochs	= saveOptions.find(theFileTypeForPreferences + " EPOCHS"),
	bn_timesystems	= saveOptions.find(theFileTypeForPreferences + " TIME_SYSTEMS"),
	bn_legends	= saveOptions.find(theFileTypeForPreferences + " LEGENDS"),
	bn_timeparams	= saveOptions.find(theFileTypeForPreferences + " TIME_PARAMS"),
	bn_winsize	= saveOptions.find(theFileTypeForPreferences + " WIN_SIZE");
	if(act && theFileType != apgen::FileType::FT_LAYOUT) {
		commandText << " EXCLUDE ";
		while(act) {
			commandText << act->get_key();
			act = act->next_node();
			if (act) commandText << ",";
		}
	}

	commandText << " [ ";

	if(bn_globals->payload == INCLUDE_NOT_AT_ALL)  {
		commandText << "GLOBALS = \"NOTHING\", "; }
	else if(bn_globals->payload == INCLUDE_AS_COMMENTS) {
		commandText << "GLOBALS = \"COMMENTS\", "; }
	else if(bn_globals->payload == INCLUDE_AS_CODE) {
		commandText << "GLOBALS = \"CODE\", "; }

	if(bn_epochs->payload == INCLUDE_NOT_AT_ALL) {
		commandText << "EPOCHS = \"NOTHING\", "; }
	else if(bn_epochs->payload == INCLUDE_AS_COMMENTS) {
		commandText << "EPOCHS = \"COMMENTS\", "; }
	else if(bn_epochs->payload == INCLUDE_AS_CODE) {
		commandText << "EPOCHS = \"CODE\", "; }

	if(bn_timesystems->payload == INCLUDE_NOT_AT_ALL) {
		commandText << "TIME_SYSTEMS = \"NOTHING\", "; }
	else if(bn_timesystems->payload == INCLUDE_AS_COMMENTS) {
		commandText << "TIME_SYSTEMS = \"COMMENTS\", "; }
	else if(bn_timesystems->payload == INCLUDE_AS_CODE) {
		commandText << "TIME_SYSTEMS = \"CODE\", "; }

	if(bn_legends->payload == INCLUDE_NOT_AT_ALL) {
		commandText << "LEGENDS = \"NOTHING\", "; }
	else if(bn_legends->payload == INCLUDE_AS_COMMENTS) {
		commandText << "LEGENDS = \"COMMENTS\", "; }
	else if(bn_legends->payload == INCLUDE_AS_CODE) {
		commandText << "LEGENDS = \"CODE\", "; }

	if(bn_timeparams->payload == INCLUDE_NOT_AT_ALL) {
		commandText << "TIME_PARAMS = \"NOTHING\", "; }
	else if(bn_timeparams->payload == INCLUDE_AS_COMMENTS) {
		commandText << "TIME_PARAMS = \"COMMENTS\", "; }
	else if(bn_timeparams->payload == INCLUDE_AS_CODE) {
		commandText << "TIME_PARAMS = \"CODE\", "; }

	if(bn_winsize->payload == INCLUDE_NOT_AT_ALL) {
		commandText << "WIN_SIZE = \"NOTHING\""; }
	else if(bn_winsize->payload == INCLUDE_AS_COMMENTS) {
		commandText << "WIN_SIZE = \"COMMENTS\""; }
	else if(bn_winsize->payload == INCLUDE_AS_CODE) {
		commandText << "WIN_SIZE = \"CODE\""; }

	commandText << "] ";
	return commandText; }

void SAVE_FILErequest::process_middle(TypedValue*) {
	stringtlist		complementary_list(eval_intfc::ListOfAllFileNames());
	emptySymbol*		N;
	stringslist::iterator	exclusions(exclude);
	IO_writer		writer;
	List			legends_to_save;
	List_iterator		legs(Dsource::theLegends());
	LegendObject		*n;
	optionNode		*bn_globals,
				*bn_epochs,
				*bn_timesystems,
				*bn_legends,
				*bn_timeparams,
				*bn_winsize;
	Cstring			theFileTypeAsAString;
	Cstring			errors;
	Cstring			file_name_to_use(file_name);

	if(file_name_to_use[0] == '"') {
		removeQuotes(file_name_to_use);
	}

	if(theFileType == apgen::FileType::FT_APF) {
		theFileTypeAsAString = "APF";
	} else if(theFileType == apgen::FileType::FT_AAF) {
		theFileTypeAsAString = "AAF";
	} else if(theFileType == apgen::FileType::FT_MIXED) {
		theFileTypeAsAString = "MIX";
	} else if(theFileType == apgen::FileType::FT_LAYOUT) {
		theFileTypeAsAString = "LAYOUT";
	} else if(theFileType == apgen::FileType::FT_XML) {
		/* We do this because we don't have specific preferences
		 * for XML and most likely, they would be identical to the
		 * APF preferences if we did have them */
		theFileTypeAsAString = "APF";
	}

	if(theFileType != apgen::FileType::FT_LAYOUT) {
		bn_globals	= saveOptions.find(theFileTypeAsAString + " GLOBALS"		);
		bn_epochs	= saveOptions.find(theFileTypeAsAString + " EPOCHS"		);
		bn_timesystems	= saveOptions.find(theFileTypeAsAString + " TIME_SYSTEMS"	);
		bn_legends	= saveOptions.find(theFileTypeAsAString + " LEGENDS"		);
		bn_timeparams	= saveOptions.find(theFileTypeAsAString + " TIME_PARAMS"	);
		bn_winsize	= saveOptions.find(theFileTypeAsAString + " WIN_SIZE"		);

		while(((N = exclusions.next()))) {
			if((N = complementary_list.find(N->get_key()))) {
				// debug
				// cerr << "save apf: excluding " << N->get_key() << endl;
				delete N;
			}
		}
		// Note: the complementary list now contains all filenames that were
		// not specifically excluded by the user, except for the "New" pseudo-filename
		// that is used when creating new legends.
		while((n = (LegendObject *) legs.next())) {
			if(	n->apf == "New"
				|| complementary_list.find(n->apf)) {
				// debug
				// cerr << "Including legend " << n->get_key() << endl;
				legends_to_save << new String_node(n->get_key());
			} else {
				// debug
				// cerr << "NOT including legend " << n->get_key() << " because its APF "
				// 	<< n->apf << " is not in the complementary list.\n";
			
			}
		}

		if(theFileType == apgen::FileType::FT_MIXED) {
			errors << "Mixed file output is no longer supported";
			add_error(errors);
			set_status(apgen::RETURN_STATUS::FAIL);
		} else if(theFileType == apgen::FileType::FT_AAF) {
			errors << "AAF output is no longer supported";
			add_error(errors);
			set_status(apgen::RETURN_STATUS::FAIL);
		} else if(theFileType == apgen::FileType::FT_APF) {
			TypedValue	result;

			if(evaluate_string_arg(
				    file_name,
				    result,
				    apgen::DATA_TYPE::STRING,
				    errors)
					!= apgen::RETURN_STATUS::SUCCESS) {
				add_error(errors);
				set_status(apgen::RETURN_STATUS::FAIL);
			} else if(writer.writeAPF(
				    result.get_string(),
				    complementary_list,
				    legends_to_save,
				    bn_globals->payload,
				    bn_epochs->payload,
				    bn_timesystems->payload,
				    bn_legends->payload,
				    bn_timeparams->payload,
				    bn_winsize->payload,
				    0,	// registered functions
				    0,	// formats
				    errors)
					!= apgen::RETURN_STATUS::SUCCESS) {
				add_error(errors);
				set_status(apgen::RETURN_STATUS::FAIL);
			}
		} else if(theFileType == apgen::FileType::FT_XML) {
			if(writer.writeXML(
				    file_name_to_use,
				    complementary_list,
				    legends_to_save,
				    bn_globals->payload,
				    bn_epochs->payload,
				    bn_timesystems->payload,
				    bn_legends->payload,
				    bn_timeparams->payload,
				    bn_winsize->payload,
				    0,	// registered functions
				    0,	// formats
				    errors)
					!= apgen::RETURN_STATUS::SUCCESS) {
				add_error(errors);
				set_status(apgen::RETURN_STATUS::FAIL);
			}
		}
	} else {
		// we really depend on main window here; use handler.
		if(req_intfc::SAVE_FILEhandler) {
			req_intfc::SAVE_FILEhandler(this, 1);
		}
	}
	return;
}

SAVE_PARTIAL_FILErequest::SAVE_PARTIAL_FILErequest(const stringslist& flist, const Cstring& cname)
	: Action_request("SAVEPARTIALFILE"),
	filelist(flist),
	name(cname) {}

SAVE_PARTIAL_FILErequest::SAVE_PARTIAL_FILErequest(Cstring& errors, ListOVal* args)
	: Action_request("SAVEPARTIALFILE") {
	ArrayElement* list_ae;
	ArrayElement* name_ae;

	if((list_ae = args->find("files"))) {
		if(!list_ae->Val().is_array()) {
			errors << "Element named \"files\" is not an array of file names. "; }
		else {
			ListOVal&					theFiles = list_ae->Val().get_array();
			ArrayElement*					a_file;

			for(int k = 0; k < theFiles.get_length(); k++) {
				a_file = theFiles[k];
				filelist << new emptySymbol(a_file->Val().get_string());
			}
		}
	} else {
		errors << "element named \"files\" not found in args. ";
	}
	if((name_ae = args->find("name"))) {
		name = name_ae->Val().get_string();
	} else {
		errors << "Element named \"name\" not found in args. ";
	}
}

const Cstring & SAVE_PARTIAL_FILErequest::get_command_text() {
	if(commandText.length() > 0)
		return commandText;
	commandText = commandType;

	emptySymbol *act;
	act = filelist.first_node();
	if (act) {
		commandText << " ";
		while (act) {
			commandText << *(act->get_key());
			act = act->next_node();
			if (act)
				commandText << ","; } }
	commandText << " " << *name;
	return commandText; }

void SAVE_PARTIAL_FILErequest::process_middle(TypedValue*) {
	IO_writer	writer;
	List		legends_to_save;
	List_iterator	legs(Dsource::theLegends());
	LegendObject	*n;

	if(!filelist.get_length()) {
		add_error("No plan file(s) are selected.\n");
		set_status(apgen::RETURN_STATUS::FAIL);
		return;
	}

	//
	// debug
	//

	// cerr << "SAVE_PARTIAL_FILErequest: list of files\n";
	// for(emptySymbol* ii = filelist.first_node(); ii != NULL; ii = ii->next_node()) {
	// 	cerr << "\t" << ii->get_key() << "\n";
	// }

	while((n = (LegendObject *) legs())) {
		// Hopper design: any restrictions??
		if(filelist.find(n->apf)) {
			legends_to_save << new String_node(n->get_key());
		}
	}

	// We should really have a data member like inclusion_code to indicate
	// whether legends (or other directives) should be saved:

	Cstring errors;

	if(writer.writeAPF(
			name,
			filelist,
			legends_to_save,
			Action_request::INCLUDE_NOT_AT_ALL,
			Action_request::INCLUDE_AS_COMMENTS,
			Action_request::INCLUDE_AS_COMMENTS,
			Action_request::INCLUDE_AS_CODE,
			Action_request::INCLUDE_NOT_AT_ALL,
			Action_request::INCLUDE_NOT_AT_ALL,
			Action_request::INCLUDE_NOT_AT_ALL,
			Action_request::INCLUDE_NOT_AT_ALL,
			errors) != apgen::RETURN_STATUS::SUCCESS) {
		add_error(errors);
		set_status(apgen::RETURN_STATUS::FAIL);
	}
	return;
}

const Cstring & 
EXPORT_DATArequest::get_command_text() {

	if(commandText.length() > 0) {
		return commandText;
	}

	commandText = commandType;
	if(What == ACTIVITY_INTERACTIONS) {
		commandText << " ACTIVITY_INTERACTIONS";
	} else if(What == ACTIVITY_INSTANCES) {
		commandText << " ACTIVITY_INSTANCES";
	} else if(What == FUNCTION_DECLARATIONS) {
		commandText << " FUNCTION_DECLARATIONS";
	} else if(What == GLOBALS) {
		commandText << " GLOBALS";
	}
	if(How == JSON_FILE) {
		commandText << " JSON_FILE";
	}

	commandText << " " << addQuotes(Filename);

	return commandText;
}

void
EXPORT_DATArequest::process_middle(TypedValue*) {
    IO_writer		writer;
    Cstring		errors;
			/* for now, singleFile is always true. The multiple file
			 * option was added at Tom Crockett's request, but it didn't
			 * really solve the problem of long transfer times to TMS
			 * for large activity plans. */
    bool		singleFile = How == JSON_FILE;
    stringslist		thisCouldGetBig;

    if(How == JSON_FILE) {
	if(What == ACTIVITY_INTERACTIONS) {
	    stringslist		thisCouldGetBig;
	    stringslist::iterator iter(thisCouldGetBig);
	    emptySymbol*	  es;

	    if(writer.write_act_interactions_to_Json_strings(
					true,
					Filename,
					thisCouldGetBig,
					errors)
			    != apgen::RETURN_STATUS::SUCCESS) {
		add_error(errors);
		set_status(apgen::RETURN_STATUS::FAIL);
		return;
	    }
	} else if(What == ACTIVITY_INSTANCES) {
	    pairtlist		thisCouldGetBig(true);
	    if(ACT_exec::WriteActivitiesToJsonStrings(
			/* singleFile = */ false,
			/* Namespace = */ "Merlin",
			/* Directory = */ Filename,
			thisCouldGetBig,
			errors) != apgen::RETURN_STATUS::SUCCESS) {
		add_error(errors);
		set_status(apgen::RETURN_STATUS::FAIL);
		return;
	    }

	    //
	    // result has all the stuff in it
	    //
	    pairslist::iterator iter(thisCouldGetBig);
	    symNode*		sym;

	    //
	    // First create the directory that will contain
	    // one file per subsystem, if necessary
	    //
	    struct stat	dir_stat;
	    int		ret = stat(*Filename, &dir_stat);
	    bool	is_directory = S_ISDIR(dir_stat.st_mode);
	    bool	is_file = S_ISREG(dir_stat.st_mode);
	    if(is_file) {
		errors << "File " << Filename << " is a regular file, not a directory\n";
		add_error(errors);
		set_status(apgen::RETURN_STATUS::FAIL);
		return;
	    }
	    if(!is_directory) {

		//
		// We want permissions drwxr-xr-x
		//
		mkdir(*Filename, 0755);
		ret = stat(*Filename, &dir_stat);
		is_directory = S_ISDIR(dir_stat.st_mode);
		if(!is_directory) {
		    errors << "Cannot create directory " << Filename << "\n";
		    add_error(errors);
		    set_status(apgen::RETURN_STATUS::FAIL);
		    return;
		}
	    }
	    while((sym = iter())) {
		Cstring subsystem = sym->Key.get_key();
		Cstring onefile;
		onefile << Filename << "/" << subsystem << ".json";
		FILE* f = fopen(*onefile, "w");
		fwrite(*sym->payload, sym->payload.length(), 1, f);
	    }
	} else if(What == FUNCTION_DECLARATIONS) {

	    //
	    // Collect AAF functions
	    //
	    global_behaving_object*	glob = behaving_object::GlobalObject();
	    const Behavior&		glob_type = glob->Type;
	    const vector<task*>&	glob_tasks = glob_type.tasks;
	    stringstream		S;

	    S << "apgen version \"Declarations\"\n";

	    for(int i = 0; i < glob_tasks.size(); i++) {
		task* T = glob_tasks[i];

		//
		// The constructor does not have a program.
		// This is an anomaly compared with standard
		// languages.
		//
		if(T->prog) {
		    S << "function ";
		    if(T->prog->ReturnType == apgen::DATA_TYPE::UNINITIALIZED) {
			S << "void";
		    } else {
			S << apgen::spell(T->prog->ReturnType);
		    }
		    S << " " << T->name << "(";

		    vector<int>& index = T->paramindex;
		    const vector<pair<Cstring, apgen::DATA_TYPE> >& V
			    = T->get_varinfo();
		    for(int j = 0; j < index.size(); j++) {
			const pair<Cstring, apgen::DATA_TYPE>& P = V[index[j]];
			if(j > 0) {
			    S << ", ";
			}
			S << apgen::spell(P.second) << " " << P.first;
		    }
		    S << ");\n";
		}
	    }
	    FILE* fout = fopen(*Filename, "w");
	    if(fout) {
		string s = S.str();
		fwrite(s.c_str(), s.size(), 1, fout);
		fclose(fout);
	    }
	} else if(What == GLOBALS) {
	    json_object* obj = globalData::GetJsonGlobals();
	    Cstring s = json_object_to_json_string_ext(
			    obj,
			    JSON_C_TO_STRING_PRETTY);
	    json_object_put(obj);
	    FILE* fout = fopen(*Filename, "w");
	    if(fout) {
		fwrite(*s, s.length(), 1, fout);
		fclose(fout);
	    }
	}
    }
}

Action_request* WRITE_SASFrequest::copy() {
	static stringslist	l;

	return new WRITE_SASFrequest(
			l,
			start_time_expr,
			end_time_expr,
			desired_file_name_expr,
			includeActivitiesThatStartAtEndTime);
}

WRITE_SASFrequest::WRITE_SASFrequest(
		const stringslist& theListOfSymbolicFilenames,
		const Cstring &S,
		const Cstring &E,
		const Cstring &desired,
		int theFlag)
	: Action_request("WRITESASF"),
		desired_file_name_expr(desired),
		includeActivitiesThatStartAtEndTime(theFlag),
		start_time_expr(S),
		end_time_expr(E) {
	emptySymbol*		sn;
	stringslist::iterator	theFilenames(theListOfSymbolicFilenames);

	while((sn = theFilenames())) {
		Cstring		unquoted(sn->get_key());

		removeQuotes(unquoted);
		original_sasf_file_attributes << new emptySymbol(unquoted);
	}
}

WRITE_SASFrequest::WRITE_SASFrequest(Cstring& errors, ListOVal* args)
	: Action_request("WRITESASF"),
	includeActivitiesThatStartAtEndTime(0) {
	ArrayElement*	start_ae;
	ArrayElement*	end_ae;
	ArrayElement*	include_flag_ae;
	ArrayElement*	attributes_ae;
	ArrayElement*	name_ae;

	if((name_ae = args->find("name"))) {
		desired_file_name_expr = name_ae->Val().to_string();
	} else {
		errors << "Element named \"name\" missing from args. ";
	}
	if((start_ae = args->find("start"))) {
		if(start_ae->Val().is_time()) {
			start_time_expr = start_ae->Val().get_time_or_duration().to_string();
		} else if(start_ae->Val().is_string()) {
			start_time_expr = start_ae->Val().get_string();
		} else {
			errors << "Element named \"start\" is neither time nor string. ";
		}
	}
	if((end_ae = args->find("end"))) {
		if(end_ae->Val().is_time()) {
			end_time_expr = end_ae->Val().get_time_or_duration().to_string();
		} else if(end_ae->Val().is_string()) {
			end_time_expr = end_ae->Val().get_string();
		} else {
			errors << "Element named \"end\" is neither time nor string. ";
		}
	}
	if((include_flag_ae = args->find("include_acts_starting_at_end"))) {
		includeActivitiesThatStartAtEndTime = include_flag_ae->Val().get_int();
	}
	if((attributes_ae = args->find("files"))) {
		if(!attributes_ae->Val().is_array()) {
			errors << "Element named \"files\" is not an array. ";
		} else {
			ListOVal&					Files(attributes_ae->Val().get_array());
			ArrayElement*					one_file;

			for(int k = 0; k < Files.get_length(); k++) {
				one_file = Files[k];
				original_sasf_file_attributes
					<< new emptySymbol(one_file->Val().get_string());
			}
		}
	} else {
		errors << "Element named \"files\" missing from args. ";
	}
}

// "borrowed" from ACT_sys.C:
class ordered_pointer_to_activity: public Time_node {
public:
	CTime_base				event_time;
	ActivityInstance*			request;
	ordered_pointer_to_activity(ActivityInstance * A)
		    : event_time(A->getetime()),
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

const Cstring& WRITE_SASFrequest::get_command_text() {
	stringslist::iterator	theFiles(original_sasf_file_attributes);
	emptySymbol*		bp;
	char			c = desired_file_name_expr[0];

	if (commandText.length() > 0) return commandText;
	commandText = commandType;
	commandText << " " << desired_file_name_expr;

	commandText << " " << start_time_expr << " " << end_time_expr << " ";
	while((bp = theFiles())) {
		commandText << addQuotes(bp->get_key());
		if(bp->following_node())
			commandText << ", "; }
	if(includeActivitiesThatStartAtEndTime) 
		commandText << " 1";
	else
		commandText << " 0";
	return commandText; }

apgen::RETURN_STATUS evaluate_string_arg(
		const Cstring&		s,
		TypedValue&		result,
		apgen::DATA_TYPE	expectedType,
		Cstring&		err_msg) {
	Cstring		theMiddle(s);
	char		c = (*s)[0];

	//
	// argument evaluation invokes a compiler that is not
	// thread-safe
	//
	mutex		mtx;
	lock_guard<mutex> lock(mtx);

	// debug
	// cout << "evaluate_string_arg...: compiling " << theMiddle << "\n";
	if(c == '`' || c == '\'') {
		char *middle = (char *) malloc(s.length() - 1);
		strncpy(middle, *s + 1, s.length() - 2);
		middle[s.length() - 2] = '\0';
		theMiddle = middle;
		// debug
		// cout << "evaluate_string_arg...: after stripping " << theMiddle << "\n";
		free(middle);
	}
	try {
		result.undefine();
		result.declared_type = expectedType;
		compiler_intfc::CompileAndEvaluateSelf(theMiddle, result);
	} catch(eval_error Err) {
		err_msg << "\nExpression " << theMiddle << " has errors in it; details:\n"
			<< Err.msg << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void WRITE_SASFrequest::process_middle(TypedValue*) {
	IO_writer	writer;
	List		commands;
	Cstring		errors, conv_file, build_msg; 
	TypedValue	result;
  
	/* CHANGE FOR V31.0: all string arguments can be expressions.
	 * First test for a simple string, then look for something more
	 * complex... No, I don't like that: we should use back quotes,
	 * just like the shell. */
	//sort out start and end time stuff

	if(evaluate_string_arg(
				start_time_expr,
				result,
				apgen::DATA_TYPE::TIME, errors) != apgen::RETURN_STATUS::SUCCESS) {
		add_error(errors);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
	theStartTime = result.get_time_or_duration();
	if(theStartTime == CTime_base(0, 0, false)) {
		model_intfc::FirstEvent(theStartTime);
		model_intfc::LastEvent(theEndTime);
	} else {
		if(evaluate_string_arg(
				end_time_expr,
				result,
				apgen::DATA_TYPE::TIME,
				errors) != apgen::RETURN_STATUS::SUCCESS) {
			add_error(errors);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		theEndTime = result.get_time_or_duration();
	}
	if(evaluate_string_arg(
				desired_file_name_expr,
				result,
				apgen::DATA_TYPE::STRING, errors) != apgen::RETURN_STATUS::SUCCESS) {
		//
		// evaluate_string_arg doesn't
		// work with strings.
		//
		removeQuotes(desired_file_name_expr);
		result = desired_file_name_expr;
	}
	desired_file_name = result.get_string();

	try {
		writer.writeSASF2(
			desired_file_name,
			original_sasf_file_attributes,
			theStartTime,
			theEndTime,
			includeActivitiesThatStartAtEndTime);
	} catch(eval_error Err) {
		add_error(Err.msg);
		set_status(apgen::RETURN_STATUS::FAIL);
		return;
	}

	return;
}

NEW_HORIZONrequest::NEW_HORIZONrequest(Cstring& errors, ListOVal* args)
		: Action_request("NEWHORIZON") {
	ArrayElement* start_ae = NULL;
	ArrayElement* end_ae = NULL;
	ArrayElement* AD_ae = NULL;

	whichDisplay = "A1";

	if((start_ae = args->find("start"))) {
		start_time = start_ae->Val().get_time_or_duration();
	} else {
		errors << "Element named \"start\" is missing from args list. ";
	}
	if((end_ae = args->find("end"))) {
		end_time = end_ae->Val().get_time_or_duration();
	} else {
		errors << "Element named \"end\" is missing from args list. ";
	}
	if((AD_ae = args->find("ad"))) {
		whichDisplay = AD_ae->Val().get_string();
	}
}

const Cstring &NEW_HORIZONrequest::get_command_text() {
	if(commandText.length())
		return commandText;
	commandText << commandType << " " << whichDisplay << " FROM ";
	if(start_time > CTime_base(0, 0, false)) {
		commandText << start_time.to_string() << " TO ";
		commandText << end_time.to_string();
	} else {
		commandText << Cstart_time << ", " << Cduration << ", " << Creal << ", " << Cmodeled;
	}
	return commandText;
}

void NEW_HORIZONrequest::process_middle(TypedValue*) {
	Pointer_node*	n;

	if(start_time == CTime_base(0, 0, false)) {

		/*
		 * We need to parse and check. The following logic was originally in UI_abspanzoom.C.
		 */

		CTime_base			start_value, duration_value;
		CTime_base			zero_time(0, 0, false);
		CTime_base			zero_duration(0, 0, true);

		// Check for improper time format in Start Time

		try {
			is_a_valid_time(Cstart_time, start_value);
		}
		catch(eval_error Err) {
			Cstring		theMessage("Starting Time is improperly formatted; error: ");

			theStatus = apgen::RETURN_STATUS::FAIL;
			theMessage << "Starting Time is improperly formatted; error:\n";
			theMessage << Err.msg
				<< "; An example of a legal time format is 1995-200T00:10:00.\n";
			add_error(theMessage);
			return;
		}

		// Check for negative time in Start Time (exactly zero is OK)

		if (start_value < zero_time) {
			theStatus = apgen::RETURN_STATUS::FAIL;
			add_error("Start Time must be during 1970 or later.\n");
			return;
		}

		// Check for improper duration format in Time Span

		try {
			is_a_valid_duration(Cduration, duration_value);
		}
		catch(eval_error Err) {
			Cstring		theMessage("Time Span is improperly formatted; error:\n");

			theStatus = apgen::RETURN_STATUS::FAIL;
			theMessage << Err.msg
				<< "; An example of a legal duration format is 200T00:10:00.\n";
			add_error(theMessage);
			return;
		}

		// Check for zero or negative duration in Time Span

		if (duration_value <= zero_duration) {
			theStatus = apgen::RETURN_STATUS::FAIL;
			add_error("Time Span must be greater than 00:00:00.\n");
			return;
		}

		/*
		 * Whew! We made it. Note that real time etc. have already been updated...
		 */
		start_time = start_value;
		end_time = start_value + duration_value;
	}

	if(req_intfc::NEW_HORIZONhandler) {
		req_intfc::NEW_HORIZONhandler(this, 0);
	}
	// Allow for extra synchronization steps
	udef_intfc::something_happened() += 3;
	return;
}

WRITE_TOLrequest::WRITE_TOLrequest(
				const Cstring& desired_file_name,
				const Cstring& s1,
				const Cstring& s2, 
				const stringslist& list_of_filters,
				const Cstring& theOptionalFormat)
		: Action_request("WRITETOL"),
			CFileName(desired_file_name),
			Cstart_time(s1),
			Cend_time(s2),
			format(theOptionalFormat),
			running(false) {
	stringslist::iterator	theFilters(list_of_filters);
	emptySymbol*		N;
	Cstring			temp;

	while((N = theFilters())) {
		temp = N->get_key();
		removeQuotes(temp);
		filters << new emptySymbol(temp);
	}
}

WRITE_TOLrequest::WRITE_TOLrequest(
				Cstring& errors,
				ListOVal* args)
		: Action_request("WRITETOL"),
			Cstart_time("1970-001T00:00:00.000"),
			Cend_time("1970-001T00:00:00.000"),
			running(false) {
	ArrayElement* name_ae;
	ArrayElement* start_ae;
	ArrayElement* end_ae;
	ArrayElement* format_ae;
	ArrayElement* filters_ae;

	if((name_ae = args->find("name"))) {
		CFileName = name_ae->Val().to_string();
	} else {
		errors << "Element named \"name\" missing from args. ";
	}
	if((format_ae = args->find("format"))) {
		format = format_ae->Val().get_string();
	}
	if((filters_ae = args->find("filters"))) {
		if(!filters_ae->Val().is_array()) {
			errors << "Element named \"filters\" is not an array. ";
		} else {
			ListOVal&					Filters(filters_ae->Val().get_array());
			ArrayElement*					a_filter;

			for(int k = 0; k < Filters.get_length(); k++) {
				a_filter = Filters[k];
				filters << new emptySymbol(a_filter->Val().get_string());
			}
		}
	}
}

const Cstring & WRITE_TOLrequest::get_command_text() {
	emptySymbol*	aStrip;

	if (commandText.length() > 0) {
		return commandText;
	}
	commandText << commandType << " " << CFileName;
	commandText << " " << Cstart_time << " " << Cend_time;
	aStrip = filters.first_node();
	if (aStrip) {
		commandText << " FILTER ";
		while (aStrip) {
			commandText << addQuotes(aStrip->get_key());
			aStrip = aStrip->next_node();
			if (aStrip) commandText << ", ";
		}
	}

	//
	// NOTE: format is always present even though it's obsolete
	//
	commandText << " \"FORMAT ";
	if (format.length()) {
		commandText << format << "\"";
	} else {
		commandText << " DEFAULT\"";
	}
	return commandText;
}

//
// Returns true if OK, false if errors
//
bool WRITE_TOLrequest::evaluate_arguments() {
	Cstring		error_msg;
	TypedValue	result;

	//
	// Old note:
	//

	//
	// CHANGE FOR V31.0: if any arg is enclosed in single quotes or backquotes, it's an
	// expression that needs to be evaluated using Compile().
	//

	//
	// Now, we evaluate the string no matter what
	//
	bool start_and_end_times_are_defined = false;
	if(Cstart_time.length()) {
		if(evaluate_string_arg(
				Cstart_time,
				result,
				apgen::DATA_TYPE::TIME,
				error_msg) != apgen::RETURN_STATUS::SUCCESS) {
			add_error(error_msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return false;
		}
		StartTime = result.get_time_or_duration();
		if(StartTime > CTime_base(0, 0, false)) {
			if(evaluate_string_arg(
					Cend_time,
					result,
					apgen::DATA_TYPE::TIME,
					error_msg) != apgen::RETURN_STATUS::SUCCESS) {
				add_error(error_msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
				return false;
			}
			EndTime = result.get_time_or_duration();

			//
			// Successful evaluation
			//
			start_and_end_times_are_defined = true;

		}
	}

	if(!start_and_end_times_are_defined) {

		//
		// Get plan horizon from activities and resources
		//
		model_intfc::FirstEvent(StartTime);
		model_intfc::LastEvent(EndTime);
	}

	//
	// Now evaluate the file name
	//
	result.undefine();
	result.declared_type = apgen::DATA_TYPE::UNINITIALIZED;
	if(!CFileName.length()) {
		result = "<empty_file_name>";
	} else if(CFileName[0] == '"') {
		removeQuotes(CFileName);
		result = CFileName;
	} else if(evaluate_string_arg(
			CFileName,
			result,
			apgen::DATA_TYPE::STRING,
			error_msg) != apgen::RETURN_STATUS::SUCCESS) {
		// assume that the user didn't bother to add quotes
		result = CFileName;
	}
	FileName = result.get_string();

	//
	// all args have now been evaluated
	//
	return true;
}

void WRITE_TOLrequest::process_middle(TypedValue*) {

	//
	// If the process_middle method was invoked, that
	// means we are part of a script and should follow
	// the "regular" execution pattern.
	//
	// If this action request was hijacked by a REMODEL
	// request, it will be executed via a direct call
	// to execute() with a non-NULL thread_intfc pointer.
	//
	try {
		in_process() = this;
		execute(NULL);
		in_process() = NULL;
	} catch(eval_error Err) {
		Cstring errs;
		errs = "An error occurred while trying to create TOL \"";
		errs << FileName << "\":\n";
		errs << Err.msg << "\n";
		theStatus = apgen::RETURN_STATUS::FAIL;
		add_error(errs);
		in_process() = NULL;
		return;
	}
}

WRITE_TOLrequest*&	WRITE_TOLrequest::in_process() {
	static WRITE_TOLrequest* wr = NULL;
	return wr;
}

//
// Version of process_middle() suitable for being launched
// in a new thread
//
void WRITE_TOLrequest::execute(
		thread_intfc* Thread) {
    WRITE_TOLrequest*	This = in_process();

    if(Thread) {

	//
	// Define global variables for this thread
	//
	thread_index = thread_intfc::MODEL_THREAD;
	global_behaving_object* model_globals = behaving_object::GlobalObject();
	thread_index = thread_intfc::TOL_THREAD;
	behaving_object::GlobalObject() = new global_behaving_object(*model_globals);
    }

    //
    // first evaluate args if necessary. We should
    // check the following strings
    //
    //	- Cstart_time
    //	- Cend_time
    //	- CFileName
    //
    // and stick their values into
    //
    //	- StartTime
    //	- EndTime
    //	- FileName
    //
    This->evaluate_arguments();

    Cstring xmltolfilename;

    if(This->xmltol_requests.get_length()) {
	WRITE_XMLTOLrequest* xmltolreq
		= dynamic_cast<WRITE_XMLTOLrequest*>(
			This->xmltol_requests.first_node());
	xmltolreq->evaluate_arguments();
	xmltolfilename = xmltolreq->FileName;
    }

    //
    // list of resources to display - only
    // used for XMLTOLs
    //
    vector<string> V;

    IO_writer	writer;
    try {
	writer.writeTOL(
			This->FileName,
			xmltolfilename,
			This->StartTime,
			This->EndTime,
			/* do_tol = */ true,
			/* do_xmltol = */ (This->xmltol_requests.get_length() > 0),
			"",	// optional timesystem
			false,	// all activities visible
			V,
			Thread);
    } catch(eval_error Err) {
	if(!Thread) {
		throw(Err);
	} else {
		Thread->errors = Err.msg;
	}
    }
    if(Thread) {
	delete behaving_object::GlobalObject();
	behaving_object::GlobalObject() = NULL;
    }
}

WRITE_XMLTOLrequest::WRITE_XMLTOLrequest(
			const Cstring&		desired_file_name,
			const Cstring&		s1,
			const Cstring&		s2, 
			const Cstring&		theOptionalFilter,
			const Cstring&		theOptionalSchema,
			const Cstring&		theOptionalTimesystem,
			const bool		all_acts_vis,
			const vector<string>&	resources)
		: Action_request("WRITETOL XML"),
			CFileName(desired_file_name),
			Cstart_time(s1),
			Cend_time(s2),
			filter(theOptionalFilter),
			schema(theOptionalSchema),
			timesystem(theOptionalTimesystem),
			AllActsVisible(all_acts_vis),
			running(false)
			{
	for(int i = 0; i < resources.size(); i++) {
		Cstring str = resources[i].c_str();
		if(str[0] == '"') {
			removeQuotes(str);
		}
		which_resources.push_back(*str);
	}
}

WRITE_XMLTOLrequest::WRITE_XMLTOLrequest(
			Cstring& errors,
			ListOVal* args)
		: Action_request("WRITETOL XML"),
			running(false) {
	CTime_base start_time(0, 0, false);
	CTime_base end_time(0, 0, false);
	ArrayElement* name_ae;
	ArrayElement* start_ae;
	ArrayElement* end_ae;
	ArrayElement* schema_ae;
	ArrayElement* filter_ae;
	ArrayElement* timesystem_ae;
	ArrayElement* acts_vis_ae;
	ArrayElement* which_res_ae;

	if((name_ae = args->find("name"))) {
		CFileName = name_ae->Val().to_string();
	} else {
		errors << "Element named \"name\" missing from args. ";
	}
	if((start_ae = args->find("start"))) {
		start_time = start_ae->Val().get_time_or_duration();
		Cstart_time = start_time.to_string();
	} else {
		Cstart_time = "1970-001T00:00:00";
	}
	if((end_ae = args->find("start"))) {
		end_time = end_ae->Val().get_time_or_duration();
		Cend_time = end_time.to_string();
	} else {
		Cend_time = "1970-001T00:00:00";
	}
	if((filter_ae = args->find("filter"))) {
		filter = filter_ae->Val().get_string();
	}
	if((schema_ae = args->find("schema"))) {
		schema = schema_ae->Val().get_string();
	}
	if((timesystem_ae = args->find("timesystem"))) {
		timesystem = timesystem_ae->Val().get_string();
	}
	if((acts_vis_ae = args->find("all_acts_visible"))) {
		AllActsVisible = acts_vis_ae->Val().get_int();
	}
	if((which_res_ae = args->find("resources"))) {
		if(which_res_ae->Val().is_array()) {
			ListOVal&	Res = which_res_ae->Val().get_array();
			ArrayElement*	one_res;

			for(int k = 0; k < Res.get_length(); k++) {
				one_res = Res[k];
				if(one_res->Val().is_string()) {
					which_resources.push_back(*one_res->Val().get_string());
				} else {
					errors = "resources arg. does is not a list of strings";
					break;
				}
			}
		} else {
			errors = "resources arg. is not an array";
		}
	}
}

const Cstring& WRITE_XMLTOLrequest::get_command_text() {
	if(commandText.length() > 0)
		return commandText;
	commandText << commandType << " " << CFileName;
	commandText << " " << Cstart_time << " " << Cend_time;
	commandText << " FILTER ";
	if(filter.is_defined()) {
		commandText << addQuotes(filter);
	} else {
		commandText << " \"DEFAULT\"";
	}
	commandText << " SCHEMA ";
	if(schema.is_defined()) {

		//
		// NOTE: this is obsolete
		//
		commandText << addQuotes(schema);
	} else {
		commandText << " \"DEFAULT\"";
	}
	if(timesystem.is_defined()) {
		commandText << " TIMESYSTEM " << addQuotes(timesystem);
	}
	if(which_resources.size() > 0) {
		aoString 	s;
		bool		first = true;
		commandText << " RESOURCE ";
		for(int i = 0; i < which_resources.size(); i++) {
			if(first) {
				first = false;
				s << " ";
			} else {
				s << ",\n\t";
			}
			s << which_resources[i].c_str();
		}
		s << "\n";
		commandText << s.str();
	} else if(AllActsVisible) {
		commandText << " ALL_ACTS_VISIBLE";
	}
	return commandText;
}

//
// returns true if OK, false if errors found
//
bool WRITE_XMLTOLrequest::evaluate_arguments() {
	Cstring		error_msg;
	TypedValue	result;

	//
	// Old notes:
	//

	/* CHANGE FOR V31.0: if any arg is enclosed in single quotes or backquotes, it's an
	 * expression that needs to be evaluated using Compile().
	 */
	bool start_and_end_times_are_defined = false;
	if(Cstart_time.length()) {
		if(evaluate_string_arg(
				Cstart_time,
				result,
				apgen::DATA_TYPE::TIME,
				error_msg) != apgen::RETURN_STATUS::SUCCESS) {
			add_error(error_msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return false;
		}
		StartTime = result.get_time_or_duration();
		if(StartTime > CTime_base(0, 0, false)) {
			start_and_end_times_are_defined = true;
			if(evaluate_string_arg(
					Cend_time,
					result,
					apgen::DATA_TYPE::TIME,
					error_msg) != apgen::RETURN_STATUS::SUCCESS) {
				add_error(error_msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
				return false;
			}
			EndTime = result.get_time_or_duration();
		}
	}
	if(!start_and_end_times_are_defined) {
		model_intfc::FirstEvent(StartTime);
		model_intfc::LastEvent(EndTime);
	}
	if(evaluate_string_arg(
			CFileName,
			result,
			apgen::DATA_TYPE::STRING,
			error_msg) != apgen::RETURN_STATUS::SUCCESS) {
		// assume that the user didn't bother to add quotes
		result = CFileName;
	}
	FileName = result.get_string();

	//
	// all arguments have now been evaluated
	//
	return true;
}

void WRITE_XMLTOLrequest::process_middle(TypedValue*) {

	//
	// If the process_middle method was invoked, that
	// means we are part of a script and should follow
	// the "regular" execution pattern.
	//
	// If this action request was hijacked by a REMODEL
	// request, it will be executed via a direct call
	// to execute() with a non-NULL thread_intfc pointer.
	//
	try {
		in_process() = this;
		execute(NULL);
		in_process() = NULL;
	} catch(eval_error Err) {
		Cstring errs;
		errs = "An error occurred while trying to create TOL \"";
		errs << FileName << "\":\n";
		errs << Err.msg << "\n";
		theStatus = apgen::RETURN_STATUS::FAIL;
		add_error(errs);
		in_process() = NULL;
		return;
	}
}

WRITE_XMLTOLrequest*&	WRITE_XMLTOLrequest::in_process() {
	static WRITE_XMLTOLrequest* wxr = NULL;
	return wxr;
}

void WRITE_XMLTOLrequest::execute(
		thread_intfc* Thread) {

    //
    // Swap the AR from the unique_ptr; this is a signal to the
    // caller that we are working on things
    //
    WRITE_XMLTOLrequest* This = in_process();

    if(Thread) {

	//
	// Define global variables for this thread
	//
	thread_index = thread_intfc::MODEL_THREAD;
	global_behaving_object* model_globals = behaving_object::GlobalObject();
	thread_index = thread_intfc::XMLTOL_THREAD;
	behaving_object::GlobalObject() = new global_behaving_object(*model_globals);
    }

    //
    // first evaluate args if necessary. We should check the following strings
    //
    //	- Cstart_time
    //	- Cend_time
    //	- CFileName
    //
    // and stick their values into
    //
    //	- StartTime
    //	- EndTime
    //	- FileName
    //
    This->evaluate_arguments();

    //
    // Recent notes:
    //
    // The first task of process_middle() is to compute
    // all the objects needed to call the writeTOLprelude()
    // function template in TOL_write.H.
    //
    try {
	IO_writer	writer;
	writer.writeTOL(
		This->FileName,
		"",
		This->StartTime,
		This->EndTime,
		/* do_tol = */ false,
		/* do_xmltol = */ true,
		This->timesystem,
		This->AllActsVisible,
		This->which_resources,
		Thread);
    } catch(eval_error Err) {
	if(Thread) {
		Thread->errors = Err.msg;
	} else {
		throw(Err);
	}
    }
    if(Thread) {
	delete behaving_object::GlobalObject();
	behaving_object::GlobalObject() = NULL;
    }
}

WRITE_JSONrequest::WRITE_JSONrequest(
			const Cstring&		desired_file_name,
			const Cstring&		s1,
			const Cstring&		s2, 
			const vector<string>&	resources)
		: Action_request("WRITEJSON"),
		CFileName(desired_file_name),
		Cstart_time(s1),
		Cend_time(s2),
		which_resources(resources) {
}

WRITE_JSONrequest::WRITE_JSONrequest(Cstring& errors, ListOVal* args)
	: Action_request("WRITEJSON") {
	CTime_base start_time(0, 0, false);
	CTime_base end_time(0, 0, false);
	ArrayElement* name_ae;
	ArrayElement* start_ae;
	ArrayElement* end_ae;
	ArrayElement* which_res_ae;

	if((name_ae = args->find("name"))) {
		CFileName = name_ae->Val().to_string();
	} else {
		errors << "Element named \"name\" missing from args. ";
	}
	if((start_ae = args->find("start"))) {
		start_time = start_ae->Val().get_time_or_duration();
		Cstart_time = start_time.to_string();
	} else {
		Cstart_time = "1970-001T00:00:00";
	}
	if((end_ae = args->find("start"))) {
		end_time = end_ae->Val().get_time_or_duration();
		Cend_time = end_time.to_string();
	} else {
		Cend_time = "1970-001T00:00:00";
	}
	if((which_res_ae = args->find("resources"))) {
		if(which_res_ae->Val().is_array()) {
			ListOVal&	Res = which_res_ae->Val().get_array();
			ArrayElement*	one_res;

			for(int k = 0; k < Res.get_length(); k++) {
				one_res = Res[k];
				if(one_res->Val().is_string()) {
					which_resources.push_back(*one_res->Val().get_string());
				} else {
					errors = "resources arg. does is not a list of strings";
					break;
				}
			}
		} else {
			errors = "resources arg. is not an array";
		}
	}
}

const Cstring& WRITE_JSONrequest::get_command_text() {
	if(commandText.length() > 0)
		return commandText;
	commandText << commandType << " " << CFileName;
	if(Cstart_time.length() && Cstart_time != "DEFAULT" && Cstart_time != "1970-001T00:00:00") {
		commandText << " " << Cstart_time << " " << Cend_time;
	}
	if(which_resources.size() > 0) {
		aoString 	s;
		bool		first = true;
		commandText << " RESOURCE ";
		for(int i = 0; i < which_resources.size(); i++) {
			if(first) {
				first = false;
				s << " ";
			} else {
				s << ",\n\t";
			}
			s << which_resources[i].c_str();
		}
		s << "\n";
		commandText << s.str();
	}
	return commandText;
}

void WRITE_JSONrequest::process_middle(TypedValue*) {
	IO_writer	writer;
	Cstring		error_msg;
	FILE*		fout;
	TypedValue	result;

	/* CHANGE FOR V31.0: if any arg is enclosed in single quotes or backquotes, it's an
	 * expression that needs to be evaluated using Compile(). */

	/* first evaluate args if necessary. We should check the following strings
	 *
	 *	- Cstart_time
	 *	- Cend_time
	 *	- CFileName
	 *
	 * and stick their values into
	 *
	 *	- StartTime
	 *	- EndTime
	 *	- FileName
	 */
	bool start_and_end_times_are_defined = false;
	if(Cstart_time.length() && Cstart_time != "DEFAULT" && Cstart_time != "1970-001T00:00:00") {
		if(evaluate_string_arg(
				Cstart_time,
				result,
				apgen::DATA_TYPE::TIME,
				error_msg) != apgen::RETURN_STATUS::SUCCESS) {
			add_error(error_msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		StartTime = result.get_time_or_duration();
		if(StartTime > CTime_base(0, 0, false)) {
			start_and_end_times_are_defined = true;
			if(evaluate_string_arg(
					Cend_time,
					result,
					apgen::DATA_TYPE::TIME,
					error_msg) != apgen::RETURN_STATUS::SUCCESS) {
				add_error(error_msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
				return;
			}
			EndTime = result.get_time_or_duration();
		}
	}
	if(!start_and_end_times_are_defined) {
		model_intfc::FirstEvent(StartTime);
		model_intfc::LastEvent(EndTime);
	}
	if(evaluate_string_arg(
			CFileName,
			result,
			apgen::DATA_TYPE::STRING,
			error_msg) != apgen::RETURN_STATUS::SUCCESS) {
		// assume that the user didn't bother to add quotes
		result = CFileName;
	}
	FileName = result.get_string();

	// all args have now been evaluated


	fout = fopen((char *) *FileName, "w");
	if(!fout) {
		error_msg = "An error occurred while trying to open file \"";
		error_msg << FileName << "\" for output\n";
		theStatus = apgen::RETURN_STATUS::FAIL;
		add_error(error_msg);
		return;
	}
	try {
		writer.writeJSON(	fout,
					StartTime,
					EndTime,
					which_resources);
	}
	catch(eval_error Err) {
		error_msg = "An error occurred while trying to create JSON \"";
		error_msg << FileName << "\":\n";
		error_msg << Err.msg << "\n";
		theStatus = apgen::RETURN_STATUS::FAIL;
		add_error(error_msg);
		return;
	}
	fclose(fout);
	return;
}

PRINTrequest::PRINTrequest()
	: Action_request("PRINT")
	{}

const Cstring & PRINTrequest::get_command_text() {
	if (commandText.length() > 0) return commandText;
	commandText = commandType;
	return commandText;
}

void PRINTrequest::process_middle(TypedValue*) {
	if(!serveropt()) {
		/* CHECKED */ cout << *get_command_text_with_modifier() << endl;
	}
	return;
}

QUITrequest::QUITrequest(bool B)
	: Action_request("QUIT"),
		fast_option(B)
	{}

QUITrequest::QUITrequest(Cstring& errors, ListOVal* args)
	: Action_request("QUIT"),
		fast_option(false) {
	ArrayElement* fast_arg = NULL;

	if((fast_arg = args->find("fast"))) {
		fast_option = fast_arg->Val().get_int(); }
	else {
		errors << "Element named \"fast\" is missing from args list. "; }
	}

const Cstring & QUITrequest::get_command_text() {
	if (commandText.length() > 0) return commandText;
	if(fast_option) {
		commandText << commandType;
		commandText << " 1"; }
	else {
		// usually you don't want to quit at the end of the script
		commandText << "#" << commandType << " 0"; }
	return commandText; }

void QUITrequest::process_middle(TypedValue* val) {
	TypedValuePtrVect empty;
	EventRegistry::Register().PropagateEvent("QUIT", empty);

	if(fast_option) {
		APcloptions::theCmdLineOptions().ExitFlag = 2; }
	else {
		APcloptions::theCmdLineOptions().ExitFlag = 1; }
		
	if(val) {
		*val = 1L; }
	return; }

NEW_ACTIVITYrequest::NEW_ACTIVITYrequest(	const Cstring&	type, 
						const Cstring&	the_name, 
						const Cstring&	id, 
						const Cstring&	time, 
						const Cstring&	legend)
	: Action_request("NEWACTIVITY"),
	act_type(type),
	act_id(id),
	act_name(the_name),
	act_time(time),
	act_legend(legend) {}

NEW_ACTIVITYrequest::NEW_ACTIVITYrequest(	const Cstring&	type, 
						const Cstring&	the_name, 
						const Cstring&	id, 
						const Cstring&	time, 
						const Cstring&	legend,
						const pairslist& params_keyword_value_pairs
						)
	: Action_request("NEWACTIVITY"), 
	act_type(type), 
	act_name(the_name), 
	act_id(id),
	act_time(time),
	act_legend(legend),
	params_as_kwd_val_pairs(params_keyword_value_pairs) {}

NEW_ACTIVITYrequest::NEW_ACTIVITYrequest(Cstring& errors, ListOVal* args)
	: Action_request("NEWACTIVITY") {
	ArrayElement* type_ae = NULL;
	ArrayElement* name_ae = NULL;
	ArrayElement* id_ae = NULL;
	ArrayElement* time_ae = NULL;
	ArrayElement* legend_ae = NULL;
	ArrayElement* params_ae = NULL;

	if((type_ae = args->find("type"))) {
		act_type = type_ae->Val().get_string();
	} else {
		errors << "Element named \"type\" is missing from args list. ";
	}
	if((name_ae = args->find("name"))) {
		act_name = name_ae->Val().get_string();
	} else {
		errors << "Element named \"name\" is missing from args list. "; }
	if((id_ae = args->find("id"))) {
		act_id = id_ae->Val().get_string();
	} else {
		errors << "Element named \"id\" is missing from args list. ";
	}
	if((time_ae = args->find("start"))) {
		act_time = time_ae->Val().get_time_or_duration().to_string();
	} else {
		errors << "Element named \"start\" is missing from args list. ";
	}
	if((legend_ae = args->find("legend"))) {
		act_legend = legend_ae->Val().get_string();
	} else {
		errors << "Element named \"legend\" is missing from args list. ";
	}

	if((params_ae = args->find("params"))) {
		if(!params_ae->Val().is_array()) {
			errors << "params element is not an array of keyword-value pairs";
		} else {
			ListOVal&	theParams = params_ae->Val().get_array();
			ArrayElement*	one_pair;

			for(int k = 0; k < theParams.get_length(); k++) {
				one_pair = theParams[k];
				Cstring theValue = one_pair->Val().to_string();
				params_as_kwd_val_pairs << new symNode(
						one_pair->get_key(),
						theValue);
			}
		}
	}
}

const Cstring &NEW_ACTIVITYrequest::get_command_text() {
	symNode*		sn;
	pairslist::iterator	pars(params_as_kwd_val_pairs);

	if(commandText.length() > 0) {
		 return commandText;
	}
	commandText = commandType;
	commandText << " " << *act_type;
	if(act_name.length()) {
		commandText << " NAME " << act_name;
	}
	commandText << " ID " << act_id;
	commandText << " TO " << act_time;
	if(act_legend.has_value()) {
		//legend can be just numerals, or contain blanks, etc., so quote it:
		commandText << " LEGEND " << addQuotes(act_legend);
	}
	if(params_as_kwd_val_pairs.get_length())
		commandText << " < ";
	while((sn = pars())) {
		commandText << " " << sn->get_key() << " = " << addQuotes(sn->payload);
		if(sn->next_node()) {
			commandText << ",";
		}
	}
	if(params_as_kwd_val_pairs.get_length()) {
		commandText << " > ";
	}
	return commandText;
}

void NEW_ACTIVITYrequest::process_middle(TypedValue* ret_val) {
	Cstring				id;
	CTime_base			the_time;

	try {
		is_a_valid_time(act_time, the_time);
	} catch(eval_error Err) {
		Cstring theMessage("Activity start time is improperly formatted; error:\n");
	  
		theMessage << Err.msg << ".  An example of a legal time format is "
		     << "2001-200T00:10:00.123\n";
		add_error(theMessage);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}

	id = act_id;
	try {
		apgenDB::CreateActivityInPlan(	*act_type,
						*act_name,
						*act_legend,
						the_time,
						params_as_kwd_val_pairs,
						id);
	} catch(eval_error &Err) {
		add_error(Err.msg);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}

	// Quick, quick! Grab the TRUE id before anybody has a chance to use the
	// requested id, which is almost certainly NOT right!
	act_id = id;
	// make sure no one uses old stuff:
	commandText.undefine();

	// apgenDB::ClearSelectedActivities();
	// apgenDB::SelectActivity(*id);

	//notify the client of this id
	add_id_to_list(act_id);
	if(ret_val) {
		Cntnr<alpha_string, ActivityInstance*>*	theTag = aaf_intfc::actIDs().find(id);
		*ret_val = *(theTag->payload); }

	return;
}

const Cstring &NEW_ACTIVITYrequest::get_type() {
	return act_type;
}

const Cstring &NEW_ACTIVITYrequest::get_id() {
	return act_id; }

const Cstring &NEW_ACTIVITYrequest::get_name() {
	return act_name; }

const Cstring &NEW_ACTIVITYrequest::get_legend() {
	return act_legend; }

const Cstring &NEW_ACTIVITYrequest::get_time() const {
	return act_time; }

NEW_ACTIVITIESrequest::NEW_ACTIVITIESrequest(const Cstring &type, const Cstring &startTime, 
		      const Cstring &period, int num_times, const Cstring &legend)
  : Action_request("NEWACTIVITIES"),
    Type(type),
    Legend(legend),
    StartTime(startTime),
    Period(period),
    NumTimes(num_times) {}

NEW_ACTIVITIESrequest::NEW_ACTIVITIESrequest(Cstring& errors, ListOVal* args)
  : Action_request("NEWACTIVITIES") {
	ArrayElement*	type_ae;
	ArrayElement*	legend_ae;
	ArrayElement*	start_time_ae;
	ArrayElement*	period_ae;
	ArrayElement*	num_times_ae;

	if((type_ae = args->find("type"))) {
		Type = type_ae->Val().get_string(); }
	else {
		errors << "Element named \"type\" is missing from args. "; }
	if((legend_ae = args->find("legend"))) {
		Legend = legend_ae->Val().get_string(); }
	else {
		errors << "Element named \"legend\" is missing from args. "; }
	if((start_time_ae = args->find("start"))) {
		if(start_time_ae->Val().is_time()) {
			StartTime = start_time_ae->Val().get_time_or_duration().to_string(); }
		else if(start_time_ae->Val().is_string()) {
			StartTime = start_time_ae->Val().get_string(); }
		else {
			errors << "Element named \"start\" is neither a time nor a string. "; } }
	else {
		errors << "Element named \"start\" is missing from args. "; }
	if((period_ae = args->find("period"))) {
		if(period_ae->Val().is_time()) {
			Period = period_ae->Val().get_time_or_duration().to_string(); }
		else if(period_ae->Val().is_string()) {
			Period = period_ae->Val().get_string(); }
		else {
			errors << "Element named \"period\" is neither a time nor a string. "; } }
	else {
		errors << "Element named \"period\" is missing from args. "; }
	if((num_times_ae = args->find("number"))) {
		if(num_times_ae->Val().is_int()) {
			NumTimes = num_times_ae->Val().get_int(); }
		else if(num_times_ae->Val().is_string()) {
			NumTimes = atoi(*num_times_ae->Val().get_string()); }
		else {
			errors << "Element named \"number\" is neither an int nor a string. "; } }
	else {
		errors << "Element named \"number\" is missing from args. "; } }

const Cstring &NEW_ACTIVITIESrequest::get_command_text() {
	if(commandText.length() > 0) {
		return commandText; }
  
	commandText = commandType;
	commandText << " " << Type;
  
	commandText << " TO " << StartTime;

	commandText << " PERIOD " << Period;
	commandText << " TIMES " << NumTimes;

	if(Legend.is_defined()) {
		//legend can be just numerals, or contain blanks, etc., so quote it:
		commandText << " LEGEND " << addQuotes(Legend); }

	return commandText;
}

void
NEW_ACTIVITIESrequest::process_middle(TypedValue* ret_val) {
	CTime_base	startTime;
	CTime_base	period;
	pairslist	empty_vec;
	stringslist	ids;

	try {
		//get startTime
		is_a_valid_time(StartTime, startTime);
		is_a_valid_duration(Period, period);
	}
	catch(eval_error Err) {
		// time formatting error, thrown by is_a_valid_time
		Cstring message("Improperly formatted time; error:\n");
		message << Err.msg << ".  An example of a legal time format is ";
		message << "2002-200T00:10:00.123\n";
		add_error(message);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
	try {
		// apgenDB::ClearSelectedActivities();
		for(int i = 0; i < NumTimes; i++) {
			Cstring id(Type);
			CTime_base fullDuration = period;
			fullDuration *= i;
			CTime_base newStart = startTime + fullDuration;

			apgenDB::CreateActivityInPlan(	Type,
							Type,
							Legend,
							newStart,
							empty_vec,
							id);
			ids << new emptySymbol(id);
		}

		stringslist::iterator iter(ids);
		emptySymbol*	N;
		ListOVal*	L = new ListOVal;
		ArrayElement*	tds;
		TypedValue	V;
		Cstring		temp_error;

		while((N = iter())) {
			Cntnr<alpha_string, ActivityInstance*>*	theTag = aaf_intfc::actIDs().find(N->get_key());

			add_id_to_list(N->get_key());
			V = *(theTag->payload);
			tds = new ArrayElement(N->get_key(), apgen::DATA_TYPE::INSTANCE);
			tds->set_typed_val(V);
			L->add(tds);
		}
		*ret_val = *L;
	}
	catch(eval_error &Err) {
		//DB error
		add_error(Err.msg);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
}

EDIT_ACTIVITYrequest::EDIT_ACTIVITYrequest(const pairslist& elist, int n, const Cstring& stringID)
	: Action_request("EDITACTIVITY"),
		assign_list(elist),
		carry(n),
		nodeID(stringID) {
	symNode*	assign = assign_list.find("legend");

	if(assign) {
		if(assign->payload & "0Hopper:") {
			Cstring		theRealLegend(":" / assign->payload);
			symNode*	stat_attr = assign_list.find("Status");

			assign->payload = theRealLegend;
			if(!stat_attr) {
				assign_list << new symNode("status", "\"unscheduled\"");
			}
		}
	}
}

EDIT_ACTIVITYrequest::EDIT_ACTIVITYrequest(Cstring& errors, ListOVal* args)
	: Action_request("EDITACTIVITY"),
		carry(true) {
	ArrayElement*	carry_ae;
	ArrayElement*	params_ae;
	ArrayElement*	id_ae;

	if((id_ae = args->find("id"))) {
		nodeID = id_ae->Val().get_string();
	} else {
		errors = "Element named \"id\" is missing. ";
	}
	if((params_ae = args->find("params"))) {
		if(!params_ae->Val().is_array()) {
			errors << "Element named \"params\" is not an array. ";
		} else {
			ListOVal&					theParams(params_ae->Val().get_array());
			ArrayElement*					a_param;

			for(int k = 0; k < theParams.get_length(); k++) {
				a_param = theParams[k];
				Cstring parname = a_param->get_key();
				Cstring parvalue;

				parvalue = a_param->Val().to_string();
				assign_list << new symNode(parname, parvalue);
			}
		}
	} else {
		errors << "Element named \"params\" does not exist. ";
	}
	if((carry_ae = args->find("carry"))) {
		carry = carry_ae->Val().get_int();
	}
}

const Cstring& EDIT_ACTIVITYrequest::get_command_text() {
	if (commandText.length() > 0)
		return commandText;
	commandText = commandType;

	if (nodeID.has_value()) commandText << " ID " << nodeID;
				// " -CARRY" Used to be " NOCARRY" which leads to ambiguous syntax...
	if (!carry) commandText << " -CARRY";

	symNode*		assign;
	int			use_nameless_parameters;

	assign = assign_list.first_node();
	if(assign) {
		int	use_nameless_parameters = (assign->get_key().length() == 0);

		if(use_nameless_parameters)
			commandText << "(";
		else
			commandText << "\n    < ";
		while(assign) {
			if(! use_nameless_parameters) {
				commandText << assign->get_key();
				commandText << " = ";
				commandText << addQuotes(assign->payload);
			} else if(assign->payload.length()) {
				commandText << addQuotes(assign->payload);
			}
			assign = assign->next_node();
			if (assign) {
				commandText << ",\n    ";
			}
		}
		if(use_nameless_parameters) {
			commandText << ")";
		} else {
			commandText << " >"; }
		}
	else {
		commandText << " < >";
	}
	return commandText;
}

bool is_a_simple_duration(pEsys::executableExp* s) {
	return true;
}

void EDIT_ACTIVITYrequest::to_script(Cstring& s) const {
	s << "xcmd(\"EDITACTIVITY\", [\"id\" = " << addQuotes(nodeID) << ", \"carry\" = ";
	if(!carry) {
		s << "false, ";
	} else {
		s << "true, ";
	}

	symNode*	assign;
	bool		first_node = true;

	s << "\"params\" = [";
	assign = assign_list.first_node();
	while(assign) {
		if(first_node) {
			first_node = false;
		} else {
			s << ", ";
		}
		s << addQuotes(assign->get_key());
		s << " = ";
		s << addQuotes(assign->payload);
		assign = assign->next_node();
	}
	s << "]]);\n";
}

void EDIT_ACTIVITYrequest::process_middle(TypedValue* ret_val) {
	Cstring			err_msg;		// (not returned yet)
	Cntnr<alpha_string, ActivityInstance*>*	tagnode = NULL;	// points to request
	ActivityInstance*	request = NULL;	// ID'd activity request
	long			colorinx, i;
	TypedValue		tval;

	parsedExp		startExpression;	//  Start (keep around)

	TypedValue		Value;

	symNode*		typeNode = NULL;
	symNode*		nameNode = NULL;
	symNode*		legendNode = NULL;
	symNode*		colorNode = NULL;
	symNode*		patternNode = NULL;
	symNode*		descriptionNode = NULL;
	symNode*		startNode = NULL;
	symNode*		durationNode = NULL;
	symNode*		parameterNode = NULL;
	Cstring			nameValue, legendValue, colorValue;
	Cstring			patternValue, descriptionValue;
	Cstring			startValue, durationValue;
	Cstring			otherAttrName, otherAttrValue;
	Cstring			parameterName, parameterValue;
	Cstring			theColorName;

	Cstring			temp;
	char			charOfName;
	pEsys::executableExp*	attribute_in_type = NULL;
	const char*		oneDoubleQuote = "\"";
	pairslist::iterator	theListOfAssignments(assign_list);
	bool			NamelessParameterListFlag = true;
	bool			the_parameters_have_changed;

	//If null command (no pairs), nothing to do!
	if (!assign_list.get_length()) {
		return;
	}

	// Step 1: Find ACT_req from ID

	if(aaf_intfc::actIDs().get_length()) {
		try {
			tagnode = find_tag_from_real_or_symbolic_id(nodeID);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: cannot find act. instance; ")
				+ err_msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}

		assert(tagnode);
		request = tagnode->payload;
	} else { //empty list of IDs
		add_error("Edit Activity Error: no activities to edit\n");
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}

	if(ret_val) {
		*ret_val = *request;
	}

	const task&	constructor = request->Object->Task;
	const Behavior&	type = constructor.Type;

	// Step 2:	Get (pointer to) activity type underlying the ACT_req (get NULL if generic
	//		type or if type is not (yet) defined).

	// NOTE: step 3 below seems wasteful. Let's try to eliminate it -- CHANGE_EDIT_ACTIVITY

	// Step 3:	Set up a "shadow" Local Symbol Table from the ACT_req:  copy contents,
	//		and then replace values for matching symbols, or add new Nodes for new
	//		symbols.  If a problem occurs, then exit with error, before the final
	//		step of replacing the values in the ACT_req's LST (plus adding new
	//		Nodes) with the modified values in the "shadow" Symbol Table.

	//Unpack symbol-value pairs, and put in the correct place (Type and Name
	//  are NOT in symbol table). See ACT_req::ACT_req for how to assign value.
	//  DELETE each Node as it is handled, so don't see it again (Nodes were
	//  created by caller of EDIT_ACTIVITYrequest::process()).  DON'T delete
	//  the List, however (end up with empty List, which is OK).  This is the
	//  best place to VERIFY that values are OK, since if not, just exit now!

	// Step 3b:	unpack Type (NOT in Symbol Table).  ???IGNORE Type change FOR NOW.
	if((typeNode = assign_list.find("type"))) {
		;	//??? IGNORE Type change FOR NOW (no-op)
		assign_list.delete_node(typeNode);
	} else {
		;
	}

	// Step 3c:	name (NOT in Symbol Table)
	if((nameNode = assign_list.find("name"))) {
		nameValue = nameNode->payload;
		// nameValue = normalizeString(nameValue, '_');
		assign_list.delete_node(nameNode);
	} else {
		nameValue.undefine();
	}

	if(nameValue.is_defined() && !nameValue.has_value()) {
		//null string
		add_error("Edit Activity Error: name is the null string\n");
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	} else if(nameValue.is_defined()) {
		//name is defined,and NOT a null string
		//'tokens' allows names beginning with alpha char, with 0+ alphanumeric
		//  chars following.  alpha is [a-zA-Z_] and alphanumeric [a-zA-Z0-9_].
		//  All this logic depends on ASCII collating sequence!!!
		charOfName = nameValue[0];		//know Name is 1+ chars
		if((charOfName >= '0') && (charOfName <= '9')) {
			add_error(Cstring("Edit Activity Error: illegal name ") + nameValue + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		} else if(!(	(charOfName >= 'a' && charOfName <= 'z')
			|| (charOfName >= 'A' && charOfName <= 'Z')
			|| (charOfName == '_'))) {
			add_error(Cstring("Edit Activity Error: illegal name ") + nameValue + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		for(i = 1; i < nameValue.length(); i++) {
			charOfName = nameValue[i];
			if (!	((charOfName >= 'a' && charOfName <= 'z')
				|| (charOfName >= 'A' && charOfName <= 'Z')
				|| (charOfName >= '0' && charOfName <= '9')
				|| (charOfName == '_'))) {
				add_error(Cstring("Edit Activity Error: illegal name ") + nameValue + "\n");
				theStatus = apgen::RETURN_STATUS::FAIL;
				return;
			}
		}
		tval = nameValue;
		map<Cstring, int>::const_iterator name_ind = request->Object->Task.get_varindex().find("name");
		assert(name_ind != request->Object->Task.get_varindex().end());
		try {
			request->Object->assign_w_side_effects(name_ind->second, tval);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: problem setting name to ") + nameValue +
				":\n" + Err.msg + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
			
	}

	// Step 3d:	handle simple Attributes (Legend, Color, Pattern, Description;
	//  all except Description are required, and will therefore exist)

	//Legend is treated as a literal string -- no evaluation is done.
	if((legendNode = assign_list.find("legend"))) {
		legendValue = legendNode->payload;
		assign_list.delete_node(legendNode);
	} else {
		legendValue.undefine();
	}
	if(legendValue.is_defined() && !legendValue.has_value()) {
		//null string
		add_error("Edit Activity Error: Illegal value <empty string> for Legend\n");
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	} else if(legendValue.is_defined()) {
		//name is defined,and NOT a null string
		tval = legendValue;
		map<Cstring, int>::const_iterator name_ind = request->Object->Task.get_varindex().find("legend");
		assert(name_ind != request->Object->Task.get_varindex().end());
		try {
			request->Object->assign_w_side_effects(name_ind->second, tval);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: problem setting legend to ")
				+ legendValue + ":\n" + Err.msg + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
	}

	//FUTURE if legendValue is NEW legend, ERROR OUT (unless allow from cmd)

	// STEP 3E: color is treated as an expression, to be evaluated to an integer.
	// For nifty DEBUG option (type Color expression in Description field) use:
	//  if (colorNode = (symNode*)assign_list.find("description"))	//DEBUG
	if((colorNode = assign_list.find("color"))) {
		colorValue = colorNode->payload;
		assign_list.delete_node(colorNode);
	}		//COMMENT OUT for DEBUG
	else {
		colorValue.undefine();
	}
	if(colorValue.has_value()) {
		//Must explicitly set expected type, so eval_expr() can check type.
		//Note, this evaluation uses ORIGINAL local symbols, not any new ones!
		//  (DSG and PFM agree this is the only RELIABLE way to proceed, though
		//  the user may want new symbol values to depend on other new values).
		// PFM 2/3/98
		try {
			Value.undefine();
			Value.declared_type = apgen::DATA_TYPE::INTEGER;
			compiler_intfc::CompileAndEvaluate(colorValue, request->Object, Value);
		}
		catch(eval_error Err) {
			add_error("Edit Activity Error: syntax error in color expr. ");
			temp << colorValue;
			temp << " -\n" << Err.msg << "\n";
			add_error(temp);
			add_error("\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		//Evaluated OK, so has already been successfully cast as an integer,so
		if((Value.get_int() < 0)	//check if out-of-range
		 || (Value.get_int() >= NUM_OF_COLORS)) {
			err_msg = "Result of expression ";
			err_msg << colorValue;
			err_msg << " = " << Value.get_int();
			err_msg << " is out of range of valid Colors.\n";
			add_error(err_msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		theColorName = Patterncolors[Value.get_int()].colorname;
		tval = theColorName;
		map<Cstring, int>::const_iterator name_ind = request->Object->Task.get_varindex().find("Color");
		assert(name_ind != request->Object->Task.get_varindex().end());
		try {
			request->Object->assign_w_side_effects(name_ind->second, tval);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: problem setting Color to ")
				+ theColorName + ":\n" + Err.msg + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
	}

	// STEP 3F: Color is treated as an expression, to be evaluated as a string.
	if((colorNode = assign_list.find("Color"))) {
		colorValue = colorNode->payload;
		assign_list.delete_node(colorNode);
	}		//COMMENT OUT for DEBUG
	else {
		colorValue.undefine();
	}
	if(colorValue.has_value()) {
		try {
			Value.undefine();
			Value.declared_type = apgen::DATA_TYPE::STRING;
			compiler_intfc::CompileAndEvaluate(
					colorValue,
					request->Object,
					Value);
		}
		catch(eval_error Err) {
			add_error("Edit Activity Error: syntax error in color expr. ");
			temp << colorValue;
			temp << " -\n" << Err.msg << "\n";
			add_error(temp);
			add_error("\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		//Evaluated OK, so has already been successfully cast as an integer,so
		theColorName = Value.get_string();
		tval = theColorName;
		map<Cstring, int>::const_iterator name_ind = request->Object->Task.get_varindex().find("Color");
		assert(name_ind != request->Object->Task.get_varindex().end());
		try {
			request->Object->assign_w_side_effects(name_ind->second, tval);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: problem setting Color to ")
				+ theColorName + ":\n" + Err.msg + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		// TypedSymbol::set_typed_symbol(NewSymbolTable.find("Color"), tval, err_msg);
	}

	//Pattern is treated as an expression, to be evaluated to an integer.
	//For nifty DEBUG option (type Pattern expression in Description field) use:
	//  if (patternNode = assign_list.find("description"))	//DEBUG
	if ((patternNode = assign_list.find("pattern"))) {
		patternValue = patternNode->payload;
		assign_list.delete_node(patternNode);
	} //COMMENT OUT for DEBUG
	else {
		patternValue.undefine();
	}
	if (patternValue.has_value()) {
		try {
			Value.undefine();
			Value.declared_type = apgen::DATA_TYPE::INTEGER;
			compiler_intfc::CompileAndEvaluate(patternValue, request->Object, Value);
		}
		catch(eval_error Err) {
			add_error("Edit Activity Error: Syntax Error in Pattern expression ");
			temp << patternValue;
			temp << " -\n" << Err.msg << "\n";
			add_error(temp);
			add_error("\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		if ((Value.get_int() < 0)	//check if out-of-range
		 || (Value.get_int() >= NUM_OF_PATTERNS)) {
			err_msg = "Edit Activity Error: Result of expression ";
			err_msg << patternValue;
			err_msg << " = " << Value.get_int();
			err_msg << " is out of range of valid patterns.\n";
			add_error(err_msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		//update numeric "pattern" code (0-63):
		map<Cstring, int>::const_iterator name_ind = request->Object->Task.get_varindex().find("pattern");
		assert(name_ind != request->Object->Task.get_varindex().end());
		try {
			request->Object->assign_w_side_effects(name_ind->second, Value);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: problem setting pattern to ")
				+ Value.get_int() + ":\n" + Err.msg + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
	}

	//Description is treated as a literal string, which may contain any text
	//  EXCEPT for the double-quote '"' character.
	if((descriptionNode = assign_list.find("description"))) {
		descriptionValue = descriptionNode->payload;
		assign_list.delete_node(descriptionNode);
	}
	else {
		descriptionValue.undefine();
	}	//this is NOT same as null string

	//NOTE descriptionValue can legitimately be null string (that clears it),so
	if(descriptionValue.is_defined()) {	//<-- used, instead of has_value()
		tval = descriptionValue;
		map<Cstring, int>::const_iterator name_ind = request->Object->Task.get_varindex().find("description");
		assert(name_ind != request->Object->Task.get_varindex().end());
		try {
			request->Object->assign_w_side_effects(name_ind->second, tval);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: problem setting description to ")
				+ Value.get_int() + ":\n" + Err.msg + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
	}	//alpha blist,so inserts alphabetically

	//FOURTH, handle difficult Attributes:  Start, Duration.  Note that both of
	//  these can affect the "finish" entry in the Symbol Table, which is in
	//  essence, a "hidden", derived Attribute.

	//Start is treated as an expression, to be evaluated to a TIME.
	if((startNode = assign_list.find("start"))) {
		startValue = startNode->payload;
		assign_list.delete_node(startNode);
	} else {
		startValue.undefine();
	}
	if(startValue.has_value()) {
		parsedExp temp;
		// Rationale: if activity is a request, start doesn't exist.
		try {
			compiler_intfc::CompileExpression(startValue, temp);
			startExpression = temp;
		}
		catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: Syntax error in Start expression; details:\n"));
			add_error(" -\n");
			add_error(Err.msg);
			add_error("\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		try {
			Value.undefine();
			Value.declared_type = apgen::DATA_TYPE::TIME;
			compiler_intfc::Evaluate(startExpression, request->Object, Value);
		}
		catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: Evaluation error in Start expression; details:\n"));
			add_error(Err.msg);
			add_error("\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		//Evaluated OK, so has already been successfully cast as a time,so
		if(Value.get_time_or_duration() < CTime_base(0, 0, false)) { //check if out-of-range
			add_error(Cstring("Edit Activity Error: Range Error in Start expression; ")
				+ " result of expression is before January 1, 1970.\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		//Update the Start Attribute:
		map<Cstring, int>::const_iterator name_ind = request->Object->Task.get_varindex().find("start");
		assert(name_ind != request->Object->Task.get_varindex().end());
		try {
			request->Object->assign_w_side_effects(name_ind->second, Value);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: problem setting start to ")
				+ startValue + ":\n" + Err.msg + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
	}

	//Duration is treated as an expression, to be evaluated to a DURATION.
	//  Note that Duration is a SPECIAL CASE!!!
	if((durationNode = assign_list.find("span"))) {
		durationValue = durationNode->payload;
		assign_list.delete_node(durationNode);
	} else {
		durationValue.undefine();
	}
	//Handle SPECIAL CASE of non-simple (i.e. formula) Duration, AFTER the
	//  "shadow" symbol table has been fully updated, and used to update the
	//  ACT_req's own symbol table.  This is because we need the formula to be
	//  evaluated using ALL OF THE NEW INFO!!!  THIS IS THE ONLY CASE OF THIS,
	//  everything else uses the OLD values, for consistency.  Code for this
	//  special case is therefore at the very bottom of this method.

	if(constructor.prog) {
		pEsys::Program* pg = constructor.prog.object();
		// no longer correct:
		// attribute_in_type = (pEsys::instruction *) pg->find("span");
		attribute_in_type = NULL;
	}
		

	if (	durationValue.has_value()		//want to update Duration
		&& (	(request->Object->Task.Type.name == "generic")// AND type is generic OR
			|| (attribute_in_type
			    && is_a_simple_duration(attribute_in_type))
			|| (!attribute_in_type)
		   )
	   ) {
		try {
			Value.undefine();
			Value.declared_type = apgen::DATA_TYPE::DURATION;
			compiler_intfc::CompileAndEvaluate(
					durationValue,
					request->Object,
					Value);
		}
		catch(eval_error Err) {
			add_error("Edit Activity Error: Syntax Error in Duration expression ");
			add_error(durationValue);
			add_error(" -\n");
			add_error(Err.msg);
			add_error("\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		//Evaluated OK, so has already been successfully cast as DURATION.  PFM
		//  agrees a range check for negative instance Duration is a good idea.
		if(Value.get_time_or_duration() < CTime_base(0, 0, true)) { //OK for DURATIONs
			err_msg = "Result of expression=";
			err_msg << Value.get_time_or_duration().to_string();
			err_msg << " is negative.\n";
			add_error(Cstring("Edit Activity Error: ") + err_msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}

		//Update the Duration Attribute:
		map<Cstring, int>::const_iterator name_ind = request->Object->Task.get_varindex().find("span");
		assert(name_ind != request->Object->Task.get_varindex().end());
		try {
			request->Object->assign_w_side_effects(name_ind->second, Value);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: problem setting span to ")
				+ durationValue + ":\n" + Err.msg + "\n");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		
	}
	//If request update and that is not possible (have formula, or have an
	//  undefined type), don't allow it!
	else if(durationValue.has_value()) {	//want to update Duration, but ILLEGAL
		err_msg = "Activity type defines Duration as a formula, so Duration cannot be modified.\n";
		add_error(Cstring("Edit Activity Error: ") + err_msg);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
	//Else, no request to update Duration

	pairslist::iterator iter(assign_list);
	symNode*	one_item;
	while((one_item = iter())) {
		Cstring value_string = one_item->payload;
		if(!request->Object->defines_property(one_item->get_key())) {
			add_error(Cstring("Edit Activity Error: item ") + one_item->get_key() + " not found.");
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		int	oldval_index = request->Object->Task.get_varindex().at(one_item->get_key());
		TypedValue& 	oldval = (*request->Object)[oldval_index];
		try {
			tval.undefine();
			tval.declared_type = oldval.declared_type;
			compiler_intfc::CompileAndEvaluate(value_string, request->Object, tval);
			map<Cstring, int>::const_iterator name_ind = request->Object->Task.get_varindex().find(one_item->get_key());
			assert(name_ind != request->Object->Task.get_varindex().end());
			request->Object->assign_w_side_effects(name_ind->second, tval);
		} catch(eval_error Err) {
			add_error(Cstring("Edit Activity Error: problem setting " + one_item->get_key() + " to "
				+ durationValue + ":\n" + Err.msg + "\n"));
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
	}
}

EDIT_GLOBALrequest::EDIT_GLOBALrequest(const Cstring& name, const Cstring& valuestring)
	: Action_request("EDITGLOBAL"),
		global_value(valuestring),
		global_name(name) {
}

EDIT_GLOBALrequest::EDIT_GLOBALrequest(Cstring& errors, ListOVal* args)
	: Action_request("EDITGLOBAL") {
	ArrayElement*	value_ae;
	ArrayElement*	name_ae;

	if((name_ae = args->find("name"))) {
		if(!name_ae->Val().is_string()) {
			errors << "Element named \"name\" is not a string. ";
		} 
		global_name = name_ae->Val().get_string();
	} else {
		errors = "Element named \"name\" is missing. ";
	}
	if((value_ae = args->find("value"))) {
		if(!value_ae->Val().is_string()) {
			errors << "Element named \"value\" is not a string. ";
		} 
		global_value = value_ae->Val().get_string();
	} else {
		errors << "Element named \"value\" does not exist. ";
	}
}

const Cstring& EDIT_GLOBALrequest::get_command_text() {
	if (commandText.length() > 0)
		return commandText;
	commandText = commandType;
	commandText << " " << global_name << " = " << global_value;
	return commandText;
}

void EDIT_GLOBALrequest::to_script(Cstring& s) const {
	s << "xcmd(\"EDITGLOBAL\", [\"name\" = " << addQuotes(global_name)
		<< ", \"value\" = " << addQuotes(global_value) << "]);\n";
};

void EDIT_GLOBALrequest::process_middle(TypedValue* ret_val) {
	TypedValue	result;
	Cstring		value_string;

	try {

		//
		// Note: the cmd grammar has already stripped quotes
		// from the script command
		//
		value_string = global_value;
		compiler_intfc::CompileAndEvaluateSelf(value_string, result);

		int level_to_use;
		int index_to_use;
		apgen::DATA_TYPE the_type = apgen::DATA_TYPE::UNINITIALIZED;
		bool found = aafReader::find_symbol_in_task(
						global_name,
						level_to_use,
						index_to_use,
						the_type);
		if(!found) {
			Cstring err_msg;
			err_msg << "Edit Global Error: "
				<< "Symbol " << global_name
				<< " not found.\n";
			add_error(err_msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		assert(level_to_use == 0);
		(*behaving_object::GlobalObject())[index_to_use] = result;
	} catch(eval_error Err) {
		add_error(Cstring("Edit Global Error: problem setting " + global_name + " to "
			+ value_string + ":\n" + Err.msg + "\n"));
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
	theStatus = apgen::RETURN_STATUS::SUCCESS;
}


apgen::RETURN_STATUS ACTIVITY_LISTrequest::get_pointers() {
	if(!list_of_act_pointers.get_length()) {
		bool		no_no_no;

		if(extract_pointers(no_no_no) != apgen::RETURN_STATUS::SUCCESS) {
			theStatus = apgen::RETURN_STATUS::FAIL;
			return apgen::RETURN_STATUS::FAIL;
		} else if(no_no_no) {
			return apgen::RETURN_STATUS::FAIL;
		}
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS ACTIVITY_LISTrequest::extract_pointers(bool& selection_contains_noneditable_activities) {
	Cstring		errors;
	ActivityInstance	*req;
	bool			there_are_errors = false;

	selection_contains_noneditable_activities = 0;
	if(list_of_act_names.get_length()) {
		stringslist::iterator		copy2(list_of_act_names);
		emptySymbol*			sn;
		Cntnr<alpha_string, ActivityInstance*>*	tag;

		list_of_act_pointers.clear();
		while((sn = copy2())) {
			errors.undefine();
			try {
				tag = find_tag_from_real_or_symbolic_id(sn->get_key());
			} catch(eval_error Err) {
				there_are_errors = true;
				add_error(Err.msg);
				// V31.0 changes, round 2
				// return apgen::RETURN_STATUS::FAIL;
			}
			if(!there_are_errors) {
				list_of_act_pointers << new dumb_actptr(
								tag->payload,
								tag->payload);
			}
		}
	}

	// at Steve Wissler's request (December 2016):
#ifdef LETS_NOT_MAKE_THIS_AN_ERROR_ANYMORE
	else if(commandType != "PASTEACTIVITY" && !list_of_act_pointers.get_length()) {
		add_error("Activity List Error: empty ID list\n");
		return apgen::RETURN_STATUS::FAIL;
	}
#endif /* LETS_NOT_MAKE_THIS_AN_ERROR_ANYMORE */

	if(selection_contains_noneditable_activities) {
		list_of_act_pointers.clear();
	}
	if(there_are_errors) {
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void ACTIVITY_LISTrequest::set_selection() {
	slist<alpha_void, dumb_actptr>::iterator new_pointers(list_of_act_pointers);
	dumb_actptr*				ptr;
	smart_actptr*				ptr_node;
	slist<alpha_void, smart_actptr>		selection_list;
	slist<alpha_void, smart_actptr>::iterator li(selection_list);

	eval_intfc::get_all_selections(selection_list);
	while((ptr_node = li())) {
		ptr_node->BP->unselect(); }
	while((ptr = new_pointers())) {
		ptr->payload->select(); } }

void Action_request::set_to_selection(TypedValue* val) {
	slist<alpha_void, smart_actptr>	selection_list;
	slist<alpha_void, smart_actptr>::iterator li(selection_list);
	ArrayElement*		tds;
	TypedValue		V;
	Cstring			temp_error;
	ListOVal*		L = new ListOVal;
	ActivityInstance*	req;
	smart_actptr*		ptr;

	eval_intfc::get_all_selections(selection_list);

	while((ptr = li())) {
		if(L->get_array_type() == TypedValue::arrayType::UNDEFINED) {
			L->set_array_type(TypedValue::arrayType::STRUCT_STYLE);
		}
		req = ptr->BP;
		V = *req;
		tds = new ArrayElement(req->get_unique_id(), apgen::DATA_TYPE::INSTANCE);
		tds->set_typed_val(V);
		L->add(tds);
	}
	*val = *L;
}

void ACTIVITY_LISTrequest::append_act_list_to_cmd_text(Cstring& s, bool quoted) const {
	int		length_since_last_newline = s.length();
	int		length_overhead_regular = 0;
	int		length_overhead_newline = 4;

	if(quoted) {
		length_overhead_regular += 2;
		length_overhead_newline += 2;
	}

	if(list_of_act_names.get_length()) {
		stringslist::iterator	copy2(list_of_act_names);
		emptySymbol*		sn;

		while((sn = copy2())) {
			length_since_last_newline += length_overhead_regular + sn->get_key().length();
			if(length_since_last_newline > 80) {
				s << "\n    ";
				length_since_last_newline = length_overhead_newline + sn->get_key().length();
			}
			if(quoted) {
				s << addQuotes(sn->get_key());
			} else {
				s << sn->get_key();
			}
			if(sn != list_of_act_names.last_node()) {
				s << ", ";
				length_since_last_newline += 2;
			}
		}
	} else if(list_of_act_pointers.get_length()) {
		slist<alpha_void, dumb_actptr>::iterator	copy2(list_of_act_pointers);
		dumb_actptr*					ptr;
		ActivityInstance*				act;

		ptr = copy2.next();
		act = ptr->payload;
		if(act) {
			length_since_last_newline += length_overhead_regular + act->get_unique_id().length();
			if(length_since_last_newline > 80) {
				s << "\n    ";
				length_since_last_newline = length_overhead_newline + act->get_unique_id().length();
			}
			if(quoted) {
				s << addQuotes(act->get_unique_id());
			} else {
				s << act->get_unique_id();
			}
			while ((ptr = copy2())) {
				act = ptr->payload;
				length_since_last_newline += 2 + length_overhead_regular + act->get_unique_id().length();
				if(length_since_last_newline > 80) {
					s << ",\n    ";
					length_since_last_newline = length_overhead_newline + act->get_unique_id().length();
				} else {
					s << ", ";
				}
				if(quoted) {
					s << addQuotes(act->get_unique_id());
				}
				else {
					s << act->get_unique_id();	
				}
			}
		}
	}
}

void ACTIVITY_LISTrequest::removeUnwantedDescendants() {
	slist<alpha_void, dumb_actptr>::iterator	acts(list_of_act_pointers);
	dumb_actptr*					actPtr;
	tlist<alpha_void, dumb_actptr>			copyOfActivities;
	slist<alpha_void, dumb_actptr>::iterator	copyIterator(copyOfActivities);
	vector<dumb_actptr*>				toRemove;
	bool						use_internal_pointers = true;
	bool						use_active_list = false;
	status_aware_iterator				all_acts(
			eval_intfc::get_act_lists().get_scheduled_active_iterator());
	ActivityInstance*				a;

	if(All) {
		use_internal_pointers = false;
		use_active_list = true;
	}

	while(	(	use_internal_pointers
			&& (actPtr = acts())
			&& (a = actPtr->payload)
		) ||
		(	use_active_list
			&& (a = all_acts())
		)
	    ) {
		copyOfActivities << new dumb_actptr(a);
	}

	while((actPtr = copyIterator())) {
		a = actPtr->payload;
		ActivityInstance*	c;
		while((c = a->get_parent())) {
			if(copyOfActivities.find(c)) {

				//
				// actPtr points to an activity whose
				// ancestor c is also in the list.
				// We keep the ancestor and delete
				// actPtr from the list.
				//
				toRemove.push_back(actPtr);
				break;
			}
			a = c;
		}
	}
	for(int k = 0; k < toRemove.size(); k++) {
		delete toRemove[k];
	}
	list_of_act_pointers.clear();
	while((actPtr = copyIterator())) {
		list_of_act_pointers << new dumb_actptr(actPtr->payload);
	}
}

SCHEDULE_ACTrequest::SCHEDULE_ACTrequest(Cstring& errors, ListOVal* args)
	: ACTIVITY_LISTrequest("SCHEDULEACTIVITY", errors, args),
		incremental(false) {
	ArrayElement* all_ae = args->find("all");
	ArrayElement* incr_ae = args->find("incremental");

	if(all_ae) {
		if(!all_ae->Val().is_int()) {
			errors = "value of \"all\" argument is not boolean. ";
		} else {
			All = all_ae->Val().get_int();

			//
			// why do we do this? Presumably because having no activity
			// in the list is an error
			//
			if(All && errors.length()) {
				errors.undefine();
			}
		}
	}
	if(incr_ae) {
		incremental = incr_ae->Val().get_int();
	}
}

const Cstring& SCHEDULE_ACTrequest::get_command_text() {
	if(commandText.has_value() > 0) {
		return commandText;
	}
	commandText << commandType;
	if(incremental) {
		commandText << " INCREMENTAL ";
	}
	if(All) {
		commandText << " ALL";
	} else {
		commandText << " ID ";
		append_act_list_to_cmd_text(commandText);
	}
	return commandText;
}


apgen::RETURN_STATUS SCHEDULE_ACTrequest::get_pointers() {
	if(!All) {
		if(!list_of_act_pointers.get_length()) {
			bool		no_no_no;

			if(extract_pointers(no_no_no) != apgen::RETURN_STATUS::SUCCESS) {
				theStatus = apgen::RETURN_STATUS::FAIL;
				return apgen::RETURN_STATUS::FAIL;
			} else if(no_no_no) {
				return apgen::RETURN_STATUS::FAIL;
			}
		}
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void SCHEDULE_ACTrequest::process_middle(TypedValue* ret_val) {
	Cstring					any_errors;
	slist<alpha_void, dumb_actptr>		resultingActivities;
	slist<alpha_void, dumb_actptr>::iterator theResults(resultingActivities);
	dumb_actptr*				p;

	if(req_intfc::SCHEDULEhandler) {
		req_intfc::SCHEDULEhandler(this, 0);
	}

	/*
	 * Do this first, in case the scheduling request comes from
	 * outside (in which case ORIGINATED_FROM_SELF is FALSE)
	 */
	if(!All) {
		if(list_of_act_names.get_length() && !list_of_act_pointers.get_length()) {
			get_pointers();
		}
		set_selection();
	}

	/*
	 * 2nd arg. is whether to use selection list
	 */
	try {
		ACT_exec::ACT_subsystem().AAF_do_schedule(
						!All,
						incremental,
						resultingActivities);
	}
	catch(eval_error Err) {
		theStatus = apgen::RETURN_STATUS::FAIL;
		add_error(Err.msg);
	}

	ListOVal* lov = new ListOVal;
	while((p = theResults())) {
		ActivityInstance* a = p->payload;
		Cstring	s(a->get_unique_id());

		//notify the client of this id
		add_id_to_list(s);
		TypedValue V;
		ArrayElement* ae = new ArrayElement(a->get_unique_id(), apgen::DATA_TYPE::INSTANCE);
		V = *a;
		ae->set_typed_val(V);
		lov->add(ae);
	}
	*ret_val = *lov;
	if(req_intfc::SCHEDULEhandler) {
		req_intfc::SCHEDULEhandler(this, 1);
	}
	return;
}

UNSCHEDULE_ACTrequest::UNSCHEDULE_ACTrequest(Cstring& errors, ListOVal* args)
	: ACTIVITY_LISTrequest("UNSCHEDULEACTIVITY", errors, args),
	all(false) {
	ArrayElement* all_ae = args->find("all");

	if(all_ae) {
		all = all_ae->Val().get_int();
	}
}

const Cstring &UNSCHEDULE_ACTrequest::get_command_text() {
	if(commandText.has_value() > 0)
		return commandText;
	commandText = commandType;
	if(all) {
		commandText << " ALL";
	} else {
		commandText << " ID ";
		append_act_list_to_cmd_text(commandText);
	}
	return commandText;
}


apgen::RETURN_STATUS UNSCHEDULE_ACTrequest::get_pointers() {
	if(!all) {
		if(!list_of_act_pointers.get_length()) {
			bool		no_no_no;

			if(extract_pointers(no_no_no) != apgen::RETURN_STATUS::SUCCESS) {
				theStatus = apgen::RETURN_STATUS::FAIL;
				return apgen::RETURN_STATUS::FAIL;
			} else if(no_no_no) {
				return apgen::RETURN_STATUS::FAIL;
			}
		}
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void UNSCHEDULE_ACTrequest::process_middle(TypedValue* ret_val) {
	Cstring		any_errors;

	if(req_intfc::UNSCHEDULEhandler) {
		req_intfc::UNSCHEDULEhandler(this, 0);
	}
	if(!all) {
		if(list_of_act_names.get_length() && !list_of_act_pointers.get_length()) {
			get_pointers();
		}
		/*
		 * Do this first, in case the scheduling request comes from
		 * outside (in which case ORIGINATED_FROM_SELF is FALSE)
		 */
		set_selection();
	}

	try {
		ACT_exec::ACT_subsystem().AAF_do_unschedule(all);
	}
	catch(eval_error Err) {
		add_error(Err.msg);
	}
	set_to_selection(ret_val);
	if(req_intfc::UNSCHEDULEhandler) {
		req_intfc::UNSCHEDULEhandler(this, 1);
	}
	return;
}

REGEN_CHILDRENrequest::REGEN_CHILDRENrequest(Cstring& errors, ListOVal* args)
	: ACTIVITY_LISTrequest("REGENCHILDREN", errors, args) {
    ArrayElement* all_ae = args->find("all");

    if(all_ae) {
	if(!all_ae->Val().is_int()) {
	    errors = "value of \"all\" is not boolean. ";
	} else {
	    All = all_ae->Val().get_int();
	}
    } else {
	All = false;
    }

    //
    // Fix for AP-1256 (REGENCHILDREN command does not work in AAF script)
    //
    get_pointers();
}

const Cstring& REGEN_CHILDRENrequest::get_command_text() {
	if(commandText.has_value() > 0)
		return commandText;
	commandText << commandType;
	if(All) {
		commandText << " ALL";
	} else {
		commandText << " ID ";
		append_act_list_to_cmd_text(commandText);
	}
	return commandText;
}


apgen::RETURN_STATUS REGEN_CHILDRENrequest::get_pointers() {
	if(!All) {
		if(!list_of_act_pointers.get_length()) {
			bool		no_no_no;

			if(extract_pointers(no_no_no) != apgen::RETURN_STATUS::SUCCESS) {
				theStatus = apgen::RETURN_STATUS::FAIL;
				return apgen::RETURN_STATUS::FAIL;
			} else if(no_no_no) {
				return apgen::RETURN_STATUS::FAIL;
			}
		}
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void REGEN_CHILDRENrequest::process_middle(TypedValue*) {
	slist<alpha_void, dumb_actptr>			resultingActivities;
	slist<alpha_void, dumb_actptr>::iterator	theResults(resultingActivities);
	slist<alpha_void, dumb_actptr>::iterator	acts(list_of_act_pointers);
	dumb_actptr*					actPtr;

	removeUnwantedDescendants();

	//
	// At this point we have a list of activities such that no activity
	// in it has an ancestor also in the list. We can safely delete
	// children and generate new children without interference.
	//
	while((actPtr = acts())) {
		ActivityInstance* a = actPtr->payload;
		if(
			(!All)
		        || a->hierarchy().can_be_decomposed_without_errors(1)
		  ) {
			instance_state_changer	ISC(a);

			//
			// debug
			//
			// cout << dbgind::indent() << ">>>   regen children for " << a->get_unique_id() << "\n";

			ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_NEWCHILDREN);
			try {
				ISC.do_it(NULL);
			} catch(eval_error Err) {
				add_error(Err.msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
				return;
			} catch(decomp_error Err) {
				add_error(Err.msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
				return;
			}
			slist<alpha_void, smart_actptr>::iterator li(a->hierarchy().get_down_iterator());
			smart_actptr*	b;
			while((b = li())) {

				//
				// debug
				//
				// cout << dbgind::indent() << "   +++   case 1 - storing " << b->BP->get_unique_id() << "\n";
				// preciousIDs() << new emptySymbol(b->BP->get_unique_id());

				resultingActivities << new dumb_actptr(b->BP);
			}
		}
	}

	//
	// debug
	//
	// extern tlist<alpha_void, dumb_actptr>& doomedActPtrs();
	while((actPtr = theResults())) {
		ActivityInstance* a = actPtr->payload;

		// if(doomedActPtrs().find(a)) {
		//
		// debug
		//
		// cout << "!!!   Oh No! Referencing a doomed pointer\n";
		// }

		//notify the client of this id
		add_id_to_list(a->get_unique_id());
	}
	return;
}

const Cstring&
UI_ACTIVITYrequest::get_command_text(){
	if(commandText.has_value() > 0)
		return commandText;
	commandText = commandType;
	commandText << " \"" << EventHandlerName << "\"";
	commandText << " ID " << ActivityID;

	return commandText; }


void
UI_ACTIVITYrequest::process_middle(TypedValue*) {
	//lets violate DB layering mandate for now

	//get the activity
	Cntnr<alpha_string, ActivityInstance*>*	theTag;
	Cstring any_errors;

	try {
		theTag = find_tag_from_real_or_symbolic_id(ActivityID);
	} catch(eval_error Err) {
		theStatus = apgen::RETURN_STATUS::FAIL;
		add_error(any_errors);
		return;
	}

	ActivityInstance*	instance = theTag->payload;

	//build the arguments
	TypedValue argument;
	argument = *instance;
	TypedValuePtrVect arguments;
	arguments.push_back(&argument);
	//call the event
	EventRegistry::Register().PropagateEvent(
		"UI_ACTIVITY_CALLBACK", EventHandlerName, arguments); }

const Cstring& UI_GLOBALrequest::get_command_text(){
	if(commandText.has_value() > 0)
		return commandText;
	commandText = commandType;
	commandText << " \"" << EventHandlerName << "\"";
	commandText << " XLOC " << X << " YLOC " << Y;
	commandText << " LEGEND \"" << LegendName << "\"";
	commandText << " TIME " << Time.to_string();

	return commandText; }

void UI_GLOBALrequest::process_middle(TypedValue*) {
	//assemble parameters
	TypedValue argumentX;
	TypedValue argumentY;
	TypedValue argumentLegendName;
	TypedValue argumentTime;

	argumentX = X;
	argumentY = Y;
	argumentLegendName = LegendName;
	argumentTime = Time;

	TypedValuePtrVect arguments;

	arguments.push_back(&argumentX);
	arguments.push_back(&argumentY);
	arguments.push_back(&argumentLegendName);
	arguments.push_back(&argumentTime);

	EventRegistry::Register().PropagateEvent(
		"UI_GLOBAL_CALLBACK", EventHandlerName, arguments); }

UNFREEZE_RESOURCESrequest::UNFREEZE_RESOURCESrequest()
	: Action_request("UNFREEZERESOURCES") {}

const Cstring& UNFREEZE_RESOURCESrequest::get_command_text() {
	if(commandText.has_value() > 0) {
		return commandText; }
	commandText << commandType;
	return commandText; }

void UNFREEZE_RESOURCESrequest::process_middle(TypedValue*) {
	Rsource*		res;
	Rsource::iterator	iter;

	while((res = iter.next())) {
		RES_state*		state_res = dynamic_cast<RES_state*>(res);
		RES_consumable*		cons_res = dynamic_cast<RES_consumable*>(res);
		RES_nonconsumable*	noncons_res = dynamic_cast<RES_nonconsumable*>(res);
		RES_settable*		settable_res = dynamic_cast<RES_settable*>(res);

		if(state_res) {
			state_res->get_history().Unfreeze();
		} else if(cons_res) {
			cons_res->get_history().Unfreeze();
		} else if(noncons_res) {
			noncons_res->get_history().Unfreeze();
		} else if(settable_res) {
			settable_res->get_history().Unfreeze();
		}
	}
}

UNGROUP_ACTIVITIESrequest::UNGROUP_ACTIVITIESrequest(
		const stringslist& alist)
	: ACTIVITY_LISTrequest("UNGROUPACTIVITIES", alist) {}

UNGROUP_ACTIVITIESrequest::UNGROUP_ACTIVITIESrequest(
		const slist<alpha_void, dumb_actptr>& alist)
	: ACTIVITY_LISTrequest("UNGROUPACTIVITIES", alist) {}

UNGROUP_ACTIVITIESrequest::UNGROUP_ACTIVITIESrequest(Cstring& errors, ListOVal* args)
	: ACTIVITY_LISTrequest("UNGROUPACTIVITIES", errors, args) {
	// debug
	// cout << "UNGROUP_ACTIVITIES: list_of_act_names has " << list_of_act_names.get_length() << " element(s)\n";
	if(list_of_act_names.get_length() && !list_of_act_pointers.get_length()) {
		get_pointers();
	}
}

const Cstring& UNGROUP_ACTIVITIESrequest::get_command_text() {
	if(commandText.has_value() > 0)
		return commandText;
	if(list_of_act_names.get_length() || list_of_act_pointers.get_length()) {
		commandText << commandType << " ID ";
		append_act_list_to_cmd_text(commandText);
	}
	return commandText;
}

void UNGROUP_ACTIVITIESrequest::process_middle(TypedValue* ret_val) {
	dumb_actptr*					ptr;
	slist<alpha_void, dumb_actptr>::iterator	thePs(list_of_act_pointers);
	ActivityInstance*				the_request;
	slist<alpha_void, dumb_actptr>			theDoomedRequests;
	slist<alpha_void, dumb_actptr>::iterator	theEnd(theDoomedRequests);

	while((ptr = thePs())) {
		slist<alpha_void, dumb_actptr>		the_copy;
		slist<alpha_void, dumb_actptr>::iterator it2(the_copy);
		smart_actptr*				b;
		dumb_actptr*				p;
		// V_7_7 change START
		List*					save_the_list = NULL;
		// V_7_7 change END

		the_request = ptr->payload;
		if(!the_request->hierarchy().children_count()) {
			// at Steve Wissler's request, this is no longer an error
			// add_error("One or more selected activity/ies cannot be ungrouped.");
			// theStatus = apgen::RETURN_STATUS::FAIL;
			continue;
		}

		// debug
		// cout << "UNGROUP(" << the_request->get_unique_id() << "): passed first stage\n";

		// need to sever .....
		slist<alpha_void, smart_actptr>::iterator it(the_request->hierarchy().get_down_iterator());

		the_copy.clear();
		while((b = it())) {
			ActivityInstance*	act = b->BP;
			the_copy << new dumb_actptr(act);
		}
		try {
			while((p = it2())) {
				instance_state_changer	ISC(p->payload);

				// debug
				// cout << "UNGROUP: processing child activity " << p->payload->get_unique_id() << "\n";

				ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
				ISC.set_desired_parent_to(NULL);
				ISC.do_it(NULL);
				p->payload->select();
			}
			// new code May 2003 -- maybe SEQtalk sent us this while the act was already decomposed
			if(the_request->agent().is_decomposed()) {
				instance_state_changer	ISC(the_request);

				ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
				ISC.do_it(NULL);
				// the_request->hierarchy().make_active(/* please sever */ false);
			}
		}
		catch(eval_error Err) {
			add_error(Err.msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		} catch(decomp_error Err) {
			add_error(Err.msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}

		if(the_request->is_a_request()) {
			/*
			 * Bye-bye request
			 */
		// V_7_7 change 2 START
			theDoomedRequests << new dumb_actptr(the_request, the_request);
		} else if(save_the_list) {
			the_request->acknowledge_timing_changes();
			save_the_list->insert_node(the_request);
		}
	}
		// V_7_7 change 2 END
	while((ptr = theEnd())) {
		delete ptr->payload;
	}
	set_to_selection(ret_val);
	return;
}

const Cstring&
GRAB_IDrequest::get_command_text() {
	if(commandText.has_value() > 0)
		return commandText;
	commandText << commandType;
	commandText << " " << id_name;
	return commandText;
}

void
GRAB_IDrequest::process_middle(TypedValue*) {
	Cstring			errs;
	OPEN_FILErequest*	of = list ? list->Owner : NULL;

	if(of) {
		emptySymbol*	id = of->IDs.last_node();
		symNode*	s = of->id_variables.find(id_name);

		if(!id) {
			theStatus = apgen::RETURN_STATUS::FAIL;
			errs << "Cannot grab id " << id_name << ". No IDs available.";
			add_error(errs);
		} else {
			if(!s) {
				of->id_variables << new symNode(id_name, id->get_key());
			} else {
				s->payload = id->get_key();
			}
			delete id;
		}
	} else {

		theStatus = apgen::RETURN_STATUS::FAIL;
		errs << "Cannot grab id " << id_name << ". Not in parsing mode.";
		add_error(errs);
	}
}

GROUP_ACTIVITIESrequest::GROUP_ACTIVITIESrequest(	const stringslist&		alist,
							const Cstring&			n,
							int				request_flag,
							int				chameleon_flag)
	: ACTIVITY_LISTrequest("GROUPACTIVITIES", alist) {
	if(request_flag) {
		theNewParentName = n; }
	else if(chameleon_flag) {
		theNewChameleonName = n; }
	else {
		theNewParentId = n; } }

GROUP_ACTIVITIESrequest::GROUP_ACTIVITIESrequest(	const slist<alpha_void, dumb_actptr>& alist,
							const Cstring&			n,
							int				request_flag,
							int				chameleon_flag) 
	: ACTIVITY_LISTrequest("GROUPACTIVITIES", alist) {
	if(request_flag) {
		theNewParentName = n; }
	else if(chameleon_flag) {
		theNewChameleonName = n; }
	else {
		theNewParentId = n; } }

GROUP_ACTIVITIESrequest::GROUP_ACTIVITIESrequest(	const stringslist&	alist,
							const Cstring&		name,
							const Cstring&		Type,
							int			request_flag,
							int			chameleon_flag)
	: ACTIVITY_LISTrequest("GROUPACTIVITIES", alist) {
	if(request_flag) {
		theNewParentName = name;
		theTemplateName = Type; }
	else if(chameleon_flag) {
		theNewChameleonName = name;
		theChameleonTypeName = Type; } }


GROUP_ACTIVITIESrequest::GROUP_ACTIVITIESrequest(	const slist<alpha_void, dumb_actptr>&	alist,
							const Cstring&				name,
							const Cstring&				Type,
							int					request_flag,
							int					chameleon_flag)
	: ACTIVITY_LISTrequest("GROUPACTIVITIES", alist) {
	if(request_flag) {
		theNewParentName = name;
		theTemplateName = Type; }
	else if(chameleon_flag) {
		theNewChameleonName = name;
		theChameleonTypeName = Type; } }

const Cstring &GROUP_ACTIVITIESrequest::get_command_text() {
	if(commandText.has_value() > 0)
		return commandText;
	commandText << commandType;
	if(theTemplateName.length()) {
		commandText << " " << theTemplateName << " PARENT " << theNewParentName; }
	else if(theChameleonTypeName.length()) {
		commandText << " " << theChameleonTypeName << " CHAMELEON " << theNewChameleonName; }
	else if(theNewParentName.length()) {
		commandText << " PARENT " << theNewParentName; }
	else if(theNewChameleonName.length()) {
		commandText << " CHAMELEON " << theNewChameleonName; }
	else if(theNewParentId.length()) {
		commandText << " ID " << theNewParentId; }
	commandText << " ID ";
	append_act_list_to_cmd_text(commandText);
	return commandText; }

void GROUP_ACTIVITIESrequest::process_middle(TypedValue*) {
	dumb_actptr*				ptr;
	slist<alpha_void, dumb_actptr>::iterator thePs(list_of_act_pointers);
	Cntnr<alpha_string, ActivityInstance*>*	tag1 = NULL;
	ActivityInstance*			the_parent, *the_child;

	try {
		tag1 = find_tag_from_real_or_symbolic_id(theNewParentId);
	} catch(eval_error Err) {
		Cstring	errs;
		errs = Cstring("Attempting to group activities under a non-existent parent; ")
			+ errs;
		add_error(errs);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
	the_parent = tag1->payload;
	while((ptr = thePs())) {
		the_child = ptr->payload;
		if(the_child == the_parent) {
			Cstring	errs;
			errs << "Attempting to group activity instance "
				<< the_parent->identify() << " as its own child.\n";
			add_error(errs);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}
		instance_state_changer	ISC(the_parent);

		ISC.set_desired_child_to(the_child);
		try {
			ISC.do_it(NULL);
		} catch(eval_error Err) {
			add_error(Err.msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			destroy_an_act_req(the_parent);
			return;
		} catch(decomp_error Err) {
			add_error(Err.msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			destroy_an_act_req(the_parent);
			return;
		}
	}
	the_parent->select();
	return;
}

	/*
	 * List of strings
	 */
COPY_ACTIVITYrequest::COPY_ACTIVITYrequest(const stringslist& alist)
	: ACTIVITY_LISTrequest("COPYACTIVITY", alist) { }

	/*
	 * List of pointers
	 */
COPY_ACTIVITYrequest::COPY_ACTIVITYrequest(
		const slist<alpha_void, dumb_actptr>& alist)
	: ACTIVITY_LISTrequest("COPYACTIVITY", alist) {}

COPY_ACTIVITYrequest::COPY_ACTIVITYrequest(Cstring& errors, ListOVal* args)
	: ACTIVITY_LISTrequest("COPYACTIVITY", errors, args) {
	get_pointers();
}

const Cstring& COPY_ACTIVITYrequest::get_command_text() {
	if (commandText.has_value() > 0) return commandText;
	commandText << commandType << " ID ";
	append_act_list_to_cmd_text(commandText);
	return commandText;
}

void COPY_ACTIVITYrequest::process_middle(TypedValue*) {
	Cstring		anyError;

	if(ACT_exec::sel_remove(list_of_act_pointers, 1, anyError) != apgen::RETURN_STATUS::SUCCESS) {
		add_error(anyError);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
	return;
}

void	COPY_ACTIVITYrequest::to_script(Cstring& s) const {
	s << "xcmd(\"ABSTRACTACTIVITY\", [\"activities\" = [";
	append_act_list_to_cmd_text(s, true);
	s << "]]);\n";
}

// names
CUT_ACTIVITYrequest::CUT_ACTIVITYrequest(const stringslist& alist)
	: ACTIVITY_LISTrequest("CUTACTIVITY", alist),
	please_delete(false)
	{}

// pointers
CUT_ACTIVITYrequest::CUT_ACTIVITYrequest(
		const slist<alpha_void, dumb_actptr>& alist)
	: ACTIVITY_LISTrequest("CUTACTIVITY", alist),
	please_delete(false) {
	get_pointers();
}

// xcmd
CUT_ACTIVITYrequest::CUT_ACTIVITYrequest(Cstring& errors, ListOVal* args)
	: ACTIVITY_LISTrequest("CUTACTIVITY", errors, args) {
	ArrayElement*	ae = args->find("delete");
	if(!ae) {
		errors = "Cannot find \"delete\" in args array.";
	} else {
		please_delete = ae->Val().get_int();
	}
	get_pointers();
}

const Cstring& CUT_ACTIVITYrequest::get_command_text() {
	if(commandText.has_value()) return commandText;
	commandText << commandType << " ID ";
	append_act_list_to_cmd_text(commandText);
	return commandText;
}

void CUT_ACTIVITYrequest::process_middle(TypedValue*) {
	Cstring			anyError;

	// second arg. is 'copyflag'; '0' means we move the act. request ITSELF to the 'Pending' list:
	if(ACT_exec::sel_remove(list_of_act_pointers, 0, anyError) != apgen::RETURN_STATUS::SUCCESS) {
		add_error(anyError);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}

	if(please_delete) {
		// allow clients to re-use act ID
		ACT_exec::clean_up_clipboard();
	}
	return;
}

// names
DELETE_ACTIVITYrequest::DELETE_ACTIVITYrequest(const stringslist& alist)
	: ACTIVITY_LISTrequest("DELETEACTIVITY", alist) {}

// pointers
DELETE_ACTIVITYrequest::DELETE_ACTIVITYrequest(
		const slist<alpha_void, dumb_actptr>& alist)
	: ACTIVITY_LISTrequest("DELETEACTIVITY", alist) {
}

DELETE_ACTIVITYrequest::DELETE_ACTIVITYrequest(Cstring& errors, ListOVal* args)
	: ACTIVITY_LISTrequest("DELETEACTIVITY", errors, args) {
	get_pointers();
}

const Cstring& DELETE_ACTIVITYrequest::get_command_text() {
	if(commandText.has_value()) return commandText;
	commandText << commandType << " ID ";
	append_act_list_to_cmd_text(commandText);
	return commandText;
}

void DELETE_ACTIVITYrequest::process_middle(TypedValue* ret_val) {
	Cstring			anyError;

	// debug
	// cerr << "DELETE_ACTIVITYrequest::process_middle(TypedValue*): " << list_of_act_pointers.get_length()
	// << " instance(s) in list.\n";

	// second arg. is 'copyflag'; '0' means we move the act. request ITSELF to the 'Pending' list:
	if(ACT_exec::sel_remove(list_of_act_pointers, 0, anyError) != apgen::RETURN_STATUS::SUCCESS) {
		add_error(anyError);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
	ACT_exec::clean_up_clipboard();
	return;
}

MOVE_ACTIVITYrequest::MOVE_ACTIVITYrequest(const stringslist& alist,
					       const CTime_base& time,
					       const CTime_base& duration,
					       const Cstring& legend)
	: ACTIVITY_LISTrequest("MOVEACTIVITY", alist),
	act_time(time),
	act_duration(duration),
	act_legend(legend)
	{}

MOVE_ACTIVITYrequest::MOVE_ACTIVITYrequest(	const slist<alpha_void, dumb_actptr>& alist,
						const CTime_base&		time,
						const CTime_base&		duration,
						const Cstring&			legend)
	: ACTIVITY_LISTrequest("MOVEACTIVITY", alist),
	act_time(time),
	act_duration(duration),
	act_legend(legend)
	{}

const Cstring & MOVE_ACTIVITYrequest::get_command_text() {
	if(commandText.length() > 0)
		return commandText;

	commandText << commandType << " ID ";
	append_act_list_to_cmd_text(commandText);

	//should have exactly one of:  act_time, act_duration
	if(act_time > CTime_base(0, 0, false) /* i.e. valid time */) {
		commandText << " TO " << act_time.to_string(); }
	else {	//act_duration may be positive, negative, or zero
		commandText << " BY " << act_duration.to_string(); }
	if (act_legend.has_value()) {
		//legend can be just numerals, or contain blanks, etc., so quote it:
		commandText << " LEGEND " << addQuotes(act_legend); }
	return commandText; }

apgen::RETURN_STATUS MOVE_ACTIVITYrequest::get_pointers() {
	bool no_no_no;

	if(extract_pointers(no_no_no) != apgen::RETURN_STATUS::SUCCESS) {
		add_error("(this error occurred while processing MOVE ACTIVITY request)\n");
		theStatus = apgen::RETURN_STATUS::FAIL;
		return apgen::RETURN_STATUS::FAIL; }
	else if(no_no_no) {
		return apgen::RETURN_STATUS::FAIL; }
	return apgen::RETURN_STATUS::SUCCESS; }

void MOVE_ACTIVITYrequest::process_middle(TypedValue*) {
	CTime_base		tmchange;
	int			vertical_refresh = 0;
	int			old_index, new_index, index_offset, nlegends;
	int			oldvid, newvid;
	dumb_actptr*		selnode;
	slist<alpha_void, dumb_actptr>::iterator selst(list_of_act_pointers);
	ActivityInstance*	act_source = NULL;
	ActivityInstance*	reference_activity;
	LegendObject*		legenddefnode;

	reference_activity = list_of_act_pointers.first_node()->payload;

	if((legenddefnode = (LegendObject *) Dsource::theLegends().find(reference_activity->getTheLegend()))) {
		old_index = legenddefnode->get_index();
	} else {
		Cstring		errmsg;

		errmsg << "MOVE_ACTIVITY: cannot find \'old\' legend \"" << reference_activity->getTheLegend() << "\"\n";
		add_error(errmsg);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
	nlegends = Dsource::theLegends().get_length();
	if (old_index >= nlegends)
		old_index = nlegends - 1;

	//Either act_time (absolute time) OR act_duration (relative time) is used:
	if(act_time > CTime_base(0, 0, false))	//compute motion from absolute time
		tmchange = act_time - reference_activity->getetime();	//may be neg,pos,zero
	else				//use relative motion (a duration)
		tmchange = act_duration;	//may be negative,positive,zero

	//Horizontal motion first:
	if(Preferences().GetSetPreference("HorizontalDragEnabled", "TRUE") == "TRUE") {
		while((selnode = selst.next())) {
			act_source = selnode->payload;
			// notifyTheDisplaySubsystem(act_source);
			int dragChildren = (Preferences().GetSetPreference("DragChildren", "TRUE") == "TRUE");
			act_source->obj()->move_starttime(tmchange, dragChildren);
		}
	}

	//Vertical motion second:
	if(Preferences().GetSetPreference("VerticalDragEnabled", "TRUE") == "TRUE") {
		//Compute legend units to move (*_index 0-based; old_index found above):
		if((legenddefnode = (LegendObject *) Dsource::theLegends().find(act_legend))) {
			new_index = legenddefnode->get_index();
		} else {
			Cstring		errmsg;

			errmsg << "MOVE_ACTIVITY: cannot find \'new\' legend \"" << act_legend << "\"\n";
			add_error(errmsg);
			theStatus = apgen::RETURN_STATUS::FAIL;
			return;
		}

		//Do the actual vertical move (based on ACT_sys::vmove, now obsolete):
		if (new_index != old_index) {
			index_offset = new_index - old_index;

			while((selnode = selst.next())) {
				act_source = selnode->payload;
				oldvid = act_source->getTheLegendIndex();
				//Drag progressively "collapses" shape of instances, if try to
				//  drag beyond the existing legends.  Logic would have to
				//  change if decide "Paste" logic is better (collapsing is not
				//  allowed, so cannot go beyond where collapsing would occur).
				newvid = oldvid + index_offset;
				if (newvid < 0)
					newvid = 0;
				else if (newvid >= nlegends)
					newvid = nlegends - 1;
				if (newvid != oldvid) {
					//Find out the new legend
					legenddefnode = (LegendObject *) Dsource::theLegends()[newvid];
					// 2nd arg is whether to instantiate
					act_source->legend().switch_to_legend(legenddefnode);
				}
			}

			vertical_refresh = 1;
		}
	}

	return;
}

const CTime_base &MOVE_ACTIVITYrequest::get_time() {
	return act_time; }

const CTime_base &MOVE_ACTIVITYrequest::get_duration() {
	return act_duration; }

const Cstring &MOVE_ACTIVITYrequest::get_legend() {
	return act_legend; }

PASTE_ACTIVITYrequest::PASTE_ACTIVITYrequest(const stringslist& alist, const Cstring& time,
						 const Cstring& legend)
	: ACTIVITY_LISTrequest("PASTEACTIVITY", alist),
	act_time(time),
	act_legend(legend),
	please_delete(false)
	{}

PASTE_ACTIVITYrequest::PASTE_ACTIVITYrequest(
		const slist<alpha_void, dumb_actptr>& alist,
		const Cstring& time,
		 const Cstring& legend)
	: ACTIVITY_LISTrequest("PASTEACTIVITY", alist),
	    act_time(time),
        act_legend(legend),
        please_delete(false) {}

const Cstring &PASTE_ACTIVITYrequest::get_command_text() {
    if(commandText.length() > 0) {
	       return commandText;
    }
    commandText << commandType << " TO " << addQuotes(act_time);
    //legend can be just numerals, or contain blanks, etc., so quote it:
    commandText << " LEGEND " << addQuotes(act_legend);
    return commandText;
}

apgen::RETURN_STATUS PASTE_ACTIVITYrequest::get_pointers() {
	bool			no_no_no;

    if(extract_pointers(no_no_no) != apgen::RETURN_STATUS::SUCCESS) {
	add_error("(this error occurred while processing PASTE ACTIVITY request)\n");
	theStatus = apgen::RETURN_STATUS::FAIL;
	return apgen::RETURN_STATUS::FAIL;
    }
    else if(no_no_no) {
	theStatus = apgen::RETURN_STATUS::FAIL;
	return apgen::RETURN_STATUS::FAIL;
    }
    return apgen::RETURN_STATUS::SUCCESS;
}

void PASTE_ACTIVITYrequest::process_middle(TypedValue*) {
    bool				no_no_no;
    Cstring				HID, theRightLegend(act_legend);
    bool				change_status;

    ACT_exec::addNewItem();
    // somewhat kludgy: we encode the hopper name in the legend name
    HID = ACT_exec::extractHopper(act_legend);
    if(HID.length()) {
	// Hopper design: we need to extract the act-sys with the proper Hopper ID

	theRightLegend = ":" / act_legend;
	// ask ACT_sys::paste_activity() to unschedule the activity
	change_status = false;
	// Don't do this! ACT_sys::paste_activity will COPY the activities in the
	// pending list and paste the copies, not the originals.
	// ACT_exec::set_clipboard_scheduled(0);
    } else {
	change_status = true;
	// theRightLegend is already set to act_legend
	// See above comment: do NOT do this.
	// ACT_exec::set_clipboard_scheduled(1);
    }

    List		top_level_IDs;
    List_iterator	ids(top_level_IDs);
    Node*		N;

    try { ACT_exec::paste_activity(
			act_time,
			theRightLegend,
			change_status,
			please_delete,
			top_level_IDs);
    }
    catch(eval_error Err) {
	add_error(Err.msg);
	theStatus = apgen::RETURN_STATUS::FAIL;
    }

    while((N = ids())) {
	// allow scripts to reference IDs through the "@nnn" method
	add_id_to_list(N->get_key());
    }
}

const Cstring &PASTE_ACTIVITYrequest::get_legend() {
	return act_legend;
}

CTime_base PASTE_ACTIVITYrequest::get_time() const {
	return CTime_base(act_time);
}

ACTIVITY_LISTrequest::ACTIVITY_LISTrequest(
		const Cstring&	type,
		Cstring&	errors,
		ListOVal*	args)
	: Action_request(type),
		All(false) {
    ArrayElement*	activityList;

    if((activityList = args->find("all"))) {
	if(!activityList->Val().is_int()) {
	    errors << "Element named \"all\" is not a boolean. ";
	} else {
	    All = true;
	}
    } else if((activityList = args->find("activities"))) {
	if(!activityList->Val().is_array()) {
	    errors << "Element named \"activities\" is not an array. ";
	} else {
	    ListOVal&	acts(activityList->Val().get_array());
	    ArrayElement*	ae;

	    for(int k = 0; k < acts.get_length(); k++) {
		ae = acts[k];
		if(ae->Val().is_instance()) {
		    if(!ae->get_key().length()) {
			errors << "activities list is not a struct indexed by activity ID";
			break;
		    }
		    list_of_act_names << new emptySymbol(ae->get_key());
		} else if(ae->Val().is_string()) {
		    list_of_act_names << new emptySymbol(ae->Val().get_string());
		} else {
		    errors << "Element " << ae->get_key()
			<< " is neither a string nor an instance.";
		    break;
		}
	    }
	}
    } else  {
	errors << "Element named \"activities\" is missing. ";
    }
}

ABSTRACT_ACTIVITYrequest::ABSTRACT_ACTIVITYrequest(const stringslist& alist, bool full)
	: ACTIVITY_LISTrequest("ABSTRACTACTIVITY", alist),
	Fully(full) {}

ABSTRACT_ACTIVITYrequest::ABSTRACT_ACTIVITYrequest(
		const slist<alpha_void, dumb_actptr>& alist,
		bool full)
	: ACTIVITY_LISTrequest("ABSTRACTACTIVITY", alist),
	Fully(full) {}

ABSTRACT_ACTIVITYrequest::ABSTRACT_ACTIVITYrequest(Cstring& errors, ListOVal* args)
	: ACTIVITY_LISTrequest("ABSTRACTACTIVITY", errors, args),
    Fully(false) {
    if(!errors.length()) {
	ArrayElement* full_ae = args->find("Fully");

	if(full_ae) {
	    Fully = full_ae->Val().get_int();
	}
    }
}

const Cstring& ABSTRACT_ACTIVITYrequest::get_command_text() {
    if (commandText.length() > 0) return commandText;
    commandText = commandType;
    commandText << " ID ";
    append_act_list_to_cmd_text(commandText);
    if(Fully) {
	commandText << " FULLY";
    }
    return commandText;
}


void ABSTRACT_ACTIVITYrequest::process_middle(TypedValue*) {
	slist<alpha_void, dumb_actptr>::iterator	thePs(list_of_act_pointers);
	dumb_actptr*					ptr;

	if(list_of_act_names.get_length() && !list_of_act_pointers.get_length()) {
		get_pointers(); }
	ptrs_to_acts_to_abstract.clear();
	// avoid problems with smart pointers
	while((	ptr = thePs())) {
		ptrs_to_acts_to_abstract << new dumb_actptr(ptr->payload, ptr->payload); }
	if(Fully) {
		do {
			process_middle_private(false); }
			while(ptrs_to_acts_to_abstract.get_length()); }
	else {
		if(!ptrs_to_acts_to_abstract.get_length()) {
			theStatus = apgen::RETURN_STATUS::FAIL;
			add_error("Nothing to abstract");
			return; }
		process_middle_private(true); } }

void ABSTRACT_ACTIVITYrequest::process_middle_private(bool report_errors) {
	dumb_actptr*					ptr;
	ActivityInstance*				activityInstance;
	slist<alpha_void, dumb_actptr>			new_ptrs;
	slist<alpha_void, dumb_actptr>::iterator	pointers(ptrs_to_acts_to_abstract);

	while((ptr = pointers())) {
		activityInstance = ptr->payload;
		ActivityInstance*		theParent = activityInstance->get_parent();
		instance_state_changer	ISC(activityInstance);

		ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_ABSTRACTED);
		try {
			ISC.do_it(NULL);
		} catch(eval_error Err) {
			theStatus = apgen::RETURN_STATUS::FAIL;
			add_error(Err.msg);
			// V31.0 changes, round 2
			// ptrs_to_acts_to_abstract.clear();
			// return;
		}
		catch(decomp_error Err) {
			if(report_errors) {
				theStatus = apgen::RETURN_STATUS::FAIL;
				add_error(Err.msg);
				// V31.0 changes, round 2
				// ptrs_to_acts_to_abstract.clear();
				// return;
			}
		}
		if(theParent) {
			new_ptrs << new dumb_actptr((void *) theParent, theParent);
			add_id_to_list(theParent->get_unique_id());
		}
	}
	ptrs_to_acts_to_abstract = new_ptrs;
}

ABSTRACT_ALLrequest::ABSTRACT_ALLrequest()
	: Action_request("ABSTRACTALL") {}

ABSTRACT_ALLrequest::ABSTRACT_ALLrequest(Cstring&, ListOVal*)
	: Action_request("ABSTRACTALL") {}

const Cstring & ABSTRACT_ALLrequest::get_command_text() {
	if (commandText.length() > 0) return commandText;
	commandText = commandType;
	return commandText; }

void ABSTRACT_ALLrequest::process_middle(TypedValue* ret_val) {
	status_aware_iterator acts(eval_intfc::get_act_lists().get_scheduled_active_iterator());
	slist<alpha_void, dumb_actptr>		ptrs;
	slist<alpha_void, dumb_actptr>::iterator pointers(ptrs);
	int					number_abstracted = 0;
	int					number_unabstractable = 0;
	int					d = 0, c = eval_intfc::get_act_lists().get_active_length();
	ActivityInstance*			activityInstance;
	dumb_actptr*				ptr;

	if(req_intfc::ABSTRACT_ALLhandler) {
		req_intfc::ABSTRACT_ALLhandler(this, 0); }
	while((activityInstance = acts.next()))
		ptrs << new dumb_actptr(activityInstance);

	while((ptr = pointers.next())) {
		activityInstance = ptr->payload;
		// first half of condition is to make sure we haven't just abstracted the activity:
		if(activityInstance->agent().is_active()) {
			if(activityInstance->get_parent()) {
				try {
					instance_state_changer	ISC(activityInstance);

					ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_ABSTRACTED);
					ISC.do_it(NULL);
				} catch(eval_error Err) {
					theStatus = apgen::RETURN_STATUS::FAIL;
					add_error(Err.msg);
				} catch(decomp_error Err) {
					theStatus = apgen::RETURN_STATUS::FAIL;
					add_error(Err.msg);
				}
				number_abstracted++;
			}
			/*
			else {
				number_unabstractable++;
				msg.undefine();
				msg << activityInstance->identify() << " does not abstract.";
				error_list << new String_node(msg); }
			*/
		}
	}
	if(!number_abstracted) {
		theStatus = apgen::RETURN_STATUS::FAIL;
		add_error("Nothing to abstract"); }
	if(req_intfc::ABSTRACT_ALLhandler) {
		req_intfc::ABSTRACT_ALLhandler(this, 1); }
	set_to_selection(ret_val);
	return; }

ABSTRACT_ALL_QUIETrequest::ABSTRACT_ALL_QUIETrequest()
	: Action_request("ABSTRACTALL_QUIET"),
	more_left(false)
	{}

ABSTRACT_ALL_QUIETrequest::ABSTRACT_ALL_QUIETrequest(Cstring&, ListOVal*)
	: Action_request("ABSTRACTALL_QUIET"),
	more_left(false)
	{}

const Cstring &ABSTRACT_ALL_QUIETrequest::get_command_text() {
	if(commandText.length() > 0) {
		return commandText; }
	commandText = "ABSTRACTALL_QUIET";
	return commandText; }

void ABSTRACT_ALL_QUIETrequest::process_middle(TypedValue* ret_val) {
	do {
		process_middle_private(); }
	while(more_left);
	set_to_selection(ret_val);
	if(req_intfc::ABSTRACT_ALLhandler) {
		req_intfc::DETAIL_ALLhandler(this, 1); } }

void ABSTRACT_ALL_QUIETrequest::process_middle_private() {
	status_aware_iterator	acts(eval_intfc::get_act_lists().get_scheduled_active_iterator());
	List		ptrs;
	List_iterator	pointers(ptrs);
	int		number_abstracted = 0;
	int		number_unabstractable = 0;
	int		d = 0, c = eval_intfc::get_act_lists().get_active_length();
	ActivityInstance		*activityInstance;
	Pointer_node	*ptr;

	if(req_intfc::ABSTRACT_ALLhandler) {
		req_intfc::ABSTRACT_ALLhandler(this, 0); }
	while((activityInstance = acts.next()))
		ptrs << new Pointer_node(activityInstance, NULL);

	while((ptr = (Pointer_node *) pointers.next())) {
		activityInstance = (ActivityInstance *) ptr->get_ptr();
		// first half of condition is to make sure we haven't just abstracted the activity:
		if(activityInstance->agent().is_active()) {
			if(activityInstance->get_parent()) {
				try {
					instance_state_changer	ISC(activityInstance);

					ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_ABSTRACTED);
					ISC.do_it(NULL);
				} catch(eval_error Err) {
					theStatus = apgen::RETURN_STATUS::FAIL;
					add_error(Err.msg);
				} catch(decomp_error Err) {
					theStatus = apgen::RETURN_STATUS::FAIL;
					add_error(Err.msg);
				}
				number_abstracted++;
			}
			/*
			else {
				number_unabstractable++;
				msg.undefine();
				msg << activityInstance->identify() << " does not abstract.";
				error_list << new String_node(msg); }
			*/
		}
	}
	if(theStatus == apgen::RETURN_STATUS::SUCCESS) {
		if(!number_abstracted) {
			more_left = false;
		} else {
			more_left = true;
		}
	}

	return;
}

DETAIL_ACTIVITYrequest::DETAIL_ACTIVITYrequest(	const stringslist& alist,
						bool NewDetailize,
						int which_res,
						int sec,
						bool full)
	: ACTIVITY_LISTrequest("DETAILACTIVITY", alist),
		New(NewDetailize),
		Fully(full),
		ptrs_to_acts_to_decompose(true)
	{}

DETAIL_ACTIVITYrequest::DETAIL_ACTIVITYrequest(	const slist<alpha_void, dumb_actptr>& alist,
						bool NewDetailize,
						int which_res,
						int sec,
						bool full)
	: ACTIVITY_LISTrequest("DETAILACTIVITY", alist),
		New(NewDetailize),
		Fully(full),
		ptrs_to_acts_to_decompose(true)
	{}

DETAIL_ACTIVITYrequest::DETAIL_ACTIVITYrequest(Cstring& errors, ListOVal* args)
	: ACTIVITY_LISTrequest("DETAILACTIVITY", errors, args),
		New(false),
		Fully(false),
		ptrs_to_acts_to_decompose(true)
	{
	ArrayElement* new_ae = args->find("redetail");
	ArrayElement* resolution_ae = args->find("resolution");
	ArrayElement* full_ae = args->find("full");

	if(new_ae) {
		New = new_ae->Val().get_int();
	}
	if(full_ae) {
		Fully = full_ae->Val().get_int();
	}
}

const Cstring & DETAIL_ACTIVITYrequest::get_command_text() {
	if(commandText.length() > 0)
		return commandText;
	if(New) {
		commandText = "REDETAILACTIVITY";
	} else {
		commandText = "DETAILACTIVITY";
	}
	commandText << " ID ";
	append_act_list_to_cmd_text(commandText);
	if(Fully) {
		commandText << " FULLY";
	}
	return commandText;
}

void DETAIL_ACTIVITYrequest::process_middle(TypedValue*) {
	slist<alpha_void, dumb_actptr>::iterator	thePs(list_of_act_pointers);
	dumb_actptr*					ptr;

	if(list_of_act_names.get_length() && !list_of_act_pointers.get_length()) {
		get_pointers();
	}

	removeUnwantedDescendants();
	ptrs_to_acts_to_decompose.clear();

	while((ptr = thePs())) {
		ActivityInstance* act = ptr->payload;
		ptrs_to_acts_to_decompose << new dumb_actptr(act, act);
	}

	if(Fully) {
		do {
			process_middle_private(false);
		} while(ptrs_to_acts_to_decompose.get_length());
	} else {
		process_middle_private(true);
	}
}

void DETAIL_ACTIVITYrequest::process_middle_private(bool report_errors) {
	dumb_actptr*					ptr;
	slist<alpha_void, dumb_actptr>::iterator	thePs(ptrs_to_acts_to_decompose);
	tlist<alpha_void, dumb_actptr>			new_ptrs(true);
	OPEN_FILErequest*				of = list ? list->Owner : NULL;

	while((ptr = thePs())) {
		bool decomposed = false;

		ActivityInstance*	req = ptr->payload;

		if((!Fully) && New) {
			instance_state_changer	ISC(req);

			ISC.set_desired_offspring_to(false);
			ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_DECOMPOSED);
			try { ISC.do_it(NULL);
			      decomposed = true;
			} catch(eval_error Err) {
				add_error(Err.msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
				ptrs_to_acts_to_decompose.clear();
				return;
			} catch(decomp_error Err) {
				add_error(Err.msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
				ptrs_to_acts_to_decompose.clear();
				return;
			}
		} else if(APcloptions::theCmdLineOptions().RedetailOptionEnabled
				|| req->hierarchy().children_count()) {
			instance_state_changer	ISC(req);

			ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_DECOMPOSED);
			try {
				ISC.do_it(NULL);
				decomposed = true;
			}
			catch(eval_error Err) {
				add_error(Err.msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
				ptrs_to_acts_to_decompose.clear();
				return;
			} catch(decomp_error Err) {
				if(report_errors) {
					add_error(Err.msg);
					theStatus = apgen::RETURN_STATUS::FAIL;
					ptrs_to_acts_to_decompose.clear();
					return;
				}
			}
		} else {
			/* not an error; keep going */
		}
		if(decomposed && of) {
			slist<alpha_void, smart_actptr>::iterator
				subs(req->hierarchy().get_down_iterator());
			smart_actptr* b;

			while((b = subs())) {

				//
				// debug
				//
				// extern stringtlist& preciousIDs();
				// preciousIDs() << new emptySymbol(b->BP->get_unique_id());

				new_ptrs << new dumb_actptr(b->BP);
				add_id_to_list(b->BP->get_unique_id());
			}
		}
	}

	// debug
	// for(dumb_actptr* da = new_ptrs.first_node(); da = da->next_node(); da) {
	// 	cerr << "DETAIL: inserting " << da->payload->get_unique_id() << "\n";
	// }
	// cerr << "DETAIL: done\n";

	ptrs_to_acts_to_decompose = new_ptrs;
	return;
}

DETAIL_ALLrequest::DETAIL_ALLrequest(int NewDetailize)
	: Action_request("DETAILALL"),
	New(NewDetailize)
	{}

DETAIL_ALLrequest::DETAIL_ALLrequest(Cstring& errors, ListOVal* args)
	: Action_request("DETAILALL"),
	New(0) {
	ArrayElement* new_ae = args->find("new");

	if(new_ae) {
		New = new_ae->Val().get_int();
	}
}

const Cstring& DETAIL_ALLrequest::get_command_text() {
	if(commandText.length() > 0) {
		return commandText;
	}
	if(New) {
		commandText = "REDETAILALL";
	} else {
		commandText = "DETAILALL";
	}
	return commandText;
}

void DETAIL_ALLrequest::process_middle(TypedValue* ret_val) {
	int		number_detailed = 0;
	status_aware_iterator	acts(eval_intfc::get_act_lists().get_scheduled_active_iterator());
	List		ptrs;
	List_iterator	pointers(ptrs);
	Pointer_node*	ptr;
	ActivityInstance* activityInstance;

	//
	// Note: no need to call removeUnwantedDescendants()
	// 	 because active activities are by definition
	// 	 a cut through the activity hierarchy, hence
	// 	 contain no (ancestor, descendant) pairs
	//

	if(req_intfc::DETAIL_ALLhandler) {
		req_intfc::DETAIL_ALLhandler(this, 0);
	}
	while((	activityInstance = acts.next()))
		ptrs << new Pointer_node(activityInstance, NULL);

	while((ptr = (Pointer_node *) pointers.next())) {
		activityInstance = (ActivityInstance *) ptr->get_ptr();
		/*
		 * this function NEVER returns 1 if the decomp type is RESOLUTION.
		 * This means that resolutions will be quietly ignored. The GUI
		 * does (should?) check that no resolutions are included when
		 * the user wants to 'detail all'.
		 */
		if(activityInstance->hierarchy().can_be_decomposed_without_errors(
					New)) {
			try {
				// V_7_8 CHANGE START
				// number_detailed++;
				// V_7_8 CHANGE end
				if(New) {
					instance_state_changer	ISC(activityInstance);

					ISC.set_desired_offspring_to(false);
					ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_DECOMPOSED);
					ISC.do_it(NULL);
					// activityInstance->hierarchy().make_redecomposed(
					// 	activityInstance->get_typeobject());
				} else if(
					APcloptions::theCmdLineOptions().RedetailOptionEnabled
					|| activityInstance->hierarchy().children_count()) {
					instance_state_changer	ISC(activityInstance);

					ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_DECOMPOSED);
					ISC.do_it(NULL);
				} else {
					//
					// Not an error
					//

#ifdef OBSOLETE
					Cstring msg("DETAIL_ALLrequest: activity ");

					msg << activityInstance->identify() << " has no descendants; cannot detail "
					     	"in -noredetail mode. Use Regen Children first. ";
					throw(eval_error(msg, NULL));
#endif /* OBSOLETE */
				}

				slist<alpha_void, smart_actptr>::iterator	subs(
						activityInstance->hierarchy().get_down_iterator());
				smart_actptr*					b;

				while((b = subs())) {
					// V_7_8 CHANGE START
					// make sure you only increment this if children _were_ created:
					number_detailed++;
					// V_7_8 CHANGE end
					add_id_to_list(b->BP->get_unique_id());
				}
			} catch(eval_error Err) {
				theStatus = apgen::RETURN_STATUS::FAIL;
				add_error(Cstring("Error while detailing act. \"")
					+ activityInstance->identify() + ":\n"
					+ Err.msg);
			} catch(decomp_error Err) {
				theStatus = apgen::RETURN_STATUS::FAIL;
				add_error(Cstring("Error while detailing act. \"")
					+ activityInstance->identify() + ":\n"
					+ Err.msg);
			}
		}
	}
	// If there are any errors, we will display them (see later in this method)
	if(theStatus == apgen::RETURN_STATUS::SUCCESS) {
		//
		// Not an error
		//
#ifdef OBSOLETE	
		if(!number_detailed) {
			theStatus = apgen::RETURN_STATUS::FAIL;
			add_error("Nothing to detail");
		}
#endif /* OBSOLETE */
	} else {
		set_to_selection(ret_val);
	}
	if(req_intfc::DETAIL_ALLhandler) {
		req_intfc::DETAIL_ALLhandler(this, 1);
	}
}

DETAIL_ALL_QUIETrequest::DETAIL_ALL_QUIETrequest(int NewDetailize)
	: Action_request("DETAILALL_QUIET"),
	New(NewDetailize),
	more_left(false) {
	if(New) {
		commandType = "REDETAILALL_QUIET";
	}
}

DETAIL_ALL_QUIETrequest::DETAIL_ALL_QUIETrequest(Cstring& errors, ListOVal* args)
	: Action_request("DETAILALL_QUIET"),
		more_left(false),
		New(0) {
	ArrayElement* new_ae = NULL;

	if((new_ae = args->find("new"))) {
		New = new_ae->Val().get_int();
	}
	if(New) {
		commandType = "REDETAILALL_QUIET";
	}
}

const Cstring&	DETAIL_ALL_QUIETrequest::get_command_text() {
	if(commandText.length() > 0) {
		return commandText;
	}
	if(New) {
		commandText = "REDETAILALL_QUIET";
	} else {
		commandText = "DETAILALL_QUIET";
	}
	return commandText;
}

void DETAIL_ALL_QUIETrequest::process_middle(TypedValue* ret_val) {
	do {
		process_middle_private();
	} while(more_left);
	set_to_selection(ret_val);
	if(req_intfc::DETAIL_ALLhandler) {
		req_intfc::DETAIL_ALLhandler(this, 1);
	}
}

void DETAIL_ALL_QUIETrequest::process_middle_private() {
	int		number_detailed = 0;
	status_aware_iterator	acts(eval_intfc::get_act_lists().get_scheduled_active_iterator());
	List		ptrs;
	List_iterator	pointers(ptrs);
	Pointer_node*	ptr;
	ActivityInstance* activityInstance;

	if(req_intfc::DETAIL_ALLhandler) {
		req_intfc::DETAIL_ALLhandler(this, 0); }
	while((activityInstance = acts.next()))
		ptrs << new Pointer_node(activityInstance, NULL);

	while((ptr = (Pointer_node *) pointers.next())) {
		activityInstance = (ActivityInstance *) ptr->get_ptr();
		/*
		 * this function NEVER returns 1 if the decomp type is RESOLUTION.
		 * This means that resolutions will be quietly ignored. The GUI
		 * does (should?) check that no resolutions are included when
		 * the user wants to 'detail all'.
		 */
		if(activityInstance->hierarchy().can_be_decomposed_without_errors(
					New)) {
			try {
				if(New) {
					instance_state_changer	ISC(activityInstance);

					ISC.set_desired_offspring_to(false);
					ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_DECOMPOSED);
					ISC.do_it(NULL);
				} else if(APcloptions::theCmdLineOptions().RedetailOptionEnabled || activityInstance->hierarchy().children_count()){
					instance_state_changer	ISC(activityInstance);

					ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_DECOMPOSED);
					ISC.do_it(NULL);
				} else {
#ifdef OBSOLETE
					Cstring msg("DETAIL_ALLrequest: activity ");

					msg << activityInstance->identify() << " has no descendants; cannot detail "
					       	"in -noredetail mode. Use Regen Children first. ";
					throw(eval_error(msg, NULL));
#endif /* OBSOLETE */
				}

				slist<alpha_void, smart_actptr>::iterator	subs(
					activityInstance->hierarchy().get_down_iterator());
				smart_actptr*					b;

				while((b = subs())) {
					// make sure you don't increment if no children were created:
					number_detailed++;
					add_id_to_list(b->BP->get_unique_id()); } }
			catch(eval_error Err) {
#				ifdef WE_MUST_BE_QUIET
				set_status(apgen::RETURN_STATUS::FAIL);
				add_error(Cstring("Error while detailing act. \"")
					+ activityInstance->identify() + ":\n"
					+ Err.msg);
#				endif /* WE_MUST_BE_QUIET */
			} catch(decomp_error Err) {
#				ifdef WE_MUST_BE_QUIET
				set_status(apgen::RETURN_STATUS::FAIL);
				add_error(Cstring("Error while detailing act. \"")
					+ activityInstance->identify() + ":\n"
					+ Err.msg);
#				endif /* WE_MUST_BE_QUIET */
			}
		}
	}
	// If there are any errors, we will display them (see later in this method)
	if(theStatus == apgen::RETURN_STATUS::SUCCESS) {
		if(!number_detailed) {
			more_left = false;
		} else {
			more_left = true;
		}
	}
}

const Cstring& PAUSErequest::get_command_text() {
	if(commandText.length() > 0) {
		return commandText;
	}
	commandText = "PAUSE";
	if(Text.length()) {
		commandText << " " << Text;
	}
	return commandText;
}

	/*
	* Old style: we had a function "user_waiter::wait_for_user_action" that blocked until
	* the user made a choice (OK or cancel) on the Pause Popup.
	*
	* New style: we don't assume that XtAppNextEvent is re-entrant, and therefore we need
	* to have a SINGLE motif loop in the entire system. But in that case we need to tell
	* the motif loop that an event has just occurred which demands additional processing,
	* e. g., when the pause panel has been clicked on or timed out on its own.
	*
	* A natural mechanism would be to set the 'user has made a choice' flag and have the
	* motif loop check it every time. Simple rule: anybody who sets that flag must supply
	* a callback stored in main.C that can be used for re-entering the code.
	*/
void PAUSErequest::process_middle(TypedValue*) {
	if(req_intfc::PAUSEhandler) {
		req_intfc::PAUSEhandler(this, 0); }
	}

DELETE_ALL_DESCENDANTSrequest::DELETE_ALL_DESCENDANTSrequest()
	: Action_request("DELETEALLDESCENDANTS") {}

DELETE_ALL_DESCENDANTSrequest::DELETE_ALL_DESCENDANTSrequest(Cstring&, ListOVal*)
	: Action_request("DELETEALLDESCENDANTS") {}

const Cstring &DELETE_ALL_DESCENDANTSrequest::get_command_text() {
	if(commandText.length() > 0) {
		return commandText; }
	commandText = "DELETEALLDESCENDANTS";
	return commandText; }

void DELETE_ALL_DESCENDANTSrequest::process_middle(TypedValue*) {
	ActivityInstance*			activityInstance;
	ActivityInstance*			next_act;
	status_aware_iterator			li(eval_intfc::get_act_lists().get_scheduled_active_iterator());
	Cstring					msg;
	slist<alpha_void, dumb_actptr>		list_of_act_pointers;
	slist<alpha_void, dumb_actptr>::iterator new_pointers(list_of_act_pointers);
	dumb_actptr*				ptr;

	if(req_intfc::DELETE_ALL_DESCENDANTShandler) {
		req_intfc::DELETE_ALL_DESCENDANTShandler(this, 0);
	}

	while((activityInstance = li())) {
		if(activityInstance->get_parent()) {
			Cstring		msg("Activity ");

			msg << activityInstance->get_key() <<
				" is not at the top of the hierarchy; descendants NOT deleted.";
			add_error(msg);
			theStatus = apgen::RETURN_STATUS::FAIL;
		} else {
			list_of_act_pointers << new dumb_actptr(activityInstance);
		}
	}

	apgen::METHOD_TYPE	mt;
	while((ptr = new_pointers())) {
		activityInstance = ptr->payload;
		if(activityInstance->has_decomp_method(mt)) {
			instance_state_changer	ISC(activityInstance);

			ISC.set_desired_offspring_to(false);
			try {
				ISC.do_it(NULL);
			} catch(eval_error Err) {
				add_error(Err.msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
			} catch(decomp_error Err) {
				add_error(Err.msg);
				theStatus = apgen::RETURN_STATUS::FAIL;
			}
		}
	}
	if(req_intfc::DELETE_ALL_DESCENDANTShandler) {
		req_intfc::DELETE_ALL_DESCENDANTShandler(this, 1);
	}
}

DRAG_CHILDRENrequest::DRAG_CHILDRENrequest(int k)
	: Action_request("DRAGCHILDREN"),
	on(k) {}

const Cstring &DRAG_CHILDRENrequest::get_command_text() {
	if(commandText.length() > 0) {
		return commandText; }
	commandText << "DRAGCHILDREN " << on;
	return commandText; }

void DRAG_CHILDRENrequest::process_middle(TypedValue*) {
	Preferences().SetPreference("DragChildren", "ON"); }

// arg defaults to 0 in header:
REMODELrequest::REMODELrequest(
		bool		P,
		CTime_base	T,
		bool		FS,
		int		rt)
	: Action_request("REMODEL"),
		real_time(rt),
		partial(P),
		to_time(T),
		from_start(FS) {
}

REMODELrequest::REMODELrequest(
		Cstring& errors,
		ListOVal* args)
	: Action_request("REMODEL"),
		real_time(0),
		partial(false),
		from_start(false) {
	ArrayElement* ae_partial = args->find("partial");
	ArrayElement* ae_to_time = args->find("to_time");
	ArrayElement* ae_from_start = args->find("from_start");
	if(ae_partial) {
		partial = ae_partial->Val().get_int();
		if(partial) {
			if(!ae_to_time) {
				errors = "to_time not specified";
				partial = false;
			} else if(!ae_to_time->Val().is_time()) {
				errors = "to_time is not a time";
				partial = false;
			} else {
				to_time = ae_to_time->Val().get_time_or_duration();
			}
		}
	} else if(ae_from_start) {
		from_start = ae_from_start->Val().get_int();
	}
}

const Cstring& REMODELrequest::get_command_text() {
	if(commandText.length() > 0) {
		return commandText;
	}
	commandText = commandType;
	if(partial) {
		commandText << " UNTIL " << to_time.to_string();
	} else if(from_start) {
		commandText << " FROM START";
	}
	// else if(real_time) {
	// 	commandText << " TIME 1";
	// }
	return commandText;
}

void REMODELrequest::process_middle(TypedValue*) {
	status_aware_iterator	acts(eval_intfc::get_act_lists().get_scheduled_active_iterator());
	List			ptrs;
	List_iterator		pointers(ptrs);
	Pointer_node*		ptr;
	ActivityInstance*	activityInstance;


	if(req_intfc::REMODELhandler) {
		req_intfc::REMODELhandler(this, 0);
	}

	//
	// Obsolete, I think:
	//
	TypedValuePtrVect empty;
	EventRegistry::Register().PropagateEvent("REMODEL", empty);

	try {
		ACT_exec::ACT_subsystem().AAF_do_model();
	} catch(eval_error Err) {
		add_error(Err.msg);
		set_status(apgen::RETURN_STATUS::FAIL);
		return;
	}

	EventRegistry::Register().PropagateEvent("REMODELEND", empty);
	if(req_intfc::REMODELhandler) {
		req_intfc::REMODELhandler(this, 1);
	}

#ifdef OBSOLETE
	while((activityInstance = acts.next())) {
		activityInstance->notify_assoc_res_if_any(/* recompute = */ true);
	}
#endif /* OBSOLETE */
}

BUILDrequest::BUILDrequest() : Action_request("BUILD") {}
BUILDrequest::BUILDrequest(
		Cstring& errors,
		ListOVal* args)
	: Action_request("BUILD") {}

const Cstring& BUILDrequest::get_command_text() {
	commandText = "BUILD";
	return commandText;
}

void BUILDrequest::process_middle(TypedValue*) {

	//
	// this should cause consolidation of all
	// input files read until now
	//
	if(unconsolidated_files_exist()) {
		try {
			aafReader::consolidate( /* skip remodel = */ false);
		} catch(eval_error(Err)) {
			add_error(Err.msg);
			set_status(apgen::RETURN_STATUS::FAIL);
		}
		unconsolidated_files_exist() = false;
	}
}

CLOSE_ACT_DISPLAYrequest::CLOSE_ACT_DISPLAYrequest(const stringslist& alist)
	: Action_request("CLOSEACTDISPLAY"), // BKL-1-20-98 altered
	my_list(alist) {		      // BKL-1-20-98 altered
	}

CLOSE_ACT_DISPLAYrequest::CLOSE_ACT_DISPLAYrequest(
		Cstring& errors,
		ListOVal* args)
	: Action_request("CLOSEACTDISPLAY") {
	ArrayElement* ae = args->find("displays");
	if(!ae) {
		errors = "Cannot find \"displays\" in args array.";
	} else if(!ae->Val().is_array()) {
		errors = "\"displays\" arg is not an array.";
	} else {
		ListOVal&					theDisplays = ae->Val().get_array();
		ArrayElement*					a_display;

		for(int k = 0; k < theDisplays.get_length(); k++) {
			a_display = theDisplays[k];
			my_list << new emptySymbol(a_display->Val().get_string());
		}
	}
}

const Cstring& CLOSE_ACT_DISPLAYrequest::get_command_text() {
	Cstring		temp;
	emptySymbol*	string_node1;
	int		not_first_time_thru_it = 0;

	if(commandText.length() > 0) return commandText;
	// BKL-1-20-98 altered to show request parameters, if any

	if (my_list.get_length()) {
		string_node1 = my_list.first_node();
		while (string_node1) {
			if (not_first_time_thru_it) {
				temp = temp + ", " + string_node1->get_key();
			} else {
				temp = temp + " " + string_node1->get_key();
			}
			removeQuotes(temp);
			string_node1 = string_node1->next_node();
			not_first_time_thru_it = 1;
		}
		// PFM 7/13/99
		if(!temp.length())
			temp = " 0";
		commandText = commandType + temp;
	} else {
		commandText = commandType + " 0";
	}

	return commandText;
}

void	CLOSE_ACT_DISPLAYrequest::to_script(Cstring& s) const {
	Cstring		temp;
	emptySymbol*	string_node1;
	int		not_first_time_thru_it = 0;

	s << "xcmd(\"CLOSEACTDISPLAY\", [\"displays\" = [";
	if (my_list.get_length()) {
		string_node1 = my_list.first_node();
		while (string_node1) {
			if (not_first_time_thru_it) {
				temp = temp + ", " + addQuotes(string_node1->get_key());
			} else {
				temp = temp + addQuotes(string_node1->get_key());
			}
			string_node1 = string_node1->next_node();
			not_first_time_thru_it = 1;
		}
	}
	if(!temp.length()) {
		s << "\"0\"";
	} else {
		s << temp;
	}
	s << "]]);\n";
}

void CLOSE_ACT_DISPLAYrequest::process_middle(TypedValue*) { // BKL-1-20-98 altered to handle script command
	if(req_intfc::CLOSE_ACT_DISPLAYhandler) {
		req_intfc::CLOSE_ACT_DISPLAYhandler(this, 0); }
	}

CLOSE_RES_DISPLAYrequest::CLOSE_RES_DISPLAYrequest(const stringslist& alist)
	: Action_request("CLOSERESDISPLAY"), // BKL-12/97 altered
	my_list(alist)		      // BKL-11/97 added
	{}

CLOSE_RES_DISPLAYrequest::CLOSE_RES_DISPLAYrequest(const StringVect& resesToClose)
	: Action_request("CLOSERESDISPLAY") {
	StringVect::const_iterator iter = resesToClose.begin();

	while(iter != resesToClose.end()) {
		my_list << new emptySymbol(iter->c_str());
		iter++; } }

CLOSE_RES_DISPLAYrequest::CLOSE_RES_DISPLAYrequest(Cstring& errors, ListOVal* args)
	: Action_request("CLOSERESDISPLAY") {
	ArrayElement* ae = args->find("displays");
	if(!ae) {
		errors = "Cannot find \"displays\" in args array."; }
	else if(!ae->Val().is_array()) {
		errors = "\"displays\" arg is not an array."; }
	else {
		ListOVal&					theDisplays = ae->Val().get_array();
		ArrayElement*					a_display;

		for(int k = 0; k < theDisplays.get_length(); k++) {
			a_display = theDisplays[k];
			my_list << new emptySymbol(a_display->Val().get_string()); } } }

const Cstring& CLOSE_RES_DISPLAYrequest::get_command_text() {
	Cstring		temp;
	emptySymbol*	string_node1;
	int		not_first_time_thru_it = 0;

	if (commandText.length() > 0) return commandText;
	// BKL-11/97 altered to show request parameters, if any

	if (my_list.get_length()) {
		string_node1 = my_list.first_node();
		while(string_node1) {
			if (not_first_time_thru_it) {
				temp = temp + ", " + string_node1->get_key(); }
			else {
				temp = temp + " " + string_node1->get_key(); }
			removeQuotes(temp);
			string_node1 = string_node1->next_node();
			not_first_time_thru_it = 1; }

		// PFM 7/13/99
		if(! temp.length())
			temp = " 0";
		commandText = commandType + temp; }
	else {
		commandText = commandType + " 0"; }

	return commandText; }

void	CLOSE_RES_DISPLAYrequest::to_script(Cstring& s) const {
	Cstring		temp;
	emptySymbol*	string_node1;
	int		not_first_time_thru_it = 0;

	s << "xcmd(\"CLOSERESDISPLAY\", [\"displays\" = [";
	if (my_list.get_length()) {
		string_node1 = my_list.first_node();
		while (string_node1) {
			if (not_first_time_thru_it) {
				temp = temp + ", " + addQuotes(string_node1->get_key()); }
			else {
				temp = temp + addQuotes(string_node1->get_key()); }
			string_node1 = string_node1->next_node();
			not_first_time_thru_it = 1; } }
	if(!temp.length()) {
		s << "\"0\""; }
	else {
		s << temp; }
	s << "]]);\n"; }

void CLOSE_RES_DISPLAYrequest::process_middle(TypedValue*) {
	if(req_intfc::CLOSE_RES_DISPLAYhandler) {
		req_intfc::CLOSE_RES_DISPLAYhandler(this, 0); }
	}

// BKL-1-20-98 altered
NEW_ACT_DISPLAYrequest::NEW_ACT_DISPLAYrequest(CTime_base start_t, CTime_base dur_t)
	: Action_request("NEWACTDISPLAY"),	// BKL-1-20-98 altered
	startTime(start_t), 		// BKL-1-20-98 added
	duration(dur_t) 			// BKL-1-20-98 added
	{}

const Cstring & NEW_ACT_DISPLAYrequest::get_command_text() {
	if (commandText.length() > 0) return commandText;
	commandText = commandType;
	return commandText; }

void NEW_ACT_DISPLAYrequest::process_middle(TypedValue*) { // BKL-1-20-98 altered
	if(req_intfc::NEW_ACT_DISPLAYhandler) {
		req_intfc::NEW_ACT_DISPLAYhandler(this, 0); }
	return; }

NEW_LEGENDrequest::NEW_LEGENDrequest(const Cstring &legend_name, int theHeight)
	: Action_request("NEWLEGEND"),
	legend(legend_name),
	legend_height(theHeight) {
	if(legend_height < ACTVERUNIT)
		legend_height = ACTVERUNIT;
}

const Cstring &NEW_LEGENDrequest::get_command_text() {
	if(commandText.length() > 0) return commandText;
	commandText = commandType + " " + addQuotes(legend) + " " + legend_height;
	return commandText; }

void NEW_LEGENDrequest::process_middle(TypedValue*) {
	//handle null string (or all white space, which was normalized to this)
	if(!legend.length()) {
		add_error("Empty Legend is not allowed.");
		theStatus = apgen::RETURN_STATUS::FAIL;
		return;
	}
	/* Let's do this to be consistent with what NEW_ACTIVITYrequest does.
	 * We should think about the "right thing to do" when reading a script.
	 * If we want the script to duplicate what the user was doing, then we
	 * should set the plan file name to "New" as done below. However, shouldn't
	 * we set the plan file name to the script file name in this case? Clearly,
	 * this and related issues should be handled by a "database manager" of
	 * some type. */
	aafReader::current_file() = "New";
	if(!Dsource::theLegends().find(legend)) {
		/*
		 * Hopper design DONE: use standard method for getting blist populated with ACT_sys'es corresponding
		 * to a given Hopper ID. Unfortunately, we don't have a very object-oriented way of assigning
		 * legends to the hopper right now; we'll just parse the legend name and see if it's of the
		 * form "<digit>Hopper:"
		 */
		LegendObject::LegendObjectFactory(legend, aafReader::current_file(), legend_height);
	}
	ACT_exec::addNewItem();
	return;
}

NEW_RES_DISPLAYrequest::NEW_RES_DISPLAYrequest()
	: Action_request("NEWRESDISPLAY") // BKL-11/97 altered
	{}

const Cstring & NEW_RES_DISPLAYrequest::get_command_text() {
	if (commandText.length() > 0) return commandText;
	commandText = commandType + " 0";
	return commandText; }

void NEW_RES_DISPLAYrequest::process_middle(TypedValue*) {
	if(req_intfc::NEW_RES_DISPLAYhandler) {
		req_intfc::NEW_RES_DISPLAYhandler(this, 0);
	}
	return;
}


ADD_RESOURCErequest::ADD_RESOURCErequest(const stringslist& arlist)
	: Action_request("ADDRESOURCE"),
	list(arlist) // BKL-1/98 altered
	{}

ADD_RESOURCErequest::ADD_RESOURCErequest(Cstring& errors, ListOVal* args)
	: Action_request("ADDRESOURCE") {
	ArrayElement*	res_ae = args->find("resource");
	ArrayElement*	disp_ae = args->find("display");
	if(!res_ae) {
		errors << "Cannot find \"resource\" in args array. "; }
	if(!disp_ae) {
		errors << "Cannot find \"display\" in args array. "; }
	if(res_ae && disp_ae) {
		list << new emptySymbol(res_ae->Val().get_string());
		list << new emptySymbol(disp_ae->Val().get_string()); } }

const Cstring& ADD_RESOURCErequest::get_command_text() {
	Cstring		temp;
	emptySymbol*	string_node1;

	if (commandText.length() > 0) return commandText;
	// BKL-1/98 altered to show request parameters, if any

	// the list MUST be populated w/ 2 elements PFM 7/13/99
	if(list.get_length() == 2) {
		string_node1 = list.first_node();
		while (string_node1) {
			temp = temp + " " + string_node1->get_key();
			string_node1 = string_node1->next_node();
		}
		commandText = commandType + temp;
	} else {
		commandText = commandType + " BogusResource BogusResourceDisplay";
	}

	return commandText;
}

void ADD_RESOURCErequest::process_middle(TypedValue*) { // BKL-1/98 filled in this method
	if(req_intfc::ADD_RESOURCEhandler) {
		req_intfc::ADD_RESOURCEhandler(this, 0); }
	return; }

DEBUGrequest::DEBUGrequest(const Cstring &l_name)
	: Action_request("DEBUG"),
	on(l_name)
	{}

DEBUGrequest::DEBUGrequest(Cstring& errors, ListOVal* args)
	: Action_request("DEBUG") {
	ArrayElement*	ae = args->find("on");
	if(!ae) {
		errors = "Cannot find argument \"on\"."; }
	else {
		on = ae->Val().get_string(); } }

const Cstring& DEBUGrequest::get_command_text() {
	if (commandText.length() > 0) return commandText;
	commandText = commandType + " " + on;
	return commandText; }

void DEBUGrequest::process_middle(TypedValue*) {
#	ifdef apDEBUG
	if(!strcasecmp(*on, "ON")) {
		enablePrint = 1; }
	else {
		enablePrint = 0; }
#	endif
	return; }

DELETE_LEGENDrequest::DELETE_LEGENDrequest(const Cstring &l_name)
	: Action_request("DELETELEGEND"),
	name(l_name)
	{}

const Cstring & DELETE_LEGENDrequest::get_command_text() {
	if (commandText.length() > 0) return commandText;
	commandText = commandType + " " + name;
	return commandText; }

void DELETE_LEGENDrequest::process_middle(TypedValue*) {
	if(!serveropt()) {
		/* CHECKED */ cout << *get_command_text_with_modifier() << endl; }
	return; }

MOVE_LEGENDrequest::MOVE_LEGENDrequest(const Cstring &l_name, int n)
	: Action_request("MOVELEGEND"),
	name(l_name),
	legend_index(n)
	{ }

const Cstring & MOVE_LEGENDrequest::get_command_text() {
	if (commandText.length() > 0) return commandText;
	commandText = commandType + " " + name + " " + legend_index;
	return commandText; }

void MOVE_LEGENDrequest::process_middle(TypedValue*) {
	if(!serveropt()) {
		/* CHECKED */ cout << *get_command_text_with_modifier() << endl; }
	return; }

REMOVE_RESOURCErequest::REMOVE_RESOURCErequest() // BKL-1/98 altered
	: Action_request("REMOVERESOURCE")
	{}

const Cstring & REMOVE_RESOURCErequest::get_command_text() {
	Cstring		temp;
	String_node		*string_node1;

	if (commandText.length() > 0) return commandText;
	// used to be just commandType; however this leads to ambiguous parsing.
	commandText = commandType + " 0";
	return commandText; }

void REMOVE_RESOURCErequest::process_middle(TypedValue*) {
	if(req_intfc::REMOVE_RESOURCEhandler) {
		req_intfc::REMOVE_RESOURCEhandler(this, 0); }
	return; }

void RESOURCE_SCROLLrequest::process_middle(TypedValue*) {
	if(req_intfc::RESOURCE_SCROLLhandler) {
		req_intfc::RESOURCE_SCROLLhandler(this, 0); }
	}

const Cstring &RESOURCE_SCROLLrequest::get_command_text() {
	if(commandText.length() > 0)
		return commandText;
	commandText = commandType;
	commandText << " " << theMinimum << " " << theSpan;
	return commandText; }

SELECT_ACT_DISPLAYrequest::SELECT_ACT_DISPLAYrequest(const Cstring &theDisplay, int on_off)
	: Action_request("SELECTACTDISPLAY"),
	which_display(theDisplay),
	on_or_off(on_off) {}

const Cstring &SELECT_ACT_DISPLAYrequest::get_command_text() {
	if(commandText.is_defined())
		return commandText;
	commandText = commandType;
	commandText << " " << which_display << " " << on_or_off;
	return commandText; }

void SELECT_ACT_DISPLAYrequest::process_middle(TypedValue*) {
	return; }


SELECT_ACT_LEGENDrequest::SELECT_ACT_LEGENDrequest(const Cstring &theLegend, int on_off)
	: Action_request("SELECTACTLEGEND"),
	which_legend(theLegend),
	on_or_off(on_off) {}

const Cstring &SELECT_ACT_LEGENDrequest::get_command_text() {
	if(commandText.is_defined())
		return commandText;
	commandText = commandType;
	commandText << " " << which_legend << " " << on_or_off;
	return commandText; }

void SELECT_ACT_LEGENDrequest::process_middle(TypedValue*) {
	LegendObject	*theLeg = (LegendObject *) Dsource::theLegends().find(which_legend);

	if(!theLeg) {
		Cstring		theError("Legend ");

		theError << which_legend << " cannot be selected because it doesn't seem to exist.";
		add_error(theError);
		theStatus = apgen::RETURN_STATUS::FAIL;
		return; }
	theLeg->set_selection(on_or_off);
	return; }

SELECT_RES_LEGENDrequest::SELECT_RES_LEGENDrequest(const Cstring &theAD, const Cstring &theLegend, int on_off)
	: Action_request("SELECTRESLEGEND"),
	which_ad(theAD),
	which_legend(theLegend),
	on_or_off(on_off) {}

const Cstring &SELECT_RES_LEGENDrequest::get_command_text() {
	if(commandText.is_defined())
		return commandText;
	commandText = commandType;
	commandText << " " << which_ad << " " << which_legend << " " << on_or_off;
	return commandText;
}

void SELECT_RES_LEGENDrequest::process_middle(TypedValue*) {
	if(req_intfc::SELECT_RES_LEGENDhandler) {
		req_intfc::SELECT_RES_LEGENDhandler(this, 0);
	}
	return;
}

//
// OBSOLETE
//
bool Action_request::validate() {
	bool	result = true;

	return result;
}

FREEZErequest::FREEZErequest(const vector<string>& res)
	: Action_request("FREEZE"),
		resources(res) {
}

const Cstring& FREEZErequest::get_command_text() {
	aoString 	s;
	bool		first = true;
	
	commandText << "FREEZE";
	for(int i = 0; i < resources.size(); i++) {
		if(first) {
			first = false;
			s << " ";
		} else {
			s << ",\n\t";
		}
		s << resources[i].c_str();
	}
	s << "\n";
	commandText << s.str();
	return commandText;
}

void FREEZErequest::process_middle(TypedValue*) {
	tlist<alpha_string, Cntnr<alpha_string, Rsource*> >  res_ptrs;
	Rsource::expand_list_of_resource_names(
	    resources,
	    res_ptrs);
	Cntnr<alpha_string, Rsource*>* N;
	slist<alpha_string, Cntnr<alpha_string, Rsource*> >::iterator iter(res_ptrs);
	while((N = iter())) {
		N->payload->get_history().Freeze();
	}
}

UNFREEZErequest::UNFREEZErequest(const vector<string>& res)
	: Action_request("UNFREEZE"),
		resources(res) {
}

const Cstring& UNFREEZErequest::get_command_text() {
	aoString 	s;
	bool		first = true;
	
	commandText << "UNFREEZE";
	for(int i = 0; i < resources.size(); i++) {
		if(first) {
			first = false;
			s << " ";
		} else {
			s << ",\n\t";
		}
		s << resources[i].c_str();
	}
	s << "\n";
	commandText << s.str();
	return commandText;
}

void UNFREEZErequest::process_middle(TypedValue*) {
	tlist<alpha_string, Cntnr<alpha_string, Rsource*> >  res_ptrs;
	Rsource::expand_list_of_resource_names(
	    resources,
	    res_ptrs);
	Cntnr<alpha_string, Rsource*>* N;
	slist<alpha_string, Cntnr<alpha_string, Rsource*> >::iterator iter(res_ptrs);
	while((N = iter())) {
		N->payload->get_history().Unfreeze();
	}
}



FIND_RESOURCErequest::FIND_RESOURCErequest(const stringslist& arlist)
	: Action_request("FINDRESOURCE"),
	// Wrong place to worry about the GUI. The right place is in the motif_widget callback.
	// not_from_gui(from_gui_flag), BKL-1/98 added
	list(arlist) // BKL-1/98 altered
	{}

const Cstring &FIND_RESOURCErequest::get_command_text() {
	Cstring		temp;
	emptySymbol*	string_node1;

	if (commandText.length() > 0) return commandText;
	// BKL-1/98 altered to show request parameters, if any

	// the list MUST be populated w/ 2 elements PFM 7/13/99
	if(list.get_length() == 2) {
		string_node1 = list.first_node();
		while (string_node1) {
			temp = temp + " " + string_node1->get_key();
			string_node1 = string_node1->next_node(); }

		commandText = commandType + temp; }
	else {
		commandText = commandType + " BogusResource BogusResourceDisplay"; }

	return commandText; }

void FIND_RESOURCErequest::process_middle(TypedValue*) { // BKL-1/98 filled in this method
	if(req_intfc::FIND_RESOURCEhandler) {
		req_intfc::FIND_RESOURCEhandler(this, 0); }
	return; }

XCMDrequest::XCMDrequest(const stringslist& list_of_names, const Cstring& req)
	: ACTIVITY_LISTrequest("XCMD", list_of_names),
	request(req) {
}

XCMDrequest::XCMDrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers, const Cstring& req)
	: ACTIVITY_LISTrequest("XCMD", list_of_pointers),
	request(req) {
}

apgen::RETURN_STATUS XCMDrequest::get_pointers() {
	if(list_of_act_names.get_length() && !list_of_act_pointers.get_length()) {
		bool		no_no_no;
		extract_pointers(no_no_no);
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

const Cstring& XCMDrequest::get_command_text() {
	Cstring			temp;

	if(commandText.length() > 0) return commandText;
	commandText = commandType + " " + request;

	if(list_of_act_names.get_length() || list_of_act_pointers.get_length()) {
		commandText << " ID ";
		append_act_list_to_cmd_text(commandText);
	}

	return commandText;
}

void XCMDrequest::process_middle(TypedValue* ret_val) {
	std::vector<TypedValue*>	args;

	if(list_of_act_names.get_length() || list_of_act_pointers.get_length()) {
		get_pointers();
		set_selection();
	}

	try {
		pEsys::FunctionCall::eval_script(
			request,
			args,
			*ret_val);
	}
	catch(eval_error Err) {
		add_error(Err.msg);
		theStatus = apgen::RETURN_STATUS::FAIL;
	}

}

TypedValue Action_request::process_an_AAF_request(
		const Cstring& request,
		ListOVal& args) {
    TypedValue			request_data;
    Action_request*			ar = NULL;
    apgen::RETURN_STATUS		return_status = apgen::RETURN_STATUS::SUCCESS;
    Cstring				errors;

    if(request == "ABSTRACT_ACTIVITY" || request == "ABSTRACTACTIVITY") {
	ar = new ABSTRACT_ACTIVITYrequest(errors, &args);
    } else if(request == "ABSTRACT_ALL" || request == "ABSTRACTALL") {
	ar = new ABSTRACT_ALLrequest(errors, &args);
    } else if(request == "ABSTRACT_ALL_QUIET" || request == "ABSTRACTALL_QUIET") {
	ar = new ABSTRACT_ALL_QUIETrequest(errors, &args);
    } else if(request == "ADD_RESOURCE" || request == "ADDRESOURCE") {
	ar = new ABSTRACT_ALL_QUIETrequest(errors, &args);
    } else if(request == "BUILD") {
	ar = new BUILDrequest(errors, &args);
    } else if(request == "DELETE_ACTIVITY" || request == "DELETEACTIVITY") {
	ar = new DELETE_ACTIVITYrequest(errors, &args);
    } else if(request == "DELETE_ALL_DESCENDANTS" || request == "DELETEALLDESCENDANTS") {
	ar = new DELETE_ALL_DESCENDANTSrequest(errors, &args);
    } else if(request == "DETAIL_ACTIVITY" || request == "DETAILACTIVITY") {
	ar = new DETAIL_ACTIVITYrequest(errors, &args);
    } else if(request == "DETAIL_ALL" || request == "DETAILALL") {
	ar = new DETAIL_ALLrequest(errors, &args);
    } else if(request == "DETAIL_ALL_QUIET" || request == "DETAILALL_QUIET") {
	ar = new DETAIL_ALL_QUIETrequest(errors, &args);
    } else if(request == "EDIT_ACTIVITY" || request == "EDITACTIVITY") {
	ar = new EDIT_ACTIVITYrequest(errors, &args);
    } else if(request == "FILE_CONSOLIDATE" || request == "CONSOLIDATE") {
	ar = new FILE_CONSOLIDATErequest(errors, &args);
    } else if(request == "NEW_ACTIVITY" || request == "NEWACTIVITY") {
	ar = new NEW_ACTIVITYrequest(errors, &args);
    } else if(request == "NEW_ACTIVITIES" || request == "NEWACTIVITIES") {
	ar = new NEW_ACTIVITIESrequest(errors, &args);
    } else if(request == "NEW_HORIZON" || request == "NEWHORIZON") {
	ar = new NEW_HORIZONrequest(errors, &args);
    } else if(request == "OPEN_FILE" || request == "OPENFILE") {
	ar = new OPEN_FILErequest(errors, &args);
    } else if(request == "QUIT") {
	ar = new QUITrequest(errors, &args);
    } else if(request == "REGEN_CHILDREN" || request == "REGENCHILDREN") {
	ar = new REGEN_CHILDRENrequest(errors, &args);
    } else if(request == "REMODEL") {
	ar = new REMODELrequest(errors, &args);
    } else if(request == "SAVE_FILE" || request == "SAVEFILE") {
	ar = new SAVE_FILErequest(errors, &args);
    } else if(request == "SAVE_PARTIAL_FILE" || request == "SAVEPARTIALFILE") {
	ar = new SAVE_PARTIAL_FILErequest(errors, &args);
    } else if(request == "SCHEDULE_ACTIVITY" || request == "SCHEDULEACTIVITY") {
	ar = new SCHEDULE_ACTrequest(errors, &args);
    } else if(request == "UNGROUP_ACTIVITIES" || request == "UNGROUPACTIVITIES") {
	ar = new UNGROUP_ACTIVITIESrequest(errors, &args);
    } else if(request == "UNSCHEDULE_ACTIVITY" || request == "UNSCHEDULEACTIVITY") {
	ar = new UNSCHEDULE_ACTrequest(errors, &args);
    } else if(request == "WRITE_JSON" || request == "WRITEJSON") {
	ar = new WRITE_JSONrequest(errors, &args);
    } else if(request == "WRITE_TOL" || request == "WRITETOL") {
	ar = new WRITE_TOLrequest(errors, &args);
    } else if(request == "WRITE_XMLTOL" || request == "WRITEXMLTOL") {
	ar = new WRITE_XMLTOLrequest(errors, &args);
    } else if(request == "WRITE_SASF" || request == "WRITESASF") {
	ar = new WRITE_SASFrequest(errors, &args);
    } else {
	errors = "Unknown request ";
	errors << request << " ";
	throw(eval_error(errors));
    }

    if(errors.length()) {
	throw(eval_error(errors));
    }

    ar->process_middle(&request_data);

    ListOVal*		lov = new ListOVal;
    ArrayElement*	status_element;

    status_element = new ArrayElement(Cstring("status"));
    lov->add(status_element);

    return_status = ar->get_status();

    if(return_status == apgen::RETURN_STATUS::SUCCESS) {

	//
	// Should log this request first
	//
	delete ar;

	TypedValue	V;
	V = "OK";
	status_element->SetVal(V);

	ArrayElement* ae = new ArrayElement(Cstring("data"));
	ae->SetVal(request_data);
	lov->add(ae);
    } else {
	aoString	theErrors;
	stringslist::iterator errs(ar->local_errors);
	emptySymbol*	N;

	while((N = errs())) {
	    theErrors << N->get_key() << "\n";
	}
	delete ar;

#ifdef NEW_SCHEME_DO_NOT_FORGIVE_ERRORS
	TypedValue V;
	V = "ERROR";
	status_element->SetVal(V);

	ArrayElement* ae = new ArrayElement(Cstring("message"));
	V = theErrors.str();
	ae->SetVal(V);
	lov->add(ae);
#endif /* NEW_SCHEME_DO_NOT_FORGIVE_ERRORS */

	throw(eval_error(theErrors.str()));

    }
    TypedValue	returned_value;
    returned_value = *lov;
    return returned_value;
}

void apcoreWaiter::setTheCommandTo(Action_request* AR) {
	getTheCommand() = AR;
	errors.undefine();
	AR->set_error_string_to(&errors);
}
