#if HAVE_CONFIG_H
#include <config.h>
#endif

 // timing stuff
 #include <cstdio>
 #include <ctime>
 #include <sys/time.h>

#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/MwmUtil.h>
#include <dlfcn.h>

#include "ActivityInstance.H"
#include "apDEBUG.H"
#include "UI_mainwindow.H"
#include "action_request.H"
#include "CON_sys.H"
#include "APerrors.H"
#include "client_errors.H"

#ifdef GTK_EDITOR
#include "gtk_bridge.H"
#endif /* GTK_EDITOR */

#include "RD_sys.H"
#include "UI_exec.H"
#include "apcoreWaiter.H"

#include "perf_util.H"

using namespace std;

extern void		register_all_internal_functions();
extern const char*	get_apgen_version_build_platform();
int			motifLoop();

extern "C" {
#include <concat_util.h>
			// in ../apcore/APmain.C:
extern void		(*activityInstanceSerializationHandle)(void *, buf_struct *);
extern void		(*activityDefsSerializationHandle)(buf_struct *);
extern void		create_subsystems(const char*);
} // extern "C"


extern bool		gtk_editor_needs_lists;
extern mutex&		using_editor_data();
			// (will be used by the dynamic messaging library:)
extern mutex&		using_messaging_data();
extern bool		message_center_needs_lists;

			// below:
int			thread_main_gui(int, char**);

extern "C" {
			// will be loaded from the dynamic messaging library:
void			*(*msg_thread_func)(void *) = NULL;
}

bool			Event_buffering = false;

UI_mainwindow*		MW;		//name used in UI_motif_widget.C (for brevity)
motif_widget*		TopLevelWidget;
extern void		load_resources(motif_widget *);

//97-12-09 DSG:  (char*) silliness keeps HP10 aCC quiet:
String fallbacks[] = {
        // (char*)"Atm*fontList:                             -*-times-medium-r-*-*-*-140-*-*-*-*-iso8859-1" ,
        (char*)"Atm*fontList:                                -*-helvetica-medium-r-*-*-*-140-*-*-*-*-iso8859-1" ,
        (char*)"Atm*menuBar*fontList:                        -*-helvetica-bold-r-*-*-*-120-*-*-*-*-iso8859-1" ,
        (char*)"Atm*Time_and_act_Font:                       *misc*fixed-medium-r-semicondensed*13*" ,
        // (char*)"Atm*Legend_Font:                             -*-helvetica-medium-r-*-*-*-120-*-*-*-*-*-*" ,
        (char*)"Atm*Legend_Font:                             *misc*fixed-medium-r-semicondensed*13*" ,
        (char*)"Atm.smallerFont:                             -*-helvetica-medium-r-*-*-*-120-*-*-*-*-iso8859-1" ,
        (char*)"Atm*foreground:                              Black" ,
        (char*)"Atm*background:                              LightSeaGreen" ,
        (char*)"Atm*sash*background:                         IndianRed" ,
        (char*)"Atm*XmText*cursorPositionVisible:            True" ,
        (char*)"Atm*XmTextField*cursorPositionVisible:       True" ,
	(char*)"Atm*printToolbarButton.tooltip:              Print",
	(char*)"Atm*remodelToolbarButton.tooltip:            Remodel",
	(char*)"Atm*closeSRDToolbarButton.tooltip:           Close Selected Resource Display",
	(char*)"Atm*newRDToolbarButton.tooltip:              New Resource Display",
	(char*)"Atm*zoomoutToolbarButton.tooltip:            Zoom Out",
	(char*)"Atm*zoominToolbarButton.tooltip:             Zoom In",
	(char*)"Atm*closeSADToolbarButton.tooltip:           Close Selected Activity Display",
	(char*)"Atm*newADToolbarButton.tooltip:              New Activity Display",
	(char*)"Atm*editActToolbarButton.tooltip:            Edit Activity",
	(char*)"Atm*pasteToolbarButton.tooltip:              Paste from Clipboard",
	(char*)"Atm*copyToolbarButton.tooltip:               Copy Selected Activities",
	(char*)"Atm*cutToolbarButton.tooltip:                Cut Selected Activities",
	(char*)"*tooltipBackground:                            yellow",
	(char*)"*tooltipForeground:                            blue",
	(char*)"*tooltip_label.borderColor:                    red",
	(char*)"*tooltip_label.borderWidth:                    1",
	(char*)"Atm*tooltip_label.alignment:                 ALIGNMENT_BEGINNING",
	(char*)"Atm.tooltipPost:                             500",
	(char*)"Atm.tooltipDuration:                         2000",
	(char*)"Atm.tooltipX:                                10",
	(char*)"Atm.tooltipY:                                2",
        NULL
};

XtWorkProcId		theWorkProcId;
extern void		theDelayedWorkProc();
extern int		weShouldWork();
extern void		apgen_test(const Cstring&, const stringslist&);


int			refresh_info::ON = 0;	// turned ON by -refreshinfo command-line option
infoList		refresh_info::servers;
infoList		refresh_info::agents;
stringtlist		refresh_info::problems;
refresh_info		refreshInfo;

extern "C" {
	extern void		disable_signals(int);
	extern void		enable_signals();
				// here: :-)
	int			ARGC;
	char			**ARGV;
}

XtAppContext		Context;
			//Dummy defines for externs in UI_motif_widget.C:
XFontStruct*		StandardFont;
bool			thereHasBeenAnEvent = false;


#define PRESSED 33
#define RELEASED 34
static int		theButtonState = RELEASED;


void refresh_info::add_level_2_trigger(const Cstring& theTrigger , const Cstring& message) {
	tlistNode*	theTriggerNode;
	stringtlist*	theList;

	trigger_count++;
	if(!(theTriggerNode = level_2_triggers.find(theTrigger))) {
		level_2_triggers << (theTriggerNode = new tlistNode(theTrigger));
	}
	theList = &theTriggerNode->payload;
	if(!theList->find(message)) {
		(*theList) << new emptySymbol(message);
	}
}

void refresh_info::add_servicing_agent(const Cstring& theAgent, const Cstring& message) {
	tlistNode*	theAgentNode;
	stringtlist*	theList;

	if(!(theAgentNode = work_proc_servicing_agents.find(theAgent))) {
		work_proc_servicing_agents << (theAgentNode = new tlistNode(theAgent));
	}
	theList = &theAgentNode->payload;
	if(!theList->find(message)) {
		(*theList) << new emptySymbol(message);
	}
}

void refresh_info::report() {
	tlistslist::iterator	theLevel2Guys(level_2_triggers);
	tlistslist::iterator	theAgents(work_proc_servicing_agents);
	tlistNode*		bp;

	if(	(!actsis_count)
		&& (!trigger_count)
		&& (!mainwin_count)
		&& (!level_2_triggers.get_length())
		&& (!work_proc_servicing_agents.get_length())
		&& (!last_actsis.is_defined())
		&& (!last_work_proc_trigger.is_defined())
		&& (!problems.get_length())
	  ) {
		cerr << "Nothing to report.\n";
		return;
	}
	if(actsis_count) {
		cerr << "Last of " << actsis_count << " ACT_sys(es) to request delayed refresh action: "
			<< last_actsis << endl;
	}
	if(trigger_count) {
		cerr << "Last of " << trigger_count << " trigger(s) to activate the workproc: "
			<< last_work_proc_trigger << endl;
	}
	if(mainwin_count) {
		cerr << "There has(have) been " << mainwin_count << " refresh(es) of the main window toolbar/menus.\n";
	}
	if(level_2_triggers.get_length()) {
		cerr << "Level 2 triggers:\n";
		while((bp = theLevel2Guys())) {
			stringtlist*		theList = &bp->payload;
			stringslist::iterator	ll(*theList);
			emptySymbol*		b;

			cerr << "    " << bp->get_key() << endl ;
			while((b = ll())) {
				cerr << "        " << b->get_key() << endl;
			}
		}
	}
	if(work_proc_servicing_agents.get_length()) {
		cerr << "Level 2 agents:\n";
		while((bp = theAgents())) {
			stringtlist*		theList = &bp->payload;
			stringslist::iterator	ll(*theList);
			emptySymbol*		b;

			cerr << "    " << bp->get_key() << endl;
			while((b = ll())) {
				cerr << "        " << b->get_key() << endl;
			}
		}
	}
}

void refresh_info::reportSyncActions(infoList* theDelegates, infoList* theEnforcers) {
	static int	theReportingLevel = 0;
	int		i;
	infoNode*	tl;

	if(theDelegates) {
		for(i = 0; i < theReportingLevel; i++) {
			cerr << "\t";
		}
		cerr << "(servers)\n";
		theReportingLevel++;
		for(	tl = theDelegates->first_node();
			tl;
			tl = tl->next_node()) {
			reportSyncActions(&tl->payload.theListOfServers , &tl->payload.theListOfAgents);
			if(tl->payload.reason.length()) {
				for(i = 0; i < theReportingLevel; i++) {
					cerr << "\t";
				}
				cerr << "reason: " << tl->payload.reason << endl;
			}
		}
		theReportingLevel--;
	}
	if(theEnforcers) {
		for(i = 0; i < theReportingLevel; i++) {
			cerr << "\t";
		}
		cerr << "(agents)\n";
		theReportingLevel++;
		for(	tl = theEnforcers->first_node();
			tl;
			tl = tl->next_node()) {
			reportSyncActions(&tl->payload.theListOfServers , &tl->payload.theListOfAgents);
			if(tl->payload.reason.length()) {
				for(i = 0; i < theReportingLevel; i++) {
					cerr << "\t";
				}
				cerr << "reason: " << tl->payload.reason << endl;
			}
		}
		theReportingLevel--;
	}
}

void refresh_info::reset() {
	tlistNode*		b;
	tlistslist::iterator	l1(level_2_triggers);
	tlistslist::iterator	l2(work_proc_servicing_agents);
	stringtlist*		theList;

	actsis_count = 0;
	trigger_count = 0;
	mainwin_count = 0;
	while((b = l1())) {
		theList = &b->payload;
		theList->clear();
	}
	while((b = l2())) {
		theList = &b->payload;
		theList->clear();
	}
	level_2_triggers.clear();
	work_proc_servicing_agents.clear();
	last_actsis.undefine();
	last_work_proc_trigger.undefine();
}

extern "C" {

static double	time_offset = 0.0;
static bool	time_offset_set = false;
static double	elapsed_time = 0.0;

void theRefreshTimerProc(XtPointer, XtIntervalId *) {
	/* The timeout has expired, meaning that it is time to refresh
	 * any global display features... */
	if(GUI_Flag) {
		struct timeval	T;
		struct timezone	Z;
		gettimeofday(&T, &Z);
		double s = ((double) T.tv_sec) + 0.000001 * (double) T.tv_usec;

		if(!time_offset_set) {
			time_offset_set = true;
			time_offset = s;
		}
		else {
			elapsed_time = s - time_offset;
		}

		if(!thereHasBeenAnEvent) {
			/* ... provided that the X loop has been "quiet". If the X loop
			 * has NOT been quiet, meaning that at least one Event occurred
			 * since the timer procedure was activated, then we should wait
			 * until things settle down; no point in prettying up the display
			 * while the user is moving frantically. */
			theDelayedWorkProc();
		}
		thereHasBeenAnEvent = false;
		if(refresh_info::ON) {
			if(!--refresh_info::ON) {
				refresh_info::ON = 5;
				refreshInfo.report();
				refreshInfo.reset();
			}
		}
		XtAppAddTimeOut(Context, (unsigned long) 500, theRefreshTimerProc, NULL);
	}
}
} // extern "C"

class eventNode: public Node {
public:
	eventNode() {}
	eventNode(const eventNode &PTP) : Node(PTP) , E(PTP.E) {}
	~eventNode() {}

	Node*		copy() {
		return new eventNode(*this);
	}
	const Cstring&	get_key() const {
		return Cstring::null();
	}
	Node_type	node_type() const {
		return UNKNOWN_EXTENSION;
	}
	XEvent		E;
};

			// list of eventNodes:
static List		events_to_do;
static List		event_bucket;

#define Emax 1000

			// blists of pointer_to_pointers:
static blist		windows_to_configure(compare_function(compare_bpointernodes , false));
static blist		windows_to_expose(compare_function(compare_bpointernodes , false));
			// graphic objects that subscribe to the DS_scroll_graph::Expose() method will
			// register themselves in this list:
blist			windows_we_redraw_ourselves(compare_function(compare_bpointernodes , false));
bool			anImportantEventHasBeenDetected = false;

static void	fill_event_list() {
	while(event_bucket.get_length() < 1000) {
		event_bucket << new eventNode;
	}
}

	// This is an unnecessary embellishment for now because we don't use the subwindow info
	// when repainting; however, we might one day. So...
static void	consolidate_exposure_windows(eventNode *a , eventNode *b) {
	int	newx = b->E.xexpose.x;
	int	newy = b->E.xexpose.y;
	int	oldx = a->E.xexpose.x;
	int	oldy = a->E.xexpose.y;
	int	newX = newx + b->E.xexpose.width;
	int	newY = newy + b->E.xexpose.height;

	if(newx < oldx) {
		a->E.xexpose.x = newx;
		a->E.xexpose.width += oldx - newx;
	}
	if(newy < oldy) {
		a->E.xexpose.y = newy;
		a->E.xexpose.height += oldy - newy;
	}
	if(newX > (a->E.xexpose.x + a->E.xexpose.width)) {
		a->E.xexpose.width = newX - a->E.xexpose.x;
	}
	if(newY > (a->E.xexpose.y + a->E.xexpose.height)) {
		a->E.xexpose.height = newY - a->E.xexpose.y;
	}
}

XEvent *get_event() {
	pointer_to_pointer	*win;
	static XEvent		E;

	while(1) {
		bool shouldBreak = true;

		// debug
		// cerr << "XtAppNextEvent() ...";

		XtAppNextEvent(Context, &E);

		// debug
		// cerr << " kicked out of its slumber!\n";

		win = (pointer_to_pointer *) windows_we_redraw_ourselves.find((void *) E.xany.window);
		if(win) {
			DS_graph	*eobject = (DS_graph *) win->PTR;
			// debug
			// cerr << "get_event: event " << spellEtype(E.type) << " for widget "
			// 	<< eobject->get_key() << "\n";
	 		if(E.type == ConfigureNotify) {

				// cerr << "main: found ConfigureNotify event - important event flag turned on\n";
				anImportantEventHasBeenDetected = true;
				shouldBreak = false;
				if(!windows_to_configure.find((void *) eobject)) {
					windows_to_configure << new bpointernode((void *) eobject, NULL);
				}
			}
			else if(E.type == Expose) {
				anImportantEventHasBeenDetected = true;
				shouldBreak = false;
				// cerr << "main: found Expose event - important event flag turned on\n";
				if(!windows_to_expose.find((void *) eobject)) {
					windows_to_expose << new bpointernode((void *) eobject, NULL);
				}
			}
		}
		if(shouldBreak) {
			break;
		}
	}
	return &E;
}

void process_stored_events() {
	List_iterator	lc(windows_to_configure);
	List_iterator	le(windows_to_expose);
	bpointernode	*bptr;
	while((bptr = (bpointernode *) lc())) {
		DS_graph* eobject = (DS_graph *) bptr->get_ptr();
		// cerr << "process_stored_events: configuring " << eobject->get_key() << "\n";
		eobject->configure_graphics(NULL);
	}
	while((bptr = (bpointernode *) le())) {
		DS_graph* eobject = (DS_graph *) bptr->get_ptr();
		// cerr << "process_stored_events: updating " << eobject->get_key() << "\n";
		eobject->update_graphics(NULL);
	}
	windows_to_configure.clear();
	windows_to_expose.clear();
}

int aButtonIsPressed() {
	return theButtonState == PRESSED;
}

extern Boolean         theWorkProc(void *theWorkProcData);

void	InsertWorkProc(void *with_this_data) {
	if(GUI_Flag) {
		/* man pages:
		 *	XtWorkProcId XtAppAddWorkProc(app_context, proc, client_data)
		 *		XtAppContext app_context;
		 *		XtWorkProc proc;
		 *		XtPointer client_data;
		 *
		 * Intrinsic.h:
		 *	typedef Boolean (*XtWorkProc)(XtPointer)
		 */

		//
		// debug-problems
		//
		// cerr << "weShouldWork = true, inserting theWorkProc...\n";

		theWorkProcId = XtAppAddWorkProc(Context, theWorkProc, with_this_data);
	}
}

static int argc;

int motifLoop() {
	XEvent	*event;


	/*	>>>>>>>> PART 11 <<<<<<<<
	 *
	 *	>>>>>>> THE MOTIF LOOP <<<<<< */


	// we will signal the editor that it's OK
	while(true) {

		// SECTION 1:	see if we should exit. We don't just "exit(1)" because we want the
		// 		destructors to clean up.

		if(APcloptions::theCmdLineOptions().ExitFlag == 2) {

			//
			// fast exit
			//
			perf_report::profile();
			if(APcloptions::theCmdLineOptions().ModelErrorsFound) {
				_exit(-1);
			} else {
				_exit(0);
			}
		} else if(APcloptions::theCmdLineOptions().ExitFlag) {
		 	if(GUI_Flag) {
				// finish processing events
				while(events_to_do.get_length()) {
					eventNode	*e = (eventNode *) events_to_do.first_node();

					event_bucket << e;
				}
			}
			// Not necessary: request goes on the History list after processing.
			//delete request;
      			break;
		}

		// SECTION 2:	see if we should do some work because the GUI is out of sync with the core.
		//		We need to be careful because in the presence of 1000's of activities and/or
		//		violations, weShouldWork() takes time to process. Solution: let weShouldWork
		//		figure out whether theUniversalCallback has been called. If it has, then the
		//		user has done something that requires attention. Else, if the workproc has
		//		been called once, that's good enough.

		if(GUI_Flag) {
			if(weShouldWork()) {
				InsertWorkProc(NULL);
				if(refresh_info::ON) {
					refreshInfo.reportSyncActions(&refresh_info::servers , &refresh_info::agents);
				}
			}

			// The following line is a wrapper around XtAppNextEvent(Context , &event):
			event = get_event();

			if(event->type == ButtonPress) {
				theButtonState = PRESSED;
			} else if(event->type == ButtonRelease) {
				// encourage vertical scrollbars to resize themselves:
				udef_intfc::something_happened() += 1;
				theButtonState = RELEASED;
			}
			thereHasBeenAnEvent = true;

			if(!UI_mainwindow::thisEventIsSynthetic) {
				XtDispatchEvent(event);
			} else {
				UI_mainwindow::thisEventIsSynthetic = false;
			}
		} else {

			//
                        // We are in "batch mode". -nogui
                        //
			if(!APcloptions::theCmdLineOptions().ExitFlag) {

				//
                                // Cleaned Up Statement.  -nogui is supposed to run with or
				// without the -talk statement.  If there have been files read in,
				// that means that we're in true batch mode, no listen.  So we read
				// in from the script files and exit.
				//
                                if(APcloptions::FilesToRead().get_length() != 0) {
					/* Read from Files */
					theWorkProc(NULL);
					if(refresh_info::ON) {
						refreshInfo.reportSyncActions(
							&refresh_info::servers,
							&refresh_info::agents);
					}
				}

                                //
				// passing -nogui by itself would cause app to hang with new setup.
				// here's the fix.
				//
				else {
						QUITrequest request;
						request.process();
				}
			}
		}

		/* DON'T USE! (disappearing cursor!!) -- this function is
		 * incompatible with the Motif Toolkit.
		 *
		 * while(XCheckIfEvent(XtDisplay(TopLevelWidget) , &event ,
		 *	dummy_pred_func , NULL))
		 *	XtDispatchEvent(&event);
		 */

	}

	// 		<<<<<<< POOL FITOM EHT >>>>>>

	if(APcloptions::theCmdLineOptions().ModelErrorsFound) {
		return -1;
	} else {
		return 0;
	}
}

void modify_subsystems() {
	LegendObject::theLegendObjectConstructorHandle = Lego::theLegoConstructor;
	// Violation_info::Violation_info_handle = Vinfo::Vinfo_constructor;

	errors::clientShouldStoreMessage = client_errors::ExpectMore;
	errors::clientShouldDisplayMessage = client_errors::DisplayMessage;
	req_intfc::ABSTRACT_ALLhandler = UI_mainwindow::ABSTRACT_ALLhandler;
	req_intfc::ADD_RESOURCEhandler = UI_mainwindow::ADD_RESOURCEhandler;
	req_intfc::CLOSE_ACT_DISPLAYhandler = UI_mainwindow::CLOSE_ACT_DISPLAYhandler;
	req_intfc::CLOSE_RES_DISPLAYhandler = UI_mainwindow::CLOSE_RES_DISPLAYhandler;
	req_intfc::COMMONhandler = UI_mainwindow::COMMONhandler;
	req_intfc::DELETE_ALL_DESCENDANTShandler = UI_mainwindow::DELETE_ALL_DESCENDANTShandler;
	req_intfc::DETAIL_ALLhandler = UI_mainwindow::DETAIL_ALLhandler;
	req_intfc::RES_LAYOUThandler = UI_mainwindow::RES_LAYOUThandler;
	req_intfc::ACT_DISPLAY_HORIZONhandler = UI_mainwindow::ACT_DISPLAY_HORIZONhandler;
	req_intfc::DISPLAY_SIZEhandler = UI_mainwindow::DISPLAY_SIZEhandler;
	req_intfc::FIND_RESOURCEhandler = UI_mainwindow::FIND_RESOURCEhandler;
	req_intfc::NEW_ACT_DISPLAYhandler = UI_mainwindow::NEW_ACT_DISPLAYhandler;
	req_intfc::NEW_HORIZONhandler = UI_mainwindow::NEW_HORIZONhandler;
	req_intfc::NEW_RES_DISPLAYhandler = UI_mainwindow::NEW_RES_DISPLAYhandler;
	req_intfc::OPEN_FILEhandler = UI_mainwindow::OPEN_FILEhandler;
	req_intfc::PAUSEhandler = UI_mainwindow::PAUSEhandler;
	req_intfc::PURGEhandler = UI_mainwindow::PURGEhandler;
	req_intfc::REMOVE_RESOURCEhandler = UI_mainwindow::REMOVE_RESOURCEhandler;
	req_intfc::REMODELhandler = UI_mainwindow::REMODELhandler;
	req_intfc::SCHEDULEhandler = UI_mainwindow::SCHEDULEhandler;
	req_intfc::UNSCHEDULEhandler = UI_mainwindow::UNSCHEDULEhandler;
	req_intfc::ENABLE_SCHEDULINGhandler = UI_mainwindow::ENABLE_SCHEDULINGhandler;
	req_intfc::RESOURCE_SCROLLhandler = UI_mainwindow::RESOURCE_SCROLLhandler;
	req_intfc::SAVE_FILEhandler = UI_mainwindow::SAVE_FILEhandler;
	req_intfc::SELECT_RES_LEGENDhandler = UI_mainwindow::SELECT_RES_LEGENDhandler;

	Dsource::derivedDataConstructor = derivedDataDS::addDerivedData;
	Dsource::derivedDataDestructor  = derivedDataDS::destroyDerivedData;

	/* Doing this requires linking with the XmlRpc library, which is not
	 * necessarily available. This is something we will only support in
	 * the APcore executable.
	 *
	 * activityInstanceSerializationHandle = getActInstanceAsXMLstring;
	 * activityDefsSerializationHandle = getAllActAttrParamDefsAsXMLstring; */

}

int main(int a, char **v) {
	int		ret_val;
	long		as_long;

	// start timers
	perf_report::initialize();

	if(a == 2 && !strcmp(v[1], "-version")) {
		cout << get_apgen_version_build_platform() << "\n";
		return 0;
	}

	argc = a;

	//
	// We used to do this:
	//
	// pthread_create(&theThread, NULL, thread_main_gui, (void *) v);

	//
	// ... but no longer.
	//
	ret_val = thread_main_gui(a, v);

	//
	// after we get out of the next wait, we will know that command-line
	// arguments have been processed by thread_main_gui
	//

#ifdef GTK_EDITOR
	if(gS::theGtkThread) {
	    {
		lock_guard<mutex>	lock1(*gS::get_gtk_mutex());
		gS::quit_requested = true;
	    }
	    gS::theGtkThread->join();
	    delete gS::theGtkThread;
	}
#endif /* GTK_EDITOR */

	perf_report::profile();

	//
	// This was not included in delete_subsystems() because
	// perf_report::profile() needs it:
	//
	Cstring::delete_permanent_strings();

	return ret_val;
}

int	thread_main_gui(int argc, char** argv) {
	stringtlist	theListOfFlags;
	Cstring		theErrors;
	int		retval;

	extern int APmain1(int, char **, Cstring &any_errors);
	extern int APmain2(Cstring& any_errors, bool ATM);
	extern int APmain4();
	extern void disable_discrepancy_popup();

	/* We do not want to create the default resource factories, because
	 * they do not include the derived data that will allow notifying the
	 * resource displays that they are stale. */
	create_subsystems("base ");

	// initialization and argument processing
	if((retval = APmain1(argc, argv, theErrors))) {
		cerr << theErrors;
		return -1;
	}
	cout << get_apgen_version_build_platform() << "\n";


	// it is essential to do this before creating subsystems
	register_all_internal_functions();

	if(APmain2(theErrors, /* ATM = */ true)) {
		cerr << theErrors;
		return -1;
	}

	if(APcloptions::theCmdLineOptions().disable_discrepancy_popup_panel) {
		disable_discrepancy_popup();
	}


	/* Here we will install our own resource factories, which have
	 * been defined in RD_sys.C. */
	modify_subsystems();

	if(APcloptions::theCmdLineOptions().test.length()) {
		apgen_test(APcloptions::theCmdLineOptions().test, APcloptions::theCmdLineOptions().testoptions);
	} else {
		ARGC = argc;
		ARGV = argv;
		TopLevelWidget = new motif_widget("Atm", xmTopLevelWidgetClass, NULL, NULL, 0, FALSE);
		load_resources(TopLevelWidget);

		MW = new UI_mainwindow("mainWindowInstance" , TopLevelWidget);
		XtRealizeWidget(TopLevelWidget->widget);
		MW->initialize_hopper();
		if(GUI_Flag) {
			XtAppAddTimeOut(Context, (unsigned long) 500, theRefreshTimerProc, NULL);
			fill_event_list();
		}
		APcloptions::getFlags(theListOfFlags);
		if(theListOfFlags.find("refreshinfo")) {
			refresh_info::ON = 1;
		}
		if(theErrors.length()) {
			errors::Post("Initialization Errors", theErrors);
		}
		// the main loop
		if(motifLoop()) {

			//
			// errors found
			//
			return -1;
		}
		// wrapping up
		// delete main window first, to get rid of all the graphics-related stuff
		MW->remove_widgets();
		delete MW;
		delete TopLevelWidget;
	}
	if(APmain4()) {
		return -1;
	}
	return 0;
}

// in ../apcore/templates/test.C:
extern int test_harness(int, char*[]);
extern int (*interp_windows_handle)(int argc, char* argv[]);
extern int test_interp_windows(int argc, char* argv[]);

void	apgen_test(const Cstring& which_test, const stringslist& options) {
	char**			args = (char**) malloc(sizeof(char*) * (options.get_length() + 1));
	int			ret;
	stringslist::iterator	it(options);
	emptySymbol*		s;
	int			i = 0;

	// install the handle for testing interpolated windows:
	interp_windows_handle = test_interp_windows;

	args[i++] = (char*) *which_test;
	while((s = it())) {
		args[i++] = (char*) *s->get_key();
	}
	try {
		ret = test_harness(options.get_length() + 1, args);
		cout << "apgen_test: returned value = " << ret << "\n";
	}
	catch(eval_error Err) {
		cout << "apgen_test Error: \n" << Err.msg;
	}
	free((char*) args);
}
