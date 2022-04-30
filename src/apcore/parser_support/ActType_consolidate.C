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

	//
	// This method is used for programs that are executed from a
	// high-level request - e. g. REMODEL or SCHEDULEACTIVITY -
	// before events on the queue are processed. Such programs
	// need to be put on hold until "their time comes up," i. e.,
	// modeling time hits the start time of the activity instance
	// containing the program. The easiest way to do this is to
	// add a wait statement at the beginning of the program.
	// This wait statement should be removed when serializing the
	// program.
	//

void ActType::add_a_wait_statement(
		parsedExp header,
		Program* theProgram) {

	//
	// we emulate grammar.y in creating the new WaitFor
	// object
	//

	parsedExp	constant(new pE_w_string(origin_info(
					header->line,
					header->file),
					"0:0:0"));
	//
	// Watch out! the Differentiator determines what type
	// of constant it is
	//

	parsedExp	DurationExp(new Constant(0, constant, Differentiator_4(0)));

	//
	// Watch out: the Constant constructor assumes that the
	// line number comes from the dynamic line number inside the parser.
	//
	DurationExp->line = header->line;

	parsedExp	wait_token(new pE_w_string(origin_info(
					header->line,
					header->file),
					"wait"));
	parsedExp	for_token(new pE_w_string(origin_info(
					header->line,
					header->file),
					"for"));
	parsedExp	semicolon(new pE_w_string(origin_info(
					header->line,
					header->file),
					";"));
	WaitFor* wf = new WaitFor(2, DurationExp, wait_token, for_token, semicolon,
			Differentiator_0(0));
	vector<smart_ptr<executableExp> > statements_copy;
	statements_copy.push_back(smart_ptr<executableExp>(wf));
	for(int i = 0; i < theProgram->statements.size(); i++) {
		statements_copy.push_back(theProgram->statements[i]);
	}
	theProgram->statements = statements_copy;
}

//
// This method is only called in the Implementation Pass
//
void ActType::handle_modeling_program(
		ModelingSection* modeling,
		Behavior& act_type_beh, int dent) {
	Cstring task_name = modeling->resource_usage_header->getData();
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "modeling section: "
			<< modeling->resource_usage_header->getData()
			<< "\n";
	}
	apgen::METHOD_TYPE mt = apgen::METHOD_TYPE::NONE;
	Program* ModelingProgram = dynamic_cast<Program*>(modeling->program.object());
	assert(ModelingProgram);

	if(task_name == "modeling") {
		mt = apgen::METHOD_TYPE::MODELING;

		//
		// We need to add a wait for 0:0:0 statement at the
		// top of the program
		//

		add_a_wait_statement(modeling->resource_usage_header, ModelingProgram);
	} else if(task_name == "resource") {
		task_name = "resource usage";
		mt = apgen::METHOD_TYPE::RESUSAGE;
	} else {
		cerr << "??????? strange modeling name " << task_name << "\n";
	}
	ModelingProgram->orig_section = mt;
	int k = act_type_beh.add_task(task_name);
	task* curtask = act_type_beh.tasks[k];
	curtask->prog.reference(ModelingProgram);
	aafReader::push_current_task(act_type_beh.tasks[k]);

	//
	// While consolidating this program, we need to watch
	// out for statements that assign a value to
	// ActivityInstance::SPAN
	//
	aafReader::consolidate_program(*ModelingProgram, dent);

	aafReader::pop_current_task();
}

void ActType::consolidate(int dent) {

	//
	// PART 1: grab activity type name and define a new Class
	//

	ActTypeInitial* initial_section
			= dynamic_cast<ActTypeInitial*>(
					initial_act_type_section.object());
	assert(initial_section);
	Cstring act_type_name = initial_section->activity_type_header->getData();
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent)
			<< "consolidating ActType " << act_type_name << "\n";
	}

	map<Cstring, int>::const_iterator act_iter
			= Behavior::ClassIndices().find(act_type_name);

	if(aafReader::CurrentPass() == aafReader::DeclarationPass) {
		if(act_iter != Behavior::ClassIndices().end()) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": activity type "
				<< act_type_name << " already exists";
			throw(eval_error(err));
		}
		int n = Behavior::add_type(new Behavior(
					act_type_name,
					/* realm = */ "activity",
					NULL));
		Behavior& act_type_beh = *Behavior::ClassTypes()[n];
		task* Constructor = act_type_beh.tasks[0];

		//
		// Before defining parameters specific to this activity type,
		// we define all the attributes an instance of this type
		// might conceivably have. Note that the generic activity
		// Type includes all custom attributes (see
		// aafReader::add_a_custom_type()).
		//

		map<Cstring, int>::const_iterator	iter
				= Behavior::ClassIndices().find("generic");
		Behavior&			generic_behavior
				= *Behavior::ClassTypes()[iter->second];
		Constructor->set_varinfo_to(generic_behavior.tasks[0]->get_varinfo());
		Constructor->set_varindex_to(generic_behavior.tasks[0]->get_varindex());

		//
		// Push the constructor
		//

		aafReader::push_current_task(Constructor);

		//
		// At this point the Type's task has all the default variables,
		// but it has neither program nor parameters yet.

		//
		// PART 2: grab parameters if any.
		//

		//
		// If there are any, update the act type constructor's task
		// variables and paramindex array, and store the declarations
		// in its parameters program (create the program if necessary)
		//

		if(initial_section->opt_act_type_param_section) {
			Parameters* pa = dynamic_cast<Parameters*>(
					initial_section->opt_act_type_param_section.object());
			assert(pa);
			pa->consolidate(dent + 2);
		}

		//
		// At this point the Constructor has no program but it has
		// parameters if any were declared.
		//

		//
		// Pop the constructor we pushed a moment ago
		//

		aafReader::pop_current_task();

		//
		// If in the Declaration Pass, return here:
		//
		return;
	}

	//
	// We are in the Implementation Pass
	//

	Behavior& act_type_beh = *Behavior::ClassTypes()[act_iter->second];
	task* Constructor = act_type_beh.tasks[0];

	//
	// Push the constructor
	//

	aafReader::push_current_task(Constructor);

	//
	// What's left below is only exercised in the implementation pass.
	// Remember that at the end of the first pass, the Constructor may
	// have parameters but it has no program.
	//

	//
	// PART 3: grab class variables if any.
	//
	// If there are any, create a program for the Constructor
	// and store them in it.
	//

	if(initial_section->opt_type_class_variables) {
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent) << "class variables section: "
				<< initial_section->opt_type_class_variables->getData()
				<< "\n";
		}

		//
		// handle class variables here; in particular, define a program
		//
		ActTypeClassVariables* atc = dynamic_cast<ActTypeClassVariables*>(
				initial_section->opt_type_class_variables.object());
		assert(atc);

		atc->consolidate(dent + 2);
	}

	//
	// PART 4: grab attributes, if any.
	//
	// If there are any, store them in the Constructor's program
	// (which needs to be created if necessary)
	//

	Program* constructor_program = NULL;
	if(Constructor->prog) {

		//
		// constructor_program was added by class variables consolidation:
		//

		constructor_program = Constructor->prog.object();
	}
	if(initial_section->opt_act_attributes_section) {
		ActTypeAttributes* attributes_section = dynamic_cast<ActTypeAttributes*>(
				initial_section->opt_act_attributes_section.object());
		assert(attributes_section);
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent + 2) << "attributes section: "
				<< initial_section->opt_act_attributes_section->getData()
				<< "\n";
		}
		Program* attributes_program = dynamic_cast<Program*>(
				attributes_section->declarative_program.object());
		assert(attributes_program);

		//
		// We have to get statements from attributes_program->statements
		// into act_type_beh.tasks[0]->prog - which needs to be created,
		// unless that was already done when handling class variables
		//

		if(constructor_program) {
			for(int i = 0; i < attributes_program->statements.size(); i++) {
				constructor_program->addExp(
					parsedExp(attributes_program->statements[i]->copy()));
			}
		} else {
			constructor_program = new Program(*attributes_program);
		}
		constructor_program->orig_section = apgen::METHOD_TYPE::ATTRIBUTES;
	}

	//
	// Now we may consolidate the type constructor, which contains class
	// variables as well as any attributes
	//

	if(constructor_program) {
		aafReader::consolidate_program(
				*constructor_program,
				dent + 2);
		if(!Constructor->prog) {
			Constructor->prog.reference(constructor_program);
		}
	}

	//
	// NOTE: an activity type always has a constructor since its Behavior
	// does. However, the constructor may not have a program associated
	// with it; this will be the case if the activity type
	//
	// 	- has no class variables
	// 	- has no attributes
	// 	- has no parameters
	//

	//
	// PART 5: grab creation, resource usage, modeling, decomposition and
	//         destruction sections and define them as
	//         new tasks in the activity type's class
	//

	ActTypeBody* body = dynamic_cast<ActTypeBody*>(
		body_of_activity_type_def.object());
	assert(body);
	TypeCreationSection* creation = dynamic_cast<TypeCreationSection*>(
		body->opt_creation_section.object());
	if(creation) {
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent + 2) << "creation section: "
				<< body->opt_creation_section->getData()
				<< "\n";
		}
		Program* creationP = dynamic_cast<Program*>(creation->program.object());
		assert(creationP);
		creationP->orig_section = apgen::METHOD_TYPE::CREATION;
		int k = act_type_beh.add_task("creation");
		task* curtask = act_type_beh.tasks[k];
		curtask->prog.reference(creationP);
		aafReader::push_current_task(act_type_beh.tasks[k]);

		aafReader::consolidate_program(*creationP, dent + 2);

		aafReader::pop_current_task();
	}
	ModelingSection* modeling = dynamic_cast<ModelingSection*>(
		body->opt_res_usage_section.object());

	if(modeling) {
		handle_modeling_program(modeling, act_type_beh, dent + 2);
	}
	ActTypeDecomp* decomp = dynamic_cast<ActTypeDecomp*>(
		body->opt_decomposition_section.object());
	if(decomp) {
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent + 2) << "decomp section: "
				<< body->opt_decomposition_section->getData()
				<< "\n";
		}

		Program*		decompP = dynamic_cast<Program*>(decomp->program.object());

		assert(decompP);

		Cstring			task_name = decomp->decomposition_header->getData();
		apgen::METHOD_TYPE	mt = apgen::METHOD_TYPE::NONE;

		bool			need_to_add_a_wait_statement = false;

		if(task_name == "nonexclusive_decomposition") {
			mt = apgen::METHOD_TYPE::NONEXCLDECOMP;
		} else if(task_name == "decomposition") {
			mt = apgen::METHOD_TYPE::DECOMPOSITION;
		} else if(task_name == "concurrent_expansion" || task_name == "expansion") {
			need_to_add_a_wait_statement = true;
			mt = apgen::METHOD_TYPE::CONCUREXP;
		} else {
			assert(false);
		}
		task_name = "decompose";
		decompP->orig_section = mt;

		int k = act_type_beh.add_task(task_name);
		task* curtask = act_type_beh.tasks[k];
		curtask->prog.reference(decompP);
		aafReader::push_current_task(act_type_beh.tasks[k]);

		if(need_to_add_a_wait_statement) {
			add_a_wait_statement(decomp->decomposition_header, decompP);
		}

		//
		// While consolidating this program,
		// we need to watch out for statements that assign
		// a value to ActivityInstance::SPAN
		//
		aafReader::consolidate_program(*decompP, dent + 2);

		aafReader::pop_current_task();

		//
		// In case the modeling section appears after the
		// decomposition section, we need to handle it here!!
		//

		ModelingSection* modeling2 = dynamic_cast<ModelingSection*>(
				decomp->opt_res_usage_section.object());
		if(modeling2) {
			handle_modeling_program(modeling2, act_type_beh, dent);
		}
	}
	TypeCreationSection* destruction = dynamic_cast<TypeCreationSection*>(
		body->opt_destruction_section.object());
	if(destruction) {
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent) << "destruction section: "
				<< body->opt_destruction_section->getData()
				<< "\n";
		}
		Program* destructionP = dynamic_cast<Program*>(destruction->program.object());
		assert(destructionP);
		destructionP->orig_section = apgen::METHOD_TYPE::DESTRUCTION;

		int k = act_type_beh.add_task("destruction");
		task* curtask = act_type_beh.tasks[k];
		curtask->prog.reference(destructionP);
		aafReader::push_current_task(act_type_beh.tasks[k]);

		aafReader::consolidate_program(*destructionP, dent + 2);

		aafReader::pop_current_task();
	}

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "Final version of act type:\n";
		aoString aos;
		act_type_beh.to_stream(&aos, dent + 2);
		cerr << aos.str();
	}

	aafReader::pop_current_task();
}

void ActTypeClassVariables::consolidate(
		int dent) {
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent)
			<< "consolidating ActTypeClassVariables "
			<< getData() << "\n";
	}

	//
	// we've already checked that type_class_variables exists
	//
	Program* pp = dynamic_cast<Program*>(type_class_variables.object());
	assert(pp);
	Program* qq = dynamic_cast<Program*>(pp->copy());
	assert(qq);
	task* T = aafReader::get_current_task();
	T->prog.reference(qq);

	//
	// We do not consolidate yet; attributes may be added to the type
	// constructor next, and we want to wait until they have been added
	// before consolidating the program.
	//

	// consolidate(*pp);
}
