#ifndef DS_TIMELINE_H
#define DS_TIMELINE_H

#include <iostream>

#include "C_global.H"
#include "UI_ds_draw.H"
	// for ACTVERUNIT:
#include "UI_dsconfig.h"
#include "UI_motif_widget.H"
#include "UTL_defs.H"
#include "UTL_time.H"

#include <map>

#define BUTTON_1_DOWN	1
#define BUTTON_2_DOWN	2
#define BUTTON_3_DOWN	4


#define ONEDAY        (24*60*60)
#define TWODAY        (2*ONEDAY)


#define NOPMODE		1                   //DISPLAY is in NOP mode
#define PASTEMODE	2                   //Display is in PASTE mode
#define DEFMODE		3                   //Display is in DEFINE mode
#define SELMODE		4                   //Display is in SELECT mode
#define MOVEMODE	5                   //Display is in MOVE mode
#define EXPANSIONMODE	6                   //Display is in EXPANSION mode (re-sizing one of the legends)

#ifndef GUI
typedef void * Window;
typedef void * XButtonEvent;
typedef void * XEvent;
typedef void * XmDrawingAreaCallbackStruct;
void XClearArea();
#endif

class ACT_sys;
class DS_graph;
class DS_legend;
class DS_scroll_graph;
class DS_timemark;
class list_of_siblings;
class MW_object;
class Schedule_def;
	// VSI = vertically scrolled item
class	VSI_owner;

#ifdef GUI
class Extensible_popup_menu;

/* Base class that handles adaptation-driven menu pop-ups when the user right-clicks
 * on the activity display. DS_lndraw (just under ACT_sys) uses this as a base class. */
class MenuPopupSelectable
{
public:
  // they have been selected
  virtual void MenuPopupSelected(	Extensible_popup_menu	*menu,
		  			const Dsource	*act,
					int			x,
					int			y,
					const Cstring&		buttonName,
					const Cstring&		legendName,
					const CTime_base	&time) = 0;
};
#endif

/* The term "siblings" in the Display Subsystem (DS) context refers to a group
 * of motif widgets that respond in unison to horizontal scrolling requests, and
 * contain two subgroups, each one of which is controlled by a separate vertical
 * scrollbar: the activity display subgroup and the resource display subgroup.
 *
 * Information that helps members of the (sub)group figure out what to do e. g.
 * when the user moves a scrollbar is held in a class called "list_of_siblings"
 * (see below). It is derived from List and contains pointer_to_siblings Nodes,
 * one for each member of the group of siblings. */
class pointer_to_siblings: public bpointernode {
private:
				/* Any (activity) windows to redraw will be stored in this clist.
				 * Only used in LegendObject subclass. */
	clist			theWindowsToRedraw;

				//  Height of this object.
	int			theHeight;

				/* Distance to the top of the containing widget. of this object.
				 * Note that this only makes sense for vertically scrolled objects
				 * that are visible, i. e., which are pointed to by one pointer
				 * in the pointersToTheVisibleLegends blist of the appropriate
				 * VSI_owner. */
	int			theDistanceToTheTop;

				/* Integer array for storing local number of overlapped
				 * activities. Used to compute staggering pattern. */
	int			*stagger_bits;

				/* x in pixel units for computing staggering pattern.
				 * Duplicate of theHeight? (sounds likely) */
	int			number_of_visible_pixels;
public:
	//  constructor
	pointer_to_siblings(VSI_owner *ls, int i)
		: bpointernode((void *) ls, NULL),
		theHeight (i),
		theDistanceToTheTop (-1),
		theWindowsToRedraw(compare_function(compare_int_nodes, false)),
		stagger_bits(NULL),
		number_of_visible_pixels(0)
		{}

	//  copy constructor
	pointer_to_siblings(const pointer_to_siblings & ps)
		: bpointernode(ps),
		theWindowsToRedraw(ps.theWindowsToRedraw),
		stagger_bits(NULL),
		theHeight (ps.theHeight),
		number_of_visible_pixels(ps.number_of_visible_pixels) {}
	//  destructor
	~pointer_to_siblings() {}

	Node		*copy() { return new pointer_to_siblings(*this); }

	friend class vertically_scrolled_item;
	};

	/* From a design pattern perspective, this class fulfills a number of responsibilities.
	 */
class vertically_scrolled_item : public bstringnode, public back_pointer {
protected:

				/* blists of pointer_to_siblings indexed by VSI_owner address; this allows us
				 * to store one pointer per VSI_owner. Actual access is through the VSI_owner
				 * methods below. */
	blist			theActualHeights;
public:
	//  Constructor
	vertically_scrolled_item(
			const Cstring	&name,
			const Cstring	&filename,
			int		preferred_height)
		: bstringnode(name),
		selection(0),
		theActualHeights(compare_function(compare_bpointernodes, false)),
		apf(filename),
		toggle_button(NULL),
		preferredHeight(preferred_height) {}
		
	//  Copy Constructor
	vertically_scrolled_item(const vertically_scrolled_item &vs)
		: bstringnode(vs),
		selection(vs.selection),
		theActualHeights(compare_function(compare_bpointernodes, false)),
		apf(vs.apf),
		toggle_button(NULL),
		preferredHeight(vs.preferredHeight) {}
	//  Destructor
	~vertically_scrolled_item() {}

		/* ------ items that belong in the "vertically_scrolled_item" base class ------ */

				//  File in which defined, or "New". Set by the constructor.
	Cstring			apf;
	int			selection;

				/* This height defaults to 32 (pixels) and is saved/restored to/from
				 * APF files. */
	int			preferredHeight;
	motif_widget		*toggle_button;

	// methods
	void			adjust_height(int new_height);
	virtual motif_widget	*create_scrolled_item(	int		index,
							VSI_owner	*the_parent,
							int		y,
							int		h
							) = 0;
	clist			&get_clist_of_windows_for(VSI_owner *);
	static Cstring		get_label_text_from(motif_widget *toggle_button_or_res_legend);
	virtual int		get_official_height() const = 0;
	int			get_selection() const { return selection; }
	int			*get_stagger_info_for(VSI_owner *);
	virtual const Cstring	&get_theoretical_label() = 0;
	virtual motif_widget	*get_toggle_button_for(motif_widget *toggle_button_or_res_legend) = 0;
	int			get_vertical_size_in_pixels_in(const VSI_owner *) const;
	double			get_vertical_size_in_abstract_units_in(const VSI_owner *vsio) const {
					return (double) get_vertical_size_in_pixels_in(vsio); }
	void			set_stagger_info_for(VSI_owner *, int *newval);
	void			set_vertical_size_in_pixels_in(VSI_owner *, int newval);
	bool			there_are_windows_to_redraw_for(const VSI_owner *) const;
	void			update_windows_in(
					VSI_owner *,
					int old_startx,
					int old_endx,
					int top_y,
					int bottom_y);
	friend class Lego;
	};

class Lego;

	/* A class that implements the "abstract" legend object; there is only one for each legend
	 * in the core database. It provides support for the motif_widgets that implement the
	 * graphic instantiation of legends on a specific ACT_sys display object. */
class VerticalThing : public vertically_scrolled_item {
protected:
				/* Version of the legend name that is truncated (with ... added)
				 * if necessary.
				 *
				 * It's not public because we really can't tell how much text needs to be
				 * truncated until after the font is known, and that requires instantiating
				 * the widget. So, clients go through an access function that checks the
				 * displayset flag for whether font-related data needs to be initialized.
				 * After that, the access function's role becomes trivial. */
	Cstring			actual_text;
	Lego			*myLego;
public:
	VerticalThing(	const Cstring	&ltext,	// text of the legend
			const Cstring	&filename,	// file in which legend was defined (or NEW)
			int		preferred_height, // defaults to 32
			Lego		*parent
			);
	VerticalThing(const VerticalThing &);
	~VerticalThing() {}

	Node			*copy() { return new VerticalThing(*this); }

	Lego			*get_lego() { return myLego; }
	motif_widget		*create_scrolled_item(	int		index,
							VSI_owner	*the_parent,
							int		y,
							int		h
							);
	int			get_official_height() const;
	const Cstring		&get_theoretical_label();
	motif_widget		*get_toggle_button_for(motif_widget *better_be_a_toggle_button); };

class Lego: public LegendObject {
protected:
	Lego(const Cstring &text, const Cstring &filename, int preferred_height);
public:
	Lego(const Lego &L)
	: LegendObject(L)
		{ vthing = new VerticalThing(*L.vthing); }
	~Lego();

	VerticalThing	*vthing;
	static Cstring	get_label_text_from(motif_widget *toggle_button_or_res_legend);
	void		adjust_height(int new_height) {
				vthing->adjust_height(new_height); }
	motif_widget	*create_scrolled_item(	int		index,
						VSI_owner	*the_parent,
						int		y,
						int		h) {
				return vthing->create_scrolled_item(index, the_parent, y, h); }
	clist		&get_clist_of_windows_for(VSI_owner *v) {
				return vthing->get_clist_of_windows_for(v); }
	int		get_official_height() const {
				return vthing->get_official_height(); }
	int		get_selection() const {
				return vthing->selection; }
	int		*get_stagger_info_for(VSI_owner *v) {
				return vthing->get_stagger_info_for(v); }
	const Cstring	&get_theoretical_label() {
				return vthing->get_theoretical_label(); }
	motif_widget	*get_toggle_button_for(motif_widget *better_be_a_toggle_button) {
				return vthing->get_toggle_button_for(better_be_a_toggle_button); }
	int		get_vertical_size_in_pixels_in(const VSI_owner *v) const {
				return vthing->get_vertical_size_in_pixels_in(v); }
	double		get_vertical_size_in_abstract_units_in(const VSI_owner *vsio) const {
				return (double) get_vertical_size_in_pixels_in(vsio); }
	void		set_selection(int k) {
				vthing->selection = k; }
	void		set_stagger_info_for(VSI_owner *v, int *newval) {
				vthing->set_stagger_info_for(v, newval); }
	void		set_vertical_size_in_pixels_in(VSI_owner *v, int newval) {
				vthing->set_vertical_size_in_pixels_in(v, newval); }
	bool		there_are_windows_to_redraw_for(const VSI_owner *v) const {
				return vthing->there_are_windows_to_redraw_for(v); }
	void		update_windows_in(VSI_owner *v, int old_startx, int old_endx, int top_y, int bottom_y) {
				vthing->update_windows_in(v, old_startx, old_endx, top_y, bottom_y); }

	static LegendObject	*theLegoConstructor(
					const Cstring &text, const Cstring &filename, int preferred_height);
	};


	/* List_of_siblings (derived from List) contains Nodes of the sibling type (see below).
	 * The purpose of the list_of_siblings class is to (1) provide a systematic way of propagating
	 * scrolling information between the various high-level DS_objects: DS_legend, ACT_sys (i. e.,
	 * DS_lndraw), DS_timemark, CON_sys and RD_sys. (2) provide a central "point of contact" for
	 * utilities such as draw_vertical_cursor() and erase_vertical_cursor() that affect all siblings
	 * within a "co-scrolling group" but only that group.
	 *
	 * All methods of this class are implemented in UI_ds_timeline.C.
	 * 
	 * Update 10/30/2002: Retrofitting this class to make use of the "vertically_scrolled_item"
	 * class, used as a base class by both DS_ and MW_ subsystems. The idea is to absorb what is
	 * generic in the scrolling functionality of DS_legend and put it in a base class.
	 *
	 * Unfortunately, we cannot
	 * just derive DS_legend from the new base class, because there is no equivalent class for the
	 * MW_ subsystem. Although wait... how about MW_legend? I kind of like it. OK, sold. Update
	 * UI_mw_timeline.H to include that.
	 */
class list_of_siblings: public List {
public:
	//  constructor
	list_of_siblings(	motif_widget *parent,
				CTime_base desired_start_time,
				CTime_base desired_time_span);
	//  destructor
	~list_of_siblings();

				//  Provides quick access to a widget that determines sibling visiblility.
	motif_widget		*common_parent;

				//  Used by DS_scroll_graph for handling horiz. scrolling.
	DS_timemark		*timemark;

				/* You would expect this method to call the method of the same name on each of its
				 * siblings. But that's not what it does; the sibling::isOutOfSync() method is the
				 * one that does that. See the discussion in the comments for weShouldWork()
				 * in IO_seqtalk.C. */
	int			isOutOfSync(infoList*);

		/* The data members and methods below are organized into 2 groups: items intended for
		 * managing the horizontal scrolling of objects, and items intended for managing the
		 * vertical scrolling. Note that ALL sibings behave as a single object for horizontal
		 * scrolling while for vertical scrolling, each RD_sys can behave independently of
		 * the ACT_sys and of each other. */

		//----------------- HORIZONTAL -----------------//
private:
				/* Transition diagram for Dwtimestart, Dwtimespan:
				 *
				 * 	"from" state		"to" state		caused by
				 * 	============		==========		=========
				 *	initial state	->	time params agree	First invocation
				 *				with Plan params	of Dwtimestart()
				 *							or Dwtimespan()
				 *
				 *	regular (any)	->	new regular state	execute_client_request()
				 *	state
				 *
				 * Clients that cause transitions:
				 *
				 *    - theWorkProc invokes execute_client_request() (only client of that method)
				 *    - sibling nodes invoke Dwtimestart(), Dwtimespan() when probed by
				 * 		weShouldWork() via the nodes' isOutOfSync() method and
				 * 		also whenever they are created, exposed or reconfigured
				 */
	CTime_base		Dwtimestart, Dwtimespan;

				/* Transition diagram for UserRequestedStart, UserRequestedSpan:
				 *
				 * 	"from" state		"to" state		caused by
				 * 	============		==========		=========
				 *	undefined state	->	a_client_has... flag	request_a_change_in
				 *				set; start/span set	_time_parameters()
				 *				to user values.
				 *
				 *	a_client_has...	->	undefined state		execute_client_request()
				 *	flag set
				 *
				 * Clients that cause transitions:
				 *
				 *     - theWorkProc invokes execute_client_request() (only client of that method)
				 *     - ACT_sys::panDisplaysThatWereNeverScrolled(),
				 * 	   UI_mainwindow::allocateActivityDisplay() (when creating display),
				 * 	   UI_mainwindow::panSelectedDisplays() (while remodeling in real time),
				 * 	   UI_mainwindow::zoomPanDisplay() (when processing a NEW_HORIZON request)
				 * 	   invoke request_a_change_in_timing...()
				 */
	CTime_base		UserRequestedStart, UserRequestedSpan;

				/* Flag meaning that UserRequestedStart/Span hold valid values; see
				 * transition diagram associated with UserRequestedStart/Span. */
	int			a_client_has_requested_a_time_change;

public:


				/* This really belongs to the "horizontal" capabilities of the class,
				 * in spite of the name. */
	int			verticalCursorEnabled;

				/* This really belongs to the "horizontal" capabilities of the class,
				 * in spite of the name. */
	void			draw_vertical_cursor(int pos);

				/* This really belongs to the "horizontal" capabilities of the class,
				 * in spite of the name. */
	void			erase_vertical_cursor();

				/* Nontrivial access function that initializes time parameters
				 * to Plan values the first time around. Note: the method knows
				 * that this needs to be done because the constructor initializes
				 * the start/span to negative values. */
	const CTime_base	&dwtimestart();

				//  See comment for dwtimestart().
	const CTime_base	&dwtimespan();

				//  For clients who want to scroll the whole set of display siblings.
	void			request_a_change_in_time_parameters(	const CTime_base &start_time,
									const CTime_base &time_span);

				/* Resets the a_client_has_requested_a_time_change flag and sets the
				 * official time/span parameters to the client-requested ones. */

	void			execute_client_request(); };

	/* A class that is appropriate for containers of vertically scrolled items. */
class VSI_owner {
private:
				/* Transition diagram for Y_offset_in_pixels_for_act:
				 *
				 * 	"from" state		"to" state		caused by
				 * 	============		==========		=========
				 *	any state	->	new state	set_vertical_offset_to
				 *
				 * Client that causes transitions:
				 *
				 * 	- ACT_sys::do_the_vertical_scrollbar_thing(), invoked by theWorkProc
				 * 	  after ACT_sys::the_vertical_scrollbar_is_out_of_sync() reports TRUE
				 *
				 * NOTE: the change in Y_offset_in_pixels will be picked up by
				 * DS_legend::isOutOfSync() through its calculation of legend positions, which
				 * invokes list_of_siblings::vertical_distance_above_legend(), which uses
				 * y_offset_in_pixels(). However, there are not enough loops in theWorkProc
				 * to propagate this all the way through (the call to do_the_vertical_scrollbar_thing
				 * occurs to late for the legend objects to pick up the discrepancy). Therefore,
				 * set_vertical_offset_to() sets SomethingHappened to 1, so as to force a second
				 * pass through the isOutOfSync logic in weShouldWork.
				 *
				 * Change 11/03/02: use bpointernotes in blist indexed by DS_scroll_graph &.
				 */
	int			Y_offset_in_pixels;

	DS_scroll_graph		*This;
public:
	//  Constructor
	VSI_owner(DS_scroll_graph *dsgraph);
	//	: Y_offset_in_pixels(0),
	//	This(dsgraph),
	//	pointersToTheVisibleLegends(compare_function(compare_bpointernodes, false), dsgraph)
	//	{}

	//  Destructor
	~VSI_owner() {}
	int			y_offset_in_pixels() const;

				/* Used by ACT_sys::do_the_vertical_scrollbar_thing(), invoked by
				 * theWorkProc when ACT_sys::the_vertical_scrollbar_is_out_of_sync().
				 *
				 * See above comments on Y_offset_in_pixels for state transition info. */
	void			set_vertical_offset_to(int I);

				//  Identifies which vertically_scrolled_item the user clicked on.
	vertically_scrolled_item *findTheVSIat(	int				y_coord,
	 					vertically_scrolled_item	*startWithThisOne) const;

				/* Uses y_offsets_in_pixels_for_act() to help a LegendObject/MW_object
				 * compute its vertical position within a DS_legend. */
	int			vertical_distance_above_item(vertically_scrolled_item *vsi) const;

	void			evaluate_vertical_scrollbar_parameters(int &y_max, int &y_slider_size);
	static blist		empty_list;
	virtual blist		&get_blist_of_vertically_scrolled_items() const {
					return empty_list; }
	virtual vertically_scrolled_item	*get_first_vsi() const = 0;
	virtual vertically_scrolled_item	*get_next_vsi(vertically_scrolled_item *) const = 0;

				/* blist of SMART bptr_to_legend Nodes (derived from bpointernode)
				 * pointing to ONLY THOSE LEGENDS THAT ARE VISIBLE FOR THIS VSI_owner.
				 *
				 * Previously we used bptr_to_legend nodes, but that is spaghetti-ish
				 * as more classes are added to the design. Intead we use simple,
				 * smart bpointernodes to vertically-scrolled_items. The data that
				 * are specific to this VSI_owner can then be retrieved
				 * from the vertically_scrolled_item::theActualHeights blist. */
	blist			pointersToTheVisibleLegends;

	DS_scroll_graph		*get_this() const { return This; } };

//  Nodes stored in list_of_siblings.
class sibling: public Node {
private:
	DS_graph	*theGraph;
public:
	sibling(DS_graph *ds) : theGraph(ds) {}
	sibling(const sibling & S) : theGraph(S.theGraph) {}
	~sibling() {}
	Node		*copy() { return new sibling(*this); }
	DS_graph	*getTheGraph() const { return theGraph; }

	Node_type	node_type() const { return SIBLING; }
	const Cstring&	get_key() const {
		return Cstring::null();
	}
	int		isOutOfSync(infoList*);
	void		configure_graphics();
};

typedef struct {
	int sx,sy;
	int ex,ey;
	} LNDEF;


/* Base class for objects that belong to the Display Subsystem (DS).
 *
 *		OVERALL ORGANIZATION
 *		====================
 *
 * 1. General Description and Class Hierarchy
 * ==========================================
 *
 * Instances of the classes defined here are owned by one of two high-level APGEN objects:
 * activity_display and resource_display. Both of these are frame widgets whose parents
 * are individual paned window widgets in APGEN's main window.
 *
 * Both activity_display and resource_display own a form widget which is the basic,
 * high-level form used for the layout of the act and res displays:
 *
 * 	activity_display (Frame) owns _activityDisplayForm (Form)
 * 	resource_display (Frame) owns _resourceDisplayForm (Form)
 *
 * From there on down, things get a little complicated. From a motif_widget perspective,
 * the two forms above own a number of children, each one of which is responsible
 * for drawing an essential part of the GUI. However, the children are of unequal
 * importance. The most important children in the sense of the class hierarchy are
 * ACT_sys for the activity subgroup and RD_sys for the resource subgroup.
 *
 * By design, both ACT_sys and RD_sys are DrawingArea motif_widgets. As such, they
 * are responsible for drawing all activity instances and all displayed resources,
 * respectively.
 *
 * However, because they are near the top of the DS and RES display hierarchies, they
 * also have additional responsibilities in terms of managing other widgets in their
 * "siblings group".
 *
 *  Group	Item/Class		Description			Where Defined?
 *		(base class)
 *  =====	============		===========			==============
 *
 * ACT		DS_timemark 		time ruler			list_of_siblings::timemark
 * 		(DS_time_scroll_graph)
 *
 * 		CON_sys			constraint violation		ACT_sys::constraint_display
 * 		(DS_time_scroll_graph)	display
 *
 * 		DS_legend		legend area, holds		ACT_sys::legend_display
 * 		(DS_scroll_graph)	the legend buttons
 * 		(VSI_owner)
 *
 * 		ACT_sys			activity instance		activity_display::activitySystem
 * 		(DS_lndraw)		display area
 * 		(DS_time_scroll_graph)
 * 		(DS_scroll_graph)
 * 		(DS_graph)
 *
 * 		scrollbars		one horizontal			DS_scroll_graph::Vscrollbar
 *		(motif_widget)		scrollbar for the
 * 					whole set of "siblings"
 * 					(see later), including
 * 					all the above plus any
 * 					RD_sys object(s)
 * 					associated with this
 * 					activity display
 *
 * 					two vertical scroll-		DS_scroll_graph::Hscrollbar
 * 					bars, one for the ACT
 * 					subgroup and one for
 * 					the RES subgroup
 *
 * 		toggle buttons		one toggle button for		activity_display::_adSelectedTB
 * 		(motif_widget)		the ACT subgroup,		resource_display::_rdSelectedTB
 * 					one for each RD_sys in
 * 					the RES subgroup (0 or 1
 * 					for the time being)
 *
 * RES		MW_legend		to display the resource
 * 					name and canvas
 *
 * 		RD_sys			manages all resource
 * 					display-related objects
 *
 * 2. NAVIGATION AMONG CLASSES THAT HANDLE VERTICAL SCROLLING
 * ==========================================================
 *
 *  The complete design of a "vertically scrolled system" is managed
 *  and controlled by two high-level entities:
 *
 *  	- a canvas-type object, which implements a DrawingArea widget
 *  	  through inheritance from DS_draw_graph. Examples: DS_lndraw
 *  	  (ACT_sys), MW_widgetdraw (RD_sys). This object is in charge
 *  	  of drawing the "X" component of the overall scrolled item.
 *	  For an activity legend, this includes all activity bars and
 *	  labels, the staggering offsets, as well as the grey lines
 *	  that separate the activity legends. For a resource legend,
 *	  this includes the curve(s) that exhibit the time-dependent
 *	  history of the resource, the dashed lines showing 'nice'
 *	  values, the filling (green/crosshatched red/yellow [experimental])
 *	  etc.
 *
 * 	- a legend-type object, which implements a Form widget through
 * 	  inheritance from DS_scroll_graph, and ALSO implements a VSI_owner
 * 	  through inheritance from the VSI_owner class. The Motif form
 * 	  will contain a number of children, each one of which is associated
 * 	  with a visible vertically scrolled item.
 *
 * Last but not least, the vertically scrolled items are the objects
 * that are under the (scrolling) control of the above two high-level
 * objects. These items are Nodes of the vertically_scrolled_item
 * variety. They are abstract in the sense that they do not directly
 * implement their own graphical representation. The reason for this
 * is that there may be hundreds of VSI's, while Motif can only manage
 * dozens of objects with reasonable efficiency. The basic idea is to
 * only implement the graphic component for those objects that are visible;
 * invisible objects do not incur the cost associated with additional
 * Motif widgets that can't be seen anyway.
 *
 * Important data items are scattered throughout the classes described
 * so far: DS_draw_graph, DS_scrolled_graph, VSI_owner, and
 * vertically_scrolled_item.
 *
 * 2.1 Access from canvas-type widgets (DS_lndraw/ACT_sys, MW_widgetdraw/RD_sys)
 * ------------------------------------------------------------------------------
 *
 * VSI_owner *DS_lndraw::get_legend_display() returns a pointer to the
 * DS_legend of the ACT_sys.
 */
class DS_graph :
	public motif_widget,
	public back_pointer,
	public synchro::detector {
protected:
	static int		sel_state;	// select state			set to OFF by constructor; set by setsel()
		
public:
				//  List of DS_line nodes:
	List			lnobjects;
	
#ifdef GUI
	static Display		*Defdisplay;	//  Display,Screen,Depth		defined in constructor
	static int		Defscreenum;
	static int		Defdepth;
	static Colormap		Defcolormap;
#endif

	virtual synchro::problem* detectSyncProblem() = 0;
	static synchro::problem* checkForSyncProblem(synchro::detector* D);
	const Cstring&		detectorName() const { return get_key(); }

	virtual list_of_siblings *get_siblings() = 0;
	virtual int		isVisible() = 0;

				/* The role of cleargraph() is to clean up any (Motif) DrawingAreas
				 * that are under the control of this.  */
	virtual void		cleargraph() = 0;

	virtual int		is_a_res_sys() { return 0; }

	virtual int		is_a_con_sys() { return 0; }

	virtual int		is_an_mw_legend() { return 0; }

	//  constructor
	DS_graph(const Cstring & name, const WidgetClass c, motif_widget * parent, void * arguments, int n);
	//  destructor
	~DS_graph()  {; }

	virtual void		configure_graphics(callback_stuff *) = 0;

				/* Overridden (through DS_draw_graph) in ACT_sys, CON_sys, RD_sys/MW_widgetdraw.
				 * Separately overridden (through DS_scroll_graph) by DS_legend and MW_legend. */
	virtual void		update_graphics(callback_stuff *) = 0;
	virtual void		remove_pointer_to_line(DS_line *) {}
	virtual void		draw_vertical_cursor(int pos) {}
	virtual void		erase_vertical_cursor() {}
	static void		ExposeCallback(Widget,
					callback_stuff *,
					void *);

				// the next two functions now query the underlying widget directly
				//  (width, height data members have been eliminated to avoid duplication):
	virtual int		getwidth();
	int			getheight();
	virtual int		update_busy_line(
					Dsource *,
					int start,
					int end_of_line,
					int end_of_label) { return 0; }
	virtual void		selected(int,int,int) {}
	static void		setsel(int selst) { sel_state |= selst; }
	static void		resetsel(int selst) { sel_state = sel_state & (0xFFFF ^ selst); }
	static int		getsel() { return sel_state; }
	Node			*copy() { return new motif_widget(* this); } };

class DS_scroll_graph : public DS_graph {
private:
	list_of_siblings	*Siblings;
public:

				/* Checks that the toggle-button number, sizes and labels agree with
				 * what the vertically_scrolled_items think they should look like in this set
				 * of siblings. */
	// int			isOutOfSync(infoList*);
	synchro::problem*	detectSyncProblem();
	virtual void		clear_all_legends() {}
	virtual void		clearlegend(DS_line * for_this_line) {}
				//  base for DS_legend and MW_legend
	void			configure_graphics(callback_stuff *);

				/* For DS_legend, MW_legend. Actually, only MW_legend needs it:
				 * DS_legend only uses toggle buttons, which are redrawn by Motif
				 * when Expose events occur. The update_graphics method is only
				 * needed by classes (such as MW_legend and the DS_draw_graph-based
				 * classes) that use a DrawArea widget. */
	void			update_graphics(callback_stuff *) {}
	int			isVisible();

	virtual void		setAllStaleFlags(bool) {}

	//  Constructor; creates a Form motif_widget. Used by DS_legend.
	DS_scroll_graph(	const Cstring	&NAME,
				motif_widget	*Parent,
				void		*some_args,
				int		how_many_args,
				list_of_siblings *);
	//  Destructor.
	~DS_scroll_graph() {	// clear line objects NOW because they may need access to virtual method update_windows():
				lnobjects.clear(); }
	virtual VSI_owner	*has_vertically_scrolled_items() { return NULL; }
	virtual void		pre_configure_graphics() {}
	virtual void		post_configure_graphics() {}

				/* NOTE: this appears in the denominator of a formula
				 * in number_of_visible_units()... */
	virtual int		get_minimum_scrolled_item_height() { return 0; }
				// for RD_sys
	virtual bool		is_stale() const { return false; }

				/* Used to move selected vertically scrolled item(s) up or down. The only client of
				 * this method (for now, 11/02/02) is ACT_sys::vlegendswap() in ACT_edit.C. The only
				 * clients of _that_ method are the two callback functions
				 * activity_display::adUp/DownArrowActivateCallback() (in
				 * UI_activitydisplay.C). */
	virtual void		vswap(int direction);
				//  Computes how many legends are at least partially visible (estimate).
	int			number_of_visible_units();
	virtual motif_widget	*widget_to_use_for_height_info() = 0;
	list_of_siblings	*get_siblings() { return Siblings; }

	friend class list_of_siblings; };

class DS_draw_graph : public DS_graph {
protected:
	list_of_siblings	*Siblings;

				/* Used by do_the_horizontal_scrollbar_thing() to decide
				 * whether the user changed the scrollbar value. */
	int			current_horizontal_offset;

				/* Used by do_the_horizontal_scrollbar_thing() to store
				 * the time value associated with the current scrollbar
				 * position. */
	CTime_base		theTimeAssociatedWithTheMinPosOfTheHorizScrollbar;

				/* Only used at the level of DS_lndraw and MW_widgetdraw,
				 * but defined here for convenience. */
	motif_widget		*Vscrollbar, *Hscrollbar;

public:
	list_of_siblings	*get_siblings() { return Siblings; }

				//  Used by isOutOfSync() and related functions.
	void			evaluate_horizontal_scrollbar_parameters(	
						int &x_min, int &x_max,
						int &value,
						int &x_slider_size,
						int &page_inc,
						CTime_base &time_at_start,
						CTime_base &time_at_end);

	int			the_horizontal_scrollbar_is_out_of_sync(infoList*);
	void			do_the_horizontal_scrollbar_thing(infoList*);
	int			the_vertical_scrollbar_is_out_of_sync(infoList*);
	void			do_the_vertical_scrollbar_thing(infoList*);

				//  for legends only.
	int			theMaximumStaggeringOffset;

	static vertically_scrolled_item	*pointer_to_legend_being_resized;
	int			minimum_height_limit;

				//  Flag set when the user moves the grey boundary line in the activity display.
	int			TrackingTheLegendBoundary;

				//  To support tracking the legend boundary.
	int			last_sizing_pos;
				//  To support tracking the legend boundary.
	int			top_of_legend_being_resized;
	void			ButtonReleased(XButtonEvent *e);
	virtual void		clear_all_legends() {}
	virtual bool		is_a_giant_window() { return false; }
	virtual bool		is_in_hierarchy_select_mode() { return false; }
	virtual bool		is_in_hierarchy_color_mode() { return false; }
	virtual bool		is_in_down_select_mode() { return false; }
	virtual bool		is_a_hopper() { return false; }

	virtual void		clearlegend(DS_line * for_this_line) {}

	//  Constructor; creates a DrawingArea motif_widget. Used by DS_lndraw.
	DS_draw_graph(		const Cstring	&NAME,
				motif_widget	*Parent,
				int		vbar_desired,
				int		hbar_desired,
				list_of_siblings *
				);
	//  Destructor.
	~DS_draw_graph() {	// clear line objects NOW because they may need access to virtual method update_windows():
				lnobjects.clear(); }

	void			redraw_the_cute_grey_lines();

				//  Returns legend_display (ACT_sys and RD_sys only)
	virtual VSI_owner	*get_legend_graph() { return NULL; }
	void			vlegendswap(int direction);
	void			selectalllegends(int on_or_off);
	virtual void		add_vertical_cursor_if_necessary(int UL_x, int UL_y, int LR_x, int LR_y) {};
	virtual motif_widget	*get_Vscrollbar(VSI_owner **V) const { *V = NULL; return NULL; }
	virtual motif_widget	*get_Hscrollbar() const { return NULL; }

				//  Supports DS_lndraw::Event() and RD_sys::<TBD>
	int			we_are_moving_a_dividing_line(void *button_event);

	int			last_cursor_position;
	int			the_vertical_cursor_is_visible;
	// Not needed: overloaded in derived classes
	// void			update_graphics(callback_stuff *);

	int			isVisible();

				//  to allow workproc to figure out what needs to be redrawn:
	virtual void		update_windows(	int old_sx,
						int old_ex,
						int y_top,
						int y_bottom,
						Lego *) {}

	virtual void		add_window_for(DS_line *) {}
				/* We need this (pure virtual in DS_graph().) The method
				 * will clean the underlying DrawingArea widget which
				 * implements the Motif side of this object. */
	void			cleargraph();

	friend class list_of_siblings; };

class DS_time_scroll_graph : public DS_draw_graph {
public:
	DS_time_scroll_graph(
		const Cstring &N,
		motif_widget *parent,
		int vbar,
		int hbar,	
		list_of_siblings *L
		)
		: DS_draw_graph(N, parent, vbar, hbar, L),
		legend_display(NULL)
		{; }

#ifdef GUI
							// Regular cursor	displaymode = NOPMODE
	static Cursor		dscursor;		// Display cursor	displaymode = MOVEMODE
	static Cursor		pastecursor;		// Paste cursor		displaymode = PASTEMODE
	static Cursor		selcursor;		// Selection cursor	displaymode = SELMODE
	static Cursor		defcursor;		// Defining cursor	displaymode = DEFMODE
	static Cursor		expansion_cursor;	// Expanding cursor	displaymode = EXPANSIONMODE
	static unsigned char	displaymode;  
	static void		setcursor(unsigned char mode, Window which_window);
	static void             setcursorappearance(unsigned char mode, Window which_window);
#endif

				// DSG 96-07-05 method replaces superfluous,ill-updated data member of same name
	double			timeperhpixel() { return ((getwidth() > 0)
					      ? (Siblings->dwtimespan().convert_to_double_use_with_caution() / ((double) getwidth()))
					      : 0.); }
	void			draw_vertical_cursor(int i) { DS_draw_graph::draw_vertical_cursor(i); }

				/* Method that serves 2 purposes: (1) return the motif_widget that
				 *  implements the vertical scrollbar, and (2) return (through the
				 *  argument) a pointer to the (real) owner of the items scrolled
				 *  under the control of the vertical scrollbar. */
	motif_widget		*get_Vscrollbar(VSI_owner **v) const {
					*v = legend_display;
					return Vscrollbar; }
				//  Put here because both DS_lndraw and MW_widgetdraw need it:
	VSI_owner		*legend_display;

	VSI_owner		*get_legend_graph() { return legend_display; }

	friend class ACT_exec; }; 

class DS_lndraw;

class disambiguation_box: public motif_widget {
public:
	disambiguation_box(const Cstring &, motif_widget *, DS_lndraw *);
	~disambiguation_box() {; }
	static void		Disambiguation_OK_callback(Widget, callback_stuff *, void *);
	scrolled_list_widget	*disambiguation_list;
	motif_widget		*disambiguation_form;
	motif_widget		*disambiguation_pane;
	scrolled_list_widget	*priority_list;
	motif_widget		*logic_form;

	static void		UpdatePriorityBox(Widget, callback_stuff *, void *);
	void			update_priority_box(int);
				//  while moving the fake activity:
	void			update_priority_box(int, double);
	void			*green_color, *red_color, *grey_color;
	DS_lndraw		*parent_DS_lndraw;
	callback_stuff		data_for_window_closing;
	strintslist		selected_acts;
	List			ListOfPointersToLineObjects;
				// List of Pointer_nodes to Dsources w/scheduling info:
	List			MasterList;
	int			save_x, save_y;
	Dsource*		request_on_display; };


class logic_analysis_box: public motif_widget {
public:
	logic_analysis_box(const Cstring &, motif_widget *, DS_lndraw *, List symbols);
	~logic_analysis_box();

	//  callback
	static void		logic_analysis_box_callback(Widget, callback_stuff *, void *);

	//  data members
	motif_widget		*logic_form;
	motif_widget		*logic_paned_window;
	DS_lndraw		*parent_DS_lndraw;
	callback_stuff		data_for_window_closing;
	List			ListOfPointersToExpNodes;
	Dsource*		original_request, *deadline_activity, *fake_obs;
	double			start, deadline;
	Schedule_def		*sdef;
	void			*red_color, *green_color;
				//  contains smart pointers:
	List			ListOfPointersToActivities;

	//  methods
	void			update_obs(DS_line* source_line, double new_time); };
#ifdef GUI
class DS_lndraw_input_Handler
{
public:

  // click_context is defined in ACT_sys.H
  DS_lndraw_input_Handler() {};
  virtual ~DS_lndraw_input_Handler() {};
  virtual void DSInput(DS_lndraw* which, const XEvent* event) = 0;
};

// a class for storing widgets and pushables.  For use with ExtensiblePopupMenu.
class WidgetPush
{
  // the widget
  motif_widget* Widget;
  // the callback pointer
  MenuPopupSelectable* Push;

public:
  // constructor
  WidgetPush(motif_widget* widget, MenuPopupSelectable* push)
    : Widget(widget), 
      Push(push)
  {};

  // copy-constructor because we'll be in a container
  WidgetPush(const WidgetPush& rhs)
    : Widget(rhs.Widget),
      Push(rhs.Push)
  {};

  // widget accessor
  motif_widget* GetWidget() const {return Widget;};
  // callback accessor
  MenuPopupSelectable* GetPush() const {return Push;};
};

using std::map;
typedef map<Cstring, WidgetPush> WidgetPushMap;

/*This is a popup menu.  It can have buttons added to it with callbacks a little
more easily than traditionally.  The idea of this class is to facilitate users adding
to the APGEN UI**/
class Extensible_popup_menu : public motif_widget
{
  // the vector of WidgetPushes
  WidgetPushMap Buttons;
  // the callback activity (or NULL)
  const Dsource* Activity;
  // the x location of the click
  int X;
  // the Y location of the click
  int Y;
  // legend
  Cstring Legend;
  // time
  CTime_base Time;
public:

  // constructor
  Extensible_popup_menu(const Cstring& name, motif_widget* parent);
  // destructor
  virtual ~Extensible_popup_menu() {};
  // to add a callback button
  int AddCallbackButton(const Cstring& name, MenuPopupSelectable* push);
  // to set the sensitivity
  int SetChildButtonSensitive(const Cstring& name, int value);
  // the Selected that brings up the popup
  virtual int Push(const Dsource* act, int x, int y, const Cstring& legendName, const CTime_base& time);
  // a static callback for use with motif
  static void Callback(Widget w, callback_stuff * cb, void* data);
  // the non static version because it really should be this way
  void NonStaticCallback(Widget w, callback_stuff *cb);
};


//  Motif-wise, this is a simple Motif popup menu; however, we endow it with a few data members to store explanation-related stuff.
class act_popup_menu: public Extensible_popup_menu {
public:
	/* List of Symbol_nodes with key = activity ID of a relative and
	 * equivalent = explanation of the relationship. See C_list.H for
	 * the definition and constructors of the Symbol_node class (pretty
	 * simple stuff.)
	 */
	List                    pointersToTheRelatives;
	//  Will contain the ID of the activity instance under scrutiny.
	Cstring                 theExplainee;
	//  The self-documenting constructor.
	act_popup_menu(const Cstring& w_name, motif_widget *w_parent)
		: Extensible_popup_menu(w_name, w_parent)
		{};

	//  The self-documenting destructor.
	~act_popup_menu() {} };

#endif /* ifdef GUI */

      //  Implements the legend area which is the left part of an activity display.
class DS_legend : public DS_scroll_graph, public VSI_owner {
public:
	//  constructor. It invokes constructor # 2, which creates a Motif Form.
	DS_legend(	const Cstring	& name,
			motif_widget	*big_form,
			ACT_sys		*legend_of,
			void		*arguments,
			int		number_of_Args);

	VSI_owner*		has_vertically_scrolled_items() { return this; }
	int			get_minimum_scrolled_item_height();
	void			pre_configure_graphics();

				/* Need this (pure virtual in DS_graph) but it does
				 * not need to do anything because we own no DrawingArea
				 * widgets. */
	void			cleargraph() {}
	void			post_configure_graphics();

				//  Overrides base class method
	blist&			get_blist_of_vertically_scrolled_items() const;
	vertically_scrolled_item* get_first_vsi() const;
	vertically_scrolled_item* get_next_vsi(vertically_scrolled_item *) const;
				/* Sets the selection member of the appropriate LegendObject.
				 * Used as a callback for each toggle_button child of this. */
	static void		legendselected(Widget, callback_stuff *, void *);
				// overrides DS_scroll_graph method:
	void			vswap(int direction);

				//  to gain access to pointersToTheVisibleLegends:
	ACT_sys*		theActSysParent;
	motif_widget*		widget_to_use_for_height_info();
	Node*			copy() { return new motif_widget(*this); } };



//  Generic subclass for displays that can be scrolled along the time axis.
class DS_lndraw : public DS_time_scroll_graph, public MenuPopupSelectable {
private:
	KeySym WhichKeyPressedToInitiateDrag;
	bool KeyPressedToInitiateDrag;

protected:
				//  /* List */ Array of all the created DS_gc objects
	static DS_gc		**GC_array;
	Pixmap			tilemap;

				//  Handler for right-button clicks on activity instances.
	static void		rightButtonClickCB(Widget, callback_stuff *, void *);
				/* Handler for right-button selections that involve disambiguations,
				 * constraints or explanations. */
	static void		actPopupMenuCallback(Widget, callback_stuff *, void *);
				//  Handler for toggling cursor between vertical and normal states.
	static void		cursorPopupMenuCallback(Widget, callback_stuff *, void *);
				//  Utility function for creating the popup display for a specific instance.
	void			createActPopupMenu(
					int	Xcoord,
					int	Ycoord,
					Dsource* for_which_request);
				/* Handler for defining the regular/vertical cursor when right-clicking
				 * outside of an activity. */
	void			createCursorPopupMenu(int Xcoord, int Ycoord);

				//  Well, I think this is the activity constraint/explanation popup menu.
	act_popup_menu		*actPopupMenu;

				//  Supports adapter-defined menu options.
	Extensible_popup_menu	*cursorPopupMenu;
				/* Convenient spot for storing click location so we can
				 * position the popup menu correctly. */
	int			ClickLocation[2];

        			//  General handler for activity display.
        DS_lndraw_input_Handler* InputHandler;

public:

	void			SetDSlndrawInputHandler(DS_lndraw_input_Handler* handler) {InputHandler = handler;};

	void			draw_vertical_cursor(int pos);
	void			erase_vertical_cursor();
        void                    draw_invert_vertical_line(int pos, int bottom, int top);
        void                    draw_red_vertical_line(int pos, int botton, int top);	
	void			update_pointer_time(int xpos);
        void                    DefaultButton1Input(int x, int y);

	VSI_owner		*get_legend_display() { return legend_display; }

	motif_widget		*get_Hscrollbar() const { return Hscrollbar; }
protected:
	void			ButtonPressed(XButtonEvent *);

				/* these routines define how the object reacts to
				 * CLICKING actions by the user. */
	static void		Input(	Widget, callback_stuff *, void *);

				//  Object version of (button) input callback.
	void			input(XButtonEvent *, XmDrawingAreaCallbackStruct *);

				/* these data/routines/convenience funcs define how the object
				 * reacts to DRAGGING actions by the user. */
	static void		Motion(	Widget, callback_stuff *, void *);
	void			moving(int x, int y, bool snap, bool char_pressed, KeySym key);

public:
	//  Constructor. "parent" argument should be a motif_widget of xmPanedWindow class.
	DS_lndraw(const Cstring & name, motif_widget * parent, list_of_siblings *);
	~DS_lndraw();

	DS_gc			*gettilemap(int,int);
	virtual int		update_busy_line(Dsource*, int start, int end_of_line, int end_of_label);

				//  Used by low-level drawing functions that repaint a small window.
	void			add_vertical_cursor_if_necessary(
					int upper_left_x,
					int upper_left_y,
					int lower_right_x,
					int lower_right_y);

	// Note that we do not implement (pure) virtual method cleargraph(); this will be done in ACT_sys.

				//  Specialized function that helps with staggering offset computations.
	void			update_bits();

				//  pops up a list of activities stacked at the click point for user to choose from:
	void			disambiguate_acts_at(int x_coordinate, int y_coordinate);
  
        			//  APGEN event handling.
        void                    Event(const XEvent* event);

				//  Move enable flag (enables act instance dragging)
	unsigned char		mvenable;

	void			setmvenable() { mvenable=1; /*deltay=0;*/ }
	void			resetmvenable() { mvenable=0; }
	unsigned char		ismvenable() { return(mvenable); }

	void			clear_all_legends();
	void			clearlegend(DS_line * for_this_line);

	int			defkeypressed;
	int			getkeypressed() { return defkeypressed; }

	virtual void		selected(int button_2_flag, int fx, int fy);
	virtual void		logic_analyzer(int, int) {}
	void			send_to_back(int i, int j);

				// PFM: return true type
	virtual Dsource* newdef(void *, DS_gc *) { return (Dsource *) 0; }
	
	void			paste_at(int, int);

	void			define_new_activity_starting_at(int, int);

		
	int                     GetPositionForTime(const CTime_base &time);
	CTime_base		GetTimeForPosition(int x);
	CTime_base		GetSnappedTimeForPosition(int x);
	CTime_base		GetSnappedTimeForTime(const CTime_base &time);
	Cstring			GetLegendForPosition(int y);
	virtual void		MenuPopupSelected(	Extensible_popup_menu	*menu,
							const Dsource		*act,
							int x, int y,
							const Cstring &buttonName,
							const Cstring &legendName,
							const CTime_base &time);

		//  DATA MEMBERS

	static ACT_sys		*actsys_in_which_last_click_occurred;

				/* location when user starts
				 * dragging objects (SEL button pressed) */
	static int		mv_x, mv_y;

	disambiguation_box	*Dbox;
	// logic_analysis_box	*Lbox;
	//
				//  Handles pressing the OK button.
	void			disambiguation_OK(strintslist&);

				//  To help handle button clicks on activity instances.
	void			handle_one_selected_DS_line(DS_line *, int, int); };

//  Class that implements the time ruler at the top of an activity display
class DS_timemark : public DS_time_scroll_graph {
private:
	CTime_base		local_start, local_dur;
public:
	DS_gc			*lbgc;
	DS_gc			*lsgc;
	int			start_y;
	int			txtstart_y,
				txt2start_y;

				//  List of DS_text
	List			tmobjects;

				//  List of DS_line's of majors
	List			majormarks;

	DS_simple_line*		baseline;

	TIMEZONE		current_timezone;

private:
	CTime_base		Major;
	CTime_base		Minor;
public:
	CTime_base		get_time_unit() const;
	CTime_base		get_time_delta() const { return Major; }
	CTime_base			offset;
	
	ACT_sys*		ActSysParent;
	
	void			createtmdisplay();

	//  constructor
	DS_timemark(	const Cstring	&name,
			motif_widget	*big_form,
			ACT_sys		*act_parent);
	//  destructor
	~DS_timemark();

	// int			isOutOfSync(infoList*);
	synchro::problem*	detectSyncProblem();
	void			refresh_to_reflect_new_width(int swidth);

	void			update_graphics(callback_stuff *);
	void			configure_graphics(callback_stuff *);
	};

#endif
