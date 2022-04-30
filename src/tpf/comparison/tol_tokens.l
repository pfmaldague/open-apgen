%{
#include <tol_expressions.H>

#define YYSTYPE_IS_DECLARED
#define YYSTYPE tolExp

#include "tolcomp_grammar.H"
#include "tol_lex_intfc.H"
#include "tolReader.H"

#define register /**/

// extern	tolExp yylval;
%}
 
D				[0-9]
alpha				[a-zA-Z_]
alphanumeric			[a-zA-Z0-9_]

alphanumeric_or_minus		[a-zA-Z0-9_\-+/.]
any_char_excepts                [^\"\\]
quote                           \\\"
backslash                       \\\\
escaped_newline                 \\\n
newline                         \\n

%x name
%x start_of_record

%option yylineno
%option outfile="tolcomp_tokens.C" header-file="tolcomp_tokens.H"
%option reentrant bison-bridge
%option noyywrap nounput noinput

%%

"//"[^\n]*\n			;

\r				;

"n_hidden_resources=".*\n	;
"udef=".*\n	;
"user=".*\n	;
"date=".*\n	;
"hostname=".*\n	;
"n_apgen_files=".*\n	;
"(This header to be removed ".*\n ;

"apgen TOL format".*\n {

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_HEADER_START));
	return TOK_HEADER_START;
	};

"$$EOH"\n	{
	BEGIN(start_of_record);

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_HEADER_END));
	return TOK_HEADER_END;
}

"auxiliary_tol=".*\n {

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_AUXFILE));
	return TOK_AUXFILE;
	};

"resource_metadata:\n" {

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_RES_METADATA));
	return TOK_RES_METADATA;
	};

"activity_metadata:\n" {

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_ACT_METADATA));
	return TOK_ACT_METADATA;
	};

"apgen_file=".*\n {

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_INPUTFILE));
	return TOK_INPUTFILE;
	};

ACT_START {
	BEGIN(name);
	Cstring text_to_use = yyget_text(yyscanner);

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			text_to_use,
			TOK_START));
	return TOK_START;
	};

<name>[^,]+	{
	Cstring text_to_use = yyget_text(yyscanner);

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			text_to_use,
			TOK_SYM));
	BEGIN(INITIAL);
	return TOK_SYM;
	};

<name>, {
	return ',';
	};


<start_of_record>{D}+"-"{D}+T{D}+":"{D}+":"{D}+"."{D}+ {
	int	p_i = tol::which_parser(thread_index);
	char*	the_text = yyget_text(yyscanner);
	int	tval = TOK_TIME;
	
	if(tol::pE::first_record(p_i)) {
		tol::pE::first_record(p_i) = false;
		CTime_base first(the_text);
		tol::pE::first_time_tag(p_i).store(first.get_pseudo_millisec());
	}

	//
	// If the main thread has determined that
	// we should terminate parsing, this is
	// the right place to do it. We don't want
	// to issue the TOK_TERMINATE token at any
	// place other than the start of a new record,
	// for otherwise the grammar parser would
	// issue a parsing error.
	//
	else if(tol::pE::terminate_parsing(p_i)) {
		tval = TOK_TERMINATE;
	} else if(tol_reader::errors_found().load()) {
		tval = TOK_TERMINATE;
	}

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			the_text,
			tval));
	return  tval;
	};

<start_of_record>, {
	BEGIN(INITIAL);
	return ',';
	};

{D}+"-"{D}+T{D}+":"{D}+":"{D}+"."{D}+ {

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_TIME));
	return  TOK_TIME;
	};

{D}+"-"{D}+T{D}+":"{D}+":"{D}+		{

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_TIME));
	return  TOK_TIME;
	};

{D}+T{D}+":"{D}+":"{D}+"."{D}+		{

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_DURATION));
	return  TOK_DURATION;
	};

{D}+T{D}+":"{D}+":"{D}+			{

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_DURATION));
	return  TOK_DURATION;
	};

{D}+":"{D}+":"{D}+"."{D}+		{

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_DURATION));
	return  TOK_DURATION;
	};

{D}+":"{D}+":"{D}+			{

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_DURATION));
	return  TOK_DURATION;
	};

{D}+":"{D}+"."{D}+			{

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_DURATION));
	return  TOK_DURATION;
	};

{D}+":"{D}+				{

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_DURATION));
	return  TOK_DURATION;
	};

[0-9]+"."?[0-9]*([EeDd][+\-]?[0-9]+)?	{
	/* Put this one first!!!!! */

	bool is_an_integer = true;
	int tokenval = TOK_INTEGER;
	for(int i = 0; i < strlen(yyget_text(yyscanner)); i++) {
		if(!isdigit(yyget_text(yyscanner)[i])) {
			is_an_integer = false;
			tokenval = TOK_FLOAT;
		}
	}
	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			tokenval));
	return  tokenval;
	};

[0-9]*\.[0-9]+([Ee][+\-]?[0-9]+)?	{

	bool is_an_integer = true;
	int tokenval = TOK_INTEGER;
	for(int i = 0; i < strlen(yyget_text(yyscanner)); i++) {
		if(!isdigit(yyget_text(yyscanner)[i])) {
			is_an_integer = false;
			tokenval = TOK_FLOAT;
		}
	}

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			tokenval));
	return  tokenval;
	};

\"({any_char_excepts}|{quote}|{newline}|{escaped_newline}|{backslash})*\"  {

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			TOK_STRING));
	return TOK_STRING;
	};

;	{
	BEGIN(start_of_record);
	return ';';
	};

<start_of_record>\n {

	//
	// Make sure you jump over the newline character
	// that follows the semicolon at the end of a record
	//
	};

[ \t\n]+	{
	};

#[^\n]*\n	{
	};

{alpha}{alphanumeric_or_minus}*	{
	int tval = tol::process_alphanumeric_token(yyget_text(yyscanner));

	Cstring text_to_use = yyget_text(yyscanner);

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			text_to_use,
			tval));

	//
	// This will be TOK_SYM if it is not a keyword:
	//
	return tval;
	};

.	{

	*yylval =  tolExp(new tol::pE(
			tol::origin_info(
				yyget_lineno(yyscanner),
				tol_reader::current_file()),
			yyget_text(yyscanner),
			yyget_text(yyscanner)[0]));
	return yyget_text(yyscanner)[0];
	};

%%
