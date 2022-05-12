#ifndef _TOL_READER_H_
#define _TOL_READER_H_

#include "tol_expressions.H"
#include "tol_history.H"
#include "flexval.H"
#include <set>

namespace tol {
class resource;
}

typedef Cntnr<alpha_string, json_object*>	subsysNode;

namespace tol_reader {

string make_space(int n);

extern std::atomic<bool>&	file_extraction_complete(int parsing_index);
extern std::atomic<bool>&	ActMetadataCaptured(int parsing_index);
extern std::atomic<bool>&	ResMetadataCaptured(int parsing_index);
extern std::atomic<bool>&	errors_found();

//
// We provide error streams to all threads,
// not just the parsing threads. So, this
// stream should be invoked with thread_index,
// not parsing_index:
//
extern stringstream&		threadErrors(int the_thread_index);
extern FILE*			tol_file[2];


//
// These vectors are where all parsed objects go. We need one per thread.
//
vector<tolExp>&			input_files();
vector<tolExp>&			input_files_for_a_parser(int);
string&				current_file();

//
// Activity-based array of information extracted from auxiliary
// files (files ending in .tol.aux). This information documents the
// 'shape' of activity parameters that are APGenX arrays, and can
// translate into json lists or structs.
//
map<string, flexval>&		aux_type_info();

//
// Collection of subsystem-based json objects for output
// in separate files:
//
tlist<alpha_string, subsysNode>&		subsystem_based_list();

//
// Basic commands issued by the main program to the
// parsing, processing and comparator threads that
// do the work
//
enum class Directive: char {
	Open,
	Peek,
	Read,
	Filter,
	Output,
	ToJson
};

const char* spell(Directive d);

//
// States that the interpreter can be in as a result
// of commands issued to it by the main program
//
enum class State: char {

    //  command		semantics
    //  -------		---------
	Start,	    //  Nothing has been done
	Initial,    //	Metadata has been read
	Partial,    //	Took a peek at file content
	Final	    //	The whole file has been read
};

const char* spell(State s);

//
// Granularity of the data to be stored while processing the TOLs
//
enum class Granularity: char {

//  1. Granularity of data storage options
//  ======================================
//
//	Granularity	semantics
//	  Option
//	---------	---------
//
	Pass_Fail,  //	Only store a Boolean pass/fail value
	ErrorCount, //	Show counts for each anomaly types
	Detailed,   //	Show detailed info for records that differ
	Full	    //	Show full resource histories
};

const char* spell(Granularity g);

//  2. Parsing Scope
//  ================
//
//   Parsing		semantics
//    Scope
//  ---------		---------
//
enum class Scope: char {
    HeaderOnly,	    //	Only parse the header (metadata or earliest time)
    FirstError,	    //	Only store a pointer to the first diff
    RecordCount,    //	Examine the indicated number of records
    CPUTime,	    //	Only spend the indicated amount of time parsing
    SCETTime,	    //	Parse until the indicated SCET time tag
    All		    //	Parse and analyze everything
};

const char* spell(Scope s);

//
// See README.md for documentation on this class
//
class cmd {
public:
    cmd() = default;
    cmd(    Directive dir,
	    Granularity gran,
	    Scope scope)
	: Code(dir),
	    level_of_detail(gran),
	    parsing_scope(scope) {}
    cmd(const cmd& C) = default;
    virtual ~cmd() = default;

    Directive			Code;
    Granularity			level_of_detail;
    Scope			parsing_scope;

    //
    // args should be a struct with the keys/value
    // combinations listed below; N/A means that the
    // value is unused:
    //
    //       Key	    Value			Type
    //       ---	    -----			----
    //	"parsing_scope"	    HeaderOnly: N/A		 -
    //			    FirstError: N/A		 -
    //			    RecordCount: record number	integer
    //			    CPUTime:	number of sec.	float
    //			    SCETTime:	time		SCET
    //			    All:	N/A		 -
    //
    //	"files"		    file names			array of strings
    //
    flexval			Args;
};

//
// Classes that hold comparison results:
//

//
// The following classes describe generic items in a
// diff list. In the simplest case, such an item is
// just a time-tagged pair of values, where the pair
// represents the values in the two files being compared.
//
// However, it is of great interest to know
// when the two files _agree_ instead of being
// different. So, we need an item that means
// something like "this represents a time interval
// from Ta to Tb over which Nab nodes agree between
// the two files."
//
// It is also of interest to identify areas where
// records present in one file are missing in the
// other file. Instead of inserting a diff for
// each item with one element of the pair staying
// the same over many successive records, it is
// better to replace all these records by a single
// one that identifies a "missing sequence".
//
// It is clear that the design should feature a base
// class, diff_pattern, and derived class for
// representing the various patterns identified
// above.
//

//
// An enum for expressing the various types of
// diff records
//
enum class difftype: char {
	Same,
	DeltaVal,
	OneOnly,
	TwoOnly,
	IntOneOnly,
	IntTwoOnly,
	DotDotDot,
	Undefined
};

//
// Base class for diff records (not abstract - need it as payload).
// Unfortunately, we need to make the payload a dumb, C-like union.
// Making it a base class is self-defeating because C++ does not
// recognize that nodes whose payload is a derived class are of
// the same type as nodes whose payload is the base class, leading
// to all kinds of unpleasant errors.
//
// We create the various sub-types needed by using the appropriate
// constructor.
//
class diff {
public:
    diff() : Type(difftype::Undefined) {}

    //
    // The "real" constructors:
    //
    diff(difftype type,
	 difftype int_type)
	: Type(type),
		interval_type(int_type),
		count(1),
		Last(false) {
	assert(type == difftype::DotDotDot);
    }
    diff(difftype	type,
	 const flexval&	val,
	 bool		last = false)
	: Type(type),
		Last(last) {
	assert(	type == difftype::Same
		|| type == difftype::OneOnly
		|| type == difftype::TwoOnly);
	if(type == difftype::TwoOnly) {
		val2 = val;
	} else {
		val1 = val;
	}
    }
    diff(difftype	type,
	 const flexval&	val,
	 const flexval& otherval,
	 bool		last = false)
	: Type(type),
		val1(val),
		val2(otherval),
		Last(last) {
	assert(	type == difftype::DeltaVal
		|| type == difftype::IntOneOnly
		|| type == difftype::IntTwoOnly);
    }
    diff(const diff&) = default;
    virtual ~diff() = default;

    diff& operator=(const diff&) = default;

    virtual difftype	get_type() const {
	return Type;
    }
    virtual string	to_string() const {
	stringstream s;
	switch(Type) {
	    case difftype::Same:
		{
		s << "\t";
		string cs = val1.to_string();
		s << cs;
		int tabs = cs.length() / 8;
		for(int k = 0; k < 6 - tabs; k++) {
		    s << "\t";
		}
		s << "=\n";
		}
		break;
	    case difftype::DeltaVal:
		{
		s << "\t";
		string cs = val1.to_string();
		s << cs;
		int tabs = cs.length() / 8;
		for(int k = 0; k < 3 - tabs; k++) {
		    s << "\t";
		}
		cs = val2.to_string();
		s << cs;
		tabs = cs.length() / 8;
		for(int k = 0; k < 3 - tabs; k++) {
		    s << "\t";
		}
		s << "!\n";
		}
		break;
	    case difftype::OneOnly:
		{
		s << "\t";
		string cs = val1.to_string();
		s << cs;
		int tabs = cs.length() / 8;
		for(int k = 0; k < 6 - tabs; k++) {
		    s << "\t";
		}
		s << "1\n";
		}
		break;
	    case difftype::IntOneOnly:
		{
		s << "\t";
		string cs = val1.to_string();
		s << cs;
		int tabs = cs.length() / 8;
		for(int k = 0; k < 3 - tabs; k++) {
		    s << "\t";
		}
		cs = val2.to_string();
		s << cs;
		tabs = cs.length() / 8;
		for(int k = 0; k < 3 - tabs; k++) {
		    s << "\t";
		}
		s << "!1\n";
		}
		break;
	    case difftype::TwoOnly:
		{
		s << "\t\t\t\t";
		string cs = val2.to_string();
		s << cs;
		int tabs = cs.length() / 8;
		for(int k = 0; k < 3 - tabs; k++) {
		    s << "\t";
		}
		s << "2\n";
		}
		break;
	    case difftype::IntTwoOnly:
		{
		s << "\t";
		string cs = val1.to_string();
		s << cs;
		int tabs = cs.length() / 8;
		for(int k = 0; k < 3 - tabs; k++) {
		    s << "\t";
		}
		cs = val2.to_string();
		s << cs;
		tabs = cs.length() / 8;
		for(int k = 0; k < 3 - tabs; k++) {
		    s << "\t";
		}
		s << "!2\n";
		}
		break;
	    case difftype::DotDotDot:
		s << "\t\t" << count << "\t\t\t\t       ...\n";
		break;
	    case difftype::Undefined:
	    default:
		s << " undefined\n";
		break;
	}
	return s.str();
    }

    int			count;
    difftype		Type;
    difftype		interval_type;
    bool		Last;
    flexval		val1;
    flexval		val2;
};

//
// This class holds diffs for one resource
//
class diff_rec {
public:

	//
	// Creates a record with empty history
	//
	diff_rec() {}
	diff_rec(const diff_rec&) = default;
	virtual ~diff_rec() = default;
	diff_rec& operator=(const diff_rec&) = default;

	//
	// We use the same method as the resource class
	// to store histories, but in this case the
	// history only contains discrepancies between
	// resources
	//
	tol::history<diff>	History;
};

class results {
public:
	results()
	    : pass(true),
		resources_only_in_one_file(2)
		{}
	virtual ~results() = default;

	void			report(
				    const cmd& C,
				    ostream& s);

	bool			pass;

	//
	// This array documents discrepancies between the
	// sets of resources covered by the two files:
	//
	vector<set<string> >	resources_only_in_one_file;

	//
	// This array contains the initial times in the
	// two files:
	//
	vector<CTime_base>	initial_times;
	CTime_base		final_time;

	map<Cstring, vector<Cstring> > containers_with_diffs;

	void			clear() {
		containers_with_diffs.clear();
		resources_only_in_one_file.clear();
		pass = true;
		diffs.clear();
		final_time = CTime_base();
	}

	//
	// Detailed lists of discrepancies, one for each
	// resource:
	//
	map<Cstring, diff_rec>	diffs;
};

//
// return: 0 (success), -1 (failure)
//
extern int		interpreter(
			    const cmd&		C,
			    results&		R);

} // end namespace tol_reader

#endif /* _TOL_READER_H_ */
