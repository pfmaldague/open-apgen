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

coutx tolexcl;

using namespace tol;
using namespace std;

//
// Defined in the apcore library:
//
extern thread_local int thread_index;

//
// Link between thread_index and parser_index:
// we only have 2 parsers, but we have 3 or 6 threads:
//
//  Description					thread_index	"work function"
//  -----------					------------	---------------
//  the main thread, which runs main()		    0		    main()
//
//  the comparator thread, C			    1		tol::compare()
//
//  the first parsing thread, P[0]		    3		 tol::parse()
//
//  the first extraction thread, X[0]		    2		tol::process()
//
//  the 2nd parsing thread, P[1] (if 2 files)	    5		 tol::parse()
//
//  the 2nd extraction thread, X[1] (if 2 files)    4		tol::process()
//
static int num_files = 0;
int tol::which_parser(int indx) {
    if(num_files == 1) {
	switch(thread_index) {
	    case 1:
		return -1;
	    case 2:
		return 0;
	    case 3:
		return 0;
	    default:
		return -1;
	}
    } else {
	switch(thread_index) {
	    case 1:
		return -1;
	    case 2:
		return 0;
	    case 3:
		return 0;
	    case 4:
		return 1;
	    case 5:
		return 1;
	    default:
		return -1;
	}
    }
}

int usage(char* s) {

	cerr << "Usage: " << s
	    << " [-debug-grammar] [-abs-tol eps] [-rel-tol eps] "
	    << "[scope] [level of detail] "
	    << "<filename> [<filename 2>]\n";
	cerr << "Scope options:\n\t-header\n\t-first-error\n"
	    << "\t-rec <recno>\n\t-scet <scet time>\n"
	    << "\t-cpu <cpu time>\n\t" << "-all\n";
	cerr << "Granularity options:\n\t-pass-fail\n\t-error-count\n"
	    << "\t-detailed\n\t-full\n";
	return -1;
}

int main(int argc, char* argv[]) {

    //
    // Set default options for initial parsing run
    //
    tol_reader::Granularity	granularity = tol_reader::Granularity::Detailed;
    tol_reader::Scope		scope = tol_reader::Scope::All;
    flexval			args;
    vector<flexval>		filenames;

    args["parsing_scope"] = "";

    //
    // get name of file to read from user
    //
    if(argc == 1 || argc > 10) {
	return usage(argv[0]);
    }
    for(int i = 1; i < argc; i++) {
	if(!strcmp(argv[i], "-debug-grammar")) {
	    yydebug = 1;
	} else if(!strcmp(argv[i], "-abs-tol")) {
	    if(i == argc - 1) {
		return usage(argv[0]);
	    }
	    i++;
	    tol::abs_tolerance() = strtod(argv[i], NULL);
	} else if(!strcmp(argv[i], "-rel-tol")) {
	    if(i == argc - 1) {
		return usage(argv[0]);
	    }
	    i++;
	    tol::rel_tolerance() = strtod(argv[i], NULL);
	}

	//
	// Scope Options
	//
	else if(!strcmp(argv[i], "-header")) {
	    scope = tol_reader::Scope::HeaderOnly;
	} else if(!strcmp(argv[i], "-first-error")) {
	    scope = tol_reader::Scope::FirstError;
	} else if(!strcmp(argv[i], "-rec")) {
	    if(i == argc - 1) {
		return usage(argv[0]);
	    }
	    i++;
	    scope = tol_reader::Scope::RecordCount;
	    args["parsing_scope"] = atol(argv[i]);
	} else if(!strcmp(argv[i], "-scet")) {
	    if(i == argc - 1) {
		return usage(argv[0]);
	    }
	    i++;
	    scope = tol_reader::Scope::SCETTime;
	    cppTime S(argv[i]);
	    args["parsing_scope"] = S;
	} else if(!strcmp(argv[i], "-cpu")) {
	    if(i == argc - 1) {
		return usage(argv[0]);
	    }
	    i++;
	    scope = tol_reader::Scope::CPUTime;
	    args["parsing_scope"] = strtod(argv[i], NULL);
	} else if(!strcmp(argv[i], "-all")) {
	    scope = tol_reader::Scope::All;
	}

	//
	// Granularity Options
	//
	else if(!strcmp(argv[i], "-pass-fail")) {
	    granularity = tol_reader::Granularity::Pass_Fail;
	} else if(!strcmp(argv[i], "-error-count")) {
	    granularity = tol_reader::Granularity::ErrorCount;
	} else if(!strcmp(argv[i], "-detailed")) {
	    granularity = tol_reader::Granularity::Detailed;
	} else if(!strcmp(argv[i], "-full")) {
	    granularity = tol_reader::Granularity::Full;
	}

	//
	// File name(s)
	//
	else {
	    if(filenames.size() == 2) {
		return usage(argv[0]);
	    }
	    filenames.push_back(flexval(string(argv[i])));
	}
    }
    
    num_files = filenames.size();
    if(!num_files) {
	return usage(argv[0]);
    }

    //
    // Verify that the files exist and can be read
    //
    for(int z = 0; z < filenames.size(); z++) {
	struct stat	file_stats;
	int		stat_report
		= stat(((string)filenames[0]).c_str(), &file_stats);
	if(stat_report) {
	    cerr << "Cannot open file " << filenames[0]
			<< "\n";
	    return -1;
	}

	//
	// Unused but interesting...
	//
	long	content_length = file_stats.st_size;
    }
    args["files"].get_array() = filenames;

    //
    // This call initializes read-only globals used by parsers:
    //
    tol_initialize_tokens();

    //
    // master thread - this:
    //
    thread_index = 0;

    //
    // Preparation for multithreading. We need to allocate
    // thread indices to the processes we need:
    //
    //  Thread			Descr.			  Parsing
    //  Index						   Index
    //  -------			------			  -------
    //    0 - main thead (this one)			    N/A
    // 	  1 - one thread for parsing TOL 1		     0
    // 	  2 - one thread for parsing TOL 2		     1
    // 	  3 - one thread for extracting records from TOL 1   0
    // 	  4 - one thread for extracting records from TOL 2   1
    // 	  5 - one thread for comparing extracted records    N/A
    //
    // The loop over parsingIndex below takes care of the parsing
    // and extraction threads.
    //

    //
    // The various threads will need to be launched repeatedly
    // to accomplish the overall task of comparing the two
    // TOLs.
    //
    // The command interpreter is in charge of laying out the
    // subtasks that are necessary, and launching threads as
    // required by each subtask.
    //
    // The main program communicates with the interpreter via
    // commands. Each command has a code name and a list of
    // arguments (which can of course be empty).
    //

    tol_reader::results Results;

    tol_reader::cmd	a_command(
			    tol_reader::Directive::Open,
			    granularity,
			    scope);

    a_command.Args = args;

    if(tol_reader::interpreter(
			a_command,
			Results)) {

	//
	// Need to figure out a consistent philosophy for error handling.
	// For now, interpreter returns -1 after dumping error messages
	// to cerr.
	//
	return -1;
    }

    ostringstream s;
    Results.report(a_command, s);
    cerr << s.str();

    //
    // Clean up report data. This data does not contain arrays,
    // so it does not have to be deleted by its owner.
    //
    // Note: this is redundant - the runtime system would
    // clean up. Left over from chasing memory leaks early
    // on.
    //
    for(int parsing_thread = 0; parsing_thread < 10; parsing_thread++) {
	resource::resource_data(parsing_thread).clear();
    }


    return 0;
}
