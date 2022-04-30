#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/stat.h>
#include "aafReader.H"
#include "ActivityInstance.H"	// for dumb_actptr
// #include "ACT_exec.H"
#include "APmodel.H"
#include "apcoreWaiter.H"
#include "BehavingElement.H"
#include "fileReader.H"
#include "lex_intfc.H"
// #include "RES_def.H"

using namespace pEsys;

void ActInstance::consolidate(int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent)
		    << "consolidating ActInstance " << getData() << "\n";
	}

	//
	// PART 1: get all the basics, get the type and push its constructor
	// onto the task stack
	//

	fileParser::currentParser().theAncestors.clear();
	fileParser::currentParser().theChildren.clear();
	Cstring instance_name(getData());
	Cstring unique_id;
	if(act_instance_header->expressions[5]) {
		unique_id = act_instance_header->expressions[5]->getData();
	}
	Cstring act_type = act_instance_header->expressions[4]->getData();
	map<Cstring, int>::const_iterator iter
				= Behavior::ClassIndices().find(act_type);
	if(iter == Behavior::ClassIndices().end()) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": Activity instance " << instance_name
		    << " has type " << act_type
		    << " which has not been defined.";
		throw(eval_error(err));
	}
	Behavior& type_beh = *Behavior::ClassTypes()[iter->second];
	task* constr_task = type_beh.tasks[0];
	aafReader::push_current_task(constr_task);

	// debug
	// cerr << "consolidating act instance " << unique_id << "\n";

	//
	// At this point, the current task points to the type's constructor
	//

	//
	// PART 2: handle hierarchy section and set up theAncestors,
	// theChildren in fileParser::currentParser()
	//

	apgen::act_visibility_state instance_visibility = apgen::act_visibility_state::VISIBILITY_REGULAR;
	if(decomposition_info) {
		Hierarchy* hier = dynamic_cast<Hierarchy*>(decomposition_info.object());
		assert(hier);
		Cstring			keyword = hier->tok_hierarchy_keyword->getData();
		apgen::METHOD_TYPE	DecompType = apgen::METHOD_TYPE::NONE; // unused except for debug
		int			Invisibility = 0; 		 // unused except for debug
		int			CurrentHierarchyStatus = 0;

		while(true) {
			if(keyword == "abstractable") {
				CurrentHierarchyStatus = 4;
			} else if(keyword == "abstracted") {
				CurrentHierarchyStatus = 1;
			} else if(keyword == "decomposable") {
				DecompType = apgen::METHOD_TYPE::DECOMPOSITION;
				CurrentHierarchyStatus = 8;
			} else if(keyword == "decomposed") {
				DecompType = apgen::METHOD_TYPE::DECOMPOSITION;
				CurrentHierarchyStatus = 2;
			}
			Invisibility |= CurrentHierarchyStatus;
			if(Invisibility & 1) {
				instance_visibility = apgen::act_visibility_state::VISIBILITY_ABSTRACTED;
			} else if((Invisibility & 2) || (Invisibility & 32)) {
				instance_visibility = apgen::act_visibility_state::VISIBILITY_DECOMPOSED;
			}
			vector<Cstring> ids;
			IdList* idl = dynamic_cast<IdList*>(hier->id_list.object());
			assert(idl);
			ids.push_back(idl->getData());
			for(int i = 0; i < idl->expressions.size(); i++) {
				ids.push_back(idl->expressions[i]->getData());
			}
			bool parent_info = CurrentHierarchyStatus == 1 || CurrentHierarchyStatus == 4;
			for(int i = 0; i < ids.size(); i++) {
				if(parent_info) {
					if(!fileParser::currentParser().theAncestors.find(ids[i])) {
						// debug
						// cerr << "              adding ancestor " << ids[i] << "\n";
						fileParser::currentParser().theAncestors
							<< new emptySymbol(ids[i]);
					}
				} else {
					if(!fileParser::currentParser().theChildren.find(ids[i])) {
						// debug
						// cerr << "              adding child " << ids[i] << "\n";
						fileParser::currentParser().theChildren
							<< new emptySymbol(ids[i]);
					}
				}
			}
			if(hier->expressions.size()) {
				Hierarchy* hier2 = dynamic_cast<Hierarchy*>(hier->expressions[0].object());
				assert(hier2);
				keyword = hier2->tok_hierarchy_keyword->getData();
				hier = hier2;
			} else {
				break;
			}
		}
	}

	//
	// PART 3: handle parameters
	//

	//
	// first, define an object so we can evaluate parameters
	//

	act_object* theActObject = new act_object(
		/* ActivityInstance* = */ NULL,
		instance_name,
		unique_id,
		type_beh,
		*this
		);

	behaving_element act_beh(theActObject);

	//
	// second, define the Plan attribute of the instance -- POSTPONED;
	// we need to override all the defaults.
	//

	// (*theActObject)[ActivityInstance::PLAN] = aafReader::current_file();

	// debug
	// aoString aos;

	// Symbol::debug_symbol = true;
	// cerr << "consolidating instance: type dump\n";
	// type_beh.to_stream(&aos, 0);
	// aos << "consolidating instance: instance dump\n";
	// theActObject->to_stream(&aos, 0);
	// cerr << aos.str();
	// Symbol::debug_symbol = false;

	//
	// third, we consolidate parameters
	//

	ExpressionList* el = dynamic_cast<ExpressionList*>(param_section_inst.object());
	vector<parsedExp> actual_values;
	if(el) {
		el->consolidate(actual_values, dent + 2);
	}

	//
	// fourth, we check that the number of parameters agrees with the type
	//

	int	type_param_number = 0;
	if(type_beh.tasks[0]->parameters) {
		type_param_number = type_beh.tasks[0]->parameters->statements.size();
	}
	if(actual_values.size() != type_param_number) {
		Cstring err;
		err << "File " << file << ", line " << line << ": instance has "
			<< actual_values.size() << " parameter(s) while act. type "
			<< type_beh.name << " has " << type_param_number;
		throw(eval_error(err));
	}

	//
	// fifth, we evaluate the parameters
	//

	for(int i = 0; i < actual_values.size(); i++) {
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent)
				<< "Parser: calling Execute() with EC based on task "
				<< act_beh->Task.full_name() << "\n";
		}
		try {
			actual_values[i]->eval_expression(
					theActObject,
					(*theActObject)[constr_task->paramindex[i]]);
		} catch(eval_error Err) {
			Cstring err;
			err << "File " << file << ", line " << line << ": error while "
				<< "evaluating instance parameter # " << (i + 1) << ": "
				<< actual_values[i]->to_string() << "; details:\n" << Err.msg;
			throw(eval_error(err));
		}
	}

	//
	// PART 4: handle activity type attributes
	//

	bool duration_formula_was_enforced = false;
	if(constr_task->prog) {
		Program* constructor_program = constr_task->prog.object();
		execution_context* instanceContext = new execution_context(
					act_beh,
					constructor_program);
		execution_context::return_code		Code = execution_context::REGULAR;


		for(int i = 0; i < constructor_program->statements.size(); i++) {
			executableExp* ee = constructor_program->statements[i].object();

			// debug

			// aos.clear();
			// cerr << "executing type statement - ";
			// ee->to_stream(&aos, 0);
			// cerr << aos.str();

			Assignment* assign = dynamic_cast<Assignment*>(ee);
			if(assign) {
				Symbol* lhs = dynamic_cast<Symbol*>(assign->lhs_of_assign.object());
				QualifiedSymbol* qual_lhs = dynamic_cast<QualifiedSymbol*>(assign->lhs_of_assign.object());
				assert(lhs || qual_lhs);
				if(lhs && lhs->getData() == "span") {
					Constant* c = dynamic_cast<Constant*>(assign->Expression.object());
					if(!c) {
						duration_formula_was_enforced = true;
					}
				}
				try {
					assign->execute(instanceContext, Code);
				} catch(eval_error Err) {
					Cstring err;
					err << "File " << assign->file << ", line " << assign->line
						<< ": execution error -\n" << Err.msg;
					throw(eval_error(err));
				}
			} else {
				Declaration* decl = dynamic_cast<Declaration*>(ee);
				assert(decl);
				try {
					decl->execute(instanceContext, Code);
				} catch(eval_error Err) {
					Cstring err;
					err << "File " << decl->file << ", line " << decl->line
						<< ": execution error -\n" << Err.msg;
					throw(eval_error(err));
				}
			}
		}

		if(APcloptions::theCmdLineOptions().debug_grammar) {
			if(duration_formula_was_enforced) {
				cerr << aafReader::make_space(dent)
					<< "instance type duration is a formula which was enforced\n";
			} else {
				cerr << aafReader::make_space(dent)
					<< "instance type duration is a constant which may be "
					"overridden by the instance\n";
			}
			cerr << aafReader::make_space(dent) << "Instance after executing type attr. prog.:\n";
			aoString aos;
			act_beh->to_stream(&aos, 2);
			cerr << aos.str();
		}
		delete instanceContext;
	}

	// debug

	// aos.clear();
	// Symbol::debug_symbol = true;
	// aos << "instance after applying type attributes: instance dump\n";
	// theActObject->to_stream(&aos, 0);
	// cerr << aos.str();
	// Symbol::debug_symbol = false;

	//
	// PART 5: handle instance attributes, which override the type's
	//

	Program*	pp = dynamic_cast<Program*>(
					attributes_section_inst.object());
	if(pp) {

		//
		// We have to get statements from dp->statements
		// into new_beh.tasks[0]->prog - which needs to be created,
		// unless that was already done when handling class variables
		//

		execution_context* instanceContext = new execution_context(
					act_beh,
					pp);
		execution_context::return_code		Code = execution_context::REGULAR;
		execution_context::context_state	contextState = execution_context::TRUE_CONDITION_FOUND;
		for(int i = 0; i < pp->statements.size(); i++) {
			executableExp* ee = dynamic_cast<executableExp*>(pp->statements[i].object());
			assert(ee);
			apgen::DATA_TYPE dt = apgen::DATA_TYPE::UNINITIALIZED;
			aafReader::ConditionalFlag	don_t_care = aafReader::NotAConditional;
			bool				don_t_care_either;
			aafReader::consolidate_statement(
					pp->statements[i],
					dt,
					pp->orig_section,
					don_t_care,
					don_t_care_either,
					dent);

			//
			// object may have been modified by consolidate_statement:
			//
			ee = dynamic_cast<executableExp*>(pp->statements[i].object());
			Assignment* assign = dynamic_cast<Assignment*>(ee);
			assert(assign);
			Symbol* lhs = dynamic_cast<Symbol*>(assign->lhs_of_assign.object());
			QualifiedSymbol* qual_lhs = dynamic_cast<QualifiedSymbol*>(assign->lhs_of_assign.object());
			assert(lhs || qual_lhs);
			if(	lhs && lhs->getData() == "span"
				&& duration_formula_was_enforced == true) {
				// we don't override the type's formula
			} else {
				try {
					assign->execute(instanceContext, Code);
				} catch(eval_error Err) {
					Cstring err;
					err << "File " << assign->file << ", line " << assign->line
						<< ": execution error -\n" << Err.msg;
					throw(eval_error(err));
				}
			}
		}
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent) << "Instance after executing instance attr. prog.:\n";
			aoString aos;
			act_beh->to_stream(&aos, 2);
			cerr << aos.str();
		}
		delete instanceContext;
	}

	//
	// Part 5b: (re-)define the Plan attribute of the instance; we do it
	// now because we need to override the value listed in the APF
	//

	(*theActObject)[ActivityInstance::PLAN] = aafReader::current_file();

	// debug

	// aos.clear();
	// Symbol::debug_symbol = true;
	// aos << "instance after applying instance attributes: instance dump\n";
	// theActObject->to_stream(&aos, 0);
	// cerr << aos.str();
	// Symbol::debug_symbol = false;

	//
	// PART 6: create the activity instance (NOTE: do this AFTER start/end
	// times have been defined, otherwise all hell breaks loose with
	// dependent pointers
	//

	ActivityInstance* activityInstance = NULL;
	try {
		activityInstance = new ActivityInstance(theActObject);
	} catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ": error while "
			<< "creating instance:\n" << Err.msg;
		throw(eval_error(err));
	}

	//
	// PART 7: create instance tag
	//

	instance_tag* activityInstanceTag
				= new instance_tag(
					(*theActObject)[ActivityInstance::ID].get_string(),
					instance_tag_PLD(	activityInstance,
								instance_visibility));
	fileParser::currentParser().Instances << activityInstanceTag;
	if(!unique_id.is_defined()) {
		// get the unique id computed by the constructor
		unique_id = (*theActObject)[ActivityInstance::ID].get_string();
	}
	if((activityInstanceTag =
		fileParser::currentParser().ListOfActivityPointersIndexedByIDsInThisFile.find(
				unique_id))) {
		activityInstanceTag->payload.act = activityInstance;
	} else {
		fileParser::currentParser().ListOfActivityPointersIndexedByIDsInThisFile
			<< (activityInstanceTag = new instance_tag(
							unique_id,
							instance_tag_PLD(
								activityInstance,
								instance_visibility)));
	}

	// debug
	// cerr << "creating act instance tag\n";

	if(activityInstanceTag && activityInstance) {
		emptySymbol* one_act;

		aafReader::ActivityInstanceCount++;

		while((one_act = fileParser::currentParser().theAncestors.first_node())) {

			// debug
			// cerr << "    adding parent " << one_act->get_key() << " to instance "
			// 	<< activityInstance->get_unique_id() << "\n";

			activityInstanceTag->payload.parents << one_act;
		}
		while((one_act = fileParser::currentParser().theChildren.first_node())) {

			// debug
			// cerr << "    adding child " << one_act->get_key() << " to instance "
			// 	<< activityInstance->get_unique_id() << "\n";

			activityInstanceTag->payload.toddlers << one_act;
		}
	} else {
		fileParser::currentParser().theAncestors.clear();
		fileParser::currentParser().theChildren.clear();
		Cstring err;
		err << "File " << file << ", line " << line << ": error while "
			<< "creating instance - missing objects";
		throw(eval_error(err));
	}

	//
	// PART 8: pop the type constructor off the stack
	//

	aafReader::pop_current_task();

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "Final version of act instance:\n";
		aoString aos;
		theActObject->to_stream(&aos, dent + 2);
		cerr << aos.str();
	}
}
