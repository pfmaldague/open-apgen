#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/stat.h>
#include "aafReader.H"
#include "ACT_exec.H"
#include "ActivityInstance.H"
#include "APmodel.H"
#include "apcoreWaiter.H"
#include "BehavingElement.H"
#include "fileReader.H"
#include "lex_intfc.H"
#include "RES_def.H"

using namespace pEsys;

void Array::consolidate(int dent) {

	//
	// rest_of_list is an expression_list, keyword_value_pairs or null
	//

	ExpressionList* el = dynamic_cast<ExpressionList*>(rest_of_list.object());
	Assignment* kw = dynamic_cast<Assignment*>(rest_of_list.object());
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent + 2) << "consolidating Array of type TBD\n";
	}
	if(el) {
		el->consolidate(actual_values, dent + 2);
		is_list = true;
		is_struct = false;
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent + 2) << "Array type = list\n";
		}
		return;
	} else if(kw) {
		aafReader::consolidate_kw_pairs(*kw, actual_keys, actual_values, dent + 2);
		is_list = false;
		is_struct = true;
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent + 2) << "Array type = struct\n";
		}
		return;
	} else {
		is_list = false;
		is_struct = false;
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent + 2) << "Array type = empty\n";
		}
	}
}

void ArrayList::consolidate_one_element(int dent, parsedExp& pe, const Cstring& the_resource) {
	Symbol*	sym = dynamic_cast<Symbol*>(pe.object());
	Array*	arr = dynamic_cast<Array*>(pe.object());
	if(sym) {
		aafReader::consolidate_symbol(pe, dent + 2);

		//
		// Define sym again, because it's been replaced by a derived
		// class instance:
		//
		sym = dynamic_cast<Symbol*>(pe.object());
		assert(sym);

		//
		// remember that consolidate_symbol can substitute a new object:
		//

		SymbolicArray* symarray = dynamic_cast<SymbolicArray*>(pe.object());
		if(!symarray) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": element " << pe->getData()
				<< " is of type " << pe->spell()
				<< "; expected an array";
			throw(eval_error(err));
		}
		val.undefine();
		try {
			symarray->eval_array(
				behaving_object::GlobalObject(),
				val);
		} catch(eval_error Err) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": error evaluating resource index array "
				<< sym->getData() << "; details:\n"
				<< Err.msg;
			throw(eval_error(err));
		}

#ifdef NOT_YET
		if(val.modifiers != 0) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": index array " << sym->getData()
				<< " of resource " << the_resource
				<< " is not constant. Cannot use it for indices, "
				<< "because the adaptation could change it at any time.\n";
			err << "To avoid this error, use make_constant() in the "
				<< "last function that modified this array.\n";
			symNode* sn = aafReader::assignments_to_global_arrays().find(
					sym->getData());
			if(sn) {
				err << "Assignments to this array were found at\n"
					<< sn->payload;
			}
			throw(eval_error(err));
		}
#endif /* NOT_YET */
		if(!aafReader::globals_used_as_indices_in_resources().find(sym->getData())) {
			aafReader::globals_used_as_indices_in_resources()
				<< new symNode(sym->getData(), the_resource);
		}
		evaluated_elements.push_back(val);
	} else if(arr) {
		arr->consolidate(dent + 2);
		val.undefine();
		try {
			arr->eval_expression(
				behaving_object::GlobalObject(),
				val);
		} catch(eval_error Err) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": error evaluating array element "
				<< arr->to_string() << "; details:\n"
				<< Err.msg;
			throw(eval_error(err));
		}
		evaluated_elements.push_back(val);
	}
}

void ArrayList::consolidate(int dent, const Cstring& the_resource) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent + 2) << "consolidating ArrayList\n";
	}
	consolidate_one_element(dent + 2, one_array, the_resource);
	for(int i = 0; i < expressions.size(); i++) {
		consolidate_one_element(dent + 2, expressions[i], the_resource);
	}
}


void ByValueSymbol::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating ByValueSymbol " << getData() << "\n";
	}
	if(!aafReader::TaskForStoringArgsPassedByValue().is_defined()) {
		Cstring errs;
		errs 	<< "File " << file << ", line " << line << ": "
			<< "Attempting to use passed-by-value argument *"
			<< getData() << " outside of arguments list.\n\n"
			<< "NOTE: arrayed resource indices do not need to be passed "
			<< "by value since they are computed before dispatching "
			<< "the usage event to the event queue.";
		throw(eval_error(errs));
	}
	my_task = aafReader::get_current_task();
	Behavior&	cur_type = aafReader::CurrentType();
	map<Cstring, int>::const_iterator index_of_var_in_src_table
				= my_task->get_varindex().find(tok_id->getData());
	if(index_of_var_in_src_table == my_task->get_varindex().end()) {
		Cstring err;
		err << "Could not find *" << tok_id->getData()
			<< " in task " << my_task->full_name();
		throw(eval_error(err));
	}
	my_index = index_of_var_in_src_table->second;
	apgen::DATA_TYPE new_data_type = my_task->get_varinfo()[my_index].second;
	map<Cstring, int>::iterator	new_task_ptr
			= cur_type.taskindex.find(aafReader::TaskForStoringArgsPassedByValue());
	task*		use_clause_task = NULL;
	if(new_task_ptr == cur_type.taskindex.end()) {
		my_use_task_index = cur_type.add_task(aafReader::TaskForStoringArgsPassedByValue());
	} else {
		my_use_task_index = new_task_ptr->second;
	}

	use_clause_task = cur_type.tasks[my_use_task_index];

	Cstring		asterisked_sym;
	asterisked_sym << "*" << tok_id->getData();

	map<Cstring, int>::const_iterator index_of_var_in_usage_task
				= use_clause_task->get_varindex().find(asterisked_sym);
	if(index_of_var_in_usage_task == use_clause_task->get_varindex().end()) {
		my_index_in_use_task = use_clause_task->add_variable(
							asterisked_sym,
							new_data_type);
	} else {
		my_index_in_use_task = index_of_var_in_usage_task->second;
	}

	my_level = aafReader::LevelOfCurrentTask;
	assert(my_level == 2);

	//
	// tell Usage::consolidate() that something was passed by value:
	//

	aafReader::AVariableWasPassedByValue = true;
}

void CustomAttributes::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Custom Attributes\n";
	}
	Declaration* decl;
	for(int i = -1; i < (int)custom_decls->expressions.size(); i++) {
		if(i == -1) {
			decl = dynamic_cast<Declaration*>(custom_decls.object());
		} else {
			decl = dynamic_cast<Declaration*>(custom_decls->expressions[i].object());
		}
		if(decl) {

			// debug
			// cerr << aafReader::make_space(dent + 2)
			// 	<< "consolidating custom decl " << decl->to_string();

			DataType* dt = dynamic_cast<DataType*>(
				decl->param_scope_and_type.object());
			assert(dt);
			dt->consolidate(dent + 2);
			decl->val = dt->val;
			decl->val.declared_type = dt->val.declared_type;

			assert(dt->val.declared_type != apgen::DATA_TYPE::UNINITIALIZED);
			aafReader::add_a_custom_type(decl->tok_id->getData(), dt->val.declared_type);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << aafReader::make_space(dent + 2) << "type: ";
				if(dt->builtin_type) {
					cerr << dt->builtin_type->getData() << "\n";
				} else if(dt->tok_dyn_type) {
					cerr << dt->tok_dyn_type->getData() << "\n";
				}
			}
		}
	}
}

void DataType::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating DataType for ";
		if(builtin_type) {
			cerr << builtin_type->getData() << "\n";
		} else if(tok_dyn_type) {
			cerr << tok_dyn_type->getData() << "\n";
		}
	}
	
	if(builtin_type) {
		val.generate_default_for_type(builtin_type->getData());
		if(builtin_type->getData() == "instance") {
			val.declared_type = apgen::DATA_TYPE::INSTANCE;
		} else {
			val.declared_type = val.get_type();
		}
	} else if(tok_dyn_type) {
		val.generate_default_for_type(tok_dyn_type->getData());
		val.declared_type = apgen::DATA_TYPE::ARRAY;
	}
}

namespace aafReader {

bool array_is_homogeneous(
		parsedExp& Expression,
		apgen::DATA_TYPE& element_type,
		int& array_size,
		int& dimensions,
		int dent) {
	Array* array = dynamic_cast<Array*>(Expression.object());
	if(!array) {
		//
		// could be a function call
		//

		return false;
	}
	ExpressionList* el = dynamic_cast<ExpressionList*>(array->rest_of_list.object());
	if(!el) {
		return false;
	}
	array_size = 0;
	dimensions = 1;

	//
	// Array has already been consolidated:
	//

	// vector<parsedExp> args;
	// el->consolidate(args, dent + 2);

	int sub_dimensions = 0;
	apgen::DATA_TYPE homogeneous_type = apgen::DATA_TYPE::UNINITIALIZED;
	for(int i = 0; i < array->actual_values.size(); i++) {
		if(i == 0) {
			homogeneous_type = array->actual_values[i]->get_result_type();
			if(homogeneous_type == apgen::DATA_TYPE::ARRAY) {
				if(!array_is_homogeneous(
					array->actual_values[i],
					element_type,
					array_size,
					sub_dimensions,
					dent + 2)) {
					return false;
				}
				dimensions = sub_dimensions + 1;
			} else {
				element_type = homogeneous_type;
			}
		} else {
			if(element_type == apgen::DATA_TYPE::FLOATING || element_type == apgen::DATA_TYPE::INTEGER) {
				switch(array->actual_values[i]->get_result_type()) {
					case apgen::DATA_TYPE::INTEGER:
					case apgen::DATA_TYPE::FLOATING:
						break;
					default:
						return false;
				}
			} else {
				if(array->actual_values[i]->get_result_type() != element_type) {
					return false;
				}
			}
			if(homogeneous_type == apgen::DATA_TYPE::ARRAY) {
				int sub_array_size;
				apgen::DATA_TYPE sub_type = apgen::DATA_TYPE::UNINITIALIZED;
				if(!array_is_homogeneous(
					array->actual_values[i],
					sub_type,
					sub_array_size,
					sub_dimensions,
					dent + 2)) {
					return false;
				}
				if(sub_array_size != array_size) {
					return false;
				}
				if(sub_dimensions != dimensions - 1) {
					return false;
				}
				
				if(element_type == apgen::DATA_TYPE::FLOATING || element_type == apgen::DATA_TYPE::INTEGER) {
					switch(sub_type) {
						case apgen::DATA_TYPE::INTEGER:
						case apgen::DATA_TYPE::FLOATING:
							break;
						default:
							return false;
					}
				} else {
					if(sub_type != element_type) {
						return false;
					}
				}
			}
		}
	}
	array_size = array->actual_values.size();
	return true;
}
} // namespace aafReader

void Declaration::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Declaration for "
			<< tok_id->getData() << "\n";
	}
	DataType* dt = dynamic_cast<DataType*>(param_scope_and_type.object());
	assert(dt);
	dt->consolidate(dent + 2);
	val = dt->val;
	val.declared_type = dt->val.declared_type;

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent + 2) << "type: ";
		if(dt->builtin_type) {
			cerr << dt->builtin_type->getData() << "\n";
		} else if(dt->tok_dyn_type) {
			cerr << dt->tok_dyn_type->getData() << "\n";
		}
	}

	try {
		int var_index = aafReader::get_current_task()->add_variable(
			tok_id->getData(),
			val.declared_type);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line
			<< ": error found while exercising declaration -\n"
			<< Err.msg;
		throw(eval_error(err));
	}

	//
	// execute() will want to stick the rhs into the lhs.
	// We need to make sure that the lhs is a properly defined
	// Symbol - the right way to do this is to call
	// consolidate(Symbol).
	//

	Cstring name = tok_id->getData();

	parsedExp	nameExp(new pE_w_string(origin_info(line, file), name));
	Symbol*	lhs = new Symbol(0, nameExp, Differentiator_0(0));
	tok_id.dereference();

	//
	// Note that this substitution works with to_stream() because sym->theData
	// is set to the tok_id's data by the Symbol constructor
	//

	tok_id.reference(lhs);
	aafReader::consolidate_symbol(tok_id, dent + 2);

	//
	// Next we consolidate the rhs. However, there is a twist - we should make
	// sure the type of Expression, which is really the default value for the
	// variable being defined, agrees with the specified data type. Think of
	// the case of a declaration that reads
	//
	// 	float F default to 0;
	//

	aafReader::consolidate_expression(Expression, dent + 2, val.declared_type);

	//
	// Before handling the default case, we want to optimize the case of
	// homogeneous arrays.
	//

	if(val.declared_type == apgen::DATA_TYPE::ARRAY) {
		apgen::DATA_TYPE element_type = apgen::DATA_TYPE::UNINITIALIZED;
		int array_size = 0;
		int dimensions = 0;
		if(aafReader::array_is_homogeneous(
			Expression,
			element_type,
			array_size,
			dimensions,
			dent + 2)) {

			//
			// fetch the current task and set the appropriate
			// element of its homogeneoustype member
			//

			if(APcloptions::theCmdLineOptions().debug_grammar) {
			    cerr << aafReader::make_space(dent) << "consolidate declaration: setting homogeneous type of "
				<< tok_id->getData() << " to "
				<< apgen::spell(element_type) << "; dimension is " << dimensions << "\n";
			}

			aafReader::get_current_task()->homogeneoustype[tok_id->getData()]
				= pair<apgen::DATA_TYPE, int>(element_type, dimensions);
		} else {
			if(APcloptions::theCmdLineOptions().debug_grammar) {
			    cerr << aafReader::make_space(dent) << "consolidate declaration: array "
				<< tok_id->getData() << " is inhomogeneous or not detailed; cannot optimize.\n";
			}
		}
	}
}

void Decomp::consolidate_temporal_spec(
		apgen::METHOD_TYPE	method_type,
		int			dent) {
    Program::ProgStyle	style = Program::compute_type(method_type);

    //
    // Remember that in all cases, the purpose of the temporal spec is
    // to compute a vector of times as efficiently as possible.
    //

    //
    // A decomposition clause can appear in two contexts:
    //
    // 	1. in an activity type decomposition section or
    // 	   decomposition method
    //
    // 	2. in a global decomposition method
    //
    // In case 1, LevelOfCurrentTask is 2; in case 1, it is 1.
    //
    // Note that method_type also provides this information.
    //
    bool	global_method = aafReader::LevelOfCurrentTask == 1;

    if(temporalSpecification) {
	TemporalSpec* ts = dynamic_cast<TemporalSpec*>(temporalSpecification.object());
	if(ts->tok_at) {
		aafReader::consolidate_expression(ts->Expression, dent + 2);
		at_expression = ts->Expression;
		eval_decomp_times = &Decomp::get_time_from_one_expression;
	} else if(ts->tok_from || ts->tok_immediately) {
		Cstring err;
		err << "File " << file << ", line " << line
			<< ": problems trying to consolidate time spec of decomposition clause. "
			<< "Valid time qualifier is 'at'.";
		throw(eval_error(err));
	}
    } else {

	//
	// Be careful about this! The style of the section
	// containing the decomposition clause dictates the
	// default timing. (Nonexclusive) decomposition always
	// use the start of the parent activity as the default.
	// Expansion sections use 'now', which is set dynamically.
	//
	if(global_method) {
	    eval_decomp_times = &pEsys::Decomp::get_time_from_now;
	} else {
	    if(style == Program::ASYNCHRONOUS_STYLE) {
		eval_decomp_times = &pEsys::Decomp::get_time_from_start;
	    } else {
		eval_decomp_times = &pEsys::Decomp::get_time_from_now;
	    }
	}
    }
}

void Decomp::consolidate(apgen::METHOD_TYPE method_type, int dent) {
    Program::ProgStyle style = Program::compute_type(method_type);

    if(method_type != apgen::METHOD_TYPE::DECOMPOSITION
      && method_type != apgen::METHOD_TYPE::NONEXCLDECOMP
      && method_type != apgen::METHOD_TYPE::CONCUREXP) {
	Cstring errs;
	errs << "File " << file << ", line " << line
	     << ": decomposition clause is only allowed "
	     << "in decomposition or expansion sections";
	throw(eval_error(errs));
    }

    Cstring child_type;
    if(tok_act_type) {
	child_type = tok_act_type->getData();
	ExpressionList* el = dynamic_cast<ExpressionList*>(optional_expression_list.object());
	if(el) {
	    el->consolidate(ActualArguments, dent + 2);
	}
	map<Cstring, int>::const_iterator iter
			= Behavior::ClassIndices().find(child_type);
	if(iter == Behavior::ClassIndices().end()) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line
		<< ": decomposition error - " << child_type
		<< " could not be found among activity types";
	    throw(eval_error(errs));
	}
	Type = Behavior::ClassTypes()[iter->second];

	if(ActualArguments.size() != Type->tasks[0]->paramindex.size()) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line
		<< ": decomposing into activity type "
		<< Type->name << " which requires "
		<< Type->tasks[0]->paramindex.size()
		<< " parameter(s), but " << ActualArguments.size()
		<< " is/were supplied.";
	    throw(eval_error(errs));
	}
    } else if(tok_call) {
	ExpressionList* el = dynamic_cast<ExpressionList*>(call_or_spawn_arguments.object());
	assert(el);
	el->consolidate(ActualArguments, dent + 2);

	if(ActualArguments.size() < 2 || ActualArguments.size() > 3) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line
		<< ": " << ActualArguments.size()
		<< " argument(s) found; call() requires 2 or 3";
	    throw(eval_error(errs));
	}

	//
	// Type will be determined at run time
	//

	Type = NULL;
    } else if(tok_spawn) {
	ExpressionList* el = dynamic_cast<ExpressionList*>(call_or_spawn_arguments.object());
	assert(el);
	el->consolidate(ActualArguments, dent + 2);

	if(ActualArguments.size() < 2 || ActualArguments.size() > 3) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line
		<< ": " << ActualArguments.size()
		<< " argument(s) found; spawn() requires 2 or 3";
	    throw(eval_error(errs));
	}

	//
	// Type will be determined at run time
	//

	Type = NULL;
    }

    //
    // The time specification of a decomposition statement is one of the
    // following possibilities:
    //
    //    - 'at' a given time
    //    -	(none)
    //    -	'every' delta 'from' 'to'
    //    -	'every' delta
    //
    // Set method pointer accordingly.
    //

    consolidate_temporal_spec(method_type, dent + 2);

    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << aafReader::make_space(dent) << "consolidating Decomp\n";
    }
}

void Directive::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Directive " << getData() << "\n";
		cerr << "  contents: " << to_string();
	}
	if(assignment) {
		aafReader::consolidate_assignment(assignment, dent + 2, true);
		try {
			// reassign because consolidate_assignment deleted the old assign
			Assignment* assign = dynamic_cast<Assignment*>(assignment.object());
			assert(assign);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << aafReader::make_space(dent) << "evaluating lval of lhs "
					<< assign->lhs_of_assign->to_string() << "\n";
			}
			Constant* con = dynamic_cast<Constant*>(assign->lhs_of_assign.object());
			if(!con) {
				TypedValue& lv = assign->lhs_of_assign->get_val_ref(
						behaving_object::GlobalObject());
				assign->Expression->eval_expression(
					behaving_object::GlobalObject(),
					lv);
			}

			//
			// else, we have a directive with a string-valued lhs
			// which will be processed later
			//

			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << aafReader::make_space(dent) << "finished evaluating Assignment "
					<< assign->to_string() << "\n";
			}
		} catch(eval_error Err) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": problems trying to handle directive "
				"assignment " << one_declarative_assignment->to_string()
				<< "; details:\n" << Err.msg;
			throw(eval_error(err));
		}
	} else {
		Cstring err;
		err << "File " << file << ", line " << line
			<< ": do not know (yet?) how to handle a directive "
			"that is not an assignment";
		throw(eval_error(err));
	}
}

void ExpressionList::consolidate(
		vector<parsedExp>&	args,
		int			dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent)
			<< "consolidating ExpressionList "
			<< getData() << "\n";
		cerr << aafReader::make_space(dent) << to_string() << "\n";
	}

	//
	// The main objective is to extract the 'real' arguments and stick
	// them in a list of expressions, then consolidate each expression.
	//
	// NOTE: an ExpressionList has at least one expression in it.
	//

	aafReader::consolidate_expression(Expression, dent + 2);
	args.push_back(Expression);
	for(int i = 0; i < expressions.size(); i++) {
		bool must_consolidate = true;
		if(expressions[i]->getData() == "(") {
			must_consolidate = false;
		} else if(expressions[i]->getData() == ")") {
			must_consolidate = false;
		} else if(expressions[i]->getData() == ",") {
			must_consolidate = false;
		} else if(expressions[i]->getData() == ";") {
			must_consolidate = false;
		} else if(expressions[i]->getData() == "parameters") {
			must_consolidate = false;
		}
		if(must_consolidate) {
			aafReader::consolidate_expression(expressions[i], dent + 2);
			args.push_back(expressions[i]);
		}
	}
}

void FunctionCall::consolidate(int dent) {
    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << aafReader::make_space(dent)
	     << "consolidating FunctionCall " << getData() << "\n";
    }

    //
    // We already did the work in found_a_function() (lexer_support.C)
    // but we are doing it again here... easier than trying to consolidate
    // all code
    //
    func = NULL;
    TaskInvoked = NULL;

    bfuncnode*		N = aaf_intfc::internalAndUdefFunctions().find(getData());
    apgen::DATA_TYPE	returned_type;
    if(N) {

	//
	// Internal function
	//
	func = N;
	returned_type = func->payload.returntype;
    } else {

	//
	// For better or worse, we have a dual system for storing
	// functions and methods:
	// 
	// 	- aafReader::functions() for AAF-defined functions
	// 	- aafReader::methods() for AAF-defined methods
	// 	- Behavior::GlobalConstructor().tasks for both
	//

	for(int i = aafReader::LevelOfCurrentTask; i >= 0; i--) {
	    task*	cur_task = aafReader::CurrentTasks[i];
	    Behavior*	beh = &cur_task->Type;
	    map<Cstring, int>::iterator	iter = beh->taskindex.find(getData());
	    if(iter != beh->taskindex.end()) {
		TaskInvoked = beh->tasks[iter->second];

		//
		// NOTE: if TaskInvoked has not been
		// consolidated, the return type will not be
		// known
		//
		if(TaskInvoked->prog) {
		    returned_type = TaskInvoked->prog->ReturnType;
		} else {
		    returned_type = TaskInvoked->return_type;
		}
		break;
	    }
	}
    }

    if(!func && !TaskInvoked) {
	Cstring err;
	err << "File " << file << ", line " << line
	    << ": trying to consolidate function call - could not find function "
	    << getData() << " anywhere (not internal, not local)";
	throw(eval_error(err));
    }

    ExpressionList* el = dynamic_cast<ExpressionList*>(zero_or_more_args.object());

    //
    // if zero_or_more_args has no args, it's a token equal to '('
    //

    if(!el) {

	//
	// No arguments. If this is an AAF function, check that the function requires no parameters
	//
	if(TaskInvoked) {
	    task* T = TaskInvoked;
	    if(T->paramindex.size() != 0) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": trying to call " << getData()
		    << " with no parameters; function requires "
		    << T->paramindex.size();
		throw(eval_error(err));
	    }
	}
	if(APcloptions::theCmdLineOptions().debug_grammar) {
	    cerr << aafReader::make_space(dent) << "consolidating function call to " << getData()
		 << " with no arguments\n";
	}
    } else {
	el->consolidate(actual_arguments, dent + 2);

	if(TaskInvoked) {
	    task* T = TaskInvoked;
	    if(T->paramindex.size() != actual_arguments.size()) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": trying to call " << getData()
		    << " with " << actual_arguments.size()
		    << " parameter(s); function requires "
		    << T->paramindex.size();
		throw(eval_error(err));
	    }
	    for(int k = 0; k < T->paramindex.size(); k++) {
		apgen::DATA_TYPE expected_dt = T->get_varinfo()[T->paramindex[k]].second;
		apgen::DATA_TYPE actual_dt = apgen::DATA_TYPE::UNINITIALIZED;
		if((actual_dt = actual_arguments[k]->get_result_type()) != apgen::DATA_TYPE::UNINITIALIZED) {
		    if(!TypedValue::compatible_types(actual_dt, expected_dt)) {
			Cstring err;
			err << "File " << file << ", line " << line
			    << ": trying to call " << getData()
			    << "; parameter " << (k+1) << " has type "
			    << apgen::spell(actual_dt) << " but function "
			    << "expects " << apgen::spell(expected_dt);
			throw(eval_error(err));
		    }
		}
	    }
	}
	if(APcloptions::theCmdLineOptions().debug_grammar) {
	    cerr << aafReader::make_space(dent) << "consolidating function call to " << getData()
		 << " with expression list\n";
	}
    }

    // if(tok_precomp_id) {
	//
	// debug
	//
	// cerr << "FunctionCall: precomp id found\n";
    // }

    //
    // handle possible qualifications
    //

    if(expressions.size() && expressions[0]->getData() != ";") {
	Qualifications* qual = dynamic_cast<Qualifications*>(expressions[0].object());
	assert(qual);

	bool this_is_an_indexed_value = false;
	bool this_is_a_class_member = false;

	for(int i = 0; i < qual->IndicesOrMembers.size(); i++) {
	    MultidimIndices* multi
		= dynamic_cast<MultidimIndices*>(qual->IndicesOrMembers[i].object());
	    ClassMember* cm =
		dynamic_cast<ClassMember*>(qual->IndicesOrMembers[i].object());
	    if(multi) {
		if(this_is_a_class_member) {
		    Cstring err;
		    err << "File " << file << ", line " << line << ": "
			<< "expression is too complex; set the instance member "
			<< "to an array variable, then apply the indices to it.";
		    throw(eval_error(err));
		}
		this_is_an_indexed_value = true;
		smart_ptr<MultidimIndices> multiptr(multi);
		aafReader::consolidate_multidim_indices(multiptr, dent + 2);
		multi = multiptr.object();
		indices.reference(multi);
	    } else if(cm) {
		this_is_an_indexed_value = false;
		this_is_a_class_member = true;
		a_member.reference(cm);
	    } else {
		assert(false);
	    }
	}

	if(this_is_an_indexed_value) {

	    //
	    // simple indexed symbol
	    //

	    if(returned_type != apgen::DATA_TYPE::ARRAY) {
		Cstring err;
		err << "File " << file << ", line " << line << ": function " << getData()
		    << " should return an array, "
		    << " but it returns a(n) " << apgen::spell(returned_type);
		throw(eval_error(err));
	    }

	    //
	    // get_val_ref will use the simple indices, and also
	    // a_member if applicable
	    //

	} else if(this_is_a_class_member) {

	    //
	    // simple instance member
	    //

	    if(returned_type != apgen::DATA_TYPE::INSTANCE) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": function " << getData() << " should return an instance, "
		    << " but it returns a(n) " << apgen::spell(returned_type);
		throw(eval_error(err));
	    }
	}
    }
}

void FunctionDeclaration::consolidate(
		int dent) {
	Cstring function_name = getData();
	FunctionIdentity* fi = dynamic_cast<FunctionIdentity*>(start_function_decl.object());
	assert(fi);
	Cstring func_keyword = fi->tok_func->getData();
	bool	is_a_script = func_keyword == "script";

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating FunctionDeclaration "
			<< func_keyword << " " << function_name << "\n";
	}

	task* new_task = NULL;
	map<Cstring, int>::iterator	iter
			= aafReader::CurrentType().taskindex.find(function_name);
	if(iter != aafReader::CurrentType().taskindex.end()) {

		//
		// Nothing to do - function was already declared or
		// implemented
		//

		return;

	} else {

		//
		// function was neither declared nor defined
		//

		int func_index = aafReader::CurrentType().add_task(function_name);
		new_task = aafReader::CurrentType().tasks[func_index];
	}
	new_task->script_flag = is_a_script;

	aafReader::push_current_task(new_task);


	//
	// Get the parameters (if any)
	//

	if(!signature) {

		//
		// No parameters
		//

		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent + 2) << "no parameters\n";
		}
	} else {
		Signature* sig = dynamic_cast<Signature*>(signature.object());
		assert(sig);
		vector<Cstring>	param_names;
		vector<apgen::DATA_TYPE> param_types;

		//
		// grab the type
		//

		DataType* dt = dynamic_cast<DataType*>(sig->signature_stuff.object());
		assert(dt);

		dt->consolidate(dent + 2);
		param_types.push_back(dt->val.get_type());

		//
		// grab the name
		//

		param_names.push_back(sig->tok_id->getData());

		for(int i = 0; i < sig->expressions.size(); i++) {
			if(sig->expressions[i]->getData() == ",") {
				continue;
			}
			DataType* dt2 = dynamic_cast<DataType*>(sig->expressions[i].object());
			if(dt2) {
				// it's a type
				dt2->consolidate(dent + 2);
				param_types.push_back(dt2->val.get_type());
			} else {
				// must be a name
				param_names.push_back(sig->expressions[i]->getData());
			}
		}

		//
		// Now we update the type to reflect the parameters we just
		// got
		//
		for(int k = 0; k < param_names.size(); k++) {
			int var_index = new_task->add_variable(
				param_names[k],
				param_types[k]);
			new_task->paramindex.push_back(var_index);
		}
	}

	aafReader::pop_current_task();

	return;

	//
	// End of declaration phase
	//

}

void FunctionDefinition::consolidate(
		int dent) {
    Cstring function_name = getData();
    FunctionIdentity* fi = dynamic_cast<FunctionIdentity*>(
				start_function_def.object());
    assert(fi);

    //
    // Be careful: tok_func is not defined if this is a
    // precomputed resource
    //
    Cstring	func_keyword = fi->tok_func->getData();
    bool	is_a_script = func_keyword == "script";
    bool	is_a_modeling_method = fi->tok_model;
    bool	is_a_resource_usage_method = fi->tok_resource;
    bool	is_a_decomposition_method = fi->tok_decomposition;
    bool	is_a_decomposition = false;
    bool	is_a_nonexcl_decomp = false;
    bool	is_a_concur_exp = false;
    bool	is_a_method =   is_a_modeling_method
				|| is_a_resource_usage_method
				|| is_a_decomposition_method;

    if(is_a_decomposition_method) {
	is_a_decomposition = fi->tok_decomposition->getData() == "decomposition";
	is_a_nonexcl_decomp = fi->tok_decomposition->getData() == "nonexclusive_decomposition";
	is_a_concur_exp = fi->tok_decomposition->getData() == "expansion"
			  || fi->tok_decomposition->getData() == "concurrent_expansion";
    }

    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << aafReader::make_space(dent) << "consolidating FunctionDefinition "
	     << func_keyword << " " << function_name << "\n";
    }

    if(aafReader::CurrentPass() == aafReader::DeclarationPass) {
	if(!aafReader::functions_declared_but_not_implemented().find(function_name)) {
	    smart_ptr<FunctionDefinition> ptr(this);
	    aafReader::functions_declared_but_not_implemented()
		<< new Cntnr<alpha_string, smart_ptr<FunctionDefinition> >(function_name, ptr);
	}
    }

    task* new_task = NULL;
    map<Cstring, int>::iterator	iter
    		= aafReader::CurrentType().taskindex.find(function_name);
    if(iter != aafReader::CurrentType().taskindex.end()) {
	new_task = aafReader::CurrentType().tasks[iter->second];

	//
	// The expected behavior is that new_task->prog has not been
	// defined yet. If the task exists, that means that a declaration
	// was found; the declaration does not fill the prog member.
	//
	if(new_task->prog) {

	    //
	    // If this is a regular function, this is not an error,
	    // because we may have consolidated this function while
	    // processing the function definition or global definitions
	    // in the first phase. We have nothing to do, regardless of
	    // which pass this is.
	    //
	    // However, if it is a method, then this should never
	    // happen.
	    //
	    if(is_a_method) {

		//
		// Methods cannot be declared (as of now); they can only
		// be implemented.
		//
		Cstring err;
		err << "In file " << file << ", line " << line
		    << ", the definition of method " << function_name
		    << " is a duplicate - the method already exists.\n";
		throw(eval_error(err));
	    } else {
		return;
	    }
	}
    } else {

	//
	// function was neither declared nor defined
	//

	//
	// CurrentType() is set to the GlobalConstructor.
	// In the case of a method, this is somewhat misleading;
	// the type is really a virtual type which in practice
	// must be an activity type or an abstract resource.
	//
	// So, what we call a method is really a template for
	// a class method. Since we don't have virtual types,
	// we use the GlobalConstructor as a master type even
	// though that is semantically incorrect.
	//
	int func_index = aafReader::CurrentType().add_task(function_name);
	new_task = aafReader::CurrentType().tasks[func_index];
	new_task->script_flag = is_a_script;

	//
	// If this is a method, the new task should define the
	// 'parent' variable
	//
	if(is_a_method) {
	    new_task->add_variable("parent", apgen::DATA_TYPE::INSTANCE);
	}

	aafReader::push_current_task(new_task);

	//
	// Get the parameters (if any)
	//
	if(optional_func_params->getData() == ")") {

	    //
	    // No parameters
	    //
	    if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent + 2) << "no parameters\n";
	    }
	} else {
	    int param_declaration_index = 0;

	    //
	    // We will capture parameter names in the vector below,
	    // and match them against declarations later on:
	    //
	    vector<Cstring>	param_names;

	    if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent + 2) << "parameters:\n";
	    }

	    parsedExp& params = optional_func_params;

	    if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent + 4)
		     << params->getData() << "\n";
	    }
	    param_names.push_back(params->getData());

	    for(int i = 0; i < params->expressions.size(); i++) {
		if(params->expressions[i]->getData() == ")") {

		    //
		    // the syntax rule stuffs all items into
		    // params, so we have to figure out where the
		    // declarations are...
		    //
		    param_declaration_index = i + 2;
		    break;
		} else if(params->expressions[i]->getData() == ",") {
		    continue;
		}
		param_names.push_back(params->expressions[i]->getData());

		if(APcloptions::theCmdLineOptions().debug_grammar) {
		    cerr << aafReader::make_space(dent + 4)
		         << params->expressions[i]->getData() << "\n";
		}
	    }
	    parsedExp& param_declarations
		= params->expressions[param_declaration_index];

	    //
	    // same pattern as above: grab the item itself, then its
	    // expressions
	    //
	    Declarations*	decls = dynamic_cast<Declarations*>(param_declarations.object());
	    assert(decls);
	    Declaration*	decl = dynamic_cast<Declaration*>(decls->declaration.object());
	    map<Cstring, Declaration*>	param_declaration_map;
	    param_declaration_map[decl->tok_id->getData()] = decl;

	    if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent)
			 << "consolidate declarations: first declaration "
			 << decl->to_string() << "\n";
		cerr << aafReader::make_space(dent) << decl->expressions.size()
			 << " more to go\n";
	    }
	    for(int i = 0; i < decls->expressions.size(); i++) {
		Declaration* decl1
			= dynamic_cast<Declaration*>(decls->expressions[i].object());
		assert(decl1);
		param_declaration_map[decl1->tok_id->getData()] = decl1;
	    }

	    if(param_declaration_map.size() != param_names.size()) {
		Cstring err;
		err << "In file " << file << ", line " << line
		    << ", the definition of function " << function_name
		    << ", the signature has " << param_names.size()
		    << " parameter(s) but " << param_declaration_map.size()
		    << " declaration(s) are provided.\n";
		throw(eval_error(err));
	    }

	    //
	    // Now we consolidate all the declarations
	    //
	    map<Cstring, Declaration*>::iterator decl_iter;
	    for(int k = 0; k < param_names.size(); k++) {
		decl_iter = param_declaration_map.find(param_names[k]);
		if(decl_iter == param_declaration_map.end()) {
		    Cstring err;
		    err << "In file " << file << ", line " << line
			<< ", in the definition of function " << function_name
			<< ", could not find declaration for parameter "
			<< param_names[k];
		    throw(eval_error(err));
		}
		decl = decl_iter->second;

		//
		// Fast APGen reference: Syntax::build_declaration
		//
		decl->consolidate(dent + 2);

		//
		// Need to add to the parameters of the task. First,
		// let's check that the parameter was added to the
		// symbol table of the task...
		//
		map<Cstring, int>::const_iterator iter
			= new_task->get_varindex().find(param_names[k]);
		assert(iter != new_task->get_varindex().end());
		new_task->paramindex.push_back(iter->second);
	    }
	}

	aafReader::pop_current_task();

    }


    //
    // In the declaration pass, we stop here
    //
    if(aafReader::CurrentPass() == aafReader::DeclarationPass) {

	return;
    }

    aafReader::push_current_task(new_task);

    //
    // Get the statements in the body
    //
    Program* theProg = dynamic_cast<Program*>(program.object());
    assert(theProg);
    if(!is_a_method) {
	theProg->orig_section = apgen::METHOD_TYPE::FUNCTION;
    } else if(is_a_modeling_method) {
	theProg->orig_section = apgen::METHOD_TYPE::MODELING;
    } else if(is_a_resource_usage_method) {
	theProg->orig_section = apgen::METHOD_TYPE::RESUSAGE;
    } else if(is_a_decomposition_method) {
	if(is_a_decomposition) {
	    theProg->orig_section = apgen::METHOD_TYPE::DECOMPOSITION;
	} else if(is_a_nonexcl_decomp) {
	    theProg->orig_section = apgen::METHOD_TYPE::NONEXCLDECOMP;
	} else if(is_a_concur_exp) {
	    theProg->orig_section = apgen::METHOD_TYPE::CONCUREXP;
	}
    }

    aafReader::get_current_task()->prog = theProg;
    aafReader::consolidate_program(*theProg, dent + 2);

    aafReader::pop_current_task();

    //
    // Remove the function from declared list since
    // now it's been fully implemented
    //
    Cntnr<alpha_string, smart_ptr<FunctionDefinition> >* cn;
    if((cn = aafReader::functions_declared_but_not_implemented().find(function_name))) {
    	delete cn;
    }

    if(APcloptions::theCmdLineOptions().generate_cplusplus) {

	FunctionDefinition::generate_behavior_header(*new_task);
	FunctionDefinition::generate_behavior_body(*new_task);

    }
}

void Global::consolidate(int dent) {
	Cstring name(tok_id->getData());

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Global " << name << "\n";
		cerr << aafReader::make_space(dent + 2) << "contents: "
			<< to_string();
	}

	//
	// step 1: type determination
	//
	apgen::DATA_TYPE	theType;
	if(local_or_global) {
		if(any_data_type->getData() == "boolean") {
			theType = apgen::DATA_TYPE::BOOL_TYPE;
		} else if(any_data_type->getData() == "instance") {
			theType = apgen::DATA_TYPE::INSTANCE;
		} else if(any_data_type->getData() == "integer") {
			theType = apgen::DATA_TYPE::INTEGER;
		} else if(any_data_type->getData() == "duration") {
			theType = apgen::DATA_TYPE::DURATION;
		} else if(any_data_type->getData() == "time") {
			theType = apgen::DATA_TYPE::TIME;
		} else if(any_data_type->getData() == "string") {
			theType = apgen::DATA_TYPE::STRING;
		} else if(any_data_type->getData() == "float") {
			theType = apgen::DATA_TYPE::FLOATING;
		} else if(any_data_type->getData() == "array") {
			theType = apgen::DATA_TYPE::ARRAY;
		} else {
			// this should be a typedef
			theType = apgen::DATA_TYPE::ARRAY;
		}
	} else if(tok_epoch) {
		theType = apgen::DATA_TYPE::TIME;
	} else if(tok_timesystem) {
		theType = apgen::DATA_TYPE::ARRAY;
	}

	task* T = Behavior::GlobalType().tasks[0];
	map<Cstring, int>::const_iterator iter = T->get_varindex().find(name);

	//
	// step 2: variable creation
	//
	if(iter != T->get_varindex().end()) {
		Cstring err;
		err << "File " << file << ", line " << line << ", global " << name
			<< " already exists";
		throw(eval_error(err));
	}
	int varind = T->add_variable(name, theType);

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent + 2)
			<< "global consolidation: Added variable # " << varind
			<< " (" << name << ") to " << T->name << "\n";
	}

	//
	// step 3: replace tok_id by a properly defined Symbol
	//
	parsedExp	nameExp(new pE_w_string(origin_info(line, file), name));
	Symbol*	lhs = new Symbol(0, nameExp, Differentiator_0(0));
	lhs->my_level = 0;
	lhs->my_index = varind;
	tok_id.dereference();

	//
	// Note that this substitution works with to_stream() because theData
	// is set to the tok_id's data by the Symbol constructor
	//
	tok_id.reference(lhs);
	
	//
	// step 4: consolidate the rhs
	//
	aafReader::consolidate_expression(Expression, dent + 2, theType);

	if(theType == apgen::DATA_TYPE::ARRAY) {
		apgen::DATA_TYPE element_type = apgen::DATA_TYPE::UNINITIALIZED;
		int array_size = 0;
		int dimensions = 0;
		if(aafReader::array_is_homogeneous(
			Expression,
			element_type,
			array_size,
			dimensions,
			dent + 2)) {

			//
			// fetch the current task and set the appropriate
			// element of its homogeneoustype member
			//

			// debug
			// cerr << aafReader::make_space(dent)
			//   << "consolidate global array: setting homogeneous type of "
			//   << tok_id->getData()
			//   << " to " << apgen::spell(element_type)
			//   << "; dimension is " << dimensions << "\n";

			aafReader::get_current_task()->homogeneoustype[tok_id->getData()]
				= pair<apgen::DATA_TYPE, int>(element_type, dimensions);
		} else {
			// debug
			// cerr << aafReader::make_space(dent)
			//    << "consolidate global array: array " << tok_id->getData()
			//    << " is inhomogeneous or not detailed; cannot optimize.\n";
		}
	}

	//
	// step 5: evaluate the rhs and set the lhs's value
	//
	try {
		Expression->eval_expression(
			behaving_object::GlobalObject(),
			(*behaving_object::GlobalObject())[varind]);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ", global " << name
			<< ": evaluation error -\n" << Err.msg;
		throw(eval_error(err));
	}

	//
	// Check whether this global is constant and set
	// the appropriate flag
	//
	if(	(local_or_global && local_or_global->getData() == "constant")
		|| tok_timesystem || tok_epoch) {
	    global_behaving_object::constants().insert(name);
	    (*behaving_object::GlobalObject())[varind].make_constant();
	}

	//
	// Make sure the global symbol in GlobalObject has its
	// modifiers member set to 0:
	//
	TypedValue& glob_value = (*behaving_object::GlobalObject())[varind];

	if(glob_value.get_type() == apgen::DATA_TYPE::UNINITIALIZED) {
		Cstring err;
		err << "File " << file << ", line " << line << ", global " << name
			<< ": right-hand side evaluation results in an undefined value";
		throw(eval_error(err));
	}

	//
	// If epoch or timesystem, let globalData know
	//
	if(tok_timesystem) {
		globalData::qualifySymbol(
			name,
			apgen::DATA_TYPE::ARRAY,
			/* reserved = */ false,
			/* epoch = */ false,
			/* timesystem = */ true);
	} else if(tok_epoch) {
		globalData::qualifySymbol(
			name,
			apgen::DATA_TYPE::TIME,
			/* reserved = */ false,
			/* epoch = */ true,
			/* timesystem = */ false);
	}

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "After evaluation: "
			<< (*behaving_object::GlobalObject())[varind].to_string() << "\n";
	}
}


void Parameters::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Parameters\n";
	}
	Declarations* decls = dynamic_cast<Declarations*>(
				param_declarations.object());
	assert(decls);

	//
	// Now we have to do two things:
	//    (1) consolidate each parameter declaration
	//    (2) create a parameters program for the current task
	//    (3) populate the parameters section of the current task
	//
	task* cur_task = aafReader::get_current_task();
	Behavior* beh = &cur_task->Type;

	Declaration* decl = dynamic_cast<Declaration*>(
				decls->declaration.object());
	assert(decl);
	decl->consolidate(dent + 2);
	map<Cstring, int>::const_iterator iter
		= cur_task->get_varindex().find(decl->tok_id->getData());

	//
	// NOTE: consolidate() put the variable in the current task's varindex
	//

	assert(iter != cur_task->get_varindex().end());
	cur_task->paramindex.push_back(iter->second);

	parsedExp declPE(decl);
	Program* pp = new Program(0, declPE, Differentiator_0(0));
	cur_task->parameters.reference(pp);

	for(int i = 0; i < param_declarations->expressions.size(); i++) {
		decl = dynamic_cast<Declaration*>(
				param_declarations->expressions[i].object());

		decl->consolidate(dent + 2);
		iter = cur_task->get_varindex().find(decl->tok_id->getData());
		assert(iter != cur_task->get_varindex().end());
		cur_task->paramindex.push_back(iter->second);

		parsedExp declPE2(decl);
		pp->addExp(declPE2);
	}
}

void WaitFor::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating " << spell() << " statement\n";
	}
	aafReader::consolidate_expression(Expression, dent + 2);
}

void WaitUntil::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating " << spell() << " statement\n";
	}
	aafReader::consolidate_expression(Expression, dent + 2);
}

void GetWindows::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating " << spell() << " statement\n";
	}
	ExpressionList* el = dynamic_cast<ExpressionList*>(expression_list.object());

	//
	// guaranteed by the syntax:
	//

	// assert(el);


	//
	// Step 1: get the scheduling condition.
	//
	el->consolidate(actual_arguments, dent + 2);

	if(actual_arguments.size() != 1) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": get_windows should have a single argument, i. e., "
			<< "the condition that needs to be true for a window to exist.";
		throw(eval_error(errs));
	}

	//
	// actual_arguments[0] contains the scheduling condition
	//
	SchedulingCondition = actual_arguments[0];

	//
	// Now we figure out which resources need to be tracked for this condition
	//
	currentval_finder CF;
	SchedulingCondition->recursively_apply(CF);
	TriggerNames = CF.container_names;

	//
	// debug
	//
	// cerr << "consolidating GetWindows:\n\tcondition = "
	// 	<< SchedulingCondition->to_string() << "\n\ttriggers =\n";
	// emptySymbol* es;
	// stringslist::iterator trigger_iter(TriggerNames);
	// while((es = trigger_iter())) {
	// 	cerr << "\t\t" << es->get_key() << "\n";
	// }

	//
	// Step 2: get the options.
	//
	// Valid entries in Options tags are
	//
	// 	"start"
	// 	"min"
	// 	"max"
	// 	"actual"
	//
	// However, we are reluctant to evaluate this array now... the array
	// could contain expressions that need to be evaluated as part of
	// executing the scheduling program.
	//
	// Exception: the "actual" item in the list; it should refer to a
	// variable in which we should stick the computed length of the first
	// window.
	//
	aafReader::consolidate_expression(Expression, dent + 2);
	Array* array = dynamic_cast<Array*>(Expression.object());
	if(!array || !array->is_struct) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": get_windows should have a 'for' clause followed by "
			<< "a single struct-style array defining the following:\n"
			<< "\tstart (optional - where to start searching)\n"
			<< "\tmin (mandatory - minimum duration of windows)\n"
			<< "\tmax (mandatory - length of the interval to search)\n"
			<< "\tactual (optional - length of the first window)\n";
		throw(eval_error(errs));
	}


	// debug
	// aoString aos;

	// Symbol::debug_symbol = true;
	// cerr << "consolidating GetWindows: options dump\n";
	// for(int i = 0; i < array->actual_values.size(); i++) {
	// 	cerr << "\t" << i << ": " << array->actual_keys[i]->to_string()
	// 		<< " = " << array->actual_values[i]->to_string()
	// 		<< "\n";
	// }
	// cerr << aos.str();
	// Symbol::debug_symbol = false;

	for(int i = 0; i < array->actual_values.size(); i++) {
		Constant* cst = dynamic_cast<Constant*>(array->actual_keys[i].object());
		if(!cst) {
			Cstring errs;
			errs << "File " << file << ", line " << line
				<< ": error while consolidating options of get_windows(); "
				<< "the option keys should be constant strings (like \"start\" etc.)";
			throw(eval_error(errs));
		}
		Cstring the_key = cst->getData();
		removeQuotes(the_key);
		if(the_key == "actual") {
			Symbol* sym = dynamic_cast<Symbol*>(array->actual_values[i].object());
			if(!sym) {
				Cstring errs;
				errs << "File " << file << ", line " << line
					<< ": error while consolidating options of get_windows(); "
					<< "the value of 'actual' should be a global or local symbol.";
				throw(eval_error(errs));
			}
			ActualSymbol.reference(sym);
		} else if(the_key == "min") {
			;
		} else if(the_key == "max") {
			;
		} else if(the_key == "start") {
			;
		} else {
			Cstring errs;
			errs << "File " << file << ", line " << line
				<< ": error while consolidating option \"" << the_key << " of get_windows(); "
				<< "valid option keys are \"actual\", \"max\", \"min\" and \"start\".";
			throw(eval_error(errs));
		}
	}

	Options.reference(array);
}

void GetInterpwins::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating " << spell() << " statement\n";
	}
	ExpressionList* el = dynamic_cast<ExpressionList*>(expression_list.object());

	//
	// guaranteed by the syntax:
	//

	// assert(el);


	//
	// Step 1: get the scheduling condition.
	//

	el->consolidate(actual_arguments, dent + 2);

	if(actual_arguments.size() != 1) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": get_windows should have a single argument, i. e., "
			<< "the condition that needs to be true for a window to exist.";
		throw(eval_error(errs));
	}

	//
	// actual_arguments[0] contains the scheduling condition
	//

	SchedulingCondition = actual_arguments[0];

	//
	// Step 2: get the options.
	//
	// Valid entries in Options tags are
	//
	// 	"start"
	// 	"min"
	// 	"max"
	// 	"actual"
	//
	// However, we are reluctant to evaluate this array now... the array
	// could contain expressions that need to be evaluated as part of
	// executing the scheduling program.
	//
	// Exception: the "actual" item in the list; it should refer to a
	// variable in which we should stick the computed length of the first
	// window.
	//

	aafReader::consolidate_expression(Expression, dent + 2);
	Array* array = dynamic_cast<Array*>(Expression.object());
	if(!array || !array->is_struct) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": get_windows should have a 'for' clause followed by "
			<< "a single struct-style array defining the following:\n"
			<< "\tstart (optional - where to start searching)\n"
			<< "\tmin (mandatory - minimum duration of windows)\n"
			<< "\tmax (mandatory - length of the interval to search)\n"
			<< "\tactual (optional - length of the first window)\n";
		throw(eval_error(errs));
	}


	// debug
	// aoString aos;

	// Symbol::debug_symbol = true;
	// cerr << "consolidating GetWindows: options dump\n";
	// for(int i = 0; i < array->actual_values.size(); i++) {
	// 	cerr << "\t" << i << ": " << array->actual_keys[i]->to_string()
	// 		<< " = " << array->actual_values[i]->to_string()
	// 		<< "\n";
	// }
	// cerr << aos.str();
	// Symbol::debug_symbol = false;

	for(int i = 0; i < array->actual_values.size(); i++) {
		Constant* cst = dynamic_cast<Constant*>(array->actual_keys[i].object());
		if(!cst) {
			Cstring errs;
			errs << "File " << file << ", line " << line
				<< ": error while consolidating options of get_windows(); "
				<< "the option keys should be constant strings (like \"start\" etc.)";
			throw(eval_error(errs));
		}
		Cstring the_key = cst->getData();
		removeQuotes(the_key);
		if(the_key == "actual") {
			Symbol* sym = dynamic_cast<Symbol*>(array->actual_values[i].object());
			if(!sym) {
				Cstring errs;
				errs << "File " << file << ", line " << line
					<< ": error while consolidating options of get_windows(); "
					<< "the value of 'actual' should be a global or local symbol.";
				throw(eval_error(errs));
			}
			ActualSymbol.reference(sym);
		} else if(the_key == "min") {
			;
		} else if(the_key == "max") {
			;
		} else if(the_key == "start") {
			;
		} else {
			Cstring errs;
			errs << "File " << file << ", line " << line
				<< ": error while consolidating option \"" << the_key << " of get_windows(); "
				<< "valid option keys are \"actual\", \"max\", \"min\" and \"start\".";
			throw(eval_error(errs));
		}
	}

	Options.reference(array);
}

void WaitUntilRegexp::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating " << spell() << " statement\n";
	}
	aafReader::consolidate_expression(Expression, dent + 2);
}
