#ifndef _XML_ACT_PARSER_H_
#define _XML_ACT_PARSER_H_

#ifdef have_xmltol

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <libxml++/libxml++.h>

// pure virtual class
class plan_parser {
public:
	plan_parser(const char* input_string) {}
	virtual ~plan_parser() {}

				// ex.: "IMCE-style JSON parser", "TOL_XML parser"
	virtual Cstring		parser_name() = 0;
				/* checks that at the highest level, the data
				 * encoded in the string (e. g. as XML or JSON)
				 * represent an array of items: */
	virtual bool		verify_that_string_is_an_array(Cstring& errors) = 0;
				/* The first call to iterate() should parse the
				 * input string till the end of the first time-
				 * based record. Subsequent calls should
				 * retrieve successive time-based records until
				 * there is none left. */
	virtual bool		iterate(	bool&	isAnActivity,
						bool&	isAResource,
						bool&	isAConstraint) = 0;
				/* tests whether this parser can return values
				 * as TypedValues, or as strings. Only the old
				 * JSON reader returns values as strings at
				 * this point (Sept. 2016). */
	virtual bool		can_extract_values() = 0;

				/* some info in the file is time-independent metadata;
				 * typically, most of the information is in the form of
				 * time-based records. The time tag should be NULL for
				 * time-independent data. */
	virtual CTime_base*	time_tag() = 0;

	// ACTIVITY-centric methods

	// for use if can_extract_values() returns false:
	virtual void		act_get_attribute_strings(pairslist&) = 0;
	virtual void		act_get_parameter_strings(pairtlist&) = 0;
	// for use if can_extract_values() returns true:
	virtual void		act_get_attribute_values(tvslist&) = 0;
	virtual void		act_get_parameter_values(tvslist&) = 0;

	virtual Cstring		act_get_type() = 0;
	virtual Cstring		act_get_name(pairtlist&) = 0;
	virtual bool		act_has_parent() = 0;
	virtual Cstring		act_get_parent(bool& abstractable, bool& abstracted) = 0;
	virtual Cstring		act_get_id() = 0;

	// RESOURCE-centric methods

	virtual Cstring		res_get_name() = 0;
	virtual TypedValue	res_get_value() = 0;
	virtual bool		res_has_indices(pairslist& indices) = 0;
};

#endif /* have_xmltol */

#endif /* _XML_ACT_PARSER_H_ */
