/*	PROJECT		: APGEN
 *	SUBSYSTEM	: CONstraint
 *	FILE NAME	: CON_sys.H
 *
 *	ABSTRACT
 *
 *	Constraint "system" (display), coscrolling with ACT_sys as an integral
 *	part of it (like timeline DS_timemark, and indeed placed just below it)
 *
 */

#ifndef CON_SYS_H
#define CON_SYS_H

#include "UI_ds_timeline.H"
#include "DSsource.H"

#include "Constraint.H"

//
// add support for motif_widget "wrapper" and derived type con_popup
//
#include "UI_popups.H"

class DS_violation: public DS_line_for_constraints {
public:
	DS_violation(	DS_gc		& gc,
                        Drawable	draw,
                        int		s_x_coor,
                        int		top_coor,
                        int		e_x_coor,
                        int		bottom_coor,
                        int		W,
                        Dsource*	source,
                        DS_draw_graph* obj);

	~DS_violation() {}

	int		y_start();

	int		y_end();

	friend class CON_sys;
};

//
// Constraint "system" is roughly analogous to ACT_sys "A" and RD_sys "R".
//  Like these, it is derived from DS_time_scroll_graph "D" (ACT_sys is derived
//  from DS_lndraw, which is derived therefrom).  If that derivation yields
//  adequate functionality, there is no need to derive from DS_lndraw.  Closest
//  analog may actually be DS_timemark, after which this is also patterned.
//  Letters on right indicate source of data member or method.
// Aspects like DS_timemark are:	(CURRENTLY) instantaneous events at exact times
//				vertical tick-marks mark events
// Aspects like ACT_sys are:	0+ displayable entities, at arbitrary times
//				(FUTURE) entities may have duration>0
// Aspects like RD_sys are:	only way to update events is via Remodel
//				need access to timezone, for popup output (mark
//				    with "TIMEZONEFIX"; RD_sys not yet marked)
//
class CON_sys : public DS_time_scroll_graph {
protected:

	//
	//Most data members are based on DS_timemark:
	//
	DS_gc		*con_error_gc;						//D
	DS_gc		*con_warning_gc;					//D

			//Popup wrapper and activity-system pointer(for timezone) based on RD_sys:
			//SEQ_REVIEW-style "Wrapper" for Parent Form Widget, so that callbacks can
			//  be attached, allowing informational popup windows
	motif_widget	*adConstraintDA_wrapper;				//R

			//Parent Activity System pointer (need to get timezone):	//TIMEZONEFIX
	ACT_sys		*activitySystem;						//R

			//Remainder of data members (NONE) would be based on ACT_sys.
	int	 	update_requested;	// 0 initially, set to 1 when update_request()

	int		NumberOfLinesDisplayed;

			//Click-pops-up-info event-handler callback (registered with motif_wrapper)
	static void	popup_at_timeCB(Widget, callback_stuff *, void *);

			//
			// Convenience function for popup_at_timeCV
			//
	void		compute_popup_params(
				int x_position,
				CTime_base& atime,
				CTime_base& aspan,
				CTime_base& center,
				Cstring& tot_report,
				Cstring& sel_report);
	
public:
	CON_sys(ACT_sys* actSys,					//TIMEZONEFIX
		const Cstring& name,
		motif_widget * Parent);			//D
	~CON_sys();

	int		is_horizontal() { return 1; }
	synchro::problem* detectSyncProblem();
	int		is_a_con_sys() { return 1; }
	void		cleargraph();

	void		activate();
	void		inactivate();
	
	// Create display for the found added constraint violations
	void		create_con_display();				//D+A

	//
	// Create a display object after CON request is given		//A
	//
	DS_object*	CreateNewDSline(con_violation_gui*, DS_gc*);

	//
	// Duplicates Constraint::violations() as a List of Dsource
	// objects - required by old-style GUI
	//
	List		violations;				

	//
	// Set the time view. starttime: TIME in UTC, timespan in INTERVAL
	//
	apgen::RETURN_STATUS	getviewtime(CTime &starttime, CTime &timespan)	//A
			    { starttime = Siblings->dwtimestart();
			      timespan = Siblings->dwtimespan();
				      return apgen::RETURN_STATUS::SUCCESS; }


	// FUTURE TIMEZONEFIX:  add 3rd arg TIMEZONE timezone to setviewtime, so
	//  CON_sys (and RD_sys and ACT_sys) all know about current timezone --
	//  just like with stime and tspan, ALL coscrolling siblings should know
	//  the current timezone, even though only ONE sibling is allowed to SET it
	apgen::RETURN_STATUS	setviewtime(double stime,double tspan);	//D+R

	// REPLACED BY define_GCs() void		setsize();
	void		define_GCs();					//D
	// DISABLED PFM 12/16/96 void		setwidth(int swidth);

	void		setvunitsize(int) {; }				//R
	void		vdraw() {; }					//R
	void		vscroll(int) {; }				//R
	void		hscroll(int) {; }				//D

	//
	// take care of pure virtual functions:
	//
	void		configure_graphics(callback_stuff *);
	void		rebuild_graphics();
	void		update_graphics(callback_stuff *);

	//****IO

			//Summary information on ALL Constraint Violations (regardless of time)
	virtual int	report_total(Cstring& report);

	//Detailed information on all Constraint Violations within given time span
	//  (as known by objects in this Constraint System);return #lines in report
	virtual int	report_in_span(const CTime &atime, const CTime &aspan, Cstring &report);
	
	//****UTILITIES
			// !!
	Node*		copy() { return NULL; }
};

#endif
