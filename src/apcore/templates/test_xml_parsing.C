#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef have_xmltol

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <libxml++/libxml++.h>


#ifndef LIBXMLCPP_EXCEPTIONS_ENABLED
#define LIBXMLCPP_EXCEPTIONS_ENABLED
#endif

#include <assert.h>

int test_xml_usage(char* s) {
	fprintf(stderr, "Usage: %s <XML file>\n", s);
	return -1; }

/*	useful reference for libxml++ programming:
 *	/usr/share/doc/libxml++-devel-2.30.0/examples/dom_build/main.cc
 */

int test_xml_parsing(int argc, char* argv[]) {

	/* we expect the following arguments:
	 *
	 *    - the name of an XML file to parse
	 *
	 *    - whether or not to validate, using DTD reference in
	 * 	the XML file
	 *
	 *    - optionally, the name of an .xsd file decribint the schema
	 *      to use for validation; if such a name is provided, the program
	 *      assumes that the XML file should be validated using this
	 *      schema
	 */

	if(argc < 2 || argc > 4) {
		return test_xml_usage(argv[0]); }

	char*			xml_file = argv[1];
	char*			schema_file = NULL;
	bool			should_validate = false;
	xmlpp::Document*	document = NULL;

	if(argc >= 3) {
		if(!strcmp(argv[2], "validate")) {
			should_validate = true; }
		if(argc == 4) {
			schema_file = argv[3]; } }
		
	xmlpp::DomParser	dom_parser(xml_file, /* validate = */ should_validate);

	document = dom_parser.get_document();
	if(schema_file) {
#ifdef HAVE_SCHEMA_VALIDATOR
		xmlpp::DomParser	schema_parser(schema_file, false);
		xmlpp::SchemaValidator	validator(*schema_parser.get_document());

		if(validator.validate(document)) {
			printf("Document %s is valid for schema %s.\n", xml_file, schema_file);
			return 0; }
		else {
			printf("Document %s is not valid for schema %s.\n", xml_file, schema_file);
			return -1; }
#else
		printf("Schema validator not implemented on this platform\n");
		return -1;
#endif /* HAVE_SCHEMA_VALIDATOR */
		}
	const xmlpp::Node*	top_level_obj = dom_parser.get_document()->get_root_node();
	if(!top_level_obj) {
		std::cout << "top-level node not found\n";
		return -1; }
	const xmlpp::Element*		topElement;
	const xmlpp::Element*		an_element;
	topElement = dynamic_cast<const xmlpp::Element*>(top_level_obj);
	xmlpp::Node::const_NodeList::iterator	mainIterator;
	xmlpp::Node::const_NodeList		list;

	if(topElement) {
		const Glib::ustring	nodename = top_level_obj->get_name();
		const Glib::ustring	namespace_prefix = top_level_obj->get_namespace_prefix();
		int			record_index = 0;

		list = top_level_obj->get_children();
		if(namespace_prefix.empty()) {
			; }
		mainIterator = list.begin();
		const xmlpp::Node*	a_node;
	 	while((mainIterator != list.end())) {
			a_node = *mainIterator;
			an_element = dynamic_cast<const xmlpp::Element*>(a_node);
			if(an_element) {
				const Glib::ustring	nodename = an_element->get_name();
				const Glib::ustring	namespace_prefix = an_element->get_namespace_prefix();
				std::cout << "Element " << nodename << "\n";
				if(nodename == "OrbiterRequest") {
					const xmlpp::Attribute*	hail_start_time = an_element->get_attribute("HailStartTime");
					const xmlpp::Attribute*	hail_duration = an_element->get_attribute("HailDuration");
					const xmlpp::Attribute*	request_type = an_element->get_attribute("RequestType");
					if(request_type->get_value() == "request") {
						std::cout << "    got a request\n"; }
					else {
						std::cout << "    got " << request_type->get_value() << "\n"; }
					}
				}
			++mainIterator; } }
	else {
		std::cout << "top-level object is not an element\n";
		return -1; }

	// normal return
	return 0; }


#endif /* have_xmltol */
