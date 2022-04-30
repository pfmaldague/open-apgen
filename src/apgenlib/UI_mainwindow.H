#ifndef MAINWINDOWCLASSDERIVED_H
#define MAINWINDOWCLASSDERIVED_H

#include "APrequest_handler.H"
#include "C_global.H"
#include "UTL_defs.H"
#include "UI_mainwindowbx.H"
#include "UTL_time.H"

// #include "C_list.H"  // Note BKL this will be need if your ListOfUnabbreviatedMessages below is used
                        // Note BKL will need to instantiate this in UI_mainwindow.C
			// as List UI_mainwindow::ListOfUnabbreviatedMessages; // added by BKL

class ACT_sys;
class activity_display;
class resource_display;
class activity_display;
class sasf_popup;
class pwd_popup;

typedef enum {
	IN_ANY_MODE = 0,
	IN_MANUAL_MODE = 1,
	// WITH_RESOURCE = 2,
	// irrelevant for scrolled resources (there is always room) but keep
	// to ease the transition:
	WITHOUT_RESOURCE = 4 }
		resourceDisplayStatus;

class UI_mainwindow : public UI_mainwindowbx {
protected:

	void				initialize();
	void				initialize_hopper();
	apgen::RETURN_STATUS		getRDControlInfo(RD_YSCROLL_MODE &mode,int &resolution);

	static int			weFoundSomethingToDoInADelayedManner;
public:
	static void			bringToolbarButtonsInSync();

	static void			send_a_dummy_event_to_unblock_XtAppNextEvent();
	static int			thisEventIsSynthetic;
	static int			isOutOfSync(infoList*);
	static int			foundSomethingToDoInADelayedManner() { return weFoundSomethingToDoInADelayedManner; }
protected:
	static void			zoomSelectedDisplays(double factor);
	static int			findNthCreatedAD(int n);

public:
	//BKL-11/97 MADE THE FOLLOWING PUBLIC:

					/* if last arg is specified nonzero, the first RD (if any) that matches
					 *	ALL the specified (OR'ed) criteria will be returned: */
	static void			findFirstSelectedRD(	int &adIndex,
								int &rdIndex,
								resourceDisplayStatus theStatusOfTheResourceDisplay = IN_ANY_MODE);
	static int			get_default_timesystem(Cstring &l);

	static int			findFirstSelectedAD();
	static int			findFirstVisibleAD();
					// Will fill list with up to 3 String_nodes with labels A1, A2, A3 (whichever ones are selected)
	static void			findAllSelectedADs(List &);
	static void			deallocateResourceDisplays(int adIndex);
	static void			deallocateSelectedResourceDisplays(int adIndex);
	static void			deallocateActivityDisplays();
	static void			desensitizeADButtons();
	static void			desensitizeActiveADButtons();
	static int			allocateResourceDisplay(int adIndex);

					// used to be 3, then 4 to allow 1 pointer to the hopper, then 5 to accommodate type editor
	static activity_display*	activityDisplay[TOTAL_NUM_ACTSYS];
	static pwd_popup*		passwordDisplay;
	static void			purge();

    //
    // Callbacks to interface with Motif.
    //

	static void			openButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			savePlanFilesButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			integratePlanFilesButtonActivateCallback(Widget, callback_stuff *, void *);
	static void			writeIntegratedPlanFileButtonActivateCallback(Widget, callback_stuff *, void *);
        static void                     exportDataButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			genSASFButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			genTOLButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			genXMLTOLButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			genCsourceButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			printButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			purgeButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			quitButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			fastquitButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			opsrevButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			seqrButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			unfreezeButtonActivateCallback(		Widget, callback_stuff *, void *);

	static void			cutButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			copyButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			pasteButtonActivateCallback(		Widget, callback_stuff *, void *);

	static void			newActivityButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			abstractActivityButtonActivateCallback(Widget, callback_stuff *, void *);
	static void			detailActivityButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			editActivityButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			activityDefinitionsButtonActivateCallback(Widget, callback_stuff *, void *);
	static void			NewResourceDefOK(			Widget, callback_stuff *, void *);

	static void			activityLockButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			preferencesButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			showActivityNameOrTypeActivateCallback(	Widget, callback_stuff *, void *);
	static void			ungroupActivitiesCallback(		Widget, callback_stuff *, void *);
	static void			activityDesignButtonActivateCallback(	Widget, callback_stuff *, void *);

	static void			hopperButtonActivateCallback(		Widget, callback_stuff *, void *);

	static void			addResourceButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			removeResourceButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			remodelButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			resourceDefButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			chooseResourceToDisplay(		Widget, callback_stuff *, void *);
	static void			aafScriptButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			chooseScriptToExecute(			Widget, callback_stuff *, void *);
	static void			editGlobalsButtonActivateCallback(	Widget, callback_stuff *, void *);

	static void			newADButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			newRDButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			newLegendButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			closeSADButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			closeSRDButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			deleteSelectedLegendsButtonActivateCallback(Widget, callback_stuff *, void *);
	static void			unselectAllLegendsButtonActivateCallback(Widget, callback_stuff *, void *);
	static void			unsquishSelectedLegendsButtonActivateCallback(Widget, callback_stuff *, void *);
	static void			squishSelectedLegendsButtonActivateCallback(Widget, callback_stuff *, void *);
	static void			flattenSelectedLegendsButtonActivateCallback(Widget, callback_stuff *, void *);
	static void			expandSelectedLegendsButtonActivateCallback(Widget, callback_stuff *, void *);
	static void			zoominButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			zoomoutButtonActivateCallback(		Widget, callback_stuff *, void *);
	static void			absolutePanZoomButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			resOptionsButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			resAbsScrollZoomButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			timeSystemButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			time_system_ok(				Widget, callback_stuff *, void *);

	static void			apgenHelpButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			aboutApgenButtonActivateCallback(	Widget, callback_stuff *, void *);

#ifdef INCLUDE_ADEF
	static void			aboutComponentsButtonActivateCallback(	Widget, callback_stuff *, void *);

#endif

#ifdef OBSOLETE
	static void			newConstraintButtonActivateCallback(	Widget, callback_stuff *, void *);
	static void			deleteConstraintButtonActivateCallback(	Widget, callback_stuff *, void *);
					// to support CONSTRAINTrequest::process_middle()
	static void			turnConstraintsOnOff(int);
#endif /* OBSOLETE */

	static void			scheduleButtonActivateCallback(		Widget, callback_stuff *, void *);

	// other static

	static void			setStatusBarMessage(const Cstring & message);
	static void			appendStatusBarMessage(const Cstring & message);
	static void			changeCursor(int cursorIndex);
	static void			update_all_time_system_labels();

	static sasf_popup		*&sasf_popup_instance();

	UI_mainwindow(const char *, motif_widget *);
	~UI_mainwindow();

	static UI_mainwindow	*mainWindowObject;

	static void		allocateActivityDisplay(CTime startTime, CTime duration, int freeze_start_time = 0);

				// for use by purge:
	void			clearActivityDisplays();
	int			countManualSelectedRDs();
	static void		zoomPanDisplay(int WhichOne, CTime startTime, CTime duration);
	static void		zoomPanGiantWindow(CTime startTime, CTime duration);
	static void             zoomPanHopper(CTime startTime, CTime duration);
	static void		panSelectedDisplays(CTime startTime);
	void			zoomScrollSelResourceDisplays(double min, double span);
	static void		setApplicationSize(int width, int height);
	static void		getApplicationSize(int &width, int &height);
	/**
	 * The following 2 functions have been moved to 'private' so that only friends of the UI_mainwindow
	 * class can invoke them. The idea is to migrate ALL error display tasks to the action_request class,
	 * leading to more modular design and paving the way for a GUI-less version of APGEN.
	 */
private:
	 static void	displayMessage(const Cstring &windowTitle, const Cstring &messageTitle, const Cstring &message);
	 static void	displayMessage(const Cstring &windowTitle, const Cstring &messageTitle, const List & messages);
public:
	// void			addResourceToDisplay(const Cstring & resource_to_add);
	void			setRDControlInfo(RD_YSCROLL_MODE mode,int resolution);
	void			purgeResourceDisplays();

	apgen::RETURN_STATUS	printButtonUpdateSelections();

	// req_intfc support:
	static void	ABSTRACT_ALLhandler(Action_request *, int);
	static void	ADD_RESOURCEhandler(Action_request *, int);
	static void	CLOSE_ACT_DISPLAYhandler(Action_request *, int);
	static void	CLOSE_RES_DISPLAYhandler(Action_request *, int);
	static void	COMMONhandler(Action_request *, int);
	static void	DETAIL_ALLhandler(Action_request *, int);
	static void	DELETE_ALL_DESCENDANTShandler(Action_request *, int);
	static void	FIND_RESOURCEhandler(Action_request *, int);
	static void	NEW_ACT_DISPLAYhandler(Action_request *, int);
	static void	NEW_HORIZONhandler(Action_request *, int);
	static void	NEW_RES_DISPLAYhandler(Action_request *, int);
	static void	OPEN_FILEhandler(Action_request *, int);
	static void	PAUSEhandler(Action_request *, int);
	static void	PURGEhandler(Action_request *, int);
	static void	REMOVE_RESOURCEhandler(Action_request *, int);
	static void	REMODELhandler(Action_request *, int);
	static void	SCHEDULEhandler(Action_request *, int);
	static void	UNSCHEDULEhandler(Action_request *, int);
	static void	ENABLE_SCHEDULINGhandler(Action_request *, int);
	static void	RESOURCE_SCROLLhandler(Action_request *, int);
	static void	SAVE_FILEhandler(Action_request *, int);
	static void	SELECT_RES_LEGENDhandler(Action_request *, int);
	static void	RES_LAYOUThandler(
					const Cstring&		layout_id,
					const vector<Cstring>&	res_names,
					const vector<int>&	res_display_heights,
					const Cstring&		source_file,
					int			line_number);
	static void	ACT_DISPLAY_HORIZONhandler(
					const CTime_base&	start,
					const CTime_base&	duration);
	static void	DISPLAY_SIZEhandler(
					const int&		x,
					const int&		y);


	friend int thread_main_gui(int, char**);
	friend class	UI_exec;
			/**
			 * to allow the action request class to display error messages (Note:
			 * the displayMessage() methods were made private to make sure that only
			 * a few 'class friends' would have the authority to display error messages,
			 * as opposed to just about anybody as in previous incarnations of the
			 * Apgen code.
			 */
	friend class	Action_request;
			// for UI_motif_widget.C:
	friend void	theUniversalCallback(Widget, callback_stuff *, void *);
			// to assist with programming in isOutOfSync() and bringToolbarButtonsInSync():
	friend class	auto_flag_raiser; };

	/* The purpose of the following class is to let apgen report to the user what kind of
	 * refresh action was taken and by whom. Originally this was for DELAYED events only,
	 * but later I decided that all refresh-related events needed to be captured. */
class refresh_info {
public:
	refresh_info();
	~refresh_info() {}

	static void		explain_problem();
				// Set by a the -refreshinfo command-line option.
	static int		ON;
				/* servers detect out-of-sync components; agents bring them in sync.
				 * In hindsight, I think this is the wrong philosophy. It's not so
				 * important _who_ fixes and detects problems; what is important is
				 * that each problem should be addressed. We don't want anything
				 * falling through the cracks.  */
	static infoList		servers, agents;

				/* List of bstringnodes indicating out-of-sync problems. After
				 * theWorkProc has been invoked, no problems should remain, or at
				 * least SOMETHING should change in the nature of the problems. */
	static stringtlist	problems;

				/** Resets the counts and string(s) to get ready for next
				 * "information accumulation period". */
	void			reset();
	void			report();
	void			reportSyncActions(infoList*, infoList*);
				// Adds a tlistNode, assuming it doesn't exist already.
	void			add_level_2_trigger(const Cstring& triggering_object, const Cstring& triggering_reason);
	void			add_servicing_agent(const Cstring& agent, const Cstring& reason_called);
	Cstring			last_actsis, last_work_proc_trigger;
	tlisttlist		work_proc_servicing_agents;
	tlisttlist		level_2_triggers;
	int			actsis_count,
				trigger_count,
				mainwin_count; };

#endif
