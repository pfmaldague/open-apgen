#ifndef _JSON_ACT_PARSER_H_
#define _JSON_ACT_PARSER_H_

#include 	"json.h"
#include	"aafReader.H"
#include	"generic_parser.H"

class json_act_parser: public generic_parser {
private:
	json_object*	top_level_object;
	json_object*	the_array;
	json_object*	one_act;
	stringslist	context;
	stringslist	save_context;
	int		i;
	stringslist	theAbstractableAncestors;
	stringslist	theAbstractedAncestors;
	bool		got_attributes;
public:
	int		line_where_defined;

	json_act_parser(const char* s)
		: start_parsing_here(s),
			line_where_defined(0),
			top_level_object(NULL),
			the_array(NULL),
			one_act(NULL),
			i(0),
			got_attributes(false) {}

	const char*	start_parsing_here;
	Cstring		parser_name() override { return "Json activity parser"; }

	void		parse();
			// normal return = true
	bool		verify_that_string_is_an_array(Cstring& errors) override;
	bool		iterate(	bool&	isAnAct,
					bool&	isARes,
					bool&	isACons,
					int*	lineno) override;

	bool		can_extract_values() override { return false; }

	CTime_base*	time_tag() override { return NULL; }

	Cstring		act_get_id() override;
	Cstring		act_get_type() override;
	Cstring		act_get_name(pairtlist&) override;
	bool		act_has_parent() override;
	Cstring		act_get_parent(bool& abstractable, bool& abstracted) override;
	void		act_get_attribute_values(tvslist&) override;
	void		act_get_attribute_strings(pairslist&) override;
	void		act_get_parameter_values(tvslist&) override;
	void		act_get_parameter_strings(pairtlist&) override;

	Cstring		res_get_name() override { throw(eval_error("Not implemented")); }
	TypedValue	res_get_value() override { throw(eval_error("Not implemented")); }
	bool		res_has_indices(pairslist& indices) override { throw(eval_error("Not implemented")); }
};

void json_act_parser::parse() {
	enum json_tokener_error	jsonError;
	fileParser&		conv(fileParser::currentParser());

	aafReader::initialize_counts_and_behaviors();
	conv.Instances.clear();
	conv.ListOfActivityPointersIndexedByIDsInThisFile.clear();
	// conv.allRequests.clear();
	// conv.allChameleons.clear();

	top_level_object = json_tokener_parse_verbose(start_parsing_here, &jsonError);
	if(!top_level_object) {
		Cstring errors = "Json parsing error: ";
		errors << json_tokener_error_desc(jsonError) << "\n";
		throw(eval_error(errors)); } }

bool	json_act_parser::verify_that_string_is_an_array(Cstring& errors) {
	json_object*	the_struct;
	json_object*	atl;

	try {
		// will throw of top_level_object is not an object
		context << new emptySymbol("top-level item is an object");
		if(json_object_get_type(top_level_object) != json_type_object) throw(eval_error("top-level item is not an object"));
		context << new emptySymbol("the top-level object contains Timeline Data");
		if(!json_object_object_get_ex(top_level_object, "Timeline Data", &the_array))
			throw(eval_error("Timeline Data key not found in top-level object"));
		if(json_object_get_type(the_array) != json_type_array) throw(eval_error("Timeline Data is not an array")); }
	catch(eval_error Err) {
		stringslist::iterator	error_it(context);
		emptySymbol*		s;

		errors << "Uh Oh: json parsing error\n\tcontext:\n";
		while((s = error_it())) {
			errors << "\t\t" << s->get_key() << "\n"; }
		errors << "\terror:\n" << Err.msg << "\n";
		return false; }
	save_context = context;
	return true; }

bool	json_act_parser::iterate(bool& isAnAct, bool& isARes, bool& isACons, int* lineno) {
	json_object*	insert;

	if(i >= json_object_array_length(the_array)) {
		return false; }
	context = save_context;
	isAnAct = true;
	isARes = false;
	isACons = false;
	try {
		one_act = json_object_array_get_idx(the_array, i);
		if(json_object_get_type(one_act) != json_type_object) throw(eval_error("Array element is not a struct")); }
	catch(eval_error Err) {
		stringslist::iterator	error_it(context);
		emptySymbol*		s;
		Cstring			errors("Uh Oh: json parsing error\n\tcontext:\n");

		while((s = error_it())) {
			errors << "\t\t" << s->get_key() << "\n"; }
		errors << "\terror:\n" << Err.msg << "\n";
		throw(eval_error(errors)); }
	i++;
	theAbstractableAncestors.clear();
	theAbstractedAncestors.clear();
	return true; }

Cstring	json_act_parser::act_get_id() {
	json_object*	act_id;

	context << new emptySymbol("looking for unique id");
	if(!json_object_object_get_ex(
		one_act,
		"Activity Name",
		&act_id)) {
		stringslist::iterator	error_it(context);
		emptySymbol*		s;
		Cstring			errors("Uh Oh: json parsing error\n\tcontext:\n");

		while((s = error_it())) {
			errors << "\t\t" << s->get_key() << "\n"; }
		errors << "\terror: " << "unique id key not found in struct" << "\n";
		throw(eval_error(errors)); }
	return Cstring(json_object_get_string(act_id)); }

Cstring json_act_parser::act_get_type() {
	Cstring		type;
	json_object*	act_type;

	context << new emptySymbol("looking for act type");
	if(!json_object_object_get_ex(one_act, "Activity Type", &act_type)) {
		stringslist::iterator	error_it(context);
		emptySymbol*		s;
		Cstring			errors("Uh Oh: json parsing error\n\tcontext:\n");

		while((s = error_it())) {
			errors << "\t\t" << s->get_key() << "\n"; }
		errors << "\terror: " << "act type key not found in struct" << "\n";
		throw(eval_error(errors)); }
	type = json_object_get_string(act_type);
	return type; }

void	json_act_parser::act_get_attribute_values(tvslist& atts) {}
void	json_act_parser::act_get_parameter_values(tvslist& atts) {}

void	json_act_parser::act_get_attribute_strings(pairslist& atts) {
	json_object*				metadata;
	json_object*				att_name;
	Cstring					attName;
	json_object*				jsonval;

	got_attributes = true;
	context << new emptySymbol("looking for metadata struct");
	if(!json_object_object_get_ex(one_act, "Metadata", &metadata)) throw(eval_error("metadata key not found in struct"));
	if(json_object_get_type(metadata) != json_type_array) throw(eval_error("metadata is not an array"));

	long num_metadata_items = json_object_array_length(metadata);
	long i = 0;
	while(i < num_metadata_items) {
		json_object* keyword_value_pair = json_object_array_get_idx(metadata, i);

		if(json_object_get_type(keyword_value_pair) != json_type_object) {
			throw(eval_error("metadata item is not a keyword-value pair")); }
		if(!json_object_object_get_ex(keyword_value_pair, "Name", &att_name)) {
			throw(eval_error("keyword-value pair has no Name element")); }
		if(!json_object_object_get_ex(keyword_value_pair, "Value", &jsonval)) {
			throw(eval_error("keyword-value pair has no Value element")); }
		attName = json_object_get_string(att_name);

		// must capture hierarchy data first
		if(attName == "AbstractableParent") {
			json_object*	parent = json_object_array_get_idx(jsonval, 0);

			theAbstractableAncestors << new emptySymbol(json_object_get_string(parent)); }
		else if(attName == "AbstractedParent") {
			json_object*	parent = json_object_array_get_idx(jsonval, 0);

			theAbstractedAncestors << new emptySymbol(json_object_get_string(parent)); }
		else {
			Cstring		text;

			if(json_object_get_type(jsonval) == json_type_string) {
				text = json_object_get_string(jsonval); }
			else {
				// must be a number (bool, int, double) - unfortunately, we must re-serialize...
				text = json_object_to_json_string(jsonval); }
			atts << new symNode(attName, text); }

		i++; } }

Cstring	json_act_parser::act_get_parent(bool& is_abstractable, bool& is_abstracted) {
	if(theAbstractableAncestors.get_length()) {
		is_abstractable = true;
		is_abstracted = false;
		return theAbstractableAncestors.first_node()->get_key(); }
	else if(theAbstractedAncestors.get_length()) {
		is_abstractable = false;
		is_abstracted = true;
		return theAbstractedAncestors.first_node()->get_key(); }
	return ""; }

Cstring	json_act_parser::act_get_name(pairtlist& l) {
	symNode*	nameSym = l.find("Name");

	if(!nameSym) {
		throw(eval_error("Name key not found in attributes list")); }
	return nameSym->payload; }

bool	json_act_parser::act_has_parent() {
	if(!got_attributes) {
		throw(eval_error("has_parent(): attributes have not been parsed yet!")); }
	return theAbstractableAncestors.get_length() > 0 || theAbstractedAncestors.get_length() > 0; }

void	json_act_parser::act_get_parameter_strings(pairtlist& paramlist) {
	json_object*				parameters;
	json_object*				att_name;
	json_object*				jsonval;
	Cstring					paramName;

	try {
		/* Nasty problem for parameters: TMS indexes them by name only, no guarantee on order */
		context << new emptySymbol("looking for parameter struct");
		if(!json_object_object_get_ex(one_act, "Activity Parameters", &parameters)) {
			throw(eval_error("Activity Parameters key not found in struct")); }
		if(json_object_get_type(parameters) != json_type_array) {
			throw(eval_error("Activity Parameters is not an array")); } }
	catch(eval_error Err) {
		stringslist::iterator	error_it(context);
		emptySymbol*		s;
		Cstring			errors("Uh Oh: json parsing error\n\tcontext:\n");

		while((s = error_it())) {
			errors << "\t\t" << s->get_key() << "\n"; }
		errors << "\terror:\n" << Err.msg << "\n";
		throw(eval_error(errors)); }

	// iterate over parameters to get name and attributes
	long num_params = json_object_array_length(parameters);
	long i = 0;
	while(i < num_params) {
		json_object* keyword_value_pair = json_object_array_get_idx(parameters, i);

		if(json_object_get_type(keyword_value_pair) != json_type_object) {
			throw(eval_error("parameter item is not a keyword-value pair")); }
		if(!json_object_object_get_ex(keyword_value_pair, "Name", &att_name)) {
			throw(eval_error("keyword-value pair has no Name element")); }
		if(!json_object_object_get_ex(keyword_value_pair, "Value", &jsonval)) {
			throw(eval_error("keyword-value pair has no Value element")); }
		paramName = json_object_get_string(att_name);
		Cstring		text;

		if(json_object_get_type(jsonval) == json_type_string) {
			text = json_object_get_string(jsonval); }
		else {
			// must be a number (bool, int, double) - unfortunately, we must re-serialize...
			text = json_object_to_json_string(jsonval); }
		paramlist << new symNode(paramName, text);
		i++; } }

#endif /* _JSON_ACT_PARSER_H_ */
