#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef have_xmltol
/* if have_xml_reader is defined, we are using the (old and unmaintained)
 * gdome library, now superseded by libxml++ */

#include <iostream>
#include <libxml++/libxml++.h>


#ifndef LIBXMLCPP_EXCEPTIONS_ENABLED
#define LIBXMLCPP_EXCEPTIONS_ENABLED
#endif

/*	useful reference for libxml++ programming:
 *	/usr/share/doc/libxml++-devel-2.30.0/examples/dom_build/main.cc
 */

int test_xml2(int argc, char* argv[]) {

// this ifdef looks pedantic, I will probably remove it soon
#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
try {
#endif

	xmlpp::Document	document;

	/* create_root_node() has 2 more args which default to empty strings:
	 * a URI namespace and a prefix namespace */
	xmlpp::Element*	root_node = document.create_root_node("xml2_test");
	// without this you cannot do the next step:
	root_node->set_namespace_declaration("http://www.w3.org/2001/XMLSchema-instance", "xsi");
	root_node->set_attribute("noNamespaceSchemaLocation", "foo.xsd", "xsi");
	xmlpp::Element*	child_node = root_node->add_child_element("LevelOneDown");
	// third argument defaults to empty namespace string
	child_node->set_attribute("id", "1");
	child_node->add_child_text("Life is short, although at times it is unbearably slow.");
	child_node->add_child_element("Hmm... I doubt it.");
	child_node->add_child_element("grandchild");
	child_node = root_node->add_child_element("examplechild");
	child_node->set_attribute("id", "2");

	std::string whole = document.write_to_string_formatted();
	std::cout << "Test XML file we just created:\n" << whole << "\n";

#ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
}
catch(const std::exception& ex) {
	std::cout << "test_xml2: exception - msg:" << ex.what() << "\n"; }
#endif
	// normal return
	return 0; }


#endif /* have_xmltol */
