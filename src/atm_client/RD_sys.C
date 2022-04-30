#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define apDEBUG
#include "apDEBUG.H"

#include "action_request.H"
#include "ACT_sys.H"
#include <ActivityInstance.H>
#include "RD_sys.H"
#include "RES_exceptions.H"
#include "UI_activitydisplay.H"
#include "UI_exec.H"
#include "UI_mainwindow.H"
#include "UI_mw_timeline.H"
#include "UI_resourcedisplay.H"
#include <mw_intfc.H>

using namespace std;

blist	&RD_sys::theResourceDisplayRegistration() {
		static blist foo(compare_function(compare_bpointernodes, false));
		return foo; }

blist	&RD_sys::theMWobjectRegistration() {
		static blist bar(compare_function(compare_bstringnodes, false));
		return bar; }

mw_obj	*mw_obj::find_mw_object(const Cstring &resname) {
	mw_obj	*found = (mw_obj *) RD_sys::theMWobjectRegistration().find(resname);

	// debug
	// cerr << "mw_obj::find: looking for " << resname << "... not found\n";
	if(!found) {
		Cstring altname = resname + "[\"min\"]";
		// debug
		// cerr << "mw_obj::find: looking for " << altname;
		found = (mw_obj *) RD_sys::theMWobjectRegistration().find(altname);
		// if(found) {
		// 	cerr << "... found\n"; }
		// else {
		// 	cerr << "... not found\n"; }
		}
	if(!found) {
		Cstring altname = resname + "[\"max\"]";
		// debug
		// cerr << "mw_obj::find: looking for " << altname;
		found = (mw_obj *) RD_sys::theMWobjectRegistration().find(altname);
		// if(found) {
		// 	cerr << "... found\n"; }
		// else {
		// 	cerr << "... not found\n"; }
		}
	return found; }

void	mw_obj::add_external_event(const CTime_base &T, double &D) {
	// cout << "RD_sys: mw_obj adding external event\n";
	obj->add_external_event(T, D); }

void	rsource_display_ext::notify_clients(void *) {
	List_iterator	mwos(RD_sys::theMWobjectRegistration());
	mw_obj		*bp;
	MW_object	*mwo;

	while((bp = (mw_obj *) mwos())) {
		mwo = bp->obj;
		mwo->get_resource_handle()->get_history().change_version();
		// debug
		// cerr << "notify_client " << mwo->get_resource_handle()->get_key() << "\n";
		mwo->setStaleFlag(true);
	}
}

const int	RD_sys::REPORT_NUMERIC_MAXFMT = 24;	//longest numeric fmt for report
							//  (accept any precision loss)
const int	RD_sys::REPORT_STRING_MAXFMT  = 40;	//longest string fmt for report
							//  (truncate + "..." if longer)

tlist<alpha_string, Cnode0<alpha_string, RD_sys::layout_stuff> >	RD_sys::list_of_tagged_layout_lists;

Cstring&				RD_sys::thePendingLayout() {
	static Cstring s;
	return s;
}

// EXTERNS:

				// in UI_popups.C:
extern rd_popup			*_rdPopupDialogShell;
extern UI_rdBackgroundPts	*_rdBackgroundPtsPopupDialogShell;


				// in UI_motif_widget.C
extern resource_data		ResourceData;

				// in main.C:
extern motif_widget*		MW;
extern refresh_info		refreshInfo;

extern UI_exec*			UI_subsystem;

// STATIC GLOBALS:
static int n = 0;
static char buffer[80];

static Arg	args[32];
static Cardinal	ac = 0;



/* Constructor initializes internal data items to null values.
 * Note APGEN color scheme:  left area is grey_color, right is papaya_color
 *
 * Note: the DS_time_scroll_graph constructor is a pass-through for the
 * DS_scroll_graph constructor in UI_ds_timeline.C.
 *
 * The parent arg is defined by the RD_sys constructor is a form defined
 * in resource_display, called _rdAxisForm there. It is named rdAxisFormMW
 * here.
 *
 * Since DS_scroll_graph already has logic for defining a vertical scrollbar,
 * let's use that and simplify the overall structure. We'll delete _rdAxisForm
 * (all incarnations), use resource_display::_resourceDisplayForm as the main
 * form for building our widget hierarchy, and go from there.
 * The 'parent' argument below is now resource_display::_resourceDisplayForm,
 * a motif form that spans the width of the paned window except for the
 * vertical scrollbar and the toggle selection button to the right.
 */
MW_widgetdraw::MW_widgetdraw(
		const Cstring &parent_act_sis,
		motif_widget *parent,
		list_of_siblings *LS)
	// This constructor creates a DrawingArea:
	: DS_time_scroll_graph(
		Cstring("RD_sys") + ("ACT_system" / parent_act_sis),
		parent,
		1, // vertical scrolling desired
		0,   // horizontal scrolling not desired (we are a slave to the ACT_sys sibling)
		LS
		),
	mw_objects(compare_function(compare_bstringnodes, false)) {
	DS_scroll_graph		*legend_as_a_ds_scroll_graph;

	if(GUI_Flag) {
	n = 0;
						// x position should match width of legend, below:
	XtSetArg(args[n], XmNx,		ACTLEGNDWIDTH); n++;  // FIXED width
						// width inspired from ACT_sys:
	XtSetArg(args[n], XmNwidth,		261); n++;  // FIXED width
	XtSetArg(args[n], XmNheight,		100); n++;  // FIXED height
	XtSetArg(args[n], XmNbackground,	ResourceData.papaya_color); n++;
	XtSetValues(widget, args, n);

	// emulate ACT_sys::ACT_sys setting legend_display:
	n = 0;
	XtSetArg(args[n], XmNbackground,ResourceData.grey_color); n++;
	XtSetArg(args[n], XmNnavigationType, XmNONE); n++;
	XtSetArg(args[n], XmNresizePolicy,	XmRESIZE_GROW)	; n ++;
	XtSetArg(args[n], XmNwidth,		ACTLEGNDWIDTH); n++;  // FIXED width
	XtSetArg(args[n], XmNheight,		100); n++;  // FIXED height
	XtSetArg(args[n], XmNx,		0); n++;
	XtSetArg(args[n], XmNy,		0); n++; } // if(GUI_Flag)

	legend_display = new MW_legend(get_key() + "_legend",
				myParent,
				(RD_sys *) this, // Don't like this too much but will do for now
				args, n);
	if(GUI_Flag) {
	legend_as_a_ds_scroll_graph = legend_display->get_this();

	// debug
	Arg	largs[3];
	Dimension	x, theWidth, theHeight;

	XtSetArg(largs[0], XmNx	, &x);
	XtSetArg(largs[1], XmNwidth	, &theWidth);
	XtSetArg(largs[2], XmNheight	, &theHeight);
	XtGetValues(widget, largs, 3);
	DBG_NOINDENT("MW_widgetdraw::MW_widgetdraw: After assigning: x = " << x
		<< ", width = " << theWidth << ", height = " << theHeight << endl);
	DBG_NOINDENT("    legend display width: " << legend_as_a_ds_scroll_graph->getwidth() << endl);

	*myParent ^ *this ^ *myParent ^ endform;
	// leave room for scrollbar:
	*myParent < ACTLEGNDWIDTH < *this < 17 < *myParent < endform;
	DBG_NOINDENT("Attaching ExposeCallback to EXPOSE events for " << get_key() << endl);
	sr_xm_initialize_DisplayStructure(); } }

MW_widgetdraw::~MW_widgetdraw() {
	DBG_INDENT("MW_widgetdraw::~MW_widgetdraw() START.\n");
	DBG_UNINDENT("MW_widgetdraw::~MW_widgetdraw() DONE.\n"); }

/* The widget hierarchy is as follows:
 *
 * 	- main window owns a _paned_window member that is a motif paned window widget
 *
 * 	- resource_display is a motif frame in the _paned_window
 *
 * 	- resource_display owns a _resourceDisplayForm that is a motif form widget
 * 	  with the resource_display frame as its parent
 *
 * 	- resource_display owns a _rdAxisForm member that is a motif form widget
 * 	  with the _resourceDisplayForm as its parent. _rdAxisForm leaves some
 * 	  room on the right for the scrollbar and selection button.
 *
 * 	- Now the last few steps are tricky. RD_sys is based on DS_time_scroll_graph,
 * 	  and therefore "is" a motif_widget, in this case a motif DrawingArea created
 * 	  by the DS_graph constructor (Note: the chain of constructors passes through
 * 	  DS_scroll_graph with 2 int flags for scrollbars, which creates a drawing
 * 	  area widget. The other DS_scroll_graph constructor builds a form.)
 *
 * We are going to get rid of rdCanvasFormMW and replace it by this.
 *
 * Note also that the method is very similar to the ACT_sys constructor. While the latter
 * gets its form widget from an activity_display object, RD_sys gets its form widget
 * from a resource_display object. In both cases, the form spans the entire width of the
 * APGEN display and we need to restrict the width of 'this' to the whole thing minus
 * the width of the legend widget (legend_display, a MW_legend object.) */
RD_sys::RD_sys(ACT_sys *actSys, motif_widget *form)
	  	/* MW_widgetdraw is somewhat analogous to the ACT_sys' DS_lndraw object
		 * but with big differences (organization of legend and canvas areas).
		 * It derives from the DS_time_scroll_graph class.
		 * See the code for its constructor in UI_mw_timeline.C. */
	: MW_widgetdraw(actSys->get_key(), form, actSys->get_siblings()),
	rdOptionsMenuPopup(NULL) {
	DBG_INDENT("RD_sys::RD_sys: START; form = \"" << form->get_key() << "\"...\n");

	activitySystem = actSys;

	// DUMB pointer:
	theResourceDisplayRegistration() << new bpointernode(this, NULL);

	/** Initialize Y-scrollbar, including scroll-mode, parameters, and callbacks
	 * MODIFY THIS. OK (no more Widget argument).
	 *
	 * Scrolled resources: migrated function to MW_object and made it obsolete...
	 * Just disable the call; we scroll differently anyway.  */
	// inityscroll();

	/* Already have motif_widget wrapper (done in parent class constructor); add
	 * RD_sys-specific info windows (thus, NOT in MW_widgetdraw constructor);
	 * if gets large, put into initcallbk() method -- cf. UI_ds_timeline.*.
	 *
	 * NOTE: rdCanvasFormMW is a DrawingArea that makes this redundant;
	 * what we should do instead is to use THIS as the working widget. */
	add_keyboard_and_mouse_handler(popup_at_timeCB,
				BTTN1_DN, NO_MODIFIER, this);
	add_keyboard_and_mouse_handler(popup_rd_optionsCB,
				BTTN3_DN, NO_MODIFIER, this);
	add_keyboard_and_mouse_handler(popup_rd_optionsCB,
				CTRL_BTTN1_DN, NO_MODIFIER, this);
	add_event_handler(	HandleEvent,
				PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask,
				this);
	
	ac = 0;
	XtSetArg(args[ac], XmNleftAttachment, XmATTACH_OPPOSITE_FORM); ac++;
	XtSetArg(args[ac], XmNleftOffset, /* -19 */ -17); ac++;
	XtSetValues(Vscrollbar->widget, args, ac);

	ac = 0;
	XtSetArg(args[ac] , XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(args[ac] , XmNbottomAttachment, XmATTACH_FORM); ac++;
	XtSetArg(args[ac] , XmNleftAttachment, XmATTACH_NONE); ac++;
	XtSetArg(args[ac] , XmNrightAttachment, XmATTACH_FORM); ac++;
	/* Maybe we should keep 19, which I think is the size of the selection button.
	 * Actually: size of button is dictated by its XmNindicatorSize arg, set to 15
	 * (see above.) */

	// NO OFFSET here - there is no toggle button anymore.
	// XtSetArg(args[ac] , XmNtopOffset, 19); ac++;
	// XtSetArg(args[ac] , XmNtopOffset, 17); ac++;
	
	// XtSetArg(args[ac] , XmNbottomOffset, 2); ac++;
	// XtSetArg(args[ac] , XmNrightOffset, 2); ac++;
	XtSetValues(Vscrollbar->widget, args, ac);
	DBG_UNINDENT("RD_sys::RD_sys: DONE\n"); }

RD_sys::~RD_sys() {
	bpointernode		*toThis = (bpointernode *) theResourceDisplayRegistration().find((void *) this);

	if(toThis) {
		delete toThis; } }

void MW_widgetdraw::HandleEvent(Widget, callback_stuff * client_data, void * call_data) {
	RD_sys			*res_sis = (RD_sys *) client_data->data;
	XEvent			*E = (XEvent *) call_data;

	if(E->type == MotionNotify) {
	    if(res_sis->TrackingTheLegendBoundary) {
		int	U = E->xmotion.y;
		int	M = res_sis->minimum_height_limit, L = res_sis->last_sizing_pos;

		if(U < M ) {
			U = M; }
		if(U != L) {

			if(res_sis->Siblings->verticalCursorEnabled) {
				// we really want to ERASE the cursor:
				if(res_sis->the_vertical_cursor_is_visible)
					res_sis->draw_vertical_cursor(res_sis->last_cursor_position);
				res_sis->last_cursor_position = -1; }

			XSetFunction(Defdisplay, DS_object::separation_gc->getGC(), GXinvert);
			// PIXMAP
			XDrawLine(Defdisplay, XtWindow(res_sis->widget),
				DS_object::get_separation_gc()->getGC(), 0, L, res_sis->getwidth(), L);
			L = (res_sis->last_sizing_pos = U);
			// PIXMAP
			XDrawLine(Defdisplay, XtWindow(res_sis->widget),
				DS_object::separation_gc->getGC(), 0, U, res_sis->getwidth(), U);
			// COPY PIXMAP TO WINDOW (0, use_this_height, res_sis->getwidth(), use_this_height)
			XSetFunction(Defdisplay, DS_object::separation_gc->getGC(), GXcopy); } }
	    else {
		res_sis->activitySystem->update_pointer_time(E->xmotion.x);
		if(res_sis->Siblings->verticalCursorEnabled) {
			res_sis->Siblings->draw_vertical_cursor(E->xmotion.x); } } }
	else if(E->type == ButtonRelease) {
		res_sis->ButtonReleased((XButtonEvent *) E);
		res_sis->TrackingTheLegendBoundary = 0;
		if(displaymode != NOPMODE) {
			setcursor(NOPMODE, E->xany.window); } }
	}

void RD_sys::remodelingHasOccurred() {
	List_iterator		res_sisses(theResourceDisplayRegistration());
	RD_sys			*res_sis;
	bpointernode		*bptr;

	DBG_NOINDENT("RD_sys::remodelingHasOccurred(): setting "
		<< theResourceDisplayRegistration().get_length()
		<< " RD_sys stale flags.\n");
	while((bptr = (bpointernode *) res_sisses())) {
		res_sis = (RD_sys *) bptr->get_ptr();

		res_sis->legend_display->get_this()->setAllStaleFlags(true);
	}
}

void RD_sys::record_time_and_duration_parameters() {
	myStartTime = Siblings->dwtimestart();
	myDuration = Siblings->dwtimespan();
}

//Return percent of RD width (0% at left;100% at right), based on time(NOTE all
//  info is at DS_time_scroll_graph level, but NEED for % is at RD_sys level).
double RD_sys::time_to_percent(const CTime &atime) {
	return 100.0 * ((atime - Siblings->dwtimestart()) / Siblings->dwtimespan());
}

void MW_object::setyscrollmode(RD_YSCROLL_MODE mode) {  //set current Y-scroll mode

#ifdef   apDEBUG
	COUT << "MW_object::setyscrollmode:  old yscrollmode=" << yscrollmode
	 << "  new yscrollmode=" << mode << endl;
#endif /*apDEBUG*/

	//If mode does not change, this is a no-op:
	if(yscrollmode == mode)
		return;

	//Reset the saved Y-scroll mode first thing:
	yscrollmode = mode;

	//Handle affected button sensitivities:
	UI_mainwindow::mainWindowObject->countManualSelectedRDs();

	//Do adjustments needed to go from old to new mode.  Only one _MANUAL
	//  mode, so 3 transition types are:  between different _AUTO_modes;
	//  from _MANUAL to an _AUTO_ mode; and from an _AUTO_ mode to _MANUAL.

	//First, do transitions of the first 2 types (the logic is the same):
	if(	(	yscrollmode != YSCROLL_MANUAL
			&& mode != YSCROLL_MANUAL
		)
	  	||	yscrollmode == YSCROLL_MANUAL
	 ) {
		//Do update Y-scale for each actual plot object (mw_object).  When
		//  changing between automatic modes, or from the manual mode to an
		//  automatic one, the view window ALMOST ALWAYS changes, since
		//  "automatic" presumes adjusting the "natural" bounds (with pad added
		//  to top and bottom, for visibility) .  Zero objects is OK.
		adjust_yaxis(); }
	else { // (mode == YSCROLL_MANUAL)	//3rd transition type
		//When changing from an automatic mode to the manual one, there is
		//  only the possibility of expanding from the automatic range 
		//  associated with the particular automatic mode, to the one (the
		//  broadest, i.e. the "entire range") associated with the manual mode.
		//  The view window itself does not change, nor do the actual plot
		//  objects!  (ACTUALLY, this is only true for the FIRST plot object,
		//  which the Y-scrollbar keys off.  Other plot objects MAY need
		//  adjustment to conform with the shared Y-scrollbar!)
		; }

	// Should be done at the higher level:
	theResSysParent->update_graphics(NULL);
	}

void	RD_sys::WriteLegendLayoutToStream(ostream &fout, const Cstring &theTag) {
	MW_object		*mo;
	List_iterator		all_legends(mw_objects);
	extern Cstring		addQuotes (const Cstring& s);

	if(!mw_objects.get_length()) {
		return; }
	fout << "directive \"Resource Legend Layout\" = [\n    \"" << *theTag << "\",\n";

	while((mo = (MW_object *) all_legends())) {
		fout << "    [ " << addQuotes(mo->get_key()) << ", " << mo->get_official_height() << " ]";
		if(mo != (MW_object *) mw_objects.last_node()) {
			fout << ", "; }
		fout << "\n"; }

	fout << "    ];\n";
	}

void RD_sys::reorder_legends_as_per(
		const Cstring&		layoutID,
		const vector<Cstring>&	res_names,
		const vector<int>&	heights,
		const Cstring&		source_file,
		int			lineno
		) {
	// 1. find out if there is an active RD_sys in a selected display
	int		ad_index = UI_mainwindow::findFirstSelectedAD();
	int		rd_index = -1;
	Cstring		id(layoutID);

	if(ad_index < 0) { // no AD selected
		// use the first one that's visible
		ad_index = UI_mainwindow::findFirstVisibleAD();
	}
	if(ad_index >= 0) { // found AD
		if(UI_mainwindow::activityDisplay[ad_index]->resourceDisplay->is_managed()) {
			rd_index = 0;
		}
	}
	layout_stuff LS;
	LS.res_names = res_names;
	LS.heights = heights;
	LS.source_file = source_file;
	LS.lineno = lineno;
	if(rd_index == 0) { // found RD
		RD_sys*	res_sis = UI_mainwindow::activityDisplay[ad_index]->resourceDisplay->resourceSystem;

		return res_sis->implement_layout_as_per(LS);
	} else {
		// store in static list for later use
		Cnode0<alpha_string, layout_stuff>* pointerToListOfLegendInfo;

		if(!id.length()) {
			id = "layout";
		}
		while((pointerToListOfLegendInfo = RD_sys::list_of_tagged_layout_lists.find(id))) {

			//
			// Modify id until no longer found
			//
			int	k = id.length();

			while(isdigit(id[k - 1])) {
				if(!--k) {
					break;
				}
			}
			if(k == id.length()) { // no digit
				id = id + "_1";
			} else {
				int	n = atoi((*id) + k);
				if(k > 0) {
					Cstring	prefix(id(0, k));
					id = prefix + "_" + (n + 1);
				}
			}
		}

		//
		// at this point we have an id that is unique. We store a copy of the list for
		// future use.
		//
		list_of_tagged_layout_lists << new Cnode0<alpha_string, layout_stuff>(id, LS);
		thePendingLayout() = id;
	}
}

void RD_sys::implement_layout_as_per(const RD_sys::layout_stuff& LS) {
	MW_object*	mo;

	//
	// 1. remove all existing resources
	//
	while((mo = (MW_object *) mw_objects.first_node())) {
		delete mo;
	}

	//
	// 2. insert all resources in the order of the new list
	//
	for(int i = 0; i < LS.res_names.size(); i++) {
		Cstring errors;

		//
		// Need to create new resource object here. The resource may not exist...
		//
		if(addResource(LS.res_names[i], errors) != apgen::RETURN_STATUS::SUCCESS) {
			Cstring errs;
			errs << "File " << LS.source_file << ", line " << LS.lineno
				<< ":\n" << errors;
			throw(eval_error(errs));
		}
	}
	List_iterator	theDisplays(mw_objects);

	for(int i = 0; i < LS.heights.size(); i++) {
		mo = (MW_object *) theDisplays();
		mo->adjust_height(LS.heights[i]);
	}
}


RD_YSCROLL_MODE MW_object::getyscrollmode() const {	//get current Y-scroll mode
	return yscrollmode;
}

synchro::problem* RD_sys::detectSyncProblem() {
	if(!isVisible()) {
		// debug
		// cout << get_key() << "->isOutOfSync: not visible -- NO.\n";
		return 0; }
	synchro::problem*	a_problem;

	if(myStartTime != Siblings->dwtimestart() || myDuration != Siblings->dwtimespan()) {

		/* Performance fix: MW_objects no longer call get_values() every time they
		 * are tickled. However, as of now the history values stored in the MW_object
		 * only cover the current window (we should change this). So we have to make
		 * sure that get_values will get called when configuring_graphics.  */
		List_iterator	mos(mw_objects);
		MW_object*	mo;

		while((mo = (MW_object *) mos())) {
			mo->my_res_version = -1;
		}

		// debug
		// cerr << get_key() << "->isOutOfSync: time params out of sync -- YES.\n";
		return new synchro::problem("time params out of sync", this, DS_graph::checkForSyncProblem, NULL, NULL);
	}
	if((a_problem = legend_display->get_this()->detectSyncProblem())) {
		// debug
		// cerr << get_key() << "->isOutOfSync: legend_display out of sync -- YES.\n";
		Cstring reason;
		reason << get_key() << " has a legend_display out of sync";
		return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL, a_problem);
	}
	if(legend_display->get_this()->is_stale()) {
		// debug
		// cerr << get_key() << "->isOutOfSync: legend_display stale -- YES.\n";
		Cstring reason;
		reason << get_key() << " has a stale legend_display";
		return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL);
	}
	// debug
	// cerr << get_key() << "->isOutOfSync: nothing found -- NO.\n";
	return NULL;
}

/* The following method should be migrated to the appropriate MW_object.
 * Remember that unlike the ACT_sys case with LegendObjects, each RD_sys
 * has its own list of MW_objects, so we always zoom on a specific resource
 * display.
 *
 * Absolute pan/zoom for any/all Resources with continuous numeric values
 * (allows complete control -- i.e. superset of pan==scroll and zoom).
 *
 *  Migrated to MW_object PFM 12/06/02. Hey, it's already there!! */
// apgen::RETURN_STATUS MW_object::yabspanzoom (double min, double span)

/* Get minimum and span for FIRST Resource, IF is has continuous numeric values
 *  Logic works for Resource System with 0, 1, or more Resources displayed.
 *
 *  Migrated to MW_object PFM 12/06/02. */
apgen::RETURN_STATUS MW_object::ygetminspan(Cstring &min, Cstring &span) {
	bool		have_increments;
	double		major, minor, dmin, dspan;
	int		mwn;

	//First, check for continuous numeric values (use presence/absence of minor
	//  increment on Y-axis as PROXY for continuous/discrete).  Exit if not so.
	have_increments = getyincrements(major, minor);
	if(minor <= 0.0 /* Epsilon better, but exact zero should stay as such */) {
		min  = "0.0";
		span = "0.0";
		return apgen::RETURN_STATUS::FAIL;
	}

	//Note that getyminspan() sets min and span to zero if no success.
	if(	getyminspan(dmin, dspan)
		&& format_yvalue(dmin, RD_sys::REPORT_NUMERIC_MAXFMT, min) != MW_object::UNFORMATTED
		&& format_ydelta(dspan, RD_sys::REPORT_NUMERIC_MAXFMT, span) != MW_object::UNFORMATTED) {
		return apgen::RETURN_STATUS::SUCCESS;
	}

	return apgen::RETURN_STATUS::FAIL;
}

/* Adjust Y-axis for SINGLE Resource (created ONLY as place for common code).
 * Cannot be a method of MW_object and its derived classes, since yscrollmode
 * is a property of a RD_sys,and NOT of the 0+ MW_objects associated with it.
 *
 * Migrated to MW_object PFM 12/06/02. */

// void RD_sys::adjust_yaxis(MW_object * mwobjectnode)
void MW_object::adjust_yaxis() {
	double		ymin, yspan;

	//Adjust Y-axis based on Y-scrollmode and appropriate stored value range
	switch (yscrollmode) {
	    case YSCROLL_MANUAL:
	        if(getyminspan(ymin, yspan)) {
	            adjust_yaxis_limits(FALSE, ymin, yspan);
	            break;
		}
	        //else fall thru to next-best source of range info:
	    case YSCROLL_AUTO_WINDOW:    //range in current time window
	        if(getyupdate_minspan(ymin, yspan)) {
	            adjust_yaxis_limits(TRUE, ymin, yspan);
	            break;
		}
	        //else fall thru to next-best source of range info:
	    case YSCROLL_AUTO_PLAN:        //range in entire plan
	        if(getymodeled_minspan(ymin, yspan)) {
	            adjust_yaxis_limits(TRUE, ymin, yspan);
	            break;
		}
	        //else fall thru to next-best source of range info:
	    case YSCROLL_AUTO_RANGE:    //full-range-based
	        if(getyrange_minspan(ymin, yspan)) {
	            adjust_yaxis_limits(TRUE, ymin, yspan);
	            break;
		}
	    //else have no range info, so fall thru to default behavior:
	    default:
	       ;    //do nothing to Y-axis!!!
	}
}


// *** Resources ***

apgen::RETURN_STATUS RD_sys::addResource(
		const Cstring&	resourcename,
		Cstring&	errors) {
	MW_object*	mw_object;
	Rsource*	res_handle = NULL;
	apgen::RETURN_STATUS	ret;

	if(mw_objects.find(resourcename)) {
		errors << "Error: resource " << resourcename << " already exists in this display.\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	//Enforce E4 limit of ONE Rsource	/MW_object/RES_bk_timenode triplet:
	// NO! (Scrolled resources).
	// if(mw_objects.get_length() > 0) {
	// 	errors = "RD_sys::addResource: ERROR: mw_objects List is not empty ...\n";
	// 	return apgen::RETURN_STATUS::FAIL; }


	//First, use provided method to get a handle on the Resource (UI checks
	//  name of Resource, so should be no problem just using the argument).
	//  Use another provided method to get handle on Single_variable_func,
	//  which contains the actual data.  Resource type is also needed.
	res_handle = eval_intfc::FindResource(resourcename);
	if(!res_handle) {	//NULL returned if no match to name (UI should prevent)
		errors = "RD_sys::addResource: ERROR: resource_subsystem could not locate \"";
		errors << resourcename << "\" ...\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	//Seventh, assuming all is going OK, add the matched triplet of Resource
	//  reference, background points reference, and MW_object to the
	//  appropriate Lists of this RD_sys.
	mw_object = create_an_mw_object(resourcename, "New", errors, this);
	if(mw_object) {
		mw_obj	*mo;
		mw_objects << mw_object;
		// cout << "RD_sys: adding mw object " << resourcename << " to registration list.\n";
		theMWobjectRegistration() << (mo = new mw_obj(resourcename, mw_object));
		mw_object->theObject = mo;
	} else {
		errors = errors + "\nRD_sys::addResource: ERROR: resource widget factory "
			"did not build a widget.";
		return apgen::RETURN_STATUS::FAIL;
	}

	configure_graphics(NULL);

	return apgen::RETURN_STATUS::SUCCESS;
}

	// We need to emulate what happens when removing a legend.
apgen::RETURN_STATUS RD_sys::removeResource(const Cstring& theResource) {
	MW_object	*theObject = (MW_object *) mw_objects.find(theResource);


	if(theObject) {
		delete theObject;
		// PFM added:
		cleargraph();
	}

	return apgen::RETURN_STATUS::SUCCESS;
}

//Removes ALL (0+) Resources associated with Res_sys, so there's no error mode
void RD_sys::removeAllResources() {
	List_iterator	theChildrenOfTheLegend(legend_display->get_this()->children);
	motif_widget	*W;

	// PFM added:
	cleargraph();

	while((W = (motif_widget *) theChildrenOfTheLegend())) {
		W->destroy_widgets();
	}
	mw_objects.clear();		//makes list empty and deletes nodes
	legend_display->get_this()->children.clear();
}

//Next 2 methods only work with the modeling granularity of this Resource
//  System (i.e. number of background points in the current time window).
//  Granularity applies to ALL Resources plotted, even though they conceptually
//  could have different granularities(and modeling subsystem provides for it).

// Simple interface function:
void RD_sys::setModelingResolution(const Cstring& theResource, int no_points) {
	MW_object* theObject = (MW_object*) mw_objects.find(theResource);

	if(theObject) {
		theObject->setModelingResolution(no_points, Siblings->dwtimespan());
	}
}

void MW_object::setModelingResolution(int number_of_pts, const CTime& theSpan) {
	Rsource* the_resource = get_resource_handle();

	// first term was missing; Linda Lee found out inadvertently...:

	if(the_resource) {
		if(number_of_pts) {

			// debug

			// cerr << "\t" << get_key() << "->setModelingResolution(" << number_of_pts << ")...\n";
			the_resource->derivedResource->get_resolution() = theSpan/number_of_pts;
		} else {

			// debug

			// cerr << "\t" << get_key() << "->setModelingResolution(" << number_of_pts << ")...\n";
			the_resource->derivedResource->get_resolution() = CTime_base(0,0,true);
		}
	}
	/* We need to make sure things get redrawn. Best way: like ACT_sys, delegate this task to
	 * isOutOfSync(). Note that the resource handle has the theoretical information necssary;
	 * isOutOfSync() should compare it against the value last used to update the graphics,
	 * which of course needs to be stored somewhere.
	 *
	 * See if this works: */
	udef_intfc::something_happened() += 1;
	setStaleFlag(true);
}

int RD_sys::getModelingResolution(const Cstring& theResource) {
	MW_object*	theObject = (MW_object *) mw_objects.find(theResource);

	if(theObject) {
		return theObject->getModelingResolution(Siblings->dwtimespan());
	}
	return -1;
}

int MW_object::getModelingResolution(const CTime& theSpan) {
	CTime		res_in_sec;
	double		backgrd_pts;
	Rsource*	the_resource = get_resource_handle();

	if(the_resource) {
		res_in_sec = the_resource->derivedResource->get_resolution();
		//get if have "Resolution" = 0:0:0 in resource def.:
		if(res_in_sec == CTime(0, 0, true)) {
			return 0;
		}
		backgrd_pts = theSpan / res_in_sec;
		if(backgrd_pts < 1.5) {
			return 1; //want non-0 if non-zero res_in_sec
		} else if(backgrd_pts > MAX_BACKGROUND_PTS) {
			return MAX_BACKGROUND_PTS;
		} else { //round to nearest whole number of background pts
			return (int) (backgrd_pts + 0.5);
		}
	}
	return 0;
}

//Click-pops-up-info event-handler callback(registered with motif_wrapper)
void RD_sys::popup_at_timeCB(Widget, callback_stuff* client_data, void* call_data) {
	if(GUI_Flag) {
	int		x_position, y_position, graph_width, nlines;
	CTime		atime;
	Cstring		report;
	RD_sys		*res_sis = (RD_sys*)(client_data->data);
	MW_object	*theObject;
	XButtonEvent	*bevent = (XButtonEvent*)call_data;

	//get width of Canvas area, and get event (click) position
	graph_width = res_sis->getwidth();
	x_position = bevent->x;

#ifdef   DEBUG
	printf(">>> RD_sys::popup_at_timeCB:  x=%d,y=%d,width=%d!\n",
		bevent->x, bevent->y, graph_width);
	printf(">>>                            Pixel x_position in Res Graph=%d\n",
		x_position);
#endif /*DEBUG*/

	//NO ACTION if in axis/label area
	if(x_position < 0)
		return;

	// Emulate behavior of DS_lndraw here...
	if(res_sis->we_are_moving_a_dividing_line(bevent)) {
		return; }

	//compute time from X-position (0-based pixel:  0<=x_position<graph_width)
	//  (NOTE -- this is subject to possible off-by-one-pixel errors, depending
	//  on how MW_object's do their job, AND how AD displays are done)
	//  (???break out as double DS_time_scroll_graph::pixel_to_time(int pixel))
	atime = res_sis->Siblings->dwtimestart()
	  + (((double) x_position / (double) graph_width) * res_sis->Siblings->dwtimespan());

	/* Lookup the Resouce(s) values. First, we have to find out which resource
	 * was clicked on. */
	theObject = (MW_object *) res_sis->legend_display->findTheVSIat(bevent->y, NULL);
	if(!theObject) {
		return; }
	if(theObject->get_resource_handle()) {
		nlines = res_sis->report_at_time(atime, report, theObject); }
	else {
		nlines = 1;
		report = "UNDEFINED"; }
#ifdef   DEBUG
	printf("RD_sys::popup_at_time:  nlines=%d, report=\n", nlines);
	printf("%s", report());
#endif /*DEBUG*/

	//reset contents of motif_widget, and pop it up (if not already managed)
	//  -- temporarily do directly, until can get operator= to work!!!
//  *_rdPopupDialogShell->rdPopupTextMultiline = report;

	if(! _rdPopupDialogShell)
		_rdPopupDialogShell = new rd_popup("rdPopupDialogShell", MW, "UNINITIALIZED WIDGET!!!");

	// Well... if we are going to unmanage the Widget, better make sure we set the attributes AFTER we manage...
	// XtVaSetValues(_rdPopupDialogShell->rdPopupTextMultiline->widget,
	// 		XmNvalue, report(),
	//		NULL);
	// NO NO NO! Force stupid window manager (such as Solaris) to display window on top:
#ifndef WELL_MAYBE_SIMPLICITY_IS_BETTER
	if(! (_rdPopupDialogShell->rdPopupForm->is_managed()))
		_rdPopupDialogShell->rdPopupForm->manage();
#else
	if(_rdPopupDialogShell->rdPopupForm->is_managed()) {
		// preserve the location, otherwise the @#$*&#$*&^ window manager will move the window around
		// in unpredictable ways:
		XtVaGetValues(_rdPopupDialogShell->widget, XmNx, &x_position, XmNy, &y_position, NULL);
		_rdPopupDialogShell->rdPopupForm->unmanage();
		_rdPopupDialogShell->rdPopupForm->manage();
		XtVaSetValues(_rdPopupDialogShell->rdPopupTextMultiline->widget,
			XmNx, x_position, XmNy, y_position, NULL); }
	else {
		_rdPopupDialogShell->rdPopupForm->manage(); }
#endif

	// XtVaSetValues(_rdPopupDialogShell->rdPopupTextMultiline->widget,
	// 		XmNvalue, report(),
	// 		NULL);
	*((motif_widget *) _rdPopupDialogShell->rdPopupTextMultiline) = report;

	// Doesn't work; using xmMenuShellWidgetClass now:
	// XMapRaised(XtDisplay(_rdPopupDialogShell->widget), XtWindow(XtParent(_rdPopupDialogShell->widget)));
	}
	}

//Report on the value(s) of all Resources at a given time (as known by
//  the objects in this Resource System); return is #lines in report
int RD_sys::report_at_time(
			const CTime	atime,
			Cstring&	report,
			MW_object*	mwobjectnode) {
	int			nlines = 0;
	double			xval, yval;
	Cstring			ystring;
	char			str_copy_array[256];	//should be enough for label+unit
	char			*str_copy = str_copy_array;
	char			*str_newline;
	MW_object::Formatting	ret;
	int			maxchar, namelen, valuelen;
	TIMEZONE		ADzone;

	report.undefine();		//undefines output string (allocated by caller)
	xval = time_to_percent(atime); //get % required for lookup_yvalue()

	//Get current timezone, from parent/sibling AD's timemark's timezone.  Find
	//  the timemark NOT by cycling through AD siblings (which comprise all
	//  parts of the AD and all its RD's),but instead by calling new AD method.
	ADzone = activitySystem->timezone;

	//String the 2 date formats together, eliding the duplicated 4-digit year:
	Cstring scet_time;
	atime.time_to_complete_SCET_string(scet_time, ADzone);
	strcpy(str_copy, *scet_time);
	report = Cstring(str_copy);
	report << '\n';
	nlines++;

	//Cycle through 0+ objects plotted in this Resource System
	strcpy(str_copy, *mwobjectnode->get_key());
	if((str_newline = strchr(str_copy, '\n')))
	    str_newline[0] = ' ';	//newline->blank before any units
	report << str_copy << " = ";
	namelen = strlen(str_copy) + 3; //length of name, units, and = sign

	//Want REPORT_NUMERIC_MAXFMT characters maximum for numbers; allow up
	//  to REPORT_STRING_MAXFMT (guaranteed longer) characters for strings.
	if(apgen::RETURN_STATUS::SUCCESS == mwobjectnode->lookup_yvalue(xval, yval)) {
		ret = mwobjectnode->format_yvalue(yval,
			      REPORT_NUMERIC_MAXFMT,
			      ystring);

	    if(ret == MW_object::UNFORMATTED) //shouldn't happen with REPORT_NUMERIC_MAXFMT;don't
		report << "UNPRINTABLE" << '\n'; //bother with multiline logic
	    else {		//apgen::RETURN_STATUS::SUCCESS, or NOP (string truncated)
		//loop until no truncation, or reach string maximum
		for (maxchar = (REPORT_NUMERIC_MAXFMT+1);
		     maxchar <= REPORT_STRING_MAXFMT;
		     maxchar++)
		    if(MW_object::FORMAT_OK ==
		           mwobjectnode->format_yvalue(yval, maxchar, ystring))
			break;
		valuelen = ystring.length();	//length of formatted value
		if((namelen + valuelen) > REPORT_STRING_MAXFMT) {
		    report << "\n    ";
		    nlines++;
		}
		report << ystring << '\n';
	    }
	} else {		//lookup failed (i.e., before/after plotted values)
	    valuelen = 9;	//length of "UNDEFINED"
	    if((namelen + valuelen) > REPORT_STRING_MAXFMT) {
	        report << "\n    ";
	        nlines++;
	    }
	    report << "UNDEFINED" << '\n';
	}
	nlines ++;
	return nlines;
}

static MW_object*	theObjectWhoseOptionsAreBeingChanged = NULL;

	/* In this and other callback functions, we need to emulate DS_lndraw and determine the appropriate
	 * object to delegate things to based on the y coordinate of the clicking event, which will determing
	 * which vertically_scrolled_item should be put in charge. */
void RD_sys::rdOptionsMenuCallback(Widget, callback_stuff *client_data, void *call_data) {
	if(GUI_Flag) {
	int			resolution;
				//motif_wrapper passes object pointer as member "data" of client_data
	RD_sys			*res_sis = (RD_sys *) client_data->data;
	XButtonEvent		*bevent = (XButtonEvent *) call_data;
	DS_scroll_graph		*legend_as_a_ds_scroll_graph = res_sis->legend_display->get_this();

	// We need to determine which MW_object was hit, taking into account scrolling parameters.
	// theObject = (MW_object *) res_sis->legend_display->findTheVSIat(bevent->y, NULL);
	if(!theObjectWhoseOptionsAreBeingChanged) {
		return;
	}

	//do appropriate action (NOTE that if a mode swap occurs, setyscrollmode()
	//  handles relevant button de/sensitizations) 
	if(client_data->parent->get_key() == "Manual") {
	    	theObjectWhoseOptionsAreBeingChanged->setyscrollmode(YSCROLL_MANUAL);
		// legend_as_a_ds_scroll_graph->configure_graphics(NULL);
		udef_intfc::something_happened() += 1;
		theObjectWhoseOptionsAreBeingChanged->setStaleFlag(true);
	} else if(client_data->parent->get_key() == "Auto Window") {
	    	theObjectWhoseOptionsAreBeingChanged->setyscrollmode(YSCROLL_AUTO_WINDOW);
		// legend_as_a_ds_scroll_graph->configure_graphics(NULL);
		udef_intfc::something_happened() += 1;
		theObjectWhoseOptionsAreBeingChanged->setStaleFlag(true);
	} else if(client_data->parent->get_key() == "Auto Plan") {
	    	theObjectWhoseOptionsAreBeingChanged->setyscrollmode(YSCROLL_AUTO_PLAN);
		// legend_as_a_ds_scroll_graph->configure_graphics(NULL);
		udef_intfc::something_happened() += 1;
		theObjectWhoseOptionsAreBeingChanged->setStaleFlag(true);
	} else if(client_data->parent->get_key() == "Background Points") {
		if(! _rdBackgroundPtsPopupDialogShell)
	    		_rdBackgroundPtsPopupDialogShell = new
				UI_rdBackgroundPts("rdBackgroundPtsPopupDialogShell", MW, UI_rdBackgroundPts::rdbOKCallback);

		resolution = theObjectWhoseOptionsAreBeingChanged->getModelingResolution(res_sis->Siblings->dwtimespan());
		_rdBackgroundPtsPopupDialogShell->initialize(res_sis, resolution, theObjectWhoseOptionsAreBeingChanged->get_key());

		if(_rdBackgroundPtsPopupDialogShell->_UI_rdBackgroundPts->is_managed())
			_rdBackgroundPtsPopupDialogShell->_UI_rdBackgroundPts->unmanage();
		_rdBackgroundPtsPopupDialogShell->_UI_rdBackgroundPts->manage();
	}
	theObjectWhoseOptionsAreBeingChanged = NULL;
	}
}

void RD_sys::popup_rd_optionsCB(Widget, callback_stuff * client_data, void * call_data) {
	if(GUI_Flag) {
		XButtonEvent			*bevent = (XButtonEvent *) call_data;
		RD_sys				*res_sis = (RD_sys*)(client_data->data);

		/* We can no longer do that because with scrolled resources, many
		 * resources are displayed in a single RD_sys. */
		// Rsource		*theResource = res_sis->get_resource_handle();
		theObjectWhoseOptionsAreBeingChanged = (MW_object *) res_sis->legend_display->findTheVSIat(bevent->y, NULL);
		if(!theObjectWhoseOptionsAreBeingChanged)
			return;
		
		// emulate seq_review code in plot_util.C, Menu::Menu():
		if(! res_sis->rdOptionsMenuPopup) {
			res_sis->rdOptionsMenuPopup = new motif_widget("popup_menu",
				xmPopupMenuWidgetClass, res_sis->myParent, NULL, 0, FALSE);
			res_sis->manual_button = new motif_widget("Manual",
				xmPushButtonWidgetClass, res_sis->rdOptionsMenuPopup);
			res_sis->manual_button->add_callback(rdOptionsMenuCallback,
				XmNactivateCallback, res_sis);
			res_sis->auto_window_button = new motif_widget("Auto Window",
				xmPushButtonWidgetClass, res_sis->rdOptionsMenuPopup);
			res_sis->auto_window_button->add_callback(rdOptionsMenuCallback,
				XmNactivateCallback, res_sis);
			res_sis->auto_plan_button = new motif_widget("Auto Plan",
				xmPushButtonWidgetClass, res_sis->rdOptionsMenuPopup);
			res_sis->auto_plan_button->add_callback(rdOptionsMenuCallback,
				XmNactivateCallback, res_sis);
			//res_sis->auto_range_button = new motif_widget("Auto Range",
			//  xmPushButtonWidgetClass, res_sis->rdOptionsMenuPopup);
			//res_sis->auto_range_button->add_callback(rdOptionsMenuCallback,
			//  XmNactivateCallback, res_sis);
			res_sis->background_button = new motif_widget("separator",
				xmSeparatorWidgetClass, res_sis->rdOptionsMenuPopup);
			res_sis->background_button = new motif_widget("Background Points",
				xmPushButtonWidgetClass, res_sis->rdOptionsMenuPopup);
			res_sis->background_button->add_callback(rdOptionsMenuCallback,
				XmNactivateCallback, res_sis);
		}
		if(theObjectWhoseOptionsAreBeingChanged->getyscrollmode() == YSCROLL_AUTO_WINDOW) {
			res_sis->auto_window_button->set_sensitive(FALSE);
			res_sis->manual_button->set_sensitive(TRUE);
			res_sis->auto_plan_button->set_sensitive(TRUE);
		} else if(theObjectWhoseOptionsAreBeingChanged->getyscrollmode() == YSCROLL_MANUAL) {
			res_sis->auto_window_button->set_sensitive(TRUE);
			res_sis->manual_button->set_sensitive(FALSE);
			res_sis->auto_plan_button->set_sensitive(TRUE);
		} else if(theObjectWhoseOptionsAreBeingChanged->getyscrollmode() == YSCROLL_AUTO_PLAN) {
			res_sis->auto_window_button->set_sensitive(TRUE);
			res_sis->manual_button->set_sensitive(TRUE);
			res_sis->auto_plan_button->set_sensitive(FALSE);
		}
		res_sis->rdOptionsMenuPopup->set_menu_position(bevent);
		res_sis->rdOptionsMenuPopup->manage();
		// KEEP WINDOW
		XMapRaised(	XtDisplay(res_sis->rdOptionsMenuPopup->widget),
				XtWindow(XtParent(res_sis->rdOptionsMenuPopup->widget)));
	}
}

	/* Configure_graphics is used in two different contexts: When the core database
	 * changes (e. g. because a command was received or remodeling took place), and
	 * when the GUI undergoes a change (e. g. the user repainting a window.)
	 *
	 * When configure_graphics is called as a result of an Expose or Resize event,
	 * the callback_stuff is the "real thing". When it is the result of a
	 * discrepancy with the core data, the argument is NULL. In the latter case,
	 * the MW_legend doesn't know that it's out of Sync, so we need to tell
	 * it to configure itself. */
void RD_sys::configure_graphics(callback_stuff *) {
	List_iterator		the_scrolled_items(legend_display->pointersToTheVisibleLegends);
	bpointernode		*b;
	MW_object		*theOb;
	Cstring			any_errors;

	// debug
	// cerr << get_key() << "->configure_graphics() start\n";
	cleargraph();
	while((b = (bpointernode *) the_scrolled_items())) {
		theOb = (MW_object *) b->get_ptr();
		// debug
		// cerr << theOb->get_key() << "->refresh_all_internal_data() will be called...\n";
		theOb->refresh_all_internal_data(any_errors); }
	XtVaSetValues(legend_display->get_this()->widget, XmNheight, getheight(), NULL);
	legend_display->get_this()->configure_graphics(NULL);

	// The update_graphics method actually used is in MW_widgetdraw:
	update_graphics(NULL);
}

RD_YSCROLL_MODE RD_sys::getyscrollmode(const Cstring &s) {
	MW_object	*m = (MW_object *) mw_objects.find(s);

	if(m) {
		return m->getyscrollmode();
	}
	return YSCROLL_AUTO_PLAN;
}


void RD_sys::setyscrollmode(const Cstring &s, RD_YSCROLL_MODE r) {
	MW_object	*m = (MW_object *) mw_objects.find(s);

	if(m) {
		m->setyscrollmode(r);
	}
}


int MW_widgetdraw::getwidth() {
	if(GUI_Flag) {
		Dimension	I = 0;

		if(widget) {
			XtVaGetValues(widget, XmNwidth, &I, NULL);
		}
		return I;
	}
	return 0;
}

int MW_widgetdraw::getheight() {
	if(GUI_Flag) {
		Dimension	I = 0;

		if(widget) {
			XtVaGetValues(widget, XmNheight, &I, NULL);
		}
		return I;
	}
	return 0;
}

void MW_widgetdraw::draw_vertical_cursor(int xpos) {
	if(GUI_Flag) {
	if(!Siblings->verticalCursorEnabled) {
		return;
	}
	if(widget) {
		last_cursor_position = xpos;
		the_vertical_cursor_is_visible = (the_vertical_cursor_is_visible + 1) % 2;
		XSetFunction(Defdisplay, DS_object::gethilite()->getGC(), GXinvert);
		// KEEP WINDOW
		XDrawLine(Defdisplay, XtWindow(widget),
			DS_object::gethilite()->getGC(), xpos, 0, xpos, getheight());
		XSetFunction(Defdisplay, DS_object::gethilite()->getGC(), GXcopy);
	}
	}
}

void MW_widgetdraw::erase_vertical_cursor() {
	if(GUI_Flag) {
	if(!Siblings->verticalCursorEnabled)
		return;
	if(!the_vertical_cursor_is_visible)
		return;
	the_vertical_cursor_is_visible = 0;
	if(widget) {
		XSetFunction(Defdisplay, DS_object::gethilite()->getGC(), GXinvert);
		// KEEP WINDOW
		XDrawLine(Defdisplay, XtWindow(widget),
			DS_object::gethilite()->getGC(), last_cursor_position, 0, last_cursor_position, getheight());
		XSetFunction(Defdisplay, DS_object::gethilite()->getGC(), GXcopy);
	}
	}
}

void MW_widgetdraw::manage() {
	motif_widget::manage();
	// will initialize toggle flag and draw cursor if necessary:
	cleargraph();
	if(RD_sys::thePendingLayout().length()) {
		Cnode0<alpha_string, RD_sys::layout_stuff>*
			tag = RD_sys::list_of_tagged_layout_lists.find(RD_sys::thePendingLayout());
		if(tag) {
			RD_sys::list_of_tagged_layout_lists.remove_node(tag);
			RD_sys::reorder_legends_as_per(RD_sys::thePendingLayout(),
				tag->payload.res_names,
				tag->payload.heights,
				tag->payload.source_file,
				tag->payload.lineno);
			delete tag;
		}
		RD_sys::thePendingLayout() = "";
	}
}

//
// Draw EACH MW_object
//
void MW_widgetdraw::update_graphics(callback_stuff *) {
	List_iterator		the_scrolled_items(legend_display->pointersToTheVisibleLegends);
	bpointernode		*b;
	MW_object		*theOb = (MW_object *) legend_display->findTheVSIat(0, NULL);
	DS_scroll_graph		*legend_as_a_ds_scroll_graph = legend_display->get_this();
	int			bottom_of_VSI = legend_display->vertical_distance_above_item(theOb);
	int			top_of_legend, H;

	// debug
	// cerr << "MW_widgetdraw(" << get_key() << ")->update_graphics(): START" << endl;

	//DSG 96-10-04 cannot put in constructor, since widgets MUST be REALIZED!!!
	//DSG 96-10-07 code now checks for that; fails check ONLY if no RD yet
	sr_xm_initialize_gcs(legend_as_a_ds_scroll_graph->widget, widget);
	sr_xm_initialize_gravity(legend_as_a_ds_scroll_graph->widget);
	sr_xm_initialize_gravity(widget);

	// REAL TIME DEBUG
	// cerr << get_key() << " update graphics start.\n";

	/* PROBLEM: redraw the cursor correctly. If we call cleargraph now, what happens?
	 * Answer: don't worry. The point is that the logic is different from the ACT_sys
	 * case. In ACT_sys, we first cleargraph(), then draw activities, then add the
	 * vertical cursor if necessary (see DS_lndraw::add_vertical_cursor_if_necessary).
	 *
	 * For the resource case, we use the old logic: first draw all the resource plots,
	 * then draw the vertical cursor globally.
	 *
	 * For efficiency, we call cleargraph once and will not re-invoke it from the
	 * MW_object::refresh_xxx() methods.  */
	// debug
	// cerr << get_key() << "->update_graphics() calling cleargraph...\n";
	cleargraph();
	while((b = (bpointernode *) the_scrolled_items())) {
		theOb = (MW_object *) b->get_ptr();
		top_of_legend = bottom_of_VSI;
		H = theOb->get_vertical_size_in_pixels_in(legend_display);
		bottom_of_VSI += H;
		
		// Need to store the correct parameters in this device. We used to
		// own the whole DrawingArea, but not anymore.
		plot_screen	device2(	this,
						legend_display->get_this()->getwidth(),
						top_of_legend + RESVERUNIT,
						H - RESVERUNIT,
						USE_CANVAS_GC);

		/* clears the drawArea. Note that the mwobjectnode 'knows' its
		 * position in the MW_widgetdraw's DrawingArea widget.  */

		// debug
		// cerr << "  calling " << theOb->get_key() << "->refresh_canvas...\n";
		theOb->my_res_version = -1;
		theOb->refresh_canvas(device2);
	}
	if(Siblings->verticalCursorEnabled) {
		the_vertical_cursor_is_visible = 0;
		draw_vertical_cursor(((RD_sys *) this)->activitySystem->last_cursor_position);
	}
}

    // Draw EACH MW_object
void MW_widgetdraw::update_events() {
	List_iterator		the_scrolled_items(legend_display->pointersToTheVisibleLegends);
	bpointernode		*b;
	MW_object		*theOb = (MW_object *) legend_display->findTheVSIat(0, NULL);
	DS_scroll_graph		*legend_as_a_ds_scroll_graph = legend_display->get_this();
	int			bottom_of_VSI = legend_display->vertical_distance_above_item(theOb);
	int			top_of_legend, H;

	while((b = (bpointernode *) the_scrolled_items())) {
		theOb = (MW_object *) b->get_ptr();
		top_of_legend = bottom_of_VSI;
		H = theOb->get_vertical_size_in_pixels_in(legend_display);
		bottom_of_VSI += H;
		
		// Need to store the correct parameters in this device. We used to
		// own the whole DrawingArea, but not anymore.
		plot_screen	device2(	this,
						legend_display->get_this()->getwidth(),
						top_of_legend + RESVERUNIT,
						H - RESVERUNIT,
						USE_CANVAS_GC);

		theOb->refresh_events(device2);
	}
}

//Hardcopy PostScript plotting analogue to update_graphics().  Note separate
//  args for X for legend and canvas (they are side-by-side), but shared args
//  for Y (side-by-side areas fit in same horizontal slice of the page).
void MW_object::plot_graphics(Cstring & f,
		int legend_x_off, int canvas_x_off, int y_off,
		int legend_x_wid, int canvas_x_wid, int y_hgt,
		int pages_per_display_width, int page,
		double plotter_resolution_in_points) {
	Cstring		g;		//allows interleaving Legend and Canvas writes
	Plotter_slice	*LegendPlot, *CanvasPlot;

	//Set up Legend,Canvas Plot Devices (re-create for EACH call of this
	//  method).  Plotter_slice constructor does transformation && clip region.
	//  //  NO scaling is used here (i.e. X and Y scale values are all 1.0).
	//HARDCODE scaling as 0.05, which is used to increase pixel (integer unit)
	//  density by 20 in both X,Y directions, by using integer numbers passed
	//  in which are 20 times what they ultimately should be.  Must do this way
	//  to avoid unwanted effects of "true" scaling, which affects line width.
	LegendPlot = new Plotter_slice(legend_x_off, legend_x_wid,
				    plotter_resolution_in_points,
				    y_off, y_hgt,
				    plotter_resolution_in_points);
	f << LegendPlot->get_output();
	CanvasPlot = new Plotter_slice(canvas_x_off, canvas_x_wid,
				    plotter_resolution_in_points,
				    y_off, y_hgt,
				    plotter_resolution_in_points);
	g << CanvasPlot->get_output();

	//Plot the Legend and Canvas object pair(s).
	refresh_legend(*LegendPlot);
	f << LegendPlot->get_output();
	refresh_canvas(*CanvasPlot, pages_per_display_width, page);
	g << CanvasPlot->get_output();

	//Concatenate the Canvas PS output onto the Legend output, and also add the
	//  graphics-restore code (really want to do in destructor, but then it is
	//  too late to use get_output() to get the result out!).
	f << LegendPlot->destructor() << g << CanvasPlot->destructor();

	//Delete Legend and Canvas Plot Devices.
	delete(LegendPlot);
	delete(CanvasPlot);

	DBG_NOINDENT("MW_widgetdraw::plot_graphics:  completed, string=" << endl);
	DBG_NOINDENT(f << endl);
	}

MW_legend::MW_legend(	const Cstring	&name,
			motif_widget	*parent,
			RD_sys		*legend_of,
			void		*arguments,
			int		number_of_Args) 
	: // The following constructor creates a form:
	DS_scroll_graph(name, parent, arguments, number_of_Args, legend_of->get_siblings()),
	VSI_owner(this),
	theResSysParent(legend_of)
	{
	DBG_INDENT("Will attach ExposeCallback for ExposureMask events to DrawingArea widget later on.\n");
	DBG_UNINDENT("...DONE.\n");
	}

MW_legend::~MW_legend() {
}

/* The following method initializes the arguments to be used for the top-level widget
 * of each MW_object (a form). Note that in addition to the options below, the client
 * (DS_scroll_graph::configure_graphics()) sets the XmNheight and XmNy attributes and
 * adds them to the list of arguments. */
int MW_legend::InitializeArgsForScrolledItems(void *theArgs) {
	int	ac  = 0;

	if(GUI_Flag) {
	Arg	*args = (Arg *) theArgs;

	XtSetArg(args[ ac  ], XmNrecomputeSize,	False)				; ac ++;
	// XtSetArg(args[ ac  ], XmNhighlightThickness,	0)				; ac ++;
	// XtSetArg(args[ ac  ], XmNborderWidth,	1)				; ac ++;
	// XtSetArg(args[ ac  ], XmNalignment,		XmALIGNMENT_BEGINNING)		; ac ++;
	XtSetArg(args[ ac  ], XmNbackground,		ResourceData.grey_color)	; ac ++;
	XtSetArg(args[ ac  ], XmNforeground,		ResourceData.black_color)	; ac ++;
	XtSetArg(args[ ac  ], XmNwidth,		ACTLEGNDWIDTH)			; ac++;
	}
	return ac ;
}

blist &MW_legend::get_blist_of_vertically_scrolled_items() const {
	// This includes all MW_continuous and MW_discrete objects
	return theResSysParent->mw_objects; }

bool MW_legend::is_stale() const {
	List_iterator	theObjects(get_blist_of_vertically_scrolled_items());
	MW_object*	obj;

	DBG_INDENT(get_key() << "->is_stale() START\n");
	while((obj = (MW_object*) theObjects())) {
		if(obj->is_stale()) {
			DBG_UNINDENT(get_key() << "->is_stale() DONE (yes)\n");
			return true;
		}
	}
	DBG_UNINDENT(get_key() << "->is_stale() DONE (no)\n");
	// debug
	// cout << get_key() << "->is_stale(): no object found to be stale; NO.\n";
	return false;
}

void MW_legend::pre_configure_graphics(callback_stuff *) {
	DBG_NOINDENT(get_key() << "->pre_configure_graphics(): calling setAllStaleFlags(false).\n");
	setAllStaleFlags(false); }


void MW_legend::setAllStaleFlags(bool value) {
	List_iterator	theObjects(get_blist_of_vertically_scrolled_items());
	MW_object	*obj;

	DBG_NOINDENT(get_key() << "->setAllStaleFlags(): setting all stale flags to " << value << ".\n");
	while((obj = (MW_object *) theObjects())) {
		// debug
		// cout << "setting stale flag of " << obj->get_key() << " to " << value << endl;
		obj->setStaleFlag(value); } }

// Resource display selection CallBack
void MW_legend::resourceselected(Widget wid, callback_stuff *client_data, XtPointer) {
	if(GUI_Flag) {
	MW_object			*theObj = (MW_object *) client_data->data;
	RD_sys				*res_sis = theObj->get_res_sys();
	VSI_owner			*vo = res_sis->get_legend_display();
	MW_legend			*lg = (MW_legend *) vo->get_this();
	motif_widget			*toggle_button = (motif_widget *) client_data->parent;
	bpointernode			*b = (bpointernode *) vo->pointersToTheVisibleLegends.find((void *) theObj);
	SELECT_RES_LEGENDrequest	*request;

	if(!b) {
		// debug
		cerr << "Weird: MW_legend::resourceselected cannot find the toggle button.\n";
		return; }
	DBG_NOINDENT("MW_legend::resourceselected: legend = " << theObj->get_key() << "\n");

	if(XmToggleButtonGetState(toggle_button->widget)) {
		request = new SELECT_RES_LEGENDrequest(Cstring("A") + ("ACT_system" / res_sis->activitySystem->get_key()), theObj->get_key(), 1); }
	else {
		request = new SELECT_RES_LEGENDrequest(Cstring("A") + ("ACT_system" / res_sis->activitySystem->get_key()), theObj->get_key(), 0); }
	request->process(); } }

void MW_legend::cleargraph() {
	if(GUI_Flag) {
	List_iterator	theVSIpointers(pointersToTheVisibleLegends);
	bpointernode	*vp;
	MW_object	*obj;
	motif_widget	*mw;
	Dimension	I = 0, J = 0;

	DBG_INDENT("MW_legend(" << get_key() << ")::cleargraph START; will clear "
			<< pointersToTheVisibleLegends.get_length() << " legend(s)...\n");
	while((vp = (bpointernode *) theVSIpointers())) {
		obj = (MW_object *) vp->get_ptr();
		if(obj) {
			mw = obj->get_drawarea_widget();
			XtVaGetValues(mw->widget, XmNwidth, &I, NULL);
			XtVaGetValues(mw->widget, XmNheight, &J, NULL);
			XClearArea(Defdisplay,
				XtWindow(mw->widget),
				0,             0,
				I,		J,
				FALSE); } }
	DBG_UNINDENT("MW_legend(" << get_key() << ")::cleargraph DONE\n"); } }

// Called by DS_graph::ExposeCallback when receiving an Expose Xevent
void MW_legend::update_graphics(callback_stuff *) {
	List_iterator		the_scrolled_items(pointersToTheVisibleLegends);
	bpointernode		*b;
	MW_object		*theOb = (MW_object *) findTheVSIat(0, NULL);
	DS_scroll_graph		*legend_as_a_ds_scroll_graph = get_this();
	int			bottom_of_VSI = vertical_distance_above_item(theOb);
	int			top_of_legend, H;

	sr_xm_initialize_gcs(widget, theResSysParent->widget);

	sr_xm_initialize_gravity(widget);
	sr_xm_initialize_gravity(theResSysParent->widget);


	// REAL TIME DEBUG
	// cerr << get_key() << " update graphics start.\n";
	DBG_INDENT("MW_legend(" << get_key() << ")::update_graphics START\n");
	cleargraph();
	while((b = (bpointernode *) the_scrolled_items())) {
		theOb = (MW_object *) b->get_ptr();
		top_of_legend = bottom_of_VSI;
		H = theOb->get_vertical_size_in_pixels_in(this);
		bottom_of_VSI += H;
		plot_screen	device(	theOb->get_drawarea_widget(),
					legend_as_a_ds_scroll_graph->getwidth(),
					0, // top_of_legend + RESVERUNIT,
					H - RESVERUNIT,
					USE_LEGEND_GC);
		// ::refresh_legend() better expect the global form in this case...
		DBG_NOINDENT("Calling refresh_legend on node " << theOb->get_key() << endl);
		theOb->refresh_legend(device); }
	DBG_UNINDENT("MW_legend(" << get_key() << ")::update_graphics DONE\n");
	}

motif_widget *MW_legend::widget_to_use_for_height_info() {
	return theResSysParent; }


synchro::problem* RD_sys::checkResPlotStaleness(synchro::detector* D) {
	RD_sys*		res_sis = dynamic_cast<RD_sys*>(D);
	List_iterator	the_scrolled_items(res_sis->legend_display->pointersToTheVisibleLegends);
	bool		stale = false;
	bpointernode*	b;
	MW_object*	theOb;

	while((b = (bpointernode *) the_scrolled_items())) {
		theOb = (MW_object *) b->get_ptr();
		if(theOb->is_stale()) {
			stale = true;
			break;
		}
	}
	if(stale) {
		return new synchro::problem(
			"legend_display->pointersToTheVisibleLegends reports stale legends",
			res_sis,
			RD_sys::checkResPlotStaleness,
			NULL,
			NULL);
	}
	return NULL;
}

void RD_sys::fixResPlotStaleness(synchro::problem* F) {
	RD_sys*		res_sis = dynamic_cast<RD_sys*>(F->payload.the_fixer);

	F->action_taken << res_sis->fixerName() <<
		"->configure_graphics(NULL)";
	res_sis->configure_graphics(NULL);
}
