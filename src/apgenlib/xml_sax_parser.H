#ifndef _XML_SAX_PARSER_H_
#define _XML_SAX_PARSER_H_

#ifdef have_xmltol

#include <stdio.h>
#include <iostream>
#include <string.h>

#include "generic_parser.H"
#include <RES_exceptions.H>
#include <APdata.H>

class XmlTolSaxParser;

class xml_reader : public generic_parser {
public:
	xml_reader(int (*s)(long))
		: gimme_a_char(s),
		ptr_to_sax_parser(NULL) {}

	~xml_reader();

	int		(*gimme_a_char)(long);
	Cstring		parser_name() override { return "XML SAX activity parser"; }
	XmlTolSaxParser* ptr_to_sax_parser;

	void		parse();

			// Methods needed by template client:
	bool		verify_that_string_is_an_array(Cstring& errors) override;
	bool		iterate(	bool&	isAnActivity,
					bool&	isAResource,
					bool&	isAConstraint,
					int*	lineno) override;
	Cstring		act_get_id() override;
	Cstring		act_get_type() override;
	bool		can_extract_values() override { return true; }
	void		act_get_attribute_values(tvslist&) override;
	void		act_get_attribute_strings(pairslist&) override;
	Cstring		act_get_name(pairtlist&) override;
	bool		act_has_parent() override;
	Cstring		act_get_parent(bool& abstractable, bool& abstracted) override;
	void		act_get_parameter_values(tvslist&) override;
	void		act_get_parameter_strings(pairtlist&) override;
	Cstring		res_get_name() override;
	TypedValue	res_get_value() override;
	bool		res_has_indices(pairslist& indices) override;
	CTime_base*	time_tag() override;
			// end of methods needed by template client

	Cstring		act_id;
	Cstring		act_type;
	tvslist		attributes_values;
	Cstring		act_name;
	Cstring		act_parent;
	tvslist		parameter_values;
	Cstring		resource_name;
	pairslist	resource_indices;

	};

#endif /* have_xmltol */

#endif /* _XML_SAX_PARSER_H_ */
