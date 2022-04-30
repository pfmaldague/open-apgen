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
#include "Constraint.H"
#include "fileReader.H"
#include "lex_intfc.H"
#include "RES_def.H"

extern int abdebug;
extern int yylineno;

using namespace pEsys;

class function_finder: public exp_analyzer {
public:
    stringtlist	missing_functions;
    stringtlist unfinished_functions;
    Cstring		global_name;

    function_finder(const Cstring& name)
    	: global_name(name) {}

    virtual void pre_analyze(pE* exp) {}
    virtual void post_analyze(pE* exp) {
	FunctionCall* fc = dynamic_cast<FunctionCall*>(exp);
	bool	unfinished = false;
	bool	missing = false;

	if(fc) {
	    Cstring function_name = fc->getData();
	    if(aaf_intfc::internalAndUdefFunctions().find(function_name)) {
		    // we are OK
		    return;
	    }
	    map<Cstring, int>::iterator iter
			= aafReader::CurrentType().taskindex.find(
					function_name);
	    if(iter == aafReader::CurrentType().taskindex.end()) {
			missing = true;
			if(!missing_functions.find(function_name)) {
				missing_functions << new emptySymbol(function_name);
			}
	    } else {
		task* func_task = aafReader::CurrentType().tasks[iter->second];
		if(!func_task->prog) {
		    Cnode0<alpha_string, smart_ptr<FunctionDefinition> >* cn =
			aafReader::functions_declared_but_not_implemented().find(function_name);
		    if(cn) {
			unfinished = true;
			if(!unfinished_functions.find(function_name)) {
				unfinished_functions << new emptySymbol(function_name);
				cn->payload->recursively_apply(*this);
			}
		    } else {
			missing = true;
			if(!missing_functions.find(function_name)) {
				missing_functions << new emptySymbol(function_name);
			}
		    }
		}
	    }
	    // debug
	    // cerr << "finder: global " << global_name << " needs function "
	    // 	<< function_name << " which is ";
	    // if(missing) {
	    // 	cerr << "missing\n";
	    // } else if(unfinished) {
	    // 	cerr << "unfinished\n";
	    // } else {
	    // 	cerr << "OK\n";
	    // }
	}
    }
};

class permanent_string_maker {
public:
	permanent_string_maker() {
		Cstring::make_all_strings_permanent = true;
	}
	~permanent_string_maker() {
		Cstring::make_all_strings_permanent = false;
	}
};

namespace aafReader {

//
// Loosely phrased requirement: at the end of the consolidation process,
// everything that has been parsed out of the AAF should have a functional
// eval_expression() method, or a functional execute() method is it is an
// executable expression.
//
void consolidate(bool skip_remodeling /* = false */ ) {
	permanent_string_maker	PSM;

	initialize_counts_and_behaviors();

	//
	// We handle consolidation in two passes: declaration and
	// implementation. The reason for this is that forward declarations
	// in the APGen DSL are not sufficient for building dependent objects
	// that reference them; APGen often needs most of the object to be
	// actually available. This is a problem e. g. for resources that have
	// dependencies on each other via profiles invoking currentval().
	// While in principle the code could be organized so that everything
	// appears in exactly the right order, we don't want to impose that
	// requirement on adapters. Hence the two passes.
	//
	// One tricky aspect of the two-pass mechanism concerns globals,
	// functions and resource arrays. Because global definitions may
	// invoke functions that are not available yet, global evaluation is
	// deferred till the implementation pass. However, if a resource array
	// uses indices taken from a global, it is essential to consolidate the
	// global array completely before the resource can be constructed.
	// Therefore, we need to identify all global arrays that are used as
	// resource indices so we can direct the consolidation algorithm to do
	// the right thing.
	//

	//
	// Consolidation basically proceeds in the order in which objects are
	// defined in the adaptation and plan files. However, there are a few
	// exceptions due to the possible dependencies of objects on one
	// another.
	//
	// Possible dependencies:
	//
	//	 1. a global definition can reference another global
	//
	// 	 2. a global definition can reference a function
	//
	// 	 3. a function can reference another function
	//
	// 	 4. a function can reference a global
	//
	// 	 5. a resource array can reference a global
	//
	// 	 6. a resource profile can reference another resource (via
	// 	    currentval())
	//
	// 	 7. an abstract resource can reference a concrete resource or
	// 	    resource array
	//
	// 	 8. an abstract resource can reference another abstract resource
	//
	// 	 9. an activity type can reference concrete resources
	//
	// 	10. an activity type can reference abstract resources
	//
	// 	11. an activity type can reference other activity types
	//
	// In principle, all items required by an object should have been
	// defined by the time the object is consolidated. This requires a
	// strict ordering of object definitions, which is difficult to
	// achieve in a large adaptation consisting of many files. For this
	// reason, consolidation is carried out in two passes:
	//
	// 	- a declaration pass, in which objects are defined along with
	// 	  their parameters
	//
	// 	- an implementation pass, in which all object methods are
	// 	  filled in in detail
	//
	// Since the declaration pass skips the "body" of object definitions
	// (e. g. the body of a function or the modeling and decomposition
	// sections of activity types), it is largely independent of the order
	// in which definitions occur in the adaptation file.
	//
	// In the implementation pass, the "body" of object definitions is
	// fully handled. But by this time, all required objects have already
	// been partially defined thanks to the work done in the declaration
	// pass, and consolidation can proceed regardless of the order in
	// which objects are defined.
	//
	// That's theory. Unfortunately, practice isn't quite this nice
	// because of a few nasty details:
	//
	// 	1. when a function is used in an adaptation program, the
	// 	   consolidation algorithm requires not only its signature,
	// 	   which is handled in the declaration pass, but also its type,
	// 	   which requires analyzing the body of the function since the
	// 	   APGen DSL does not specify the returned type in the function
	// 	   header. Therefore, a function cannot be used until after it
	// 	   has been fully defined.
	//
	//      2. when a resource array is processed in the declaration
	//         phase, the consolidation algorithm requires a complete
	//         specification of the array(s) of indices. These indices can
	//         be explicit (e. g. ["one", "two", "three"])  but they can
	//         also refer to global arrays, in which case the array should
	//         have been fully defined by the time the resource is being
	//         processed. But in the declaration phase, global array
	//         processing determines the name and the type of the global
	//         but the value of the global array is not evaluated, since
	//         that might require exercising a function which has not been
	//         fully defined yet. Worse, the function on which this global
	//         array definition relies may itself require another global
	//         variable to be defined, in which case we get yet another
	//         dependency on a new global.
	//
	// The problem of global dependencies is not specific to the APGen
	// DSL; other languages such as C++ need to address this issue as
	// well. For the time being, the following policy will be adopted.
	// This policy is consistent with the behavior of "legacy APGen". Here
	// it is:
	//
	// 	1. a function can only reference globals and functions that
	// 	   have already been defined
	//
	// 	2. a global definition can only reference globals and functions
	// 	   that have already been defined
	//
	// 	3. the indices of a resource array can only reference globals
	// 	   that have already been defined. However, the profile and
	// 	   usage sections of the resource can refer (via currentval())
	// 	   to resources that occur anywhere in the adaptation file(s).
	//
	// 	4. the modeling or resource usage section of an abstract
	// 	   resource or activity type definition can invoke concrete or
	// 	   abstract resources that occur anywhere in the adaptation
	// 	   file(s).
	//
	// 	5. the decomposition section of an activity type definition
	// 	   can invoke activity types that occur anywhere in the
	// 	   adaptation file(s).
	//

	bool did_something = false;

	for(int phase = 0; phase < 2; phase++) {

		if(phase == 0) {
			CurrentPass() = DeclarationPass;
		} else {
			CurrentPass() = ImplementationPass;
		}

		for(int m = 0; m < input_files().size(); m++) {
			parsedExp		item = input_files()[m];
			InputFile*	iF
				= dynamic_cast<InputFile*>(item.object());
			if(!iF) {

				//
				// Not an error - the file is empty
				//

				continue;
			}
			try {
				did_something = true;
				consolidate(*iF);
			} catch(eval_error Err) {
				Cstring err;
				err << "consolidation error:\n" << Err.msg;
				input_files().clear();
				throw(eval_error(err));
			}
		}
	}

	for(int m = 0; m < input_files().size(); m++) {
		if(input_files()[m]) {

			//
			// Experiment: shed parsed information to save
			// memory. Main impact is on output features;
			// spitting back parsed info is more difficult now.
			//

			//
			// Temporarily reinstate to re-enable AAF output
			//

			consolidated_files().push_back(input_files()[m]);
			input_files()[m].dereference();
		}
	}

	input_files().clear();

	int layout_count = 0;
	try {
		ACT_exec::executeNewDirectives(directives(), layout_count);
	} catch(eval_error Err) {
		Cstring errs;
		errs << "Error executing directives:\n" << Err.msg;
		throw(eval_error(errs));
	}

	if(ConcreteResourceCount) {
	    consolidate_resource_dependencies();
	}

	if(ConstraintCount) {
	    try {
		consolidate_constraints_round_two();
	    } catch(eval_error Err) {
		Cstring errs;
		errs << "Error encountered while checking global usage by constraints after consolidation:\n";
		errs << Err.msg;
		throw(eval_error(errs));
	    }
	}

	//
	// We indent parentheses the way we indent curly brackets:
	//
	if(
	    did_something
	    && (
		!skip_remodeling
	    	&& APcloptions::theCmdLineOptions().AutomaticRemodelOnFileRead
	   )
	) {

		//
		// REMODEL (without issuing the action request)
		//
		try {

			//
			// Sets the modeling pass:
			//
			model_control control(model_control::MODELING);

			//
			// Sets theMasterTime:
			//
			model_intfc::init_model(
				ACT_exec::ACT_subsystem().theMasterTime);

			ACT_exec::ACT_subsystem().generate_time_events();

			//
			// Do this, because modeling may result in multiple threads
			// and making strings permanent is not thread-safe
			//
			Cstring::make_all_strings_permanent = false;

			model_intfc::doModel(ACT_exec::ACT_subsystem().theMasterTime);

			//
			// Restore permanent string-making
			//
			Cstring::make_all_strings_permanent = true;

		} catch(eval_error Err) {
			Cstring errs;
			errs << "Error encountered while remodeling after consolidation:\n";
			errs << Err.msg;
			throw(eval_error(errs));
		}
	}
}

extern void consolidate_one(pE& item);

void consolidate(InputFile& iF, int) {
	File*	f = dynamic_cast<File*>(iF.file_body.object());
	assert(f);
	int count = 0;

	current_file() = iF.InputFileName;
	if(!eval_intfc::ListOfAllFileNames().find(current_file())) {
		eval_intfc::ListOfAllFileNames() << new emptySymbol(current_file());
	}

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << "Top level contents[" << ++count << "]: ";
		if(f->adaptation_item) {
			cerr << f->adaptation_item->to_string() << "\n";
		} else if(f->plan_item) {
			cerr << f->plan_item->to_string() << "\n";
		}
	}

	//
	// Initialize globals that influence activity instance consolidation
	//

	ActivityInstanceCount = 0;
	fileParser::currentParser().Instances.clear();
	fileParser::currentParser().ListOfActivityPointersIndexedByIDsInThisFile.clear();

	//
	// Consolidate everything in the file
	//

	if(f->adaptation_item) {
		consolidate_one(*f->adaptation_item.object());
	} else if(f->plan_item) {
		consolidate_one(*f->plan_item.object());
	}
	for(int m = 0; m < f->expressions.size(); m++) {
		parsedExp item = f->expressions[m];

		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << "Top level contents[" << ++count << "]: "
				<< item->to_string() << "\n";
		}

		consolidate_one(*item.object());
	}

#ifdef VERBOSE

	static double prev_usage = 0.0;
	double total_usage = 0.0;
	static bool usage_initialized = false;
	total_usage = ((double) MemoryUsed()) / (1024.0 * 1024.0);
	if(!usage_initialized) {
		usage_initialized = true;
		prev_usage = total_usage;
	}
	cerr << current_file() << ":\n";
	cerr << "    incr. memory usage: " << (total_usage - prev_usage) << "\n";
	cerr << "    total memory usage: " << total_usage << "\n";
	prev_usage = total_usage;

#endif /* VERBOSE */

	//
	// Instantiate any activity instances found
	//

	if(ActivityInstanceCount) {

		//
		// Note: this only takes place in the second, implementation pass
		//

		fileParser::currentParser().FixAllPointersToParentsAndChildren(true);
		ACT_exec::ACT_subsystem().instantiate_all(fileParser::currentParser().Instances);
		eval_intfc::get_act_lists().synchronize_all_lists();

		//
		// Delete ActInstances to save memory
		//

		iF.delete_all_instances();
		ActivityInstanceCount = 0;
	}
}

void consolidate_one(pE& item) {
	//
	// Possible children:
	// 	ActInstance
	// 	ActType
	// 	CustomAttributes
	// 	Directive
	// 	FunctionDeclaration
	// 	FunctionDefinition
	// 	Global
	// 	PassiveCons
	// 	Resource
	// 	Typedef

	if(CurrentPass() == DeclarationPass) {
		Typedef* tdef = dynamic_cast<Typedef*>(&item);
		if(tdef) {
			consolidate(*tdef, 2);
			return;
		}
		CustomAttributes* cust_att = dynamic_cast<CustomAttributes*>(&item);
		if(cust_att) {
			cust_att->consolidate(2);
			return;
		}
		Resource* resource = dynamic_cast<Resource*>(&item);
		if(resource) {
			resource->consolidate_declaration_phase(2);
			return;
		}

#ifdef OBSOLETE
		PrecomputedResource* precomp_res = dynamic_cast<PrecomputedResource*>(&item);
		if(precomp_res) {

			//
			// This is similar to a global in the sense that
			// we execute the initialization function of the
			// precomputed resources as soon as we consolidate
			// it.
			//
			precomp_res->consolidate(2);
			return;
		}
#endif /* OBSOLETE */

		Global* glob = dynamic_cast<Global*>(&item);
		if(glob) {

			//
			// First, figure out which functions, if any, are
			// required to evaluate the global. This needs to
			// be recursive since functions can invoke functions,
			// as can other global definitions.
			//
			function_finder FF(glob->tok_id->getData());
			glob->Expression->recursively_apply(FF);

			//
			// Report an error if any functions are missing
			//
			aoString aos1;
			bool functions_are_missing = false;
			bool functions_are_unfinished = false;

			stringslist::iterator iter(FF.missing_functions);
			emptySymbol* es;
			while((es = iter())) {
				functions_are_missing = true;
				aos1 << es->get_key() << " ";
			}
			if(functions_are_missing) {
				Cstring errs;
				errs << "File " << glob->file << ", line " << glob->line
					<< ": the following functions need to be fully "
					<< "defined to evaluate global "
					<< glob->tok_id->getData() << ": "
					<< aos1.str();
				throw(eval_error(errs));
			}

			//
			// Now let's finish consolidating any required
			// functions that were declared but not fully
			// implemented.
			//
			// We trick the function consolidator into believing
			// that this is the second pass...
			//
			CurrentPass() = ImplementationPass;
			stringslist::iterator iter2(FF.unfinished_functions);
			while((es = iter2())) {
				Cnode0<alpha_string, smart_ptr<FunctionDefinition> >* cn =
					functions_declared_but_not_implemented().find(es->get_key());
				assert(cn);
				cn->payload->consolidate(2);
			}

			//
			// ... then we restore the pass variable.
			CurrentPass() = DeclarationPass;

			//
			// Finally, we finish consolidating the global
			// variable which we are processing.
			//
			glob->consolidate(2);
			return;
		}
		FunctionDeclaration* funcdecl = dynamic_cast<FunctionDeclaration*>(&item);
		if(funcdecl) {
			funcdecl->consolidate(2);
			return;
		}
		Directive* direct = dynamic_cast<Directive*>(&item);
		if(direct) {
			function_finder FF(direct->getData());
			direct->recursively_apply(FF);
			aoString aos1;
			bool functions_are_missing = false;
			bool functions_are_unfinished = false;
			stringslist::iterator iter(FF.missing_functions);
			emptySymbol* es;
			while((es = iter())) {
				functions_are_missing = true;
				aos1 << es->get_key() << " ";
			}
			if(functions_are_missing) {
				Cstring errs;
				errs << "File " << direct->file << ", line " << direct->line
					<< ": the following functions need to be fully "
					<< "defined to evaluate directive: "
					<< aos1.str();
				throw(eval_error(errs));
			}

			//
			// Now let's finish consolidating any required functions that
			// were declared but not fully implemented
			//

			CurrentPass() = ImplementationPass;
			stringslist::iterator iter2(FF.unfinished_functions);
			while((es = iter2())) {
				Cnode0<alpha_string, smart_ptr<FunctionDefinition> >* cn =
					functions_declared_but_not_implemented().find(es->get_key());
				assert(cn);
				cn->payload->consolidate(2);
			}
			CurrentPass() = DeclarationPass;
			direct->consolidate(2);
			return;
		}
	} else if(CurrentPass() == ImplementationPass) {
		ActInstance* actinstance = dynamic_cast<ActInstance*>(&item);
		if(actinstance) {
			actinstance->consolidate(2);
			return;
		}
		PassiveCons* passivecons = dynamic_cast<PassiveCons*>(&item);
		if(passivecons) {
			passivecons->consolidate(2);
			return;
		}
		Resource* resource = dynamic_cast<Resource*>(&item);
		if(resource) {
			resource->consolidate_implementation_phase(2);
			return;
		}
	}

	//
	// The following expressions need to detect whether the current pass
	// is declaration or implementation, and to act accordingly:
	//

	ActType* acttype = dynamic_cast<ActType*>(&item);
	if(acttype) {
		acttype->consolidate(2);
		return;
	}
	FunctionDefinition* funcdef = dynamic_cast<FunctionDefinition*>(&item);
	if(funcdef) {
		funcdef->consolidate(2);
		return;
	}
}


//
// Build the tree of resources dependencies via their profile
//

void consolidate_resource_dependencies() {
	RCsource*				theContainer;
	Rsource*				theResource;
	stringtlist				names;
	emptySymbol*				strnode;
	emptySymbol*				container_used_in_profile;
	RCsource*				container_whose_currentval_is_used_in_profile;
	RCsource::container_ptrs		resources_without_circular_dependencies;
	RCsource::container_ptrs		CheckList;
	bool					is_an_array;
	long					count;
	Cstring					errormsg;
	String_node*				Sn;
	const char*				errcontext = "";

	eventPLD::resources_with_no_dependencies().clear();

	slist<alpha_string, RCsource>::iterator	all_containers(RCsource::resource_containers());


	//
	// First step: clean up previously computed dependencies
	//
	while((theContainer = all_containers())) {
		theContainer->payload->containers_whose_profile_invokes_this.clear();
		theContainer->payload->ptrs_to_containers_used_in_profile.clear();
		theContainer->payload->names_of_containers_used_in_profile.clear();
	}

	//
	// Second step: compute direct profile dependencies
	//
	Rsource::iterator res_iter;
	Rsource* rs;
	while((rs = res_iter.next())) {
	    slist<alpha_time, Cnode0<alpha_time, parsedExp> >::iterator profile_nodes(rs->theProfile);
	    Cnode0<alpha_time, parsedExp>* one_node;
	    while((one_node = profile_nodes())) {
		currentval_finder CF;
		one_node->payload->recursively_apply(CF);
		resContPLD* cont_pld = rs->parent_container.payload;
		emptySymbol* es;
		stringslist::iterator iter(CF.container_names);
		while((es = iter())) {
		    if(!cont_pld->names_of_containers_used_in_profile.find(es->get_key())) {

			//
			// debug
			//
			// cerr << "Res. " << rs->name << " depends on " << es->get_key() << "\n";

			cont_pld->names_of_containers_used_in_profile << new emptySymbol(es->get_key());
		    }
		}
	    }
	}

	//
	// Here we establish the list of direct dependents and direct
	// prerequisites, in the sense of profile dependency
	//
	while((theContainer = all_containers())) {
	    for(	container_used_in_profile = theContainer->payload
				->names_of_containers_used_in_profile.first_node();
			container_used_in_profile;
			container_used_in_profile
				= container_used_in_profile->next_node()
	       ) {
		if(!(container_whose_currentval_is_used_in_profile
				= RCsource::resource_containers().find(
						container_used_in_profile->get_key()))) {

		    Cstring errs = Cstring("resource container ") + theContainer->get_key()
				+ "'s profile needs undefined resource container \""
				+ container_used_in_profile->get_key() + "\"";
		    throw(eval_error(errs));

		} else if(container_whose_currentval_is_used_in_profile == theContainer) {

		    Cstring errs = Cstring("resource container ") + theContainer->get_key()
				+ "'s profile refers to self";
		    theContainer->clear_profiles();
		    throw(eval_error(errs));
		}

		//
		// check whether resource is associative - for now, cannot mix and match!
		//
		if( (theContainer->is_associative()
			&& !container_whose_currentval_is_used_in_profile->is_associative())
		    || (!theContainer->is_associative()
			&& container_whose_currentval_is_used_in_profile->is_associative())
		  ) {

		    Cstring errs = Cstring("resource containers ") + theContainer->get_key()
				+ " and " + container_whose_currentval_is_used_in_profile->get_key()
				+ " are not both associative or non-associative; "
				"mixing associative character through currentval() is not allowed.";

		    theContainer->clear_profiles();
		    throw(eval_error(errs));
		}
		if(!container_whose_currentval_is_used_in_profile
			->payload->containers_whose_profile_invokes_this
				.find((void *) theContainer)) {

		    //
		    // debug
		    //
		    // cerr << ">>>>> Adding " << theContainer->get_key()
		    // 	<< " to containers whose profile invokes "
		    // 	<< container_whose_currentval_is_used_in_profile->get_key() << "\n";

		    container_whose_currentval_is_used_in_profile
			->payload->containers_whose_profile_invokes_this
				<< new RCsource::container_ptr(theContainer, theContainer);
		}

		//
		// the same container may appear several times in a single profile:
		//
		if(!theContainer->payload->ptrs_to_containers_used_in_profile.find(
				(void *) container_whose_currentval_is_used_in_profile)) {


		    //
		    // debug
		    //
		    // cerr << ">>>>> Adding " << container_whose_currentval_is_used_in_profile->get_key()
		    // 	<< " to ptrs to containers used in profile of " << theContainer->get_key()
		    // 	<< "\n";

		    theContainer->payload->ptrs_to_containers_used_in_profile
				<< new RCsource::container_ptr(
						container_whose_currentval_is_used_in_profile,
						container_whose_currentval_is_used_in_profile);
		}
	    }

	    if(!theContainer->payload->ptrs_to_containers_used_in_profile.get_length()) {

		//
		// debug
		//
		// cerr << "   <<<  Adding " << theContainer->get_key()
		// 	<< " to the list of resources w/o dependencies\n";

		eventPLD::resources_with_no_dependencies()
			<< new RCsource::container_ptr((void*) theContainer, theContainer);

		// debug
		// cerr << "   <<<  Adding " << theContainer->get_key()
		// 	<< " to the list of resources w/o circular deps.\n";

		resources_without_circular_dependencies
			<< new RCsource::container_ptr(theContainer, theContainer);
	    }
	}

	//
	// Next we "take the closure" of the relation "has dependent"
	// through a recursive call to complete_dependency_list()
	//
	RCsource::container_ptrss::iterator
					reslst2(eventPLD::resources_with_no_dependencies());
	Cstring				errors;
	RCsource::container_ptr*	rcptr;

	while((rcptr = reslst2())) {
	    theContainer = rcptr->payload;
	    CheckList.clear();

	    //
	    //		IMPORTANT
	    //		=========
	    //
	    // Before the following call, containers_whose_profile_invokes_this contains only the first-order
	    // (immediate) dependencies. After the call, the notion of dependency has been extended to the
	    // entire dependency tree below each node.
	    //
	    if(theContainer->complete_dependency_list(
				theContainer->payload->containers_whose_profile_invokes_this,
				resources_without_circular_dependencies,
				CheckList,	// starts empty (Duh, theContainer has no dependencies)
				errors) != apgen::RETURN_STATUS::SUCCESS) {
	    	Cstring errs = theContainer->get_key() + " starts circular dependency list, i. e. "
					+ errors;
	    	throw(eval_error(errs));
	    }
	}

	//
	// Next we compute the inverse relation, "has a dependency on ".
	//
	while((theContainer = all_containers())) {

	    //
	    // debug
	    //
	    // cerr << theContainer->get_key() << " has no dependency.\n";

	    RCsource::container_ptrss::iterator
			reslst2(theContainer->payload->containers_whose_profile_invokes_this);
	    RCsource::container_ptr*	ptr_to_dependent;

	    while((ptr_to_dependent = reslst2())) {
		RCsource*	dependent = ptr_to_dependent->payload;

		if(!dependent->payload->ptrs_to_containers_used_in_profile.find(
				(void *) theContainer)) {
			dependent->payload->ptrs_to_containers_used_in_profile
				<< new RCsource::container_ptr(theContainer, theContainer);

			//
			// debug
			//
			// cerr << ">>>> adding  " << theContainer->get_key() << " as a dependency of "
			// 	<< dependent->get_key() << "\n";
		} else {
			//
			// debug
			//
			// cerr << ">>>> " << theContainer->get_key() << " is already a dependency of "
			// 	<< dependent->get_key() << "\n";
		}
	    }
	}

	//
	// reminder:
	//
	// slist<alpha_string, RCsource>::iterator
	//	all_containers(RCsource::resource_containers());
	//

	//
	// Now we compute a list of resource containers that is suitable for
	// resource initialization. The key requirement is that all the
	// containers required by container A appear earlier than A in the list.
	//
	// The list must obviously start with containers with no dependencies.
	// It then goes on with containers which only depend on containers with
	// no dependencies, and so on.
	//

	RCsource::initialization_list().clear();
	while(true) {
	    while((theContainer = all_containers())) {
		RCsource::container_ptrss::iterator
			prerequisites(theContainer->payload->ptrs_to_containers_used_in_profile);
		RCsource::container_ptr*	prereq;
		bool				prerequisites_are_satisfied = true;

		while((prereq = prerequisites())) {
			if(!RCsource::initialization_list().find(prereq->payload)) {
				prerequisites_are_satisfied = false;
				break;
			}
		}
		if(prerequisites_are_satisfied) {
		    if(!RCsource::initialization_list().find(theContainer)) {
			RCsource::initialization_list()
			    << new RCsource::container_ptr(theContainer, theContainer);

			//
			// debug
			//
			// cerr << "    adding " << theContainer->get_key() << " to the initialization list.\n";
		    }
		}
	    }
	    if(RCsource::initialization_list().get_length()
			== RCsource::resource_containers().get_length()) {

		//
		// debug
		//
		// cerr << "    all containers taken care of; DONE with the initialization list.\n";

		//
		// We are all done
		//
		break;
	    } else {

		//
		// debug
		//
		// cerr << "containers to initialize: "
		//	<< (RCsource::resource_containers().get_length()
		//		- RCsource::initialization_list().get_length())
		//	<< " still looking for prerequisites.\n";
	    }
	}
}

void consolidate_constraints_round_two() {

    slist<alpha_string, Constraint>::iterator	con_iter(Constraint::allConstraints());
    Constraint*					constraint;

    while((constraint = con_iter())) {
	constraint->payload->consolidate_round_two(constraint);

	//
	// Analyze the use of non-constant globals in the constraint body
	//
	constraint_analyzer	CA;
	// CA.debug_constraint_analyzer = true;
	constraint->payload->recursively_apply(CA);
    }
}

} /* namespace aafReader */

