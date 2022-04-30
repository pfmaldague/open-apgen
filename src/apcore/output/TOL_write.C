#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "Constraint.H"
#include "TOL_write.H"
#include <sys/stat.h>

// debug
// #define DEBUG_THIS

extern coutx excl;

streamx&	tol_debug_stream() {
	static streamx S;
	return S;
}


void handle_a_resource_event(
	value_node&			vn,
	const CTime_base&		event_time,
	Rsource*			resource,
	tlist<alpha_void, res_record>&	resources_taken_care_of,
	vector<bool>&			res_selection) {
    TypedValue	after;
    int		resindex = resource->output_index[thread_index];

    Cnode0<alpha_void, RCsource*>* ptr_to_dep;
    bool			interpolated
		= resource->properties[(int)Rsource::Property::has_interpolation];

    //
    // If the the owner of the vn is not resource, that means
    // that resource has a profile which depends on vn's owner.
    // The logic in that case is in the 'else' part of the code
    // below.
    //
    if(interpolated && vn.list->Owner == resource) {
	res_record* ptr_to_prev_entry = NULL;

	//
	// Fix for AP-836 - only works for numeric resources!
	//
	after = resource->nodeval2resval(&vn);

	//
	// Normally, one would think that an interpolated resource
	// should be smooth, i. e., free of discontinuities. In
	// particular, it should never have more than one data
	// point at any time point. However, in practice that is
	// not the way "interpolation" is understood by APGenX
	// adapters; they tolerate discontinuities, and complain
	// when these discontinuities do not show up in the (XML)TOL.
	//
	// To accommodate this "piecewise smooth" interpretation of
	// the word "interpolated", we faithfully reproduce the
	// entire history of interpolated resources in the (XML)TOL.
	// This will allow the semantics to carry over to the post-
	// processing software, which had better be ready to deal
	// with discontinuities!
	//
    	if((ptr_to_prev_entry = resources_taken_care_of.find((void*)resource))) {

	    //
	    // We already have a resource value. Let's check whether it's the same.
	    // Make sure to use is_equal_to, which uses a finite tolerance.
	    //
	    if(after.is_equal_to(ptr_to_prev_entry->payload.second, resource)) {

		//
		// We never, ever want to repeat both time tags and values.
		//
		return;
	    } else {

		//
		// We have a new value at the same time point. Update
		// the entry, because the main output loop only updates
		// the list of entries when time advances.
		//
		ptr_to_prev_entry->payload.second = after;
	    }
	} else {

    	    resources_taken_care_of << new res_record(
					resource,
					pair<Rsource*, TypedValue>(resource, after));
	}

	//
	// BUG in X.4.20a: resources that depend on this one via their
	// profile should now be evaluated and reported
	//
	// FIX in X.4.20b: comment out the following statement; make
	// sure that _all_ dependent resources get processed down
	// below
	//
	// return;
    } else {

	//
	// If the resource has already been taken care of, we don't need
	// to process it again. (This happens when the resource history
	// has more than one node per time point.)
	//
	if(resources_taken_care_of.find((void *) resource)) {
	    return;
	}
	resource->evaluate_present_value(
					after,
					event_time);
	after.cast(resource->get_datatype());

	//
	// For non-interpolated resources, we do the following:
	//
	// 	- only one value per time point
	//
	// 	- the value is the one reported with the AFTER evaluation policy
	//
	// 	- if the AFTER value is the same as the previous value, it only
	// 	  gets reported if the "No Filtering" attribute is true
	//

#ifdef NO_FILTERING_IN_THIS_THREAD
	res_record2& prev_res_val = previous_resource_values[resindex];
	if(prev_res_val.selected) {

	    //
	    // AP-787: the "No Filtering" attribute must prevent history pruning
	    //
	    // Accordingly, we only filter out redundant values (i. e. values
	    // that repeat a previously reported value) if the No Filtering
	    // attribute is not set.
	    //
	    if(!resource->properties[(int)Rsource::Property::has_nofiltering]
	       && prev_res_val.val.is_equal_to(after, resource)) {
		return;
	    } // not filtered and equal
	} // previous value available
#endif /* NO_FILTERING_IN_THIS_THREAD */

	//
	// We insert the node _after_ checking that the resource value
	// does not repeat a previous value
	//
	resources_taken_care_of
		<< new res_record(
				resource,
				pair<Rsource*, TypedValue>(resource, after));
    }

    slist<alpha_void, Cnode0<alpha_void, RCsource*> >::iterator
	dep_iter(resource->parent_container.payload->containers_whose_profile_invokes_this);

    assert(after.get_type() != apgen::DATA_TYPE::UNINITIALIZED);

    //
    // Automatically report the value of all dependent resources.
    // Note that we have a value_node, but it is for the triggering
    // resource, not for the dependent resource. The logic inside
    // handle_a_resource_event handles that.
    //
    // Also note that we do _not_ want to evaluate settable resources
    // which may have a resource currentval in their default section;
    // the default section is only used in initializing the resource
    // and is never used thereafter.
    //
    while((ptr_to_dep = dep_iter())) {
	Rsource*	dependent;
	RCsource*       dependent_cont = ptr_to_dep->payload;
	simple_res_iter iter(dependent_cont);

	while((dependent = iter.next())) {
	    RES_settable* settable_dependent
			    = dynamic_cast<RES_settable*>(dependent);
	    if(settable_dependent) {
		continue;
	    }
	    if(!resources_taken_care_of.find((void *) dependent)
		&& res_selection[dependent->output_index[thread_index]]) {

		handle_a_resource_event(
				    vn,
				    event_time,
				    dependent,
				    resources_taken_care_of,
				    res_selection);
	    }
	}
    }
}

//
// writeTOL is split it into two functions:
//
// 	- writeTOLbody<tol_publisher> for TOL
// 	- writeTOLbody<xml_publisher> for XMLTOL
//
// The Thread argument can be used to tell whether the output
// process should be single- or multi-threaded.
//
// As of now (Oct. 2020) Thread is always NULL, because
// TOL and XMLTOL output no longer occur in parallel
// with modeling. However, we keep it around just in case
// parallel output raises its head again.
//
void IO_writer::writeTOL(
		const Cstring&		file_name,
		const Cstring&		xml_file_name,
		CTime_base		start,
		CTime_base		end,
		bool			do_tol,
		bool			do_xmltol,
		const Cstring&		optional_timesystem,
		bool			all_acts_visible,
		const vector<string>&	resources,
		thread_intfc*		Thread) {
    FILE*		fout = NULL;
    FILE*		xml_fout = NULL;
    stringslist		resourceList;
    stringslist::iterator iterator(resourceList);
    emptySymbol*	resourceNode;
    Rsource*		resource;
    pinfotlist		param_info;

    //
    // First open the output file
    //
    fout = fopen((char *) *file_name, "w");
    if(!fout) {
	Cstring errs;
	errs << "Write TOL: An error occurred while trying to open file \""
			<< file_name << "\"\n";
	throw(eval_error(errs));
    }

    if(do_tol && do_xmltol) {
	xml_fout = fopen((char *) *xml_file_name, "w");
	if(!xml_fout) {
	    Cstring errs;
	    errs << "Write TOL: An error occurred while trying to open file \""
			<< xml_file_name << "\"\n";
	    throw(eval_error(errs));
	}
    }

    //
    // TOL declarations
    //
    aoString		Stream;
    FILE*		faux = NULL;
    
    //
    // XMLTOL declarations
    //
#   ifdef have_xmltol
    xml_publisher::first_time_writing	= true;
    xml_publisher::timesystem		= NULL;
    if(do_xmltol) {
	if(do_tol) {
		xml_publisher::theOutputFile	= xml_fout;
	} else {
		xml_publisher::theOutputFile	= fout;
	}
    }
    xmlpp::Document*  document		= NULL;
#   endif /* have_xmltol */

    //
    // TOL preliminaries. This includes writing the
    // start of the TOL header and paving the way
    // for metadata, but not writing the metadata
    // itself; that will be done by writeTOLbody().
    //
    if(do_tol) {
	Cstring	aux_file_name(file_name + ".aux");

	faux = fopen((char *) *aux_file_name, "w");
	if(!faux) {
	    Cstring errs;
	    errs << "Write TOL: An error occurred while trying to open file \"";
	    errs << aux_file_name << "\"\n";
	    throw(eval_error(errs));
	}

	//
	// Write TOL file header:
	// 	- TOL identifier
	// 	- name of auxiliary file
	// 	- list of hidden resources
	//	- list of AAF and APF files
	//	- resource metadata (list of all resources w/attributes)
	//	- activity metadata (list of all act types w/attr. and param.)
	//

	//
	// Start with initial TOL identifier and name of
	// auxiliary file. Note: "V2.1" added in APGenX
	// version X.7.
	//
	Stream << "apgen TOL format V2.1 -- "
		<< get_apgen_version_build_platform()
		<< "\nauxiliary_tol=" << *aux_file_name << "\n";

	//
	// Continue with list of hidden resources
	//
	Rsource::get_all_resource_names(resourceList);

	Cstring	hidden_resources("n_hidden_resources=");
	aoString	hidden_stuff;
	int		n_hidden = 0;

	while((resourceNode = iterator.next())) {
	    resource = eval_intfc::FindResource(resourceNode->get_key());
	    if(resource->is_hidden()) {
		n_hidden++;
		hidden_stuff << "hidden_resource="
			 << resourceNode->get_key() << "\n";
	    }
	}
	hidden_resources << n_hidden << "\n" << hidden_stuff.str();
	Stream << hidden_resources;

	//
	// Add list of apgen adaptation and other files that were read
	// at the beginning of this session
	//
	Stream << get_tol_header_info(file_name);

	//
	// If this is a "complete" TOL, i. e., not a partial TOL,
	// continue with list of all activity types, which are
	// behavior types in the "activity" realm.
	//
	// The criterion for deciding whether this is a "complete"
	// or "partial" TOL is whether the resources array contains
	// any elements. This is somewhat inconsistent if the list
	// contains all resources, but so be it.
	//
	if(resources.size() == 0) {
	    Stream << "activity_metadata:\n";
	    map<Cstring, int>::const_iterator iter;
	    for(  iter = Behavior::ClassIndices().begin();
		  iter != Behavior::ClassIndices().end();
		  iter++) {
		int act_index = iter->second;
		Behavior* beh = Behavior::ClassTypes()[act_index];
		if(beh->realm == "activity") {

		    //
		    // start with the (unquoted) activity type name
		    //
		    Stream << beh->name << "=[";

		    //
		    // node that will hold style information for
		    // any activity parameter of type ARRAY
		    //
		    pinfoNode* act_type_node = NULL;

		    //
		    // Get the parameters of the constructor
		    // and list their name and data type as
		    // (quoted) keyword-value pairs
		    //
		    for(int tind = 0; tind < beh->tasks.size(); tind++) {
			task* T = beh->tasks[tind];
			if(T->name == "constructor") {
			    for(  int p_ind = 0;
				  p_ind < T->paramindex.size();
				  p_ind++) {
				int	var_ind = T->paramindex[p_ind];
				const	pair<Cstring, apgen::DATA_TYPE>& p
				    	    = T->get_varinfo()[var_ind];
				Cstring	p_name = p.first;
				apgen::DATA_TYPE t = p.second;

				if(p_ind > 0) {
				    Stream << ",";
				}
				Stream << addQuotes(p_name)
				       << "=" << addQuotes(apgen::spell(t));

				//
				// If t == apgen::DATA_TYPE::ARRAY, we
				// need to figure out whether the array
				// is of the LIST style or of the STRUCT
				// style, so that the tol2json converter
				// can do the right thing. This information
				// will be provided in the auxiliary TOL
				// file.
				//
				if(t == apgen::DATA_TYPE::ARRAY) {
				    if(!act_type_node) {
					act_type_node = new pinfoNode(beh->name);
				    }
				    act_type_node->payload << new arrayNode(
								p_name,
								flexval());
				}
			    }
			}
		    }
		    if(act_type_node) {
			param_info << act_type_node;
		    }
		    Stream << "],\n";
		}
	    }
	}

	//
	// Add resource metadata
	//
	Stream << "resource_metadata:\n";

	//
	// Tell publisher about the stream so it can write to it.
	// The publisher will write the resource metadata, the
	// $$EOH token, and then the body of the (XML)TOL.
	//
	tol_publisher::set_stream_to(Stream, fout, faux);
    }


    //
    // XMLTOL preliminaries. This includes paving the way for
    // metadata. The actual writing to file will occur when
    // eventPublisher::write_everything_you_got_up_to_this_point()
    // is invoked, from within filter_records() and
    // filter_xml_records() in TOL_iterate.C.
    //
    if(do_xmltol) {

#	ifdef have_xmltol

	document = new xmlpp::Document;
	xml_publisher::set_document_to(document);
	xmlpp::Element*	 root_node
		    = document->create_root_node("XML_TOL");
	xml_publisher::set_element_to(root_node);

	//
	// X.7 addition:
	//
	root_node->set_attribute("version", "V2.1");
    
	xmlpp::Element*	 metadata
		    = root_node->add_child_element("ResourceMetadata");
	xml_publisher::set_metadata_to(metadata);
#	endif /* have_xmltol */
    }


    //
    // Do the work
    //
    if(do_tol) {

	//
	// Write the TOL and, optionally, the XMLTOL
	//
	writeTOLbody<tol_publisher>(
				start,
				end,
				resources,
				all_acts_visible,
				optional_timesystem,
				/* write both = */ do_xmltol,
				param_info,
				Thread);
    } else if(do_xmltol) {

	//
	// Write the XMLTOL only
	//
#	ifdef have_xmltol
	writeTOLbody<xml_publisher>(
				start,
				end,
				resources,
				all_acts_visible,
				optional_timesystem,
				/* write both = */ false,
				param_info,
				Thread);
#	endif /* have_xmltol */
    }

    //
    // XMLTOL cleanup
    //
    if(do_xmltol) {
#	ifdef have_xmltol

	//
	// the publisher may have created new documents
	// during writing to file incrementally:
	//
	document = xml_publisher::theDocument;

	std::string xml_tol = document->write_to_string_formatted();
	const char* s = xml_tol.c_str();
	if(!xml_publisher::first_time_writing) {

		//
		// skip over the initial stuff until we find the <XML_TOL> element
		//
		while(*s && !xml_publisher::test_for_xml_tol_element(s)) {
			s++;
		}
		s += strlen("<XML_TOL>\n");
	}
	if(do_tol) {
		fwrite(s, strlen(s), 1, xml_fout);

		//
		// Do not forget to close the file. If the user
		// does a FAST QUIT, the code will _exit() which
		// does not flush file buffers.
		//
		fclose(xml_fout);
	} else {
		fwrite(s, strlen(s), 1, fout);
	}
	xml_publisher::cleanup();

#	endif /* have_xmltol */

    }

    //
    // TOL final write and cleanup
    //
    if(do_tol) {

	//
	// Flush Stream towards fout
	//
	Stream.write_to(fout);

	//
	// Fill faux with parameter information. We express the
	// data in a flexval and use flexval::to_json() to output.
	//
	flexval faux_stuff;

	for(    pinfoNode* act_type_node = param_info.first_node();
	        act_type_node;
	        act_type_node = act_type_node->next_node()) {

	    flexval act_stuff;
	    int	i_param = 0;

	    for(    arrayNode* aN = act_type_node->payload.first_node();
		    aN;
		    aN = aN->next_node()) {
		flexval one_param;
		if(aN->payload.getType() == flexval::TypeInvalid) {
		    one_param[*aN->Key.get_key()] = string("invalid");
		} else {
		    one_param[*aN->Key.get_key()] = aN->payload;
		}
		act_stuff[i_param]=one_param;
		i_param++;
	    }

	    faux_stuff[*act_type_node->Key.get_key()] = act_stuff;

	}

	json_object* stuff =  faux_stuff.to_json();
	const char* to_output = json_object_to_json_string_ext(
	    stuff, JSON_C_TO_STRING_PRETTY
	    );
	fwrite(to_output, strlen(to_output), 1, faux);
	json_object_put(stuff);

	fclose(faux);
    }

    fclose(fout);

    //
    // Make sure to communicate the fact that this thread is done
    //
    if(Thread) {
	    Thread->Done.store(true);
    }
}

tol_record*	tol_record::actStartFactory(ActivityInstance* act, long prio) {
	tol_record*     pr = new tol_record(act->getetime(), prio, NULL);
	pr->payload = new act_start_event(act);
	return pr;
}

tol_record*	tol_record::actEndFactory(ActivityInstance* act, long prio) {
	tol_record*     pr = new tol_record(act->getetime() + act->get_timespan(), prio, NULL);
	pr->payload = new act_end_event(act);
	return pr;
}

tol_record*	tol_record::resValueFactory(value_node* vn, long prio) {
	tol_record*     pr = new tol_record(vn->Key.getetime(), prio, NULL);
	pr->payload = new val_node_event(vn);
	return pr;
}

tol_record*	tol_record::violationFactory(con_violation* v, long prio) {
	 tol_record*     pr = new tol_record(v->getetime(), prio, NULL);
	 pr->payload = new vio_event(v);
	 return pr;
}

#include "TOL_iterate.C"

//
// This template writes the metadata and the body of
// a TOL (regular or XMLTOL). It was designed to handle
// two distinct use cases:
//
// 	- an isolated TOL output request; this is the
// 	  non-concurrent case
//
// 	- a TOL output request that runs in parallel with
// 	  a modeling thread; this is the concurrent case
//
// The value of Thread determines which case we are in: if it
// is NULL, we are in the non-concurrent case; otherwise, we
// are in the concurrent case.
//
// As of version X.6 of APGenX, the multithreaded version
// is no longer used. What is done instead is that the
// TOL and XMLTOL files are produced in parallel, after
// modeling has completed. The algorithm used in resource
// processing is much more efficient than before. Combined
// with the disappearance of the multithreading overhead
// imposed by parallel TOL output on the modeling process,
// the new process is almost as efficient as the previous
// approach, but much simpler in its implementation. In
// particular, handling output after modeling avoids the
// need for the addition of retroactive points, a major
// difficulty with the original approach.
//
template <class eventPublisher>
void
writeTOLbody(
	CTime_base		start,
	CTime_base		end,
	const vector<string>&	resources,
	bool			AllActsVisible,
	const Cstring&		timesystem,
	bool			write_both,
	pinfotlist&		param_info,
	thread_intfc*		Thread) {

    if(Thread) {
	if(eventPublisher::is_xml_publisher()) {
	    thread_index = thread_intfc::XMLTOL_THREAD;
	} else {
	    thread_index = thread_intfc::TOL_THREAD;
	}
    }

    /****************************************************
     *							*
     * PART I -- Activity Starts			*
     *							*
     ***************************************************/

    //
    // We need to include activity starts - for which we can
    // use the activities themselves - then the activity ends,
    // then all resource events, then all violations.
    //
    // Activity ends need to get top priority; this ensures
    // that ends of previous activities occur before activity
    // starts of new activities.
    //
    // However, there is one case where we want to override this
    // priority: activities of zero duration. We will watch out for
    // those in the code below.
    //
    // For activity ends, a multiterator is overkill since we have
    // them all in a single list. Ditto for violations. So, we use
    // a simple iterator in the iteration loop below.
    //

    int		priority = 1;

    status_aware_multiterator*  startIterator
			= eval_intfc::get_instance_multiterator(
			      priority,
			      /* include scheduled   = */ true,
			      /* include unscheduled = */ false);

    unique_ptr<status_aware_multiterator>
	    		act_iter_destroyer(startIterator);


    /****************************************************
     *							*
     * PART II -- Resource Histories			*
     *							*
     ***************************************************/

    try {
    	eventPublisher::handle_timesystem(timesystem);
    } catch(eval_error Err) {
	Cstring err;
	err << "Error in TOL output: time system \""
		<< timesystem << "\" does not exist";
	throw(eval_error(err));
    }

    tlist<alpha_string, Cnode0<alpha_string, Rsource*> >  all_included_res;

    Rsource::expand_list_of_resource_names(
			resources,
			all_included_res);

    //
    // NOTE: activities are only included if this is _not_ a
    // partial TOL. If there are any items in the resources list,
    // it is assumed that this is a partial TOL, even if the
    // list contains all resources.
    //
    bool	include_all_res_and_acts = resources.size() == 0;

    //
    // X.8.11 addition
    //
    if(include_all_res_and_acts && write_both) {

	//
	// Note that if this is an XMLTOL, we have already defined
	// the XML document. Therefore, it is safe to call the
	// global handler. For TOLs, this is a no-op call.
	//
	try {
	    TypedValue&     incon_globals = globalData::get_symbol("INCON_GLOBALS");

	    xml_publisher::handle_globals(&incon_globals);
	} catch(eval_error Err) {
	    // not an error
	}
    }

    //
    // Important topic: miterator initialization; in particular, how
    // to make sure that in its initial state, the miterator "sits"
    // at the start time of the TOL output request.
    //
    // In the concurrent case, this was handled "automatically" in
    // the sense that the modeling thread launched the output thread
    // just before 'now' was going to become greater than or equal to
    // the TOL request's start time. Note that the dual purpose
    // iterator's constructor below waits until the start_thread flag
    // is set by the modeling thread.
    //
    // In the non-concurrent case, the initialization process is
    // performed in TOL_write writeTOLbody().
    //
    // One final note: in the concurrent case, the dual_purpose_iterator
    // constructor automatically creates a signal_sender which waits
    // for the signal from the main thread.
    //

    //
    // Internally, the dual_purpose_iterator uses a res_curval miter
    // if Thread is NULL, and a res_trailing_miter otherwise. The
    // dual-purpose iterator API is the same for both.
    //
    dual_purpose_iterator theResIterator(Thread);

    Cnode0<alpha_string, Rsource*>*	cres;
    slist<alpha_string,
	Cnode0<alpha_string, Rsource*> >::iterator
					all_res(all_included_res);
    Rsource*				resource;

    //
    // Add all required resources to theResIterator.
    //
    // In doing so, we take advantage of the work we did in
    // RES_exec::consolidate to order resources in order of profile
    // dependency. We also take advantage of the iteration over
    // non-abstract resources to put out the relevant metadata.
    //
    // Note that adding lists to miterators affects the list, and is
    // therefore not thread-safe. Use a lock_guard accordingly.
    //
    // Note that the modeling thread is not adding threads to its
    // master miterator (potential_triggers), so the potential
    // conflicts only exist within trailing threads.
    //
    {
	lock_guard<mutex>	lock(dual_purpose_iterator::Mutex());

	//
	// XMLTOL resource metadata do not worry about containers,
	// because the container name is built into the schema
	// for individual resources. So it's OK to iterate over
	// individual resources.
	//
	// TOL metadata are best organized by container; the list
	// of indices for each resource is just handled as a string.
	// Note that if we want TOLs to be "partial" just like an
	// XMLTOL can be, then we may not want to include all the
	// resources within a container. This makes the programming
	// a little tricky but so be it.
	//

	int	outputIndex = 0;
	while((cres = all_res())) {
	    resource = cres->payload;
	    resource->output_index[thread_index] = outputIndex++;

	    //
	    // we need the dynamic casting this because the base history class returned
	    // by get_history() does not have an "addToIterator()" method
	    //
	    RES_state*		state_res = dynamic_cast<RES_state*>(resource);
	    RES_settable*	settable_res = dynamic_cast<RES_settable*>(resource);
	    RES_numeric*	numeric_res = dynamic_cast<RES_numeric*>(resource);

	    //
	    // we must include ALL resources 
	    //
	    if(state_res) {
		state_res->get_history().addToIterator(
		    *theResIterator,
		    2 + resource->evaluation_priority,
		    /* store = */ true);
	    } else if(numeric_res) {
		numeric_res->get_history().addToIterator(
		    *theResIterator,
		    2 + resource->evaluation_priority,
		    /* store = */ true);
	    } else if(settable_res) {
		settable_res->get_history().addToIterator(
		    *theResIterator,
		    2 + resource->evaluation_priority,
		    /* store = */ true);
	    }

	    //
	    // handle metadata
	    //
	    if(include_all_res_and_acts || all_included_res.find(resource->name)) {
		eventPublisher	  eP(resource);

		//
		// Temporal bounds don't matter for resource metadata
		//
		eP.write_to_stream(TOL_REGULAR, NULL);
		if(write_both) {
		    xml_publisher xP(eP);
		    xP.write_to_stream(TOL_REGULAR, NULL);
		}
	    }
	}
    }

    eventPublisher::write_end_of_metadata();

    //
    // initialize the template-based iterator
    //
    theResIterator.initialize_at(start);


    /****************************************************
     *							*
     * PART III -- Constraint Violations		*
     *							*
     ***************************************************/

    //
    // Nothing to do - the iterate function will track
    // the vioNode inside the Kit; by design, it is safe
    // for the (XML)TOL threads to use.
    //

    //
    // We have finished constructing and initializing
    // all the iterators we need.
    //

    TOL_kit	theKit(
			start,
			end,
			AllActsVisible,
			include_all_res_and_acts,
			all_included_res,
			theResIterator,
			startIterator,
			write_both,
			param_info);

    /****************************************************
     *							*
     * PART IV -- Iterate over Events			*
     *							*
     * Note: this works both for single- and		*
     * multi-threaded contexts. The thread_intfc	*
     * object is available from theResIterator.		*
     *							*
     ****************************************************/

    try {
	iterateOverTOLevents<eventPublisher>(theKit);
    }
    catch(eval_error Err) {
	Cstring errors;
	errors << "write TOL body: got error --\n" << Err.msg << "\n";
	throw(eval_error(errors));
    }
}


Cstring get_tol_header_info(
		const Cstring& tol_file_name,
		bool xml) {
    aoString			headerStream;
    char*			userName = getenv("LOGNAME");
    Cstring			user = "UNKNOWN";
    struct timeval		currentTime;
    struct timezone		currentZone;
    char			hostname[1024];//should be large enough
    stringslist::iterator	fileNameListIterator(eval_intfc::ListOfAllFileNames());
    emptySymbol*		fileNamePtr = NULL;

    if(xml) {
	headerStream << "<ApgenVersionInfo>\n"
		<< "    <ApgenTitle>"
		<< fix_for_XML(get_apgen_version_build_platform())
		<< "</ApgenTitle>\n";
    }

    if(xml) {
	headerStream << "    <udef_library>"
		<< fix_for_XML(udef_intfc::get_user_lib_version())
		<< "</udef_library>\n";
    } else {
	headerStream << "udef=" << udef_intfc::get_user_lib_version() << "\n";
    }

    if(userName) {
	user = userName;
    }
    if(xml) {
	headerStream << "    <user>" << fix_for_XML(user) << "</user>\n";
    } else {
	headerStream << "user=" << user << "\n";
    }

    gettimeofday(&currentTime, &currentZone);
    CTime_base writeTime(currentTime.tv_sec, 0, false);
    if(xml) {
	headerStream << "    <CreationDate>"
		<< writeTime.to_string() << "</CreationDate>\n";
    } else {
	headerStream << "date="
		<< writeTime.to_string() << "\n";
    }

    hostname[0] = 0;
    gethostname(hostname, sizeof(hostname));
    if(xml) {
	headerStream << "    <hostname>"
		<< fix_for_XML(hostname) << "</hostname>\n";
    } else {
	headerStream << "hostname=" << hostname;
    }

    if(xml) {
	headerStream << "    <apgen_files number=\""
		<< eval_intfc::ListOfAllFileNames().get_length()
		<< "\">\n";
    } else {
	headerStream << "\nn_apgen_files="
		<< eval_intfc::ListOfAllFileNames().get_length();
    }

    while((fileNamePtr = fileNameListIterator())) {
	Cstring		fileName = fileNamePtr->get_key();
	struct stat	fileStats;

	if(!xml) {
	    headerStream << "\n";
	    headerStream << "apgen_file=" << fileName;
	}

	if(stat(*fileName, &fileStats) >= 0) {
	    CTime_base	fileModTime(fileStats.st_mtime, 0, false);
	    if(xml) {
		headerStream << "        <apgen_file mod_time=\""
			<< fileModTime.to_string()
			<< "\">" << fix_for_XML(fileName)
			<< "</apgen_file>\n";
	    } else {
		headerStream << " " << fileModTime.to_string();
	    }
	} else if(xml) {
	    headerStream << "        <apgen_file>"
		    << fix_for_XML(fileName) << "</apgen_file>\n";
	}
    }
    if(xml) {
	headerStream << "    </apgen_files>\n"
		<< "</ApgenVersionInfo>\n";
    } else {
	headerStream << "\n";
    }
    return headerStream.str();
}
