/*	PROJECT		: APGEN
 *	SUBSYSTEM	: RES
 *	FILE NAME	: RD_sys.H
 *
 *
 *	ABSTRACT:
 *	
 *	Header file for RD_sys class, which is the Resource System that
 *	underlies each Resource Display.
 *
 */

#ifndef  RD_SYS_H
#define  RD_SYS_H

#include <iostream>
#include "UI_mw_timeline.H"

class ACT_sys;
class MW_object;

// PFM: in UTL_defs.H now:
//	enum RD_YSCROLL_MODE
#include "UTL_defs.H"

//add support for motif_widget "wrapper" and derived type rd_popup used by RD
#include "UI_motif_widget.H"
#include "UI_popups.H"

/** RD_sys (which is derived from MW_widgetdraw) is analogous to ACT_sys
 * (which is derived from DS_lndraw). This is functionally appropriate,
 * since each Resource Display (and its underlying Resource System) is associated with
 * exactly one Activity Display (and its underlying Activity System).  Therefore
 * the UI performs most Resource Display interaction via methods of this class.
 * 
 * All Resource Displays associated with a single Activity Display are
 * horizontal (i.e. time) co-scrolling siblings with it, although each Display
 * is (currently) contained within a separate subwindow of the main window.  Any
 * vertical scrolling capability is UNRELATED to Activity Display vertical
 * scrolling, and thus is implemented separately to avoid unwanted co-scrolling.
 * 
 * Each Resource Display can display one (initially) or more Resources.  Each
 * Resource is displayed using a resource plot; multiple plots are stacked.
 */
class RD_sys : public MW_widgetdraw, public synchro::fixer {
private:
	CTime			myStartTime, myDuration;
public:
				/* Parent Activity System (for time-scrolling purposes, it is a "sibling",
				 * but each Resource System must "register" with its parent, in order to
				 * be under sibling "scroll control" by the Parent Activity System). */
	ACT_sys			*activitySystem;
	
				/* Maximum formatted length (\0 not counted) for reported numeric values.
				 * Values needing longer format should not occur (accept precision loss). */
	static const int	REPORT_NUMERIC_MAXFMT;

				/* Maximum formatted length (\0 not counted) for reported string values;
				 * longer strings are truncated and have an ellipsis "..." to indicate it. */
	static const int	REPORT_STRING_MAXFMT;   //MUST be > REPORT_NUMERIC_MAXFMT

	static blist		&theMWobjectRegistration();

protected:
				/* blist of DUMB bpointernodes pointing to THIS; used in remodelingHasOccurred()
				 * to keep track of which resource displays should be updated after remodeling. */
	static blist		&theResourceDisplayRegistration();
	
	//*** SEE MW_widgetdraw for inherited DS_graph "id" and rdAxisForm_wrapper
	
				// Click-pops-up-info event-handler callback (registered with motif_wrapper)
	static void		popup_at_timeCB(Widget, callback_stuff*, void*);
	
				// Right-mouse-button click pops up menu; last item yields backgrd-pts popup
	static void		rdOptionsMenuCallback(Widget, callback_stuff*, void*);
	static void		popup_rd_optionsCB(Widget, callback_stuff*, void*);
	motif_widget		*rdOptionsMenuPopup;
	motif_widget		*manual_button, *auto_window_button, *auto_plan_button;
	motif_widget		*separator, *background_button;

public:
	/**
	 * Constructor needs to know:  Parent Activity System (so that the Resource
	 * System can "register" itself with it for panning and zooming updates as
	 * well as other interaction); Parent Form Widget (the area allocated by
	 * the Resource Display to the graph). No longer needed: System's Y-scroll bar,
	 * which is now created by the DS_scroll_graph class below this level.
	 */
	RD_sys(ACT_sys *actSys, motif_widget *form);
	~RD_sys();

	//
	// To support handling of layout files:
	//
	typedef struct {
		vector<Cstring>	res_names;
		vector<int>	heights;
		Cstring		source_file;
		int		lineno;
		} layout_stuff;

	const Cstring&		detectorName() const { return get_key(); }
	const Cstring&		fixerName() const { return get_key(); }
				// Overrides virtual method in DS_scroll_graph
	int			is_a_res_sys() { return 1; }
				// Required to support weShouldWork
	// int			isOutOfSync(infoList*);
	static synchro::problem* checkResPlotStaleness(synchro::detector*);
	static void		fixResPlotStaleness(synchro::problem*);
	synchro::problem*	detectSyncProblem();

				// Return percent of RD width (0% at left, 100% at right), based on time
	double			time_to_percent(const CTime &atime);
	void			setyscrollmode(const Cstring& theResource, RD_YSCROLL_MODE theMode);
	RD_YSCROLL_MODE 	getyscrollmode(const Cstring& theResource);
				// Add a resource. Should work the same way as legends.
	virtual apgen::RETURN_STATUS	addResource(const Cstring& resourcename, Cstring & any_errors);
				//works for 1 res. max per RD
	apgen::RETURN_STATUS		removeResource(const Cstring& theNameOfTheResource);
	virtual void		removeAllResources(); 
	void			record_time_and_duration_parameters();
				// delegates to MW_object now
	void			setModelingResolution (const Cstring& theResource, int resolution);
				// Delegates to MW_object now
	int			getModelingResolution (const Cstring& theResource);
				/* Report on the value(s) of all Resources at a given time (as known by
				 * the objects in this Resource System); return is #lines in report */
	virtual int		report_at_time(	const CTime	atime,
						Cstring		&report,
						MW_object	*theClickedItem);

	// *** Interface with Resource Modeler ***

	/* Update all Resources displayed by this Resource System (this method is
	 *  called AUTOMATICALLY when Modeler recomputes).  First, map (copy, with
	 *  any needed reformatting) data from each Resource's Single_variable_func
	 *  into DataSeries(s), and then do a draw().
	 */

				/* Used to be called 'flush()'; new term is consistent with new
				 * and improved DS system -- PFM */
	virtual void		configure_graphics(callback_stuff *);

				// Used by RES_exec to notify RD_sys's that they'd better clean up their act
	static void		remodelingHasOccurred();
	  
				// To support IO_write::writeLAYOUT()
	void			WriteLegendLayoutToStream(	std::ostream &,
								const Cstring &LayoutTag);
				// To support processing of resource layout directives
	static void		reorder_legends_as_per(
						const Cstring&		layout_id,
						const vector<Cstring>&	res_names,
						const vector<int>&	heights,
						const Cstring&		source_file,
						int			lineno);
				// ditto
	void			implement_layout_as_per(const layout_stuff&);
				// Standard Utility Method
				// !!
	Node			*copy() { return NULL; }

	static Cstring&		thePendingLayout();
	static tlist<alpha_string, Cnode0<alpha_string, layout_stuff> > list_of_tagged_layout_lists;
};

#endif /*RD_SYS_H*/
