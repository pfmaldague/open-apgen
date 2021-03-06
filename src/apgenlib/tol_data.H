#ifndef _TOL_DATA_H_
#define _TOL_DATA_H_

#include <atomic>
#include <thread>

#include <sys/types.h>

//
// For encoding resource histories:
//
#include "tol_history.H"

// #define DEBUG_THIS
extern coutx tolexcl;

//
// tol = Time-Ordered Listing
//
namespace tol_reader {
	class cmd;
	class results;
}

namespace tol {

//
// "Work functions" for the three types of threads we need:
// parsing, processing and comparing. In all three cases,
// the last argument is the integer that the thread should
// set thread_index equal to.
//
extern void parse  (const tol_reader::cmd& C, int index);
extern void process(const tol_reader::cmd& C, int index);
extern void compare(const tol_reader::cmd& C, tol_reader::results* R, int index);
extern void interrupt(const tol_reader::cmd& C);

class safe_data;

//
// Utilities for comparator
//
extern bool same_within_tolerance(
		const flexval& f1,
		const flexval& f2);

//
// Utilities for comparing 2 files
//
extern CTime_base compare_histories(
	tol_reader::Granularity granny,
	vector<
	      pair<
		  tol::history<flexval>::flexnode*,
		  tol::history<flexval>::flexnode*
		  >
	      >&	current_vec,
	tol::safe_data* safe_vecs[],
	tol_reader::results* theResults);

extern void compare_initial_states(
	vector<
	      pair<
		  tol::history<flexval>::flexnode*,
		  tol::history<flexval>::flexnode*
		  >
	      >&	current_vec,
	tol_reader::results* theResults);

//
// Utilities for analyzing 1 file
//
extern CTime_base analyze_history(
	tol_reader::Granularity granny,
	vector<
	      pair<
		  tol::history<flexval>::flexnode*,
		  tol::history<flexval>::flexnode*
		  >
	      >&	current_vec,
	tol::safe_data* safe_vecs[],
	tol_reader::results* theResults);

extern void analyze_initial_state(
	vector<
	      pair<
		  tol::history<flexval>::flexnode*,
		  tol::history<flexval>::flexnode*
		  >
	      >&	current_vec,
	tol_reader::results* theResults);

//
// Utilities for translating 1 file
//

//
// NOTE: the arguments should reflect activities,
//       not resources
//
extern CTime_base translate_history_2_json(
	vector<
	      pair<
		  tol::history<flexval>::flexnode*,
		  tol::history<flexval>::flexnode*
		  >
	      >&	current_vec,
	tol::safe_data* safe_vecs[]);

//
// Can be overridden by command-line options:
//
extern double&						abs_tolerance();
extern double&						rel_tolerance();

//
// Not atomic because it is extracted from the
// first records in the TOL and by the time this
// information is shared among processes, the
// parser has moved on. No interference can
// occur.
//
extern CTime_base&					earliest_time_stamp(int);

//
// Vector of pointers to "safe history nodes", one for each resource.
// The history nodes from beginning of history till the "safe node"
// are OK for any thread to access, since the processing thread is
// done with them. Safe vectors are produced once in a while (every
// thousand records) by the processing threads. The comparison thread
// waits until safe vectors are available for both files for some
// common time interval, then processes them and waits for more until
// both files have been exhausted.
//
class safe_data {
public:
	safe_data() = default;
	safe_data(
	    CTime_base	capture_time,
	    const vector<tol::history<flexval>::flexnode*>&	vec)
		: safe_vector(vec),
		    safe_time(capture_time) {
	}
	~safe_data() = default;

	//
	// latest history nodes, in alphabetic order
	// of resource names
	//
	vector<tol::history<flexval>::flexnode*>	safe_vector;

	//
	// Capture time
	//
	CTime_base					safe_time;
};

//
// conversion from parsed value to flexval:
//
flexval				tolexp2flexval(const tolExp&);
flexval				tolarray2flexval(const tolExp&);

//
// safe data for comparator to use
//
extern atomic<safe_data*>&				safe_vector(int);

//
// Parsing data, generated by a TOL parser, is stored into
// the static object tol::theRecords() vector, which contains
// smart pointers to tol::pE objects. These objects must be
// deleted by the parser that created them, because of the
// way smart pointers work.
//

//
// Processed data cannot be stored as tol::pE objects, for the
// reasons explained above; the parser must delete all such
// objects just before it exits, and therefore such data
// cannot survive parsing.
//
// We use flexval to implement the various types of resource values.
//
class resource {
public:
	resource() = delete;
	resource(const resource&) = delete;

	//
	// A resource's history is never empty. One can add
	// to the history list, but never remove anything.
	//
	resource(
		const Cstring&	n,
		const Cstring&	container_n,
		const Cstring&	restype,
		flexval::Type	resdatatype,
		double		abs_tol,
		double		rel_tol,
		bool		interp,
		const flexval&	atts
		)
	    : name(n),
		container(container_n),
		Type(restype),
		DataType(resdatatype),
		AbsTolerance(abs_tol),
		RelTolerance(rel_tol),
		interpolated(interp),
		attributes(atts),
		History(this /* , T, tolexp2flexval(e) */ ) {
	}
	virtual ~resource() {}

	virtual const Cstring		get_name() const {
		return name;
	}

	Cstring				name;
	Cstring				container;
	Cstring				Type;
	flexval::Type			DataType;
	double				AbsTolerance;
	double				RelTolerance;
	bool				interpolated;
	flexval				attributes;

	flexval::Type			get_type() const {
		return History.currentval().getType();
	}

	//
	// For evaluation with linear interpolation:
	//
	flexval				eval(
			    history<flexval>::flexnode* prev,
			    history<flexval>::flexnode* curr,
			    const CTime_base&		T) {
	    assert(interpolated);
	    flexval ret;
	    CTime_base deltaT = T - prev->Key.getetime();
	    CTime_base delta_total = curr->Key.getetime() - prev->Key.getetime();
	    double fraction = deltaT / delta_total;
	    switch(DataType) {
		case flexval::TypeDouble:
		    {
		    double oldval = (double)prev->payload;
		    double newval = (double)curr->payload;
		    double val = newval * fraction + oldval * (1.0 - fraction);
		    ret = val;
		    }
		    break;
		case flexval::TypeInt:
		    {
		    long int oldval = (long int)prev->payload;
		    long int newval = (long int)curr->payload;
		    long int val = (long int)(newval * fraction + oldval * (1.0 - fraction));
		    ret = val;
		    }
		    break;
		case flexval::TypeDuration:
		    {
		    CTime_base olddur = ((cppDuration)prev->payload).T;
		    CTime_base newdur = ((cppDuration)curr->payload).T;
		    CTime_base val = (olddur + (newdur - olddur) * fraction);
		    ret = cppDuration(val);
		    }
		    break;
		case flexval::TypeTime:
		    {
		    CTime_base oldtime = ((cppDuration)prev->payload).T;
		    CTime_base newtime = ((cppDuration)curr->payload).T;
		    CTime_base val = (oldtime + (newtime - oldtime) * fraction);
		    ret = cppTime(val);
		    }
		    break;
		default:
		    break;
	    }
	    return ret;
	}

	history<flexval>		History;

	//
	// Used by comparator for output:
	//
	Cstring				get_info();
	bool				same_within_tolerance(
					    const flexval& v1,
					    const flexval& v2);

	//
	// The index map of the "resource database"
	//
	static map<Cstring, int>&	resource_indices(int);

	//
	// The actual data in the "resource database":
	//
	static vector<unique_ptr<resource> >& resource_data(int);
};

class activity_type {
public:
	activity_type() = delete;
	activity_type(const resource&) = delete;
	activity_type(const Cstring& s)
		: name(s) {}
	virtual ~activity_type() = default;

	Cstring				name;

	//
	// struct of <param name>, <param data type> pairs
	//
	flexval				parameters;

	//
	// The index map of the "activity database"
	//
	static map<Cstring, int>&	activity_indices(int);

	//
	// The actual data in the "activity database":
	//
	static vector<unique_ptr<activity_type> >& activity_data(int);
};

} // namespace tol

#endif /* _TOL_DATA_H_ */
