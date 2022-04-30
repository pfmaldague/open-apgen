#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/stat.h>
#include "aafReader.H"
#include "ActivityInstance.H"	// for dumb_actptr
#include "ACT_exec.H"
#include "APmodel.H"
#include "apcoreWaiter.H"
#include "BehavingElement.H"
#include "fileReader.H"
#include "lex_intfc.H"
#include "RES_def.H"

extern int abdebug;
extern int yylineno;

using namespace pEsys;


namespace aafReader {

string make_space(int n) {
	return string(n, ' ');
}

bool find_symbol_in_task(
		const Cstring&		symbol_name,
		int&			level_to_use,
		int&			index_to_use,
		apgen::DATA_TYPE&	dt) {
	map<Cstring, int>::const_iterator index_of_var_in_table;

	for(int icontext = LevelOfCurrentTask; icontext >= 0; icontext--) {
		task*	curtask = CurrentTasks[icontext];

		if((index_of_var_in_table = curtask->get_varindex().find(symbol_name))
						  != curtask->get_varindex().end()) {
			level_to_use = icontext;
			index_to_use = index_of_var_in_table->second;
			dt = curtask->get_varinfo()[index_of_var_in_table->second].second;
			return true;
		}
	}

	return false;
}

void consolidate_if(
		smart_ptr<executableExp>& E,
		apgen::DATA_TYPE& return_type,
		apgen::METHOD_TYPE method_type,
		int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidating If " << E->getData() << "\n";
	}
	If* ee = dynamic_cast<If*>(E.object());
	If* new_ee;

	//
	// An if statement is followed by a single statement or a program
	//

	if(ee->execute_method == &If::execute_if) {
		E.reference((new_ee = new NonRecursiveIf(true, *ee)));
		ee = new_ee;
		consolidate_expression(ee->Expression, dent + 2);
	} else if(ee->execute_method == &If::execute_elseif) {
		E.reference((new_ee = new NonRecursiveElseIf(true, *ee)));
		ee = new_ee;
		consolidate_expression(ee->Expression, dent + 2);
	} else {
		E.reference((new_ee = new NonRecursiveElse(true, *ee)));
		ee = new_ee;
	}
	assert(ee->expressions.size() == 1);
	Program* pr = dynamic_cast<Program*>(ee->expressions[0].object());
	assert(pr);
	pr->orig_section = method_type;
	consolidate_program(*pr, dent + 2);
	return_type = pr->ReturnType;
}

void consolidate_program(Program& pc, int dent) {
    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "consolidating Program " << pc.getData() << "\n";
    }

    ConditionalFlag	cf = NotAConditional;

    //
    // NOTE: the current task has been pushed onto the task stack
    //
    for(int i = 0; i < pc.statements.size(); i++) {
	ConditionalFlag	new_cf = NotAConditional;
	bool		is_ret = false;

	consolidate_statement(
			pc.statements[i],
			pc.ReturnType,
			pc.orig_section,
			new_cf,
			is_ret,
			dent + 2);

	if(is_ret) {
	    if(pc.orig_section != apgen::METHOD_TYPE::FUNCTION) {
		Cstring err;
		err << "File " << pc.statements[i]->file << ", line "
		    << pc.statements[i]->line << ": a global method "
		    << "may not have a return statement, because "
		    << "in the event of a spawned WAIT the return "
		    << "value is not known and cannot be returned "
		    << "to the caller.";
		throw(eval_error(err));
	    }
	}

	//
	// Check for invalid conditional transitions
	//
	if(new_cf == IfAlone) {

	    //
	    // Always OK
	    //
	} else if(new_cf == ElseAlone) {

	    //
	    // Must be in state IfAlone or ElseIf
	    //
	    if(cf != IfAlone && cf != ElseIf) {
		Cstring errs;
		executableExp*	the_statement = pc.statements[i].object();
		errs << "File " << the_statement->file << ", line "
		     << the_statement->line << ": else statement "
		     << "is not preceded by if nor if else.";
		throw(eval_error(errs));
	    }
	} else if(new_cf == ElseIf) {

	    //
	    // Must be in state IfAlone or ElseIf
	    //
	    if(cf != IfAlone && cf != ElseIf) {
		Cstring errs;
		executableExp*	the_statement = pc.statements[i].object();
		errs << "File " << the_statement->file << ", line "
		     << the_statement->line << ": 'if else' statement "
		     << "is not preceded by 'if' nor 'if else'.";
		throw(eval_error(errs));
	    }
	}
	cf = new_cf;

	//
	// This is to help ResourceValue.C identify key variables needed
	// for displaying resources correctly:
	//
	Assignment* assign = dynamic_cast<Assignment*>(pc.statements[i].object());
	if(assign) {
	    Symbol* sym = dynamic_cast<Symbol*>(assign->lhs_of_assign.object());
	    QualifiedSymbol* qual_sym
		= dynamic_cast<QualifiedSymbol*>(assign->lhs_of_assign.object());
	    if(sym) {
		pc.symbols[sym->getData()] = assign;
	    } else if(qual_sym) {
		pc.symbols[qual_sym->symbol->getData()] = assign;
	    }
	}
    }
}

void consolidate_statement(
		smart_ptr<executableExp>&	E,
		apgen::DATA_TYPE&		return_type,
		apgen::METHOD_TYPE		method_type,
		ConditionalFlag&		cond_flag,
		bool&				is_return,
		int				dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidating Statement " << E->getData() << "\n";
	}

	cond_flag = NotAConditional;
	is_return = false;

	Declaration* decl = dynamic_cast<Declaration*>(E.object());
	if(decl) {
		decl->consolidate(dent + 2);
		return;
	}
	Assignment* as = dynamic_cast<Assignment*>(E.object());
	if(as) {
		consolidate_assignment(E, dent + 2);
		return;
	}
	Return* ret = dynamic_cast<Return*>(E.object());
	if(ret) {
		consolidate_return(*ret, return_type, dent + 2);
		is_return = true;
		return;
	}
	Continue* continue_stmt = dynamic_cast<Continue*>(E.object());
	if(continue_stmt) {
		consolidate_continue(*continue_stmt, dent + 2);
		return;
	}
	If* if_stmt = dynamic_cast<If*>(E.object());
	if(if_stmt) {
		if(if_stmt->tok_if) {
			if(if_stmt->tok_else) {
				cond_flag = ElseIf;
			} else {
				cond_flag = IfAlone;
			}
		} else if(if_stmt->tok_else) {
			cond_flag = ElseAlone;
		}
		consolidate_if(E, return_type, method_type, dent + 2);
		return;
	}
	While* wh = dynamic_cast<While*>(E.object());
	if(wh) {
		consolidate_while(E, return_type, method_type, dent + 2);
		return;
	}
	FunctionCall* fc = dynamic_cast<FunctionCall*>(E.object());
	if(fc) {
		consolidate_function_call_exe(E, dent + 2);
		return;
	}
	Decomp* dec = dynamic_cast<Decomp*>(E.object());
	if(dec) {
		dec->consolidate(method_type, dent + 2);
		return;
	}
	Usage* usage = dynamic_cast<Usage*>(E.object());
	if(usage) {

	    //
	    // Check that we are in the correct section
	    //
	    switch(method_type) {
		    case apgen::METHOD_TYPE::CONCUREXP:
		    case apgen::METHOD_TYPE::MODELING:
		    case apgen::METHOD_TYPE::RESUSAGE:
			    break;
		    default:
			    {
			    Cstring errs;
			    errs << "File " << usage->file << ", line "
				<< usage->line << ": a usage statement "
				<< "should not occur within a section of type "
				<< apgen::spell(method_type);
			    throw(eval_error(errs));
			    }
	    }
	    ResUsageWithArgs* resusage
			= dynamic_cast<ResUsageWithArgs*>(
				usage->resource_usage_with_arguments.object());
	    assert(resusage);
	    Cstring resource_used = resusage->getData();
	    Cstring action = resusage->tok_action->getData();

	    RCsource*	container = RCsource::resource_containers().find(resource_used);

	    Behavior* abs_beh = Behavior::find_type("abstract resource", resource_used);

	    if(container) {

		//
		// This will allow us to make resource evaluation
		// more efficient. When use_method_count is zero,
		// it is sufficient to evaluate the resource's
		// profile.
		//
		container->use_method_count++;
	    }
	    //
	    // Check for immediately temporal clause
	    //
	    bool action_is_immediate = false;

	    if(usage->temporalSpecification) {
		TemporalSpec* tempspec = dynamic_cast<TemporalSpec*>(
		    usage->temporalSpecification.object());
		assert(tempspec);
		if(tempspec->tok_immediately) {
		    action_is_immediate = true;
		}
	    }
	    if(action == "set") {
		if(action_is_immediate) {

		    //
		    // replace usage by an ImmediateSetUsage object. Note
		    // that it will handle signals; for non-immediate
		    // signal setting/sending, we have a special class
		    // SetSignal. This is not totally consistent, but
		    // so be it.
		    //
		    ImmediateSetUsage* imm_stmt = new ImmediateSetUsage(*usage);
		    E.reference(imm_stmt);
		    imm_stmt->consolidate(dent + 2);
		} else if(resource_used == "signal") {
		    SetSignal* set_signal = new SetSignal(*usage);
		    E.reference(set_signal);
		    set_signal->consolidate(dent + 2);
		} else {

		    //
		    // replace usage by a SetUsage object
		    //
		    SetUsage* set_statement = new SetUsage(*usage);
		    E.reference(set_statement);
		    set_statement->consolidate(dent + 2);
		}
	    } else if(action == "reset") {
		if(action_is_immediate) {

		    //
		    // replace usage by an ImmediateResetUsage object
		    //
		    ImmediateResetUsage* imm_stmt = new ImmediateResetUsage(*usage);
		    E.reference(imm_stmt);
		    imm_stmt->consolidate(dent + 2);
		} else {

		    //
		    // replace usage by a ResetUsage object
		    //
		    ResetUsage* reset_statement = new ResetUsage(*usage);
		    E.reference(reset_statement);
		    reset_statement->consolidate(dent + 2);
		}
	    } else {
		if(container) {
		    if(action_is_immediate) {

			//
			// replace usage by an ImmediateUsage object
			//
			ImmediateUsage* imm_stmt = new ImmediateUsage(*usage);
			E.reference(imm_stmt);
			imm_stmt->consolidate(dent + 2);
		    } else {
			usage->consolidate(dent + 2);
		    }
		} else if(abs_beh) {

		    //
		    // make sure action is not immediate
		    //
		    if(action_is_immediate) {
			Cstring errs;
			errs << "File " << usage->file << ", line "
			    << usage->line << ":\nresource \""
			    << resource_used << "\" is abstract; "
			    << "it cannot be used in an immediate clause. "
			    << "Remove 'immediately' and\nadd 'wait for 0:0:0;' "
			    << "after the usage statement; that will probably "
			    << "do what you want.";
			throw(eval_error(errs));
		    }

		    //
		    // replace usage by an AbstractUsage object
		    //
		    AbstractUsage* abs_usage;

		    //
		    // The factory is implemented in class_generation.C.
		    // If generate_cplusplus flag is set, it creates an
		    // instance of a resource-specific class. Else, it
		    // create an instance of the generic AbstractUsage
		    // instruction.
		    //
		    abs_usage = AbstractUsage::AbstractUsageFactory(usage);
		    if(abs_usage) {

			//
			// debug
			//
			// cerr << "factory created " << resource_used << "_AbstractUsage\n";
		    } else {
			abs_usage = new AbstractUsage(*usage);

			//
			// debug
			//
			// cerr << "factory could not find " << resource_used << "\n";
		    }
		    E.reference(abs_usage);
		    abs_usage->consolidate(dent + 2);
		} else {
		    Cstring errs;
		    errs << "File " << usage->file << ", line "
			<< usage->line << ": resource \""
			<< resource_used << "\" was not defined; cannot use it";
		    throw(eval_error(errs));
		}
	    }
	    return;
	}
	WaitFor* wf = dynamic_cast<WaitFor*>(E.object());
	if(wf) {

	    //
	    // Check that we are in the correct section
	    //
	    switch(method_type) {
		case apgen::METHOD_TYPE::CONCUREXP:
		case apgen::METHOD_TYPE::MODELING:
		    break;
		default:
		    {
		    Cstring errs;
		    errs << "File " << wf->file << ", line "
			<< wf->line << ": a WAIT FOR statement "
			<< "should not occur within a section of type "
			<< apgen::spell(method_type);
		    throw(eval_error(errs));
		    }
	    }
	    wf->consolidate(dent + 2);
	    return;
	}
	WaitUntil* wu = dynamic_cast<WaitUntil*>(E.object());
	if(wu) {

	    //
	    // Check that we are in the correct section
	    //
	    switch(method_type) {
		case apgen::METHOD_TYPE::CONCUREXP:
		case apgen::METHOD_TYPE::MODELING:
		    break;
		default:
		    {
		    Cstring errs;
		    errs << "File " << wu->file << ", line "
			<< wu->line << ": a WAIT UNTIL statement "
			<< "should not occur within a section of type "
			<< apgen::spell(method_type);
		    throw(eval_error(errs));
		    }
	    }
	    wu->consolidate(dent + 2);
	    return;
	}
	WaitUntilRegexp* wur = dynamic_cast<WaitUntilRegexp*>(E.object());
	if(wur) {
		wur->consolidate(dent + 2);
		return;
	}
	GetWindows* gw = dynamic_cast<GetWindows*>(E.object());
	if(gw) {
		gw->consolidate(dent + 2);
		return;
	}
	GetInterpwins* giw = dynamic_cast<GetInterpwins*>(E.object());
	if(giw) {
		giw->consolidate(dent + 2);
		return;
	}
	Cstring err;
	err << "File " << E->file << ", line " << E->line << ", statement type " << E->spell()
		<< " is not handled yet...";
	throw(eval_error(err));
}

void consolidate(Typedef& t, int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidating Typedef " << t.getData() << "\n";
	}
	if(list_of_all_typedefs().find(t.getData())) {
		Cstring errs;
		errs << "File " << t.file << ", line " << t.line
			<< ": typedef " << t.getData() << " was already defined.";
		throw(eval_error(errs));
	}
	consolidate_expression(t.Expression, dent + 2, apgen::DATA_TYPE::ARRAY);
	TypedValue val;
	try {
		t.Expression->eval_expression(
			behaving_object::GlobalObject(), val);
	} catch(eval_error Err) {
		Cstring errs;
		errs << "File " << t.file << ", line " << t.line
			<< ": error evaluating typedef value -\n"
			<< Err.msg;
		throw(eval_error(errs));
	}
	ArrayElement* ae;
	list_of_all_typedefs().add(ae = new ArrayElement(t.getData()));
	ae->SetVal(val);
}

void consolidate_symbol(
		parsedExp& pe,
		int dent /* = 0 */,
		bool is_assigned_to /* = false */) {
	Symbol* sym = dynamic_cast<Symbol*>(pe.object());
	assert(sym);
	Symbol& s = *sym;

	//
	// Appropriate for a reference to an existing symbol. To create an
	// entry in a symbol table, consolidate a declaration.
	//
	Cstring	symbol_name = s.getData();

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidating Symbol " << symbol_name << "\n";
	}

	assert(LevelOfCurrentTask >= 0);
	int level_to_use = -1;
	int index_to_use = 0;
	apgen::DATA_TYPE dt = apgen::DATA_TYPE::UNINITIALIZED;
	if(!find_symbol_in_task(
			symbol_name,
			level_to_use,
			index_to_use,
			dt)) {
		Cstring err;
		err << "File " << s.file << ", line " << s.line << ", Symbol " << symbol_name
			<< " is not declared in current context ("
			<< get_current_task()->full_name() << ")";
		throw(eval_error(err));
	} else if(level_to_use == 0 && is_assigned_to) {

	    //
	    // Verify that we are not trying to modify a constant.
	    // We need access to the Global instance corresponding
	    // to this symbol.
	    //
	    set<Cstring>::const_iterator	iter
			= global_behaving_object::constants().find(symbol_name);
	    if(iter != global_behaving_object::constants().end()) {
		Cstring err;
		err << "File " << s.file << ", line " << s.line << ": Constant " << symbol_name
			<< " cannot be assigned to";
		throw(eval_error(err));
	    }
	}

	//
	// If the symbol represents a global variable and it
	// is being assigned to, make sure that we mark the
	// value of that variable as non-constant
	//
	if(level_to_use == 0) {
	    if(is_assigned_to) {
		symNode* sn;
		if((sn = globals_used_as_indices_in_resources().find(symbol_name))) {
		    Cstring err;
		    err << "File " << s.file << ", line " << s.line << ", Symbol " << symbol_name
			<< " was used as a set of indices for arrayed resource "
			<< sn->payload
			<< ".\n"
			<< " You cannot modify it after it has been used in this way.\n";
		    throw(eval_error(err));
		}

#ifdef NOT_YET
		//
		// We need to find out whether this symbol occurs in
		// an adaptation section that is thread-specific, i. e.,
		// which is run in a thread while other sections run in
		// other threads.
		//
		task* surrounding_task = get_current_task();

		//
		// debug
		//
		cerr << "task " << surrounding_task->full_name() << " - sets glob. var. "
			<< symbol_name << "\n";


		//
		// Finding the section of origin is not obvious; it depends
		// on the context. It is pretty obvious that each callable
		// function, including the following:
		//
		//	- global function
		//	- abstract_resource::model()
		// 	- act_type::model()
		// 	- act_type::resource_usage()
		//	- act_type::decompose()
		//	- act_type::schedule()
		//
		// should have a list of global variables that it sets
		// and/or uses. This list cannot be computed directly
		// when consolidating, but it can be computed at the
		// end of consolidation if a list of functions called
		// is also set up. It is then possible to build the
		// required list of globals through a recursive
		// computation similar to the one performed to compute
		// resource dependencies.
		//
		apgen::METHOD_TYPE	method_type = surrounding_task->prog->section_of_origin();
		cerr << "    type: " << apgen::spell(method_type) << "\n";

		TypedValue& master_val = (*behaving_object::GlobalObject())[index_to_use];


		//
		// We need to change this, based on what kind of AAF section
		// the symbol comes from. We can figure that out by looking
		// at the current task:
		//
		if(master_val.is_constant) {
		    master_val.set_is_constant_recursively_to(false);
		    Cstring equiv = Cstring("File ") + s.file + ", line " + s.line;
		    symNode* sn = assignments_to_global_arrays().find(symbol_name);
		    if(sn) {
			sn->payload << "\n" << equiv;
		    } else {
			assignments_to_global_arrays() << new symNode(symbol_name, equiv);
		    }
		}
#endif /* NOT_YET */


	    } else {

		//
		// variable is consulted but not set
		//

#ifdef NOT_YET
		//
		// debug
		//
		cerr << "task " << get_current_task()->full_name() << " - uses glob. var. "
			<< symbol_name << "\n";
#endif /* NOT_YET */
	    }
	}

	sym = NULL;
	switch(dt) {
		case apgen::DATA_TYPE::INTEGER:
			sym = new SymbolicInt(true, s);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << make_space(dent) << "converting "
					<< symbol_name << " to SymbolicInt\n";
			}
			break;
		case apgen::DATA_TYPE::FLOATING:
			sym = new SymbolicDouble(true, s);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << make_space(dent) << "converting "
					<< symbol_name << " to SymbolicDouble\n";
			}
			break;
		case apgen::DATA_TYPE::BOOL_TYPE:
			sym = new SymbolicBool(true, s);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << make_space(dent) << "converting "
					<< symbol_name << " to SymbolicBool\n";
			}
			break;
		case apgen::DATA_TYPE::TIME:
			sym = new SymbolicTime(true, s);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << make_space(dent) << "converting "
					<< symbol_name << " to SymbolicTime\n";
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			sym = new SymbolicDuration(true, s);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << make_space(dent) << "converting "
					<< symbol_name << " to SymbolicDuration\n";
			}
			break;
		case apgen::DATA_TYPE::STRING:
			sym = new SymbolicString(true, s);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << make_space(dent) << "converting "
					<< symbol_name << " to SymbolicString\n";
			}
			break;
		case apgen::DATA_TYPE::ARRAY:
			sym = new SymbolicArray(true, s);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << make_space(dent) << "converting "
					<< symbol_name << " to SymbolicArray\n";
			}
			break;
		case apgen::DATA_TYPE::INSTANCE:
			sym = new SymbolicInstance(true, s);
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << make_space(dent) << "converting "
					<< symbol_name << " to SymbolicInstance\n";
			}
			break;
		default:
			assert(false);
			break;
	}
	if(sym) {
		pe.reference(sym);
		sym->my_level = level_to_use;
		sym->my_index = index_to_use;

		//
		// Symbol is always overridden by a specific typed symbol
		//
		// sym->val.generate_default_for_type(dt);
		// sym->val.declared_type = dt;
	} else {
		// take care of types that haven't been optimized
		// yet...
		s.my_level = level_to_use;
		s.my_index = index_to_use;

		//
		// Symbol is always overridden by a specific typed symbol
		//
		// s.val.generate_default_for_type(dt);
		// s.val.declared_type = dt;
	}
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidate_symbol: final = "
			<< pe->to_string() << ", type = " << apgen::spell(pe->get_result_type())
			<< "\n";
	}
}

void consolidate_qualified_symbol(
		parsedExp& pe,
		int dent,
		bool is_assigned_to /* = false */) {
    QualifiedSymbol* qsym = dynamic_cast<QualifiedSymbol*>(pe.object());
    assert(qsym);
    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "consolidate qualified symbol (" << qsym->spell() << ")\n";
	cerr << make_space(dent) << qsym->to_string() << "\n";
    }

    consolidate_symbol(qsym->symbol, dent + 4, is_assigned_to);

    Symbol* sym = dynamic_cast<Symbol*>(qsym->symbol.object());
    assert(sym);
    qsym->my_index = sym->my_index;
    qsym->my_level = sym->my_level;

    Cstring	symbol_name = sym->getData();

    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "consolidate_qualified_symbol (" << sym->my_level
	     << ", " << sym->my_index << ") " << symbol_name << "\n";
	cerr << make_space(dent) << qsym->to_string() << "\n";
    }

    Qualifications* qual = dynamic_cast<Qualifications*>(qsym->qualifications.object());
    bool this_is_an_indexed_value = false;
    bool this_is_a_class_member = false;

    //
    // See the comments in the QualifiedSymbol entry in
    // constructor-specifics.txt for a test case.
    // Go through it systematically.  The logic in this part of the
    // consolidation process should go hand-in-hand with the logic in the
    // evaluation functions in gen-parsedExp.C.
    //
    // Note that we are not trying to strongly type instance
    // variables. Therefore, we have no knowledge about
    // members of instances, and we cannot consolidate
    // a class member if we find one. All the work is
    // performed at runtime.
    //
    for(int i = 0; i < qual->IndicesOrMembers.size(); i++) {
	MultidimIndices* multi
	    = dynamic_cast<MultidimIndices*>(qual->IndicesOrMembers[i].object());
	ClassMember* cm =
	    dynamic_cast<ClassMember*>(qual->IndicesOrMembers[i].object());
	if(multi) {

	    //
	    // This is obsolete; only one index is allowed,
	    // at the very end, and it is stored inside the
	    // last member.
	    //
	    this_is_an_indexed_value = true;
	    smart_ptr<MultidimIndices> multiptr(multi);

	    //
	    // will optimize the indices if all integers:
	    //
	    consolidate_multidim_indices(multiptr, dent + 2);

	    //
	    // may have been optimized:
	    //
	    multi = multiptr.object();
	    qsym->indices.reference(multi);
	} else if(cm) {

	    //
	    // No consolidation here. Analysis of the
	    // class member will be performed at run time.
	    //
	    this_is_a_class_member = true;

	    //
	    // Allow for the possibility that this member
	    // is indexed internally
	    //
	    if(cm->OptionalIndex) {
		MultidimIndices* mi = dynamic_cast<MultidimIndices*>(cm->OptionalIndex.object());
		assert(mi);
		smart_ptr<MultidimIndices> multiptr(mi);
		consolidate_multidim_indices(multiptr, dent + 2);
	    }
	    qsym->a_member.reference(cm);
	} else {
		assert(false);
	}
    }

    //
    // We need to look at all possible cases that have been handled, and
    // any that cannot be handled for the time being.
    //
    if(this_is_an_indexed_value) {

	//
	// indexed symbol
	//
	if(sym->get_result_type() != apgen::DATA_TYPE::ARRAY) {
	    Cstring err;
	    err << "File " << qsym->file << ", line "
		<< qsym->line << ", Symbol " << symbol_name
		<< " should be an array, "
		<< " but it was declared as a(n) "
		<< apgen::spell(sym->get_result_type());
	    throw(eval_error(err));
	}

	//
	// get_val_ref will use the indices and a_member if applicable
	//
	if(!this_is_a_class_member) {
	    map<Cstring, pair<apgen::DATA_TYPE, int> >::const_iterator iter
			= get_task(sym->my_level)->homogeneoustype.find(symbol_name);
	    if(iter != get_current_task()->homogeneoustype.end()) {

		//
		// the symbol is indexed to the max
		//
		if(qsym->indices->actual_indices.size() == iter->second.second) {
		    if(iter->second.first == apgen::DATA_TYPE::FLOATING) {
			qsym = new IndexedSymbolicDouble(true, *qsym);
			pe.reference(qsym);
		    }
		}
	    }
	}
    } else if(this_is_a_class_member) {

	//
	// simple instance member
	//
	if(sym->get_result_type() != apgen::DATA_TYPE::INSTANCE) {
	    Cstring err;
	    err << "File " << qsym->file << ", line " << qsym->line << ", Symbol " << symbol_name
		<< " should be an instance, "
		<< " but it was declared as a(n) "
		<< apgen::spell(sym->get_result_type());
	    throw(eval_error(err));
	}
    }

    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "consolidate_qualified_symbol: final = "
	     << pe->to_string() << ", type = "
	     << apgen::spell(pe->get_result_type())
	     << ", declared type = "
	     << apgen::spell(pe->get_result_type()) << "\n";
    }
}

void consolidate_multidim_indices(smart_ptr<MultidimIndices>& miptr, int dent) {

    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "consolidating MultidimIndices " << miptr->getData() << "\n";
	cerr << make_space(dent) << miptr->to_string() << "\n";
    }

    bool all_integers = true;
    assert(miptr->actual_indices.size() == 0);

    for(int i = 0; i < miptr->expressions.size(); i++) {
	SingleIndex* si = dynamic_cast<SingleIndex*>(miptr->expressions[i].object());
	assert(si);
	consolidate_expression(si->Expression, dent + 2);
	if(si->Expression->get_result_type() != apgen::DATA_TYPE::INTEGER) {
	    all_integers = false;
	}
	miptr->actual_indices.push_back(si->Expression);
    }
    if(all_integers) {
	MultidimIntegers* mints = new MultidimIntegers(true, *miptr.object());
	miptr.reference(mints);
    }
}

void consolidate_expression(
		parsedExp&	 E,
		int		 dent,	// default 0
		apgen::DATA_TYPE dt	// default apgen::DATA_TYPE::UNINITIALIZED
		) {
	assert(E);

	//
	// First we handle freshly compiled special expressions (not part of
	// an adaptation file)
	//

	if(E->getData() == "SINGLE EXPRESSION") {

		//
		// grab the actual expression
		//

		assert(E->expressions.size() == 1);
		parsedExp F = E;
		E.dereference();
		E.reference(F->expressions[0].object());
	} else if(E->getData() == "SINGLE STATEMENT") {
		return;
	} else if(E->getData() == "SINGLE PARAMETER LIST") {
		return;
	}

	//
	// we follow the cascading possibilities in new-skeleton.txt:
	//

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidate_expression(" << E->spell() << ")\n";
	}

	Logical*	logic = dynamic_cast<Logical*>(E.object());
	if(logic) {
		consolidate_expression(logic->Lhs, dent + 2);
		consolidate_expression(logic->Rhs, dent + 2);
		bool seems_right = false;
		apgen::DATA_TYPE type1 = logic->Lhs->get_result_type();
		apgen::DATA_TYPE type2 = logic->Rhs->get_result_type();
		if(type1 == apgen::DATA_TYPE::UNINITIALIZED || type2 == apgen::DATA_TYPE::UNINITIALIZED) {
			seems_right = true;
		} else if(type1 == apgen::DATA_TYPE::BOOL_TYPE) {
			if(type2 == apgen::DATA_TYPE::BOOL_TYPE) {
				if(logic->Operator->getData() == "||") {
					seems_right = true;
					E.reference(new OrBool(true, *logic));
				} else if(logic->Operator->getData() == "&&") {
					seems_right = true;
					E.reference(new AndBool(true, *logic));
				}
			}
		}

		//
		// We do more work than usual here because the AND and OR operators
		// do not necessarily evaluate both arguments, so type mismatches
		// can go undetected if we don't figure them out now.
		//

		if(!seems_right) {
			Cstring err;
			err << "File " << E->file << ", line " << E->line
				<< ": trying to apply operator "
				<< logic->Operator->getData()
				<< " to at least one non-boolean type "
				<< apgen::spell(logic->Lhs->get_result_type())
				<< " and "
				<< apgen::spell(logic->Rhs->get_result_type());
			throw(eval_error(err));
		}

		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type "
				<< apgen::spell(E->get_result_type()) << "\n";
		}
		return;
	}
	Comparison* comp = dynamic_cast<Comparison*>(E.object());
	if(comp) {
		consolidate_expression(comp->maybe_compared, dent + 2);
		consolidate_expression(comp->maybe_compared_2, dent + 2);
		bool seems_right = false;
		apgen::DATA_TYPE type1 = comp->maybe_compared->get_result_type();
		apgen::DATA_TYPE type2 = comp->maybe_compared_2->get_result_type();
		if(type1 == apgen::DATA_TYPE::INTEGER) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				if(comp->Operator->getData() == "<") {
					seems_right = true;
					E.reference(new LessIntInt(true, *comp));
				} else if(comp->Operator->getData() == "<=") {
					seems_right = true;
					E.reference(new LessEqualIntInt(true, *comp));
				} else if(comp->Operator->getData() == ">") {
					seems_right = true;
					E.reference(new GreaterIntInt(true, *comp));
				} else if(comp->Operator->getData() == ">=") {
					seems_right = true;
					E.reference(new GreaterEqualIntInt(true, *comp));
				}
			} else if(type2 == apgen::DATA_TYPE::FLOATING) {
				if(comp->Operator->getData() == "<") {
					seems_right = true;
					E.reference(new LessIntDouble(true, *comp));
				} else if(comp->Operator->getData() == "<=") {
					seems_right = true;
					E.reference(new LessEqualIntDouble(true, *comp));
				} else if(comp->Operator->getData() == ">") {
					seems_right = true;
					E.reference(new GreaterIntDouble(true, *comp));
				} else if(comp->Operator->getData() == ">=") {
					seems_right = true;
					E.reference(new GreaterEqualIntDouble(true, *comp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::FLOATING) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				if(comp->Operator->getData() == "<") {
					seems_right = true;
					E.reference(new LessDoubleInt(true, *comp));
				} else if(comp->Operator->getData() == "<=") {
					seems_right = true;
					E.reference(new LessEqualDoubleInt(true, *comp));
				} else if(comp->Operator->getData() == ">") {
					seems_right = true;
					E.reference(new GreaterDoubleInt(true, *comp));
				} else if(comp->Operator->getData() == ">=") {
					seems_right = true;
					E.reference(new GreaterEqualDoubleInt(true, *comp));
				}
			} else if(type2 == apgen::DATA_TYPE::FLOATING) {
				if(comp->Operator->getData() == "<") {
					seems_right = true;
					E.reference(new LessDoubleDouble(true, *comp));
				} else if(comp->Operator->getData() == "<=") {
					seems_right = true;
					E.reference(new LessEqualDoubleDouble(true, *comp));
				} else if(comp->Operator->getData() == ">") {
					seems_right = true;
					E.reference(new GreaterDoubleDouble(true, *comp));
				} else if(comp->Operator->getData() == ">=") {
					seems_right = true;
					E.reference(new GreaterEqualDoubleDouble(true, *comp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::TIME) {
			if(type2 == apgen::DATA_TYPE::TIME) {
				if(comp->Operator->getData() == "<") {
					seems_right = true;
					E.reference(new LessTimeTime(true, *comp));
				} else if(comp->Operator->getData() == "<=") {
					seems_right = true;
					E.reference(new LessEqualTimeTime(true, *comp));
				} else if(comp->Operator->getData() == ">") {
					seems_right = true;
					E.reference(new GreaterTimeTime(true, *comp));
				} else if(comp->Operator->getData() == ">=") {
					seems_right = true;
					E.reference(new GreaterEqualTimeTime(true, *comp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::DURATION) {
			if(type2 == apgen::DATA_TYPE::DURATION) {
				if(comp->Operator->getData() == "<") {
					seems_right = true;
					E.reference(new LessDurationDuration(true, *comp));
				} else if(comp->Operator->getData() == "<=") {
					seems_right = true;
					E.reference(new LessEqualDurationDuration(true, *comp));
				} else if(comp->Operator->getData() == ">") {
					seems_right = true;
					E.reference(new GreaterDurationDuration(true, *comp));
				} else if(comp->Operator->getData() == ">=") {
					seems_right = true;
					E.reference(new GreaterEqualDurationDuration(true, *comp));
				}
			}
		}
		if(type1 == apgen::DATA_TYPE::UNINITIALIZED || type2 == apgen::DATA_TYPE::UNINITIALIZED) {
			seems_right = true;
		}
		if(!seems_right) {
			Cstring err;
			err << "File " << E->file << ", line " << E->line
				<< ": trying to apply operator "
				<< comp->Operator->getData()
				<< " to incompatible types "
				<< apgen::spell(comp->maybe_compared->get_result_type())
				<< " and "
				<< apgen::spell(comp->maybe_compared_2->get_result_type());
			throw(eval_error(err));
		}

		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(E->get_result_type()) << "\n";
		}
		return;
	}
	AdditiveExp* additive_exp = dynamic_cast<AdditiveExp*>(E.object());
	if(additive_exp) {
		apgen::DATA_TYPE type1 = apgen::DATA_TYPE::UNINITIALIZED;
		apgen::DATA_TYPE type2 = apgen::DATA_TYPE::UNINITIALIZED;
		consolidate_expression(additive_exp->Lhs, dent + 2);
		consolidate_expression(additive_exp->Rhs, dent + 2);
		type1 = additive_exp->Lhs->get_result_type();
		type2 = additive_exp->Rhs->get_result_type();

		bool seems_right = false;

		if(type1 == apgen::DATA_TYPE::INTEGER) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				if(additive_exp->Operator->getData() == "+") {
					seems_right = true;
					E.reference(new PlusIntInt(true, *additive_exp));
				} else if(additive_exp->Operator->getData() == "-") {
					seems_right = true;
					E.reference(new MinusIntInt(true, *additive_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::FLOATING) {
				if(additive_exp->Operator->getData() == "+") {
					seems_right = true;
					E.reference(new PlusIntDouble(true, *additive_exp));
				} else if(additive_exp->Operator->getData() == "-") {
					seems_right = true;
					E.reference(new MinusIntDouble(true, *additive_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::FLOATING) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				if(additive_exp->Operator->getData() == "+") {
					seems_right = true;
					E.reference(new PlusDoubleInt(true, *additive_exp));
				} else if(additive_exp->Operator->getData() == "-") {
					seems_right = true;
					E.reference(new MinusDoubleInt(true, *additive_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::FLOATING) {
				if(additive_exp->Operator->getData() == "+") {
					seems_right = true;
					E.reference(new PlusDoubleDouble(true, *additive_exp));
				} else if(additive_exp->Operator->getData() == "-") {
					seems_right = true;
					E.reference(new MinusDoubleDouble(true, *additive_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::TIME) {
			if(type2 == apgen::DATA_TYPE::DURATION) {
				if(additive_exp->Operator->getData() == "+") {
					seems_right = true;
					E.reference(new PlusTimeDuration(true, *additive_exp));
				} else if(additive_exp->Operator->getData() == "-") {
					seems_right = true;
					E.reference(new MinusTimeDuration(true, *additive_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::TIME) {
				if(additive_exp->Operator->getData() == "-") {
					seems_right = true;
					E.reference(new MinusTimeTime(true, *additive_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::DURATION) {
			if(type2 == apgen::DATA_TYPE::DURATION) {
				if(additive_exp->Operator->getData() == "+") {
					seems_right = true;
					E.reference(new PlusDurationDuration(true, *additive_exp));
				} else if(additive_exp->Operator->getData() == "-") {
					seems_right = true;
					E.reference(new MinusDurationDuration(true, *additive_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::TIME) {
				if(additive_exp->Operator->getData() == "+") {
					seems_right = true;
					E.reference(new PlusDurationTime(true, *additive_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::STRING) {
			seems_right = true;
		} else if(type1 == apgen::DATA_TYPE::ARRAY) {
			if(type2 == apgen::DATA_TYPE::ARRAY) {
				if(additive_exp->Operator->getData() == "+") {
					seems_right = true;
					E.reference(new PlusArrayArray(true, *additive_exp));
				}
			}
		}
		if(type1 == apgen::DATA_TYPE::UNINITIALIZED || type2 == apgen::DATA_TYPE::UNINITIALIZED) {
			seems_right = true;
		}
		if(!seems_right) {
			Cstring err;
			err << "File " << E->file << ", line " << E->line
				<< ": trying to apply operator "
				<< additive_exp->Operator->getData()
				<< " to incompatible types "
				<< apgen::spell(additive_exp->Lhs->get_result_type())
				<< " and "
				<< apgen::spell(additive_exp->Rhs->get_result_type());
			throw(eval_error(err));
		}
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(E->get_result_type()) << "\n";
			cerr << make_space(dent + 2) << "final " << E->to_string() << "\n";
		}
		return;
	}
	EqualityTest* equality_exp = dynamic_cast<EqualityTest*>(E.object());
	if(equality_exp) {
		consolidate_expression(equality_exp->maybe_equal, dent + 2);
		consolidate_expression(equality_exp->maybe_equal_2, dent + 2);
		bool seems_right = false;
		apgen::DATA_TYPE type1 = equality_exp->maybe_equal->get_result_type();
		apgen::DATA_TYPE type2 = equality_exp->maybe_equal_2->get_result_type();
		if(type1 == apgen::DATA_TYPE::INTEGER) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				if(equality_exp->Operator->getData() == "==") {
					seems_right = true;
					E.reference(new EqualsInt(true, *equality_exp));
				} else if(equality_exp->Operator->getData() == "!=") {
					seems_right = true;
					E.reference(new UnequalsInt(true, *equality_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::STRING) {
			if(type2 == apgen::DATA_TYPE::STRING) {
				if(equality_exp->Operator->getData() == "==") {
					seems_right = true;
					E.reference(new EqualsString(true, *equality_exp));
				} else if(equality_exp->Operator->getData() == "!=") {
					seems_right = true;
					E.reference(new UnequalsString(true, *equality_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::BOOL_TYPE) {
			if(type2 == apgen::DATA_TYPE::BOOL_TYPE) {
				if(equality_exp->Operator->getData() == "==") {
					seems_right = true;
					E.reference(new EqualsBool(true, *equality_exp));
				} else if(equality_exp->Operator->getData() == "!=") {
					seems_right = true;
					E.reference(new UnequalsBool(true, *equality_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::DURATION) {
			if(type2 == apgen::DATA_TYPE::DURATION) {
				if(equality_exp->Operator->getData() == "==") {
					seems_right = true;
					E.reference(new EqualsDuration(true, *equality_exp));
				} else if(equality_exp->Operator->getData() == "!=") {
					seems_right = true;
					E.reference(new UnequalsDuration(true, *equality_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::TIME) {
			if(type2 == apgen::DATA_TYPE::TIME) {
				if(equality_exp->Operator->getData() == "==") {
					seems_right = true;
					E.reference(new EqualsTime(true, *equality_exp));
				} else if(equality_exp->Operator->getData() == "!=") {
					seems_right = true;
					E.reference(new UnequalsTime(true, *equality_exp));
				}
			}
		}
		if(type1 == apgen::DATA_TYPE::UNINITIALIZED || type2 == apgen::DATA_TYPE::UNINITIALIZED) {
			seems_right = true;
		}
		if(!seems_right) {
			Cstring err;
			err << "File " << E->file << ", line " << E->line
				<< ": trying to apply operator "
				<< equality_exp->Operator->getData()
				<< " to incompatible types "
				<< apgen::spell(equality_exp->maybe_equal->get_result_type())
				<< " and "
				<< apgen::spell(equality_exp->maybe_equal_2->get_result_type());
			throw(eval_error(err));
		}

		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(E->get_result_type())
				<< ", replaced EqualityTest by " << E->spell() << "\n";
		}
		return;
	}
	MultiplicativeExp* multi_exp = dynamic_cast<MultiplicativeExp*>(E.object());
	if(multi_exp) {
		consolidate_expression(multi_exp->maybe_a_product, dent + 2);
		consolidate_expression(multi_exp->maybe_a_factor, dent + 2);

		apgen::DATA_TYPE type1 = multi_exp->maybe_a_product->get_result_type();
		apgen::DATA_TYPE type2 = multi_exp->maybe_a_factor->get_result_type();
		if(type1 == apgen::DATA_TYPE::INTEGER) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				if(multi_exp->Operator->getData() == "*") {
					E.reference(new TimesIntInt(true, *multi_exp));
				} else if(multi_exp->Operator->getData() == "/") {
					E.reference(new DivideIntInt(true, *multi_exp));
				} else if(multi_exp->Operator->getData() == "%") {
					E.reference(new ModIntInt(true, *multi_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::FLOATING) {
				if(multi_exp->Operator->getData() == "*") {
					E.reference(new TimesIntDouble(true, *multi_exp));
				} else if(multi_exp->Operator->getData() == "/") {
					E.reference(new DivideIntDouble(true, *multi_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::DURATION) {
				if(multi_exp->Operator->getData() == "*") {
					E.reference(new TimesIntDuration(true, *multi_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::FLOATING) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				if(multi_exp->Operator->getData() == "*") {
					E.reference(new TimesDoubleInt(true, *multi_exp));
				} else if(multi_exp->Operator->getData() == "/") {
					E.reference(new DivideDoubleInt(true, *multi_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::FLOATING) {
				if(multi_exp->Operator->getData() == "*") {
					E.reference(new TimesDoubleDouble(true, *multi_exp));
				} else if(multi_exp->Operator->getData() == "/") {
					E.reference(new DivideDoubleDouble(true, *multi_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::DURATION) {
				if(multi_exp->Operator->getData() == "*") {
					E.reference(new TimesDoubleDuration(true, *multi_exp));
				}
			}
		} else if(type1 == apgen::DATA_TYPE::DURATION) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				if(multi_exp->Operator->getData() == "*") {
					E.reference(new TimesDurationInt(true, *multi_exp));
				} else if(multi_exp->Operator->getData() == "/") {
					E.reference(new DivideDurationInt(true, *multi_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::FLOATING) {
				if(multi_exp->Operator->getData() == "*") {
					E.reference(new TimesDurationDouble(true, *multi_exp));
				} else if(multi_exp->Operator->getData() == "/") {
					E.reference(new DivideDurationDouble(true, *multi_exp));
				}
			} else if(type2 == apgen::DATA_TYPE::DURATION) {
				if(multi_exp->Operator->getData() == "/") {
					E.reference(new DivideDurationDuration(true, *multi_exp));
				} else if(multi_exp->Operator->getData() == "%") {
					E.reference(new ModDurationDuration(true, *multi_exp));
				}
			}
		} else {
			assert(multi_exp->binaryFunc);
		}
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(E->get_result_type()) << "\n";
		}
		return;
	}
	RaiseToPower* raise_to = dynamic_cast<RaiseToPower*>(E.object());
	if(raise_to) {
		consolidate_expression(raise_to->maybe_a_factor, dent + 2);
		consolidate_expression(raise_to->atom, dent + 2);

		apgen::DATA_TYPE type1 = raise_to->maybe_a_factor->get_result_type();
		apgen::DATA_TYPE type2 = raise_to->atom->get_result_type();
		if(type1 == apgen::DATA_TYPE::INTEGER) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				E.reference(new RaiseToPowerIntInt(true, *raise_to));
			} else if(type2 == apgen::DATA_TYPE::FLOATING) {
				E.reference(new RaiseToPowerIntDouble(true, *raise_to));
			}
		} else if(type1 == apgen::DATA_TYPE::FLOATING) {
			if(type2 == apgen::DATA_TYPE::INTEGER) {
				E.reference(new RaiseToPowerDoubleInt(true, *raise_to));
			} else if(type2 == apgen::DATA_TYPE::FLOATING) {
				E.reference(new RaiseToPowerDoubleDouble(true, *raise_to));
			}
		} else {
			assert(raise_to->binaryFunc);
		}
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(raise_to->get_result_type()) << "\n";
		}
		return;
	}
	UnaryMinus* ufunc = dynamic_cast<UnaryMinus*>(E.object());
	if(ufunc) {
		consolidate_expression(ufunc->exp_modifiable_by_unary_minus, dent + 2);

		//
		// operator is encoded as ufunc->Operator; it was
		// handled by FuncUnary::initExpression() in pE_system_2.C
		//

		if(ufunc->exp_modifiable_by_unary_minus->get_result_type() == apgen::DATA_TYPE::INTEGER) {
			E.reference(new MinusInt(true, *ufunc));
		} else if(ufunc->exp_modifiable_by_unary_minus->get_result_type() == apgen::DATA_TYPE::FLOATING) {
			E.reference(new MinusDouble(true, *ufunc));
		} else if(ufunc->exp_modifiable_by_unary_minus->get_result_type() == apgen::DATA_TYPE::DURATION) {
			E.reference(new MinusDuration(true, *ufunc));
		}
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(E->get_result_type()) << "\n";
		}
		return;
	}

	//
	// track down descendants of the "atom" syntax rule
	//

	Constant* con = dynamic_cast<Constant*>(E.object());
	if(con) {

		//
		// could be a time, a duration (base or timesystem-based), PI,
		// RAD, a string, a number, false, true. All these cases are
		// handled by the Constant constructor in gen-parsedExp.C;
		// there is nothing more to do (except try to be smart in
		// the case of an array...
		//

		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "consolidate_expression(" << con->getData()
				<< "): Constant constructor handled everything, ";
			cerr << "= " << con->val.to_string() << "\n";
			cerr << make_space(dent + 2) << "type " << apgen::spell(con->get_result_type()) << "\n";
		}
		if(dt == apgen::DATA_TYPE::UNINITIALIZED) {
			;
		} else if(dt == apgen::DATA_TYPE::INSTANCE && con->val.get_type() == apgen::DATA_TYPE::STRING) {
			con->val.generate_default_for_type(apgen::DATA_TYPE::INSTANCE);
		} else if(con->val.get_type() == apgen::DATA_TYPE::UNINITIALIZED) {
			con->val.generate_default_for_type(dt);
		} else if(!con->val.is_compatible_with_type(dt)) {
			Cstring err;
			err << "File " << E->file << ", line " << E->line
				<< ": trying to cast an expression of type "
				<< apgen::spell(con->val.get_type())
				<< " to type " << apgen::spell(dt);
			throw(eval_error(err));
		} else {
			con->val.declared_type = dt;
		}
		if(con->is_timesystem_based) {

			//
			// the one thing the constructor did not do was fetch
			// the time system definition and fully evaluate the
			// value of the constant, which we do now
			//

			task* T = Behavior::GlobalType().tasks[0];
			map<Cstring, int>::const_iterator iter = T->get_varindex().find(con->timesystem_name);
			if(iter == T->get_varindex().end()) {
				Cstring err;
				err << "File " << con->file << ", line " << con->line << ": time system "
					<< con->timesystem_name << " does not exist";
				throw(eval_error(err));
			}
			int ts_index = iter->second;
			TypedValue& ts_arrayval = (*behaving_object::GlobalObject())[ts_index];
			ArrayElement* origin_ae = ts_arrayval.get_array()["origin"];
			ArrayElement* scale_ae = ts_arrayval.get_array()["scale"];
			if(!origin_ae) {
				Cstring err;
				err << "File " << con->file << ", line " << con->line << ": time system "
				    << con->timesystem_name << " does not have an origin\n";
				throw(eval_error(err));
			}
			if(!scale_ae) {
				Cstring err;
				err << "File " << con->file << ", line " << con->line << ": time system "
				    << con->timesystem_name << " does not have a scale\n";
				throw(eval_error(err));
			}
			if(origin_ae->payload.get_type() != apgen::DATA_TYPE::TIME) {
				Cstring err;
				err << "File " << con->file << ", line " << con->line << ": time system "
					<< con->timesystem_name << " has an origin that is not a time";
				throw(eval_error(err));
			}
			if(!scale_ae->payload.is_numeric()) {
				Cstring err;
				err << "File " << con->file << ", line " << con->line << ": time system "
					<< con->timesystem_name << " has an scale that is not a number";
				throw(eval_error(err));
			}
			CTime_base the_value = origin_ae->payload.get_time_or_duration()
				+ scale_ae->payload.get_double() * con->timesystem_duration;
			con->val = the_value;
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << make_space(dent + 2) << "consolidate_expression(" << con->getData()
					<< "): computed time  = " << the_value.to_string() << "\n";
			}
		} else switch(con->get_result_type()) {
			case apgen::DATA_TYPE::INTEGER:
				E.reference(new ConstantInt(true, *con));
				if(APcloptions::theCmdLineOptions().debug_grammar) {
					cerr << make_space(dent + 2) << "converting Constant to ConstantInt\n";
				}
				break;
			case apgen::DATA_TYPE::FLOATING:
				E.reference(new ConstantDouble(true, *con));
				if(APcloptions::theCmdLineOptions().debug_grammar) {
					cerr << make_space(dent + 2) << "converting Constant to ConstantDouble\n";
				}
				break;
			case apgen::DATA_TYPE::BOOL_TYPE:
				E.reference(new ConstantBool(true, *con));
				if(APcloptions::theCmdLineOptions().debug_grammar) {
					cerr << make_space(dent + 2) << "converting Constant to ConstantBool\n";
				}
				break;
			case apgen::DATA_TYPE::TIME:
				E.reference(new ConstantTime(true, *con));
				if(APcloptions::theCmdLineOptions().debug_grammar) {
					cerr << make_space(dent + 2) << "converting Constant to ConstantTime\n";
				}
				break;
			case apgen::DATA_TYPE::DURATION:
				E.reference(new ConstantDuration(true, *con));
				if(APcloptions::theCmdLineOptions().debug_grammar) {
					cerr << make_space(dent + 2) << "converting Constant to ConstantDuration\n";
				}
				break;
			case apgen::DATA_TYPE::STRING:
				E.reference(new ConstantString(true, *con));
				if(APcloptions::theCmdLineOptions().debug_grammar) {
					cerr << make_space(dent + 2) << "converting Constant to ConstantString\n";
				}
				break;
			case apgen::DATA_TYPE::ARRAY:
				E.reference(new ConstantArray(true, *con));
				if(APcloptions::theCmdLineOptions().debug_grammar) {
					cerr << make_space(dent + 2) << "converting Constant to ConstantArray\n";
				}
				break;
			case apgen::DATA_TYPE::INSTANCE:
				E.reference(new ConstantInstance(true, *con));
				if(APcloptions::theCmdLineOptions().debug_grammar) {
					cerr << make_space(dent + 2) << "converting Constant to ConstantInstance\n";
				}
				break;
			default:
				break;
		}
		return;
	}
	Array* arr = dynamic_cast<Array*>(E.object());
	if(arr) {
		arr->consolidate(dent);
		return;
	}

	Parentheses* paren = dynamic_cast<Parentheses*>(E.object());
	if(paren) {
		consolidate(*paren, dent + 2);
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(paren->get_result_type()) << "\n";
		}
		return;
	}
	DataTypeDefaultValue* defaultval = dynamic_cast<DataTypeDefaultValue*>(E.object());
	if(defaultval) {
		consolidate(*defaultval, dent + 2);
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(defaultval->get_result_type()) << "\n";
		}
		return;
	}
		ByValueSymbol* byval = dynamic_cast<ByValueSymbol*>(E.object());
	if(byval) {
		byval->consolidate(dent + 2);
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(byval->get_result_type()) << "\n";
		}
		return;
	}
	QualifiedSymbol* qsym = dynamic_cast<QualifiedSymbol*>(E.object());
	if(qsym) {
		consolidate_qualified_symbol(E, dent);
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(E->get_result_type()) << "\n";
		}
		return;
	}
	Symbol* sym = dynamic_cast<Symbol*>(E.object());
	if(sym) {
		consolidate_symbol(E, dent);
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(E->get_result_type()) << "\n";
		}
		return;
	}
	FunctionCall* func_call = dynamic_cast<FunctionCall*>(E.object());
	if(func_call) {
		consolidate_function_call_exp(E, dent + 2);
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent + 2) << "type " << apgen::spell(E->get_result_type()) << "\n";
		}
		return;
	}
	aoString err;
	err << "File " << E->file << ", line " << E->line
		<< ": expression of unrecognizable type "<< E->spell() << "\n";
	err << "Content: " << E->to_string();
	throw(eval_error(err.str()));
}

//
// As part of sanitizing JSON plan output, we'd like to know, for
// every activity parameter that is an empty array, whether that 
// array is intended to be treated as a list or as a struct.
//
// The easiest way to go after this information is to hunt assignments
// whose left-hand side is a parameter symbol. There are other assignments
// in disguise when calling a function (or an abstract resource) with an
// argument that is an array, so in principle those should be monitored
// as well. Hopefully, that will not be necessary; the sanitization
// process is not intended to be rigorous, at least not for the time
// being.
//
// It is easy to figure out the context in which an assignment is
// consolidated: just consult the task currently on the stack.
//
void consolidate_assignment(
		smart_ptr<executableExp>& pe,
		int dent,
		bool inside_directive) {
    Assignment* assign = dynamic_cast<Assignment*>(pe.object());
    assert(assign);

    assert(LevelOfCurrentTask >= 0);
    task* current_task = get_current_task();

    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "consolidating Assignment "
	     << assign->getData() << "\n";
    }

    //
    // There are two contexts for Assignment:
    //
    // 	- an assignment statement in a Program
    // 	- a 'keyword = value' item in an Array
    //
    // The first type is handled by the Assignment::execute() method,
    // which is automatically invoked when running the Program. Therefore,
    // consolidation should make sure that execute() will run w/o
    // problems.
    //
    // The second type should be handled by Array::eval_expression(). The
    // Array class contains Assignments which should be taken apart by the
    // Array consolidation method; those Assignments should therefore
    // never be executed.
    //
    // So the only case we should worry about here is the first case. We
    // can easily tell which case applies by looking at the assignment_op
    // data member, which is only defined in the first case.
    //
    if(assign->assignment_op) {

	//
	// execute() will want to stick the rhs into the lhs.
	// We need to make sure that the lhs is a properly defined
	// Symbol - the right way to do this is to call
	// consolidate(Symbol).
	//
	Symbol* lhs = dynamic_cast<Symbol*>(assign->lhs_of_assign.object());
	QualifiedSymbol* qual_lhs = dynamic_cast<QualifiedSymbol*>(assign->lhs_of_assign.object());
	Cstring name;
	if(lhs) {
	    name = lhs->getData();
	} else if(qual_lhs) {
	    name = qual_lhs->symbol->getData();
	} else {

	    //
	    // APGenX idiosyncrasy: attributes can be set by referencing their
	    // "official" name as a string in the l.h.s. of an assignment
	    //
	    Constant* string_lhs = dynamic_cast<Constant*>(assign->lhs_of_assign.object());
	    assert(string_lhs);

	    Cstring lhs_as_string = string_lhs->getData();
	    removeQuotes(lhs_as_string);
	    if(inside_directive) {

			//
			// Directive case
			//
			consolidate_expression(assign->Expression, dent + 2);

			//
			// Not necessary - done in exec_agent::executeNewDirectives()
			//
			// assign->Expression->eval_expression(
			// 		behaving_object::GlobalObject());
			directives() << new bsymbolnode(lhs_as_string, assign->Expression);
			return;
	    }
	    map<Cstring, pair<Cstring, int> >::const_iterator iter =
			string_to_nickname().find(assign->lhs_of_assign->getData());
	    if(iter == string_to_nickname().end()) {
			Cstring err;
			err << "File " << assign->file << ", line " << assign->line
				<< ", assignment consolidation: could not find nickname "
				"for l.h.s. " << assign->lhs_of_assign->getData();
			throw(eval_error(err));
	    }
	    name = iter->second.first;
	    assign->original_lhs = iter->first;

	    //
	    // we are going to replace this expression by a
	    // Symbol. That's OK except that to_stream will get
	    // confused. Need to fix that at some point.
	    //
	    parsedExp  nameExp(new pE_w_string(
					origin_info(assign->line,
						assign->file),
					name));
	    lhs = new Symbol(0, nameExp, Differentiator_0(0));
	    lhs->file = assign->file;
	    lhs->line = assign->line;
	    assign->lhs_of_assign.dereference();
	    assign->lhs_of_assign.reference(lhs);
	}

	//
	// will replace lhs_of_assign.object() by an optimized version:
	//
	if(lhs) {

	    //
	    // This call should throw an exception if the assignment
	    // attempts to modify a constant global:
	    //
	    consolidate_symbol(assign->lhs_of_assign, dent + 2, /* is_assigned_to = */ true);

	    //
	    // grab the potentially new object:
	    //
	    lhs = dynamic_cast<Symbol*>(assign->lhs_of_assign.object());
	} else if(qual_lhs) {

	    //
	    // This call should throw an exception if the assignment
	    // attempts to modify a constant global:
	    //
	    consolidate_qualified_symbol(assign->lhs_of_assign, dent + 2, /* is_assigned_to */ true);
	}
	if(APcloptions::theCmdLineOptions().debug_grammar) {
	    cerr << make_space(dent) << "consolidate "
		 << assign->spell() << " (a) lhs = "
		 << assign->lhs_of_assign->to_string() << "\n";
	}

	//
	// next we consolidate the rhs
	//
	consolidate_expression(assign->Expression, dent + 2);
	if(APcloptions::theCmdLineOptions().debug_grammar) {
	    aoString aos;
	    aos << make_space(dent) << "consolidate "
		<< assign->spell() << " (b) rhs = "
		<< assign->Expression->to_string() << "\n"
		<< make_space(dent) << "consolidate "
		<< assign->spell() << " (c) a = "
		<< assign->to_string() << "\n";
	    cerr << aos.str();
	}

	//
	// Finally, we optimize the assignment. Before anything
	// else, we need to check if this is an activity attribute,
	// which requires a special subclass of assignment called
	// assign_w_side_effects.
	//
	bool seems_right = false;
	bool is_an_attribute = false;
	if(  lhs && lhs->my_level == 1) {
	    map<Cstring, Cstring>::const_iterator iter
			= nickname_to_activity_string().find(
				lhs->getData());
	    if(iter != nickname_to_activity_string().end()) {
		is_an_attribute = true;
	    	if(assign->assignment_op->getData() == "=") {
		    seems_right = true;
		    pe.reference(new Assign2Attr(*assign));
		    assign = dynamic_cast<Assignment*>(pe.object());
		} else if(assign->assignment_op->getData() == "+=") {
		    if(   assign->lhs_of_assign->get_result_type() != apgen::DATA_TYPE::ARRAY
			  || assign->Expression->get_result_type() != apgen::DATA_TYPE::ARRAY) {
			Cstring err;
			err << "File " << assign->file << ", line " << assign->line
			    << ": trying to append to variable of type "
			    << apgen::spell(assign->lhs_of_assign->get_result_type())
			    << " a variable of type "
			    << apgen::spell(assign->Expression->get_result_type());
			throw(eval_error(err));
		    }
		    seems_right = true;
		    pe.reference(new AppendArray2Array(true, *assign));
		    assign = dynamic_cast<Assignment*>(pe.object());
		}
	    }
	}
	
	//
	// Now we optimize the assignment, if it's not an attribute
	// (we dealt with that case above)
	//
	apgen::DATA_TYPE left_type
			= assign->lhs_of_assign->get_result_type();
	apgen::DATA_TYPE right_type
			= assign->Expression->get_result_type();

	//
	// Before optimizing, since we now have type information
	// about the lhs and the rhs, and since in all cases we
	// have a symbol name (name) and either a pointer to a
	// Symbol (lhs) or a pointer to a qualified symbol (qual_lhs),
	// now is a good time to determine whether the symbol is a
	// parameter of an activity type, and if so, whether it's an
	// array, and if so, whether the array is a list or a struct.
	//
	// No, this is too cumbersome - let's not do it.
	//

	if( !is_an_attribute
	    && assign->assignment_op->getData() == "=") {

	    if(left_type == apgen::DATA_TYPE::INTEGER) {

		if(right_type == apgen::DATA_TYPE::INTEGER) {
			seems_right = true;
			assert(!strcmp(assign->lhs_of_assign->spell(), "SymbolicInt"));
			pe.reference(new AssignInt2Int(true, *assign));
		} else if(right_type == apgen::DATA_TYPE::FLOATING) {
			seems_right = true;
			pe.reference(new AssignDouble2Int(true, *assign));
		} else if(right_type == apgen::DATA_TYPE::UNINITIALIZED) {
			seems_right = true;
		}
	    } else if(left_type == apgen::DATA_TYPE::FLOATING) {

		if(right_type == apgen::DATA_TYPE::INTEGER) {
			seems_right = true;
			pe.reference(new AssignInt2Double(true, *assign));
		} else if(right_type == apgen::DATA_TYPE::FLOATING) {
			seems_right = true;
			pe.reference(new AssignDouble2Double(true, *assign));
		} else if(right_type == apgen::DATA_TYPE::UNINITIALIZED) {
			seems_right = true;
		}
	    } else if(left_type == apgen::DATA_TYPE::BOOL_TYPE
		      && right_type == apgen::DATA_TYPE::BOOL_TYPE) {

		seems_right = true;
		pe.reference(new AssignBool2Bool(true, *assign));
	    } else if(left_type == apgen::DATA_TYPE::TIME
		      && right_type == apgen::DATA_TYPE::TIME) {

		seems_right = true;
		pe.reference(new AssignTime2Time(true, *assign));
	    } else if(left_type == apgen::DATA_TYPE::DURATION
		      && right_type == apgen::DATA_TYPE::DURATION) {

		seems_right = true;
		pe.reference(new AssignDuration2Duration(true, *assign));
	    } else if(left_type == apgen::DATA_TYPE::STRING
		      && right_type == apgen::DATA_TYPE::STRING) {

		seems_right = true;
		pe.reference(new AssignString2String(true, *assign));
	    } else if(left_type == apgen::DATA_TYPE::ARRAY
		      && right_type == apgen::DATA_TYPE::ARRAY) {

		seems_right = true;
		pe.reference(new AssignArray2Array(true, *assign));
	    } else if(left_type == apgen::DATA_TYPE::INSTANCE
		      && right_type == apgen::DATA_TYPE::INSTANCE) {
		seems_right = true;
		pe.reference(new AssignInstance2Instance(true, *assign));
	    } else if(left_type == apgen::DATA_TYPE::UNINITIALIZED) {

		//
		// Must be an assignment to an array element
		//
		seems_right = true;
	    } else if(right_type == apgen::DATA_TYPE::UNINITIALIZED) {

		//
		// Must be assigning a computed value such as instance
		// member which cannot be ascertained at consolidation time
		//
		seems_right = true;
	    }
	} else if(!is_an_attribute && assign->assignment_op->getData() == "+=") {
	    if(   left_type != apgen::DATA_TYPE::ARRAY
		  || right_type != apgen::DATA_TYPE::ARRAY) {
		Cstring err;
		err << "File " << assign->file << ", line " << assign->line
		    << ": trying to append to variable of type "
		    << apgen::spell(left_type)
		    << " a variable of type "
		    << apgen::spell(right_type);
		throw(eval_error(err));
	    }
	    seems_right = true;
	    pe.reference(new AppendArray2Array(true, *assign));
	}
	if(!seems_right) {
	    Cstring err;
	    err << "File " << assign->file << ", line "
		<< assign->line << ": trying to assign "
		<< apgen::spell(right_type) << " to a(n) "
		<< apgen::spell(left_type);
	    throw(eval_error(err));
	}
	if(APcloptions::theCmdLineOptions().debug_grammar) {
	    aoString aos;
	    cerr << make_space(dent) << "Final content of "
		 << pe->spell() << ":\n";
	    pe->to_stream(&aos, dent + 2);
	    cerr << aos.str() << "\n";
	}
    }
}

void consolidate_return(Return& r, apgen::DATA_TYPE& dt, int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidating Return " << r.getData() << "\n";
	}
	consolidate_expression(r.Expression, dent + 2);
	dt = r.Expression->get_result_type();
}

void consolidate_continue(Continue& c, int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidating Continue " << c.getData() << "\n";
	}

	//
	// Nothing much to do... the smarts are in the execute() method,
	// which is implemented in pE_system.C.
	//
}

void consolidate_while(
		smart_ptr<executableExp>& E,
		apgen::DATA_TYPE& return_type,
		apgen::METHOD_TYPE method_type,
		int dent) {
	While* wh = dynamic_cast<While*>(E.object());
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidating While " << wh->getData() << "\n";
	}
	consolidate_expression(wh->while_statement_header, dent + 2);
	Program* pr = dynamic_cast<Program*>(wh->program.object());
	assert(pr);
	pr->orig_section = method_type;
	consolidate_program(*pr, dent + 2);
	return_type = pr->ReturnType;
}

void consolidate_kw_pairs(
		Assignment&		assign,
		vector<parsedExp>&	the_keys,
		vector<parsedExp>&	the_values,
		int			dent) {
	consolidate_expression(assign.Expression, dent + 2);
	the_keys.push_back(assign.Expression);
	consolidate_expression(assign.Expression_2, dent + 2);
	the_values.push_back(assign.Expression_2);
	for(int i = 0; i < assign.expressions.size(); i++) {
		bool must_consolidate = true;
		if(assign.expressions[i]->getData() == ",") {
			must_consolidate = false;
		}
		if(must_consolidate) {
			Assignment* assign2 = dynamic_cast<Assignment*>(assign.expressions[i].object());
			assert(assign2);
			consolidate_expression(assign2->Expression, dent + 2);
			the_keys.push_back(assign2->Expression);
			consolidate_expression(assign2->Expression_2, dent + 2);
			the_values.push_back(assign2->Expression_2);
		}
	}
}

void consolidate(Parentheses& p, int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidating Parentheses " << p.getData() << "\n";
	}
	consolidate_expression(p.Expression, dent + 2);
}

void consolidate(DataTypeDefaultValue& d, int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "consolidating DataTypeDefaultValue "
			<< d.getData() << "\n";
	}
	if(d.tok_dyn_type) {
		ArrayElement* tds = list_of_all_typedefs().find(
					d.tok_dyn_type->getData());
		if(!tds) {
			Cstring errs;
			errs << "File " << d.file << ", line " << d.line
				<< ": dynamic type \"" << d.tok_dyn_type->getData()
				<< "\" does not exist";
			throw(eval_error(errs));
		}
		d.val.recursively_copy(tds->Val().get_array());
	} else {
		d.val.generate_default_for_type(d.builtin_type->getData());
	}
}

stringtlist& resource_methods() {
	static stringtlist	L;
	static bool		initialized = false;
	if(!initialized) {
		initialized = false;
		L << new emptySymbol("currentval");
		L << new emptySymbol("value");
		L << new emptySymbol("interpval");
		L << new emptySymbol("history");
	}
	return L;
}

stringtlist& activity_methods() {
	static stringtlist	A;
	static bool		initialized = false;
	if(!initialized) {
		initialized = false;
		A << new emptySymbol("exists");
	}
	return A;
}

//
// Note on Resource Methods
// ========================
//
// This is our opportunity to create an evaluator
// that is appropriate for the situation at hand.
// The evaluators are defined in ParsedExpressionSystem.H
// and implemented in resource_expressions.C. As of
// November 16, 2021 they are all derived from the
// base class ResourceMethod. The derived classes are
//
// 	ResourceCurrentVal
// 	ResourceVal
// 	ResourceInterpVal
// 	ResourceHist
// 	ArrayedResourceCurrentVal
// 	ArrayedResourceVal
// 	ArrayedResourceInterpVal
// 	ArrayedResourceHist
//
// The duplication between scalar and arrayed resources
// is not elegant, but I don't have time to change that.
//
bool look_for_resource_method(
		FunctionCall* func_call,
		smart_ptr<ResourceMethod>& resource_method,
		smart_ptr<InstanceMethod>& instance_method,
		int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << make_space(dent) << "look for resource method...\n";
	}

	//
	// This function was called from consolidate_function_call_exp
	// after checking that func_call->qualified_symbol was defined,
	// so here we can safely assume its existence
	//
	QualifiedSymbol* qs = dynamic_cast<QualifiedSymbol*>(
		func_call->qualified_symbol.object());

	Cstring symbol_name = qs->getData();

	Qualifications* q = dynamic_cast<Qualifications*>(
		qs->qualifications.object());

	smart_ptr<MultidimIndices> multiptr;

	//
	// The resource may be arrayed
	//
	for(int k = 0; k < q->IndicesOrMembers.size(); k++) {
	    MultidimIndices* multi
		= dynamic_cast<MultidimIndices*>(
				q->IndicesOrMembers[k].object());
	    ClassMember* cm = dynamic_cast<ClassMember*>(
				q->IndicesOrMembers[k].object());
	    if(multi) {
		assert(k == 0);
		multiptr.reference(multi);
		if(APcloptions::theCmdLineOptions().debug_grammar) {
		    cerr << make_space(dent)
			<< "calling MultidimIndices->consolidate...\n";
		}
		consolidate_multidim_indices(multiptr, dent + 2);
		if(APcloptions::theCmdLineOptions().debug_grammar) {
		    cerr << make_space(dent)
			<< "returning from MultidimIndices->consolidate...\n";
		}
	    } else if(cm) {

		//
		// We don't consolidate cm because all we want is the
		// member name given by cm->getData()
		//

		//
		// Investigate cases where the object on which
		// a method is called is a resource
		//
		RCsource* container = NULL;

		if(resource_methods().find(cm->getData())) {

		    //
		    // First, check that the resource exists
		    //
		    if(!(container = RCsource::resource_containers().find(symbol_name))) {
			Cstring err;
			err << "File " << func_call->file << ", line "
			    << func_call->line << ": Cannot handle method call "
			    << func_call->to_string() << " because resource "
			    << symbol_name << " cannot be found.";
			throw(eval_error(err));
		    }

		    //
		    // Second, check that it has an index if arrayed
		    //
		    if( !multiptr
			&& container->payload->Object->array_elements.size() != 1) {
		        Cstring err;
		        err << "File " << func_call->file << ", line "
			    << func_call->line << ": resource "
		    	    << symbol_name << " is an array; an index is "
		    	    << "required before invoking " << cm->getData();
		        throw(eval_error(err));
		    }
		}

		//
		// Resource methods
		//
		if(cm->getData() == "currentval") {

		    //
		    // Check that there are no arguments
		    //
		    if(multiptr) {
			smart_ptr<MultidimIndices> pe_multi(multiptr.object());
			assert(k == 1);
			ArrayedResourceCurrentVal* rv = new
				    ArrayedResourceCurrentVal(
					origin_info(func_call->line,
					    func_call->file),
					symbol_name,
		    			pe_multi);
			resource_method.reference(rv);
			return true;
		    } else {

			assert(k == 0);
			ResourceCurrentVal* rv = new
				    ResourceCurrentVal(
			    		origin_info(func_call->line,
					    func_call->file),
					symbol_name);
			resource_method.reference(rv);
			return true;
		    }
		} else if(cm->getData() == "value") {

		    //
		    // Check that there is exactly one argument
		    //
		    vector<parsedExp> actual_arguments;
		    ExpressionList* el = dynamic_cast<ExpressionList*>(
				func_call->zero_or_more_args.object());
		    if(!el) {
			Cstring err;
			err << "File " << func_call->file << ", line "
			    << func_call->line << ": method value() "
			    << "requires a time argument.";
			throw(eval_error(err));
		    }
		    el->consolidate(actual_arguments, dent + 2);
		    if(actual_arguments.size() != 1) {
			Cstring errs;
			errs << "File " << func_call->file << ", line "
			    << func_call->line << ": value() takes one "
			    << "argument; " << actual_arguments.size()
			    << " provided";
			throw(eval_error(errs));
		    }

		    apgen::DATA_TYPE ret_type
			= actual_arguments[0]->get_result_type();
		    if(   ret_type != apgen::DATA_TYPE::UNINITIALIZED
		       && ret_type != apgen::DATA_TYPE::TIME) {
			Cstring errs;
			errs << "File " << func_call->file << ", line "
			    << func_call->line << ": argument of value() "
			    << "method should be a time, but a(n)\n"
			    << apgen::spell(ret_type) << " is provided.";
			throw(eval_error(errs));
		    }
		    if(multiptr) {
			smart_ptr<MultidimIndices> pe_multi(multiptr.object());
			assert(k == 1);
			ArrayedResourceVal* rv = new ArrayedResourceVal(
				origin_info(func_call->line,
				    func_call->file),
				symbol_name,
				pe_multi,
				actual_arguments[0]);
			resource_method.reference(rv);
			return true;
		    } else {

			assert(k == 0);
			ResourceVal* rv = new ResourceVal(
				origin_info(func_call->line,
				    func_call->file),
				symbol_name,
				actual_arguments[0]);
			resource_method.reference(rv);
			return true;
		    }
		} else if(cm->getData() == "interpval") {

		    //
		    // Check that there are no arguments
		    //
		    vector<parsedExp> actual_arguments;
		    if(actual_arguments.size()) {
			Cstring errs;
			errs << "File " << func_call->file << ", line "
			    << func_call->line
			    << ": interpval() takes no arguments";
			throw(eval_error(errs));
		    }

		    if(multiptr) {
			smart_ptr<MultidimIndices> pe_multi(multiptr.object());
			assert(k == 1);
			ArrayedResourceInterpVal* rv = new
			    ArrayedResourceInterpVal(
				origin_info(func_call->line,
					func_call->file),
				symbol_name,
				pe_multi);
			resource_method.reference(rv);
			return true;
		    } else {
			assert(k == 0);
			ResourceInterpVal* rv = new ResourceInterpVal(
				origin_info(func_call->line,
				    func_call->file),
				symbol_name);
			resource_method.reference(rv);
			return true;
		    }
		} else if(cm->getData() == "history") {

		    //
		    // Check that there are exactly two arguments:
		    // start and end
		    //
		    vector<parsedExp> actual_arguments;
		    ExpressionList* el = dynamic_cast<ExpressionList*>(
				func_call->zero_or_more_args.object());
		    if(!el) {
			Cstring err;
			err << "File " << func_call->file << ", line "
			    << func_call->line << ": method history() "
			    << "requires two time arguments.";
			throw(eval_error(err));
		    }
		    el->consolidate(actual_arguments, dent + 2);
		    if(actual_arguments.size() == 2) {
			; // OK: history(start, end)
		    } else if(actual_arguments.size() == 4) {
			; // OK: history(start, end, <value symbol>, <expression>)
		    } else {
			Cstring errs;
			errs << "File " << func_call->file << ", line "
			    << func_call->line << ": history() takes two "
			    << "arguments; " << actual_arguments.size()
			    << " provided";
			throw(eval_error(errs));
		    }

		    //
		    // Check that the first two argument types are times
		    //
		    for(int zz = 0; zz < 2; zz++) {
			apgen::DATA_TYPE ret_type
			    = actual_arguments[zz]->get_result_type();
			if(   ret_type != apgen::DATA_TYPE::UNINITIALIZED
			      && ret_type != apgen::DATA_TYPE::TIME) {
			    Cstring err;
			    err << "File " << func_call->file << ", line "
				<< func_call->line << ": argument " << (zz + 1)
				<< " of history() "
				<< "method should be a time, but a(n)\n"
				<< apgen::spell(ret_type) << " is provided.";
			    throw(eval_error(err));
			}
		    }

		    //
		    // If 4 arguments are present, check that the 3rd arg
		    // is a symbol and the 4th a Boolean expression
		    //
		    if(actual_arguments.size() == 4) {

			//
			// Arguments have been consolidated when invoking
			// ExpressionList->consolidate() above. The Boolean
			// expression we are interested in is actual_arguments[3],
			// the symbol which should take on the resource value is
			// actual_arguments[2].
			//
			// Here we just verify the types of those two arguments.
			//
			Symbol* sym = dynamic_cast<Symbol*>(actual_arguments[2].object());
			if(!sym) {
			    Cstring err;
			    err << "File " << func_call->file << ", line "
				<< func_call->line << ": argument 3 "
				<< " of history() "
				<< "method should be a symbol, but a(n)\n"
				<< actual_arguments[2]->spell() << " is provided.";
			    throw(eval_error(err));
			}
    			if(actual_arguments[3]->get_result_type() == apgen::DATA_TYPE::BOOL_TYPE) {
			    Cstring err;
			    err << "File " << func_call->file << ", line "
				<< func_call->line << ": argument 4 "
				<< " of history() "
				<< "method should be a Boolean expression, but a(n)\n"
				<< apgen::spell(actual_arguments[3]->get_result_type()) << " is provided.";
			    throw(eval_error(err));
			}
		    }

		    //
		    // Check that the resource has (an) index/ices
		    // if necessary
		    //
		    if(multiptr) {
			smart_ptr<MultidimIndices> pe_multi(multiptr.object());
			assert(k == 1);
			ArrayedResourceHist* rv = new ArrayedResourceHist(
				origin_info(func_call->line,
				    func_call->file),
				symbol_name,
				pe_multi,
				actual_arguments[0],
				actual_arguments[1]);
			resource_method.reference(rv);
			return true;
		    } else {

			assert(k == 0);
			ResourceHist* rv = new ResourceHist(
				origin_info(func_call->line,
				    func_call->file),
				symbol_name,
				actual_arguments[0],
				actual_arguments[1]);
			resource_method.reference(rv);
			return true;
		    }
		}

		//
		// Activity method (there is only one as of now)
		//
		else if(cm->getData() == "exists") {

		    //
		    // Check that there are no arguments
		    //
		    vector<parsedExp> actual_arguments;
		    if(actual_arguments.size()) {
			Cstring errs;
			errs << "File " << func_call->file << ", line " << func_call->line
			    << ": exists() takes no arguments";
			throw(eval_error(errs));
		    }

		    //
		    // For now, check that this is a simple statement - no indices.
		    // We will remove this limitation later.
		    //
		    if(multiptr) {
			Cstring errs;
			errs << "File " << func_call->file << ", line " << func_call->line
				<< ": for now, exists() can only be applied to a simple instance, "
				<< "not to a complex expression";
			throw(eval_error(errs));
		    }
		    if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent) << "calling Symbol->consolidate...\n";
		    }
		    consolidate_symbol(qs->symbol, dent + 2);
		    if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << make_space(dent) << "returning from Symbol->consolidate...\n";
		    }
		    InstanceExists* ie = new InstanceExists(
				origin_info(func_call->line,
				    func_call->file),
				qs->symbol);
		    instance_method.reference(ie);
		    return true;

		} else {
		    return false;
		}
	    }
	}
	return false;
}

FunctionCall* FunctionCallOverrider(FunctionCall* func_call, int dent) {
    FunctionCall* overloaded_call = NULL;

    func_call->consolidate(dent + 2);

    if(func_call->get_result_type() == apgen::DATA_TYPE::INTEGER) {
    	overloaded_call = new IntFuncCall(true, *func_call);
    } else if(func_call->get_result_type() == apgen::DATA_TYPE::BOOL_TYPE) {
    	overloaded_call = new BoolFuncCall(true, *func_call);
    } else if(func_call->get_result_type() == apgen::DATA_TYPE::FLOATING) {
    	overloaded_call = new DoubleFuncCall(true, *func_call);
    } else if(func_call->get_result_type() == apgen::DATA_TYPE::TIME) {
    	overloaded_call = new TimeFuncCall(true, *func_call);
    } else if(func_call->get_result_type() == apgen::DATA_TYPE::DURATION) {
    	overloaded_call = new DurationFuncCall(true, *func_call);
    } else if(func_call->get_result_type() == apgen::DATA_TYPE::STRING) {
    	overloaded_call = new StringFuncCall(true, *func_call);
    } else if(func_call->get_result_type() == apgen::DATA_TYPE::ARRAY) {
    	if(func_call->expressions.size()) {

	    //
	    // there are qualifications
	    //
	    overloaded_call = new QualifiedArrayFuncCall(true, *func_call);
	} else {
	    overloaded_call = new ArrayFuncCall(true, *func_call);
	}
    } else if(func_call->get_result_type() == apgen::DATA_TYPE::INSTANCE) {
	if(func_call->expressions.size()) {

	    //
	    // there are qualifications
	    //
	    overloaded_call = new QualifiedInstanceFuncCall(true, *func_call);
	} else {
	    overloaded_call = new InstanceFuncCall(true, *func_call);
	}

	//
	// Else, the return type is undefined
	//
    }
    return overloaded_call;
}

//
// function call consolidation takes place in two contexts:
//
// 	1. a standalone function invocation, e. g.
//
// 		write_to_stdout("Hello\n");
//
// 	2. part of an expression, e. g.
//
// 		global integer y_plus_1 = compute_y() + 1;
//
// 	   or
//
// 	  	s: string default to initialize_s();
//
// consolidate_function_call_exe() deals with case 1, while
// consolidate_function_call_exp() deals with case 2.
//
void consolidate_function_call_exe(
		smart_ptr<executableExp>& E,
		int dent) {
    FunctionCall* func_call = dynamic_cast<FunctionCall*>(E.object());
    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "consolidate function call exe ("
	    << E->spell() << ")\n";
	cerr << make_space(dent) << E->to_string() << "\n";
    }

    //
    // Since the value returned by the function is thrown
    // away, we don't want to deal with the useless
    // complexity of dealing with a member of an instance
    // or an array element:
    //
    if(func_call->qualified_symbol) {
	Cstring errs;
	errs << "File " << func_call->file << ", line " << func_call->line
	    << ": return from function call must be assigned to a variable.";
	throw(eval_error(errs));
    }
    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "calling FunctionCall->consolidate()\n";
    }

    FunctionCall* overloaded_call = FunctionCallOverrider(func_call, dent);

    if(overloaded_call) {
	E.reference(overloaded_call);
    }

    if(APcloptions::theCmdLineOptions().debug_grammar) {
    	cerr << make_space(dent) << "returned from FunctionCall->consolidate()\n";
    }

    //
    // Else, the return type is undefined
    //
}

//
// function call consolidation takes place in two contexts:
//
// 	1. a standalone function invocation, e. g.
//
// 		write_to_stdout("Hello\n");
//
// 	2. part of an expression, e. g.
//
// 		global integer y_plus_1 = compute_y() + 1;
//
// 	   or
//
// 	  	s: string default to initialize_s();
//
// consolidate_function_call_exe() deals with case 1, while
// consolidate_function_call_exp() deals with case 2.
//
void consolidate_function_call_exp(
		parsedExp& E,
		int dent) {
    FunctionCall* func_call = dynamic_cast<FunctionCall*>(E.object());
    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "consolidate function call exp ("
	    << E->spell() << ")\n";
	cerr << make_space(dent) << E->to_string() << "\n";
    }

    if(func_call->qualified_symbol) {

	//
	// The function is called as a method of an object, e. g.
	// <resource>.currentval()
	//
	smart_ptr<ResourceMethod> res_method;
	smart_ptr<InstanceMethod> inst_method;

	//
	// This call will create a resource method or an instance
	// method, depending on the kind of object that the method is
	// applied to:
	//
	if(look_for_resource_method(
			func_call,
			res_method,
			inst_method,
			dent + 2)) {

		//
		// grab the consolidated object and reference it instead of
		// the original object in the smart pointer that was passed to
		// us
		//
		if(res_method) {
			E.dereference();
			E.reference(res_method.object());
		} else if(inst_method) {
			E.dereference();
			E.reference(inst_method.object());
		}
	} else {
		Cstring errs;
		errs << "File " << func_call->file << ", line "
		    << func_call->line
		    << ": cannot figure out method call";
		throw(eval_error(errs));
	}
	return;
    } else if(func_call->getData() == "currentval") {

	//
	// The function is called as a standalone method, e. g.
	// currentval(). This is OK inside a resource definition.
	//
	if(LevelOfCurrentTask != 2) {
		Cstring errs;
		errs << "File " << func_call->file << ", line " << func_call->line
			<< ": currentval() can only be used in a resource method";
		throw(eval_error(errs));
	}

	//
	// get the ambient resource
	//
	task* currentTask = get_current_task();
	Behavior* currentBehavior = &currentTask->Type;
	Cstring resourceName = currentBehavior->name;

	Rsource* resource = eval_intfc::FindResource(resourceName);

	assert(resource);

	//
	// Build the consolidated object and reference it instead of
	// the original object in the smart pointer that was passed to
	// us
	//
	SimpleCurrentVal* sc = new SimpleCurrentVal(
					origin_info(
						func_call->line,
						func_call->file),
					resource);
	E.dereference();
	E.reference(sc);
	return;
    }

    if(APcloptions::theCmdLineOptions().debug_grammar) {
    	cerr << make_space(dent) << "calling FunctionCall->consolidate()\n";
    }

    FunctionCall* overloaded_call = FunctionCallOverrider(func_call, dent);

    if(overloaded_call) {
	E.reference(overloaded_call);
    }

    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << make_space(dent) << "returned from FunctionCall->consolidate()\n";
    }

    //
    // Else, the return type is undefined
    //
}

} // interface aafReader
