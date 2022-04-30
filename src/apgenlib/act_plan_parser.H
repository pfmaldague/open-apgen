#ifndef _ACT_PLAN_PARSER_H_
#define _ACT_PLAN_PARSER_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "ACT_exec.H"
#include "APdata.H"
#include "fileReader.H"
#include "generic_parser.H"
#include "RES_def.H"
#include "xml_act_parser.H"

// #define DEBUG_THE_READER

/* We define a structure that contains all the activity data we are
 * going to need to call process_activity_instance(). This is
 * necessary because the expression parser is not reentrant. */
class process_act_inst_call_argsPLD {
public:
    fileReader::act_viz			Visibility;
    fileReader::act_sort		request_or_chameleon;
    pairslist				string_attributes;
    tvslist				tv_attributes;
    pairtlist				unsorted_parameters;
    pairslist				the_parameters;
    tvslist				tv_parameters;
    Cstring				activityName;
    Cstring				activityTypeName;
    stringslist				theAbstractableAncestors;
    stringslist				theAbstractedAncestors;
    stringslist				theDecomposableChildren;
    stringslist				theDecomposedChildren;
    Cstring				ID_for_this_file;
    CTime_base				start_time;
    int					line_where_defined;

    process_act_inst_call_argsPLD()
	: Visibility(fileReader::TBD),
		request_or_chameleon(fileReader::regular_act) {}
    process_act_inst_call_argsPLD(const process_act_inst_call_argsPLD& p)
	: Visibility(p.Visibility),
		line_where_defined(0),
		request_or_chameleon(p.request_or_chameleon),
		string_attributes(p.string_attributes),
		unsorted_parameters(p.unsorted_parameters),
		the_parameters(p.the_parameters),
		activityName(p.activityName),
		activityTypeName(p.activityTypeName),
		theAbstractableAncestors(p.theAbstractableAncestors),
		theAbstractedAncestors(p.theAbstractedAncestors),
		theDecomposableChildren(p.theDecomposableChildren),
		theDecomposedChildren(p.theDecomposedChildren),
		start_time(p.start_time),
		ID_for_this_file(p.ID_for_this_file) {}
    ~process_act_inst_call_argsPLD() {}
};

typedef Cnode0<alpha_int, process_act_inst_call_argsPLD> pai_args_node;

class resourceConsumptionInfoPLD {
public:
    Rsource*	res;
    TypedValue	consumption;

    resourceConsumptionInfoPLD() : res(NULL) {}
    resourceConsumptionInfoPLD(const resourceConsumptionInfoPLD& r) : res(r.res), consumption(r.consumption) {}
    resourceConsumptionInfoPLD(Rsource* R) : res(R) {}
    ~resourceConsumptionInfoPLD() {}
};

typedef Cnode0<alpha_void, resourceConsumptionInfoPLD> resourceConsumptionInfo;

//
// Implemented in xml_reader.C:
//
extern void
parse_act_plan(
	generic_parser& givenParser);

template <class parserClass>
void
parse_incon(
	parserClass& givenParser) {
    parserClass&					parser(givenParser);
    stringslist					context;
    Rsource*					res;

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
	context << new emptySymbol("iterating over resources");

	bool finalRec = false;
	bool globalRec = false;
	while(parser.iterate(finalRec, globalRec)) {

	    //
	    // get the record time tag
	    //
	    if(finalRec) {
		Cstring				Name = parser.res_get_name();
		TypedValue			Value = parser.res_get_value();
		TypedValue			SetToValue = parser.res_get_set_to_value();
		pairslist			indices;
		long				count;
		CTime_base&			timeTag(*parser.time_tag());
		resourceConsumptionInfo*	rci;
		TypedValue			prev_value;
		bool				there_is_a_prev_value = false;
		Rsource*			res = eval_intfc::FindResource(Name);

		if(!res) {
			Cstring	e("XML TOL parser: cannot find resource ");
			e << Name;
			throw(eval_error(e));
		}
		RES_state*		state_res = dynamic_cast<RES_state*>(res);
		RES_settable*		settable_res = dynamic_cast<RES_settable*>(res);
		RES_consumable*		cons_res = dynamic_cast<RES_consumable*>(res);
		RES_nonconsumable*	noncons_res = dynamic_cast<RES_nonconsumable*>(res);
		if(state_res) {
			if(!parser.res_get_usage_depth()) {
				state_res->append_to_history(
					SetToValue,
					timeTag,
					timeTag,
					0,
					0,
					apgen::USAGE_TYPE::SET);
			} else {
				state_res->append_to_history(
					Value,
					timeTag,
					timeTag,
					0,
					0,
					apgen::USAGE_TYPE::SET);
			}
			// debug
			// cout << "got state resource " << fullNameString << "\n";
		} else if(cons_res) {
			TypedValue	currentval;
			TypedValue	ZeroValue;
			CTime_base	time_now(time_saver::get_now());

			cons_res->evaluate_present_value(
				currentval,
				time_now);

			// debug
			// Cstring currentTemp;
			// currentval.print(currentTemp);
			// Cstring temp;
			// Value.print(temp);
			// cout << "parse_incon: " << fullNameString << " - time " << time_now.to_string()
			// 	<< ", value from XMLTOL " << temp << ", currentval = " << currentTemp << "\n";

			if(cons_res->get_datatype() == apgen::DATA_TYPE::FLOATING) {
				double	P = currentval.get_double();
				// should satisfy Value = P - consumption;
				double	correction = P - Value.get_double();
				Value = correction;
				ZeroValue = (double) 0.0;
			} else if(cons_res->get_datatype() == apgen::DATA_TYPE::INTEGER) {
				long	P = currentval.get_int();
				// should satisfy Value = P - consumption;
				long	correction = P - Value.get_int();
				Value = correction;
				ZeroValue = (long) 0;
			} else if(cons_res->get_datatype() == apgen::DATA_TYPE::TIME || cons_res->get_datatype() == apgen::DATA_TYPE::DURATION) {
				CTime_base	P = currentval.get_time_or_duration();
				CTime_base	correction = P - Value.get_time_or_duration();
				CTime_base	zeroduration(0, 0, true);
				Value.declared_type = apgen::DATA_TYPE::DURATION;
				Value = correction;
				ZeroValue = zeroduration;
			}

			cons_res->append_to_history(
					ZeroValue,
					time_now,
					time_now,
					0,
					0,
					apgen::USAGE_TYPE::USE);
			cons_res->append_to_history(
					Value,
					time_now,
					time_now,
					0,
					0,
					apgen::USAGE_TYPE::USE);
		} else if(noncons_res) {
			Cstring errs;
			errs << "Attempting to set INCON for nonconsumable resource "
				<< Name;
			throw(eval_error(errs));
		}
	    } else if(globalRec) {
		TypedValue			the_globals = parser.get_all_globals();
		ListOVal*			lov = &the_globals.get_array();
		ArrayElement*			ae;

		ae = lov->first_node();
		for(int z = 0; z < lov->get_length(); z++) {
			ae = (*lov)[z];
			Cstring	globname = ae->get_key();
			try {
				TypedValue&	one_global = globalData::get_symbol(globname);
				one_global = ae->Val();
			} catch(eval_error Err) {
				Cstring tmp("global ");
				tmp << globname << " defined in the INCON but has not been defined in the AAF(s)";
				throw(eval_error(tmp));
			}
		}
	    }
	}
    }
    catch(eval_error Err) {
	Cstring			any_errors;
	stringslist::iterator	error_it(context);
	emptySymbol*		s;

	any_errors << "Uh Oh: " << parser.parser_name() << " parsing error\n\tcontext:\n";
	while((s = error_it())) {
		any_errors << "\t\t" << s->get_key() << "\n";
	}
	any_errors << "\terror:\n" << Err.msg << "\n";
	throw(eval_error(any_errors));
    }
}

#endif /* _ACT_PLAN_PARSER_H_ */
