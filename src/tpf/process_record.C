#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <string>

#include "tolcomp_grammar.H"
#include "tolcomp_tokens.H"
#include "tolReader.H"
#include "tol_expressions.H"
#include "tol_data.H"
#include "json.h"

#include "APdata.H" // for removeQuotes()

using namespace tol;
using namespace std;

//
// Defined in the apcore library:
//
extern thread_local int thread_index;

//
// Defined in tol_system.C:
//
extern void transfer_act_to_json(
		CTime_base	time_tag,
		CTime_base	duration,
		bool		visibility,
		const Cstring&	namestring,
		const Cstring&	uniqueID,
		const Cstring&	act_type,
		const Cstring&	description,
		const Cstring&	legend,
		const Cstring&	parent,
		const flexval&	params,
		const flexval&	attrs);


flexval
tol::tolarray2flexval(const tolExp& raw_value) {
    ValueArray* A = dynamic_cast<ValueArray*>(raw_value.object());
    assert(A);
    flexval F;
    tol_value* tv = dynamic_cast<tol_value*>(A->rest_of_list.object());
    KeywordValuePair* kv = dynamic_cast<KeywordValuePair*>(A->rest_of_list.object());
    if(tv) {
	F[0] = tolexp2flexval(tv->TOLValue);
	vector<tolExp>& exp = tv->expressions;
	for(int i = 0; i < exp.size(); i++) {
	    tol_value* other_tv = dynamic_cast<tol_value*>(exp[i].object());
	    assert(other_tv);
	    F[i + 1] = tolexp2flexval(other_tv->TOLValue);
	}
    } else if(kv) {
	Cstring tmp(kv->getData());
	removeQuotes(tmp);
	string key(*tmp);
	F[key] = tolexp2flexval(kv->TOLValue);
	vector<tolExp>& exp = kv->expressions;
	for(int i = 0; i < exp.size(); i++) {
	    KeywordValuePair* other_kv = dynamic_cast<KeywordValuePair*>(exp[i].object());
	    assert(other_kv);
	    tmp = other_kv->getData();
	    removeQuotes(tmp);
	    string other_key(*tmp);
	    F[other_key] = tolexp2flexval(other_kv->TOLValue);
	}
    } else {

	//
	// Empty list
	//
	// assert(false);
    }
    return F;
}

//
// We use flexval instances to store values from the parser into the
// activity, resource and constraint databases; this is the main
// conversion routine: 
//
flexval
tol::tolexp2flexval(const tolExp& raw_value) {

    //
    // capture the value and its type
    //
    assert(raw_value);
    ValueFloat*		floatval =	dynamic_cast<ValueFloat*>	(raw_value.object());
    ValueInteger*	intval =	dynamic_cast<ValueInteger*>	(raw_value.object());
    ValueTime*		timeval =	dynamic_cast<ValueTime*>	(raw_value.object());
    ValueDuration*	durationval =	dynamic_cast<ValueDuration*>	(raw_value.object());
    ValueTrue*		trueval =	dynamic_cast<ValueTrue*>	(raw_value.object());
    ValueFalse*		falseval =	dynamic_cast<ValueFalse*>	(raw_value.object());
    ValueString*	stringval =	dynamic_cast<ValueString*>	(raw_value.object());
    ValueArray*		arrayval =	dynamic_cast<ValueArray*>	(raw_value.object());

    if(floatval) {
	double V;
	sscanf(*floatval->tok_float->getData(), "%lf", &V);
	if(floatval->optional_minus) {
	    V = -V;
	}
	return flexval(V);
    } else if(intval) {
	long int I;
	sscanf(*intval->tok_integer->getData(), "%ld", &I);
	if(intval->optional_minus) {
	    I = -I;
	}
	return flexval(I);
    } else if(timeval) {
	CTime_base T(timeval->tok_time->getData());
	return flexval(cppTime(T));
    } else if(durationval) {
	CTime_base T(durationval->tok_duration->getData());
	return flexval(cppDuration(T));
    } else if(trueval) {
	bool B = true;
	return flexval(B);
    } else if(falseval) {
	bool B = false;
	return flexval(B);
    } else if(stringval) {
	Cstring S = stringval->tok_string->getData();
	removeQuotes(S);
	return flexval(string(*S));
    } else if(arrayval) {
	return tolarray2flexval(raw_value);
    }
    return flexval();
}

Cstring	extract_name(
		tol::TOLResValue*	res,
		Cstring& containername) {

	//
	// Get the full resource name
	//
	containername = res->tok_sym->getData();
	Cstring index;
	if(res->indices) {
	    aoString aos;
	    Indices* indices = dynamic_cast<Indices*>(
		res->indices.object());
	    assert(indices);
	    aos << "[" << indices->getData() << "]";

	    pE* the_indices = dynamic_cast<pE*>(indices->TOLindices.object());
	    for(int i = 0; i < the_indices->expressions.size(); i++) {
		aos << "[" << the_indices->expressions[i]->getData() << "]";
	    }
	    index << aos.str();
	}
	Cstring resname;
	resname << containername << index;
	return resname;
}

void process_a_tol_record(
		tolNode*		pe,
		const tol_reader::cmd&	CMD,
		CTime_base&		time_tag,
		CTime_base&		previous_time,
		int			parser_index) {
    TimeStamp*		ts = dynamic_cast<TimeStamp*>(pe->payload.object());
    assert(ts);
    pE*			content = ts->record_content.object();

    TOLResValue*	res =		dynamic_cast<TOLResValue*>(content);
    TOLActStart*	act_start =	dynamic_cast<TOLActStart*>(content);
    TOLActEnd*		act_end =	dynamic_cast<TOLActEnd*>(content);

    bool		out_of_order = false;

    time_tag = CTime_base(ts->time_stamp->getData());

#   ifdef DEBUG_THIS
    Cstring exclstr;
#   endif /* DEBUG_THIS */

    //
    // If CMD.Code is Open, the focus is on resources.
    // Resource records are analyzed in depth; for activity
    // records, we only check out-of-orderness for now.
    //
    // If CMD.Code is ToJson, the focus is on activities.
    // We only translate activities for the time being.
    //
    if(CMD.Code == tol_reader::Directive::Open && res) {
	Cstring containername;
	Cstring resname = extract_name(res, containername);

	//
	// Get the resource index for this resource; it does not
	// exist if the resource is new.
	//
	map<Cstring, int>::const_iterator res_iter
		= resource::resource_indices(parser_index).find(resname);

	//
	// Placeholder: Flag out-of-order values
	//
	if(time_tag < previous_time) {
	    out_of_order = true;
	    cout << "resource " << resname << " out of order; this = "
		<< time_tag.to_string() << ", prev = "
		<< previous_time.to_string() << "\n";
	}

	//
	// For the time being we don't look at Scope options in detail:
	//	if(CMD.parsing_scope == FirstError)
	//	if(CMD.parsing_scope == RecordCount)
	//	if(CMD.parsing_scope == CPUTime)
	//	if(CMD.parsing_scope == SCETTime)
	//
	if(CMD.parsing_scope == tol_reader::Scope::All
		|| CMD.parsing_scope == tol_reader::Scope::CPUTime) {

	    tol::resource* the_resource = NULL;
	    int res_index = -1;
	    if(res_iter != resource::resource_indices(parser_index).end()) {
		res_index = res_iter->second;
		the_resource = tol::resource::resource_data(parser_index)[res_index].get();
	    } else {
		Cstring err;
		err << "Resource " << resname
		    << " appears in a record but is not defined.";
		throw(eval_error(err));
	    }

	    flexval	val = tolexp2flexval(res->TOLValue);

	    //
	    // At this point, we have a pointer to the resource object
	    // (the_resource), the time stamp of the record (time_tag)
	    // and the resource value (val). We are going to append
	    // this information to the resource history if requested.
	    //

	    //
	    // add to history
	    //
	    the_resource->History.append(new history<flexval>::flexnode(time_tag, val));
 
#	    ifdef DEBUG_THIS
		const CTime_base& curtime = the_resource->History.currenttime();
		const flexval& curval	  = the_resource->History.currentval();
		exclstr << time_tag.to_string() << " - " << resname << "(type="
		    << flexval::type2string(val.getType()) << ") "
		    << Cstring(curval.to_string().c_str()) << " -> "
		    << Cstring(val.to_string().c_str())
		    << " hist size = " << the_resource->History.values.get_length()
		    << "\n";
		tolexcl << exclstr;
		exclstr.undefine();
#	    endif /* DEBUG_THIS */

	}
    } else if(act_start) {

	if(CMD.Code == tol_reader::Directive::Open) {

	    //
	    // Flag out-of-order values
	    //
	    if(time_tag < previous_time) {
		out_of_order = true;
		cout << "act start " << act_start->tok_sym->getData()
		     << " out of order; this = "
		     << time_tag.to_string() << ", prev = "
		     << previous_time.to_string() << "\n";
	    }
	} else if(CMD.Code == tol_reader::Directive::ToJson) {

	    //
	    // We would like to build activity records in json
	    // format, using logic similar to what is today in
	    // exec_agent::WriteOneActToJsonString(). Here are
	    // the fields that need to be filled in order to
	    // use that logic:
	    //
	    // 	bool			act_is_visible;
	    // 	Cstring			namestring;
	    // 	ActivityInstance*	theParent;
	    // 	Cstring			uniqueID;
	    // 	Cstring			type;
	    // 	tvslist			attributes;
	    // 	tvslist			parameters;
	    //
	    // The types listed above are those used by the
	    // method in exec_agent, which has direct access
	    // to the ActivityInstance class. In our case,
	    // we have access to similar objects encoded in
	    // the TOL record.
	    //
	    Cstring	namestring = act_start->activity_name->getData();
	    Cstring	uniqueID = act_start->activity_id->getData();
	    Cstring	description = act_start->tok_string_22->getData();
	    removeQuotes(description);
	    Cstring	act_type = act_start->tok_sym_12->getData();
	    Cstring	vis_string = act_start->tok_string->getData();
	    removeQuotes(vis_string);
	    bool	visibility = vis_string == "VISIBLE";
	    Cstring	legend = act_start->tok_string_18->getData();
	    removeQuotes(legend);
	    Cstring	parent = act_start->tok_sym_38->getData();
	    CTime_base	duration(act_start->tok_duration->getData());

#	    ifdef DEBUG_THIS
	    cerr << "\n";
	    cerr << "name: " << namestring << " ID: " << uniqueID << " descr: " << description
		<< "\n\tvisibility: " << visibility << ", " << vis_string << " type: " << act_type
		<< " legend: " << legend << " parent: " << parent << "\n";
#	    endif /* DEBUG_THIS */

	    Attributes*	atts = dynamic_cast<Attributes*>(act_start->attributes.object());
	    Parameters*	pars = dynamic_cast<Parameters*>(act_start->rest_of_parameters.object());
	    assert(atts);
	    Attribute* one_att = dynamic_cast<Attribute*>(atts->one_attribute.object());
	    assert(one_att);
	    Cstring	att_name = one_att->tok_string->getData();
	    removeQuotes(att_name);
	    flexval	att_val = tolexp2flexval(one_att->TOLValue);

	    flexval	attrs;

#	    ifdef DEBUG_THIS
	    cerr << act_start->activity_id->getData() << " attributes:\n";
	    cerr << "\t" << att_name << " = " << att_val << "\n";
#	    endif /* DEBUG_THIS */

	    attrs[string(*att_name)] = att_val;
	    for(int k = 0; k < atts->expressions.size(); k++) {
		one_att = dynamic_cast<Attribute*>(atts->expressions[k].object());

		//
		// one_att could be a comma
		//
		if(one_att) {
		    att_name = one_att->tok_string->getData();
		    removeQuotes(att_name);
		    att_val = tolexp2flexval(one_att->TOLValue);
		    attrs[string(*att_name)] = att_val;

#		    ifdef DEBUG_THIS
		    cerr << "\t" << att_name << " = " << att_val << "\n";
#		    endif /* DEBUG_THIS */
		}
	    }

	    flexval params;

	    if(pars) {
		Parameter* one_par = dynamic_cast<Parameter*>(pars->one_parameter.object());
		Cstring par_name = one_par->tok_sym->getData();
		flexval	par_val = tolexp2flexval(one_par->TOLValue);

#	        ifdef DEBUG_THIS
		cerr << act_start->activity_id->getData() << " parameters:\n";
		cerr << "\t" << par_name << " = " << par_val << "\n";
#	        endif /* DEBUG_THIS */

		for(int k = 0; k < pars->expressions.size(); k++) {
		    one_par = dynamic_cast<Parameter*>(pars->expressions[k].object());

		    //
		    // one_par could be a comma
		    //
		    if(one_par) {
			par_name = one_par->tok_sym->getData();
			par_val = tolexp2flexval(one_par->TOLValue);
			params[string(*par_name)] = par_val;

#	    		ifdef DEBUG_THIS
			cerr << "\t" << par_name << " = " << par_val << "\n";
#	    		endif /* DEBUG_THIS */
		    }
		}
	    }

	    //
	    // We now want to create an activity object and add it
	    // to the master json container for this particular
	    // subsystem.
	    //
	    // Object creation is handled inside transfer_act_to_json().
	    //
	    transfer_act_to_json(
		time_tag,
		duration,
		visibility,
		namestring,
		uniqueID,
		act_type,
		description,
		legend,
		parent,
		params,
		attrs);
	}
    } else if(act_end) {

	if(CMD.Code == tol_reader::Directive::Open) {

	    //
	    // Flag out-of-order values
	    //
	    if(time_tag < previous_time) {
		out_of_order = true;
		cout << "act end   " << act_end->tok_sym->getData()
		     << " out of order; this = "
		     << time_tag.to_string() << ", prev = "
		     << previous_time.to_string() << "\n";
	    }
	} else if(CMD.Code == tol_reader::Directive::ToJson) {
	}
    }
    if(!out_of_order) {
	previous_time = time_tag;
    }
}


