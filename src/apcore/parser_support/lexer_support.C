#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "UTL_time.H"
#include <apcoreWaiter.H>
#include <aafReader.H>
#include "ParsedExpressionSystem.H"
#include "fileReader.H"
#include "ActivityInstance.H"

#define YYSTYPE_IS_DECLARED
#define YYSTYPE parsedExp


#include "grammar.H"
#include "lex_intfc.H"

extern int abdebug;
int	FIRST_call = 0;
tlist<alpha_string, btokennode>&	Tokens() {
	static tlist<alpha_string, btokennode> s;
	return s;
}
using namespace std;

			// must share with grammar_intfc.C:
int			parsing_phase = 0 ;


int
process_alphanumeric_token(
		const char *Ytext);

void
process_when() {
	// we don't want to catch currentvals used in when clause:
	// temporalFlag = 0;
}

			// defined in grammar.C
extern parsedExp	ablval;
extern int		ablinenumber();


void
CCOUT(
		char *S) {
}

void
Ccout(
		char *S) {
}

int
ablex() {
	static int		tval;

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		fprintf(stderr, "(line %d) - %s\n", ablinenumber(), yytext);
	}

	Cstring text_to_use;

	if(FIRST_call == 1) {
		FIRST_call = 0;
		tval = TOK_SINGLE_EXPRESSION;
		text_to_use = "SINGLE EXPRESSION";
	} else if(FIRST_call == 2) {
		FIRST_call = 0;
		// not sure this is the right thing:
		tval = TOK_SINGLE_STATEMENT;
		text_to_use = "SINGLE STATEMENT";
	} else if(FIRST_call == 3) {
		FIRST_call = 0;
		tval = TOK_SINGLE_PARAM_LIST;
		text_to_use = "SINGLE PARAMETER LIST";
	} else {
		tval = yylex();
		text_to_use = yytext;
	}

	if(tval == TOK_ALPHA) {
		tval = process_alphanumeric_token(yytext);
	}

	ablval = parsedExp(new pEsys::pE_w_string(
				pEsys::origin_info(ablinenumber(), aafReader::current_file()),
				text_to_use,
				tval));
	return tval;
}

void	add_to_error_string(const char *s) {
	fileReader::theErrorStream() << s << "\n";
}

int
aberror(const char *s) {
	static char* buffer;

	buffer = (char*) malloc(strlen(s) + strlen(yytext) + 120);

	//
	// We used to have "..." instead of -->...<---. Hopefully,
	// the arrows are clearer, especially when the error token
	// starts and/or ends with a double quote.
	//
	// In addition, we want to correct the line number so it
	// points to the place where the error token starts, not
	// where it ends.
	//
	int theLine = ablinenumber();
	const char* c = yytext;
	while(*c) {
		if(*c == '\n') {
			theLine--;
		}
		c++;
	}
	sprintf(	buffer,
			"%s on line %d near -->%s<-- #",
			s,
			theLine,
			yytext);

	add_to_error_string(buffer);
	if(abdebug) {
		cerr << buffer;
	}
	free(buffer);
	return 0;
}

void
add_to_Tokens_list(
		const char *s,
		int v,
		void (*P)()) {
	Cstring S(s), T(s);

	Tokens() << new btokennode(S, v, P);
	T.to_upper();
	Tokens() << new btokennode(T, v, P);
}

int
found_a_token(
		const char *s) {
	btokennode* N = Tokens().find(s);

	if(N) {
		if(N->payload.tokenproc) {
			N->payload.tokenproc();
		}
		return N->payload.tokenval;
	}
	return 0;
}

parsedExp
found_a_function(const char* s) {
    bfuncnode*	N = aaf_intfc::internalAndUdefFunctions().find(s);
    parsedExp	se;
    if(N) {
	se = parsedExp(new pEsys::pE_w_string(
		pEsys::origin_info(ablinenumber(), aafReader::current_file()),
		s));
    }

    return se;
}

parsedExp
found_a_local_function(const char* s) {
    parsedExp	se;

    if(aafReader::functions().find(s)) {
	se = parsedExp(new pEsys::pE_w_string(
		pEsys::origin_info(ablinenumber(), aafReader::current_file()),
		s));
    }
    return se;
}

parsedExp
found_a_global_method(const char* s) {
	parsedExp	se;

	if(aafReader::methods().find(s)) {
		se = parsedExp(new pEsys::pE_w_string(
				pEsys::origin_info(ablinenumber(), aafReader::current_file()),
				s));
	}
	return se;
}

#ifdef OBSOLETE
parsedExp
found_a_precomputed_resource(const char* s) {
    parsedExp	se;

    if(aafReader::precomputed_resources().find(s)) {
	se = parsedExp(new pEsys::pE_w_string(
		pEsys::origin_info(ablinenumber(), aafReader::current_file()),
		s));
    }
    return se;
}
#endif /* OBSOLETE */

void
process_from() {
}

int
process_alphanumeric_token(
		const char *Ytext) {
	int		token_value = found_a_token( /* which_pass, */ (char *) Ytext);
	const char*	c = Ytext;

	/* NOTE: DO THIS ONE LAST because otherwise numbers with engineering exponents
		 will be parsed as two symbols instead of one number... just try it! */

	if(token_value) {
		/*
		The following token values activate a token-processing
		function that sets ablval to a value that is actually
		used by the yacc parser:

			TOK_DECOMPOSITION
			TOK_CON_KWD
			TOK_TYPE
			TOK_DATA_TYPE
			TOK_INSTANCE
			TOK_HIERARCHY_KEYWORD
			TOK_INTERNAL_FUNC
			TOK_LOCAL_FUNCTION
			TOK_ACTION
				The following tokens are all equivalent in terms of how they
				are processed. It is therefore tempting to lump them all as
				as single token. The reason that is not done is that the syntax
				needs to set precedence rules for proper parsing.
			TOK_MINUS, TOK_PLUS, TOK_MULT, TOK_DIV, TOK_MOD, TOK_EXPONENT, TOK_EQUAL,
			TOK_NOTEQUAL, TOK_LESSTHAN, TOK_GTRTHAN, TOK_LESSTHANOREQ, TOK_GTRTHANOREQ,
			TOK_ANDOP, TOK_OROP
		*/
		return token_value;
	}
	parsedExp se = found_a_function(Ytext);
	if(se) {
		return TOK_INTERNAL_FUNC;
	}
	se = found_a_local_function(Ytext);
	if(se) {
		return TOK_LOCAL_FUNCTION;
	}
	se = found_a_global_method(Ytext);
	if(se) {
		return TOK_METHOD;
	}

#ifdef OBSOLETE
	se = found_a_precomputed_resource(Ytext);
	if(se) {
		return TOK_PRECOMP_ID;
	}
#endif /* OBSOLETE */
	
	if(aafReader::typedefs().find(Ytext)) {
		return TOK_DYN_TYPE;
	}
	if((!strcmp(Ytext, "generic")) || aafReader::activity_types().find(Ytext)) {
		return TOK_ACT_TYPE;
	}
	return TOK_ID;
}

void ab_initialize_tokens() {
	// yacc debugging
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		abdebug = 1;
	}

	add_to_Tokens_list( "abstract",		TOK_ABSTRACT	, NULL );
	add_to_Tokens_list( "abstractable",	TOK_HIERARCHY_KEYWORD, NULL );
	add_to_Tokens_list( "abstracted",	TOK_HIERARCHY_KEYWORD, NULL );
	add_to_Tokens_list( "activity",		TOK_ACTIVITY	, NULL );
	add_to_Tokens_list( "apgen",		TOK_APGEN	, NULL );
	add_to_Tokens_list( "array",		TOK_DATA_TYPE	, NULL );
	add_to_Tokens_list( "associative",	TOK_ASSOC	, NULL );
	add_to_Tokens_list( "at",		TOK_AT		, NULL );
	add_to_Tokens_list( "attributes",	TOK_ATTRIBUTES 	, NULL );
	add_to_Tokens_list( "attribute",	TOK_ATTRIBUTE 	, NULL );
	add_to_Tokens_list( "begin",		TOK_BEGIN 	, NULL );
	add_to_Tokens_list( "call",		TOK_CALL 	, NULL );
	add_to_Tokens_list( "continue",		TOK_CONTINUE 	, NULL );
	add_to_Tokens_list( "boolean",		TOK_DATA_TYPE	, NULL );
	add_to_Tokens_list( "chameleon",	TOK_INSTANCE 	, NULL );
	add_to_Tokens_list( "concurrent_expansion",TOK_DECOMPOSITION,NULL );
	add_to_Tokens_list( "condition",	TOK_CON_KWD	, NULL );
	add_to_Tokens_list( "constant",		TOK_CONSTANT	, NULL );
	add_to_Tokens_list( "constraint",	TOK_CONSTRAINT	, NULL );
	add_to_Tokens_list( "consumable",	TOK_CONSUMABLE	, NULL );
	add_to_Tokens_list( "creation",		TOK_CREATION	, NULL );
	add_to_Tokens_list( "decomposable",	TOK_HIERARCHY_KEYWORD, NULL );
	add_to_Tokens_list( "decomposed",	TOK_HIERARCHY_KEYWORD, NULL );
	add_to_Tokens_list( "decomposition",	TOK_DECOMPOSITION, NULL );
	add_to_Tokens_list( "default",		TOK_DEFAULT	, NULL );
	add_to_Tokens_list( "destruction",	TOK_DESTRUCTION, NULL );
	add_to_Tokens_list( "directive",	TOK_DIRECTIVE	, NULL );
	add_to_Tokens_list( "duration",		TOK_DATA_TYPE	, NULL );
	add_to_Tokens_list( "else",		TOK_ELSE	, NULL );
	add_to_Tokens_list( "end",		TOK_END		, NULL );
	add_to_Tokens_list( "epoch",		TOK_EPOCH	, NULL );
	add_to_Tokens_list( "expansion",	TOK_DECOMPOSITION,NULL );
	add_to_Tokens_list( "extern",		TOK_EXTERN	, NULL );
	add_to_Tokens_list( "false",		TOK_FALSE	, NULL );
	add_to_Tokens_list( "interpolated_windows", TOK_INTERPWINS, NULL );
	add_to_Tokens_list( "finish",		TOK_FINISH 	, /* process_finish */ NULL);
	add_to_Tokens_list( "float",		TOK_DATA_TYPE	, NULL );
	add_to_Tokens_list( "for",		TOK_FOR 	, NULL );
	add_to_Tokens_list( "from",		TOK_FROM 	, NULL );
	add_to_Tokens_list( "function",		TOK_FUNC 	, NULL );
	add_to_Tokens_list( "get_windows",	TOK_GETWINDOWS 	, NULL );
	add_to_Tokens_list( "global",		TOK_GLOBAL 	, NULL );
	add_to_Tokens_list( "id",		TOK_NODEID	, NULL );
	add_to_Tokens_list( "if",		TOK_IF		, NULL );
	add_to_Tokens_list( "immediately",	TOK_IMMEDIATELY	, NULL );
	add_to_Tokens_list( "integrates",	TOK_INTEGRAL	, NULL );
	add_to_Tokens_list( "instance",		TOK_INSTANCE 	, NULL );
	add_to_Tokens_list( "integer",		TOK_DATA_TYPE	, NULL );
	add_to_Tokens_list( "into",		TOK_INTO 	, NULL );
	add_to_Tokens_list( "local",		TOK_LOCAL 	, NULL );
	add_to_Tokens_list( "list",		TOK_LIST 	, NULL );
	add_to_Tokens_list( "message",		TOK_MESSAGE 	, NULL );
	add_to_Tokens_list( "+=",		TOK_MERGE 	, NULL );
	add_to_Tokens_list( "modeling",		TOK_MODEL	, NULL );
	add_to_Tokens_list( "nonconsumable",	TOK_NONCONSUMABLE,NULL );
	add_to_Tokens_list( "nonexclusive_decomposition",TOK_DECOMPOSITION,NULL );
	add_to_Tokens_list( "of",		TOK_OF 		, NULL );
	add_to_Tokens_list( "parameters",	TOK_PARAMETERS 	, NULL );
	add_to_Tokens_list( "pi",		TOK_PI		, NULL );
	add_to_Tokens_list( "prerequisite",	TOK_CON_KWD	, NULL );
	// add_to_Tokens_list( "precomputed_resource", TOK_PRECOMP_RES, NULL );
	add_to_Tokens_list( "profile",		TOK_PROFILE	, NULL );
	add_to_Tokens_list( "rad",		TOK_RAD		, NULL );
	add_to_Tokens_list( "range",		TOK_RANGE	, NULL );
	add_to_Tokens_list( "regexp",		TOK_REGEXP	, NULL );
	add_to_Tokens_list( "request",		TOK_INSTANCE 	, NULL );
	add_to_Tokens_list( "reset",		TOK_ACTION	, NULL );
	add_to_Tokens_list( "resolvable",	TOK_HIERARCHY_KEYWORD, NULL );
	add_to_Tokens_list( "resolved",		TOK_HIERARCHY_KEYWORD, NULL );
	add_to_Tokens_list( "resource",		TOK_RESOURCE	, NULL );
	add_to_Tokens_list( "return",		TOK_RETURN	, NULL );
	add_to_Tokens_list( "script",		TOK_FUNC	, NULL );
	add_to_Tokens_list( "set",		TOK_ACTION	, NULL );
	add_to_Tokens_list( "settable",		TOK_SETTABLE	, NULL );
	add_to_Tokens_list( "severity",		TOK_SEVERITY	, NULL );
	add_to_Tokens_list( "spawn",		TOK_SPAWN	, NULL );
	add_to_Tokens_list( "start",		TOK_START	, /* process_start */ NULL);
	add_to_Tokens_list( "state",		TOK_STATE	, NULL );
	add_to_Tokens_list( "states",		TOK_STATES	, NULL );
	add_to_Tokens_list( "struct",		TOK_STRUCT	, NULL );
	add_to_Tokens_list( "string",		TOK_DATA_TYPE	, NULL );
	add_to_Tokens_list( "template",		TOK_TEMPLATE	, NULL );
	add_to_Tokens_list( "then",		TOK_THEN	, NULL );
	add_to_Tokens_list( "time",		TOK_DATA_TYPE	, NULL );
	add_to_Tokens_list( "to",		TOK_TO		, NULL );
	add_to_Tokens_list( "time_system",	TOK_TIMESYSTEM	, NULL );
	add_to_Tokens_list( "time_series",	TOK_TIME_SERIES	, NULL );
	add_to_Tokens_list( "true",		TOK_TRUE	, NULL );
	add_to_Tokens_list( "type",		TOK_TYPE	, NULL );
	add_to_Tokens_list( "typedef",		TOK_TYPEDEF	, NULL );
	add_to_Tokens_list( "until",		TOK_UNTIL	, NULL );
	add_to_Tokens_list( "usage",		TOK_USAGE	, NULL );
	add_to_Tokens_list( "use",		TOK_ACTION	, NULL );
	add_to_Tokens_list( "version",		TOK_VERSION	, NULL );
	add_to_Tokens_list( "void",		TOK_VOID	, NULL );
	add_to_Tokens_list( "wait",		TOK_WAIT	, NULL );
	add_to_Tokens_list( "when",		TOK_WHEN	, NULL );
	add_to_Tokens_list( "while",		TOK_WHILE	, NULL );
}
