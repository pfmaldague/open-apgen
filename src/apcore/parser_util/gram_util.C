#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <gramgen.H>

string& first_rule() {
	static string S;
	return S;
}

map<string, bool>& tokens() {
	static map<string, bool> M;
	static bool please_initialize = true;
	if(please_initialize) {
		please_initialize = false;
		if(full_item::reentrant()) {
			M["TOK_TERMINATE"] = true;
		}
	}
	return M;
}

map<string, vector<string> >& constructors_of_nonterminal() {
	static bool initialized = false;
	static map<string, vector<string> > A;
	if(!initialized) {
		vector<string> just_one;
		just_one.push_back("ParsedNull");
		initialized = true;
		A["null"] = just_one;
	}
	return A;
}

map<string, bool>& pending_constructors() {
	static map<string, bool> P;
	return P;
}

map<string, string>& extra_members_and_methods() {
	static map<string, string> M;
	return M;
}

map<string, string>& initializers() {
	static map<string, string> I;
	return I;
}

map<string, string>& constructor_additions() {
	static map<string, string> C;
	return C;
}

map<string, string>& special_copy_constructor() {
	static map<string, string> M;
	return M;
}

map<string, string>& special_shallow_constructor() {
	static map<string, string> S;
	return S;
}

map<string, string>& special_recursion_method() {
	static map<string, string> R;
	return R;
}

map<string, string>& special_destructor() {
	static map<string, string> M;
	return M;
}

map<string, string>& base_class() {
	static map<string, string> M;
	return M;
}

map<string, string>& initExpression() {
	static map<string, string> E;
	return E;
}

map<string, string>& eval_expression() {
	static map<string, string> E;
	return E;
}

map<string, vector<vector<full_item> > >& dictionary() {
	static map<string, vector<vector<full_item> > > D;
	return D;
}

int  full_item::numline = 1;


string& full_item::DSL() {
	static string s = "aaf";
	return s;
}

bool&	full_item::reentrant() {
	static bool B = false;
	return B;
}

string& full_item::Prefix() {
	static string g = "gen";
	return g;
}

map<string, vector<constr_spec> >&	pE_constructors() {
	static map<string, vector<constr_spec> > P;
	return P;
}

map<string, vector<string> >&		pE_class_members() {
	static map<string, vector<string> > V;
	return V;
}

bool is_a_token(const string& item_name) {
	if(item_name.find("TOK_") == 0) {
		return true;
	}
	return false;
}

bool is_a_character(const string& item_name) {
	if(item_name.find("ASCII_") == 0) {
		return true;
	}
	return false;
}

// we know that item_name starts with 'ASCII_'
char back_to_char(const string& item_name) {
	size_t	char_start = item_name.find("_");
	string just_the_number = item_name.substr(char_start + 1);
	int i = atoi(just_the_number.c_str());
	char c = (char) i;
	return c;
}

string all_lower_case(const string& s) {
	char*	t = (char*) malloc(s.size() + 1);
	const char* u = s.c_str();
	char*	v = t;
	while(*u) {
		*t = tolower(*u);
		t++;
		u++;
	}
	*t = '\0';
	return string(v);
}


full_item check_if_token(
		const string& raw_item) {
	static bool	first = true;
	string		constructor;
	string		item_name;
	item_semantics	semantics = ADD_TO_OBJECT;

	if(first) {
		first = false;
		first_rule() = raw_item;
		return full_item(raw_item, semantics);
	}
	if(raw_item.find("=!") == 0) {
		string constructor_and_item = raw_item.substr(2);
		size_t	constructor_start;
		if((constructor_start = constructor_and_item.find("-"))
				!= constructor_and_item.npos) {
			semantics = BECOMES_NEW_OBJECT;
			item_name = constructor_and_item.substr(constructor_start + 1);
			constructor = constructor_and_item.substr(0, constructor_start);
		} else if((constructor_start = constructor_and_item.find("+"))
				!= constructor_and_item.npos) {
			semantics = ADD_TO_NEW_OBJECT;
			item_name = constructor_and_item.substr(constructor_start + 1);
			constructor = constructor_and_item.substr(0, constructor_start);
		} else {
			fprintf(stderr, "no - following constructor on line %d\n", full_item::numline);
			exit(-1);
		}
	} else if(raw_item.find("=") == 0) {
		semantics = BECOMES_OBJECT;
		item_name = raw_item.substr(1);
	} else {
		item_name = raw_item;
	}
	if(is_a_token(item_name)) {
		tokens()[item_name] = true;
	}
	if(semantics == BECOMES_NEW_OBJECT || semantics == ADD_TO_NEW_OBJECT) {
		return full_item(constructor, item_name, semantics);
	}
	return full_item(item_name, semantics);
}


int	output_yacc_source_file(FILE* Yfile) {
    vector<full_item>			 		definition;
    vector<vector<full_item> >		 		definitions;
    map<string, vector<vector<full_item> > >::iterator	iter;
    string genericExp;
    string genericError;
    string readerClass;
    string genericWrap;

    if(full_item::reentrant()) {

	//
	// The code in this block is intended to replace the original
	// core with DSL() == "aaf", which produced a non-reentrant
	// parser. As of now (May 13 2020), this code is experimental;
	// it will be used to verify that the output parser source (i. e.,
	// the generated .y file) works as well as the old one.
	//
	// To stay close to what the old .y file did, Prefix() should be
	// set to "gen". This will produce C files like gen_grammar.C
	// and gen_grammar.H, which is what the old parser generator
	// did. Previously, this was ensured by the command-line options
	// provided to bison by the Makefile. In the reentrant version,
	// the options are built into the .y file via directives.
	//
	stringstream s;
	write_bison_preamble(s, full_item::DSL(), full_item::Prefix());
	fprintf(Yfile, "%s", s.str().c_str());
	genericError = "yyerror(scanner, ";
	genericExp = full_item::DSL() + "Exp";
	readerClass = full_item::DSL() + "_reader";
    } else {
	if(full_item::DSL() == "aaf") {
		fprintf(Yfile, "%%{\n#include <ParsedExpressionSystem.H>\n");
		fprintf(Yfile, "#include <aafReader.H>\n");
		fprintf(Yfile, "using namespace pEsys;\n");
		fprintf(Yfile, "extern int ablex();\n");
		fprintf(Yfile, "extern int aberror(const char*);\n");
		fprintf(Yfile, "extern int ablinenumber();\n");
		fprintf(Yfile, "#define YYDEBUG 1\n");
		fprintf(Yfile, "#define YYSTYPE_IS_DECLARED\n");
		genericExp = "parsedExp";

		//
		// include the first parenthesis to allow for more than 1 arg
		//
		genericError = "aberror(";
		readerClass = "aafReader";
		genericWrap = "yywrap";
	} else {
		const char* pre = full_item::DSL().c_str();
		fprintf(Yfile, "%%{\n#include <%s_expressions.H>\n", pre);
		fprintf(Yfile, "#include <%sReader.H>\n", pre);
		fprintf(Yfile, "using namespace %s;\n", pre);
		fprintf(Yfile, "extern int %slex();\n", pre);
		fprintf(Yfile, "extern int %serror(const char*);\n", pre);
		fprintf(Yfile, "extern int %slinenumber();\n", pre);
		fprintf(Yfile, "#define YYDEBUG 1\n");
		fprintf(Yfile, "#define YYSTYPE_IS_DECLARED\n");
		genericExp = full_item::DSL() + "Exp";
		genericError = full_item::DSL() + "error(";
		readerClass = full_item::DSL() + "_reader";
		genericWrap = full_item::DSL() + "wrap";
	}
	fprintf(Yfile, "#define YYSTYPE %s\n", genericExp.c_str());
	fprintf(Yfile, "%%}\n");

    } // if not reentrant

    fprintf(Yfile, "%%start %s\n", first_rule().c_str());
    fprintf(Yfile, "%%token TOK_ALPHA\n");
    map<string, bool>::iterator tok_iter;
    for(tok_iter = tokens().begin(); tok_iter != tokens().end(); tok_iter++) {
	fprintf(Yfile, "%%token %s\n", tok_iter->first.c_str());
    }

    fprintf(Yfile, "\n%%%%\n\n");

    for(iter = dictionary().begin(); iter != dictionary().end(); iter++) {
	definitions = iter->second;
	fprintf(Yfile, "%s:\n", iter->first.c_str());
	bool first = true;
	// first determine the semantic structure:
	for(int i0 = 0; i0 < definitions.size(); i0++) {
	    definition = definitions[i0];
	    int equal_index = -1;
	    bool constructor_needed = false;
	    string constructor_name;
	    bool object_needed = true;
	    bool extract_data = false;

	    //
	    // the int will be used to store the index of each
	    // item in the r.h.s. of a rule; the bool will be
	    // used to tell whether only the data part of the
	    // item should be passed to the constructor (assuming
	    // that a constructor is needed.)
	    //

	    vector<pair<int, bool> > constructor_args;

	    vector<string>	argument_names;
	    stringstream	argument_types;
	    for(int i1 = 0; i1 < definition.size(); i1++) {
		if(definition[i1].Semantics == BECOMES_OBJECT) {
		    if(equal_index >= 0) {
			fprintf(stderr,
				"More than one item defines the rhs of %s\n",
				iter->first.c_str());
			return -1;
		    }
		    equal_index = i1;
		} else if(definition[i1].Semantics == BECOMES_NEW_OBJECT
			  || definition[i1].Semantics == ADD_TO_NEW_OBJECT) {
		    if(equal_index >= 0) {
			fprintf(stderr,
				"More than one item defines the rhs of %s\n",
				iter->first.c_str());
			return -1;
		    }
		    equal_index = i1;
		    constructor_needed = true;
		    constructor_name = definition[i1].Constructor;
		    argument_names.push_back(definition[i1].item);
		    if(definition[i1].Semantics == BECOMES_NEW_OBJECT) {
			/* we only have access to the object via this arg,
			 * so we'd better pass the whole object */
			argument_types << "s";
			constructor_args.push_back(pair<int, bool>(i1, false));
		    } else {
			/* we have access to the object via the elements
			 * vector, so we can afford to only pass theData */
			argument_types << "s";
			constructor_args.push_back(pair<int, bool>(i1, true));
		    }
		} else if(definition[i1].Semantics == ADD_TO_OBJECT) {
		    argument_names.push_back(definition[i1].item);
		    /* we have access to the object via the elements
		     * vector, so we can afford to only pass theData */
		    argument_types << "s";
		    constructor_args.push_back(pair<int, bool>(i1, true));
		}
	    }
	    if(object_needed && equal_index < 0) {
		fprintf(stderr,
			"An object is needed for %s but none is supplied\n",
			iter->first.c_str());
	    }

	    char differentiator[80];
	    int diff_count;
	    // issue the header fragment for the constructor, if required:
	    if(object_needed && constructor_needed) {
		map<string, vector<constr_spec> >::iterator constr_iter;
		constr_spec		cs(constructor_name);

		cs.mainArgAdded = definition[equal_index].Semantics == ADD_TO_NEW_OBJECT;
		for(int vv = 0; vv < argument_names.size(); vv++) {
		    cs.argnames.push_back(argument_names[vv]);
		}
		cs.signature = argument_types.str();
		cs.mainIndex = equal_index;
		if((constr_iter = pE_constructors().find(constructor_name))
				== pE_constructors().end()) {
		    vector<constr_spec>	V;
		    V.push_back(cs);
		    pE_constructors()[constructor_name] = V;
		    diff_count = 0;
		} else {
		    diff_count = constr_iter->second.size();
		    constr_iter->second.push_back(cs);
		}
		sprintf(differentiator, "%d", diff_count);

		// debug
		// cs.print();

	    }
	    // issue the yacc source:
	    if(first) {
		first = false;
		fprintf(Yfile, "\t");
	    } else {
		fprintf(Yfile, "\t| ");
	    }
	    for(int l = 0; l < definition.size(); l++) {
		if(is_a_character(definition[l].item)) {
		    fprintf(Yfile, "'%c' ", back_to_char(definition[l].item));
		} else {
		    fprintf(Yfile, "%s ", definition[l].item.c_str());
		}
	    }
	    fprintf(Yfile, "\n");
	    if(object_needed) {
		// NOTE: yacc numbers rule items starting at 1, not 0:
		if(constructor_needed) {
		    if(constructor_name != "Null") {
			fprintf(Yfile, "\t\t{\n");
			// print informational comments about constructor arguments
			for(int l = 0; l < definition.size(); l++) {
				string one_item = definition[l].item;
				map<string, vector<string> >::iterator constr_iter
					= constructors_of_nonterminal().find(one_item);
				if(constr_iter != constructors_of_nonterminal().end()) {
					fprintf(Yfile, "\t\t// $%d:\n", l + 1);
					for(int l4 = 0; l4 < constr_iter->second.size(); l4++) {
						fprintf(Yfile, "\t\t//\t%s\n", constr_iter->second[l4].c_str());
					}
				}
			}
			// allow for exceptions thrown by the constructor
			fprintf(Yfile, "\t\ttry {\n");
			fprintf(Yfile, "\t\t\t$$ = %s(new %s(%d",
					genericExp.c_str(),
					constructor_name.c_str(),
					equal_index);
			fprintf(Yfile, ", $%d", equal_index + 1);
			for(int i2 = 0; i2 < constructor_args.size(); i2++) {
				if(i2 != equal_index) {
					fprintf(Yfile, ", ");
					// we pass the whole object for this argument
					fprintf(Yfile, "$%d", constructor_args[i2].first + 1);
				}
			}
			fprintf(Yfile, ", Differentiator_%s(0)));\n", differentiator);
			fprintf(Yfile, "\t\t} catch(eval_error Err) {\n");

			//
			// genericError includes the first parenthesis
			//
			fprintf(Yfile, "\t\t\t%s*Err.msg);\n", genericError.c_str());
			fprintf(Yfile, "\t\t\tYYACCEPT;\n");
			fprintf(Yfile, "\t\t}\n");
		    } else {
			fprintf(Yfile, "\t\t{\n");
			fprintf(Yfile, "\t\t$$ = $1;\n");
		    }
		} else {
			fprintf(Yfile, "\t\t{\n");
			fprintf(Yfile, "\t\t$$ = $%d;\n",
				equal_index + 1);
			for(int i2 = 0; i2 < constructor_args.size(); i2++) {
				fprintf(Yfile, "\t\t$$->addExp($%d);\n",
					constructor_args[i2].first + 1);
			}
		}
		if(iter->first == first_rule()) {
		    fprintf(Yfile,
			"\t\t%s::input_files().push_back($$);\n", readerClass.c_str());
		}
		fprintf(Yfile, "\t\t}\n");
	    }
	}
	fprintf(Yfile, "\t;\n\n");
    }

    //
    // null rule
    //
    fprintf(Yfile, "null:\n");
    fprintf(Yfile, "\t\t{\n");
    fprintf(Yfile, "\t\t$$ = %s();\n", genericExp.c_str());
    fprintf(Yfile, "\t\t}\n");
    fprintf(Yfile, "\t;\n");

    //
    // terminate rule
    //
    if(full_item::reentrant()) {
	fprintf(Yfile, "terminate: TOK_TERMINATE\n");
	fprintf(Yfile, "\t\t{\n");
	fprintf(Yfile, "\t\tYYACCEPT;\n");
	fprintf(Yfile, "\t\t}\n");
	fprintf(Yfile, "\t;\n");
    }

    if(genericWrap.size()) {
	fprintf(Yfile, "\n%%%%\n\n");
	fprintf(Yfile, "extern \"C\" {\n");
	fprintf(Yfile, "    int %s() {\n", genericWrap.c_str());
	fprintf(Yfile, "\treturn 1;\n");
	fprintf(Yfile, "    }\n");
	fprintf(Yfile, "}\n");
    }

    return 0;
}

vector<string> compute_direct_constructors(
		string	nonterminal) {
	map<string, vector<string> >::iterator constr_iter = constructors_of_nonterminal().find(nonterminal);
	if(constr_iter != constructors_of_nonterminal().end()) {
		return constr_iter->second;
	}
	vector<full_item>			 		definition;
	vector<vector<full_item> >		 		definitions;
	map<string, vector<vector<full_item> > >::iterator	iter = dictionary().find(nonterminal);
	vector<string>						to_return;
	map<string, bool>					constructor_map;
	map<string, bool>::iterator				constructor_iter;

	if(iter == dictionary().end()) {
		fprintf(stderr, "compute_direct_constructors(): nonterminal %s not found\n",
				nonterminal.c_str());
		return to_return;
	}
	definitions = iter->second;
	bool first = true;
	pending_constructors()[nonterminal] = true;

#	ifdef PRINT_STUFF
	static int indent = 0;
	indent += 3;
	for(int kip = 0; kip < indent; kip++) {
		printf(" ");
	}
	printf("scanning defs for nonterminal %s\n", nonterminal.c_str());
#	endif /* PRINT_STUFF */

	// scan all alternatives for this rule:
	for(int i0 = 0; i0 < definitions.size(); i0++) {
		definition = definitions[i0];
		int equal_index = -1;
		vector<string> constructor_names;
		string constructor_name;
		vector<int> constructor_args;

		for(int i1 = 0; i1 < definition.size(); i1++) {
			if(definition[i1].Semantics == BECOMES_OBJECT) {
				if(is_a_token(definition[i1].item) || is_a_character(definition[i1].item)) {
					constructor_name = "pE";
					constructor_iter = constructor_map.find(constructor_name);
					if(constructor_iter == constructor_map.end()) {
						to_return.push_back(constructor_name);
						constructor_map[constructor_name] = true;
					}
				// avoid infinite recursion:
				} else if(definition[i1].item != nonterminal) {
					if(equal_index >= 0) {
						fprintf(stderr,
							"More than one item defines the rhs of %s\n",
							iter->first.c_str());
#						ifdef PRINT_STUFF
						indent -= 3;
#						endif /* PRINT_STUFF */
						return to_return;
					}
					constructor_names = compute_direct_constructors(definition[i1].item);
					if(!constructor_names.size()) {
						fprintf(stderr,
							"nonterminal %s has no constructors; need one for %s\n",
							definition[i1].item.c_str(),
							nonterminal.c_str());
#						ifdef PRINT_STUFF
						indent -= 3;
#						endif /* PRINT_STUFF */
						return to_return;
					}
					for(int k1 = 0; k1 < constructor_names.size(); k1++) {
						constructor_name = constructor_names[k1];
						constructor_iter = constructor_map.find(constructor_name);
						if(constructor_iter == constructor_map.end()) {
							to_return.push_back(constructor_name);
							constructor_map[constructor_name] = true;
						}
					}
				}
			} else if(definition[i1].Semantics == BECOMES_NEW_OBJECT
				  || definition[i1].Semantics == ADD_TO_NEW_OBJECT) {
				equal_index = i1;
				constructor_name = definition[i1].Constructor;
				constructor_iter = constructor_map.find(constructor_name);
				if(constructor_iter == constructor_map.end()) {
					to_return.push_back(constructor_name);
					constructor_map[constructor_name] = true;
				}
			}
		}
	}
	constructors_of_nonterminal()[nonterminal] = to_return;
	constructor_iter = pending_constructors().find(nonterminal);
	if(constructor_iter != pending_constructors().end()) {
		pending_constructors().erase(constructor_iter);
	}
#	ifdef PRINT_STUFF
	for(int kip = 0; kip < indent; kip++) {
		printf(" ");
	}
	printf("constructors of %s:\n", nonterminal.c_str());
	for(int z = 0; z < to_return.size(); z++) {
		for(int kip = 0; kip < indent; kip++) {
			printf(" ");
		}
		printf("   %s\n", to_return[z].c_str());
	}
	indent -= 3;
#	endif /* PRINT_STUFF */
	return to_return;
}

static int linecount = 1;

void skip_over_newline(FILE* F) {
	char		c;
	while(fread(&c, 1, 1, F)) {
		if(c == '\n') {
			linecount++;
			return;
		}
	}
}

int ingest_constructor_file(FILE* Cfile) {
	char		c[2];
	stringstream	S;
	string		constructor;
	typedef enum {
		LOOKING_FOR_CONSTRUCTOR,
		LOOKING_FOR_CODE_LETTER,
		LOOKING_FOR_CLOSING_BRACE,
		GRABBING_CODE } ingestor_state;

	ingestor_state	state = LOOKING_FOR_CONSTRUCTOR;

	c[1] = '\0';

	//
	// scan the file one character at a time
	//
	while((fread(c, 1, 1, Cfile))) {

		//
		// Most of the time, we are just grabbing code, which we will
		// stuff into a header file or a source file. Code ALWAYS has
		// to start with a blank or a tab:
		//
		if(c[0] == ' ' || c[0] == '\t') {

			//
			// We just started a new Class Constructor. Catch the
			// name we just captured, and switch to code-grabbing
			// mode.
			//
			if(state == LOOKING_FOR_CONSTRUCTOR) {
				constructor = S.str();
				// debug
				// printf("ingest_constructor_file: constructor = %s\n", constructor.c_str());
				skip_over_newline(Cfile);
				S.str("");
				state = GRABBING_CODE;

			//
			// We are in the middle of grabbing code.
			//
			} else {
				// we are looking for a code letter, a closing brace or grabbing code
				state = GRABBING_CODE;
				S << c;
			}

		//
		// We just finished scanning a whole line:
		//
		} else if(c[0] == '\n') {

			//
			// While grabbing code, the only thing that can make
			// us switch to another mode is a lowercase code
			// letter in the first column:
			//
			if(state == GRABBING_CODE) {

				// debug
				// printf("ingest_constructor_file: found newline, looking for code letter...\n");
				//
				state = LOOKING_FOR_CODE_LETTER;

			//
			// Nothing much to do; still looking...
			//
			} else if(state == LOOKING_FOR_CODE_LETTER) {
				;

			//
			// Anything else is an error:
			//
			} else {
				fprintf(stderr, "ingest_constructor: found newline while looking "
						"for } on line %d.\n", linecount);
				return -1;
			}
			linecount++;
			S << c;

		//
		// Closed curly brackets are meaningful outside of grabbing
		// code:
		//
		} else if(c[0] == '}') {

			//
			// We just finished a class definition
			//
			if(state == LOOKING_FOR_CLOSING_BRACE) {
				// debug
				// printf("ingest_constructor_file: found closing brace, looking for constructor...\n");
				skip_over_newline(Cfile);
				state = LOOKING_FOR_CONSTRUCTOR;

			//
			// Nothing special - the curly bracket was part of the
			// code. Grab it.
			//
			} else {
				S << c;
			}

		//
		// MAGIC CODE LETTERS for constructor-specifics.txt file
		// =====================================================
		//
		//
		// letter	meaning
		// -----	-------
		//
		// b		base class
		//
		// c		additional constructor info (tricky - need to know which
		//              constructor to add to)
		//
		// d		special destructor
		//
		// e		custom version of initExpression()
		//
		// i		special initializers (unused)
		//
		// h		special shallow copy constructor
		//
		// m		additional members and methods
		//
		// r		custom version of recursively_apply()
		//
		// s		special copy constructor
		//
		// v		custom version of eval_expression()
		//

		} else if(islower(c[0])) {

			//
			// We are looking for a code-letter - typically, we
			// just processed an endline character:
			//
			if(state == LOOKING_FOR_CODE_LETTER) {
				if(c[0] == 'b') {
					// debug
					// printf("ingest_constructor_file: FOUND 'b' - ");
					string tmp = S.str();
					size_t	char_end = tmp.find("\n");
					base_class()[constructor] = tmp.substr(1, char_end-1);
					// debug
					// printf("\"%s\"", base_class()[constructor].c_str());
					skip_over_newline(Cfile);
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else if(c[0] == 'r') {
					// debug
					// printf("ingest_constructor_file: FOUND 's'\n");
					special_recursion_method()[constructor] = S.str();
					skip_over_newline(Cfile);
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else if(c[0] == 's') {
					// debug
					// printf("ingest_constructor_file: FOUND 's'\n");
					special_copy_constructor()[constructor] = S.str();
					skip_over_newline(Cfile);
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else if(c[0] == 'h') {
					// debug
					// printf("ingest_constructor_file: FOUND 'h'\n");
					special_shallow_constructor()[constructor] = S.str();
					skip_over_newline(Cfile);
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else if(c[0] == 'd') {
					// debug
					// printf("ingest_constructor_file: FOUND 'd'\n");
					special_destructor()[constructor] = S.str();
					skip_over_newline(Cfile);
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else if(c[0] == 'm') {
					// debug
					// printf("ingest_constructor_file: FOUND 'm'\n");
					// extra code is for members and methods
					extra_members_and_methods()[constructor] = S.str();
					skip_over_newline(Cfile);
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else if(c[0] == 'i') {
					// debug
					// printf("ingest_constructor_file: FOUND 'i'\n");
					// extra code is for initializers
					initializers()[constructor] = S.str();
					skip_over_newline(Cfile);
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else if(c[0] == 'c') {
					// debug
					// printf("ingest_constructor_file: FOUND 'c'\n");
					long check;
					string addition = S.str();
					S.str("");
					do {
						check = fread(c, 1, 1, Cfile);
						if(isdigit(c[0])) {
							S << c;
						}
					} while(check && c[0] != '\n');
					string num_args = S.str();
					S.str("");
					S << constructor << "_" << num_args;
					string extended_constr = S.str();
					constructor_additions()[extended_constr] = addition;
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else if(c[0] == 'e') {
					// debug
					// printf("ingest_constructor_file: FOUND 'e'\n");
					// extra code is for initExpression
					initExpression()[constructor] = S.str();
					skip_over_newline(Cfile);
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else if(c[0] == 'v') {
					// debug
					// printf("ingest_constructor_file: FOUND 'v'\n");
					// extra code is for eval_expression
					eval_expression()[constructor] = S.str();
					skip_over_newline(Cfile);
					S.str("");
					state = LOOKING_FOR_CLOSING_BRACE;
				} else {
					fprintf(stderr,
						"ingest_constructor: unknown letter '%c' on line %d\n",
						c[0], linecount);
					return -1;
				}

			//
			// We are in code-grabbing mode:
			//
			} else {
				S << c;
			}
		} else if(c[0] == '#') {
			if(state == GRABBING_CODE) {
				S << c;
			} else {

				//
				// comment line
				//
				skip_over_newline(Cfile);
			}
		} else {
			if(state == LOOKING_FOR_CLOSING_BRACE) {
				state = GRABBING_CODE;
			}
			S << c;
		}
	}
	return 0;
}
