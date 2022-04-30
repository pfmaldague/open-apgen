#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "TOL_write.H"

void apply_xslt_filter_to_doc_and_output(
		xmlDocPtr	doc,
		const char*	xslt_filter_file,
		FILE*		fout) {
	xmlDocPtr		res;
	xsltStylesheetPtr	cur = NULL;
	const char*		params[1] = { NULL };

	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;
	cur = xsltParseStylesheetFile((const xmlChar*)xslt_filter_file);
	res = xsltApplyStylesheet(cur, doc, params);
	xsltSaveResultToFile(fout, res, cur);
	xsltFreeStylesheet(cur);
	xmlFreeDoc(res);
}

//
// TOL globals
//
aoString*	publisher::theStream = NULL;
FILE*		publisher::theTOLFile = NULL;
FILE*		publisher::theAuxiliaryFile = NULL;
bool		publisher::all_acts_visible = false;

		//
		// tells the TOL publisher to publish
		// an XMLTOL in parallel:
		//
bool		publisher::dual_flag = false;

//
// XMLTOL globals
//
xmlpp::Document*	publisher::theDocument = NULL;
xmlpp::Element*		publisher::theElement = NULL;
xmlpp::Element*		publisher::theMetadata = NULL;
bool			publisher::first_time_writing = true;
FILE*			publisher::theOutputFile = NULL;
const char*		publisher::timesystem = NULL;
CTime_base		publisher::offset(0, 0, false);
double			publisher::multiplier = 0.0;

publisher::publisher(   ActivityInstance*   A,
			CTime_base	    time,
			publisher_type	    act_type,
			pinfotlist&	    param_info)
	: time_tag(time),
		unreleased(false),
		activity(A),
		type(act_type),
		Rnode(NULL),
		Pinfo(&param_info) {}

publisher::publisher()
	: type(TOL_META),
		unreleased(false),
		activity(NULL),
		Rnode(NULL),
		violation(NULL),
		Pinfo(NULL) {}

publisher::publisher(   const Cstring&	  name,
			const TypedValue& V,
			CTime_base	  time_of_event,
			Rsource*	  R)
	: time_tag(time_of_event),
		unreleased(false),
		Val(V),
		Rnode(R),
		type(TOL_RES_VAL),
		activity(NULL),
		Pinfo(NULL) {}

publisher::publisher(Rsource* R)
	: activity(NULL),
		unreleased(false),
		type(TOL_META),
		Rnode(R),
		Pinfo(NULL)
		{}

publisher::publisher(con_violation*	V,
		     bool		unrelease_status)
	: time_tag(V->getKey().getetime()),
		violation(V),
		unreleased(unrelease_status),
		type(TOL_VIO),
		activity(NULL),
		Rnode(NULL),
		Pinfo(NULL) {}

publisher::publisher(const publisher& T)
	: type(T.type),
		unreleased(T.unreleased),
		Rnode(T.Rnode),
		activity(T.activity),
		violation(T.violation),
		resource_id(T.resource_id),
		Val(T.Val),
		time_tag(T.time_tag),
		Pinfo(T.Pinfo) {}

#ifdef have_xmltol
xml_publisher::xml_publisher() {}

//
// activity
//
xml_publisher::xml_publisher(   ActivityInstance*   A,
				CTime_base	    time,
				publisher_type	    act_type,
				pinfotlist&	    param_info)
	: publisher(A, time, act_type, param_info) {}

//
// resource value
//
xml_publisher::xml_publisher(   const Cstring&	  name,
				const TypedValue& V,
				CTime_base	  time_of_event,
				Rsource*	  R)
	: publisher(name, V, time_of_event, R) {
	resource_id = R->get_container()->get_key();
	if(R->get_container()->is_array()) {
		resource_indices = R->indices;
	}
}

//
// resource metadata
//
xml_publisher::xml_publisher(Rsource* R)
	: publisher(R) {
	resource_id = R->get_container()->get_key();
	if(R->get_container()->is_array()) {
		resource_indices = R->indices;
	}
}

//
// constraints
//
xml_publisher::xml_publisher(con_violation* V,
			     bool	    unrelease_status)
	: publisher(V, unrelease_status) {}

xml_publisher::xml_publisher(const tol_publisher& T)
	: publisher(T) {
    if(Rnode) {
	resource_id = Rnode->get_container()->get_key();
	if(Rnode->get_container()->is_array()) {
	    resource_indices = Rnode->indices;
	}
    }
}

xml_publisher::xml_publisher(const xml_publisher& T)
	: publisher(T) {
	if(Rnode && Rnode->get_container()->is_array()) {
		resource_indices = Rnode->indices;
	}
}

void
xml_publisher::write_to_stream(
		tol_output_flag transient_flag,
		CTime_base*	bounds) {
    CTime_base	      tm(time_tag);
    xmlpp::Element*	 record_el;
    xmlpp::Element*	 time_el;
    xmlpp::Element*	 content;
    xmlpp::Element*	 resname;
    xmlpp::Element*	 rulename;
    xmlpp::Element*	 message;
    xmlpp::Element*	 unique_id;
    char		 buf[30];

    switch(type) {
	case TOL_ACT_START:
	    // the activity knows its start time, which it should use as a time tag
	    activity->transfer_to_xmlpp(
				theElement,
				all_acts_visible,
				bounds,
				timesystem);
	    break;
	case TOL_ACT_END:
	    {
	    CTime_base start_time(activity->getetime());
	    CTime_base duration(activity->get_timespan());
	    CTime_base end_time = start_time + duration;
	    record_el = theElement->add_child_element("TOLrecord");
	    time_el = record_el->add_child_element("TimeStamp");
	    if(timesystem) {
		time_el->set_attribute(timesystem, *convert_to_timesystem(end_time));
	    }
	    content = record_el->add_child_element("ActivityID");

	    record_el->set_attribute("type", "ACT_END");
	    time_el->add_child_text(*end_time.to_string());
	    content->add_child_text(*activity->get_unique_id());
	    }
	    break;
	case TOL_RES_VAL:
	    record_el = theElement->add_child_element("TOLrecord");
	    time_el = record_el->add_child_element("TimeStamp");
	    content = record_el->add_child_element("Resource");
	    resname = content->add_child_element("Name");
	    if(transient_flag == TOL_FINAL) {

		RES_state*	      state_res = dynamic_cast<RES_state*>(Rnode);
		record_el->set_attribute("type", "RES_FINAL_VAL");

		if(state_res) {

		    //
		    // we need to handle state resources differently
		    //
		    state_state     S = state_res->get_resource_state_at(time_tag);
		    xmlpp::Element* state_el = content->add_child_element("State");

		    if(S.currently == apgen::STATE_STATE::RESET) {
				state_el->set_attribute("res_state", "RESET");
		    } else if(S.currently == apgen::STATE_STATE::SET) {
				// set value is the current value
				state_el->set_attribute("res_state", "SET");
		    } else if(S.currently == apgen::STATE_STATE::USE_SET_PENDING) {
				xmlpp::Element* state_value = state_el->add_child_element("ValueSetTo");
				state_el->set_attribute("res_state", "USE_SET_PENDING");
				state_el->set_attribute("usage_depth", *Cstring(S.end_uses.get_length()));
				S.setval.print_to_xmlpp(state_value);
		    } else if(S.currently == apgen::STATE_STATE::USE_RESET_PENDING) {
				state_el->set_attribute("res_state", "USE_RESET_PENDING");
				state_el->set_attribute("usage_depth", *Cstring(S.end_uses.get_length()));
		    }
		}
	    } else {
		record_el->set_attribute("type", "RES_VAL");
	    }
	    time_el->add_child_text(*tm.to_string());
	    if(timesystem) {
		time_el->set_attribute(timesystem, *convert_to_timesystem(tm));
	    }
	    resname->add_child_text(*resource_id);
	    {
	    emptySymbol*	    es;
	    long		    i = 0;
	    xmlpp::Element*	 resindex;

	    for(int z = 0; z < resource_indices.size(); z++) {

		char    buf[30];
		sprintf(buf, "%ld", i);
		i++;
		resindex = content->add_child_element("Index");
		resindex->set_attribute("level", buf);
		resindex->add_child_text(*resource_indices[z]);
	    }
	    }
	    Val.print_to_xmlpp(content);
	    break;
	case TOL_VIO:
	    record_el = theElement->add_child_element("TOLrecord");
	    time_el = record_el->add_child_element("TimeStamp");
	    content = record_el->add_child_element("Rule");
	    rulename = content->add_child_element("Name");
	    message = record_el->add_child_element("Message");
	    unique_id = record_el->add_child_element("ID");
	    sprintf(buf, "%d", violation->ID);
	    unique_id->add_child_text(buf);

	    tm = violation->getetime();
	    time_el->add_child_text(*tm.to_string());
	    if(timesystem) {
		time_el->set_attribute(timesystem, *convert_to_timesystem(tm));
	    }
	    if(unreleased) {
		record_el->set_attribute("type", "UNRELEASED");
	    } else {
		switch(violation->payload.severity) {
		    case Violation_info::WARNING :
			record_el->set_attribute("type", "WARNING");
			break;
		    case Violation_info::ERROR :
			record_el->set_attribute("type", "ERROR");
			break;
		    case Violation_info::RELEASE :
			record_el->set_attribute("type", "RELEASE");
			break;
		}
	    }
	    rulename->add_child_text(*violation->payload.parent_constraint->get_key());
	    message->add_child_text(*violation->payload.message);
	    break;
	case TOL_META:
	    content = theMetadata->add_child_element("ResourceSpec");
	    resname = content->add_child_element("Name");
	    resname->add_child_text(*resource_id);
	    {

	    // handle index if necessary

	    emptySymbol*	es;
	    long		i = 0;
	    xmlpp::Element*	resindex;
	    Rsource*	rs = Rnode;

	    for(int z = 0; z < resource_indices.size(); z++) {
		char    buf[30];
		sprintf(buf, "%ld", i);
		i++;
		resindex = content->add_child_element("Index");
		resindex->set_attribute("level", buf);
		resindex->add_child_text(*resource_indices[z]);
	    }

	    //
	    // handle important attributes:
	    //
	    // 		Units,
	    // 		Interpolation,
	    // 		Max,
	    // 		Min,
	    // 		data
	    // 		type,
	    // 		subsystem
	    //
	    // Note: In APGenX internals, Min and Max values may
	    // change with time in which case the value reported
	    // here is useless.
	    //
	    bool	maxSym_exists  = rs->Object->defines_property("maximum");
	    bool	minSym_exists  = rs->Object->defines_property("minimum");
	    bool	units_exists   = rs->Object->defines_property("units");
	    bool	linear_exists  = rs->Object->defines_property("interpolation");
	    bool	linear_is_nonzero = false;
	    bool	subsys_exists  = rs->Object->defines_property("subsystem");

	    TypedValue	     maxSym;
	    TypedValue	     minSym;
	    TypedValue	     units;
	    TypedValue	     linear;
	    TypedValue	     subsys;

	    xmlpp::Element*	 el;

	    el = content->add_child_element("DataType");
	    el->add_child_text(*spell(Rnode->get_datatype()));

	    //
	    // add list of possible states for state resources
	    //
	    RES_state*      state_res = dynamic_cast<RES_state*>(Rnode);
	    if(state_res) {
		xmlpp::Element*	 possible_states
			= content->add_child_element("PossibleStates");

		for(int si = 0; si < state_res->rangevec.size(); si++) {
		    state_res->rangevec[si].print_to_xmlpp(possible_states);
		}
	    }

	    el = content->add_child_element("Units");
	    if(units_exists) {
		units = (*rs->Object)["units"];
		el->add_child_text(*units.get_string());
	    }

	    el = content->add_child_element("Interpolation");

	    if(linear_exists) {
		linear = (*rs->Object)["interpolation"];
		if(linear.get_int()) {
		    linear_is_nonzero = true;
		}
	    }
	    if((linear_exists && linear_is_nonzero)) {
		el->add_child_text("linear");
	    } else {
		el->add_child_text("constant");
	    }

	    el = content->add_child_element("Maximum");
	    if(maxSym_exists) {
		maxSym = (*rs->Object)["maximum"];
		maxSym.print_to_xmlpp(el);
	    }
	    el = content->add_child_element("Minimum");
	    if(minSym_exists) {
		minSym = (*rs->Object)["minimum"];
		minSym.print_to_xmlpp(el);
	    }
	    el = content->add_child_element("Subsystem");
	    if(subsys_exists) {
		subsys = (*rs->Object)["subsystem"];
		el->add_child_text(*subsys.get_string());
	    }
	    }
	    break;
	default:
	    break;
    }
}

bool
xml_publisher::test_for_xml_tol_element(const char* s) {
	const char* u = s;
	const char* v = "<XML_TOL>\n";
	while(*u && *u == *v) {
		u++;
		v++;
	}
	if(*v) return false;
	return true;
}

void
xml_publisher::write_everything_you_got_up_to_this_point() {
	std::string xml_tol = theDocument->write_to_string_formatted();
	const char* s = xml_tol.c_str();
	const char* orig_string = s;


	// debug
	// cout << "xml_publisher: incremental XML string has length " << strlen(s) << "\n";

	if(first_time_writing) {
		// keep the header stuff
		first_time_writing = false;
	} else {
		// skip over the initial stuff until we find the <XML_TOL> element
		while(*s && !test_for_xml_tol_element(s)) {
			s++;
		}
		if(*s && strlen(s) > strlen("<XML_TOL>\n")) {
			s += strlen("<XML_TOL>\n");
		}
		// debug
		// cout << "     skipping " << (s - orig_string)
		//   << " characters at beginning of increlemental XML string...\n";
	}
	if(*s) {
		// assert(strlen(s) - strlen("</XML_TOL>\n") > 0);
		// debug
		// cout << "     adding " << (strlen(s) - strlen("</XML_TOL>\n")) << " bytes to the XMLTOL file...\n";
		fwrite(s, strlen(s) - strlen("</XML_TOL>\n"), 1, theOutputFile);
	}

	delete theDocument;

	theDocument = new xmlpp::Document;
	theElement = theDocument->create_root_node("XML_TOL");
	// theMetadata = theElement->add_child_element("ResourceMetadata");
}

void
xml_publisher::handle_globals(TypedValue* incon_globals) {
    xmlpp::Element*	propagated
	= theElement->add_child_element("PropagatedGlobals");
    ListOVal*		lov = &incon_globals->get_array();      
    ArrayElement*	ae;

    for(int k = 0; k < lov->get_length(); k++) {
	ae = (*lov)[k];
	if(ae->Val().is_string()) {
	    try {
		TypedValue&     one_global
		    = globalData::get_symbol(ae->Val().get_string());
		xmlpp::Element* one_global_el
		    = propagated->add_child_element("GlobalVariable");
		one_global_el->set_attribute("Name", *ae->Val().get_string());
		one_global.print_to_xmlpp(one_global_el);
	    } catch(eval_error Err) {
		cout << "xml_publisher::handle_globals: could not find global "
		    << ae->Val().get_string() << "\n";
	    }
	}
    }
}

#endif /* have_xmltol */

void tol_publisher::write_end_of_metadata() {
	(*theStream) << "$$EOH\n";
}

tol_publisher::tol_publisher(
		ActivityInstance* A,
		CTime_base time,
		publisher_type act_type,
		pinfotlist& param_info)
	: publisher(A, time, act_type, param_info) {}

tol_publisher::tol_publisher(
		const Cstring&		name,
		const TypedValue&	V,
		CTime_base		time_of_event,
		Rsource*		R)
	: publisher(name, V, time_of_event, R) {
	resource_id = R->name;
}

tol_publisher::tol_publisher(
		Rsource*	   R)
	: publisher(R) {
	resource_id = R->name;
}

tol_publisher::tol_publisher(
		con_violation*	V,
		bool		unrelease_status)
	: publisher(V, unrelease_status) {}

tol_publisher::tol_publisher(const tol_publisher& T)
	: publisher(T) {}

tol_publisher::tol_publisher() {}

void
tol_publisher::write_to_stream(
		tol_output_flag transient_flag,
		CTime_base*	bounds) {
    CTime_base	      tm(time_tag);

    switch(type) {
	case TOL_ACT_START:
	    activity->transfer_to_stream(
		*theStream,
		apgen::FileType::FT_TOL,
		bounds,
		Pinfo
		);
	    break;
	case TOL_ACT_END:
	    {
	    CTime_base start_time(activity->getetime());
	    CTime_base duration(activity->get_timespan());
	    CTime_base end_time = start_time + duration;
	    *theStream << end_time.to_string() << ",ACT_END," << activity->get_unique_id() << ";\n";
	    }
	    break;
	case TOL_RES_VAL:
	    *theStream << tm.to_string() << ",";
	    if(transient_flag == TOL_TRANSIENT) {
		*theStream << "TRANSIENT,";
	    } else {
		*theStream << "RES,";
	    }
	    *theStream << resource_id << "=";
	    Val.print(	*theStream,
			/* indentation = */ 0,
			MAX_SIGNIFICANT_DIGITS,
			/* one_line = */ true);
	    *theStream << ";\n";
	    break;
	case TOL_VIO:
	    tm = violation->getetime();
	    *theStream << tm.to_string() << ",";
	    if(unreleased) {
		*theStream << "UNRELEASED,";
	    } else {
		switch(violation->payload.severity) {
		    case Violation_info::WARNING :
			*theStream << "WARNING,";
			break;
		    case Violation_info::ERROR :
			*theStream << "ERROR,";
			break;
		    case Violation_info::RELEASE :
			*theStream << "RELEASE,";
			break;
		}
	    }
	    *theStream << violation->payload.parent_constraint->get_key() << ",";
	    *theStream << addQuotes(violation->payload.message) << ";\n";
	    break;
	case TOL_META:
	    {

	    //
	    // Set up the main array for this resource. The format of the
	    // metadata will be
	    //
	    // metadata:
	    // 	  <res name> = ["<att 1>" = <val>, "<att 2>" = <val>, ...],
	    // 	  <res name> = ["<att 1>" = <val>, "<att 2>" = <val>, ...],
	    // 	  ...
	    //
	    //
	    // To maintain compatibility with the existing parsetol.py utility
	    // in the OPS repository, no semicolons are used except of course
	    // any semicolons inside strings. The python TOL parser reads
	    // strings correctly, so hopefully it could be extended to include
	    // metadata as well.
	    //
	    // Note also that the resource name will just be a string, unlike the
	    // XMLTOL case where the indices are broken down into individual
	    // elements. It is therefore useful to include a special attribute
	    // with an impossible-to-conflict-with name: "contained in".
	    //
	    ListOVal*	lov = new ListOVal;

	    //
	    // This is the metadata for just one resource
	    //
	    TypedValue	metadata;
	    metadata = *lov;

	    //
	    // For convenience when using operator<<()
	    //
	    ListOVal&	L = *lov;
	    Rsource*	rs = Rnode;

	    //
	    // Take care of container attribute
	    //
	    RCsource*	container = rs->get_container();
	    TypedValue	att_val;
	    att_val = container->get_key();
	    L.add(new ArrayElement("contained in", att_val));
	    
	    //
	    // handle important attributes:
	    // 	Units, Interpolation, Max, Min, data type,
	    // 	subsystem, absolute and relative tolerances
	    //
	    bool	maxSym_exists  = rs->Object->defines_property("maximum");
	    bool	minSym_exists  = rs->Object->defines_property("minimum");
	    bool	units_exists   = rs->Object->defines_property("units");
	    bool	linear_exists  = rs->Object->defines_property("interpolation");
	    bool	linear_is_nonzero = false;
	    bool	subsys_exists  = rs->Object->defines_property("subsystem");
	    bool	rel_tolerance_exists = rs->Object->defines_property("min_rel_delta");
	    bool	abs_tolerance_exists = rs->Object->defines_property("min_abs_delta");

	    TypedValue	maxSym;
	    TypedValue	minSym;
	    TypedValue	units;
	    TypedValue	linear;
	    TypedValue	subsys;
	    TypedValue	min_abs_tol;
	    TypedValue	min_rel_tol;

	    RES_settable*	rset	= dynamic_cast<RES_settable*>(rs);
	    RES_state*		rstate	= dynamic_cast<RES_state*>(rs);
	    RES_consumable*	rcons	= dynamic_cast<RES_consumable*>(rs);
	    RES_nonconsumable*	rnon	= dynamic_cast<RES_nonconsumable*>(rs);
	    if(rset) {
		att_val = "settable";
	    } else if(rstate) {
		att_val = "state";
	    } else if(rcons) {
		att_val = "consumable";
	    } else if(rnon) {
		att_val = "nonconsumable";
	    }
	    L.add(new ArrayElement("Type", att_val));
	    att_val = spell(rs->get_datatype());
	    L.add(new ArrayElement("DataType", att_val));

	    // add list of possible states for state resources
	    RES_state*      state_res = dynamic_cast<RES_state*>(rs);
	    if(state_res) {
		ListOVal*	l2 = new ListOVal;

		for(int si = 0; si < state_res->rangevec.size(); si++) {
		    ArrayElement* ae = new ArrayElement(si);
		    l2->add(ae);
		    ae->SetVal(state_res->rangevec[si]);
		}
		att_val = *l2;
		L.add(new ArrayElement("PossibleStates", att_val));
	    }

	    if(units_exists) {
		units = (*rs->Object)["units"];
		L.add(new ArrayElement("Units", units));
	    }
	    if(linear_exists) {
		linear = (*rs->Object)["interpolation"];
		if(linear.get_int()) {
		    linear_is_nonzero = true;
		}
	    }

	    if((linear_exists && linear_is_nonzero)) {
		att_val = "linear";
	    } else {
		att_val = "constant";
	    }
	    L.add(new ArrayElement("Interpolation", att_val));

	    if(maxSym_exists) {
		maxSym = (*rs->Object)["maximum"];
		L.add(new ArrayElement("Maximum", maxSym));
	    }
	    if(minSym_exists) {
		minSym = (*rs->Object)["minimum"];
		L.add(new ArrayElement("Minimum", minSym));
	    }
	    if(subsys_exists) {
		subsys = (*rs->Object)["subsystem"];
		L.add(new ArrayElement("Subsystem", subsys));
	    } else {
		subsys = "Default";
		L.add(new ArrayElement("Subsystem", subsys));
	    }
	    if(rel_tolerance_exists) {
		min_rel_tol = (*rs->Object)["min_rel_delta"];
		L.add(new ArrayElement("Min Rel Delta", min_rel_tol));
	    }
	    if(abs_tolerance_exists) {
		min_abs_tol = (*rs->Object)["min_abs_delta"];
		L.add(new ArrayElement("Min Abs Delta", min_abs_tol));
	    }

	    //
	    // It remains to print L to the stream.
	    //
	    (*theStream) << rs->name << "=";
	    metadata.print(*theStream,
			   /* indentation = */ 0,
			   MAX_SIGNIFICANT_DIGITS,
			   /* one_line = */ true);

	    //
	    // Note: we use a comma instead of a semicolon to keep
	    // things compatible with the python TOL file reader
	    //
	    (*theStream) << ",\n";
	    }
	    break;
	default:
	    break;
    }
}
