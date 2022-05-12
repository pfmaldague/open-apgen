#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <action_request.H>
#include <ActivityInstance.H>
#include "apDEBUG.H"
#include <EventRegistry.H>
#include <EventImpl.H>
#include <RES_def.H>
#include <CompilerIntfc.H>

#include "assert.h"

using namespace std;
using namespace pEsys;

int				mEvent::currentEventID = 0;
int				mEvent::lastEventIDbeforeScheduling = 0;
int				mEvent::firstEventIDbeforeScheduling = 0;

Rsource::iterator::iterator()
	: iter(RCsource::resource_containers()),
		res_index(0),
		container(NULL) {
	iter.first();
}

Rsource* Rsource::iterator::next(
			bool reset  /* = false */) {
	if(reset) {
		container = iter.first();
		res_index = 0;
		return NULL;
	}

	//
	// first call, possibly after a full iteration
	//
	if(!container) {
		container = iter.first();
		res_index = 0;
		if(!container) {

			//
			// We are in the midst of deleting subsystems
			//
			return NULL;
		}
	}

	//
	// container is non-NULL; we are in the midst of iterating
	//
	if(container->payload && res_index < container->payload->Object->array_elements.size()) {
		return container->payload->Object->array_elements[res_index++];
	}

	//
	// res_index = container->payload->Object->array_elements.size(): we have
	// reached the end of the inner iteration loop.
	//
	// Move on to the next container:
	//
	container = iter();

	//
	// Start at the first resource of the new container:
	//
	res_index = 0;
	if(container && container->payload) {
		return container->payload->Object->array_elements[res_index++];
	}

	//
	// We have run out of containers
	//
	return NULL;
}
		
vector<TypedValue>&	Rsource::default_range() {
	static vector<TypedValue> T;
	return T;
}

tlist<alpha_void, smart_actptr>& eval_intfc::sel_list() {
	static tlist<alpha_void, smart_actptr> s;
	return s;
}

eval_intfc&	eval_intfc::evalIntfc() {
	static eval_intfc	e;
	return e;
}

/* originally indexed by (void *) AbstractResource but that was no good because
 * an abstract resource can be involved in more than one active thread.
 * Now I use the context from the execution stack. That'd BETTER be unique. */
tlist<alpha_int, Cntnr<alpha_int, instruction_node*> >&	eval_intfc::actsOrResWaitingOnSignals() {
	static tlist<alpha_int, Cntnr<alpha_int, instruction_node*> >	a;
	return a;
}
 
tlist<alpha_int, Cntnr<alpha_int, instruction_node*> >&	eval_intfc::actsOrResWaitingOnCond() {
	static tlist<alpha_int, Cntnr<alpha_int, instruction_node*> >	a;
	return a;
}
 
// blist&	eval_intfc::theListOfAllDynamicallyLoadedLibraries() {
// 	static blist B(compare_function(compare_bstringnodes, false));
// 	return B;
// }

tlist<alpha_string, RCsource>& RCsource::resource_containers() {
	static tlist<alpha_string, RCsource> B;
	return B;
}

RCsource::container_ptrs& RCsource::initialization_list() {
	static RCsource::container_ptrs CP;
	return CP;
}

multilist<alpha_time, mEvent, eval_intfc*>&	mEvent::eventQueue() {
	static multilist<alpha_time, mEvent, eval_intfc*> EQ(true);
	return EQ;
}

multilist<alpha_time, mEvent, eval_intfc*>&	mEvent::expansionQueue() {
	static multilist<alpha_time, mEvent, eval_intfc*> XQ(true);
	return XQ; }

multilist<alpha_time, mEvent, eval_intfc*>&	mEvent::schedulingQueue() {
	static multilist<alpha_time, mEvent, eval_intfc*> SQ(true);
	return SQ; }

multilist<alpha_time, mEvent, eval_intfc*>&	mEvent::currentQueue() {
	static multilist<alpha_time, mEvent, eval_intfc*> XQ(true);
	return XQ; }

multilist<alpha_time, mEvent, eval_intfc*>&	mEvent::initializationQueue() {
	static multilist<alpha_time, mEvent, eval_intfc*> XQ(true);
	return XQ; }

const alpha_time&	init_event::getKey() const {
	mEvent*			tn;
	const alpha_time*	at = &mEvent::getKey();

	if(((tn = mEvent::eventQueue().earliest_node()) != NULL) && tn->getetime() < at->getetime()) {
		at = &tn->getKey();
	}
	if(((tn = mEvent::expansionQueue().earliest_node()) != NULL) && tn->getetime() < at->getetime()) {
		at = &tn->getKey();
	}
	if(((tn = mEvent::currentQueue().earliest_node()) != NULL) && tn->getetime() < at->getetime()) {
		at = &tn->getKey();
	}
	if(((tn = mEvent::schedulingQueue().earliest_node()) != NULL) && tn->getetime() < at->getetime()) {
		at = &tn->getKey();
	}
	return *at;
}

res_container_object::~res_container_object() {
	for(int i = 0; i < array_elements.size(); i++) {
		delete array_elements[i];
	}
	array_elements.clear();
}

Miterator<slist<alpha_time, mEvent, eval_intfc*>, mEvent>&	eval_intfc::Events() {
	static Miterator<slist<alpha_time, mEvent, eval_intfc*>, mEvent> MI("Events");
	return MI; }

void eval_intfc::get_list_of_ptrs_to_resources(tlist<alpha_void, Cntnr<alpha_void, Rsource*> >& l) {
	Rsource::iterator	iter;
	Rsource*		resnode;

	while((resnode = iter.next()))
		l << new Cntnr<alpha_void, Rsource*>(resnode, resnode);
}

void eval_intfc::get_list_of_ptrs_to_containers(tlist<alpha_void, Cntnr<alpha_void, RCsource*> >& bl) {
	slist<alpha_string, RCsource>::iterator	all_containers(RCsource::resource_containers());
	RCsource*				theContainer;

	while((theContainer = all_containers())) {
		bl << new Cntnr<alpha_void, RCsource*>(theContainer, theContainer);
	}
}

tlist<alpha_void, Cntnr<alpha_void, Constraint*> >&	eval_intfc::active_constraints() {
	static tlist<alpha_void, Cntnr<alpha_void, Constraint*> > B;
	return B;
}

CTime_base& eval_intfc::MaxModelTime() {
	static CTime_base T(0, 0, false);
	return T;
}

bool	eval_intfc::MaxModelTimeExceededAt(const CTime_base &t) {
	static CTime_base Zero(0, 0, false);
	if(MaxModelTime() > Zero && t > MaxModelTime()) {
		return true;
	}
	return false;
}

// tlist<alpha_string, bsymbolnode>& eval_intfc::list_of_attribute_nicknames() {
// 	static tlist<alpha_string, bsymbolnode> B;
// 	return B;
// }

// blist &eval_intfc::list_of_custom_attributes() {
// 	static blist B(compare_function(compare_bstringnodes, false));
// 	return B;
// }

stringtlist& eval_intfc::ListOfAllFileNames() {
	static stringtlist B;
	return B;
}

// blist& eval_intfc::pointers_to_AAF_defined_functions() {
// 	static blist B(compare_function(compare_bstringnodes, false));
// 	return B;
// }

// List& eval_intfc::AAF_defined_functions() {
// 	static List L;
// 	return L;
// }

// blist& eval_intfc::pointers_to_AAF_defined_scripts() {
// 	static blist B(compare_function(compare_bstringnodes, false));
// 	return B;
// }

// List& eval_intfc::AAF_defined_scripts() {
// 	static List L;
// 	return L;
// }

bool	eval_intfc::restore_previous_id_behavior = false;
bool	eval_intfc::found_a_custom_attribute = false;

bool	(*eval_intfc::first_act_handler)(CTime_base&, int s) = NULL;

bool	eval_intfc::there_is_a_first_act(CTime_base& itsTime, int s) {
	if(first_act_handler) {
		return first_act_handler(itsTime, s);
	}
	return false;
}

bool	(*eval_intfc::last_event_handler)(CTime_base&) = NULL;

bool	eval_intfc::there_is_a_last_event(CTime_base& itsTime) {
	if(last_event_handler) {
		return last_event_handler(itsTime);
	}
	return false;
}

back_pointer *(*eval_intfc::act_type_as_back_ptr_handler)(void *V) = NULL;

back_pointer *eval_intfc::act_type_as_back_ptr(void *V) {
	if(act_type_as_back_ptr_handler) {
		return act_type_as_back_ptr_handler(V);
	}
	return NULL;
}

bool	eval_intfc::a_signal_matches(const Cstring& patt) {
	sigPatternNode*	p = evalIntfc().recent_patterns.find(patt);

	if(!p) {

		evalIntfc().recent_patterns << (p = new sigPatternNode(patt, patternInfo(patt)));
		try {
			// can throw:
			p->payload.compile();
		} catch(eval_error Err) {
			Cstring err("a_signal_matches(): error in pattern ");
			err << patt << "; details:\n" << Err.msg << "\n";
			throw(eval_error(err));
		}
		slist<alpha_string, sigInfoNode>::iterator	iter(evalIntfc().recent_signals);
		sigInfoNode*					s;
		while((s = iter())) {
			if(p->payload.matches(s->get_key())) {
				p->payload.Matches << new sigInfoPtr(s->get_key(), s);
			}
		}
	}
	return p->payload.Matches.get_length() > 0;
}

void eval_intfc::clear_signals() {
	evalIntfc().recent_signals.clear();
}

void eval_intfc::clear_patterns() {
	evalIntfc().recent_patterns.clear();
}

void eval_intfc::add_a_signal(sigInfoNode* s) {
	if(!evalIntfc().recent_signals.find(s->get_key())) {
		slist<alpha_string, sigPatternNode>::iterator	iter(evalIntfc().recent_patterns);
		sigPatternNode*					p;

		evalIntfc().recent_signals << s;
		while((p = iter())) {
			if(p->payload.matches(s->get_key())) {
				p->payload.Matches << new sigInfoPtr(s->get_key(), s);
			}
		}
	} else {
		delete s;
	}
}

sigInfoNode*	eval_intfc::find_a_signal(const Cstring& sig) {
	return evalIntfc().recent_signals.find(sig);
}

sigPatternNode*	eval_intfc::find_a_pattern(const Cstring& sig) {
	return evalIntfc().recent_patterns.find(sig);
}

void	eval_intfc::get_all_signals(stringtlist& A) {
	slist<alpha_string, sigInfoNode>::iterator	iter(evalIntfc().recent_signals);
	sigInfoNode*					s;

	while((s = iter())) {
		A << new emptySymbol(s->get_key());
	}
}

long	eval_intfc::number_of_recent_signals() {
	return evalIntfc().recent_signals.get_length();
}

Rsource* eval_intfc::FindResource(
		const Cstring& nm) {
	RCsource*	cont;
	Rsource*	res = NULL;
	Cstring		str;
	unsigned int	index;
	Cstring		resname = nm;
	Behavior*	cont_type;

	if(nm & "[") {	// numeric-index case: forget it
		resname = nm / "[";	//extract Array name
		cont = RCsource::resource_containers().find(resname);
		map<Cstring, int>::const_iterator cont_iter
			= Behavior::ClassIndices().find(resname);

		if(!cont || cont_iter == Behavior::ClassIndices().end()) {
			//no match on Resource [Array] name
			return NULL;
		}
		cont_type = Behavior::ClassTypes()[cont_iter->second];
		// look for resource index
		map<Cstring, int>::iterator res_iter
			= cont_type->SubclassFlatMap.find(nm);
		if(res_iter == cont_type->SubclassFlatMap.end()) {
			return NULL;
		}
		res = cont->payload->Object->array_elements[res_iter->second];
		return res;
	} else {		//non-array and string-index cases
		//Simple resource and string-indexed resource-array are both stored in
		//  the standard format (noted in method header comment), so this just works!
		cont = RCsource::resource_containers().find(nm);
		if(cont) {
			if(cont->payload->Object->array_elements.size() == 1) {
				res = cont->payload->Object->array_elements[0];
			}
		}
		return res;
	}
}

void recursively_add_indices(flexval& l, vector<string>& it, int i) {
	if(i < it.size()) {
		flexval&		sublevel(l[it[i]]);
		recursively_add_indices(sublevel, it, ++i);
	} else {
		l = true;
	}
}

act_lists::act_lists() :
		someActiveInstances(		"exec instances" ,	this, compare_function(compare_Time_nodes , true)),
		someDecomposedActivities(	"exec decomposed" ,	this, compare_function(compare_Time_nodes , true)),
		someAbstractedActivities(	"exec abstracted" ,	this, compare_function(compare_Time_nodes , true)),
		somePendingActivities(		"exec pending" ,	this, compare_function(compare_Time_nodes , true)),
		someBrandNewActivities(		"exec brand new" ,	this, compare_function(compare_Time_nodes , true)),
		clipboardIterator(false, true, somePendingActivities),
		activeIteratorScheduledOnly(true, false, someActiveInstances),
		activeIteratorScheduledOrNot(true, true, someActiveInstances),
		abstractedIterator(true, false, someAbstractedActivities),
		decomposedIterator(true, false, someDecomposedActivities),
		brandNewIterator(true, true, someBrandNewActivities) {
}


act_lists &eval_intfc::get_act_lists() {
	static act_lists	theActLists;
	return theActLists;
}

bool	eval_intfc::something_happened(
		int&	activeCount,
		int&	decomposedCount,
		int&	abstractedCount
		) {
	bool	retval = false;
	if(activeCount != get_act_lists().get_active_length()) {
		activeCount = get_act_lists().get_active_length();
		retval = true; }
	if(decomposedCount != get_act_lists().get_decomposed_length()) {
		decomposedCount = get_act_lists().get_decomposed_length();
		retval = true; }
	if(abstractedCount != get_act_lists().get_abstracted_length()) {
		abstractedCount = get_act_lists().get_abstracted_length();
		retval = true; }
	// else if(pendingCount != PendingActivities.get_length()) {
	// 	return true; }
	// else if(brandnewCount != brandnewActivities.get_length()) {
	// 	return true; }
	return retval;
}

status_aware_multiterator* eval_intfc::get_instance_multiterator(
				int& first_available_priority,
				bool incl_scheduled,
				bool incl_unscheduled) {
			// this constructor increments the priotity for each list;
			// see EventRegistry.C
	status_aware_multiterator* M = new status_aware_multiterator(
			&get_act_lists(),
			first_available_priority,
			incl_scheduled,
			incl_unscheduled);

	/* Note: the second argument describes the kind of 'thread' we are adding
	 * to the multiterator (e. g. "instances".) It's only used for fast TOL
	 * output when the -C option is used, in which case we use a special-purpose
	 * Multiterator method. Here we anticipate that the client will want to use
	 * first() and next(), mostly. */
	return M; }

const Cstring&	RESptr::get_key() const {
	return payload->name;
}

RCsource::RCsource(const RCsource& rcs)
	: baseC<alpha_string, RCsource>(rcs),
		use_method_count(0),
		payload(new Rcontainer(
				rcs.payload->Object,
				rcs.payload->Type,
				rcs.payload->myClass,
				rcs.payload->cardinality,
				rcs.payload->datatype)) {
	payload->set_source_to(this);
}

const CTime_base&	mEvent::getetime() const {

	return getKey().getetime();
}

apgen::RETURN_STATUS eval_intfc::string_to_valid_time(
		const Cstring&	given_time,
		CTime_base&	result,
		Cstring&	errors) {
	parsedExp Expression;

	try {
		compiler_intfc::CompileExpression(given_time, Expression);
	}
	catch(eval_error Err) {
		errors = "Syntax Error in start time expression ";
		errors << given_time << ":\n";
		errors << Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	TypedValue tv(apgen::DATA_TYPE::TIME);
	try {
		Expression->eval_expression(behaving_object::GlobalObject(), tv);
	}
	catch(eval_error Err) {
		errors = "Error in start time expression ";
		errors << given_time << ":\n";
		errors << Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	result = tv.get_time_or_duration();
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS eval_intfc::duration_to_timesystem_relative_string(
		const CTime_base&	given_dur,
		const Cstring&		EpochName,
		Cstring&		result,
		double&			multiplier,
		Cstring&		errors) {
	CTime_base	delta;

	if(EpochName == "UTC") {
		result = given_dur.to_string();
		multiplier = 1.0;
		return apgen::RETURN_STATUS::SUCCESS;
	}
	try {
		TypedValue&	tdv = globalData::get_symbol(EpochName);
		if(globalData::isAnEpoch(EpochName)) {
			result = given_dur.to_string();
			multiplier = 1.0;
			return apgen::RETURN_STATUS::SUCCESS;
		} else if(globalData::isATimeSystem(EpochName)) {
			ListOVal*	tv = &tdv.get_array();
			CTime_base	offset = tv->find("origin")->Val().get_time_or_duration();

			multiplier = tv->find("scale")->Val().get_double();
			delta = given_dur / multiplier;
			if(delta >= CTime_base(0, 0, true)) {
				result = Cstring("\"") + EpochName + "\":" + delta.to_string();
			} else {
				CTime_base	D(-delta) ;
	
				result = Cstring("-\"") + EpochName + "\":" + D.to_string();
			}
		} else {
			// Somewhat theoretical
			result.undefine();
			errors << "Epoch \"" << EpochName << "\" cannot be found.\n";
			return apgen::RETURN_STATUS::FAIL;
		}
	} catch(eval_error) {
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS eval_intfc::time_to_epoch_relative_string(
		const CTime_base &given_time,
		const Cstring &EpochName,
		Cstring &result,
		Cstring &errors) {
	CTime_base	delta;

	if(!EpochName.length()) {
		result.undefine();
		errors = "Epoch name is empty.";
		return apgen::RETURN_STATUS::FAIL;
	}
	if( EpochName == "UTC" ) {
		result = given_time.to_string();
		return apgen::RETURN_STATUS::SUCCESS;
	}
	try {
		TypedValue&	tdv = globalData::get_symbol(EpochName);
		if(globalData::isAnEpoch(EpochName)) {
			delta = given_time - tdv.get_time_or_duration();
			if(delta >= CTime_base(0, 0, true)) {
				result = EpochName + " + " + delta.to_string();
			} else {
				CTime_base	D(-delta);
	
				result = EpochName + " - " + D.to_string();
			}
		} else if(globalData::isATimeSystem(EpochName)) {
			ListOVal* tv = &tdv.get_array();
			CTime_base	offset = tv->find("origin")->Val().get_time_or_duration();
			double		multiplier = tv->find("scale")->Val().get_double();

			delta = (given_time - offset) / multiplier;
			if(delta >= CTime_base(0, 0, true)) {
				result = Cstring("\"") + EpochName + "\":" + delta.to_string();
			} else {
				CTime_base	D(- delta);

				result = Cstring("-\"") + EpochName + "\":" + D.to_string();
			}
		} else {
			// Somewhat theoretical
			result.undefine();
			errors << "Epoch " << EpochName << " cannot be found.\n";
			return apgen::RETURN_STATUS::FAIL;
		}
	} catch(eval_error) {
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void eval_intfc::purge_queues_and_histories(model_control::modelingPass currentPass) {
	Rsource::iterator	iter;
	Rsource*		res;

	while((res = iter.next())) {
		if(currentPass != model_control::SCHEDULING_2) {

			//
			// Don't clean up associative histories! That can only be done by
			// creating and destroying activity instances
			//
			if(res->get_class() != apgen::RES_CLASS::ASSOCIATIVE && !res->isFrozen()) {
				res->get_history().clear();
			}
		}
		res->get_history().clear_skipped_nodes_list();
	}
	mEvent::eventQueue().clear();
	mEvent::initializationQueue().clear();
	mEvent::expansionQueue().clear();
	mEvent::schedulingQueue().clear();
	mEvent::currentQueue().clear();
}

static void parse_resource_index(const Cstring& index, vector<stringtlist>& Vec) {
	const char*		resname = *index;
	const char*		s = resname;
	const char*		t = resname + strlen(resname) - 1;
	int			N = Vec.size();
	int			j;

	s = resname;
	for(j = 0; j < N; j++) {
		while(*s && *s != '[') {
			s++; }
		s += 2;
		t = s;
		while(*t && *t != '"') {
			t++; }
		// t now points to the " immediately following the jth index
		char* u = (char*) malloc(t - s + 1);
		strncpy(u, s, t - s);
		u[t - s] = '\0';
		if(!Vec[j].find(u)) {
			Vec[j] << new emptySymbol(u); }
		s = t + 2; // this is the ']' character at the end of the j-th index
		free(u); } }
	
RESptr::~RESptr() {
	if(list) {
		list->remove_node(this);
	}
}

template class Miterator<slist<alpha_time, mEvent>, mEvent>;
template class Cntnr<relptr<slist<alpha_time, mEvent>::iterator>, slist<alpha_time, mEvent>::iterator*>;
template class tlist<relptr<slist<alpha_time, mEvent>::iterator>, Cntnr<relptr<slist<alpha_time, mEvent>::iterator>, slist<alpha_time, mEvent>::iterator*> >;
template class slist<relptr<slist<alpha_time, mEvent>::iterator>, Cntnr<relptr<slist<alpha_time, mEvent>::iterator>, slist<alpha_time, mEvent>::iterator*> >;

// template class Miterator<slist<alpha_time, pef_record>, pef_record>;
// template class Cntnr<relptr<slist<alpha_time, pef_record>::iterator>, slist<alpha_time, pef_record>::iterator*>;
// template class tlist<relptr<slist<alpha_time, pef_record>::iterator>, Cntnr<relptr<slist<alpha_time, pef_record>::iterator>, slist<alpha_time, pef_record>::iterator*> >;
// template class slist<relptr<slist<alpha_time, pef_record>::iterator>, Cntnr<relptr<slist<alpha_time, pef_record>::iterator>, slist<alpha_time, pef_record>::iterator*> >;
template class tlist<alpha_void, backptr<ActivityInstance>::ptr2p2b, short>;
