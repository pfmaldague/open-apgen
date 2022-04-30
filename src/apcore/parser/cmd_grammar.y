%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmd_grammar_incl.h"
#include "ParsedExpressionSystem.H"
#include "CMD_exec.H"
#include "ActivityInstance.H"

	// for ACTVERUNIT:
#include "UI_dsconfig.h"

extern char* wwtext;
#define YYDEBUG 1
#define YYSTYPE_IS_DECLARED
#define YYSTYPE cmdExp
%}

%start input_file

%token TOK_AAF
%token TOK_ABSTRACT
%token TOK_ABSTRACTALL
%token TOK_ABSTRACTALL_QUIET
%token TOK_ACTLEGSELECT
%token TOK_ADSELECT
%token TOK_ADDRECIPIENT
%token TOK_ADDRESOURCE
%token TOK_ALLACTSVISIBLE
%token TOK_APF
%token TOK_APGEN
%token TOK_ARRAYVAL
%token TOK_BUILD
%token TOK_BY
%token TOK_CHAMELEON
%token TOK_CLOSEACTDISPLAY
%token TOK_CLOSERESDISPLAY
%token TOK_COLON
%token TOK_COMMA
%token TOK_CONSOLIDATE
%token TOK_COPY
%token TOK_CUT
%token TOK_DEBUG
%token TOK_DELETE
%token TOK_DELETELEGEND
%token TOK_DELETEALLDESCENDANTS
%token TOK_DETAIL
%token TOK_DETAILALL
%token TOK_DETAILALL_QUIET
%token TOK_DISPLAYPARENTS
%token TOK_DRAGCHILDREN
%token TOK_DURATION
%token TOK_EDIT
%token TOK_EDIT_GLOBAL
%token TOK_ENDTIME
%token TOK_EQUAL
%token TOK_EXCLUDE
%token TOK_EXPORT
%token TOK_FILENAME
%token TOK_FILTER
%token TOK_FIXRESOURCES
%token TOK_FLOAT
%token TOK_FORMAT
%token TOK_FREEZE
%token TOK_FROM
%token TOK_FULLY
%token TOK_GRAB
%token TOK_GROUPACTIVITIES
%token TOK_ID
%token TOK_IDHOLDER
%token TOK_INCREMENTAL
%token TOK_INTEGER
%token TOK_LAYOUT
%token TOK_LEGEND
%token TOK_MIX
%token TOK_MODIFIER
%token TOK_MOVEACTIVITY
%token TOK_MOVELEGEND
%token TOK_NAME
%token TOK_NEW
%token TOK_NEWACTIVITIES
%token TOK_NEWACTDISPLAY
%token TOK_NEWLEGEND
%token TOK_NEWRESDISPLAY
%token TOK_NOCARRY
%token TOK_NOLEGEND
%token TOK_NONE
%token TOK_OFF
%token TOK_ON
%token TOK_OPENFILE
%token TOK_PARENT
%token TOK_PASTE
%token TOK_PAUSE
%token TOK_PERIOD
%token TOK_PRINT
%token TOK_PURGE
%token TOK_QUIT
%token TOK_REDETAIL
%token TOK_REDETAILALL
%token TOK_REDETAILALL_QUIET
%token TOK_REGENCHILDREN
%token TOK_REMODEL
%token TOK_REMOVERESOURCE
%token TOK_RESOURCE
%token TOK_RESSCROLL
%token TOK_RESLEGSELECT
%token TOK_SASF
%token TOK_SAVE
%token TOK_SAVE_PARTIALFILE
%token TOK_SCHEDULE
%token TOK_SCHEMA
%token TOK_SCRIPT
%token TOK_SELECT
%token TOK_SINGLE_EXPRESSION
%token TOK_SINGLE_QUOTED
%token TOK_START
%token TOK_STARTTIME
%token TOK_STRINGVAL
%token TOK_SYMBOL
%token TOK_TAG
%token TOK_TIME
%token TOK_TIMES
%token TOK_TIMESYSTEM
%token TOK_TO
%token TOK_TOL
%token TOK_UIACTIVITY
%token TOK_UNFREEZE
%token TOK_UNFREEZERESOURCES
%token TOK_UNGROUPACTIVITIES
%token TOK_UNKNOWN
%token TOK_UNSCHEDULE
%token TOK_UNTIL
%token TOK_VERSION
%token TOK_XCMD
%token TOK_XML
%token TOK_XLOC
%token TOK_YLOC
%token TOK_ZOOM

%%


a_symbol: TOK_AAF
	  {
	  $$ = $1;
	  }
	| TOK_ABSTRACT
	  {
	  $$ = $1;
	  }
	| TOK_ABSTRACTALL
	  {
	  $$ = $1;
	  }
	| TOK_ABSTRACTALL_QUIET
	  {
	  $$ = $1;
	  }
	| TOK_ACTLEGSELECT
	  {
	  $$ = $1;
	  }
	| TOK_ADDRECIPIENT
	  {
	  $$ = $1;
	  }
	| TOK_ADDRESOURCE
	  {
	  $$ = $1;
	  }
	| TOK_APF
	  {
	  $$ = $1;
	  }
	| TOK_APGEN
	  {
	  $$ = $1;
	  }
	| TOK_BUILD
	  {
	  $$ = $1;
	  }
	| TOK_BY
	  {
	  $$ = $1;
	  }
	| TOK_CLOSEACTDISPLAY
	  {
	  $$ = $1;
	  }
	| TOK_CLOSERESDISPLAY
	  {
	  $$ = $1;
	  }
/*         | TOK_CMDREQUEST
	| TOK_CONNECT
	| TOK_CONSTRAINT
 */
	| TOK_CONSOLIDATE
	  {
	  $$ = $1;
	  }
	| TOK_COPY
	  {
	  $$ = $1;
	  }
	| TOK_CUT
	  {
	  $$ = $1;
	  }
	| TOK_DEBUG
	  {
	  $$ = $1;
	  }
	| TOK_DELETE
	  {
	  $$ = $1;
	  }
	| TOK_DELETEALLDESCENDANTS
	  {
	  $$ = $1;
	  }
	| TOK_DELETELEGEND
	  {
	  $$ = $1;
	  }
	| TOK_DETAIL
	  {
	  $$ = $1;
	  }
	| TOK_DETAILALL
	  {
	  $$ = $1;
	  }
	| TOK_DETAILALL_QUIET
	  {
	  $$ = $1;
	  }
        | TOK_DISPLAYPARENTS
	  { /* HAVE_EUROPA only */ ; }
	| TOK_EDIT
	  {
	  $$ = $1;
	  }
	| TOK_EDIT_GLOBAL
	  {
	  $$ = $1;
	  }
	| TOK_ENDTIME
	  {
	  $$ = $1;
	  }
	| TOK_EXCLUDE
	  {
	  $$ = $1;
	  }
	| TOK_EXPORT
	  {
	  $$ = $1;
	  }
	| TOK_FILENAME
	  {
	  $$ = $1;
	  }
	| TOK_FILTER
	  {
	  $$ = $1;
	  }
        | TOK_FIXRESOURCES
	  {
	  $$ = $1;
	  }
	| TOK_FORMAT
	  {
	  $$ = $1;
	  }
	| TOK_FREEZE
	  {
	  $$ = $1;
	  }
	| TOK_UNFREEZE
	  {
	  $$ = $1;
	  }
	| TOK_FROM
	  {
	  $$ = $1;
	  }
	| TOK_GROUPACTIVITIES
	  {
	  $$ = $1;
	  }
	| TOK_ID
	  {
	  $$ = $1;
	  }
	| TOK_LEGEND
	  {
	  $$ = $1;
	  }
	| TOK_MIX
	  {
	  $$ = $1;
	  }
	| TOK_MOVEACTIVITY
	  {
	  $$ = $1;
	  }
	| TOK_MOVELEGEND
	  {
	  $$ = $1;
	  }
	| TOK_NAME
	  {
	  $$ = $1;
	  }
	| TOK_NEW
	  {
	  $$ = $1;
	  }
        | TOK_NEWACTIVITIES
	  {
	  $$ = $1;
	  }
	| TOK_NEWACTDISPLAY
	  {
	  $$ = $1;
	  }
	| TOK_NEWLEGEND
	  {
	  $$ = $1;
	  }
	| TOK_NEWRESDISPLAY
	  {
	  $$ = $1;
	  }
	| TOK_NOCARRY
	  {
	  $$ = $1;
	  }
	| TOK_NOLEGEND
	  {
	  $$ = $1;
	  }
	| TOK_NONE
	  {
	  $$ = $1;
	  }
	| TOK_OFF
	  {
	  $$ = $1;
	  }
	| TOK_ON
	  {
	  $$ = $1;
	  }
	| TOK_OPENFILE
	  {
	  $$ = $1;
	  }
	| TOK_PASTE
	  {
	  $$ = $1;
	  }
        | TOK_PERIOD
	  {
	  $$ = $1;
	  }
	| TOK_PRINT
	  {
	  $$ = $1;
	  }
	| TOK_PURGE
	  {
	  $$ = $1;
	  }
	| TOK_QUIT
	  {
	  $$ = $1;
	  }
	| TOK_REDETAIL
	  {
	  $$ = $1;
	  }
	| TOK_REDETAILALL
	  {
	  $$ = $1;
	  }
	| TOK_REDETAILALL_QUIET
	  {
	  $$ = $1;
	  }
	| TOK_REGENCHILDREN
	  {
	  $$ = $1;
	  }
	| TOK_REMODEL
	  {
	  $$ = $1;
	  }
	| TOK_REMOVERESOURCE
	  {
	  $$ = $1;
	  }
	| TOK_RESSCROLL
	  {
	  $$ = $1;
	  }
	| TOK_RESLEGSELECT
	  {
	  $$ = $1;
	  }
	| TOK_SASF
	  {
	  $$ = $1;
	  }
	| TOK_SAVE
	  {
	  $$ = $1;
	  }
	| TOK_SAVE_PARTIALFILE
	  {
	  $$ = $1;
	  }
	| TOK_SCRIPT
	  {
	  $$ = $1;
	  }
	| TOK_SELECT
	  {
	  $$ = $1;
	  }
	| TOK_START
	  {
	  $$ = $1;
	  }
	| TOK_STARTTIME
	  {
	  $$ = $1;
	  }
	| TOK_SYMBOL
	  {
	  $$ = $1;
	  }
	| TOK_TO
	  {
	  $$ = $1;
	  }
	| TOK_TOL
	  {
	  $$ = $1;
	  }
        | TOK_TIMES
	  {
	  $$ = $1;
	  }
	| TOK_UNKNOWN
	  {
	  $$ = $1;
	  }
	| TOK_VERSION
	  {
	  $$ = $1;
	  }
	| TOK_XML
	  {
	  $$ = $1;
	  }
	| TOK_ZOOM
	  {
	  $$ = $1;
	  }
	;

input_file	    :  TOK_APGEN 
			TOK_SCRIPT 
			TOK_VERSION TOK_STRINGVAL command_list
			| TOK_SINGLE_EXPRESSION command1
		    ;


command_list	    : command1
			| command_list command1
		    ;

command1	    : command
		      {
		      reset_error_flag();
		      CMD_exec::cmd_subsystem().clear_lists();
		      }
		    ;

command		      :	cmd_abstract_activity
			| cmd_abstract_all
			| cmd_abstract_all_quiet
			| cmd_add_recipient
			| cmd_add_resource
			| cmd_save_file
			| cmd_build
			| cmd_close_act_display
			| cmd_close_res_display
/*                         | cmd_cmdrequest
			| cmd_constraint
			| cmd_connect */
			| cmd_consolidate
			| cmd_copy_activity
			| cmd_cut_activity
			| cmd_delete_activity
			| cmd_delete_all_desc
			| cmd_debug
			| cmd_delete_legend
			| cmd_detail_activity
			| cmd_drag_children
			| cmd_detail_all
			| cmd_detail_all_quiet
			| cmd_edit_activity
			| cmd_edit_global
			| cmd_export_file
			| cmd_freeze
			| cmd_unfreeze
			| cmd_grab
			| cmd_group_activity
			| cmd_move_activity
			| cmd_move_legend
			| cmd_new_activity
                        | cmd_new_activities
			| cmd_new_act_display
			| cmd_new_legend
			| cmd_new_res_display
			| cmd_open_file
			| cmd_paste_activity
			| cmd_pause
			| cmd_print
			| cmd_purge
			| cmd_quit
			| cmd_redetail_activity
			| cmd_redetail_all
			| cmd_redetail_all_quiet
			| cmd_regenchildren
			| cmd_remove_resource
			| cmd_remodel
			| cmd_res_scroll
			| cmd_save_partial_file
			| cmd_schedule_activity
			| cmd_select_act_display
			| cmd_select_act_legend
			| cmd_select_res_legend
			| cmd_ungroup_activity
			| cmd_unfreeze_activity
			| cmd_unschedule_activity
			| cmd_ui_activity
			| cmd_write_sasf
			| cmd_write_tol
			| cmd_write_xmltol
			| cmd_xcmd
			| cmd_zoom
                        | command modifier
			;

modifier                : TOK_MODIFIER TOK_EQUAL TOK_STRINGVAL
                          {
			  // process_modifier();
			  }
			;

cmd_open_file		: TOK_OPENFILE filename
			  {
			  OPEN_FILErequest*	request;
			  Cstring		name_of_file($2->theData);
			  // V31.0 change
			  removeQuotes(name_of_file);
			  request = new OPEN_FILErequest(compiler_intfc::FILE_NAME, name_of_file);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			| TOK_OPENFILE filename TOK_STRINGVAL
			  {
			  OPEN_FILErequest*	request;
			  Cstring		garbage($2->theData);
			  Cstring		temp($3->theData);
			  removeQuotes(temp);
			  request = new OPEN_FILErequest(compiler_intfc::CONTENT_TEXT, temp);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

export_file_info	: a_symbol TOK_STRINGVAL
		 	  {
			  $$ = $2;
			  }
			| TOK_STRINGVAL
			  {
			  $$ = $1;
			  }

cmd_export_file		: TOK_EXPORT a_symbol export_file_info
			  {
			  EXPORT_DATArequest*	request;
			  Cstring what_string = $2->theData;

			  //
			  // Initially there were going to be several
			  // file formats available, but that is not
			  // very useful nor realistic
			  //
			  Cstring how_string = "JSON_FILE";

			  Cstring where_to_export = $3->theData;
			  removeQuotes(where_to_export);

			  EXPORT_DATArequest::what_to_export what;
			  EXPORT_DATArequest::how_to_export how;
			  bool found_an_error = false;
			  if(what_string == "ACTIVITY_INTERACTIONS") {
				what = EXPORT_DATArequest::ACTIVITY_INTERACTIONS;
			  } else if(what_string == "ACTIVITY_INSTANCES") {
				what = EXPORT_DATArequest::ACTIVITY_INSTANCES;
			  } else if(what_string == "FUNCTION_DECLARATIONS") {
				what = EXPORT_DATArequest::FUNCTION_DECLARATIONS;
			  } else if(what_string == "GLOBALS") {
				what = EXPORT_DATArequest::GLOBALS;
			  } else {
				found_an_error = true;
			  }
			  if(how_string == "JSON_FILE") {
				how = EXPORT_DATArequest::JSON_FILE;
			  } else {
				found_an_error = true;
			  }
			  if(found_an_error) {
			    Cstring err;
			    err << "export data: invalid options " << what_string
				<< ", " << how_string
				<< ". Valid options are "
				<< "ACTIVITY_INTERACTIONS and JSON_FILE.";
			    zzerror((char*) *err);
			    YYACCEPT;
			  }
			  request = new EXPORT_DATArequest(
						what,
						how,
						where_to_export);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_grab		: TOK_GRAB TOK_IDHOLDER
			  {
			  // process_grab_id();
			  }
			;

cmd_ui_activity		: TOK_UIACTIVITY TOK_STRINGVAL act_id
			  {
			  // process_ui_activity();
			  }
			;

cmd_write_sasf		: TOK_SASF filename  write_sasf_tail
			  {
			  WRITE_SASFrequest*	request;

			  //
			  // debug
			  //
			  // cout << "    (cmd_grammar) WRITE SASF command: filename = "
			  // 	<< $2->theData << "\n";
			  // cout << "                      start time: "
			  // 	<< $3->expressions[0]->theData << "\n";
			  // cout << "                      end time: "
			  // 	<< $3->expressions[1]->theData << "\n";
			  // cout << "                      number of symbols: "
			  // 	<< ($3->expressions.size() - 2) << "\n";

			  Cstring desired_file_name = $2->theData;
			  stringslist symbolic_file_names;

			  //
			  // this flag determines whether activities that
			  // start at the end time should be included in
			  // the output SASF:
			  //
			  int inclusion_flag = 0;

			  // Makeup of write_sasf_tail:
			  //	element 0 is start time
			  //	element 1 is end time
			  //	element 2 is number of files
			  //	elements 3 through 2 + <number of files> are the file names
			  //	element 3 + <number of files> is the inclusion flag
			  for(int i = 3; i < $3->expressions.size(); i++) {
			      if(i < $3->expressions.size() - 1) {
				symbolic_file_names << new emptySymbol(
						$3->expressions[i]->theData);
			      } else {
				inclusion_flag = atoi(* $3->expressions[i]->theData);
			      }
			  }
			  Cstring start_time = $3->expressions[0]->theData;
			  Cstring end_time = $3->expressions[1]->theData;
			  request = new WRITE_SASFrequest(
			  			symbolic_file_names,
			  			start_time, end_time,
			  			desired_file_name,
			  			inclusion_flag);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

write_sasf_tail		: start_end_time list_of_strings  TOK_INTEGER
			  {
			  Cstring start_time($1->theData);
			  Cstring end_time($1->expressions[0]->theData);
			  $$ = cmdExp(new cmdSys::cE("sasf_tail"));
			  cmdExp start(new cmdSys::cE($1->theData));
			  cmdExp end(new cmdSys::cE($1->expressions[0]->theData));

			  // element 0 is start time
			  $$->addExp(cmdExp(new cmdSys::cE(start_time)));
			  // element 1 is start time
			  $$->addExp(cmdExp(new cmdSys::cE(end_time)));

			  int number_of_files = 0;
			  if($2) {
				number_of_files = 1 + $2->expressions.size();
			  }
			  // element 2 is number of files
			  $$->addExp(cmdExp(new cmdSys::cE(number_of_files)));
			  if($2) {
				// elements 3 through 2 + <number of files> are the file names
				$$->addExp(cmdExp(new cmdSys::cE($2->theData)));
				for(int i = 0; i < $2->expressions.size(); i++) {
					$$->addExp(cmdExp(new cmdSys::cE($2->expressions[i]->theData)));
				}
			  }
			  int k = atoi(*$3->theData);
			  // element 3 + <number of files> is the inclusion flag
			  $$->addExp(cmdExp(new cmdSys::cE(k)));
			  }
			;

start_end_time_option   : start_end_time
			  {
			  $$ = $1;
			  }
			| null
			  {
			  $$ = $1;
			  }
			;

start_end_time		: stime etime
			  {
			  $$ = $1;
			  $$->addExp($2);
			  }
			;

stime			: TOK_TIME
			  {
			  $$ = $1;
			  }
			| TOK_SINGLE_QUOTED
			  {
			  /* NOTE: it's up to the higher-level processing code to note that
			   * in this case the time value is an expression, not a constant.
			   */
			  $$ = $1;
			  }
			;

etime			: TOK_TIME
			  {
			  $$ = $1;
			  }
			| TOK_SINGLE_QUOTED
			  {
			  /* NOTE: it's up to the higher-level processing code to note that
			   * in this case the time value is an expression, not a constant.
			   */
			  $$ = $1;
			  }
			;

cmd_save_file		: TOK_SAVE save_name save_exclude save_options
			  {
			  SAVE_FILErequest*	request;
			  apgen::FileType	theApgenFileType = apgen::FileType::FT_NONE;
			  Cstring		the_type_of_the_file , theFile;
			  tlist<alpha_string, optionNode> theOpts;

			  the_type_of_the_file = $2->expressions[0]->theData;
			  Cstring theFileTypeForPreferences = the_type_of_the_file;
			  if(the_type_of_the_file == "XML") {
				theFileTypeForPreferences = "APF";
			  }

			  // better be one of those five:
			  if(the_type_of_the_file == "AAF") {
				theApgenFileType = apgen::FileType::FT_AAF;
			  } else if(the_type_of_the_file == "APF") {
				theApgenFileType = apgen::FileType::FT_APF;
			  }
			  else if(the_type_of_the_file == "MIX") {
				theApgenFileType = apgen::FileType::FT_MIXED;
			  }
			  else if(the_type_of_the_file == "LAYOUT") {
				theApgenFileType = apgen::FileType::FT_LAYOUT;
			  }
			  else if(the_type_of_the_file == "XML") {
				theApgenFileType = apgen::FileType::FT_XML;
			  }

			  theFile = $2->theData;

			  if($4) { 	// options are specified. Expect 6 items.
			    Cstring				temp;
			    Action_request::save_option	theSaveOption;

			    for(int i = 0; i < $4->expressions.size(); i++) {
				cmdSys::Assignment* assign = dynamic_cast<cmdSys::Assignment*>($4->expressions[i].object());
				temp = assign->rhs->theData;
				removeQuotes(temp);
				if(temp == "NOTHING")
					theSaveOption = Action_request::INCLUDE_NOT_AT_ALL;
				else if(temp == "COMMENTS")
					theSaveOption = Action_request::INCLUDE_AS_COMMENTS;
				else if(temp == "CODE")
					theSaveOption = Action_request::INCLUDE_AS_CODE;
				else {
					Cstring err;
					err << "save file: invalid option value " << temp;
					zzerror((char*) *err);
					YYACCEPT;
				}
				theOpts << new optionNode(theFileTypeForPreferences
					+ " " + assign->lhs->theData, theSaveOption);
			    }
			  } else if(theApgenFileType == apgen::FileType::FT_APF) {
			    theOpts << new optionNode( "APF GLOBALS",	Action_request::INCLUDE_NOT_AT_ALL);
			    theOpts << new optionNode( "APF EPOCHS",	Action_request::INCLUDE_AS_CODE);
			    theOpts << new optionNode( "APF TIME_SYSTEMS",Action_request::INCLUDE_AS_CODE);
			    theOpts << new optionNode( "APF LEGENDS",	Action_request::INCLUDE_AS_COMMENTS);
			    theOpts << new optionNode( "APF WIN_SIZE",	Action_request::INCLUDE_NOT_AT_ALL );
			    theOpts << new optionNode( "APF TIME_PARAMS",Action_request::INCLUDE_NOT_AT_ALL );
			  } else if( theApgenFileType == apgen::FileType::FT_AAF ) {

			    // not quite right; use UI_popups.C for inspiration:

			    theOpts << new optionNode( "AAF GLOBALS",	Action_request::INCLUDE_NOT_AT_ALL );
			    theOpts << new optionNode( "AAF EPOCHS",	Action_request::INCLUDE_AS_CODE );
			    theOpts << new optionNode( "AAF TIME_SYSTEMS",Action_request::INCLUDE_AS_CODE );
			    theOpts << new optionNode( "AAF LEGENDS",	Action_request::INCLUDE_AS_COMMENTS );
			    theOpts << new optionNode( "AAF WIN_SIZE",	Action_request::INCLUDE_NOT_AT_ALL );
			    theOpts << new optionNode( "AAF TIME_PARAMS",Action_request::INCLUDE_NOT_AT_ALL );
			  } else if( theApgenFileType == apgen::FileType::FT_MIXED ) {
			    theOpts << new optionNode( "MIX GLOBALS",	Action_request::INCLUDE_NOT_AT_ALL );
			    theOpts << new optionNode( "MIX EPOCHS",	Action_request::INCLUDE_AS_CODE );
			    theOpts << new optionNode( "MIX TIME_SYSTEMS",Action_request::INCLUDE_AS_CODE );
			    theOpts << new optionNode( "MIX LEGENDS",	Action_request::INCLUDE_AS_COMMENTS );
			    theOpts << new optionNode( "MIX WIN_SIZE",	Action_request::INCLUDE_NOT_AT_ALL );
			    theOpts << new optionNode( "MIX TIME_PARAMS",Action_request::INCLUDE_NOT_AT_ALL );
			  }
			  stringslist excluded;
			  if($3) {
			    for(int i = 0; i < $3->expressions.size(); i++) {
			    	excluded << new emptySymbol($3->expressions[i]->theData);
			    }
			  }

			  request = new SAVE_FILErequest(theFile, theApgenFileType, excluded, theOpts);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

save_name	       :  TOK_MIX filename
			  {
			  $$ = $2;
			  $2->addExp($1);
			  }
			| TOK_AAF filename
			  {
			  $$ = $2;
			  $2->addExp($1);
			  }
			| TOK_APF filename
			  {
			  $$ = $2;
			  $2->addExp($1);
			  }
			| TOK_LAYOUT filename
			  {
			  $$ = $2;
			  $2->addExp($1);
			  }
			| TOK_XML filename
			  {
			  $$ = $2;
			  $2->addExp($1);
			  }
			;

save_exclude		: TOK_EXCLUDE file_list
			  {
			  $$ = $2;
			  }
			| TOK_TAG TOK_STRINGVAL
			  {
			  // for layout files
			  $$ = $1;
			  $$->addExp($2);
			  }
			| null
			;

file_list		: file_list TOK_COMMA a_symbol
			  {
			  $$ = $1;
			  $$->addExp($3);
			  }
			| a_symbol
			  {
			  $$ = cmdExp(new cmdSys::cE("file_list"));
			  $$->addExp($1);
			  }
			;

save_options	     : '[' assign_list ']'
			  {
			  $$ = $2 ;
			  }
			| null
			;

at_symbol		: TOK_IDHOLDER
			  {

			  //
			  // this is a symbol of the form @1 or @2a1bccc
			  //
			  $$ = $1;
			  }
			;

resname			: a_symbol
			  {
			  $$ = $1 ;
			  }
			| TOK_ARRAYVAL
			  {
			  $$ = $1 ;
			  }
			;

filename		: TOK_FILENAME
			  {
			  Cstring theText = addQuotes(wwtext);
			  $$ = cmdExp(new cmdSys::cE(theText));
			  }
			| TOK_SINGLE_QUOTED
			  {
			  Cstring theText = wwtext;
			  $$ = cmdExp(new cmdSys::cE(theText));
			  }
			| TOK_SYMBOL
			  {
			  Cstring theText = addQuotes(wwtext);
			  $$ = cmdExp(new cmdSys::cE(theText));
			  }
			| TOK_STRINGVAL
			  {
			  Cstring temp = $1->theData;
			  $$ = cmdExp(new cmdSys::cE(temp));
			  }
			;

cmd_consolidate		: TOK_CONSOLIDATE list_and_name
			  {
			  $$ = $1;
			  $$->addExp($2);
			  }
			;

list_and_name		: file_list filename
			  {
			  $$ = $2;
			  $$->addExp($1);
			  }
			;

cmd_save_partial_file   : TOK_SAVE_PARTIALFILE list_and_name
			  {
			  SAVE_PARTIAL_FILErequest*	request;
			  stringtlist			sorted;

			  cmdExp	files = $2->expressions[0];
			  for(int i = 0; i < files->expressions.size(); i++) {
				Cstring theName(files->expressions[i]->theData);
			  	removeQuotes(theName);
				sorted << new emptySymbol(theName);
			  }
			  Cstring outputFileName($2->theData);
			  removeQuotes(outputFileName);
			  request = new SAVE_PARTIAL_FILErequest(sorted, outputFileName);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_schedule_activity	: sched_start sched_rest
			  {
			  SCHEDULE_ACTrequest	*request;
			  bool is_it_all = $2->theData == "ALL" || $2->theData == "all";
			  bool incremental = $1->expressions.size() == 1;
			  stringslist q;
			  if(is_it_all) {
				request = new SCHEDULE_ACTrequest(q, is_it_all, incremental);
			  } else {
			  	stringslist	L;
				L << new emptySymbol($2->theData);
				for(int i = 0; i < $2->expressions.size(); i++) {
					L << new emptySymbol($2->expressions[i]->theData);
				}
				request = new SCHEDULE_ACTrequest(L, is_it_all, incremental);
			  }
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

sched_start		: TOK_SCHEDULE TOK_INCREMENTAL
			  {
			  $$ = $1;
			  $$->addExp($2);
			  }
			| TOK_SCHEDULE
			  {
			  $$ = $1;
			  }
			;

sched_rest		: non_empty_act_list 
			  {
			  $$ = $1 ;
			  }
			| TOK_SYMBOL
			  {
			  /* the symbol better be 'ALL'... */
			  $$ = $1 ;
			  }
			;

cmd_unschedule_activity	: TOK_UNSCHEDULE sched_rest
			  {
			  // process_unscheduling( $2 ) ;
			  }
			;

cmd_write_tol		: TOK_TOL filename  start_end_time_option  tol_para
			  {
			  Cstring format;
			  if($4->theData != "DEFAULT_FORMAT") {
				format = $4->theData;
			  }
		
			  cmdExp filter_exp;
			  if($4->theData != "DEFAULT_FILTER") {
			  	filter_exp = $4->expressions[0];
			  }

			  Cstring start_time;
			  Cstring end_time;
			  if($3) {
			  	start_time = $3->theData;
			  	end_time = $3->expressions[0]->theData;
			  }
			  Cstring filename = $2->theData;

			  //
			  // Do not accept symbols for the time being; the
			  // action request will attempt to parse it, with
			  // disastrous results.
			  //

			  if(!filename.length()) {
				filename = "<empty_file_name>";
			  } else if(filename[0] != '"') {
				filename = addQuotes(filename);
			  }
			  stringslist filters;
			  if(filter_exp) {
				filters << new emptySymbol(filter_exp->theData);
			  	for(int i = 0; i < filter_exp->expressions.size(); i++) {
					filters << new emptySymbol(
						filter_exp->expressions[i]->theData);
				}
			  }
			  WRITE_TOLrequest* request = new WRITE_TOLrequest(
				filename,
				start_time,
				end_time,
			  	filters,
				format);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_write_xmltol	: TOK_TOL TOK_XML filename  start_end_time_option TOK_FILTER xmltol_filter
			  xmltol_schema xmltol_timesystem xmltol_everything_else
			  {
			  /*
			   * NOTE: remember that formerly constant args can now be expressions. Make
			   *	 sure downstream code can handle this case.
			   */

			  Cstring		file_name_wanted,
						start_time,
						end_time,
						filter,
						schema,
						timesystem,
						all_acts_visible;
			  WRITE_XMLTOLrequest*	request;
			  vector<string>	v;

			  if($9) {
			  	all_acts_visible = $9->theData;
				for(int i = 0; i < $9->expressions.size(); i++) {
					v.push_back(*$9->expressions[i]->theData);
				}
			  }
			  if($8) {
				timesystem = $8->theData;
			  }
			  if($7) {
				schema = $7->theData;
			  }
			  if($6) {
				filter = $6->theData;
			  }
			  if($4 && $4->theData.length()) {
			  	start_time = $4->theData;
			  	end_time = $4->expressions[0]->theData;
			  } else {
				start_time = "DEFAULT";
				end_time = "DEFAULT";
			  }
			  file_name_wanted = $3->theData;

			  /* V31.0 change: all four arguments can now be expressions; the telltale sign is that
			   * an expression is enclosed in a single pair of single quotes or backquotes.
			   *
			   * NOTE: if the string is double-quoted, that will be taken out when we evaluate the string.
			   */
			  request = new WRITE_XMLTOLrequest(
				file_name_wanted,
				start_time,
				end_time,
				filter,
				schema,
				timesystem,
				all_acts_visible == "ALL_VISIBLE",
				v);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_freeze		: TOK_FREEZE non_empty_res_list
			  {
			  vector<string> resources;

			  resources.push_back(*$2->theData);
			  for(int i = 0; i < $2->expressions.size(); i++) {
				resources.push_back(*$2->expressions[i]->theData);
			  }
			  FREEZErequest* request = new FREEZErequest(resources);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_unfreeze		: TOK_UNFREEZE non_empty_res_list
			  {
			  vector<string> resources;

			  resources.push_back(*$2->theData);
			  for(int i = 0; i < $2->expressions.size(); i++) {
				resources.push_back(*$2->expressions[i]->theData);
			  }
			  UNFREEZErequest* request = new UNFREEZErequest(resources);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

tol_para		: tol_filter tol_format
			  {
			  if($2) {
				$$ = $2;
			  } else {
				$$ = cmdExp(new cmdSys::cE("DEFAULT_FORMAT"));
			  }
			  if($1) {
			  	$$->addExp($1);
			  } else {
				$$ = cmdExp(new cmdSys::cE("DEFAULT_FILTER"));
			  }
			  }
			;

tol_filter		: TOK_FILTER nonempty_list_of_strings
			  {
			  Cstring	theActualFilter($2->theData);

			  removeQuotes(theActualFilter);
			  if(theActualFilter == "DEFAULT") {
				theActualFilter.undefine();
			  }
			  $$ = cmdExp(new cmdSys::cE(theActualFilter));
			  for(int i = 0; i < $2->expressions.size(); i++) {
				Cstring tmp = $2->expressions[i]->theData;
				removeQuotes(tmp);
				cmdExp ce(new cmdSys::cE(tmp));
				$$->addExp(ce);
			  }
			  }
			| null
			;

xmltol_filter		: TOK_STRINGVAL
			  {
			  Cstring filter($1->theData);
			  removeQuotes(filter);
			  if(filter == "DEFAULT") {
				filter.undefine();
			  }
			  $$ = cmdExp(new cmdSys::cE(filter));
			  }
			| null
			  {
			  $$ = $1;
			  }
			;

xmltol_schema		: TOK_SCHEMA TOK_STRINGVAL
			  {
			  {
			  Cstring schema($2->theData);
			  removeQuotes(schema);
			  if(schema == "DEFAULT") {
				schema.undefine();
			  }
			  $$ = cmdExp(new cmdSys::cE(schema));
			  }
			  }
			| null
			  {
			  $$ = $1;
			  }
			;

xmltol_timesystem	: TOK_TIMESYSTEM TOK_STRINGVAL
			  {
			  Cstring timesys($2->theData);
			  removeQuotes(timesys);
			  if(timesys == "DEFAULT") {
				timesys.undefine();
			  }
			  $$ = cmdExp(new cmdSys::cE(timesys));
			  }
			| null
			;

xmltol_everything_else	: TOK_ALLACTSVISIBLE
			  {
			  // process_xmltol_allactsvisible(1);
			  $$ = cmdExp(new cmdSys::cE("ALL_VISIBLE"));
			  }
			| TOK_RESOURCE non_empty_res_list
			  {
			  $$ = cmdExp(new cmdSys::cE("RESOURCES_ONLY"));
			  cmdExp first_exp(new cmdSys::cE($2->theData));
			  $$->addExp(first_exp);
			  for(int i = 0; i < $2->expressions.size(); i++) {
				$$->addExp($2->expressions[i]);
			  }
			  }
			| null
			;

tol_format		: TOK_STRINGVAL
			  {
			  Cstring theActualFormat($1->theData);
			  removeQuotes( theActualFormat );
			  theActualFormat = "FORMAT " / theActualFormat;
			  $$ = cmdExp(new cmdSys::cE(theActualFormat));
			  }
			| null
			;

cmd_print		: TOK_PRINT
			  {
			  // process_print();
			  }
			;

cmd_purge		: TOK_PURGE TOK_STRINGVAL
			  {
			  // process_purge(1);
			  }
			| TOK_PURGE
			  {
			  // process_purge(0);
			  }
			;

cmd_quit		: TOK_QUIT
			  {
			  QUITrequest* request = new QUITrequest();
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			| TOK_QUIT TOK_INTEGER
			  {
			  int k = atoi(*$2->theData);

			  //
			  // Fast quit
			  //
			  QUITrequest* request = new QUITrequest(k > 0);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

non_empty_res_list	: non_empty_res_list TOK_COMMA one_res
			  {
			  $$ = $1;
			  $$->addExp($3);
			  }
			| one_res
			  {
			  $$ = $1;
			  }
			;

one_res			: TOK_ARRAYVAL
			{
			  $$ = $1;
			}
			| TOK_SYMBOL
			{
			  $$ = $1;
			}
			;


optional_act_list	: non_empty_act_list
			  {
			  $$ = $1;
			  }
			| null
			  {
			  $$ = $1;
			  }
			;

non_empty_act_list	: TOK_ID one_or_more_act
			  {
			  $$ = $2;
			  }
			;

one_or_more_act         : one_or_more_act TOK_COMMA an_id_symbol
			  {
			  $$ = $1;
			  $$->addExp($3);
			  }
			| an_id_symbol
			  {
			  $$ = $1;
			  }
			;

an_id_symbol		: a_symbol
			  {
			  $$ = $1;
			  }
			| TOK_IDHOLDER
			  {
			  $$ = $1;
			  }
			;

cmd_cut_activity	: TOK_CUT non_empty_act_list
			  {
			  stringslist	L;
			  L << new emptySymbol($2->theData);
			  for(int i = 0; i < $2->expressions.size(); i++) {
			  	L << new emptySymbol($2->expressions[i]->theData);
			  }
			  CUT_ACTIVITYrequest* request = new CUT_ACTIVITYrequest(L);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_delete_activity	: TOK_DELETE non_empty_act_list
			  {
			  stringslist	L;
			  L << new emptySymbol($2->theData);
			  for(int i = 0; i < $2->expressions.size(); i++) {
			  	L << new emptySymbol($2->expressions[i]->theData);
			  }
			  DELETE_ACTIVITYrequest* request = new DELETE_ACTIVITYrequest(L);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

edit1			: TOK_EDIT act_id edit_carry
			  {
			  $$ = cmdExp(new cmdSys::EditActivity($2, $3));
			  }
			;

cmd_edit_global		: TOK_EDIT_GLOBAL assign 
			  {
			  $$ = $1;

			  cmdSys::Assignment* assign
					= dynamic_cast<cmdSys::Assignment*>($2.object());
			  assert(assign);
			  Cstring key = assign->lhs->theData;
			  Cstring value = assign->rhs->theData;
			  EDIT_GLOBALrequest* request =
				new EDIT_GLOBALrequest(key, value);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;

			  //
			  // Now we are ready to issue an EDIT_GLOBAL action_request
			  //
			  }
			;

cmd_edit_activity       : edit1 '<' assign_list '>'
			  {
			  $$ = $1;
			  cmdSys::EditActivity* ea = dynamic_cast<cmdSys::EditActivity*>($1.object());
			  pairslist	symbol_list;
			  int		carry = 1;

			  if(ea->carry_flag) {
				carry = atoi(*ea->carry_flag->theData);
			  }

			  assert($3->theData == "Assignment");
			  vector<cmdExp>& symbols = $3->expressions;
			  for(int i = 0; i < symbols.size(); i++) {
				cmdSys::Assignment* assign
					= dynamic_cast<cmdSys::Assignment*>(symbols[i].object());
				assert(assign);
				Cstring key = assign->lhs->theData;
				Cstring value = assign->rhs->theData;
				symbol_list << new symNode(key, value);
			  }
			  EDIT_ACTIVITYrequest* request =
				new EDIT_ACTIVITYrequest(
						symbol_list,
						carry,
						ea->act_id->theData);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			| edit1
			  {
			  $$ = $1;
			  }
			| edit1 '(' list_of_strings ')'
			  {
			  /* Not sure this was ever used */
			  $$ = $1;
			  $$->addExp($3);
			  }
			;

act_id			: TOK_ID a_symbol
			  {
			  $$ = $2;
			  }
			| TOK_ID at_symbol
			  {
			  $$ = $2;
			  }
			;

act_name		: TOK_NAME a_symbol
			  {
			  $$ = $2;
			  }
			| null
			;

edit_carry		: TOK_NOCARRY
			  {
			  $$ = $1;
			  }
			| null
			;

assign_list		: assign_list TOK_COMMA assign
			  {
			  $$ = $1;
			  $$->addExp($3);
			  }
			| assign
			  {
			  $$ = cmdExp(new cmdSys::cE($1->theData));
			  $$->addExp($1);
			  }
			;

assign			: assign_name TOK_EQUAL assign_value
			  {
			  $$ = cmdExp(new cmdSys::Assignment($1, $3));
			  }
			;

assign_name		: a_symbol
			  {
			  $$ = $1;
			  }
			;

assign_value		: TOK_STRINGVAL
			  {
			  Cstring temp = $1->theData;
			  removeQuotes(temp);
			  $$ = cmdExp(new cmdSys::cE(temp));
			  }
			;

list_of_strings		: a_single_string_or_null
			  {
			  $$ = $1;
			  }
			| list_of_strings TOK_COMMA a_single_string_or_null
			  {
			  $$ = $1;
			  $$->addExp($3);
			  }
			;

nonempty_list_of_strings: a_non_null_string
			  {
			  $$ = $1;
			  }
			| list_of_strings TOK_COMMA a_non_null_string
			  {
			  $$ = $1;
			  $$->addExp($3);
			  }
			;

a_single_string_or_null	: a_non_null_string
			  {
			  $$ = $1;
			  }
			| null
			;

a_non_null_string	: TOK_STRINGVAL
			  {
			  $$ = $1;
			  // process_edit_assign_value() ;
			  }
			;

cmd_copy_activity       : TOK_COPY non_empty_act_list
			  {
			  stringslist	L;
			  L << new emptySymbol($2->theData);
			  for(int i = 0; i < $2->expressions.size(); i++) {
			  	L << new emptySymbol($2->expressions[i]->theData);
			  }
			  COPY_ACTIVITYrequest	*request;

			  request = new COPY_ACTIVITYrequest(L);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  CMD_exec::cmd_subsystem().clear_lists();
			  }
			;

cmd_group_activity	: TOK_GROUPACTIVITIES parent non_empty_act_list
			  { /*
				 * <group> PARENT/CHAMELEON <the name of the request> ID <list of children>
				 *
				if( $2 == 1 ) {
					process_group_activity( 1 ) ; }
				if( $2 == 2 ) {
					process_group_activity( 2 ) ; }
			     */
			  }
			|
			TOK_GROUPACTIVITIES act_id non_empty_act_list
				{ /*
				 * <group> ID <the unique id of the parent instance> ID <list of children>
				 */
				// process_group_activity( 3 ) ;
				}
			| TOK_GROUPACTIVITIES templ parent non_empty_act_list
			  { /*
				 <group> <template> PARENT <the name of the request> ID <list of children>

				if( $3 == 1 ) {
					process_group_activity( 4 ) ; }
				if( $3 == 2 ) {
					process_group_activity( 5 ) ; }
			     */
			  }
			;

parent			: TOK_PARENT a_symbol
			  {
			  $$ = $1;
			  $$->addExp($2);
			  }
			| TOK_CHAMELEON a_symbol
			  {
			  $$ = $1 ;
			  $$->addExp($2);
			  }
			;

templ			: a_symbol
			  {
			  $$ = $1;
			  }
			;

cmd_xcmd		: TOK_XCMD a_symbol optional_act_list
			  {
			  Cstring	theCommand = $2->theData;
			  XCMDrequest*	request;
			  stringslist	theActivities;

			  $$ = $1;
			  if($3) {
				theActivities << new emptySymbol($3->theData);
				for(int i = 0; i < $3->expressions.size(); i++) {
					theActivities << new emptySymbol($3->expressions[i]->theData);
				}
			  }
			  request = new XCMDrequest(theActivities, theCommand);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_unfreeze_activity	: TOK_UNFREEZERESOURCES
			  {
			  UNFREEZE_RESOURCESrequest*	request = new UNFREEZE_RESOURCESrequest;
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_ungroup_activity	: TOK_UNGROUPACTIVITIES non_empty_act_list
			{
			  stringslist	L;
			  L << new emptySymbol($2->theData);
			  for(int i = 0; i < $2->expressions.size(); i++) {
			  	L << new emptySymbol($2->expressions[i]->theData);
			  }
			  UNGROUP_ACTIVITIESrequest*	request;

			  request = new UNGROUP_ACTIVITIESrequest(L);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_move_activity       : TOK_MOVEACTIVITY move_act_para
			  {
			  // process_move_activity() ;
			  }
			;

move_act_para		: non_empty_act_list  where
			;

where			: where_only 
			| where_only legend
			;

where_only		: to_time
			  {
			  // process_to_time();
			  }
			| by_duration
			  {
			  // process_by_duration();
			  }
			;

by_duration		: TOK_BY TOK_DURATION 
			  {
			  // grab_and_push_text();
			  }
			;

period_duration         : TOK_PERIOD TOK_DURATION
			  {
			  // grab_and_push_text();
			  }
			;

times_integer  	        : TOK_TIMES TOK_INTEGER
	                  {
			  // grab_and_push_text();
			  }
                        ;

to_time			: TOK_TO TOK_TIME 
			  {
			  $$ = $2;
			  }
			| TOK_TO TOK_STRINGVAL
			  {
			  Cstring temp = $2->theData;
			  removeQuotes(temp);
			  $$ = cmdExp(new cmdSys::cE(temp));
			  }
			;

cmd_paste_activity      : TOK_PASTE to_time legend
			  {

			  //
			  // The following used to be legal, but TOK_ID was
			  // ignored:
			  //
			  //   TOK_PASTE TOK_ID to_time legend

			  Cstring			act_time, act_legend;
			  PASTE_ACTIVITYrequest*	request;

			  act_legend = $3->theData;
			  act_time = $2->theData;

			  if(act_time[0] == '\"') {
				Cstring		errmsg;

				// need to extract an epoch
				removeQuotes(act_time);
			  }

			  stringslist l;
			  request = new PASTE_ACTIVITYrequest(l, act_time, act_legend);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_pause		: TOK_PAUSE
			{
			  // process_pause( 1 ) ;
			  }
			| TOK_PAUSE TOK_STRINGVAL
			{
			  // process_pause( 2 ) ;
			  }
			;

cmd_new_activity	: TOK_NEW act_type  act_name  act_id new_activity_details
			  {
			  /* NOTE: "TOK_NEW" stands for "NEWACTIVITY" */
			  NEW_ACTIVITYrequest* request;
			  Cstring act_type = $2->theData;
			  Cstring act_name = $3->theData;
			  Cstring act_id = $4->theData;
			  Cstring act_time = $5->theData;
			  Cstring act_legend = $5->expressions[0]->expressions[0]->theData;

			  //
			  // act_time may be a constant time OR an Epoch-relative string.
			  // If it is epoch-relative, we want the activity instance to make
			  // use of the string; converting to a time right now would lose that
			  // information.
			  //

			  if($5->expressions.size() > 1) {
				pairslist		symbol_list;
				emptySymbol		*string_node1, *string_node2;
				Cstring			temp;

				//
				// assign_list contains a vector of cmdSys::Assignment cE's
				//

				assert($5->expressions[1]->theData == "assign_list");
				vector<cmdExp>& symbols = $5->expressions[1]->expressions[0]->expressions;
				for(int i = 0; i < symbols.size(); i++) {
					cmdSys::Assignment* assign
						= dynamic_cast<cmdSys::Assignment*>(symbols[i].object());
					assert(assign);
					Cstring key = assign->lhs->theData;
					Cstring value = assign->rhs->theData;
					symbol_list << new symNode(key, value);
				}
			  	request = new NEW_ACTIVITYrequest(
							act_type,
							act_name,
							act_id,
							act_time,
							act_legend,
							symbol_list);
			  } else {
				request = new NEW_ACTIVITYrequest(
							act_type,
							act_name,
							act_id,
							act_time,
							act_legend);
			  }
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_new_activities      : TOK_NEWACTIVITIES act_type to_time period_duration times_integer legend
	                {
			// process_new_activities();
			}
			;

new_activity_details	: time_and_legend
			  {

			  //
			  // time_and_legend has time in theData; its expressions[0] is a cE whose
			  // theData is "legend" and whose expressions[0] is the legend
			  //

			  $$ = $1;
			  }
			| time_and_legend '<' assign_list '>'
			  {
			  $$ = $1;
			  cmdExp list_exp(new cmdSys::cE("assign_list"));
			  list_exp->addExp($3);

			  //
			  // time_and_legend has time in theData; its expressions[0] is a cE whose
			  // theData is "legend" and whose expressions[0] is the legend. Here we
			  // are adding expressions[1], a cE whose theData is "assign_list" and
			  // whose expressions[0] is a cE whose theData is the first assignment
			  // and the remaining assignments are in its expressions list.
			  //

			  $$->addExp(list_exp);
			  }
			;

time_and_legend		: to_time legend
			  {
			  $$ = $1;
			  cmdExp legend_exp(new cmdSys::cE("legend"));
			  legend_exp->addExp($2);
			  $$->addExp(legend_exp);
			  }
			| to_time
			  {
			  $$ = $1;
			  cmdExp legend_exp(new cmdSys::cE("legend"));
			  cmdExp generic_acts(new cmdSys::cE("Generic Activities"));
			  legend_exp->addExp(generic_acts);
			  $$->addExp(legend_exp);
			  }
			;

act_type		: a_symbol
			  {
			  $$ = $1;
			  }
			;

legend			: TOK_LEGEND TOK_STRINGVAL
			  {
			  Cstring temp = $2->theData;
			  removeQuotes(temp);
			  $$ = cmdExp(new cmdSys::cE(temp));
			  }
			;

cmd_abstract_activity	: TOK_ABSTRACT non_empty_act_list opt_full
			  {
			  ABSTRACT_ACTIVITYrequest* request;
			  if($2) {
				stringslist	L;
				L << new emptySymbol($2->theData);
				for(int i = 0; i < $2->expressions.size(); i++) {
					L << new emptySymbol($2->expressions[i]->theData);
				}
				if($3) {
					request = new ABSTRACT_ACTIVITYrequest(
						L,		// list of IDs
						true		// full
						);
				} else {
					request = new ABSTRACT_ACTIVITYrequest(
						L,		// list of IDs
						false		// full
						);
				}
				(*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			  }
			;

cmd_abstract_all	: TOK_ABSTRACTALL
			  {
			  ABSTRACT_ALLrequest* request;
			  request = new ABSTRACT_ALLrequest();
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_abstract_all_quiet	: TOK_ABSTRACTALL_QUIET
			  {
			  ABSTRACT_ALL_QUIETrequest* request;
			  request = new ABSTRACT_ALL_QUIETrequest();
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_drag_children	: TOK_DRAGCHILDREN TOK_INTEGER
			{
			  // process_drag_children() ;
			  }
			;

cmd_detail_activity	: TOK_DETAIL non_empty_act_list opt_full
			  {
			  // process_detail_activity( 0 ) ;
			  DETAIL_ACTIVITYrequest* request;
			  stringslist	L;
			  L << new emptySymbol($2->theData);
			  for(int i = 0; i < $2->expressions.size(); i++) {
				L << new emptySymbol($2->expressions[i]->theData);
			  }
			  if($3) {
				request = new DETAIL_ACTIVITYrequest(
					L,		// list of IDs
					false,		// redetail?
					0,		// which resolution (OBSOLETE)
					0,		// secret message?
					true		// full
					);
			  } else {
				request = new DETAIL_ACTIVITYrequest(
					L,		// list of IDs
					false,		// redetail?
					0		// which resolution (OBSOLETE)
					);
			  }
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			| TOK_DETAIL non_empty_act_list TOK_SELECT TOK_INTEGER opt_full
			  {
			  // for resolution - obsolete
			  }
			;

opt_full		: TOK_FULLY
			  {
			  $$ = $1;
			  $$->addExp($1);
			  }
			| null
			  {
			  $$ = $1;
			  }
			;

cmd_detail_all	  : TOK_DETAILALL
			  {
			  DETAIL_ALLrequest* request;
			  request = new DETAIL_ALLrequest(false);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_detail_all_quiet	  : TOK_DETAILALL_QUIET
			  {
			  DETAIL_ALL_QUIETrequest* request;
			  request = new DETAIL_ALL_QUIETrequest(false);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_redetail_activity   : TOK_REDETAIL non_empty_act_list
			  {
			  DETAIL_ACTIVITYrequest* request;
			  stringslist	L;
			  L << new emptySymbol($2->theData);
			  for(int i = 0; i < $2->expressions.size(); i++) {
				L << new emptySymbol($2->expressions[i]->theData);
			  }
			  request = new DETAIL_ACTIVITYrequest(L, true, 0);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_redetail_all	: TOK_REDETAILALL
			  {
			  DETAIL_ALLrequest* request;
			  request = new DETAIL_ALLrequest(true);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_redetail_all_quiet	: TOK_REDETAILALL_QUIET
			  {
			  DETAIL_ALL_QUIETrequest* request;
			  request = new DETAIL_ALL_QUIETrequest(true);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_regenchildren	: TOK_REGENCHILDREN non_empty_act_list 
			  {
			  REGEN_CHILDRENrequest	*request;

			  stringslist	L;
			  L << new emptySymbol($2->theData);
			  for(int i = 0; i < $2->expressions.size(); i++) {
			  	L << new emptySymbol($2->expressions[i]->theData);
			  }
			  request = new REGEN_CHILDRENrequest(L, 0);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			| TOK_REGENCHILDREN TOK_SYMBOL
			  {
			  REGEN_CHILDRENrequest	*request;
			  /* the symbol better be 'ALL'... */
			  stringslist	L;
			  request = new REGEN_CHILDRENrequest(L, 1);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_add_recipient	: TOK_ADDRECIPIENT resource_spec
                          {
			  //process_add_recipient(); 
			  }
			;

cmd_add_resource	: TOK_ADDRESOURCE resource_spec resource_spec
			  {
			  stringslist L;
			  ADD_RESOURCErequest* request;
			  L << new emptySymbol($2->theData);
			  L << new emptySymbol($3->theData);
			  request = new ADD_RESOURCErequest(L);
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

resource_spec		: a_symbol
			  {
			  $$ = $1;
			  }
			| TOK_ARRAYVAL
			  {
			  $$ = $1;
			  }
			;

cmd_remove_resource     : TOK_REMOVERESOURCE TOK_INTEGER
			  {	/* the integer is supposed to be 0; the old rule said 'null'
				instead of 0, but that's ambiguous with the new definition of a_symbol */
				// process_remove_resource();
			  }
			| TOK_REMOVERESOURCE a_symbol
			  {
			  // process_remove_resource_name_spec();
			  }
			;

cmd_remodel		: TOK_REMODEL optional_time
			  {
			  REMODELrequest* request;
        		  if($2 && !strcasecmp(*$2->theData, "until")) {
        			// until <time>
        		  	CTime_base	T($2->expressions[0]->theData);
        			request = new REMODELrequest(true, T, false);
        		  } else if($2 && !strcasecmp(*$2->theData, "from")) {
        			// from start
			  	CTime_base	T($2->expressions[0]->theData);
				request = new REMODELrequest(false, T, true);
			  } else {
				CTime_base	T;
				request = new REMODELrequest(false, T, false);
			  }
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;
optional_time		: TOK_UNTIL TOK_TIME
	       		  {
			  $$ = $1;
			  $$->addExp($2);
			  }
			| TOK_FROM TOK_START
			  {
			  $$ = $1;
			  $$->addExp($2);
			  }
			| null
			;

cmd_delete_all_desc	: TOK_DELETEALLDESCENDANTS
			  {
			  DELETE_ALL_DESCENDANTSrequest*	request;

			  request = new DELETE_ALL_DESCENDANTSrequest();
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_new_act_display     : TOK_NEWACTDISPLAY
			  {
			  NEW_ACT_DISPLAYrequest *request;
			  request = new NEW_ACT_DISPLAYrequest(
						CTime_base(0, 0, false),
						CTime_base(0, 0, true));
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_new_res_display     : TOK_NEWRESDISPLAY TOK_INTEGER
			  {
			  {
			  NEW_RES_DISPLAYrequest*	request;
			  request = new NEW_RES_DISPLAYrequest();
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;
			  }
			| TOK_NEWRESDISPLAY disp_list
			  {
			  NEW_RES_DISPLAYrequest*	request;
			  request = new NEW_RES_DISPLAYrequest();
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

disp_list		: disp_list TOK_COMMA a_symbol
			  {
			  $$ = $1;
			  $$->addExp($3);
			  }
			| a_symbol
			  {
			  $$ = $1;
			  }
			;

cmd_new_legend		: TOK_NEWLEGEND a_symbol
			  {
			  int			height;
			  NEW_LEGENDrequest*	request;
			  Cstring		theLegendName = $2->theData;
			  height = ACTVERUNIT;
			  request = new NEW_LEGENDrequest( theLegendName , height );
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			| TOK_NEWLEGEND TOK_STRINGVAL TOK_INTEGER
			  {
			  int			height;
			  NEW_LEGENDrequest*	request;
			  Cstring		theLegendName;
			  Cstring temp = $2->theData;
			  removeQuotes(temp);
			  theLegendName = temp;
			  height = atoi(*$3->theData);
			  request = new NEW_LEGENDrequest( theLegendName , height );
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			| TOK_NEWLEGEND a_symbol TOK_INTEGER
			  {
			  int			height;
			  NEW_LEGENDrequest*	request;
			  Cstring		theLegendName = $2->theData;
			  height = atoi(*$3->theData);
			  request = new NEW_LEGENDrequest( theLegendName , height );
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			| TOK_NEWLEGEND TOK_INTEGER TOK_INTEGER
			  {
			  int			height;
			  NEW_LEGENDrequest*	request;
			  Cstring		theLegendName = $2->theData;
			  height = atoi(*$3->theData);
			  request = new NEW_LEGENDrequest( theLegendName , height );
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_debug		: TOK_DEBUG a_symbol
			  {
			  // process_debug();
			  }
			;


cmd_delete_legend       : TOK_DELETELEGEND a_symbol
			  {
			  // process_delete_legend();
			  }
			;

cmd_move_legend	 	: TOK_MOVELEGEND a_symbol TOK_INTEGER
			  {
			  // process_move_legend();
			  }
			;

cmd_build		: TOK_BUILD
			  {
			  BUILDrequest*	request;
			  request = new BUILDrequest();
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_close_act_display   : TOK_CLOSEACTDISPLAY TOK_INTEGER
			  {
			  // process_close_act_display();
			  }
			| TOK_CLOSEACTDISPLAY close_act_disp_list
			  {
			  // process_close_act_display();
			  }
			;

close_act_disp_list	: close_act_disp_list TOK_COMMA a_symbol
			  {
			  // process_close_act_disp_name();
			  }
			| a_symbol
			  {
			  // process_close_act_disp_name();
			  }
			;

cmd_close_res_display   : TOK_CLOSERESDISPLAY TOK_INTEGER
			  {
			  // process_null_close_disp_name();
			  }
			| TOK_CLOSERESDISPLAY close_disp_list
			  {
			  // process_close_res_display();
			  }
			;

close_disp_list		: close_disp_list TOK_COMMA a_symbol
			  {
			  // process_close_disp_name();
			  }
			| a_symbol
			  {
			  // process_close_disp_name();
			  }
			;

/*
cmd_connect		: TOK_CONNECT a_symbol
			  {
			  // process_connect();
			  }
			;
 */

cmd_select_act_display  : TOK_ADSELECT a_symbol TOK_INTEGER
			  {
			  // process_ad_select() ;
			  }
			;

cmd_select_act_legend  : TOK_ACTLEGSELECT a_symbol TOK_INTEGER
			  {
			  // process_act_leg_select() ;
			  }
			;


cmd_zoom_prefix		: TOK_ZOOM TOK_FROM stime TOK_TO etime
			  {
			  $$ = cmdExp(new cmdSys::cE("NEWHORIZON"));
			  cmdExp start(new cmdSys::cE($3->theData));
			  cmdExp end(new cmdSys::cE($5->theData));
			  $$->addExp(start);
			  $$->addExp(end);
			  }
			| TOK_ZOOM a_symbol TOK_FROM stime TOK_TO etime
			  {
			  $$ = cmdExp(new cmdSys::cE("NEWHORIZON"));
			  cmdExp disp(new cmdSys::cE($2->theData));
			  cmdExp start(new cmdSys::cE($4->theData));
			  cmdExp end(new cmdSys::cE($6->theData));
			  $$->addExp(start);
			  $$->addExp(end);
			  $$->addExp(disp);
			  }
			| TOK_ZOOM TOK_FROM stime TOK_COMMA TOK_DURATION TOK_COMMA TOK_DURATION TOK_COMMA TOK_DURATION
			  {
			  // For compatibility with older versions of the command syntax
			  CTime_base dur($5->theData);
			  CTime_base S($3->theData);
			  CTime_base E;
			  E = S + dur;
			  $$ = cmdExp(new cmdSys::cE("NEWHORIZON"));
			  cmdExp start(new cmdSys::cE($3->theData));
			  cmdExp end(new cmdSys::cE(E.to_string()));
			  $$->addExp(start);
			  $$->addExp(end);
			  }
			| TOK_ZOOM a_symbol TOK_FROM stime TOK_COMMA TOK_DURATION TOK_COMMA TOK_DURATION TOK_COMMA TOK_DURATION
			  {
			  // For compatibility with older versions of the command syntax
			  CTime_base dur($6->theData);
			  CTime_base S($4->theData);
			  CTime_base E;
			  E = S + dur;
			  $$ = cmdExp(new cmdSys::cE("NEWHORIZON"));
			  cmdExp start(new cmdSys::cE($4->theData));
			  cmdExp end(new cmdSys::cE(E.to_string()));
			  cmdExp disp(new cmdSys::cE($2->theData));
			  $$->addExp(start);
			  $$->addExp(end);
			  $$->addExp(disp);
			  }
			;

cmd_zoom		: cmd_zoom_prefix
			  {
			  CTime_base S = CTime_base($1->expressions[0]->theData);
			  CTime_base T = CTime_base($1->expressions[1]->theData);
			  NEW_HORIZONrequest* request;
			  if($1->expressions.size() == 3) {
				Cstring disp = $1->expressions[2]->theData;
				request = new NEW_HORIZONrequest(disp, S, T);
			  } else {
				request = new NEW_HORIZONrequest(S, T);
			  }
			  (*CMD_exec::cmd_subsystem().cmdlist) << request;
			  }
			;

cmd_select_res_legend  : TOK_RESLEGSELECT a_symbol resname TOK_INTEGER
			  {
			  $$ = $3;
			  // process_res_leg_select( $3 ) ;
			  }
			;

cmd_res_scroll		: TOK_RESSCROLL TOK_INTEGER TOK_INTEGER
			{
			  // process_res_scroll( 1 ) ;
			  }
			| TOK_RESSCROLL TOK_TIME TOK_DURATION
			{
			  // process_res_scroll( 2 ) ;
			  }
			| TOK_RESSCROLL TOK_DURATION TOK_DURATION
			{
			  // process_res_scroll( 3 ) ;
			  }
			| TOK_RESSCROLL TOK_FLOAT TOK_FLOAT
			{
			  // process_res_scroll( 4 ) ;
			  }
			;

null			:
			  {
			  $$ = cmdExp();
			  }
			;

%%


/* BKL-11/97 updated cmd_new_res_display above */
/* BKL-12/97 updated cmd_close_res_display above */
/* BKL-1/98 updated cmd_add_resource above */
/* BKL-1-20-98 updated cmd_close_act_display above */

extern void Ccout(char*);
/* extern int readNextChar(); */

extern char*	wwtext;

extern int	zzlinenumber();

extern int	errorFlag;
extern void	add_to_error_string(const char*);

int zzerror(const char* s) {
        static char* buffer;
 
        buffer = (char*) malloc(strlen(s) + strlen(wwtext) + 120);
        sprintf(buffer, "CMD parser error: %s on line %d near \"%s\" #", s, zzlinenumber(), wwtext);
        add_to_error_string(buffer);
	// pEsys::stack().reset();
	errorFlag = 1;
        free(buffer);
	return 0;
}

int CMD_special_expression_token() {
	return TOK_SINGLE_EXPRESSION;
}

extern "C" {
int wwwrap () {
	return 1;
}
}
