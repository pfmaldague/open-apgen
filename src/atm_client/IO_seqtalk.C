#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define apDEBUG

#include "apDEBUG.H"
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>	// for setting options below (Solaris needed this)
#endif
#include <sys/stat.h>
#include <unistd.h>


#include "ACT_sys.H"
#include "action_request.H"
#include <ActivityInstance.H>
#include "C_global.H"
// #include "CON_exec.H"
#include "CON_sys.H"
#include "IO_write.H"
#include "RES_eval.H"
#include "RD_sys.H"
#include "UI_activitydisplay.H"
#include "UI_exec.H"
#include "UI_mainwindow.H"
#include "UI_messagewarndialog.H"
#include "UI_save.H"
#include "mw_intfc.H"
#include "apcoreWaiter.H"
#include "gtk_bridge.H"


using namespace std;

// to enable C++ features in APserver.h:
// #define THIS_IS_APGEN 1
// #include "APserver.h"

// EXTERNS:

			// in UI_exec.C:
extern stringtlist&	FileNamesWithInstancesAndOrNonLayoutDirectives();

			// in UI_openfsd.C:
extern Cstring          pendingInputFile;
extern void             open_anyway_CB(const Cstring &);

			// in main.C:
extern refresh_info	refreshInfo;
extern motif_widget	*TopLevelWidget;
extern void		InsertWorkProc(void *with_this_data);
extern bool		anImportantEventHasBeenDetected;

			// in UI_abspanzoom.C:
extern CTime		real_time_update_interval, modeled_time_update_interval;

XtInputId		theInputIdForTheInitialHubCall;
			// in UI_motif_widget.C:
extern XtAppContext	Context;

// FORWARD DECLARATION:

typedef XtInputId	*inputIdTypeForSocketEvent;

void	process_non_X_event(void *client_data, int *, inputIdTypeForSocketEvent);

// GLOBALS:


void bell() {
	XBell(XtDisplay(TopLevelWidget->widget), 0); }

tlist<alpha_string, synchro::problem>&	theActionsToTake() {
	static tlist<alpha_string, synchro::problem>	a;
	return a; }


#ifdef apDEBUG

void dump_window_list(const clist & cl) {
	win_node	*t1 = (win_node *) cl.first_node();
	int		i = 0;

	cerr << endl;
	while(t1) {
		cerr << "node " << (++i) << " " << t1->end_type << " at " << t1->_i << endl;
		t1 = (win_node *) t1->next_node(); }
	cerr << endl; }

int verify_window_list(const clist & cl) {
	win_node	*t2, *t1 = (win_node *) cl.first_node();

	while(t1) {
		if(t1->end_type != 's') {
			cerr << "verify_window_list: bad list\n";
			dump_window_list(cl);
			return 0; }
		t2 = (win_node *) t1->next_node();
		if((! t2) || (t2->end_type != 'e')) {
			cerr << "verify_window_list: bad list\n";
			dump_window_list(cl);
			return 0; }
		t1 = (win_node *) t2->next_node(); }
	return 1; }
#endif

static bool		theWorkProcHasNotYetBeenCalled = false;
static int		theFileSemaphore = 0;
int			UpdateBusyLine = 0;

static infoNode*	the_tagged_list;

//
// DESIGN OF THE SYNCHRONIZATION MECHANISM IN APGEN
// ================================================
//
// See comments in main.C.
//
//
// DETAILED NOTES ON SCROLLBAR SYNCHRONIZATION
// ===========================================
//
// (The purpose of the note below was to help me (PFM) figure out how to re-
// organize the synchronization logic for vertical scrolling so as to make
// it consistent between ACT_sys and RES_sys for resource scrolling. The note
// is somewhat disorganized as it reflects my evolving thining.)
//
// list_of_siblings::isOutOfSync() simply checks the flag a_client_has_requested
// _a_time_change. This can be understood as follows. The list_of_siblings
// object is not really present on the GUI, nor does it have a representative in the
// apgen core database. The purpose of this object is to detect discrepancies between
// the value(s) associated with the scrollbar(s), on the one hand, and what is actually
// being displayed by the various siblings.

// For example, the NEW_HORIZONrequest which implements panning the act. display
// calls UI_mainwindow::panZoomDisplay() which in turn does two things:
//
// 	- call list_of_siblings::request_a_change_in_time_parameters()
// 	- set the "never scrolled" flag of the act sys to 0.
//
// How does this change the GUI? Well, weShouldWork() (in IO_seqtalk.C) determines
// whether we should work to refresh some aspect(s) of the GUI. It queries each and
// every act sys through its isOutOfSync() method, and the act sys in turn invokes
// isOutOfSync() on its Siblings (a pointer to the list_of_siblings object for the
// group the act sys belongs to.)

// The above ensures that the event is caught. But what about actually drawing the
// panned display(s)? That is done in theWorkProc, which contains a loop over all
// act sys objects (actually, more than one loop). In the first loop, each act sys
// (again) checks whether Siblings->isOutofSync(); if yes, then it
// Siblings->execute_client_request()s.
//
// Note that this "does nothing": it simply resets the a_client_has_... flag and sets the
// "real" time origin and span values to those desired by the "user" (could be an action
// request in reality). At this point, we are in a somewhat dangerous, "transient"
// state: theWorkProc must ensure that each sibling has a chance to "notice" that its
// configuration no longer reflects the "official" start and span values stored in
// the list_of_siblings object.
//
// This actually happens. In a (second) loop over all act sys objects, theWorkProc
// queries every single act instance in the plan and asks _it_ whether it is correctly
// displayed in the act sys object. Eventually ACT_req::compute_start_and_end_times_in()
// is invoked, and it explicitly computes pixel coordinates given (among other things)
// the official start and span values stored in the list_of_siblings.
//
// This is still not enough. In a (third) loop over all act sys objects, timemark objects
// (the time rulers) are queried; that'll give them a chance to realize that they
// need to adjust to the new official start and span.
//
// In that same loop, there is a call to ACT_sys::the_..._scrollbar_is_out_of_sync().
// Again, we have unique semantics here: what is checked by that method is that the
// value (in the Motif sense) of the scrollbar agrees with the "current" value that
// the act sys stores internally. A discrepancy at this stage can only occur when
// the user interacts with a scrollbar. If there is a discrepancy, a NEW_HORIZONrequest
// is issued. This is how explicit manipulation of the scrollbar by the user is
// handled.
//
// Otherwise, there may still be changes to the scrollbar but they involve
// scale etc., not the value itself. (Note that the_..._scrollbar_is_out_of_sync() is
// invoked by ACT_sys::isOutOfSync(), ensuring that all user actions will be caught
// by weShouldWork().)
//

int	weShouldWork() {
	ACT_sys			*act_sis;
	actsis_iterator		act_sisses;
	Lego			*aLegend;
	infoList		*theDelegates = NULL, *theEnforcers = NULL;

	// debug
	// cerr << "weShouldWork() called\n";

	//
	// Consult the gtk subsystem:
	//
	{
		lock_guard<mutex>	lock1(*gS::get_gtk_mutex());

		// debug
		// cerr << "WeShouldWork(): testing for data availability...\n";

		if(eS::gtk_editor_data_available) {

			//
			// This will process an EDIT_ACTIVITYrequest
			// if changes are detected in activity data:
			//
			eS::grab_gtk_editor_data();
			eS::gtk_editor_data_available = false;
		}
		if(xS::gtk_xmlwin_data_available) {

			// debug
			// cerr << "WeShouldWork(): XML data available\n";

			xS::grab_gtk_xmlwin_data();
			xS::gtk_xmlwin_data_available = false;
		}
		if(glS::gtk_globwin_data_available) {
			glS::grab_gtk_globwin_data();
			glS::gtk_globwin_data_available = false;
		}
	}

	if(refreshInfo.ON) {

		//
		// Following the "new" (11/2001) way of tracking and documenting "out-of-syncness", we are
		// creating a list that will hold all servers (who are in charge of detecting out_of_syncness)
		// and agents (who are in charge of synchronizing everybody with the database.)
		// If nothing happens, i. e. the list is empty, then we should delete the list on exit; else,
		// we should add it to some central repository who can make the list public (e. g. by dumping
		// it somewhere, or sending it to a client over a socket.)
		//
		theDelegates = &refresh_info::servers;
		theEnforcers = &refresh_info::agents;
	}

	//
	// Put this at the beginning, because we ALWAYS want to check:
	//
	UI_mainwindow::isOutOfSync(theDelegates);
	if(UI_mainwindow::foundSomethingToDoInADelayedManner()) {
		UI_mainwindow::bringToolbarButtonsInSync();
	}

	if(udef_intfc::something_happened() <= 0) {
		// debug
		// cerr << "We have already been called and nothing significant happened\n";

		return 0;
	}
	udef_intfc::something_happened()--;

	// Little trick ensures that we don't waste time figuring out whether or not to work if we already
	// found out we ought to and yet haven't:
	if(theWorkProcHasNotYetBeenCalled) {
		// cerr << "We return 0 because the WorkProc was registered but
		// never called so it WILL! Just BE PATIENT!!\n";

		return 0;
	}
	if((!theFileSemaphore) && APcloptions::FilesToRead().get_length()) {
		theWorkProcHasNotYetBeenCalled = true;

		if(refreshInfo.ON) {
			(*theDelegates) << (the_tagged_list = new infoNode("weShouldWork"));
			the_tagged_list->payload.reason = "  there are files to read; so: YES. ";
			refreshInfo.trigger_count++;
			refreshInfo.last_work_proc_trigger = "FilesToRead";
		}

		// cerr << "weShouldWork: files etc. to read, so: YES\n";
		return 1;
	}
	if(anImportantEventHasBeenDetected) {
		theWorkProcHasNotYetBeenCalled = true;

		// cerr << "weShouldWork: an important event has been detected, so: YES\n";
		return 1;
	}

	while((act_sis = act_sisses())) {
		synchro::problem*	a_problem;
		/* Note for the scrolled resource case: ACT_sys::isOutOfSync does extensive checking,
		 * including with its RES_sys; in principle, that is where any out-of-sync MW_legends
		 * should be detected and reported. */
		if((a_problem = act_sis->detectSyncProblem())) {

			// cerr << "weShouldWork: (act_sys) found problem " << a_problem->get_key() << "\n";

			/* I hate to do this - there's got to be a better way of utilizing the information
			 * gathered by the ACT_sys detector */
			delete a_problem;
			theWorkProcHasNotYetBeenCalled = true;
			if(refreshInfo.ON) {
				refreshInfo.trigger_count++;
				refreshInfo.last_work_proc_trigger = act_sis->get_key();
			}

			// cerr << "weShouldWork: at least one act-sis is out of sync, so: YES\n";
			return 1;
		}
	}

	return 0;
}

	/* ugly Boolean type is left over from Motif: */
Boolean theWorkProc(void *) {
	Boolean	ret_val;
	if(theWorkProcCplusplus(NULL)) {
		ret_val = TRUE;
	} else {
		ret_val = FALSE;
	}
	return ret_val;
}

void	stickInToDoList(synchro::problem*	a_problem) {
	synchro::problem*	a_similar_problem;
	if((a_similar_problem = theActionsToTake().find(a_problem->get_key()))) {
		// we already know descr fields are identical
		if(	a_similar_problem->payload.the_detector		== a_problem->payload.the_detector
			&& a_similar_problem->payload.the_fixer		== a_problem->payload.the_fixer
			&& a_similar_problem->payload.detect_action	== a_problem->payload.detect_action
			&& a_similar_problem->payload.fix_action	== a_problem->payload.fix_action
		  )  {
			// the 2 problems have the same detector and the same fixer
			// cerr << "stickInToDoList: duplicate problem " << a_problem->get_key() << ", deleting it\n";
			delete a_problem;
			a_problem = NULL;
		}
	}
	if(a_problem) {
		// cerr << "    stickInToDoList: adding problem " << a_problem->get_key() << " to actions to take.\n";
		theActionsToTake() << a_problem;
	}
}

bool theWorkProcCplusplus(void *) {
	ACT_sys*		act_sis;
	Dsource*		request;
	DS_line*		theLine;
	DS_line*		theOldLine;
	Lego*			aLegend;
	emptySymbol*		N;
	actsis_iterator		act_sisses;
	infoList		*theDelegates = NULL, *theEnforcers = NULL;
	static bool		theGtkObjectHasNotBeenCreated = true;
	synchro::problem*	a_problem;

	if(refreshInfo.ON) {
		/* Following the "new" (11/2001) way of tracking and documenting "out-of-syncness", we are
		 * creating a list that will hold all servers (who are in charge of detecting out_of_syncness)
		 * and agents (who are in charge of synchronizing everybody with the database.)
		 * If nothing happens, i. e. the list is empty, then we should delete the list on exit; else,
		 * we should add it to some central repository who can make the list public (e. g. by dumping
		 * it somewhere, or sending it to a client over a socket.) */
		theDelegates = &refresh_info::servers;
		theEnforcers = &refresh_info::agents;
	}


	// cerr << "theWorkProcCplusplus called. Setting theWorkProcHasNotYetBeenCalled to false.\n";

	theWorkProcHasNotYetBeenCalled = false;

#ifdef GTK_EDITOR
	if(theGtkObjectHasNotBeenCreated) {
		theGtkObjectHasNotBeenCreated = false;
		gS::initializeGtkSubsystem();
	}
#endif /* GTK_EDITOR */

#ifdef DEBUG_PROBLEMS
	cerr << "theWorkProcCplusplus called\n";
#endif /* DEBUG_PROBLEMS */

	/* The rationale for using theFileSemaphore is that a script may contain a PAUSE statement
	 * which causes the action_request to 'complete', i. e. to return, even though not all
	 * commands have been executed. In that case, we will pass through here again, but since
	 * we are already in the midst of processing input files we should not scan the loop again...  */
	if(!theFileSemaphore) {
	    while((N = APcloptions::FilesToRead().first_node())) {
		theFileSemaphore = 1;
		// Check if file is already opened
		// 96-08-07 DSG reversed following for FR021:  user chooses to read or not:
		// PFM: we won't check. Assume the user knows what he/she's doing.

		// UUUUUU Need to handle environment

		pendingInputFile = N->get_key();

		if(refreshInfo.ON) {
			(*theEnforcers) << (the_tagged_list = new infoNode("theWorkProc"));
			the_tagged_list->payload.reason << "creating action request to handle file " << pendingInputFile;
			refreshInfo.add_servicing_agent("FileReader", "FilesToRead not empty");
		}

		if(FileNamesWithInstancesAndOrNonLayoutDirectives().find(N->get_key())) {
			List		buttons;

			buttons << new String_node("Open File Anyway");
			buttons << new String_node("Forget It");
			UI_messagewarndialog::initialize (
				"Open File",
				"Duplicate Input File",
				"The selected file has already been opened, or "
					"the filename is already in use.  Re-reading a "
					"file may yield duplicates of activity instances.",
				buttons,
				open_anyway_CB
				);
		} else {
			OPEN_FILErequest* request = new OPEN_FILErequest(compiler_intfc::FILE_NAME, N->get_key());

			try {
		        	request->process();
			} catch(eval_error(Err)) {
				// hopefully, the Action_Request class has logged the error
				APcloptions::FilesToRead().clear();
				break;
			}
			if(request->get_status() != apgen::RETURN_STATUS::SUCCESS) {
				APcloptions::FilesToRead().clear();
				break;
			}
			pendingInputFile.undefine();
		}
		delete N;
	    }
	    theFileSemaphore = 0;
	}

	while((act_sis = act_sisses())) {
		// Question: could a RES_sys be visible if its parent ACT_sys isn't?
		if(act_sis->isVisible()) {
			activity_display*		AD = act_sis->get_AD_parent();
			int             		violation_count =
								Constraint::violations().get_length() -
								Constraint::release_count();
			static char     		theViolationCountAsAString[89];
			DS_scroll_graph*		legend_as_a_ds_scroll_graph = act_sis->legend_display->get_this();

			if(AD->adViolationCountLabel) {
				if(APcloptions::theCmdLineOptions().constraints_active) {
					sprintf(theViolationCountAsAString, "%d", violation_count);
					if(AD->adViolationCountLabel->get_text() != theViolationCountAsAString) {
						*AD->adViolationCountLabel = (char *) theViolationCountAsAString;
					}
				} else {
					if(AD->adViolationCountLabel->get_text() != "0") {
						*AD->adViolationCountLabel = (char *) "0";
					}
				}
			}
			if(act_sis->Siblings->isOutOfSync(theDelegates)) {
				/*
				* NOTE: the debug code below makes it clear that we plan to call the delayed work procedure,
				* 	and yet we don't call it yet. This is potentially dangerous logic... it works because
				* 	there is another check in the NEXT loop over act-sisses, and that one WILL set the
				*	delayed work procedure for sure.
				*
				*	The danger here is that list_of_siblings::isOutOfSync() might not always trigger
				*	the same way as whatever detector is used in the NEXT loop, in which case we'd
				*	have inconsistent behavior. A better coding technique is desirable.
				*/
				// execute_client_request sets the Dwtimestart and Dwtimespan members of list_of_siblings
				// based on the user's wishes. Only horizontal scrolling is taken into account here.
				act_sis->Siblings->execute_client_request();
			}
			/* Note for scrolled resource: we detect out-of-sync resource MW_legend objects in weShouldWork;
			 * now is out chance to act on them. The code below checks DS_legend objects; we should do the
			 * same for the resource equivalents. */
			if((a_problem = legend_as_a_ds_scroll_graph->detectSyncProblem())) {
				/* We take a number of global steps here, because an out-of-sync legend may mean
				 * that the user moved a grey line, after which the legends have the wrong height,
				 * and everything needs to be redrawn. Here we call configure_graphics on the
				 * DS_legend object, and in theDelayedWorkProc all dependent quantities (busy line
				 * etc.) will be redrawn.
				 */
				stickInToDoList(a_problem);
				act_sis->cleargraph();
				// need to pass a list of problems - activating does nothing now!
				// act_sis->activateTheDelayedWorkProc();

#ifdef DEBUG_PROBLEMS
				cerr << "theWorkProcCplusplus: found problem " << a_problem->get_key() << "\n";
#endif /* DEBUG_PROBLEMS */

				legend_as_a_ds_scroll_graph->configure_graphics(NULL);
			}
			if((a_problem = act_sis->constraint_display->detectSyncProblem())) {
				// need to pass a list of problems - activating does nothing now!
				// act_sis->activateTheDelayedWorkProc();

				// hate to do this... OK now

				stickInToDoList(a_problem);
				act_sis->constraint_display->configure_graphics(NULL);
			}
			// need to address the violation count
		
		}
	}


	/* SECOND: find out whether we should redraw some regions (e. g. because of edits.)
	 *
	 *	HOWEVER:
	 *
	 *		Before checking windows, we should update any obsolete DS_lines. DS_line_owners have
	 *		already notified higher-level objects that they were not displayed properly. Here
	 *		we ask them to remedy the problem. They will use the (protected) DS_line::newhcoor() method,
	 *		which creates change windows as necessary.
	 */

	// Should have a similar loop for violations:
	while((act_sis = act_sisses())) {
		if((a_problem = act_sis->detectActivityListProblem())) {
			stickInToDoList(a_problem);
		}
	}

	/*
	 *	... now we continue with window processing:
	 *
	 *
	 *	   If a global change occurred, don't bother, because we will redraw the whole thing anyway.
	 *	   Finally we do the actual drawing if necessary.
	 */
	while((act_sis = act_sisses())) {
		if(act_sis->isVisible()) {
			int				we_need_to_redraw_windows = 0;
			list_of_siblings*		sib = act_sis->Siblings;
			List_iterator			theSiblings(*sib);
			sibling*			aSibling;
			DS_draw_graph*			res_sis;

			/* Do this one first! (dependencies) */
			if((a_problem = sib->timemark->detectSyncProblem())) {
				if(refreshInfo.ON) {
					refreshInfo.add_servicing_agent(sib->timemark->get_key(), "Is out of sync"); }
				// hate this...
				delete a_problem;
				sib->timemark->configure_graphics(NULL);
			}
			while((aSibling = (sibling *) theSiblings())) {
			    if(aSibling->getTheGraph() != act_sis) {
				if(aSibling->isOutOfSync(theDelegates)) {
					// If this is a legend, it will set theStaggeringNeed:
					if(refreshInfo.ON) {
						refreshInfo.add_servicing_agent(act_sis->get_key(), "Sibling is out of sync");
					}
					aSibling->configure_graphics();
				} else {
				}
				if(	(res_sis = (DS_draw_graph *) aSibling->getTheGraph())->is_a_res_sys()
					&& res_sis->the_vertical_scrollbar_is_out_of_sync(theDelegates)) {
					res_sis->do_the_vertical_scrollbar_thing(theEnforcers);
				}
			    }
			}
			if(act_sis->the_horizontal_scrollbar_is_out_of_sync(theDelegates)) {
				act_sis->do_the_horizontal_scrollbar_thing(theEnforcers);
			}
			if(act_sis->the_vertical_scrollbar_is_out_of_sync(theDelegates)) {
				act_sis->do_the_vertical_scrollbar_thing(theEnforcers);
			}

			{
			List_iterator*	theLegsList(ACT_sys::getLegendListIterator());

			while((aLegend = (Lego *) (*theLegsList)())) {

				// these windows represent horizontal legend real-estate that needs redrawing
				if(aLegend->there_are_windows_to_redraw_for(act_sis->legend_display)) {
					we_need_to_redraw_windows = 1;
					// UUUU need to pass a list of problems - activating does nothing now!
					// act_sis->activateTheDelayedWorkProc();
					break;
				}
			}
			delete theLegsList;
			}
			if(we_need_to_redraw_windows) {
				List_iterator		*theLegsList(ACT_sys::getLegendListIterator());
				// we are dealing with local change(s)...
				// Hopper design: check that only lines that belong to this ACT_sys are included
				List_iterator		lines(act_sis->lnobjects);
				DS_line			*line;
				win_node		*wn, *wn1, *wn2;

				while((aLegend = (Lego *) (*theLegsList)())) {
					clist		&the_windows = aLegend->get_clist_of_windows_for(act_sis->legend_display);

#					ifdef apDEBUG
					if(! verify_window_list(the_windows)) {
						cerr << "bad list found in Workproc; aborting...\n";
						exit(- 3);
					}
#					endif

					wn = (win_node *) the_windows.first_node();
					while(wn) {
						int	s = wn->_i, e;

						wn1 = (win_node *) wn->next_node();
						e = wn1->_i;
						if(s < act_sis->getwidth() && e > 0) {
							// may be highlighted; allow for extra pixel:
							s--;
							e++;
							if(s < 0)
								s = 0;
							if(e > act_sis->getwidth())
								e = act_sis->getwidth();
							// don't erase the separation lines:
							XClearArea(DS_gc::getdisplay(),
								// PIXMAP
								XtWindow(act_sis->widget),
								// s, wn1->t + 1, e - s, wn1->b - wn1->t - 2, FALSE
								s, wn1->t + 1, e - s, wn1->b - wn1->t - 1, FALSE);
							act_sis->add_vertical_cursor_if_necessary(s, wn1->t + 1, e, wn1->b);
							act_sis->VertLineManager.refreshBlock(s, wn1->t + 1, e, wn1->b);
						}
						wn = (win_node *) wn1->next_node();
					}
				}
				while((line = (DS_line *) lines())) {

					// NOTE: (1) make sure all 'draw()' methods below draw to the PIXMAP, not the WINDOW
					// 	 (2) make sure that someone copies the pixmap to the window when all is said and done.
					// 	     Should that be done asynchronously?
					if(line->is_visible()) {
						int			EX = line->getendx();
						int_node		theReference(line->getstartx());
									/*
									 * get_legend_object() is virtual in the Dsource base
									 * class and properly overridden in the derived ACT_request
									 * class, so we are ACT_request-compliant.
									 */
						Lego			*theLeg = (Lego *) line->getsource()->get_legend();
						clist			&the_windows = theLeg->get_clist_of_windows_for(act_sis->legend_display);

						wn = (win_node *) the_windows.find_latest_before(& theReference);
						if(wn) {
							if(wn->end_type == 's') {
								// we KNOW that the end of the interval is to the right of
								// line->start_x...
								line->draw();
							} else {
								wn1 = (win_node *) wn->next_node();
								if(wn->_i <= EX) {
									line->draw();
								}
							}
						} else if((wn = (win_node *) the_windows.first_node())) {
							if(wn->_i <= EX) {
								line->draw();
							}
						}
					}
				}
				act_sis->redraw_the_cute_grey_lines();
				delete theLegsList;
			}
			if(act_sis->thePointersToTheNewDsLines.get_length()) {
				bpointernode	*ptr;

				while((ptr = (bpointernode *) act_sis->thePointersToTheNewDsLines.first_node())) {
					DS_line		*theLine = (DS_line *) ptr->get_ptr();

					// deletes ptr:
					theLine->draw();
				}
			}
		}
	}

	// SHOULD UPDATE SCROLLBARS IF NECESSARY...
	while((act_sis = act_sisses())) {
		if(act_sis->isVisible()) {
			List_iterator	*theLegsList(ACT_sys::getLegendListIterator());

			// Hopper design: should filter out legends that do not apply here:
			while((aLegend = (Lego *) (*theLegsList)())) {
				clist			&the_windows = aLegend->get_clist_of_windows_for(act_sis->legend_display);

				the_windows.clear();
			}
			// debug
			// cerr << "clearing thePointersToTheNewDsLines for " << act_sis->get_key() << endl;
			act_sis->thePointersToTheNewDsLines.clear();
			delete theLegsList;
		}
	}

	if(refresh_info::ON) {
		refreshInfo.reportSyncActions(theDelegates, theEnforcers);
		theDelegates->clear();
		theEnforcers->clear();
	}

	// According to the X Toolkit reference guide, if we return False the procedure
	// will be called again the next time the X event queue dries up. If we return
	// True then the procedure is removed from the workproc queue upon completion.
	// We want to stay in business, so...
	return True;
}

void theDelayedWorkProc() {
	slist<alpha_string, synchro::problem>::iterator	to_do(theActionsToTake());

	ACT_sys*		act_sis;
	actsis_iterator		act_sisses;
	infoList*		theDelegates = NULL, *theEnforcers = NULL;

	bool			grant_access = false;

	synchro::problem*	a_problem = NULL;

	// cerr << "theDelayedWorkProc starts.\n";

	// before detecting any more stuff let's make sure we've fixed whatever needed to be fixed

	// cerr << "theDelayedWorkProc: scanning list of " << theActionsToTake().get_length() << " problem(s)...\n";
	while((a_problem = to_do())) {
		synchro::problem*	still_a_problem;


		// cerr << "    handling problem " << a_problem->get_key() << "...\n";

		if(a_problem->payload.fix_action) {
			if(a_problem->action_taken.length()) {
				if((still_a_problem = a_problem->payload.detect_action(a_problem->payload.the_detector))) {
					// cerr << "problem \"" << a_problem->get_key() << "\" "
					// 	<< " persists even though the following action was taken:\n"
					// 	<< "\t" << a_problem->action_taken << "\n";
					// cerr << "    initial problem: " << a_problem->get_key()
					// 	<< "\n    new problem: " << still_a_problem->get_key() << "\n";
					delete still_a_problem;
				} else {
					// cerr << a_problem->get_key() <<
					// 	" was solved by " << a_problem->action_taken << "\n";
				}
			} else {
				// cerr << "    calling fix_action() for problem " << a_problem->get_key() << "\n";
				a_problem->payload.fix_action(a_problem);
			}
		} else {
			// cerr << "problem \"" << a_problem->get_key() << "\" had no fix; just triggered this delayed proc.\n";
		}
	}
	// cerr << "theDelayedWorkProc: clearing all actions to take.\n";
	theActionsToTake().clear();

	if(refreshInfo.ON) {
		// not used now but we may want to pass these pointers to methods like bringToolbarButtonsInSync.
		theDelegates = &refresh_info::servers;
		theEnforcers = &refresh_info::agents; }


	while((act_sis = act_sisses())) {
		if(act_sis->isVisible()) {
			DS_draw_graph*		res_sis;
			sibling*		aSibling;
			list_of_siblings*	sib = act_sis->Siblings;
			List_iterator		theSiblings(*sib);
			bool			stale = false;


			if((a_problem = ACT_sys::checkActivityCount(act_sis))) {
				a_problem->payload.the_fixer = act_sis;
				a_problem->payload.fix_action = ACT_sys::fixActivityCount;
				stickInToDoList(a_problem); }
			while((aSibling = (sibling *) theSiblings())) {
				if((res_sis = (DS_draw_graph *) aSibling->getTheGraph())->is_a_res_sys()) {
					RD_sys*	trueRDsis = (RD_sys*) res_sis;

					// cerr << "checking resource plot staleness for " << trueRDsis->get_key() << "...\n";

					if((a_problem = RD_sys::checkResPlotStaleness(trueRDsis))) {
						a_problem->payload.the_fixer = trueRDsis;
						a_problem->payload.fix_action = RD_sys::fixResPlotStaleness;

						// cerr << "    resource plot is stale; adding fixResPlotStaleness to list of things to do.\n";
						stickInToDoList(a_problem);
					}
				}
			}
		}
	}

	if(anImportantEventHasBeenDetected) {
		extern void process_stored_events();
		// cerr << "theDelayedWorkProc: important event detected, calling process_stored_events()\n"
		// 	<< "and setting flag to false\n";
		anImportantEventHasBeenDetected = false;
		process_stored_events();
	}

	// let's make sure we try to fix whatever needs to be fixed
	// cerr << "theDelayedWorkProc: about to exit, resolving " << theActionsToTake().get_length() << " problems...\n";
	while((a_problem = to_do())) {
		a_problem->payload.fix_action(a_problem); }


}
