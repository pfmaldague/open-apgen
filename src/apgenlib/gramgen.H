#ifndef _GRAM_GEN_H_
#define _GRAM_GEN_H_

#include <stdio.h>
#include <sstream>
#include <map>
#include <list>
#include <vector>
#include <assert.h>
#include <stdlib.h>
#include <algorithm>

// #define PRINT_STUFF

using namespace std;

extern string& first_rule();
extern map<string, bool>& tokens();
extern map<string, vector<string> >& constructors_of_nonterminal();
extern map<string, bool>& pending_constructors();
extern map<string, string>& extra_members_and_methods();
extern map<string, string>& initializers();
extern map<string, string>& constructor_additions();
extern map<string, string>& initExpression();
extern map<string, string>& eval_expression();
extern map<string, string>& special_copy_constructor();
extern map<string, string>& special_shallow_constructor();
extern map<string, string>& special_recursion_method();
extern map<string, string>& special_destructor();
extern map<string, string>& base_class();

typedef enum {
	ADD_TO_OBJECT,			// nothing special
	BECOMES_OBJECT,			// preceded by '='
	ADD_TO_NEW_OBJECT,		// preceded by "=!<constructor>+"
	BECOMES_NEW_OBJECT		// preceded by "=!<constructor>-"
} item_semantics;

class full_item {
public:
	full_item()
		: Semantics(ADD_TO_OBJECT),
			Constructor("pE"),
			item("") {}
	full_item(
		string		constructor,
		string		an_item,
		item_semantics	semantics)
			: Semantics(semantics),
				Constructor(constructor),
				item(an_item) {}
	full_item(
		string		an_item,
		item_semantics	semantics)
			: Semantics(semantics),
				item(an_item) {}
	full_item(
		const full_item& FI)
			: Semantics(FI.Semantics),
				Constructor(FI.Constructor),
				item(FI.item) {}

	~full_item() {}

	static int	numline;
	static string&	DSL();
	static bool&	reentrant();
	static string&	Prefix();
	string		Constructor;
	string		item;
	item_semantics	Semantics;
};

class constr_spec {
public:
	constr_spec()
		: name("unknown"),
			mainArgAdded(false),
			mainIndex(0) {}
	constr_spec(const string& n)
		: name(n),
			mainArgAdded(false),
			mainIndex(0) {}
	constr_spec(const constr_spec& cd)
		: name(cd.name),
			mainArgAdded(cd.mainArgAdded),
			mainIndex(cd.mainIndex),
       			signature(cd.signature),
			argnames(cd.argnames) {}
	~constr_spec() {}

	string		name;
	string		signature;
	vector<string>	argnames;
	int		mainIndex;
	bool		mainArgAdded;
	void		print() {
		printf("%s: signature %s main %d (added: %d)\n",
			name.c_str(),
			signature.c_str(),
			mainIndex,
			(int)mainArgAdded);
		for(int i = 0; i < argnames.size(); i++) {
			printf("\t%s\n", argnames[i].c_str());
		}
	}
};

extern map<string, vector<vector<full_item> > >& dictionary();
extern map<string, vector<constr_spec> >& pE_constructors();
extern map<string, vector<string> >& pE_class_members();
extern vector<string> compute_direct_constructors(
		string	nonterminal);
extern void compute_max_number_of_constructor_signatures();
extern int output_yacc_source_file(
		FILE* to_this_stream);
extern int ingest_constructor_file(
		FILE* the_file);
extern int output_header_file(
		FILE* to_this_stream);
extern int output_source_files(
		FILE* to_this_stream);
extern full_item check_if_token(
		const string& raw_item);
extern bool is_a_token(
		const string& item_name);
extern bool is_a_character(
		const string& item_name);
extern char back_to_char(
		const string& item_name);
extern string all_lower_case(
		const string& s);
extern void write_bison_preamble(
		stringstream& str,
		const string& dsl,
		const string& output_file_prefix);

#endif /* _GRAM_GEN_H_ */
