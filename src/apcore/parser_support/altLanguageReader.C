#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "aafReader.H"
#include "ActivityInstance.H"
#include "DB.H"
#include "act_plan_parser.H"
#include "action_request.H"
#include "json_act_parser.H"
#include "RES_exec.H"
#include "xml_act_parser.H"
#include "xml_sax_parser.H"

void fileReader::parse_json_string(const Cstring& s) {
#ifdef GUTTED
	fileParser&			conv(fileParser::currentParser());
	const char*			start_here = *conv.theDataToParse;

	// skip over the first line
	while (*start_here && *start_here != '\n') {
		start_here++; }
	assert(*start_here == '\n');
	start_here++;

	if(conv.which_parser == fileParser::JSON_PARSER_ACT_DATA) {
		json_act_parser		parser(start_here);

		try {
			parser.parse();
			parse_act_plan /* <json_act_parser> */ (parser);
		}
		catch(eval_error Err) {
			Cstring err;
			err << "Error while trying to parse json string:\n"
				<< Err.msg;
			throw(eval_error(err));
		}
	} else if(conv.which_parser == fileParser::JSON_PARSER_RES_SPEC) {
	} else if(conv.which_parser == fileParser::JSON_PARSER_RES_DATA) {
	}
#endif /* GUTTED */
}

/* In the new, SAX-based version of parse_xml_string, the APGen thread
 * handles parsing with the libxml++ SAX parser, which works through
 * callback functions. These functions are implemented in xml_sax_parser.C.
 * They are designed to wait for parse_act_plan to request various quantities.
 * Needless to say, this little ballet is tricky because the two parties have
 * to know in what order to do things. But when I am done it will work
 * beautifully. */

static FILE* use_this_file = NULL;

int one_char_at_a_time(long) {
	char	c;
	long k = fread(&c, 1, 1, use_this_file);
	if(!k) {
		return 0;
	} else {
		return c;
	}
}

void fileReader::parse_xml_stream(
		const Cstring& filename) {
	fileParser&		conv(fileParser::currentParser());
	use_this_file = fopen(*filename, "r");

	// we know that conv.which_parser == XML_PARSER_TOL_DATA
	xml_reader		parser(one_char_at_a_time);

	try {
		parser.parse();
		fclose(use_this_file);
	} catch(eval_error Err) {
		Cstring any_errors;
		fclose(use_this_file);
		any_errors << "parse_xml_stream: error - details:\n"
			<< Err.msg;
		throw(eval_error(any_errors));
	}
}

/* next task: emulate process_activity_instance() in apf_intfc.C, without
 * globals, and taking all the data from the json string...
 *
 * In the case of parsing a file, attributes emerge as instructions. In the
 * present case (i. e. Json), they will emerge as values or strings which
 * could easily be parsed as expressions. This has an advantage: we could
 * evaluate the expressions in the correct activity instance context, which
 * would provide us with interesting flexibility. I like this idea.
 *
 * So, the boolean argument in fileParser::FixAllPointersToParentsAndChildren()
 * would let us choose between two specs for attributes: instructions (for
 * files) or pE's (for Json strings).
 *
 * Next question: we face the same issue with parameters. But there, we have
 * already done our homework: when reading from a file, parameters are supplied via
	 * Cntnr<alpha_string, pEsys::pE> objects. We can obviously do the same thing
 * when extracting values from a Json string.
 *
 * List of things in fileParser that need to be cleared and
 * maintained while parsing a Json string:
 *
 *	conv.activity_instanceCount
 *	conv.ListOfActivityPointersIndexedByIDsInThisFile.find(ID_for_this_file)
 *	conv.allRequests
 *	conv.allChameleons
	conv.Instances
 * */

void	fileReader::process_activity_instance(
		const CTime_base&	start_time,
		act_viz			Visibility,
		act_sort		request_or_chameleon,
		tvslist&		the_attributes,
		tvslist&		the_parameters,
		const Cstring&		activityName,
		const Cstring&		activityTypeName,
		const stringslist&	theAbstractableAncestors,
		const stringslist&	theAbstractedAncestors,
		const stringslist&	theDecomposableChildren,
		const stringslist&	theDecomposedChildren,
		Cstring&		ID_for_this_file) {
	int				p = 0;
	bool				found_errors = false;
	instance_pointer*		ip = NULL;
	apgen::act_visibility_state	instance_visibility = apgen::act_visibility_state::VISIBILITY_REGULAR;
	instance_tag*			activityInstanceTag = NULL;
	ActivityInstance*		activityInstance = NULL;
	fileParser&			conv(fileParser::currentParser());

	/* The first step is to determine visibility based on parent
	 * information alone. */
	if(Visibility == abstracted) {
		instance_visibility = apgen::act_visibility_state::VISIBILITY_ABSTRACTED;
	} else if(Visibility == decomposed) {
		instance_visibility = apgen::act_visibility_state::VISIBILITY_DECOMPOSED;
	} else {
		instance_visibility = apgen::act_visibility_state::VISIBILITY_REGULAR;
	}

	aafReader::ActivityInstanceCount++ ;

	Cstring		legend_to_use;
	pairslist	opt_pairs;

	if(!found_errors) {
		tvslist::iterator	attrs(the_attributes);
		TaggedValue*		N;

		/* NOTE: when reading from an APF, we accept
		 * expressions and we need to evaluate in the proper
		 * context. In the present case, however, we are
		 * reading from a database, and all objects should be
		 * values, not expressions. */
		while((N = attrs())) {

			/* In most cases this step is not necessary,
			 * because Compile() already invokes the bison
			 * AAF parser, which does create pE values
			 * when defining simple expressions that
			 * translate directly into TypedValues.
			 * It would be nice to ascertain that values
			 * are ALWAYS create... if we could make sure,
			 * then the following step could be omitted
			 * for sure. */
			if(N->get_key() == "legend") {
				legend_to_use = N->payload.get_string();
			}
		}

		tvslist::iterator	params(the_parameters);
		while((N = params())) {
			opt_pairs << new symNode(N->get_key(), N->payload.to_string());
		}

		Cstring		any_errors;

			
		/* regular instance */
		assert(request_or_chameleon == regular_act);
		try {
			activityInstance = apgenDB::CreateActivity(
						activityTypeName,
						activityName,
						legend_to_use,
						start_time,
						opt_pairs,
						ID_for_this_file);
		}
		catch(eval_error Err) {
			any_errors << " -- Cannot create activity instance ";
			activityInstance = NULL;
			throw(eval_error(any_errors));
		}
		if(any_errors.length()) {
			Cstring full_message;

			if(activityInstance) {
				destroy_an_act_req(activityInstance, /* force = */ true);
			}
			activityInstance = NULL;
			full_message << "Error while attempting to create activity instance "
				<< activityName << ", ID " << ID_for_this_file << ":\n"
				<< any_errors;
			throw(eval_error(*full_message));
		} else {
			conv.Instances << (activityInstanceTag = new instance_tag(
						activityInstance->get_unique_id(),
						instance_tag_PLD(	activityInstance,
									instance_visibility)));
			if((activityInstanceTag =
				conv.ListOfActivityPointersIndexedByIDsInThisFile.find( ID_for_this_file))) {
				activityInstanceTag->payload.act = activityInstance;
			} else {
				conv.ListOfActivityPointersIndexedByIDsInThisFile <<
					(activityInstanceTag = new instance_tag(
								ID_for_this_file,
								instance_tag_PLD(
									activityInstance,
									instance_visibility)));
			}
		}
	}

	// 3. Fill in parents/toddlers lists in the instance tag and visibility attribute of tag and instance_pointer
	if(activityInstanceTag && (activityInstance || request_or_chameleon != regular_act)) {
		emptySymbol* one_act;

		while((one_act = theAbstractableAncestors.first_node())) {
			activityInstanceTag->payload.parents << one_act;
		}
		while((one_act = theAbstractedAncestors.first_node())) {
			activityInstanceTag->payload.parents << one_act;
		}
		while((one_act = theDecomposableChildren.first_node())) {
			activityInstanceTag->payload.toddlers << one_act;
		}
		while((one_act = theDecomposedChildren.first_node())) {
			activityInstanceTag->payload.toddlers << one_act;
		}

		// if(activityInstance) {
		// 	activityInstance->hierarchy().set_downtype(DecompType);
		// }
	}
	activityInstance = NULL;
}

void extract_a_global_from(
		const string&	object_type,
		const string&	object_id,
		json_object*	glob,
		bool		editable) {
	assert(json_object_get_type(glob) == json_type_object);
	json_object*	name;
	json_object*	kind;
	json_object*	documentation;
	json_object_object_get_ex(glob, "name", &name);
	json_object_object_get_ex(glob, "quantityKind", &kind);
	json_object_object_get_ex(glob, "documentation", &documentation);
	const char*	name_as_chars = json_object_get_string(name);
	const char*	kind_as_chars = json_object_get_string(kind);
	const char*	doc = json_object_get_string(documentation);
	// we need to parse doc as a json string
	enum json_tokener_error		jsonError;
	json_object*			global_value = json_tokener_parse_verbose(
							doc,
							&jsonError);
	if(!global_value) {
		string errors = "Json parsing error: ";
		errors = errors + json_tokener_error_desc(jsonError) + "\n";
		throw(eval_error(errors.c_str()));
	}
	json_object*			global_typed_value = json_object_new_object();
	json_object_object_add(global_typed_value, "type", kind);
	json_object_object_add(global_typed_value, "value", global_value);
	TypedValue v;
	v.from_json(global_typed_value);
	v.declared_type = v.get_type();
	task* T = Behavior::GlobalType().tasks[0];
	int ind = T->add_variable(name_as_chars, v.get_type());

	// this does nothing:
	// globalData::qualifySymbol(name_as_chars, v.get_type());
	
	(*behaving_object::GlobalObject())[ind] = v;

	json_object_put(global_typed_value);
}

static map<int, string>&	allFunctions() {
	static map<int, string>	m;
	return m;
}

static map<int, string>&	allConcreteResources() {
	static map<int, string>	m;
	return m;
}

static map<int, string>&	allAbstractResources() {
	static map<int, string>	m;
	return m;
}

static map<int, string>&	allActivityTypes() {
	static map<int, string>	m;
	return m;
}

static map<int, string>&	allInstances() {
	static map<int, string>	m;
	return m;
}

void parse_json_constraint(
		const string&	object_type,
		const string&	object_id,
		json_object*	constraint) {
	assert(json_object_get_type(constraint) == json_type_object);
	json_object*	id;
	json_object_object_get_ex(constraint, "id", &id);
	const char*	id_as_chars = json_object_get_string(id);
	string		id_as_string(id_as_chars);
	int		semicolons = id_as_string.find("::");
	string		sub_id = id_as_string.substr(semicolons + 2);
	int		semicolons2 = sub_id.find("::");
	string		func_index_as_string = sub_id.substr(0, semicolons2);
	int		func_index = atoi(func_index_as_string.c_str());
	json_object*	name;
	json_object_object_get_ex(constraint, "name", &name);
	const char*	name_as_chars = json_object_get_string(name);
	json_object*	body;
	json_object_object_get_ex(constraint, "body", &body);
	const char*	body_as_chars = json_object_get_string(body);
	allFunctions()[func_index] = body_as_chars;
}

void parse_json_property_group(
		const string&	object_type,
		const string&	object_id,
		json_object*	property_group) {
	assert(json_object_get_type(property_group) == json_type_object);
	json_object*	state_variables;
	json_object_object_get_ex(property_group, "stateVariables", &state_variables);
	assert(json_object_get_type(state_variables) == json_type_array);
	json_object*	parameters;
	json_object_object_get_ex(property_group, "parameters", &parameters);
	assert(json_object_get_type(parameters) == json_type_array);
	int L = json_object_array_length(state_variables);
	for(int i = 0; i < L; i++) {
		json_object*	one_global = json_object_array_get_idx(state_variables, i);
		extract_a_global_from(object_type, object_id, one_global, true);
	}
	L = json_object_array_length(parameters);
	for(int i = 0; i < L; i++) {
		json_object*	one_global = json_object_array_get_idx(parameters, i);
		extract_a_global_from(object_type, object_id, one_global, false);
	}
}

void parse_json_subsystem_constraints(
		const string& object_type,
		const string& object_id,
		json_object* constraints) {
	assert(json_object_get_type(constraints) == json_type_array);
	int	L = json_object_array_length(constraints);
	for(int i = 0; i < L; i++) {
		json_object*	constraint = json_object_array_get_idx(constraints, i);
		parse_json_constraint(object_type, object_id, constraint);
	}
}

void parse_json_resource_property_group(
		int		res_index,
		json_object*	property_group) {
	assert(json_object_get_type(property_group) == json_type_object);
	json_object*	documentation;
	json_object_object_get_ex(property_group, "documentation", &documentation);
	const char*	body = json_object_get_string(documentation);
	allConcreteResources()[res_index] = body;
}

void parse_json_resource_property_groups(
		int		res_index,
		json_object*	property_groups) {
	assert(json_object_get_type(property_groups) == json_type_array);
	int	L = json_object_array_length(property_groups);
	for(int i = 0; i < L; i++) {
		json_object*	property_group = json_object_array_get_idx(property_groups, i);
		parse_json_resource_property_group(res_index, property_group);
	}
}

void parse_json_subsystem_property_groups(
		const string& object_type,
		const string& object_id,
		json_object* property_groups) {
	assert(json_object_get_type(property_groups) == json_type_array);
	int	L = json_object_array_length(property_groups);
	for(int i = 0; i < L; i++) {
		json_object*	property_group = json_object_array_get_idx(property_groups, i);
		parse_json_property_group(object_type, object_id, property_group);
	}
}

void parse_json_characterization(json_object* characterization) {
	assert(json_object_get_type(characterization) == json_type_object);
	json_object*	id;
	json_object_object_get_ex(characterization, "id", &id);
	const char*	id_as_chars = json_object_get_string(id);
	string		id_as_string(id_as_chars);
	int		semicolons = id_as_string.find("::");
	string		prefix = id_as_string.substr(0, semicolons);
	string		sub_id = id_as_string.substr(semicolons + 2);
	int		semicolons2 = sub_id.find("::");
	string		index_as_string = sub_id.substr(0, semicolons2);
	int		the_index = atoi(index_as_string.c_str());
	json_object*	name;
	json_object_object_get_ex(characterization, "name", &name);
	const char*	name_as_chars = json_object_get_string(name);
	json_object*	documentation;
	json_object_object_get_ex(characterization, "documentation", &documentation);
	const char*	body = json_object_get_string(documentation);
	if(prefix == "ABS_RES") {
		allAbstractResources()[the_index] = body;
	}
	else if(prefix == "ACT") {
		allActivityTypes()[the_index] = body;
	}
}

void parse_json_characterizations(json_object* characterizations) {
	assert(json_object_get_type(characterizations) == json_type_array);
	int	M = json_object_array_length(characterizations);
	for(int j = 0; j < M; j++) {
		json_object*	one_characterization = json_object_array_get_idx(characterizations, j);
		parse_json_characterization(one_characterization);
	}
}

void parse_timelines(
		json_object*	timelines) {
	assert(json_object_get_type(timelines) == json_type_array);
	int	L = json_object_array_length(timelines);
	for(int i = 0; i < L; i++) {
		json_object*	timeline = json_object_array_get_idx(timelines, i);
		assert(json_object_get_type(timeline) == json_type_object);

		json_object*	durative_events = NULL;
		json_object_object_get_ex(timeline, "durativeEvents", &durative_events);
		if(durative_events) {
			assert(json_object_get_type(durative_events) == json_type_array);
			int M = json_object_array_length(durative_events);

			for(int j = 0; j < M; j++) {
				json_object* durative_event = json_object_array_get_idx(durative_events, j);
				assert(json_object_get_type(durative_event) == json_type_object);

				// first get the sequence number, id_index
				json_object*	id;
				json_object_object_get_ex(durative_event, "id", &id);
				assert(json_object_get_type(id) == json_type_string);
				string	id_as_string = json_object_get_string(id);
				int	id_prefix_length = id_as_string.find("::");
				assert(id_prefix_length != string::npos);
				string	object_type = id_as_string.substr(0, id_prefix_length);
				string	object_id = id_as_string.substr(id_prefix_length + 2);
				id_prefix_length = object_id.find("::");
				assert(id_prefix_length != string::npos);
				string	id_index_as_string = object_id.substr(0, id_prefix_length);
				int	id_index = atoi(id_index_as_string.c_str());

				// then get the AAF snippet
				json_object*	start_obj;
				json_object_object_get_ex(durative_event, "start", &start_obj);
				assert(json_object_get_type(start_obj) == json_type_object);
				json_object*	constraints;
				json_object_object_get_ex(start_obj, "constraints", &constraints);
				int	N = json_object_array_length(constraints);
				assert(N == 1);
				json_object*	constraint = json_object_array_get_idx(constraints, 0);
				assert(json_object_get_type(constraint) == json_type_object);
				json_object*	documentation;
				json_object_object_get_ex(constraint, "documentation", &documentation);
				allInstances()[id_index] = json_object_get_string(documentation);
			}
		}
		// to do: read resource histories
	}
}

void parse_behaving_elements(
		json_object*	Element) {
	assert(json_object_get_type(Element) == json_type_array);
	int	L = json_object_array_length(Element);
	for(int i = 0; i < L; i++) {
		json_object*	item = json_object_array_get_idx(Element, i);
		assert(json_object_get_type(item) == json_type_object);
		json_object*	id;
		json_object_object_get_ex(item, "id", &id);
		assert(json_object_get_type(id) == json_type_string);
		string	id_as_string = json_object_get_string(id);
		int	id_prefix_length = id_as_string.find("::");
		assert(id_prefix_length != string::npos);
		string	object_type = id_as_string.substr(0, id_prefix_length);
		string	object_id = id_as_string.substr(id_prefix_length + 2);
		if(object_type == "SUBSYSTEM") {
			// debug
			cerr << "found SUBSYSTEM " << object_id << "\n";
			json_object*	property_groups;
			json_object*	constraints;
			json_object_object_get_ex(item, "propertyGroups", &property_groups);
			parse_json_subsystem_property_groups(object_type, object_id, property_groups);
			json_object_object_get_ex(item, "constraints", &constraints);
			parse_json_subsystem_constraints(object_type, object_id, constraints);
			json_object*	characterizations;
			json_object_object_get_ex(item, "elementBehaviorCharacterizations", &characterizations);
			parse_json_characterizations(characterizations);
		}
		else if(object_type == "RES") {
			json_object*	property_groups;
			json_object_object_get_ex(item, "propertyGroups", &property_groups);
			json_object*	id;
			json_object_object_get_ex(item, "id", &id);
			const char*	id_as_chars = json_object_get_string(id);
			string		id_as_string(id_as_chars);
			int		semicolons = id_as_string.find("::");
			string		sub_id = id_as_string.substr(semicolons + 2);
			int		semicolons2 = sub_id.find("::");
			string		res_index_as_string = sub_id.substr(0, semicolons2);
			int		res_index = atoi(res_index_as_string.c_str());
			parse_json_resource_property_groups(res_index, property_groups);
		}

	}
}

OPEN_FILErequest* fileReader::parse_json_imce(
		const Cstring& data_to_parse) {
	fileParser&			conv(fileParser::currentParser());
	const char*			theJsonString = *data_to_parse;
	Cstring				any_errors;
	enum json_tokener_error		jsonError;
	json_object*			top_level = json_tokener_parse_verbose(
							theJsonString,
							&jsonError);
	if(!top_level) {
		any_errors = any_errors + "Json parsing error: " + json_tokener_error_desc(jsonError) + "\n";
		throw(eval_error(any_errors));
	}
	assert(json_object_get_type(top_level) == json_type_object);
	struct json_object_iterator	top_iter = json_object_iter_begin(top_level);
	struct json_object_iterator	top_end = json_object_iter_end(top_level);
	while(!json_object_iter_equal(&top_iter, &top_end)) {
		const char*	top_name = json_object_iter_peek_name(&top_iter);
		json_object*	top_data = json_object_iter_peek_value(&top_iter);

		try {
			// so far we only have 2 types of elements: behavingElements and timelines
			if(!strcmp(top_name, "behavingElements")) {
				parse_behaving_elements(top_data);
			}
			else if(!strcmp(top_name, "timelines")) {
				parse_timelines(top_data);
			}
		}
		catch(eval_error Err) {
			any_errors = Err.msg;
			throw(eval_error(any_errors));
		}
		json_object_iter_next(&top_iter);
	}
	map<int, string>::iterator	it;
	stringstream			func_file;
	func_file << "apgen version \"json_imce\"\n";
	for(	it = allFunctions().begin();
		it != allFunctions().end();
		it++) {
		func_file << it->second << "\n";
	}
	for(	it = allConcreteResources().begin();
		it != allConcreteResources().end();
		it++) {
		func_file << it->second << "\n";
	}
	for(	it = allAbstractResources().begin();
		it != allAbstractResources().end();
		it++) {
		func_file << it->second << "\n";
	}
	for(	it = allActivityTypes().begin();
		it != allActivityTypes().end();
		it++) {
		func_file << it->second << "\n";
	}
	for(	it = allInstances().begin();
		it != allInstances().end();
		it++) {
		func_file << it->second << "\n";
	}
	string			func_file_text(func_file.str());

	// debug
	cerr << "\nFunction and Resource File:\n" << func_file_text << "\n";

	OPEN_FILErequest*	req = new OPEN_FILErequest(
					compiler_intfc::CONTENT_TEXT,
					func_file_text.c_str());
	// OPEN_FILErequest::process_middle() will handle these commands
	return req;
}
