#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <string>

//
// parsing stuff
//
#include "tolcomp_grammar.H"
#include "tolcomp_tokens.H"
#include "tolReader.H"
#include "tol_lex_intfc.H"
#include "tol_expressions.H"

//
// extraction stuff
//
#include "tol_data.H"

using namespace tol;
using namespace std;

//
// The interpreter, tol_reader::interpreter, is launched
// as a call from main(); it is not a separate thread.
// Its main job is to
//
// 	1. launch a comparison thread with thread function
//
// 		tol::compare
//
// 	2. launch a processing thread with thread function
//
// 		tol::process
//
// 	3. launch a parsing thread with thread function
//
// 		tol::parse
//
// 	4. if necessary, launch a second processing thread
//
// 	5. if necessary, launch a second parsing thread
//
// That's it! The highest-level thread is tol::compare,
// which is given a reference to theCommand and therefore
// knows what to do.
//
// tol::compare() is implemented in tol_comparator.C;
// tol::process() is implemented in process_tol.C;
// tol::parse()   is implemented in tol_interpreter.C (below).
//
// tol::process() delegates record-oriented work to
// the source file process_record.C, which implements
// method process_a_tol_record().
//

//
// Defined in the apcore library:
//
extern thread_local int thread_index;

//
// yyscan_t is defined in tolcomp_tokens.H
//
yyscan_t& get_scanner(int index) {

	//
	// Assume 10 threads max for now
	//
	static yyscan_t scanners[10];
	return scanners[index];
}

int tol_get_line_number(int index) {
	return yyget_lineno(get_scanner(index));
}

//
// Static function for the parsing threads.
//
void tol::parse(
		const tol_reader::cmd& theCommand,
		int index) {
    thread_index = index;
    int parser_index = which_parser(thread_index);

    //
    // Initialize the scanner. Make sure to use the one returned
    // by get_scanner(), since that's the one that will be used
    // by the tol parsed expression system.
    //
    yyscan_t& scanner = get_scanner(parser_index);
    yylex_init(&scanner);

    const vector<flexval>&	filenames
	= theCommand.Args.get_struct().find("files")->second.get_array();
    const string my_file = (string)filenames[parser_index];
    tol_reader::tol_file[parser_index] = fopen(my_file.c_str(), "r");
    if(!tol_reader::tol_file[parser_index]) {
	pE::file_parsing_complete(parser_index).store(true);
	tol_reader::threadErrors(thread_index)
	    << "Cannot find file " << my_file << "\n";
	tol_reader::errors_found().store(true);
	return;
    }

    yyset_in(tol_reader::tol_file[parser_index], scanner);

    try {
	yyparse(get_scanner(parser_index));
    } catch(eval_error Err) {
	cerr << Err.msg << "\n";
    }

    //
    // Tell the extraction threads that we are done
    //
    pE::file_parsing_complete(parser_index).store(true);

    //
    // Wait till the extraction threads are done, so
    // we can delete the smart pointers we own (only
    // this thread can do this)
    //
    while(!tol_reader::file_extraction_complete(parser_index).load()) {
	this_thread::sleep_for(chrono::milliseconds(10));
    }

    //
    // clean up the scanner
    //
    fclose(yyget_in(scanner));
    yylex_destroy(scanner);

    //
    // Avoid memory leaks. We must do this ourselves, because
    // smart pointers only delete their object when deleted by
    // their owners. Note that these structures only contain
    // parsing objects derived from tol::pE; processed data
    // are stored in structures defined in tol_data.H, mainly
    // tol::abstract_type, which do not rely on smart pointers
    // and may therefore be deleted by the main process without
    // memory leaks.
    //
    vector<tolExp>& vec = tol_reader::input_files();
    vec.clear();
    slist<alpha_int, tolNode>& s = pE::theRecords(parser_index);
    s.clear();
    slist<alpha_int, tolNode>& at = pE::theActTypes(parser_index);
    at.clear();
}

tol_reader::results&	the_results() {
	static tol_reader::results R;
	return R;
}

int tol_reader::interpreter(
	const tol_reader::cmd&	theCommand,
	tol_reader::results&	theResults) {

    //
    // See the README.md file for comments on the design.
    //

#   ifdef DEBUG_THIS
    Cstring exclstr;
    exclstr << "interpreter(cmd = " << tol_reader::spell(theCommand.Code)
	<< ") START\n";
    tolexcl << exclstr;
    exclstr.undefine();
#   endif /* DEBUG_THIS */

    //
    // Process the command code and options. Actually, nothing to do
    // here - all the action is in the parsing and processing threads.
    //
    if(theCommand.Code == tol_reader::Directive::Open) {
	;
    } else if(theCommand.Code == tol_reader::Directive::ToJson) {
	;
    } else {
	Cstring errs;
	errs << "interpreter: command " << tol_reader::spell(theCommand.Code)
	    << " has not been implemented yet.";
	cerr << errs << "\n";
	return -1;
    }

    //
    // At this point we expect to have one or two files
    // to parse and analyze. Code that deals with inspection
    // and processing of data already accumulated should
    // not exercise the stuff below.
    //

    //
    // We will set this to the thread_index value we want a
    // spawned thread to use just before we launch it:
    //
    int thread_index_to_use = 0;
    vector<string> thread_names;

    //
    // Parsing threads
    //
    std::thread		P[2];

    //
    // Extraction threads
    //
    std::thread		X[2];

    //
    // Comparator thread
    //
    std::thread		C;

    //
    // Timer thread (if scope == CPUTime)
    //
    std::thread		T;

    //
    // Figures out file(s) to parse
    //
    const vector<flexval>& filenames
	= theCommand.Args.get_struct().find("files")->second.get_array();

    //
    //
    // Be careful here: need to reset _all_ the atomic
    // items used in inter-thread communications.
    //
    for(    int parsingIndex = 0;
	    parsingIndex < filenames.size();
	    parsingIndex++) {

	//
	// The default time is Jan. 1 1970 midnight
	//
	tol::safe_vector(parsingIndex).store(NULL);
	pE::LastParsedRecord(parsingIndex).store(NULL);
	pE::FirstRecordToProcess(parsingIndex).store(NULL);
	pE::file_parsing_complete(parsingIndex).store(false);
	tol_reader::file_extraction_complete(parsingIndex).store(false);
	pE::terminate_parsing(parsingIndex).store(false);
	tol_reader::ActMetadataCaptured(parsingIndex).store(false);
	tol_reader::ResMetadataCaptured(parsingIndex).store(false);

	//
	// This one is not atomic because it is set before
	// launching any threads and the only user after
	// that is the token analyzer:
	//
	pE::first_record(parsingIndex) = true;
    }

    //
    // Here we do not assume we have two files; even so, we
    // launch the comparator thread first. It will start executing
    // when both processing threads have obtained some measurements.
    //
    thread_names.push_back("main");
    thread_index_to_use++;
    thread_names.push_back("compare");
    C = std::thread(
		tol::compare,
		theCommand,
		&theResults,
		thread_index_to_use);

    if(theCommand.parsing_scope == tol_reader::Scope::CPUTime) {
	T = std::thread(
			tol::interrupt,
			theCommand);
    }


    //
    // We have one or two pairs of parsing/processing threads, depending on
    // how many files we were given. We loop over files and launch each
    // pair via their assigned "work functions": launch_parser for parsing
    // threads, process_tol for processing threads.
    //
    for(    int parsingIndex = 0;
	    parsingIndex < filenames.size();
	    parsingIndex++) {

	thread_index_to_use++;
	stringstream tempstream;
	tempstream << "process " << parsingIndex;
	thread_names.push_back(tempstream.str());
	X[parsingIndex] = std::thread(
			tol::process,
			theCommand,
			thread_index_to_use);

	tempstream.str("");
	tempstream << "parse " << parsingIndex;
	thread_names.push_back(tempstream.str());
	thread_index_to_use++;
	P[parsingIndex] = std::thread(
			tol::parse,
			theCommand,
			thread_index_to_use);
    }

    //
    // Gather the threads (not doing this causes a segfault).
    // Also, collect any errors they generated.
    //
    for(    int parsingIndex = 0;
	    parsingIndex < filenames.size();
	    parsingIndex++) {

	//
	// Before anything else, let's gather any errors while
	// the threads are still alive. We need a protocol for
	// doing that, before the threads exit.
	//
	if(X[parsingIndex].joinable()) {
	    X[parsingIndex].join();
	}
	if(P[parsingIndex].joinable()) {
	    P[parsingIndex].join();
	}
    }
    if(C.joinable()) {

	//
	// Need to process errors just as above.
	//
	C.join();
    }
    if(T.joinable()) {

	//
	// Need to process errors just as above.
	//
	T.join();
    }

    //
    // Gather errors, if any:
    //

    aoString all_errors;
    bool errors_found = false;
    for(int threadIndex = 0; threadIndex < 10; threadIndex++) {
	if(tol_reader::threadErrors(threadIndex).str().size()) {
	    errors_found = true;

	    //
	    // Not perfect; later on we could have a better
	    // description of each thread, including the
	    // name of the file it's parsing:
	    //
	    all_errors << "errors from thread \"" << thread_names[threadIndex]
		<< "\": " << tol_reader::threadErrors(threadIndex).str().c_str()
		<< "\n";
	    if(thread_names[threadIndex] == "compare") {
		ostringstream s;
		theResults.report(theCommand, s);
		cerr << s.str();   
	    }
	}
    }

    //
    // report errors
    //
    if(errors_found) {
	cerr << all_errors.str() << "\n";
	return -1;
    }

    return 0;
}
