#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <unistd.h>

#include "aafReader.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "apDEBUG.H"
#include "RES_exec.H"
#include "EventImpl.H"
#include "RES_def.H"
#include "apcoreWaiter.H"

#ifdef have_xmltol
#include <libxml++/libxml++.h>
#endif /* have_xmltol */

using namespace std;


const Cstring&	actAgent::get_owner_name() const {
	return mySource->get_key();
}

void	actAgent::get_param_info(	int		ind_0,
					Cstring&	p_name,
					Cstring&	p_descr,
					TypedValue&	p_value,
					List&		range,
					apgen::DATA_TYPE& param_type
					) const {
	const task&	T = mySource->Object->Task;	// the constructor for this act type
	int		var_index_for_param;

	if(ind_0 < T.paramindex.size()) {
		var_index_for_param = T.paramindex[ind_0];
		p_name = T.get_varinfo()[var_index_for_param].first;
		param_type = T.get_varinfo()[var_index_for_param].second;
		p_value = (*mySource->Object)[var_index_for_param];
	} else {
		// should be an error
		return;
	}
}

//
// Provides information about a parameter that is an array.
//
void actAgent::get_struct_param_info(
		int				ind_0,		// the index of the parameter
		const Cstring&			prefix,		// client-provided path to parameter or sub-elemetn
		map<string, string>&		names,		// mapping from paths to element names
		map<string, string>&		types,
		map<string, list<string> >&	ranges,
		map<string, string>&		descriptions,
		map<string, string>&		units,
		TypedValue&			actual_value
		) const {
	assert(actual_value.get_type() == apgen::DATA_TYPE::ARRAY);

	//
	// We can't just throw an exception; the user desperately wants
	// to provide something usable...
	//
	const task& T = mySource->Object->Task;
	names[string(*prefix)] = *T.get_varinfo()[T.paramindex[ind_0]].first;

	//
	// we need to provide as much info as possible based on actual_value
	//
	actual_value.extract_array_info(
		-1,	// indentation (none in this case)

		prefix,	// path to parameter or sub-element whose value
			// this is; the path to every array element starts
			// with this prefix

		names,	// map from paths of array elements to their labels;
			//   paths start with the prefix; colon is the separator
			//   (example: "1:3:0")

		types,	// map from paths to either "struct" or "list" (only if element is an array)

		ranges	// presently unused because we don't capture ranges
		);
	return;
}

bool actAgent::hasAssociates() const {
	int		creat = mySource->Object->type_has_task_named("creation");
	if(creat >= 0) {
	    // debug
	    // cout << identify() << "->notify_assoc_res: isActive = " << isActive << endl;
	    // cout << "    a_type: " << ((void *) a_type) << endl;
	    parsedProg& P = mySource->Object->Task.Type.tasks[creat]->prog;
	    if(P && P->has_associative_resources()) {
		return true;
	    }
	}
	return false;
}

void actAgent::dump() const {
}


#ifdef have_xmltol

//
// Last two params default to NULL
//
void actAgent::transfer_to_xmlpp(
		xmlpp::Element*	element,
		bool		AllActsVisible,
		CTime_base*	bounds,
		const char*	timesystem) const {
	xmlpp::Element*			record_el = element->add_child_element("TOLrecord");
	xmlpp::Element*			time_tag = record_el->add_child_element("TimeStamp");
	xmlpp::Element*			act_el = record_el->add_child_element("Instance");
	xmlpp::Element*			id_el = act_el->add_child_element("ID");
	xmlpp::Element*			el;

	record_el->set_attribute("type", "ACT_START");
	time_tag->set_first_child_text(*mySource->getetime().to_string());
	if(timesystem) {
		TypedValue&	tdv = globalData::get_symbol(timesystem);
		ListOVal*	tv = &tdv.get_array();

		CTime_base	offset = tv->find("origin")->Val().get_time_or_duration();
		double		multiplier = tv->find("scale")->Val().get_double();
		CTime_base	given_dur(mySource->getetime() - offset);
		CTime_base	delta(given_dur / multiplier);
		if(delta >= CTime_base(0, 0, true)) {
			time_tag->set_attribute(timesystem, *delta.to_string());
		}
		else {
			CTime_base	D(-delta);
			time_tag->set_attribute(timesystem, *(Cstring("-") + D.to_string()));
		}
	}
	id_el->set_first_child_text(*mySource->get_unique_id());
	el = act_el->add_child_element("Name");
	el->set_first_child_text(*mySource->theConstActAgent()->get_owner_name());
	el = act_el->add_child_element("Type");
	el->add_child_text(*mySource->Object->Task.Type.name);

	xmlpp::Element*			parent = act_el->add_child_element("Parent");

	ActivityInstance* the_parent = mySource->get_parent();
	if(the_parent) {
	    if(    !bounds ||
		   (the_parent->getetime() >= bounds[0]
		    && the_parent->getetime() <= bounds[1])
	      ) {
		parent->add_child_text(*mySource->get_parent()->get_unique_id());
	    }
	}

	xmlpp::Element*			viz_state = act_el->add_child_element("Visibility");

	if(AllActsVisible) {
		viz_state->add_child_text("visible");
	}
	else if(mySource->const_agent().is_decomposed()) {
		viz_state->add_child_text("decomposed");
	}
	else if(mySource->const_agent().is_abstracted()) {
		viz_state->add_child_text("abstracted");
	}
	else {
		viz_state->add_child_text("visible");
	}

	xmlpp::Element*			attributes = act_el->add_child_element("Attributes");
	xmlpp::Element*			parameters = act_el->add_child_element("Parameters");

	tvslist				Atts_off, Atts_nick, Params;
	stringslist			Parent, Children;
	Cstring				ID, Type, Name, Legend, theValue;
	CTime_base			temp_time;
	TaggedValue*			tds;
	tvslist::iterator		atts(Atts_nick), params(Params);
	xmlpp::Element*			attribute;
	xmlpp::Element*			parameter;
	apgen::act_visibility_state	V;
	xmlpp::Element*			name;
	Cstring				subsystem;

	mySource->export_symbols(
			ID,
			Type,
			Name,
			temp_time,
			Atts_off,
			Atts_nick,
			Params,
			Parent,
			Children,
			V,
			subsystem);

	while((tds = atts())) {
		attribute = attributes->add_child_element("Attribute");
		name = attribute->add_child_element("Name");
		name->add_child_text(*tds->get_key());
		if(tds->get_key() == "status") {
			if(tds->payload.get_int()) {
				TypedValue val;
				val = "scheduled";
				val.print_to_xmlpp(attribute);
			} else {
				TypedValue val;
				val = "unscheduled";
				val.print_to_xmlpp(attribute);
			}
		} else {
			tds->payload.print_to_xmlpp(attribute);
		}
	}
	while((tds = params())) {
		// Cstring	normalized = normalizeString(tds->get_key(), '_');
		parameter = parameters->add_child_element("Parameter");
		name = parameter->add_child_element("Name");
		name->add_child_text(*tds->get_key());
		tds->payload.print_to_xmlpp(parameter);
	}
}
#endif /* have_xmltol */

void actAgent::transfer_to_stream(
		aoString&	Stream,
		apgen::FileType	file_type,
		CTime_base*	bounds,	    /* = NULL */
		pinfotlist*	param_info  /* = NULL */ ) const {
    Cstring			step_type;
    ActivityInstance*	child;
    smart_actptr*		child_ptr;

    switch(file_type) {
	case apgen::FileType::FT_APF:
		// Steve Wissler's requested 'hidden from APF' attribute:
		if(mySource->Object->defines_property("undocumented")) {
			if((*mySource->Object)["undocumented"].get_int()) {
				return;
			}
		}
		{
		Cstring act_instance_name(mySource->get_key());
		const char* h = *act_instance_name;
		bool	quotes_required = false;
		while(*h) {
			if(isalpha(*h) || isdigit(*h) || *h == '_') {
				;
			} else {
				quotes_required = true;
				break;
			}
			h++;
		}
		if(quotes_required) {
			act_instance_name = addQuotes(act_instance_name);
		}
		Stream << "activity instance " << act_instance_name << " of type "
				<< (*mySource->Object)[ActivityInstance::TYPE].get_string()
				<< " id " << mySource->get_unique_id();
		Stream << "\n    begin\n";

		apgen::METHOD_TYPE	mt;
		if(	mySource->has_decomp_method(mt)
			&&
			mySource->const_hierarchy().children_count()
		  ) {
			int	line_length = 25;
			if(mySource->const_agent().is_decomposed()) {
				Stream << "\tdecomposed into ";
			} else {
				Stream << "\tdecomposable into ";
			}
			slist<alpha_void, smart_actptr>::iterator
					li(mySource->get_down_iterator());
			while((child_ptr = li())) {
				if((child = child_ptr->BP)) {
					if(child_ptr != li.const_first()) {
						line_length += 2;
						Stream << ", ";
					}
					if(line_length > 80) {
						line_length = 16;
						Stream << "\n\t\t";
					}
					line_length += child->get_unique_id().length();
					Stream << child->get_unique_id();
				}
			}
			Stream << ";\n";
		}
		if(mySource->get_parent()) {
			if(mySource->const_agent().is_abstracted())
				Stream << "\tabstracted into " << mySource->get_parent()->get_unique_id() << ";\n";
			else
				Stream << "\tabstractable into " << mySource->get_parent()->get_unique_id() << ";\n";
		}
		Cstring					attribute_nickname;
		tlist<alpha_string, Cnode0<alpha_string, int> >		correct_order(true);
		slist<alpha_string, Cnode0<alpha_string, int> >::iterator CorrectOrder(correct_order);
		Cnode0<alpha_string, int>*		tag;
		bool					first_attribute = true;
		behaving_base*				obj = mySource->Object.object();

		for(int iatt = 0; iatt < obj->Task.get_varinfo().size(); iatt++) {
			TypedValue default_value;
			default_value.generate_default_for_type((*obj)[iatt].declared_type);
			Cstring att_name = obj->Task.get_varinfo()[iatt].first;
			map<Cstring, Cstring>::iterator iter
				= aafReader::nickname_to_activity_string().find(att_name);

			//
			// Include the following:
			// 	- status (always)
			// 	- pattern (always)
			// 	- never include finish
			// 	- arrays of length > 0
			// 	- other values if they are not the default
			//

			bool include_this_attr_based_on_its_value =
				att_name != "finish"
				&& (
					att_name == "status"
					|| att_name == "pattern"
					|| ((*obj)[iatt].get_type() == apgen::DATA_TYPE::ARRAY
					    && (*obj)[iatt].get_array().get_length() != 0)
					|| ((*obj)[iatt].get_type() != apgen::DATA_TYPE::ARRAY
					    && (*obj)[iatt] != default_value)
				   );

			//
			// debug
			//
			
			// cerr << "(APF) attribute: " << att_name << ", value = '"
			// 	<< obj->values[iatt].to_string() << "', included = "
			// 	<< include_this_attr_based_on_its_value << "\n";

			if(iter != aafReader::nickname_to_activity_string().end()
			   && include_this_attr_based_on_its_value
			  ) {
				correct_order << new Cnode0<alpha_string, int>(iter->second, iatt);
			}
		}

		while((tag = CorrectOrder())) {
			int iatt = tag->payload;
			attribute_nickname = obj->Task.get_varinfo()[iatt].first;

			// if( obj->values[iatt].get_type() == apgen::DATA_TYPE::ARRAY
			//   && obj->values[iatt].value.LI->get_length() == 0) {
			// 	continue;
			// }

			if(first_attribute) {
				Stream << "\tattributes\n";
				first_attribute = false;
			}
			Stream << "\t    " << tag->get_key();

			// this should happen automatically, but...
			Stream.LineLength = 12;

			if(tag->get_key() == "\"SASF\"") {
				//
				// Use '+=' to indicate that we want to MERGE
				// attributes with those of the AAF (if any)
				//
				Stream << " += ";
			} else {
				Stream << " = ";
			}
			if(attribute_nickname == "span") {
				// Special case: always use ACTUAL value, not formula.
				// This way the instance will look OK even if the type is
				// not available.
				CTime_base dur(mySource->get_timespan());

				Stream << dur.to_string();
			} else if(attribute_nickname == "start") {
				// Special case: always use ACTUAL value, not formula.
				// This way the instance will look OK even if the type is
				// not available.
				// ... except for epochs, of course.
				Stream << mySource->write_start_time(0);
			} else if(attribute_nickname == "status") {
				if((*obj)[iatt].get_int()) {
					Stream << "\"scheduled\"";
				} else {
					Stream << "\"unscheduled\"";
				}
			} else {
				(*obj)[iatt].print(Stream, 0);
			}
			Stream << ";\n";
		}

		if(obj->Task.paramindex.size()) {
			Stream << "\tparameters\n\t    (";
			int j;
			int indent_by = 0;
			bool first = true;

			for(j = 0; j < obj->Task.paramindex.size(); j++) {
				if(first) {
					Stream << "\n";
					first = false;
				}
				Stream << "\t    ";
				Stream.LineLength = 0;
				(*obj)[obj->Task.paramindex[j]].print(
					Stream, indent_by, MAX_SIGNIFICANT_DIGITS);
				if (j != obj->Task.paramindex.size() - 1) {
					// Stream << ", ";
					Stream << ",\n";
				} else {
					Stream << "\n";
				}
			}
			if(first) {
				Stream << ");\n";
			} else {
				Stream << "\t    );\n";
			}
		}
		Stream << "    end activity instance " << act_instance_name << "\n\n";
		}
		break;
	case apgen::FileType::FT_TOL:
	    {
	    CTime_base start_time(mySource->getetime());
	    CTime_base duration(mySource->get_timespan());
	    bool	has_description_attribute =
					mySource->Object->defines_property("description");
	    bool	has_legend_attribute =
					mySource->Object->defines_property("legend");
	    Cstring	attribute_nickname;
	    int attrcount = 0;

	    Stream << start_time.to_string() << ",ACT_START,";
	    Stream << mySource->get_key() << ",";
	    Stream << "d,";
	    Stream << duration.to_string();
	    if(	mySource->const_agent().is_decomposed() ||
		mySource->const_agent().is_abstracted()) {
		Stream << ",\"INVISIBLE\",";
	    } else {
		Stream << ",\"VISIBLE\",";
	    }

	    Cstring act_type = (*mySource->Object)[ActivityInstance::TYPE].get_string();
	    Stream << "type=" << act_type << ",";
	    Stream << "node_id=" << mySource->get_unique_id() << ",";
	    Stream << "legend=";
	    if(has_legend_attribute) {
		(*mySource->Object)["legend"].print(
					Stream,
					0,
					MAX_SIGNIFICANT_DIGITS,
					true);
	    } else {
		    Stream << "\"\"";
	    }
	    Stream << ",";
	    Stream << "description=";
	    if(has_description_attribute) {
		(*mySource->Object)["description"].print(
					Stream,
					0,
					MAX_SIGNIFICANT_DIGITS,
					true);
	    } else {
		Stream << "\"\"";
	    }
	    Stream << ",";

	    Stream << "attributes=(";
	    tlist<alpha_string, Cnode0<alpha_string, int> >	correct_order(true);
	    slist<alpha_string, Cnode0<alpha_string, int> >::iterator CorrectOrder(correct_order);
	    Cnode0<alpha_string, int>*				tag;
	    behaving_base*					obj = mySource->Object.object();


	    //
	    // NOTE: we have already output start, duration,
	    // legend and description, so we should not output
	    // them again as part of the attributes list.
	    //

	    for(int iatt = 0; iatt < obj->Task.get_varinfo().size(); iatt++) {
		TypedValue	default_value;
		default_value.generate_default_for_type((*obj)[iatt].declared_type);
		Cstring att_name = obj->Task.get_varinfo()[iatt].first;
		map<Cstring, Cstring>::iterator it3
			= aafReader::nickname_to_activity_string().find(att_name);

		//
		// Include the following:
		// 	- status (always)
		// 	- pattern (always)
		// 	- never include finish
		// 	- never include start
		// 	- never include duration
		// 	- never include legend
		// 	- never include description
		// 	- arrays of length > 0
		// 	- other values if they are not the default
		//

		bool include_this_attr_based_on_its_value =
			att_name != "finish"
			&& att_name != "start"
			&& att_name != "span"
			&& att_name != "legend"
			&& att_name != "description"
			&& (
			    att_name == "status"
			    || att_name == "pattern"
			    || ((*obj)[iatt].get_type() == apgen::DATA_TYPE::ARRAY
				&& (*obj)[iatt].get_array().get_length() != 0)
			    || ((*obj)[iatt].get_type() != apgen::DATA_TYPE::ARRAY
				&& (*obj)[iatt] != default_value)
			   );

		//
		// debug
		//
		
		// cerr << "(TOL) attribute: " << att_name << ", value = '"
		//      << obj->values[iatt].to_string() << "', included = "
		// 	<< include_this_attr_based_on_its_value << "\n";

		if(	it3 != aafReader::nickname_to_activity_string().end()
			&& include_this_attr_based_on_its_value
		  ) {
		    correct_order << new Cnode0<alpha_string, int>(it3->second, iatt);
		}
	    }
	    bool first = true;
	    while((tag = CorrectOrder())) {
		int iatt = tag->payload;
		attribute_nickname = obj->Task.get_varinfo()[iatt].first;
		if((*obj)[iatt].get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
		    if(first) {
			first = false;
		    } else {
		      	Stream << ",";
		    }
				
		    Stream << tag->get_key() << "=";
		    if(attribute_nickname == "status") {
			if((*obj)[tag->payload].get_int()) {
			    Stream << "\"scheduled\"";
			} else {
			    Stream << "\"unscheduled\"";
			}
		    } else {
			// adds quotes if a string:
			(*obj)[tag->payload].print(Stream, 0, MAX_SIGNIFICANT_DIGITS, true);
		    }
		}
	    }
	    Stream << "),";

	    Stream << "parameters=(";

	    pinfoNode* actInfo = NULL;
	    if(param_info) {
		actInfo = param_info->find(act_type);
	    }
	    for(int j = 0; j < obj->Task.paramindex.size(); j++) {
		Cstring parname = obj->Task.get_varinfo()[obj->Task.paramindex[j]].first;
		Stream << parname << "=";
		TypedValue& actualParam = (*obj)[obj->Task.paramindex[j]];

		//
		// If this parameter is an array and if this instance
		// has a well-defined value style for the array,
		// update the param_info list accordingly.
		//
		// Unfortunately, it is not sufficient to document the
		// parameter array itself; its elements may themselves
		// be arrays, in which case we need to document _them_
		// as well.
		//
		// But this means that the payload of an arrayNode,
		// an object intended to document a single parameter,
		// should be suitable for documenting a hierarchy of
		// arrays than just the array parameter itself.
		//
		// Accordingly, the typedef that defines arrayNode
		// should feature a payload that is rich enough for
		// that purpose, for example a flexval. We could
		// then set the flexval to a structure that contains
		// one of the following:
		//
		// 	- a string S if the values stored in the array
		// 	  being documented are all non-array objects;
		// 	  S will be either "list" or "struct".
		//
		// 	- a list of one element E if the array being
		// 	  documented is a list of arrays; E will document
		// 	  those arrays. Note that a list is always
		// 	  homogeneous, so a single element suffices. By
		// 	  the way, homogeneity is not mandated by APGenX;
		// 	  it's a feature of the adaptation.
		//
		// 	- a struct of N elements E_1, E_2, ..., E_N if
		// 	  the array being documented is a struct that
		// 	  contains N arrays. The key of E_i will be the
		// 	  key of the i-th array in the struct, and the
		// 	  value of E_i will document that array.
		//
		if(actInfo) {
		    arrayNode* paramInfo = actInfo->payload.find(parname);
		    if(actInfo && actualParam.get_type() == apgen::DATA_TYPE::ARRAY) {

			if(actualParam.get_array().get_array_type()
				!= TypedValue::arrayType::UNDEFINED) {

			    actualParam.get_array().document_arrays_in(paramInfo->payload);

		        }
		    }
		}

		// adds quotes if a string:
		actualParam.print(Stream, 0, MAX_SIGNIFICANT_DIGITS, true);
		if(j != obj->Task.paramindex.size() - 1) {
		    Stream << ",";
		}
	    }
	    Stream << "),";

	    //
	    // Do this at the very end for compatibility with
	    // TOL parsers (e. g. OPS/seq/bin/parsetol.py) that
	    // do not care about trailing information in an
	    // ACT_START record
	    //
	    ActivityInstance* the_parent = mySource->get_parent();
	    bool no_parent = true;
	    if(the_parent) {
		if(   !bounds ||
		      (mySource->get_parent()->getetime() >= bounds[0]
		       && mySource->get_parent()->getetime() <= bounds[1])) {
		    no_parent = false;
		    Stream << "parent=" << mySource->get_parent()->get_unique_id() << ";\n";
		}
	    }
	    if(no_parent) {
		Stream << "parent=none;\n";
	    }

	    }
	    break;
	default:
	    break;
    }
}

void actAgent::export_symbols(
		Cstring&			id,
		Cstring&			type,
		Cstring&			name,
		CTime_base&			start,
		tvslist&			Atts_w_official_tag,
		tvslist&			Atts_w_nickname,
		tvslist&			Params,
		stringslist&			Parent,
		stringslist&			Children,
		apgen::act_visibility_state&	V,
		Cstring&			Subsystem) const {
    tlist<alpha_string, Cnode0<alpha_string, int> >			correct_order(true);
    tlist<alpha_string, Cnode0<alpha_string, int> >::iterator	CorrectOrder(correct_order);
    Cnode0<alpha_string, int>*					tag;
    List			rangevals;
    int			i;
    behaving_base*		obj = mySource->Object.object();

    //assemble a correctly ordered list of attributes
    for(int iatt = 0; iatt < obj->Task.get_varinfo().size(); iatt++) {
	TypedValue default_value;
	default_value.generate_default_for_type((*obj)[iatt].declared_type);
	Cstring att_name = obj->Task.get_varinfo()[iatt].first;
	map<Cstring, Cstring>::iterator iter
		= aafReader::nickname_to_activity_string().find(att_name);

	//
	// Old (pre-AP-1047) criteria for inclusion in the attributes section:
	//
	// if(iter != aafReader::nickname_to_activity_string().end()
	//    && obj->Task.get_varinfo()[iatt].first != "finish"
	//    // watch out: generics don't have a type
	//    // && (!obj->Task.prog || obj->Task.prog->symbols.find(att_name) != obj->Task.prog->symbols.end())
	//    && (obj->Task.get_varinfo()[iatt].first == "status" || (*obj)[iatt] != default_value)
	//    // hopefully redundant:
	//    && (*obj)[iatt].get_type() != apgen::DATA_TYPE::UNINITIALIZED
	//   ) {}
	//
	// The new criteria are as follows:
	//
	// 	- the span (duration), start and status attributes are always included
	// 	- all other attributes (including custom attributes) are included
	// 	  if (and only if) they appear in the activity type definition
	//
	// In the past, APGen also included any attributes that were read in from
	// an APF but were not featured in the activity type definition. This was
	// sometimes convenient, but in the spirit of bringing the APGenX DSL
	// closer to commonly used languages, an activity instance should not feature
	// items that are not included in its type (i. e. 'class') definition.
	//
	bool var_name_qualifies_as_an_attribute
		= iter != aafReader::nickname_to_activity_string().end();
	bool attr_is_span_or_start_or_status = att_name == "start"
		  			    || att_name == "span"
		  			    || att_name == "status";
	bool activity_type_defines_this_attr
		= obj->Task.prog && obj->Task.prog->symbols.find(att_name) != obj->Task.prog->symbols.end();
	bool attribute_value_is_known = (*obj)[iatt].get_type() != apgen::DATA_TYPE::UNINITIALIZED;

	if(	var_name_qualifies_as_an_attribute
		&& (attr_is_span_or_start_or_status
		    || activity_type_defines_this_attr)
		&& attribute_value_is_known) {
		correct_order << new Cnode0<alpha_string, int>(iter->second, iatt);
	}
    }

    //now iterating through attributes
    while((tag = CorrectOrder())) {
	int iatt = tag->payload;
	Cstring the_key = obj->Task.get_varinfo()[iatt].first;
	if(the_key == "subsystem") {
	    Subsystem = (*obj)[iatt].get_string();
	}
	Atts_w_official_tag << new TaggedValue(the_key, (*obj)[iatt]);
	Atts_w_nickname << new TaggedValue(the_key, (*obj)[iatt]);
    }


    if(obj->Task.paramindex.size()) {
	const task& instance_task = obj->Task;
	TaggedValue* tv;

	for(int j = 0; j < instance_task.paramindex.size(); j++) {
	    Params << (tv = new TaggedValue(
		instance_task.get_varinfo()[instance_task.paramindex[j]].first, 
		(*obj)[instance_task.paramindex[j]]));
	    tv->payload.declared_type = instance_task.get_varinfo()[instance_task.paramindex[j]].second;
	}
    }

    // finish with the easy stuff:
    id = mySource->get_unique_id();

    type = mySource->Object->Task.Type.name;
    name = mySource->get_key();
    start = mySource->getetime();

    V = apgen::act_visibility_state::VISIBILITY_REGULAR;
    if(mySource->const_agent().is_decomposed()) {
    	V = apgen::act_visibility_state::VISIBILITY_DECOMPOSED;
    } else if(mySource->const_agent().is_abstracted()) {
    	V = apgen::act_visibility_state::VISIBILITY_ABSTRACTED;
    }

    if(mySource->const_hierarchy().children_count()) {
	slist<alpha_void, smart_actptr>::iterator currentChildren(mySource->get_down_iterator());
	smart_actptr* currentChildNode;

	while((currentChildNode = currentChildren())) {
	    ActivityInstance* child = currentChildNode->BP;

	    Children << new emptySymbol(child->get_unique_id());
	}
    }
    if(mySource->get_parent()) {
	Parent << new emptySymbol(mySource->get_parent()->get_unique_id());
    }
}

//
// exercise_modeling() is an excellent candidate for refactoring.
//
// Things that should clearly be changed:

// 	- instead of discovering whether the activity type
// 	  contains 'modeling' or 'resource usage', the
// 	  action method should reflect whether this is a
// 	  call or a spawn
// 
// 	- the is_unscheduled() call is confusing. One should
// 	  be able to identify whether the activity is disabled
// 	  at a higher level
// 
// Outline of a better structure:
// 
// 	- identify the proper task
//
// 	- identify whether the task invocation is immediate or staged
// 
// 	- launch the task accordingly
// 

execution_context::return_code	actAgent::exercise_modeling(
		bool skip_wait /* = false */,
		pEsys::execStack* use_this_stack /* = NULL */ ) {
    Behavior&			beh = mySource->Object->Task.Type;

#ifdef PREMATURE
    map<Cstring, int>::iterator method_iter = beh.taskindex.find("modeling");
    if(method_iter != beh.taskindex.end()) {

	//
	// stage the modeling method at start time
	//
    } else if((method_iter = beh.taskindex.find("resource usage")) != beh.taskindex.end()) {

	//
	// execute the modeling method immediately
	//
    }
#endif /* PREMATURE */

    time_saver				ts;
    execution_context::return_code	Code = execution_context::REGULAR;

    if(mySource->is_unscheduled()) {
	return Code;
    }

    pEsys::Program*		program_to_use = NULL;
    apgen::METHOD_TYPE		section_to_use = apgen::METHOD_TYPE::NONE;
    task*			task_to_use = NULL;
    bool			use_the_given_stack = false;
    bool			really_skip_wait = false;

    map<Cstring, int>::iterator method_iter = beh.taskindex.find("modeling");
    if(method_iter != beh.taskindex.end()) {
	section_to_use = apgen::METHOD_TYPE::MODELING;
	task_to_use = beh.tasks[method_iter->second];
	program_to_use = task_to_use->prog.object();
	if(skip_wait) {
	    use_the_given_stack = true;
	    really_skip_wait = true;
	}
    }
    if(!program_to_use) {
	method_iter = beh.taskindex.find("resource usage");
	if(method_iter != beh.taskindex.end()) {
	    section_to_use = apgen::METHOD_TYPE::RESUSAGE;
	    task_to_use = beh.tasks[method_iter->second];
	    program_to_use = task_to_use->prog.object();
	}
    }

    if(program_to_use && program_to_use->statements.size()) {
	pEsys::execStack*	temp_stack = /* stack_to_use*/ NULL;
	act_method_object*	amo = new act_method_object(mySource, *task_to_use);
	behaving_element	be(amo);
	execution_context*	AC = new execution_context(
					be, // mySource->Object,
					program_to_use,
					section_to_use,
					really_skip_wait
					);

	ts.set_now_to(mySource->getetime());


	try {
	    if(use_the_given_stack) {
		assert(use_this_stack);
		use_this_stack->push_context(AC);
		use_this_stack->Execute(Code);
	    } else {
		temp_stack = new pEsys::execStack(AC);
		execution_context::return_code	NewCode = execution_context::REGULAR;
		temp_stack->Execute(NewCode);
		delete temp_stack;
	    }
	} catch(eval_error Err) {
	    Cstring errs;
	    	errs << "Error in modeling act. " << mySource->get_unique_id()
		    << "; details:\n" << Err.msg;
	    	if(!use_the_given_stack) {
		    delete temp_stack;
	    	}
	    	throw(eval_error(errs));
	}
    }
    return Code;
}


//
// The previous method's comments apply here too
//
execution_context::return_code actAgent::exercise_decomposition(
		bool skip_wait /* = false */,
		pEsys::execStack* use_this_stack /* = NULL */ ) {
	execution_context::return_code		Code = execution_context::REGULAR;
	Behavior&				beh = mySource->Object->Task.Type;
	map<Cstring, int>::const_iterator	iter = beh.taskindex.find("decompose");
	time_saver				ts;

	ts.set_now_to(mySource->getetime());

	if(iter != beh.taskindex.end()) {
		task*			decomp_task = beh.tasks[iter->second];
		behaving_base*		decomp_obj = new act_method_object(
							mySource,
							*decomp_task);
		behaving_element	decomp_el(decomp_obj);
		execution_context*	AC = new execution_context(
						decomp_el,
						decomp_task->prog.object(),
						decomp_task->prog->section_of_origin(),
						skip_wait
						);
		pEsys::execStack*		temp_stack;

		try {
			if(skip_wait) {
				assert(use_this_stack);
				use_this_stack->push_context(AC);
				use_this_stack->Execute(Code);
			} else {
				temp_stack = new pEsys::execStack(AC);
				execution_context::return_code	NewCode = execution_context::REGULAR;
				temp_stack->Execute(NewCode);
				delete temp_stack;
			}
		} catch(eval_error Err) {
			Cstring errs;
			errs << "Error in decomposing act. " << mySource->get_unique_id()
				<< "; details:\n" << Err.msg;
			if(!skip_wait) {
				delete temp_stack;
			}
			throw(eval_error(errs));
		}
	}
	return Code;
}
