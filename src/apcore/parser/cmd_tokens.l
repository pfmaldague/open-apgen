%{
#include <string.h>
#include "apcoreWaiter.H"
#include "cmd_grammar.H"
#include "CMD_exec.H"
#include "ActivityInstance.H"

#define register /**/

extern void	add_to_CMD_Tokens_list(const char*, int);
extern int	found_a_CMD_token(const char*);
extern void	add_to_ambiguous_tokens_list(const char*);
%}

%option yylineno

D				[0-9]
alpha				[a-zA-Z_$]
alphanumeric			[a-zA-Z0-9_]
specialchars			[_\-+/\\$\(\)~]
file_characters		 	[a-z$A-Z0-9_\-+./\\~]
any_char_excepts		[^\"\\]
quote				\\\"
backslash			\\\\
escaped_newline			\\\n
newline				\\n
singleindex			\[((\"({any_char_excepts}|{quote}|{backslash})+\")|({D}+))\]


%%

\r				;

{alpha}{alphanumeric}*	{
	int token_value = found_a_CMD_token((char*) yytext);

	if(token_value) {
		return token_value;
	}
	return TOK_SYMBOL;
	};

"//"[^\n]*$			;

":"	{
	return TOK_COLON ;
	} ;

","	{
	return TOK_COMMA ;
	} ;

"="	{
	return TOK_EQUAL ;
	} ;

\-{D}+					{

	return  TOK_INTEGER ;
	} ;

'[^']+'					{
	/* CHANGE FOR V31.0  - 3/9/2010 */
	return  TOK_SINGLE_QUOTED ;
	} ;

`[^']+`					{
	/* CHANGE FOR V31.0  - 3/9/2010 */
	return  TOK_SINGLE_QUOTED ;
	} ;

{D}*{alpha}{file_characters}*		{

	return  TOK_FILENAME ;
	} ;

{D}*{specialchars}{file_characters}*	{
	/* Watch out... a single parenthesis will match this: */
	if( yyleng == 1 && ( yytext[0] == '(' || yytext[0] == ')' ) ) {
		return yytext[0] ; }
	else if( (!strcmp( yytext , "-carry" ) ) || (!strcmp( yytext , "-CARRY" ) ) ) {
		return TOK_NOCARRY ; }
	return  TOK_FILENAME;
};

@{alphanumeric}+					{
	return TOK_IDHOLDER;
};

{D}*"."{D}*{alpha}{file_characters}*		{

	return TOK_FILENAME;
};

{D}*"."{D}*{specialchars}{file_characters}*	{

	return TOK_FILENAME;
};

{D}*"."{D}*"."{file_characters}*		{

	return  TOK_FILENAME;
};

{D}+"-"{D}+T{D}+":"{D}+":"{D}+"."{D}+	{

	return  TOK_TIME;
};

{D}+"-"{D}+T{D}+":"{D}+":"{D}+		{

	return  TOK_TIME;
};

\-?{D}+T{D}+":"{D}+":"{D}+"."{D}+	{

	return  TOK_DURATION;
};

\-?{D}+T{D}+":"{D}+":"{D}+		{

	return  TOK_DURATION;
};

\-?{D}+":"{D}+":"{D}+"."{D}+		{

	return TOK_DURATION;
};

\-?{D}+":"{D}+":"{D}+			{

	return  TOK_DURATION;
};

\-?{D}+":"{D}+"."{D}+			{

	return TOK_DURATION;
};

\-?{D}+":"{D}+				{

	return  TOK_DURATION;
};

{D}+					{

	return  TOK_INTEGER ;
	} ;


{D}+"."{D}*				{

	return  TOK_FLOAT ;
	} ;

[0-9]*\.[0-9]+([Ee][+\-]?[0-9]+)?	{

	return  TOK_FLOAT ;
	} ;

\"({any_char_excepts}|{quote}|{newline}|{escaped_newline}|{backslash})*\"  {
	return (TOK_STRINGVAL) ;
	} ;

{alpha}{alphanumeric}*{singleindex}+  {
	return (TOK_ARRAYVAL) ;
	} ;

[ \t\n]+			 ;

#[^\n]*\n	{
	}				;

.	{
	return yytext[0] ;
	} ;

%%

	/* function to work around the fact that YY_CURRENT_BUFFER is static, yet cmd_grammar needs it */
void	*zz_get_current_buffer() {
	return YY_CURRENT_BUFFER ;
}

void cmd_initialize_tokens() {

	// yacc debugging
	//if(APcloptions::theCmdLineOptions().debug_grammar) {
	//	zzdebug = 1;
	//}

	add_to_CMD_Tokens_list("AAF",			TOK_AAF			);
	add_to_CMD_Tokens_list("ABSTRACTACTIVITY",	TOK_ABSTRACT		);
	add_to_CMD_Tokens_list("ABSTRACTALL",		TOK_ABSTRACTALL		);
	add_to_CMD_Tokens_list("ABSTRACTALL_QUIET",	TOK_ABSTRACTALL_QUIET	);
	add_to_CMD_Tokens_list("ADDRECIPIENT",		TOK_ADDRECIPIENT	);
	add_to_CMD_Tokens_list("ADDRESOURCE",		TOK_ADDRESOURCE		);
	add_to_CMD_Tokens_list("ALL_ACTS_VISIBLE",	TOK_ALLACTSVISIBLE	);
	add_to_CMD_Tokens_list("APF",			TOK_APF			);
	add_to_CMD_Tokens_list("apgen",			TOK_APGEN		);
	add_to_CMD_Tokens_list("BUILD",			TOK_BUILD		);
	add_to_CMD_Tokens_list("BY",			TOK_BY			);
	add_to_CMD_Tokens_list("CHAMELEON",		TOK_CHAMELEON		);
	add_to_CMD_Tokens_list("CLOSEACTDISPLAY",	TOK_CLOSEACTDISPLAY	);
	add_to_CMD_Tokens_list("CLOSERESDISPLAY",	TOK_CLOSERESDISPLAY	);
/*	add_to_CMD_Tokens_list("CONNECT",		TOK_CONNECT		); */
	add_to_CMD_Tokens_list("CONSOLIDATE",		TOK_CONSOLIDATE		);
/*	add_to_CMD_Tokens_list("CONSTRAINT",		TOK_CONSTRAINT		);
	add_to_CMD_Tokens_list("CMDREQUEST",            TOK_CMDREQUEST	); */
	add_to_CMD_Tokens_list("COPYACTIVITY",		TOK_COPY		);
	add_to_CMD_Tokens_list("CUTACTIVITY",		TOK_CUT			);
	add_to_CMD_Tokens_list("DEBUG",			TOK_DEBUG		);
	add_to_CMD_Tokens_list("DELETEALLDESCENDANTS",	TOK_DELETEALLDESCENDANTS);
	add_to_CMD_Tokens_list("DELETELEGEND",		TOK_DELETELEGEND	);
	add_to_CMD_Tokens_list("DETAILACTIVITY",	TOK_DETAIL		);
	add_to_CMD_Tokens_list("DETAILALL",		TOK_DETAILALL		);
	add_to_CMD_Tokens_list("DETAILALL_QUIET",	TOK_DETAILALL_QUIET	);
	add_to_CMD_Tokens_list("DISPLAYPARENTS",        TOK_DISPLAYPARENTS	);
	add_to_CMD_Tokens_list("DRAGCHILDREN",		TOK_DRAGCHILDREN	);
	add_to_CMD_Tokens_list("EDITACTIVITY",		TOK_EDIT		);
	add_to_CMD_Tokens_list("EDITGLOBAL",		TOK_EDIT_GLOBAL		);
	add_to_CMD_Tokens_list("ENDTIME",		TOK_ENDTIME		);
	add_to_CMD_Tokens_list("EXCLUDE",		TOK_EXCLUDE		);
	add_to_CMD_Tokens_list("EXPORTDATA",		TOK_EXPORT		);
	add_to_CMD_Tokens_list("FILTER",		TOK_FILTER		);
	add_to_CMD_Tokens_list("FIXRESOURCES",          TOK_FIXRESOURCES	);
	add_to_CMD_Tokens_list("FORMAT",		TOK_FORMAT		);
	add_to_CMD_Tokens_list("FROM",			TOK_FROM		);
	add_to_CMD_Tokens_list("FREEZE",		TOK_FREEZE		);
	add_to_CMD_Tokens_list("FULLY",			TOK_FULLY		);
	add_to_CMD_Tokens_list("GRAB",			TOK_GRAB		);
	add_to_CMD_Tokens_list("GROUPACTIVITIES",	TOK_GROUPACTIVITIES	);
	add_to_CMD_Tokens_list("ID",			TOK_ID			);
	add_to_CMD_Tokens_list("INCREMENTAL",		TOK_INCREMENTAL		);
	add_to_CMD_Tokens_list("LAYOUT",		TOK_LAYOUT		);
	add_to_CMD_Tokens_list("LAYOUT_TAG",		TOK_TAG			);
	add_to_CMD_Tokens_list("LEGEND",		TOK_LEGEND		);
	add_to_CMD_Tokens_list("MIX",			TOK_MIX			);
	add_to_CMD_Tokens_list("MODIFIER",              TOK_MODIFIER            );
	add_to_CMD_Tokens_list("MOVEACTIVITY",		TOK_MOVEACTIVITY	);
	add_to_CMD_Tokens_list("MOVELEGEND",		TOK_MOVELEGEND		);
	add_to_CMD_Tokens_list("NAME",			TOK_NAME		);
	add_to_CMD_Tokens_list("NEWACTIVITY",		TOK_NEW			);
	add_to_CMD_Tokens_list("NEWACTIVITIES",         TOK_NEWACTIVITIES	);
	add_to_CMD_Tokens_list("NEWACTDISPLAY",		TOK_NEWACTDISPLAY	);
	add_to_CMD_Tokens_list("NEWHORIZON",		TOK_ZOOM		);
	add_to_CMD_Tokens_list("NEWLEGEND",		TOK_NEWLEGEND		);
	add_to_CMD_Tokens_list("NEWRESDISPLAY",		TOK_NEWRESDISPLAY	);
	add_to_CMD_Tokens_list("-CARRY",		TOK_NOCARRY		);
	add_to_CMD_Tokens_list("NOLEGEND",		TOK_NOLEGEND		);
	add_to_CMD_Tokens_list("NONE",			TOK_NONE		);
	add_to_CMD_Tokens_list("OFF",			TOK_OFF			);
	add_to_CMD_Tokens_list("ON",			TOK_ON			);
	add_to_CMD_Tokens_list("OPENFILE",		TOK_OPENFILE		);
	add_to_CMD_Tokens_list("PARENT",		TOK_PARENT		);
	add_to_CMD_Tokens_list("PAUSE",			TOK_PAUSE		);
	add_to_CMD_Tokens_list("PASTEACTIVITY",		TOK_PASTE		);
	add_to_CMD_Tokens_list("PERIOD",                TOK_PERIOD		);
	add_to_CMD_Tokens_list("PRINT",			TOK_PRINT		);
	add_to_CMD_Tokens_list("PURGE",			TOK_PURGE		);
	add_to_CMD_Tokens_list("QUIT",			TOK_QUIT		);
	add_to_CMD_Tokens_list("REDETAILACTIVITY",	TOK_REDETAIL		);
	add_to_CMD_Tokens_list("REDETAILALL",		TOK_REDETAILALL		);
	add_to_CMD_Tokens_list("REDETAILALL_QUIET",	TOK_REDETAILALL_QUIET	);
	add_to_CMD_Tokens_list("REGENCHILDREN",		TOK_REGENCHILDREN	);
	add_to_CMD_Tokens_list("REMODEL",		TOK_REMODEL		);
	add_to_CMD_Tokens_list("REMOVERESOURCE",	TOK_REMOVERESOURCE	);
	add_to_CMD_Tokens_list("RESOURCE",		TOK_RESOURCE		);
	add_to_CMD_Tokens_list("RESOURCESCROLL",	TOK_RESSCROLL		);
	add_to_CMD_Tokens_list("SAVEFILE",		TOK_SAVE		);
	add_to_CMD_Tokens_list("SAVEPARTIALFILE",	TOK_SAVE_PARTIALFILE	);
	add_to_CMD_Tokens_list("SCHEDULEACTIVITY",	TOK_SCHEDULE		);
	add_to_CMD_Tokens_list("SCHEMA",		TOK_SCHEMA		);
	add_to_CMD_Tokens_list("SELECT",		TOK_SELECT		);
	add_to_CMD_Tokens_list("SELECTACTDISPLAY",	TOK_ADSELECT		);
	add_to_CMD_Tokens_list("SELECTACTLEGEND",	TOK_ACTLEGSELECT	);
	add_to_CMD_Tokens_list("SELECTRESLEGEND",	TOK_RESLEGSELECT	);
	add_to_CMD_Tokens_list("script",		TOK_SCRIPT		);
	add_to_CMD_Tokens_list("START",	 		TOK_START		);
	add_to_CMD_Tokens_list("STARTTIME",	 	TOK_STARTTIME		);
	add_to_CMD_Tokens_list("TO",			TOK_TO			);
	add_to_CMD_Tokens_list("TIMES",                 TOK_TIMES		);
	add_to_CMD_Tokens_list("TIMESYSTEM",            TOK_TIMESYSTEM		);
	add_to_CMD_Tokens_list("UIACTIVITY",		TOK_UIACTIVITY		);
	add_to_CMD_Tokens_list("UNFREEZE",		TOK_UNFREEZE		);
	add_to_CMD_Tokens_list("UNFREEZERESOURCES",	TOK_UNFREEZERESOURCES	);
	add_to_CMD_Tokens_list("UNGROUPACTIVITIES",	TOK_UNGROUPACTIVITIES	);
	add_to_CMD_Tokens_list("UNSCHEDULEACTIVITY",	TOK_UNSCHEDULE		);
	add_to_CMD_Tokens_list("UNTIL",			TOK_UNTIL		);
	add_to_CMD_Tokens_list("VERSION",		TOK_VERSION		);
	add_to_CMD_Tokens_list("WRITESASF",	 	TOK_SASF		);
	add_to_CMD_Tokens_list("WRITETOL",		TOK_TOL			);
	add_to_CMD_Tokens_list("XLOC",			TOK_XLOC		);
	add_to_CMD_Tokens_list("XCMD",			TOK_XCMD		);
	add_to_CMD_Tokens_list("XML",			TOK_XML			);
	add_to_CMD_Tokens_list("YLOC",			TOK_YLOC		);

	add_to_ambiguous_tokens_list("legend");
}

static YY_BUFFER_STATE c_for_command_parser;

void zz_lex_buffers_initialize_to(const char *b) {
	YY_BUFFER_STATE c ;

	/*
	* see http://www.gnu.org/manual/flex-2.5.4/html_chapter/flex_12.html#SEC12
	* for programming details. yy_scan_string() expects a null-terminated
	* string, which will be copied into the parser's internal memory; b can
	* be discarded after this call.
	*/
	wwlineno = 1;
	c_for_command_parser = yy_scan_string(b);
}

void zz_cleanup_lex_buffer() {
	YY_FLUSH_BUFFER;
	yy_delete_buffer(c_for_command_parser);
}

int zzlinenumber() {
	/* debug
	   fprintf(stderr, "zzlinenumber(): %d\n", wwlineno);
	 */
	return wwlineno;
}
