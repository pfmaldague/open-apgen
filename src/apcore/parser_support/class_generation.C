#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "aafReader.H"
#include "apcoreWaiter.H"
#include "ParsedExpressionSystem.H"
#include "AbsResTemplates.H"

using namespace pEsys;

static FILE* cpp_abs_res_C = NULL;
static FILE* cpp_abs_res_2_C = NULL;
static FILE* cpp_abs_res_H = NULL;
static FILE* cpp_experimental_C = NULL;

#ifdef have_AAF_compiler

#include "adapt_abs_res.H"

//
// prerequisite for the abstract usage factory:
//
#include "adapt_abs_res_2.C"

//
// This file, if not empty, will define the
// GENERATED_ABSTRACT_USAGE_FACTORY macro
// and the associated factory method:
//
#include "adapt_abs_res.C"

#endif /* have_AAF_compiler */

#ifndef GENERATED_ABSTRACT_USAGE_FACTORY

//
// Dummy factory; NULL result will be ignored in consolidation code
//
AbstractUsage* AbstractUsage::AbstractUsageFactory(Usage*) {
	return NULL;
}
#endif /* GENERATED_ABSTRACT_USAGE_FACTORY */

#ifndef GENERATED_ABSTRACT_RES_FACTORY

//
// Dummy factory; NULL result will be ignored in consolidation code
//
behaving_object* behaving_object::AbstractResFactory(task&) {
	return NULL;
}
#endif /* GENERATED_ABSTRACT_RES_FACTORY */

void ResourceDef::generate_abstract_usage_factory(const Cstring& resname) {
    FILE* dotC = cpp_abs_res_C;
    aoString s;
    static bool first_time = true;

    if(first_time) {
	first_time = false;
	s << "#define GENERATED_ABSTRACT_USAGE_FACTORY\n\n"
	  << "AbstractUsage* AbstractUsage::AbstractUsageFactory(Usage* u) {\n"
	  << "    static map<Cstring, AbstractUsage* (*)(Usage*)> M;\n"
	  << "    static bool first = true;\n"
	  << "    ResUsageWithArgs* resusage = dynamic_cast<ResUsageWithArgs*>(\n"
	  << "\t\tu->resource_usage_with_arguments.object());\n"
	  << "    Cstring name = resusage->getData();\n"
	  << "    if(first) {\n"
	  << "\tfirst = false;\n";
    }
    s << "\tM[" << addQuotes(resname) << "] = mini_factory_" << resname << ";\n";
    Cstring t(s.str());
    fwrite(*t, t.length(), 1, dotC);
}

void ResourceDef::generate_abstract_usage_mini_factory(const Cstring& resname) {
    FILE* dotC = cpp_abs_res_2_C;
    aoString s;
    s << "AbstractUsage* mini_factory_" << resname << "(Usage* u) {\n"
      << "\treturn new AbsUsageTemplate<" << resname << "_obj>(*u);\n"
      << "}\n";

    //
    // Let's also implement the execute() method of the
    // compiled abstract resource here
    //

    Cstring t(s.str());
    fwrite(*t, t.length(), 1, dotC);
}

void ResourceDef::generate_abstract_behavior_header(
			const Behavior&	res_type) {
    FILE* dotH = cpp_abs_res_H;
    aoString s;

    //
    // This class is the master object representing the compiled version
    // of an AAF-defined abstract resource.
    //
    // The two key challenges of defining this class are
    //
    // 	(1) define a complete set of global and local variables
    // 	    based on the static abstract resource object defined
    // 	    during consolidation (for globals) and the
    // 	    behaving_object base class (for local variables).
    //
    // 	(2) define an execute method that is a C++ implementation
    // 	    of the 'resource usage' or 'modeling' section found
    // 	    in the AAF adaptation.
    //
    s << "\n// automatically generated abstract resource class \""
      << res_type.name << "\"\n";
    s	<< "class " << res_type.name << "_obj: public abs_res_object {\n"
	<< "public:\n";

    //
    // Declare references to parameters here
    //
    task*	modeling_task = res_type.tasks[1];

    s << "// using task \"" << modeling_task->name
	 << "\" to generate header for abs. res. "
	 << res_type.name << "\n";

    for(int z = 0; z < modeling_task->paramindex.size(); z++) {
	int p_index = modeling_task->paramindex[z];
	s << "// \tP[" << z << "]: "
	  << apgen::spellC(modeling_task->get_varinfo()[p_index].second)
	  << " "
	  << modeling_task->get_varinfo()[p_index].first << "\n";
    }

    //
    // Do not define the call parameters as class members. They will
    // be supplied by the call.
    //
    for(int z = (modeling_task->paramindex.size() + 1); z < modeling_task->get_varinfo().size(); z++) {
	s << "\tTypedValue&\t" << modeling_task->get_varinfo()[z].first << ";\n";
    }

    s	<< "    " << res_type.name << "_obj(const Behavior& T,\n"
	<< "\t\tbehaving_object* parent_scope,\n"
	<< "\t\tbehaving_element& parent_context)\n"
	<< "\t: abs_res_object(T, 1, parent_scope)";

    //
    // Retrieve the variables (not including parameters) of the
    // modeling method and initialize references to them
    //
    if(modeling_task->get_varinfo().size() > (modeling_task->paramindex.size() + 1)) {
	s << ",\n";
    }
    for(int z = (modeling_task->paramindex.size() + 1); z < modeling_task->get_varinfo().size(); z++) {
	s << "\t\t" << modeling_task->get_varinfo()[z].first
	    << "((*this)[" << z << "])";
	if(z < modeling_task->get_varinfo().size() - 1) {
	    s << ",\n";
	} else {
	    s << "\n";
	}
    }
    s   << "\t{\n";
    s	<< "    }\n";

    s	<< "    " << res_type.name << "_obj(const " << res_type.name
	<< "_obj&) = delete;\n"
	<< "    ~" << res_type.name << "_obj() {}\n";

    //
    // For use in an "interpretive" mode:
    //
    s	<< "    void\texecute(execution_context::return_code&, pEsys::execStack* = NULL);\n";

    //
    // For use in a "compiled" mode:
    //
    s   << "    void execute_C(";
    for(int z = (modeling_task->paramindex.size() + 1); z < modeling_task->get_varinfo().size(); z++) {
	if(z > (modeling_task->paramindex.size() + 1)) {
	    s << ", ";
	}
	s << spellC(modeling_task->get_varinfo()[z].second) << " " << modeling_task->get_varinfo()[z].first;
    }
    s   << ");\n";

    s << "};\n";
    Cstring t(s.str());
    fwrite(*t, t.length(), 1, dotH);
}

void FunctionDefinition::generate_behavior_header(const task& T) {
#ifdef OBSOLETE
    FILE* dotH = cpp_abs_res_H;
    aoString s;
    Program* p = T.prog.object();

    s << "extern ";
    s << apgen::spellC(p->ReturnType) << " " << T.name << "(";
    for(int z = 0; z < T.paramindex.size(); z++) {
	int p_index = T.paramindex[z];
	if(z > 0) {
	    s << ", ";
	}
	s << apgen::spellC(T.get_varinfo()[p_index].second)
	  << " "
	  << T.get_varinfo()[p_index].first;
    }
    s << ");\n";
    Cstring t(s.str());
    fwrite(*t, t.length(), 1, dotH);
#endif /* OBSOLETE */
}

void FunctionDefinition::generate_behavior_body(const task& T) {
    aoString s;
    FILE* dotH = cpp_abs_res_H;
    s << "\n// automatically generated function class \""
      << T.name << "\"\n";
    s << "class " << T.name << " {\npublic:\n";

    s << "    // internal variables of this function:\n";
    for(int z = 0; z < T.get_varinfo().size(); z++) {
	const pair<Cstring, apgen::DATA_TYPE>& P1 = T.get_varinfo()[z];

	//
	// Do not include parameters
	//
	if(z == 0 || z > T.paramindex.size()) {
	    s << "    " << apgen::spellC(P1.second) << "\t" << P1.first << ";\n";
	}
    }
    s << "\n    " << T.name << "() = default;\n";
    s << "    virtual ~" << T.name << "() {}\n";

    Program* p = T.prog.object();

    s << "\n    // Method for use by other generated functions:\n    ";
    s << apgen::spellC(p->ReturnType) << " execute(";
    for(int z = 0; z < T.paramindex.size(); z++) {
	int p_index = T.paramindex[z];
	if(z > 0) {
	    s << ", ";
	}
	s << apgen::spellC(T.get_varinfo()[p_index].second)
	  << " "
	  << T.get_varinfo()[p_index].first;
    }
    s << ");\n";

    s << "\n    // Static udef-like function for use by AAF programs:\n";
    s	<< "    static apgen::RETURN_STATUS " << T.name << "_udef(\n"
	<< "\tCstring& errs,\n\tTypedValue* result,\n\tslst<TypedValue*>& args);\n";

    s << "};\n";
    Cstring	t(s.str());
    fwrite(*t, t.length(), 1, dotH);

    FILE* dotC = cpp_experimental_C;

    s << "\n" << apgen::spellC(p->ReturnType) << " " << T.name
      << "::execute(";
    for(int z = 0; z < T.paramindex.size(); z++) {
	int p_index = T.paramindex[z];
	if(z > 0) {
	    s << ", ";
	}
	s << apgen::spellC(T.get_varinfo()[p_index].second)
	  << " "
	  << T.get_varinfo()[p_index].first;
    }
    s << ") {\n";
    p->to_stream(&s, 4);
    s << "}\n";

    s << "\n// Static udef-like function for use by AAF programs:\n";
    s	<< "apgen::RETURN_STATUS " << T.name << "_udef(\n"
	<< "\tCstring& errs,\n\tTypedValue* result,\n\tslst<TypedValue*>& args) {\n";

    s << "\n    // Capture parameters under their actual names:\n";

    for(int z = T.paramindex.size() - 1; z >= 0; z--) {
	int p_index = T.paramindex[z];
	apgen::DATA_TYPE dt = T.get_varinfo()[p_index].second;
	s << "    ";
	Cstring param_name(T.get_varinfo()[p_index].first);
	switch(dt) {
	    case apgen::DATA_TYPE::ARRAY:
		s << "array_ptr " << param_name << "(&args.pop_front()->get_array());\n";
		break;
	    case apgen::DATA_TYPE::BOOL_TYPE:
		s << "bool " << param_name << "args.pop_front()->get_int();\n";
		break;
	    case apgen::DATA_TYPE::INTEGER:
		s << "long int " << param_name << "args.pop_front()->get_int();\n";
		break;
	    case apgen::DATA_TYPE::FLOATING:
		s << "double " << param_name << "args.pop_front()->get_double();\n";
		break;
	    case apgen::DATA_TYPE::STRING:
		s << "Cstring " << param_name << "(args.pop_front()->get_string());\n";
		break;
	    case apgen::DATA_TYPE::TIME:
	    case apgen::DATA_TYPE::DURATION:
		s << "CTime_base " << param_name << "(args.pop_front()->get_time_or_duration());\n";
		break;
	    case apgen::DATA_TYPE::INSTANCE:
		s << "behaving_element " << param_name << "(args.pop_front()->get_instance().object());\n";
		break;
	    case apgen::DATA_TYPE::UNINITIALIZED:
		throw(eval_error("Uninitialized parameter in function call"));
	}
    }

    s << "\n    // Create the object containing the internal function variables:\n";
    s << "    " << T.name << " func_obj;\n";

    s << "\n    // Invoke the execute method:\n";
    s << "    try {\n\t*result = func_obj.execute(";
    for(int z = 0; z < T.paramindex.size(); z++) {
	int p_index = T.paramindex[z];
	if(z > 0) {
	    s << ", ";
	}
	s << T.get_varinfo()[p_index].first;
    }
    s << ");\n    } catch(eval_error Err) {\n\terrs = Err.msg;\n"
      << "\treturn apgen::FAIL;\n"
      << "    }\n"
      << "    return apgen::SUCCESS;\n}\n";
    t = s.str();
    fwrite(*t, t.length(), 1, dotC);
}

void ResourceDef::generate_execute_method(
			const Behavior&	res_type) {
    FILE* expF = cpp_experimental_C;
    aoString s;
    assert(res_type.tasks.size() >= 2);

    //
    // tasks[0] is the constructor
    //
    task* modeling_task = res_type.tasks[1];

    //
    // method definition start
    //
    s << "void " << res_type.name << "::execute(";

    for(int z = 0; z < modeling_task->paramindex.size(); z++) {
	int p_index = modeling_task->paramindex[z];
	if(z > 0) {
	    s << ", ";
	}
	s << apgen::spellC(modeling_task->get_varinfo()[p_index].second)
	  << " " << modeling_task->get_varinfo()[p_index].first;
    }
    s << ") {\n";
    modeling_task->prog->to_stream(&s, /* indentation = */ 4);
    s << "}\n\n";
    Cstring t(s.str());
    fwrite(*t, t.length(), 1, expF);
}

bool create_files_generated_during_consolidation(Cstring& any_errors) {
    cpp_abs_res_C = fopen("adapt_abs_res.C", "w");
    if(!cpp_abs_res_C) {
	any_errors << "Cannot create file \"adapt_abs_res.C\"\n";
	return false;
    }
    cpp_abs_res_2_C = fopen("adapt_abs_res_2.C", "w");
    if(!cpp_abs_res_2_C) {
	any_errors << "Cannot create file \"adapt_abs_res_2.C\"\n";
	return false;
    }
    cpp_abs_res_H = fopen("adapt_abs_res.H", "w");
    if(!cpp_abs_res_H) {
	any_errors << "Cannot create file \"adapt_abs_res.H\"\n";
	return false;
    }
    cpp_experimental_C = fopen("adapt_experimental.C", "w");
    if(!cpp_experimental_C) {
	any_errors << "Cannot create file \"adapt_experimental.C\"\n";
	return false;
    }

    return true;
}

void clean_up_files_generated_during_consolidation() {
	FILE*& f_source = cpp_abs_res_C;
	FILE*& f_source_2 = cpp_abs_res_2_C;
	FILE*& f_header = cpp_abs_res_H;
	FILE*& f_exp = cpp_experimental_C;
	if(f_source) {
		aoString s;
		s << "    }\n    return (*M[name])(u);\n}\n";
		Cstring t(s.str());
		fwrite(*t, t.length(), 1, f_source);
		fclose(f_source);
		f_source = NULL;
	}
	if(f_source_2) {
		fclose(f_source_2);
		f_source_2 = NULL;
	}
	if(f_header) {
		fclose(f_header);
		f_header = NULL;
	}
	if(f_exp) {
		fclose(f_exp);
		f_exp = NULL;
	}
}
