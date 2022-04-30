#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "tolReader.H"
#include "tol_data.H"

//
// A generic node in a resource history. Note that
// iterating over a history requires more work than
// just getting successive tolevent nodes...
//
typedef tol::history<flexval>::flexnode tolevent;

//
// Used in results::report():
//
typedef tol::history<tol_reader::diff>::flexnode diffevent;

//
// Used in vectors that store the current state
// of the comparator, while it is scanning the
// histories of resources from both processors:
//
typedef pair<tolevent*, tolevent*> eventpair;

//
// pointer to a vector of safe flexnodes
//
std::atomic<tol::safe_data*>&	tol::safe_vector(int i) {
	static std::atomic<tol::safe_data*> V[10];
	return V[i];
}

void tol_reader::results::report(
		const tol_reader::cmd&	CMD,
		ostream&		s) {

    //
    // General info
    //
    s << "Earliest time " << initial_times[0].to_string() << "\n";
    if(final_time > CTime_base()) {
	s << "Latest   time " << final_time.to_string() << "\n";
    }
    s << "Number of resources: " << tol::resource::resource_indices(0).size() << "\n\n";

    bool two_files = initial_times.size() == 2;

    //
    // Discrepancy in initial times
    //
    if(two_files && initial_times[0] != initial_times[1]) {
	s << "Initial times are different:\n"
	    << "\tfile 1: " << initial_times[0].to_string()
	    << "\n\tfile 2: " << initial_times[1].to_string()
	    << "\n";
    }

    //
    // Discrepancies in sets of resources
    //
    if(two_files) {
	for(int i = 0; i < 2; i++) {
	    set<string>::const_iterator	iter;
	    if(resources_only_in_one_file[i].size()) {
		s << "resources only in file " << (i + 1) << ":\n";
	    }
	    for(iter = resources_only_in_one_file[i].begin();
		iter != resources_only_in_one_file[i].end();
		iter++) {
		s << "\t" << *iter << "\n";
	    }
        }
    }

    //
    // Discrepancies in initial values
    //
    map<Cstring, vector<Cstring> >::const_iterator iter;
    if(containers_with_diffs.size() > 0) {
	s << "The following " << containers_with_diffs.size()
	    << " container(s) have resources with different initial values:\n";
	for(iter = containers_with_diffs.begin();
	    iter != containers_with_diffs.end();
	    iter++) {
	    s << "\t" << iter->first << "\n";
	    for(int k = 0; k < iter->second.size(); k++) {
		s << "\t\t" << iter->second[k] << "\n";
	    }
	}
    }

    //
    // if a detailed report is required, report history differences
    //
    if(two_files &&
	(CMD.level_of_detail == Granularity::Detailed
         || CMD.level_of_detail == Granularity::Full
	)
      ) {
	map<Cstring, int>::const_iterator iter;
	map<Cstring, int>::const_iterator iter2;
	for(	iter = tol::resource::resource_indices(0).begin();
		iter != tol::resource::resource_indices(0).end();
		iter++) {
	    int res_index = iter->second;
	    tol::resource* res1 = tol::resource::resource_data(0)[res_index].get();
	    Cstring resname = res1->name;

	    //
	    // Do not report anything for resources that agree in both files
	    //
	    if(!diffs[resname].History.empty()) {
		if(!res1->interpolated) {
		    s << res1->get_info() << ": "
			<< diffs[resname].History.values.get_length()
			<< " node(s)\n";

		    //
		    // Loop over history, which is a tol::history<flexpair>
		    // and contains a values list which is a tlist of nodes
		    // of type history<flexpair>::flexnode, a.k.a. diffevent.
		    //
		    tol::history<tol_reader::diff>& diff_hist = diffs[resname].History;
		    for(  diffevent* de = diff_hist.values.first_node();
			  de;
			  de = de->next_node()) {

			s << "\t" << de->getKey().getetime().to_string()
			    << de->payload.to_string();
		    }
		} else {
		    iter2 = tol::resource::resource_indices(1).find(resname);
		    int res_index_2 = iter2->second;
		    tol::resource* res2 = tol::resource::resource_data(1)[res_index_2].get();

		    //
		    // We walk simultaneously through the various
		    // histories
		    //
		    tol::history<tol_reader::diff>& diff_hist = diffs[resname].History;
		    tolevent* inode1 = res1->History.earliest_node();
		    tolevent* inode2 = res2->History.earliest_node();
		    tolevent* next1 = inode1->next_node();
		    tolevent* next2 = inode2->next_node();
		    bool no_diff_header_yet = true;
		    for(  diffevent* de = diff_hist.values.first_node();
			  de;
			  de = de->next_node()) {

			//
			// Check assumptions and decide whether to output a diff
			//
			bool skip = false;
			bool both = false, just_1 = false, just_2 = false;
			flexval val1, val2;
			switch(de->payload.get_type()) {
			    case tol_reader::difftype::Same:
				both = true;
				skip = true;
			    case tol_reader::difftype::DeltaVal:
				both = true;
				assert(de->Key.getetime() == inode1->Key.getetime());
				assert(de->Key.getetime() == inode2->Key.getetime());
				break;
			    case tol_reader::difftype::OneOnly:
				just_1 = true;
				assert(de->Key.getetime() == inode1->Key.getetime());
				{
				val1 = de->payload.val1;

				//
				// Compare value 1 to the interpolated
				// value of resource 2:
				//
				if(next2) {
				    val2 = res2->eval(
						inode2,
						next2,
						inode1->Key.getetime());
				} else {
				    val2 = res2->History.currentval();
				}
				if(res2->same_within_tolerance(val1, val2)) {
				    skip = true;
				}
				}
				break;
			    case tol_reader::difftype::TwoOnly:
				just_2 = true;
				assert(de->Key.getetime() == inode2->Key.getetime());
				{
				val2 = de->payload.val2;

				//
				// Compare value 2 to the interpolated
				// value of resource 1:
				//
				if(next1) {
				    val1 = res1->eval(
						inode1,
						next1,
						inode2->Key.getetime());
				} else {
				    val1 = res1->History.currentval();
				}
				if(res1->same_within_tolerance(val2, val1)) {
				    skip = true;
				}
				}
				break;
			    case tol_reader::difftype::DotDotDot:
			    case tol_reader::difftype::Undefined:
			    default:
				assert(false);
				break;
			}
			if(!skip) {

			    //
			    // Write diff header if necessary
			    //
			    if(no_diff_header_yet) {
				no_diff_header_yet = false;
				s << res1->get_info() << ": found diff node(s)\n";
			    }
			    if(de->payload.Type == tol_reader::difftype::OneOnly) {
				de->payload = tol_reader::diff(tol_reader::difftype::IntOneOnly,
								val1, val2);
			    } else if(de->payload.Type == tol_reader::difftype::TwoOnly) {
				de->payload = tol_reader::diff(tol_reader::difftype::IntTwoOnly,
								val1, val2);
			    }
			    s << "\t" << de->getKey().getetime().to_string()
				<< de->payload.to_string();
			} // do not skip this node

			//
			// Update resource history nodes as appropriate
			//
			if(next1) {
			    if(next2) {
				if(next1->Key.getetime() > next2->Key.getetime()) {
				    inode2 = next2;
				    next2 = next2->next_node();
				} else if(next1->Key.getetime() < next2->Key.getetime()) {
				    inode1 = next1;
				    next1 = next1->next_node();
				} else {
				    inode2 = next2;
				    next2 = next2->next_node();
				    inode1 = next1;
				    next1 = next1->next_node();
				}
			    } else {
				inode1 = next1;
				next1 = next1->next_node();
			    }
			} else if(next2) {
			    inode2 = next2;
			    next2 = next2->next_node();
			} // update history nodes
		    } // for this diff node
		} // if interpolated
	    } // if there are diffs
	} // for each resource
    } // if there are 2 files
} // end of report()

void tol::analyze_initial_state(
		vector< eventpair >& current_data_vector,
		tol_reader::results* Results)  {
    map<Cstring, int>::const_iterator iter;

    Results->pass = true;
    Results->initial_times.clear();
    Results->initial_times.push_back(tol::earliest_time_stamp(0));

    for(    iter = resource::resource_indices(0).begin();
	    iter != resource::resource_indices(0).end();
	    iter++) {
	int res_index_1 = iter->second;
	resource* res1 = resource::resource_data(0)[res_index_1].get();
	tolevent* inode1 = res1->History.earliest_node();
	flexval val1 = inode1->payload;

	//
	// Initialize things we will need while parsing
	// the rest of the files:
	// 	- current_data_vector
	// 	- Results->diffs
	//
    
	//
	// The order of the records in the current data
	// vector will reflect the ordering of the first TOL
	//
	current_data_vector.push_back(eventpair(inode1, NULL));

	//
	// Creates a record with empty history
	//
	Results->diffs[res1->name] = tol_reader::diff_rec();

    }
}

//
// Comparison results will be logged in Results
//
void tol::compare_initial_states(
    vector< eventpair >& current_data_vector,
    tol_reader::results* Results)  {

    bool files_contain_the_same_resources = true;
    bool initial_times_are_the_same = true;

    //
    // Check for the following:
    // 	- the two files contain the same resources
    // 	- the initial time stamps are the same
    // 	- the initial values are the same
    //
    map<Cstring, int>::const_iterator iter;
    map<Cstring, int>::const_iterator iter2;
    for(	iter = resource::resource_indices(0).begin();
		iter != resource::resource_indices(0).end();
		iter++) {
	Cstring resname = iter->first;
	int cur_index = iter->second;

	//
	// Find the resource in the second database
	//
	iter2 = resource::resource_indices(1).find(resname);
	if(iter2 == resource::resource_indices(1).end()) {
	    Results->resources_only_in_one_file[0].insert(string(*resname));
	}
    }
    if(Results->resources_only_in_one_file[0].size() > 0) {
	Results->pass = false;
	files_contain_the_same_resources = false;
    }
    for(	iter = resource::resource_indices(1).begin();
		iter != resource::resource_indices(1).end();
		iter++) {
	Cstring resname = iter->first;
	int cur_index = iter->second;

	//
	// Find the resource in the second database
	//
	iter2 = resource::resource_indices(0).find(resname);
	if(iter2 == resource::resource_indices(0).end()) {
	    Results->resources_only_in_one_file[1].insert(string(*resname));
	}
    }
    if(Results->resources_only_in_one_file[1].size() > 0) {
	Results->pass = false;
	files_contain_the_same_resources = false;
    }

    //
    // Next, check initial time stamps
    //
    CTime_base first_header_time = tol::earliest_time_stamp(0);
    CTime_base second_header_time = tol::earliest_time_stamp(1);
    Results->initial_times.clear();
    for(int i = 0; i < 2; i++) {
	Results->initial_times.push_back(
			tol::earliest_time_stamp(i));
    }
    if(Results->initial_times[0] != Results->initial_times[1]) {
	initial_times_are_the_same = false;
	Results->pass = false;
    }

    //
    // Finally, check initial values
    //
    if(files_contain_the_same_resources
       && initial_times_are_the_same) {

	for(    iter = resource::resource_indices(0).begin();
		    iter != resource::resource_indices(0).end();
		    iter++) {
	    iter2 = resource::resource_indices(1).find(iter->first);
	    int res_index_1 = iter->second;
	    int res_index_2 = iter2->second;
	    resource* res1 = resource::resource_data(0)[res_index_1].get();
	    resource* res2 = resource::resource_data(1)[res_index_2].get();
	    tolevent* inode1 = res1->History.earliest_node();
	    tolevent* inode2 = res2->History.earliest_node();
	    flexval val1 = inode1->payload;
	    flexval val2 = inode2->payload;

	    //
	    // Initialize things we will need while parsing
	    // the rest of the files:
	    // 	- current_data_vector
	    // 	- Results->diffs
	    //
    
	    //
	    // The order of the records in the current data
	    // vector will reflect the ordering of the first TOL
	    //
	    current_data_vector.push_back(eventpair(inode1, inode2));

	    //
	    // Creates a record with empty history
	    //
	    Results->diffs[res1->name] = tol_reader::diff_rec();

	    bool they_are_different = !res1->same_within_tolerance(val1, val2);
	    if(they_are_different) {
		// Results->pass = false;

		//
		// Results->diffs is a map<Cstring, diff_rec>
		//
		// Results->diffs[res1->name].History.append(
		// 	new diffevent(
		// 		first_header_time,
		// 		tol_reader::diff(
		// 		    tol_reader::difftype::DeltaVal, val1, val2)));

		map<Cstring, vector<Cstring> >::iterator iter
		    = Results->containers_with_diffs.find(res1->container);
		if(iter == Results->containers_with_diffs.end()) {
		    vector<Cstring> V;
		    Results->containers_with_diffs[res1->container] = V;
		    iter = Results->containers_with_diffs.find(res1->container);
		}

		aoString aos;
		aos << res1->get_info()
		    << ": val1 = " << val1.to_string()
		    << ", val2 = " << val2.to_string();
		iter->second.push_back(aos.str());
	    }
	}
    }
}

CTime_base tol::analyze_history(
	tol_reader::Granularity	granularity,
	vector< eventpair >&	current_vector,
	safe_data*		last_safe_vector[],
	tol_reader::results*	Results
	) {

    //
    // Will be returned:
    //
    CTime_base	final_time;

    map<Cstring, int>::const_iterator iter;

    CTime_base	safe_time(last_safe_vector[0]->safe_time);
    bool processing_has_completed =
	tol_reader::file_extraction_complete(0).load();

    //
    // We loop over resources and for each one, we grab
    // any new data in their resource histories. We store
    // any diffs in our own "database".
    //
    int current_vector_index = 0;
    int res_index = -1;

    // use this in error messages
    static int iteration = 0;
    iteration++;

    for(  iter = resource::resource_indices(0).begin();
	  iter != resource::resource_indices(0).end();
	  iter++) {

	int res_index_1 = iter->second;

	//
	// We need to do this because iter scans the resource
	// list in alphabetic order
	//
	res_index++;
	resource* res1 = resource::resource_data(0)[res_index_1].get();
	history<tol_reader::diff>& hist = Results->diffs[res1->name].History;

	//
	// Remember that a tolevent is a flexnode inside
	// a resource history.
	//
	tolevent* safe1 = last_safe_vector[0]->safe_vector[res_index];

	//
	// The index in our own data vector follows
	// the ordering of the first TOL file.
	//
	eventpair& a_pair = current_vector[current_vector_index];
	tolevent* e1 = a_pair.first;

	//
	// Check our assumptions
	//
	assert(safe1->list->Owner == res1);
	assert(e1->list->Owner == res1);
	assert(e1->getKey().getetime() <= safe1->getKey().getetime());

	while(true) {

	    if(!processing_has_completed && (e1 == safe1)) {
		break;
	    }

	    if(!processing_has_completed &&
		 (e1->getKey().getetime() >= safe_time)
	      ) {
		break;
	    }
	    const CTime_base& tag1 = e1->getKey().getetime();
	    flexval uninitialized;
	    tolevent* current_event = e1;

	    //
	    // Advance time.
	    //
	    if(e1->next_node()) {
		e1 = e1->next_node();
	    } else {
		break;
	    }
	}

	final_time = e1->getKey().getetime();

	//
	// Update pointers to current events
	//
	a_pair.first = e1;
	current_vector_index++;
    }

    return final_time;
}

//
// We need to change this. Currently, tol::safe_data only addresses
// resources, not activities.
//
CTime_base tol::translate_history_2_json(
	vector< eventpair >&	current_vector,
	safe_data*		last_safe_vector[]) {
	CTime_base T(0, 0, false);
	return T;
}

//
// The purpose of this method is to create a single "diff" list
// out of the two separate histories for each resource. Filtering
// of the diff list is delegated to a separate function, which will
// e. g. format the results as desired.
//
CTime_base tol::compare_histories(
	tol_reader::Granularity	granularity,
	vector< eventpair >&	current_vector,
	safe_data*		last_safe_vector[],
	tol_reader::results*	Results
	) {

#   ifdef DEBUG_THIS
    Cstring exclstr;
#   endif /* DEBUG_THIS */

    //
    // Will be returned:
    //
    CTime_base	final_time;

    map<Cstring, int>::const_iterator iter;
    map<Cstring, int>::const_iterator iter2;

    CTime_base	safe_time(last_safe_vector[0]->safe_time);
    CTime_base	safe_time2(last_safe_vector[1]->safe_time);
    if(safe_time > safe_time2) {

	//
	// Pick the earlier one of the two
	//
	safe_time = safe_time2;
    }
    bool processing_has_completed =
	tol_reader::file_extraction_complete(0).load()
	&& tol_reader::file_extraction_complete(1).load();

    //
    // We loop over resources and for each one, we grab
    // any new data in their resource histories. We store
    // any diffs in our own "database".
    //
    int current_vector_index = 0;
    int res_index = -1;

    // use this in error messages
    static int iteration = 0;
    iteration++;

    for(  iter = resource::resource_indices(0).begin();
	  iter != resource::resource_indices(0).end();
	  iter++) {

	//
	// In principle, the two resources databases
	// could have indices sorted differently,
	// so we don't want to assume the index is
	// the same.
	//
	iter2 = resource::resource_indices(1).find(iter->first);
	int res_index_1 = iter->second;
	int res_index_2 = iter2->second;

	//
	// We need to do this because iter scans the resource
	// list in alphabetic order
	//
	res_index++;
	resource* res1 = resource::resource_data(0)[res_index_1].get();
	resource* res2 = resource::resource_data(1)[res_index_2].get();
	history<tol_reader::diff>& hist = Results->diffs[res1->name].History;

	//
	// Remember that a tolevent is a flexnode inside
	// a resource history.
	//
	tolevent* safe1 = last_safe_vector[0]->safe_vector[res_index];
	tolevent* safe2 = last_safe_vector[1]->safe_vector[res_index];

	//
	// The index in our own data vector follows
	// the ordering of the first TOL file.
	//
	eventpair& a_pair = current_vector[current_vector_index];
	tolevent* e1 = a_pair.first;
	tolevent* e2 = a_pair.second;

	//
	// Check our assumptions
	//
	assert(safe1->list->Owner == res1);
	assert(safe2->list->Owner == res2);
	assert(e1->list->Owner == res1);
	assert(e2->list->Owner == res2);
	assert(e1->getKey().getetime() <= safe1->getKey().getetime());
	assert(e2->getKey().getetime() <= safe2->getKey().getetime());

	//
	// We scan the two lists as a Miterator would, and we
	// consolidate the difference(s) between the two lists
	// into a stream which we pass on to a separate thread.
	//

#	ifdef DEBUG_THIS
	bool first = true;
#	endif /* DEBUG_THIS */

	while(true) {

	    if(!processing_has_completed && (e1 == safe1 || e2 == safe2)) {
		break;
	    }

	    if(!processing_has_completed &&
		 (e1->getKey().getetime() >= safe_time
		  || e2->getKey().getetime() >= safe_time
		 )
	      ) {
		break;
	    }

#	    ifdef DEBUG_THIS
	    if(first) {
		first = false;
		exclstr << "compare_histories START\n";
		tolexcl << exclstr;
		exclstr.undefine();
	    }
#	    endif /* DEBUG_THIS */

	    const CTime_base& tag1 = e1->getKey().getetime();
	    const CTime_base& tag2 = e2->getKey().getetime();
	    flexval uninitialized;
	    pair<tolevent*, tolevent*> current_pair;
	    if(tag1 == tag2) {

#		ifdef DEBUG_THIS
		exclstr << res1->name << " both tags = " << tag1.to_string() << "\n";
		tolexcl << exclstr;
		exclstr.undefine();
#		endif /* DEBUG_THIS */

		current_pair.first = e1;
		current_pair.second = e2;
	    } else if(tag1 < tag2) {

#		ifdef DEBUG_THIS
		exclstr << res1->name << " tag1 = " << tag1.to_string() << " < tag 2 = "
		    << tag2.to_string() << "\n";
		tolexcl << exclstr;
		exclstr.undefine();
#		endif /* DEBUG_THIS */

		current_pair.first = e1;
		current_pair.second = NULL;
	    } else {

#		ifdef DEBUG_THIS
		exclstr << res1->name << " tag1 = " << tag1.to_string() << " > tag 2 = "
		    << tag2.to_string() << "\n";
		tolexcl << exclstr;
		exclstr.undefine();
#		endif /* DEBUG_THIS */

		current_pair.first = NULL;
		current_pair.second = e2;
	    }

	    //
	    // Possible patterns (note: "current node" is a pair):
	    //
	    //	   pattern			class used	current node
	    //	   =======			==========	============
	    //
	    // 	T    T	 val	val		  same		  (e1, e2)
	    //
	    // 	T    T	 val1	val2		delta_val	  (e1, e2)
	    //
	    // 	T    -	 val1	 -		one_only	  (e1, NULL)
	    //
	    // 	-    T	  -	val2		two_only	  (NULL, e2)
	    //

	    //
	    // CASE 1: REC IN BOTH FILES
	    //
	    if(current_pair.first && current_pair.second) {
		const flexval& val1 = e1->payload;
		const flexval& val2 = e2->payload;

		//
		// SUB-CASE A: VALUES DISAGREE
		//
		if(!res1->same_within_tolerance(val1, val2)) {
		    if(hist.empty()) {
			hist.append(new diffevent(
				    tag1,
				    tol_reader::diff(tol_reader::difftype::DeltaVal, val1, val2)));
		    } else {
			diffevent*		fn = hist.currentnode();
			tol_reader::difftype	prev_type = fn->payload.get_type();
			if(granularity == tol_reader::Granularity::Full
			   || res1->interpolated) {

			    //
			    // We insert a deltaval node no matter what
			    //
			    hist.append(new diffevent(
				tag1,
				tol_reader::diff(
				    tol_reader::difftype::DeltaVal, val1, val2)));
			} else {

			    //
			    // Is the previous node a deltaval node?
			    //
			    if(prev_type != tol_reader::difftype::DeltaVal) {

				//
				// No, append a new deltaval node
				//
				hist.append(new diffevent(
				    tag1,
				    tol_reader::diff(tol_reader::difftype::DeltaVal, val1, val2)));
			    } else {

				//
				// Previous node is a deltaval
				//
				tol_reader::diff* delta = &fn->payload;

				//
				// Is it the last node of a sequence?
				//
				if(delta->Last) {

				    //
				    // Does the sequence have more than 2 elements?
				    //
				    diffevent* fn1 = fn->previous_node();
				    if(fn1->payload.get_type() == tol_reader::difftype::DotDotDot) {
					tol_reader::diff* dots = &fn1->payload;

					//
					// Update existing dots and final nodes
					//
					dots->count++;
					hist.values.remove_node(fn);
					fn->Key = tag1;
					delta->val1 = val1;
					delta->val2 = val2;
					hist.values << fn;
				    } else {

					//
					// Insert dots and update final node
					//
					hist.values.remove_node(fn);
					hist.append(new diffevent(
					    fn->Key.getetime(),
					    tol_reader::diff(
						tol_reader::difftype::DotDotDot,
						tol_reader::difftype::DeltaVal)));
					fn->Key = tag1;
					delta->val1 = val1;
					delta->val2 = val2;
					hist.values << fn;
				    }
				} else {

				    //
				    // Insert a final node
				    //
				    hist.append(new diffevent(
					tag1,
					tol_reader::diff(tol_reader::difftype::DeltaVal, val1, val2, true)));
				} // not the last node of a sequence
			    } // previous type is deltaval
			} // granularity is Detailed
		    } // diff history is not empty

		//
		// SUB-CASE B: VALUES AGREE
		//
		} else {

		    //
		    // The two values are identical within tolerance
		    //
		    const flexval& val = e1->payload;
		    if(granularity == tol_reader::Granularity::Full
		       || res1->interpolated) {

			//
			// We insert a Same node no matter what
			//
			hist.append(new diffevent(
				tag1,
				tol_reader::diff(tol_reader::difftype::Same, val)));
		    } // granularity is full or res is interpolated
		} // values are identical within tolerance

	    //
	    // CASE 2: REC IN FILE 1 ONLY
	    //
	    } else if(current_pair.first) {

		//
		// Only file 1 has this record. To decide whether this
		// is acceptable or not, we must get into details:
		//
		// - if interpolation is constant, we check the new
		//   value against the currentval in the file with
		//   the missing record.
		//
		// - if interpolation is linear, we cannot tell whether
		//   the two files agree or not until we have a new
		//   data point for the file with the missing record.
		//   Therefore, we always add a diff node, and we defer
		//   cleaning up till report() is called.
		//
		const flexval& val1 = e1->payload;

		//
		// We need to check val1 against the currentval of res2.
		// This is _not_ provided by History.currentval(), which
		// reports the last node in the history.
		//
		flexval val2;
		if(!e2) {
		    val2 = res2->History.currentval();
		} else {
		    val2 = e2->previous_node()->payload;
		}
		bool values_are_different = !res1->same_within_tolerance(val1, val2);

		if(granularity == tol_reader::Granularity::Full
		   || res1->interpolated) {

		    //
		    // We insert a one_only node no matter what
		    //
		    hist.append(new diffevent(
				    tag1,
				    tol_reader::diff(tol_reader::difftype::OneOnly, val1)));
		} else if(values_are_different) {
		    if(hist.empty()) {
			hist.append(new diffevent(
				tag1,
				tol_reader::diff(tol_reader::difftype::OneOnly, val1)));
		    } else {
			diffevent*		fn = hist.currentnode();
			tol_reader::difftype	prev_type = fn->payload.get_type();

			//
			// Is the previous node a one_only node?
			//
			if(prev_type != tol_reader::difftype::OneOnly) {
				hist.append(new diffevent(
					    tag1,
					    tol_reader::diff(tol_reader::difftype::OneOnly, val1)));
			} else {

			    //
			    // Previous node is a new_type node
			    //
			    tol_reader::diff* one = &fn->payload;

			    //
			    // Is it the last node of a sequence?
			    //
			    if(one->Last) {

				//
				// Does the sequence have more than 2 elements?
				//
				diffevent* fn1 = fn->previous_node();
				if(fn1->payload.get_type() == tol_reader::difftype::DotDotDot) {
				    tol_reader::diff* dots = &fn1->payload;

				    //
				    // Update existing dots and final nodes
				    //
				    dots->count++;
				    hist.values.remove_node(fn);
				    fn->Key = tag1;
				    one->val1 = val1;
				    hist.values << fn;
				} else {

				    //
				    // Insert dots and update final node
				    //
				    hist.values.remove_node(fn);
				    hist.append(new diffevent(
					    fn->Key.getetime(),
					    tol_reader::diff(
						tol_reader::difftype::DotDotDot,
						tol_reader::difftype::OneOnly)));
				    fn->Key = tag1;
				    one->val1 = val1;
				    hist.values << fn;
				}
			    } else {

				//
				// Insert a final node
				//
				hist.append(new diffevent(
					tag1,
					tol_reader::diff(tol_reader::difftype::OneOnly, val1, true)));
			    } // not the last node of a sequence
			} // previous type is one_only
		    } // history is not empty
		} // values are different

	    //
	    // CASE 3: REC IN FILE 2 ONLY
	    //
	    } else {

		//
		// Only file 2 has this record. To decide whether this
		// is acceptable or not, we must get into details:
		//
		// - if interpolation is constant, we check the new
		//   value against the currentval in the file with
		//   the missing record.
		//
		// - if interpolation is linear, we cannot tell whether
		//   the two files agree or not until we have a new
		//   data point for the file with the missing record.
		//

		const flexval& val2 = e2->payload;

		//
		// We need to check val2 against the currentval of res1.
		// This is _not_ provided by History.currentval(), which
		// reports the last node in the history.
		//
		flexval val1;
		if(!e1) {
		    val1 = res1->History.currentval();
		} else {
		    val1 = e1->previous_node()->payload;
		}
		bool values_are_different = !res1->same_within_tolerance(val1, val2);

		if(granularity == tol_reader::Granularity::Full
		   || res1->interpolated) {

		    //
		    // We insert a two_only node no matter what
		    //
		    hist.append(new diffevent(
			    tag2,
			    tol_reader::diff(tol_reader::difftype::TwoOnly, val2)));
		} else if(values_are_different) {
		    if(hist.empty()) {
			hist.append(new diffevent(
				tag2,
				tol_reader::diff(
				    tol_reader::difftype::TwoOnly, val2)));
		    } else {
			diffevent*		fn = hist.currentnode();
			tol_reader::difftype	prev_type = fn->payload.get_type();

			//
			// Is the previous node a two_only node?
			//
			if(prev_type != tol_reader::difftype::TwoOnly) {
			    hist.append(new diffevent(
				    tag2,
				    tol_reader::diff(tol_reader::difftype::TwoOnly, val2)));
			} else {

			    //
			    // Previous node is a two_only node
			    //
			    tol_reader::diff* two = &fn->payload;

			    //
			    // Is it the last node of a sequence?
			    //
			    if(two->Last) {

				//
				// Does the sequence have more than 2 elements?
				//
				diffevent* fn1 = fn->previous_node();
				if(fn1->payload.get_type() == tol_reader::difftype::DotDotDot) {
				    tol_reader::diff* dots = &fn1->payload;

				    //
				    // Update existing dots and final nodes
				    //
				    dots->count++;
				    hist.values.remove_node(fn);
				    fn->Key = tag2;
				    two->val2 = val2;
				    hist.values << fn;
				} else {

				    //
				    // Insert dots and update final node
				    //
				    hist.values.remove_node(fn);
				    hist.append(new diffevent(
					    fn->Key.getetime(),
					    tol_reader::diff(
						tol_reader::difftype::DotDotDot,
						tol_reader::difftype::TwoOnly)));
				    fn->Key = tag2;
				    two->val2 = val2;
				    hist.values << fn;
				}
			    } else {

				//
				// Insert a final node
				//
				hist.append(new diffevent(
					tag2,
					tol_reader::diff(
						tol_reader::difftype::TwoOnly, val2, true)));
			    } // not the last node of a sequence
			} // previous type is two_only
		    } // history is not empty
		} // values are different
	    } // Only file 2 has this record

	    //
	    // Advance time.
	    //

	    //
	    // Possible patterns (note: "current node" is a pair):
	    //
	    //	   pattern			class used	current node
	    //	   =======			==========	============
	    //
	    // 	T    T	 val	val		  same		  (e1, e2)
	    //
	    // 	T    T	 val1	val2		delta_val	  (e1, e2)
	    //
	    // 	T    -	 val1	 -		one_only	  (e1, NULL)
	    //
	    // 	-    T	  -	val2		two_only	  (NULL, e2)
	    //
	    if(current_pair.first && current_pair.second) {
		if(e1->next_node()) {
		    if(e2->next_node()) {

			//
			// Always update both nodes
			//
			e1 = e1->next_node();
			e2 = e2->next_node();
			if(e1->getKey().getetime()
				== e2->getKey().getetime()) {
			    current_pair.first = e1;
			    current_pair.second = e2;
			} else if(e1->getKey().getetime()
				  < e2->getKey().getetime()) {
			    current_pair.first = e1;
			    current_pair.second = NULL;
			} else {
			    current_pair.first = NULL;
			    current_pair.second = e2;
			}
		    } else {
			e1 = e1->next_node();
			current_pair.first = e1;
			current_pair.second = NULL;
		    }
		} else if(e2->next_node()) {

		    //
		    // Only update the node that will not become NULL
		    //
		    e2 = e2->next_node();
		    current_pair.first = NULL;
		    current_pair.second = e2;
		} else {
		    break;
		}
	    } else if(current_pair.first) {

		if(e1->next_node()) {

		    //
		    // Always update the first node,
		    // since we just processed it
		    //
		    e1 = e1->next_node();
		} else if(tag2 > tag1) {
		    current_pair.first = NULL;
		    current_pair.second = e2;
		} else {
		    break;
		}
	    } else {

		if(e2->next_node()) {

		    //
		    // Always update the second node,
		    // since we just processed it
		    //
		    e2 = e2->next_node();
		} else if(tag1 > tag2) {

		    current_pair.first = e1;
		    current_pair.second = NULL;
		} else {
		    break;
		}
	    }
	}

	final_time = e1->getKey().getetime();
	if(e2->getKey().getetime() > final_time) {
	    final_time = e2->getKey().getetime();
	}

	//
	// Update pointers to current events
	//
	a_pair.first = e1;
	a_pair.second = e2;
	current_vector_index++;
    }
    return final_time;
}

//
// The "work function" for the comparator thread
//
void tol::compare(
	    const tol_reader::cmd&	CMD,
	    tol_reader::results*	Results,
	    int				index) {
    thread_index = index;
    map<Cstring, int>::const_iterator iter;
    map<Cstring, int>::const_iterator iter2;

    //
    // Initialize the comparison results which will 
    // be examined by tol_interpreter.
    //
    Results->pass = true;

    //
    // Pointers to data vectors from the processing threads
    //
    safe_data*	last_safe_vector[2] = {NULL, NULL};

    //
    // Our own vector pointing to last history nodes we processed.
    // This vector needs to persist across calls, since typically
    // a session starts with a header-parsing command and continues
    // with a command to parse the rest of the file.
    //
    // For the time being we just make it static; we will worry
    // about more sophisticated management schemes later on.
    //
    static vector< eventpair >	current_data_vector;

    if(CMD.Code == tol_reader::Directive::Open) {

	//
	// First inspect the command to see whether we have
	// one file or two
	//
	const vector<flexval>& filenames
	    = CMD.Args.get_struct().find("files")->second.get_array();
	if(filenames.size() == 2) {

	    if(CMD.parsing_scope == tol_reader::Scope::All
		      || CMD.parsing_scope == tol_reader::Scope::RecordCount
		      || CMD.parsing_scope == tol_reader::Scope::CPUTime) {
		bool		break_from_outer_loop = false;
		int		res_index = 0;
		CTime_base	final_time;

		//
		// Note that we cannot really ascertain the differences
		// between partial TOLs until after _all_ records with
		// the same time stamp have been processed. Therefore,
		// we must wait until both processing threads have
		// either completed or gone beyond the previously processed
		// time tag.
		//
		bool		all_initial_records_have_been_read[2] = {false, false};

		//
		// We keep track of time tags processed so far in this
		// variable:
		//
		CTime_base	tag_of_latest_avail_data;
		CTime_base	tag_of_latest_processed_data;
		CTime_base	tag_of_avail_data[2];
		bool		first_time_through = true;

		//
		// Loop over data vectors from the processing threads
		//
		while(true) {

		    //
		    // Wait till data becomes available from both
		    // processing threads, or till both processes
		    // finish. Use a vector of safe values, one for
		    // each resource, in analogy with APGenX.
		    //
		    while(true) {
			if( tol_reader::file_extraction_complete(0).load()
			    && tol_reader::file_extraction_complete(1).load()) {

			    break_from_outer_loop = true;

			    assert(tol_reader::errors_found().load()
				|| first_time_through
				|| (last_safe_vector[0] && last_safe_vector[1]));

			}
			for(int k = 0; k < 2; k++) {
			    safe_data* tmp_vec = tol::safe_vector(k).load();
			    if(   tmp_vec && tmp_vec != last_safe_vector[k]) {

				//
				// It is safe to fetch first_time_tag, since
				// at least one time-tagged record has been
				// parsed
				//
				CTime_base	first_time(pE::first_time_tag(k).load());

				tag_of_avail_data[k] = tmp_vec->safe_time;
				all_initial_records_have_been_read[k] = tag_of_avail_data[k]
					> first_time;

				if(last_safe_vector[k]) {
				    delete last_safe_vector[k];
				}
				last_safe_vector[k] = tmp_vec;

#				ifdef DEBUG_THIS
				Cstring exclstr;
				exclstr << "compare: copied safe_vector[" << k << "], set it to NULL\n";
				tolexcl << exclstr;
#				endif /* DEBUG_THIS */

				safe_vector(k).store(NULL);
			    }
			}
			if(last_safe_vector[0] && last_safe_vector[1]) {

			    //
			    // Some data is available for both threads
			    //
			    if(last_safe_vector[0]->safe_time < last_safe_vector[1]->safe_time) {
				tag_of_latest_avail_data = last_safe_vector[0]->safe_time;
			    } else {
				tag_of_latest_avail_data = last_safe_vector[1]->safe_time;
			    }
			}

			if(	(all_initial_records_have_been_read[0]
				&& all_initial_records_have_been_read[1])
				|| break_from_outer_loop) {

			    //
			    // Before anything else, we need to make sure there was no error
			    //
			    if(tol_reader::errors_found().load()) {
				return;
			    }

			    assert(last_safe_vector[0] && last_safe_vector[1]);
			    break;
			}
			this_thread::sleep_for(chrono::milliseconds(10));

		    } // end of waiting for initial data

		    tag_of_latest_processed_data = tag_of_latest_avail_data;

		    //
		    // The first time we process data is special,
		    // because the very first batch of resource
		    // records in the TOL define the initial state
		    // of the resource system. We need to make sure,
		    // among other things, that both files contain
		    // the same resource, and that the initial data
		    // are compatible.
		    //
		    if(first_time_through) {
			first_time_through = false;
			compare_initial_states(
				current_data_vector,
				Results);

			//
			// If we found problems but they are not fatal,
			// we continue checking resource histories
			//
			if(!Results->pass
			   && (Results->resources_only_in_one_file.size() != 0
			       || Results->initial_times.size() != 2
			       || Results->initial_times[0] != Results->initial_times[1]
			      )
			  ) {

			    //
			    // Errors are too significant - cannot check histories.
			    //
			    // Note: this could be relaxed, but we would need to
			    //	     change the code so it tolerates initial values
			    //	     at different times
			    //
			    tol_reader::threadErrors(thread_index)
				<< "Compare: significant discrepancies between files\n";
			    tol_reader::errors_found().store(true);
			    return;
			}
		    }

		    assert(last_safe_vector[0] && last_safe_vector[1]);
		    final_time = compare_histories(
				CMD.level_of_detail,
				current_data_vector,
				last_safe_vector,
				Results);

		    if(break_from_outer_loop) {
			break;
		    }
		} // loop over data vectors

		//
		// Clean up
		//
		for(int k = 0; k < 2; k++) {
		    if(last_safe_vector[k]) {
			delete last_safe_vector[k];
		    }
		}
		if(Results->final_time < final_time) {
		    Results->final_time = final_time;
		}
	    } // if Scope = All
	} // if filenames.size() = 2

	//
	// Now we know there is only one file
	//
	else {
	    bool	break_from_outer_loop = false;
	    int		res_index = 0;
	    CTime_base	final_time;

	    //
	    // Note that we cannot really ascertain the differences
	    // between partial TOLs until after _all_ records with
	    // the same time stamp have been processed. Therefore,
	    // we must wait until both processing threads have
	    // either completed or gone beyond the previously processed
	    // time tag.
	    //
	    bool	all_initial_records_have_been_read = false;

	    //
	    // We keep track of time tags processed so far in this
	    // variable:
	    //
	    CTime_base	tag_of_latest_avail_data;
	    CTime_base	tag_of_latest_processed_data;
	    CTime_base	tag_of_avail_data;
	    bool	first_time_through = true;

	    //
	    // Loop over data vectors from the processing thread
	    //
	    while(true) {

		//
		// Wait till data becomes available or till
		// the extraction process finishes. Use a vector
		// of safe values for each resource, in analogy
		// with APGenX.
		//
		while(true) {
		    if( tol_reader::file_extraction_complete(0).load()) {

			    break_from_outer_loop = true;

			    assert(tol_reader::errors_found().load()
				|| first_time_through
				|| last_safe_vector[0]);

		    }
		    safe_data* tmp_vec = tol::safe_vector(0).load();
		    if(   tmp_vec && tmp_vec != last_safe_vector[0]) {

			//
			// It is safe to fetch first_time_tag, since
			// at least one time-tagged record has been
			// parsed
			//
			CTime_base	first_time(pE::first_time_tag(0).load());

			tag_of_avail_data = tmp_vec->safe_time;
			all_initial_records_have_been_read = tag_of_avail_data > first_time;

			if(last_safe_vector[0]) {
			    delete last_safe_vector[0];
			}
			last_safe_vector[0] = tmp_vec;

#			ifdef DEBUG_THIS
			Cstring exclstr;
			exclstr << "compare: copied safe_vector, set it to NULL\n";
			tolexcl << exclstr;
			exclstr.undefine();
			exclstr << "         initial records read: "
				<< all_initial_records_have_been_read << "\n";
			tolexcl << exclstr;
#			endif /* DEBUG_THIS */

			safe_vector(0).store(NULL);
		    }
		    if(last_safe_vector[0]) {

			//
			// Some data is available
			//
			tag_of_latest_avail_data = last_safe_vector[0]->safe_time;
		    }

		    if(all_initial_records_have_been_read || break_from_outer_loop) {

			//
			// Before anything else, we need to make sure there was no error
			//
			if(tol_reader::errors_found().load()) {
			    return;
			}

			assert(last_safe_vector[0]);

			break;
		    }

		    this_thread::sleep_for(chrono::milliseconds(10));

		} // end of waiting for initial data

		tag_of_latest_processed_data = tag_of_latest_avail_data;

		//
		// The first time we process data is special,
		// because the very first batch of resource
		// records in the TOL define the initial state
		// of the resource system. We need to make sure,
		// among other things, that both files contain
		// the same resource, and that the initial data
		// are compatible.
		//
		if(first_time_through) {
		    first_time_through = false;

#		    ifdef DEBUG_THIS
		    Cstring exclstr;
		    exclstr << "compare: first time through, calling analyze initial state\n";
		    tolexcl << exclstr;
#		    endif /* DEBUG_THIS */

		    analyze_initial_state(
				current_data_vector,
				Results);

		    //
		    // If we found problems but they are not fatal,
		    // we continue checking resource histories
		    //
		    if(!Results->pass) {

			//
			// Errors are too significant - cannot check histories.
			//
			// Note: this could be relaxed, but we would need to
			//	     change the code so it tolerates initial values
			//	     at different times
			//
			tol_reader::threadErrors(thread_index)
				<< "Analyze: significant problems with the file\n";
			tol_reader::errors_found().store(true);
			return;
		    }
		}

		assert(last_safe_vector[0]);

#		ifdef DEBUG_THIS
		Cstring exclstr;
		exclstr << "compare: calling analyze history\n";
		tolexcl << exclstr;
#		endif /* DEBUG_THIS */

		final_time = analyze_history(
				CMD.level_of_detail,
				current_data_vector,
				last_safe_vector,
				Results);

		if(break_from_outer_loop) {
		    break;
		}
	    } // loop over data vectors

	    //
	    // Clean up
	    //
	    if(last_safe_vector[0]) {
		delete last_safe_vector[0];
	    }
	    if(Results->final_time < final_time) {
		Results->final_time = final_time;
	    }
	} // if one or two files
    } // if cmd directive = Open
    else if(CMD.Code == tol_reader::Directive::ToJson) {
	const vector<flexval>& filenames
	    = CMD.Args.get_struct().find("files")->second.get_array();

	assert(filenames.size() == 1);

	bool		break_from_outer_loop = false;
	int		res_index = 0;
	CTime_base	final_time;

	//
	// We keep track of time tags processed so far in this
	// variable:
	//
	CTime_base	tag_of_latest_avail_data;
	CTime_base	tag_of_latest_processed_data;
	CTime_base	tag_of_avail_data;

	//
	// Loop over data vectors from the processing thread
	//
	while(true) {

	    //
	    // Wait till data becomes available or till
	    // the extraction process finishes. Use a vector
	    // of safe values for each resource, in analogy
	    // with APGenX.
	    //
	    while(true) {
		bool extraction_is_complete = tol_reader::file_extraction_complete(0).load();
		safe_data* tmp_vec = tol::safe_vector(0).load();
		if(tmp_vec && tmp_vec != last_safe_vector[0]) {

		    //
		    // It is safe to fetch first_time_tag, since
		    // at least one time-tagged record has been
		    // parsed
		    //
		    CTime_base	first_time(pE::first_time_tag(0).load());

		    tag_of_avail_data = tmp_vec->safe_time;

		    if(last_safe_vector[0]) {
			delete last_safe_vector[0];
		    }
		    last_safe_vector[0] = tmp_vec;

		    safe_vector(0).store(NULL);
		}
		if(last_safe_vector[0]) {

		    //
		    // Some data is available
		    //
		    tag_of_latest_avail_data = last_safe_vector[0]->safe_time;
		}

		if(extraction_is_complete) {

		    break_from_outer_loop = true;

		    //
		    // We grabbed the safe vector _after_ we detected
		    // that extraction was complete
		    //
		    assert(tol_reader::errors_found().load()
			|| last_safe_vector[0]);

		}

		if(break_from_outer_loop) {

		    //
		    // Before anything else, we need to make sure there was no error
		    //
		    if(tol_reader::errors_found().load()) {
			return;
		    }

		    assert(last_safe_vector[0]);

		    break;
		}

		this_thread::sleep_for(chrono::milliseconds(10));

	    } // end of waiting for initial data

	    tag_of_latest_processed_data = tag_of_latest_avail_data;

	    assert(last_safe_vector[0]);

	    final_time = translate_history_2_json(
				current_data_vector,
				last_safe_vector
				);

	    if(break_from_outer_loop) {
		break;
	    }
	} // loop over data vectors

	//
	// Clean up
	//
	if(last_safe_vector[0]) {
	    delete last_safe_vector[0];
	}
	if(Results->final_time < final_time) {
	    Results->final_time = final_time;
	}
    }
}
