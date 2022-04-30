#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "apDEBUG.H"

#include "ACT_exec.H"
#include <ActivityInstance.H>
#include "CMD_exec.H"
#include "fileReader.H"
#include "RES_eval.H"
#include "UTL_time.H"

extern cmdExp zzlval;

using namespace std;
static stringslist& empty_slist() {
	static stringslist q;
	return q;
}

blist&	Ambiguous_tokens() {
	static blist a(compare_function(compare_bstringnodes, false));
	return a;
}

extern void		removeQuotes(Cstring &);

Cstring		empty, popped_data;
stringslist	theListOfExcludedApfFiles;

int		errorFlag = 0;
extern int	FIRST_call;
extern int	CMD_special_expression_token();
extern char     *wwtext;
extern int	wwlex();
extern void	zzerror(const char *);

CMD_exec &CMD_exec::cmd_subsystem() {
	static CMD_exec s;
	return s;
}

int		WATCH_OUT_FOR_SYMBOL_VS_TOKEN_AMBIGUITY = 0;

CMD_exec::CMD_exec() {
}

CMD_exec::~CMD_exec() {
	clear();
}

void CMD_exec::clear() {
	strlist.clear();
	ptrlist.clear();
	// stack.reset();
}

void CMD_exec::clear_lists() {
	strlist.clear();
	ptrlist.clear();
}

void CMD_exec::add_string_node_to_list(const Cstring& name) {
	strlist << new emptySymbol(name);
}

int zzlex() {
	if(FIRST_call) {
		FIRST_call = 0;
		return CMD_special_expression_token();
	} else {
		int tval = wwlex();
		zzlval = cmdExp(new cmdSys::cE(wwtext));
		return tval;
	}
}

void reset_error_flag() {
	WATCH_OUT_FOR_SYMBOL_VS_TOKEN_AMBIGUITY = 0;
	errorFlag = 0; }

void	add_to_ambiguous_tokens_list(const char * s) {
	if(! Ambiguous_tokens().find(s))
		Ambiguous_tokens() << new bstringnode(s);
}

void	add_to_CMD_Tokens_list(const char* s, int v) {
	Cstring S(s), T(s);

	fileReader::theFileReader().CMD_Tokens() << new btokennode(S, v, NULL);
	if(islower((int) s[0]))
		T.to_upper();
	else
		T.to_lower();
	fileReader::theFileReader().CMD_Tokens() << new btokennode(T, v, NULL);
}

int	found_a_CMD_token(const char * s) {
	btokennode* N = fileReader::theFileReader().CMD_Tokens().find(s);

	if(N) {
		if(WATCH_OUT_FOR_SYMBOL_VS_TOKEN_AMBIGUITY) {
			if(Ambiguous_tokens().find(s)) {
				return 0;
			}
		}
		return N->payload.tokenval;
	}
	return 0;
}
