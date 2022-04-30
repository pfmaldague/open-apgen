#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/stat.h>
#include "aafReader.H"
#include "ActivityInstance.H"	// for dumb_actptr
#include "ACT_exec.H"
#include "apcoreWaiter.H"
#include "BehavingElement.H"
#include "fileReader.H"
#include "lex_intfc.H"
#include "RES_def.H"
#include "aafReader.H"

using namespace pEsys;

void Resource::consolidate_one_list_of_states(
		ExpressionList&	aList,
		tvslist&	list_of_states,
		int		dent) {
	vector<parsedExp>	args;

	aList.consolidate(args, dent + 2);
	for(int i = 0; i < args.size(); i++) {
		TypedValue val;
		args[i]->eval_expression(
			behaving_object::GlobalObject(),
			val);
		list_of_states << new TaggedValue(
					Cstring(i),
					val);
	}
}

void Resource::consolidate_one_profile(
		RCsource* container,
		ProfileList& profile_exp,
		tlist<alpha_time, Cnode0<alpha_time, parsedExp> >& profile_list,
		int dent) {
	CTime_base		time_tag(0, 0, false);
	vector<parsedExp>&	V = profile_exp.expressions;

	profile_list << new Cnode0<alpha_time, parsedExp>(
				time_tag,
				profile_exp.Expression);
	for(int i = 0; i < V.size(); i++) {

		//
		// Use modular arithmetic to catch the time stamps
		// and the profile values
		//
		int m = i % 4;
		if(m == 1) {
			TypedValue val;

			//
			// i is the index of the time stamp
			//
			V[i]->eval_expression(
				behaving_object::GlobalObject(),
				val);
			if(!val.is_time()) {
				Cstring err;
				err << "File " << file << ", line " << line
					<< ": Error while evaluating profile of resource "
					<< container->payload->Type.name << ": "
					<< V[i]->to_string() << " is not a time.";
				throw(eval_error(err));
			}
			time_tag = val.get_time_or_duration();

			//
			// (i + 2) is the index of the profile value
			//
			profile_list << new Cnode0<alpha_time, parsedExp>(
				time_tag,
				V[i + 2]);
		}
	}
}

//
// In this stage, we concentrate on the following:
//
// 	- handling basic type and data type information
//
// 	- handling indices of arrayed resources
//
// 	- defining Types and Tasks
//
// 	- defining parameters for the appropriate task(s)
//
// 	- defining attributes, including index-specific sections
//
// 	- defining appropriate variables like 'newval' so usage
// 	  instructions can be executed
//
void Resource::consolidate_declaration_phase(int dent) {
    ResourceInfo*	ri = dynamic_cast<ResourceInfo*>(resource_prefix.object());
    assert(ri);
    ResourceDef*	rd = dynamic_cast<ResourceDef*>(resource_def.object());
    assert(rd);
    bool		is_a_declaration = rd->ASCII_123;

    //
    // Resource declarations are no longer necessary
    //
    if(is_a_declaration) {
	return;
    }

    //
    // STEP 1: get resource name
    //

    Cstring resource_name = ri->tok_id->getData();

    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << aafReader::make_space(dent) << "consolidating Resource "
		<< resource_name;
    }

    //
    // STEP 2: get resource type
    //

    Cstring			resource_type;

    if(rd->abstract_resource_def_top_line) {

	//
	// ABSTRACT CASE
	//

	resource_type = rd->abstract_resource_def_top_line->getData();
	if(APcloptions::theCmdLineOptions().debug_grammar) {
	    cerr << " of type abstract\n";
	}

	aafReader::AbstractResourceCount++;

	int abs_res_index = Behavior::add_type(
				new Behavior(
					resource_name,
					"abstract resource", // realm
					NULL));
	Behavior& abs_res_type = *Behavior::ClassTypes()[abs_res_index];

	//
	// Need to implement parameters
	//
	ModelingSection* modeling = dynamic_cast<ModelingSection*>(
			rd->resource_usage_section.object());
	if(modeling) {

	    //
	    // add "resource usage" or "modeling" task to behavior
	    //
	    Cstring task_name = modeling->resource_usage_header->getData();
	    if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent + 2) << "modeling section: "
			<< task_name << "\n";
	    }

	    apgen::METHOD_TYPE mt = apgen::METHOD_TYPE::NONE;

	    task*	constructor = abs_res_type.tasks[0];

	    //
	    // Coordinate this carefully with the variable_name
	    // enum in BehavingElement.H and also with ActivityInstance.H:
	    //
	    constructor->add_variable("id", apgen::DATA_TYPE::STRING);
	    constructor->add_variable("type", apgen::DATA_TYPE::STRING);
	    constructor->add_variable("parent", apgen::DATA_TYPE::INSTANCE);
	    constructor->add_variable("start", apgen::DATA_TYPE::TIME);
	    constructor->add_variable("finish", apgen::DATA_TYPE::TIME);
	    constructor->add_variable("subsystem", apgen::DATA_TYPE::STRING);

	    //
	    // Push constructor. Note: the constructor task
	    // was created automatically by the Behavior constructor.
	    // As of now, the constructor task has no variables.
	    // We should add any class variables to it.
	    //
	    aafReader::push_current_task(constructor);

	    Program* ModelingProgram = dynamic_cast<Program*>(modeling->program.object());
	    assert(ModelingProgram);
	    if(task_name == "modeling") {

		mt = apgen::METHOD_TYPE::MODELING;

	    } else if(task_name == "resource") {
		task_name = "resource usage";

		mt = apgen::METHOD_TYPE::RESUSAGE;

	    } else {
		cerr << "??????? strange modeling name " << task_name << "\n";
	    }


	    int k = abs_res_type.add_task(task_name);
	    task* curtask = abs_res_type.tasks[k];


	    //
	    // Push modeling task
	    //
	    aafReader::push_current_task(curtask);


	    //
	    // Consolidating parameters will automatically
	    // add the corresponding variables to the
	    // modeling / resource usage task
	    //
	    if(rd->res_parameter_section_or_null) {
		Parameters* params = dynamic_cast<Parameters*>(
				rd->res_parameter_section_or_null->copy());
		params->consolidate(dent + 2);
		delete params;
	    }

	    //
	    // Pop modeling task
	    //
	    aafReader::pop_current_task();

	    //
	    // Pop constructor
	    //
	    aafReader::pop_current_task();
	}

	//
	// Nothing else to do in the abstract case.
	//
	return;

	//
	// end of ABSTRACT CASE
	//
    }

    ConcreteResourceTypeEtc* crte = dynamic_cast<ConcreteResourceTypeEtc*>(
		rd->concrete_resource_def_top_line.object());
    assert(crte);

    ResourceType* rt = dynamic_cast<ResourceType*>(crte->concrete_resource_type_etc.object());
    assert(rt);

 			//
			// Pick an arbitrary default
			//
    apgen::RES_CLASS	resClass = apgen::RES_CLASS::CONSUMABLE;
    Cstring		integrand;
    apgen::DATA_TYPE	res_data_type = apgen::DATA_TYPE::UNINITIALIZED;
    if(crte->resource_data_type) {
	DataType* data_type = dynamic_cast<DataType*>(crte->resource_data_type.object());
	assert(data_type);
	data_type->consolidate(dent + 2);
	res_data_type = data_type->val.declared_type;
    }

    if(rt->tok_consumable) {

	aafReader::ConcreteResourceCount++;

	resource_type = rt->tok_consumable->getData();
	resClass = apgen::RES_CLASS::CONSUMABLE;
    } else if(rt->tok_nonconsumable) {

	aafReader::ConcreteResourceCount++;

	resource_type = rt->tok_nonconsumable->getData();
	resClass = apgen::RES_CLASS::NONCONSUMABLE;
    } else if(rt->tok_settable) {

	aafReader::ConcreteResourceCount++;

	resource_type = rt->tok_settable->getData();
	resClass = apgen::RES_CLASS::SETTABLE;
    } else if(rt->tok_state) {

	aafReader::ConcreteResourceCount++;

	resource_type = rt->tok_state->getData();
	resClass = apgen::RES_CLASS::STATE;
    } else if(rt->tok_extern) {

	aafReader::ConcreteResourceCount++;

	resource_type = rt->tok_extern->getData();
	resClass = apgen::RES_CLASS::EXTERNAL;
    }
    if(	resource_type == "consumable"
	|| resource_type == "nonconsumable"
	|| resource_type == "state"
	|| resource_type == "settable") {

        //
        // We will have to remove container from its list if any errors
        // are found during construction
        //

        RCsource*		container = NULL;

        try {

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << " of type " << resource_type << " "
			<< apgen::spell(res_data_type) << "\n";
	}

	vector<map<Cstring, int> >	vector_of_maps;
	vector<vector<Cstring> >	vector_of_vectors;
	int*				index_sizes = NULL;

	container = new RCsource(resource_name, NULL);

	//
	// check the data type
	//

	bool data_type_is_OK = false;

	switch(res_data_type) {
	    case apgen::DATA_TYPE::FLOATING:
		if(resource_type == "consumable"
		   || resource_type == "nonconsumable"
		   || resource_type == "settable") {
			data_type_is_OK = true;
		}
		break;
	    case apgen::DATA_TYPE::INTEGER:
	    case apgen::DATA_TYPE::DURATION:
	    case apgen::DATA_TYPE::TIME:
		if(resource_type == "consumable"
		   || resource_type == "state"
		   || resource_type == "nonconsumable"
		   || resource_type == "settable") {
			data_type_is_OK = true;
		}
		break;
	    case apgen::DATA_TYPE::STRING:
	    case apgen::DATA_TYPE::BOOL_TYPE:
		if(resource_type == "state"
		   || resource_type == "settable") {
			data_type_is_OK = true;
		}
		break;
	    default:
		break;
	}
	if(!data_type_is_OK) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": resource of type "
		<< resource_type << " should not have data type "
		<< apgen::spell(res_data_type);
	    throw(eval_error(errs));
	}


	//
	// Create the Type for the resource container
	//
	Behavior* container_beh = new Behavior(
					resource_name,
					"resource container",	// realm
					NULL);
	int		container_index = Behavior::add_type(container_beh);
	Behavior*	container_type = Behavior::ClassTypes()[container_index];
	apgen::RES_CONTAINER_TYPE simple_or_array = apgen::RES_CONTAINER_TYPE::SIMPLE;

	RCsource::resource_containers() << container;

	//
	//	DEAL WITH INDICES (or lack of them)
	//	-----------------------------------
	//

	if(!ri->resource_arrays) {

	    //
	    // simple resource
	    //
	    simple_or_array = apgen::RES_CONTAINER_TYPE::SIMPLE;

	    container_type->add_resource_subtypes(
					resource_name,
					"resource",	// sub-realm
					0,
					index_sizes,
					vector_of_vectors,
					vector_of_maps);
	    smart_ptr<res_container_object> ptr_to_container;
	    Rcontainer*		rc = new Rcontainer(
					ptr_to_container,
					*container_type,
					resClass,
					simple_or_array,
					res_data_type);
	    container->payload = rc;
	    rc->set_source_to(container);

	    //
	    // Cannot do this here - attributes are missing,
	    // object would be incomplete
	    //

	    // container->payload->Object.reference(
	    //     new res_container_object(container, integrand));

	    simple_or_array = apgen::RES_CONTAINER_TYPE::SIMPLE;

	    //
	    // single resource - already handled in declaration
	    // pass
	    //

	} else {

	    //
	    // arrayed resource
	    //
	    simple_or_array = apgen::RES_CONTAINER_TYPE::ARRAY;

	    Array*	anArray = dynamic_cast<Array*>(ri->resource_arrays.object());
	    ArrayList* anArrayList = dynamic_cast<ArrayList*>(ri->resource_arrays.object());

	    if(anArray) {

		//
		// single array of elements, hopefully
		// strings, enclosed in square brackets
		//
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent) << "found simple array\n";
		}
		anArray->consolidate(dent + 2);
		if(!anArray->is_list) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": the array used in an arrayed resource definition should be a list";
			throw(eval_error(err));
		}

		index_sizes = (int*) malloc(sizeof(int));
		index_sizes[0] = anArray->actual_values.size();

		vector_of_vectors.push_back(vector<Cstring>());
		vector_of_maps.push_back(map<Cstring, int>());
		vector<Cstring>& stringvec = vector_of_vectors[0];
		map<Cstring, int>& stringmap = vector_of_maps[0];
		for(int i = 0; i < anArray->actual_values.size(); i++) {
			apgen::DATA_TYPE the_item_type = anArray->actual_values[i]->get_result_type();
			if(the_item_type != apgen::DATA_TYPE::STRING) {
				Cstring err;
				err << "File " << file << ", " << line
					<< ": item " << (i+1) << " in list of indices for "
					"resource " << resource_name << " appears to be a(n) "
					<< apgen::spell(the_item_type) << "; expected a string";
				throw(eval_error(err));
			}
			TypedValue theString;
			anArray->actual_values[i]->eval_expression(
					behaving_object::GlobalObject(),
					theString);
			if(!theString.is_string()) {
				Cstring err;
				err << "File " << file << ", " << line
					<< ": item " << (i+1) << " in list of indices for "
					"resource " << resource_name << " is a "
					<< apgen::spell(theString.get_type()) << "; expected a string";
				throw(eval_error(err));
			}
			stringmap[theString.get_string()] = stringvec.size();
			stringvec.push_back(theString.get_string());
		}
		container_type->add_resource_subtypes(
					resource_name,
					"resource",	// sub-realm
					1,
					index_sizes,
					vector_of_vectors,
					vector_of_maps);
	    } else if(anArrayList) {

		//
		// single or multiple lists, comma-separated,
		// enclosed in parentheses
		//
		if(APcloptions::theCmdLineOptions().debug_grammar) {
		    cerr << aafReader::make_space(dent) << "found multiple arrays, N = "
			<< anArrayList->evaluated_elements.size() << "\n";
		}
		anArrayList->consolidate(dent + 2, resource_name);

		index_sizes = (int*) malloc(sizeof(int) * anArrayList->evaluated_elements.size());

		//
		// anArrayList->evaluated_elements is a vector of ListOVal objects
		//
		for(	int list_index = 0;
			list_index < anArrayList->evaluated_elements.size();
			list_index++) {

		    TypedValue& theList = anArrayList->evaluated_elements[list_index];
		    if(!theList.is_array()) {
			Cstring err;
			err << "File " << file << ", " << line
				<< ": item " << (list_index+1) << " in list of arrays for "
				"resource " << resource_name << " is a(n) "
				<< apgen::spell(theList.get_type()) << "; expected a list";
			throw(eval_error(err));
		    }
		    if(theList.get_array().get_array_type()
				!= TypedValue::arrayType::LIST_STYLE) {
			Cstring err;
			err << "File " << file << ", " << line
				<< ": item " << (list_index+1) << " in list of arrays for "
				"resource " << resource_name << " is empty or a struct, not a list";
			throw(eval_error(err));
		    }
		    ListOVal&		lov = theList.get_array();

		    index_sizes[list_index] = lov.get_length();
		    vector_of_vectors.push_back(vector<Cstring>());
		    vector_of_maps.push_back(map<Cstring, int>());

		    vector<Cstring>&	stringvec = vector_of_vectors[list_index];
		    map<Cstring, int>&	stringmap = vector_of_maps[list_index];

		    //
		    // iterate over elements of the list
		    //

		    for(int j = 0; j < lov.get_length(); j++) {
			if(!lov[j]->Val().is_string()) {
				Cstring err;
				err << "File " << file << ", " << line
					<< ": item " << (j+1) << " in " << list_index
					<< "-th list of indices for resource "
					<< resource_name << " is a "
					<< apgen::spell(lov[j]->Val().get_type())
					<< "; expected a string";
				throw(eval_error(err));
			}
			stringmap[lov[j]->Val().get_string()] = stringvec.size();
			stringvec.push_back(lov[j]->Val().get_string());
		    }
		}
		container_type->add_resource_subtypes(
					resource_name,
					"resource",	// sub-realm
					anArrayList->evaluated_elements.size(),
					index_sizes,
					vector_of_vectors,
					vector_of_maps);
	    } else {
		assert(false);
	    }
	}
	if(index_sizes) {
	    free((char*) index_sizes);
	}

	//
	//	DEAL WITH ATTRIBUTES (or lack of them)
	//	--------------------------------------
	//

	//
	// We will be computing an array of
	// booleans for the 'properties' member of the
	// Rsource class; this array will be used later,
	// when we create the res_container_object.
	//
	vector<vector<bool> > res_properties(container_type->SubclassTypes.size());

	//
	// We need to initialize properties even if no attributes are defined
	//
	for(int simple_res = 0; simple_res < container_type->SubclassTypes.size(); simple_res++) {
	    vector<bool>&	property_array = res_properties[simple_res];
	    int num_props = (int)Rsource::Property::has_modeling_only;
	    property_array.resize(num_props);
	    for(int zp = 0; zp < num_props; zp++) {
		property_array[zp] = false;
	    }
	}
	

	if(rd->opt_attributes_section_res) {

	    //
	    // attributes
	    //
	    map<Cstring, int>&	flat_map = container_type->SubclassFlatMap;

	    //
	    // for a given full label, provides the index of the
	    // labeled program attached to it:
	    //
	    map<Cstring, int>	map_of_labels;

	    int			default_index = -1;

	    //
	    // for a given attribute name, provides the set of
	    // indices of labeled programs that list this
	    // attribute:
	    //
	    map<Cstring, vector<smart_ptr<executableExp> > > all_attrs;

	    Program*	attributes_program = dynamic_cast<Program*>(
				rd->opt_attributes_section_res->expressions[0].object());
	    LabeledProgram*		labeled_prog = dynamic_cast<LabeledProgram*>(
				rd->opt_attributes_section_res->expressions[0].object());
	    if(attributes_program) {
		if(APcloptions::theCmdLineOptions().debug_grammar) {
		    cerr << aafReader::make_space(dent)
			 << "consolidating attributes of resource "
			 << resource_name
			 << " - found a single attribute program\n";
		}
		default_index = 0;
		for(int j = 0; j < attributes_program->statements.size(); j++) {
		    Assignment* assign = dynamic_cast<Assignment*>(
					attributes_program->statements[j].object());
		    assert(assign);
		    Constant* lhs = dynamic_cast<Constant*>(
					assign->lhs_of_assign.object());
		    if(!lhs) {
			Cstring err;
			err << "File " << attributes_program->file << ", line "
				<< attributes_program->line << ": attribute "
				<< assign->lhs_of_assign->to_string()
				<< " should be written as a double-quoted string";
			throw(eval_error(err));
		    }
		    Cstring attr_name = lhs->getData();
		    map<Cstring, vector<smart_ptr<executableExp> > >::const_iterator iter
						= all_attrs.find(attr_name);
		    if(iter == all_attrs.end()) {
			vector<smart_ptr<executableExp> > M(1);
			all_attrs[attr_name] = M;
		    }

		    //
		    // state that this attribute has a value for this label:
		    //
		    all_attrs[attr_name][0] = attributes_program->statements[j];
		}
	    } else if(labeled_prog) {
		if(APcloptions::theCmdLineOptions().debug_grammar) {
			cerr << aafReader::make_space(dent) << "consolidating attributes of resource "
				<< resource_name << " - found labeled attribute program(s)\n";
		}

		//
		// first we reorganize the LabeledProgram's
		// data into two vectors, one for labels and
		// one for attribute declarations
		//

		vector<Cstring>	labels;
		vector<smart_ptr<Program> > programs;
		Cstring aLabel = labeled_prog->when_case_label->expressions[0]->to_string();
		if(aLabel[0] == '"') {
			removeQuotes(aLabel);
		}
		labels.push_back(aLabel);
		Program* prog = dynamic_cast<Program*>(labeled_prog->declarative_program.object());
		programs.push_back(smart_ptr<Program>(prog));
		for(int i = 0; i < labeled_prog->expressions.size(); i++) {
		    LabeledProgram* lp2
			= dynamic_cast<LabeledProgram*>(labeled_prog->expressions[i].object());
		    aLabel = lp2->when_case_label->expressions[0]->to_string();
		    if(aLabel[0] == '"') {
			removeQuotes(aLabel);
		    }
		    labels.push_back(aLabel);
		    prog = dynamic_cast<Program*>(lp2->declarative_program.object());
		    programs.push_back(smart_ptr<Program>(prog));
		}

		//
		// collect all labels
		//

		for(int i = 0; i < labels.size(); i++) {
		    Cstring one_label = labels[i];

		    if(one_label == "default") {
			default_index = i;
		    } else {
			Cstring full_label(container_type->name);
			full_label << one_label;
			map<Cstring, int>::const_iterator iter = flat_map.find(full_label);
			if(iter == flat_map.end()) {
			    Cstring err;
			    err << "File " << labeled_prog->file << ", line "
				<< labeled_prog->line << ": label "
				<< full_label << " not found in arrayed resource "
				<< resource_name;
			    throw(eval_error(err));
			}
			map_of_labels[full_label] = i;
		    }

		    //
		    // now look for corresponding
		    // assignments:
		    //

		    for(int j = 0; j < programs[i]->statements.size(); j++) {
			Assignment* assign = dynamic_cast<Assignment*>(
				programs[i]->statements[j].object());
			assert(assign);
			Constant* lhs = dynamic_cast<Constant*>(
				assign->lhs_of_assign.object());
			if(!lhs) {
			    Cstring err;
			    err << "File " << labeled_prog->file << ", line "
				<< labeled_prog->line << ": attribute "
				<< assign->lhs_of_assign->to_string()
				<< " should be written as a double-quoted string";
			    throw(eval_error(err));
			}
			Cstring attr_name = lhs->getData();
			map<Cstring, vector<smart_ptr<executableExp> > >::const_iterator iter
				= all_attrs.find(attr_name);
			if(iter == all_attrs.end()) {
			    vector<smart_ptr<executableExp> > M(labels.size());
			    all_attrs[attr_name] = M;
			}
			// state that this attribute has a value for this label:
			all_attrs[attr_name][i] = programs[i]->statements[j];
		    }
		}
		if(default_index == -1 && map_of_labels.size() != container_type->SubclassTypes.size()) {
		    if(map_of_labels.size() != container_type->SubclassTypes.size()) {
			Cstring err;
			err << "File " << labeled_prog->file << ", line "
				<< labeled_prog->line << ": resource "
				<< resource_name << " has " << container_type->SubclassTypes.size()
				<< " single resources, but no default list of attributes is "
				"provided and only " << map_of_labels.size()
				<< " list(s) of attributes are listed in the attributes section.";
			throw(eval_error(err));
		    }
		}
	    }

	    //
	    // We can now add the relevant attributes to each
	    // resource constructor.
	    //

	    for(int simple_res = 0; simple_res < container_type->SubclassTypes.size(); simple_res++) {
		Behavior*			beh = container_type->SubclassTypes[simple_res];
		map<Cstring, int>::iterator	iter = map_of_labels.find(beh->name);

		//
		// 'how_many_properties' is the number of items in the
		// Rsource::Property::which_property enum. Note that property_array
		// has already been initialized before the check on whether
		// the resource has any attributes defined.
		//
		vector<bool>&	property_array = res_properties[simple_res];

		int				index_to_use = 0;
		if(iter == map_of_labels.end()) {
		    index_to_use = default_index;
		} else {
		    index_to_use = iter->second;
		}

		//
		// build a custom list of assignments for this resource
		// 

		map<Cstring, vector<smart_ptr<executableExp> > >::iterator iter2;

		parsedExp		theAttrProgram;
		Program*		attrProgObject = NULL;
		stringtlist		AttrsAlreadyDefined;

		for(iter2 = all_attrs.begin(); iter2 != all_attrs.end(); iter2++) {
		    Cstring attr_name = iter2->first;

		    if(iter2->second[index_to_use]) {
			if(APcloptions::theCmdLineOptions().debug_grammar) {
				cerr << aafReader::make_space(dent) << "\tattribute "
					<< attr_name << " is defined for single resource "
					<< beh->name << "\n";
			}
			AttrsAlreadyDefined << new emptySymbol(attr_name);
			if(!theAttrProgram) {
				parsedExp first_assignment(iter2->second[index_to_use]->copy());
				theAttrProgram.reference(attrProgObject = new Program(
					0, first_assignment, Differentiator_0(0)));
			} else {
				theAttrProgram->addExp(
					parsedExp(
						iter2->second[index_to_use]->copy())
					);
			}
		    }
		}

		if(	default_index != -1			// a default has been provided
			&& index_to_use != default_index	// we are processing a non-default label
		  ) {

		    //
		    // this resource has a non-default label, but it also inherits
		    // all default attributes that haven't been overridden in its
		    // labeled section:
		    //

		    for(iter2 = all_attrs.begin(); iter2 != all_attrs.end(); iter2++) {
			Cstring attr_name = iter2->first;

			if(iter2->second[default_index] && !AttrsAlreadyDefined.find(attr_name)) {
				if(APcloptions::theCmdLineOptions().debug_grammar) {
					cerr << aafReader::make_space(dent) << "\tattribute " << attr_name
					<< " is defined for single resource " << beh->name << "\n";
				}
				theAttrProgram->addExp(parsedExp(iter2->second[default_index]->copy()));
			}
		    }
		}
		assert(!beh->tasks[0]->prog);
    

		//
		// We need to consolidate the attribute
		// program. In 'normal' consolidation, any
		// symbols (e. g. the nickname of the string
		// in the lhs of the assignment) should be
		// present in the symbol table of the current
		// task or in the symbol table of one of the
		// tasks above it.
		//
		// But in the case of resources, we only
		// define task variables for the symbols that
		// are actually defined in the attributes
		// section. In order to do that, we need to
		// know the type of each lhs variable. In the
		// case of resources, that is a little tricky
		// because symbols like "Minimum" and
		// "Maximum" take on the type of the resource,
		// which must therefore be taken into account.
		//
		// In a first pass through the attributes, we
		// will extract the nickname of the lhs, make
		// sure it's listed in the
		// nickname_to_resource_data_type array, and
		// use that to figure out the nickname type,
		// which will then be used to add a variable
		// to the appropriate resource Behavior.
		//
		// NOTE: it is possible for theAttrProgram to
		// be empty.
		//

		aafReader::push_current_task(beh->tasks[0]);

		if(theAttrProgram) {
		    for(int k = 0; k < attrProgObject->statements.size(); k++) {

			Assignment* assign = dynamic_cast<Assignment*>(
					attrProgObject->statements[k].object());
			assert(assign);
			Constant* lhs = dynamic_cast<Constant*>(
				assign->lhs_of_assign.object());

			//
			// we already checked:
			//
			assert(lhs);

			map<Cstring, pair<Cstring, int> >::iterator iter3
				= aafReader::string_to_nickname().find(lhs->getData());
			if(iter3 == aafReader::string_to_nickname().end()
			   || iter3->second.second == 0) {
				Cstring err;
				err << "File " << assign->file << ", line " << assign->line
					<< ": attribute " << lhs->getData()
					<< " is not appropriate for a resource.";
				throw(eval_error(err));
			}
			Cstring nickname = iter3->second.first;

			//
			// At this point, we can certify that the current resource
			// has the attribute identified by 'nickname'.
			//
			// Furthermore, the resource has no other attributes than
			// the ones identified here. Therefore, this is a good
			// place to fill in the table of Booleans in the Rsource
			// class. However, we have not yet defined the Rsource
			// objects; that will take place when we create a
			// res_container_object below. So, in the meantime we
			// create an array of boolean values to be used later: 
			//
			// In addition, the meaning of the Boolean array is that
			// the corresponding variable not only exists, but that
			// its value (if it is a Boolean) is true. We won't know
			// that until the implementation phase of consolidation;
			// until then, the assignments below are only tentative.
			//
			if(nickname == "interpolation") {
			    property_array[(int)Rsource::Property::has_interpolation] = true;
			}
			if(nickname == "hidden") {
			    property_array[(int)Rsource::Property::has_hidden] = true;
			}
			if(nickname == "subsystem") {
			    property_array[(int)Rsource::Property::has_subsystem] = true;
			}
			if(nickname == "min_abs_delta") {
			    property_array[(int)Rsource::Property::has_min_abs_delta] = true;
			}
			if(nickname == "min_rel_delta") {
			    property_array[(int)Rsource::Property::has_min_rel_delta] = true;
			}
			if(nickname == "maximum") {
			    property_array[(int)Rsource::Property::has_maximum] = true;
			}
			if(nickname == "minimum") {
			    property_array[(int)Rsource::Property::has_minimum] = true;
			}
			if(nickname == "nofiltering") {
			    property_array[(int)Rsource::Property::has_nofiltering] = true;
			}
			if(nickname == "modeling_only") {
			    property_array[(int)Rsource::Property::has_modeling_only] = true;
			}

			//
			// NOTE: As of APGenX version X.4.04, action_programs
			//       have not been implemented yet
			//
			property_array[(int)Rsource::Property::has_oldval] = false;
			property_array[(int)Rsource::Property::has_newval] = false;

			map<Cstring, apgen::DATA_TYPE (*)(apgen::DATA_TYPE)>::iterator iter4
				= aafReader::nickname_to_resource_data_type().find(nickname);
			assert(iter4 != aafReader::nickname_to_resource_data_type().end());
			apgen::DATA_TYPE dt = iter4->second(res_data_type);
			beh->tasks[0]->add_variable(nickname, dt);
		    }
		    attrProgObject->orig_section = apgen::METHOD_TYPE::ATTRIBUTES;
		    aafReader::consolidate_program(
				    *attrProgObject,
				    dent + 2);
		    aafReader::get_current_task()->prog.reference(attrProgObject);
		}
		aafReader::pop_current_task();
	    }
	}

	//
	//	DEAL WITH USAGE EXPRESSION (or lack of it)
	//	------------------------------------------
	//


	UsageSection* usage_section = dynamic_cast<UsageSection*>(
		rd->usage_section_or_null.object());

	//
	// For each individual resource in the array, create a
	// copy of the parameters section, and of the usage
	// section if it exists
	//
	for(int i = 0; i < container_type->SubclassTypes.size(); i++) {
	    Behavior*	one_res = container_type->SubclassTypes[i];

	    //
	    // push the constructor task
	    //
	    aafReader::push_current_task(one_res->tasks[0]);

	    if(resource_type != "settable") {

		int index_of_use_task = one_res->add_task("use");
		task* use_task = one_res->tasks[index_of_use_task];

		//
		// push the use task
		//
		aafReader::push_current_task(use_task);


		if(rd->res_parameter_section_or_null) {
			Parameters* params = dynamic_cast<Parameters*>(
					rd->res_parameter_section_or_null->copy());
			params->consolidate(dent + 2);
			delete params;
		}


		if(usage_section) {

			//
			// We create a new variable,
			// 'consumption', to which we assign
			// the value specified in the usage
			// Expression; this then constitutes a
			// one-statement program which we can
			// exercise at runtime.
			//

			//
			// Pave the way for an assignment later on
			//
			if(	(resource_type == "consumable"
				|| resource_type == "nonconsumable")
				&& res_data_type == apgen::DATA_TYPE::TIME) {
				use_task->add_variable("consumption", apgen::DATA_TYPE::DURATION);
			} else {
				use_task->add_variable("consumption", res_data_type);
			}
		}


		//
		// pop the use task
		//
		aafReader::pop_current_task();

	    } else {

		//
		// resource_type is "settable"
		//
		int index_of_set_task = one_res->add_task("set");
		task* set_task = one_res->tasks[index_of_set_task];

		//
		// Check whether the resource is precomputed
		//
		ExpressionListPrecomp* precomp_exp_list = NULL;
		if(rd->opt_time_series) {
		    precomp_exp_list = dynamic_cast<ExpressionListPrecomp*>(
						rd->opt_time_series.object());
		}

		if(precomp_exp_list) {
		    int index_of_evaluate_task = one_res->add_task("evaluate");

		    //
		    // Requirements change: evaluate() takes no parameters
		    //
		    // task* evaluate_task = one_res->tasks[index_of_evaluate_task];
		    // int var_index = evaluate_task->add_variable(
		    // 		"timeval",
		    // 		apgen::DATA_TYPE::TIME);
		    // evaluate_task->paramindex.push_back(var_index);
		}

		//
		// Make sure the Usage Section was defined
		//
		if(!usage_section) {
		    Cstring errs;
		    errs << "File " << file << ", line " << line << ": resource of type "
			 << "\"settable\" should have a usage section.";
		    throw(eval_error(errs));
		}

		//
		// push the 'set' task
		//
		aafReader::push_current_task(set_task);

		if(rd->res_parameter_section_or_null) {
		    Parameters* params = dynamic_cast<Parameters*>(
				rd->res_parameter_section_or_null->copy());
		    params->consolidate(dent + 2);
		    delete params;
		}

		//
		// Pave the way for an assignment later on
		//
		int set_value_index = set_task->add_variable("set_value", res_data_type);

		//
		// set_value is NOT the only variable besides 'enabled',
		// because we have just consolidated the parameters
		// which have been added to the symbol table.
		//
		// assert(set_value_index == 1);

		//
		// pop the 'set' task
		//
		aafReader::pop_current_task();
	    }

	    //
	    // For state resources, we need to add set and reset
	    // tasks
	    //
	    if(resource_type == "state") {
		int index_of_set_task = one_res->add_task("set");
		task* set_task = one_res->tasks[index_of_set_task];

		aafReader::push_current_task(set_task);

		//
		// what do we do here? We must mimic whatever is done
		// by Parameters to provide room for the evaluated
		// argument of 'set' clauses (actually, this is not
		// used but it doesn't hurt)
		//
		int set_value_index = set_task->add_variable("set_value", res_data_type);

		//
		// should be the only variable besides
		// 'enabled':
		//
		assert(set_value_index == 1);

		aafReader::pop_current_task();

		int index_of_reset_task = one_res->add_task("reset");
		task* reset_task = one_res->tasks[index_of_reset_task];

		// aafReader::push_current_task(reset_task);

		//
		// what do we do here? Hopefully we can
		// execute the task without problems
		//

		// aafReader::pop_current_task();

	    }

	    //
	    // pop the constructor task
	    //
	    aafReader::pop_current_task();

	}

	//
	// Establish the bridge between the Behavior subsystem and the
	// resource subsystem
	//

	//
	// Unfortunately, a few manual steps are required to establish
	// 2-way pointers between the various representations of a 
	// resource container...
	//

	if(simple_or_array == apgen::RES_CONTAINER_TYPE::ARRAY) {
	    smart_ptr<res_container_object> ptr_to_container;
	    Rcontainer*		rc = new Rcontainer(
					ptr_to_container,
					*container_type,
					resClass,
					simple_or_array,
					res_data_type);
	    container->payload = rc;
	    rc->set_source_to(container);

	    container->payload->Object.reference(
			new res_container_object(container, integrand));
	} else {

	    //
	    // Already handled above - except for
	    // for defining the Object so it includes attributes:
	    //
	    container->payload->Object.reference(
			new res_container_object(container, integrand));
	}

	//
	// Now we can fill the properties array of booleans for
	// each Rsource object.
	//
        vector<Rsource*>& simple_resources
			= container->payload->Object->array_elements;
	for(int s = 0; s < simple_resources.size(); s++) {

	    //
	    // Watch out - there may not be any attributes, in which case
	    // the booleans in res_properties were never set.
	    //
	    // Note that the assignments in the loop below are tentative.
	    // All we have checked is that the variable whose name is
	    // associated with an item in Rsource::Property::properties has been
	    // defined. Its value will not be known until the implementation
	    // phase, below.
	    //
	    if(res_properties[s].size() > 0) {
		for( int p = 0; p < 10; p++) {
		    simple_resources[s]->properties[p] = res_properties[s][p];
		}
	    }
	}
        } catch(eval_error Err) {
	    RCsource::resource_containers().remove_node(container);

	    //
	    // at the cost of memory leaks, do not try to delete the
	    // half-baked container node - it may not be deletable
	    //
	    throw(Err);
        }
    }
}

//
// In the implementation phase, we focus on the following:
//
// 	- modeling / resource usage sections of abstract resources
//
// 	- profile, states, default sections
//
// 	- make sure the index-specific sections of arrayed resources
// 	  are handled correctly
//
void Resource::consolidate_implementation_phase(int dent) {
    ResourceInfo*	ri = dynamic_cast<ResourceInfo*>(resource_prefix.object());
    assert(ri);
    ResourceDef*	rd = dynamic_cast<ResourceDef*>(resource_def.object());
    assert(ri);
    bool		is_a_declaration = rd->ASCII_123;

    //
    // Resource declarations are no longer necessary
    // in the implementation phase
    //
    if(is_a_declaration) {
	return;
    }

    //
    // STEP 1: get resource name
    //

    Cstring resource_name = ri->tok_id->getData();

    if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Resource "
			<< resource_name;
    }

    //
    // STEP 2: get resource type
    //
    Cstring		resource_type;

    if(rd->abstract_resource_def_top_line) {

	//
	// ABSTRACT RESOURCE
	//
	resource_type = rd->abstract_resource_def_top_line->getData();

	map<Cstring, int>::const_iterator iter
			= Behavior::ClassIndices().find(resource_name);
	int abs_res_index = iter->second;
	Behavior& abs_res_type = *Behavior::ClassTypes()[abs_res_index];

	ModelingSection* modeling = dynamic_cast<ModelingSection*>(
			rd->resource_usage_section.object());

	if(modeling) {

	    //
	    // add "resource usage" or "modeling" task to behavior
	    //
	    Cstring task_name = modeling->resource_usage_header->getData();

	    if(task_name == "resource") {
		task_name = "resource usage";
	    } else if(task_name == "modeling") {
		;
	    } else {
		cerr << "??????? strange modeling name " << task_name << "\n";
	    }

	    if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent + 2) << "modeling section: "
			<< task_name << "\n";
	    }

	    apgen::METHOD_TYPE mt = apgen::METHOD_TYPE::NONE;

	    map<Cstring, int>::iterator iter = abs_res_type.taskindex.find(task_name);

	    //
	    // The modeling task was added in the declaration
	    // pass. Check:
	    //
	    assert(iter != abs_res_type.taskindex.end());

	    int k = iter->second;
	    task* curtask = abs_res_type.tasks[k];

	    //
	    // Push constructor
	    //
	    aafReader::push_current_task(abs_res_type.tasks[0]);

	    Program* ModelingProgram = dynamic_cast<Program*>(modeling->program.object());
	    assert(ModelingProgram);
	    if(task_name == "modeling") {

		//
		// We need to add a wait for 0:0:0 statement at the
		// top of the program
		//
		mt = apgen::METHOD_TYPE::MODELING;

		//
		// We need to add a wait for 0:0:0 statement at the
		// top of the program
		//


		//
		// we emulate grammar.y in creating the new WaitFor
		// object
		//
		parsedExp	constant(new pE_w_string(origin_info(
							modeling->resource_usage_header->line,
							modeling->resource_usage_header->file),
							"0:0:0"));

		//
		// Watch out! Use the correct differentiator
		// for constant duration
		//
		parsedExp	DurationExp(new Constant(0, constant, Differentiator_4(0)));
		parsedExp	wait_token(new pE_w_string(origin_info(
							modeling->resource_usage_header->line,
							modeling->resource_usage_header->file),
							"wait"));
		parsedExp	for_token(new pE_w_string(origin_info(
							modeling->resource_usage_header->line,
							modeling->resource_usage_header->file),
							"for"));
		parsedExp	semicolon(new pE_w_string(origin_info(
							modeling->resource_usage_header->line,
							modeling->resource_usage_header->file),
							";"));
		WaitFor* wf = new WaitFor(2, DurationExp, wait_token, for_token, semicolon,
					Differentiator_0(0));
		vector<smart_ptr<executableExp> > statements_copy;
		statements_copy.push_back(smart_ptr<executableExp>(wf));
		for(int i = 0; i < ModelingProgram->statements.size(); i++) {
			statements_copy.push_back(ModelingProgram->statements[i]);
		}
		ModelingProgram->statements = statements_copy;
	    } else if(task_name == "resource usage") {
		mt = apgen::METHOD_TYPE::RESUSAGE;
	    } else {
		cerr << "??????? strange modeling name " << task_name << "\n";
	    }

	    ModelingProgram->orig_section = mt;

	    //
	    // The modeling task was defined in pass one. Now we can
	    // have it reference the modeling program:
	    //

	    curtask->prog.reference(ModelingProgram);

	    //
	    // Push modeling task so we can consolidate ModelingProgram
	    //
	    aafReader::push_current_task(curtask);

	    aafReader::consolidate_program(
			    *ModelingProgram,
			    dent + 2);

	    //
	    // Pop modeling task
	    //
	    aafReader::pop_current_task();

	    //
	    // Pop constructor so the stack is in its original state
	    //
	    aafReader::pop_current_task();

	    //
	    // Handle generation of resource-specific usage class,
	    // if appropriate.
	    //
	    // The various "generate" methods invoked here are defined in
	    // source file class_generation.C.
	    //
	    if(APcloptions::theCmdLineOptions().generate_cplusplus) {

		ResourceDef::generate_abstract_usage_factory(
				resource_name);

		ResourceDef::generate_abstract_usage_mini_factory(
				resource_name);

		ResourceDef::generate_abstract_behavior_header(
				abs_res_type);

		ResourceDef::generate_execute_method(
				abs_res_type);
	    }
	}

	//
	// Nothing else to do in the abstract case
	//
	return;
    }

    //
    // CONCRETE RESOURCE
    //
    ConcreteResourceTypeEtc* crte = dynamic_cast<ConcreteResourceTypeEtc*>(
			rd->concrete_resource_def_top_line.object());
    assert(crte);

    ResourceType* res_type = dynamic_cast<ResourceType*>(
				crte->concrete_resource_type_etc.object());
    assert(res_type);

    Cstring		integrand;
    apgen::DATA_TYPE	res_data_type = apgen::DATA_TYPE::UNINITIALIZED;
    if(crte->resource_data_type) {
	DataType* data_type = dynamic_cast<DataType*>(crte->resource_data_type.object());
	assert(data_type);

	// already done:
	// data_type->consolidate(dent + 2);
	res_data_type = data_type->val.declared_type;
    }

    if(res_type->tok_consumable) {

	aafReader::ConcreteResourceCount++;

	resource_type = res_type->tok_consumable->getData();
    } else if(res_type->tok_nonconsumable) {

	aafReader::ConcreteResourceCount++;

	resource_type = res_type->tok_nonconsumable->getData();
    } else if(res_type->tok_state) {

	aafReader::ConcreteResourceCount++;

	resource_type = res_type->tok_state->getData();
    } else if(res_type->tok_settable) {

	aafReader::ConcreteResourceCount++;

	resource_type = res_type->tok_settable->getData();
    } else if(res_type->tok_extern) {

	aafReader::ConcreteResourceCount++;

	resource_type = res_type->tok_extern->getData();
    }

    //
    // Here is where we do the bulk of the work for concrete resources:
    //
    if(	  resource_type == "consumable"
	  || resource_type == "nonconsumable"
	  || resource_type == "settable"
	  || resource_type == "state"
	  || resource_type == "extern") {
	 RCsource*	container = RCsource::resource_containers().find(resource_name);
	 assert(container);
	 try {

	    if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << " of type " << resource_type << " "
			<< apgen::spell(res_data_type) << "\n";
	    }

	    vector<map<Cstring, int> >	vector_of_maps;
	    vector<vector<Cstring> >	vector_of_vectors;
	    int*			index_sizes = NULL;

	    //
	    // check the data type
	    //
	    bool data_type_is_OK = false;

	    switch(res_data_type) {
		case apgen::DATA_TYPE::FLOATING:
			if(resource_type == "consumable"
			   || resource_type == "settable"
			   || resource_type == "nonconsumable") {
				data_type_is_OK = true;
			}
			break;
		case apgen::DATA_TYPE::INTEGER:
		case apgen::DATA_TYPE::DURATION:
		case apgen::DATA_TYPE::TIME:
			if(resource_type == "consumable"
			   || resource_type == "state"
			   || resource_type == "settable"
			   || resource_type == "nonconsumable") {
				data_type_is_OK = true;
			}
			break;
		case apgen::DATA_TYPE::STRING:
		case apgen::DATA_TYPE::BOOL_TYPE:
			if(resource_type == "state"
			   || resource_type == "settable") {
				data_type_is_OK = true;
			}
			break;
		default:
			break;
	    }
	    if(!data_type_is_OK) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": resource of type "
			<< resource_type << " should not have data type "
			<< apgen::spell(res_data_type);
		throw(eval_error(errs));
	    }


	    //
	    // Get the Type for the resource container
	    //
	    apgen::RES_CONTAINER_TYPE simple_or_array
				= apgen::RES_CONTAINER_TYPE::SIMPLE;
		
	    //
	    // Determine whether this is a single resource or an arrayed resource
	    //
	    if(!ri->resource_arrays) {
		simple_or_array = apgen::RES_CONTAINER_TYPE::SIMPLE;
	    } else {
		simple_or_array = apgen::RES_CONTAINER_TYPE::ARRAY;
	    }

	    int			container_index = -1;
	    Behavior*		container_type = NULL;

	    map<Cstring, int>::const_iterator iter
			    = Behavior::ClassIndices().find(resource_name);

	    container_index = iter->second;
	    container_type = Behavior::ClassTypes()[container_index];

	    vector<Rsource*>&	simple_resources
			    = container->payload->Object->array_elements;


	    //
	    // Handle states section ('state' resources only)
	    //
	    if(rd->opt_states_section) {
		if(resource_type != "state") {
		    Cstring err;
		    err << "File " << rd->opt_states_section->file << ", line "
			<< rd->opt_states_section->line
			<< ": a resource of type " << resource_type
			<< " is not supposed to have a states section.";
		    throw(eval_error(err));
		}
		map<Cstring, int>&	flat_map = container_type->SubclassFlatMap;
		vector<tvslist>		vector_of_state_lists;
		int			default_index = -1;
		map<Cstring, int>	map_of_labels;

		ExpressionList*		aList = dynamic_cast<ExpressionList*>(
					    rd->opt_states_section->expressions[0].object());
		LabeledStates*		states = dynamic_cast<LabeledStates*>(
					    rd->opt_states_section->expressions[0].object());
		if(aList) {
		    tvslist		list_of_states;
		    try {
			consolidate_one_list_of_states(*aList, list_of_states, dent + 2);
		    } catch(eval_error Err) {
			Cstring err;
			err << "File " << aList->file << ", line " << aList->line
				<< ": error evaluating states "
				<< "for resource " << resource_name
				<< "; details:\n" << Err.msg;
			throw(eval_error(err));
		    }
		    vector_of_state_lists.push_back(list_of_states);
		    default_index = 0;
		} else if(states) {

		    //
		    // first we reorganize the LabeledStates'
		    // data into two vectors, one for labels and
		    // one for expression lists
		    //

		    vector<Cstring>	labels;
		    vector<smart_ptr<ExpressionList> > state_lists;
		    Cstring aLabel = states->when_case_label->expressions[0]->to_string();
		    if(aLabel[0] == '"') {
				removeQuotes(aLabel);
		    }
			
		    labels.push_back(aLabel);
		    ExpressionList* el = dynamic_cast<ExpressionList*>(
						states->expression_list.object());
		    state_lists.push_back(smart_ptr<ExpressionList>(el));
		    for(int i = 0; i < states->expressions.size(); i++) {
			LabeledStates* states2
				= dynamic_cast<LabeledStates*>(
						states->expressions[i].object());
			aLabel = states2->when_case_label->expressions[0]->to_string();
			if(aLabel[0] == '"') {
				removeQuotes(aLabel);
			}
			labels.push_back(aLabel);
			el = dynamic_cast<ExpressionList*>(
						states2->expression_list.object());
			state_lists.push_back(smart_ptr<ExpressionList>(el));
		    }

		    //
		    // Now we evaluate the lists of states
		    //
		    for(int i = 0; i < labels.size(); i++) {
			tvslist			list_of_states;
			try {
				consolidate_one_list_of_states(
					*state_lists[i].object(),
					list_of_states,
					dent + 2);
			} catch(eval_error Err) {
				Cstring err;
				err << "File " << state_lists[i]->file << ", line "
					<< state_lists[i]->line
					<< ": error evaluating states "
					<< "for resource " << resource_name
					<< "; details:\n" << Err.msg;
				throw(eval_error(err));
			}
			vector_of_state_lists.push_back(list_of_states);
			Cstring one_label = labels[i];
			if(one_label == "default") {
			    default_index = i;
			} else {
			    Cstring full_label(container_type->name);
			    full_label << one_label;
			    map<Cstring, int>::const_iterator iter = flat_map.find(full_label);
			    if(iter == flat_map.end()) {
				Cstring err;
				err << "File " << states->file << ", line "
					<< states->line << ": label "
					<< full_label << " not found in arrayed resource "
					<< resource_name;
				throw(eval_error(err));
			    }
			    map_of_labels[full_label] = i;
			}
		    }
		}
		if(   default_index == -1
		      && map_of_labels.size() != container_type->SubclassTypes.size()) {
		    Cstring err;
		    err << "File " << rd->opt_states_section->file << ", line "
			<< rd->opt_states_section->line << ": resource "
			<< resource_name << " has " << container_type->SubclassTypes.size()
			<< " single resources, but no default list of states is "
			"provided and only " << map_of_labels.size()
			<< " list(s) of states are listed in the states section.";
		    throw(eval_error(err));
		}
		for(int j = 0; j < simple_resources.size(); j++) {
		    map<Cstring, int>::iterator iter
				= map_of_labels.find(simple_resources[j]->name);
		    int index_to_use = 0;
		    if(iter == map_of_labels.end()) {
			index_to_use = default_index;
		    } else {
			index_to_use = iter->second;
		    }

		    simple_resources[j]->add_state(
				&vector_of_state_lists[index_to_use],
				index_to_use == default_index);
		}
	    } else if(resource_type == "state") {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": a resource of type " << resource_type
		    << " must have a states section to list "
		    << "the possible values of the resource.";
		throw(eval_error(err));
	    }


	    //
	    // Handle the profile section (all resources except 'settable')
	    //
	    if(rd->opt_profile_section) {
		if(resource_type == "settable") {
		    Cstring err;
		    err << "File " << file << ", line " << line
			<< ": a resource of type " << resource_type
			<< " must have a default section, not a profile section.";
		    throw(eval_error(err));
		}
		ProfileList*		aProfile = dynamic_cast<ProfileList*>(
						rd->opt_profile_section->expressions[0].object());
		LabeledProfiles*	labeledProfiles = dynamic_cast<LabeledProfiles*>(
						rd->opt_profile_section->expressions[0].object());
		map<Cstring, int>&	flat_map = container_type->SubclassFlatMap;
		vector<tlist<alpha_time, Cnode0<alpha_time, parsedExp> > >
						vector_of_profile_lists;
		int			default_index = -1;
		map<Cstring, int>	map_of_labels;

		if(aProfile) {
		    tlist<alpha_time, Cnode0<alpha_time, parsedExp> > profile_list(true);
		    try {
			consolidate_one_profile(
					container,
					*aProfile,
					profile_list,
					dent + 2);
		    } catch(eval_error Err) {
			Cstring err;
			err << "File " << aProfile->file << ", line " << aProfile->line
			    << ": error evaluating profile "
			    << "for resource " << resource_name
			    << "; details:\n" << Err.msg;
			throw(eval_error(err));
		    }
		    vector_of_profile_lists.push_back(profile_list);
		    default_index = 0;
		} else if(labeledProfiles) {

		    //
		    // first we reorganize the LabeledStates'
		    // data into two vectors, one for labels and
		    // one for expression lists
		    //
		    vector<Cstring>	labels;
		    vector<ProfileList*> profiles;
		    Cstring aLabel = labeledProfiles->when_case_label->expressions[0]->to_string();
		    if(aLabel[0] == '"') {
			removeQuotes(aLabel);
		    }
		
		    labels.push_back(aLabel);

		    ProfileList* pl = dynamic_cast<ProfileList*>(
				labeledProfiles->list_of_profile_expressions.object());
		    profiles.push_back(pl);
		    for(int i = 0; i < labeledProfiles->expressions.size(); i++) {
			LabeledProfiles* pl2
				= dynamic_cast<LabeledProfiles*>(
					labeledProfiles->expressions[i].object());
			aLabel = pl2->when_case_label->expressions[0]->to_string();
			if(aLabel[0] == '"') {
				removeQuotes(aLabel);
			}
			labels.push_back(aLabel);
			pl = dynamic_cast<ProfileList*>(
					pl2->list_of_profile_expressions.object());
			profiles.push_back(pl);
		    }

		    //
		    // Now we evaluate the profiles
		    //
		    for(int i = 0; i < labels.size(); i++) {
			tlist<alpha_time, Cnode0<alpha_time, parsedExp> > profile_list(true);
			try {
			    consolidate_one_profile(
					container,
					*profiles[i],
					profile_list,
					dent + 2);
			} catch(eval_error Err) {
			    Cstring err;
			    err << "File " << profiles[i]->file << ", line "
				<< profiles[i]->line
				<< ": error evaluating profile "
				<< "for resource " << resource_name
				<< "; details:\n" << Err.msg;
			    throw(eval_error(err));
			}
			vector_of_profile_lists.push_back(profile_list);
			Cstring one_label = labels[i];
			if(one_label == "default") {
				default_index = i;
			} else {
			    Cstring full_label(container_type->name);
			    full_label << one_label;
			    map<Cstring, int>::const_iterator iter = flat_map.find(full_label);
			    if(iter == flat_map.end()) {
				Cstring err;
				err << "File " << profiles[i]->file << ", line "
					<< profiles[i]->line << ": label "
					<< full_label << " not found in arrayed resource "
					<< resource_name;
				throw(eval_error(err));
			    }
			    map_of_labels[full_label] = i;
			}
		    }
		}
		if(default_index == -1 && map_of_labels.size() != container_type->SubclassTypes.size()) {
		    Cstring err;
		    err << "File " << rd->opt_profile_section->file << ", line "
			<< rd->opt_profile_section->line << ": resource "
			<< resource_name << " has " << container_type->SubclassTypes.size()
			<< " single resources, but no default profile is "
			"provided and only " << map_of_labels.size()
			<< " profile(s) is/are listed in the profile section.";
		    throw(eval_error(err));
		}
		for(int j = 0; j < simple_resources.size(); j++) {
		    map<Cstring, int>::iterator iter = map_of_labels.find(simple_resources[j]->name);
		    int index_to_use = 0;
		    if(iter == map_of_labels.end()) {
			index_to_use = default_index;
		    } else {
			index_to_use = iter->second;
		    }
	
		    tlist<alpha_time, Cnode0<alpha_time, parsedExp> >& list_to_use
				= vector_of_profile_lists[index_to_use];
		    tlist<alpha_time, Cnode0<alpha_time, parsedExp> > clean_profile(true);
		    slist<alpha_time, Cnode0<alpha_time, parsedExp> >::iterator prof_it(list_to_use);
		    Cnode0<alpha_time, parsedExp>* one_profile;
	
		    Behavior*	beh = container_type->SubclassTypes[j];
		    aafReader::push_current_task(beh->tasks[0]);
		    while((one_profile = prof_it())) {
			parsedExp deep_copy(one_profile->payload->copy());
			aafReader::consolidate_expression(deep_copy, dent + 2);
			clean_profile << new Cnode0<alpha_time, parsedExp>(
				one_profile->getKey(),
				deep_copy);
		    }
		    aafReader::pop_current_task();
	
		    simple_resources[j]->addToTheProfile(clean_profile);
		}
	    } else if(resource_type != "settable") {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": a resource of type " << resource_type
		    << " must have a profile section to define its behavior "
		    << "in the absence of usage statements.";
		throw(eval_error(err));
	    }

	    //
	    // Handle default section ('settable' resources only)
	    //
	    if(rd->opt_default_section) {
		if(resource_type != "settable") {
		    Cstring err;
		    err << "File " << rd->opt_default_section->file << ", line "
			<< rd->opt_default_section->line
			<< ": a resource of type " << resource_type
			<< " is not supposed to have a default section.";
		    throw(eval_error(err));
		}
		map<Cstring, int>&	flat_map = container_type->SubclassFlatMap;
		vector<parsedExp>	vector_of_defaults;
		int			default_index = -1;
		map<Cstring, int>	map_of_labels;
		vector<Cstring>		labels;

		DefaultExpression*	one_exp = dynamic_cast<DefaultExpression*>(
						rd->opt_default_section->expressions[0].object());
		LabeledDefaultList*	multiple_exp = dynamic_cast<LabeledDefaultList*>(
						rd->opt_default_section->expressions[0].object());
		assert(one_exp || multiple_exp);

		if(one_exp) {
		    parsedExp theDefault = one_exp->Expression;
		    aafReader::consolidate_expression(theDefault, dent + 2);

		    for(int j = 0; j < simple_resources.size(); j++) {

			//
			// Define a simple profile list with
			// one element in it (the default):
			//
			tlist<alpha_time, Cnode0<alpha_time, parsedExp> > profile_list(true);
			parsedExp deep_copy(theDefault->copy());
			profile_list << new Cnode0<alpha_time, parsedExp>(
					CTime_base(),
					deep_copy);
			simple_resources[j]->addToTheProfile(profile_list);
		    }
		} else {
		    LabeledDefault* first_labeled_default
				= dynamic_cast<LabeledDefault*>(
					multiple_exp->labeled_default_list.object());
		    assert(first_labeled_default);

		    //
		    // Let's save the first default and its label
		    //
		    Cstring aLabel =
			first_labeled_default->when_case_label->expressions[0]->to_string();
		    parsedExp theDefaultExp = first_labeled_default->Expression;

		    aafReader::consolidate_expression(theDefaultExp, dent + 2);

		    vector_of_defaults.push_back(theDefaultExp);
		    if(aLabel[0] == '"') {
			removeQuotes(aLabel);
		    }
		    Cstring full_label(container_type->name);
		    map<Cstring, int>::const_iterator label_iter;
		    if(aLabel == "default") {
			default_index = labels.size();
		    } else {
			full_label << aLabel;
			label_iter = flat_map.find(full_label);
			if(label_iter == flat_map.end()) {
			    Cstring err;
			    err << "File " << first_labeled_default->file << ", line "
			        << first_labeled_default->line << ": label "
			        << full_label
			        << " not found in arrayed settable resource "
			        << resource_name;
			    throw(eval_error(err));
			}
			map_of_labels[full_label] = labels.size();
		    }
		    labels.push_back(aLabel);

		    //
		    // get the remaining labels and values
		    //
		    for(   int z = 0;
			   z < multiple_exp->labeled_default_list->expressions.size();
			   z++) {
			LabeledDefault*	labeled_default
			    = dynamic_cast<LabeledDefault*>(
				multiple_exp->labeled_default_list->expressions[z].object());
			aLabel = labeled_default->when_case_label->expressions[0]->to_string();
			theDefaultExp = labeled_default->Expression;
			aafReader::consolidate_expression(theDefaultExp, dent + 2);
			vector_of_defaults.push_back(theDefaultExp);
			if(aLabel[0] == '"') {
			    removeQuotes(aLabel);
			}
			if(aLabel == "default") {
			    default_index = labels.size();
			} else {
			    full_label = container_type->name;
			    full_label << aLabel;
			    label_iter = flat_map.find(full_label);
			    if(label_iter == flat_map.end()) {
				Cstring err;
				err << "File " << labeled_default->file << ", line "
				    << labeled_default->line << ": label "
				    << full_label
				    << " not found in arrayed settable resource "
				    << resource_name;
				throw(eval_error(err));
			    }
			    map_of_labels[full_label] = labels.size();
			}
			labels.push_back(aLabel);
		    }

		    //
		    // If no label is named "default", then the number of
		    // labels provided must match the list of resources exactly:
		    //
		    if( default_index == -1
		        && map_of_labels.size() != container_type->SubclassTypes.size()) {
			Cstring err;
			err << "File " << rd->opt_default_section->file << ", line "
			    << rd->opt_default_section->line << ": resource "
			    << resource_name << " has " << container_type->SubclassTypes.size()
			    << " single resources, but no default label is "
			    "provided and only " << map_of_labels.size()
			    << " labels are listed in the default section.";
			throw(eval_error(err));
		    }
		    for(int j = 0; j < simple_resources.size(); j++) {
			map<Cstring, int>::iterator iter
			    = map_of_labels.find(simple_resources[j]->name);
			int index_to_use = 0;
			if(iter == map_of_labels.end()) {
			    index_to_use = default_index;
			} else {
			    index_to_use = iter->second;
			}

			//
			// Define a simple profile list with
			// one element in it (the default):
			//
			tlist<alpha_time, Cnode0<alpha_time, parsedExp> > profile_list(true);
			parsedExp deep_copy(vector_of_defaults[index_to_use]->copy());
			profile_list << new Cnode0<alpha_time, parsedExp>(
						CTime_base(),
						deep_copy);
			simple_resources[j]->addToTheProfile(profile_list);
		    }
		}
	    } else if(resource_type == "settable") {
		Cstring err;
		err << "File " << file << ", line " << line
			<< ": a resource of type " << resource_type
			<< " must have a default section to list "
			<< "the default value of the resource.";
		throw(eval_error(err));
	    }

	    UsageSection* usage_section = dynamic_cast<UsageSection*>(
			rd->usage_section_or_null.object());

	    //
	    // For each individual resource in the array, do the
	    // following:
	    //
	    // 		- create the parameters section
	    //
	    // 		- create the usage section (if it exists)
	    //
	    // 		- create the attributes section (ditto)
	    //
	    // Creation is handled by copying the tasks found in the
	    // sub-type definition of each individual resource.
	    //
	    for(int i = 0; i < container_type->SubclassTypes.size(); i++) {
		Behavior*	one_res = container_type->SubclassTypes[i];


		//
		// push the constructor task
		//
		aafReader::push_current_task(one_res->tasks[0]);

		if(resource_type != "settable") {

		    task* use_task = NULL;

		    map<Cstring, int>::iterator iter = one_res->taskindex.find("use");
		    assert(iter != one_res->taskindex.end());
		    use_task = one_res->tasks[iter->second];

		    //
		    // push the use task
		    //
		    aafReader::push_current_task(use_task);

		    if(usage_section) {

			//
			// In the declaration phase, we created a new variable,
			// 'consumption', which we added to the use task for
			// this resource.
			//
			// Now, we need to create an assignment in which we
			// assign to this variable the value specified in the
			// usage Expression; this then constitutes a
			// one-statement program which we can exercise at
			// runtime.
			//
			parsedExp usage_exp(usage_section->Expression->copy());

			//
			// Create a new left-hand side, 'consumption'
			//
			parsedExp	raw_lhs(
					    new pE_w_string(
						origin_info(
						    usage_section->line,
						    usage_section->file),
						"consumption")
					    );

			//
			// Create all the pieces of an assignment for 'consumption'
			//
			parsedExp	equal_sign(
					    new pE_w_string(
						origin_info(
						    usage_section->line, 
						    usage_section->file),
						"=")
					    );
			parsedExp	semicolon(
					    new pE_w_string(
						origin_info(
						    usage_section->line,
						     usage_section->file),
						    ";")
					    );
			Symbol* consumption_sym = new Symbol(
							0,
							raw_lhs,
							Differentiator_0(0));
			consumption_sym->file = usage_section->file;
			consumption_sym->line = usage_section->line;
			parsedExp lhs(consumption_sym);

			//
			// stick the new pieces together with
			// usage_exp into the new assignment
			//
			Assignment* assign = new Assignment(
							0,
							lhs,
							equal_sign,
							usage_exp,
							semicolon,
							Differentiator_1(0));
			assign->file = usage_section->file;
			assign->line = usage_section->line;

			smart_ptr<executableExp> assign_exp(assign);
			parsedExp parsed_assign_exp(assign);

			//
			// Consolidate the new, synthetic assignment
			// expression the consumption value
			//
			aafReader::consolidate_assignment(
						assign_exp,
						dent + 2,
						/* inside_directive = */ false);

			//
			// Stick the assignment in a program and have
			// the use_task reference it as its 'prog'
			// element
			//
			use_task->prog.reference(
					new Program(
						0,
						parsed_assign_exp,
						Differentiator_0(0)));

		    }

		    //
		    // pop the use task
		    //
		    aafReader::pop_current_task();

		} else { // resource_type is "settable"

		    task* set_task = NULL;

		    map<Cstring, int>::iterator iter = one_res->taskindex.find("set");
		    assert(iter != one_res->taskindex.end());
		    set_task = one_res->tasks[iter->second];

		    //
		    // push the set task
		    //
		    aafReader::push_current_task(set_task);

		    if(!usage_section) {
			Cstring err;
			err << "File " << file << ", line " << line
			    << ": resource of type "
			    << "\"settable\" should have a usage section.";
			throw(eval_error(err));
		    }

		    //
		    // In the declaration phase, we created a new variable,
		    // 'set_value', which we added to the use task for
		    // this resource.
		    //
		    // Now, we need to create an assignment in which we
		    // assign to this variable the value specified in the
		    // usage Expression; this then constitutes a
		    // one-statement program which we can exercise at
		    // runtime.
		    //
		    parsedExp usage_exp(usage_section->Expression->copy());

		    //
		    // Create a left-hand side, 'set_value'
		    //
		    parsedExp	raw_lhs(
				    new pE_w_string(
					    origin_info(
						usage_section->line, 
						usage_section->file),
					    "set_value")
				    );

		    //
		    // Create all the pieces of an assignment for 'set_value'
		    //
		    parsedExp	equal_sign(
				    new pE_w_string(
					     origin_info(
						usage_section->line,
						usage_section->file),
					    "=")
				    );
		    parsedExp	semicolon(
				    new pE_w_string(
					    origin_info(
						usage_section->line, 
						usage_section->file),
					    ";")
				    );
		    Symbol* set_value_sym = new Symbol(
						    0,
						    raw_lhs,
						    Differentiator_0(0));
		    set_value_sym->file = usage_section->file;
		    set_value_sym->line = usage_section->line;
		    parsedExp lhs(set_value_sym);

		    //
		    // stick the new pieces together with
		    // usage_exp into the new assignment
		    //
		    Assignment* assign = new Assignment(
						    0,
						    lhs,
						    equal_sign,
						    usage_exp,
						    semicolon,
						    Differentiator_1(0));
		    assign->file = usage_section->file;
		    assign->line = usage_section->line;

		    smart_ptr<executableExp> assign_exp(assign);
		    parsedExp parsed_assign_exp(assign);

		    //
		    // Consolidate the new, synthetic assignment
		    // expression the consumption value
		    //
		    aafReader::consolidate_assignment(
						assign_exp,
						dent + 2,
						/* inside_directive = */ false);

		    //
		    // Stick the assignment in a program and have
		    // the set_task reference it as its 'prog'
		    // element
		    //
		    set_task->prog.reference(new Program(
						0,
						parsed_assign_exp,
						Differentiator_0(0)));

		    //
		    // pop the set task
		    //
		    aafReader::pop_current_task();
		}

		//
		// pop the constructor task
		//
		aafReader::pop_current_task();


		for(int j = 0; j < simple_resources.size(); j++) {

		    //
		    // Need to execute the constructor program (i.
		    // e., attributes) if it exists
		    //
		    Rsource* res = simple_resources[i];

		    const parsedProg& constructor = res->get_attribute_program();
		    if(constructor) {

			execution_context*		ec = new execution_context(
								res->Object,
								constructor.object());
			smart_ptr<execution_context>	context(ec);
			execution_context::return_code	Code = execution_context::REGULAR;

			try {
			    ec->ExCon(Code);
			} catch(eval_error Err) {
			    Cstring errs;
			    errs << "Error in evaluating attributes of resource "
				<< res->name << "; details:\n" << Err.msg;
			    throw(eval_error(errs));
			}

			//
			// Now we update the properties array for each
			// resource. Although strange, it is not impossible
			// that a specification like
			//
			// 	"No Filtering" = false;
			//
			// could be used - especially when using index-specific
			// values for an arrayed resource.
			//
			if(res->properties[(int)Rsource::Property::has_interpolation]) {
			    if(!(*res->Object)["interpolation"].get_int()) {
				res->properties[(int)Rsource::Property::has_interpolation] = false;
			    }
			}
			if(res->properties[(int)Rsource::Property::has_hidden]) {
			    if(!(*res->Object)["hidden"].get_int()) {
				res->properties[(int)Rsource::Property::has_hidden] = false;
			    }
			}

			//
			// For the time being and until AP-814 is correctly implemented,
			// we disable filtering completely for interpolated resources
			//
			// OK, should be OK to use filtering with interpolation now
			//
			// if(res->properties[(int)Rsource::Property::has_interpolation]) {
			// 	res->properties[(int)Rsource::Property::has_nofiltering] = true;
			// } else
			if(res->properties[(int)Rsource::Property::has_nofiltering]) {
			    if(!(*res->Object)["nofiltering"].get_int()) {
				res->properties[(int)Rsource::Property::has_nofiltering] = false;
			    }
			}
			if(res->properties[(int)Rsource::Property::has_modeling_only]) {
			    if(!(*res->Object)["modeling_only"].get_int()) {
				res->properties[(int)Rsource::Property::has_modeling_only] = false;
			    }
			}
		    }
		}
	    }
	} catch(eval_error(Err)) {

	    //
	    // If an error is caught at this point, the resource we were
	    // working on needs to be deleted to prevent future objects
	    // from dealing with an unfinished resource.
	    //
	    delete container;
	    while(aafReader::LevelOfCurrentTask > 0) {
		aafReader::pop_current_task();
	    }
	    throw(Err);
	}
    }
}

void Usage::consolidate(int dent) {
	ResUsageWithArgs* resusage
		= dynamic_cast<ResUsageWithArgs*>(
			resource_usage_with_arguments.object());
	assert(resusage);
	Cstring resource_name = resusage->resource_usage_name->getData();

	//
	// 1. find resource based on the name
	//

	theContainer = RCsource::resource_containers().find(resource_name);
	if(!theContainer) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": resource \""
			<< resource_name << "\" was not defined; cannot use it";
		throw(eval_error(errs));
	}

	//
	// 2. identify action and allocate Task for storing symbols passed by
	// value, if any
	//

	Cstring action = resusage->tok_action->getData();
	aafReader::TaskForStoringArgsPassedByValue() =
			action
			+ "_" + resource_name
			+ "_" + aafReader::get_current_task()->Type.tasks.size();
	
	assert(action == "use");
	usageType = apgen::USAGE_TYPE::USE;

	//
	// 3. identify indices, if any
	//

	if(resusage->resource_usage_name->expressions.size()
	   && resusage->resource_usage_name->expressions[0]) {

	    //
	    // The Usage statement contains indices
	    //
	    MultidimIndices* multi = dynamic_cast<MultidimIndices*>(
			resusage->resource_usage_name->expressions[0].object());
	    indices.reference(multi);
	    aafReader::consolidate_multidim_indices(indices, dent + 2);
	    multi = indices.object();

	    //
	    // Make sure the numbed of indices is correct
	    // (see JIRA AP-1536)
	    //
	    int	number_of_indices_in_usage = multi->actual_indices.size();
	    int	number_of_indices_in_resource = theContainer->get_index_count();
	    if(number_of_indices_in_usage != number_of_indices_in_resource) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": resource has dimension " << number_of_indices_in_resource
		    << " but usage statement has " << number_of_indices_in_usage
		    << " indices.";
		throw(eval_error(err));
	    }
	} else if(theContainer->payload->Object->array_elements.size() != 1) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": resource "
		 << resource_name << " is an array; usage requires an index.";
	    throw(eval_error(errs));
	}

	//
	// 4. identify arguments. This may include arguments passed by value.
	//

	aafReader::AVariableWasPassedByValue = false;

	ExpressionList* arglist = dynamic_cast<ExpressionList*>(resusage->optional_expression_list.object());
	if(arglist) {
		arglist->consolidate(actual_arguments, dent + 2);
	}

	//
	// 5. verify that the number and type of arguments is correct
	//

	Behavior*		resBehavior = NULL;
	map<Cstring, int>::const_iterator res_iter
			= Behavior::ClassIndices().find(resource_name);
	int			expected_number_of_args = -1;
	RCsource*		container = NULL;


	if(res_iter == Behavior::ClassIndices().end()) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
		     << "trying to use non-existent resource "
		     << resource_name;
		throw(eval_error(errs));
	} else {

	    //
	    // We have a Type for this resource. Since we are consolidating
	    // a usage clause, we must have consolidated the resource already;
	    // a container object should be available. Let's get it.
	    //
	    resBehavior = Behavior::ClassTypes()[res_iter->second];
	    assert(resBehavior->realm == "resource container");

	    //
	    // Determine the task being called and the expected
	    // number of arguments. This will take some work because
	    // of the various possibilities, as documented in the following
	    // table:
	    //
	    //	Resource	Usage	Remarks
	    //	  Type 		Type
	    //	========	=====	=======
	    //	consumable,	use	parameter section, if it exists,
	    //	nonconsumable		determines # of parameters
	    //
	    //			set	not allowed
	    //			reset	not allowed
	    //
	    //	settable	set	parameter section, if it exists,
	    //				determines # of parameters
	    //
	    //			use	not allowed
	    //			reset	not allowed
	    //
	    //	state		use	parameter section, if it exists,
	    //				determines # of parameters
	    //
	    //			set	1 parameter
	    //
	    //			reset	0 parameter
	    //
	    //
	    Behavior* first_res_type = resBehavior->SubclassTypes[0];
	    map<Cstring, int>::const_iterator action_iter
		    = first_res_type->taskindex.find(action);

	    //
	    // Make sure the action keyword is appropriate for the resource
	    // being used / set / reset
	    //
	    if(action_iter == first_res_type->taskindex.end()) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "Attempting to " << action << " resource " << resource_name
			<< " which does not have a " << action << " section";
		throw(eval_error(errs));
	    }
	    theTaskIndex = action_iter->second;

	    //
	    // Grab the task for this action; we already
	    // checked that it's "use"
	    //
	    task*	use_task = first_res_type->tasks[theTaskIndex];


	    //
	    // Make sure variable 'consumption' is defined
	    //
	    map<Cstring, int>::const_iterator consumption_iter
			= use_task->get_varindex().find("consumption");
	    assert(consumption_iter != use_task->get_varindex().end());

	    //
	    // Set the index for the consumption variable
	    // as this->consumptionIndex
	    //
	    consumptionIndex = consumption_iter->second;

	    //
	    // Base number of arguments on parameters, if they exist
	    //
	    if(use_task->parameters) {
		expected_number_of_args
			= use_task->parameters->statements.size();

	    } else {

		//
		// If not, no parameters are expected
		//
		expected_number_of_args = 0;
	    }

	}

	if(expected_number_of_args != actual_arguments.size()) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": "
		<< "Error in '" << action << " "
		<< resource_name << "' clause - "
		<< expected_number_of_args
		<< " arguments expected, "
		<< actual_arguments.size() << " supplied";
	    throw(eval_error(errs));
	}

	//
	// look for arguments passed by value
	//

	if(aafReader::AVariableWasPassedByValue) {
	    map<Cstring, int>::iterator	new_task_ptr
		= aafReader::CurrentType().taskindex.find(
			aafReader::TaskForStoringArgsPassedByValue());
	    assert(new_task_ptr != aafReader::CurrentType().taskindex.end());
	    theStorageTaskIndex = new_task_ptr->second;
	}


	//
	// 6. identify temporal clause, if any
	//

	consolidate_temporal_spec(dent + 2);

	//
	// 7. identify when clause, if any
	//

	if(APcloptions::theCmdLineOptions().debug_grammar) {
	    cerr << aafReader::make_space(dent)
		<< "consolidating Usage of resource "
		<< resource_name << "\n";
	}
}

void Usage::consolidate_temporal_spec(
		int	dent) {

    //
    // In all cases, the purpose of the temporal spec is
    // to compute a vector of times as efficiently as possible.
    //

    //
    // Let's find out what kind of program is using us
    //
    task* curtask = aafReader::get_current_task();
    Program* prog_using_this = curtask->prog.object();

    assert(prog_using_this->orig_section != apgen::METHOD_TYPE::NONE);

    Program::ProgStyle prog_type = prog_using_this->get_style();

    bool from_to_is_required = false;
    if(usageType == apgen::USAGE_TYPE::USE
	&& theContainer
	&& theContainer->payload->Object->array_elements[0]->requires_end_usage()) {
	from_to_is_required = true;
    }

    bool global_method = aafReader::LevelOfCurrentTask == 1;

    if(temporalSpecification) {
	TemporalSpec* ts = dynamic_cast<TemporalSpec*>(temporalSpecification.object());
	if(ts->tok_at) {
	    if(from_to_is_required) {
		Cstring err;
		err << "File " << file << ", line " << line
			<< ": resource requires from-to usage, but one 'at' time is provided";
		throw(eval_error(err));	
	    }
	    aafReader::consolidate_expression(ts->Expression, dent + 2);
	    at_expression = ts->Expression;
	    eval_usage_times = &Usage::get_time_from_one_expression;
	} else if(ts->tok_from) {
	    if(usageType != apgen::USAGE_TYPE::USE || !from_to_is_required) {
		Cstring err;
		err << "File " << file << ", line " << line
			<< ": resource requires a single 'at' time, but 'from-to' times are provided";
		throw(eval_error(err));	
	    }
	    aafReader::consolidate_expression(ts->Expression, dent + 2);
	    from_expression = ts->Expression;
	    aafReader::consolidate_expression(ts->Expression_3, dent + 2);
	    to_expression = ts->Expression_3;
	    eval_usage_times = &Usage::get_time_from_two_expressions;
	} else if(ts->tok_immediately) {
	}
    } else {
	if(global_method) {
	    if(from_to_is_required) {
		Cstring errs;
		errs << "File " << file << ", line " << line
		     << ": from-to resource usage inside a global "
		     << "method requires explicit from and to specs.";
		throw(eval_error(errs));
	    }
	    eval_usage_times = &Usage::get_time_from_now;
	} else {

	    //
	    // consult the resource to find out how many times are needed
	    //
	    if(from_to_is_required) {
		if(prog_type == Program::ASYNCHRONOUS_STYLE) {

		    //
		    // resource usage style
		    //
		    eval_usage_times = &Usage::get_time_from_start_and_finish;
		} else {

		    //
		    // modeling style
		    //
		    Cstring errs;
		    errs << "File " << file << ", line " << line
			 << ": from-to resource usage inside a modeling "
			 << "section requires explicit from and to specs.";
		    throw(eval_error(errs));
		}
	    } else {
		if(prog_type == Program::ASYNCHRONOUS_STYLE) {

		    //
		    // resource usage style
		    //
		    eval_usage_times = &Usage::get_time_from_start;
		} else {

		    //
		    // modeling style
		    //
		    eval_usage_times = &Usage::get_time_from_now;
		}
	    }
	}
    }
}

void ImmediateUsage::consolidate(int dent) {
	ResUsageWithArgs* resusage
		= dynamic_cast<ResUsageWithArgs*>(
			resource_usage_with_arguments.object());
	assert(resusage);
	Cstring resource_name = resusage->resource_usage_name->getData();

	//
	// 1. find resource based on the name
	//
	theContainer = RCsource::resource_containers().find(resource_name);
	if(!theContainer) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": resource \""
			<< resource_name << "\" was not defined; cannot use it";
		throw(eval_error(errs));
	}

	//
	// 2. identify action and allocate Task for storing symbols passed by
	// value, if any
	//
	Cstring action = resusage->tok_action->getData();
	aafReader::TaskForStoringArgsPassedByValue() =
			action
			+ "_" + resource_name
			+ "_" + aafReader::get_current_task()->Type.tasks.size();

	assert(action == "use");
	usageType = apgen::USAGE_TYPE::USE;

	//
	// 3. identify indices, if any
	//

	if(resusage->resource_usage_name->expressions.size()
	   && resusage->resource_usage_name->expressions[0]) {
	    MultidimIndices* multi = dynamic_cast<MultidimIndices*>(
	    resusage->resource_usage_name->expressions[0].object());
	    indices.reference(multi);
	    aafReader::consolidate_multidim_indices(indices, dent + 2);
	    multi = indices.object();

	    //
	    // Make sure the numbed of indices is correct
	    // (see JIRA AP-1536)
	    //
	    int	number_of_indices_in_usage = multi->actual_indices.size();
	    int	number_of_indices_in_resource = theContainer->get_index_count();
	    if(number_of_indices_in_usage != number_of_indices_in_resource) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": resource has dimension " << number_of_indices_in_resource
		    << " but usage statement has " << number_of_indices_in_usage
		    << " indices.";
		throw(eval_error(err));
	    }
	} else if(theContainer->payload->Object->array_elements.size() != 1) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": resource "
		 << resource_name << " is an array; usage requires an index.";
	    throw(eval_error(errs));
	}

	//
	// 4. identify arguments. This may include arguments passed by value.
	//

	aafReader::AVariableWasPassedByValue = false;

	ExpressionList* arglist
		= dynamic_cast<ExpressionList*>(
				resusage->optional_expression_list.object());
	if(arglist) {
		arglist->consolidate(actual_arguments, dent + 2);
	}

	//
	// 5. verify that the number and type of arguments is correct
	//

	Behavior*		resBehavior = NULL;
	map<Cstring, int>::const_iterator res_iter
			= Behavior::ClassIndices().find(resource_name);
	int			expected_number_of_args = -1;


	if(res_iter == Behavior::ClassIndices().end()) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": "
		<< "trying to use non-existent resource " << resource_name;
	    throw(eval_error(errs));
	} else {
	    resBehavior = Behavior::ClassTypes()[res_iter->second];
	    assert(resBehavior->realm == "resource container");


	    //
	    // Determine number of arguments. See Usage::consolidate() for
	    // a list of allowed combinations.
	    //
	    Behavior* first_res_type = resBehavior->SubclassTypes[0];
	    map<Cstring, int>::const_iterator action_iter
		    = first_res_type->taskindex.find(action);
	    if(action_iter == first_res_type->taskindex.end()) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "Attempting to " << action
			<< " resource " << resource_name
			<< " which does not have a " << action << " section";
		throw(eval_error(errs));
	    }
	    theTaskIndex = action_iter->second;
	    task* use_task = first_res_type->tasks[theTaskIndex];
	    map<Cstring, int>::const_iterator consumption_iter
		    = use_task->get_varindex().find("consumption");
	    assert(consumption_iter != use_task->get_varindex().end());
	    consumptionIndex = consumption_iter->second;
	    if(use_task->parameters) {
	 	expected_number_of_args
			= use_task->parameters->statements.size();
	    } else {
		expected_number_of_args = 0;
	    }
	}

	if(expected_number_of_args != actual_arguments.size()) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "Error in '" << action << " " << resource_name << "' clause - "
			<< expected_number_of_args << " arguments expected, "
			<< actual_arguments.size() << " supplied";
		throw(eval_error(errs));
	}

	//
	// look for arguments passed by value
	//

	if(aafReader::AVariableWasPassedByValue) {
		map<Cstring, int>::iterator	new_task_ptr
			= aafReader::CurrentType().taskindex.find(
				aafReader::TaskForStoringArgsPassedByValue());
		assert(new_task_ptr != aafReader::CurrentType().taskindex.end());
		theStorageTaskIndex = new_task_ptr->second;
	}

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Usage of resource "
			<< resource_name << "\n";
	}
}

void SetSignal::consolidate(int dent) {
	usageType = apgen::USAGE_TYPE::SET;
	ResUsageWithArgs* resusage
		= dynamic_cast<ResUsageWithArgs*>(
			resource_usage_with_arguments.object());
	assert(resusage);
	Cstring resource_name = resusage->resource_usage_name->getData();

	//
	// Identify arguments. This may include arguments passed by value.
	//

	aafReader::AVariableWasPassedByValue = false;

	ExpressionList* arglist = dynamic_cast<ExpressionList*>(resusage->optional_expression_list.object());
	if(arglist) {
		arglist->consolidate(actual_arguments, dent + 2);
	}

	int			min_expected_number_of_args = 1;
	int			max_expected_number_of_args = 2;

	if(   min_expected_number_of_args > actual_arguments.size()
	   || max_expected_number_of_args < actual_arguments.size()) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "Error in 'set signal' clause - "
			<< "1 or 2 argument(s) expected, "
			<< actual_arguments.size() << " supplied";
		throw(eval_error(errs));
	}

	//
	// The first argument should be the name of the signal
	//

	if(actual_arguments[0]->get_result_type() != apgen::DATA_TYPE::STRING) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "first argument of set signal() should be a string";
		throw(eval_error(errs));
	}
	if(actual_arguments.size() == 2) {
		if(actual_arguments[1]->get_result_type() != apgen::DATA_TYPE::ARRAY) {
			Cstring errs;
			errs << "File " << file << ", line " << line << ": "
				<< "second argument of set signal() should be an array";
			throw(eval_error(errs));
		}
	}

	//
	// look for arguments passed by value
	//

	if(aafReader::AVariableWasPassedByValue) {
		map<Cstring, int>::iterator	new_task_ptr
			= aafReader::CurrentType().taskindex.find(
				aafReader::TaskForStoringArgsPassedByValue());
		assert(new_task_ptr != aafReader::CurrentType().taskindex.end());
		theStorageTaskIndex = new_task_ptr->second;
	}

	consolidate_temporal_spec(dent + 2);
}


//
// Note:
//
// 	- SetUsage handles state and settable resources, but not
//	  signals, which are handled by SetSignal.
//
//	- SetSignal handles signals in the non-immediate case
//
//	- ImmediateSetUsage handles state and settable resources,
//	  and signals as well. This is slightly inconsistent
//	  since other usage types have special classes for
//	  immediate usage; there should really be a
//	  "ImmediateSetSignal" class.
//
void SetUsage::consolidate(int dent) {
	ResUsageWithArgs* resusage
		= dynamic_cast<ResUsageWithArgs*>(
			resource_usage_with_arguments.object());
	assert(resusage);
	Cstring resource_name = resusage->resource_usage_name->getData();

	//
	// 1. find resource based on the name
	//

	theContainer = RCsource::resource_containers().find(resource_name);
	if(!theContainer) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": resource \""
			<< resource_name << "\" was not defined; cannot set it";
		throw(eval_error(errs));
	}

	//
	// 2. identify action and allocate Task for storing symbols passed by
	// value, if any
	//

	Cstring action = resusage->tok_action->getData();
	assert(action == "set");
	aafReader::TaskForStoringArgsPassedByValue() =
			action
			+ "_" + resource_name
			+ "_" + aafReader::get_current_task()->Type.tasks.size();
	usageType = apgen::USAGE_TYPE::SET;

	//
	// 3. identify indices, if any
	//

	if(resusage->resource_usage_name->expressions.size()
           && resusage->resource_usage_name->expressions[0]) {
	    MultidimIndices* multi = dynamic_cast<MultidimIndices*>(
		resusage->resource_usage_name->expressions[0].object());
	    indices.reference(multi);
	    aafReader::consolidate_multidim_indices(indices, dent + 2);
	    multi = indices.object();

	    //
	    // Make sure the numbed of indices is correct
	    // (see JIRA AP-1536)
	    //
	    int	number_of_indices_in_usage = multi->actual_indices.size();
	    int	number_of_indices_in_resource = theContainer->get_index_count();
	    if(number_of_indices_in_usage != number_of_indices_in_resource) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": resource has dimension " << number_of_indices_in_resource
		    << " but set statement has " << number_of_indices_in_usage
		    << " indices.";
		throw(eval_error(err));
	    }
	} else if(theContainer->payload->Object->array_elements.size() != 1) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": resource "
		 << resource_name << " is an array; usage requires an index.";
	    throw(eval_error(errs));
	}

	//
	// 4. identify arguments. This may include arguments passed by value.
	//

	aafReader::AVariableWasPassedByValue = false;

	ExpressionList* arglist = dynamic_cast<ExpressionList*>(
		resusage->optional_expression_list.object());
	if(arglist) {
		arglist->consolidate(actual_arguments, dent + 2);
	}

	//
	// 5. verify that the number and type of arguments is correct
	//

	Behavior*		resBehavior = NULL;
	map<Cstring, int>::const_iterator res_iter
				= Behavior::ClassIndices().find(
					resource_name);
	int			min_expected_number_of_args = 0;
	int			max_expected_number_of_args = -1;

	assert(res_iter != Behavior::ClassIndices().end());
	resBehavior = Behavior::ClassTypes()[res_iter->second];
	assert(resBehavior->realm == "resource container");

	Behavior* first_res_type = resBehavior->SubclassTypes[0];

	//
	// Make sure the action keyword is appropriate for the resource
	// being used / set / reset
	//
	map<Cstring, int>::const_iterator action_iter
		    = first_res_type->taskindex.find(action);
	if(action_iter == first_res_type->taskindex.end()) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "Attempting to " << action << " resource " << resource_name
			<< " which does not have a " << action << " section";
		throw(eval_error(errs));
	}
	theTaskIndex = action_iter->second;

	if(theContainer->get_class() == apgen::RES_CLASS::SETTABLE) {

	    //
	    // Grab the task for this action; we already
	    // checked that it's "set"
	    //
	    task*	set_task = first_res_type->tasks[theTaskIndex];


	    //
	    // Make sure variable 'set_value' is defined
	    //
	    map<Cstring, int>::const_iterator consumption_iter
			= set_task->get_varindex().find("set_value");
	    assert(consumption_iter != set_task->get_varindex().end());

	    //
	    // Set the index for the consumption variable
	    // as this->consumptionIndex
	    //
	    consumptionIndex = consumption_iter->second;

	    //
	    // Base number of arguments on parameters, if they exist
	    //
	    if(set_task->parameters) {
		max_expected_number_of_args
			= set_task->parameters->statements.size();

	    } else {

		//
		// If not, no parameters are expected
		//
		max_expected_number_of_args = 0;
	    }

	} else {
	    assert(theContainer->get_class() == apgen::RES_CLASS::STATE);

	    //
	    // State resoure; determine number of arguments
	    //
	    min_expected_number_of_args = 1;
	    max_expected_number_of_args = 1;
	}

	//
	// We do this so that adapters can use non-precomputed
	// versions of any precomputed resources. Thus, consolidation
	// will accept both set(<args>) where <args> agrees with
	// the parameters section of the non-precomputed resource
	// definition, and set() with no arguments, which is appropriate
	// for a precomputed resource.
	//
	if(max_expected_number_of_args == 0) {

	    //
	    // We are dealing with a precomputed resource. We have no
	    // way of telling how many parameters are required for
	    // the non-precomputed resource, since we don't have its
	    // definition.  So, we set the max number of params to
	    // a ridiculously high value.
	    //
	    min_expected_number_of_args = 0;
	    max_expected_number_of_args = 1024;
	} else {

	    //
	    // max_expected_number_of_args was set to the correct
	    // value above, so we just need to make sure that set()
	    // for precomputed resources is acceptable (but hopefully
	    // will never be called!)
	    //
	    min_expected_number_of_args = 0;
	}

	if(actual_arguments.size() < min_expected_number_of_args
	   || actual_arguments.size() > max_expected_number_of_args) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "Error in '" << action << " "
			<< resource_name << "' clause - "
			<< "between " << min_expected_number_of_args
			<< " and " << max_expected_number_of_args << " expected, "
			<< actual_arguments.size() << " supplied";
		throw(eval_error(errs));
	}

	//
	// look for arguments passed by value
	//
	if(aafReader::AVariableWasPassedByValue) {
	    map<Cstring, int>::iterator	new_task_ptr
		= aafReader::CurrentType().taskindex.find(
			aafReader::TaskForStoringArgsPassedByValue());
	    assert(new_task_ptr != aafReader::CurrentType().taskindex.end());
	    theStorageTaskIndex = new_task_ptr->second;
	}


	//
	// 6. identify temporal clause, if any
	//
	consolidate_temporal_spec(dent + 2);

	//
	// 7. identify when clause, if any: obsolete; code was removed.
	//
	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent)
			<< "consolidating Usage of resource "
			<< resource_name << "\n";
	}
}

//
// Note:
//
// 	- SetUsage handles state and settable resources, but not
//	  signals, which are handled by SetSignal.
//
//	- SetSignal handles signals in the non-immediate case
//
//	- ImmediateSetUsage handles state and settable resources,
//	  and signals as well. This is slightly inconsistent
//	  since other usage types have special classes for
//	  immediate usage; there should really be a
//	  "ImmediateSetSignal" class.
//
void ImmediateSetUsage::consolidate(int dent) {
	ResUsageWithArgs* resusage
		= dynamic_cast<ResUsageWithArgs*>(
			resource_usage_with_arguments.object());
	assert(resusage);
	Cstring resource_name = resusage->resource_usage_name->getData();

	//
	// 1. find resource based on the name
	//

	theContainer = RCsource::resource_containers().find(resource_name);
	if(!theContainer) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": resource \""
			<< resource_name << "\" was not defined; cannot use it";
		throw(eval_error(errs));
	}

	//
	// 2. identify action and allocate Task for storing symbols passed by
	// value, if any
	//
	Cstring action = resusage->tok_action->getData();
	aafReader::TaskForStoringArgsPassedByValue() =
			action
			+ "_" + resource_name
			+ "_" + aafReader::get_current_task()->Type.tasks.size();
	usageType = apgen::USAGE_TYPE::SET;

	//
	// 3. identify indices, if any
	//
	if(resusage->resource_usage_name->expressions.size()
	   && resusage->resource_usage_name->expressions[0]) {
	    MultidimIndices* multi = dynamic_cast<MultidimIndices*>(
			resusage->resource_usage_name->expressions[0].object());
	    indices.reference(multi);
	    aafReader::consolidate_multidim_indices(indices, dent + 2);
	    multi = indices.object();

	    //
	    // Make sure the numbed of indices is correct
	    // (see JIRA AP-1536)
	    //
	    int	number_of_indices_in_usage = multi->actual_indices.size();
	    int	number_of_indices_in_resource = theContainer->get_index_count();
	    if(number_of_indices_in_usage != number_of_indices_in_resource) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": resource has dimension " << number_of_indices_in_resource
		    << " but set statement has " << number_of_indices_in_usage
		    << " indices.";
		throw(eval_error(err));
	    }
	} else if(theContainer->payload->Object->array_elements.size() != 1) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": resource "
		 << resource_name << " is an array; usage requires an index.";
	    throw(eval_error(errs));
	}

	//
	// 4. identify arguments. This may include arguments passed by value.
	//
	aafReader::AVariableWasPassedByValue = false;

	ExpressionList* arglist = dynamic_cast<ExpressionList*>(
			resusage->optional_expression_list.object());
	if(arglist) {
		arglist->consolidate(actual_arguments, dent + 2);
	}

	//
	// 5. verify that the number and type of arguments is correct
	//
	Behavior*		resBehavior = NULL;
	map<Cstring, int>::const_iterator res_iter
		= Behavior::ClassIndices().find(resource_name);

	//
	// We check min and max numbers because this may be a "set signal"
	// clause, which accepts one or two arguments.
	//
	// We also need to treat precomputed resources in a special way,
	// since we want adapters to use either the old-style, non-
	// precomputed resource, in which case set() takes 1 arg, or the
	// new-style, precomputed resource, in which case set() takes no
	// args.
	//
	int			min_expected_number_of_args = -1;
	int			max_expected_number_of_args = -1;


	if(res_iter != Behavior::ClassIndices().end()) {

	    //
	    // The resource exists; we are not in a "set signal" situation
	    //
	    resBehavior = Behavior::ClassTypes()[res_iter->second];
	    Behavior* first_res_type = resBehavior->SubclassTypes[0];

	    assert(resBehavior->realm == "resource container");

	    //
	    // determine number of arguments
	    //
	    if(theContainer->get_class() == apgen::RES_CLASS::STATE) {

		//
		// Make sure the 'set' task exists
		//
		map<Cstring, int>::const_iterator action_iter
			= first_res_type->taskindex.find(action);
		if(action_iter == first_res_type->taskindex.end()) {
			Cstring errs;
			errs << "File " << file << ", line " << line << ": "
				<< "Attempting to " << action << " resource " << resource_name
				<< " which does not have a " << action << " section";
			throw(eval_error(errs));
		}
		theTaskIndex = action_iter->second;
		min_expected_number_of_args = 1;
		max_expected_number_of_args = 1;
	    } else {

		//
		// Settable resource
		//

		//
		// Too harsh - the adapter may be mistaken and attempt
		// to set a consumable resource. This will be caught below.
		//
		// assert(theContainer->get_class() == apgen::RES_CLASS::SETTABLE);

		//
		// Make sure the 'set' task exists
		//
		map<Cstring, int>::const_iterator action_iter
			= first_res_type->taskindex.find(action);
		if(action_iter == first_res_type->taskindex.end()) {
			Cstring errs;
			errs << "File " << file << ", line " << line << ": "
				<< "Attempting to " << action << " resource " << resource_name
				<< " which does not have a " << action << " section";
			throw(eval_error(errs));
		}
		theTaskIndex = action_iter->second;

		//
		// Grab the task for this action; we already
		// checked that it's "set"
		//
		task*	set_task = first_res_type->tasks[theTaskIndex];


		//
		// Make sure variable 'set_value' is defined
		//
		map<Cstring, int>::const_iterator consumption_iter
			= set_task->get_varindex().find("set_value");
		assert(consumption_iter != set_task->get_varindex().end());

		//
		// Set the index for the consumption variable
		// as this->consumptionIndex
		//
		consumptionIndex = consumption_iter->second;

		//
		// Base number of arguments on parameters, if they exist
		//
		if(set_task->parameters) {
			max_expected_number_of_args
				= set_task->parameters->statements.size();
		} else {

			//
			// If not, no parameters are expected
			//
			max_expected_number_of_args = 0;
		}

		//
		// We do this so that adapters can use non-precomputed
		// versions of any precomputed resources. Thus, consolidation
		// will accept both set(<args>) where <args> agrees with
		// the parameters section of the non-precomputed resource
		// definition, and set() with no arguments, which is appropriate
		// for a precomputed resource.
		//
		if(max_expected_number_of_args == 0) {

		    //
		    // We are dealing with a precomputed resource. We have no
		    // way of telling how many parameters are required for
		    // the non-precomputed resource, since we don't have its
		    // definition.  So, we set the max number of params to
		    // a ridiculously high value.
		    //
		    min_expected_number_of_args = 0;
		    max_expected_number_of_args = 1024;
		} else {

		    //
		    // max_expected_number_of_args was set to the correct
		    // value above, so we just need to make sure that set()
		    // for precomputed resources is acceptable (but hopefully
		    // will never be called!)
		    //
		    min_expected_number_of_args = 0;
		}

	    } // end of case 'settable resource'
	} else {

	    //
	    // no resource by that name exists
	    //
	    if(resource_name != "signal") {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "trying to set non-existent resource " << resource_name;
		throw(eval_error(errs));
	    }

	    //
	    // signal
	    //
	    min_expected_number_of_args = 1;

	    //
	    // could also be 2 !!!
	    //
	    max_expected_number_of_args = 2;
	}


	if(   min_expected_number_of_args > actual_arguments.size()
	   || max_expected_number_of_args < actual_arguments.size()) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "Error in '" << action << " " << resource_name << "' clause - "
			<< "between " << min_expected_number_of_args
			<< " and " << max_expected_number_of_args
			<< " arguments expected, "
			<< actual_arguments.size() << " supplied";
		throw(eval_error(errs));
	}

	//
	// look for arguments passed by value
	//
	if(aafReader::AVariableWasPassedByValue) {
		map<Cstring, int>::iterator	new_task_ptr
			= aafReader::CurrentType().taskindex.find(
				aafReader::TaskForStoringArgsPassedByValue());
		assert(new_task_ptr != aafReader::CurrentType().taskindex.end());
		theStorageTaskIndex = new_task_ptr->second;
	}


	//
	// 6. identify temporal clause, if any: N/A since this is immediate
	//

	//
	// 7. identify when clause, if any: N/A since this is now obsolete
	//

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Usage of resource "
			<< resource_name << "\n";
	}
}

void ResetUsage::consolidate(int dent) {
	ResUsageWithArgs* resusage
		= dynamic_cast<ResUsageWithArgs*>(
			resource_usage_with_arguments.object());
	assert(resusage);
	Cstring resource_name = resusage->resource_usage_name->getData();

	//
	// 1. find resource based on the name
	//

	theContainer = RCsource::resource_containers().find(resource_name);
	if(!theContainer) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": resource \""
			<< resource_name << "\" was not defined; cannot reset it";
		throw(eval_error(errs));
	}

	//
	// 2. identify action and allocate Task for storing symbols passed by
	// value, if any
	//

	Cstring action = resusage->tok_action->getData();
	aafReader::TaskForStoringArgsPassedByValue() =
			action
			+ "_" + resource_name
			+ "_" + aafReader::get_current_task()->Type.tasks.size();
	usageType = apgen::USAGE_TYPE::RESET;

	//
	// 3. identify indices, if any
	//

	if(resusage->resource_usage_name->expressions.size()
	   && resusage->resource_usage_name->expressions[0]) {
	    MultidimIndices* multi = dynamic_cast<MultidimIndices*>(
			resusage->resource_usage_name->expressions[0].object());
	    indices.reference(multi);
	    aafReader::consolidate_multidim_indices(indices, dent + 2);
	    multi = indices.object();

	    //
	    // Make sure the numbed of indices is correct
	    // (see JIRA AP-1536)
	    //
	    int	number_of_indices_in_usage = multi->actual_indices.size();
	    int	number_of_indices_in_resource = theContainer->get_index_count();
	    if(number_of_indices_in_usage != number_of_indices_in_resource) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": resource has dimension " << number_of_indices_in_resource
		    << " but reset statement has " << number_of_indices_in_usage
		    << " indices.";
		throw(eval_error(err));
	    }
	} else if(theContainer->payload->Object->array_elements.size() != 1) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": resource "
		 << resource_name << " is an array; usage requires an index.";
	    throw(eval_error(errs));
	}


	map<Cstring, int>::const_iterator res_iter
			= Behavior::ClassIndices().find(resource_name);
	Behavior*		resBehavior = NULL;

	if(res_iter != Behavior::ClassIndices().end()) {
	    resBehavior = Behavior::ClassTypes()[res_iter->second];
	    if(resBehavior->realm == "resource container") {
		// determine number of arguments
		Behavior* first_res_type = resBehavior->SubclassTypes[0];
		map<Cstring, int>::const_iterator action_iter = first_res_type->taskindex.find(action);
		if(action_iter == first_res_type->taskindex.end()) {
			Cstring errs;
			errs << "File " << file << ", line " << line << ": "
				<< "Attempting to " << action << " resource " << resource_name
				<< " which does not have a " << action << " section";
			throw(eval_error(errs));
		}
		theTaskIndex = action_iter->second;
	    }
	}

	if(actual_arguments.size() != 0) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "Error in '" << action << " " << resource_name << "' clause - "
			<< 0 << " arguments expected, "
			<< actual_arguments.size() << " supplied";
		throw(eval_error(errs));
	}

	//
	// 6. identify temporal clause, if any
	//

	consolidate_temporal_spec(dent + 2);

	//
	// 7. identify when clause, if any
	//

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Usage of resource "
			<< resource_name << "\n";
	}
}

void ImmediateResetUsage::consolidate(int dent) {
	ResUsageWithArgs* resusage
		= dynamic_cast<ResUsageWithArgs*>(
			resource_usage_with_arguments.object());
	assert(resusage);
	Cstring resource_name = resusage->resource_usage_name->getData();

	//
	// 1. find resource based on the name
	//

	theContainer = RCsource::resource_containers().find(resource_name);
	if(!theContainer) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": resource \""
			<< resource_name << "\" was not defined; cannot use it";
		throw(eval_error(errs));
	}

	//
	// 2. identify action and allocate Task for storing symbols passed by
	// value, if any
	//

	Cstring action = resusage->tok_action->getData();
	aafReader::TaskForStoringArgsPassedByValue() =
			action
			+ "_" + resource_name
			+ "_" + aafReader::get_current_task()->Type.tasks.size();
	usageType = apgen::USAGE_TYPE::RESET;

	//
	// 3. identify indices, if any
	//

	if(resusage->resource_usage_name->expressions.size()
	   && resusage->resource_usage_name->expressions[0]) {
	    MultidimIndices* multi = dynamic_cast<MultidimIndices*>(
			resusage->resource_usage_name->expressions[0].object());
	    indices.reference(multi);
	    aafReader::consolidate_multidim_indices(indices, dent + 2);
	    multi = indices.object();

	    //
	    // Make sure the numbed of indices is correct
	    // (see JIRA AP-1536)
	    //
	    int	number_of_indices_in_usage = multi->actual_indices.size();
	    int	number_of_indices_in_resource = theContainer->get_index_count();
	    if(number_of_indices_in_usage != number_of_indices_in_resource) {
		Cstring err;
		err << "File " << file << ", line " << line
		    << ": resource has dimension " << number_of_indices_in_resource
		    << " but reset statement has " << number_of_indices_in_usage
		    << " indices.";
		throw(eval_error(err));
	    }
	} else if(theContainer->payload->Object->array_elements.size() != 1) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": resource "
		 << resource_name << " is an array; usage requires an index.";
	    throw(eval_error(errs));
	}


	map<Cstring, int>::const_iterator res_iter
				= Behavior::ClassIndices().find(resource_name);
	Behavior*		resBehavior = NULL;

	if(res_iter != Behavior::ClassIndices().end()) {
	    resBehavior = Behavior::ClassTypes()[res_iter->second];
	    if(resBehavior->realm == "resource container") {
		// determine number of arguments
		Behavior* first_res_type = resBehavior->SubclassTypes[0];
		map<Cstring, int>::const_iterator action_iter
				= first_res_type->taskindex.find(action);
		if(action_iter == first_res_type->taskindex.end()) {
			Cstring errs;
			errs << "File " << file << ", line " << line << ": "
			     << "Attempting to " << action << " resource "
			     << resource_name
			     << " which does not have a "
			     << action << " section";
			throw(eval_error(errs));
		}
		theTaskIndex = action_iter->second;
	    }
	}

	if(actual_arguments.size() != 0) {
		Cstring errs;
		errs << "File " << file << ", line " << line << ": "
			<< "Error in '" << action << " " << resource_name << "' clause - "
			<< 0 << " arguments expected, "
			<< actual_arguments.size() << " supplied";
		throw(eval_error(errs));
	}

	//
	// 6. identify temporal clause, if any
	//

	// consolidate_temporal_spec(dent + 2);

	//
	// 7. identify when clause, if any
	//

	if(APcloptions::theCmdLineOptions().debug_grammar) {
		cerr << aafReader::make_space(dent) << "consolidating Usage of resource "
			<< resource_name << "\n";
	}
}

void AbstractUsage::consolidate(int dent) {
    ResUsageWithArgs* resusage
		= dynamic_cast<ResUsageWithArgs*>(
			resource_usage_with_arguments.object());
    assert(resusage);

    Cstring resource_name = resusage->resource_usage_name->getData();

    //
    // 1. find resource based on the name
    //

    //
    // First, the behavior
    //
    Behavior* abs_beh = Behavior::find_type(resource_name);
    if(!abs_beh || abs_beh->realm != "abstract resource") {
	Cstring errs;
	errs << "File " << file << ", line " << line << ": abstract resource \""
	     << resource_name << "\" was not defined; cannot use it";
	throw(eval_error(errs));
    }

    //
    // Next, the ontological object. This is terribly WRONG. One
    // should use the Behavior and its tasks, not some kind of
    // globally scoped behaving_object. A behaving_object should
    // only be created in support of executing a task.
    //
    // Cnode0<alpha_string, behaving_object*>* abs_ptr =
    // 		behaving_object::abstract_resources().find(resource_name);

    //
    // One should exist if the other does:
    //
    // assert(abs_ptr);
    abs_type = abs_beh;

    //
    // 2. identify action and allocate Task for storing symbols passed by
    // value, if any
    //

    Cstring action = resusage->tok_action->getData();
    aafReader::TaskForStoringArgsPassedByValue() =
			action
			+ "_" + resource_name
			+ "_" + aafReader::get_current_task()->Type.tasks.size();
    if(action == "use") {
	usageType = apgen::USAGE_TYPE::USE;
    } else {
	Cstring errs;
	errs << "File " << file << ", line " << line << ": abstract resource \""
	     << resource_name << "\" only support 'use', not '" << action << "'";
	throw(eval_error(errs));
    }

    //
    // 4. identify arguments. This may include arguments passed by value.
    //

    aafReader::AVariableWasPassedByValue = false;

    ExpressionList* arglist = dynamic_cast<ExpressionList*>(
			resusage->optional_expression_list.object());
    if(arglist) {
	arglist->consolidate(actual_arguments, dent + 2);
    }

    //
    // 5. verify that the number and type of arguments is correct
    //

    Behavior*	resBehavior = NULL;
    map<Cstring, int>::const_iterator res_iter
			= Behavior::ClassIndices().find(resource_name);
    int		expected_number_of_args = -1;


    if(res_iter != Behavior::ClassIndices().end()) {
	resBehavior = Behavior::ClassTypes()[res_iter->second];

	//
	// We already checked that the resource was abstract
	// when we replaced the base Usage by AbstractUsage:
	//
	// assert(resBehavior->realm == "abstract resource");
	if(action != "use") {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": "
		 << "action " << action
		 << " is not appropriate for using an abstract resource";
	    throw(eval_error(errs));
	}
	// determine number of arguments
	// const Behavior* abs_res_type = &abstract_resource->Task.Type;
	map<Cstring, int>::const_iterator action_iter
		    = resBehavior->taskindex.find("resource usage");
	if(action_iter == resBehavior->taskindex.end()) {
	    action_iter = resBehavior->taskindex.find("modeling");
	}
	if(action_iter == resBehavior->taskindex.end()) {
	    Cstring errs;
	    errs << "File " << file << ", line " << line << ": "
		 << "Attempting to use resource " << resource_name
		 << " which does not have a usage section";
	    throw(eval_error(errs));
	}
	theTaskIndex = action_iter->second;

	//
	// Abstract resources only have 2 tasks: constructor and usage
	//
	assert(theTaskIndex == 1);

	task*	use_task = resBehavior->tasks[theTaskIndex];
	if(use_task->parameters) {
	    expected_number_of_args = use_task->parameters->statements.size();
	} else {
	    expected_number_of_args = 0;
	}
    } else {
	Cstring errs;
	errs << "File " << file << ", line " << line << ": "
	     << "abstract resource " << resource_name << " does not seem to exist";
	throw(eval_error(errs));
    }

    if(expected_number_of_args != actual_arguments.size()) {
	Cstring errs;
	errs << "File " << file << ", line " << line << ": "
	     << "Error in '" << action << " " << resource_name << "' clause - "
	     << expected_number_of_args << " arguments expected, "
	     << actual_arguments.size() << " supplied";
	throw(eval_error(errs));
    }

    //
    // look for arguments passed by value
    //

    if(aafReader::AVariableWasPassedByValue) {
	map<Cstring, int>::iterator	new_task_ptr
		= aafReader::CurrentType().taskindex.find(
			aafReader::TaskForStoringArgsPassedByValue());
	assert(new_task_ptr != aafReader::CurrentType().taskindex.end());
	theStorageTaskIndex = new_task_ptr->second;
    }


    //
    // 6. identify temporal clause, if any
    //

    consolidate_temporal_spec(dent + 2);

    //
    // 7. identify when clause, if any - OBSOLETE
    //

    if(APcloptions::theCmdLineOptions().debug_grammar) {
	cerr << aafReader::make_space(dent) << "consolidating Usage of abstract resource "
	     << resource_name << "\n";
    }
}

void AbstractUsage::consolidate_temporal_spec(
		int	dent) {

	//
	// Remember that in all cases, the purpose of the temporal spec is
	// to compute a vector of times as efficiently as possible.
	//

	//
	// First, make sure the task is at the object method level; neither
	// globals nor ordinary functions are allowed to contain statements
	// with time specifications.
	//

	if(aafReader::LevelOfCurrentTask < 2) {
		Cstring err;
		err << "File " << file << ", line " << line
			<< ": problems trying to consolidate time spec "
			"from level " << aafReader::LevelOfCurrentTask
			<< "; only activity type decomposition/modeling/scheduling sections "
			"and abstract resource modeling sections are allowed to issue "
			"time specifications.";
		throw(eval_error(err));
	}

	//
	// Let's find out what kind of program is using us
	//

	task* curtask = aafReader::get_current_task();
	Program* prog_using_this = curtask->prog.object();

	assert(prog_using_this->orig_section != apgen::METHOD_TYPE::NONE);

	Program::ProgStyle prog_type = prog_using_this->get_style();

	if(temporalSpecification) {
		TemporalSpec* ts = dynamic_cast<TemporalSpec*>(temporalSpecification.object());
		if(ts->tok_at) {
			aafReader::consolidate_expression(ts->Expression, dent + 2);
			at_expression = ts->Expression;
			eval_usage_times = &Usage::get_time_from_one_expression;
		} else if(ts->tok_from) {
			aafReader::consolidate_expression(ts->Expression, dent + 2);
			from_expression = ts->Expression;
			aafReader::consolidate_expression(ts->Expression_3, dent + 2);
			to_expression = ts->Expression_3;
			eval_usage_times = &Usage::get_time_from_two_expressions;
		}
	} else {
		if(prog_type == Program::ASYNCHRONOUS_STYLE) {

			//
			// resource usage style
			//

			eval_usage_times = &Usage::get_time_from_start_and_finish;
		} else {

			//
			// modeling style
			//

			eval_usage_times = &Usage::get_time_from_now;
		}
	}
}


