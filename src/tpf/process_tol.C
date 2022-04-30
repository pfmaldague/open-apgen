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

using namespace tol;
using namespace std;

//
// Defined in the apcore library:
//
extern thread_local int thread_index;

//
// If CMD.Code == Directive::ToJson, this method will append
// a json object to the json container it belongs to - assuming
// it knows how to do that. As of now (mid-Oct. 2020) only
// activity instances are being collected into the json container(s).
//
// Json containers are organized by subsystem.
//
void process_a_tol_record(
		tolNode*		pe,
		const tol_reader::cmd&	CMD,
		CTime_base&		time_stamp,
		CTime_base&		previous_time,
		int			parser_index);
//
// Waits for the comparator thread to read the previous
// vector, then creates a new vector and stores it.
//
// When the comparator reads a new vector, it sets the
// pointer to it to NULL, so all we have to do is load
// it in a loop and wait until its value is NULL.
//
safe_data*
create_safe_vector(const CTime_base& time_tag, int parser_index) {

    //
    // loop over resources, gather the last nodes
    // in history index maps and store pointers to
    // them in a vector with the same ordering
    // as tol::resource::resource_data()
    //
    map<Cstring, int>::const_iterator iter;
    vector<history<flexval>::flexnode*> new_vector(
		resource::resource_indices(parser_index).size());
    int i = 0;
    for( iter = resource::resource_indices(parser_index).begin();
	 iter != resource::resource_indices(parser_index).end();
	 iter++) {
	resource* res1 = resource::resource_data(parser_index)[iter->second].get();

	new_vector[i++] = res1->History.values.last_node();
    }

#   ifdef DEBUG_THIS
    Cstring exclstr;
    exclstr << "create_safe_vector(" << time_tag.to_string() << ")\n";
    tolexcl << exclstr;
    exclstr.undefine();
#   endif /* DEBUG_THIS */
    return new safe_data(time_tag, new_vector);
}

void create_and_store_safe_vector(
		const tol_reader::cmd& CMD,
		const CTime_base& tag,
		int parser_index) {
    if(CMD.Code == tol_reader::Directive::Open
       || CMD.Code == tol_reader::Directive::ToJson
      ) {

	    //
	    // Make sure the comparator has read the
	    // previous vector
	    //
	    while(true) {
		if(safe_vector(parser_index).load() == NULL) {

#		    ifdef DEBUG_THIS
		    Cstring exclstr;
		    exclstr << "create_and_store safe_vector: comparator has set safe_vector to NULL\n";
		    tolexcl << exclstr;
		    exclstr.undefine();
#		    endif /* DEBUG_THIS */

		    break;
		}
		if(pE::terminate_parsing(parser_index).load()
		   || tol_reader::errors_found().load()) {

		    //
		    // We ran out of CPU time, or we got errors
		    //
		    return;
		}
		this_thread::sleep_for(chrono::milliseconds(10));
	    }
	    safe_vector(parser_index).store(create_safe_vector(
				tag, parser_index));
    }

    //
    // Else, nothing to do
    //
}


//
// tol::process() implements the interface between record
// parsing and record processing.
//
// The parsing side is implemented in method Record::addExp(),
// in tol_system.C. When a given number of records (1000)
// have been parsed, it sets up an atomic flag which is
// picked up by process() to trigger a record processing
// loop.
//
// Record processing is implemented in tol::process_a_tol_record(),
// which extract parsing trees and converts the information they
// contain into a "database" of object information. Objects are
// 	- resources
// 	- activities
//	- constraint violations.
//
void tol::process(
		const tol_reader::cmd& CMD,
		int index) {
    thread_index = index;
    int parser_index = which_parser(thread_index);

    tolNode*	last_parsed_record = NULL;
    tolNode*	first_record_to_process = NULL;
    long int	records_processed = 0;
    CTime_base	time_stamp;
    CTime_base	previous_time(0, 0, false);

#   ifdef DEBUG_THIS
    Cstring exclstr;
    exclstr << "tol_process(cmd = " << tol_reader::spell(CMD.Code) << ") START\n";
    tolexcl << exclstr;
    exclstr.undefine();
#   endif /* DEBUG_THIS */

    try {

	//
	// First, wait till the parser gets underway; LastParsedRecord
	// was initialized to NULL by main():
	//
	while((last_parsed_record = pE::LastParsedRecord(parser_index).load()) == NULL) {
	    this_thread::sleep_for(chrono::milliseconds(10));
	    if(pE::file_parsing_complete(parser_index).load()) {

		//
		// There may be errors at the beginning of the
		// file, in which case parsing will never start
		// in earnest. In that case we should exit right
		// away. It's also possible that parsing will 
		// take very little time, so a "parsing complete"
		// message does not necessarily indicate an error.
		//
		if(tol_reader::errors_found().load()) {

#		    ifdef DEBUG_THIS
		    exclstr << "tol_process: setting file_extr_complete "
			<< "to true because errors were found\n";
		    tolexcl << exclstr;
		    exclstr.undefine();
#		    endif /* DEBUG_THIS */

		    tol_reader::file_extraction_complete(parser_index).store(true);
		    return;
		}
		break;
	    }
	}

	//
	// The parser has provided us with some records; therefore,
	// the first time tag has been detected by the token
	// analyzer - see the rule for <start_of_record> in file
	// tok_tokens.l. We grab the first time tag here and define
	// the corresponding global time, earliest_time_stamp:
	//
	tol::earliest_time_stamp(parser_index)
		= CTime_base(tol::pE::first_time_tag(parser_index));

#	ifdef DEBUG_THIS
	exclstr << "setting earliest_time_stamp to "
	    << tol::earliest_time_stamp(parser_index).to_string()
	    << "\n";
	tolexcl << exclstr;
	exclstr.undefine();
#	endif /* DEBUG_THIS */

	while(true) {
	    bool should_break = false;

	    //
	    // The two pointers below - first_record_to_process
	    // and last_parsed_record - define the bounds of the
	    // interval of records which we can safely access.
	    //
	    first_record_to_process	= pE::FirstRecordToProcess(parser_index).load();
	    last_parsed_record		= pE::LastParsedRecord(parser_index).load();

#	    ifdef DEBUG_THIS
	    exclstr << "processing from " << ((void*)first_record_to_process)
		<< " to " << ((void*)last_parsed_record) << "\n";
	    tolexcl << exclstr;
	    exclstr.undefine();
#	    endif /* DEBUG_THIS */

	    for(  tolNode* pe = first_record_to_process;
		  pe != last_parsed_record;
		  pe = pe->next_node()
	    ) {
		process_a_tol_record(
			pe,
			CMD,
			time_stamp,
			previous_time,
			parser_index);
		records_processed++;
	    }

	    if(time_stamp > tol::earliest_time_stamp(parser_index)) {
		if(pE::terminate_parsing(parser_index).load()) {

		    //
		    // We ran out of CPU time allocation
		    //

		    //
		    // Tell the main thread that we are done
		    //
#		    ifdef DEBUG_THIS
		    exclstr << "tol_process: setting file_extr_complete "
			<< "to true because terminate_parsing is set\n";
		    tolexcl << exclstr;
		    exclstr.undefine();
#		    endif /* DEBUG_THIS */

		    tol_reader::file_extraction_complete(parser_index).store(true);

		}
	    }

	    //
	    // We use this atomic pointer to tell the parsing
	    // thread that we are ready to start extracting
	    // the records starting where the previous batch
	    // ended:
	    //
	    pE::FirstRecordToProcess(parser_index).store(last_parsed_record);
  

	    //
	    // We use this atomic index to tell the reporting
	    // thread that it can safely access data records
	    // whose time stamp is less than this:
	    //
	    create_and_store_safe_vector(CMD, time_stamp, parser_index);

	    //
	    // Wait for the parser to advance
	    //
	    while(pE::LastParsedRecord(parser_index).load() == last_parsed_record) {

		this_thread::sleep_for(chrono::milliseconds(10));
		if(pE::file_parsing_complete(parser_index)) {
		    should_break = true;
		    break;
		}
		if(tol_reader::errors_found().load()) {
		    should_break = true;
		    break;
		}
	    }
	    if(should_break) {

		//
		// break out of the outer loop
		//
		break;
	    }
	}

	if(tol_reader::errors_found().load()) {

#	    ifdef DEBUG_THIS
	    exclstr << "tol_process: setting file_extr_complete "
		<< "to true because errors were found\n";
	    tolexcl << exclstr;
	    exclstr.undefine();
#	    endif /* DEBUG_THIS */

	    tol_reader::file_extraction_complete(parser_index).store(true);
	    return;
	}

	//
	// It remains to process the last few records
	//
	first_record_to_process = pE::FirstRecordToProcess(parser_index).load();
	last_parsed_record = pE::LastParsedRecord(parser_index).load();
	for(tolNode* pe = first_record_to_process;
	    pe;
	    pe = pe->next_node()
	) {
	    process_a_tol_record(
			pe,
			CMD,
			time_stamp,
			previous_time,
			parser_index);
	    records_processed++;
	    if(pe == last_parsed_record) {
		break;
	    }
	}
    } catch(eval_error Err) {

#	ifdef DEBUG_THIS
	exclstr << "tol_process: setting file_extr_complete "
	"to true because errors were found\n";
	tolexcl << exclstr;
	exclstr.undefine();
#	endif /* DEBUG_THIS */

	tol_reader::file_extraction_complete(parser_index).store(true);

	//
	// We report errors to the calling thread:
	//
	tol_reader::threadErrors(thread_index) << "extraction error:\n" << Err.msg << "\n";
	tol_reader::errors_found().store(true);
	return;
    }

    //
    // Finally, signal other threads that we are done.
    // Note: the "database" which we created survives
    // this process, so we don't need for the comparator
    // (which uses that database as its primary input)
    // to complete before exiting. However, we do need
    // to tell it that new data is available.
    //
    create_and_store_safe_vector(CMD, time_stamp, parser_index);

#   ifdef DEBUG_THIS
    exclstr << "tol_process: setting file_extr_complete to true "
        << "because we are done and a safe vector was just stored.\n";
    tolexcl << exclstr;
    exclstr.undefine();
#   endif /* DEBUG_THIS */

    tol_reader::file_extraction_complete(parser_index).store(true);
}

void tol::interrupt(const tol_reader::cmd& CMD) {
    double seconds
	= (double)CMD.Args.get_struct().find("parsing_scope")->second;
    int millisec = (int)(seconds * 1000.0);
    this_thread::sleep_for(chrono::milliseconds(millisec));
    pE::terminate_parsing(0).store(true);
    pE::terminate_parsing(1).store(true);
}
