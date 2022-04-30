#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define apDEBUG

#include "apDEBUG.H"
#include <ActivityInstance.H>
#include "C_global.H"

using namespace std;

//
// This class captures the 'state' of an activity instance through
// a number of 'state attributes':
//
//    -	invisibility, a member of the apgen::act_visibility_state enum;
//    	possible values:
//
//	enum member			visible?	modeled?
//	===========			========	========
//
//    	VISIBILITY_REGULAR 		yes		yes
//
//	VISIBILITY_DECOMPOSED		no		if non-exclusive
//
//	VISIBILITY_ABSTRACTED		no		if non-exclusive
//
//	VISIBILITY_BRAND_NEW		no		no
//
//	VISIBILITY_NEWCHILDREN		N/A -  transient state, used to request children regeneration
//
//	VISIBILITY_ON_CLIPBOARD		no		no
//
act_instance_state::act_instance_state(ActivityInstance *ACT_req_1)
	: child(NULL) {
	invisibility = apgen::act_visibility_state::VISIBILITY_REGULAR;
	if(ACT_req_1->agent().is_decomposed()) {
		invisibility = apgen::act_visibility_state::VISIBILITY_DECOMPOSED;
	} else if(ACT_req_1->agent().is_abstracted()) {
		invisibility = apgen::act_visibility_state::VISIBILITY_ABSTRACTED;
	} else if(ACT_req_1->agent().is_on_clipboard()) {
		invisibility = apgen::act_visibility_state::VISIBILITY_ON_CLIPBOARD;
	} else if(ACT_req_1->agent().is_brand_new()) {
		invisibility = apgen::act_visibility_state::VISIBILITY_BRAND_NEW;
	}

	resolution_case = 0;

	parent = ACT_req_1->get_parent();
	has_offspring = (ACT_req_1->hierarchy().children_count() > 0);
}

Cstring act_instance_state::to_string() {
	Cstring s("(vis = ");
	switch(invisibility) {
		case apgen::act_visibility_state::VISIBILITY_REGULAR:
			s << "REGULAR";
			break;
		case apgen::act_visibility_state::VISIBILITY_DECOMPOSED:
			s << "DECOMPOSED";
			break;
		case apgen::act_visibility_state::VISIBILITY_ABSTRACTED:
			s << "ABSTRACTED";
			break;
		case apgen::act_visibility_state::VISIBILITY_ON_CLIPBOARD:
			s << "ON_CLIPBOARD";
			break;
		case apgen::act_visibility_state::VISIBILITY_BRAND_NEW:
			s << "BRAND_NEW";
			break;
		default:
			s << "??";
	}
	if(parent) {
		s << ", parent = " << parent->get_unique_id();
	}
	s << ", has_offspring = ";
	if(has_offspring) {
		s << "true";
	} else {
		s << "false";
	}
	if(child) {
		s << ", child = " << child->get_unique_id();
	}
	s << ")";
	return s;
}

void instance_state_changer::create_missing_nonexcl_children() {
	bool creation_has_been_handled = false;
	execution_context::return_code Code = execution_context::REGULAR;
	apgen::METHOD_TYPE	mt;

	try {
	    if(ACT_req_changing->theActAgent()->hasType()) {
		if(	!ACT_req_changing->hierarchy().children_count()
		    	&& (	ACT_req_changing->has_decomp_method(mt) &&
				(mt == apgen::METHOD_TYPE::NONEXCLDECOMP)
			   )
		  ) {

			try {
				creation_has_been_handled = true;
				ACT_req_changing->create();
				ACT_req_changing->exercise_decomposition(Code);
			}
			catch(eval_error Err) {
				throw(Err);
			}
		}
	    }
	} catch(eval_error Err) {
		throw(Err);
	}
	catch(decomp_error Err) {
		throw(Err);
	}
	if(!creation_has_been_handled) {
		ACT_req_changing->create();
	}
	return;
}

void instance_state_changer::instantiate_new_act() {
	bool	was_brand_new = ACT_req_changing->agent().is_brand_new();

	assert(was_brand_new);

	bool creation_has_been_handled = false;

	try {
	    switch(desired_state.invisibility) {
		case apgen::act_visibility_state::VISIBILITY_ON_CLIPBOARD:
		case apgen::act_visibility_state::VISIBILITY_NEWCHILDREN:

		    //
		    // should never happen. The visibility attribute is
		    // determined by whatever it was in the APF which
		    // APGenX just read, and that attribute can only
		    // have one of the three values below.
		    //
		    assert(false);
		    break;

		case apgen::act_visibility_state::VISIBILITY_DECOMPOSED:

		    eval_intfc::get_act_lists().someDecomposedActivities << ACT_req_changing;
		    assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
		    break;

		case apgen::act_visibility_state::VISIBILITY_ABSTRACTED:

		    eval_intfc::get_act_lists().someAbstractedActivities << ACT_req_changing;
		    assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
		    break;

		case apgen::act_visibility_state::VISIBILITY_REGULAR:

		    eval_intfc::get_act_lists().someActiveInstances << ACT_req_changing;
		    assert(ACT_req_changing->insertion_state == Dsource::in_a_list);

		    if(ACT_req_changing->agent().is_active()) {
			if(ACT_req_changing->dataForDerivedClasses) {
			    ACT_req_changing->dataForDerivedClasses->handle_instantiation(true, true);
			}
		    }
		    break;

		default:

		    break;
	    }
	} catch(eval_error Err) {
		throw(Err);
	}
}

//
// This is a very complex method because it must handle all possible
// transitions in the state of an activity instance. Over time, the
// logic became too complex. Starting with APGenX, the logic has been
// simplified to the extent that it could while keeping the behavior
// of APGenX the same as that of APGen.
//
// Desired transitions are expressed via two members of the
// instance_state_changer class, both of them instances of
// act_instance_state. See the comments above the act_instance_state
// constructor in this file. One instance is called current_state,
// the other desired_state. These instances capture the 'state' of
// the activity instance as a number of state attributes.
//
// One reason the code used to be complex is that the current and
// desired states could differ in more than one state attribute,
// resulting in a large number of possible combinations which had
// to be identified, then dealt with appropriately.
//
void	instance_state_changer::do_it(
		pEsys::execStack*	stack_to_use /* ,
		bool			exercise_constructors = true */ ) {
	// apgen::RETURN_STATUS	ret = apgen::RETURN_STATUS::SUCCESS;
	bool			was_brand_new = ACT_req_changing->agent().is_brand_new();
	apgen::METHOD_TYPE	mt;
	bool			has_decomp_method = ACT_req_changing->has_decomp_method(mt);

	if(was_brand_new) {
		instantiate_new_act();
		create_missing_nonexcl_children();
		return;
	}

	//
	// debug
	//
	// dbgind	indenter;
	// cout << dbgind::indent() << "do_it() - " << ACT_req_changing->get_unique_id() << "\n";


	execution_context::return_code Code = execution_context::REGULAR;
	bool	exercise_constructors = true;

	if(ACT_req_changing->is_unscheduled()) {
		exercise_constructors = false;
	}

	//
	// compute weakly active status before severing from parent
	//
	//	DANGER: is_weakly_active will loop forever if
	// 	the instance hierarchy is in a transient state,
	// 	e. g. being constructed.
	//
	bool	was_weakly_active = ACT_req_changing->agent().is_weakly_active();

	//
	// debug
	//
	// cout << dbgind::indent() << "act was weakly active: " << was_weakly_active << "\n";

	// wrong thing to do when putting this act on the clipboard:
	if(desired_state.invisibility != apgen::act_visibility_state::VISIBILITY_ON_CLIPBOARD) {
		if(ACT_req_changing->get_parent() && !desired_state.parent) {
			/* This has pretty serious consequences, but does NOT
			 * affect the visibility of ACT_req_changing: */

			//
			// debug
			//
			// dbgind	indenter;
			// cout << dbgind::indent() << "calling sever_from_parent\n";

			sever_from_parent(stack_to_use);
		}
	}

	// SECTION 1: destroy children if necessary

	if(	ACT_req_changing->hierarchy().children_count()
		&&	(
			(!desired_state.has_offspring)
			|| desired_state.invisibility == apgen::act_visibility_state::VISIBILITY_NEWCHILDREN
			)
	  ) {

		//
		// debug
		//
		// cout << dbgind::indent() << "Destroying children\n";

		smart_actptr*		ptrnode = NULL ;
		ActivityInstance*	ACT_req_1 ;

		// first check whether our client also wants to detail the instance...
		if(	desired_state.invisibility == apgen::act_visibility_state::VISIBILITY_DECOMPOSED
			|| desired_state.invisibility == apgen::act_visibility_state::VISIBILITY_NEWCHILDREN) {
			if(	(!ACT_req_changing->theActAgent()->hasType())
				|| !has_decomp_method
				|| mt == apgen::METHOD_TYPE::CONCUREXP
			  ) {
				Cstring rmessage;
				rmessage << "instance " << ACT_req_changing->identify()
					<< " cannot be redetailed. ";
				throw(decomp_error(rmessage));
			}
		}
		while((ptrnode = ACT_req_changing->get_down_iterator().const_first())) {
			ACT_req_1 = ptrnode->BP;
			instance_state_changer		ISC(ACT_req_1);

			ISC.set_desired_offspring_to(false);
			ISC.do_it(stack_to_use);

			//
			// let's not overdo it. If unscheduling, for instance, we have already
			// 'destroyed' the activity when calling 'move_from_decomposed_to_active_list().'
			//
			if(	ACT_req_1->agent().is_active() ||
				(has_decomp_method &&
				    (mt == apgen::METHOD_TYPE::NONEXCLDECOMP)
				)
			  ) {
				ACT_req_1->destroy();
			}
			try {
				destroy_an_act_req(ACT_req_1);
			} catch(eval_error Err) {
				Cstring errs;
				errs << "destruction error while trying to remove old children of "
					<< ACT_req_changing->get_unique_id()
					<< "; error:\n\t" << Err.msg << "\n";
				throw(decomp_error(errs));
			}
		}
		assert(ACT_req_changing->hierarchy().subactivities.get_length() == 0);
		// SMART pointers have been deleted already:
		//subactivities.clear() ;
	}

	// SECTION 2: handle straightforward cases

	if(desired_state.child) {

		//
		// debug
		//
		// cout << dbgind::indent() << "\"straightforward case\": calling add_a_child()\n";

		add_a_child(stack_to_use, desired_state.child);
	}

	// if(desired_state.resolution_case > 0) {
	// 	resolve(desired_state.resolution_case);
	// 	return;
	// }

	if(desired_state.parent) {
		assert(desired_state.parent != ACT_req_changing);

		//
		// debug
		//
		// cout << dbgind::indent() << "New state requires calling attach_to_parent\n";

		ACT_req_changing->hierarchy().attach_to_parent(desired_state.parent->hierarchy());
	}

	if(desired_state.invisibility == current_state.invisibility) {

		//
		// debug
		//
		// cout << dbgind::indent() << "current state has the right visibility; returning\n";
		return;
	} else {
		//
		// debug
		//
		// cout << dbgind::indent() << "current state does NOT have the right visibility; continuing\n";
	}

	hierarchy_member&	theHierarchy = ACT_req_changing->hierarchy();

	// SECTION 3:	insert the changing activity in the appropriate list, if necessary
	//		(i. e. if the act is brand new or if it was on the clipboard)
	//		In either case, we will return within the next two ifs.

	if(ACT_req_changing->agent().is_on_clipboard()) {

		bool creation_has_been_handled = false;

		try {
	    	    switch(desired_state.invisibility) {
			case apgen::act_visibility_state::VISIBILITY_ON_CLIPBOARD:
				break;
			case apgen::act_visibility_state::VISIBILITY_NEWCHILDREN:
				{
				Cstring rmessage;
				rmessage << ACT_req_changing->identify() << " is on clipboard; cannot "
					<< "regenerate children. ";
				throw(decomp_error(rmessage));
				}
				break;
			case apgen::act_visibility_state::VISIBILITY_DECOMPOSED:
				{
				Cstring rmessage;
				rmessage << ACT_req_changing->identify() << " is on clipboard; cannot "
					<< "decompose. ";
				throw(decomp_error(rmessage));
				}
				break;
			case apgen::act_visibility_state::VISIBILITY_ABSTRACTED:
				eval_intfc::get_act_lists().someAbstractedActivities << ACT_req_changing;
				assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
				if(ACT_req_changing->theActAgent()->hasType()) {
					if(	!ACT_req_changing->hierarchy().children_count()
					    	&& (has_decomp_method &&
							(mt == apgen::METHOD_TYPE::NONEXCLDECOMP)
						   )
					  ) {
						try {
							creation_has_been_handled = true;

							//
							// THIS IS WRONG if creation has already been exercised!
							//
							ACT_req_changing->create();
							ACT_req_changing->exercise_decomposition(Code);
						}
						catch(eval_error Err) {
							throw(Err);
						}
					}
				}
				if(ACT_req_changing->hierarchy().children_count()) {
					slist<alpha_void, smart_actptr>::iterator
						li(ACT_req_changing->get_down_iterator());
		    			smart_actptr* p;

		    			while((p = li())) {
						ActivityInstance*	ACT_req_2 = p->BP;
						instance_state_changer	isc(ACT_req_2);

						isc.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_ABSTRACTED);
						isc.do_it(stack_to_use);
					}
				}
				break;
			case apgen::act_visibility_state::VISIBILITY_REGULAR:
				if(ACT_req_changing->theActAgent()->hasType()) {
					if(	!ACT_req_changing->hierarchy().children_count()
					    	&& (	has_decomp_method &&
							(mt == apgen::METHOD_TYPE::NONEXCLDECOMP)
						   )
					  ) {
						if(mt == apgen::METHOD_TYPE::NONEXCLDECOMP) {
							eval_intfc::get_act_lists().someActiveInstances << ACT_req_changing;
						}
						assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
						try {
							creation_has_been_handled = true;
							ACT_req_changing->create();
							ACT_req_changing->exercise_decomposition(Code);
						}
						catch(eval_error Err) {
							throw(Err);
						}
						if(	(!ACT_req_changing->hierarchy().children_count())
							&& ACT_req_changing->agent().is_decomposed()) {
							// back-pedal
							eval_intfc::get_act_lists().someActiveInstances << ACT_req_changing;
							assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
						}
					} else {
						eval_intfc::get_act_lists().someActiveInstances << ACT_req_changing;
						assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
					}
				} else {
					eval_intfc::get_act_lists().someActiveInstances << ACT_req_changing;
					assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
				}
				if(ACT_req_changing->hierarchy().children_count()) {
					slist<alpha_void, smart_actptr>::iterator
								li(ACT_req_changing->get_down_iterator());
		    			smart_actptr*	p;

		    			while((p = li())) {
						ActivityInstance*	ACT_req_2 = p->BP;
						instance_state_changer	isc(ACT_req_2);

						isc.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_ABSTRACTED);
						isc.do_it(stack_to_use);
					}
				}
				if(ACT_req_changing->agent().is_active()) {
					if(ACT_req_changing->dataForDerivedClasses) {
						ACT_req_changing->dataForDerivedClasses->handle_instantiation(true, true);
					}
				}
				break;
			// pacify compiler
			default:
				break;
		    }
		}
		catch(eval_error Err) {
			throw(Err);
		}
		if(	exercise_constructors
			&& !was_weakly_active
			&& ACT_req_changing->agent().is_weakly_active()
			&& !creation_has_been_handled) {
			ACT_req_changing->create();
		}
		return;
	}

	// SECTION 3:	From now on we may assume that the activity is not brand new, i. e.,
	//		it is already part of a 'sane' hierarchy. We need to update the children
	//		to reflect the hierarchy.
	try {
	    smart_actptr*		ptrnode;
	    ActivityInstance*		ACT_req_3;
	    ActivityInstance*		theACT_req_Parent;
	    bool			creation_has_been_handled = false;

	    switch(desired_state.invisibility) {
		case apgen::act_visibility_state::VISIBILITY_ON_CLIPBOARD:
		    {
		    // Recursively put children on the clipboard:
		    slist<alpha_void, smart_actptr>::iterator li(ACT_req_changing->get_down_iterator());
		    smart_actptr*	p;

		    /* It is essential that this loop should be carried out
		     * before the activity is severed from its parent: */
		    while((p = li())) {
			ActivityInstance*	ACT_req_4 = p->BP;
			instance_state_changer	isc(ACT_req_4);

			isc.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_ON_CLIPBOARD);
			isc.do_it(stack_to_use);
		    }

		    /* Now it's OK to do this: */
		    if(ACT_req_changing->get_parent() && !desired_state.parent) {
			sever_from_parent(stack_to_use);
		    }

		    /* We need a criterion to see whether or not to
		     * invoke the destructor. The 'weakly active' flag should
		     * do: */
		    if(was_weakly_active) {
		    	ACT_req_changing->destroy();
		    }

		    ACT_req_changing->unselect();
		    eval_intfc::get_act_lists().somePendingActivities << ACT_req_changing;
		    assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
		    ACT_req_changing->set_scheduled(false);
		    return;
		    }
		    // redundant...
		    break;
		case apgen::act_visibility_state::VISIBILITY_DECOMPOSED:
		case apgen::act_visibility_state::VISIBILITY_NEWCHILDREN:
		    {
		    bool		should_demote = false;

		    if(!ACT_req_changing->theActAgent()->hasType()) {
			    /* Allow decomposition if children were read from apf even though
			     * no type was defined */
			    if(theHierarchy.children_count()) {
				should_demote = true;
			    } else {
				Cstring rmessage;
				rmessage << "instance " << ACT_req_changing->identify()
					<< " cannot be detailed: "
					<< "activity has no children and its type is undefined. " ;
				throw(decomp_error(rmessage));
			    }
		    } else {
			if(has_decomp_method) {
			    switch(mt) {
				case apgen::METHOD_TYPE::CONCUREXP:
				case apgen::METHOD_TYPE::NONE:

					//
					// type may have been overridden by grouping:
					//
					if(!theHierarchy.children_count()) {
						Cstring rmessage;
						rmessage << "request " << ACT_req_changing->identify()
							<< " is of a type that cannot be detailed # " ;
						throw(decomp_error(rmessage));
					}

					//
					// fall through to decompose
					//

				case apgen::METHOD_TYPE::DECOMPOSITION:
				case apgen::METHOD_TYPE::NONEXCLDECOMP:
					if(theHierarchy.children_count()) {
						should_demote = true;
					} else {

						if(ACT_req_changing->is_a_request()) {
							Cstring rmessage;
							rmessage = ACT_req_changing->identify()
								+ " is a request and does not re-decompose.";
							throw(decomp_error(rmessage));
						}

						if(desired_state.invisibility == apgen::act_visibility_state::VISIBILITY_DECOMPOSED) {
							ACT_req_changing->agent().move_to_decomposed_list();
						}
						try {
							creation_has_been_handled = true;

							//
							// THIS IS WRONG if creation has already been exercised:
							//
							ACT_req_changing->create();
							ACT_req_changing->exercise_decomposition(Code);
						}
						catch(eval_error Err) {
							throw(Err);
						}
						if(	desired_state.invisibility == apgen::act_visibility_state::VISIBILITY_DECOMPOSED
							&& !theHierarchy.children_count()) {
							// back-pedal
							ACT_req_changing->agent().move_to_active_list();
						}
						slist<alpha_void, smart_actptr>::iterator
							subs(ACT_req_changing->get_down_iterator());
						if(desired_state.invisibility == apgen::act_visibility_state::VISIBILITY_DECOMPOSED) {
							while((ptrnode = subs())) {
								ACT_req_3 = ptrnode->BP;
								ACT_req_3->select();
							}
						}
					}
					break ;
				default:
					{
					Cstring rmessage;
					rmessage << "instance " << ACT_req_changing->identify()
						<< " has unknown downtype";
					throw(decomp_error(rmessage));
					}
			    }
			}
		    }
		    if(		desired_state.invisibility == apgen::act_visibility_state::VISIBILITY_DECOMPOSED
				&& should_demote) {
					/* We used to also check this, but sometimes the
					 * activity has ALREADY been removed from the active list!
					 *
					 *&& is_in_list( ACT_exec::ACT_subsystem->someActiveInstances )
					 */

			//
			// debug
			//
			// cout << dbgind::indent() << "unselecting act\n";

			ACT_req_changing->unselect() ;
			slist<alpha_void, smart_actptr>::iterator subs(ACT_req_changing->get_down_iterator());

			if(ACT_req_changing->dataForDerivedClasses) {

				//
				// debug
				//
				// cout << dbgind::indent() << "calling handle_instantiation()\n";

				ACT_req_changing->dataForDerivedClasses->handle_instantiation(false, true);
			}

			//
			// debug
			//
			// cout << dbgind::indent() << "making " << ACT_req_changing->hierarchy().children_count()
			// 	<< " child(ren) visible\n";

			while((ptrnode = subs())) {
				ACT_req_3 = ptrnode->BP;
				// The second case is to handle resolved activities that were just created:
				if(	ACT_req_3->agent().is_abstracted()
					|| ACT_req_3->agent().is_brand_new()
				  ) {
					instance_state_changer	ISC(ACT_req_3);
		
					ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
					ISC.do_it(stack_to_use);
				}
				ACT_req_3->select();
			}
			// do this AFTER the children have been moved, otherwise they won't know they are weakly active!!
			eval_intfc::get_act_lists().someDecomposedActivities << ACT_req_changing;
			assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
		    }
		    }
		    break;
		case apgen::act_visibility_state::VISIBILITY_ABSTRACTED:
		    theACT_req_Parent = ACT_req_changing->get_parent();

		    if(theACT_req_Parent) {
			    if(theACT_req_Parent->is_decomposed()) {
				// Don't do anything at the child level. This is a job for the parent.

				// will call create() if necessary:
				instance_state_changer	ISC(theACT_req_Parent);
				
				// do this FIRST, while the children are still active; otherwise,
				// the Parent won't know that it's weakly active...
				ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
				ISC.do_it(stack_to_use);
				theACT_req_Parent->select();
				return;
			    }
		    } else {
				Cstring rmessage;
				rmessage = "Activity " + ACT_req_changing->identify() + " does not abstract.";
				throw(decomp_error(rmessage));
		    }


		    if(ACT_req_changing->agent().is_decomposed()) {
				instance_state_changer	ISC(ACT_req_changing);

				/* Recursively invokes the children and moves them to
				 * 'abstracted'; also invokes the destroy() method */
				ISC.undisplay_hierarchy();
		    } else {
			if(ACT_req_changing->dataForDerivedClasses) {
				ACT_req_changing->dataForDerivedClasses->handle_instantiation(false, true);
			}
		    }
		    ACT_req_changing->unselect();
		    ACT_req_changing->agent().move_to_abstracted_list();
		    break;

		case apgen::act_visibility_state::VISIBILITY_REGULAR:
		    eval_intfc::get_act_lists().someActiveInstances << ACT_req_changing;
		    assert(ACT_req_changing->insertion_state == Dsource::in_a_list);
		    if(ACT_req_changing->dataForDerivedClasses) {
			ACT_req_changing->dataForDerivedClasses->handle_instantiation(true, true);
		    }

		    {
		    PR_manager&	theAgent = ACT_req_changing->agent();



		    /* STEP 4. Take care of children.
		     *
		     * We used to check on the size of the subactivities List. But that's not necessarily
		     * right... how about adding more children? It's really up to the client to specify
		     * whether or not to exercize the decomposition clause.
		     *
		     * Come to think of it, if there are any children (other than while running the
		     * expansion), it's because either the decomp section was run, or someone wants
		     * to override it. So it's not a bad check after all. */
		    if(!theHierarchy.children_count()) {

			//
			// SHOULD SIMPLIFY THIS -- brand new case was already processed above.
			//
			if(  has_decomp_method &&
			     mt == apgen::METHOD_TYPE::NONEXCLDECOMP) {

				//
				// special (bad) case of
				// apgen::METHOD_TYPE::NONEXCLDECOMP:
				//
				if(ACT_req_changing->is_a_request()) {
					Cstring rmessage;
					rmessage = ACT_req_changing->identify()
							+ " is a request and does not re-decompose.";
					throw(decomp_error(rmessage));
				}

				// Here we should put the parent in its desired state
				// (at a low level!) BEFORE running the decomp program.

				// We execute the decomposition program. This will call
				// do_it(desired state = abstracted) on the children, so
				// we have a recursive situation. The parent was already
				// handled and should NOT be modified again.
				try {
					creation_has_been_handled = true;
					ACT_req_changing->create();
					ACT_req_changing->exercise_decomposition(Code);
				}
				catch(eval_error Err) {
					throw(Err);
				}
			}
		    } else {

			//
			// Here we take care of existing children, e. g. because we are being pasted
			// and need to put them into an abstracted mode
			//
			slist<alpha_void, smart_actptr>::iterator	li(ACT_req_changing->get_down_iterator());
			smart_actptr*	p;

			while((p = li())) {
				ActivityInstance*	the_ACT_req_child = p->BP;

				if(!the_ACT_req_child->agent().is_abstracted()) {
					instance_state_changer	ISC(the_ACT_req_child);

					ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_ABSTRACTED);
					ISC.do_it(stack_to_use);
				}
			}
		    }
		    }
		    break;
		default:
		    break;
	    } // end of visibility switch

	    // Section 4: final adjustments (constructors/destructors)

	    if(was_weakly_active && !ACT_req_changing->agent().is_weakly_active()) {
		ACT_req_changing->destroy();
	    } else if(	exercise_constructors
			&& ACT_req_changing->agent().is_weakly_active()
			&& !was_weakly_active
			&& !creation_has_been_handled) {
		ACT_req_changing->create();
	    }
	} // end of try block
	catch(eval_error Err) {
		throw(Err);
	}
	catch(decomp_error Err) {
		throw(Err);
	}
	return;
}

		/*
		 * Watch out: ACT_req_3 must be either NULL (new activity is then created
		 * and pointer to it is passed back to the caller), or a pointer to an
		 * existing activity instance (which will be re-parented).
		 */
void instance_state_changer::add_a_child(
		pEsys::execStack*	stack_to_use,
		ActivityInstance*	ACT_req_adopted) {
	int			make_child_epoch_relative_if_parent_is = 1;
	ActivityInstance*	existing_ACT_req_parent = ACT_req_adopted->get_parent();



	/* NOTE: the child activity is probably visible, because the user may have picked
	 * it interactively. However, it could also be that the call resulted from a
	 * SEQtalk message, in which case the activity may have different statuses (stati??)
	 * in different instances of APGEN, so we should be careful...
	 *
	 * We get into a tricky situation when the child activity has a start time that is
	 * specified in terms of an epoch. Generally, the parent and its children are intended
	 * to move as a group; however if the child activity pre-exists this call, then we
	 * should probably leave the child's epoch alone if it exists.  */
	if(ACT_req_adopted->EpochIfApplicable.length()) {
		make_child_epoch_relative_if_parent_is = 0;
	}

	// This will preserve the parent's epoch if any. If a pre-existing child is epoch-less,
	// it acquires the epoch of the parent:
	if(	ACT_req_changing->EpochIfApplicable.length()
		&& make_child_epoch_relative_if_parent_is ) {
		ACT_req_adopted->EpochIfApplicable = ACT_req_changing->EpochIfApplicable;
	}
	// to avoid "interesting behavior":
	if(ACT_req_changing->is_unscheduled()) {
		ACT_req_adopted->set_scheduled(false);
	} else {
		ACT_req_adopted->set_scheduled(true);
	}

	if(existing_ACT_req_parent == ACT_req_changing) {
		return;
	} else if(existing_ACT_req_parent) {
		instance_state_changer	ISC(ACT_req_adopted);

		ISC.set_desired_parent_to(NULL);
		try {
			ISC.do_it(stack_to_use);
		}
		catch(eval_error Err) {
			Cstring		msg( "The following occurred while trying to create children of " );
			msg << ACT_req_changing->identify() << "\n" << Err.msg;
			throw(eval_error(msg));
		}
		catch(decomp_error Err) {
			Cstring		msg( "The following occurred while trying to create children of " );
			msg << ACT_req_changing->identify() << "\n" << Err.msg;
			throw(decomp_error(msg));
		}
	}


	ACT_req_adopted->hierarchy().attach_to_parent(ACT_req_changing->hierarchy());

	/* If the child was pre-existent, we need to move it to the right List, and probably make it
	 * invisible. Note that if we are reading a file, all activities are in the 'brand new' list and
	 * we don't want to interfere with the logic that's coming up. */
	if( ACT_req_adopted->agent().is_active() || ACT_req_adopted->agent().is_decomposed() ) {
		instance_state_changer	ISC(ACT_req_adopted);

		ISC.undisplay_hierarchy();
		if(ACT_req_adopted->agent().is_active()) {
			if(ACT_req_adopted->dataForDerivedClasses) {
				ACT_req_adopted->dataForDerivedClasses->handle_instantiation(false, true);
			}
		}
		ACT_req_adopted->agent().move_to_abstracted_list();
	}
}


//
// The following method is called by ACT_req::move_from_decomposed_to_active_list() and also by
// create_down_activity(). The idea of the method is to move 'everybody'
// (i. e. this activity and its descendants) from the decomposed/active/abstracted
// lists to abstracted only. The destruction section of the children is exercised
// in the non-exclusive case.
//
void instance_state_changer::undisplay_hierarchy() {
	slist<alpha_void, smart_actptr>::iterator	ListOfDownlinks(ACT_req_changing->hierarchy().subactivities);
	smart_actptr*					ptrnode;
	ActivityInstance*				ACT_req_1;
	apgen::METHOD_TYPE				mt;

	while((ptrnode = ListOfDownlinks.next())) {
		ACT_req_1 = ptrnode->BP;
		ACT_req_1->unselect();
		if(ACT_req_1->agent().is_decomposed()) {
			instance_state_changer	ISC(ACT_req_1);

			//
			// Keep looking for a level at which things are displayed:
			//
			ISC.undisplay_hierarchy();
		} else {
			// This seems correct:
			if(  ACT_req_changing->has_decomp_method(mt) &&
			     (mt != apgen::METHOD_TYPE::NONEXCLDECOMP
			     )
			  ) {
				ACT_req_1->destroy();
			}
			if(ACT_req_1->dataForDerivedClasses) {
				ACT_req_1->dataForDerivedClasses->handle_instantiation(false, true);
			}
		}

		//
		// PFM NOTES:	1. ACT_req is still available as the pointed-to object in the Pointer_node
		//		2. The next call automatically removes the request from the Activities List
		//		3. This action is reversed by ACT_req::decompose(), which re-inserts the
		//		   request into the Activities List.
		//
		ACT_req_1->agent().move_to_abstracted_list();
	}
}

void instance_state_changer::sever_from_parent(
		pEsys::execStack*	stack_to_use) {
	ActivityInstance*	ACT_req_parent;

	if((ACT_req_parent = ACT_req_changing->get_parent())) {
		smart_actptr*				ptrnode = NULL;
		backptr<ActivityInstance>::ptr2p2b*	dptr;
		slist<alpha_void, backptr<ActivityInstance>::ptr2p2b>::iterator	the_pointers_to_me(
							ACT_req_changing->Pointers()->pointers);
 
		while((dptr = the_pointers_to_me())) {
			p2b<ActivityInstance>*	thePointer = dptr->payload;

			// PFM: added check on ptr2 because the pointer may have been deactivated.
			if(thePointer) {
				smart_actptr*	ActPtr = dynamic_cast<smart_actptr*>(thePointer);

				if(ActPtr && ActPtr->list == &ACT_req_parent->hierarchy().subactivities) {
					// Aha, we found it
					ptrnode = ActPtr;
					break;
				}
			}
		}
		if(!ptrnode) {
			Cstring	msg( "Cannot sever " ) ;
			msg << ACT_req_changing->identify() << " from its parent activity because a pointer is missing" ;
			throw(decomp_error(msg));
		}
		delete ptrnode;

		if(	ACT_req_parent->agent().is_decomposed()
			&& !ACT_req_parent->hierarchy().subactivities.get_length() ) {
			try {
				instance_state_changer	ISC(ACT_req_parent);

				ISC.set_desired_visibility_to(apgen::act_visibility_state::VISIBILITY_REGULAR);
				ISC.do_it(stack_to_use);
			} catch(eval_error Err) {
				Cstring	msg("The following occurred while trying to "
					"instantiate a now childless parent:\n");
				msg << Err.msg;
				throw(eval_error(msg));
			}
			catch(decomp_error Err) {
				Cstring	msg("The following occurred while trying to "
					"instantiate a now childless parent:\n");
				msg << Err.msg;
				throw(decomp_error(msg));
			}
		}
		ACT_req_changing->hierarchy().superactivities.clear();
		(*ACT_req_changing->Object)[ActivityInstance::PARENT].get_object().dereference();

		//
		// we made it
		//
		ACT_req_changing->hierarchy().update_up_accelerator(ACT_req_changing);
	} else {
		cerr << "APGEN INTERNAL ERROR: Oops... " << ACT_req_changing->identify()
			<< "->sever_from_parent() can't find pointer to parent.\n";
		cerr << "Something's not right; save everything and quit while you're ahead.\n";
		cerr << "Hate to say this, but the forecast is gloomy.\n";
	}
}

void instance_state_changer::fix_accelerator(ActivityInstance *ACT_req_1) {
	ACT_req_1->hierarchy().update_up_accelerator(ACT_req_1);
}

