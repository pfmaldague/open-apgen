/*	PROJECT		: APGEN
 *	SUBSYSTEM	: UI (MW sub-subsystem)
 *	FILE NAME	: UI_mw_timeline.H
 *
 *
 *	ABSTRACT:
 *	
 *	Header file for MW_widget* classes, which are motif_widget classes
 *	derived from DS_graph, DS_scroll_graph, and DS_time_scroll_graph.  See
 *	UI_mw_draw.H for other MW_* classes.
 *
 */

#ifndef  MW_TIMELINE_H
#define  MW_TIMELINE_H

#include "UI_ds_timeline.H"
#include "UI_mw_draw.H"		//brings "IO_plot_util.H"
#include "UTL_defs.H"		// for RD_YSCROLL_MODE

class MW_legend;
class RD_sys;


	/* MW_widgetdraw is analogous to DS_lndraw, in that it derives directly from
	 * DS_time_scroll_graph, and therefore is a Node which draws Graphics and can
	 * Scroll and which has Time as the horizontal axis.  However, DS_lndraw draws
	 * DS_object X-geometry objects (e.g. DS_ln), whereas MW_widgetdraw draws
	 * motif_widget objects, which as Widgets draw themselves.
	 *
	 * Oct. 2002: big change; we want scrolled resources, similar to legend objects.
	 * We accomplish this by incrementally modifying this class to resemble DS_lndraw
	 * more closely.
	 *
	 * First significant issue: difference with DS_lndraw. There is a List of mw_objects
	 * for each MW_widgetdraw, but only one global list of legend objects. Presumably
	 * flexibility (different resources in different RD_sys objects) is good, so
	 * we'll keep the difference.
	 *
	 * Second issue: ACT_sys uses a list_of_siblings to keep track of both horizontal
	 * and vertical scrolling. But here, although we want to be a 'slave' in the horizontal
	 * direction, we do want to do our own vertical scrolling. We don't want to duplicate
	 * information in the list_of_siblings object that's already there. So, let's enhance
	 * list_of_siblings so it can keep track of several scrollbars (up to 11 if we keep the
	 * current limit of 1 ACT_sys and up to 10 RD_sys objects per 'display unit'.)
	 */
class MW_widgetdraw : public DS_time_scroll_graph {
// protected:
// 	static Cstring		*thePendingLayout;
// 	static blist		list_of_tagged_layout_lists;
public:
	// constructor
	MW_widgetdraw(	const Cstring	&name_of_parent_act_sys,
			motif_widget	*parent,
			list_of_siblings *);
	// destructor
	~MW_widgetdraw();

	static Cstring		&pending_layout();


				/**List of MW_object instances, each of which handles one plot (legend/axis
				 * plus graph); note that unlike DS_lndraw, this is NOT static since
				 * cannot share Widgets.
				 *
				 * While ACT_sys::theLegends contains LegendObject nodes, mw_objects contains
				 * MW_object nodes. */
	blist			mw_objects;

	void			manage();

				/** This function is implemented in the DS_scroll_graph base class,
				 * but that base implementation is meant for use by DS_legend and
				 * MW_legend. What needs to be done by MW_widgetdraw/RD_sys is to
				 * handle the 'canvas' part of the update.
				 */
	void			update_graphics(callback_stuff *);
				// called after telemetry events have been received
	void			update_events();

				/** required because this widget is broken into 2:
				 * one for the legend and one for the canvas.
				 * The DS_graph base class doesn't know how to deal
				 * with this, hence we overload. */
	virtual	int		getwidth();

				// See above comment.
	virtual	int		getheight();

	void			draw_vertical_cursor(int xpos);

	void			erase_vertical_cursor();

	Node			*copy() { return new motif_widget(*this); }

				// Virtual in DS_scroll_graph
	VSI_owner		*get_legend_display() { return legend_display; }

				// To support events in the canvas part of the display
	static void		HandleEvent(Widget, callback_stuff *, void *);
				};

/** Vertically scrolled class derived from motif-widget that holds the
 * visible images of the MW_objects inside RD_sys. Similar to DS_legend,
 * and bears to MW_object the same relationship that DS_legend holds to
 * LegendObject. */
class MW_legend: public DS_scroll_graph, public VSI_owner {
public:
	// Constructor (similar to DS_legend)
	MW_legend(	const Cstring	&name,
			motif_widget	*parent,
			RD_sys		*legend_of,
			void		*arguments,
			int		number_of_Args);
	// Destructor
	~MW_legend();
	RD_sys		*theResSysParent;

			// Clears the DrawArea of each resource legend
	void		cleargraph();

	// Methods that are virtual in DS_scroll_graph
			// Sets some globals prior to creating widgets
	void		pre_configure_graphics(callback_stuff *);
			// This method is needed since we are using a DrawArea widget
	void		update_graphics(callback_stuff *);
			// Sets the selection member of the appropriate MW_object.
	static void	resourceselected(Widget, callback_stuff *, void *);
			// Invoked by pre_configure_graphics
	int		InitializeArgsForScrolledItems(void *theArgs);
	void		setAllStaleFlags(bool value);

	// Methods that are virtual in VSI_owner
	blist		&get_blist_of_vertically_scrolled_items() const;
	vertically_scrolled_item *get_first_vsi() const { return (vertically_scrolled_item *)
				get_blist_of_vertically_scrolled_items().first_node(); }
	vertically_scrolled_item *get_next_vsi(vertically_scrolled_item *v) const {
				return (vertically_scrolled_item *) v->next_node(); }
	bool		is_stale() const;
			// overrides base class method
	int		is_an_mw_legend() { return 1; }
	int		get_minimum_scrolled_item_height();

	motif_widget	*widget_to_use_for_height_info();

			// virtual in DS_scroll_graph
	VSI_owner	*has_vertically_scrolled_items() { return this; }
	};

#endif /*MW_TIMELINE_H*/
