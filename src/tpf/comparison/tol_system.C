#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "tolReader.H"
using namespace tol;
#include "gen_tol_exp.C"
#include "tol_data.H"
#include "json.h"


const char* tol_reader::spell(Directive d) {
    switch(d) {
	case Directive::Open:
	    return "Open";
	case Directive::Peek:
	    return "Peek";
	case Directive::Read:
	    return "Read";
	case Directive::Filter:
	    return "Filter";
	case Directive::Output:
	    return "Output";
	default:
	    return "Unknown";
    }
}

const char* tol_reader::spell(State s) {
    switch(s) {
	case State::Start:
	    return "Start";
	case State::Initial:
	    return "Initial";
	case State::Partial:
	    return "Partial";
	case State::Final:
	    return "Final";
	default:
	    return "Unknown";
    }
}

const char* tol_reader::spell(Granularity g) {
    switch(g) {
	case Granularity::Pass_Fail:
	    return "Pass_Fail";
	case Granularity::ErrorCount:
	    return "ErrorCount";
	case Granularity::Detailed:
	    return "Detailed";
	case Granularity::Full:
	    return "Full";
	default:
	    return "Unknown";
    }
}

const char* tol_reader::spell(Scope s) {
    switch(s) {
	case Scope::HeaderOnly:
	    return "HeaderOnly";
	case Scope::FirstError:
	    return "FirstError";
	case Scope::RecordCount:
	    return "RecordCount";
	case Scope::CPUTime:
	    return "CPUTime";
	case Scope::SCETTime:
	    return "SCETTime";
	case Scope::All:
	    return "All";
	default:
	    return "Unknown";
    }
}

FILE*			tol_reader::tol_file[2];

std::atomic<bool>&	tol_reader::file_extraction_complete(int index) {
	static std::atomic<bool>	fpc2[10];
	return fpc2[index];
}

std::atomic<bool>&	tol_reader::ActMetadataCaptured(int index) {
	static std::atomic<bool>	B[10];
	return B[index];
}

std::atomic<bool>&	tol_reader::ResMetadataCaptured(int index) {
	static std::atomic<bool>	B[10];
	return B[index];
}

std::atomic<bool>&	tol_reader::errors_found() {
	static std::atomic<bool>	B { false };
	return B;
}

double&			tol::abs_tolerance() {
	static double T = 1.0e-15;
	return T;
}
double&			tol::rel_tolerance() {
	static double T = 1.0e-15;
	return T;
}

CTime_base&		tol::earliest_time_stamp(int index) {
	static CTime_base T[10];
	return T[index];
}

map<Cstring, int>&	tol::activity_type::activity_indices(int index) {
	static map<Cstring, int> M[10];
	return M[index];
}

vector<unique_ptr<tol::activity_type> >& tol::activity_type::activity_data(int index) {
	static vector<unique_ptr<tol::activity_type> > V[10];
	return V[index];
}

//
// Activity-based array of information extracted from auxiliary
// files (files ending in .tol.aux). This information documents the
// 'shape' of activity parameters that are APGenX arrays, and can
// translate into json lists or structs.
//
map<string, flexval>&			tol_reader::aux_type_info() {
	static map<string, flexval> m;
	return m;
}

//
// We emulate ACT_exec_json.C in defining subsystem-based
// containers of json information:
//
tlist<alpha_string, subsysNode>&	tol_reader::subsystem_based_list() {
	static tlist<alpha_string, subsysNode> T;
	return T;
}

map<Cstring, int>&	tol::resource::resource_indices(int index) {
	static map<Cstring, int> M[10];
	return M[index];
}

vector<unique_ptr<tol::resource> >&	tol::resource::resource_data(int index) {
	static vector<unique_ptr<tol::resource> > V[10];
	return V[index];
}

bool tol::resource::same_within_tolerance(const flexval& v1, const flexval& v2) {
	flexval::Type type1 = v1.getType();
	flexval::Type type2 = v2.getType();
	if(type1 != type2) {
		return false;
	}
	if(type1 == flexval::TypeDouble) {
		double d1 = (double)v1;
		double d2 = (double)v2;
		double diff = fabs(d1 - d2);
		double halfsum = 0.5 * (fabs(d1) + fabs(d2));

		//
		// Previously, we used diff itself.  However, that is incorrect
		// since each resource can in principle deviate from the ideal
		// value by the full tolerance.
		//
		if(0.5 * diff > AbsTolerance + RelTolerance * halfsum) {
			return false;
		}
		return true;
	}
	return v1 == v2;
}

Cstring tol::resource::get_info() {
    aoString aos;
    aos << name;

    ostringstream extra_info;
    map<string, flexval>& atts = attributes.get_struct();
    map<string, flexval>::const_iterator attiter;
    attiter = atts.find("Type");
    extra_info << "type=" << attiter->second;
    attiter = atts.find("DataType");
    if(attiter != atts.end() && ((string)attiter->second) == "float") {
	if((attiter = atts.find("Interpolation")) != atts.end()
	    && ((string)attiter->second) == "linear") {
	    extra_info << ", ";
	    extra_info << "interpolation=" << attiter->second;
	}
	if((attiter = atts.find("Min Rel Delta")) != atts.end()) {
	    extra_info << ", ";
	    extra_info << "min_rel_delta=" << attiter->second;
	}
	if((attiter = atts.find("Min Abs Delta")) != atts.end()) {
	    extra_info << ", ";
	    extra_info << "min_abs_delta=" << attiter->second;
	}
    }
    aos << "(" << extra_info.str().c_str() << ")";
    return aos.str();
}

std::atomic<tolNode*>&	pE::LastParsedRecord(int index) {
	static std::atomic<tolNode*>	lpr[10];
	return lpr[index];
}

std::atomic<tolNode*>&	pE::LastProcessedRecord(int index) {
	static std::atomic<tolNode*>	lpr2[10];
	return lpr2[index];
}

std::atomic<tolNode*>&	pE::FirstRecordToProcess(int index) {
	static std::atomic<tolNode*>	frp[10];
	return frp[index];
}

std::atomic<bool>&	pE::file_parsing_complete(int index) {
	static std::atomic<bool>	fpc[10];
	return fpc[index];
}

slist<alpha_int, tolNode>&	pE::theRecords(int index) {
	static slist<alpha_int, tolNode> L[10];
	return L[index];
}

slist<alpha_int, tolNode>&	pE::theActTypes(int index) {
	static slist<alpha_int, tolNode> L[10];
	return L[index];
}

std::atomic<bool>&		pE::terminate_parsing(int index) {
	static std::atomic<bool>	B[10];
	return B[index];
}

bool&			pE::first_record(int index) {
	static bool	B[10];
	return B[index];
}

std::atomic<long int>&		pE::first_time_tag(int index) {
	static atomic<long int>	C[10];
	return C[index];
}

//
// The purpose of this method is to transfer record information,
// which is held inside the Record instances in the TOL parser's
// internal database, into a global structure that is not going
// to go away when the re-entrant parser terminates and deletes
// all its internal data.
//
// Internal parser data is organized as instances of the tol:pE
// class. Such instances can exist outside of the parsing function.
// However, they can only be deleted by the thread that created
// them, because of the peculiar structure of smart pointers which
// was designed for maximum efficiency in multi-threading. That's
// why the launch_parser() function (in tol_interpreter.C) destroys
// static parsing objects prior to exiting. 
//
void Record::transfer_records_so_far(bool last_time /* = false */ ) {
    long int recno = expressions.size();
    int max_records = 1000;
    int parser_index = which_parser(thread_index);

#   ifdef DEBUG_THIS
	Cstring str;
#   endif /* DEBUG_THIS */

    //
    // Transfer the very first record, if applicable
    //
    if(record) {
	theRecords(parser_index) << new tolNode(0L, record);
	record.dereference();
	max_records--;
    }

    if(last_time) {
	max_records = recno;
    }

    if(last_time || recno % max_records == 0) {

#	ifdef DEBUG_THIS
	Cstring str;
	str << "transfer: storing " << max_records << " records\n";
	tolexcl << str;
	str.undefine();
#	endif /* DEBUG_THIS */

	//
	// First thing to do is to transfer all records to the list
	//
	for(int i = 0; i < max_records; i++) {
	    theRecords(parser_index) << new tolNode(0L, expressions[i]);
	}
	expressions.clear();
	assert(expressions.size() == 0);

	tolNode* last_parsed_record = LastParsedRecord(parser_index).load();

	//
	// The very first time we execute, this pointer
	// will be null; in every subsequent call, it
	// will point to a valid record. Therefore, we
	// use this is a flag telling us we need to initialize
	// pointers:
	//
	if(!last_parsed_record) {

	    //
	    // This pointer is for our benefit; it tells us
	    // where to start deleting records after the
	    // trailing thread no longer needs them:
	    //
	    first_record_to_delete = theRecords(parser_index).first_node();

	    //
	    // This pointer is for the benefit of the trailing
	    // thread; it tells it where in the list it should
	    // start extracting records:
	    //
	    FirstRecordToProcess(parser_index).store(first_record_to_delete);
	}

	//
	// Update the pointer to the last record we parsed.
	//
	// This pointer is for the benefit of the extraction
	// thread(s); it tells it how far we've gone in
	// parsing the TOL:
	//
	LastParsedRecord(parser_index).store(theRecords(parser_index).last_node());

#	ifdef DEBUG_THIS
	tolNode* tN = LastParsedRecord(parser_index).load();
	TimeStamp* ts = dynamic_cast<TimeStamp*>(tN->payload.object());
	assert(ts);
	str << "setting last = " << ts->time_stamp->getData() << "\n";
	tolexcl << str;
	str.undefine();
#	endif /* DEBUG_THIS */

	//
	// Set by the extraction thread(s) (except for the first time)
	// to tell us where it plans to start extracting the next
	// batch. Any records prior to that are safe for us to delete.
	//
	tolNode* first_record_to_process
		= FirstRecordToProcess(which_parser(thread_index)).load();
	tolNode* next;

	while(true) {
	    if(first_record_to_delete == first_record_to_process) {

		//
		// We have reached the point where the extraction
		// thread(s) have stopped; leave the remaining
		// records untouched
		//
		break;
	    }
	    next = first_record_to_delete->next_node();
	    delete first_record_to_delete;
	    first_record_to_delete = next;
	}

	//
	// Now we resume parsing. We will update theRecords() after
	// we've parsed 1000 new records.
	//
    }
}

//
// This method is used by the main (parsing) thread whenever
// a single record has been fully parsed; the successfully
// parsed record is 'added' to the Record.expressions vector.
//
// We don't want to keep accumulating records forever; instead,
// we pass them on to the trailing (extraction) thread which
// will analyze the content.
//
// For efficiency, we don't pass every single record immediately;
// we buffer 1000 records and send a signal to the trailing
// thread that there is work to do. The slist called
// pE::theRecords() holds the records we want the trailing
// thread to process.
//
// In principle, theRecords() could grow very large if it
// took a lot of time to process records, but in pratice the
// trailing thread is faster, so records don't really
// accumulate much.
//
void Record::addExp(const tolExp& pe) {

    assert(pe.object());

    //
    // Do this first, so that extraction thread(s) can
    // access the item with index recno:
    //
    expressions.push_back(pe);

    transfer_records_so_far();
}

void ActivityData::capture_activity_data() {
    tolExp& meta = act_metadata;
    OneActivityDatum* datum = dynamic_cast<OneActivityDatum*>(meta.object());
    assert(datum);
    int parser_index = which_parser(thread_index);

    theActTypes(parser_index) << new tolNode(0L, tolExp(datum));
    for(long int i = 0; i < datum->expressions.size(); i++) {
	OneActivityDatum* other_datum
	    = dynamic_cast<OneActivityDatum*>(datum->expressions[i].object());
	assert(other_datum);
	theActTypes(parser_index) << new tolNode(i + 1L, tolExp(other_datum));
    }
    tol_reader::ActMetadataCaptured(parser_index).store(true);
}

void OneResourceDatum::capture_resource_data() {
    // tolExp& meta = res_metadata;
    // OneResourceDatum* datum = dynamic_cast<OneResourceDatum*>(meta.object());
    // assert(datum);
    int parser_index = which_parser(thread_index);

    //
    // Extract container name and indices, if any, using
    // the data members of OneResourceDatum
    //
    Cstring containername = tok_sym->getData();
    Cstring index;
    if(indices) {
	aoString aos;
	Indices* indices_obj = dynamic_cast<Indices*>(indices.object());
	assert(indices_obj);
	aos << "[" << indices_obj->getData() << "]";

	pE* the_indices = dynamic_cast<pE*>(indices_obj->TOLindices.object());
	for(int i = 0; i < the_indices->expressions.size(); i++) {
		aos << "[" << the_indices->expressions[i]->getData() << "]";
	}
	index << aos.str();
    }
    flexval atts = tolarray2flexval(TOLArray);
    Cstring resname;
    resname << containername << index;
    map<Cstring, int>::const_iterator res_iter
		= resource::resource_indices(parser_index).find(resname);
    int	res_index = resource::resource_data(parser_index).size();

    if(res_iter == resource::resource_indices(parser_index).end()) {
	resource::resource_indices(parser_index)[resname] = res_index;

	//
	// creates an empty unique_ptr<resource>:
	//
	resource::resource_data(parser_index).push_back(NULL);

#	ifdef DEBUG_THIS
	Cstring str;
	str << "capture_resource_data(): defining resource["
	    << res_index << "] = " << resname << "\n";
	tolexcl << str;
	str.undefine();
	map<string, flexval>& A = atts.get_struct();
	map<string, flexval>::const_iterator att_iter;
	for(att_iter = A.begin(); att_iter != A.end(); att_iter++) {
	    str << "    " << att_iter->first << " = "
		<< att_iter->second.to_string() << "\n";
	    tolexcl << str;
	    str.undefine();
	}
#	endif /* DEBUG_THIS */
    } else {
	Cstring err;
	err << "Resource " << resname
	    << " has more than one record in the initial section.";
	throw(eval_error(err));
    }

    //
    // Extract resource type, data type and tolerance(s) if any
    //
    map<string, flexval>& A = atts.get_struct();
    map<string, flexval>::const_iterator attiter;
    attiter = A.find("Type");
    Cstring restype = ((string)attiter->second).c_str();
    attiter = A.find("DataType");
    Cstring resdatatype = ((string)attiter->second).c_str();
    flexval::Type flextype;
    if(resdatatype == "float") {
	flextype = flexval::TypeDouble;
    } else if(resdatatype == "integer") {
	flextype = flexval::TypeInt;
    } else if(resdatatype == "string") {
	flextype = flexval::TypeString;
    } else if(resdatatype == "boolean") {
	flextype = flexval::TypeBool;
    } else if(resdatatype == "duration") {
	flextype = flexval::TypeDuration;
    } else if(resdatatype == "time") {
	flextype = flexval::TypeTime;
    } else {
	assert(false);
    }
    attiter = A.find("Interpolation");
    bool interp;
    string interpstr = (string)attiter->second;
    if(interpstr == "linear") {
	interp = true;
    } else if(interpstr == "constant") {
	interp = false;
    } else {
	assert(false);
    }

    //
    // Override user-supplied tolerances with values from the
    // TOL header, if supplied, and only if greater than the
    // user-supplied value:
    //
    double rel_tolerance = tol::rel_tolerance();
    double abs_tolerance = tol::abs_tolerance();
    if((attiter = A.find("Min Rel Delta")) != A.end()) {
	if(((double)attiter->second) > rel_tolerance) {
	    rel_tolerance = attiter->second;
	}
	// rel_tolerance = (double) attiter->second;
    }
    if((attiter = A.find("Min Abs Delta")) != A.end()) {
	if(((double)attiter->second) > abs_tolerance) {
	    abs_tolerance = attiter->second;
	}
	// abs_tolerance = (double) attiter->second;
    }
    tol::resource* new_resource = new tol::resource(
		resname,
		containername,
		restype,
		flextype,
		abs_tolerance,
		rel_tolerance,
		interp,
		atts);
    resource::resource_data(parser_index)[res_index].reset(new_resource);
}

void OneInputFile::capture_auxiliary_file_name(const Cstring& s) {
	const char* t = *s;
	while(*t && *t != '=') {
		t++;
	}
	t++;

	// set approriate global - for now:
	// Cstring str;

	// debug
	// str << "aux file name: " << t << "\n";
	// tolexcl << str;
}

void tol_value::addExp(const tolExp& pe) {
    tol_value* tv = dynamic_cast<tol_value*>(pe.object());
    if(tv) {
	expressions.push_back(pe);
    }
}

void KeywordValuePair::addExp(const tolExp& pe) {
    KeywordValuePair* kv = dynamic_cast<KeywordValuePair*>(pe.object());
    if(kv) {
	expressions.push_back(pe);
    }
}

#ifdef PREVIOUS_VERSION

//
// This is essentially TransferActToJson() from ACT_exec_json.C,
// modified so it uses flexval instead of tlists.
//
// Important change compared to early versions: for parameters that
// are arrays and either are empty or contain empty arrays among
// its elements, the json output must specify whether the empty
// items are lists or structs. This requires auxiliary data,
// which the tol2json program extracted from tol.aux files. That
// auxiliary data is needed here.
//
void transfer_act_to_json(
	CTime_base	time_tag,
	CTime_base	duration,
	bool		act_is_visible,
	const Cstring&	namestring,
	const Cstring&	ID,
	const Cstring&	type,
	const Cstring&	description,
	const Cstring&	legend,
	const Cstring&	parentID,
	const flexval&	the_params,
	const flexval&	attributes) {
    json_object*	new_act = json_object_new_object();
    json_object*	act_metadata = json_object_new_array();
    json_object*	keyword_value_pair;
    json_object*	kwd_value_pair = NULL;

    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"Start",
		json_object_new_string(*time_tag.to_string()));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);

    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"Duration",
		json_object_new_string(*duration.to_string()));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);


    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"Visible",
		json_object_new_boolean(act_is_visible));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);

    json_object_object_add(
		new_act,
		"Activity Name",
		json_object_new_string(*namestring));
    kwd_value_pair = json_object_new_object();
    json_object_object_add(
			kwd_value_pair,
			"Parent",
			json_object_new_string(*parentID));
    json_object_array_add(
			act_metadata,
			kwd_value_pair);

    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"ID",
		json_object_new_string(*ID));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);

    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"Description",
		json_object_new_string(*description));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);

    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"Legend",
		json_object_new_string(*legend));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);

    kwd_value_pair = json_object_new_object();
    json_object_object_add(
		kwd_value_pair,
		"Type",
		json_object_new_string(*type));
    json_object_array_add(
		act_metadata,
		kwd_value_pair);

    const map<string, flexval>&			atts = attributes.get_struct();
    map<string, flexval>::const_iterator	iter;
    string					subsystem;
    for(iter = atts.begin(); iter != atts.end(); iter++) {
	keyword_value_pair = json_object_new_object();
	string key = iter->first;

	//
	// Instead of writing the subsystem attribute,
	// we send its value to the caller, who can
	// then organize the activities into files that
	// are subsystem-specific. There will be one JSON
	// file per subsystem. This helps make sure that
	// the JSON files don't get huge.
	//
	if(key == "Subsystem") {
	    subsystem = (string)iter->second;
	    continue;
	}

	json_object_object_add(
		keyword_value_pair,
		key.c_str(),
		iter->second.to_json()
		);
	json_object_array_add(
		act_metadata,
		keyword_value_pair);
    }
    if(!subsystem.size()) {
	subsystem = "GenericSubsystem";
    }

    json_object* act_params = json_object_new_array();

    //
    // There may not be any parameters
    //
    if(the_params.getType() == flexval::TypeStruct) {
	const map<string, flexval>&	params = the_params.get_struct();

	//
	// We are going to traverse the set of parameters as extracted
	// from the TOL data for this particular activity. However, in
	// case a parameter is or contains emtpy arrays, we need to
	// figure out whether the array is intended to be a list or
	// a struct. For that purpose, we consult the list of activity
	// information nodes that was extracted from the .tol.aux files.
	//
	// Note that every activity type that contains at least one
	// parameter that is an array is documented in aux_type_info().
	//
	map<string, flexval>::const_iterator aux_iter
		= tol_reader::aux_type_info().find(*type);
	map<string, flexval>	aux_param_map;

	//
	// Arrange the aux param info as a map
	//
	if(aux_iter != tol_reader::aux_type_info().end()) {
	    const vector<flexval>& aux_param_vector = aux_iter->second.get_array();
	    for(int z = 0; z < aux_param_vector.size(); z++) {
		map<string, flexval>::const_iterator one_iter
			= aux_param_vector[z].get_struct().begin();
		string parname = one_iter->first;
		aux_param_map[parname] = one_iter->second;
	    }
	}
	
	for(iter = params.begin(); iter != params.end(); iter++) {
	    json_object*	keyword_value_pair = json_object_new_object();
	    string		key = iter->first;

	    bool		param_is_apgen_array
			= iter->second.getType() == flexval::TypeArray
			  || iter->second.getType() == flexval::TypeStruct;
	    if(param_is_apgen_array) {

		//
		// Extract the parameter shape from the auxiliary
		// data and provide it to flexval::to_json as a
		// guide in creating json output
		//
		flexval& aux_param = aux_param_map[key];
		json_object_object_add(
		    keyword_value_pair,
		    key.c_str(),
		    iter->second.to_json(aux_param));
	    } else {
		json_object_object_add(
		    keyword_value_pair,
		    key.c_str(),
		    iter->second.to_json());
	    }

	    json_object_array_add(
		act_params,
		keyword_value_pair);
	}
    }

    json_object_object_add(new_act, "Activity Parameters", act_params);
    json_object_object_add(new_act, "Metadata", act_metadata);

    //
    // Now we add this to the subsystem for this activity
    //
    subsysNode* sN;
    if(!(sN = tol_reader::subsystem_based_list().find(subsystem.c_str()))) {
	json_object*	subsysn = json_object_new_array();
	sN = new subsysNode(subsystem.c_str(), subsysn);
	tol_reader::subsystem_based_list() << sN;
    }
    json_object_array_add(sN->payload, new_act);
}
#endif /* PREVIOUS_VERSION */

//
// This is essentially TransferActToJson() from ACT_exec_json.C,
// modified so it uses flexval instead of tlists.
//
// Important change compared to early versions: for parameters that
// are arrays and either are empty or contain empty arrays among
// its elements, the json output must specify whether the empty
// items are lists or structs. This requires auxiliary data,
// which the tol2json program extracted from tol.aux files. That
// auxiliary data is needed here.
//
// Additional change: the json schema now reflects Gavin Martin's
// suggestion of a flatter, simpler structure:
//
// {
//    "Activity Name":"VENUS_d2",
//    "Activity Parameters": {
//        "Reason":"Flyby"
//        "Duration":"014T00:00:00"
//    },
//    "Metadata": {
//        "Parent":"PeriapsisAct",
//        "ID":"SetCruiseSchedulingKeepout",
//        "Type":"SetCruiseSchedulingKeepout",
//        "Visible":true,
//        "Color":"Dodger Blue",
//        "span":"014T00:00:00",
//        "legend":"Clipper",
//        "pattern":3,
//        "start":"2023-315T14:58:58.045",
//        "status":1,
//        "subsystem":"Clipper",
//    }
// }
//
void transfer_act_to_json(
	CTime_base	time_tag,
	CTime_base	duration,
	bool		act_is_visible,
	const Cstring&	namestring,
	const Cstring&	ID,
	const Cstring&	type,
	const Cstring&	description,
	const Cstring&	legend,
	const Cstring&	parentID,
	const flexval&	the_params,
	const flexval&	attributes) {
    json_object*	new_act = json_object_new_object();
    json_object*	keyword_value_pair;

    //
    // 1. Name
    //
    json_object_object_add(
		new_act,
		"Activity Name",
		json_object_new_string(*namestring));

    //
    // 2. Parameters
    //
    json_object* act_params = json_object_new_object();

    map<string, flexval>::const_iterator	iter;

    //
    // There may not be any parameters
    //
    if(the_params.getType() == flexval::TypeStruct) {
	const map<string, flexval>&	params = the_params.get_struct();

	//
	// We are going to traverse the set of parameters as extracted
	// from the TOL data for this particular activity. However, in
	// case a parameter is or contains emtpy arrays, we need to
	// figure out whether the array is intended to be a list or
	// a struct. For that purpose, we consult the list of activity
	// information nodes that was extracted from the .tol.aux files.
	//
	// Note that every activity type that contains at least one
	// parameter that is an array is documented in aux_type_info().
	//
	map<string, flexval>::const_iterator aux_iter
		= tol_reader::aux_type_info().find(*type);
	map<string, flexval>	aux_param_map;

	//
	// Arrange the aux param info as a map
	//
	if(aux_iter != tol_reader::aux_type_info().end()) {
	    const vector<flexval>& aux_param_vector = aux_iter->second.get_array();
	    for(int z = 0; z < aux_param_vector.size(); z++) {
		map<string, flexval>::const_iterator one_iter
			= aux_param_vector[z].get_struct().begin();
		string parname = one_iter->first;
		aux_param_map[parname] = one_iter->second;
	    }
	}
	
	for(iter = params.begin(); iter != params.end(); iter++) {
	    string		key = iter->first;

	    bool		param_is_apgen_array
			= iter->second.getType() == flexval::TypeArray
			  || iter->second.getType() == flexval::TypeStruct;
	    if(param_is_apgen_array) {

		//
		// Extract the parameter shape from the auxiliary
		// data and provide it to flexval::to_json as a
		// guide in creating json output
		//
		flexval& aux_param = aux_param_map[key];
		json_object_object_add(
		    act_params,
		    key.c_str(),
		    iter->second.to_json(aux_param));
	    } else {
		json_object_object_add(
		    act_params,
		    key.c_str(),
		    iter->second.to_json());
	    }
	}
    }

    json_object_object_add(new_act, "Activity Parameters", act_params);

    //
    // 3. Metadata
    //
    json_object*	act_metadata = json_object_new_object();

    json_object_object_add(
		act_metadata,
		"Parent",
		json_object_new_string(*parentID));

    json_object_object_add(
		act_metadata,
		"ID",
		json_object_new_string(*ID));

    json_object_object_add(
		act_metadata,
		"Type",
		json_object_new_string(*type));

    json_object_object_add(
		act_metadata,
		"Visible",
		json_object_new_boolean(act_is_visible));

    const map<string, flexval>&			atts = attributes.get_struct();
    string					subsystem;
    for(iter = atts.begin(); iter != atts.end(); iter++) {
	string key = iter->first;

	//
	// Instead of writing the subsystem attribute,
	// we send its value to the caller, who can
	// then organize the activities into files that
	// are subsystem-specific. There will be one JSON
	// file per subsystem. This helps make sure that
	// the JSON files don't get huge.
	//
	if(key == "Subsystem") {
	    subsystem = (string)iter->second;
	    continue;
	}

	json_object_object_add(
		act_metadata,
		key.c_str(),
		iter->second.to_json()
		);
    }
    if(!subsystem.size()) {
	subsystem = "GenericSubsystem";
    }
    json_object_object_add(new_act, "Metadata", act_metadata);

    //
    // Now we add this to the subsystem for this activity
    //
    subsysNode* sN;
    if(!(sN = tol_reader::subsystem_based_list().find(subsystem.c_str()))) {
	json_object*	subsysn = json_object_new_array();
	sN = new subsysNode(subsystem.c_str(), subsysn);
	tol_reader::subsystem_based_list() << sN;
    }
    json_object_array_add(sN->payload, new_act);
}
