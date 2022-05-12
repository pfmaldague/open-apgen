#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <dlfcn.h>
#include <unistd.h>

#include "aafReader.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "ACT_exec.H"
#include "apcoreWaiter.H"
#include "C_global.H"
#include "EventImpl.H"
#include "Legends.H"
#include "RES_exec.H"
#include "UI_dsconfig.h"

using namespace std;

				// in action_request.C:
extern long int			find_color_id(const Cstring &);

// debug
int	act_object::act_obj_count = 0;

const Cstring&	act_object::GenericType() {
	static Cstring g("generic");
	return g;
}

extern "C" {
	// in Dynamic.c:
	extern void	*initialize_a_c_type_act_instance(const char *init_function_name, void **the_args);
	extern int	destroy_a_c_type_act_instance(const char *destructor_name, void *the_instance);
	extern const char *get_dynamic_error();
	// closes the currently loaded library (if any)
	extern void	close_the_c_library(); 
	};

	// list of bpointer_with_list's
blist&	ActivityInstance::NumbersInUse() {
	static blist n(compare_function(compare_bpointernodes, false));
	return n; }

	// list of bpointernodes
blist&	ActivityInstance::NumbersAvailable() {
	static blist na(compare_function(compare_bpointernodes, false));
	return na; }

bool&	ActivityInstance::we_are_writing_an_SASF() {
	static bool foo;
	return foo; }

unsigned long long&	ActivityInstance::abs_act_count() {
	static unsigned long long i = 0;
	return i; }

			/* for information only; no longer used
			 * for setting ID automatically. */
unsigned long long&	ActivityInstance::number_of_activities() {
	static unsigned long long i = 0;
	return i; }

class bpointer_with_list: public bpointernode {
public:
	List			ptrs_to_activities;

	bpointer_with_list(void *a, ActivityInstance *act);
	bpointer_with_list(const bpointer_with_list &B)
		: bpointernode(B),
		ptrs_to_activities(B.ptrs_to_activities) {}

	~bpointer_with_list() {}

	Node		*copy() {
		return new bpointer_with_list(*this); 
	}
};

bpointer_with_list::bpointer_with_list(void *a, ActivityInstance *act)
	: bpointernode(a, NULL) {
	ptrs_to_activities << new Pointer_node(act, NULL);
	}

bool
has_a_postfix(const Cstring &id, unsigned long long &postfix) {
	const char	*c;

	if(id & "_") {
		bool is_a_number = false;

		c = (*id) + id.length() - 1;
		while(*c != '_') {
			if(isdigit(*c--))
				is_a_number = 1;
			else {
				is_a_number = 0;
				break;
			}
		}

		if(is_a_number) {
			postfix = atoll(++c);
			return true;
		}
	}
	return false;
}

const Cstring &get_static_status() {
	static Cstring	the_status("status");
			 
	return the_status; }

void ActivityInstance::purge() {
	NumbersInUse().clear();
	NumbersAvailable().clear();
	abs_act_count() = 0LL;
	number_of_activities() = 0LL; }

Cstring ActivityInstance::generate_id(const Cstring& ID) {
    static unsigned long long	id = 0LL;
    Cstring			unique_id;

    if(eval_intfc::restore_previous_id_behavior) {
	static bpointer_with_list	*bpwl;
	static bpointernode		*bp;

	number_of_activities()++;

	if(	(!NumbersInUse().find((void *) number_of_activities())) &&
		(!NumbersAvailable().find((void *) number_of_activities()))
		) {
	    NumbersAvailable() << new bpointernode((void *) number_of_activities(), NULL);
	}

	/*
	 * NOTE: at this point, the NumbersAvailable list ALWAYS has at least one element in it.
	 * 	 This is not obvious, because according to the previous conditional statement, it
	 * 	 is conceivable that NumbersInUse COULD contain a node tagged with number_of_activities
	 * 	 (hence the find() call would result in something nonzero), while NumbersAvilable would
	 * 	 be empty.
	 *
	 * 	 But this can NEVER be the case. The reason is that if NumbersInUse contains
	 *	 number_of_activities, then AN ACTIVITY MUST HAVE BEEN DELETED at some point.
	 */

	if(ID.length()) {
	    if(!has_a_postfix(ID, id)) {
			if(eval_intfc::restore_previous_id_behavior) {
				// "how do we know that NumbersAvailable is not empty? "
				bp = (bpointernode *) NumbersAvailable().last_node();
				unique_id << ID << "_" << (id = (long) bp->get_ptr());
				delete bp;
				NumbersInUse() << new bpointer_with_list((void *) id, this);
			} else {
				if(!aaf_intfc::actIDs().find(ID)) {
					unique_id = ID;
				}
			}
	    }
	    // no need to check unicity; the fact that we got it from NumbersAvailable guarantees it.
	    else if(!aaf_intfc::actIDs().find(ID)) {
			if(!(bpwl = (bpointer_with_list *) NumbersInUse().find((void *) id))) {
				NumbersInUse() << new bpointer_with_list((void *) id, this);
			} else {
				bpwl->ptrs_to_activities << new Pointer_node(this, NULL);
			}
			if((bp = (bpointernode *) NumbersAvailable().find((void *) id))) {
				delete bp;
			}

			unique_id = ID;
	    }
	    if(!unique_id.is_defined()) { // append a unique integer:
			// "how do we know that NumbersAvailable is not empty? "
			bp = (bpointernode *) NumbersAvailable().last_node();
			if(eval_intfc::restore_previous_id_behavior) {
				char	buf[32];

				sprintf(buf, "_%lld", id);
				unique_id = ID / buf;
				unique_id << "_" << (id = (long) bp->get_ptr());
			} else {
				unique_id << ID << "_" << (id = (long) bp->get_ptr());
			}
			delete bp;
			NumbersInUse() << new bpointer_with_list((void *) id, this);
	    }
	} else {
	    bp = (bpointernode *) NumbersAvailable().last_node();
	    // "how do we know that NumbersAvailable is not empty? "
	    unique_id << "generic" << "_" << (id = (long) bp->get_ptr());
	    delete bp;
	    NumbersInUse() << new bpointer_with_list((void *) id, this);
	}
    } else {
	static char	buf[128];
	Cstring		ID_copy(ID);

	number_of_activities()++;
	abs_act_count()++;

	if(!ID.length()) {
	    ID_copy = "generic"; }
	if(!has_a_postfix(ID_copy, id)) {
		// no postfix, so we are OK
	    if(!aaf_intfc::actIDs().find(ID_copy)) {
			unique_id = ID_copy;
	    } else {
			unique_id << ID_copy << "_" << abs_act_count();
	    }
	} else if(!aaf_intfc::actIDs().find(ID_copy)) {
	    // there is a postfix... The only thing we care about is making sure
	    // that it won't interfere with our numbering scheme.
	    if(abs_act_count() < id) {
			abs_act_count() = id; }
	    unique_id = ID_copy;
	} else {
	    // There is a postfix but it's in use; append a unique integer:
	    sprintf(buf, "%lld", id);
	    unique_id = ID_copy / buf;
	    unique_id << abs_act_count();
	}
    }

    aaf_intfc::actIDs() << new Cntnr<alpha_string, ActivityInstance*>(unique_id, this);
    return unique_id;
}

extern "C" {
	void *(*ActCreator)(const char *type_name, void *d_source, void *b_list, char **errs_found);
}

ActivityInstance::ActivityInstance(
		act_object*	ao)
	: theHierarchyHandler(apgen::METHOD_TYPE::NONE, this),
		A(this),
		theLegendHandler(this),
		thePRmanager(this),
		Key(0, 0) {
	Object.reference(ao);
	ao->act = this;
	Cstring proposed_id;
	if((*ao)[ID].get_string().length() == 0) {
		if((*ao)[NAME].get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
			proposed_id = (*ao)[NAME].get_string();
		} else {
			proposed_id = (*ao)[TYPE].get_string();
		}
	} else {
		proposed_id = (*ao)[ID].get_string();
	}
	(*ao)[ID] = generate_id(proposed_id);

	if((*ao)[ActivityInstance::cOLOR].get_string().length() == 0) {
		(*ao)[ActivityInstance::cOLOR] = GENERICCOLOR;
	}
	(*ao)[COLOR] = find_color_id((*ao)[cOLOR].get_string());
	if((*ao)[ActivityInstance::STATUS].get_int() == false) {
		(*ao)[ActivityInstance::STATUS] = true;
	}
	TypedValue&	finishval = (*ao)[ActivityInstance::FINISH];
	TypedValue&	spanval = (*ao)[ActivityInstance::SPAN];
	finishval = (*ao)[ActivityInstance::START].get_time_or_duration()
			+ (*ao)[ActivityInstance::SPAN].get_time_or_duration();

	time_actptr*	smartref = new time_actptr(
						finishval.get_time_or_duration_ref(),
						this);
	ACT_exec::ACT_subsystem().ActivityEnds << smartref;

	//
	// Make sure that this instance is properly attached to its legend.
	//
	LegendObject*	ldef;

	Cstring the_leg((*ao)[ActivityInstance::LEGEND].get_string());
	ldef = (LegendObject *) Dsource::theLegends().find(the_leg);

	if(!ldef) {
		ldef = LegendObject::LegendObjectFactory(
				the_leg,
				aafReader::current_file(),
				ACTVERUNIT);
	}
	legend().switch_to_legend(ldef);

	set_scheduled(true);

	//
	// derivedDataConstructor is a handle for subclasses like those in the ATM client
	//
	if(Dsource::derivedDataConstructor) {
		Cstring any_errors;
		derivedDataConstructor(this, any_errors);
	}
	agent().move_to_brand_new_list();
}

	// 2nd arg defaults to false but it's obsolete anyway
int destroy_an_act_req(ActivityInstance* doomed, bool) {
	assert(doomed);
	delete doomed;
	return 1;
}

ActivityInstance::ActivityInstance(
			const ActivityInstance& ar)
		:  theHierarchyHandler(apgen::METHOD_TYPE::NONE, this),
			A(this),
			theLegendHandler(this),
			thePRmanager(this),
			Key(ar.getKey()) {
	int		the_index = Behavior::ClassIndices().at((*ar.Object)[TYPE].get_string());
	Behavior*	beh = Behavior::ClassTypes()[the_index];
	Cstring		unique_id;
	act_object* ao = new act_object(
				this,
				(*ar.Object)[NAME].get_string(),
				unique_id,
				*beh);
	Object.reference(ao);
	ao->act = this;
	(*ao)[LEGEND] = (*ar.Object)[LEGEND];
	(*ao)[START] = (*ar.Object)[START];
	(*ao)[SPAN] = (*ar.Object)[SPAN];
	(*ao)[FINISH] = (*ar.Object)[FINISH];
	if((*ao)[cOLOR].get_string() == "") {
		(*ao)[cOLOR] = GENERICCOLOR;
	}
	(*ao)[COLOR] = find_color_id((*ao)[cOLOR].get_string());
	if((*ao)[STATUS].get_int() == false) {
		(*ao)[STATUS] = true;
	}

	(*ao)[PLAN] = "New";

	TypedValue&	finishval = (*ao)[FINISH];
	TypedValue&	spanval = (*ao)[SPAN];
	finishval = (*ao)[START].get_time_or_duration()
			+ (*ao)[SPAN].get_time_or_duration();

	time_actptr*	smartref = new time_actptr(
						finishval.get_time_or_duration_ref(),
						this);
	ACT_exec::ACT_subsystem().ActivityEnds << smartref;

	/* Make sure that this instance is properly attached to its legend. */
	LegendObject*	ldef;

	Cstring the_leg((*ao)[LEGEND].get_string());
	ldef = (LegendObject *) Dsource::theLegends().find(the_leg);
	if(!ldef) {
		ldef = LegendObject::LegendObjectFactory(
				the_leg,
				aafReader::current_file(),
				ACTVERUNIT);
	}
	legend().switch_to_legend(ldef);


	if(Dsource::derivedDataConstructor) {
		Cstring	any_errors;
		derivedDataConstructor(this, any_errors);
	}
	agent().move_to_brand_new_list();
}

ActivityInstance::~ActivityInstance() {
    unsigned long long	id = 0LL;

    //
    // DO THIS NOW -- later on, a lower-level destructor would have trouble
    // removing this from a blist since the getetime() method used in
    // comparisons while removing the node is no longer applicable:
    //
    if(list) list->remove_node(this);

    //
    // now do the same for the template-based smart pointers:
    //
    Pointers()->delete_pointers_to_this();

    number_of_activities()--;

    if(eval_intfc::restore_previous_id_behavior) {
	bpointer_with_list*	bpwl;

	// the next few lines'd better work like a charm!!
	// On the positive side, I am quite sure that users will report any problems that occur here!
	has_a_postfix(get_unique_id(), id);
	bpwl = (bpointer_with_list *) NumbersInUse().find((void *) id);

	// because of requests, bpwl may not exist...
	if(bpwl) {
	    if(bpwl->ptrs_to_activities.get_length() == 1) {
		delete bpwl;

		//
		// "one of the place(s) where we populate NumbersAvailable..."
		//
		NumbersAvailable() << new bpointernode((void *) id, NULL);
	    } else {
		delete (Pointer_node *) bpwl->ptrs_to_activities.find((void *) this);
	    }
	}
    }

    // defensive programming
    Cntnr<alpha_string, ActivityInstance*>*	N;
    if((N = aaf_intfc::actIDs().find(get_unique_id()))) {
	delete N;
    }
    if(derivedDataDestructor) {
	derivedDataDestructor(this);
    }

    //
    // Do this to remove any self-references:
    //
    (*Object)[THIS].undefine();
    Object->clear();
}

TypedValue act_object::to_struct() const {
	ListOVal*	lov = new ListOVal;
	ArrayElement*	ae;
	TypedValue V;

	for(int i = 0; i < Task.get_varinfo().size(); i++) {
		Cstring key = Task.get_varinfo()[i].first;
		if(key == "parent") {
			TypedValue	P = operator[](ActivityInstance::PARENT);

			//
			// parent is set to an empty list if undefined:
			//
			if(P.get_type() != apgen::DATA_TYPE::ARRAY) {
				assert(P.get_type() == apgen::DATA_TYPE::INSTANCE);
				behaving_element&	be(P.get_object());
				TypedValue parent_content = be->to_struct();
				ae = new ArrayElement(key, parent_content);
				lov->add(ae);
			}
		} else if(key == "this") {
			;
		} else if(operator[](i).get_type() != apgen::DATA_TYPE::UNINITIALIZED) {
			ae = new ArrayElement(key, operator[](i));
			lov->add(ae);
		}
		
	}
	V = (*lov);
	return V;
}


void act_object::assign_w_side_effects(
		int			var_to_assign,
		const TypedValue&	newval,
		bool			notify) {
	static Cstring		the_value;
	bool			position_has_changed = false;
	bool			appearance_has_changed = false;

	TypedValue*	oldvalptr = &operator[](var_to_assign);

	if(oldvalptr->is_int()) {
		if(oldvalptr->get_int() == newval.get_int()) {
			return;
		}
	} else if(oldvalptr->is_time() || oldvalptr->is_duration()) {
		if(oldvalptr->get_time_or_duration() == newval.get_time_or_duration()) {
			return;
		}
	} else if(oldvalptr->is_string()) {
		if(oldvalptr->get_string() == newval.get_string()) {
			return;
		}
	} else  {

		//
		// In principle, we could check for arrays here
		// (sasf and misc attr. cases)... Does
		// TypedValue::operator=
		// support this?
		//
	}

	try {
	    switch(var_to_assign) {
	    //duration and start cases also set "finish", latter via setetime()
		case ActivityInstance::SPAN:
		    if(newval.get_time_or_duration() < CTime_base(0, 0, true)) {
			Cstring		err;
			CTime_base   	bad_time(newval.get_time_or_duration());

			err << "Activity '"
			    << operator[](ActivityInstance::ID).get_string()
			    << "' has negative duration "
			    << bad_time.to_string() << "\n";
			operator[](ActivityInstance::SPAN)
			    = ACT_exec::ACT_subsystem().default_req_span();
			throw(eval_error(err));
		    }

		    //
		    // Unfortunately we have to accommodate changes in 
		    // span during modeling.
		    //
		    if(!newval.get_time_or_duration().is_duration()) {
			Cstring		err;
			err << "Activity '"
			    << operator[](ActivityInstance::ID).get_string()
			    << "' was given illegal duration "
			    << newval.get_time_or_duration().to_string() << "\n";
			throw(eval_error(err));
		    }

		    appearance_has_changed = true;
		    set_timespan(newval.get_time_or_duration());
		    break;
		case ActivityInstance::START:
		    if(newval.get_time_or_duration() < CTime_base(0, 0, false)) {
			Cstring		err;
			err << "Activity '"
			    << operator[](ActivityInstance::ID).get_string()
			    << "' was given illegal start time " <<
				newval.get_time_or_duration().to_string() << "\n";
			throw(eval_error(err));
		    }

		    //
		    // Need to detect whether this is occurring during
		    // modeling. If so, there'd better not be a TOL
		    // output activity at the same time, because it is
		    // going to get confused.
		    //
		    if(model_control::get_pass() != model_control::INACTIVE) {
			Cstring		rmessage;

			rmessage << "Activity " << operator[](ActivityInstance::ID).get_string()
			    << " is having its start time changed while modeling. This "
			    << "is going to cause problems, especially if (XML)TOL "
			    << "output is going on in parallel.";
			throw(eval_error(rmessage));
		    }

		    //This handles default situation -- move any children together
		    //  with parent instance.  Must call move_starttime() directly,
		    //  instead of assign_w_side_effects(), to NOT carry children along.  If
		    //  any other special cases are added to assign_w_side_effects(), it would
		    //  be worth adding another signature, with a 3rd "flag" arg.
		    move_starttime(newval.get_time_or_duration()
			- operator[](ActivityInstance::START).get_time_or_duration(),
			true);
		    break;
		case ActivityInstance::NAME:
		    appearance_has_changed = true;
		    operator[](ActivityInstance::NAME) = newval;
		    break;
		case ActivityInstance::STATUS:
		    {
			TypedValue	val_to_use;
			if(newval.is_string()) {
			    if(newval.get_string() == "scheduled") {
				val_to_use = 1L;
			    } else if(newval.get_string() == "unscheduled") {
				val_to_use = 0L;
			    }
			} else if(newval.is_int()) {
			    val_to_use = newval;
			}
			if(!val_to_use.is_int()) {
			    Cstring		rmessage;
			    Cstring   	bad_status = newval.to_string();

			    rmessage << operator[](ActivityInstance::ID).get_string()
				<< " was given illegal status value " << bad_status << "\n";
			    throw(eval_error(rmessage));
			}
			if(!get_req()) {
			    return;
			}
			slist<alpha_void, smart_actptr>::iterator	children(
							get_req()->get_down_iterator());
			smart_actptr*					tptr;
			ActivityInstance*				child;

			appearance_has_changed = true;
			*oldvalptr = val_to_use;
			// in ACT_exec.C, ds line owner isDisplayedCorrectlyIn() should check status.
			while((tptr = children())) {
			    child = tptr->BP;
			    child->Object->assign_w_side_effects(ActivityInstance::STATUS, val_to_use);
			}
		    }
		    break;
		case ActivityInstance::LEGEND:
		    {
			Cstring		zeLegend = newval.get_string();
			LegendObject*	ldef = (LegendObject *) Dsource::theLegends().find(zeLegend);

			if(!ldef) {
			    ldef = LegendObject::LegendObjectFactory(
					zeLegend,
					operator[](ActivityInstance::PLAN).get_string(),
					ACTVERUNIT);
			}
			// OK if the following method is private:
			if(get_req()) {
			    get_req()->legend().switch_to_legend(ldef);
			    position_has_changed = true;
			} else {
			    *oldvalptr = newval;
			}
		    }
		    break;
		case ActivityInstance::PATTERN:
		    if(	(!newval.is_int())
			    || newval.get_int() < 0
			    || newval.get_int() >= NUM_OF_PATTERNS) {
			    Cstring rmessage("Activity ");

			    rmessage << operator[](ActivityInstance::ID).get_string() << " has illegal pattern " <<
					newval.get_int() << "\n";
			    throw(eval_error(rmessage));
		    } else {
			    *oldvalptr = newval;
			    appearance_has_changed = true;
		    }
		    break;
		case ActivityInstance::FINISH:
		    ;   //do nothing -- should not attempt to directly set "finish"
		    break;
		case ActivityInstance::cOLOR:
		    {
			int		color_index = 0;
			TypedValue*	c_symbol = &operator[](ActivityInstance::COLOR);
			TypedValue	v;

			if(!newval.is_string()) {
			    Cstring rmessage;
			    rmessage << "Activity " << operator[](ActivityInstance::ID).get_string()
					<< " has illegal color value " << newval.to_string();
			    rmessage << "\n";
			    throw(eval_error(rmessage));
			}
			color_index = find_color_id(newval.get_string());
			if(color_index > 0 && color_index <= NUM_OF_COLORS) {
			    *c_symbol = (long) color_index;
			    *oldvalptr = newval;
			    appearance_has_changed = true;
			} else {	Cstring rmessage;
			    rmessage << "Activity " << operator[](ActivityInstance::NAME).get_string()
					<< " has unknown color " << newval.get_string();
			    throw(eval_error(rmessage));
			}
		    }
		    break;
		case ActivityInstance::PLAN:
		    {
			Cstring pname = newval.get_string();
	
			*oldvalptr = newval;
			if(!eval_intfc::ListOfAllFileNames().find(pname)) {
			    eval_intfc::ListOfAllFileNames() << new emptySymbol(pname);
			}
		    }
		default:
		    {
			*oldvalptr = newval;
		    }
		    break;
	    }
	} // end of try block
	catch(eval_error Err) {
		Cstring	errs(operator[](ActivityInstance::ID).get_string());
		errs << ": assign_w_side_effects error; details:\n" << Err.msg;
		throw(eval_error(errs));
	}

	/* Having made the requested changes to a symbol of the activity instance, we face the problem
	 * of propagating the change to the places that need to know about it.
	 *
	 * The first, obvious case is that of symbols that directly affect the graphics. This, of course,
	 * only applies to instances of apcore built with atm_client; in other words, when the activity
	 * instance is really part of a high-level object that includes display information. So, we check
	 * for the presence of the higher-level stuff by looking at dataForDerivedClasses, which is NULL
	 * when running in server mode:
	 */

	if(get_req() && get_req()->dataForDerivedClasses && (position_has_changed || appearance_has_changed)) {

		/* The instance is already displayed; we need to update the right way...
		 * atm_client implements update methods which we invoke here: */
		if(position_has_changed) {
			get_req()->dataForDerivedClasses->handle_position();
		}
		if(appearance_has_changed) {
			get_req()->dataForDerivedClasses->handle_appearance();
		}
	}

}

time_actptr*	ActivityInstance::get_end_marker() const {
	slist<alpha_void, backptr<ActivityInstance>::ptr2p2b>::iterator	pointernodes(Pointers()->pointers);
	backptr<ActivityInstance>::ptr2p2b*				n;
	p2b<ActivityInstance>*						N;

	while((n = pointernodes())) {
		if((N = n->payload)) {
			time_actptr*	end_marker = dynamic_cast<time_actptr*>(N);

			if(end_marker && end_marker->list == &ACT_exec::ACT_subsystem().ActivityEnds) {
				return end_marker;
			}
		}
	}
	return NULL;
}

const Cstring& ActivityInstance::get_key() const {
	// static Cstring s;
	return (*Object)[NAME].get_string();
	// return s;
}

void ActivityInstance::set_scheduled(bool s) {
	TypedValue	val;

	val = s;
	Object->assign_w_side_effects(STATUS, val);
}

bool ActivityInstance::is_unscheduled() const {
	bool	is_it = false;
	const TypedValue&	tdv((*Object)[STATUS]);
	return !tdv.get_int();
}

bool ActivityInstance::has_decomp_method(apgen::METHOD_TYPE& mt) const {
	map<Cstring, int>::const_iterator iter = Object->Task.Type.taskindex.find("decompose");
	if(iter != Object->Task.Type.taskindex.end()) {
		mt = Object->Task.Type.tasks[iter->second]->prog->section_of_origin();
		return true;
	}
	return false;
}

CTime_base ActivityInstance::get_timespan() const {
	return (*Object)[SPAN].get_time_or_duration();
}

void act_object::set_timespan(const CTime_base& S) {
	TypedValue*	startval = &operator[](ActivityInstance::START);
	TypedValue*	spanval = &operator[](ActivityInstance::SPAN);
	TypedValue*	finishval = &operator[](ActivityInstance::FINISH);

	*spanval = S;
	*finishval = *startval->value.TM + S;
}

void act_object::move_starttime(
		const CTime_base& deltatime,
		bool drag_children) {

	// we want to allow parameters in "Start" values; these are set prior to insertion.
	if((deltatime == CTime_base(0, 0, true))) {
		return;
	}
	setetime(getetime() + deltatime);
	if(get_req()) {
		if(drag_children && get_req()->hierarchy().children_count()) {
			get_req()->hierarchy().move_all_descendants_by(deltatime);
		}
	}

	// if(dataForDerivedClasses) {
	// 	dataForDerivedClasses->handle_position();
	// }
}

void ActivityInstance::create() {
	Behavior&			a_type = Object->Task.Type;
	map<Cstring, int>::iterator	iter = a_type.taskindex.find("creation");

	if(iter != a_type.taskindex.end()) {
		task*			C = a_type.tasks[iter->second];
		act_method_object*	am = new act_method_object(this, *C);
		behaving_element	be(am);

		execution_context*	AC = new execution_context(
						be,
						C->prog.object(),
						C->prog->section_of_origin()
						);
		
		execution_context::return_code	Code = execution_context::REGULAR;

		try {
			//
			// NOTE: usage_clause_node::execute_usage uses an extension mechanism to
			// insert a "staleness handler" that will (hopefully) cause resource displays
			// for associative resources to refresh themselves.
			//

			AC->ExCon(Code);
		}
		catch(eval_error Err) {
			Cstring	rmessage;
			rmessage << "Error:\n" << Err.msg
				<< "\nThis error occurred during creation of activity "
				<< identify() << ".";
			throw(eval_error(rmessage));
		}
		delete AC;
	}
}

Cstring	ActivityInstance::report_value_of_this() {
	Cstring tmp = (*Object)[THIS].to_string();
	return tmp;
}

void	ActivityInstance::destroy() {
	Behavior&	a_type = Object->Task.Type;
	map<Cstring, int>::iterator iter = a_type.taskindex.find("destruction");
	if(iter != a_type.taskindex.end()) {
		task* C = a_type.tasks[iter->second];

		act_method_object*	am = new act_method_object(this, *C);
		behaving_element	be(am);

		execution_context*	AC = new execution_context(
						be,
						C->prog.object(),
						C->prog->section_of_origin()
						// getetime(),
						// getetime() + get_timespan()
						);
		execution_context::return_code	Code = execution_context::REGULAR;

		try {
			AC->ExCon(Code);
		}
		catch(eval_error Err) {
			Cstring rmessage(identify() + " destroy error:\n");
			rmessage << Err.msg;
			throw(eval_error(rmessage));
		}
		delete AC;
	}
}

apgen::RETURN_STATUS ActivityInstance::add_new_attributes_to_the_list(Cstring& any_errors) {
	return apgen::RETURN_STATUS::SUCCESS;
}

void ActivityInstance::handle_epoch_in_start_time(
		const Cstring	&epoch_name,
		TypedValue	*epoch_value,
		bool		extend_to_children) {
	if(epoch_value) {
		if(EpochIfApplicable != epoch_name) {
			EpochIfApplicable = epoch_name; } }
	else {
		EpochIfApplicable.undefine(); }
	if(extend_to_children) {
		slist<alpha_void, smart_actptr>::iterator	desc(get_down_iterator());
		smart_actptr*					ptr;
		ActivityInstance*					child;

		while((ptr = desc())) {
			child = ptr->BP;
			child->handle_epoch_in_start_time(epoch_name, epoch_value, true); } }
	}

#define PLOTATTNBR 5
#define PLOTATTMAXLEN 9		/* long enough for Duration\0 */
char plotattsallowed[PLOTATTNBR][PLOTATTMAXLEN]=
	{"Start", "Duration", "Color", "Pattern", "Legend" };

int	consult_act_req_for_attribute_value(
		const ActivityInstance*	a,
		const Cstring&	theN,
		TypedValue&	theV) {
	if(!a->Object->defines_property(theN)) {
		Cstring errs;
		errs << "instance " << a->get_unique_id() << " has no attibute named " << theN;
		throw(eval_error(errs));
	}
	const TypedValue&	attribute((*a->Object)[theN]);

	//
	// NOTE: we pass the EVALUATED form of the attribute value, so there should be no
	// complex symbolic things here; pE_extra::set_typed_symbol() should have no trouble
	// doing the job. In addition, act_object::side_effects() assumes that the tds arg
	// belongs to the ACT_req!!!
	theV = attribute;
	return 1;
}

void	ActivityInstance::get_param_info(
			int		ind_0,
			Cstring&	p_name,
			Cstring&	p_descr,
			TypedValue&	p_value,
			List&		range,
			apgen::DATA_TYPE& p_type) const {
	theConstActAgent()->get_param_info(ind_0, p_name, p_descr, p_value, range, p_type);
}

void ActivityInstance::set_APFplanname(
			const Cstring& newname) {
	TypedValue	val(apgen::DATA_TYPE::STRING);
			 
	val = newname;
	map<Cstring, int>::const_iterator name_ind = Object->Task.get_varindex().find("plan");
	assert(name_ind != Object->Task.get_varindex().end());
	Object->assign_w_side_effects(name_ind->second, val);
}

Cstring ActivityInstance::get_APFplanname() const {
	return (*Object)[PLAN].get_string();
}

void ActivityInstance::dump() const {
	theConstActAgent()->dump();
}

void ActivityInstance::transfer_to_stream(
			aoString&	Stream,
			apgen::FileType	file_type,
			CTime_base*	bounds,
			pinfotlist*	Pinfo) const {
	theConstActAgent()->transfer_to_stream(Stream, file_type, bounds, Pinfo);
}

#ifdef have_xmltol
void ActivityInstance::transfer_to_xmlpp(
		xmlpp::Element* el,
		bool AllActsVisible,
		CTime_base* bounds,
		const char* timesystem) const {
	theConstActAgent()->transfer_to_xmlpp(
					el,
					AllActsVisible,
					bounds,
					timesystem);
}
#endif /* have_xmltol */
	

bool ActivityInstance::is_command() const {
	const TypedValue&	val((*Object)[SASF]);
	ArrayElement*		ae;

	if(val.is_array() && (ae = val.get_array().find("type"))) {
		if(ae->Val().get_string() == "command") {
			return true;
		}
	}
	return false;
}

// useful for exporting plan from within APcore
void ActivityInstance::export_symbols(
		Cstring&			id,
		Cstring&			type,
		Cstring&			name,
		CTime_base&			start,
		tvslist&			Atts_w_official_tag,
		tvslist&			Atts_w_nickname,
		tvslist&			Params,
		stringslist&			Parent,
		stringslist&			Children,
		apgen::act_visibility_state&	V,
		Cstring&			subsystem) const {
	theConstActAgent()->export_symbols(
				id,
				type,
				name,
				start,
				Atts_w_official_tag,
				Atts_w_nickname,
				Params,
				Parent,
				Children,
				V,
				subsystem);
}

bool ActivityInstance::verify_smart_pointers(ActivityInstance* a, bool saved) {
	slist<alpha_void, backptr<ActivityInstance>::ptr2p2b>::iterator the_pointers(a->Pointers()->pointers);
	p2b<ActivityInstance>*						N;
	backptr<ActivityInstance>::ptr2p2b*				dpn;

	while((dpn = the_pointers())) {
		N = dpn->payload;
		smart_actref*	Ref = dynamic_cast<smart_actref*>(N);
		if(Ref) {
			if(saved) {
				assert(Ref->saved_list && !Ref->list);
				Ref->saved_list->check();
			} else {
				assert((!Ref->saved_list) && Ref->list);
				Ref->list->check();
			
			}
		}
	}
	return true;
}
	
void ActivityInstance::action_callback(void *theExecutive, list_event whatHappened) {
	slist<alpha_void, backptr<ActivityInstance>::ptr2p2b>::iterator the_pointers(Pointers()->pointers);
	backptr<ActivityInstance>::ptr2p2b*			dpn;
	p2b<ActivityInstance>*					N;

	switch(whatHappened) {
		case AFTER_NODE_INSERTION:
			if(insertion_state == need_to_reattach_pointers) {
				while((dpn = the_pointers())) {
					N = dpn->payload;
					if(N->reacts_to_time_events()) {
						N->re_insert();
					}
				}
			} else {
				assert(insertion_state == unattached);
			}
			insertion_state = in_a_list;
			break;
		case AFTER_NODE_REMOVAL :
			assert(insertion_state == is_about_to_be_removed);
			insertion_state = temporarily_unattached;
			break;
		case AFTER_FINAL_NODE_REMOVAL :
			assert(insertion_state == is_about_to_be_removed);
			insertion_state = unattached;
			break;
		case BEFORE_NODE_INSERTION :
			if(insertion_state == temporarily_unattached) {
				// pointers were removed from their list
				insertion_state = need_to_reattach_pointers;
			} else {
				assert(insertion_state == unattached);
			}
			break;
		case BEFORE_NODE_REMOVAL :
			assert(insertion_state == in_a_list);

			while((dpn = the_pointers())) {
				N = dpn->payload;
				if(N->reacts_to_time_events()) {
					N->remove();
				}
			}
			insertion_state = is_about_to_be_removed;
			break;
		case BEFORE_FINAL_NODE_REMOVAL:
			insertion_state = is_about_to_be_removed;
			break;
		default:
			break;
	}
}

void act_object::setetime(const CTime_base& T) {
	TypedValue*	span_symbol = &operator[](ActivityInstance::SPAN);
	TypedValue*	finish_symbol= &operator[](ActivityInstance::FINISH);
	TypedValue*	start_value = &operator[](ActivityInstance::START);

	*start_value = T;
	*finish_symbol = *start_value->value.TM + *span_symbol->value.TM;
}

void ActivityInstance::setetime(const CTime_base& T) {
	obj()->setetime(T);
}

// arg. defaults to 0 (see .h):
Cstring ActivityInstance::write_start_time(int epoch_only) const {
	CTime_base	ctime_start = getetime();
	Cstring		cstring_start;

	if(EpochIfApplicable.length()) {
		TypedValue& tdv = globalData::get_symbol(EpochIfApplicable);

		if(globalData::isAnEpoch(EpochIfApplicable)) {
			CTime_base	epoch_time = tdv.get_time_or_duration();

			if(ctime_start >= epoch_time)
				cstring_start << EpochIfApplicable << " + "
					<< (ctime_start - epoch_time).to_string();
			else
				cstring_start << EpochIfApplicable << " - "
					<< (epoch_time - ctime_start).to_string(); }
		else if(!epoch_only) {
			ListOVal*	tv = &tdv.get_array();
			CTime_base	offset = tv->find("origin")->Val().get_time_or_duration();
			double		multiplier = tv->find("scale")->Val().get_double();
			CTime_base	delta = (ctime_start - offset) / multiplier;

			if(delta >= CTime_base(0, 0, true)) {
				cstring_start << "\"" << EpochIfApplicable << "\":"
					<< delta.to_string(); }
			else {
 
				cstring_start << "-\"" << EpochIfApplicable << "\":"
					<< (-delta).to_string(); } }
		else {
			cstring_start = ctime_start.to_string(); } }
	else {
		cstring_start = ctime_start.to_string(); }
	return cstring_start; }

const Cstring& ActivityInstance::getTheLegend() const {
	static Cstring temp;
	if(theLegendHandler.get_legend_object()) {
		return theLegendHandler.get_legend_object()->get_key();
	} else {
		const TypedValue tdv((*Object)[LEGEND]);
		temp = tdv.get_string();
		return temp;
	}
}

int	ActivityInstance::getTheLegendIndex() const {
	static LegendObject	*LO;

	if(!(LO = theLegendHandler.get_legend_object()))
		LO = (LegendObject *) Dsource::theLegends().find(getTheLegend());
	if(LO)
		return LO->get_index();
	else
		return 0;
}

const CTime_base& ActivityInstance::getetime() const {
	return obj()->getetime();
}

const CTime_base& act_object::getetime() const {
	return *operator[](ActivityInstance::START).value.TM;
}

bool ActivityInstance::is_SASF_enabled() const {
	return true;
}

int ActivityInstance::get_SASF_attribute(const Cstring &keyword, Cstring &value) const {
	const TypedValue*	theSASFAttributeValue = NULL;
	const TypedValue*	theSasfAttrValue = NULL;
	bool			ok_so_far = false;
	Cstring			any_errors;

	//Only instances with "SASF" attribute present, as a non-null
	//  string, are eligible for writing to an SASF file.
	const TypedValue&	attrib_val((*Object)[SASF]);
	if (((theSASFAttributeValue = &attrib_val)->is_array())
	  && (theSASFAttributeValue->get_array().get_length())) {
		ok_so_far = true;
	}
	if(!ok_so_far) {
		return 0;
	}
	//Instances with "SASF File" attribute present, as a non-null
	//  string, are eligible for writing to an file with that name;
	//  all other circumstances default to the standard output file.
	if(	(theSASFAttributeValue->get_array()[keyword] != NULL)
		&& (theSasfAttrValue = &theSASFAttributeValue->get_array()[keyword]->Val())->is_string()
		&& theSasfAttrValue->get_string().length()) {
		ok_so_far = true;
	} else {
		ok_so_far = false;
	}
	if(!ok_so_far) {
		return 0;
	}
	value = theSasfAttrValue->get_string();
	return 1;
}

execution_context::return_code ActivityInstance::exercise_decomposition(
		bool skip_wait /* = false */,
		pEsys::execStack* use_this_stack /* = NULL */ ) {
	try {
		return theActAgent()->exercise_decomposition(skip_wait, use_this_stack);
	} catch(eval_error Err) {
		Cstring errs = identify() + " instantiation error:\n" + Err.msg;
		throw(eval_error(errs));
	}
}

	/*
	 * fills L with Symbol_nodes with appropriate keyword/value pairs
	 */
void ActivityInstance::get_optional_SASF_attributes(List &L) const {
	const TypedValue*	theSASFAttributeValue = NULL;
	ListOVal*		theArray;
	Cstring			any_errors;

	L.clear();

	//Only instances with "SASF" attribute present, as a non-null
	//  string, are eligible for writing to an SASF file.
	try {
		const TypedValue&	sasfAtt((*Object)[SASF]);
		if(	(theSASFAttributeValue = &sasfAtt)->is_array()
		    &&  (theSASFAttributeValue->get_array().get_length())) {
			theArray = &theSASFAttributeValue->get_array();
			ArrayElement*					attrib_val;

			for(int k = 0; k < theArray->get_length(); k++) {
				attrib_val = (*theArray)[k];
				if(!theStandardSASFelementNames().find(attrib_val->get_key())) {
					L << new Symbol_node(attrib_val->get_key(),
						attrib_val->Val().get_string());
				}
			}
		}
	}
	catch(eval_error) {
		;
	}
}

execution_context::return_code ActivityInstance::exercise_modeling(
		bool	skip_wait /* = false */,
		pEsys::execStack* use_this_stack /* = NULL */ ) {
	return theActAgent()->exercise_modeling(skip_wait, use_this_stack);
}

bool ActivityInstance::has_editable_duration() const {
	return false;
}

using namespace apgen;
using namespace pEsys;

act_object::act_object(
		ActivityInstance* a,
		const Cstring& name,
		const Cstring& proposed_id,
		Behavior& act_type,
		ActInstance& my_parsed_source)
	: act(a),
		parsedThis(&my_parsed_source),
		behaving_object(*act_type.tasks[Behavior::CONSTRUCTOR]) {
	act_obj_count++;
	operator[](ActivityInstance::NAME) = name;
	operator[](ActivityInstance::ID) = proposed_id;
	// define a few basics so we can execute the attribute declarations
	operator[](ActivityInstance::SPAN) = CTime_base("00:01:40");
	operator[](ActivityInstance::START) = CTime_base("2020-020T02:02:02");
	operator[](ActivityInstance::FINISH) = CTime_base("2020-020T02:03:42");
	operator[](ActivityInstance::LEGEND) = "Generic Activities";
	operator[](ActivityInstance::STATUS) = true;
	Cstring temp(name);
	operator[](ActivityInstance::TYPE) = act_type.name;
	behaving_element be;
	operator[](ActivityInstance::THIS) = be;
	operator[](ActivityInstance::THIS).get_object().reference(this);
}

act_object::act_object(
		ActivityInstance* a,
		const Cstring& name,
		const Cstring& proposed_id,
		Behavior& act_type)
	: act(a),
		behaving_object(*act_type.tasks[Behavior::CONSTRUCTOR]) {

	//
	// This object obviously was not created from a file,
	// so its parsedThis member should be void
	//

	// if(a) {
	// 	act_object* ao = dynamic_cast<act_object*>(a->Object.object());
	// 	if(ao->parsedThis) {
	// 		parsedThis.reference(ao->parsedThis.object());
	// 	}
	// }
	act_obj_count++;
	operator[](ActivityInstance::NAME) = name;
	operator[](ActivityInstance::ID) = proposed_id;
	// define a few basics so we can execute the attribute declarations
	operator[](ActivityInstance::SPAN) = CTime_base("00:01:40");
	operator[](ActivityInstance::START) = CTime_base("2020-020T02:02:02");
	operator[](ActivityInstance::FINISH) = CTime_base("2020-020T02:03:42");
	operator[](ActivityInstance::LEGEND) = "Generic Activities";
	operator[](ActivityInstance::STATUS) = true;
	Cstring temp(name);
	operator[](ActivityInstance::TYPE) = act_type.name;
	behaving_element be;
	operator[](ActivityInstance::THIS) = be;
	operator[](ActivityInstance::THIS).get_object().reference(this);
}

//
// declared private in class definition
//

act_object::act_object(const act_object& EC)
	: act(EC.act),
		behaving_object(*EC.Type.tasks[0]) {
	act_obj_count++;
	// debug
	cout << " NO NO NO - act_obj count " << act_obj_count << "\n";
}

act_object::~act_object() {
	act_obj_count--;
}

behaving_element act_object::get_parent() {
	return get_req()->get_parent()->Object;
}
