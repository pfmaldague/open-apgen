#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "apDEBUG.H"

#include "ACT_sys.H"					/* TIMEZONEFIX !!! */
#include <ActivityInstance.H>
#include "Constraint.H"
#include "CON_sys.H"
#include "DSsource.H"

using namespace std;

//
// GLOBALS
//
extern con_popup*	_conPopupDialogShell;
extern motif_widget*	MW;

#ifdef GUI
static int nargs;
static Arg args[20];
#endif

//
// Constraint "System" (used to display Constraint Violations), is a coscrolling
//  part of the Activity Display (AD)
//
CON_sys::CON_sys(
	ACT_sys* actSys,				//TIMEZONEFIX
	const Cstring& name,
	motif_widget * Parent)		//D
	: DS_time_scroll_graph(name, Parent, 0, 0, actSys->get_siblings()),
	update_requested( false ),
	NumberOfLinesDisplayed(0) {

	activitySystem = actSys;                               //TIMEZONEFIX

	//initialization
	con_error_gc = NULL;
	con_warning_gc = NULL;
	if (APcloptions::theCmdLineOptions().constraints_active)
		activate();
	else
		inactivate();

	//initialize callback for violation selection 
	add_keyboard_and_mouse_handler(popup_at_timeCB, BTTN1_DN, NO_MODIFIER, this);

}

CON_sys::~CON_sys() {
	if (con_error_gc)
		delete con_error_gc;
	if (con_warning_gc)
		delete con_warning_gc;
	lnobjects.clear();
	violations.clear();
}


synchro::problem* CON_sys::detectSyncProblem() {
	List_iterator
			violst(violations);
	con_violation_gui*
			vionode;


	if(!isVisible()) {
		return NULL;
	}

	if(Constraint::violations().get_length() != NumberOfLinesDisplayed) {
		Cstring reason;
		reason << "violations List contains " << Cstring(Constraint::violations().get_length()) <<
			" element(s) while NumberOfLinesDisplayed = " << Cstring(NumberOfLinesDisplayed);
		return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL);
	} else if(Constraint::violations().get_length() != lnobjects.get_length()) {
		Cstring reason;
		reason << "violations List contains " << Cstring(Constraint::violations().get_length()) <<
			" element(s) while " << Cstring(lnobjects.get_length()) << " line(s) are displayed.";
		return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL);
	}

	while((vionode = (con_violation_gui*) violst())) {
		if(!((derivedDataVio *) vionode->dataForDerivedClasses)->isDisplayedCorrectlyIn(this)) {
			Cstring reason;
			reason << " vio and line have different times";
			return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL);
		}
	}

	return NULL;
}


//Activate the constraint subsystem
void CON_sys::activate () {
#ifdef GUI
	if(GUI_Flag) {

	//set to active color
	nargs = 0;
	XtSetArg(args[nargs], XmNbackground, ResourceData.white_color); nargs++;
	XtSetValues(widget, args, nargs);

	}
#endif
}


//Inactivate the constraint subsystem, so it does nothing until (re-)activate()
void CON_sys::inactivate () {
#ifdef GUI
	if(GUI_Flag) {

	//clear the constraint display (but constraint data structures left alone
	//  by inactivation or activation)
	//Clear for redraw (used in DS_timemark::draw(), not in DS_lndraw::draw())
	cleargraph();

	//set to inactive color
	nargs = 0;
	XtSetArg(args[nargs], XmNbackground, ResourceData.peach_color); nargs++;
	  // CONVERT(widget, "PeachPuff1", XmRPixel, 0, &argok)); if (argok) nargs++;
	XtSetValues(widget, args, nargs);

	}
#endif
}

void CON_sys::cleargraph() {
	DS_draw_graph::cleargraph();
	NumberOfLinesDisplayed = 0;
}

//Create a display object after CON request is given (code based on ACT_sys::
//  with heavy influence from DS_timemark)

/*

RESTRICTIONS;

	height

*/

DS_object* CON_sys::CreateNewDSline(con_violation_gui* newVIO, DS_gc *gc) {
	DS_violation* create_obj = NULL;

#ifdef GUI
    if(GUI_Flag) {

	//
	//NOTE:  ACT_sys creates DS_violation objects which are drawn in the X direction
	//  (time) and have WIDTH (e.g. 10 pixels) in the Y direction.  DS_timemark
	//  draws its ticks with the X location specified using the alternate
	//  "timeloc" data member, and draws it in the Y direction with WIDTH of
	//  TMLBIG (thick major marks) or TMLSMALL (thin minor marks).  CON_sys MAY
	//  at some future time need to draw in the X direction, but for now,
	//  constraint violations are instantaneous, so draw in the Y direction
	//  with line width CONLINEWIDTH.  CON_sys also uses timeloc as the MASTER
	//  X-location source (need the precision!!!); like DS_timemark,it converts
	//  this into X1,Y1, X2,Y2 (start,end pixel locations) for DS_violation::draw().
	//
	int sx;			//start_x (== end_x) coordinate (pixels)
	int sy, ey;		//start_y and end_y coordinates (pixels)

	if(!isVisible()) return NULL;

	//
	// WARNING:  the following calculation can yield integer overflow!!!
	// Realistically, this is not a problem, for the following reasons:
	// 1)  Overflow results in silent wraparound (negative-to-positive or vice-
	//     versa), hence no program-halting error.  Also, all possible int
	//     values are legal (and therefore none play havoc with program logic),
	//     but only the values 0 to (width-1) are displayed (width=1024 is a
	//     typical value, for an HP workstation with APGEN window maximized).
	// 2)  HP and Sun workstations will cycle into this visible range (yielding
	//     spurious marks), every 2**32 pixels.  The 1024-or-so visible range
	//     is truly negligible compared to all possible int values, so that it
	//     is *extremely* unlikely to have a wraparound-spurious mark appear.
	// 3)  At 1 sec/pixel (about 17 minutes window span, for 1024 pixels), the
	//     wraparound cycle is about 136 years!  For 1 minute window span (such
	//     short windows are rare), cycle is just under 8 years, which is a
	//     long (but not unreasonable) time over which to evaluate constraints.


	sx = (int) 	(0.5 +
				(
				newVIO->getetime() - Siblings->dwtimestart()
				).convert_to_double_use_with_caution()
				/ timeperhpixel()
			);

	//
	// set CENTER of tick, in pixels, from top of constraint display:
	//
	ey = getheight();
	if(newVIO->ref->payload.severity == Violation_info::ERROR) //full-height ticks, i.e. span area
		sy = 0;
	else		// WARNING or RELEASE	  half-height ticks, at bottom of area
		sy = 5;

	//
	// Use 0 in place of y_val (to start); really want height/severity relation:
	//
	create_obj  = new DS_violation(	*gc,
					XtWindow(widget),
					sx, sy,
					sx, ey,
					CONLINEWIDTH,
					newVIO,
					this);

	lnobjects << create_obj;
	
    }
#	endif
	return create_obj;
}

DS_violation::DS_violation(
		DS_gc		&gc,
		Drawable	draw,
		int		s_x_coor,
		int		top_coor,
		int		e_x_coor,
		int		bottom_coor,
		int		W,
		Dsource		*source,
		DS_draw_graph	*obj)
		: DS_line_for_constraints(gc, draw, s_x_coor, e_x_coor, W,
			source, obj) {
		source->set_y_to(top_coor, bottom_coor);
		// Done in the DS_line constructor:
		// source->theListOfPointersToDsLines << new Pointer_node(this, this);
		}

int DS_violation::y_start() {
#ifdef GUI
	if(GUI_Flag) {
		con_violation_gui* cv = dynamic_cast<con_violation_gui*>(sourceobj);
		assert(cv);
		return cv->ystart;
	} else {
		return 0;
	}
#else
		return 0;
#endif
}

int DS_violation::y_end() {
#ifdef GUI
	if(GUI_Flag) {
		con_violation_gui* cv = dynamic_cast<con_violation_gui*>(sourceobj);
		assert(cv);
		return cv->yend;
	} else {
		return 0;
	}
#else
		return 0;
#endif
}

//*****DISPLAY    

/*

RESTRICTIONS:

	calls cleargraph()

*/
void CON_sys::update_graphics(callback_stuff *) {
	List_iterator	lnlist(lnobjects);
	Node		*cnode;

	//DS_lndraw iterates thru siblings and forces them to draw(), so shouldn't
	//  have to do that here too, since CON_sys is ALWAYS a sibling of ACT_sys

	//
	//don't draw anything if constraints are inactive!
	// nor if the widget hasn't been exposed!
	//
	if ((! APcloptions::theCmdLineOptions().constraints_active) || (! isVisible()))
		return;

	//
	// Clear for redraw (used in DS_timemark::draw(), not in DS_lndraw::draw())
	//
	cleargraph();

	//
	// Iterate thru the lines
	//
	NumberOfLinesDisplayed = lnobjects.get_length();
	while((cnode = lnlist.next()))
		((DS_violation*) cnode)->draw();
}

/*

RESTRICTIONS:

	height
	width

*/

void CON_sys::define_GCs() {
#ifdef GUI
    if(GUI_Flag) {
	Dimension	lwidth,lheight;
	Arg		largs[2];
	XGCValues	gcvalue;
	
	XtSetArg(largs[0], XmNwidth, &lwidth);
	XtSetArg(largs[1], XmNheight, &lheight);
	XtGetValues(widget, largs, 2);

	//
	// Set BOTH error and warning Graphic Contexts (GC's), if they are unset
	// (this part is based on DS_timemark::setsize())
	//
	if ((lwidth) && (!con_error_gc)) {
		gcvalue.foreground=BlackPixel(/*Xlib.h deficiency causes aCC warning*/
					XtDisplay(widget),
					XDefaultScreen(XtDisplay(widget)));
		gcvalue.line_style=LineSolid;
		gcvalue.line_width=CONLINEWIDTH;
		con_error_gc = new DS_gc(XtDisplay(widget),
			// PIXMAP
			XtWindow(widget),
			GCLineWidth | GCLineStyle | GCForeground,
					&gcvalue);

		//
		// reset color for warning:
		//
		con_warning_gc = new DS_gc(XtDisplay(widget),
				// PIXMAP
				XtWindow(widget),
				GCLineWidth | GCLineStyle | GCForeground,
					&gcvalue);
	}
    }
#endif
}

void CON_sys::rebuild_graphics() {
	slist<alpha_time, con_violation>::iterator
			vioIter(Constraint::violations());
	con_violation*
			vionode;

	lnobjects.clear();

	define_GCs();

	violations.clear();

	//
	// Changed logic from Act_sys::create_req_display(), from removing each node
	// to leaving each node in place (can't imagine why nodes were removed!).
	//
	while((vionode = vioIter())) {

		//
		// Would be nice to use vionode instead of con_copy,
		// but the old, clunky GUI system is hard to refactor
		//
		con_violation_gui* con_copy = new con_violation_gui(vionode);
		
		violations << con_copy;
		CreateNewDSline(
			con_copy,
			(vionode->payload.severity == Violation_info::ERROR) ?
				con_error_gc :
				con_warning_gc
			);
	}
}

void DS_line_for_constraints::newhcoor(int xpos, int) {
	start_x = xpos;
	end_x = xpos; }

void CON_sys::configure_graphics(callback_stuff *) {
	List_iterator		lnobjs(lnobjects);
	DS_violation*		lnobjptr;
	int			xpos;

	if(!isVisible()) {
		update_requested = 1;
		return;
	}

	cleargraph();

	//
	// do this FIRST!! (found out the hard way... PFM)
	//
	define_GCs();

	//
	// Should also check whether the times are OK...
	//
	if(Constraint::violations().get_length() != violations.get_length()) {
		rebuild_graphics();
	}

	//
	// Update position of all DS_violation's
	//
	while((lnobjptr = (DS_violation*) lnobjs.next())) {

		//
		// WARNING: this calculation can yield integer overflow!!!  (SEE
		//	    COMMENT on this same calculation, in CON_sys::CreateNewDSline)
		//
		xpos = (int) (0.5 +
			(lnobjptr->timeloc - Siblings->dwtimestart()).convert_to_double_use_with_caution()
				/ timeperhpixel());
		lnobjptr->newhcoor(xpos, 0);
	}

	update_graphics(NULL);
}

//****IO

void CON_sys::compute_popup_params(
			int x_position,
			CTime_base& atime,
			CTime_base& aspan,
			CTime_base& center,
			Cstring& tot_report,
			Cstring& sel_report) {
	int	ntotlines, nsellines;

	//
	// compute time from X-position (0-based pixel:  0 <= x_position < width)
	//  (NOTE -- this is subject to possible off-by-one-pixel errors, depending
	//  on how CON_sys does its job, AND how AD displays are done)
	//  (???break out as double DS_time_scroll_graph::pixel_to_time(int pixel))
	//
	center = Siblings->dwtimestart()
		+ CTime_base::convert_from_double_use_with_caution(
			((double) x_position) * timeperhpixel(), true);

	//
	// now offset about center time, to get start and span of "nearby" pixels
	//
	aspan = CTime_base::convert_from_double_use_with_caution(
		2.0 * CONSENSITIVITY * timeperhpixel(), true);
	atime = center - (0.5 * aspan);

	//
	// adjust if span goes outside the window!
	//
	if(atime < Siblings->dwtimestart()) {
		aspan -= Siblings->dwtimestart() - atime;
		atime = Siblings->dwtimestart();
		if(aspan < CTime(0, 0, true)) {

			//
			// shouldn't happen, but protect anyway
			//
	    		aspan = CTime(0, 0, true);
		}
	} else if ((atime + aspan) > (Siblings->dwtimestart()  + Siblings->dwtimespan())) {
		aspan = (Siblings->dwtimestart() + Siblings->dwtimespan()) - atime;
		if(aspan < CTime(0, 0, true)) {

			//
			// shouldn't happen, but protect anyway
			//
			atime = Siblings->dwtimestart() + Siblings->dwtimespan();
			aspan = CTime(0, 0, true);
		}
	}

	//
	// get the total-count information; lookup the selected-span violation(s)
	//
	ntotlines = report_total(tot_report);
	nsellines = report_in_span(atime, aspan, sel_report);
}

//Click-pops-up-info event-handler callback (registered with motif_wrapper)
void CON_sys::popup_at_timeCB(Widget		,
			       callback_stuff	*client_data,
			       void		*call_data) {
#ifdef GUI
    if(GUI_Flag) {

	//
	// don't popup if contraints are inactive (easier than disabling callback)!
	//
	if(!APcloptions::theCmdLineOptions().constraints_active) {
		return;
	}

	int		x_position;
	CTime		center, atime, aspan;
	Cstring		tot_report;
	Cstring		sel_report;

			// motif_wrapper passes object pointer as member "data" of client_data
	CON_sys*	consis = (CON_sys *) client_data->data;

			// motif_wrapper passes XEvent directly as call_data
	XButtonEvent*	bevent = (XButtonEvent *) call_data;

	//
	// get button-event location (in pixels)
	//
	x_position = bevent->x;

	consis->compute_popup_params(x_position, atime, aspan, center, tot_report, sel_report);

	//
	// Reset contents of both text fields in motif_widget, and pop it up (if not
	//  already managed) -- temporarily do directly, until operator= works!!!
	//  *_conPopupDialogShell->conPopupTextMultiline = tot_report;

	if(! _conPopupDialogShell) {
		_conPopupDialogShell = new con_popup("conPopupDialogShell", MW, "UNINITIALIZED WIDGET!!!");
	}

	_conPopupDialogShell->set_centertime(center);
	*((motif_widget*) _conPopupDialogShell->conPopupTextMultiline) = tot_report;
	*((motif_widget*) _conPopupDialogShell->conPopupTextScrolled) = sel_report;

	// XtVaSetValues(_conPopupDialogShell->conPopupTextMultiline->widget,
	// 		XmNvalue, *tot_report,
	// 		NULL);
	// XtVaSetValues(_conPopupDialogShell->conPopupTextScrolled->widget,
	// 		XmNvalue, *sel_report,
	// 		NULL);

	//
	// Force stupid window managers such as solaris to re-display the panel:
	//
	if(_conPopupDialogShell->paned_window->is_managed()) {
		_conPopupDialogShell->paned_window->unmanage();
	}
	_conPopupDialogShell->paned_window->manage();

	// Doesn't work:
	// XMapRaised(
	// 	XtDisplay(_conPopupDialogShell->widget),
	// 	XtWindow(XtParent(_conPopupDialogShell->widget)));

    }
#endif
}

/*Summary+detail information on all Constraint Violations within a given time
 *  span(as known by objects in this Constraint System);return #lines in report */
int CON_sys::report_total(Cstring& report) {
    int		nlines = 0;

#   ifdef GUI
    if(GUI_Flag) {
	CTime		ct_time(0, 0, false);
	TIMEZONE	ADzone;
	aoString	Stream;

	//
	//Get current timezone, from parent/sibling AD's timemark's timezone.  Find
	//  the timemark NOT by cycling through AD siblings (which comprise all
	//  parts of the AD and all its RD's),but instead by calling new AD method.
	//
	ADzone = activitySystem->timezone;

	//
	//Find total number of violations (errors + warnings), which always equals
	//  the number of graphic marks, since exactly 1 is generated per violation
	//
	long int ntotal = Constraint::violations().get_length() - Constraint::release_count();

	if (ntotal) {
		Stream << "Total Violations=" << ntotal << ", between\n";

		//
		// Find time of earliest Violation, and print using compound time string
		//
		ct_time = Constraint::violations().first_node()->getKey().getetime();
		Cstring temp;
		ct_time.time_to_complete_SCET_string(temp, ADzone);
		Stream << temp << " and\n";

		//Find and print time of latest Violation
		ct_time = Constraint::violations().last_node()->getKey().getetime();
		ct_time.time_to_complete_SCET_string(temp, ADzone);
		Stream << temp << '\n';
	} else {
		Stream << "Total Violations=0\n";
	}

	report = Stream.str();
	nlines = report.OccurrencesOf("\n");
    }
#   endif
    return nlines;
}


/*Summary+detail information on all Constraint Violations within a given time
 *  span(as known by objects in this Constraint System);return #lines in report */
int CON_sys::report_in_span(
		const CTime&	from,
		const CTime&	aspan,
		Cstring&	report) {
    int		nlines = 0;
#ifdef GUI
    if(GUI_Flag) {

	CTime		to = from + aspan;
	CTime		ct_time(0, 0, false);
	TIMEZONE	ADzone;
	aoString	Stream;

	//
	// Get current timezone, from parent/sibling AD's timemark's timezone.  Find
	// the timemark NOT by cycling through AD siblings (which comprise all
	// parts of the AD and all its RD's),but instead by calling new AD method.
	//
	ADzone = activitySystem->timezone;

	//
	// Get begin and end times w/zone info
	//
	Cstring scet_time_from;
	from.time_to_complete_SCET_string(scet_time_from, ADzone);
	Cstring scet_time_to;
	to.time_to_complete_SCET_string(scet_time_to, ADzone);

	//
	// Search for violations
	//
	con_violation*	vionode = Constraint::violations().find_at_or_after(from);
	if(!vionode || vionode->getKey().getetime() > to) {

		//
		// No violations in interval
		//
		Stream << "No Violations between\n"
			<< scet_time_from << " and\n"
			<< scet_time_to << "\n";
		report = Stream.str();
		return 0;
	}

	con_violation*	vio2;
	long int	count = 0;
	for(vio2 = vionode; vio2; vio2 = vio2->next_node()) {
		if(vio2->getKey().getetime() > to) {
			break;
		}
		count++;
	}
	Stream << "Selected Violation Events=" << count
		<< ", between\n"
		<< scet_time_from << " and\n"
		<< scet_time_to << "\n";

	Cstring scet_time;

	//
	// iterate (again) through (time-ordered) violations
	//
	for(vio2 = vionode; vio2; vio2 = vio2->next_node()) {
		if(vio2->getKey().getetime() > to) {
			break;
		}

		//
		// Find and print time of Violation
		//
		ct_time = vio2->getKey().getetime();
		ct_time.time_to_complete_SCET_string(scet_time, ADzone);
		Stream << '\n' << scet_time << '\n';
		switch(vio2->payload.severity) {
			case Violation_info::ERROR:
				Stream << "ERROR";
				break;
			case Violation_info::WARNING:
				Stream << "WARNING";
				break;
			case Violation_info::RELEASE:
				Stream << "RELEASE";
				break;
		}
		Stream << " in " << vio2->payload.parent_constraint->get_key() << ":\n";
		Stream << vio2->payload.message << '\n';
		// need terminal newline!  add #lines(need Cstring method to count)
	}
	report = Stream.str();
	nlines = report.OccurrencesOf("\n");
    }
#endif
    return nlines;
}
