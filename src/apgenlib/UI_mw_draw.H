/*	PROJECT		: APGEN
 *	SUBSYSTEM	: UI (MW sub-subsystem)
 *	FILE NAME	: UI_mw_draw.H
 *
 *
 *	ABSTRACT:
 *	
 *	Header file for those MW_* classes which enscapsulate the information
 *	needed to draw a single plot.
 *
 */

#ifndef  MW_DRAW_H
#define  MW_DRAW_H

#include <iostream>
#include "C_global.H"

#include "UI_dsconfig.h"
#include "IO_plot_util.H"
#include "UI_ds_timeline.H"


#define POS_EPSILON  0.00001	/* ==min FLT_EPSILON in ANSI C <float.h>     */

/*Note that HP <float.h> FLT_EPSILON 1.19209290E-07; can't find Sun <float.h>*/


//object which encapsulates the information needed to
//  draw a "simple plot" -- a single display of a single set of data.
//  This class will probably be used as only a meta-class.

class Rsource;
class RD_sys;
class mw_obj;

/* Motif Widget object class; used as high-level object for resource displays.
 * This class is used as a base class for the 'real' widget containers, which
 * are specialized to the various resource types (state, continuous etc.)
 * Member variables which describe Y-axis data ranges and Y-axis plot range
 * are provided ONLY for each of the derived classes, since their nature
 * depends on the particulars of each class.
 *
 * Scrolled resources: MW_object is analogous to LegendObject. We index them
 * based on the name of the resource that the MW_object displays, for fast
 * retrieval by clients (such as RD_sys).
 */
class MW_object : public vertically_scrolled_item { // Note: vertically_scrolled_item already inherits from bstringnode
protected:
				/* Points being displayed (normalized coordinates, i.e. 0.0-100.0, for X).
				 * Takes place of (implicit) storage in C.O. widgets.  Also, point count. */
	int     		norm_count ;
    				// Could use int's for Y in some subclasses ...
	double			*norm_x, *norm_y;
	Cstring			theOriginalName;
				// Is truncated for fuzzy resources.
	Cstring			the_name_to_display;
				// unit information (optional)
	Cstring			Units;
				// String that is actually displayed in the legend
	Cstring			actual_text;
				/* Maximum length for plotting of (newline-delimited portions of) sg_name;
				 *  longer strings are truncated and have ellipsis "..." to indicate it.  */
	static const int	RES_NAME_MAXPLOT;
				/* Stores one or two (for fuzzy resources) handles for the
				 * resource displayed in this widget. Contains bpointernodes. */
	tlist<alpha_void, RESptr> displayedResources;
    	
				/* Y-scrolling mode:  automatic modes (which disable scrollbar) versus
				 * manual mode (which enables scrollbar); see mode enum above. This should
				 * be disabled in view of scrolled resources. */
	RD_YSCROLL_MODE		yscrollmode;
	RD_sys			*theResSysParent;
	bool			IamStale;
	motif_widget		*the_form, *the_draw_area;

	// protected constructor (only derived classes should use)
	MW_object(	const Cstring&	name_to_display,
			Rsource*	handle1,
			const Cstring&	filename,
			int		default_height,
			RD_sys*		res_sis);
public:

	typedef enum {
		UNFORMATTED,
		TRUNCATED,
		FORMAT_OK
	} Formatting;

	apgen::RETURN_STATUS	synchronize_data_with_resource(
					tlist<alpha_time, unattached_value_node, Rsource*>&	theValues,
					CTime&								time_resolution,
					bool&								new_values_found,
					Cstring&							errors_found) ;
	// public destructor
	~MW_object() ;

	mw_obj*			theObject;

	bool			have_error_high;
	bool			have_warn_high;
	bool			have_warn_low;
	bool			have_error_low;

				// to store versions compared with those in RES_history
	long			my_res_version;
    				// Get handle of displayed resource
	Rsource*		get_resource_handle() ;
				// gets resource values etc.
	apgen::RETURN_STATUS	refresh_all_internal_data(Cstring &anyErrors) ;
				// gets mode
	RD_YSCROLL_MODE		getyscrollmode() const ;
				// sets mode
	void			setyscrollmode(RD_YSCROLL_MODE) ;

				/* (Migrated from RD_sys)
				 * Next 2 methods only work with the modeling granularity of this Resource
				 * System (i.e. number of background points in the current time window).
				 * DSG 97/11/12:  getModelingResolution returns approx. # background pts,
				 * to match setModelingResolution's input (was giving Resolution in sec) */
	void			setModelingResolution (int resolution, const CTime &res_sys_span);
	int			getModelingResolution (const CTime &res_sys_span);
	void			setStaleFlag(bool value) { IamStale = value; }
	virtual bool		is_continuous() = 0;
	virtual bool		new_events_are_present() { return false; }

				// for handling telemetry from gmsec:
	virtual void		add_external_event(const CTime_base &event_time, double &res_value) {}

				// requesting that toggle_button exists guarantees that this is visible:
	bool			is_stale() const { return toggle_button && IamStale; }

	RD_sys			*get_res_sys() const { return theResSysParent; }

				/* GET various minimum+span pairs (no public methods to SET the values).
				 * bool return indicates whether both values are set (and retrievable).
				 * Methods are defined as no-ops, in line with meta-class use of MW_object.
				 */
	virtual bool		getyminspan(double& min, double& span)     //actual plotted
					{ double dummy = min; dummy = span; return FALSE ; }
	virtual bool		getyupdate_minspan(double& min, double& span)//last-updated
					{ double dummy = min; dummy = span; return FALSE ; }
	virtual bool		getymodeled_minspan(double& min, double& span)//all modeled
					{ double dummy = min; dummy = span; return FALSE ; }
	virtual bool		getyrange_minspan(double& min, double& span) //entire range
					{ double dummy = min; dummy = span; return FALSE ; }

				// Get(i.e. retrieve from member vars) major & minor Y-axis tick increments.
	virtual bool		getyincrements(double& major, double& minor)
					{ double dummy = major; dummy = minor; return FALSE ; }

				/* Compute from ymin & yspan, the major & minor Y-axis tick increments,
				 * and save them in member variables.  */
	virtual bool		compute_yincrements(double & major, double & minor)
					{ double dummy=major; dummy=minor; return FALSE ; }

				/* Updates data (Model) using reference to function values, plus domain
				 * start and span (and % of plotted domain those map to--usually 0%,100%).
				 * The second argument is to support fuzzy resources that are modeled in
				 * terms of two regular resources. In the non-fuzzy case that list is empty.
				 * In the fuzzy case the list contains the max values; the first list contains
				 * the min values. */
	virtual Formatting	update_data(tlist<alpha_time, unattached_value_node, Rsource*>&	values,
						CTime startval,
						double startpct,
						CTime spanval,
						double spanpct,
						Cstring& errors_found) ;

	    			/*
				 * lookup range (Y) for a given domain (X) value (0% to 100%), using the
				 * ACTUAL DATA within the Widget object, which produces the visible plot;
				 * lookup fails if outside data domain (i.e. Y undefined and unplotted)
				 */
	virtual apgen::RETURN_STATUS	lookup_yvalue(const double xval, double &yval);

				// format a looked-up Y value (this depends on what derived class requires)
	virtual Formatting	format_yvalue(const double	/*yval*/,
					 const int		/*maxchar*/,
					 Cstring		&/*ystring*/) {
		return FORMAT_OK;
	}

				// format a DIFFERENCE of looked-up Y values
	virtual Formatting	format_ydelta(const double	/*ydelta*/,
					 const int		/*maxchar*/,
					 Cstring		&/*ystring*/) {
		return FORMAT_OK;
	}

				// update_data() and others call this to change Y-axis limits
	virtual void		adjust_yaxis_limits(bool do_pad, double min, double span) ;

    				//  Migrated here from RD_sys, where it sat previously.
	void			adjust_yaxis() ;


				/*  Absolute pan/zoom for any/all Resources with continuous numeric values
				 (derived classes' methods will "know" how/whether to do this action) */
	virtual Formatting	yabspanzoom (double min, double span);

				/* (migrated from RD_sys)
				 * Get min+span for FIRST Resource, IF it has continuous numeric values
				 * ... changed to strings so we can format them properly (PFM 12/2000).
				 * Also: method used to be virtual; no need for that.
				 */
	apgen::RETURN_STATUS		ygetminspan(Cstring &min, Cstring &span) ;
				//  get double min, span values
	// apgen::RETURN_STATUS		ygetminspan(double &min, double &span) ;

				/*  To support redrawing the screen or plotting to PostScript. */
	virtual void		refresh_legend(Device &) ;

    				/*  support drawing to Motif or PostScript. */
	virtual void		refresh_canvas(	Device &,
						const int pages_per_display = 1,
						const int page = 1) ;

				// for refreshing screen after telemetry events have been received:
	virtual void		refresh_events(Device &) {}

				//  To support IO_plot.C
	void			plot_graphics(Cstring &,
					int, int, int,
					int, int, int,
					int, int, double) ;

	/* Methods that are pure virtual in vertically_scrolled_item
	 * (i. e., which implement the vertically_scrolled_item interface): */
	int			get_official_height() const ;
	const Cstring		&get_theoretical_label() ;
	const Cstring		&get_units() const { return Units ; }

    				//  Virtual in vertically_scrolled_item
	motif_widget		*get_toggle_button_for(motif_widget *) ;

				/*  Given the top of the legend hierarchy, the object must
				 * figure out which descendant is the DrawArea (if any) */
	motif_widget		*get_drawarea_widget() { return the_draw_area ; }

				//  To support redrawing the legend
	void			expose_callback() ;

    				/*  Pure virtual in base class. Signature changed 1/9/03 to
				 * reflect location & height instead of Motif arguments. */
	motif_widget		*create_scrolled_item(	int 		Index,
							VSI_owner	*vo,
							int		y,
							int		h
							) ;

	// void			adjust_one_scrolled_item(VSI_owner *, motif_widget *) ;
	};


extern MW_object	*create_an_mw_object(
					const Cstring	&theResource,
					const Cstring	&filename,
					Cstring&	any_errors,
					RD_sys		*res_sis);


	/* MW_discrete is an object for displaying state resources.
	 State resources have a discrete-valued range (i.e. with multiple distinct values, and
	 having no possible meaning to being "between" any two such values).  The
	 values MAY or MAY NOT have an "ordering"; also, the values MAY or MAY NOT
	 require a "mapping" from underlying non-numeric range values (e.g. String,
	 bool) to numeric values (was required by Chart Object).

	 If there is NO "ordering", then a "sensible" pseudo-ordering of the mapping
	 must be specified (???how), so as to determine Y-axis tick-mark placement.
	 (And, if there IS a (numeric) "ordering", then no mapping is needed.)

	 Maybe we could derive MW_discrete_mapped for non-ordered == mapped range case.
	*/
class MW_discrete : public MW_object {
protected:
    /* List of all defined discrete values, each value as a String_node.  Order
     * of values in the list is used for display purposes (i.e. zeroth node's
     * value appears nearest origin, and last node's value appears farthest). */
    List discrete_values ;

    /* Maximum length (not counting \0) of label strings for discrete values.
     * Formatted int or float values count as strings for this limitation. */
    static const int DISCRETE_LABEL_MAXLEN ;
    /* Maximum length (not counting \0) of label strings for plotting.
     * FUTURE better way is to determine dynamically with an
     * XmString using the label font, and comparing with pixels available.
     * Formatted int or float values count as strings for this limitation. */
    static const int DISCRETE_LABEL_MAXPLOT ;
    /* Minimum length (not counting \0) of space for label strings for discrete
     *  values (a string can be shorter; this is the shortest allocated space). */
    static const int DISCRETE_LABEL_MINSLOT ;

    /* ACTUAL plotted Y-axis minimum and span (use double, so can fine-tune the
     *  plotted positions of discrete values, which are integers 0-(count-1)).  */

	double  ymin, yspan;

    bool yset ;		//are these 2 values set?

    /* Minimum & span of Y-values, within the last-updated time window.  ???If
     *  update doesn't cover entire window, then refers to updated part only???
     *  Y-values are integers >= 0. The value count is just (yupdate_span + 1).  */
    int     yupdate_min, yupdate_span ;

    bool yupdate_set ;	//are these 2 values set?

    /* Minimum & span of Y-values, within the entire modeled set of values.
     * Y-values are integers>=0.  The value count is just (ymodeled_span + 1). */
    int     ymodeled_min, ymodeled_span ;

    bool ymodeled_set ;	//are these 2 values set?

    /* Minimum & span of all possible Y-values, is info within discrete_values,
     * so no separate set of "yrange_" variables is needed.
     * 
     * Computed major (NO minor) tick increments (were stored in C.O. widgets). */
    double  yincr_major ;	// getyincrements(),compute_yincrements()
    bool yincr_set ;		//is this value set?

public:
	/* Need a name.  ALSO, need an arg containing a list of range values, whose
	 * order is RETAINED (since there MAY be an ordering). */
	MW_discrete(	const Cstring	&name_to_display,
			Rsource*	handle,
			const Cstring&	filename,
			int		default_height,
			RD_sys*		res_sis,
			vector<TypedValue>& range_values // list of TDnodes (PFM)
			);
	~MW_discrete();

				/* GET various minimum+span pairs (no public methods to SET the values).
				 * bool return indicates whether both values are set (and retrievable).
				 * By convention, the caller allocates all reference args.
				 * actual plotted */
	virtual bool		getyminspan(double& min, double& span) ;
	virtual bool		getyupdate_minspan(double& min,double& span);// last-updated
	virtual bool		getymodeled_minspan(double& min,double& span);// all modeled
	virtual bool		getyrange_minspan(double& min,double& span); //entire range

				// Get(i.e. retrieve from member vars) major & minor Y-axis tick increments.
	virtual bool		getyincrements(double& major, double& minor) ;

	    			/* Compute from ymin & yspan, the major & minor Y-axis tick increments,
				 * and save them in member variables. */
	virtual bool		compute_yincrements(double& major, double& minor);

	bool			is_continuous() {
		return false;
	}

				/* Updates data (Model) using reference to function values, plus domain
				 * start and span (and % of plotted domain those map to--usually 0%,100%)
				 * Changed to reflect switch from Single_variable_func to blist of unattached
				 * value nodes (PFM). */
	virtual Formatting	update_data(
		    			  tlist<alpha_time, unattached_value_node, Rsource*>&	values,
					  CTime			startval,
					  double		startpct,
					  CTime			spanval,
					  double		spanpct,
					  Cstring&		theErrors);

				/* use MW_object method as long as it works for ALL derived classes!!!
				 * virtual apgen::RETURN_STATUS lookup_yvalue(const double xval, double& yval) ;
				 * format a looked-up Y value, based on requirements of this class */
	virtual Formatting	format_yvalue(const double yval, const int maxchar, Cstring&  ystring);

	virtual Formatting	format_ydelta(const double ydel, const int maxchar, Cstring&  ystring);

				// Update_data() and others call this to change Y-axis limits.
	virtual void		adjust_yaxis_limits(bool do_pad, double min, double span);

				/* Absolute pan/zoom for any/all Resources with continuous numeric values
				 * (derived classes' methods will "know" how/whether to do this action) */
	virtual Formatting	yabspanzoom (double min, double span);

				// !!
	Node*			copy() { return NULL; }

	// *** FOLLOWING BASED UPON UI_state_panel.H class State_panel ***

				/* For drawing with Motif/PostScript. */
	void			refresh_legend(Device &);

				/* For drawing with Motif/PostScript. Default values
				 * of the arguments are used in the Motif case. */
	void			refresh_canvas(
					Device & ,
					const int pages_per_display = 1,
					const int page = 1);

	friend MW_object	*create_an_mw_object(
					const Cstring	&theResource,
					const Cstring	&filename,
					Cstring&	any_errors,
					RD_sys		*res_sis);
	};

class resCurve {
public:
	resCurve() : norm_count(0),
			norm_x(NULL),
			norm_y(NULL) {}
	~resCurve() {
		if(norm_x) {
			delete [] norm_x; }
		if(norm_y) {
			delete [] norm_y; } }

	double*		norm_x;
	double*		norm_y;
	int		norm_count;
};

typedef Cntnr<alpha_string, resCurve> resCurveNode;


/*	MW_continuous is an object that contains a simple plot
	which has a continuous-valued, ordered range (i.e. with a theoretical
	continuum of values, even if only a subset of that continuum is usable by
	actual values).  Thus, there is validity and meaning to being "between" any
	2 (plotted) values, and thus sloped graphical interpolation is appropriate.
	Essentially, only numeric (integer or real values, or subsets thereof)
	ranges satisfy this, so the issue of "mapping" of values does not come up. */

class MW_continuous : public MW_object {
protected:
		    /* Maximum length (\0 not counted) of space for formatted continuous numeric
		     * values (value can use up to this many chars;i.e. max. allocated space).  */
	static const int CONTINUOUS_VALUE_MAXFMT;
		    /* Maximum length (not counting \0) of formatted label values for plotting.
		     * FUTURE better way is to determine dynamically with an
		     * XmString using the label font, and comparing with pixels available. */
	static const int CONTINUOUS_VALUE_MAXPLOT;
		    /* Minimum length (\0 not counted) of space for formatted continuous numeric
		     * values (value can use fewer chars; this is shortest allocated space). */
	static const int CONTINUOUS_VALUE_MINFMT;

	double		*event_x, *event_y;
	int		event_length;

			// ACTUAL plotted Y-axis minimum and span.
	double 		ymin, yspan;
	bool		yset;	

			/* Minimum & span of Y-values, within the last-updated time window.  ???If
			 * update doesn't cover entire window, then refers to updated part only??? */
	double 		yupdate_min, yupdate_span;
	bool		yupdate_set;	//are these 2 values set?

			// Minimum & span of Y-values, within the entire modeled set of values.
	double  	ymodeled_min, ymodeled_span;

			// To support fuzzy modeling (tells the widget whether to draw a yellow band)
	bool		things_are_fuzzy;
	bool		ymodeled_set;	//are these 2 values set?

	double		*norm_error_high, *norm_warn_high, *norm_warn_low, *norm_error_low;
	slist<alpha_string, resCurveNode>	auxResources;

	/* Minimum & span of all possible Y-values cannot be computed (any value
	 * representable by the machine is possible), so instead keep a running
	 * minimum & span (sum yields maximum) across successive modelings. */
	double		yrange_min, yrange_span;
	bool		yrange_set;	//are these 2 values set?

			// Computed major & minor tick increments (formerly stored in C.O. widgets).
	double 		yincr_major, yincr_minor; // getyincrements(),compute_yincrements()
	bool		yincr_set;		//are these 2 values set?

			// Maximum-value threshold (as primitive "resource-centric" constraint)
	double		ythreshold_max;
	bool		ythreshold_max_set;	//is this value set?

			// Minimum-value threshold (as primitive "resource-centric" constraint)
	double		ythreshold_min;
	bool		ythreshold_min_set;	// is this value set?

			// time-ordered list of external events:
	blist		ListOfEvents;

public:
	//  Constructor
	MW_continuous(	const Cstring	&name_to_display,
			Rsource		*handle1,
			const Cstring&	filename,
			int		default_height,
			RD_sys		*res_sis);
	~MW_continuous();

				/* GET various minimum+span pairs (no public methods to SET the values).
				 * bool return indicates whether both values are set (and retrievable).
				 * By convention, the caller allocates all reference args.  */
	virtual bool		getyminspan(double& min, double& span);   // actual plotted
	virtual bool		getyupdate_minspan(double& min,double& span);// last-updated
	virtual bool		getymodeled_minspan(double& min,double& span);// all modeled
	virtual bool		getyrange_minspan(double& min,double& span); //entire range

				// Get(i.e. retrieve from member vars) major & minor Y-axis tick increments.
	virtual bool getyincrements(double& major, double& minor);
	//Compute from ymin & yspan, the major & minor Y-axis tick increments,
	//   and save them in member variables.
	virtual bool compute_yincrements(double& major, double& minor);

	//updates data (Model) using reference to function values, plus domain
	//  start and span (and % of plotted domain those map to--usually 0%,100%)
	//  Changed to reflect switch from Single_variable_func to blist of unattached_value_nodes (PFM):
	virtual Formatting	update_data(tlist<alpha_time, unattached_value_node, Rsource*>&	values,
						CTime	startval,
						double	startpct,
						CTime	spanval,
						double	spanpct,
						Cstring	&any_errors);
	void			refresh_events(Device &);

				/* use MW_object method as long as it works for ALL derived classes!!!
				 * virtual apgen::RETURN_STATUS lookup_yvalue(const double xval, double& yval);
				 * format a looked-up Y value, based on requirements of this class
				 */
	virtual Formatting	format_yvalue(const double yval, const int maxchar, Cstring&  ystring);
	virtual Formatting	format_ydelta(const double ydel, const int maxchar, Cstring&  ystring);

				// update_data() and others call this to change Y-axis limits
	virtual void		adjust_yaxis_limits(bool do_pad, double min, double span);

				/* Absolute pan/zoom for any/all Resources with continuous numeric values
				 * (derived classes' methods will "know" how/whether to do this action) */
	virtual Formatting	yabspanzoom (double min, double span);
	bool			is_continuous() { return true; }
	bool			new_events_are_present() { return event_length < ListOfEvents.get_length(); }

				// !!
	Node			*copy() { return NULL; }

	//  *** FOLLOWING BASED UPON UI_state_panel.H class State_panel ***

	void			refresh_legend(Device &);
	void			refresh_canvas(
					Device &,
					const int pages_per_display = 1,
					const int page = 1);

	apgen::RETURN_STATUS	handle_one_res_plot(
					tlist<alpha_time, unattached_value_node, Rsource*>& values,
					CTime			startval,
					double			startpct,
					CTime			spanval,
					double			spanpct,
					resCurveNode*		res_node,
					Cstring&		any_errors);

	/* To support fuzzy modeling. This function is called twice from refresh_canvas
	 * when the resource to be displayed is fuzzy; a yellow band will appear in the
	 * zone of uncertainty beween min and max resource values. */
	void draw_one_curve_and_fill(	int		theCount,
					int		theWidth,
					int		theHeight,
					int		pages_per_disp,
					int		page,
					Device&		dev,
					int		min_val,
					int		max_val,
					sr_DRAW_STYLE	the_style_to_use_for_filling);

	void		draw_one_curve(	int		theCount,
					double*		res_norm_x,
					double*		res_norm_y,
					int		theWidth,
					int		theHeight,
					int		pages_per_disp,
					int		page,
					Device&		dev,
					int		min_val,
					int		max_val);

	void	draw_external_points(int count, int theWidth, int theHeight, Device &dev, sr_DRAW_STYLE fill_style);

				// for handling telemetry from gmsec:
	void	add_external_event(const CTime_base &event_time, double &res_value);

	friend MW_object	*create_an_mw_object(
					const Cstring	&theResource,
					const Cstring	&filename,
					Cstring&	any_errors,
					RD_sys		*res_sis);
};


#endif /* MW_DRAW_H */
