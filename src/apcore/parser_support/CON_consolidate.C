#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/stat.h>

#include "aafReader.H"
#include "apcoreWaiter.H"
#include "Constraint.H"
#include "Rsource.H"

using namespace pEsys;

void PassiveCons::consolidate(int dent) {
    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << aafReader::make_space(dent) << "consolidating PassiveCons " << getData() << "\n";
    }

    //
    // What have we got here? From the grammar:
    //
    //   PassiveConstraint has the following members:
    //      tok_id
    //	    which is the type of the constraint
    //	tok_id_1
    //	    which is the name of the constraint
    //	tok_id_10
    //	    which is a repeat of the constraint name
    //	constraint_body
    //	    which is a PassiveConsItem with the following members:
    //	    	constraint_item
    //	    	more constraint_items in its expressions vector
    //	constraint_message
    //	constraint_severity
    //
    // For now, we are going to issue a few debug messages. Later we will
    // do more work.
    //

    Cstring name = tok_id_1->to_string();
    Cstring type = tok_id->to_string();
    Violation_info::Severity severity_code = Violation_info::RELEASE;

    int n = Behavior::add_type(new Behavior(
		name,
		/* realm = */ "constraint",
		NULL));
    Behavior& constraint_beh = *Behavior::ClassTypes()[n];
    task* Constructor = constraint_beh.tasks[0];
    aafReader::push_current_task(Constructor);

    //
    // Consolidate the pieces, but first identify them
    //
    if(opt_constraint_class_variables) {
	Declaration* one_var
		= dynamic_cast<Declaration*>(
			opt_constraint_class_variables.object());
	assert(one_var);

	//
	// emulate ActTypeClassVariables::consolidate()
	//
	one_var->consolidate(dent + 2);

	//
	// Account for the presence of '{' and '}'
	//
	for(int w = 1; w < one_var->expressions.size() - 1; w++) {
	    Declaration* another =
		dynamic_cast<Declaration*>(
			one_var->expressions[w].object());
	    assert(another);
	    another->consolidate(dent + 2);
	}
    }

    PassiveConsMessage* message = dynamic_cast<PassiveConsMessage*>(constraint_message.object());
    if(message) {
	theMessage = message->Expression;
	aafReader::consolidate_expression(theMessage, dent + 2);
    } else {
	aafReader::pop_current_task();
	Cstring errs;
	errs << "File " << file << ", line " << line << ": constraint "
		<< name << " has no message.";
	throw(eval_error(errs));
    }

    //
    // The severity is obtained as constraint_severity->getData()
    //
    Cstring aaf_severity = constraint_severity->getData();
    if(aaf_severity[0] == '"') {
	removeQuotes(aaf_severity);
    }
    if(!strcasecmp(*aaf_severity, "WARNING")) {
	severity_code = Violation_info::WARNING;
	severity = "WARNING";
    } else if(!strcasecmp(*aaf_severity, "ERROR")) {
	severity_code = Violation_info::ERROR;
	severity = "ERROR";
    } else {
	Cstring errs;
	aafReader::pop_current_task();
	errs << "File " << file << ", line " << line << ": constraint "
		<< name << " has non-standard severity "
		<< constraint_severity->getData()
		<< "; it should be WARNING or ERROR.";
	throw(eval_error(errs));
    }

    aafReader::ConstraintCount++;

    //
    // Ideally, we would like to finish consolidation in the next block of
    // code. However, we have not yet computed resource dependencies; that
    // will take place at the end of consolidation. Therefore, we have to
    // postpone the final part of constraint consolidation till after the
    // resources have been fully processed.
    //
    // Method "consolidate_second_round()" handles this final phase of
    // consolidation. The main purpose of this phase of consolidation
    // is that we will populate the list
    //
    //	    Constraint::allConstraints()
    //
    // with one more Constraint object; we will put the finishing
    // touches to it, including a list of all resources that are
    // queried by it, including dependencies, in round two.
    //
    PassiveConsItem* item = dynamic_cast<PassiveConsItem*>(constraint_body.object());
    if(item) {

	//
	// Make sure the constraint name is unique
	if(Constraint::allConstraints().find(name)) {
		aafReader::pop_current_task();
		Cstring errs;
		errs << "File " << file << ", line " << line << ": constraint "
			<< name << " already exists.";
		throw(eval_error(errs));
	}

	//
	// We now create a wrapper around this PassiveConstraint and
	// store it. The rationale for such a wrapper is that we want
	// to decouple the details of constraint handling from the
	// tasks of constraint parsing and consolidation.
	//

	if(type == "forbidden_condition") {
	    if(item->getData() != "condition") {
		aafReader::pop_current_task();
		Cstring errs;
		errs << "File " << file << ", line " << line << ": constraint "
			<< name << " appears to be malformed; its body "
			<< "should start with the keyword \"condition\".";
		throw(eval_error(errs));
	    }
	    vector<parsedExp>& cond_stuff = item->constraint_item->expressions;

	    //
	    // Consolidate the forbidden condition, which is the
	    // first item in cond_stuff
	    //
	    condition = cond_stuff[0];
	    aafReader::consolidate_expression(condition, dent + 2);

	    //
	    // Create the wrapper in the Constraint subsystem
	    //
	    Constraint::allConstraints() << new ForbiddenCondition(name, this, Constructor);

	    //
	    // To be continued in consolidate_round_two().
	    //

	} else if(type == "maximum_duration") {
	    if(item->getData() != "condition") {
		aafReader::pop_current_task();
		Cstring errs;
		errs << "File " << file << ", line " << line << ": constraint "
			<< name << " appears to be malformed; its body "
			<< "should start with the keyword \"condition\".";
		throw(eval_error(errs));
	    }
	    vector<parsedExp>& cond_stuff = item->constraint_item->expressions;

	    //
	    // First item is the expression that
	    // implements the condition
	    //
	    condition = cond_stuff[0];
	    aafReader::consolidate_expression(condition, dent + 2);

	    if(	item->expressions.size() != 1
	    	|| item->expressions[0]->getData() != "duration"
	      ) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": constraint "
			<< name << " appears to be malformed; the 2nd item in its body "
			<< "should start with the keyword \"duration\".";
		throw(eval_error(errs));
	    }
	    vector<parsedExp>& dur_stuff = item->expressions[0]->expressions;
	    if(dur_stuff.size() != 2) {
		aafReader::pop_current_task();
		Cstring errs;
		errs << "File " << file << ", line " << line << ": constraint "
			<< name << " appears to be malformed; the duration "
			<< "should be specified by an expression.";
		throw(eval_error(errs));
	    }

	    //
	    // Second item is the expression that implements the
	    // duration.
	    //
	    aafReader::consolidate_expression(dur_stuff[0], dent + 2);
	    TypedValue dur_value;
	    dur_stuff[0]->eval_expression(
			behaving_object::GlobalObject(),
			dur_value);
	    if(!dur_value.is_duration()) {
		aafReader::pop_current_task();
		Cstring errs;
		errs << "File " << file << ", line " << line << ": constraint "
			<< name << " has an invalid duration expression.";
		throw(eval_error(errs));
	    }
	    maximum_duration = dur_value.get_time_or_duration();

	    //
	    // Create the wrapper in the Constraint subsystem
	    //
	    Constraint::allConstraints() << new MaximumDuration(name, this, Constructor);

	    //
	    // To be continued in consolidate_round_two().
	    //

	}
    } else {
	Cstring errs;
	aafReader::pop_current_task();
	errs << "File " << file << ", line " << line << ": constraint "
		<< tok_id_1->to_string() << " appears to be malformed; its body "
		<< "has type " << constraint_body->spell() << " while type "
		<< "PassiveConsItem was expected.";
	throw(eval_error(errs));
    }
    aafReader::pop_current_task();
}

void PassiveCons::consolidate_round_two(Constraint* constraint_ptr) {
    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << aafReader::make_space(4)
		<< "consolidating PassiveCons (round two) "
		<< getData() << "\n";
    }

    Cstring name = tok_id_1->to_string();
    Cstring type = tok_id->to_string();

    PassiveConsItem* item = dynamic_cast<PassiveConsItem*>(constraint_body.object());
    if(item) {

	// store it. The rationale for such a wrapper is that we want
	// to decouple the details of constraint handling from the
	// tasks of constraint parsing and consolidation.
	//

	if(type == "forbidden_condition") {

	    //
	    // We pick up where we left off at the first consolidation
	    //
	    vector<parsedExp>& cond_stuff = item->constraint_item->expressions;
	    condition = cond_stuff[0];

	    //
	    // Do a recursive search for all the resources
	    // involved in the constraint definition
	    //
	    currentval_finder CF;

	    //
	    // Identify all the high-level calls to currentval()
	    //
	    condition->recursively_apply(CF);

	    //
	    // Fix for ECAP-1535
	    //
	    theMessage->recursively_apply(CF);

	    stringslist::iterator	iter(CF.container_names);
	    emptySymbol*		s;

	    //
	    // For each container, we need to insert the resources
	    // it contains into the list of resources subject to
	    // constraints
	    //
	    while((s = iter())) {
		RCsource* container = RCsource::resource_containers().find(
						s->get_key());
		container->add_constraint(constraint_ptr);

		//
		// Now for each one of these containers, let's
		// add the containers it depends on e. g. via
		// its profile
		//
		slist<alpha_void, Cnode0<alpha_void, RCsource*> >::iterator it2(
				container->payload->ptrs_to_containers_used_in_profile);
		Cnode0<alpha_void, RCsource*>* ptr;

		while((ptr = it2())) {
		    RCsource* dependency = ptr->payload;

		    //
		    // add_constraint tolerates multiple calls, in
		    // case a container is a dependency for multiple
		    // resources invoked by the constraint
		    //
		    dependency->add_constraint(constraint_ptr);
		}
	    }
	} else if(type == "maximum_duration") {
	    vector<parsedExp>& cond_stuff = item->constraint_item->expressions;

	    //
	    // First item is the expression that
	    // implements the condition
	    //
	    condition = cond_stuff[0];

	    currentval_finder CF;

	    condition->recursively_apply(CF);

	    //
	    // Fix for ECAP-1535
	    //
	    theMessage->recursively_apply(CF);

	    stringslist::iterator	iter(CF.container_names);
	    emptySymbol*	s;

	    //
	    // For each container, we need to insert the resources
	    // it contains into the list of resources subject to
	    // constraints
	    //
	    while((s = iter())) {
		RCsource* container = RCsource::resource_containers().find(
							s->get_key());
		container->add_constraint(constraint_ptr);

		//
		// Now for each one of these containers, let's
		// add the containers it depends on e. g. via
		// its profile
		//
		slist<alpha_void, Cnode0<alpha_void, RCsource*> >::iterator it2(
				container->payload->ptrs_to_containers_used_in_profile);
		Cnode0<alpha_void, RCsource*>* ptr;

		while((ptr = it2())) {
		    RCsource* dependency = ptr->payload;

		    //
		    // add_constraint tolerates multiple calls, in
		    // case a container is a dependency for multiple
		    // resources invoked by the constraint
		    //
		    dependency->add_constraint(constraint_ptr);
		}
	    }
	}
    } else {
	Cstring errs;
	errs << "File " << file << ", line " << line << ": constraint "
		<< tok_id_1->to_string() << " appears to be malformed; its body "
		<< "has type " << constraint_body->spell() << " while type "
		<< "PassiveConsItem was expected.";
	throw(eval_error(errs));
    }
}
