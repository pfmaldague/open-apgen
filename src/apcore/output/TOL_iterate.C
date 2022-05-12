template <class eventPublisher>
std::atomic<vector<eventPublisher>*>&  records_to_filter(int I) {
	static std::atomic<vector<eventPublisher>*> V[10];
	return V[I];
}

std::atomic<bool>& no_more_work(int I) {
	static std::atomic<bool> B[10];
	return B[I];
}

std::atomic<vector<xml_publisher>*>&	data_for_xml_filtering() {
	static std::atomic<vector<xml_publisher>*> X;
	return X;
}

std::atomic<bool>& no_more_xml_work() {
	static std::atomic<bool> B;
	return B;
}

void filter_xml_records(
		CTime_base		start,
		CTime_base		end) {

    //
    // Define time bounds based on start, end times
    //
    CTime_base*	bounds = new CTime_base[2];
    bounds[0] = start;
    bounds[1] = end;
    vector<xml_publisher>* vec_ptr;

    while(true) {
	while((vec_ptr = data_for_xml_filtering().load()) == NULL) {
	    if(no_more_xml_work().load()) {
		delete [] bounds;
		return;
	    }
	    this_thread::sleep_for(chrono::milliseconds(1));
	}
	for(int i = 0; i < vec_ptr->size(); i++) {
	    (*vec_ptr)[i].write_to_stream(TOL_REGULAR, bounds);
	}
	xml_publisher::write_everything_you_got_up_to_this_point();
	delete vec_ptr;
	data_for_xml_filtering().store(NULL);
    }
}

template <class eventPublisher>
void filter_records(
		int			tIndex,
		int			parent_index,
		CTime_base		start,
		CTime_base		end,
    		vector<res_record2>*	resource_values) {

    //
    // Define time bounds based on start, end times
    //
    CTime_base*	bounds = new CTime_base[2];
    bounds[0] = start;
    bounds[1] = end;

    thread_index = tIndex;
    vector<eventPublisher>* vec_ptr;
    tlist<alpha_void, res_record>	resources_taken_care_of;
    Rsource*				resource;
    CTime_base				etime;
    CTime_base				prev_time;
    bool				first_time = true;
    vector<res_record2>&		previous_resource_values = *resource_values;
    vector<int>				last_index(previous_resource_values.size(), -1);
    std::thread				xmlWriter;
    vector<xml_publisher>*		xmlvec = NULL;

    //
    // Launch the XMLTOL writing thread if necessary
    //
    if(eventPublisher::dual_flag) {
	data_for_xml_filtering().store(NULL);
	no_more_xml_work().store(false);
	xmlWriter = std::thread(filter_xml_records,
		start,
		end);
    }
    while(true) {
	while((vec_ptr = records_to_filter<eventPublisher>(tIndex).load()) == NULL) {
	    if(no_more_work(tIndex).load()) {
		if(eventPublisher::dual_flag) {

		    //
		    // Wait for the xml thread to finish and join it
		    //
		    while(data_for_xml_filtering().load() != NULL) {
			this_thread::sleep_for(chrono::milliseconds(1));
		    }
		    no_more_xml_work().store(true);
		    if(xmlWriter.joinable()) {
			xmlWriter.join();
		    }
		}
		delete [] bounds;
		return;
	    }
	    this_thread::sleep_for(chrono::milliseconds(1));
	}
	vector<eventPublisher>& vec = *vec_ptr;
	vector<bool>		disabled(vec.size(), false);

	//
	// First run through - identify repeated interpolated values
	//
	for(int i = 0; i < vec.size(); i++) {
	    if(vec[i].type == TOL_RES_VAL) {
		resource = vec[i].Rnode;
		bool interpolated = resource->properties[
			(int)Rsource::Property::has_interpolation];
		bool no_filtering = resource->properties[
			(int)Rsource::Property::has_nofiltering];
		int resindex = resource->output_index[parent_index];
		res_record2& rec = previous_resource_values[resindex];
		if(interpolated && !no_filtering) {
		    if(rec.selected) {
			if(rec.val.is_equal_to(vec[i].Val, resource)) {
			    disabled[i] = true;
			} else {

			    //
			    // Reenable last disabled index
			    //
			    disabled[rec.last_index] = false;
			}
		    } else {
			rec.selected = true;
		    }
		    rec.val = vec[i].Val;
		    rec.last_index = i;
		}
	    }
	}

	if(eventPublisher::dual_flag) {
	    xmlvec = new vector<xml_publisher>;
	}

	//
	// Publishing run
	//
	for(int i = 0; i < vec.size(); i++) {
	    if(vec[i].type == TOL_RES_VAL) {
		etime = vec[i].getetime();
		resource = vec[i].Rnode;
		int resindex = resource->output_index[parent_index];
		res_record2& rec = previous_resource_values[resindex];
		bool interpolated = resource->properties[
			(int)Rsource::Property::has_interpolation];
		if(interpolated) {
		    if(disabled[i] && i != rec.last_index) {

			//
			// There are repeated values before and after this one
			//
			;
		    } else {
			vec[i].write_to_stream(TOL_REGULAR, bounds);
			if(eventPublisher::dual_flag) {
			    xml_publisher xp(vec[i]);
			    xmlvec->push_back(xp);
			}
		    }
		} else {
		    if(rec.selected) {

			//
			// do some filtering
			//
			if(rec.val.is_equal_to(vec[i].Val, resource)) {

			    //
			    // do not update previous resource values
			    //
			    ;
			} else {
			    vec[i].write_to_stream(TOL_REGULAR, bounds);
			    if(eventPublisher::dual_flag) {
				xml_publisher xp(vec[i]);
				xmlvec->push_back(xp);
			    }
			    rec.val = vec[i].Val;
			}
		    } else {
			rec.selected = true;
			rec.val = vec[i].Val;
			vec[i].write_to_stream(TOL_REGULAR, bounds);
			if(eventPublisher::dual_flag) {
			    xml_publisher xp(vec[i]);
			    xmlvec->push_back(xp);
			}
		    } // if value is different from previous
		} // if not interpolated
	    } else {
		vec[i].write_to_stream(TOL_REGULAR, bounds);
		if(eventPublisher::dual_flag) {
		    xml_publisher xp(vec[i]);
		    xmlvec->push_back(xp);
		}
	    } // if not a resource publisher
	} // for each publisher

	if(eventPublisher::dual_flag) {

	    //
	    // Wait for the xml filter to finish its work
	    //
	    while(data_for_xml_filtering().load() != NULL) {
		this_thread::sleep_for(chrono::milliseconds(1));
	    }

	    //
	    // xml filter will delete:
	    //
	    data_for_xml_filtering().store(xmlvec);
	}

	//
	// Re-initialize previous values
	//
	for(int i = 0; i < previous_resource_values.size(); i++) {
	    previous_resource_values[i].selected = false;
	}

	eventPublisher::write_everything_you_got_up_to_this_point();
	delete vec_ptr;
	records_to_filter<eventPublisher>(tIndex).store(NULL);
    }
}

//
// This function template can be called in two contexts:
//
// 	- in single-threaded mode, in which case it's just an ordinary
// 	  function with ordinary Miterators
//
// 	- in parallel mode, in which case it has been provided to
// 	  std::thread as the "main" function for a new thread.
//
// The function is provided a dual-purpose Miterator, which is the
// theResIterator member of the TOL_kit argument. In single-threaded
// mode, the internal Miterator is a res_curval_miter; in parallel
// mode, it is a trailing_miter which is trickier to use. The tricky
// part is handled by the dual_purpose_iterator interface, which
// handles both cases transparently through a common API.
//
template <class eventPublisher>
void
iterateOverTOLevents(
	TOL_kit&	K) {
    dual_purpose_iterator&	tol_miter = K.theResIterator;

    //
    // Will be NULL if this TOLrequest is standalone (not parallel):
    //
    thread_intfc*		Thread = tol_miter.Thread;

    //
    // Instruct the eventPublisher to publish 2 files
    // (in which case eventPublisher is in fact a
    // tol_publisher)
    //
    eventPublisher::set_dual_flag_to(K.WriteBothFiles);
    eventPublisher::all_acts_visible = K.AllActsVisible;

    //
    // Will also be NULL if we are not running in a parallel thread:
    //
    trailing_miter* trailer
			= dynamic_cast<trailing_miter*>(tol_miter.MiterPtr.object());
    thread_intfc& modeling_thread
			= *thread_intfc::threads()[thread_intfc::MODEL_THREAD].get();

    CTime_base			lastProcessedEventTime;
    CTime_base			prev_time;
    bool			thereWasAtLeastOneEvent = false;
    tlist<alpha_string, Cntnr<alpha_string, ActivityInstance*> > pendingActivityEnds;
    tlist<alpha_string, Cntnr<alpha_string, ActivityInstance*> > endlessActivityStarts;
    Rsource*			resource;
    int				record_count = 0;
    Cntnr<alpha_string, ActivityInstance*>* ptr;

    /****************************************
     *				            *
     * These are the events we track:       *
     *				            *
     ***************************************/

    ActivityInstance*		request;
    Time_node*			actStartEvent;
    time_actptr*		actEndEvent = NULL;

    value_node*			resEvent;
    value_node*			prev_res_event = NULL;
    con_violation*		vio;
    Cntnr<alpha_int, con_violation*>*
				active_con = NULL;
    tlist<alpha_int, Cntnr<alpha_int, con_violation*> >
	   			active_violations(true);
    tlist<alpha_time, con_violation>
	   			unreleased_violations(true);
    tlist<alpha_time, con_violation>
	   			earlier_violations(true);

    //
    // Important auxiliary data
    //

    Rsource::iterator	iter;
    int			unique_res_index = 0;
    vector<bool>	resource_selection;
    while((resource = iter.next())) {
	resource->output_index[thread_index] = unique_res_index++;
	if(K.include_all_res_and_acts || K.all_included_res.find(resource->name)) {
		resource_selection.push_back(true);
	} else {
		resource_selection.push_back(false);
	}
    }
    int total_num_res = unique_res_index;

    //
    // Only contains a few records:
    //
    tlist<alpha_void, res_record> resources_taken_care_of;

    //
    // Contains a lot of records (for a full TOL):
    //
    vector<res_record2>	previous_resource_values(total_num_res);

    //
    // now we create a list that is analogous to the list of tol_threads
    // inside a Miterator. We will mimic the logic of a Miterator based
    // on lists of tol_records. We do this instead of using a
    // Miterator<tol_record> because we don't want to copy zillions of
    // objects (potentially) into a large list of tol_records.
    //
    tlist<prio_time, tol_record>    theTOLlist(true);


    //
    // PART IV(a) -- add activity starts and ends
    //

    if(K.include_all_res_and_acts) {
	if((actStartEvent = K.startIterator->next())) {
	    assert(actStartEvent->node_type() == ACT_REQ);
	    request = (ActivityInstance*) actStartEvent;
	    theTOLlist << tol_record::actStartFactory(request, 1);
	}
	if((actEndEvent = ACT_exec::ACT_subsystem().ActivityEnds.first_node())) {
	    request = actEndEvent->BP;
	    theTOLlist << tol_record::actEndFactory(request, 0);
	}
    }

    //
    // PART IV(b) -- add resources
    //

    //
    // Note: the only way tol_miter->next() would return NULL
    // is if there are no more nodes any any resource history
    // after the requested start time of this TOL request.
    //
    if((resEvent = tol_miter->peek())) {

	//
	// This will cause the loop below to output the resource
	// event:
	//
	theTOLlist << tol_record::resValueFactory(resEvent, 2);

	//
	// We do this now, because we don't want to report this
	// event again:
	//
	tol_miter->next();
	prev_res_event = resEvent;
    }


    //
    // PART IV(c) -- add constraint violations
    //

    CTime_base	lastResourceReportTime;

    //
    // Report the initial value of all resources. If K.start is
    // the earliest time in the plan, this will be done automatically.
    //
    // Note that we do not publish anything inside
    // handle_a_resource_event(); we merely publish
    // the resources_taken_care_of list.
    //
    tol_record*			theEvent = theTOLlist.first_node();

    if(theEvent && theEvent->Key.getetime() < K.start) {

	//
	// If output is proceeding in parallel, we need to wait until
	// the modeling loop has finished processing everything up
	// to and including K.start
	//
	if(Thread) {
	    while(true) {
		CTime_base mtime = thread_intfc::current_time(trailer->the_thread_we_trail);
		if(mtime > K.start) {

		    //
		    // OK, it's safe to query resources up to K.start now
		    //
		    break;
		}
		if(modeling_thread.Done.load()) {

		    //
		    // modeling is done - it can't hurt us!
		    //
		    break;
		}
		thread_intfc::sleep_for_ms(1);
	    }
	}

	//
	// We advance the resource multiterator until peek() returns
	// something beyond K.start. This makes sure that the
	// resource iterators are all pointed at nodes that are
	// as close as possible to the start of the TOL.
	//
	while((resEvent = tol_miter->peek())) {
	    if(resEvent->Key.getetime() <= K.start) {
		tol_miter->next();
	    } else {
		break;
	    }
	}

	//
	// Set things up so currentval evaluation works:
	//
	thread_intfc::update_current_time(K.start);

	//
	// Set things up so 'now' has the correct value:
	//
	time_saver::set_now_to(K.start);

	//
	// We now populate the "resources_taken_care_of" array
	// with the initial values of the resources.
	//
	while((resource = iter.next())) {
	    if(resource_selection[resource->output_index[thread_index]]) {
		value_node* vn = tol_miter->current(resource);

		//
		// We need to check because it's conceivable that
		// modeling never took place
		//
		if(vn) {
		    handle_a_resource_event(
			*vn,
			K.start,
			resource,
			resources_taken_care_of,
			resource_selection);
		}
	    }
	}

	//
	// publish, if anything was found
	//
	slist<alpha_void, res_record>::iterator iter2(resources_taken_care_of);
	res_record*	resrec;
	while((resrec = iter2())) {
	    resource = resrec->payload.first;

	    eventPublisher ep(resource->name,
				resrec->payload.second,
				K.start,
				resource);
	    ep.write_to_stream(TOL_REGULAR, NULL);
	    if(K.WriteBothFiles) {
		xml_publisher xp(ep);
		xp.write_to_stream(TOL_REGULAR, NULL);
	    }
	}
	lastResourceReportTime = K.start;
    }

    bool violations_need_to_be_included = K.include_all_res_and_acts;

    if(violations_need_to_be_included) {

	//
	// We may not have found any violations by the
	// time this code is run. What we need to do
	// is keep track of the current node in the
	// violations list.
	//
	if(   !K.next_vio_to_process // we have processed the vio in theTOLlist
	      && Constraint::last_safe_node != K.last_processed_vio) {
	    tol_record* con_event = NULL;
	    vio = K.last_processed_vio;

	    if(vio) {
		K.next_vio_to_process = vio->next_node();
	    } else {
		K.next_vio_to_process = Constraint::violations().first_node();
	    }
	    vio = K.next_vio_to_process;
	    assert(vio);
	    theTOLlist << (con_event = tol_record::violationFactory(vio, 3));

	    //
	    // AP-1484: we need to keep track of unreleased violations.
	    //
	    ForbiddenCondition*	fc
		    = dynamic_cast<ForbiddenCondition*>(
			vio->payload.parent_constraint);
	    if(fc) {
		if(vio->payload.severity == Violation_info::RELEASE) {
		    active_con = active_violations.find(vio->ID);
		    if(active_con) {
			delete active_con;
		    }
		} else {
		    active_violations
			<< new Cntnr<alpha_int, con_violation*>
				(vio->ID, vio);
		}
	    }

	    if(con_event->Key.getetime() < theEvent->Key.getetime()) {
		theEvent = con_event;
	    }
	}
    }

    //
    // In all cases, the most important constraint on the
    // loop is not to repeat resource values already reported
    // on. Note that this version of (XML)TOL output does
    // _not_ report any transients, as per agreement with
    // Eric Ferguson.
    //
    // It turns out that filtering out repeated values takes
    // time. We therefore handle that in a separate thread
    // to which we feed vectors of publishers.
    //
    vector<eventPublisher>*	publish_vec = new vector<eventPublisher>;
    std::thread	Filter;
    int thread_index_to_use = thread_index + 2;

    //
    // The thread will wait for a non-NULL records_to_filter,
    // which it will grab and then set to NULL.
    //
    records_to_filter<eventPublisher>(thread_index_to_use).store(NULL);
    no_more_work(thread_index_to_use).store(false);
    Filter = std::thread(
			filter_records<eventPublisher>,
			thread_index_to_use,
			thread_index,
			K.start,
			K.end,
    			&previous_resource_values);

    //
    // Do this when you have a full vector:
    //
    // records_to_filter(thread_index_to_use).store(publish_vec);

    bool we_have_passed_the_start_time = false;

    while((theEvent = theTOLlist.first_node())) {

	CTime_base etime = theEvent->Key.getetime();

	//
	// Watch out: prev_time is initialized to 1970...
	//
	assert(etime >= prev_time);

	//
	// End time processing. There are three possibilities:
	//
	//	1. the last event occurs strictly before K.end
	//
	// 	2. an event has a tag exactly equal to K.end
	//
	// 	3. there is an event strictly before K.end and
	// 	   another event strictly after K.end
	//
	// In case 1., we just fall off the loop. The post-
	// processing code has to deal with lastResourceReportTime
	// being behind lastProcessedEventTime and report values at
	// lastProcessedEventTime. Then the code has to issue a report
	// _again_ at K.end.
	// 
	// In case 2., we go on and process the event as
	// usual. The post-processing code has to again deal
	// with lastResourceReportTime being behind lastProcessedEventTime.
	// However, lastProcessedEventTime will now be equal to K.end,
	// so a last recap report will not be necessary.
	//
	// Case 3. must be handled the same way as case 1.
	//

	//
	// Make sure lastProcessedEventTime is updated; we'll need it
	// after we exit from the loop over events
	//
	if(etime > K.end) {
	    break;
	}

	lastProcessedEventTime = etime;

	//
	// Figure out whether event time has met our
	// minimum requirement for output. This criterion
	// is _not_ the same for all records: resource
	// values have already been reported on at the
	// beginning of the (XML)TOL. That makes them
	// special.
	//

	//
	// For records other than resources:
	//
	bool    produce_output = etime >= K.start;

	//
	// for resource records:
	//
	bool    produce_res_output = produce_output && prev_time > lastResourceReportTime;

	//
	// incremental output: we print to file every 50000 records,
	// so we don't accumulate too much data in RAM
	//
	if(produce_output) {
	    if(!we_have_passed_the_start_time) {
		we_have_passed_the_start_time = true;

		//
		// Handle constraint violations that occurred
		// prior to the start of the TOL horizon
		//
		for(  active_con = active_violations.first_node();
		      active_con;
		      active_con = active_con->next_node()) {
		    con_violation*	earlier;
		    vio = active_con->payload;
		    earlier_violations << (earlier = new con_violation(
					K.start,
					vio->payload.parent_constraint,
					vio->payload.message,
					vio->payload.severity));
		
		    eventPublisher ep(earlier);
		    publish_vec->push_back(ep);
		}
	    }

	    if(record_count > 50000) {

		//
		// Wait till the filtering thread is done with its current data
		//
		while(records_to_filter<eventPublisher>(
			thread_index_to_use).load() != NULL) {
		    thread_intfc::sleep_for_ms(1);
		}

		//
		// The filtering thread will delete publish_vec when done
		//
		records_to_filter<eventPublisher>(thread_index_to_use).store(publish_vec);
		publish_vec = new vector<eventPublisher>;

		record_count = 0;
	    }
	}

	if(etime > prev_time) {

	    //
	    // First take care of resource publication, if appropriate
	    //
	    if(produce_res_output) {

		//
		// publish, if anything was found.
		//
		slist<alpha_void, res_record>::iterator
				iter2(resources_taken_care_of);
		res_record* resrec;
		while((resrec = iter2())) {
		    resource = resrec->payload.first;
		    eventPublisher ep(resource->name,
				resrec->payload.second,
				prev_time,
				resource);
		    publish_vec->push_back(ep);
		}
		lastResourceReportTime = prev_time;
	    }

	    prev_time = etime;

	    //
	    // Update the set of previous values. All the resources
	    // in resources_taken_care_of have just had their value
	    // updated with a time tag of etime. Let's delete their
	    // old representatives in previous_resource_values...
	    // ... and then insert their new values into
	    // previous_resource_values.
	    //
	    resources_taken_care_of.clear();
	}

	if(theEvent->payload->eType == tolEventPLD::ACT_START) {
	    if(produce_output) {
		    request = theEvent->payload->get_instance();

		    record_count++;

		    //
		    // NOTE for TOL only:
		    // -----------------
		    //
		    // To support tol2json, it is necessary to analyze
		    // parameters whose type is ARRAY and figure out
		    // whether the style is LIST or STRUCT.
		    //
		    // We should take advantage of activity type info
		    // already produced when creating the TOL header.
		    //
		    eventPublisher ep(request,
				etime,
				TOL_ACT_START,
				K.parameter_info);
		    publish_vec->push_back(ep);
	    }

	    //
	    // keep track of ends, put them here if zero duration act
	    //
	    delete theEvent;

	    //
	    // next, check whether there is a pending end statement to
	    // append here - but only if we are producing output
	    //
	    if(produce_output) {
		if((ptr = pendingActivityEnds.find(request->get_unique_id()))) {
		    eventPublisher ep(
				ptr->payload,
				ptr->payload->getetime(),
				TOL_ACT_END,
				K.parameter_info);
		    publish_vec->push_back(ep);
		    delete ptr;
		} else {
		    endlessActivityStarts <<
			new Cntnr<alpha_string, ActivityInstance*>(
					request->get_unique_id(),
					request);
		}
	    }

	    if((actStartEvent = K.startIterator->next())) {
		assert(actStartEvent->node_type() == ACT_REQ);
		request = (ActivityInstance*) actStartEvent;
		if(request->getetime() < prev_time) {
		    cerr << prev_time.to_string()
			    << " - (TOL) OUT OF ORDER act start event: "
			    << request->identify() << " @ "
			    << request->getetime().to_string()
			    << "\n";
		} else {
		    tol_record* act_event;
		    theTOLlist << (act_event = tol_record::actStartFactory(request, 1));
		}
	    }
	} else if(theEvent->payload->eType == tolEventPLD::CONS_VIO) {
	    ForbiddenCondition*	fc;

	    vio = theEvent->payload->get_violation();

	    //
	    // AP-1484: we need to keep track of unreleased violations.
	    //
	    fc = dynamic_cast<ForbiddenCondition*>(
			vio->payload.parent_constraint);
	    if(fc) {
		if(vio->payload.severity == Violation_info::RELEASE) {
		    active_con = active_violations.find(vio->ID);
		    if(active_con) {
			delete active_con;
		    }
		} else {
		    active_violations << new Cntnr<alpha_int, con_violation*>
							(vio->ID, vio);
		}
	    }

	    if(produce_output) {
		record_count++;
		eventPublisher ep(vio);
		publish_vec->push_back(ep);
	    } else {
	    }

	    K.last_processed_vio = vio;
	    delete theEvent;

	    //
	    // Note: here we are relying on the fact that Constraint::violations
	    // is a tlist whose linked list order agrees with the btree order,
	    // and also on the fact that the modeling thread does not add violations
	    // in the past.
	    //
	    // However, we should still check that we are safely behind the current
	    // modeling time.
	    //
	    if(vio != Constraint::last_safe_node.load()) {
		vio = vio->next_node();
		K.next_vio_to_process = vio;
		tol_record* con_event;
		theTOLlist << (con_event = tol_record::violationFactory(vio, 3));
		if(theTOLlist.get_length() > 4) {
		    cerr << "got over 4 error\n";
		}
		assert(con_event->Key.getetime() >= prev_time);
	    } else {

		//
		// We have processed all the violations in the
		// current batch
		K.next_vio_to_process = NULL;
	    }
	} else if(theEvent->payload->eType == tolEventPLD::VALUE_NODE) {

	    //
	    // a little more complicated - how do we get the resource?
	    // Answ: through the history
	    //
	    value_node*     vn = theEvent->payload->get_value_node();

	    if(produce_output) {
		record_count++;

		//
		// Set things up so currentval evaluation works:
		//
		thread_intfc::update_current_time(etime);

		//
		// Set things up so 'now' has the correct value:
		//
		time_saver::set_now_to(etime);

		//
		// This call does not publish anything:
		//
		handle_a_resource_event(
			*vn,
			etime,
			vn->list->Owner,
			resources_taken_care_of,
			resource_selection);

	    }

	    delete theEvent;

	    //
	    // Remember, peek() is a look at what the Miterator
	    // would return next. In general, the returned node
	    // lies ahead of the modeling thread's safe time.
	    // We want to be no later than the safe time.
	    //
	    if(  (resEvent = tol_miter.peek())
		  && (!Thread
		      || resEvent->Key.getetime() <= CTime_base(modeling_thread.SafeTime.load())
		     )
	      ) {

		assert(resEvent->Key.getetime() >= prev_res_event->Key.getetime());

		//
		// Advance the Miterator
		//
	    	tol_miter->next();
		tol_record* res_event;
	        theTOLlist << (res_event = tol_record::resValueFactory(resEvent, 2));
		assert(res_event->Key.getetime() >= prev_time);
		prev_res_event = resEvent;
	    }
#	    ifdef DEBUG_THIS
	    else {
		excl_str << etime.to_string() << " - No resource events left!\n";
		excl << excl_str;
		excl_str.undefine();
	    }
#	    endif /* DEBUG_THIS */

	}

// #	ifdef SUPPRESS_ACTIVITY_END_PROCESSING

	else if(theEvent->payload->eType == tolEventPLD::ACT_END) {

#	    ifdef DEBUG_THIS
	    excl << "(TOL)  case ACT_END\n";
#	    endif /* DEBUG_THIS */
	    if(produce_output) {
		record_count++;
		request = theEvent->payload->get_instance();
		if((!request->is_unscheduled()) && (!request->is_on_clipboard())) {
		    if((ptr = endlessActivityStarts.find(request->get_unique_id()))) {
			if(produce_output) {
			    eventPublisher ep(
				    	request,
					request->getetime(),
					TOL_ACT_END,
					K.parameter_info);
			    publish_vec->push_back(ep);
			}
			delete ptr;
		    } else {
			pendingActivityEnds <<
			    new Cntnr<alpha_string, ActivityInstance*>(
				      request->get_unique_id(),
				      request);
		    }
		}
	    }
    	    time_actptr* new_actevent = actEndEvent->next_node();

	    delete theEvent;

	    if((actEndEvent = new_actevent)) {
		request = actEndEvent->BP;
		tol_record* act_event = tol_record::actEndFactory(request, 0);
		if(act_event->Key.getetime() >= prev_time) {
		    theTOLlist << act_event;
		} else {
		    delete act_event;
		}
	    }
	}

// #	endif /* SUPPRESS_ACTIVITY_END_PROCESSING */

	else {
	    assert(false);
	}
	if(produce_output) {
	    thereWasAtLeastOneEvent = true;
	}

	if(Thread) {

	    //
	    // We have reached the end of the loop. Let us see
	    // what kind of events are in theTOLlist. If all
	    // possible sources of events are represented, we
	    // don't need to worry; processing the earliest of
	    // them is not going to lead to problems. But if
	    // some of the sources are not represented, then
	    // we need to make sure that the processing time
	    // for those sources is at least equal to the next
	    // event time in the list.
	    //
	    bool search_for_res_event = true;
	    bool search_for_vio_node = true;
	    bool we_have_a_projected_next_time = false;
	    CTime_base projected_next_time;
	    for(tol_record* rec = theTOLlist.first_node();
		rec != NULL;
		rec = rec->next_node()) {
		act_start_event*	start_node = dynamic_cast<act_start_event*>(
			    			rec->payload);
		act_end_event*	end_node = dynamic_cast<act_end_event*>(
			    			rec->payload);
		val_node_event*	res_node = dynamic_cast<val_node_event*>(
			    			rec->payload);
		vio_event*		vio_node = dynamic_cast<vio_event*>(
			    			rec->payload);
		if(res_node) {
		    search_for_res_event = false;
		} else if(vio_node) {
		    search_for_vio_node = false;
		}
		if(!we_have_a_projected_next_time) {
		    we_have_a_projected_next_time = true;
		    projected_next_time = rec->Key.getetime();
		} else if(rec->Key.getetime() < projected_next_time) {
		    projected_next_time = rec->Key.getetime();
		}
	    }

	    if(!we_have_a_projected_next_time) {

		//
		// Let's try resource events - wait until either
		// some event shows up, or we are done.
		//
		// Actually, the only way that could be true is
		// if the modeling loop was done, because 
		// tol_miter.peek() does return an event if at
		// all possible - meaning it's already waited
		// until something - anything - could be gotten
		// from the event loop.
		//
		// In this case, all we can do is wait for the
		// constraint-checking thread to continue until
		// either it ends without violations, or it does
		// produce some violation. But we have already
		// done that: if there are any constraints, the
		// tol_miter has waited until the constraint-checking
		// thread exited before returning NULL for peek().
		//
		// In short, we have tried everything we could to
		// get an additional node to report on, and we failed.
		// We don't need to do anything. The loop will exit.
		//
		;

	    } else {

		//
		// We need to wait until either some violation
		// shows up before the projected next_time, or
		// there are no such violations and we can
		// safely continue iterating without a violation
		// node in the list
		//
		thread_intfc& our_master
			= *thread_intfc::threads()[trailer->the_thread_we_trail].get();

		//
		// The constraint-checking thread has detected
		// no new violations. We must let it go far
		// enough to make sure it has found all the
		// violations up to the projected next time.
		//
		while(  !our_master.Done.load()
			&& CTime_base(our_master.SafeTime.load()) < projected_next_time) {
		    thread_intfc::sleep_for_ms(1);
		}
		assert(CTime_base(modeling_thread.SafeTime.load()) >= projected_next_time);

		//
		// Now we are ready to look at any violations found.
		// Note that Constraint::last_safe_node will have
		// been updated to any violations found.
		//

		if(	search_for_vio_node
			&& violations_need_to_be_included
			&& Constraint::last_safe_node != K.last_processed_vio) {

		    tol_record* con_event = NULL;
		    con_violation* con = K.last_processed_vio;

		    if(con) {
			K.next_vio_to_process = con->next_node();
		    } else {
			K.next_vio_to_process = Constraint::violations().first_node();
		    }
		    assert(K.next_vio_to_process);
		    theTOLlist << (con_event = tol_record::violationFactory(
						K.next_vio_to_process, 3));
		    if(theTOLlist.get_length() > 4) {
			cerr << "got over 4 error\n";
		    }
		    assert(con_event->Key.getetime() >= prev_time);
		}
		if(	search_for_res_event
			&& (resEvent = tol_miter.peek())
			&& resEvent->Key.getetime()
			  	<= CTime_base(modeling_thread.SafeTime.load())) {

		    assert(resEvent->Key.getetime() > prev_res_event->Key.getetime());

		    //
		    // Advance the Miterator
		    //
		    tol_miter->next();
		    tol_record* res_event;
		    theTOLlist << (res_event = tol_record::resValueFactory(resEvent, 2));
		    if(theTOLlist.get_length() > 4) {
			cerr << "got over 4 error\n";
		    }
		    assert(res_event->Key.getetime() >= prev_time);
		    prev_res_event = resEvent;
		}
	    }
	}
    }

    //
    // Handle unreleased constraints
    //
    while((active_con = active_violations.first_node())) {
	con_violation*	unreleased;
	vio = active_con->payload;
	unreleased_violations << (unreleased = new con_violation(
			K.end,
			vio->payload.parent_constraint,
			vio->payload.message,
			vio->payload.severity));

	//
	// Need a constructor that creates an UNRELEASED record
	//
	eventPublisher ep(unreleased, /* unreleased = */ true);
	publish_vec->push_back(ep);
	delete active_con;
    }

    //
    // Wait till the filtering thread is done with its current data
    //
    while(records_to_filter<eventPublisher>(thread_index_to_use).load() != NULL) {
	thread_intfc::sleep_for_ms(1);
    }

    if(publish_vec->size()) {

	//
	// The filtering thread will delete publish_vec when done
	//
	records_to_filter<eventPublisher>(thread_index_to_use).store(publish_vec);

	//
	// Wait for the thread to finish processing the last batch
	//
	while(records_to_filter<eventPublisher>(thread_index_to_use).load() != NULL) {
	    thread_intfc::sleep_for_ms(1);
	}
    }

    //
    // Tell filtering thread it can stop
    //
    no_more_work(thread_index_to_use).store(true);

    //
    // Wait for filtering thread to finish and join it
    //
    if(Filter.joinable()) {
	Filter.join();
    }

    //
    // Possibility 1: there are records to output, but
    // this is not the end of the TOL. Just output what
    // is needed. We'll recap later.
    //
    if(lastResourceReportTime < lastProcessedEventTime
       && lastProcessedEventTime < K.end) {

	//
	// publish, if anything was found.
	//
	slist<alpha_void, res_record>::iterator iter2(resources_taken_care_of);
	res_record*				resrec;
	while((resrec = iter2())) {
	    resource = resrec->payload.first;
	    eventPublisher ep(resource->name,
			resrec->payload.second,
			lastProcessedEventTime,
			resource);
	    ep.write_to_stream(TOL_REGULAR, NULL);
	}
	lastResourceReportTime = lastProcessedEventTime;
    }

    //
    // Possibility 2: this is the end of the TOL.
    // We just publish everything, except possibly
    // for records already published with this
    // time tag.
    //
    if(thereWasAtLeastOneEvent) {

	//
	// Make sure all resources are included if
	// K.end is later than anything already
	// reported
	//
	if(lastResourceReportTime < K.end) {
	    resources_taken_care_of.clear();
	}
	//
	// else, we have already published what's in
	// resources_taken_care_of and we don't want
	// duplicates.
	//

	Rsource::iterator iter;
        while((resource = iter.next())) {
	    if(resource_selection[resource->output_index[thread_index]]) {

		//
		// This call publishes everything except
		// what's in resources_taken_care_of
		//
		handle_last_resource_event<eventPublisher>(
			K.end,
		      	resource,
		      	resources_taken_care_of,
		      	resource_selection,
    			K.WriteBothFiles);
	    }
	}
    }

    //
    // handle globals (if needed)
    //
    if(K.include_all_res_and_acts) {
	    try {
		TypedValue&     incon_globals = globalData::get_symbol("INCON_GLOBALS");
		eventPublisher::handle_globals(&incon_globals);
	    } catch(eval_error Err) {
		// not an error
	    }
    }

    if(APcloptions::theCmdLineOptions().TOLdebug
       && !eventPublisher::is_xml_publisher()) {
	string dbg_out = tol_debug_stream().str();
	if(dbg_out.length()) {
	    ofstream S(
		*(APcloptions::theCmdLineOptions().theResourceToDebug
			+ ".txt")
		);
	    S << dbg_out;
	}
    }
}
