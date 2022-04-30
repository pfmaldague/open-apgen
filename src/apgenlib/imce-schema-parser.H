#ifndef _IMCE_SCHEMA_PARSER_H_
#define _IMCE_SCHEMA_PARSER_H_

#include <json.h>

class imce_schema {
public:
	json_object*	top_level_object;
	json_object*	properties;
	const char*	string_to_parse;

	imce_schema(const char* F);
	~imce_schema();

	static const char*	spell_json_type(json_object* O);
	void			process_top_level();
	void			process_properties();
};

#endif /* _IMCE_SCHEMA_PARSER_H_ */
