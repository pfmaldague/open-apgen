#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <gramgen.H>

int MaxSignatures = 0;

void	write_main_constructor(
		const string&	constructor,
		bool		header,
		constr_spec&	cs,
		const char*	differentiator,
		FILE*		Wfile) {
	const char*			constr = constructor.c_str();
	map<string, string>::iterator	initializer_iter;
	map<string, bool>		arg_names_used;
	map<string, string>::iterator base_class_iter
		= base_class().find(constructor);
	string			base_class_name = "pE";

	if(base_class_iter != base_class().end()) {
		base_class_name = base_class_iter->second;
	}

	if(header) {
		fprintf(Wfile, "\t%s(\n\t\tint\t\tmain_index", constr);
	} else {
		fprintf(Wfile, "%s::%s(\n\tint\t\tmain_index", constr, constr);
	}

	//
	// Right now, argnames are used to define the arguments of the main
	// constructor. Later on we will also define data members, one for
	// each argument.
	//

	string mainname = cs.argnames[cs.mainIndex];
	if(is_a_token(mainname)) {
		mainname = all_lower_case(mainname);
	}
	arg_names_used[mainname] = true;

	string prefix;
	if(header) {
		prefix = "\t";
	}

	string genericExp;
	string readerClass;
	if(full_item::DSL() == "aaf") {
		genericExp = "parsedExp";
		readerClass = "aafReader";
	} else {
		genericExp = full_item::DSL() + "Exp";
		readerClass = full_item::DSL() + "_reader";
	}

	fprintf(Wfile, ",\n%s\t", prefix.c_str());
	fprintf(Wfile, "%s&\tP_%s", genericExp.c_str(), mainname.c_str());
	if(cs.argnames.size() != 1) {
		fprintf(Wfile, ",");
	} else {
		fprintf(Wfile, ",\n%s\t%s)", prefix.c_str(), differentiator);
	}
	if(is_a_character(mainname)) {
		fprintf(Wfile, "\t// %c\n", back_to_char(mainname));
	} else {
		fprintf(Wfile, "\n");
	}
	int actual_argno = 1;
	for(int j = 0; j < cs.argnames.size(); j++) {
		if(j != cs.mainIndex) {
			string argname = cs.argnames[j];
			if(is_a_token(argname)) {
				argname = all_lower_case(argname);
			}
			if(arg_names_used.find(argname) != arg_names_used.end()) {
				char buf[1024];
				sprintf(buf, "%s_%d", argname.c_str(), j);
				argname = buf;
			}
			arg_names_used[argname] = true;
			fprintf(Wfile, "%s\t", prefix.c_str());
			fprintf(Wfile, "%s&\tP_%s", genericExp.c_str(), argname.c_str());
			fprintf(Wfile, ",");
			if(is_a_character(argname)) {
				fprintf(Wfile, "\t// %c\n", back_to_char(argname));
			} else {
				fprintf(Wfile, "\n");
			}
			if(actual_argno == cs.argnames.size() - 1) {
				fprintf(Wfile, "%s\t%s)", prefix.c_str(), differentiator);
			}
			actual_argno++;
		}
	}
	if(header) {
		fprintf(Wfile, ";\n");
	} else {

		//
		// We write initializers, one for each data member/argument
		//

		fprintf(Wfile, "\n");

		arg_names_used.clear();
		arg_names_used[mainname] = true;

		if(full_item::DSL() == "aaf") {
			fprintf(Wfile,
			    "\t: %s(origin_info(yylineno, %s::current_file())),",
			    base_class_name.c_str(), readerClass.c_str());
		} else {
			fprintf(Wfile,
			    "\t: %s(origin_info(%slineno, %s::current_file())),",
			    base_class_name.c_str(), full_item::DSL().c_str(), readerClass.c_str());
		}
		fprintf(Wfile, "\n\t\t%s(P_%s)", mainname.c_str(), mainname.c_str());
		if(cs.argnames.size() > 1) {
			fprintf(Wfile, ",\n");
		} else {
			fprintf(Wfile, "\n");
		}
		int actual_argno = 1;
		for(int j = 0; j < cs.argnames.size(); j++) {
			if(j != cs.mainIndex) {
				string argname = cs.argnames[j];
				if(is_a_token(argname)) {
					argname = all_lower_case(argname);
				}
				if(arg_names_used.find(argname) != arg_names_used.end()) {
					char buf[1024];
					sprintf(buf, "%s_%d", argname.c_str(), j);
					argname = buf;
				}
				arg_names_used[argname] = true;
				fprintf(Wfile, "\t\t%s(P_%s)", argname.c_str(), argname.c_str());
				if(actual_argno == cs.argnames.size() - 1) {
					// fprintf(Wfile, "\n");
				} else {
					fprintf(Wfile, ",\n");
				}
				actual_argno++;
			}
		}

		//
		// Add any special initializers found in the
		// constructor-specifics file
		//

		initializer_iter = initializers().find(constructor);
		if(initializer_iter != initializers().end()) {
			fprintf(Wfile, ",\n\t\t%s", initializer_iter->second.c_str());
		} else {
			fprintf(Wfile, "\n");
		}
		fprintf(Wfile, "\t{\n");
		fprintf(Wfile, "\tmainIndex = main_index;\n");
		char buf[1024];
		sprintf(buf, "%s_%ld", constructor.c_str(), cs.argnames.size());
		initializer_iter = constructor_additions().find(buf);
		if(initializer_iter != constructor_additions().end()) {
			fprintf(Wfile, "%s", initializer_iter->second.c_str());
		}
		fprintf(Wfile, "\tif(%s) {\n", mainname.c_str());
		fprintf(Wfile, "\t\tinitExpression(%s->getData());\n", mainname.c_str());
		fprintf(Wfile, "\t} else {\n");
		fprintf(Wfile, "\t\tinitExpression(\"(null)\");\n");
		fprintf(Wfile, "\t}\n");
		fprintf(Wfile, "}\n");
	}
}

void	compute_max_number_of_constructor_signatures() {
	map<string, vector<constr_spec> >::iterator constr_iter;

	for(	constr_iter = pE_constructors().begin();
		constr_iter != pE_constructors().end();
		constr_iter++) {
		vector<constr_spec>&	specs = constr_iter->second;
		if(specs.size() > MaxSignatures) {
			MaxSignatures = specs.size();
		}
	}
}

int	output_header_file(FILE* Hfile) {
	map<string, vector<constr_spec> >::iterator constr_iter;

	string genericExp;
	if(full_item::DSL() == "aaf") {
		genericExp = "parsedExp";
	} else {
		genericExp = full_item::DSL() + "Exp";
	}

	fprintf(Hfile, "#ifndef _GEN_PARSED_EXP_H_\n");
	fprintf(Hfile, "#define _GEN_PARSED_EXP_H_\n");

	//
	// Define constructor differentiator classes
	//
	fprintf(Hfile, "\n");

	char diff_buf[80];
	for(int n = 0; n < MaxSignatures; n++) {
		sprintf(diff_buf, "Differentiator_%d", n);
		fprintf(Hfile, "class %s {\npublic:\n", diff_buf);
		fprintf(Hfile, "\t%s(int d) : id(d) {}\n", diff_buf);
		fprintf(Hfile, "\tint id;\n};\n\n");
	}

	//
	// Declare all pE subclasses to allow constructor specifics to
	// reference them without error
	//
	for(	constr_iter = pE_constructors().begin();
		constr_iter != pE_constructors().end();
		constr_iter++) {
		fprintf(Hfile, "class %s;\n", constr_iter->first.c_str());
	}
	fprintf(Hfile, "\n\n");

	//
	// Define grammar constructors
	//
	for(	constr_iter = pE_constructors().begin();
		constr_iter != pE_constructors().end();
		constr_iter++) {
		vector<constr_spec>&	specs = constr_iter->second;
		const char* constr = constr_iter->first.c_str();
		map<string, bool>	class_members;
		map<string, bool>	arg_names_used;
		map<string, string>::iterator base_class_iter
			= base_class().find(constr_iter->first);
		string			base_class_name = "pE";

		if(base_class_iter != base_class().end()) {
			base_class_name = base_class_iter->second;
		}

		//
		// Write class header
		//
		fprintf(
			Hfile,
			"\nclass %s: public %s {\n",
			constr, base_class_name.c_str());
		fprintf(Hfile, "\tpublic:\n");

		//
		// Write main constructor
		//
		for(int i = 0; i < specs.size(); i++) {
			constr_spec&	cs = specs[i];
			sprintf(diff_buf, "Differentiator_%d", i);

			write_main_constructor(
					constr_iter->first,
					true,
					cs,
					diff_buf,
					Hfile);

			//
			// add members to match the constructor parameters
			//
			string mainname = cs.argnames[cs.mainIndex];
			if(is_a_token(mainname)) {
				mainname = all_lower_case(mainname);
			}
			arg_names_used[mainname] = true;
			class_members[mainname] = true;
			for(int j = 0; j < cs.argnames.size(); j++) {
				if(j != cs.mainIndex) {
					string argname = cs.argnames[j];
					if(is_a_token(argname)) {
						argname = all_lower_case(argname);
					}
					if(arg_names_used.find(argname) != arg_names_used.end()) {
						char buf[1024];
						sprintf(buf, "%s_%d", argname.c_str(), j);
						argname = buf;
					}
					class_members[argname] = true;
					arg_names_used[argname] = true;
				}
			}
		}

		vector<string> vec;
		map<string, bool>::iterator it3;
		for(it3 = class_members.begin(); it3 != class_members.end(); it3++) {
			vec.push_back(it3->first);
		}
		pE_class_members()[constr_iter->first] = vec;

		//
		// Write boilerplate destructor
		//
		map<string, string>::iterator destr_iterator
				= special_destructor().find(constr);
		if(destr_iterator == special_destructor().end()) {
			fprintf(Hfile, "\t~%s() {}\n", constr);
		} else {
			fprintf(Hfile, "\t~%s();\n", constr);
		}

		//
		// Declare copy constructor
		//
		fprintf(Hfile, "\t%s(const %s& p);\n", constr, constr);

		//
		// Declare shallow copy constructor
		//
		fprintf(Hfile, "\t%s(bool, const %s& p);\n", constr, constr);

		//
		// Declare recursive method
		//
		fprintf(Hfile, "\tvoid recursively_apply(exp_analyzer& EA) override;\n");

		//
		// Define deep copy method
		//
		fprintf(Hfile, "\tvirtual pE* copy() override {\n");
		fprintf(Hfile, "\t\treturn new %s(*this);\n", constr);
		fprintf(Hfile, "\t}\n");

		//
		// Define shallow copy method
		//
		fprintf(Hfile, "\tvirtual pE* shallow_copy() override {\n");
		fprintf(Hfile, "\t\treturn new %s(true, *this);\n", constr);
		fprintf(Hfile, "\t}\n");

		//
		// Declare boilerplate methods: initExpression, eval_expression
		//
		fprintf(Hfile, "\tvirtual void	initExpression(\n");
		fprintf(Hfile, "\t\t\t\tconst Cstring& nodeData) override;\n");

		fprintf(Hfile, "\tvirtual void eval_expression(\n");
		if(full_item::DSL() == "aaf") {
			fprintf(Hfile, "\t\tbehaving_base* loc,\n");
			fprintf(Hfile, "\t\tTypedValue&	result) override;\n");
		} else {
			fprintf(Hfile, "\t\tTypedValue&	result) override;\n");
		}

		//
		// add the special members and methods for this class - these
		// were obtained by parsing constructor-specific.txt
		//
		map<string, string>::iterator extra_iterator
			= extra_members_and_methods().find(constr_iter->first);
		if(extra_iterator != extra_members_and_methods().end()) {
			fprintf(Hfile, "%s", extra_iterator->second.c_str());
		}
		for(map<string, bool>::iterator	member_iter = class_members.begin();
			member_iter != class_members.end();
			member_iter++) {
			fprintf(Hfile, "\t%s\t%s;\n", genericExp.c_str(), member_iter->first.c_str());
		}

		//
		// write spell() method
		//
		fprintf(Hfile, "\tvirtual const char* spell() const override {\n");
		fprintf(Hfile, "\t\treturn \"%s\";\n", constr);
		fprintf(Hfile, "\t}\n");
		fprintf(Hfile, "};\n");
	}
	fprintf(Hfile, "#endif /* _GEN_PARSED_EXP_H_ */\n");
	return 0;
}

int	output_source_files(
		FILE*	Cfile	// generated source (usable as is)
		) {
	map<string, vector<constr_spec> >::iterator	constr_iter;
	map<string, string>::iterator			initializer_iter;
	char diff_buf[80];

	string genericExp;
	if(full_item::DSL() == "aaf") {
		genericExp = "parsedExp";
	} else {
		genericExp = full_item::DSL() + "Exp";
	}

	for(	constr_iter = pE_constructors().begin();
		constr_iter != pE_constructors().end();
		constr_iter++) {
		vector<constr_spec>&	specs = constr_iter->second;
		const char*		constr = constr_iter->first.c_str();
		map<string, string>::iterator base_class_iter
			= base_class().find(constr_iter->first);
		string			base_class_name = "pE";

		if(base_class_iter != base_class().end()) {
			base_class_name = base_class_iter->second;
		}

		for(int i = 0; i < specs.size(); i++) {
			constr_spec&	cs = specs[i];
			sprintf(diff_buf, "Differentiator_%d", i);

			write_main_constructor(
					constr_iter->first,
					false,
					cs,
					diff_buf,
					Cfile);
		}

		// map<string, string>::iterator initializer_iter;
		map<string, string>::iterator copy_cons_iterator
				= special_copy_constructor().find(constr_iter->first);
		map<string, string>::iterator destr_iterator
				= special_destructor().find(constr);
		map<string, vector<string> >::iterator class_member_iter
				= pE_class_members().find(constr_iter->first);
		vector<string>& class_members = class_member_iter->second;

		if(copy_cons_iterator == special_copy_constructor().end()) {

			//
			// default copy constructor
			//
			fprintf(Cfile, "%s::%s(const %s& p)\n", constr, constr, constr);
			fprintf(Cfile, "\t: %s(origin_info(p.line, p.file)) {\n", base_class_name.c_str());
			fprintf(Cfile, "\tfor(int i = 0; i < p.expressions.size(); i++) {\n");
			fprintf(Cfile, "\t\tif(p.expressions[i]) {\n");
			fprintf(Cfile, "\t\t\texpressions.push_back(%s(p.expressions[i]->copy()));\n", genericExp.c_str());
			fprintf(Cfile, "\t\t} else {\n");
			fprintf(Cfile, "\t\t\texpressions.push_back(%s());\n", genericExp.c_str());
			fprintf(Cfile, "\t\t}\n");
			fprintf(Cfile, "\t}\n");

			//
			// Let's apply the method to all known data members,
			// including those in expressions[]:
			//

			for(int i = 0; i < class_members.size(); i++) {
				fprintf(Cfile, "\tif(p.%s) {\n", class_members[i].c_str());
				fprintf(Cfile, "\t\t%s.reference(p.%s->copy());\n",
							class_members[i].c_str(),
							class_members[i].c_str());
				fprintf(Cfile, "\t}\n");
			}

			//
			// Now we can invoke initExpression
			//

			fprintf(Cfile, "\tinitExpression(p.getData());\n");
			fprintf(Cfile, "}\n");
		} else {
			fprintf(Cfile, "%s", copy_cons_iterator->second.c_str());
		}

		map<string, string>::iterator shallow_cons_iterator
				= special_shallow_constructor().find(constr_iter->first);

		if(shallow_cons_iterator == special_shallow_constructor().end()) {

			//
			// default shallow copy constructor
			//
			fprintf(Cfile, "%s::%s(bool, const %s& p)\n", constr, constr, constr);
			fprintf(Cfile, "\t: %s(origin_info(p.line, p.file))", base_class_name.c_str());
			if(class_members.size()) {
				fprintf(Cfile, ",\n");
			} else {
				fprintf(Cfile, " {\n");
			}

			//
			// Let's apply the method to all known data members,
			// including those in expressions[]:
			//

			for(int i = 0; i < class_members.size(); i++) {
				fprintf(Cfile, "\t\t%s(p.%s)", class_members[i].c_str(),
						class_members[i].c_str());
				if(i < class_members.size() - 1) {
					fprintf(Cfile, ",\n");
				} else {
					fprintf(Cfile, " {\n");
				}
			}

			fprintf(Cfile, "\texpressions = p.expressions;\n");

			//
			// Now we can invoke initExpression
			//

			fprintf(Cfile, "\tinitExpression(p.getData());\n");
			fprintf(Cfile, "}\n");
		} else {
			fprintf(Cfile, "%s", shallow_cons_iterator->second.c_str());
		}

		map<string, string>::iterator recurse_iterator
				= special_recursion_method().find(constr_iter->first);
		if(recurse_iterator == special_recursion_method().end()) {
			fprintf(Cfile, "void %s::recursively_apply(exp_analyzer& EA) {\n", constr);
			fprintf(Cfile, "\tEA.pre_analyze(this);\n");
			fprintf(Cfile, "\tfor(int i = 0; i < expressions.size(); i++) {\n");
			fprintf(Cfile, "\t\tif(expressions[i]) {\n");
			fprintf(Cfile, "\t\t\texpressions[i]->recursively_apply(EA);\n");
			fprintf(Cfile, "\t\t}\n");
			fprintf(Cfile, "\t}\n");

			//
			// Let's apply the method to all known data members:
			//

			for(int i = 0; i < class_members.size(); i++) {
				fprintf(Cfile, "\tif(%s) {\n", class_members[i].c_str());
				fprintf(Cfile, "\t\t%s->recursively_apply(EA);\n", class_members[i].c_str());
				fprintf(Cfile, "\t}\n");
			}

			//
			// Now we can invoke the analyzer on this:
			//

			fprintf(Cfile, "\tEA.post_analyze(this);\n");
			fprintf(Cfile, "}\n");
		} else {
			fprintf(Cfile, "%s", recurse_iterator->second.c_str());
		}


		if(destr_iterator != special_destructor().end()) {
			fprintf(Cfile, "%s", destr_iterator->second.c_str());
		}

		map<string, string>::iterator initExp_iterator
				= initExpression().find(constr_iter->first);
		if(initExp_iterator == initExpression().end()) {
			fprintf(Cfile, "void	%s::initExpression(\n", constr);
			fprintf(Cfile, "\t\t\tconst Cstring& nodeData) {\n");
			fprintf(Cfile, "\ttheData = nodeData;\n");
			fprintf(Cfile, "}\n");
		} else {
			fprintf(Cfile, "%s", initExp_iterator->second.c_str());
		}

		map<string, string>::iterator eval_iter = eval_expression().find(constr_iter->first);
		if(eval_iter == eval_expression().end()) {
			fprintf(Cfile, "void	%s::eval_expression(\n", constr);
			if(full_item::DSL() == "aaf") {
				fprintf(Cfile, "\t\tbehaving_base* loc,\n");
				fprintf(Cfile, "\t\tTypedValue& result) {\n");
			} else {
				fprintf(Cfile, "\t\tTypedValue& result) {\n");
			}
			fprintf(Cfile, "}\n");
		} else {
			fprintf(Cfile, "%s", eval_iter->second.c_str());
		}
	}
	return 0;
}
