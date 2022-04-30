#if HAVE_CONFIG_H
#include <config.h>
#endif
// #define apDEBUG

#include "apDEBUG.H"

	// THIS VERSION CONTAINS NOTES IN THE FORM "CR 1 REQUIREMENT"... this is
	// to support the Cassini request for disambiguation of activities that 'pile up'.

#include <Xm/ToggleB.h>

#include "ACT_exec.H"
#include "ActivityInstance.H"
#include "ACT_sys.H"
#include "APmodel.H"
#include "APerrors.H"
#include "EventRegistry.H"
#include "RES_eval.H"
#include "UI_activitydisplay.H"
#include "UI_dsconfig.h"
#include "UI_ds_draw.H"
#include "UI_ds_timeline.H"
#include "UI_exec.H"
#include "UI_GeneralMotifInclude.H"
#include "UI_mainwindow.H"
#include "UI_messagewarndialog.H"
#include "UI_resourcedisplay.H"
#include "UTL_defs.H"
#include "UTL_time.H"
#include "action_request.H"
#include "Prefs.H"
#include "gtk_bridge.H"

using namespace std;

Cstring  HorizontalScrollingAction("HorizontalScrollingAction") ;
Cstring  HorizontalZoomingAction("HorizontalZoomingAction");
Cstring  VerticalScrollingAction("VerticalScrollingAction");
Cstring  VerticalExpansion("VerticalExpansion");
Cstring  VerticalSwap("VerticalSwap");
Cstring  ActivityCreation("ActivityCreation");
Cstring  GlobalChange("GlobalChange");
Cstring  PrepareForAction("PrepareForAction");
Cstring  Purge("Purge");
Cstring  Remodel("Remodel");
Cstring  RefreshActivities("RefreshActivities");

				// in UI_motif_widget.C:
extern resource_data		ResourceData;

				// in main.C:
extern refresh_info		refreshInfo;

				// Class globals for the Display Subsystem (DS_)

int				DS_lndraw::mv_x = 0;
int				DS_lndraw::mv_y = 0;

int				DS_graph::Defscreenum = - 1;
int				DS_graph::Defdepth = 0;
Colormap			DS_graph::Defcolormap;

Cursor				DS_time_scroll_graph::dscursor = (Cursor) NULL;
Cursor				DS_time_scroll_graph::pastecursor = (Cursor) NULL;
Cursor				DS_time_scroll_graph::selcursor = (Cursor) NULL;
Cursor				DS_time_scroll_graph::defcursor = (Cursor) NULL;
Cursor				DS_time_scroll_graph::expansion_cursor = (Cursor) NULL;
unsigned char			DS_time_scroll_graph::displaymode = NOPMODE;

extern blist			windows_we_redraw_ourselves;
DS_pixmap_board			*DS_object::bk_board = NULL;
Display				*DS_graph::Defdisplay = NULL;

ACT_sys				*DS_lndraw::actsys_in_which_last_click_occurred = 0;
// List				DS_lndraw::lngclst;
DS_gc				**DS_lndraw::GC_array = NULL;

int				DS_graph::sel_state = 0;

				
					// in UI_exec.C:
extern const char*			contextForRequestingTheNewActivity;
extern UI_exec*				UI_subsystem;
static Cstring				selected_type_of_new_activity = "generic";
extern slist<alpha_void, dumb_actptr>&	tentativeDescendants();

					// in UI_mainwindow.C:
extern motif_widget*			MW;
extern motif_widget*			TopLevelWidget;

				// in UI_activityeditor.C:
extern apgen::RETURN_STATUS	nime_to_epoch_relative_string(const CTime_base &newstartime,
					const Cstring &epoch, Cstring &result, Cstring &errmsg);

				// below:
CTime_base			determineTimemarkOffset(
					CTime_base		startTime,
					CTime_base		majors,
					const TIMEZONE		&timezone);

int				DragChildren = 1;
int				SelectionFrozen = 0;

#define				CLICK_INTERVAL 250
Time				last_click = 0;

static Arg      args[26];
static Cardinal ac=0;
extern BITMAPDEF AP_Bitmap_pool[];

#define SEL_ON 17
#define SEL_OFF 18

extern int LNDRAWWIDTH;

extern apgen::RETURN_STATUS determineTimemarkLayout (
	CTime_base startTime,
	CTime_base duration,
	const TIMEZONE &timezone,
	CTime_base &Majors,
	CTime_base &Minors,
	CTime_base &offset);

DS_graph::DS_graph(const Cstring & name, const WidgetClass c, motif_widget *parent, void * arguments, int number)
	: motif_widget(name, c, parent, arguments, number, TRUE) {

	if(! Defdisplay) {
		Defdisplay = XtDisplay(widget);
		Defscreenum = XDefaultScreen(Defdisplay);
		Defdepth = XDefaultDepth(Defdisplay, Defscreenum);
		Defcolormap = XDefaultColormap(Defdisplay, Defscreenum); }

	DBG_NOINDENT("DS_graph::DS_graph() for Widget \"" << XtName(widget) << "\"\n"); }

int	DS_graph::getwidth() {
#ifdef GUI
	if(GUI_Flag) {
	Dimension	I = 0;

	if(widget)
		XtVaGetValues(widget, XmNwidth, & I, NULL);
	// debug
	// if(I <= 1) {
	// 	cout << "\n\n(" << get_key() << ") has small width... something happened.\n\n"; }
	return I;
	}
	else {
		return 0; 
	}
#else
	return 0; 
#endif
	}

int	DS_graph::getheight() {
#ifdef GUI
	if(GUI_Flag) {
	Dimension	I = 0;

	if(widget)
		XtVaGetValues(widget, XmNheight, & I, NULL);
	return I;
	}
	else {
		return 0; 
	}
#else
	return 0;
#endif
	}

int	DS_scroll_graph::isVisible() {
	return myParent->myParent->is_managed(); }

void DS_graph::ExposeCallback(Widget w, callback_stuff *client_data, void *E) {
#ifdef GUI
	if(GUI_Flag) {
	XEvent				*event = (XEvent *) E;
	DS_graph			*eobject = (DS_graph *) client_data->data;

	udef_intfc::something_happened() += 1;

	// REAL TIME DEBUG
	// cerr << eobject->get_key() << " ExposeCallback start\n";

	// Do not forget this, which will eliminate unnecessary redraw actions:
	if(!windows_we_redraw_ourselves.find((void *) XtWindow(w))) {

		// debug
		// cerr << "Inserting ptr for window " << ((long) XtWindow(w)) << ", object " << eobject->get_key() << "\n";

		windows_we_redraw_ourselves << new pointer_to_pointer((void *) XtWindow(w), eobject); }
	DBG_INDENT("DS_graph(\"" << XtName(w) << "\")::ExposeCallback for object \"" <<
		eobject->get_key() << "\"\n");
	DBG_NOINDENT("WIDTH-HEIGHT check: (" << eobject->getwidth() << ", " << eobject->getheight() << ")\n");

	if(! DS_object::bk_board)
		DS_object::bk_board = new DS_pixmap_board(XtParent(eobject->widget), AP_Bitmap_pool, NUM_OF_PATTERNS);

	if(event->type == ConfigureNotify && eobject->isVisible()) {
		if(refresh_info::ON) {
			refreshInfo.add_level_2_trigger(eobject->get_key(), "RESIZE event"); }
		DBG_NOINDENT("...RESIZE event; will reset all objects...\n"; cout.flush());
		eobject->configure_graphics(client_data); }
	else if(event->type == Expose && eobject->isVisible()) {
		if(refresh_info::ON) {
			refreshInfo.add_level_2_trigger(eobject->get_key(), "EXPOSE event"); }
		DBG_NOINDENT("...EXPOSE event; will call update_graphics only...\n"; cout.flush());
		eobject->update_graphics(client_data); }

	DBG_UNINDENT("DS_graph(\"" << XtName(w) << "\")::ExposeCallback: END\n"; cout.flush());
	}
#endif
}

// form 1: creates a form
DS_scroll_graph::DS_scroll_graph(const Cstring &N,
		motif_widget	*parent,
		void		*arguments,
		int		number,
		list_of_siblings *LS)
	: DS_graph(N, xmFormWidgetClass, parent, arguments, number),
	Siblings(LS) {
	DBG_NOINDENT("	DS_scroll_graph::DS_scroll_graph(\"" << N << "\")...\n"; cout.flush()); }

DS_gc	*DS_object::get_separation_gc() {
	if(GUI_Flag) {
	if(!separation_gc) {
		XGCValues	gcvalue;

		gcvalue.fill_style = FillSolid;
		gcvalue.foreground = ResourceData.grey_color;
		gcvalue.line_width = 1;
		separation_gc = new DS_gc(
				XtDisplay(TopLevelWidget->widget),
				// KEEP_WINDOW
				XtWindow(TopLevelWidget->widget),
				GCForeground | GCLineWidth | GCFillStyle,
				&gcvalue); } }
	return separation_gc; }

DS_gc	*DS_object::get_grey_gc() {
	if(GUI_Flag) {
	if(!grey_gc) {
		XGCValues	gcvalue;

		gcvalue.fill_style = FillSolid;
		gcvalue.foreground = ResourceData.grey_color;
		gcvalue.line_width = LNDRAWWIDTH;
		grey_gc = new DS_gc(
				XtDisplay(TopLevelWidget->widget),
				// KEEP_WINDOW
				XtWindow(TopLevelWidget->widget),
				GCForeground | GCLineWidth | GCFillStyle,
				&gcvalue); } }
	// Do this because composite activities bypass the call to gettilemap, which
	// is where the short_act_gc is defined:
	if(!DS_line::short_act_gc) {
		XGCValues	gcvalue;

		gcvalue.fill_style = FillSolid;
		gcvalue.foreground = ResourceData.black_color;
		gcvalue.line_width = LNDRAWWIDTH;
		DS_line::short_act_gc = new DS_gc(
				XtDisplay(TopLevelWidget->widget),
				// KEEP WINDOW
				XtWindow(TopLevelWidget->widget),
				GCForeground |
					GCLineWidth |
					GCFillStyle,
				&gcvalue); }
	return grey_gc; }

	/*
	 * The following function is invoked by ACT_sys::ACT_sys(), which attaches it to itself as an
	 * event_handler for pointer motion events:
	 */

void DS_lndraw::Motion(Widget, callback_stuff *client_data, void *call_data) {
    if(GUI_Flag) {
	ACT_sys			*theObj = (ACT_sys *) client_data->data;
	XEvent			*E = (XEvent *) call_data;

	if(theObj->TrackingTheLegendBoundary) {
		int	U = E->xmotion.y;
		int	M = theObj->minimum_height_limit, L = theObj->last_sizing_pos;

		// debug
		// cerr << "DS_lndraw::Motion() called (1)\n";
		if(U < M + (ACTSQUISHED - ACTFLATTENED)) {
			if(U < (M + (ACTSQUISHED - ACTFLATTENED) / 2)) {
				U = M; }
			else {
				U = M + (ACTSQUISHED - ACTFLATTENED); } }
		else if(U < M + (ACTVERUNIT - ACTFLATTENED)) {
			if(U < (M + (ACTVERUNIT + ACTSQUISHED) / 2 - ACTFLATTENED)) {
				U = M + (ACTSQUISHED - ACTFLATTENED); }
			else {
				U = M + (ACTVERUNIT - ACTFLATTENED); } }
		if(U != L) {

			if(theObj->Siblings->verticalCursorEnabled) {
				// we really want to ERASE the cursor:
				if(theObj->the_vertical_cursor_is_visible)
					theObj->draw_vertical_cursor(theObj->last_cursor_position);
				theObj->last_cursor_position = -1; }

			XSetFunction(Defdisplay, DS_object::separation_gc->getGC(), GXinvert);
			// PIXMAP
			XDrawLine(Defdisplay, XtWindow(theObj->widget),
				DS_object::get_separation_gc()->getGC(), 0, L, theObj->getwidth(), L);
			L = (theObj->last_sizing_pos = U);
			// PIXMAP
			XDrawLine(Defdisplay, XtWindow(theObj->widget),
				DS_object::separation_gc->getGC(), 0, U, theObj->getwidth(), U);
			// COPY PIXMAP TO WINDOW (0, use_this_height, theObj->getwidth(), use_this_height)
			XSetFunction(Defdisplay, DS_object::separation_gc->getGC(), GXcopy); } }
	else if(theObj->ismvenable()) {
		// This is the only case where we really need to detect out-of-syncness.
		// debug
		// cerr << "DS_lndraw::Motion() called (2)\n";
		udef_intfc::something_happened() += 1;
		// debug
		// cerr << "Calling moving() from Motion(); x " << E->xmotion.x << endl;
		theObj->moving(	E->xmotion.x, E->xmotion.y,
				false,
				theObj->KeyPressedToInitiateDrag,
				theObj->WhichKeyPressedToInitiateDrag); }
	
	else if(theObj->Siblings->verticalCursorEnabled) {
		int	use_this_x_position = E->xmotion.x;

		// debug
		// cerr << "DS_lndraw::Motion() called (3)\n";
		theObj->Siblings->draw_vertical_cursor(use_this_x_position); }
	// debug
	// else {
		// debug
		// cerr << "DS_lndraw::Motion() falling through the cracks\n";
		// }
	theObj->update_pointer_time(E->xmotion.x); } }

void DS_lndraw::update_pointer_time(int xpos) {
	if(GUI_Flag) {
		CTime_base roundedTime = GetSnappedTimeForPosition(xpos);
		
		Cstring current_time_as_string =
			CTime_base(roundedTime).to_string(((ACT_sys *) this)->timezone,
			       /* keep_ms */ true, /* keep_century */ false, /* use_tee */ true);
		Cstring stripped_current_time = (*((ACT_sys *) this)->timezone.epoch) / current_time_as_string;
		*((ACT_sys *)this)->get_AD_parent()->adPointerTimeLabel = stripped_current_time; 
		return; } }

void DS_lndraw::draw_red_vertical_line(int where, int bottom, int top) {
	//set up a gc here
	XGCValues gcvalue;

	gcvalue.line_width = 1;
	gcvalue.fill_style = FillSolid;
	gcvalue.foreground = ResourceData.red_color;

	DS_gc gc(DS_graph::Defdisplay,
		  DefaultRootWindow(DS_graph::Defdisplay),
		  GCForeground | GCLineWidth | GCFillStyle,
		  &gcvalue);
	
	//set color to red?
	XDrawLine(Defdisplay, XtWindow(widget),
		   gc.getGC(), where, bottom, where, top); }


void DS_lndraw::draw_invert_vertical_line(int where, int bottom, int top) {
	if(GUI_Flag) {
		XSetFunction(Defdisplay, DS_object::gethilite()->getGC(), GXinvert);
		XDrawLine(Defdisplay, XtWindow(widget),
			   DS_object::gethilite()->getGC(), where, bottom, where, top);
		XSetFunction(Defdisplay, DS_object::gethilite()->getGC(), GXcopy); } }

void DS_lndraw::draw_vertical_cursor(int where) {
	if(GUI_Flag) {
	if(!Siblings->verticalCursorEnabled)
		return;
	// toggle the visibility flag
	the_vertical_cursor_is_visible = (the_vertical_cursor_is_visible + 1) % 2;

	draw_invert_vertical_line(where, 0, getheight());
			
	last_cursor_position = where; } }

void DS_lndraw::erase_vertical_cursor() {
	if(GUI_Flag) {
	if(!Siblings->verticalCursorEnabled)
		return;
	if(!the_vertical_cursor_is_visible) {
		// cursor was already erased; maybe this happens when the mouse leaves the display?
		return; }
	the_vertical_cursor_is_visible = 0;

	draw_invert_vertical_line(last_cursor_position, 0, getheight()); } }

void
DS_lndraw::Event(const XEvent	*event) {
	context_for_click_event which_case_are_we_talking_about;
	which_case_are_we_talking_about = OTHER;
	DBG_INDENT(get_key() << "->input() START\n");

	// debug
	// cerr << "DS_lndraw::Event() called\n";
	
	// 0. KEY PRESS EVENT

	if(event->type == KeyPress) {
		KeyPressedToInitiateDrag = true;
		XKeyEvent* keyEvent = (XKeyEvent*) event;

		char buffer[20];
		int bufsize = 20;
		XComposeStatus compose;
		int charcount;

		charcount = XLookupString(keyEvent, buffer, bufsize, &WhichKeyPressedToInitiateDrag, &compose);
	  

		// the first item inside the 'if' fixes part of FR 1.1-7:
		if(	ACT_exec::there_are_selections()
			&&
			(getsel() & BUTTON_1_DOWN)
			&&
			(! (getsel() & BUTTON_3_DOWN))
			) {
			DBG_NOINDENT("got a key press event...\n");
			which_case_are_we_talking_about = INITIATE_AN_ACTIVITY_DRAG_ACTION; } }
	// 1. BUTTON PRESS EVENT
	
	else if (event->type == ButtonPress) {
		XButtonEvent* bevent = (XButtonEvent*) event;
		
		unsigned char modifierByte = bevent->state;
		
		if(! (modifierByte & ShiftMask)) {
			DBG_NOINDENT("... no modifier key...\n");
			defkeypressed = 0; }
		// FIRST, FIGURE OUT WHICH CASE WE ARE TALKING ABOUT:
		
		if(bevent->button == Button1) {
			if(	(bevent->time - last_click) < CLICK_INTERVAL
				&&
				ACT_exec::number_of_selections() == 1
				// the next two lines fix FR 1.1-6
				&&
				(! SelectionFrozen)
				// FIXES FR 1.1-26
				&&
				// modified 11/2/98 PFM (BUG)
				// (! modifierByte)
				(! (modifierByte & (ShiftMask | ControlMask)))
				) {
				// for information only (self-documenting code!!):
				DBG_NOINDENT("got second click of a fast pair...\n");
				which_case_are_we_talking_about = SECOND_CLICK_OF_A_FAST_PAIR; }
			else {
				mv_x = bevent->x;
				mv_y = bevent->y;
				actsys_in_which_last_click_occurred = (ACT_sys *) this;
				// we allow users to fake mouse buttons other than button 1
				// through combinations of shift & alt keys:
				if(modifierByte & ControlMask) {
					if(modifierByte & Mod1Mask) {
						DBG_NOINDENT("ALT/CTRL/BTN1; will disambiguate...\n");
						which_case_are_we_talking_about = DISAMBIGUATE_ACTIVITY_STACK; }
					else {	// we are mimicking (1) select w/BTN 1 (2) tap BTN 2 to release activity
						DBG_NOINDENT("CTRL/BTN1; will drag...\n");
						// debug
						// cerr << "Calling selected() from Event() w/x = " << bevent->x << endl;
						selected(0, bevent->x, bevent->y);
						KeyPressedToInitiateDrag = false;
						which_case_are_we_talking_about = INITIATE_AN_ACTIVITY_DRAG_ACTION; } }
				else if(modifierByte & Mod1Mask) {
					DBG_NOINDENT("ALT/BTN1: will reshuffle...\n");
					which_case_are_we_talking_about = RESHUFFLE_ACTIVITY_STACK; }
				else {	// simple selection
					if(modifierByte & ShiftMask) {
						DBG_NOINDENT("SHIFT/BTN1: will multi-select...\n");
						defkeypressed = 1; }
					DBG_NOINDENT("SELECT W FIRST BUTTON...\n");
					which_case_are_we_talking_about = SELECT_W_FIRST_BUTTON; } }
			setsel(BUTTON_1_DOWN); }
		else if(bevent->button == Button2) {
			// the first item inside the 'if' fixes part of FR 1.1-7:
			if(	ACT_exec::there_are_selections()
				&&
				(getsel() & BUTTON_1_DOWN)
				&&
				(! (getsel() & BUTTON_3_DOWN))
				) {
				DBG_NOINDENT("Tapping btn 2; will INITIATE A DRAG ACTION...\n");
				KeyPressedToInitiateDrag = false;
				which_case_are_we_talking_about = INITIATE_AN_ACTIVITY_DRAG_ACTION; }
			else if((! (getsel() & BUTTON_1_DOWN)) && (! (getsel() & BUTTON_3_DOWN))) {
				DBG_NOINDENT("Btn 2 only; will reshuffle the activity stack\n");
				which_case_are_we_talking_about = RESHUFFLE_ACTIVITY_STACK; }
			// DO THIS LAST! In the above logic, we need to know if the PREVIOUS click was on button 1:
			setsel(BUTTON_2_DOWN); }
		
		last_click = bevent->time - 2 * CLICK_INTERVAL;
	}
			/* makes sure we won't mis-interpret a
			 * second click... Note that we will
			 * re-define last_click shortly if we
			 * do have a simple selection. */
	
	// 2. BUTTON RELEASE
	
	else if (event->type == ButtonRelease) {
		XButtonEvent* bevent = (XButtonEvent*) event;

		which_case_are_we_talking_about = OTHER;
		if(bevent->button == Button1) {
			if(TrackingTheLegendBoundary) {
				DBG_NOINDENT("Stop tracking...\n");
				ButtonReleased(bevent);
				TrackingTheLegendBoundary = 0;
			} else {
				DBG_NOINDENT("resetting sel flags from btn 1...\n");
				resetsel(BUTTON_1_DOWN);
			}
		} else if(bevent->button == Button2) {
			DBG_NOINDENT("resetting sel flags from btn 2...\n");
			resetsel(BUTTON_2_DOWN);
		} else if(bevent->button == Button3) {
			DBG_NOINDENT("resetting sel flags from btn 3...\n");
			resetsel(BUTTON_3_DOWN);
		}
		if(bevent->button == Button1) {
			if(ismvenable()) {
				// debug
				// cerr << "calling moving() from Event(); x " << bevent->x << "\n";
				moving(	bevent->x,
					bevent->y,
					true,
					KeyPressedToInitiateDrag,
					WhichKeyPressedToInitiateDrag);
				if(Preferences().GetSetPreference("RemodelAfterDrag", "FALSE") == "TRUE") {
					CTime_base	to_time;
					REMODELrequest*	request = new REMODELrequest(false, to_time, false);
	  
					request->process();
				}
			}
			if (displaymode != NOPMODE) {
				setcursor(NOPMODE, event->xany.window);
			}
			resetmvenable();
		}
	}

	// NOW THAT WE KNOW WHAT WE ARE TALKING ABOUT, LET'S TAKE ACTION:
	//default handling of input here
	if(which_case_are_we_talking_about == SECOND_CLICK_OF_A_FAST_PAIR) {
		DBG_NOINDENT("taking action: case = SECOND_CLICK_OF_A_FAST_PAIR\n");
		UI_mainwindow::editActivityButtonActivateCallback(NULL, NULL, NULL); }
	else if(which_case_are_we_talking_about == INITIATE_AN_ACTIVITY_DRAG_ACTION) {
		DBG_NOINDENT("taking action: case = INITIATE_AN_ACTIVITY_DRAG_ACTION\n");
		setcursor(MOVEMODE, event->xany.window);
		setmvenable(); }
	else if(which_case_are_we_talking_about == SELECT_W_FIRST_BUTTON) {
		XButtonEvent *bevent = (XButtonEvent*) event;
 
		DBG_NOINDENT("taking action: case = SELECT_W_FIRST_BUTTON\n");
		/* Second member of the if was missing; Steve Wissler was wondering why 'paste' actions failed...
		 * we must process activity creation events first, then worry about dragging legend boundary
		 * lines. */
		if(displaymode == DEFMODE || displaymode == PASTEMODE) {
			DBG_NOINDENT("taking action: displaymode = DEFMODE; calling selected(0,x,y)...\n");
			// debug
			// cerr << "Calling selected() from Event() with x = " << bevent->x << endl;
			selected(0, bevent->x, bevent->y); }
		else {
			// code transferred to DS_scroll_graph
			DBG_NOINDENT("taking action: displaymode != DEFMODE; checking for proximity...\n");
			if(!we_are_moving_a_dividing_line(bevent)) {
				DBG_NOINDENT("No proximity; simple selection.\n");
				// debug
				// cerr << "Calling selected() from Event() with x = " << bevent->x << endl;
				selected(0, bevent->x, bevent->y);
				last_click = bevent->time; } } }
	else if(which_case_are_we_talking_about == RESHUFFLE_ACTIVITY_STACK) {
		DBG_NOINDENT("taking action: case = RESHUFFLE_ACTIVITY_STACK\n");
		XButtonEvent* bevent = (XButtonEvent*) event;
		selected(1, bevent->x, bevent->y); }
	else {
		DBG_NOINDENT("taking action: case = OTHER; doing NOTHING\n"); }
	
#	ifdef DEBUG
	if (event->type == MotionNotify)
		printf("Motion\n");
#	endif
	DBG_UNINDENT("input: DONE\n"); }

DS_lndraw::DS_lndraw(const Cstring & N, motif_widget * Parent, list_of_siblings * sib)
	: DS_time_scroll_graph(N, Parent, 1, 1, sib),
	Dbox(NULL),
	mvenable(0),
	InputHandler(NULL),
	actPopupMenu(NULL),
	cursorPopupMenu(NULL),
	MenuPopupSelectable(),
	defkeypressed(0) {
	if(GUI_Flag) {
	XColor		fgcolor;
	XColor		bgcolor;

	// static member!
	TrackingTheLegendBoundary = 0;

			//Create lndraw special cursor
	if(! dscursor) {
		dscursor		= XCreateFontCursor(	Defdisplay,	XC_diamond_cross);
		pastecursor		= XCreateFontCursor(	Defdisplay,	XC_crosshair);
		selcursor		= XCreateFontCursor(	Defdisplay,	XC_X_cursor);
		defcursor		= XCreateFontCursor(	Defdisplay,	XC_question_arrow);
		expansion_cursor	= XCreateFontCursor(	Defdisplay,	XC_sb_v_double_arrow);
		Defcolormap		= XDefaultColormap(	Defdisplay,	Defscreenum);

		if(XParseColor(Defdisplay, Defcolormap, APbcolor, & fgcolor) &&
	    	XParseColor(Defdisplay, Defcolormap, "white", & bgcolor)) {
			XRecolorCursor(Defdisplay,dscursor,&fgcolor,&bgcolor);
			XRecolorCursor(Defdisplay,pastecursor,&fgcolor,&bgcolor);
			XRecolorCursor(Defdisplay,selcursor,&fgcolor,&bgcolor);
			XRecolorCursor(Defdisplay,defcursor,&fgcolor,&bgcolor); } }
	}
}

DS_lndraw::~DS_lndraw() {
	if(DS_object::bk_board) {
		delete DS_object::bk_board;
		DS_object::bk_board = NULL; }
	if(Dbox) delete Dbox; }

Dsource	*theRequestWeChose;

void DS_lndraw::rightButtonClickCB(
	Widget,
	callback_stuff *client_data,
	void *call_data) {
	if(GUI_Flag) {
		DS_lndraw	*Obj = (DS_lndraw *) client_data->data;
		XButtonEvent	*bevent = (XButtonEvent *) call_data;
		DS_line		*aLine, *theTopLine = NULL, *theFirstSelectedLine = NULL;
		int		total_count = 0, selected_count = 0;
		
		// first you need to identify whether there is an activity there:
		for(	aLine = (DS_line *) Obj->lnobjects.last_node();
			aLine;
			aLine = (DS_line *) aLine->previous_node()
		   ) {
			if(aLine->is_selected(bevent->x, bevent->y)) {
				if(aLine->getsource()->is_selected()) {
					if(!selected_count++) {
						theFirstSelectedLine = aLine; } }
			if(!total_count++) {
				theTopLine = aLine; } } }

		Cstring legendName = Obj->GetLegendForPosition(bevent->y);
		CTime_base selectedTime = Obj->GetSnappedTimeForPosition(bevent->x);

		if(total_count > 0) {
			Dsource		*which_activity;
			static void	*theExplanationHandle = NULL;
			
			// Let's decide which activity governs any adaptation-defined menu items:
			if(selected_count) {
				which_activity = (Dsource *) theFirstSelectedLine->getsource(); }
			else {
				which_activity = (Dsource *) theTopLine->getsource(); }
			theRequestWeChose = which_activity;

			if(Obj->actPopupMenu) {
				// If apgen crashes here it's Dennis'fault:
				Obj->actPopupMenu->destroy_widgets();
				delete Obj->actPopupMenu;
				Obj->actPopupMenu = NULL; }
			Obj->createActPopupMenu(bevent->x, bevent->y, which_activity); 
			if(total_count > 1) {
				Obj->actPopupMenu->SetChildButtonSensitive("Disambiguate", true); }
			else {
				Obj->actPopupMenu->SetChildButtonSensitive("Disambiguate", false); }
 
			Obj->ClickLocation[0] = bevent->x;
			Obj->ClickLocation[1] = bevent->y;
			Obj->actPopupMenu->set_menu_position(bevent);
			Obj->actPopupMenu->Push(
				theRequestWeChose,
				bevent->x, bevent->y,
				legendName, selectedTime); }
		else {	
			// handle vertical cursor
			if(Obj->cursorPopupMenu) {
				// If apgen crashes here it's Dennis'fault:
				Obj->cursorPopupMenu->destroy_widgets();
				delete Obj->cursorPopupMenu;
				Obj->cursorPopupMenu = NULL; }

			Obj->createCursorPopupMenu(bevent->x, bevent->y); 

			if(Obj->Siblings->verticalCursorEnabled) {
				Obj->cursorPopupMenu->SetChildButtonSensitive("Vertical Cursor", false);
				Obj->cursorPopupMenu->SetChildButtonSensitive("Regular Cursor", true); }
			else {
				Obj->cursorPopupMenu->SetChildButtonSensitive("Vertical Cursor", true);
				Obj->cursorPopupMenu->SetChildButtonSensitive("Regular Cursor", false); }
			
			Obj->ClickLocation[0] = bevent->x;
			Obj->ClickLocation[1] = bevent->y;
			Obj->cursorPopupMenu->set_menu_position(bevent);
			Obj->cursorPopupMenu->Push(NULL, bevent->x, bevent->y, legendName, selectedTime); } } }

class snapToEventClient: public EventRegistryClient {
public:
	void HandleEvent(const string& type, const TypedValuePtrVect& arguments);
};

static snapToEventClient &theSnapToEventClient() {
	static snapToEventClient sTEC;
	return sTEC; }

static UI_messagewarndialog	*thePopup;
static Cstring			theChosenOne;

void snapToEventClient::HandleEvent(const string& type, const TypedValuePtrVect& arguments) {
	TypedValuePtrVect::const_iterator citer = arguments.begin();

	if(type == "UI_ACTIVITY_SELECTED") {
		TypedValue	*val = *citer;

		UI_messagewarndialog::enable_ok();
		theChosenOne = val->get_string(); }
	else if(type == "UI_ACTIVITY_UNSELECTED") {
		UI_messagewarndialog::disable_ok(); } }

void snap_to_act(const Cstring &which_button) {
	MOVE_ACTIVITYrequest	*action_req;
	Cnode0<alpha_string, ActivityInstance*>* theTag;
	CTime_base		from, to;
	stringslist		list_of_one_id;
	Cstring			existing_legend;
	Dsource*		target = NULL;

	// the 'snap to' activity is the one whose ID is theChosenOne
	// the 'move this' activity is the one pointed to by theRequestWeChose
	if(!(theTag = aaf_intfc::actIDs().find(theChosenOne))) {
		cerr << "snap_to_act: error -- selected activity has a bad tag!?\n";
		return; }
	target = theTag->payload;
	list_of_one_id << new emptySymbol(theRequestWeChose->get_unique_id());
	if(	which_button == "Move start of activity to end of selection") {
		from = theRequestWeChose->getetime();
		to = target->getetime() + target->get_timespan(); }
	else if(which_button == "Move start of activity to start of selection") {
		from = theRequestWeChose->getetime();
		to = target->getetime(); }
	else if(which_button == "Move end of activity to end of selection") {
		from = theRequestWeChose->getetime() + theRequestWeChose->get_timespan();
		to = target->getetime() + target->get_timespan(); }
	else if(which_button == "Move end of activity to start of selection") {
		from = theRequestWeChose->getetime() + theRequestWeChose->get_timespan();
		to = target->getetime(); }
	existing_legend = theRequestWeChose->get_legend()->get_key();
	// debug
	// cerr << "MOVE by " << (to - from).to_string() << " from snap_to_act()\n";
	action_req = new MOVE_ACTIVITYrequest(list_of_one_id, CTime_base(), to - from, existing_legend);
	action_req->process(); }

void cancel_snap(void){
	EventRegistry::Register().UnSubscribe("UI_ACTIVITY_SELECTED", "theSnapToClient"); }

void
DS_lndraw::MenuPopupSelected(
		Extensible_popup_menu	*menu,
		const Dsource		*act,
		int			x,
		int			y,
		const Cstring		&buttonName,
		const Cstring		&legendName,
		const CTime_base		&time) {
	if(menu == cursorPopupMenu) {
		if(buttonName == "Vertical Cursor") {
			Siblings->verticalCursorEnabled = 1;
			last_cursor_position = -1;
		} else if(buttonName == "Regular Cursor") {
			Siblings->erase_vertical_cursor();
			Siblings->verticalCursorEnabled = 0;
		} else {
			UI_GLOBALrequest* request = new UI_GLOBALrequest(
					*buttonName, *legendName, x, y, time);
			request->process();
			delete request;
		}
	} else if(menu == actPopupMenu) {
		if(buttonName == "Disambiguate") {
			disambiguate_acts_at(ClickLocation[0], ClickLocation[1]);
		}
		else if(buttonName == "Center") {
			//
			CTime_base start = act->getetime();

			//
			CTime_base dur = act->get_timespan();

			//create NEW_HORIZONrequest and process it
				                Cstring TAS = "A";
			TAS = TAS + ("ACT_system" / get_key());

			// at Steve's request
			NEW_HORIZONrequest horizReq(TAS, start, start + dur);

			horizReq.process();
		} else if(buttonName == "Snap To...")  {
			slist<alpha_void, smart_actptr>			selection_list;
			slist<alpha_void, smart_actptr>::iterator	li(selection_list);
			smart_actptr*					pnode;
			List						button_list;

			eval_intfc::get_all_selections(selection_list);
			while((pnode = li())) {
				pnode->BP->unselect(); }
			EventRegistry::Register().Subscribe(
					"UI_ACTIVITY_SELECTED",
					"theSnapToClient",
					&theSnapToEventClient());
			button_list << new String_node("Move start of activity to end of selection");
			button_list << new String_node("Move start of activity to start of selection");
			button_list << new String_node("Move end of activity to end of selection");
			button_list << new String_node("Move end of activity to start of selection");
			UI_messagewarndialog::initialize(
				"Snap To",
				"Move To Start/End of Activity",
				"Select the activity you want to "
				"snap to; use the buttons below to "
				"indicate whether you want to move "
				"to the start or end of that activity, "
				"then click OK, Apply or Cancel.",
				button_list,
				snap_to_act,
				true,
				cancel_snap);
			UI_messagewarndialog::disable_ok();
		} else if(buttonName == "Edit") {
#ifdef GTK_EDITOR
			Cstring			errors;
			slist<alpha_void, smart_actptr>			selection_list;
			slist<alpha_void, smart_actptr>::iterator	li(selection_list);
			smart_actptr*					pnode;

			eval_intfc::get_all_selections(selection_list);
			while((pnode = li())) {
				pnode->BP->unselect();
			}
			theRequestWeChose->select();
			if(eS::fire_up_gtk_editor(errors) != apgen::RETURN_STATUS::SUCCESS) {
				errors::Post("Editor Error", errors);
			}
#endif /* GTK_EDITOR */
		} else {

			UI_ACTIVITYrequest *request = new UI_ACTIVITYrequest(*buttonName, *(act->get_unique_id()));
			request->process();
			delete request;
		}
	}
}

void DS_lndraw::createCursorPopupMenu(int X, int Y) {
	if(GUI_Flag) {
		// emulate RES_sys::popup_rd_optionsCB():
		cursorPopupMenu = new Extensible_popup_menu("cursor_popup_menu", this);
		
		cursorPopupMenu->AddCallbackButton("Vertical Cursor", this);
		cursorPopupMenu->AddCallbackButton("Regular Cursor", this);

		StringVect names;
		EventRegistry::Register().GetRegisteredNamesForType("UI_GLOBAL_CALLBACK", &names);

		StringVect::iterator iter = names.begin();
		
		while(iter != names.end()) {
			cursorPopupMenu->AddCallbackButton(*iter, this);
			iter++;
		}
	}
}

#ifdef DEBUG_STRANGE_EVENTS
void actPopupMenuCallback2(Widget, callback_stuff * client_data, void *) {
	motif_widget		*origin = client_data->parent;
	ACT_sys			*Obj = (ACT_sys *) client_data->data;

	// debug
	cerr << "Case disarm\n"; }

void actPopupMenuCallback1(Widget, callback_stuff * client_data, void *) {
	motif_widget		*origin = client_data->parent;
	ACT_sys			*Obj = (ACT_sys *) client_data->data;

	// debug
	cerr << "Case arm\n"; }
#endif /* DEBUG_STRANGE_EVENTS */

void DS_lndraw::createActPopupMenu(int X, int Y, Dsource *theRequest) {
		slist<alpha_void, smart_actptr>	selection_list;

		// emulate RES_sys::popup_rd_optionsCB():
		actPopupMenu = new act_popup_menu("act_popup_menu", this);
		actPopupMenu->AddCallbackButton("Disambiguate", this);
		actPopupMenu->AddCallbackButton("Center", this);
		eval_intfc::get_all_selections(selection_list);
		if(selection_list.get_length() == 1) {
			actPopupMenu->AddCallbackButton("Snap To...", this); }
#ifdef GTK_EDITOR
		actPopupMenu->AddCallbackButton("Edit", this);
#endif /* GTK_EDITOR */



		StringVect names;
		EventRegistry::Register().GetRegisteredNamesForType("UI_ACTIVITY_CALLBACK", &names);

		StringVect::iterator iter = names.begin();
		
		while(iter != names.end()) {
			actPopupMenu->AddCallbackButton(*iter, this);
			iter++; } }

Extensible_popup_menu::Extensible_popup_menu(const Cstring& name, motif_widget* parent)
	: motif_widget(name, xmPopupMenuWidgetClass, parent, NULL, 0, FALSE),
		Buttons(),
		Activity(NULL),
		X(0),
		Y(0) {}

int
Extensible_popup_menu::AddCallbackButton(const Cstring& name, MenuPopupSelectable* push) {
	motif_widget* newWid = new motif_widget(name, xmPushButtonWidgetClass, this);
	newWid->add_callback(Callback, XmNactivateCallback, this);

	WidgetPush widgetPush(newWid, push);
	WidgetPushMap::iterator iter = Buttons.find(name);
	if(iter != Buttons.end())
		return 0;
	WidgetPushMap::value_type val(name, widgetPush);
	Buttons.insert(val);
	return 1; }  


int
Extensible_popup_menu::SetChildButtonSensitive(const Cstring& name, int value) {
	WidgetPushMap::iterator iter = Buttons.find(name);

	if(iter != Buttons.end())
		iter->second.GetWidget()->set_sensitive(value);
	return 1; }
 

int
Extensible_popup_menu::Push(	const Dsource	*act,
				int x, int y,
				const Cstring& legendName,
				const CTime_base& time) {
	Activity = act;
	X = x;
	Y = y;
	Legend = legendName;
	Time = time;
	manage();
	return 1; }

void
Extensible_popup_menu::Callback(Widget w, callback_stuff* cb, void* data) {
	Extensible_popup_menu* menu = (Extensible_popup_menu*) cb->data;

	menu->NonStaticCallback(w, cb); }

void
Extensible_popup_menu::NonStaticCallback(Widget w, callback_stuff *cb) {
	motif_widget* offending_widget = cb->parent;

	Cstring name = offending_widget->get_text();
	WidgetPushMap::iterator iter = Buttons.find(name);
	if(iter == Buttons.end())
		return;
	iter->second.GetPush()->MenuPopupSelected(this, Activity, X, Y, offending_widget->get_text(), Legend, Time);
	unmanage(); }

	/* ifdef GUI -- way way up, before void DS_lndraw::rightButtonClickCB() */

	/* General idea for DS_lndraw::update_busy_line() below:
	 *
	 * 	A high-level object sets the global UpdateBusyLine (in IO_seqtalk.C),
	 * 	then asks each activity instance to 'draw' itself. The draw() method
	 * 	in UI_ds_draw.C does not draw anything; it just calls update_busy_line
	 * 	so that at the end of the process, each legends has the information it
	 * 	needs to "stagger" the activities as needed.
	 *
	 * 	The "staggering information" is stored in the array stagger_bits inside
	 * 	"bptr_to_legend" objects, which are held in the pointersToTheVisibleLegends
	 * 	list for each DS_lndraw. Note that the size of the stagger_bits array is
	 * 	equal to the width of the screen; this should be re-engineered if we want
	 * 	the same code to apply to very long PostScript timeline plots.
	 *
	 * 	What is stored in the stagger_bits array is an integer n that contains
	 * 	the number of overlapping instances at each pixel along the given legend.
	 * 	This helps the layout routine later on; it will be told that "from pixel
	 * 	A to pixel B, there are N overlapping activities to be accommodated within
	 * 	the available legend height.
	 *
	 * 	Note that the computation also keeps track of the maximum value of N over
	 * 	all legends (not immediately obvious how to use this). That value is stored
	 * 	in theMaximumStaggeringOffset.
	 *
	 *
	 *
	 * Arguments:
	 *		- Dsource is (usually) an Dsource to be drawn on a DS_lndraw/ACT_sys
	 *		- sX is the location of the (horizontal) start of the line in pixels
	 *		- eX is the location of the (horizontal) end of the line in pixels
	 *		- label_eX is the location of the (horizontal) end of the label in pixels
	 *		- v is the vertical location of each instance, measured from... I don't know!
	 *		  (see the meaning of DS_line::y_pos()) ... Hey, unused parameter!!
	 *
	 *	(All units in pixels)
	 */
int DS_lndraw::update_busy_line(Dsource *obj, int sX, int eX, int label_eX) {
	int		Xtotal = getwidth();
	int		n = 0;
	int		i;
	bpointernode	*leg_ptr;
	Lego		*LO;
	int		highest_n = 0;
	int		*bits;

	if(eX < 0 || sX > Xtotal) return n;
	if(!isVisible()) return n;
	if(!obj) return n;
	leg_ptr = (bpointernode *) legend_display->pointersToTheVisibleLegends.find(
			(void *) (LO = (Lego *) obj->get_legend())->vthing);
	if(!leg_ptr) {
		DBG_NOINDENT(get_key() << "->update_busy_line: legend \"" << LO->get_key()
			<< "\" @ " << ((void *) LO)
			<< " not found in pointersToTheVisibleLegends; returning.\n");
		return n; }
	bits = ((Lego *) obj->get_legend())->get_stagger_info_for(legend_display);


	DBG_NOINDENT("update_busy_line: updating legend \"" << LO->get_key() << "\" from "
		<< sX << " to " << eX << "; Xtotal = " << Xtotal << "...\n");

	if(sX < 0) sX = 0;
	if(eX > Xtotal) eX = Xtotal;
	if(label_eX < eX) label_eX = eX;
	if(label_eX > Xtotal) label_eX = Xtotal;
	for(i = sX; i < label_eX; i++) {
		n = bits[i];
		if(n > highest_n) highest_n = n; }
	highest_n++;
	for(i = sX; i < label_eX; i++) {
		bits[i] = highest_n; }
	DBG_NOINDENT("highest_n: " << highest_n << endl);
	highest_n--;
	if(highest_n > theMaximumStaggeringOffset)
		theMaximumStaggeringOffset = highest_n;
	return highest_n; }

void DS_lndraw::update_bits() {
	List_iterator		someOfTheLegends(legend_display->pointersToTheVisibleLegends);
	List_iterator		allLegends(legend_display->get_blist_of_vertically_scrolled_items());
	bpointernode		*bleg;
	Lego			*LO;
				int             	i, j, Xtotal, number_of_visible_legends = 0, top_pixels = 0;
	int			visible_pixels;
	int			start_counting_legends = 0;
	int			*bits;

	DBG_INDENT(get_key() << "->update_bits(): clearing pointersToTheVisibleLegends\n");

	// Can get memory leak if we limit loop to visible legends:
	while((LO = (Lego *) allLegends())) {
		bits = LO->get_stagger_info_for(legend_display);
		if(bits) {
			free((char *) bits); }
		LO->set_stagger_info_for(legend_display, NULL); }

	legend_display->pointersToTheVisibleLegends.clear();
				Xtotal = getwidth();
	while((LO = (Lego *) allLegends())) {
		top_pixels += LO->get_vertical_size_in_pixels_in(legend_display);
		if(start_counting_legends) {
			number_of_visible_legends++;
			visible_pixels += LO->get_vertical_size_in_pixels_in(legend_display); }
		else if(
			// display bug
			// top_pixels >= legend_display->y_offset_in_pixels()
			top_pixels > legend_display->y_offset_in_pixels()
			) {
			start_counting_legends = 1;
			number_of_visible_legends = 1;
			visible_pixels = top_pixels - legend_display->y_offset_in_pixels(); }
		if(start_counting_legends) {
			legend_display->pointersToTheVisibleLegends << (bleg = new bpointernode((void *) LO->vthing, LO->vthing));
			DBG_NOINDENT("update_bits(): adding pointer -> " << ((void *) LO->vthing)
				<< ", legend \"" <<
				LO->get_key() << "\"; pointersToTheVisibleLegends size = "
				<< legend_display->pointersToTheVisibleLegends.get_length() << endl);
			bits = (int *) malloc(sizeof(int) * Xtotal);
			LO->set_stagger_info_for(legend_display, bits);
				        	for(j = 0; j < Xtotal; j++) {
				bits[j] = 0; }
			if(visible_pixels >= getheight())
				break; } }
	DBG_UNINDENT("update_bits() DONE\n"); }

#ifdef GUI
void DS_time_scroll_graph::setcursor(unsigned char mode, Window theWin) {
	if(GUI_Flag) {
		displaymode = mode;
		setcursorappearance(mode, theWin); } }
	
void DS_time_scroll_graph::setcursorappearance(unsigned char mode, Window theWin) {
	if(GUI_Flag){
		switch(mode) {
		case MOVEMODE:
			XDefineCursor(Defdisplay, theWin, dscursor);
			break;
		case NOPMODE:
			XUndefineCursor(Defdisplay, theWin);
			break;
		case PASTEMODE:
			XDefineCursor(Defdisplay, theWin, pastecursor);
			break;
		case SELMODE:
			XDefineCursor(Defdisplay, theWin, selcursor);
			break;
		case EXPANSIONMODE:
			XDefineCursor(Defdisplay, theWin, expansion_cursor);
			break;
		case DEFMODE:
			XDefineCursor(Defdisplay, theWin, defcursor);
			break; }
	}
}
#endif /* ifdef GUI */

// This method is used to create the display line object from a data source:
DS_gc	*DS_lndraw::gettilemap(int patternid, int colorid) {
	if(GUI_Flag) {
	XGCValues	gcvalue;
	DS_gc		* lngc;
			// range: 15 + 16 * 63 = 1023
	int		combined = (colorid - 1) + 16 * patternid;

	if(! isVisible()) return NULL;
	if(! DS_object::bk_board)
		DS_object::bk_board = new DS_pixmap_board(	XtParent(widget),
								AP_Bitmap_pool,
								NUM_OF_PATTERNS);

	//Set bk_board with the specified id's
	// List_iterator	gcsearch(lngclst);
	if(!GC_array) {
		int	i;

		GC_array = (DS_gc **) malloc(sizeof(DS_gc *) * 1024);
		for(i = 0; i < 1024; i++) {
			GC_array[i] = NULL; } }
	DS_object::bk_board->selcolor(colorid);
	DS_object::bk_board->selpattern(patternid);
	lngc = GC_array[combined];

	if (!lngc) {
		tilemap = XCreatePixmap(XtDisplay(widget),
			DefaultRootWindow(XtDisplay(widget)), 	/*Xlib.h deficiency causes aCC warning*/
			DS_object::bk_board->getmaxwidth(),
			DS_object::bk_board->getmaxheight(),
			XDefaultDepth(XtDisplay(widget), XDefaultScreen(XtDisplay(widget))));
		gcvalue.background = WhitePixel(/*Xlib.h deficiency causes aCC warning*/
				XtDisplay(widget),
				XDefaultScreen(XtDisplay(widget)));
		gcvalue.foreground = WhitePixel(/*Xlib.h deficiency causes aCC warning*/
				XtDisplay(widget),
				XDefaultScreen(XtDisplay(widget)));
		{
		DS_gc	* Normal_GC = new DS_gc(XtDisplay(widget),tilemap,(GCForeground|
					   GCBackground),&gcvalue);

		XFillRectangle(XtDisplay(widget), tilemap, Normal_GC->getGC(),
			0, 0,
			DS_object::bk_board->getmaxwidth() + 1,  DS_object::bk_board->getmaxheight() + 1);

		delete(Normal_GC);
		}

		XCopyPlane(XtDisplay(widget),  DS_object::bk_board->getselpattern(), tilemap,
			  (DS_object::bk_board->gethilite())->getGC(),
			  0, 0,
			  DS_object::bk_board->getmaxwidth(), DS_object::bk_board->getmaxheight(),0,0,1);
		//Set up the line drawing GC
		gcvalue.line_width = LNDRAWWIDTH;
		gcvalue.line_style = LineSolid;
		gcvalue.fill_style = FillTiled;
		gcvalue.tile = tilemap;
		lngc= new DS_gc(XtDisplay(widget),
				// KEEP WINDOW
				XtWindow(widget),
				GCLineWidth | GCLineStyle | GCFillStyle | GCTile,
				& gcvalue);
		lngc->id = patternid;
		lngc->cid = colorid;
		// lngclst << lngc;
		GC_array[combined] = lngc; }

	if(!DS_line::short_act_gc) {
		gcvalue.fill_style = FillSolid;
		// gcvalue.foreground = ResourceData.grey_color;
		gcvalue.foreground = ResourceData.black_color;
		gcvalue.line_width = LNDRAWWIDTH;
		DS_line::short_act_gc = new DS_gc(
				XtDisplay(widget),
				// KEEP WINDOW
				XtWindow(widget),
				GCForeground |
					GCLineWidth |
					GCFillStyle,
				&gcvalue); }
	if(!DS_line::busy_gc) {
		gcvalue.fill_style = FillSolid;
		gcvalue.foreground = ResourceData.grey_color;
		gcvalue.line_width = 3;
		DS_line::busy_gc = new DS_gc(
				XtDisplay(widget),
				// KEEP WINDOW
				XtWindow(widget),
				GCForeground |
					GCLineWidth |
					GCFillStyle,
				&gcvalue); }

	return lngc; }
	else {
		return 0; } }

void DS_lndraw::send_to_back(int x, int y) {
	if(GUI_Flag) {
	slist<alpha_void, smart_actptr>	selection_list;
	smart_actptr*			pnode;
	Dsource*			req;
	DS_line		*a_line, *old_node_in_front = NULL, *new_node_in_front = NULL;

	if(SelectionFrozen) return;
	eval_intfc::get_all_selections(selection_list);
	pnode = selection_list.last_node();
	if(pnode) {
		req = pnode->BP;
		old_node_in_front = ((derivedDataDS *) req->dataForDerivedClasses)->get_representative_in(this); }

	if(old_node_in_front) {

		// NEED update technique here; solution: select() and unselect()
		// create 'windows' (win_nodes) used by the WorkProc to clean things
		// up as soon as there is some time available after servicing Motif
		// events.

		old_node_in_front->getsource()->unselect();
		lnobjects.remove_node(old_node_in_front);
		lnobjects.insert_first_node_after_second(old_node_in_front, NULL);
		// UI_subsystem->decrementNumActSelected();
		// (traverse the list in reverse order so it agrees with what the user sees)
		for(	a_line = (DS_line *) lnobjects.first_node();
			a_line;
			a_line = (DS_line *) a_line->next_node()
		  ) {
			if(a_line->is_selected(x, y)) {
				new_node_in_front = a_line;
				// redraw the whole stack in order:
				a_line->draw(); } }
		if(new_node_in_front && new_node_in_front != old_node_in_front)
			handle_one_selected_DS_line(new_node_in_front, x, y); } } }


// PFM -- the following comment is misleading: button 1 selects an object
//						      =

//This method is invoked through the SELECT BUTTON- Button 2
void DS_lndraw::selected(int button2_flag, int x, int y) {
    if(GUI_Flag) {
	DS_line		*lnode;
	DS_line		*a_line;
	int		count = 0;

	// debug
	// cerr << "selected: x = " << x << endl;
	if(button2_flag) {
		for(	a_line = (DS_line *) lnobjects.last_node();
			a_line;
			a_line = (DS_line *) a_line->previous_node()
		  )
				/* The name 'is_selected' is horribly misleading. What the method
				 * really does is return 1 if! the point at (x,y) is within the
				 * boundaries of the DS_line shape. */
				if(a_line->is_selected(x, y))
					count++;
		if(count < 1) {
			return; } }
	if(displaymode == PASTEMODE) {
		paste_at(x,y); }
	else if(displaymode == DEFMODE) {
		define_new_activity_starting_at(x, y); }
	else if(! SelectionFrozen) {
		bool	notification_flag = ACT_exec::there_are_selections();

		lnode = NULL;
		// First, deselect all the selected objects
		// when shift key is not pressed
		if(!getkeypressed()) {
			slist<alpha_void, smart_actptr>			selection_list;
			smart_actptr*					pnode;
			slist<alpha_void, smart_actptr>::iterator	li(selection_list);

			eval_intfc::get_all_selections(selection_list);
			while((pnode = li())) {
				pnode->BP->unselect(); } }
		// Next, see if more than one DS_line exists at the location at which the mouse
		// click occurred. We traverse the list in reverse order so it agrees with what the user sees.
		for(	a_line = (DS_line *) lnobjects.last_node();
			a_line;
			a_line = (DS_line *) a_line->previous_node()
		  )
			/* The name 'is_selected' is horribly misleading. What the method
			 * really does is return 1 if! the point at (x,y) is within the
			 * boundaries of the DS_line shape. */
			if(a_line->is_selected(x, y)) {
				lnode = a_line;
				break; }
		if(lnode) {
			handle_one_selected_DS_line(lnode, x, y); }
		else if(notification_flag) {
			TypedValuePtrVect	arguments;
			// empty argument list: we don't really know who was unselected...
			EventRegistry::Register().PropagateEvent("UI_ACTIVITY_UNSELECTED", arguments); } }
	if(button2_flag)
		send_to_back(x, y); } }

void DS_lndraw::disambiguate_acts_at(int x, int y) {
	DS_line * a_line, * lnode;
	int	count = 0;

	if(SelectionFrozen) return;

	// See if more than one activity exists at this point:
	// (traverse the list in reverse order so it agrees with what the user sees)
	for(	a_line = (DS_line *) lnobjects.last_node();
		a_line;
		a_line = (DS_line *) a_line->previous_node()
	  )
			if(a_line->is_selected(x, y))
				if(! count++)
					lnode = a_line;
	// Yep:
	if(count > 1) {
		if(!Dbox)
			Dbox = new disambiguation_box("DisambiguationBox", MW, this);
		Dbox->disambiguation_list->clear();
		Dbox->ListOfPointersToLineObjects.clear();
		// (traverse the list in reverse order so it agrees with what the user sees)
		for(	a_line = (DS_line *) lnobjects.last_node();
			a_line;
			a_line = (DS_line *) a_line->previous_node()
		  )
			if (a_line->is_selected(x, y)) {
				// EVERY line gets an entry in the list:
				*Dbox->disambiguation_list << a_line->get_linename();
				Dbox->ListOfPointersToLineObjects << new Pointer_node(a_line, a_line); }
		Dbox->save_x = x;
		Dbox->save_y = y;
		Dbox->disambiguation_pane->manage(); } }

void	DS_lndraw::handle_one_selected_DS_line(DS_line *lnode, int, int) {
#ifdef GUI
	if(GUI_Flag) {
	TypedValuePtrVect	arguments;
	TypedValue		val;

	// lnode points to the one and only DS_line
	// THE NEW ACTIVITY WAS ALREADY SELECTED
	if(lnode->getselection() && getkeypressed()) {
		// NEED update technique here. Solution: select() and unselect()
		// create 'windows' (win_nodes) used by the WorkProc to clean things
		// up as soon as there is some time available after servicing Motif
		// events.
		lnode->getsource()->unselect();
		val = lnode->getsource()->get_unique_id();
		arguments.push_back(&val);
		// Core dumps if no client has subscribed:
		// EventRegistry::Register().Event("UI_ACTIVITY_UNSELECTED", "UnselectionSignal", arguments);
		EventRegistry::Register().PropagateEvent("UI_ACTIVITY_UNSELECTED", arguments);
		resetmvenable(); }
	else {
		if (!lnode->getselection()) {
			lnobjects.remove_node(lnode);
			// appends; this makes sure the selected act. will be displayed on top:
			lnobjects << lnode;
			// NEED update technique here. Solution: select() and unselect()
			// create 'windows' (win_nodes) used by the WorkProc to clean things
			// up as soon as there is some time available after servicing Motif
			// events.
			if(is_in_hierarchy_select_mode()) {
				lnode->getsource()->select_hierarchy(/* down_only */ false); }
			if(is_in_down_select_mode()) {
				lnode->getsource()->select_hierarchy(/* down_only */ true); }
			else {
				lnode->getsource()->select(); } }
		// else, lnode was already selected... in any event, we want to publish the fact that
		// the user has selected something:
		val = ((Dsource *) lnode->getsource())->get_unique_id();
		arguments.push_back(&val);
		// Core dumps if no client has subscribed:
		// EventRegistry::Register().Event("UI_ACTIVITY_SELECTED", "SelectionSignal", arguments);
		EventRegistry::Register().PropagateEvent("UI_ACTIVITY_SELECTED", arguments);
		}
	}
#endif
}

void DS_lndraw::moving(int x, int y, bool snap, bool char_pressed, KeySym key) {
	smart_actptr*					selnode;
	slist<alpha_void, smart_actptr>			selection_list;
	slist<alpha_void, smart_actptr>::iterator	selst(selection_list);
	slist<alpha_void, dumb_actptr>			selection_move;
				/* The following variables are indices associated with individual legends.
				In the "old days", this was simply proportional to the vertical distance
				between a given (selected) activity and the top of the activity display.
				In the "new world" of variable-height legends, this proportionality is lost.
				However, it is still OK to work with indices. In particular, the algorithm
				for dragging a swarm of activities that spreads over multiple legends remains
				the same: all activities are moved up or down by the same number of legends,
				regardless of the actual width of each legend. If the target legend is too
				high/too low, the target is adjusted to whatever the highest/lowest available
				legend is. */
	Lego		*theNewLegend, *theOldLegend, *theReferenceLegend;
	int		new_legend_index, ref_legend_index;
	int		delta_button;
	Cstring		move_legend;
	CTime_base	move_duration;
	smart_actptr*	ptr;
	Dsource*	act_request = NULL;
	MOVE_ACTIVITYrequest *request = NULL;

	if (!ismvenable())
		return;
	eval_intfc::get_all_selections(selection_list);
	ptr = selection_list.first_node();
	// fixes problem of empty lists in 'move' requests:
	if(!ptr)	// the list of selections is empty
		return;
	act_request = ptr->BP;

	DBG_INDENT("DS_lndraw(\"" << get_key() << "\")::moving(x = " << x << ", y = "
			<< y << "): START" << endl);

	//  x,y is the current pixel location of the cursor, relative to the
	//  origin of the DS_lndraw in which the "move cursor" event originated.
	if ((x > 0) && (x < getwidth()) && (y > 0) && (y < getheight())) {
		//Figure out the Legend index, and use it to get actual Legend
		//  text, for the action request:

		// CR 1 REQUIREMENT: WE NEED TO KNOW in which "EXTENDED LEGEND" the button click
		// took place.

		// button_inx = y / vunitsize + Siblings->y_offset();
		theNewLegend = (Lego *) legend_display->findTheVSIat(y, NULL);
		theOldLegend = (Lego *) legend_display->findTheVSIat(mv_y, NULL);

		// CR 1 REQ: instead of just tracking the vertical position of the last button
		// press (which is that mv_y is), we could keep track of which extended legend we're on:
		// old_button_inx = mv_y / vunitsize + legend_display->y_offset();
		// if(old_button_inx >= get_blist_of_vertically_scrolled_items().get_length()) //below Legends
		// 	old_button_inx = get_blist_of_vertically_scrolled_items().get_length() - 1;

		// CR 1 REQ: a simple delta no longer captures what we want to do... or maybe it does,
		// but that's a clumsy way of encoding which legend we want to move TO:

		if(! (theNewLegend || theOldLegend)) {
			DBG_UNINDENT("Can't find one of the legends; y = " << y
					<< ", mv_y = " << mv_y << endl);
			// nothing we can do
			return; }
		// delta_button = button_inx - old_button_inx;
		delta_button = theNewLegend->get_index() - theOldLegend->get_index();

		//can move cursor so y is above legends (<0),but IF above saves

		// NO: we will figure out the new legend of the first act. IN THE LIST
		// (not the earliest activity, which could be ambiguous)
		// move_legend = *get_blist_of_vertically_scrolled_items()[button_inx]->get_key();
		// ... remember that act_request is the first activity in the list of selections...
		ref_legend_index = act_request->getTheLegendIndex();
		new_legend_index = ref_legend_index + delta_button;
		DBG_NOINDENT("red legend index = " << ref_legend_index <<
				", delta = " << delta_button << endl);
		if(new_legend_index >= legend_display->get_blist_of_vertically_scrolled_items().get_length())
			new_legend_index = legend_display->get_blist_of_vertically_scrolled_items().get_length()
						- 1;
		if(new_legend_index < 0) new_legend_index = 0;
		move_legend = legend_display->get_blist_of_vertically_scrolled_items()[
					new_legend_index ]->get_key();

		//Compute Duration moved (may be negative) for action request:
		
		if(snap) {
			CTime_base exactPrevTime = act_request->getetime();
			CTime_base roundedTime = GetSnappedTimeForTime(exactPrevTime);
			move_duration = roundedTime - exactPrevTime; }
		else {
			// debug
			// cerr << "moving: x " << x << ", mv_x " << mv_x << endl;
			move_duration = CTime_base::convert_from_double_use_with_caution(
				(x - mv_x) * timeperhpixel(), true); }

		// Get list of IDs to move, from the Selection list. Be careful because the
		// giant window allows multiple intra-hierarchy selection.
		tlist<alpha_void, dumb_actptr>		activities_with_a_selected_ancestor;
		ActivityInstance*			actreqnode;

		while((selnode = selst())) {
			actreqnode = selnode->BP;
			actreqnode->recursively_get_ptrs_to_all_descendants(
					activities_with_a_selected_ancestor);
		}
		while((selnode = selst())) {
			actreqnode = selnode->BP;
			if(!activities_with_a_selected_ancestor.find(selnode->BP)) {
				selection_move << new dumb_actptr(actreqnode);
			}
		}

		//Call the action request:
		// debug
		// cerr << "MOVE by " << move_duration.to_string() << " from DS_lndraw::moving\n";
		request = new MOVE_ACTIVITYrequest(
				selection_move,
				CTime_base(0.0) /* unused; flag by setting to zero */,
				move_duration, move_legend);
		request->process();

		//Reset stored horizontal-pixel and vertical-pixel locations.
		//  OK to do after action request, because action request (or
		//  anything it calls) doesn't set mv_x or mv_y.
		mv_x = x;
		mv_y = y; }
	DBG_UNINDENT("DS_lndraw::moving END" << endl);
}

void DS_lndraw::paste_at(int x, int y) {
    if(GUI_Flag) {
	CTime_base		newstartime;
	int			legend_index;
	Lego*			ldef;
	Cstring			genlabel(GENERICLEGEND), time_as_string, errmsg;
	static stringslist	empty_list;
	PASTE_ACTIVITYrequest*	request;
	vertically_scrolled_item* vsi;
	extern VerticalThing* lego2vsi(Node *N);

	newstartime = GetSnappedTimeForPosition(x);
	ACT_sys::exit_pastemode();

	if((vsi = legend_display->findTheVSIat(y, NULL))) {
		DBG_NOINDENT(get_key() << "->paste_at(): found legend \"" << vsi->get_key() << "\".\n");
		legend_index = ((VerticalThing *) vsi)->get_lego()->get_index(); }
	else {
		DBG_NOINDENT(get_key() << "->paste_at(): no legend found where user clicked; "
			<< "will use generic legend \"" << genlabel << "\".\n");
		if(!(vsi = (vertically_scrolled_item *) legend_display->get_blist_of_vertically_scrolled_items().find(genlabel))) {
			ldef = (Lego *) LegendObject::LegendObjectFactory(genlabel, "NEW", ACTVERUNIT);
			vsi = lego2vsi(ldef); }
		legend_index = ldef->get_index(); }
	time_as_string = newstartime.to_string();

	/* Notes:    1. this PASTE_ACTIVITY request ALWAYS has an empty list of ID pointers.
	*		The only other place in the code that generates a PASTE_ACTIVITY request is
	*		the command generation code, in which the list is ALWAYS used.
	*
	*	     2. For a proper paste-into-the-hopper implementation, we need to tell the
	*	        PASTE ACTIVITY request that the activities in the clipboard should now
	*	        acquire a status of "unscheduled", consistent with being put in the hopper,
	*	        when the clicking is detected in a hopper-type DS_lndraw object.
	*/
	if(this == ACT_sys::theHopper) {
		// The action request will do this now.
		// 	ACT_exec::set_clipboard_scheduled(0);
		request = new PASTE_ACTIVITYrequest(
			empty_list,
			time_as_string,
			Cstring("0Hopper:") + legend_display->get_blist_of_vertically_scrolled_items()[legend_index]->get_key()); }
	else {
		// The action request will do this now.
		// 	ACT_exec::set_clipboard_scheduled(1);
		request = new PASTE_ACTIVITYrequest(
			empty_list,
			time_as_string,
			legend_display->get_blist_of_vertically_scrolled_items()[legend_index]->get_key()); }
	request->process(); } }

int
DS_lndraw::GetPositionForTime(const CTime_base& time) {
	CTime_base total = Siblings->dwtimestart() + Siblings->dwtimespan();

	if(time < Siblings->dwtimestart())
		return -1;
	if(time > (Siblings->dwtimestart() + Siblings->dwtimespan()))
		return -1;

	CTime_base durFromStart = time - Siblings->dwtimestart();
	CTime_base durPerPixel = Siblings->dwtimespan() / getwidth();

	return (int) (durFromStart / durPerPixel); }

CTime_base
DS_lndraw::GetTimeForPosition(int x) {
	// debug
	// cerr << "GetTimeForPosition(" << x << "): dwtimestart() = " << Siblings->dwtimestart().to_string()
	// 	<< endl;
	return CTime_base::convert_from_double_use_with_caution(x * timeperhpixel(), true) + Siblings->dwtimestart(); }

CTime_base
DS_lndraw::GetSnappedTimeForTime(const CTime_base &time) {
	static CTime_base curtime, major, minor, offset;
	static Cstring stripped_current_time, current_time_as_string;
	
	string pref = Preferences().GetSetPreference("SnapToGrid", "TRUE");

	if((pref == "TRUE") &&  determineTimemarkLayout(
			      Siblings->dwtimestart(),
			      Siblings->dwtimespan(),
			      ((ACT_sys *) this)->timezone,
			      major,
			      minor,
			      offset) == apgen::RETURN_STATUS::SUCCESS) {

		//gets us to the first major
		CTime_base roundedTime = Siblings->dwtimestart() + offset;
		CTime_base halfMinor = minor/2;
		
		while(abs(time - roundedTime) > halfMinor) {
			if(roundedTime < time) {
				roundedTime += minor; }
			else {
				roundedTime -= minor; } }
			return roundedTime; }
	return time; }

CTime_base
DS_lndraw::GetSnappedTimeForPosition(int x) {
	CTime_base curtime = GetTimeForPosition(x);

	return GetSnappedTimeForTime(curtime); }

Cstring
DS_lndraw::GetLegendForPosition(int y) {
	Cstring newlegend = GENERICLEGEND;
	vertically_scrolled_item *ldef = NULL;

	if((ldef = legend_display->findTheVSIat(y, NULL))) {
		newlegend = ldef->get_key(); }
	return newlegend; }


void set_type_of_new_act_to(const Cstring &tname) {
	selected_type_of_new_activity = tname; }

void DS_lndraw::define_new_activity_starting_at(int x, int y) {
	CTime_base			newstartime;
	Cstring			newlegend, newid;
	Cstring			newtype;
	LegendObject		*ldef = NULL;
	NEW_ACTIVITYrequest	*request1;
	GROUP_ACTIVITIESrequest *request2;

	// Setting planFilename is done by the action request now; this is necessary because
	// SEQ_TALK propagates activity creation between participants, and the Dsource needs
	// to be defined exactly as if the creation had occurred from within:
	// ACT_sys::ACT_subsystem->planFilename = "New";

	ACT_sys::exit_defmode();

	// Figure out the Start time:
	newstartime = GetSnappedTimeForPosition(x);

	DBG_INDENT("DS_lndraw(\"" << get_key() << "\")::define_new_activity_starting_at: "
		<< CTime_base(newstartime).to_string() << ", x=" << x << " y=" << y << "\n");

	DBG_NOINDENT("  selected_type_of_new_activity is defined as \"" <<
			selected_type_of_new_activity << "\"; using it to get legend...\n");
	newtype = selected_type_of_new_activity;

	newlegend = GENERICLEGEND;
	if((ldef = (LegendObject *) legend_display->findTheVSIat(y, NULL))) {
		DBG_NOINDENT("Hmmm... the user clicked on a specific legend, \"" << ldef->get_key() << "\"...\n");
		newlegend = ldef->get_key(); }
	else {
		DBG_NOINDENT("Hmmm... findTheVSIat(" << y << ") fails to return a legend. Using \"" << newlegend << "\".\n"); }

	/* Don't figure out a new ID -- that will be automatically generated by
	*  the Dsource constructor, with optimal results -- but seed it with
	*  the Type (used also as Name, for new instances), for best root.
	*/
	newid = newtype;
	if(this == ACT_sys::theHopper) {
		Cstring		prefix("0Hopper:");

		// This is the one 'quick and dirty' feature we keep:
		// a legend starting with "0Hopper:" is an indication
		// that we want this activity to appear in the hopper.
		newlegend = prefix + newlegend;
		// debug
		// cerr << "Creating new activity with legend " << newlegend << endl;
		}
					//  name = id = type
	request1 = new NEW_ACTIVITYrequest(newtype, newtype, newid, newstartime.to_string(), newlegend);
	if(	request1->process() == apgen::RETURN_STATUS::SUCCESS
		&& !strcmp(contextForRequestingTheNewActivity, "parent of a group")) {
		request2 = new GROUP_ACTIVITIESrequest(tentativeDescendants(), request1->get_id(), 0, 0);
		request2->process(); }

	((ACT_sys *) this)->never_scrolled = 0;

	DBG_UNINDENT("DS_lndraw::define_new_activity_starting_at: END\n"; cout.flush()); }

void DS_lndraw::clear_all_legends() {
	List_iterator		someLegends(legend_display->pointersToTheVisibleLegends);
	bpointernode		*bleg;
	int			Xtotal = getwidth(), j;

	DBG_INDENT("DS_lndraw(" << get_key() << ")::clear_all_legends() START\n");
 
	while((bleg = (bpointernode *) someLegends())) {
		VerticalThing	*vt = (VerticalThing *) bleg->get_ptr();
		Lego		*obj = vt->get_lego();
		int		*StaggerInfo = obj->get_stagger_info_for(legend_display);

		for(j = 0; j < Xtotal; j++) {
			StaggerInfo[j] = 0; } }
	theMaximumStaggeringOffset = 0;
	DBG_UNINDENT("DS_lndraw(" << get_key() << ")::clear_all_legends() DONE\n"); }

void DS_lndraw::clearlegend(DS_line *for_this_line) {
#ifdef GUI
	if(GUI_Flag) {
	bpointernode		*bleg;
	Lego			*theLegObj;
	Dsource			*the_source;
	vertically_scrolled_item	*vsi;
	int			Xtotal = getwidth(), j, sx, ex, sy, ey;
	int			*bits;

	if(!for_this_line)
		return;
	the_source = for_this_line->getsource();
	theLegObj = (Lego *) the_source->get_legend();
	if(!(bleg = (bpointernode *) legend_display->pointersToTheVisibleLegends.find((void *) theLegObj->vthing)))
		return;
	vsi = (vertically_scrolled_item *) bleg->get_ptr();
	theLegObj = ((VerticalThing *) vsi)->get_lego(); 
	bits = theLegObj->get_stagger_info_for(legend_display);
	DBG_INDENT("DS_lndraw(" << get_key() << ")::clearlegend("
		<< for_this_line->getsource()->get_key() << ") START\n");
	for_this_line->getposition(sx, sy, ex, ey);
	for(j = sx; j < ex; j++) {
		if(j >= 0 && j < Xtotal) {
			// logic is somewhat uncertain here... will have to see what it looks like!
			if(bits[j] > 0)
				bits[j]--;
			// if(bleg->busy_bits[j] > 0)
			// 	bleg->busy_bits[j]--;
			} }
	DBG_UNINDENT("DS_lndraw(" << get_key() << ")::clearlegend() DONE\n");
	}
#endif
}

void DS_lndraw::disambiguation_OK(strintslist&) {
	strint_node*	N;
	Pointer_node	*ptr;
	smart_actptr*	pnode;
	DS_line		*dsnode;
	Dsource*	actnode;
	strintslist::iterator obsearch(Dbox->selected_acts);
	slist<alpha_void, smart_actptr>	selection_list;

	eval_intfc::get_all_selections(selection_list);
	if(!getkeypressed()) {
		slist<alpha_void, smart_actptr>::iterator	sels(selection_list);

		while((pnode = sels())) {
			actnode = pnode->BP;
			actnode->unselect(); } }

	while((N = obsearch.next())) {
		ptr = (Pointer_node *) Dbox->ListOfPointersToLineObjects[N->payload - 1];
		if(ptr && (dsnode = (DS_line *) ptr->get_ptr()))
			handle_one_selected_DS_line(dsnode, Dbox->save_x, Dbox->save_y); }

	Dbox->selected_acts.clear();
	Dbox->ListOfPointersToLineObjects.clear(); }

void DS_lndraw::add_vertical_cursor_if_necessary(int ul1, int ul2, int lr1, int lr2) {
#ifdef GUI
	if(GUI_Flag) {
		if(Siblings->verticalCursorEnabled && last_cursor_position >= ul1 && last_cursor_position <= lr1) {
			if(the_vertical_cursor_is_visible) {
				draw_invert_vertical_line(last_cursor_position, ul2, lr2);
			}
		}
	}
#endif
}

DS_timemark::DS_timemark(const Cstring & N, motif_widget * big_form, ACT_sys * act_parent)
	: DS_time_scroll_graph(N, big_form, 0, 0, act_parent->get_siblings()),
	lbgc(NULL),
	lsgc(NULL),
	ActSysParent(act_parent),
	current_timezone(act_parent->timezone),
	local_start(0, 0, false),
	local_dur(0, 0, true) {
	DBG_NOINDENT("DS_timemark constructor for " << get_key() << ": XtWindow(widget)=" << XtWindow(widget) << endl);
	start_y = 0;
	baseline =  0; }

DS_timemark::~DS_timemark() {
	if(lbgc) delete lbgc;
	if(lsgc) delete lsgc;
	if(baseline) delete baseline; }

	/* This method allows the timemark to adopt the same width
	 * as the controlling display graph. It is called at the
	 * very end of DS_timemark::configure_graphics(). */
void DS_timemark::refresh_to_reflect_new_width(int W) {
#ifdef GUI
	if(GUI_Flag) {
	List_iterator	lnobjs(lnobjects);
	List_iterator	majors(majormarks);
	DS_simple_line	*lnobjptr;
	int		lnct = 0;
	int		xpos;

	DBG_INDENT("DS_timemark(\"" << get_key() << "\")::refresh_to_reflect_new_width(" << W << ")... START\n");
	if (!baseline) {
		// PIXMAP
		baseline = new DS_simple_line(*lsgc, XtWindow(widget), 0, 0, 0, 0); }

	//Set up line location when line objects have been created
	//with the desired GC
	if (lbgc && W) {
		baseline->newcoor(0, start_y, W, start_y);
		//minor marks
		while((lnobjptr = (DS_simple_line *) lnobjs.next())) {
			xpos=(int)((lnobjptr->timeloc - local_start) /
				   local_dur * ((float) W));
			lnobjptr->newcoor(xpos, start_y - TMLSHEIGHT, xpos, start_y); }
		//major marks
		while((lnobjptr = (DS_simple_line *) majors.next())) {
			xpos = (int) (((lnobjptr->timeloc) - local_start) /
				   local_dur * ((float) W));
			lnobjptr->newcoor(xpos, start_y - TMLBHEIGHT, xpos, start_y); }
		if (majormarks.get_length())
			createtmdisplay(); }
	update_graphics(NULL);
	DBG_UNINDENT("DS_timemark(\"" << get_key() << "\")::refresh_to_reflect_new_width(" << W << ")... END\n");
	}
#endif
}

void DS_timemark::configure_graphics(callback_stuff *) {
#ifdef GUI
	if(GUI_Flag) {
	CTime_base			currentmajor;
	CTime_base			nextpoint, endtime;
	DS_simple_line		*newline;
	Dimension		lheight;
	activity_display	*AD = ActSysParent->get_AD_parent();

	/*
	 * Since this function is called whenever a Configure event occurs, we have the option of
	 * using it to detect changes in Pixmap size and update the Pixmap accordingly. We should
	 * keep Pixmap coordinates handy; simplest: convenience functions to access Pixmap data.
	 * If there is any change between window and pixmap sizes, re-define the Pixmap.
	 */
	local_start = Siblings->dwtimestart();
	local_dur = Siblings->dwtimespan();
	DBG_INDENT("DS_timemark(\"" << get_key() << "\")::configure_graphics: START\n");
	DBG_NOINDENT("will use start = " << CTime_base(local_start).to_string() << ", duration = ");
	DBG_NOPREFIX(CTime_base(local_dur).to_string() << endl);
	current_timezone = ActSysParent->timezone;
	DBG_NOINDENT("timezone for AD " << AD->get_key() << ": zone " << current_timezone.zone << ", epoch "
		<< current_timezone.epoch << endl);
	*AD->adPointerTimeLabel = "";
	if(determineTimemarkLayout(
		local_start,
		local_dur,
		ActSysParent->timezone,
		Major,
		Minor,
		offset) != apgen::RETURN_STATUS::SUCCESS) {
		// we should probably do something more drastic here... oh well
		return; }
	endtime = local_start + local_dur;
	currentmajor = local_start + offset;
	DBG_NOINDENT("...setting currentmajor to " << currentmajor.to_string() << "...\n");
	nextpoint = currentmajor - Minor;

	if(! isVisible()) {
		DBG_UNINDENT("DS_timemark(\"" << get_key() << "\")::configure_graphics: END (not configured yet)\n");
		return; }

	// will define gc's etc. if first time; not much time wasted since we just deleted all ticmarks...
	update_graphics(NULL);
	lnobjects.clear();
	majormarks.clear();

	//Create the DS_simple_line's for the minors before first major
	while(nextpoint >= local_start) {
		// PIXMAP
		newline = new DS_simple_line(*lsgc, XtWindow(widget), 0, 0, 0, 0);
		newline->timeloc = nextpoint;
		lnobjects << newline;
		nextpoint = nextpoint - Minor; }

	DBG_NOINDENT("...lnobjects now has " << lnobjects.get_length() << " nodes...\n");

	while(currentmajor <= endtime) {
		// PIXMAP
		newline = new DS_simple_line(*lbgc, XtWindow(widget), 0, 0, 0, 0);
		newline->timeloc = currentmajor;
		majormarks << newline;
		nextpoint = currentmajor + Minor;
		currentmajor = currentmajor + Major;
		DBG_NOINDENT("...incrementing currentmajor to " << currentmajor.to_string() << "...\n");
		while(nextpoint < currentmajor && nextpoint <= endtime) {
			// PIXMAP
			newline = new DS_simple_line(*lsgc, XtWindow(widget), 0, 0, 0, 0);
			newline->timeloc = nextpoint;
			DBG_NOINDENT("...setting line timeloc to " << newline->timeloc.to_string() << "...\n");
			lnobjects << newline;
			nextpoint = nextpoint + Minor; } }

	XtSetArg(args[0], XmNheight, &lheight);
	XtGetValues(widget, args, 1);
	//Determine the vertical position for time mark line
	start_y = (int) (TMVPOS * ((float) lheight));
	txtstart_y = (int) (TMTXTVPOS * ((float) lheight));
	txt2start_y = (int) (TMTXT2VPOS * ((float) lheight));
	refresh_to_reflect_new_width(getwidth());

	DBG_UNINDENT("DS_timemark(\"" << get_key() << "\")::configure_graphics: END\n"; cout.flush()); }
#endif
}

//Create the Time text DS_text's after all DS_simple_line's
void DS_timemark::createtmdisplay() {
#ifdef GUI
	if(GUI_Flag) {
	CTime_base		start_time_of_display = Siblings->dwtimestart();
	CTime_base		span_of_display = Siblings->dwtimespan() / ActSysParent->timezone.scale;
				// Lower Label is always used;
				// Upper Label is for optional additional information:
	DS_text		*LowerTicLabelText = NULL, * UpperTicLabelText = NULL;
	int		nbrmajor=majormarks.get_length();
			//example time can use ANY time 1+ day after 0=="1970-001T00:00:00", so
			//  that timezones West of Greenwich yield non-negative time values:
	CTime_base		tmtemp(start_time_of_display),
			tmtemp2(start_time_of_display + span_of_display);
	Cstring		tmexample(tmtemp.to_string(ActSysParent->timezone, false));
	Cstring		tmexample2(tmtemp2.to_string(ActSysParent->timezone, false));
	int		leftbound =0;
	int		rightbound = getwidth();
	List_iterator	majors(majormarks);
	DS_simple_line	*dnode;
	int		startloc;
	int		upper_text_width, lower_text_width;
	Cstring		left_edge_time, stripped_left_edge_time;

	DBG_INDENT("DS_timemark(\"" << get_key() << "\")::createtmdisplay() START\n");

	tmobjects.clear();

	while((dnode = (DS_simple_line *) majors())) {
		CTime_base		current_time_as_time(dnode->timeloc);
		Cstring		current_time_as_string(current_time_as_time.to_string(
						ActSysParent->timezone,
						false));
		Cstring		stripped_current_time;

		startloc = dnode->getstartx();
		DBG_NOINDENT("...current_time_as_string = " << current_time_as_string << "\n");
		DBG_NOINDENT("...current_time_as_time = " << current_time_as_time.to_string() << "\n");
		// PIXMAP
		LowerTicLabelText = new DS_text(*lbgc, XtWindow(widget), widget, NULL);
		if(span_of_display > CTime_base(5, 0, true)) {
			// CASE 1: long span
			if(ActSysParent->timezone.zone == TIMEZONE_EPOCH) {
				// CASE 1A: epoch-relative long

				// lower label

				stripped_current_time = (* ActSysParent->timezone.epoch) / current_time_as_string;
				LowerTicLabelText->settext(stripped_current_time);
				lower_text_width = LowerTicLabelText->strwidth(*stripped_current_time);
				LowerTicLabelText->setto((dnode->getstartx()) - (lower_text_width >> 1), txtstart_y);

				// no upper label!

				upper_text_width = 0;
			} else	{
				// CASE 1B: straight long

				// lower label

				LowerTicLabelText->settext(current_time_as_string);
				lower_text_width = LowerTicLabelText->strwidth(*current_time_as_string);
				LowerTicLabelText->setto((dnode->getstartx()) - (lower_text_width >> 1), txtstart_y);

				// upper label
				// PIXMAP
				UpperTicLabelText = new DS_text(*lbgc, XtWindow(widget), widget, NULL);
				Cstring scet;
				current_time_as_time.time_to_word_date_SCET_string(scet, ActSysParent->timezone);
				upper_text_width = UpperTicLabelText->strwidth(*scet);
				UpperTicLabelText->setto((dnode->getstartx()) - (upper_text_width >> 1), txt2start_y);
				UpperTicLabelText->settext(scet);
			}
		} else {
			// CASE 2: small span
			CTime_base	A;
			CTime_base	nice_left_edge_time;
			Cstring		offset_duration;

			tmtemp = start_time_of_display;
			// 'FALSE' means 'do NOT keep milliseconds':
			left_edge_time = tmtemp.to_string(ActSysParent->timezone, false);

			// upper label
			// PIXMAP
			UpperTicLabelText = new DS_text(*lbgc, XtWindow(widget), widget, NULL);
			if(ActSysParent->timezone.zone == TIMEZONE_EPOCH)	{
				// CASE 2A: epoch-relative short
				// nice_left_edge_time = CTime_base(left_edge_time);
				/*	The above line won't work (trust me), because we are being somewhat
					inconsistent in our formats... need to convert from

						mars_time+4:01:23
					OR
						mars_time-4:01:23

					to

						"mars_time":4:01:23

				*/
				stripped_left_edge_time = (* ActSysParent->timezone.epoch) / left_edge_time;
				nice_left_edge_time = CTime_base(Cstring("\"") + ActSysParent->timezone.epoch + "\":" +
					stripped_left_edge_time);
				upper_text_width = UpperTicLabelText->strwidth(*stripped_left_edge_time);
				UpperTicLabelText->setto((dnode->getstartx()) - (upper_text_width >> 1), txt2start_y);
				UpperTicLabelText->settext(stripped_left_edge_time); }
			else {
				nice_left_edge_time = CTime_base(left_edge_time);
				upper_text_width = UpperTicLabelText->strwidth(*left_edge_time);
				UpperTicLabelText->setto((dnode->getstartx()) - (upper_text_width >> 1), txt2start_y);
				UpperTicLabelText->settext(left_edge_time); }

			// lower label

			A = (dnode->timeloc - nice_left_edge_time) / ActSysParent->timezone.scale;

			if(A >= CTime_base(0, 0, true))
				offset_duration = Cstring("+ ") + Cstring(A.convert_to_double_use_with_caution(), true) + Cstring(" sec");
			else
				offset_duration = Cstring("- ") + Cstring(- A.convert_to_double_use_with_caution(), true) + Cstring(" sec");
			lower_text_width = LowerTicLabelText->strwidth(*offset_duration);
			LowerTicLabelText->setto((dnode->getstartx()) - (lower_text_width >> 1), txtstart_y);
			LowerTicLabelText->settext(offset_duration); }
		if(	   ((startloc - (upper_text_width >> 1)) > leftbound)
			&& ((startloc + (upper_text_width >> 1)) < rightbound)
			&& ((startloc - (lower_text_width >> 1)) > leftbound)
			&& ((startloc + (lower_text_width >> 1)) < rightbound)
		 ) {
			if (LowerTicLabelText) {
				tmobjects << LowerTicLabelText;
				leftbound = dnode->getstartx() + (lower_text_width >> 1);
				if (UpperTicLabelText) { //only use supplemental text if use primary text
					if(leftbound < dnode->getstartx() + (upper_text_width >> 1))
						leftbound = dnode->getstartx() + (upper_text_width >> 1);
					tmobjects << UpperTicLabelText; } } }
		else {
			delete LowerTicLabelText;
			delete UpperTicLabelText; }
		LowerTicLabelText = 0;
		UpperTicLabelText = 0; }

	DBG_NOINDENT("...tmobjects has " << tmobjects.get_length() << " nodes...\n");
	DBG_UNINDENT("DS_timemark \"" << get_key() << "\" ::createtmdisplay() END\n");
	}
#endif
}


//Draw time mark
void DS_timemark::update_graphics(callback_stuff *) {
#ifdef GUI
	if(GUI_Flag) {
	List_iterator		lnlist(lnobjects);
	List_iterator		majorlist(majormarks);
	List_iterator		tmlist(tmobjects);
	Node			*cnode;
	XGCValues		gcvalue;

	if(! isVisible()) return;
	DBG_INDENT("DS_timemark(\"" << get_key() << "\")::update_graphics() START\n");
	DBG_NOINDENT("  XtWindow(widget)=" << XtWindow(widget) << endl);

	//Check to make sure it is ready for drawing
	if (!lbgc) {
		//Create line draw GC
		// gcvalue.foreground = BlackPixel(XtDisplay(widget),
		// 			  XDefaultScreen(XtDisplay(widget)));
		gcvalue.foreground = ResourceData.black_color;
		gcvalue.line_style = LineSolid;
		gcvalue.line_width = TMLBIG;
		// MEMORY LEAK (hmmm... was it fixed? I think so... the DS_timemark() destructor frees it.)
		lbgc = new DS_gc(XtDisplay(widget),
				// KEEP WINDOW
				XtWindow(widget),
				GCLineWidth | GCLineStyle | GCForeground,
					&gcvalue);
		gcvalue.line_width=TMLSMALL;
		// KEEP WINDOW
		lsgc = new DS_gc(XtDisplay(widget), XtWindow(widget),
			GCLineWidth | GCLineStyle | GCForeground,
			&gcvalue); }
	//clear for redraw
	cleargraph();
	//Draw timemark
	DBG_NOINDENT("Drawing baseline...\n");
	if (!baseline) {
		// PIXMAP
		baseline = new DS_simple_line(*lsgc, XtWindow(widget), 0, 0, 0, 0); }
	baseline->draw();
	while((cnode = lnlist.next())) {
		((DS_simple_line *) cnode)->draw(); }
	while((cnode = majorlist.next())) {
		((DS_simple_line *) cnode)->draw(); }
	//Write the time text
	while((cnode = tmlist.next())) {
		DBG_NOINDENT("...drawing \"" << ((DS_text *) cnode)->tdata << "\"\n");
		((DS_text *) cnode)->draw(); }

	DBG_UNINDENT("DS_timemark(\"" << get_key() << "\")::update_graphics() END\n"); }
#endif
}

// int DS_timemark::isOutOfSync(infoList* who)
synchro::problem*	DS_timemark::detectSyncProblem() {
#ifdef GUI
	if(GUI_Flag) {
		synchro::problem*	a_problem;
	// infoNode*	tl;

	// if(refresh_info::ON) {
	// 	tl = new infoNode(get_key());
	// 	(*who) << tl; }

	if(	local_start != Siblings->dwtimestart()
		|| local_dur != Siblings->dwtimespan()) {
		Cstring		the_info(get_key());

		the_info << ": Discrepancy between ";
		if(local_start != Siblings->dwtimestart()) {
			the_info << "local_start = " << local_start.to_string() << " and siblings start = ";
			the_info << Siblings->dwtimestart().to_string(); }
		else if(local_dur != Siblings->dwtimespan()) {
			the_info << "local_start = " << local_dur.to_string() << " and siblings span = ";
			the_info << Siblings->dwtimespan().to_string(); }
		a_problem = new synchro::problem(
					the_info,
					this,
					DS_graph::checkForSyncProblem,
					NULL,
					NULL);
		DBG_NOINDENT("    DS_timemark::isOutOfSync() returning YES because\n");
		DBG_NOINDENT("        local start = " << CTime_base(local_start).to_string());
		DBG_NOPREFIX(", Siblings dwtimestart = " << CTime_base(Siblings->dwtimestart()).to_string() << endl);
		DBG_NOINDENT("        local dur = " << CTime_base(local_dur).to_string());
		DBG_NOPREFIX(", Siblings dwtimespan = " << CTime_base(Siblings->dwtimespan()).to_string() << endl);
		return a_problem; }
	if(!(current_timezone == ActSysParent->timezone)) {
		Cstring		the_info(get_key());
		the_info << ": Discrepancy between " << "current_timezone: " << current_timezone.epoch << ", scale "
			<< current_timezone.scale << ", origin " << current_timezone.theOrigin->to_string();
		the_info << "\nACT_sys zone: " << ActSysParent->timezone.epoch << ", scale "
			<< ActSysParent->timezone.scale << ", origin " << ActSysParent->timezone.theOrigin->to_string();
		a_problem = new synchro::problem(
					the_info,
					this,
					DS_graph::checkForSyncProblem,
					NULL,
					NULL);
#		ifdef apDEBUG
		DBG_NOINDENT("    current_timezone: " << current_timezone.epoch << ", scale " << current_timezone.scale << ", origin ");
		if(*current_timezone.theOrigin > CTime_base(0, 0, false)) {
			cout << current_timezone.theOrigin->to_string(); }
		cout << ", zone " << current_timezone.zone << endl;
		DBG_NOINDENT("    act_sys timezone: " << ActSysParent->timezone.epoch << ", scale " << ActSysParent->timezone.scale << ", origin ");
		if(*ActSysParent->timezone.theOrigin > CTime_base(0, 0, false)) {
			cout << ActSysParent->timezone.theOrigin->to_string(); }
		cout << ", zone " << ActSysParent->timezone.zone << endl;
		DBG_NOINDENT("    DS_timemark::isOutOfSync() returning YES because timezone does not agree with that of ACT_sys parent.\n");
#		endif
		return a_problem; }
	// if(who) {
	// 	delete tl; }
	}
#endif
	return 0; }

CTime_base DS_timemark::get_time_unit() const {
	static CTime_base		T1, T2, horizon, theStartTimeOfThePlan, theEndTimeOfThePlan;

	if(model_intfc::FirstEvent(T1)) {
		model_intfc::LastEvent(T2);
		theStartTimeOfThePlan = (Siblings->dwtimestart() < T1 ? Siblings->dwtimestart() : T1);
		if((Siblings->dwtimestart() + Siblings->dwtimespan()) > T2) {
			theEndTimeOfThePlan = Siblings->dwtimestart() + Siblings->dwtimespan(); }
		else {
			theEndTimeOfThePlan = T2; } }
	else {
		theStartTimeOfThePlan = Siblings->dwtimestart();
		theEndTimeOfThePlan = Siblings->dwtimestart() + Siblings->dwtimespan(); }
	// One billion seconds =~ 30 years; sounds reasonable. I am assuming that the scrollbar can
	// handle any 4-byte integer.
	if((theEndTimeOfThePlan - theStartTimeOfThePlan) / Minor > 1000000000.) {
		return (theEndTimeOfThePlan - theStartTimeOfThePlan) / 1000000000.; }
	else {
		return Minor; } }

apgen::RETURN_STATUS  determineTimemarkLayout(
				CTime_base		startTime,
				CTime_base		duration,
				const TIMEZONE	&timezone,
				CTime_base		&majors,
				CTime_base		&minors,
				CTime_base		&offset) {
	static CTime_base oneSec	(		"0000T00:00:01");
	static CTime_base twoSecs	(		"0000T00:00:02");
	static CTime_base fiveSecs	(		"0000T00:00:05");
	static CTime_base tenSecs	(		"0000T00:00:10");
	static CTime_base fifteenSecs (		"0000T00:00:15");
	static CTime_base twentySecs	(		"0000T00:00:20");
	static CTime_base thirtySecs (		"0000T00:00:30");
	static CTime_base oneMin	(		"0000T00:01:00");
	static CTime_base twoMins	(		"0000T00:02:00");
	static CTime_base fiveMins	(		"0000T00:05:00");
	static CTime_base tenMins	(		"0000T00:10:00");
	static CTime_base fifteenMins (		"0000T00:15:00");
	static CTime_base thirtyMins	(		"0000T00:30:00");
	static CTime_base oneHour	(		"0000T01:00:00");
	static CTime_base twoHours	(		"0000T02:00:00");
	static CTime_base fourHours	(		"0000T04:00:00");
	static CTime_base eightHours (		"0000T08:00:00");
	static CTime_base oneDay	(		"0001T00:00:00");
	static CTime_base twoDays	(		"0002T00:00:00");
	static CTime_base fourDays	(		"0004T00:00:00");
	static CTime_base oneWeek	(		"0007T00:00:00");
	static CTime_base twoWeeks	(		"0014T00:00:00");
	static CTime_base fourWeeks	(		"0028T00:00:00");
	static CTime_base oneMonth	(		"0030T10:30:00");
	static CTime_base eightWeeks (		"0056T00:00:00");
	static CTime_base oneQuarter (		"0091T07:30:00");
	static CTime_base sixteenWeeks (		"0112T00:00:00");
	static CTime_base twoQuarters (		"0182T15:00:00");
	static CTime_base thirtytwoWeeks (		"0224T00:00:00");
	static CTime_base oneYear	(		"0365T06:00:00");
	static CTime_base twoYears	(		"0730T12:00:00");
	static CTime_base fourYears	(		"1461T00:00:00");
	static CTime_base eightYears (		"2922T00:00:00");
	static CTime_base sixteenYears (		"5844T00:00:00");

	// apply any time offset and scale factor:
	if(timezone.zone == TIMEZONE_EPOCH) {
		startTime = (startTime - *timezone.theOrigin) / timezone.scale;
		duration /= timezone.scale; }

	if(duration <= CTime_base(0, 0, true)) {
		cerr << "timemark_layout: illegal duration " << duration.to_string() << endl;
		return apgen::RETURN_STATUS::FAIL; }
	else if(duration <= CTime_base(20, 0, true)) {
		double		power_of_three = 1.;
		// static double	integral_part, fractional_part;

		while(1) {
			if(duration <= CTime_base(5, 0, true) * power_of_three)
				power_of_three /= 2.;
			else
				break;
			if(duration <= CTime_base(5, 0, true) * power_of_three)
				power_of_three /= 5.;
			else
				break; }
		majors = CTime_base::convert_from_double_use_with_caution(power_of_three, true);
		// fractional_part = modf(power_of_three, &integral_part);
		// majors = CTime_base((int) integral_part, (int) (1000 * fractional_part));
		minors = majors / 5.; }
	else if(duration < oneMin) {		// < 12 majors, 5 minors/major
		majors = fiveSecs; minors = oneSec; }
	else if(duration < twoMins) {		// 6-12 majors, 5 minors/major
		majors = tenSecs; minors = twoSecs; }
	else if(duration < fiveMins) {		// 4-10 majors, 6 minors/major
		majors = thirtySecs; minors = fiveSecs; }
	else if(duration < tenMins) {		// 5-10 majors, 4 minors/major
		majors = oneMin; minors = fifteenSecs; }
	else if(duration < thirtyMins) {		// 5-15 majors, 4 minors/major
		majors = twoMins; minors = thirtySecs; }
	else if(duration < twoHours) {		// 6-24 majors, 5 minors/major
		majors = fiveMins; minors = oneMin; }
	else if(duration < fourHours) {		// 4-8 majors, 6 minors/major
		majors = thirtyMins; minors = fiveMins; }
	else if(duration < eightHours) {		// 4-8 majors, 4 minors/major
		majors = oneHour; minors = fifteenMins; }
	else if(duration < oneDay) {		// 4-12 majors, 4 minors/major
		majors = twoHours; minors = thirtyMins; }
	else if(duration < twoDays) {		// 6-12 majors, 4 minors/major
		majors = fourHours; minors = oneHour; }
	else if(duration < fourDays) {		// 6-12 majors, 4 minors/major
		majors = eightHours; minors = twoHours; }
	else if(duration < oneWeek) {		// 4-7 majors, 3 minors/major
		majors = oneDay; minors = eightHours; }
	else if(duration < twoWeeks) {		// 7-14 majors, 3 minors/major
		majors = oneDay; minors = eightHours; }
	else if(duration < fourWeeks) {		// 7-14 majors, 3 minors/major
		majors = twoDays; minors = eightHours; }
	else if(duration < eightWeeks) {		// 4-8 majors, 7 minors/major
		majors = oneWeek; minors = oneDay; }
	else if(duration < sixteenWeeks) {	// 4-8 majors, 2 minors/major
		majors = twoWeeks; minors = oneWeek; }
	else if(duration < thirtytwoWeeks) {	// 4-8 majors, 4 minors/major
		majors = fourWeeks; minors = oneWeek; }
	else if(duration < oneYear) {		// 4-6.5 majors, 4 minors/major
		majors = eightWeeks; minors = twoWeeks; }
	else if(duration < twoYears) {		// 4-8 majors, 3 minors/major
		majors = oneQuarter; minors = oneMonth; }
	else if(duration < fourYears) {		// 4-8 majors, 6 minors/major
		majors = twoQuarters; minors = oneMonth; }
	else if(duration < eightYears) {		// 4-8 majors, 4 minors/major
		majors = oneYear; minors = oneQuarter; }
	else if(duration < sixteenYears) {	// 4-8 majors, 4 minors/major
		majors = twoYears; minors = twoQuarters; }
	else { // (duration >= sixteenYears)	// >=4 majors, 4 minors/major
		majors = fourYears; minors = oneYear; }

	offset = determineTimemarkOffset(startTime, majors, timezone);
// debug
// cout << "determine...: offset = " << offset.to_string() << endl;
// cout.flush();
	if(timezone.zone == TIMEZONE_EPOCH) {
		offset *= timezone.scale;
				majors *= timezone.scale;
				minors *= timezone.scale; }
	return apgen::RETURN_STATUS::SUCCESS; }

CTime_base determineTimemarkOffset(CTime_base		startTime,
			       CTime_base		majors,
			       const TIMEZONE	&timezone) {
	CTime_base		fudgeDuration;
	double		intpart, fractpart;
	double		timeval = startTime.convert_to_double_use_with_caution();

		//Cannot use operator% to get remainder, since it truncates fractional sec.

	if(timezone.zone != TIMEZONE_EPOCH) {
		int	timezone_offset = 60 * ((abs(timezone.zone) + 30) / 60); //1-min granularity

		timeval += (double) ((timezone.zone < 0) ? -timezone_offset : timezone_offset); }
	timeval /= majors.convert_to_double_use_with_caution();
	fractpart = modf(timeval, &intpart);
	if(fractpart < 0.)
		fractpart = 1.0 + fractpart;
// debug
// cerr << "determineTimemarkOffset: fract. part = " << fractpart << endl;
// cerr << "    int. part = " << intpart << endl;
	fudgeDuration = majors * fractpart;
// cerr << "    fudge duration = " << fudgeDuration.to_string() << endl;
// cerr << "    offset = " << (majors - fudgeDuration).to_string() << endl;

	return majors - fudgeDuration; }


// The following global method makes it possible for the user to close
// a window using the standard menu at the top left of the window:
extern void AllowWindowClosingFromMotifMenu(Widget, callback_proc, void*);

void disambiguation_box::Disambiguation_OK_callback(Widget, callback_stuff * clientData, void*) {
	disambiguation_box*	obj0 = (disambiguation_box*) clientData->data;

	obj0->disambiguation_list->get_selected_items(obj0->selected_acts);
#ifdef OBSOLETE
	if(obj0->selected_acts.get_length())
		obj0->ok_was_pressed = 1;
	else
		obj0->ok_was_pressed = 0;
#endif
	obj0->parent_DS_lndraw->disambiguation_OK(obj0->selected_acts);
	obj0->disambiguation_pane->unmanage(); }

void	disambiguation_box::UpdatePriorityBox(Widget, callback_stuff *client_data, void *) {
	disambiguation_box*	obj = (disambiguation_box*) client_data->data;
	strintslist		L;
	strint_node*		N;

	obj->disambiguation_list->get_selected_items(L);
	if((N = L.first_node()))
		obj->update_priority_box(N->payload - 1); }

void	disambiguation_box::update_priority_box(int /* i */) {}

void	disambiguation_box::update_priority_box(int, double)
	{
	}

disambiguation_box::disambiguation_box(
		const Cstring	&name,			//will be name of top widget
		motif_widget	*parent_widget,		//parent of this widget
		DS_lndraw	*P
	)
	: motif_widget(	name,
			xmDialogShellWidgetClass, parent_widget,
			NULL, 0,
			FALSE),
	data_for_window_closing(NULL, Disambiguation_OK_callback, NULL, get_this_widget()),
	parent_DS_lndraw(P),
	request_on_display(NULL)
	{
#ifdef GUI
	if(GUI_Flag) {
	motif_widget	*bottomForm;
	motif_widget	*disambOKbutton;
	motif_widget	*disambLabel;
	Widget		scroll_bar;
	static		int n;
	blist		some_props(compare_function(compare_bstringnodes, false));

	AllowWindowClosingFromMotifMenu(widget, Disambiguation_OK_callback, & data_for_window_closing);
	disambiguation_pane = new motif_widget("disambiguation_pane", xmPanedWindowWidgetClass, this, NULL, 0, FALSE);
	disambiguation_form = new motif_widget("disambiguation_form", disambiguation_pane, form_position(1), TRUE);

	disambLabel = new motif_widget("disambLabel", xmLabelWidgetClass, disambiguation_form);
	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	XtSetArg(args[n], XmNwidth, 300); n++;
	XtSetArg(args[n], XmNheight, 30); n++;
	XtSetValues(disambLabel->widget, args, n);
	*disambLabel = "Select Activity/ies below:";
	*disambiguation_form ^ 10 ^ *disambLabel ^ endform;
	*disambiguation_form < 10 < *disambLabel < 10 < *disambiguation_form < endform;

	some_props << new bstringnode(SCROLLBAR_DISPLAY_POLICY_STATIC);
	disambiguation_list = new scrolled_list_widget(
		"disambiguation_list",
		disambiguation_form,
		5,
		some_props,
		TRUE);

	XtVaGetValues(XtParent(disambiguation_list->widget), XmNverticalScrollBar, &scroll_bar, NULL);

	n = 0;
	*disambiguation_form < 10 < *disambiguation_list < 10 < *disambiguation_form < endform;
	*disambLabel ^ 5 ^ *disambiguation_list ^ *disambiguation_form ^ endform;

	bottomForm = new motif_widget("bottomForm", disambiguation_pane, form_position(39), TRUE);
	n = 0;
	XtSetArg(args[n], XmNwidth, 300); n++;
	XtSetArg(args[n], XmNheight, 30); n++;
	XtSetValues(bottomForm->widget, args, n);

	disambOKbutton = new motif_widget("disambOKbutton", xmPushButtonWidgetClass, bottomForm);
	*bottomForm ^ *disambOKbutton ^ *bottomForm ^ endform;
	form_position(10) < *disambOKbutton < form_position(29) < endform;
	*disambOKbutton = Cstring("OK");
	disambOKbutton->add_callback(Disambiguation_OK_callback, XmNactivateCallback, this);
#ifdef HAVE_MACOS
	bottomForm->fix_height(20, disambOKbutton->widget);
#else
	bottomForm->fix_height(disambOKbutton->widget);
#endif /* HAVE_MACOS */
	//  disambiguation_form->manage();	NO! wait until popup is needed!!!
	}
#endif /* GUI */
}

list_of_siblings::list_of_siblings(motif_widget *P, CTime_base desired_start_time, CTime_base desired_time_span)
	: Dwtimestart(desired_start_time),
	Dwtimespan(desired_time_span),
	verticalCursorEnabled(0),
	a_client_has_requested_a_time_change(1),	// force a re-evaluation of time parameters
	UserRequestedStart(-1.),
	UserRequestedSpan(-1.),
	common_parent(P),
	timemark(NULL)
	{}

list_of_siblings::~list_of_siblings()
	{}

void list_of_siblings::draw_vertical_cursor(int where) {
	List_iterator		allSiblings(*this);
	sibling			*aSibling;
	DS_graph		*sg;

	while((aSibling = (sibling *) allSiblings())) {
		sg = aSibling->getTheGraph();
		sg->erase_vertical_cursor();
		sg->draw_vertical_cursor(where);
	}
}

void list_of_siblings::erase_vertical_cursor() {
	List_iterator		allSiblings(*this);
	sibling			*aSibling;
	DS_graph		*sg;

	while((aSibling = (sibling *) allSiblings())) {
		sg = aSibling->getTheGraph();
		sg->erase_vertical_cursor();
	}
}

int list_of_siblings::isOutOfSync(infoList* who) {
	if(a_client_has_requested_a_time_change) {
		if(who) {
			infoNode*	tl = new infoNode(common_parent->get_key() + "->list_of_siblings");

			tl->payload.reason = "client has requested new time params (flag is set).\n";
			(*who) << tl;
		}
		return 1;
	}
	return 0;
}

void list_of_siblings::execute_client_request() {
	DBG_NOINDENT("list_of_siblings::execute_client_request for "
		<< ((activity_display *) common_parent)->activitySystem->get_key()
		<< ": setting start to " << CTime_base(UserRequestedStart).to_string());
	DBG_NOPREFIX(", span to " << CTime_base(UserRequestedSpan).to_string() << "\n");
	Dwtimestart = UserRequestedStart;
	Dwtimespan = UserRequestedSpan;
	a_client_has_requested_a_time_change = 0;
}

const CTime_base &list_of_siblings::dwtimestart() {
	if(Dwtimespan < CTime_base(0, 0, true) || Dwtimestart < CTime_base(0, 0, false)) {
		CTime_base		t1;

		// fist time; need to initialize time parameters
		// need to figure out what the correct extents are:
		Dwtimespan = CTime_base(ONEDAY, 0, true);
		model_intfc::FirstEvent(t1);
		Dwtimestart = t1;
	}
	return Dwtimestart;
}

const CTime_base &list_of_siblings::dwtimespan() {
	if(Dwtimespan < CTime_base(0, 0, true) || Dwtimestart < CTime_base(0, 0, false)) {
		CTime_base		t1;

		// fist time; need to initialize time parameters
		// need to figure out what the correct extents are:
		Dwtimespan = CTime_base(ONEDAY, 0, true);
		model_intfc::FirstEvent(t1);
		Dwtimestart = t1;
	}
	return Dwtimespan;
}

void	list_of_siblings::request_a_change_in_time_parameters(const CTime_base &start_time, const CTime_base &time_span) {
	udef_intfc::something_happened() += 1;
	UserRequestedStart = start_time;
	UserRequestedSpan = time_span;
	a_client_has_requested_a_time_change = 1;
}

int sibling::isOutOfSync(infoList* l) {
	synchro::problem*	a_problem = getTheGraph()->detectSyncProblem();
	if(a_problem) {
		delete a_problem;
		return 1;
	}
	return 0;
}

void sibling::configure_graphics() {
	getTheGraph()->configure_graphics(NULL); }

LegendObject	*Lego::theLegoConstructor(	const Cstring &text,
						const Cstring &filename,
						int preferred_height) {
	Lego	*LO = new Lego(text, filename, preferred_height);
	ACT_sys::addALegend(LO);
	return LO;
}

VSI_owner::VSI_owner(DS_scroll_graph *dsgraph)
		: Y_offset_in_pixels(0),
		This(dsgraph),
		pointersToTheVisibleLegends(compare_function(compare_bpointernodes, false))
		{}

synchro::problem*	DS_graph::checkForSyncProblem(synchro::detector* D) {
	DS_graph*	the_graph = dynamic_cast<DS_graph*>(D);

	return the_graph->detectSyncProblem(); }
