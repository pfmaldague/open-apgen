#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <gramgen.H>
#include <string.h>

int get_one_char();

void skip_until_newline() {
	int ic;
	while((ic = get_one_char()) != EOF && ic != '\n') {
		;
	}
	full_item::numline++;
	return;
}

int usage(char* s) {
	fprintf(stderr, "Usage: %s <-dsl DSL> <-indent #spaces> <-f skeleton_file_name> <-c constructor_file_name>\n", s);
	fprintf(stderr, "\twhere #spaces (defaults to 4) is the number of spaces per "
			"indentation level in the skeleton file.\n");
	return -1;
}

static bool read_from_file = false;
static FILE* the_input_file = NULL;

int get_one_char() {
	if(read_from_file) {
		char c;
		int n = fread(&c, 1, 1, the_input_file);
		if(!n) {
			return EOF;
		}
		return c;
	} else {
		return getchar();
	}
}


int main(int argc, char* argv[]) {
	int					ic;
	int					state = 0;
	int					numblank = 0;
	int					current_indent = 0;
	string					current_lhs;
	full_item				current_item;
	string					current_string;
	stringstream				ss;
	list<string>				stack;
	vector<full_item>			definition;
	list<vector<full_item> >		definition_stack;
	vector<vector<full_item> >		definitions;
	list<vector<vector<full_item> > >	definitions_stack;
	int					indent = 4;
	char*					skel_filename = NULL;
	const char*				constr_filename = "constructor-specifics.txt";
	FILE*					Hinput = NULL;
	bool					indent_flag = false;
	bool					dsl_flag = false;
	bool					prefix_flag = false;

	//
	// set the default DSL
	//
	full_item::DSL() = "aaf";

	if(argc > 1) {
		for(int i = 1; i < argc; i++) {
			if(dsl_flag) {
				full_item::DSL() = argv[i];
				dsl_flag = false;
			} else if(indent_flag) {
				indent = atoi(argv[i]);
				indent_flag = false;
			} else if(prefix_flag) {
				full_item::Prefix() = argv[i];
				prefix_flag = false;
			} else if(!strcmp(argv[i], "-indent")) {
				indent_flag = true;
			} else if(!strcmp(argv[i], "-dsl")) {
				dsl_flag = true;
			} else if(!strcmp(argv[i], "-prefix")) {
				prefix_flag = true;
			} else if(!strcmp(argv[i], "-f")) {
				read_from_file = true;
				if(i == argc-1) {
					return usage(argv[0]);
				}
				skel_filename = argv[++i];
				if(!(the_input_file = fopen(skel_filename, "r"))) {
					fprintf(stderr, "Cannot open %s\n", skel_filename);
					return -1;
				}
			} else if(!strcmp(argv[i], "-c")) {
				if(i == argc-1) {
					return usage(argv[0]);
				}
				constr_filename = argv[++i];
			} else if(!strcmp(argv[i], "-reentrant")) {
				full_item::reentrant() = true;
			} else {
				return usage(argv[0]);
			}
		}
		if(dsl_flag || prefix_flag || indent_flag) {
			return usage(argv[0]);
		}
	}

	while((ic = get_one_char()) != EOF) {
		char c = (char) ic;
		if(c == '\t') {
			fprintf(stderr, "Please convert all tabs to %d spaces in the input file.\n",
					indent);
			return -1;
		} else if(c == '#') {
			skip_until_newline();
			continue;
		}
		switch(state) {
			case 0:
				if(c == ' ') {
					numblank++;
				} else if(numblank % indent) {
					fprintf(stderr, "# of blanks = %d, not divisible by %d on line %d\n",
							numblank, indent, full_item::numline);
					return -1;
				} else {
					int new_indent = numblank / indent;
					// printf("  %d: first nonblank char; current_string = %s\n",
					// 		full_item::numline, current_string.c_str());
					ss.str("");
					if(new_indent == current_indent + 1) {
						current_item = check_if_token(current_string);
						current_lhs = current_item.item;
						stack.push_back(current_lhs);
						definition_stack.push_back(definition);
						definitions_stack.push_back(definitions);
#						ifdef PRINT_STUFF
						printf("level %d, defining %s\n",
							new_indent, current_lhs.c_str());
#						endif /* PRINT_STUFF */
						definitions.clear();
						definition.clear();
					} else if(new_indent == current_indent) {
						if(full_item::numline > 1) {
#							ifdef PRINT_STUFF
							printf("continuing definition of %s\n",
								current_lhs.c_str());
#							endif /* PRINT_STUFF */
						}
					} else {
						while(new_indent < current_indent) {
							definitions.push_back(definition);
#							ifdef PRINT_STUFF
							printf("popping %s\n", stack.back().c_str());
#							endif /* PRINT_STUFF */
							current_lhs = stack.back();
							dictionary()[current_lhs] = definitions;
#							ifdef PRINT_STUFF
							printf("setting dict[%s] to\n", current_lhs.c_str());
							for(int k = 0; k < definitions.size(); k++) {
								printf("\t%d: ", k);
								for(int l = 0; l < definitions[k].size(); l++) {
									printf("%s ", definitions[k][l].item.c_str());
								}
								printf("\n");
							}
#							endif /* PRINT_STUFF */
							definitions = definitions_stack.back();
							definition = definition_stack.back();
							stack.pop_back();
							definitions_stack.pop_back();
							definition_stack.pop_back();
							current_indent--;
						}
						current_lhs = stack.back();
#						ifdef PRINT_STUFF
						printf("resuming definition of %s\n",
							current_lhs.c_str());
#						endif /* PRINT_STUFF */
					}
					current_indent = new_indent;
					if(c == '|') {
						definitions.push_back(definition);
						definition.clear();
						state = 2;
					} else if(c == '\'') {
						state = 6;
					} else {
						ss << c;
						state = 1;
					}
				}
				break;
			case 1:
				if(c == '\n') {
					current_string = ss.str();
					current_item = check_if_token(current_string);
					definition.push_back(current_item);
					ss.str("");
#					ifdef PRINT_STUFF
					printf("current rhs(item %ld, line %d) = %s\n",
							definition.size(), full_item::numline, current_string.c_str());
#					endif /* PRINT_STUFF */
					state = 0;
					numblank = 0;
					full_item::numline++;
				} else if(c == ' ') {
					current_string = ss.str();
					current_item = check_if_token(current_string);
					definition.push_back(current_item);
					ss.str("");
#					ifdef PRINT_STUFF
					printf("current rhs(item %ld, line %d) = %s\n",
						definition.size(), full_item::numline, current_string.c_str());
#					endif /* PRINT_STUFF */
				} else if(c == '\'') {
					state = 6;
				} else {
					ss << c;
				}
				break;
			case 2:
				if(c != ' ') {
					fprintf(stderr, "non-blank char %c after | on line %d\n",
							c, full_item::numline);
					return -1;
				} else {
					state = 3;
				}
				break;
			case 3:
				if(isalpha(c) || c == '_') {
					ss << c;
				} else if(c == '\'') {
					state = 4;
				} else if(c == ' ') {
					current_string = ss.str();
					current_item = check_if_token(current_string);
					definition.push_back(current_item);
#					ifdef PRINT_STUFF
					printf("current rhs(item %ld, line %d) = %s\n",
						definition.size(), full_item::numline, current_string.c_str());
#					endif /* PRINT_STUFF */
					ss.str("");
				} else if(c == '\n') {
					current_string = ss.str();
					current_item = check_if_token(current_string);
					definition.push_back(current_item);
#					ifdef PRINT_STUFF
					printf("current rhs(item %ld, line %d) = %s\n",
						definition.size(), full_item::numline, current_string.c_str());
#					endif /* PRINT_STUFF */
					ss.str("");
					state = 0;
					numblank = 0;
					full_item::numline++;
				} else {
					ss << c;
				}
				break;
			case 4:
				{
				char ascii_buf[21];
				int k = c;
				sprintf(ascii_buf, "%d", k);
				ss << "ASCII_" << ascii_buf;
				state = 5;
				}
				break;
			case 5:
				state = 3;
				break;
			case 6:
				{
				char ascii_buf[21];
				int k = c;
				sprintf(ascii_buf, "%d", k);
				ss << "ASCII_" << ascii_buf;
				state = 7;
				}
				break;
			case 7:
				state = 1;
				break;
			default:
				break;
		}
	}
	if(the_input_file) {
		fclose(the_input_file);
	}
	while(0 < current_indent) {
		definitions.push_back(definition);
#		ifdef PRINT_STUFF
		printf("popping %s\n", stack.back().c_str());
#		endif /* PRINT_STUFF */
		current_lhs = stack.back();
		dictionary()[current_lhs] = definitions;
#		ifdef PRINT_STUFF
		printf("setting dict[%s] to\n", current_lhs.c_str());
		for(int k = 0; k < definitions.size(); k++) {
			printf("\t%d: ", k);
			for(int l = 0; l < definitions[k].size(); l++) {
				printf("%s ", definitions[k][l].item.c_str());
			}
			printf("\n");
		}
#		endif /* PRINT_STUFF */
		definitions = definitions_stack.back();
		definition = definition_stack.back();
		stack.pop_back();
		definitions_stack.pop_back();
		definition_stack.pop_back();
		current_indent--;
	}
#	ifdef PRINT_STUFF
	printf("===================\n");
#	endif /* PRINT_STUFF */

	map<string, vector<vector<full_item> > >::iterator	iter;
	for(iter = dictionary().begin(); iter != dictionary().end(); iter++) {
		vector<string> constructors = compute_direct_constructors(iter->first);
	}
	FILE* Youtput = fopen("grammar.y", "w");
	assert(Youtput);
	int r1 = output_yacc_source_file(Youtput);
	fclose(Youtput);

	//
	// process the constructor file
	//

	Hinput = fopen(constr_filename, "r");
	if(Hinput) {
		int r3 = ingest_constructor_file(Hinput);
		fclose(Hinput);
		if(r3) {
			return -1;
		}
	}
	//
	// Not an error if file does not exist
	//

	compute_max_number_of_constructor_signatures();

	FILE* Houtput = fopen("gen-parsedExp.H", "w");
	assert(Houtput);
	int r2 = output_header_file(Houtput);
	fclose(Houtput);
	FILE* Coutput = fopen("gen-parsedExp.C", "w");
	assert(Coutput);
	int r4 = output_source_files(Coutput);
	fclose(Coutput);
	return std::min<int>(r1, std::min<int>(r2, r4));
}
