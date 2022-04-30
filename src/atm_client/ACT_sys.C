#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define apDEBUG

#include "apDEBUG.H"
#include <stdlib.h>

#include "ACT_exec.H"
#include <ActivityInstance.H>
#include "ACT_sys.H"
#include "APmodel.H"
#include "DSsource.H"
#include "action_request.H"
// #include "CON_exec.H"
#include "CON_sys.H"
#include "C_global.H"
#include "Multiterator.H"
#include "RD_sys.H"
#include "UI_activitydisplay.H"
#include "UI_ds_draw.H"
#include "UI_ds_timeline.H"
#include "UI_exec.H"
#include "UI_mainwindow.H"

using namespace std;

#define BXC_LEGEND_WIDTH_P2 200
#define BXC_LEGEND_WIDTH 200
#define NULLPLAN    "New"

extern UI_exec*		UI_subsystem;
extern int		LNDRAWWIDTH;

			// in IO_seqtalk.C:
extern blist		theTimeWindows;
extern int		verify_window_list(const clist &);
extern void		dump_window_list(const clist &);
extern int		UpdateBusyLine;

			// in RES_eval.C:
extern blist		ListOfGlobalSymbols;

//in UI_ds_timeline.C
extern int SelectionFrozen;

// GLOBALS

List	&ACT_sys::Registration() {
	static List	R;
	return R; }

	// blist of btags: (old static list, used for compatibility)
blist	&ACT_sys::Hoppers() {
	static blist	H(compare_function(compare_bstringnodes, false));
	return H; }

			// use this now:
ACT_sys			*ACT_sys::theHopper = NULL;

List_iterator	*ACT_sys::getLegendListIterator() {
	return new List_iterator(Dsource::theLegends()); }

Lego	*ACT_sys::findTheLegend(const Cstring & t) {
	return (Lego *) Dsource::theLegends().find(t);
}

// void	ACT_sys::activateTheDelayedWorkProc() {
// 	if(is_a_giant_window()) {
// 		DBG_NOINDENT("Giant window " << get_key() << ": delayed workproc activated.\n"); }
// 	delayed_flag = 1; }

// int	ACT_sys::TheDelayedWorkProcHasBeenActivated() {
// 	return delayed_flag; }

// void	ACT_sys::resetTheDelayedWorkProc() {
// 	delayed_flag = 0; }


// END OF GLOBALS

		// C-STYLE STATICS:
static		Arg      args[56];
static		Cardinal ac=0;
static		Boolean  argok = False;
extern		XtPointer CONVERT(Widget, const char *, const char *, int, char *);

			// in main.C:
int			aButtonIsPressed();
extern refresh_info	refreshInfo;

	/* This constructor is given a motif_widget parent of the xmForm class that spans
	* the whole width of the APGEN display and (initially) the whole height as well
	* (except for menubar and toolbar at the top and the display label at the bottom).
	*
	* The constructor is only invoked from the activity_display constructor (in
	* UI_activitydisplay.C). Note that an ACT_sys is always part of a set of siblings; a set
	* of siblings is always accompanied by a (unique) list_of_siblings object that is used
	* to coordinate scrolling and other updating functions within that group.
	*
	* The list_of_siblings object is explicitly created by activity_display BEFORE invoking
	* the ACT_sys constructor. The list_of_siblings constructor is given a pointer to the
	* activity_display object.
	*/
ACT_sys::ACT_sys(	const Cstring		&n,
			motif_widget		*_activityDisplayForm,
			activity_display	*ad_parent,
			list_of_siblings	*ls)
		: DS_lndraw(n, _activityDisplayForm, ls),
		never_scrolled(true),
		NumberOfLinesActuallyDrawn(0),
		never_configured(true),
		timezone(TIMEZONE_UTC),
		AD_parent(ad_parent),
		thePointersToTheNewDsLines(compare_function(compare_bpointernodes, false)),
		activeCount(0),		// for checking out-of-syncness
		decomposedCount(0),	// for checking out-of-syncness
		abstractedCount(0),	// for checking out-of-syncness
		delayed_flag(0) {
	DBG_INDENT("ACT_sys(\"" << get_key() << "\")::ACT_sys...\n");
    if(GUI_Flag) {
	  
	CTime			t1;
	DS_scroll_graph		*legend_as_ds_scroll_graph;

	// NOTE: a lot of the following motif options should be taken out; they are left over from
	//	 BX.
	ac = 0;
	XtSetArg(args[ac], XmNborderWidth,	0); ac++;
	XtSetArg(args[ac], XmNbackground,	ResourceData.papaya_color); ac++;
	XtSetArg(args[ac], XmNx,		202); ac++;
	XtSetArg(args[ac], XmNy,		52); ac++;
	XtSetArg(args[ac], XmNwidth,		261); ac++;
	XtSetArg(args[ac], XmNheight,		261); ac++;
	XtSetValues(widget, args, ac);
	*_activityDisplayForm ^ 52 ^ *this ^ 19 ^ *_activityDisplayForm ^ endform;
	*_activityDisplayForm < BXC_LEGEND_WIDTH_P2 < *this < 19 < *_activityDisplayForm < endform;

	// We no longer tell the time mark panel what to do; it should figure that out by itself.

	// this should make Eric (Newman) happy:
	model_intfc::FirstEvent(t1);

	// Don't do this anymore: let the Siblings object figure that out through its isOutOfSync method...
	// Siblings->set_time_parameters(t1, ONEDAY);

	/* IMPORTANT NOTE: a rather unusual technique is used to manage the "legend_display" widget.
	Instead of managing it directly, there is a separate, non-graphic structure that is implemented
	as a list: the "ACT_sys::theLegends" list of legend_object Nodes. The legend_object objects know who they are
	and which activities are drawn on them; they are truly the "legend objects". Part of the reason
	for doing this is that I wanted to avoid drawing hundreds (literally!!) of toggle buttons in
	a huge scrolled widget in case the user wants to display very complex plans. Therefore, the
	motif_widget that implements the legends only knows about visible legends; the "shadow" objects
	in ACT_sys::theLegends are the ones which know everything. Since they don't have to be displayed,
	there can be arbitrarily many of these (to within RAM exhaustion...) */

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNbackground, ResourceData.peach_color); ac++;
	XtSetArg(args[ac], XmNnavigationType, XmNONE); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_GROW); ac++;
	XtSetArg(args[ac], XmNx, 2); ac++;
	XtSetArg(args[ac], XmNy, 52); ac++;
	XtSetArg(args[ac], XmNwidth, BXC_LEGEND_WIDTH); ac++;
	XtSetArg(args[ac], XmNheight, 261); ac++;
	legend_display = new DS_legend(n + "_LEGEND_DISPLAY", _activityDisplayForm,
		this, (void *) args, ac);
	legend_as_ds_scroll_graph = legend_display->get_this();

	DBG_NOINDENT("ACT_sys::ACT_sys: passing legend to addsibling...\n");
	*Siblings << new sibling(legend_as_ds_scroll_graph);
	
	// PFM 2/23: the following call used to be made just PRIOR to defining times...
	DBG_NOINDENT("ACT_sys::ACT_sys: creating timemark...\n");
	Siblings->timemark =
		new DS_timemark(get_key() + "_TIME_MARK", _activityDisplayForm, this);

	DBG_NOINDENT("ACT_sys::ACT_sys: passing timemark to addsibling...\n");
	*Siblings << new sibling(Siblings->timemark);

	DBG_NOINDENT("Act_sys::Act_sys: creating constraint_display...\n");
	constraint_display = new CON_sys(this, get_key() + "_CONSTRAINT_DISPLAY",
					 _activityDisplayForm);
	DBG_NOINDENT("Act_sys::Act_sys: pass constraint_display to addsibling...\n");
	*Siblings << new sibling(constraint_display);

	// The following is stuff that used to be in UI_activitydisplay.C:
	legend_as_ds_scroll_graph->add_property(CHILD_OF_A_FORM);
	*_activityDisplayForm ^ 52 ^ *legend_as_ds_scroll_graph ^ 19 ^ *_activityDisplayForm ^ endform;
	*_activityDisplayForm < 2 < *legend_as_ds_scroll_graph < endform;
	legend_as_ds_scroll_graph->add_property(FORM_PARENT);


	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNbackground, 
		CONVERT(widget, "White", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNnavigationType, XmNONE); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNx, 202); ac++;
	XtSetArg(args[ac], XmNy, 42); ac++;
	XtSetArg(args[ac], XmNwidth, 261); ac++;
	XtSetArg(args[ac], XmNheight, 10); ac++;
	XtSetValues(constraint_display->widget, args, ac);
	
	*_activityDisplayForm ^ 42 ^ *constraint_display ^ endform;
	*_activityDisplayForm < BXC_LEGEND_WIDTH_P2 < *constraint_display < 19 < *_activityDisplayForm < endform;

	*_activityDisplayForm ^ 52 ^ *Vscrollbar ^ 19 ^ *_activityDisplayForm ^ endform;
	*_activityDisplayForm < 202 < *Hscrollbar < 19 < *_activityDisplayForm < endform;

	ac = 0;
	XtSetArg(args[ac], XmNleftAttachment, XmATTACH_OPPOSITE_FORM); ac++;
	XtSetArg(args[ac], XmNleftOffset, -19); ac++;
	XtSetValues(Vscrollbar->widget, args, ac);

	ac = 0;
	XtSetArg(args[ac], XmNtopAttachment, XmATTACH_OPPOSITE_FORM); ac++;
	XtSetArg(args[ac], XmNtopOffset, -19); ac++;
	XtSetValues(Hscrollbar->widget, args, ac);

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNbackground, ResourceData.peach_color); ac++;
	XtSetArg(args[ac], XmNnavigationType, XmNONE); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNx, 202); ac++;
	XtSetArg(args[ac], XmNy, 2); ac++;
	XtSetArg(args[ac], XmNwidth, 261); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetValues(Siblings->timemark->widget, args, ac);

	*_activityDisplayForm < BXC_LEGEND_WIDTH_P2 < *Siblings->timemark < 19 < *_activityDisplayForm < endform;
	*_activityDisplayForm ^ 2 ^ *Siblings->timemark ^ endform;
	// end of insertion from UI_activitydisplay.C
  
	add_event_handler(Motion, PointerMotionMask, this);

	/* KeyPressMask added 2/28/99 PFM to handle motif crashes on linux when mouse button
	 * is pressed while the control key is down. */
	add_event_handler(		Input,	// see DS_lndraw::Input() below; starts a chain of pass-through
						// functions ending with DS_lndraw::Event() in UI_ds_timeline.C
					ButtonPressMask | ButtonReleaseMask | KeyPressMask,
					this);
	add_keyboard_and_mouse_handler(rightButtonClickCB, // see DS_lndraw::rightButtonClick in UI_ds_timeline.C
					// BTTN3_DN,
					BTTN3_UP,	// attempt to avoid contorsions in order to
							// activate menu selection when running
							// remotely on solaris
					NO_MODIFIER,
					this);
	add_keyboard_and_mouse_handler(	Motion,
					FOCUS,
					NO_MODIFIER,
					this);
	}
    // LET's NOT FORGET THIS:
    *Siblings << new sibling(this);
    DBG_NOINDENT("Adding this to Registration list (will remove if a Hopper)\n");
    Registration() << new Pointer_node(this, NULL);
    DBG_UNINDENT("ACT_sys::ACT_sys... DONE\n"); }

ACT_sys::~ACT_sys() {}

ACT_hopper::ACT_hopper(	const Cstring		&n,
			motif_widget		*_activityDisplayForm,
			const Cstring		&HID,
			activity_display	*myAD,
			list_of_siblings	*ls)
	: ACT_sys(n, _activityDisplayForm, myAD, ls) {
	// the ACT_sys constructor put it there:
	DBG_NOINDENT("Removing this from Registration list (we are a Hopper)\n");
	Pointer_node	*ptr = (Pointer_node *) Registration().find((void *) this);

	HopperID = HID;
	delete ptr;
	if(HopperID == "0Hopper") {
		theHopper = this; }
	DBG_NOINDENT("Adding this to Hopper list with ID " << HopperID << endl);
	Hoppers() << new btag(HopperID, this); }

void DS_lndraw::Input(Widget, callback_stuff *client_data, void *call_data) {
	if(GUI_Flag) {
	DS_lndraw*	eobject = (DS_lndraw *) client_data->data;
	XEvent*		event = (XEvent *) call_data;

	udef_intfc::something_happened() += 5;
	eobject->Event(event); } }

template<class keyClass, class nbase, class owner>
nbase* find_at_or_after(tlist<keyClass, nbase, owner>& L, const CTime_base& T) {
	if(!keyClass::reacts_to_time_events()) {
		throw(eval_error("find_at_or_after: keyClass does not react to time events.\n"));
	}
	nbase*		current_node = L.root;
	nbase*		last_later_node = NULL;
	CTime_base	S;

	if(!current_node) return NULL;

	do {
		if(T < (S = current_node->getKey().getetime())) {
			last_later_node = current_node;
			current_node = current_node->get_left();
		} else if(T == S) {
			while(current_node->get_left() && T == current_node->get_left()->getKey().getetime()) {
				current_node = current_node->get_left(); }
			return current_node;
		} else {
			current_node = current_node->get_right(); }
		} while(current_node);
	return last_later_node;
}

template<class keyClass, class nbase, class owner>
nbase* find_at_or_before(tlist<keyClass, nbase, owner>& L, const CTime_base& T) {
	if(!keyClass::reacts_to_time_events()) {
		throw(eval_error("find_at_or_before: keyClass does not react to time events.\n"));
	}
	nbase*		current_node = L.root;
	nbase*		last_earlier_node = NULL;
	CTime_base	S;

	if(!current_node) return NULL;

	do {
		if(T > (S = current_node->getKey().getetime())) {
			last_earlier_node = current_node;
			current_node = current_node->get_right();
		} else if(T == S) {
			while(current_node->get_right() && T == current_node->get_right()->getKey().getetime()) {
				current_node = current_node->get_right(); }
			return current_node;
		} else {
			current_node = current_node->get_left();
		}
	} while(current_node);
	return last_earlier_node;
}

synchro::problem*	ACT_sys::detectActivityListProblem() {
	if(isVisible()) {
		bool			do_something = false;
		ActivityInstance*	request;
		Cstring			reason(get_key());
		status_aware_iterator	theActiveInsts(eval_intfc::get_act_lists().get_all_active_iterator());
		status_aware_iterator	theDecomposedInsts(eval_intfc::get_act_lists().get_decomposed_iterator());
		status_aware_iterator	theAbstractedInsts(eval_intfc::get_act_lists().get_abstracted_iterator());
		status_aware_iterator	thePendingInsts(eval_intfc::get_act_lists().get_clipboard_iterator());

		check_top_level_pointers();
		// debug-problems
#ifdef DEBUG_PROBLEMS
		cerr << "theWorkProc: checking whether activities->areDisplayedCorrectly() in " << this->get_key()
			<< "; checking " << eval_intfc::get_act_lists().get_active_length() << " instance(s)...\n";
#endif /* DEBUG_PROBLEMS */
		DBG_NOINDENT("detectActivityListProblem: checking whether activities->areDisplayedCorrectly() in " << get_key() << ";\n");
		DBG_NOINDENT("    checking " << eval_intfc::get_act_lists().get_active_length() << " active, "
				<< eval_intfc::get_act_lists().get_decomposed_length() << " decomposed, "
				<< eval_intfc::get_act_lists().get_abstracted_length() << " abstracted, "
				<< eval_intfc::get_act_lists().get_clipboard_length() << " clipped, "
				<< eval_intfc::get_act_lists().get_brand_new_length() << " brand new acts\n");

			/* We need to use iterators that include all activities, especially those that
			 * have been put into the hopper ('unsecheduled') */
			 while((request = (ActivityInstance*) theActiveInsts())) {
				if(!((derivedDataDS *) request->dataForDerivedClasses)->isDisplayedCorrectlyIn(this)) {
					DBG_NOINDENT("request " << request->identify() << " not displayed correctly in "
						<< get_key() << endl);
					// debug-problems
					// cerr << "request " << request->identify() << " not displayed correctly in "
					// 	<< get_key() << endl;
					if(!do_something) {
						reason << ": " << request->get_unique_id() << " not displayed correctly; "; }
					do_something = true;
					((derivedDataDS *) request->dataForDerivedClasses)->update_ds_line_coordinates(); }
				else {
					DBG_NOINDENT("request " << request->identify() << " OK\n"); } }
			while((request = (ActivityInstance*) theDecomposedInsts())) {
				if(!((derivedDataDS *) request->dataForDerivedClasses)->isDisplayedCorrectlyIn(this)) {
					DBG_NOINDENT("request " << request->identify() << " not displayed correctly in "
						<< get_key() << endl);
					// debug-problems
					// cerr << "request " << request->identify() << " not displayed correctly in "
					// 	<< get_key() << endl;
					if(!do_something) {
						reason << ": " << request->get_unique_id() << " not displayed correctly; "; }
					do_something = true;
					((derivedDataDS *) request->dataForDerivedClasses)->update_ds_line_coordinates(); }
				else {
					DBG_NOINDENT("request " << request->identify() << " OK\n"); } }
			while((request = (ActivityInstance*) theAbstractedInsts())) {
				if(((derivedDataDS *) request->dataForDerivedClasses)->isDisplayedCorrectlyIn(this)) {
					DBG_NOINDENT("request " << request->identify() << " not displayed correctly in "
						<< get_key() << endl);
					// debug-problems
#ifdef DEBUG_PROBLEMS
#endif /* DEBUG_PROBLEMS */
					// cerr << "request " << request->identify() << " not displayed correctly in "
					// 	<< get_key() << endl;
					if(!do_something) {
						reason << ": " << request->get_unique_id() << " not displayed correctly; "; }
					do_something = true;
					((derivedDataDS *) request->dataForDerivedClasses)->update_ds_line_coordinates(); }
				else {
					DBG_NOINDENT("request " << request->identify() << " OK\n"); } }
			while((request = (ActivityInstance*) thePendingInsts())) {
				if(!((derivedDataDS *) request->dataForDerivedClasses)->isDisplayedCorrectlyIn(this)) {
					DBG_NOINDENT("request " << request->identify() << " not displayed correctly in "
						<< get_key() << endl);
					if(!do_something) {
						reason << ": " << request->get_unique_id() << " not displayed correctly; "; }
					do_something = true;
					((derivedDataDS *) request->dataForDerivedClasses)->update_ds_line_coordinates(); }
				else {
					DBG_NOINDENT("request " << request->identify() << " OK\n"); } }
			if(do_something) {
				DBG_NOINDENT("Found requests not properly displayed; activating delayed workproc.\n");
#ifdef DEBUG_PROBLEMS
				cerr << "Found requests not properly displayed; activating delayed workproc.\n";
#endif /* DEBUG_PROBLEMS */
				// need to pass a list of problems - activating does nothing now!
				return new synchro::problem(
					reason,
					this,
					ACT_sys::checkForActivityListProblem,
					this,
					ACT_sys::fixActivityCount); } }
	return NULL; }

synchro::problem*	ACT_sys::checkForActivityListProblem(synchro::detector*	D) {
	ACT_sys*	act_sis = dynamic_cast<ACT_sys*>(D);
	return act_sis->detectActivityListProblem(); }

	/*
	 * New protocol (11/12/2001): anybody who asks this question (the client) provides a List in which
	 * servers can insert a tag with their ID as a key and a list of delegates inside it.
	 */

class special_time_node: public Time_node {
public:
	CTime_base event_time;

	special_time_node(const CTime_base &t) : event_time(t) {}
	special_time_node(const special_time_node &cn) : event_time(cn.event_time) {}
	~special_time_node() {}

	Cstring			id;
	virtual void		setetime(
					const CTime_base& new_time) {
		event_time = new_time;
	}
	virtual const CTime_base& getetime() const {
		return event_time;
	}

	Node*			copy() {
		return new special_time_node(*this);
	}
	Node_type		node_type() const {
		return CONCRETE_TIME_NODE;
	}
	const Cstring&		get_key() const {
		return id;
	}
};

synchro::problem*	ACT_sys::detectSyncProblem() {
    if(GUI_Flag) {
	List_iterator	theLegsList(Dsource::theLegends());
	Lego		*aLegend;
	int		violation_count = Constraint::violations().get_length()
				- Constraint::release_count();
	static char	theViolationCountAsAString[89];

	if(isVisible()) {

		if(VertLineManager.isOutOfSync()) {
			Cstring reason(get_key());
			reason << ": VertLineManager is out of sync";
			return new synchro::problem(
					reason,
					this,
					DS_graph::checkForSyncProblem,
					NULL,
					NULL); }

		CTime_base			reference_time(Siblings->dwtimestart());
		special_time_node		reference_node(reference_time);
		ActivityInstance*		theEarliestVisibleActivity = NULL;
		int				which_stage = 0;
		List_iterator			the_siblings(*Siblings);
		static ActivityInstance*	theLatestVisibleActivity;
		static time_actptr*		pointerToAVisibleActivity;
		static DS_line			*theDSline;
		static sibling			*aSibling;
		static DS_graph			*res_or_con_sis;
		synchro::problem*		a_problem;

		// common sense: count activities
		
		if(lnobjects.get_length() != eval_intfc::get_act_lists().get_active_length()) {
			Cstring reason(get_key());
			reason << ": lines: " << lnobjects.get_length() << "; acts: "
				<< eval_intfc::get_act_lists().get_active_length();
			return new synchro::problem(reason,
					this,
					DS_graph::checkForSyncProblem,
					NULL,
					NULL); }

		/* 1. consult Siblings. The _only_ thing that list_of_siblings::isOutOfSync checks is
		 *    the a_client_has_requested_a_time_change flag, which is set by
		 *    list_of_siblings::execute_client_request(), which is invoked by the first
		 *    loop in theWorkProc.
		 *
		 *    In particular, list_of_siblings::isOutOfSync() does _not_ invoke the isOutOfSync()
		 *    methods of the siblings objects it owns. */
		if(Siblings->isOutOfSync(NULL)) {
			Cstring reason(get_key());
			DBG_NOINDENT(get_key() << "->isOutOfSync(): Siblings is out of sync, so: YES\n");
			reason = ": Out-of-sync Sibling - supposedly, a client has requested a time change";
			// debug-problems
			return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL); }

		/* 2. consult the timemark. It checks whether its local values of start, span agree with the
		 *    official values in the list_of_siblings object. Then it checks its local value of
		 *    the time zone against that held in the activity_display object (parent of act sys). */
		if((a_problem = Siblings->timemark->detectSyncProblem())) {
			DBG_NOINDENT(get_key() << "->isOutOfSync(): timemark is out of sync, so: YES\n");
			// debug-problems
			Cstring reason(get_key());
			reason << ": timemark is out of sync";
			return new synchro::problem(reason, this, DS_graph::checkForSyncProblem,
				NULL, NULL, a_problem); }

		/* 3. Explicitly consult RD_sys and CON_sys sibling(s). */
		while((aSibling = (sibling *) the_siblings())) {
			if((res_or_con_sis = aSibling->getTheGraph())->is_a_res_sys()) {
				RD_sys*	res_sis = (RD_sys *) res_or_con_sis;

				if((a_problem = res_or_con_sis->detectSyncProblem())) {
					DBG_NOINDENT(get_key() << "->isOutOfSync(): (res/con)_sys "
						<< res_or_con_sis->get_key()
						<< " is out of sync, so: YES\n");
					Cstring reason(get_key());
					reason << ": RD_sys sibling " << res_or_con_sis->get_key()
						<< " complains.";
					return new synchro::problem(
						reason,
						this, DS_graph::checkForSyncProblem,
						NULL, NULL, a_problem); }
				if(res_sis->the_vertical_scrollbar_is_out_of_sync(NULL)) {
					Cstring reason(get_key());
					reason << ": RD_sys " << res_sis->get_key() << " has an out-of-sync scrollbar";
					DBG_NOINDENT(get_key() << "->isOutOfSync(): res_sys "
						<< res_or_con_sis->get_key()
						<< " has out of sync vertical scrollbar, so: YES\n");
					return new synchro::problem(
						reason,
						this, DS_graph::checkForSyncProblem,
						NULL, NULL); } }
			else if(res_or_con_sis->is_a_con_sys()) {
				if((a_problem = res_or_con_sis->detectSyncProblem())) {
					DBG_NOINDENT(get_key() << "->isOutOfSync(): (res/con)_sys "
						<< res_or_con_sis->get_key() << " is out of sync, so: YES\n");
					Cstring reason(get_key());
					reason << ": CON_sys sibling, " << res_or_con_sis->get_key()
						<< " complains.";
					return new synchro::problem(
						reason,
						this, DS_graph::checkForSyncProblem,
						NULL, NULL, a_problem); } } }
		/* 4. Check if any activities are yet to be displayed, and whether any activities
		 * 	have created pointers to DS_lines ready to be used. ActivitiesNotYetDisplayed
		 * 	is populated by unconfigure_graphics(), invoked when an activity_display is
		 * 	removed from the main window, and by instantiate_graphics_for_act_sys(),
		 * 	which is invoked by theWorkProc, by ACT_sys::configure_graphics, by
		 * 	Dsource::instantiate(), and by Dsource::switch_to_legend(). */
		if(thePointersToTheNewDsLines.get_length()) {
			Cstring reason;
			reason << get_key() << ": " << Cstring(thePointersToTheNewDsLines.get_length())
				<< " pointers to new ds_lines found";
			DBG_NOINDENT(get_key() << "->isOutOfSync(): there are activities/lines not yet displayed, so: YES\n");
			return new synchro::problem(
				reason,
				this, DS_graph::checkForSyncProblem,
				NULL, NULL); }

		/* 5. Query the Legos' list of windows to redraw to see if they have anything
		 *	appropriate for this ACT_sys. */
		while((aLegend = (Lego *) theLegsList())) {
			if(aLegend->there_are_windows_to_redraw_for(legend_display)) {
				Cstring reason(get_key());
				reason << ": Legend " << aLegend->get_key()
					<< " points to a list of siblings with windows to redraw.";
				DBG_NOINDENT(get_key() << "->isOutOfSync(): there are Windows to be redrawn, so: YES\n");
				return new synchro::problem(
					reason,
					this, DS_graph::checkForSyncProblem,
					NULL, NULL); } }

		/* 6. Query the legend_display to see if it is out of sync. This function is implemented in the
		 * 	base class method, DS_scroll_graph::isOutOfSync(), which specializes in querying any
		 * 	vertically scrolled objects it holds. */
		if((a_problem = legend_display->get_this()->detectSyncProblem())) {
			Cstring reason(get_key());
			DBG_NOINDENT(get_key() << "->isOutOfSync(): legends out of sync, so: YES\n");
			reason << ": " << ": legend display " << legend_display->get_this()->get_key() << " is out of sync";
			return new synchro::problem(
				reason,
				this, DS_graph::checkForSyncProblem,
				NULL, NULL, a_problem); }

		/* 7. Check the scrollbars. NOTE: we depend on the timemark for various things,
		 *	therefore in theWorkProc we should really do the timemark first, THEN the scrollbar. */
		if(the_horizontal_scrollbar_is_out_of_sync(NULL)) {
			Cstring reason(get_key());
			DBG_NOINDENT(get_key() << "->isOutOfSync(): the scrollbar is out of sync, so: YES\n");
			reason << ": horizontal scrollbar is out of sync";
			return new synchro::problem(
				reason,
				this, DS_graph::checkForSyncProblem,
				NULL, NULL); }
		// The implementation of this function is found in the DS_scroll_bar base class: 
		if(the_vertical_scrollbar_is_out_of_sync(NULL)) {
			Cstring reason(get_key());
			DBG_NOINDENT(get_key() << "->isOutOfSync(): the scrollbar is out of sync, so: YES\n");
			reason << ": vertical scrollbar is out of sync";
			return new synchro::problem(
				reason,
				this, DS_graph::checkForSyncProblem,
				NULL, NULL); }

		/* 8. All right, nothing grossly obvious. Now let's see if perhaps our activities need to be scrolled.
		 * 	To be efficient, we don't want to check every single activity in the plan, so we'll only
		 * 	check for stuff that's visible, and even then, we'll only look for a couple of possibilities:
		 * 	the earliest and latest visible activities for this display. If both are displayed correctly,
		 * 	we'll assume nothing's out of sync.  */

		while(which_stage < 4) {
			theEarliestVisibleActivity = NULL;
			pointerToAVisibleActivity = NULL;
			if(which_stage == 0) {
				pointerToAVisibleActivity = 
					find_at_or_after<alpha_time, time_actptr, ActivityInstance*>(
						ACT_exec::ACT_subsystem().ActivityEnds,
						reference_time); }
			else if(which_stage == 1) {
				reference_time = Siblings->dwtimestart() + Siblings->dwtimespan();
				pointerToAVisibleActivity = 
					find_at_or_before<alpha_time, time_actptr, ActivityInstance*>(
						ACT_exec::ACT_subsystem().ActivityEnds,
						reference_time); }
			else if(which_stage == 2) {
				reference_node.setetime(Siblings->dwtimestart());
				theEarliestVisibleActivity = (ActivityInstance *)
					eval_intfc::get_act_lists().find_earliest_active_after(&reference_node); }
			else if(which_stage == 3) {
				reference_node.setetime(Siblings->dwtimestart() + Siblings->dwtimespan());
				theEarliestVisibleActivity = (ActivityInstance *)
					eval_intfc::get_act_lists().find_latest_active_before(&reference_node); }
			if(pointerToAVisibleActivity || theEarliestVisibleActivity) {
				static int		sx, ex;

				if(pointerToAVisibleActivity)
					theEarliestVisibleActivity = 
						pointerToAVisibleActivity->BP;
				if(!((derivedDataDS *) theEarliestVisibleActivity->dataForDerivedClasses)
						->isDisplayedCorrectlyIn(this)) {
					Cstring reason;
					reason << get_key() <<
						" -- earliest displayed act. "
						 << theEarliestVisibleActivity->get_unique_id()
						<< " is not displayed correctly";
					DBG_NOINDENT(get_key() << "->isOutOfSync(): act. "
							<< theEarliestVisibleActivity->get_unique_id()
							<< " is not displayed correctly, so: YES\n");
					return new synchro::problem(
						reason,
						this, DS_graph::checkForSyncProblem,
						NULL, NULL); } }
			which_stage++; }

		/* 9. Update violation count if necessary. */
		if(get_AD_parent()->adViolationCountLabel) {
			if(APcloptions::theCmdLineOptions().constraints_active) {
				sprintf(theViolationCountAsAString, "%d", violation_count);
				if(get_AD_parent()->adViolationCountLabel->get_text() != theViolationCountAsAString) {
					Cstring reason(get_key());
					reason << ": adViolationCountLabel "
						<< get_AD_parent()->adViolationCountLabel->get_text()
						<< " disagrees with theViolationCountAsAString "
						<< theViolationCountAsAString;
					return new synchro::problem(
						reason,
						this, DS_graph::checkForSyncProblem,
						NULL, NULL); } }
			else {
				if(get_AD_parent()->adViolationCountLabel->get_text() != "0") {
					Cstring reason(get_key());
					reason << ": constraints are not active and ad violation label is not 0";
					return new synchro::problem(
						reason,
						this, DS_graph::checkForSyncProblem,
						NULL, NULL); } } } }
		return NULL; }
    else { // GUI_Flag == 0
	return NULL; } }

actsis_iterator::actsis_iterator() {
	ptr = (Pointer_node *) ACT_sys::Registration().first_node();
	theTag = NULL; }

actsis_iterator::~actsis_iterator() {}

ACT_sys	*actsis_iterator::operator()() {
	ACT_sys		*actsis;

	if(ptr) {
		actsis = (ACT_sys *) ptr->get_ptr();
		ptr = (Pointer_node *) ptr->next_node();
		if(!ptr) {
			theTag = (btag *) ACT_sys::Hoppers().first_node(); } }
	else if(theTag) {
		actsis = (ACT_sys *) theTag->get_pointer();
		theTag = (btag *) theTag->next_node(); }
	else {
		ptr = (Pointer_node *) ACT_sys::Registration().first_node();
		return NULL; }
	return actsis; }

// Actually, this is a combination of detector and fixer
void DS_draw_graph::do_the_horizontal_scrollbar_thing(infoList* who) {
	static int		actual_value, x_max, x_min, page_inc, x_slider_size, value;
	static CTime		theMinTime, theMaxTime;
	infoList*		theDelegates = NULL;
	infoNode*		tl = NULL;
	synchro::problem*	a_problem;

	if(!get_Hscrollbar()) {
		DBG_NOINDENT(get_key() << "->do_the_horizontal_scrollbar_thing: no scrollbar! Quitting.\n");
		return; }
	if(refresh_info::ON) {
		Cstring	name(get_key());
		name << "::do_the_horizontal_scrollbar_thing() (server/agent)";
		(*who) << (tl = new infoNode(name));
		theDelegates = &tl->payload.theListOfServers; }

	// This function assumes that the timemark is correctly configured. Don't even try if not:
	// Note: the danger with this is that the work proc will never get around to fixing the TIMEMARK.
	//	 come to think of it, that's not true: the work proc truly tries to do as much as possible.
	if((a_problem = Siblings->timemark->detectSyncProblem())) {
		delete a_problem;
		return; }
	// Next: if the value just changed, TELL THE SIBLINGS!! Don't do anything else yourself...
	XtVaGetValues(get_Hscrollbar()->widget, XmNvalue, &actual_value, NULL);
	if(actual_value != current_horizontal_offset) {
		actsis_iterator		actsyslst;
		ACT_sys			*actsis;
		int			my_index = 0;
		CTime			theNewStart(Siblings->dwtimestart() + Siblings->timemark->get_time_unit()
							* (actual_value - current_horizontal_offset));
		CTime			theNewEnd;
		NEW_HORIZONrequest	*req;
		Pointer_node		*ptr;

		DBG_NOINDENT("do_the_horizontal_scrollbar_thing: current_horizontal_offset = " << current_horizontal_offset
			<< ", value = " << actual_value << "; setting Siblings straight...\n");
		if(who) {
			tl->payload.reason << " current_horizontal_offset = " << Cstring(current_horizontal_offset)
				<< ", value = " << Cstring(actual_value) << "; setting Siblings straight through NEW_HORIZON request. "; }

		while((actsis = actsyslst())) {
			my_index++;
			if(actsis == this) {
				break; } }
		theNewEnd = theNewStart + Siblings->dwtimespan();
		req = new NEW_HORIZONrequest(Cstring("A") + Cstring(my_index), theNewStart, theNewEnd);
		current_horizontal_offset = actual_value;
		req->process();
		return; }
	// Don't change anything while the user is grabbing:
	if(aButtonIsPressed())  {
		if(who) {
			delete tl; }
		return; }
	evaluate_horizontal_scrollbar_parameters(x_min, x_max, value, x_slider_size, page_inc, theMinTime, theMaxTime);

	theTimeAssociatedWithTheMinPosOfTheHorizScrollbar = theMinTime;
	current_horizontal_offset = value;
	if(who) {
		tl->payload.reason << "\nsetting theTimeAssociatedWithTheMinPosOfTheHorizScrollbar to "
			<< theMinTime.to_string() << ".\n";
		tl->payload.reason << "\nsetting scrollbar max to " << Cstring(x_max) << ".\n";
		tl->payload.reason << "\nsetting scrollbar min to " << Cstring(x_min) << ".\n";
		tl->payload.reason << "\nsetting scrollbar value to " << Cstring(value) << ".\n";
		tl->payload.reason << "\nsetting scrollbar slider size to " << Cstring(x_slider_size) << ".\n";
		tl->payload.reason << "\nsetting scrollbar page incr. to " << Cstring(page_inc) << ".\n"; }
	XtVaSetValues(	get_Hscrollbar()->widget,
			XmNmaximum,		x_max,
			XmNminimum,		x_min,
			XmNvalue,		value,
			XmNsliderSize,		x_slider_size,
			XmNpageIncrement, 	page_inc,
			NULL); }

void DS_line_owner::update_ds_line_gc() {
	if(GUI_Flag) {
	List_iterator		lines(theListOfPointersToDsLines);
	static Pointer_node	*ptr;
	static DS_line		*line;
	static TypedValue	color_val, pattern_val;
	static DS_gc		*tilepattern;
	static Node		*N;
	static ACT_sys		*actsis;
	static ActivityInstance* request;

	while((ptr = (Pointer_node *) lines())) {
		line = (DS_line *) ptr->get_ptr();
		// Note: 'this' is usually an ACT_req, but we can't cast 'this'
		// because of underlying C structure offset
		request = dynamic_cast<ActivityInstance*>(line->getsource());
		assert(request);
		pattern_val =  (*request->Object)["pattern"];
		int			color_value;

		actsis = (ACT_sys *) line->graphobj;
		
		color_val =    (*request->Object)["color"];
		color_value = color_val.get_int();

		tilepattern = actsis->gettilemap(pattern_val.get_int(), color_value);

		N = actsis->thePointersToTheNewDsLines.find((void *) line);

		DBG_NOINDENT(request->identify() << "->update_de_line_gc() telling line to adopt gc w/"
			<< " pattern = " << pattern_val.get_int()
			<< ", color = " << color_value << endl);
		if(tilepattern) {
			// tilepattern returned by gettilemap will be NULL if actsis is not visible:
			line->adopt_new_gc(*tilepattern); }
		if(!N) {
			DBG_NOINDENT(request->identify() << "->update_ds_line_gc() ADDING 1 DS POINTER\n");
			actsis->thePointersToTheNewDsLines << new bpointernode(line, line); } } } }

void DS_line_owner::update_ds_line_coordinates() {
	List_iterator		lines(theListOfPointersToDsLines);
	static Pointer_node	*ptr;
	static DS_line		*line;
	static ACT_sys		*actsis;
	static ActivityInstance	*request;
	static int		sx, ex;

	request = (ActivityInstance *) this_sort_of;
	// debug
	// cout << "request " << this_sort_of->identify() << " updating "
	// 	<< theListOfPointersToDsLines.get_length() << " line(s)...\n";

	while((ptr = (Pointer_node *) lines())) {
		double		S1, S2;

		line = (DS_line *) ptr->get_ptr();
		// request = (ActivityInstance *) line->getsource();
		if(refresh_info::ON) {
			/*
			 * Dangerous (exit()) but disabled usually since ON is OFF
			 */
			if((request) != this_sort_of) {
				cerr << "ds line owner has discrepancy: this = "
					<< ((ActivityInstance *) this_sort_of)->get_unique_id()
					<< " vs. line->getsource() = "
					<< request->get_unique_id() << endl;
				exit(-3); } }
		actsis = (ACT_sys *) line->graphobj;
		/*
		 * Was only used while testing:
		 *
		 * stime = request->getetime();
		 * tmspan = request->get_timespan();
		 */
		((derivedDataDS *) request->dataForDerivedClasses)->compute_start_and_end_times_in_pixels(actsis, sx, ex);
		/* OBSERVER PATTERN
		 * DS_line::newhcoor() notifies DS_draw_graph that a window needs updating */
		// debug
		// cout << "setting new x coord to " << sx << endl;
		line->newhcoor(sx, ex); } }

int DS_line_owner::isDisplayedCorrectlyIn(DS_time_scroll_graph *act_or_con_sis) {
    if(GUI_Flag) {
	ActivityInstance*	request = dynamic_cast<ActivityInstance*>(this_sort_of);
	con_violation_gui*	vio = dynamic_cast<con_violation_gui*>(this_sort_of);
	static Pointer_node*	ptr;
	static int		sx, ex;
	List_iterator		lines(theListOfPointersToDsLines);
	DS_line*		line = NULL;

	if(request) {
		ActivityInstance*	act = request;

		// first look for a ds line in this actsis
		while((ptr = (Pointer_node *) lines())) {
			line = (DS_line *) ptr->get_ptr();
			if(line->graphobj == act_or_con_sis) {
				break;
			}
		}
		if(!ptr) {
			// we didn't break
			line = NULL;
		}
		// first check whether the line should/should not be there:
		if(line) {
			if(act->is_on_clipboard()) {
				// will define a window for the actsis to clean up:
				delete line;
				return 0;
			} else if(act->is_unscheduled()) {
				if(act_or_con_sis->is_a_hopper()) {
					; // good
				}
				else {
					// will define a window for the actsis to clean up:
					delete line;
					return 0;
				}
			} else if(!act->is_active()) {
				// will define a window for the actsis to clean up:
				delete line;
				return 0;
			} else {
				//good: active, has line; but...
				if(act_or_con_sis->is_a_hopper()) {
					if(!act->is_unscheduled()) {
						// will define a window for the actsis to clean up:
						delete line;
						return 0;
					}
				}
			}
		} else {
			derivedDataDS *dd = (derivedDataDS *) act->dataForDerivedClasses;
			if(act->is_on_clipboard()) {
				return 1;
			} else if(act->is_unscheduled()) {
				if(act_or_con_sis->is_a_hopper()) {
					dd->instantiate_graphics_for_act_sys((ACT_sys *) act_or_con_sis);
					return 0;
				} else {
					return 1;
				}
			} else if(!act->is_active()) {
				return 1;
			} else {
				dd->instantiate_graphics_for_act_sys((ACT_sys *) act_or_con_sis);
				return 0;
			}
		}
		// We know the line exists and belongs in the actsis. Now see whether it is correctly drawn.
		((derivedDataDS *) request->dataForDerivedClasses)->compute_start_and_end_times_in_pixels(
					act_or_con_sis,
					sx,
					ex);
		if(	sx != line->getstartx()
			|| ex != line->getendx()) {
			if(refresh_info::ON) {
				Cstring		temp("request ");

				temp << request->get_unique_id()
					<< " computes start, end as (" << sx << ", " << ex
					<< ") while line is (" <<
					line->getstartx() << ", " << line->getendx() << ")";
				refreshInfo.add_level_2_trigger(
						act_or_con_sis->get_key(),
						temp);
			}
			return 0;
		} else if(line->is_out_of_sync()) {
			if(refresh_info::ON) {
				refreshInfo.add_level_2_trigger(
						act_or_con_sis->get_key(),
						request->get_unique_id() + " is out of sync"); }
			return 0;
		} else {
			return 1;
		}
	} else if(vio) {
		int sx = (int) (0.5 +
				     (
				      	(
					vio->getetime() - act_or_con_sis->get_siblings()->dwtimestart()
					).convert_to_double_use_with_caution()
					/ act_or_con_sis->timeperhpixel()
				    )
			       );

		while((ptr = (Pointer_node *) lines())) {
			line = (DS_line *) ptr->get_ptr();
			if(line->graphobj == act_or_con_sis) {
				break;
			}
		}
		if(!line) {
			return 0;
		}
		if(sx == line->getstartx()) {
			return 1;
		} else {
			return 0;
		}
	} else {	// invisible line; always correctly displayed!!
		return 1;
	}
    // if GUI_Flag
    } else {
	return 1;
    }
}

void derivedDataDS::handle_instantiation(bool present, bool active_only) {
	if(present) {
		actsis_iterator	act_sisses;
		ACT_sys		*actsis;

		while((actsis = act_sisses())) {
			instantiate_graphics_for_act_sys(actsis);
		}
	} else {
		Pointer_node	*ptrnode, *next_ptrnode;
		DS_line		*dsnode;

		if(active_only) {
			for(	ptrnode = (Pointer_node *) theListOfPointersToDsLines.first_node();
				ptrnode;
				ptrnode = next_ptrnode) {
				next_ptrnode = (Pointer_node *) ptrnode->next_node();
				dsnode = (DS_line *) ptrnode->get_ptr();
			}
		} else {
			for(	ptrnode = (Pointer_node *) theListOfPointersToDsLines.first_node();
				ptrnode;
				ptrnode = next_ptrnode) {
				next_ptrnode = (Pointer_node *) ptrnode->next_node();
				dsnode = (DS_line *) ptrnode->get_ptr();
				delete dsnode;
			}
		}
	}
}

void derivedDataVio::handle_instantiation(bool present, bool active_only) {
	if(!present) {
		Pointer_node	*ptrnode, *next_ptrnode;
		DS_line		*dsnode;

		for(	ptrnode = (Pointer_node *) theListOfPointersToDsLines.first_node();
			ptrnode;
			ptrnode = next_ptrnode) {
			next_ptrnode = (Pointer_node *) ptrnode->next_node();
			dsnode = (DS_line *) ptrnode->get_ptr();
			delete dsnode;
		}
	}
}

void derivedDataDS::handle_selection(bool selected) {
	List_iterator	dsobjs(theListOfPointersToDsLines);
	DS_line		*dnode;
	Pointer_node	*pnode;

	if(GUI_Flag) {
		DBG_NOINDENT("generate_windows(): generating "
			<< theListOfPointersToDsLines.get_length()
			<< " window(s) for "
			<< this_sort_of->identify()
			<< endl);
		while((pnode = (Pointer_node *) dsobjs())) {
			dnode = (DS_line  *) pnode->get_ptr();
			((ACT_sys *) dnode->graphobj)->add_window_for(dnode); } } }

DS_line *DS_line_owner::get_representative_in(DS_time_scroll_graph *d) {
	List_iterator   O(theListOfPointersToDsLines);
	DS_line		*line_source;
	Pointer_node	*ptr;

	if(GUI_Flag) {
		line_source = NULL;
		while((ptr = (Pointer_node *) O())) {
			line_source = (DS_line *) ptr->get_ptr();
			if(line_source->graphobj == (void *) d)
				return line_source; } }
	return NULL; }

void ACT_sys::configure_graphics(callback_stuff *) {
	ActivityInstance*		actnode;
	status_aware_iterator	acts(eval_intfc::get_act_lists().get_scheduled_active_iterator());

	if(!isVisible()) {
		return; }
	never_configured = false;

	DBG_INDENT("ACT_sys(" << get_key() << ")::configure_graphics START\n");

	while((actnode = (ActivityInstance *) acts())) {
		derivedDataDS *dd = (derivedDataDS *) actnode->dataForDerivedClasses;
		DBG_NOINDENT(get_key() << "::configure_graphics(): calling instantiate_graphics_for_act_sys("
				<< actnode->identify() << endl);
		dd->instantiate_graphics_for_act_sys(this); }
	// ActivitiesNotYetDisplayed.clear();

	// re-sync the legends (compute which are visible, etc.)
	update_bits();
	// repaint
	update_graphics((callback_stuff *) 2);
	DBG_UNINDENT("ACT_sys::configure_graphics() DONE\n"); }

	/*
	 * According to this routine, either the 'legends' List has members
	 * and that's where the legends are, or you need to consult both
	 * the "pending_legends" List (which has legend definitions not
	 * yet incarnated) and the list of activity definitions (which has
	 * the 'theoretical legends' to be displayed)... but not the
	 * activity instance lists!?
	 */

int ACT_sys::transferAllLegends(blist &pipe) {
	long		i = 0;

	pipe.clear();
	if(Dsource::theLegends().get_length()) {
		//Get from DISPLAY buffer
		List_iterator	ldlst(Dsource::theLegends());
		Lego	*legendefnode;
		
		while ((legendefnode = (Lego *) ldlst()))
			// Hopper design: should qualify the legend? No, this is a static method
			pipe << new btag(legendefnode->get_key(), (void *) i++); }

	return pipe.get_length(); }

void ACT_sys::modify_selected_legends(int delete_or_squish_or_unsquish_or_expand) {
	Lego	*dslegendnode, *next_legend;

	for(	dslegendnode = (Lego *) Dsource::theLegends().first_node();
		dslegendnode;
		dslegendnode = next_legend) {
		// Hopper design DONE: should qualify the legend here
		next_legend = (Lego *) dslegendnode->next_node();
		if(dslegendnode->get_selection()) {
			if(delete_or_squish_or_unsquish_or_expand == 0) {
				if(dslegendnode->ActivityPointers.get_length()) {
					DBG_NOINDENT("...legend \"" << dslegendnode->get_key() << "\" owns " <<
						dslegendnode->ActivityPointers.get_length() <<
						" graphic object(s); will skip instead of deleting...\n");
				} else {
					delete dslegendnode;
				}
			} else if(delete_or_squish_or_unsquish_or_expand == 1) {
				dslegendnode->adjust_height(ACTSQUISHED);
			} else if(delete_or_squish_or_unsquish_or_expand == 2) {
				dslegendnode->adjust_height(ACTVERUNIT);
			} else if(delete_or_squish_or_unsquish_or_expand == 3) {
				dslegendnode->adjust_height(ACTEXPANDED);
			} else if(delete_or_squish_or_unsquish_or_expand == 4) {
				dslegendnode->adjust_height(ACTFLATTENED);
			}
		}
	}

}

//fills list with Pointer_nodes describing all OR just the selected legends:
void ACT_sys::get_all_or_selected_legends(
		Boolean get_all_legends,
		List	&new_list) {
	Lego	*dslegendnode;
	List_iterator	ldlist(Dsource::theLegends());

	while((dslegendnode = (Lego *) ldlist.next())) {
		if (get_all_legends || dslegendnode->get_selection()) {
			new_list << new Pointer_node(dslegendnode, NULL);
		}
	}
}

List	&ACT_sys::get_all_legend_names() {
	static List	l;
	Lego		*dslegendnode;
	List_iterator	ldlist(Dsource::theLegends());

	l.clear();
	while((dslegendnode = (Lego *) ldlist())) {
		l << new String_node(dslegendnode->get_key());
	}
	return l;
}


void	ACT_sys::WriteAllLegendsToStream(
		List		&list_of_plan_files,
		aoString	&Stream) {
	Lego* curlegenddef;

	if(Dsource::theLegends().get_length()) {
		List_iterator all_legends(Dsource::theLegends());

		if(list_of_plan_files.get_length()) {
			while((curlegenddef = (Lego *) all_legends.next())) {
				slist<alpha_void, smart_actptr>::iterator actptrlst(curlegenddef->ActivityPointers);
				smart_actptr*				  pnode;
				Dsource*				  actreqnode;

				while((pnode = actptrlst.next())) {
		       	         	actreqnode = pnode->BP;
					if(list_of_plan_files.find(actreqnode->get_APFplanname())) {
						Stream << "directive \"Legend\" = \"" << curlegenddef->get_key() << "\";\n";
						DBG_NOINDENT("...writing legend to stream...\n");
						break;
					}
				}
			}
		} else {
			while((curlegenddef = (Lego *) all_legends.next())) {
				Stream << "directive \"Legend\" = \"" << curlegenddef->get_key() << "\";\n";
			}
		}
	}
}

void	ACT_sys::WriteLegendLayoutToStream(ostream & Stream, const Cstring &theTag) {
	Lego*			curlegenddef;
	List_iterator 		all_legends(Dsource::theLegends());

	if(!Dsource::theLegends().get_length()) {
		return;
	}

	Stream << "directive \"Activity Legend Layout\" = [\n    \"" << *theTag << "\",\n";
	while((curlegenddef = (Lego *) all_legends())) {
		Stream << "    [ " << addQuotes(curlegenddef->get_key())
			<< ", " << curlegenddef->get_official_height() << " ]";
		if(curlegenddef != (Lego *) Dsource::theLegends().last_node()) {
			Stream << ", ";
		}
		Stream << "\n";
	}
	Stream << "    ];\n";
}

void ACT_sys::reorder_legends_as_per(const Cstring &layoutID, List &thePointers) {
	blist		theWholeList(compare_function(compare_bpointernodes, false));
	List_iterator	ptrs(thePointers);
	Pointer_node	*ptr;
	bpointernode	*bptr;
	List_iterator	legends(Dsource::theLegends());
	Lego	*lo;

	// 1. create pointers to all existing legends (thePointers may be a partial list)
	while((lo = (Lego *) legends())) {
		theWholeList << new bpointernode(lo, NULL); }
	// 2. remove redundant pointers
	while((ptr = (Pointer_node *) ptrs())) {
		bptr = (bpointernode *) theWholeList.find(ptr->get_ptr());
		delete bptr; }
	// 3. append missing pointers
	while((bptr = (bpointernode *) theWholeList.first_node())) {
		thePointers << new Pointer_node(*bptr);
		delete bptr; }
	// 4. remove (but do not destroy) all legends from the master list
	while((ptr = (Pointer_node *) ptrs())) {
		lo = (Lego *) ptr->get_ptr();
		Dsource::theLegends().remove_node(lo); }
	// 4. re-insert all nodes in the order of the new list
	while((ptr = (Pointer_node *) thePointers.first_node())) {
		Dsource::theLegends().insert_node((Lego *) ptr->get_ptr());
		delete ptr;
	}
}

void ACT_sys::update_graphics(callback_stuff *flag) {
	if(GUI_Flag) {
	// Hopper design DONE: check that lnobjects list only contain DS_lines for THIS ACT_sys
	// Else, we need to restrict the loop to activities that belong to THIS and/or check
	// the legends explicitly.
	List_iterator		lnlist(lnobjects);
	DS_line*		cnode;
	ActivityInstance*	request;

	if(!isVisible()) {
		return;
	}

	NumberOfLinesActuallyDrawn = lnobjects.get_length();

	// For some reason, the giant window gets expose events but not ConfigureNotify on first pop-up
	if(never_configured) {
		configure_graphics(NULL); }

	DBG_INDENT("ACT_sys(\"" << get_key() << "\")::update_graphics: flag = " << ((long) flag) << ", START...\n");

	if(UpdateBusyLine)
		// clears the busy lines:
		update_bits();

	if(flag == (callback_stuff *) 1) {
		// Does NOT affect busy bits:
		cleargraph();
		// draw_vertical_cursor(last_cursor_position, 0);
		DBG_NOINDENT("...will draw acts only if pointersToTheVisibleLegends has length > 0: got "
			<< legend_display->pointersToTheVisibleLegends.get_length() << "\n");
		if(legend_display->pointersToTheVisibleLegends.get_length()) {
#			ifdef apDEBUG
			int nodeCount = 0;
#			endif
			// new 2/22/99 PFM
			// Hopper design: check that this is restricted to appropriate legends
			clear_all_legends();
			DBG_NOINDENT("...will draw " << lnobjects.get_length() << " object(s)...\n");
			while((cnode = (DS_line *) lnlist.next())) {
				ActivityInstance		*theA = (ActivityInstance *) cnode->getsource();
				Lego		*theL = (Lego *) theA->get_legend();

				/* Dsource::get_legend_object() is virtual and properly overridden
				 * by ACT_composite. We are therefore ACT_composite-compliant.  */
#				ifdef apDEBUG
				Cstring		theLegO;

				if(theL) {
					theLegO = theL->get_key(); }
				else {
					theLegO = "(NULL)"; }
				DBG_NOINDENT("DS_line # " << (++nodeCount) << ":\n");
				DBG_NOINDENT("   source = " << theA->get_unique_id() << "\n");
				DBG_NOINDENT("   legend = " << theLegO << " @ " << theL << "\n");
#				endif
				if(	cnode->is_visible()
					&& (request = dynamic_cast<ActivityInstance*>(cnode->getsource()))
					&& legend_display->pointersToTheVisibleLegends.find(
						(void *) theL->vthing)) {
					DBG_NOINDENT("line is visible, now calling draw() for act. \""
							<< request->get_unique_id() << "\"\n");
					// will remove pointer in thePointersToTheNewDsLines:
					cnode->draw(); }
				else {
					remove_pointer_to_line(cnode); } } }
			else {
				// nothing we can do about it. Avoid endless refresh.
				thePointersToTheNewDsLines.clear(); } }
	// repaint
	else if(flag == (callback_stuff *) 2) {
		if(refresh_info::ON) {
			refreshInfo.add_level_2_trigger(
				get_key(),
				Cstring("update_graphics: must draw ")
					+ Cstring(lnobjects.get_length()) + " objects"); }
		DBG_NOINDENT("...will draw " << lnobjects.get_length() << " object(s)...\n");
		while((cnode = (DS_line *) lnlist()))
			cnode->draw(); }
	else {
		DBG_NOINDENT("...calling cleargraph()...\n");
		cleargraph();
		// draw_vertical_cursor(last_cursor_position, 0);
		DBG_NOINDENT("...will draw " << lnobjects.get_length() << " object(s)...\n");
		while((cnode = (DS_line *) lnlist()))
			cnode->draw(); }

	// DO THIS BECAUSE THE LEGEND AREA IS A FORM, NOT A DrawingArea, SO IT DOES
	// NOT RESPOND TO RESIZE EVENTS:

	//check vertical lines -- at the end here
	VertLineManager.update_graphics();

	DBG_UNINDENT("ACT_sys::update_graphics: END\n"); } }

VerticalLine::VerticalLine(CTime time, DS_lndraw* draw)
	: Time(time),
	  OutOfSync(true),
	  Draw(draw)
{
}

CTime
VerticalLine::getTime() const
{
	return Time;
}

bool
VerticalLine::isOutOfSync() const
{
	return OutOfSync;
}

void
VerticalLine::setOutOfSync()
{
	OutOfSync = true;
}

void
VerticalLine::update_graphics()
{
	//calculate x from time
	int x = Draw->GetPositionForTime(Time);

	//draw line
	Draw->draw_red_vertical_line(x, 0, Draw->getheight());

	//update OutOfSync
	OutOfSync = false;
}

void
VerticalLine::refreshBlock(int ul_x1, int ul_y1, int lr_x2, int lr_y2)
{
	int x = Draw->GetPositionForTime(Time);
	
	if((x >= ul_x1) &&  (x <= lr_x2))
	{
		//redraw the portion of line affected
		Draw->draw_red_vertical_line(x, ul_y1, lr_y2);
	}
}


VerticalLineManager::VerticalLineManager()
	: OutOfSync(false)
{

}

int
VerticalLineManager::addAVerticalLine(int handle, const CTime& lineTime, DS_lndraw* draw)
{
	VerticalLine newLine(lineTime, draw);

	VerticalLineMap::value_type val(handle, newLine);
	
	VerticalLines.insert(val);

	OutOfSync = true;

	return handle;
}

void
VerticalLineManager::removeAVerticalLine(int lineHandle)
{
	VerticalLineMap::iterator iter = VerticalLines.find(lineHandle);
	VerticalLines.erase(iter);
	OutOfSync = true; }


bool
VerticalLineManager::isOutOfSync() const {
	return OutOfSync; }


void
VerticalLineManager::setOutOfSync() {
	OutOfSync = true; }

void
VerticalLineManager::update_graphics() {
	VerticalLineMap::iterator iter = VerticalLines.begin();
	while(iter != VerticalLines.end()) {
		//if its out of sync, update the graphics
		iter->second.update_graphics();
		iter++; }

	OutOfSync = false;
}

void
VerticalLineManager::refreshBlock(int ul_x1, int ul_y1, int lr_x2, int lr_y2)
{
	VerticalLineMap::iterator iter = VerticalLines.begin();
	while(iter != VerticalLines.end()) {
		//if its out of sync, update the graphics
		iter->second.refreshBlock(ul_x1, ul_y1, lr_x2, lr_y2);
		iter++; } }


int ACT_sys::addAVerticalLine(CTime& time) {
	static int nextHandle = 0;

	actsis_iterator		actsyslst;
	ACT_sys* actsis;
	while((actsis = actsyslst())) {
		actsis->VertLineManager.addAVerticalLine(nextHandle, time, actsis); }

	//update it for the next line
	int handle = nextHandle;
	nextHandle++;
	return handle; }

int
ACT_sys::removeAVerticalLine(int lineHandle) {
	actsis_iterator		actsyslst;
	ACT_sys* actsis;
	while((actsis = actsyslst())) {
		actsis->VertLineManager.removeAVerticalLine(lineHandle); }
	return 0; }


/*      Set up the scroll elements. This method is invoked when the window
        size is changed.

        Called by the following routines:

                DS_lndraw::indirecthmv() (in UI_ds_timeline.C)
                DS_timemark::configure_graphics() (in UI_ds_timeline.C)
                ACT_sys::reconfig() */

// Hopper design: check that the client adds the legend object to the proper ACT_sys...
void	ACT_sys::addALegend(Lego *LO) {
				// In this case we need ALL the act_sisses, even the hopper.
	actsis_iterator		actsyslst;
	ACT_sys			*actsis;
	Pointer_node		*ptr;
	pointer_to_siblings	*PS;
	Lego			*aPreviousLegendWithTheSameName;
	List			activitiesToRelocate;
	ActivityInstance	*aConcernedActivity;

	if((aPreviousLegendWithTheSameName = (Lego *) Dsource::theLegends().find(LO->get_key()))) {
		// smart pointers will hopefully do their magic here:
		delete aPreviousLegendWithTheSameName; }

	Dsource::theLegends() << LO;	// List order is used for vertical ordering; legend_objects are derived from
					// bstringnode, and so the blist is in alphanumeric order.
	DBG_INDENT("ACT_sys::addALegend START\n");
	while((actsis = actsyslst())) {
		DBG_NOINDENT("ACT_sys::addALegend: adding legend \"" << LO->get_key()
			<< "\" to " << actsis->get_key() << ", key = "
			<< ((void *) actsis->Siblings) << ", height = " << LO->preferredHeight << "\n");
		LO->set_vertical_size_in_pixels_in(actsis->legend_display, LO->preferredHeight); }

	DBG_UNINDENT("ACT_sys::addALegend DONE\n");
	for(	ptr = (bpointernode *) activitiesToRelocate.first_node();
		ptr;
		ptr = (bpointernode *) ptr->next_node()) {
		aConcernedActivity = (ActivityInstance *) ptr->get_ptr();
		aConcernedActivity->switch_to_legend(LO); }
	activitiesToRelocate.clear(); }

	/*

	Two methods call update_windows():

		DS_line::~DS_line()
		DS_line::newhcoor()

	*/

void ACT_sys::update_windows(int old_startx, int old_endx, int top_y, int bottom_y, Lego *theLeg) {
	DBG_NOINDENT(get_key() << "->update_windows() calling " << theLeg->get_key() << "->update_windows_in()...\n");
	theLeg->update_windows_in(legend_display, old_startx, old_endx, top_y, bottom_y); }

void vertically_scrolled_item::update_windows_in(VSI_owner *vo, int old_startx, int old_endx, int top_y, int bottom_y) {
	pointer_to_siblings	*PS = (pointer_to_siblings *) theActualHeights.find(vo);
	int_node		ref_start(old_startx);
	clist			&wins = PS->theWindowsToRedraw;
	win_node		*wn = (win_node *) wins.find_latest_before(&ref_start), *wn1, *wn2;
	win_node		*wn0 = wn;

	DBG_INDENT(get_key() << "->update_windows_in() START\n");
#	ifdef apDEBUG
	clist			cl(wins);

	if(!verify_window_list(wins)) {
		cerr << "bad list found at start of ACT_sys::update_windows(" << old_startx << ", " << old_endx << ")\n";
		exit(- 3); }
#	endif
	// Hopper design: the challenge is to make sure that the windows to redraw are correctly computed.

	if(!wn0) {
		wn = (win_node *) wins.first_node();
		wn0 = wn;
		if(!wn) {
			wins << new win_node(old_startx, 's', top_y, bottom_y); }
		else if(wn->_i > old_startx) {
			wins << new win_node(old_startx, 's', top_y, bottom_y); } }
	if(wn0) {
		if(wn0->end_type == 's') {
			while(wn) {		// start
				wn1 = (win_node *) wn->next_node();	// end
				if(wn->_i > old_startx) {
					if(wn->_i <= old_endx)
						delete wn;
					else {
						wins << (wn1 = new win_node(old_endx, 'e', top_y, bottom_y));
						break; } }
				if(wn1->_i < old_endx) {
					wn2 = (win_node *) wn1->next_node(); // start
					delete wn1;
					wn1 = NULL;
					wn = wn2; }
				else
					break; } // wn1 can be used as an endpoint...
			if(! wn1) {
				wins << new win_node(old_endx, 'e', top_y, bottom_y); } }
		else {
			wn1 = (win_node *) wn->next_node();	// start
			wn2 = NULL;
			wins << new win_node(old_startx, 's', top_y, bottom_y);
			while(wn1 && wn1->_i <= old_endx) {		// start
				wn2 = (win_node *) wn1->next_node();	// end
				delete wn1;
				wn1 = NULL;
				if(wn2->_i < old_endx) {
					wn1 = (win_node *) wn2->next_node();
					delete wn2;
					wn2 = NULL; }
				else
					break; } // wn2 can be used as an endpoint...
			if(! wn2) {
				wins << new win_node(old_endx, 'e', top_y, bottom_y); } } }
	else {
		wins << new win_node(old_endx, 'e', top_y, bottom_y); }
#	ifdef apDEBUG
	DBG_NOINDENT("update_windows_in(): done; theWindowsToRedraw now has " << wins.get_length()
		<< " window(s) in it.\n");
	if(!verify_window_list(wins)) {
		cerr << "bad list found at end of ACT_sys::update_windows(" << old_startx << ", " << old_endx << ")\n";
		cerr << "Original list:\n";
		dump_window_list(cl);
		exit(- 3); }
#	endif
	DBG_UNINDENT(get_key() << "->update_windows_in() DONE\n"); }

void ACT_sys::add_window_for(DS_line *theLine) {
	if(!thePointersToTheNewDsLines.find((void *) theLine)) {
		DBG_NOINDENT(get_key() << "->add_window_for(): ADDING 1 DS POINTER\n");
		thePointersToTheNewDsLines << new bpointernode(theLine, theLine);
	}
}

void ACT_sys::remove_pointer_to_line(DS_line *theLine) {
	Node	*N = thePointersToTheNewDsLines.find((void *) theLine);

	DBG_NOINDENT(get_key() << "->remove_pointer_to_line(): deleting 1 pointer\n");
	if(N) delete N; }

void ACT_sys::purge() {
	Pointer_node		*ptr;
	ACT_sys			*act_sis;
	List_iterator		act_sisses(Registration());
	activity_display	*AD;

	// Hopper design: need to add hoppers
	while((ptr = (Pointer_node *) act_sisses())) {
		act_sis = (ACT_sys *) ptr->get_ptr();
		act_sis->timezone.zone = TIMEZONE_UTC;
		act_sis->timezone.epoch = "";
		*act_sis->timezone.theOrigin = CTime(0, 0, false);
		act_sis->timezone.scale = 1.;
		act_sis->get_AD_parent()->adjust_time_system_label("UTC");
		act_sis->never_scrolled = true; }
	Dsource::theLegends().clear(); }

apgen::RETURN_STATUS	ACT_sys::getviewtime(CTime & starttime, CTime & timespan)
    { starttime = Siblings->dwtimestart(); timespan = Siblings->dwtimespan();
    return apgen::RETURN_STATUS::SUCCESS; }

void ACT_sys::manage() {
	DBG_NOINDENT(get_key() << "->manage() called; invoking motif_widget::manage().\n");
	motif_widget::manage();
	// will draw vertical cursor if necessary:
	cleargraph(); }

void ACT_sys::cleargraph() {
	DBG_INDENT("ACT_sys(\"" << XtName(widget) << "\")::cleargraph...\n");
	if(isVisible()) {
		List_iterator		lnlist(lnobjects);
		DS_line			*a_line;

		XClearArea(
			XtDisplay(widget),
			// PIXMAP
			XtWindow(widget),
			0,
			0,
			getwidth(),
			getheight(),
			FALSE);

		// redraws to the pixmap now:
		redraw_the_cute_grey_lines();
		thePointersToTheNewDsLines.clear();
		DBG_NOINDENT(get_key() << "->cleargraph() ADDING " << lnobjects.get_length() << " DS POINTERS\n");
		while((a_line = (DS_line *) lnlist()))
			thePointersToTheNewDsLines << new bpointernode(a_line, a_line);

		// NOTE: if there is nothing in the plan, the hilight GC may not be defined (David G. found out)
		// if(model_intfc::is_in_real_time_mode() && DS_object::gethilite()) {
		// 	XDrawLine(XtDisplay(widget),
		// 		// KEEP WINDOW (Definitely!!)
		// 		XtWindow(widget),
		// 		DS_object::gethilite()->getGC(),
		// 		getwidth() / 2, getheight(), getwidth() / 2, 0);
		// 		}
	}
	if(Siblings->verticalCursorEnabled) {
		/** Erasing the cursor won't work here. We want to (1) physically draw the
		 * cursor line, and (2) set the state of the DS system correctly.
		 *
		 * If the toggle flag is already 0, then we previously erased the cursor
		 * and there is no need to draw it.
		 *
		 * Else, if the toggle flag is set to 1, the easiest thing to do is to
		 * pretend we had just erased the cursor (which normally results in the
		 * toggle flag being zero) and officially drawing the cursor, which will
		 * set the toggle flag properly. */
		if(the_vertical_cursor_is_visible) {
			// simulate erasing the cursor
			the_vertical_cursor_is_visible = 0;
			draw_vertical_cursor(last_cursor_position);
		}
	}
	DBG_UNINDENT("ACT_sys::cleargraph() DONE\n");
}

const Cstring& derivedDataDS::label_to_use_on_display() const {
	static Cstring	nul("");
	static Cstring	temp("");
	ActivityInstance*	act = dynamic_cast<ActivityInstance*>(source);
	assert(act);

	try {
		const TypedValue&	tdv = (*act->Object)["label"];
		temp = tdv.get_string();
		return temp;
	}
	catch(eval_error) {
		cerr << "derivedDataDS::label_to_use_on_display(): failed to get to activity instance\n";
	}

	return nul;
}

void ACT_sys::enter_pastemode() {
	actsis_iterator	actsyslst;
	ACT_sys		*actsysnode;
	  
	while((actsysnode = actsyslst())) {
		if(actsysnode->isVisible()) {
		// KEEP WINDOW
		actsysnode->setcursor(PASTEMODE, XtWindow(actsysnode->widget));
		}
	}
}

void ACT_sys::exit_pastemode() {
	if (displaymode == PASTEMODE) {
		actsis_iterator	actsyslst;
		ACT_sys		*actsysnode;

		while((actsysnode = actsyslst())) {
			if(actsysnode->isVisible()) {
				// KEEP WINDOW
				actsysnode->setcursor(NOPMODE, XtWindow(actsysnode->widget)); } } } }

void ACT_sys::enter_defmode() {
	actsis_iterator	actsyslst;
	ACT_sys		*actsysnode;
	  
	// ACT_subsystem->newreqtypename = "";
	DBG_NOINDENT("ACT_sys::enter_defmode() changing cursor...\n");
	//Inform other ACT_sys's of entering def mode
	while((actsysnode = actsyslst())) {
		if(actsysnode->isVisible()) {
			// KEEP WINDOW
			actsysnode->setcursor(DEFMODE, XtWindow(actsysnode->widget)); } } }


void ACT_sys::exit_defmode() {
	if (displaymode == DEFMODE) {
		actsis_iterator	actsyslst;
		ACT_sys		*actsysnode;
		
		DBG_NOINDENT("    ACT_sys::exit_defmode() changing cursor back...\n");
		//Inform other ACT_sys's of exiting def mode
		while((actsysnode = actsyslst())) {
			if(actsysnode->isVisible()) {
				// KEEP WINDOW
				actsysnode->setcursor(NOPMODE, XtWindow(actsysnode->widget)); } } } }

void DS_draw_graph::vlegendswap(int direction) {
	VSI_owner		*theLegendObject = get_legend_graph();

	// unlikely
	if(!theLegendObject) return;

	DBG_INDENT("DS_draw_graph::vlegendswap: START\n");
	//reorder the legends
	// debug
	// cout << get_key() << "->vlegendswap(" << direction << ") called...\n";
	theLegendObject->get_this()->vswap(direction);

	// Let isOutOfSync() handle the rest.

	DBG_UNINDENT("DS_draw_graph::vlegendswap: END\n"); }


	/* Hopper design: we probably want to restrict the selection to only those that are
	* appropriate for 'this'; for example, don't select hopper legends from a regular
	* ACT_sys. However, we should really make this a non-static function, allowing for
	* selection on an individual ACT_sys.
	*/
void DS_draw_graph::selectalllegends(int on_or_off) {
	List_iterator			legdlst(get_legend_graph()->
						get_blist_of_vertically_scrolled_items());
	vertically_scrolled_item	*dslegdnode;

	if(is_a_res_sys()) {
		RD_sys		*res_sis = (RD_sys *) this;

		while((dslegdnode = (vertically_scrolled_item *) legdlst.next())) {
			SELECT_RES_LEGENDrequest *request = new SELECT_RES_LEGENDrequest(
					Cstring("A") + ("ACT_system" / res_sis->activitySystem->get_key()),
					dslegdnode->get_key(),
					on_or_off);

			request->process(); } }
	else { // is an act sys
		while((dslegdnode = (vertically_scrolled_item *) legdlst.next())) {
			SELECT_ACT_LEGENDrequest *request = new SELECT_ACT_LEGENDrequest(dslegdnode->get_key(), on_or_off);

			request->process(); } }

	// we may need to notify other act sisses in the ACT_sys case.
	// get_legend_graph()->get_this()->update_scrolled_objects(NULL);
	}


synchro::problem* ACT_sys::checkActivityCount(synchro::detector* D) {
	ACT_sys*	act_sis = dynamic_cast<ACT_sys*>(D);

	// static int		activeCount = 0;
	// static int		decomposedCount = 0;
	// static int		abstractedCount = 0;
	if(eval_intfc::something_happened(act_sis->activeCount, act_sis->decomposedCount, act_sis->abstractedCount)) {
		Cstring reason(act_sis->get_key());
		reason << ": eval::something_happened reports change in act counts";
		return new synchro::problem(
			reason,
			act_sis,
			ACT_sys::checkActivityCount,
			NULL,
			NULL); }
	if(act_sis->NumberOfLinesActuallyDrawn != eval_intfc::get_act_lists().get_active_length()) {
		Cstring reason(act_sis->get_key());
		reason << ": NumberOfLinesActuallyDrawn = " << act_sis->NumberOfLinesActuallyDrawn
			<< ", disagrees with eval_intfc::get_act_lists().get_active_length() = "
			<< eval_intfc::get_act_lists().get_active_length();
		return new synchro::problem(
			reason,
			act_sis,
			ACT_sys::checkActivityCount,
			NULL,
			NULL); }
	return NULL; }

void ACT_sys::fixActivityCount(synchro::problem* F) {
	ACT_sys*	act_sis = dynamic_cast<ACT_sys*>(F->payload.the_fixer);

	F->payload.the_fixer = act_sis;
	F->payload.fix_action = ACT_sys::fixActivityCount;

	DBG_INDENT("ACT_sys::fixActivityCount: will call update_graphics w/arg. 1 for act_sis "
		<< act_sis->get_key() << "...\n");

	F->action_taken << "UpdateBusyLine = 1, " << act_sis->fixerName() << "->update_graphics(1), UpdateBusyline = 0, "
		<< act_sis->fixerName() << "->update_graphics(1)";

	UpdateBusyLine = 1;

	act_sis->update_graphics((callback_stuff *) 1);
	UpdateBusyLine = 0;
	act_sis->update_graphics((callback_stuff *) 1);
	DBG_UNINDENT("ACT_sys::fixActivityCount: done\n"); }
