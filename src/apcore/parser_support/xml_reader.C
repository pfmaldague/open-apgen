#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>

#include <ActivityInstance.H>
#include <APsax.H>
#include <stdlib.h>	// for atoi()
#include <fstream>
#include <iostream>

// #define DEBUG_THE_READER

#include <xml_sax_parser.H>
#include <act_plan_parser.H>

#include <flexval.H>


//
// This is a generic plan parser that will work with
// any concrete parser derived from the generic_parser
// class.
//
void
parse_act_plan(
	generic_parser& givenParser) {
    generic_parser&				parser(givenParser);
    fileParser&					conv(fileParser::currentParser());
    slist<alpha_int, pai_args_node>		mainList;
    stringslist					context;
    Rsource*					res;
    tlist<alpha_void, resourceConsumptionInfo>	resources_already_initialized;

    try {
	pai_args_node*							pain;
	tlist<alpha_string, Cnode0<alpha_string, pai_args_node*> >	painById;
	int								i = 0;
	bool								isAct, isRes, isCon;

	context << new emptySymbol("Verifying that given string is an array");
	Cstring any_errors;
	if(!parser.verify_that_string_is_an_array(any_errors)) {
		throw(eval_error(any_errors));
	}

	context.clear();
	context << new emptySymbol("iterating over activities and resources");
	int lineno;
	while(parser.iterate(isAct, isRes, isCon, &lineno)) {
	    if(isAct) {
		stringslist	save_context(context);
		pairtlist	string_attributes;
		CTime_base&	timeTag(*parser.time_tag());

		pain = new pai_args_node(i);
		i++;

		pain->payload.start_time = timeTag;
		pain->payload.line_where_defined = lineno;

		context << new emptySymbol("getting ID");
		pain->payload.ID_for_this_file = parser.act_get_id();
		painById << new Cnode0<alpha_string, pai_args_node*>(
			pain->payload.ID_for_this_file,
			pain);

		context << new emptySymbol("getting type");
		pain->payload.activityTypeName = parser.act_get_type();
		if(parser.can_extract_values()) {
			context << new emptySymbol("getting attribute values");
			parser.act_get_attribute_values(pain->payload.tv_attributes);
		} else {
			/* The right thing to do here is to shoot for
			 * values, always. */
			symNode*		sn;
			pairslist::iterator	iter(string_attributes);

			context << new emptySymbol("getting attribute strings");
			parser.act_get_attribute_strings(string_attributes);
			while((sn = iter())) {
				Cstring		text(sn->payload);
				TypedValue	result;

				// exceptions will be caught below
				fileReader::CompileAndEvaluateSelf(
					text,
					result);

				pain->payload.tv_attributes
					<< new TaggedValue(sn->get_key(), result);
			}
		}

		pain->payload.activityName = parser.act_get_name(string_attributes);
		context << new emptySymbol("checking parent");
		if(parser.act_has_parent()) {
			bool	abstractable, abstracted;
			Cstring	parent;

			parent = parser.act_get_parent(abstractable, abstracted);
			if(abstractable) {
				pain->payload.theAbstractableAncestors
					<< new emptySymbol(parent);
			} else if(abstracted) {
				pain->payload.theAbstractedAncestors
					<< new emptySymbol(parent);
			}
		}

		context << new emptySymbol("getting parameters");
		if(parser.can_extract_values()) {
			parser.act_get_parameter_values(pain->payload.tv_parameters);
		} else {
			/* The right thing to do here is to shoot for
			 * values, always. */
			Behavior*	aType = Behavior::find_type(
							pain->payload.activityTypeName);
			pairtlist	string_parameters;

			parser.act_get_parameter_strings(string_parameters);

			if(string_parameters.get_length() > 0) {
				if(!aType) {
					Cstring errs("Type ");
					errs << pain->payload.activityTypeName
						<< " is not defined - line "
						<< pain->payload.line_where_defined;
					throw(eval_error(errs));
				}
    
				task*		act_constructor = aType->tasks[0];
				parsedProg&	pars = act_constructor->parameters;

				if(pars) {
				    for(int ip = 0; ip < act_constructor->paramindex.size(); i++) {
					const Cstring pName = act_constructor->get_varinfo()[
							act_constructor->paramindex[ip]].first;
					symNode* ptr = string_parameters.find(pName);
					if(!ptr) {
						Cstring errs("Parameter ");
						errs << pName << " of type "
							<< pain->payload.activityTypeName
							<< " is not found in "
							<< parser.parser_name()
							<< " parameters for activity "
							<< pain->payload.ID_for_this_file
							<< " - line " << pain->payload.line_where_defined;
						throw(eval_error(errs));
					}
					Cstring		text(ptr->payload);
					TypedValue	result;

					if(text.length() == 0) {
						Cstring	tmp("parameter ");
						tmp << pName << " has zero length\n";
						throw(eval_error(tmp));
					}

					// exceptions will be caught below
					fileReader::CompileAndEvaluateSelf(
						text,
						result);

					pain->payload.tv_parameters
						<< new TaggedValue(ptr->get_key(), result);
				    }
				}
			}
		}

		mainList << pain;
		context = save_context;

	    } else if(isRes && parser.time_tag()) {
		// the json parser hasn't implemented resources yet

		Cstring				Name = parser.res_get_name();
		TypedValue			Value = parser.res_get_value();
		pairslist			indices;
		aoString			fullName;
		Cstring				fullNameString;
		apgen::RES_CLASS		rtype;
		long				count;
		CTime_base&			timeTag(*parser.time_tag());
		resourceConsumptionInfo*	rci;
		TypedValue			prev_value;
		bool				there_is_a_prev_value = false;
		Rsource*			res = eval_intfc::FindResource(Name);

		if(!res) {
		    Cstring	e("XML TOL parser: cannot find resource ");
		    e << Name << " - line " << lineno;
		    throw(eval_error(e));
		}
		if(!(rci = resources_already_initialized.find(res))) {
		    res->initialize_value(timeTag);
		    resources_already_initialized << (rci
					= new resourceConsumptionInfo(
						res,
						resourceConsumptionInfoPLD(res)));
		} else {
		    prev_value = rci->payload.consumption;
		    there_is_a_prev_value = true;
		}
		RES_state*		state_res = dynamic_cast<RES_state*>(res);
		RES_settable*		settable_res = dynamic_cast<RES_settable*>(res);
		RES_consumable*		cons_res = dynamic_cast<RES_consumable*>(res);
		RES_nonconsumable*	noncons_res = dynamic_cast<RES_nonconsumable*>(res);
		if(state_res) {
		    state_res->append_to_history(
					Value,
					timeTag,
					timeTag,
					0,
					0,
					apgen::USAGE_TYPE::SET);
		} else if(settable_res) {
		    settable_res->append_to_history(
					Value,
					timeTag,
					timeTag,
					0,
					0,
					apgen::USAGE_TYPE::SET);
		} else if(cons_res) {
		    TypedValue	profileval;
		    cons_res->evaluate_profile(
					EvalTiming::NORMAL,
					timeTag,
					profileval);
		    if(cons_res->get_datatype() == apgen::DATA_TYPE::FLOATING) {
				double	P = profileval.get_double();
				// should satisfy Value = P - consumption;
				double	cumulative_consumption = profileval.get_double() - Value.get_double();
				double	consumption;
				if(there_is_a_prev_value) {
					consumption = cumulative_consumption - prev_value.get_double();
				} else {
					consumption = cumulative_consumption;
				}
				rci->payload.consumption = cumulative_consumption;
				Value = consumption;
		    } else if(cons_res->get_datatype() == apgen::DATA_TYPE::INTEGER) {
				long	P = profileval.get_int();
				// should satisfy Value = P - consumption;
				long	cumulative_consumption = profileval.get_int() - Value.get_int();
				long	consumption;
				if(there_is_a_prev_value) {
					consumption = cumulative_consumption - prev_value.get_int();
				} else {
					consumption = cumulative_consumption;
				}
				rci->payload.consumption = cumulative_consumption;
				Value = consumption;
		    } else if(cons_res->get_datatype() == apgen::DATA_TYPE::TIME
				|| cons_res->get_datatype() == apgen::DATA_TYPE::DURATION) {
				CTime_base	P = profileval.get_time_or_duration();
				CTime_base	cumulative_consumption = P - Value.get_time_or_duration();
				CTime_base	consumption;
				if(there_is_a_prev_value) {
					consumption = cumulative_consumption - prev_value.get_time_or_duration();
				} else {
					consumption = cumulative_consumption;
				}
				rci->payload.consumption = cumulative_consumption;
				Value.declared_type = apgen::DATA_TYPE::DURATION;
				Value = consumption;
		    }
		    cons_res->append_to_history(
					Value,
					timeTag,
					timeTag,
					0,
					0,
					apgen::USAGE_TYPE::USE);
		} else if(noncons_res) {
		    TypedValue	profileval;
		    noncons_res->evaluate_profile(
					EvalTiming::NORMAL,
					timeTag,
					profileval);
		    if(noncons_res->get_datatype() == apgen::DATA_TYPE::FLOATING) {
				double	P = profileval.get_double();
				// should satisfy Value = P - consumption;
				double	cumulative_consumption = profileval.get_double() - Value.get_double();
				double	consumption;
				if(there_is_a_prev_value) {
					consumption = cumulative_consumption - prev_value.get_double(); }
				else {
					consumption = cumulative_consumption; }
				rci->payload.consumption = cumulative_consumption;
				Value = consumption;
		    } else if(noncons_res->get_datatype() == apgen::DATA_TYPE::INTEGER) {
				long	P = profileval.get_int();
				// should satisfy Value = P - consumption;
				long	cumulative_consumption = profileval.get_int() - Value.get_int();
				long	consumption;
				if(there_is_a_prev_value) {
					consumption = cumulative_consumption - prev_value.get_int(); }
				else {
					consumption = cumulative_consumption; }
				rci->payload.consumption = cumulative_consumption;
				Value = consumption;
		    } else if(noncons_res->get_datatype() == apgen::DATA_TYPE::TIME
				  || noncons_res->get_datatype() == apgen::DATA_TYPE::DURATION) {
				CTime_base	P = profileval.get_time_or_duration();
				CTime_base	cumulative_consumption = P - Value.get_time_or_duration();
				CTime_base	consumption;
				if(there_is_a_prev_value) {
					consumption = cumulative_consumption - prev_value.get_time_or_duration();
				} else {
					consumption = cumulative_consumption;
				}
				rci->payload.consumption = cumulative_consumption;
				Value.declared_type = apgen::DATA_TYPE::DURATION;
				Value = consumption;
		    }
		    noncons_res->append_to_history(
					Value,
					timeTag,
					CTime_base(0,0,false),
					0,
					0,
					apgen::USAGE_TYPE::USE);
		}
	    }
	}

	//
	// Here we should
	//
	// 	- compute the list of children
	// 	- evaluate visibility and store it in the pain
	//

	slist<alpha_int, pai_args_node>::iterator	pai_iter(mainList);
	emptySymbol*					eparent;

	while((pain = pai_iter())) {
	    if((eparent = pain->payload.theAbstractableAncestors.first_node())) {
		Cnode0<alpha_string, pai_args_node*>*	ap = painById.find(eparent->get_key());
		if(!ap) {
			Cstring errs("Parent of act. ");
			errs << pain->payload.ID_for_this_file << " had ID "
				<< eparent->get_key() << ", not found - line "
				<< pain->payload.line_where_defined;
			throw(eval_error(errs));
		}
		ap->payload->payload.theDecomposedChildren
			<< new emptySymbol(pain->payload.ID_for_this_file);
	    } else if((eparent = pain->payload.theAbstractedAncestors.first_node())) {
		Cnode0<alpha_string, pai_args_node*>*	ap = painById.find(eparent->get_key());
		if(!ap) {
			Cstring errs("Parent of act. ");
			errs << pain->payload.ID_for_this_file << " had ID "
				<< eparent->get_key() << ", not found - line "
				<< pain->payload.line_where_defined;
			throw(eval_error(errs));
		}
		ap->payload->payload.theDecomposableChildren
			<< new emptySymbol(pain->payload.ID_for_this_file);
	    }
	}

	//
	// We now have full lists of parents and children. We should
	// be able to determine visibility.
	//
	while((pain = pai_iter())) {
	    if(pain->payload.theAbstractableAncestors.first_node()) {
		if(pain->payload.theDecomposedChildren.get_length()) {
			pain->payload.Visibility = fileReader::decomposed;
		} else if(pain->payload.theDecomposableChildren.get_length()) {
			pain->payload.Visibility = fileReader::visible;
		} else {
			pain->payload.Visibility = fileReader::visible;
		}
	    } else if(pain->payload.theAbstractedAncestors.first_node()) {
		// we should check that children are decomposable, not decomposed
		pain->payload.Visibility = fileReader::abstracted;
	    } else {
		// no parent
		if(pain->payload.theDecomposedChildren.get_length()) {
			pain->payload.Visibility = fileReader::decomposed;
		} else if(pain->payload.theDecomposableChildren.get_length()) {
			pain->payload.Visibility = fileReader::visible;
		} else {
			pain->payload.Visibility = fileReader::visible;
		}
	    }
	}

	while((pain = mainList.first_node())) {
	    try {
		    fileReader::process_activity_instance(
			pain->payload.start_time,
			pain->payload.Visibility,
			pain->payload.request_or_chameleon,
			pain->payload.tv_attributes,
			pain->payload.tv_parameters,
			pain->payload.activityName,
			pain->payload.activityTypeName,
			pain->payload.theAbstractableAncestors,
			pain->payload.theAbstractedAncestors,
			pain->payload.theDecomposableChildren,
			pain->payload.theDecomposedChildren,
			pain->payload.ID_for_this_file);
	    } catch(eval_error err) {
			Cstring msg("Error in process_activity_instance(ID = ");
			msg << pain->payload.ID_for_this_file << ") on line "
				<< pain->payload.line_where_defined << "; details:\n"
				<< err.msg;
			throw(eval_error(msg));
	    }
	    delete pain;
	}
	slist<alpha_void, resourceConsumptionInfo>::iterator	final_res_iter(resources_already_initialized);
	resourceConsumptionInfo*				rci;
	while((rci = final_res_iter())) {
	    res = rci->payload.res;
	    RES_state*		state_res = dynamic_cast<RES_state*>(res);
	    RES_settable*		settable_res = dynamic_cast<RES_settable*>(res);
	    RES_consumable*		cons_res = dynamic_cast<RES_consumable*>(res);
	    RES_nonconsumable*	noncons_res = dynamic_cast<RES_nonconsumable*>(res);

	    if(state_res) {
		state_res->get_history().setImported(true);
		state_res->get_history().Freeze();
	    } else if(settable_res) {
		settable_res->get_history().setImported(true);
		settable_res->get_history().Freeze();
	    } else if(cons_res) {
		cons_res->get_history().setImported(true);
		cons_res->get_history().Freeze();
	    } else if(noncons_res) {
		noncons_res->get_history().setImported(true);
		noncons_res->get_history().Freeze();
	    }
	}
    }
    catch(eval_error Err) {
	Cstring any_errors;
	stringslist::iterator	error_it(context);
	emptySymbol*		s;

	any_errors << "Uh Oh: " << parser.parser_name() << " parsing error\n\tcontext:\n";
	while((s = error_it())) {
		any_errors << "\t\t" << s->get_key() << "\n";
	}
	any_errors << "\terror:\n" << Err.msg << "\n";
	throw(eval_error(any_errors));
    }
    // now we call FixAll...()
    conv.FixAllPointersToParentsAndChildren(false);
    ACT_exec::ACT_subsystem().instantiate_all(fileParser::currentParser().Instances);
	eval_intfc::get_act_lists().synchronize_all_lists();
}

pthread_mutex_t& builderProcessingRecord() {
	static pthread_mutex_t	pm;
	static bool		initialized = false;
	if(!initialized) {
		initialized = true;
		pthread_mutex_init(&pm, NULL); }
	return pm; }

pthread_cond_t& parserGotToEndOfRecord() {
	static bool		initialized = false;
	static pthread_cond_t	C;

	if(!initialized) {
		initialized = true;
		pthread_cond_init(&C, NULL); }
	return C; }

pthread_mutex_t& parserDoingItsJob() {
	static pthread_mutex_t	pm;
	static bool		initialized = false;
	if(!initialized) {
		initialized = true;
		pthread_mutex_init(&pm, NULL); }
	return pm; }

void lock_parserDoingItsJob() {
#ifdef DEBUG_THE_READER
	cout << "LOCKING\n";
#endif /* DEBUG_THE_READER */
	pthread_mutex_lock(&parserDoingItsJob());
}

void unlock_parserDoingItsJob() {
#ifdef DEBUG_THE_READER
	cout << "UNLOCKING\n";
#endif /* DEBUG_THE_READER */
	pthread_mutex_unlock(&parserDoingItsJob());
}

pthread_cond_t& builderWantsToIterate() {
	static bool		initialized = false;
	static pthread_cond_t	C;

	if(!initialized) {
		initialized = true;
		pthread_cond_init(&C, NULL); }
	return C;
}


static pthread_t	thePlanBuildingThread;

	//
	// defined below
	//
extern void*	plan_builder(void* This);

//
// This is the (main) process; it starts the plan-building
// thread, then starts parsing.
//
// The parse() method is invoked by fileReader::parse_xml_stream()
// which first creates an xml_reader object, passing it a pointer
// to a function (called gimme_a_char inside the xml_reader class)
// which reads from the actual XMLTOL file, one char at a time.
//
// Parsing is done by two parallel threads which are synchronized
// so that only one is active at any time.  xml_reader creates the
// new thread, which is responsible for top-level parsing task
// using the generic, templated method
//
//	parse_act_plan<xml_reader>(*the_reader)
//
// defined in act_plan_parser.H.
//
// The interaction between the thread and the main process goes
// like this:
//
//	main process				plan_builder
//	------------				------------
//
//	1ock parserDoingItsJob
//	lock builderProcessingRecord
//	spawn the plan_builder thread
//						start execution
//	create 
void xml_reader::parse() {

	lock_parserDoingItsJob();
	pthread_mutex_lock(&builderProcessingRecord());

	pthread_create(&thePlanBuildingThread, NULL, plan_builder, (void*) this);

	ptr_to_sax_parser = new XmlTolSaxParser;

	/* debug
	 * cout << "String to parse: " << start_parsing_here << "\n";
	 * */

	ptr_to_sax_parser->main_process_waits_for_thread_to_iterate(true);

	Cstring	errorMessage;
	try {
		ptr_to_sax_parser->parse(gimme_a_char);
	}

	//
	// the first two sources of error originate from the main (i. e. this) process:
	//
	catch(sax_parser::exception Err) {
		ptr_to_sax_parser->error_found = true;
		errorMessage << "xml_reader::parse(): error\n" << Err.message
			<< " on line " << Err.line << ", col " << Err.col << "\n";
#ifdef DEBUG_THE_READER
		cout << errorMessage;
#endif

	} catch(eval_error Err) {
		ptr_to_sax_parser->error_found = true;
		Cstring	errorMessage;
		errorMessage << "xml_reader::parse(): eval error\n"
			<< Err.msg << "\n";

#ifdef DEBUG_THE_READER
		cout << errorMessage;
#endif

	}

	//
	// this third source of error is the thread:
	//
	if(ptr_to_sax_parser->error_found) {
		errorMessage = ptr_to_sax_parser->error_message;
	}

	if(ptr_to_sax_parser->error_found) {

#ifdef DEBUG_THE_READER
		cout << "(main) " << ptr_to_sax_parser->indent_by_depth()
			<< "Done parsing - error found:\n" << errorMessage << "\n";
		cout << "(main) throwing exception.\n";
		cout.flush();
#endif

		delete ptr_to_sax_parser;
		throw(eval_error(errorMessage));
	}

#ifdef DEBUG_THE_READER
	else {
		cout << "(main) " << ptr_to_sax_parser->indent_by_depth()
			<< "Done parsing (no error)\n";
	}
#endif

	pthread_mutex_unlock(&builderProcessingRecord());

#ifdef DEBUG_THE_READER
	cout << "(main) " << ptr_to_sax_parser->indent_by_depth()
		<< "returning from parse()\n";

#endif

	unlock_parserDoingItsJob();
	delete ptr_to_sax_parser;
}

	//
	// This is the process running inside the plan building thread
	//
void*	plan_builder(void* This) {
	static apgen::RETURN_STATUS ret;
	xml_reader*	the_reader = (xml_reader*) This;

	pthread_mutex_lock(&builderProcessingRecord());
	lock_parserDoingItsJob();

	apgen::RETURN_STATUS retval;

	try {
		parse_act_plan /* <xml_reader> */ (*the_reader);
	} catch(eval_error Err) {
		the_reader->ptr_to_sax_parser->error_found = true;
		the_reader->ptr_to_sax_parser->error_message = *Err.msg;
	}

	//
	// we need to free up the main parser here
	//
	the_reader->ptr_to_sax_parser->
		plan_building_thread_waits_for_main_parser_to_get_to_end_of_record(
			true);

	return (void*) retval;
}

	/* This method is used by the plan-building thread. */
bool	xml_reader::iterate(bool&	isAnActivity,
				bool&	isAResource,
				bool&	isAConstraint,
				int*	lineno) {

	ptr_to_sax_parser->plan_building_thread_waits_for_main_parser_to_get_to_end_of_record();
	if(ptr_to_sax_parser->reached_the_end == true) {

#ifdef DEBUG_THE_READER
		cout << "(thread) iterate(): The parser has reached the end! "
			<< "returning false\n";
#endif

		if(lineno) {
			*lineno = ptr_to_sax_parser->line;
		}
		return false;
	}

#ifdef DEBUG_THE_READER
	cout << "(thread) iterate(line " << ptr_to_sax_parser->line
		<< "): parser's recordType is "
		<< ptr_to_sax_parser->recordType << "\n";
#endif

	if(lineno) {
		*lineno = ptr_to_sax_parser->line; }
	if(ptr_to_sax_parser->recordType == "activity") {
		isAnActivity = true;
	} else {
		isAnActivity = false;
	}
	if(ptr_to_sax_parser->recordType == "resource") {
		isAResource = true;
	} else {
		isAResource = false;
	}
	return true;
}

xml_reader::~xml_reader() {}

bool	xml_reader::verify_that_string_is_an_array(Cstring& errors) {
		return true; }

Cstring	xml_reader::act_get_id() {

#ifdef DEBUG_THE_READER
	cout << "(thread) act_get_id: ID = " << ptr_to_sax_parser->actID << "\n";
#endif

	return ptr_to_sax_parser->actID.c_str();
}

Cstring	xml_reader::act_get_type() {

#ifdef DEBUG_THE_READER
	cout << "(thread) act_get_type: Type = " << ptr_to_sax_parser->actType << "\n";
#endif

	return ptr_to_sax_parser->actType.c_str();
}

void	xml_reader::act_get_attribute_values(tvslist& L) {
	list<pair<string, flexval> >::iterator iter;
	for(	iter = ptr_to_sax_parser->instanceAttributes.begin();
		iter != ptr_to_sax_parser->instanceAttributes.end();
		iter++) {
		L << new TaggedValue(iter->first.c_str(),
			TypedValue::flexval2typedval(iter->second));

#ifdef DEBUG_THE_READER
		cout << "(thread) act_get_attribute_values: " << iter->first << " = " << iter->second << "\n";
#endif

	}
}

void	xml_reader::act_get_attribute_strings(pairslist&) {}

Cstring	xml_reader::act_get_name(pairtlist&) {

#ifdef DEBUG_THE_READER
	cout << "(thread) act_get_name: " << ptr_to_sax_parser->actName << "\n";
#endif

	return ptr_to_sax_parser->actName.c_str();
}

bool	xml_reader::act_has_parent() {

#ifdef DEBUG_THE_READER
	cout << "(thread) act_has_parent: parent = " << ptr_to_sax_parser->actParent << "\n";
#endif

	return ptr_to_sax_parser->actParent.size() > 0;
}

Cstring	xml_reader::act_get_parent(bool& abstractable, bool& abstracted) {

#ifdef DEBUG_THE_READER
	cout << "(thread) act_get_parent: " << ptr_to_sax_parser->actParent << "\n";
#endif

	if(ptr_to_sax_parser->actVisibility == "visible") {
		abstractable = true;
		abstracted = false;
	} else if(ptr_to_sax_parser->actVisibility == "abstracted") {
		abstractable = false;
		abstracted = true;
	}
	return ptr_to_sax_parser->actParent.c_str();
}

void	xml_reader::act_get_parameter_values(tvslist& L) {
	list<pair<string, flexval> >::iterator iter;
	for(	iter = ptr_to_sax_parser->parameters.begin();
		iter != ptr_to_sax_parser->parameters.end();
		iter++) {
		L << new TaggedValue(iter->first.c_str(),
			TypedValue::flexval2typedval(iter->second));
#		ifdef DEBUG_THE_READER
		cout << "(thread) act_get_parameter_values: " << iter->first << " = " << iter->second << "\n";
#		endif
	}
}

void	xml_reader::act_get_parameter_strings(pairtlist&) {}

Cstring	xml_reader::res_get_name() {
		return ptr_to_sax_parser->fullResName.str().c_str();
}

// need conversion from flexval to TypedValue
TypedValue xml_reader::res_get_value() {
	return TypedValue::flexval2typedval(ptr_to_sax_parser->curval[ptr_to_sax_parser->cur_depth + 2]);
}

bool	xml_reader::res_has_indices(pairslist& indices) {
		return false;
}

CTime_base* xml_reader::time_tag() {
		return &ptr_to_sax_parser->record_time;
}
