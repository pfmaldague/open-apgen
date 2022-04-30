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

extern "C" {
#include <concat_util.h>
}

//
// extraction stuff
//
#include "tol_data.H"

//
// output stuff
//
#include "json.h"

coutx tolexcl;

using namespace tol;
using namespace std;

//
// Implemented in the apcore library:
//
extern thread_local int thread_index;

//
// Implemented in comparison/tol_system.C:
//
// typedef Cnode0<alpha_string, json_object*> subsysNode;
// extern tlist<alpha_string, subsysNode>& subsystem_based_list();

//
// implemented below
//
extern bool	handle_one_aux_file(const char* f, string& msg);

//
// Link between thread_index and parser_index:
// we only have 1 parsers, but we have 3 threads:
//
//  Description					thread_index	"work function"
//  -----------					------------	---------------
//  the main thread, which runs main()		    0		    main()
//
//  the comparator thread, C			    1		tol::compare()
//
//  the extraction thread, X[0]		    	    2		tol::process()
//
//  the parsing thread, P[0]		    	3		 tol::parse()
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

bool check_one_file(const char* f, string& msg) {
    struct stat	file_stats;
    int		stat_report = stat(f, &file_stats);

    if(stat_report) {
	stringstream s;
	s << "Cannot open file " << f << "\n";
	msg = s.str();

	//
	// failure
	//
	return false;
    }

    //
    // success
    //
    return true;
}

int usage(char* s) {

	cerr << "Usage: " << s << " [-debug-grammar] <TOL file> <output dir> [<aux file 1> <aux file 2> ...]\n";
	return -1;
}

int main(int argc, char* argv[]) {

    //
    // Set default options for initial parsing run.
    // Many of these are left over from tolcomp,
    // but they don't hurt.
    //
    tol_reader::Granularity	granularity = tol_reader::Granularity::Detailed;
    tol_reader::Scope		scope = tol_reader::Scope::All;
    flexval			args;
    vector<flexval>		filenames;
    vector<flexval>		auxfiles;

    args["parsing_scope"] = "";

    //
    // get name of file to read from user
    //
    if(argc < 3) {
	return usage(argv[0]);
    }

    //
    // Using a vector is overkill but provides common
    // API with tolcomp
    //
    filenames.push_back(flexval(string(argv[1])));
    
    //
    // Verify that the file exists and can be read
    //
    for(int z = 0; z < filenames.size(); z++) {
	string msg;
	if(!check_one_file(((string)filenames[0]).c_str(), msg)) {
	    cerr << msg;
	    return -1;
	}
    }

    args["files"].get_array() = filenames;


    //
    // Name of the output directory
    //
    Cstring	OutputDir = argv[2];

    //
    // Aux files, if any
    //
    for(int z = 3; z < argc; z++) {
	string msg;
	if(!check_one_file(argv[z], msg)) {
	    cerr << msg;
	    return -1;
	}
	if(!handle_one_aux_file(argv[z], msg)) {
	    cerr << msg;
	    return -1;
	}
    }

    //
    // This call initializes read-only globals used by the parser:
    //
    tol_initialize_tokens();

    //
    // master thread - this:
    //
    thread_index = 0;

    tol_reader::results Results;

    tol_reader::cmd	a_command(
			    tol_reader::Directive::ToJson,
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

    //
    // Output the json objects, one per subsystem.
    // Emulate EXPORT_DATArequest::process_middle() in
    // action_request.C.
    //
    struct stat	dir_stat;
    int		ret = stat(*OutputDir, &dir_stat);
    bool	is_directory = ((dir_stat.st_mode & S_IFDIR) != 0);
    bool	is_file = ((dir_stat.st_mode & S_IFREG) != 0);

    if(!ret) {

	//
	// The file exists
	//
	if(is_file) {
	    cerr << OutputDir << " is a regular file, not a directory\n";
	    return -1;
	} else if(!is_directory) {
	    cerr << OutputDir << " is not a directory\n";
	    return -1;
	}
    } else {

	//
	// The file does not exist
	//

	//
	// We want permissions drwxr-xr-x
	//
	if(mkdir(*OutputDir, 0755)) {
	    cerr << "Cannot create directory " << OutputDir << "\n";
	    return -1;
	}
	ret = stat(*OutputDir, &dir_stat);
	is_directory = ((dir_stat.st_mode & S_IFDIR) != 0);
	if(!is_directory) {
	    cerr << "Cannot create directory " << OutputDir << "\n";
	    return -1;
	}
    }
    slist<alpha_string, subsysNode>::iterator	iter(tol_reader::subsystem_based_list());
    subsysNode*					sym;
    while((sym = iter())) {
	Cstring subsystem = sym->Key.get_key();
	Cstring onefile;
	onefile << OutputDir << "/" << subsystem << ".json";
	FILE* f = fopen(*onefile, "w");
	const char* the_data_to_output = json_object_to_json_string_ext(
			sym->payload, JSON_C_TO_STRING_PRETTY);
	fwrite(the_data_to_output, strlen(the_data_to_output), 1, f);
	json_object_put(sym->payload);
    }


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

bool	handle_one_aux_file(const char* f, string& msg) {
    buf_struct			bs = {NULL, 0, 0};
    char			buf[2];
    enum json_tokener_error	jsonError;
    FILE*			input_file = fopen(f, "r");
    json_object*		obj = NULL;

    buf[1] = '\0';

    //
    // Read the file
    //
    while(fread(buf, 1, 1, input_file)) {
	concatenate(&bs, buf);
    }

    //
    // Invoke the json-c parser
    //
    obj = json_tokener_parse_verbose(bs.buf, &jsonError);
    if(!obj) {
	stringstream s;
	s << "json parsing error: " << json_tokener_error_desc(jsonError);
	msg = s.str();
	return false;
    }

    //
    // Now we have to decide what to do with the object...
    // For now, we make sure we can convert to a flexval
    //
    flexval newF;
    newF.from_json(obj);


    //
    // debug
    //
    // cerr << "Content of json file " << f << ":\n";
    // newF.write(cerr, true, 0);
    // cerr << "\n";

    //
    // We need to store the data in a map indexed by
    // activity type. Make sure the object is not
    // empty.
    //
    bool	map_exists = tol_reader::aux_type_info().size() > 0;
    if(newF.getType() == flexval::TypeStruct) {
	map<string, flexval>&			newM = newF.get_struct();
	map<string, flexval>::const_iterator	iter;

	for(	iter = newM.begin();
		iter != newM.end();
		iter++) {
	    if(map_exists) {

		//
		// All auxiliary files should contain the
		// same activity types as our map:
		//
		flexval& Aux = tol_reader::aux_type_info()[iter->first];

		//
		// Both iter and Aux describe a list of parameters.
		// Each parameter is a struct containing a single
		// key/value pair; the key is the parameter name
		// and the value has the same container type
		// (list or struct) as the corresponding APGenX
		// arrayType (LIST_STYLE, STRUCT_STYLE), or is
		// undefined if the APGenX array is empty.
		//
		assert(Aux.getType() == flexval::TypeArray);
		assert(iter->second.getType() == flexval::TypeArray);
		vector<flexval>&	Aux1 = Aux.get_array();
		const vector<flexval>&	newF1 = iter->second.get_array();
		assert(Aux.size() == newF1.size());

		for(	int k = 0; k < Aux1.size(); k++) {

		    //
		    // Both Aux1[k] and F1[k] are structs of one
		    // element, which lists the parameter name
		    // and the parameter 'value type'.
		    //
		    map<string, flexval>& Aux2 = Aux1[k].get_struct();

		    const map<string, flexval>& newF2 = newF1[k].get_struct();
		    map<string, flexval>::const_iterator new_iter = newF2.begin();

		    //
		    // debug
		    //
		    // cerr << "handling parameter " << new_iter->first.c_str()
		    //	<< " of activity " << iter->first.c_str() << "; old value:\n"
		    //	<< Aux2[new_iter->first].to_string() << "\n";

		    //
		    // if new_iter->second, the type data from the
		    // auxiliary file, is empty, then there is
		    // nothing to learn from it, and we can skip
		    // to the next parameter.
		    //
		    if(new_iter->second.getType() != flexval::TypeInvalid) {
			try {
			    Aux2[new_iter->first].merge(new_iter->second);
			} catch(eval_error Err) {
			    Cstring err;
			    err << "Error handling parameter " << new_iter->first.c_str()
				<< " of activity " << iter->first.c_str() << "; details:\n"
				<< Err.msg;
			    msg = *err;
			    return false;
			}
		    }

		    //
		    // debug
		    //
		    // cerr << "handling parameter " << new_iter->first.c_str()
		    // 	<< " of activity " << iter->first.c_str() << "; new value:\n"
		    //	<< Aux2[new_iter->first].to_string() << "\n";
		}
	    } else {
		tol_reader::aux_type_info()[iter->first] = iter->second;
	    }
	}

	//
	// debug
	//
	// cerr << "Content of master act file after merging " << f << ":\n";
	// for(	map<string, flexval>::const_iterator iter
	// 		= tol_reader::aux_type_info().begin();
	// 	iter != tol_reader::aux_type_info().end();
	// 	iter++) {
	//     cerr << iter->first << ":\n";
	//     iter->second.write(cerr, true, 2);
	//     cerr << "\n";
	// }
    }

    //
    // Clean up
    //
    destroy_buf_struct(&bs);
    json_object_put(obj);

    return true;
}
