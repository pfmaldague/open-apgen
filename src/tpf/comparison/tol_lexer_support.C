#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "UTL_time.H"
#include "apcoreWaiter.H"
#include "tolReader.H"
#include "tol_expressions.H"

#include "tolcomp_grammar.H"
#include "tolcomp_tokens.H"

using namespace std;

namespace tol_reader {

//
// Used by parsers; gramgen assumes this method exists.
//
vector<tolExp>&	input_files() {
	static vector<tolExp> V[10];
	return V[tol::which_parser(thread_index)];
}

//
// Used by parsers; gramgen assumes this method exists.
//
string&		current_file() {
	static string C;
	return C;
}

stringstream&	threadErrors(int threadIndex) {
	static stringstream S[10];
	return S[threadIndex];
}

} // namespace tol_reader

namespace tol {

tlist<alpha_string, btokennode>&	Tokens() {
	static tlist<alpha_string, btokennode> s;
	return s;
}

int
process_alphanumeric_token(
		const char *Ytext);

void	add_to_error_string(const char *s) {
	tol_reader::errors_found().store(true);
	tol_reader::threadErrors(thread_index) << s << "\n";
}

} // namespace tol

int
yyerror(yyscan_t scanner, const char *s) {
	char* buffer;

	buffer = (char *) malloc(strlen(s) + strlen(yyget_text(scanner)) + 120);
	sprintf(buffer, "%s on line %d near \"%s\"\n", s,
		yyget_lineno(scanner),
		(const char *) yyget_text(scanner));
	tol::add_to_error_string(buffer);
	if(yydebug) {
		cerr << buffer;
	}
	free(buffer);
	return 0;
}

namespace tol {

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

int
process_alphanumeric_token(
		const char *Ytext) {
	int		token_value = found_a_token( /* which_pass, */ (char *) Ytext);
	const char*	c = Ytext;

	/* NOTE: DO THIS ONE LAST because otherwise numbers with engineering exponents
		 will be parsed as two symbols instead of one number... just try it! */

	if(token_value) {
		return token_value;
	}
	return TOK_SYM;
}

} // namespace tol

void tol_initialize_tokens() {

	tol::add_to_Tokens_list( "act_end",	TOK_END		, NULL );
	tol::add_to_Tokens_list( "act_start",	TOK_START	, NULL );
	tol::add_to_Tokens_list( "attributes",	TOK_ATTRIBUTES	, NULL );
	tol::add_to_Tokens_list( "description",	TOK_DESCRIPTION	, NULL );
	tol::add_to_Tokens_list( "error",	TOK_ERROR	, NULL );
	tol::add_to_Tokens_list( "false",	TOK_FALSE	, NULL );
	tol::add_to_Tokens_list( "legend",	TOK_LEGEND	, NULL );
	tol::add_to_Tokens_list( "node_id",	TOK_NODEID	, NULL );
	tol::add_to_Tokens_list( "parameters",	TOK_PARAMETERS	, NULL );
	tol::add_to_Tokens_list( "release",	TOK_RELEASE	, NULL );
	tol::add_to_Tokens_list( "res",		TOK_RES		, NULL );
	tol::add_to_Tokens_list( "true",	TOK_TRUE	, NULL );
	tol::add_to_Tokens_list( "type",	TOK_TYPE	, NULL );
	tol::add_to_Tokens_list( "unreleased",	TOK_UNRELEASED	, NULL );
	tol::add_to_Tokens_list( "warning",	TOK_WARNING	, NULL );
}
