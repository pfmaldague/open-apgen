#if HAVE_CONFIG_H
#include <config.h>
#endif
/*
 * PROJECT: APGEN
 * FILE NAME: RES_exec.C
 *
 */

#include <time.h>
#include <assert.h>
#include <sstream>

#include "ACT_exec.H"
#include "AbstractResource.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "Constraint.H"
#include "RES_def.H"
#include "EventImpl.H"
#include "RES_exec.H"
#include "Scheduler.H"

#include "apcoreWaiter.H"

#ifdef have_xml_reader
extern "C" {
#include "dom_chunky_xml_intfc.h"
} // extern "C"
#endif /* have_xml_reader */

using namespace std;

//
// External methods:
//
extern void			CleanupSchedulingStructures();
extern void			initialize_all_schedulers();

thread_local int thread_index;

void model_intfc::purge() {
	PurgeSchedulingEvents();
	eval_intfc::purge_queues_and_histories(model_control::MODELING);

	//
	// NOTE: resource containers clear their own resources
	//
	RCsource::resource_containers().clear();

	// slist<alpha_string, Cntnr<alpha_string, behaving_object*> >::iterator
	// 	it3(behaving_object::abstract_resources());
	// Cntnr<alpha_string, behaving_object*>* N;
	// while((N = it3())) {
	// 	delete N->payload;
	// }
	// behaving_object::abstract_resources().clear();

	list_of_all_typedefs().clear();
}

apgen::RETURN_STATUS RES_exec::checkForEmptyArrayedResources(
		Cstring& any_errors) {
	slist<alpha_string, RCsource>::iterator	theContainers(RCsource::resource_containers());
	RCsource*							a_container;
	slist<alpha_void, Cntnr<alpha_void, RCsource*> >		theDoomedOnes;
	slist<alpha_void, Cntnr<alpha_void, RCsource*> >::iterator	Doom(theDoomedOnes);
	Cntnr<alpha_void, RCsource*>*					doomed;

	while((a_container = theContainers())) {
		if(!a_container->payload->Object->array_elements.size()) {
			any_errors << "Arrayed resource \"" << a_container->get_key() << "\" has no indices.\n";
			theDoomedOnes << new Cntnr<alpha_void, RCsource*>(a_container, a_container);
		}
	}
	while((doomed = Doom())) {
		delete doomed->payload;
	}
	if(theDoomedOnes.get_length()) {
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void model_intfc::add_timevent(
		mEvent*& new_node,
		apgen::METHOD_TYPE theAafSection) {
    if(EventLoop::CurrentEvent) {
	if(new_node->getetime() < EventLoop::CurrentEvent->getetime()) {
		Cstring errs;

		//
		// we don't want to include this node in the event queue; we will just ignore it.
		//
		errs << "APcore modeling error: chosen node time "
			<< new_node->getetime().to_string();
		errs << " precedes CurrentEvent time "
			<< EventLoop::CurrentEvent->getetime().to_string() << "\n";
		delete new_node;
		new_node = NULL;
		throw(eval_error(errs));
	}
    }


    //
    // Remember Multiterator priorities:
    //
    // Events().add(eval_intfc::initializationQueue(),	"initialization events",	0);
    // Events().add(eval_intfc::eventQueue(),		"regular events",		1);
    // Events().add(eval_intfc::currentQueue(),		"current events",		2);
    // Events().add(eval_intfc::expansionQueue(),	"expansion events",		3);
    // Events().add(eval_intfc::schedulingQueue(),	"scheduling events",		4);
    //
    if(new_node->get_queue() == &mEvent::expansionQueue()) {
	if(theAafSection == apgen::METHOD_TYPE::CONCUREXP) {

		mEvent::schedulingQueue() << new_node;

	} else if(EventLoop::CurrentEvent && EventLoop::CurrentEvent->getetime() == new_node->getetime()) {

		mEvent::currentQueue() << new_node;

	} else {

		mEvent::expansionQueue() << new_node;

	}
    } else {

	mEvent::eventQueue() << new_node;

    }
}

bool model_intfc::FirstEvent(CTime_base & T, int print_flag) {
	bool	retval = false;
	/* Remember that an "empty" EventQueue already contains
	 * an initializer node - NO! Nowadays the initialization
	 * node has its own queue */
	mEvent*	tmnode = mEvent::eventQueue().earliest_node();
	mEvent* tmnode1 = mEvent::currentQueue().earliest_node();
	mEvent* tmnode2 = mEvent::expansionQueue().earliest_node();
	mEvent* tmnode3 = mEvent::schedulingQueue().earliest_node();
	bool	eventq = false;
	bool	currentq = false;
	bool	expansionq = false;
	bool	schedulingq = false;

	if((retval = ACT_exec::ACT_subsystem().FirstRequestTime(T))) {
		if(tmnode && tmnode->getetime() < T) {
			eventq = true;
			T = tmnode->getetime();
		}
		if(tmnode1 && tmnode1->getetime() < T) {
			currentq = true;
			T = tmnode1->getetime();
		}
		if(tmnode2 && tmnode2->getetime() < T) {
			expansionq = true;
			T = tmnode2->getetime();
		}
		if(tmnode3 && tmnode3->getetime() < T) {
			schedulingq = true;
			T = tmnode3->getetime();
		}
		return true;
	}

	return retval;
}

bool model_intfc::LastEvent(CTime_base& T) {
	// Remember that an "empty" event queue already contains an initializer node:
	if(mEvent::eventQueue().get_length() > 1) {
		mEvent*	tmnode = mEvent::eventQueue().latest_node();

		if(ACT_exec::ACT_subsystem().LastRequestTime(T)) {
			if(tmnode->getetime() > T)
				T = tmnode->getetime();
			return true;
		}
		T = tmnode->getetime();
		return true;
	} else {
		return (ACT_exec::ACT_subsystem().LastRequestTime(T) != 0);
	}
}

void Rsource::get_all_resource_names(stringslist& inlist) {
	Rsource*	resnode;
	iterator	iter;

	while((resnode = iter.next())) {
		inlist << new emptySymbol(resnode->name);
	}
}

void Rsource::get_all_resource_container_names(stringslist& inlist) {
	slist<alpha_string, RCsource>::iterator	rlst(RCsource::resource_containers());
	RCsource*				resnode;

	while((resnode = rlst.next()))
		inlist << new emptySymbol(resnode->get_key());
}

/* NOTE: This method should be essentially useless since we normally delete events as we go.
 * However, there is still an option to accumulate events (for checking that things work the
 * same way whether or not we delete events on the fly.) There might also be exceptional
 * cases (modeling error inside the event processing loop) in which the queues are not empty.
 * Finally there is the special case of the initialization event.
 *
 * All this should be cleaned up - probably by purging the event loop(s) at the end of
 * EventLoop::ProcessEvents(), so it's done locally and we don't have to worry about it at
 * this level.
 *
 * NOTE: tried to remove it - got segfaults. Obviously, it's needed... reexamine the
 * situation after cleaning things up.
 */
void model_intfc::PurgeSchedulingEvents() {

	eval_intfc::Events().clear();
	mEvent::eventQueue().clear();
	mEvent::expansionQueue().clear();
	mEvent::currentQueue().clear();
	mEvent::schedulingQueue().clear();

	// DON'T FORGET THIS:
	EventLoop::CurrentEvent = NULL;
	// DON'T FORGET THIS:
	CleanupSchedulingStructures();
}

void Rsource::get_all_resources_in_dependency_order(
			tlist<alpha_string, Cntnr<alpha_string, Rsource*> >&
				all_requested_res) {
	RCsource::container_ptrss::iterator	containers(RCsource::initialization_list());
	RCsource::container_ptr*		rcptr;
	RCsource*				theContainer;

	while((rcptr = containers())) {
		theContainer = rcptr->payload;
		vector<Rsource*>& vec = theContainer->payload->Object->array_elements;
		for(int i = 0; i < vec.size(); i++) {
			all_requested_res << new Cntnr<alpha_string, Rsource*>(
						vec[i]->name, vec[i]);
		}
	}
}



// ALWAYS clears events:
void model_intfc::init_model(
			time_saver& theMaster) {
	CTime_base		t1;
				// should be NULL:
	mEvent*			theInitialEvent = mEvent::initializationQueue().first_node();
	eventPLD*		event_payload;
	double			double_now;
	TypedValue		v;

	if(FirstEvent(t1, 1)) {
		v = t1;

		/* We are about to initialize all resources, using a loop that scans
		 * everything. Before we do, if currentPass is model_control::SCHEDULING_2, 
		 * we should clear old leftovers. */
		theMaster.set_now_to(t1);
	} else {
		theMaster.set_now_to(CTime_base(time(0), 0, false));
	}

	if(theInitialEvent) {
		delete theInitialEvent;
		theInitialEvent = NULL;
	}
	EventLoop::CurrentEvent = NULL;

	//
	// NOTE: the following call does not purge histories
	// if currentPass == model_control::SCHEDULING_2
	//
	eval_intfc::purge_queues_and_histories(model_control::get_pass());
	eval_intfc::clear_signals();
	eval_intfc::clear_patterns();
	if(model_control::get_pass() != model_control::SCHEDULING_2) {
		mEvent::currentEventID = 0;
	}
	thread_intfc::update_current_time(time_saver::get_now());
	theInitialEvent = init_event::initialEventFactory(time_saver::get_now())->source;
	mEvent::initializationQueue() << theInitialEvent;
}

void RES_exec::report_history() {
	Rsource*		resnode;
	Rsource::iterator	iter;

	while((resnode = iter.next())) {
		resnode->report_history();
	}
}

void RES_exec::clear_all_lists() {
	// do this first - histories depend on the event Queue
	// NOTE: this clears the Rsource objects also
	RCsource::resource_containers().clear();

	mEvent::eventQueue().clear();
	mEvent::expansionQueue().clear();
	mEvent::schedulingQueue().clear();
	mEvent::currentQueue().clear();
	mEvent::initializationQueue().clear();

	eventPLD::resources_with_no_dependencies().clear();
}

bool	model_intfc::HasEvents() {
	return (
		mEvent::eventQueue().get_length()
			+ mEvent::expansionQueue().get_length()
			+ mEvent::schedulingQueue().get_length()
			+ mEvent::currentQueue().get_length()) > 0;
}

/* This recursive method accomplishes several things. The first time it is called,
 *
 *	- this is a resource that is known not to have any dependencies on other resources
 *	- the list of dependents is, in fact, the containers_whose_profile_invokes_this
 *	  member of this container. Initially, it only includes direct dependencies, not indirect ones
 *	  that result from multiple links through currentval's in profiles
 *	- the list of good parents only includes dependency-free resources
 *	- the list of parents contains nothing
 *
 * After the call,
 *
 *	- any circular dependencies will have been detected, resulting in an error that
 *	  interrupts the recursive call chain. The ticmarks below assume no error.
 *	- the list of dependents will be expanded to all include all indirect resources
 *	- the list of good parents will include all resources
 *	- the list of parents will be empty again
 *
 */
apgen::RETURN_STATUS Rcontainer::complete_dependency_list(
		tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& list_of_dependents_to_be_completed,
		tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& list_of_good_parents,
		tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& list_of_parents,
		Cstring &any_errors) {
	Cntnr<alpha_void, RCsource*>*			ptr_to_new_dependent;
	Cntnr<alpha_void, RCsource*>*			temp_parent;
	slist<alpha_void, Cntnr<alpha_void, RCsource*> > direct_dependents(
						containers_whose_profile_invokes_this);
	Cstring						rmessage;
	// static int					indentation = 0;

	// debug
	// indentation += 1;
	// for(int i = 0; i < indentation; i++) { cerr << "  "; }
	// cerr << mySource->get_key() << "->complete_dependency_list: START\n";

	for(	ptr_to_new_dependent = direct_dependents.first_node();
		ptr_to_new_dependent;
		ptr_to_new_dependent = ptr_to_new_dependent->next_node()) {
		RCsource*	new_dependent = ptr_to_new_dependent->payload;

		// for(int i = 0; i < indentation; i++) { cerr << "  "; }
		// cerr << mySource->get_key() << "... checking on direct dependent \""
		// 	<< new_dependent->get_key() << "\"\n";
		// for(int i = 0; i < indentation; i++) { cerr << "  "; }
		// cerr << "It has " << new_dependent->payload->containers_whose_profile_invokes_this.get_length()
		// 	<< " direct dependent(s).\n";

		if(list_of_parents.find((void *) new_dependent)) {
			any_errors << mySource->get_key() << " appears in profile of _and_ depends on "
				<< new_dependent->get_key() << " #";
			clear_profiles();
			ptrs_to_containers_used_in_profile.clear();
			containers_whose_profile_invokes_this.clear();
			// indentation--;
			return apgen::RETURN_STATUS::FAIL;
		}

		/* The list_of_parents is basically a set of resources which MAY NOT
		 * include any extended dependents of this. */
		list_of_parents << (temp_parent = new Cntnr<alpha_void, RCsource*>(new_dependent, new_dependent));

		/* The call will
		 *	- add the new_dependent's dependents to containers_whose_profile_invokes_this
		 *	- add the new_dependent to the list of good parents if no error occurs
		 */
		if(new_dependent->complete_dependency_list(
				new_dependent->payload->containers_whose_profile_invokes_this,
				list_of_good_parents,
				list_of_parents,
				rmessage) != apgen::RETURN_STATUS::SUCCESS) {
			any_errors <<  rmessage << ", reported while processing res. "
				<< new_dependent->get_key() << "; ";
			clear_profiles();
			ptrs_to_containers_used_in_profile.clear();
			containers_whose_profile_invokes_this.clear();
			// indentation--;
			return apgen::RETURN_STATUS::FAIL;
		}
		// for(int i = 0; i < indentation; i++) { cerr << "  "; }
		// cerr << "After completing the dependency list, " << new_dependent->get_key() << " now has "
		// 	<< new_dependent->payload->containers_whose_profile_invokes_this.get_length()
		// 	<< " direct or indirect dependent(s).\n";
		slist<alpha_void, Cntnr<alpha_void, RCsource*> >::iterator
			l1(new_dependent->payload->containers_whose_profile_invokes_this);
		Cntnr<alpha_void, RCsource*>*	bt;
		while((bt = l1())) {
			RCsource*	higher_level = bt->payload;
			if(!list_of_dependents_to_be_completed.find((void *) higher_level)) {
				// for(int i = 0; i < indentation; i++) { cerr << "  "; }
				// cerr << "Adding ptr to " << higher_level->get_key() << " to list of dependents of " << mySource->get_key() << "\n";
				list_of_dependents_to_be_completed << new Cntnr<alpha_void, RCsource*>(higher_level, higher_level);
			} else {
				// for(int i = 0; i < indentation; i++) { cerr << "  "; }
				// cerr << higher_level->get_key() << " is already in the list of dependents of " << mySource->get_key() << "\n";
			}
		}
		delete temp_parent;
	}
	if(!list_of_good_parents.find((void *) mySource)) {

		// for(int i = 0; i < indentation; i++) { cerr << "  "; }
		// cerr << "Adding ptr to " << mySource->get_key() << " to the list of good parents.\n";

		list_of_good_parents << new Cntnr<alpha_void, RCsource*>((void*) mySource, mySource);
	}
	// for(int i = 0; i < indentation; i++) { cerr << "  "; }
	// cerr << "...END\n";
	// indentation--;
	return apgen::RETURN_STATUS::SUCCESS;
}

void RCsource::add_constraint(Constraint* cons) {
	vector<Rsource*>&	vec = payload->Object->array_elements;

	for(int i = 0; i < vec.size(); i++) {
		Cntnr<alpha_void, Rsource*>*	already_found;
		if(!(already_found = Constraint::resourcesSubjectToConstraints()
					.find((void*)vec[i]))) {

#ifdef DEBUG_CONSTRAINTS
			cerr << "   +++ Adding res. " << vec[i]->name << " to list of res. subj. to cons.\n";
#endif /* DEBUG_CONSTRAINTS */

			Constraint::resourcesSubjectToConstraints()
				<< (already_found = new Cntnr<alpha_void, Rsource*>(
						(void*)vec[i], vec[i]));
		}
	}

	//
	// We check, because this container may be a dependency for
	// many resources involved in the same constraint:
	//
	if(!any_constraints.find((void*)cons)) {

#ifdef DEBUG_CONSTRAINTS
		cerr << "   +++ Adding constraint " << cons->get_key() << " to resource cont. "
			<< get_key() << "\n";
#endif /* DEBUG_CONSTRAINTS */

		any_constraints << new Cntnr<alpha_void, Constraint*>((void*)cons, cons);
	}
}
