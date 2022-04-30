%{
#include <ParsedExpressionSystem.H>

#define YYSTYPE_IS_DECLARED
#define YYSTYPE parsedExp

#include "grammar.H"

#define register /**/

extern	parsedExp ablval;
%}
 
D				[0-9]
alpha				[a-zA-Z_]
alphanumeric			[a-zA-Z0-9_]
any_char_excepts                [^\"\\]
quote                           \\\"
backslash                       \\\\
escaped_newline                 \\\n
newline                         \\n

%option yylineno

%%

"//"[^\n]*\n			;

\r				;

"=="	{
	return TOK_EQUAL;
	}
">"	{
	return TOK_GTRTHAN;
	}
">="	{
	return TOK_GTRTHANOREQ;
	}
"<"	{
	return TOK_LESSTHAN;
	}
"<="	{
	return TOK_LESSTHANOREQ;
	}
"!="	{
	return TOK_NOTEQUAL;
	}

"*"	{
	return TOK_MULT;
	}
"/"	{
	return TOK_DIV;
	}

"->"  {
	return TOK_RANGE_SYM;
	}

"^"	{
	return TOK_EXPONENT;
	}

"%"	{
	return TOK_MOD;
	}

"&&"	{
	return TOK_ANDOP;
	}

"||"	{
	return TOK_OROP;
	}

{D}+"-"{D}+T{D}+":"{D}+":"{D}+"."{D}+	{
	return  TOK_TIME;
	}

{D}+"-"{D}+T{D}+":"{D}+":"{D}+		{
	return  TOK_TIME;
	}

{D}+T{D}+":"{D}+":"{D}+"."{D}+		{
	return  TOK_DURATION;
	}

{D}+T{D}+":"{D}+":"{D}+			{
	return  TOK_DURATION;
	}

{D}+":"{D}+":"{D}+"."{D}+		{
	return  TOK_DURATION;
	}

{D}+":"{D}+":"{D}+			{
	return  TOK_DURATION;
	}

{D}+":"{D}+"."{D}+			{
	return  TOK_DURATION;
	}

{D}+":"{D}+				{
	return  TOK_DURATION;
	}

[0-9]+"."?[0-9]*([EeDd][+\-]?[0-9]+)?	{
	/* Put this one first!!!!! */
	return  TOK_NUMBER;
	}

[0-9]*\.[0-9]+([Ee][+\-]?[0-9]+)?	{
	return  TOK_NUMBER;
	}

"+="	{
	return TOK_MERGE;
	}


"+"	{

	/* PUT '+' and '-' AFTER engineering-notation numbers... trust me PFM 2/19/99. */

	return TOK_PLUS;
	}

"="	{
	return TOK_EQUAL_SIGN;
	}

"-"	{
	return TOK_MINUS;
	}

\"({any_char_excepts}|{quote}|{newline}|{escaped_newline}|{backslash})*\"  {
	return TOK_STRINGVAL;
	}

\.\.\.\* {
	return TOK_MULTIPLE_OF;
	}

[ \t\n]+		;

!@x		{
	// skip over "APGen X only" markers
	}

#[^\n]*\n	{
	}

!@r[^\n]*\n	{
	// skip over "APGen reset-proto-B only" markers
	}

{alpha}{alphanumeric}*	{
	return TOK_ALPHA;
	/* return process_alphanumeric_token(yytext); */
	}

.	{
	return yytext[0];
	}

%%

static YY_BUFFER_STATE c_for_adaptation_parser;

void yy_lex_buffers_initialize_to(const char *b) {

	/*
	* see http://www.gnu.org/manual/flex-2.5.4/html_chapter/flex_12.html#SEC12
	* for programming details. yy_scan_string() expects a null-terminated
	* string, which will be copied into the parser's internal memory; b can
	* be discarded after this call.
	*/
	c_for_adaptation_parser = yy_scan_string(b);
	yylineno = 1;
}

void yy_cleanup_tokens_buffer() {
	YY_FLUSH_BUFFER;
	yy_delete_buffer(c_for_adaptation_parser);
}

int ablinenumber() {
	return yylineno;
}
