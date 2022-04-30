#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "ActivityInstance.H"
#include "action_request.H"
#include "Legends.H"
#include "RES_exec.H"
#include "UI_dsconfig.h"

using namespace std;

			// implemented below:
apgen::DATA_TYPE	correct_type(const Cstring &temp);

hierarchy_member::hierarchy_member(apgen::METHOD_TYPE dt, ActivityInstance *P):
		myboss(P),
		upIterator(superactivities),
		downIterator(subactivities),
		up_accelerator(P)
       		{}

hierarchy_member::hierarchy_member(const hierarchy_member& hm):
		myboss(hm.myboss),
		upIterator(superactivities),
		downIterator(subactivities),
		up_accelerator(hm.up_accelerator)
		{}

void hierarchy_member::move_all_descendants_by(CTime_base delta) {
	slist<alpha_void, smart_actptr>::iterator	children(subactivities);
	smart_actptr*					tptr;
	dumb_actptr*					dumb;
	slist<alpha_void, dumb_actptr>			theCopy;
	slist<alpha_void, dumb_actptr>::iterator	theCopyOfTheChildren(theCopy);
	ActivityInstance*				child;

	while((tptr = children())) {
		theCopy << new dumb_actptr(tptr->BP);
	}
	while((dumb = theCopyOfTheChildren())) {
		child = dumb->payload;
		/*
		 * This is not going to work, because Time_ptrnodes will be automatically
		 * removed from their lists while children are moved...
		 */
		child->obj()->move_starttime(delta, 0);
		child->hierarchy().move_all_descendants_by(delta);
	}
}

bool PR_manager::is_brand_new() const {
	return myboss->is_in_list(eval_intfc::get_act_lists().someBrandNewActivities);
}

bool PR_manager::is_active() const {
	return myboss->is_in_list(eval_intfc::get_act_lists().someActiveInstances);
}

bool PR_manager::is_weakly_active() const {
	ActivityInstance* P;
	apgen::METHOD_TYPE mt;

	if(is_active()) {
		return true; }
	else if(is_abstracted()) {
		P = myboss->get_parent();
		while(	P && P->has_decomp_method(mt) &&
			(mt == apgen::METHOD_TYPE::NONEXCLDECOMP
			)
		     ) {
			if(P->agent().is_active()) {
				return true;
			}
			P = P->get_parent();
		}
		return false;
	} else if(is_decomposed()) {
		P = myboss;
		while(	P->has_decomp_method(mt) &&
			(mt == apgen::METHOD_TYPE::NONEXCLDECOMP
		        )
		     ) {
			slist<alpha_void, smart_actptr>::iterator li(P->get_down_iterator());
			smart_actptr*	p = li();

			if(p) {
				P = p->BP;
				if(P->agent().is_active()) {
					return true;
				}
			}
			else {
				// no children, decomposed... we must be severing a link.
				return true;
			}
		}
		return false;
	}
	return false;
}


bool PR_manager::is_decomposed() const {
	return myboss->is_in_list(eval_intfc::get_act_lists().someDecomposedActivities);
}

bool PR_manager::is_abstracted() const {
	return myboss->is_in_list(eval_intfc::get_act_lists().someAbstractedActivities);
}

bool PR_manager::is_on_clipboard() const {
	return myboss->is_in_list(eval_intfc::get_act_lists().somePendingActivities);
}

ActivityInstance* PR_manager::put_a_copy_of_me_into_clipboard() const {
	slist<alpha_void, smart_actptr>::iterator	li(myboss->get_down_iterator());
	smart_actptr*					p;
	ActivityInstance				*a, *b, *c;

	// may throw; good luck...
	c = new ActivityInstance(*myboss);
	c->set_scheduled(false);
	while((p = li())) {
		instance_state_changer	ISC(c);

		a = p->BP;
		b = a->agent().put_a_copy_of_me_into_clipboard();
		ISC.set_desired_child_to(b);
		ISC.do_it(NULL);
	}
	eval_intfc::get_act_lists().somePendingActivities << c;
	assert(c->insertion_state == Dsource::in_a_list);
	return c;
}

void PR_manager::move_to_decomposed_list() {
	if(!is_decomposed()) {
		myboss->unselect();
		eval_intfc::get_act_lists().someDecomposedActivities << myboss;
		assert(myboss->insertion_state == Dsource::in_a_list);
		} }

void	PR_manager::move_to_abstracted_list() {
	if(!is_abstracted()) {
		myboss->unselect();
		eval_intfc::get_act_lists().someAbstractedActivities << myboss;
		assert(myboss->insertion_state == Dsource::in_a_list);
	}
}

void	PR_manager::move_to_active_list() {
	if(!is_active()) {
		eval_intfc::get_act_lists().someActiveInstances << myboss;
		assert(myboss->insertion_state == Dsource::in_a_list);
	}
}

void	PR_manager::move_to_brand_new_list() {
	if(!is_brand_new()) {
		eval_intfc::get_act_lists().someBrandNewActivities << myboss;
		assert(myboss->insertion_state == Dsource::in_a_list);
	}
}

bool hierarchy_member::can_be_decomposed_without_errors(
		bool redetail) {
	apgen::METHOD_TYPE mt;

	if(redetail) {
		if(myboss->has_decomp_method(mt)) {
		    switch(mt) {
			case apgen::METHOD_TYPE::NONE:
			case apgen::METHOD_TYPE::CONCUREXP:
				return false;
			case apgen::METHOD_TYPE::DECOMPOSITION:
			case apgen::METHOD_TYPE::NONEXCLDECOMP:
			default:
				return true;
		    }
		}
	} else {
		if( subactivities.get_length() ) {
			return true;
		}
		if(myboss->has_decomp_method(mt)) {
		    switch(mt) {
			case apgen::METHOD_TYPE::NONE:
			case apgen::METHOD_TYPE::CONCUREXP:
				return false;
			case apgen::METHOD_TYPE::DECOMPOSITION:
			case apgen::METHOD_TYPE::NONEXCLDECOMP:
			default:
				return true;
		    }
		}
	}
	// dummy
	return false;
}

void hierarchy_member::update_up_accelerator(ActivityInstance *a, bool Select) {
	slist<alpha_void, smart_actptr>::iterator	li(subactivities);
	smart_actptr*					bptr;

	if(Select) {
		myboss->select();
		while((bptr = li())) {
			ActivityInstance* b = bptr->BP;
			b->hierarchy().update_up_accelerator(a, true); } }
	else {
		up_accelerator = a;
		while((bptr = li())) {
			ActivityInstance* b = bptr->BP;
			b->hierarchy().update_up_accelerator(a); } } }

void hierarchy_member::select_hierarchy(bool down_only) {
	if(down_only) {
		update_up_accelerator(NULL, true);
	} else {
		up_accelerator->hierarchy().update_up_accelerator(NULL, true);
	}
}

int	hierarchy_member::attach_to_parent(hierarchy_member &newparent) {
	if(superactivities.get_length()) {
		// already a child
		return 0;
	}
	if(newparent.subactivities.find((void *)myboss)) {
		// already a child of newparent (would imply inconsistent
		// links, so redundant check, but what the hell...)
		return 0;
	}
	assert(newparent.myboss != myboss);
	superactivities << new smart_actptr(newparent.myboss);
	(*myboss->Object)[ActivityInstance::PARENT] = newparent.myboss->Object;
	newparent.subactivities << new smart_actptr(myboss);
	update_up_accelerator(newparent.up_accelerator);
	return 1;
}

const Cstring& PR_manager::label_to_use_on_display() const {
	static Cstring	nul("");
	static Cstring temp;

	try {
		TypedValue	tdv = (*myboss->Object)["label"];
		temp = tdv.get_string();
		// NOTE: ephemeral copy... use quickly.
		return temp;
	}
	catch(eval_error) {
		return nul;
	}
}

legend_member::legend_member(ActivityInstance* p)
		: myboss(p),
		vmoveflg(0)
       		{}

	/* This used to just return theLegend. In the 'old days' before request instances, this
	 * was just fine. The problem occurs now when we read an APF that contains requests:
	 * the request needs its children to figure out what its legend is, but the children
	 * are abstracted and have no legend object!! This problem does not occur when the
	 * request is created 'on the fly', because even though the children have been abstracted
	 * they still remember 'theLegend' (they haven't been cut).
	 *
	 * Hopefully the following will work...
	 */
LegendObject*	legend_member::get_legend_object() const {
	Pointer_node*	p = (Pointer_node *) zero_or_one_legend.first_node();
	LegendObject*	ldef = NULL;

	if(p) {
		ldef = (LegendObject *) p->get_ptr();
	} else {
		try {
			TypedValue	tdv = (*myboss->Object)["legend"];
			ldef = (LegendObject *) Dsource::theLegends().find(tdv.get_string());
		}
		catch(eval_error) {
		}
	}
	return ldef;
}

void legend_member::attach_to_legend(LegendObject* new_legend) {
	Pointer_node* p = (Pointer_node *) zero_or_one_legend.first_node();

	if((!p) || new_legend != (LegendObject *) p->get_ptr()) {

		assert(myboss->Object->defines_property("legend"));

		LegendObject	*ldef, *theLegend = NULL;
		TypedValue&	tdv((*myboss->Object)["legend"]);
		Cstring		dlegend;
		Pointer_node*	ptr = (Pointer_node *) zero_or_one_legend.first_node();
		static Cstring	dummy_errors;

		if(ptr) {
			theLegend = (LegendObject *) ptr->get_ptr();
		}
		if(new_legend) {
			dlegend = new_legend->get_key();
			ldef = new_legend;
		} else {
			assert(myboss->Object->defines_property("legend"));
			if(tdv.get_string().length()) {
				dlegend = tdv.get_string();
			} else {
				dlegend = GENERICLEGEND;
			}
			ldef = (LegendObject *) Dsource::theLegends().find(dlegend);
			if(!ldef) {
				ldef = LegendObject::LegendObjectFactory(
						dlegend,
						myboss->get_APFplanname(),
						ACTVERUNIT);
			}
		}
		tdv = dlegend;

		if(theLegend && theLegend != ldef) {
			smart_actptr* pnode = theLegend->ActivityPointers.find((void *) myboss);

			zero_or_one_legend.clear();
			if(pnode) delete pnode;
		}

		zero_or_one_legend << new Pointer_node(ldef, ldef);

		if(!ldef->ActivityPointers.find((void*) myboss) ) {
			ldef->ActivityPointers << new smart_actptr(myboss);
		}
	}
}

void legend_member::switch_to_legend(LegendObject* ldef) {
	LegendObject*	theLegend = NULL;
	Pointer_node*	ptr = (Pointer_node *) zero_or_one_legend.first_node();

	if(ptr) {
		theLegend = (LegendObject *) ptr->get_ptr();
	}

	DBG_INDENT(myboss->get_unique_id() << "->switch_to_legend(" << ldef->get_key()
			<< ") START\n");
	if(ldef == theLegend) {
		smart_actptr*	pnode = theLegend->ActivityPointers.find((void*) myboss);
		if(pnode) {
			DBG_UNINDENT("ACT_req(\"" << myboss->get_unique_id()
				<< "\")::switch_to_legend: same legend -- doing NOTHING\n");
			return;
		}
	}
	// OBSERVER PATTERN
	attach_to_legend(ldef);
	if(myboss->dataForDerivedClasses) {
		myboss->dataForDerivedClasses->handle_legend(ldef);
	}
	DBG_UNINDENT("ACT_req(\"" << myboss->get_unique_id()
		<< "\")::switch_to_legend: END\n");
}

void hierarchy_member::recursively_get_ptrs_to_all_descendants(
		tlist<alpha_void, dumb_actptr>& l) {
	smart_actptr*					bptr;
	slist<alpha_void, smart_actptr>::iterator	subs(subactivities);

	while((bptr = subs())) {
		ActivityInstance* act = bptr->BP;
		if(!l.find(act) )
			l << new dumb_actptr(act);
		act->hierarchy().recursively_get_ptrs_to_all_descendants(l);
	}
}

void	hierarchy_member::recursively_get_time_ptrs_to_nonexclusive_descendants(
		tlist<alpha_time, Cnode0<alpha_time, ActivityInstance*> >& L) {
	smart_actptr*					ptr;
	ActivityInstance*				req;
	slist<alpha_void, smart_actptr>::iterator	children(subactivities);
	apgen::METHOD_TYPE				mt;

	if(	myboss->has_decomp_method(mt) &&
		(mt == apgen::METHOD_TYPE::NONEXCLDECOMP
		)
	  ) {
		while((ptr = children())) {
			req = ptr->BP;
			L << new Cnode0<alpha_time, ActivityInstance*>(req->getetime(), req);
			req->hierarchy().recursively_get_time_ptrs_to_nonexclusive_descendants(L);
		}
	}
}

