#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "apDEBUG.H"
#include "UI_GeneralMotifInclude.H"
#include <ActivityInstance.H>
#include "ACT_exec.H"
#include "ACT_sys.H"
#include "action_request.H"
#include "APmodel.H"
#include "UI_activitydisplay.H"
#include "UI_ds_timeline.H"
#include "UI_mainwindow.H"	// for refresh_info...

using namespace std;

vertically_scrolled_item	*DS_draw_graph::pointer_to_legend_being_resized = 0;

#include <Xm/ToggleB.h>
Arg      args[26];
Cardinal ac = 0;

#define LABELPAD   5
#define theTolerance 3

				// in UI_motif_widget.C:
extern resource_data		ResourceData;

				// in main.C:
extern refresh_info		refreshInfo;
extern int			aButtonIsPressed();

				// Class globals for the Display Subsystem (DS_)

blist				VSI_owner::empty_list(compare_function(compare_bstringnodes, false));

// useful utility:
VerticalThing *lego2vsi(Node *N) {
	Lego	*lego = (Lego *) N;
	if(lego) {
		return lego->vthing; }
	return NULL; }

void vertically_scrolled_item::set_vertical_size_in_pixels_in(VSI_owner *the_owner, int new_val) {
	pointer_to_siblings	*theTag = (pointer_to_siblings *) theActualHeights.find((void *) the_owner);

	DBG_NOINDENT(get_key() << "->set_vertical_size_in_pixels_in: new value = " << new_val << endl);
	if(!theTag) {
		DBG_NOINDENT(get_key() << "->set_vertical_size_in_pixels_in: ERROR! no tag w/ key = "
			<< ((void *) the_owner) << endl);
		return; }
	theTag->theHeight = new_val; }

	// This will have to be changed so resource widgets can get their height too.
int VerticalThing::get_official_height() const {
	ACT_sys		*actsis, *first_visible_actsis = NULL;
	List_iterator	actsisses(ACT_sys::Registration());
	Pointer_node	*ptr;

	while((ptr = (Pointer_node *) actsisses())) {
		actsis = (ACT_sys *) ptr->get_ptr();
		if(actsis->isVisible()) {
			first_visible_actsis = actsis;
			break; } }
	if(first_visible_actsis) {
		return get_vertical_size_in_pixels_in(first_visible_actsis->legend_display); }
	return preferredHeight; }

int vertically_scrolled_item::get_vertical_size_in_pixels_in(const VSI_owner *v_o) const {
	pointer_to_siblings	*theTag = (pointer_to_siblings *) theActualHeights.find(
					(void *) v_o);
	
	if(!theTag) {
		DBG_NOINDENT(get_key() << "->get_vertical_size_in_pixels_in: ERROR! no tag w/ key = "
				<< ((void *) v_o) << endl);
		return 0; }
	else {
		DBG_NOINDENT(get_key() << "->get_vertical_size_in_pixels_in: found pointer to siblings"
			" @ " << ((void *) theTag) << endl); }
	return theTag->theHeight; }

void vertically_scrolled_item::adjust_height(int new_value) {
	pointer_to_siblings     *ptr;
	List_iterator           the_heights(theActualHeights);

	DBG_NOINDENT(get_key() << "->adjust_height(" << new_value << "): changing "
		<< theActualHeights.get_length() << " values...\n");
	preferredHeight = new_value;
	while((ptr = (pointer_to_siblings *) the_heights())) {
		DBG_NOINDENT(get_key() << "->adjust_height: changing height of ptr to siblings @ "
			<< ((void *) ptr) << endl);
		ptr->theHeight = new_value; } }

bool vertically_scrolled_item::there_are_windows_to_redraw_for(const VSI_owner *vo) const {
	pointer_to_siblings	*PS = (pointer_to_siblings *) theActualHeights.find((void *) vo);

	if(!PS) {
		cerr << "APGEN INTERNAL ERROR: there_are_windows_to_redraw_for legend " << get_key()
			<< " can't find pointer to " << vo->get_this()->get_key() << ".\n";
		cerr << "Trouble ahead.\n"; }
	DBG_NOINDENT(get_key() << "->there_are_windows...: siblings @ "
		<< ((void *) PS) << endl);
	return PS->theWindowsToRedraw.get_length() > 0; }

clist &vertically_scrolled_item::get_clist_of_windows_for(VSI_owner *vo) {
	pointer_to_siblings	*PS = (pointer_to_siblings *) theActualHeights.find((void *) vo);

	DBG_NOINDENT("vertically_scrolled_item(" << get_key() << ")->get_clist_of_windows() for "
		" ptr @ " << ((void *) PS) << " pointing to VSI_owner "
			<< vo->get_this()->get_key() << ".\n");
	if(!PS) {
		cerr << "APGEN INTERNAL ERROR: get_clist_of_windows for legend " << get_key()
			<< " can't find pointer to " << vo->get_this()->get_key() << ".\n";
		cerr << "Trouble ahead.\n"; }
	return PS->theWindowsToRedraw; }

int	*vertically_scrolled_item::get_stagger_info_for(VSI_owner *vo) {
	pointer_to_siblings	*PS = (pointer_to_siblings *) theActualHeights.find((void *) vo);

	if(!PS) {
		cerr << "APGEN INTERNAL ERROR: get_stagger_info_for legend " << get_key()
			<< " can't find pointer to " << vo->get_this()->get_key() << ".\n";
		cerr << "Trouble ahead.\n";
		return NULL; }
	DBG_NOINDENT(get_key() << "->get_stagger_info: siblings @ " << ((void *) PS) << endl);
	return PS->stagger_bits; }

void	vertically_scrolled_item::set_stagger_info_for(VSI_owner *vo, int *newval) {
	pointer_to_siblings	*PS = (pointer_to_siblings *) theActualHeights.find((void *) vo);

	if(!PS) {
		cerr << "APGEN INTERNAL ERROR: set_stagger_info_for legend " << get_key()
			<< " can't find pointer to " << vo->get_this()->get_key() << ".\n";
		cerr << "Trouble ahead.\n"; }
	DBG_NOINDENT(get_key() << "->set_stagger_info: siblings @ " << ((void *) PS) << endl);
	PS->stagger_bits = newval; }


	// ds_graph_address must be the DS_legend or MW_legend object.
int VSI_owner::y_offset_in_pixels() const {
	return Y_offset_in_pixels; }

	// ds_graph_address must be the DS_legend or MW_legend object.
void VSI_owner::set_vertical_offset_to(int V) {
	/* give "vertical siblings" (legends) a chance to notice that they need
	 * to update themselves. Note that this is only necessary because unlike
	 * in the case of horizontal scrolling, the logic in theWorkProc requires
	 * 2 passes: one to update the list of siblings as a result of the scrollbar
	 * action, and a second pass to let the vertical offset information stored
	 * here propagate to the siblings.
	 */
	udef_intfc::something_happened() = 1;
	Y_offset_in_pixels = V; }

	/** Should never be called with a givenItem that is not in a List.
	 * If I were Adam I'd have an 'assert' here. :-)
	 *
	 * dsgraph must be the DS_legend or MW_legend. */
int	VSI_owner::vertical_distance_above_item(
		vertically_scrolled_item *gi) const {
	if(gi) {
		vertically_scrolled_item	*vsi = get_first_vsi();
		int				so_far = 0;

		while(vsi && vsi != gi) {
			so_far += vsi->get_vertical_size_in_pixels_in(this);
			vsi = get_next_vsi(vsi); }
		return so_far - y_offset_in_pixels(); }
	// We are called in a situation with no legends defined. Return value does not matter.
	return 0; }

vertically_scrolled_item	*DS_legend::get_first_vsi() const {
	return lego2vsi(get_blist_of_vertically_scrolled_items().first_node()); }

vertically_scrolled_item	*DS_legend::get_next_vsi(vertically_scrolled_item *v) const {
	return lego2vsi(((VerticalThing *) v)->get_lego()->next_node()); }

	/* First argument is vertical position from the top of the physical widget containing
	 * the desired legend. The second arg. defaults to NULL and is used to avoid N**2
	 * search times when using this method in a loop.
	 *
	 * The method used consists in incrementally subtracting the legend heights from the
	 * theoretical top of the legend area, which is y plus the Y_offset_in_pixels_for_act, until
	 * a legend is found that intersects the horizontal line at y.
	 *
	 * The dsgraph must be the DS_legend or MW_legend. */
vertically_scrolled_item *VSI_owner::findTheVSIat(
		int				y,
		vertically_scrolled_item	*startWithThisOne) const {
	vertically_scrolled_item	*vsi;
	int				bottom_of_curr_leg = 0;
					// from theoretical top of (scrolled) panel:
	int				y_after_offsetting = y + y_offset_in_pixels();


	if(startWithThisOne) {
		vsi = startWithThisOne; }
	else {
		vsi = get_first_vsi(); }

#	ifdef apDEBUG
	DS_scroll_graph	*Parent_to_use = get_this();
	DBG_INDENT("VSI_owner(" << get_this()->get_key() << ")::findTheVSIat() START; will scan "
		<< get_blist_of_vertically_scrolled_items().get_length() << " VSI items.\n");
	DBG_NOINDENT("Current width: " << Parent_to_use->getwidth() << endl);
#	endif
	while(vsi) {
		DBG_NOINDENT("   handling vsi @ " << ((void *) vsi) << endl);
		// inside this loop we KNOW that the legend is appropriate for the act_sys (hopper or regular)
		bottom_of_curr_leg += vsi->get_vertical_size_in_pixels_in(this);
		DBG_NOINDENT("bottom at " << bottom_of_curr_leg << "...\n");
		if(y_after_offsetting < bottom_of_curr_leg) {
			DBG_UNINDENT("findTheVSIat DONE.\n");
			return vsi; }
		if(!get_next_vsi(vsi)) {
			DBG_UNINDENT("findTheVSIat DONE.\n");
			return vsi; }
		vsi = get_next_vsi(vsi); }
	DBG_UNINDENT("findTheVSIat DONE (returning NULL).\n");
	return NULL; }

void VSI_owner::evaluate_vertical_scrollbar_parameters(int &y_max, int &slider_size) {
	vertically_scrolled_item	*theObj = get_first_vsi();
	Dimension			y_height;

	DBG_INDENT(This->get_key() << "-> evaluate_vertical_scrollbar_parameters() START\n");
	// Make sure there is a little room at the bottom:
	// y_max = 0;
	y_max = 20;

	// Determine number of vertical units that can be displayed
	while(theObj) {
		y_max += theObj->get_vertical_size_in_pixels_in(this);
		theObj = get_next_vsi(theObj); }
	/* On some systems, the Motif Form associated with This gets resized to
	 * the full theoretical size of the resource display, including the
	 * nominal height of partially visible resource displays. It's better,
	 * therefore, to use the size of the Frame that holds the Form that holds
	 * the Form... */
	DBG_NOINDENT("evaluating slider size based on widget " << This->myParent->get_key()
			<< " -> " << This->myParent->myParent->get_key() << "...\n");
	// slider_size = This->getheight();
	XtVaGetValues(
		// This->myParent->myParent->widget
		This->widget_to_use_for_height_info()->widget,
		XmNheight,		&y_height,
		NULL);
	slider_size = y_height;
	if(slider_size < 2 * ACTVERUNIT)
		slider_size = 2 * ACTVERUNIT;

	// This could (and did) happen e. g. after a purge, I think.
	if(y_max < slider_size + y_offset_in_pixels()) {
		y_max = slider_size + y_offset_in_pixels(); }
	DBG_UNINDENT("VSI_owner::evaluate_vertical_scrollbar_parameters: returning y_max = "
		       << y_max << ", slider_size = " << slider_size << endl); }

	/* This constructor uses the DS_scroll_graph base class' constructor #2, which
	 * creates a motif form suitable for positioning the vertically_scrolled_items
	 * as toggle_buttons. */
DS_legend::DS_legend(	const Cstring	&N,
			motif_widget	*parent,
			ACT_sys		*AS,
			void		*motif_arguments,
			int		their_number)
		: DS_scroll_graph(N, parent, motif_arguments, their_number, AS->get_siblings()),
		theActSysParent(AS),
		VSI_owner(this) {
	DBG_NOINDENT("	DS_legend(\"" << N << "\")::DS_legend...\n"); }

blist &DS_legend::get_blist_of_vertically_scrolled_items() const {
	return Dsource::theLegends(); }

int DS_legend::get_minimum_scrolled_item_height() {
	return ACTFLATTENED; }

// This method provides an OVERESTIMATE of the number of visible vertically_scrolled_items.
int DS_scroll_graph::number_of_visible_units() {
	if(!has_vertically_scrolled_items()) {
		return 0; }
	int		k, h;

	if(!isVisible()) {
		DBG_NOINDENT(get_key() << "->number_of_visible_units: not visible, returning 0.\n");
		return 0; }
	DBG_NOINDENT(get_key() << "->number_of_visible_units: dividing height = " << getheight()
		<< " by minimum scrolled height = " << get_minimum_scrolled_item_height() << ".\n");
	k = (h = getheight()) / get_minimum_scrolled_item_height();
	// add 1 to account for partially visible legends:
	if(k * get_minimum_scrolled_item_height() < h)
		k++;
	// allow for partially visible legends at the top:
	return k + 1; }

	/* We need to make the following method generic, so it applies to resource displays as well.
	 * The 'personality' of DS_legend is clear: it "is" a form and owns all widgets that
	 * contribute to the appearance and functionality of the legend area of the APGEN main
	 * window.
	 *
	 * The challenge is to define a structure that supports both the existing DS_legend
	 * implementation and the TBD resource legend implementation. Right now, the resource
	 * legend "is" a DrawingArea on which MW_widgetdraw draws text and tickmarks. We need
	 * to add to this a selection button similar to the toggle button used in legends.
	 *
	 * (After a partial implementation) I think it would be a _really_ good idea to build into
	 * the design a foolproof scheme that would force the "effector" to result in a state that
	 * the "sensor" algorithm agrees is acceptable. The problem right now is that the "isOutOfSync"
	 * method is dancing an infinite series of steps with "theWorkProc" and they never achieve
	 * synchronicity. */
synchro::problem* DS_scroll_graph::detectSyncProblem() {
	VSI_owner*	v_o = has_vertically_scrolled_items();

	if(!v_o) {
		// Let's not waste time
		return NULL; }
	if(GUI_Flag) {
					// This could be generalized if each vertically scrolled item
					// contains exactly one top-level widget.
	static motif_widget		*the_scrolled_item;

	// DO NOT use 'int' instead of 'short'... or you'll regret it!
	static short			y_top, y_height;

	static int			theoretical_height;
	vertically_scrolled_item	*VS_item = (vertically_scrolled_item *) v_o->findTheVSIat(0, NULL);
					// if the current legend is only partially visible, this will be negative:
	int				to_top = v_o->vertical_distance_above_item(VS_item);
	int				top_of_legend = 0, bottom_of_legend;
	int				theoretical_top_of_visible_portion =
						v_o->y_offset_in_pixels();
	int				theoretical_bottom_of_visible_portion =
						theoretical_top_of_visible_portion + getheight();
	List_iterator			theActualLegendButtons(children);
	int				k = 0;


	DBG_INDENT("DS_scroll_graph(" << get_key() << ")::isOutOfSync() START.\n");


	if(!v_o->get_blist_of_vertically_scrolled_items().get_length()) {
		if(children.get_length()) {
			DBG_UNINDENT("DONE; no VSIs but some widgets are present; returning 1 (NOT in sync).\n");
			Cstring reason(get_key());
			reason << ": no VSIs but some widgets are present";
			return new synchro::problem(
				reason,
				this,
				DS_graph::checkForSyncProblem,
				NULL,
				NULL); }
		else {
			DBG_UNINDENT("DONE; no VSIs, no children; returning 0 (in sync).\n");
			return NULL; } }
	else if(!VS_item) {
		DBG_NOINDENT(get_key() << "->isOutOfSync(): no legends drawn while "
				<< v_o->get_blist_of_vertically_scrolled_items().get_length()
				<< " vertically scrolled items have been defined, so: YES\n");
		Cstring reason(get_key());
		reason << ": no legends drawn while "
			<< Cstring(v_o->get_blist_of_vertically_scrolled_items().get_length())
			<< " defined";
		DBG_UNINDENT("DONE; no VSI at 0, yet there are MW_objects; returning 1 (NOT in sync).\n");
		return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL); }
	// May be a Motif bug? The resource legend shrinks to 0 when the "File Open" dialog pops up...
	// if(getwidth() != ACTLEGNDWIDTH) {
	// 	DBG_UNINDENT("DONE; width = " << getwidth() << " is messed up; returning 1 (NOT in sync).\n");
	// 	return 1; }

	DBG_NOINDENT("Calling " << get_key() << " -> is_stale()...\n");
	if(is_stale()) {
		DBG_UNINDENT("DONE; legend display is stale, returning 1 (NOT in sync).\n");
		Cstring reason(get_key());
		reason << ": legend display is stale";
		return new synchro::problem(
			reason,
			this, DS_graph::checkForSyncProblem,
			NULL,
			NULL); }

	DBG_NOINDENT("Found top VSI; distance to theoretical top of legend display (to_top) = " << to_top << ".\n");
	DBG_NOINDENT("Will now loop over " << children.get_length() <<
			" visible motif_widget(s) until to_top exceeds legend height, which is " << getheight() << endl); 
	while(	(to_top < getheight())
		&& (the_scrolled_item = (motif_widget *) theActualLegendButtons())
	    ) {
		DBG_NOINDENT("scanning VSI with to_top = " << to_top << "; getting height from Motif widget...\n");
		if(!VS_item) {
			DBG_UNINDENT(get_key() << "->isOutOfSync(): more VSIs than MW_objects, so: YES\n");
			Cstring reason(get_key());
			reason << ": more toggle buttons than legends";
			return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL); }
		// This is actually well-defined for all implementations of this class:
		XtVaGetValues(the_scrolled_item->widget,
			XmNy,			&y_top,
			XmNheight,		&y_height,
			NULL);
		if(	(theoretical_height = VS_item->get_vertical_size_in_pixels_in(v_o)) != y_height
			|| y_top != to_top) {
			DBG_UNINDENT(get_key() << "->isOutOfSync(): legend \""
				<< the_scrolled_item->get_key() << "\" is at (y = " << y_top
				<< ", h = " << y_height << ") but SHOULD be at (y = " << to_top
				<< ", h = " << getheight() << "), so: YES\n");
			Cstring reason(get_key());
			reason << ": legend \"" << the_scrolled_item->get_key() << "\" is at y = "
				<< Cstring(y_top)
				<< ", h = " << Cstring(y_height) << " but SHOULD be at y = "
				<< Cstring(to_top)
				<< ", h = " << Cstring(getheight());
			return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL); }
		else {
			Cstring ret;

			DBG_NOINDENT("heights agree. Checking selection...\n");
			if(VS_item->get_selection()) {
				if(VS_item->get_toggle_button_for(the_scrolled_item)->get_text() != "SET") {
					DBG_UNINDENT(get_key() << "->isOutOfSync(): legend \""
						<< the_scrolled_item->get_key()
						<< "\" should be selected, yet it isn't.\n");
					Cstring reason(get_key());
					reason << ": legend \"" << the_scrolled_item->get_key()
							<< "\" should be selected, yet it isn't.\n";
					return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL); } }
			else {
				if(VS_item->get_toggle_button_for(the_scrolled_item)->get_text() != "NOT SET") {
					DBG_UNINDENT(get_key() << "->isOutOfSync(): legend \""
						<< the_scrolled_item->get_key()
						<< "\" should NOT be selected, yet it is.\n");
					Cstring reason(get_key());
					reason << ": legend \"" << the_scrolled_item->get_key()
							<< "\" should NOT be selected, yet it is.\n";
					return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL); } }

			DBG_NOINDENT("Now testing VS_item " << VS_item->get_key()
				<< " against motif widget "
				<< the_scrolled_item->get_key() << "...\n" );
			if(VS_item->get_theoretical_label() != (ret = vertically_scrolled_item::get_label_text_from(
							VS_item->get_toggle_button_for(the_scrolled_item)))) {
				DBG_UNINDENT(get_key() << "->isOutOfSync(): legend \""
					<< VS_item->get_theoretical_label()
					<< "\" disagrees with toggle button label " << ret << ", so: YES\n");
				Cstring reason(get_key());
				reason << ": legend \"" << VS_item->get_theoretical_label()
						<< "\" disagrees with toggle button label " << ret;
				return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL); } }
		to_top += theoretical_height;
		VS_item = v_o->get_next_vsi(VS_item); }

	// Need to check that all legends are accounted for:
	if(VS_item) {
		to_top = v_o->vertical_distance_above_item(VS_item);
		if(to_top < getheight()) {
			DBG_UNINDENT(get_key() << "->isOutOfSync(): MW_object \"" << VS_item->get_key()
				<< "\" has dist. to top = " << to_top
				<< " while theo. bottom = " << getheight()
				<< "; looks like it's not being displayed, so: YES\n");
			Cstring reason;
			reason << "MW_object \"" << VS_item->get_key()
				<< "\" has dist. to top = " << to_top
				<< " while theo. bottom = " << getheight()
				<< "; looks like it's not being displayed.";
			return new synchro::problem(reason, this, DS_graph::checkForSyncProblem, NULL, NULL); } }
	DBG_UNINDENT("DONE; returning 0 (in sync).\n"); }
	return NULL; }

	// used for resources; activity legends use DS_legend::vswap(int)
	// which overrides this method.
void DS_scroll_graph::vswap(int direction) {
	VSI_owner	*v_o = has_vertically_scrolled_items();

	if(!v_o) {
		// Let's now waste time
		return; }
	if(GUI_Flag) {
	blist				&theList = v_o->get_blist_of_vertically_scrolled_items();
	int				destination = -1;
	int				movestart=-1;
	int				number_of_sel_items_to_move = -1;
	vertically_scrolled_item	*VS_item;
	int				curinx;
	Node				*tnode;
	Node				*tpnode;
	Node				*rnode;

	DBG_INDENT("DS_scroll_graph(\"" << get_key() << "\")::vswap: START\n");
	if(!(VS_item = (vertically_scrolled_item *) theList.first_node())) {
		DBG_UNINDENT("DS_scroll_graph(\"" << get_key() << "\")::vswap: "
			"no legends in theLegend; returning.\n");
		return; }
	if(VS_item->get_selection() && direction == SWAPUP) {
		DBG_UNINDENT("DS_scroll_graph(\"" << get_key() << "\")::vswap: "
			"trying to swap UP the FIRST legend; returning.\n");
		return; }
	// Well, we are making assumptions about the order of the legends in this list...
	// should we use several lists of legends after all? Maybe.
	VS_item = (vertically_scrolled_item *) theList.last_node();
	if(VS_item->get_selection() && direction == SWAPDOWN) {
		DBG_UNINDENT("DS_scroll_graph(\"" << get_key() << "\")::vswap: trying "
			"to swap DOWN the LAST legend; returning.\n");
		return; }
#	ifdef apDEBUG
	DBG_NOINDENT("Legends BEFORE:\n");
	for(curinx = 0; curinx < theList.get_length(); curinx++) {
		VS_item = (vertically_scrolled_item *) theList[curinx];
		DBG_NOINDENT("legend # " << (curinx + 1) << " = \"" << VS_item->get_key()
			<< "\"\n"); }
#	endif
	for(curinx = 0; curinx < theList.get_length(); curinx++) {
		VS_item = (vertically_scrolled_item *) theList[curinx];
		if (!VS_item->get_selection()) {
			if (number_of_sel_items_to_move < 0)
				// we may need to move this node (SWAPUP) or
				// insert a node below it (SWAPDOWN):
				destination  = curinx;
			else {
				//Decision for swapping
				if (direction == SWAPUP) {
					if(destination  >= 0) {
						// this is the first unselected node following
						// one or more selected node(s)
						// identify the bottom selected node:
						tnode = theList[curinx - 1];
						// remove the unselected node above the selected ones:
						rnode = theList.remove_node(
								theList[destination]);
						// re-insert it below the last selected node:
						theList.insert_node(rnode, tnode); }
					destination = curinx; }
				else {
					//SWAPDOWN
					int loops;

					// remove this node, which is below the selection:
					rnode = theList.remove_node(
							theList[curinx]);
					// [] operator returns 0 if arg is < 0 !!
					tnode = theList[ movestart - 1 ];
					tpnode = theList[movestart];

					// re-insert this node above the selection:
					if (tnode == tpnode)
						// tnode is the head
						theList.insert_node(rnode, (Node *) 0);
					else
						theList.insert_node(rnode, tnode);

					destination = -1; } }
			movestart = -1;
			number_of_sel_items_to_move = -1; }
		else {
			if (number_of_sel_items_to_move < 0) {
				movestart = curinx;
				number_of_sel_items_to_move = 1; }
			else
				number_of_sel_items_to_move++; } }
	if (number_of_sel_items_to_move > 0) { // the selected nodes extend to the bottom of the list:
		//boundary condition
		if ((direction == SWAPUP) && (destination >= 0)) {
			int loops;

			tnode = theList.last_node();
			rnode = theList.remove_node(
					theList[ destination ]);
			theList.insert_node(rnode, tnode); } }
#	ifdef apDEBUG
	DBG_NOINDENT("Legends AFTER:\n");
	for(curinx = 0; curinx < theList.get_length(); curinx++) {
		VS_item = (vertically_scrolled_item *) theList[curinx];
		DBG_NOINDENT("legend # " << (curinx + 1) << " = \"" << VS_item->get_key() << "\"\n"); }
#	endif

	DBG_UNINDENT("DS_scroll_graph::vswap: END\n"); } }

void DS_legend::vswap(int direction) {
	VSI_owner	*v_o = has_vertically_scrolled_items();

	if(!v_o) {
		// Let's now waste time
		return; }
	if(GUI_Flag) {
	blist				&theList = v_o->get_blist_of_vertically_scrolled_items();
	int				destination = -1;
	int				movestart=-1;
	int				number_of_sel_items_to_move = -1;
	Lego				*lego;
	int				curinx;
	Node				*tnode;
	Node				*tpnode;
	Node				*rnode;

	DBG_INDENT("DS_legend(\"" << get_key() << "\")::vswap: START\n");
	if(!(lego = (Lego *) theList.first_node())) {
		DBG_UNINDENT("DS_legend(\"" << get_key() << "\")::vswap: "
			"no legends in theLegend; returning.\n");
		return; }
	if(lego->vthing->get_selection() && direction == SWAPUP) {
		DBG_UNINDENT("DS_legend(\"" << get_key() << "\")::vswap: "
			"trying to swap UP the FIRST legend; returning.\n");
		return; }
	// Well, we are making assumptions about the order of the legends in this list...
	// should we use several lists of legends after all? Maybe.
	lego = (Lego *) theList.last_node();
	if(lego->vthing->get_selection() && direction == SWAPDOWN) {
		DBG_UNINDENT("DS_legend(\"" << get_key() << "\")::vswap: trying "
			"to swap DOWN the LAST legend; returning.\n");
		return; }
#	ifdef apDEBUG
	DBG_NOINDENT("Legends BEFORE:\n");
	for(curinx = 0; curinx < theList.get_length(); curinx++) {
		lego = (Lego *) theList[curinx];
		DBG_NOINDENT("legend # " << (curinx + 1) << " = \"" << lego->get_key()
			<< "\"\n"); }
#	endif
	for(curinx = 0; curinx < theList.get_length(); curinx++) {
		lego = (Lego *) theList[curinx];
		if (!lego->vthing->get_selection()) {
			if(number_of_sel_items_to_move < 0)
				// we may need to move this node (SWAPUP) or
				// insert a node below it (SWAPDOWN):
				destination  = curinx;
			else {
				//Decision for swapping
				if (direction == SWAPUP) {
					if(destination  >= 0) {
						// this is the first unselected node following
						// one or more selected node(s)
						// identify the bottom selected node:
						tnode = theList[curinx - 1];
						// remove the unselected node above the selected ones:
						rnode = theList.remove_node(
								theList[destination]);
						// re-insert it below the last selected node:
						theList.insert_node(rnode, tnode); }
					destination = curinx; }
				else {
					//SWAPDOWN
					int loops;

					// remove this node, which is below the selection:
					rnode = theList.remove_node(
							theList[curinx]);
					// [] operator returns 0 if arg is < 0 !!
					tnode = theList[ movestart - 1 ];
					tpnode = theList[movestart];

					// re-insert this node above the selection:
					if (tnode == tpnode)
						// tnode is the head
						theList.insert_node(rnode, (Node *) 0);
					else
						theList.insert_node(rnode, tnode);

					destination = -1; } }
			movestart = -1;
			number_of_sel_items_to_move = -1; }
		else {
			if (number_of_sel_items_to_move < 0) {
				movestart = curinx;
				number_of_sel_items_to_move = 1; }
			else
				number_of_sel_items_to_move++; } }
	if (number_of_sel_items_to_move > 0) { // the selected nodes extend to the bottom of the list:
		//boundary condition
		if ((direction == SWAPUP) && (destination >= 0)) {
			int loops;

			tnode = theList.last_node();
			rnode = theList.remove_node(
					theList[ destination ]);
			theList.insert_node(rnode, tnode); } }
#	ifdef apDEBUG
	DBG_NOINDENT("Legends AFTER:\n");
	for(curinx = 0; curinx < theList.get_length(); curinx++) {
		lego = (Lego *) theList[curinx];
		DBG_NOINDENT("legend # " << (curinx + 1) << " = \"" << lego->get_key() << "\"\n"); }
#	endif

	DBG_UNINDENT("DS_legend::vswap: END\n"); } }

void DS_legend::pre_configure_graphics() {
	DBG_NOINDENT("...calling ACT_sys parent->update_bits()...\n");
	theActSysParent->update_bits(); }

void DS_legend::post_configure_graphics() {
	DBG_NOINDENT("...calling ACT_sys parent->update_bits()...\n");
	theActSysParent->update_bits(); }

		// signature changed 1/9/03: use (y,h) intead of Motif args
motif_widget *VerticalThing::create_scrolled_item(
			int		the_index,
			VSI_owner	*owner,
			int		y,
			int		h) {
	if(GUI_Flag) {
	DS_legend	*parent_to_use = (DS_legend *) owner->get_this();

	ac = 0;
	XtSetArg(args[ ac  ], XmNrecomputeSize,		False)				; ac++;
	XtSetArg(args[ ac  ], XmNhighlightThickness,	0)				; ac++;
	XtSetArg(args[ ac  ], XmNborderWidth,		1)				; ac++;
	XtSetArg(args[ ac  ], XmNy,			y)				; ac++;
	XtSetArg(args[ ac  ], XmNx,			0)				; ac++;
	XtSetArg(args[ ac  ], XmNheight,		h)				; ac++;
	XtSetArg(args[ ac  ], XmNwidth,			ACTLEGNDWIDTH)			; ac++;
	XtSetArg(args[ ac  ], XmNfont,			ResourceData.legend_font)	; ac++;
	XtSetArg(args[ ac  ], XmNalignment,		XmALIGNMENT_BEGINNING)		; ac++;
	XtSetArg(args[ ac  ], XmNbackground,		ResourceData.peach_color)	; ac++;
	XtSetArg(args[ ac  ], XmNforeground,		ResourceData.black_color)	; ac++;
	toggle_button = new motif_widget(
				Cstring("AD")
					+ ("ACT_system" / parent_to_use->theActSysParent->get_key())
					+ "_" + get_theoretical_label(),
				xmToggleButtonWidgetClass,	parent_to_use,
				args,				ac ,
				TRUE);
	toggle_button->set_label(get_theoretical_label());
	toggle_button->add_callback(DS_legend::legendselected, XmNvalueChangedCallback, this);
	if(get_selection()) {
		*toggle_button = "1"; } }
	return toggle_button; }

     /* This (virtual) method is really only used by subclasses that override
      * DS_scroll_graph::has_vertically_scrolled_items(), i. e., DS_legend and
      * MW_legend. Other subclasses (such as DS_lndraw, RD_sys) either have
      * the default behavior of has_vertically_scrolled_items(), which is to
      * return NULL, or override configure_graphics itself. */
void DS_scroll_graph::configure_graphics(callback_stuff *) {
	VSI_owner	*v_o = has_vertically_scrolled_items();

	if(!v_o) {
		return; }
	// debug
	if(is_a_res_sys()) {
		// debug
		cerr << "Hmmm... DS_scroll_graph called on a RD_sys with a VSI_owner.\n";
		return; }
	if(GUI_Flag) {
	static motif_widget		*scrolled_item;
	vertically_scrolled_item	*VS_Item;
	int				theoretical_top_of_visible_portion = v_o->y_offset_in_pixels();
	int				to_top;
	int				top_of_legend = 0, bottom_of_legend;
	int				theoretical_bottom_of_visible_portion =
						theoretical_top_of_visible_portion + getheight();
					// define this because the widget grows as you add legends!!
	int				UseThisHeight = getheight();

	DBG_INDENT("DS_scroll_graph \"" << get_key() << "\"::configure_graphics START (we have a v_o of width "
		<< v_o->get_this()->getwidth() << ")\n");
	pre_configure_graphics();

	DBG_NOINDENT("will delete " << children.get_length() <<
			" old vertically scrolled object(s)...\n"; cout.flush());
	while(children.get_length()) {
		scrolled_item = (motif_widget *) children.last_node();
		scrolled_item->destroy_widgets();
		delete scrolled_item; }
	VS_Item = v_o->get_first_vsi();
	while(VS_Item) {
		VS_Item->toggle_button = NULL;
		VS_Item = v_o->get_next_vsi(VS_Item); }

	v_o->pointersToTheVisibleLegends.clear();

	if(!isVisible()) {
		DBG_UNINDENT("DS_legend not visible...\n");
		return; }
	VS_Item = (vertically_scrolled_item *) v_o->findTheVSIat(0, NULL);
	if(!VS_Item) {
		DBG_UNINDENT("no current legend...\n");
		return; }
	to_top = v_o->vertical_distance_above_item(VS_Item);
	if(to_top + VS_Item->get_vertical_size_in_pixels_in(v_o) <= 0) {
		DBG_UNINDENT("current legend above top...\n");
		// basically, nothing can be seen...
		return; }

	// FIRST CLEAN UP ALL TOGGLE BUTTONS (DS_legend) or ... (MW_legend)
	/** First clean up all sub-widgets of the XX_legend object. Remember
	 * that for both types of XX_legend objects, DS_scroll_graph is a form.
	 *
	 * In the case of DS_legend, the children are a set of toggle buttons.
	 *
	 * In the case of MW_legend, there are 3 children for each visible
	 * MW_object: a toggle button for selection, a label to hold the
	 * resource name, and a draw area to draw the vertical axis with its
	 * ticmarks and labels as well as Units (if any.)
	 *
	 * Unfortunately, the logic for handling vertically scrolled items
	 * demands that we use a single widget at the top of the hierarchy for
	 * each scrolled object (e. g., in DS_scroll_graph::we_are_moving_a_dividing_line(),
	 * the height of each object is determined from the Motif height of the widget
	 * pointed to by the vsi_pointers in the XX_legend's pointersToTheScrolledItems.)
	 *
	 * Therefore, we need to create one form in MW_legend for each visible resource
	 * display. Since we only create objects for visible items, it is hoped that
	 * the Motif overhead associated with these forms will be acceptable.
	 */

	DBG_NOINDENT("will now create new vertically scrolled objects...\n"; cout.flush());

	UseThisHeight = getheight();
	while(to_top < UseThisHeight && VS_Item) {
		DBG_NOINDENT("...setting height of VSI at " << to_top
			<< " to " << VS_Item->get_vertical_size_in_pixels_in(v_o) << endl);
		DBG_NOINDENT("...Calling create_scrolled_item() for \""
			<< VS_Item->get_theoretical_label() << "\" at y = " << to_top << endl);
		scrolled_item = VS_Item->create_scrolled_item(
					children.get_length(),
					v_o,
					to_top,
					VS_Item->get_vertical_size_in_pixels_in(v_o)
					);
		// need to use appropriate void pointers here.
		v_o->pointersToTheVisibleLegends << new bpointernode((void *) VS_Item, VS_Item);
		to_top += VS_Item->get_vertical_size_in_pixels_in(v_o);
		VS_Item = v_o->get_next_vsi(VS_Item); }
	post_configure_graphics();
	update_graphics(NULL); }
	DBG_UNINDENT("DS_scroll_graph::configure_graphics DONE\n"); }

//Legend selection CallBack
void DS_legend::legendselected(Widget wid, callback_stuff * client_data, XtPointer) {
	if(GUI_Flag) {
	motif_widget			*toggle_button = (motif_widget *) client_data->parent;
	vertically_scrolled_item	*theObj = (vertically_scrolled_item *) client_data->data;
	SELECT_ACT_LEGENDrequest	*request;

	if(! theObj) return;
	DBG_NOINDENT("legendselected: legend = " << theObj->get_key() << "\n");

	if(XmToggleButtonGetState(toggle_button->widget)) {
		request = new SELECT_ACT_LEGENDrequest(theObj->get_key(), 1); }
	else {
		request = new SELECT_ACT_LEGENDrequest(theObj->get_key(), 0); }
	request->process(); } }

motif_widget	*DS_legend::widget_to_use_for_height_info() {
	return theActSysParent; }

VerticalThing::VerticalThing(	const Cstring	&text,
				const Cstring	&filename,
				int		preferred_height,
				Lego		*lego)
	: vertically_scrolled_item(text, filename, preferred_height),
	myLego(lego)
	{}

VerticalThing::VerticalThing(const VerticalThing &LO)
	: vertically_scrolled_item(LO),
	// force a disaster:
	myLego(NULL) {}

Lego::Lego(const Cstring &ltext, const Cstring &filename, int preferred_height)
	: vthing(new VerticalThing(ltext, filename, preferred_height, this)),
	LegendObject(ltext, filename, preferred_height) {
	List_iterator		actsisses(ACT_sys::Registration());
	List_iterator		hoppers(ACT_sys::Hoppers());
	Pointer_node		*ptr;
	btag			*tag;
	ACT_sys			*actsis;
	pointer_to_siblings	*ps;

	DBG_NOINDENT("LegendObject(\"" << get_key() << "\")::LegendObject().\n");

	if(GUI_Flag) {
		DBG_NOINDENT("adding pointers in theActual Heights for "
			<< ACT_sys::Registration().get_length()
			<< " registered objects...\n");
		while((ptr = (Pointer_node *) actsisses())) {
			actsis = (ACT_sys *) ptr->get_ptr();
			ps = new pointer_to_siblings(actsis->legend_display, preferredHeight);
			DBG_NOINDENT("Legend " << ltext << " inserting ptr to siblings @ " <<
				((void *) ps) << " into theActualHeights\n");
			vthing->theActualHeights << ps; }
		// hoppers are not registered...
		DBG_NOINDENT("adding pointers in theActual Heights for "
			<< ACT_sys::Hoppers().get_length() << " hopping objects...\n");
		while((tag = (btag *) hoppers())) {
			actsis = (ACT_sys *) tag->get_pointer();
			vthing->theActualHeights << new pointer_to_siblings(
							actsis->legend_display,
							preferredHeight); } } }

Lego::~Lego() {
	delete vthing;
	vthing = NULL; }

const Cstring &VerticalThing::get_theoretical_label() {
	if(GUI_Flag) {
	char	*lbuf;

	if (!actual_text.is_defined()) {
		int		txtwidth;
		Font		fid;
		XFontStruct	*fontstruct;
		char		*labelstr = (char *) *get_key();

		fontstruct = ResourceData.legend_font;
		txtwidth = XTextWidth(fontstruct, labelstr, strlen(labelstr) + 1);
		if (txtwidth > MAXLEGENDDSTRLEN - LABELPAD) {
			int	lablength = strlen(labelstr);
			char	*lbufend;

			lbuf = (char *) malloc(lablength + 1);
			lbufend = lbuf + lablength - 4;
			strcpy(lbuf, labelstr);
			strcpy(lbufend, "...");
			while(XTextWidth(fontstruct, lbuf, strlen(lbuf) + 1)
				> MAXLEGENDDSTRLEN - LABELPAD && lbufend != lbuf) {
				lbufend--;
				strcpy(lbufend, "..."); }
			actual_text = lbuf;
			free(lbuf); }
		else {
			actual_text = labelstr; } } }
	return actual_text; }

Cstring vertically_scrolled_item::get_label_text_from(motif_widget *w) {
	Cstring		ret;
	char		*string = NULL;
	XmString	label;

	XtVaGetValues(w->widget, XmNlabelString, &label, NULL);
	// Don't really know which method we used for defining the font... so try both and hope for the best:
	if(! XmStringGetLtoR(label, XmFONTLIST_DEFAULT_TAG, &string))
		XmStringGetLtoR(label, (char *)"tag1", &string);
	XmStringFree(label);
	ret = Cstring(string);
	if(string) XtFree(string);
	return ret; }

motif_widget *VerticalThing::get_toggle_button_for(motif_widget *scrolled_item) {
	// will be something more complicated in the resource legend case:
	return scrolled_item; }

void somethingHappened(Widget, callback_stuff *, void *) {
	// redundant because theUniversalCallback already does this, but it gives us something to do:
	udef_intfc::something_happened() = 1; }

// parent should be a form:
DS_draw_graph::DS_draw_graph(
		const Cstring	&N,
		motif_widget	*parent,
		int		VerticalScrollingDesired,
		int		HorizontalScrollingDesired,
		list_of_siblings *LS
		)
	: DS_graph(N, xmDrawingAreaWidgetClass, parent, NULL, 0),
	Vscrollbar(NULL),
	Hscrollbar(NULL),
	last_cursor_position(- 1),
	the_vertical_cursor_is_visible(0),
	Siblings(LS),
	TrackingTheLegendBoundary(0),
	theMaximumStaggeringOffset(0),
	current_horizontal_offset(0) {
	if(GUI_Flag) {
	DBG_NOINDENT("	DS_draw_graph::DS_draw_graph(\"" << N << "\")...\n"; cout.flush());

	if(HorizontalScrollingDesired) {
		ac = 0;
		XtSetArg(args[ac], XmNbackground, ResourceData.papaya_color); ac++;
		XtSetArg(args[ac], XmNorientation, XmHORIZONTAL); ac++;
		XtSetArg(args[ac], XmNmaximum, 2); ac++;
		XtSetArg(args[ac], XmNminimum, 0); ac++;
		XtSetArg(args[ac], XmNsliderSize, 2); ac++;
		Hscrollbar = new motif_widget(
			N + "_HorizScrollBar",
			xmScrollBarWidgetClass,	parent,
			args,				ac,
			TRUE);
		Hscrollbar->add_callback(somethingHappened, XmNdragCallback, NULL);
		Hscrollbar->add_callback(somethingHappened, XmNincrementCallback, NULL);
		Hscrollbar->add_callback(somethingHappened, XmNdecrementCallback, NULL);
		Hscrollbar->add_callback(somethingHappened, XmNpageIncrementCallback, NULL);
		Hscrollbar->add_callback(somethingHappened, XmNpageDecrementCallback, NULL);
		Hscrollbar->add_callback(somethingHappened, XmNtoTopCallback, NULL);
		Hscrollbar->add_callback(somethingHappened, XmNtoBottomCallback, NULL); }
	if(VerticalScrollingDesired) {
		ac = 0;
		XtSetArg(args[ac], XmNborderWidth, 0); ac++;
		XtSetArg(args[ac], XmNbackground, ResourceData.papaya_color); ac++;
		XtSetArg(args[ac], XmNmaximum, 2); ac++;
		XtSetArg(args[ac], XmNminimum, 0); ac++;
		XtSetArg(args[ac], XmNsliderSize, 2); ac++;
		Vscrollbar = new motif_widget(
			N + "VertScrollBar",
			xmScrollBarWidgetClass,	parent,
			args,				ac,
			TRUE);
		Vscrollbar->add_callback(somethingHappened, XmNdragCallback, NULL);
		Vscrollbar->add_callback(somethingHappened, XmNincrementCallback, NULL);
		Vscrollbar->add_callback(somethingHappened, XmNdecrementCallback, NULL);
		Vscrollbar->add_callback(somethingHappened, XmNpageIncrementCallback, NULL);
		Vscrollbar->add_callback(somethingHappened, XmNpageDecrementCallback, NULL);
		Vscrollbar->add_callback(somethingHappened, XmNtoTopCallback, NULL);
		Vscrollbar->add_callback(somethingHappened, XmNtoBottomCallback, NULL); }

	DBG_NOINDENT("DS_draw_graph::DS_draw_graph(" << get_key()
		<< "): attaching ExposeCallback for ExposureMask and StructureNotifyMask events.\n");
	add_event_handler(ExposeCallback, ExposureMask, this);
	add_event_handler(ExposeCallback, StructureNotifyMask, this); } }

int	DS_draw_graph::isVisible() {
	if(is_a_giant_window()) {
		// paned window
		return myParent->myParent->myParent->is_managed(); }
	// activity_display
	return myParent->myParent->is_managed(); }

    /* The only kinds of DS_scroll_graph objects for which this method does
     * something non-trivial are those for which get_Vscrollbar() returns
     * a non-zero pointer, i. e., ACT_sys and RD_sys.
     *
     * The owner of the vertically scrolled objects is the XX_legend object
     * (DS_legend or MW_legend), pointed to by the_controlling_dsgraph below. */
int DS_draw_graph::we_are_moving_a_dividing_line(void *void_event_ptr) {
	XButtonEvent	*bevent = (XButtonEvent *) void_event_ptr;
	VSI_owner	*v_o;
	motif_widget	*vsb;

	if(!(vsb = get_Vscrollbar(&v_o))) {
		return 0; }
	int             		we_are_close_to_a_dividing_line = 0;
	vertically_scrolled_item	*theOb = v_o->findTheVSIat(0, NULL);
	int    				bottom_of_VSI = v_o->vertical_distance_above_item(theOb);
	bpointernode 			*b;
	DS_scroll_graph			*the_controlling_dsgraph = v_o->get_this();
	List_iterator   		the_scrolled_items(v_o->pointersToTheVisibleLegends);

	DBG_INDENT("DS_scroll_graph::we_are_moving_a_dividing line START; "
			<< v_o->pointersToTheVisibleLegends.get_length() << " scrolled item(s).\n");
	if(!theOb) {
		DBG_UNINDENT("... no object at 0; returning.\n");
		return 0; }
	while((b = (bpointernode *) the_scrolled_items())) {
		theOb = (vertically_scrolled_item *) b->get_ptr();
		top_of_legend_being_resized = bottom_of_VSI;
		bottom_of_VSI += theOb->get_vertical_size_in_pixels_in(v_o);
		DBG_NOINDENT("  checking bottom at " << bottom_of_VSI << "...\n");
		if(    bevent->y - bottom_of_VSI < theTolerance &&
			bevent->y - bottom_of_VSI > - theTolerance) {
			we_are_close_to_a_dividing_line = 1;
			break; } }
	if(we_are_close_to_a_dividing_line) {
		DBG_NOINDENT("... we ARE close\n");
		TrackingTheLegendBoundary = 1;
		// minimum_height_limit =  bt->theDistanceToTheTop + ACTVERUNIT;
		minimum_height_limit =  bottom_of_VSI
						- theOb->get_vertical_size_in_pixels_in(v_o)
						+ v_o->get_this()->get_minimum_scrolled_item_height();
		pointer_to_legend_being_resized = theOb;
		last_sizing_pos = bottom_of_VSI;
		DS_time_scroll_graph::setcursor(EXPANSIONMODE, bevent->window);
		DBG_UNINDENT("DS_scroll_graph::we_are_moving_a_dividing_line(): Yep.\n");
		return 1; }
	DBG_UNINDENT("DS_scroll_graph::we_are_moving_a_dividing_line(): Nope.\n");
	return 0; }

void DS_draw_graph::ButtonReleased(XButtonEvent *eve) {
	vertically_scrolled_item	*theOb = pointer_to_legend_being_resized;

	if(!theOb) return;
	theOb->set_vertical_size_in_pixels_in(get_legend_graph(), last_sizing_pos
			- top_of_legend_being_resized);
	pointer_to_legend_being_resized = NULL; }

void DS_draw_graph::redraw_the_cute_grey_lines() {
	VSI_owner			*v_o = get_legend_graph();

	if(!v_o) {
		DBG_NOINDENT("    redraw_the_cute_grey_lines(): no VSI_owner, returning.\n");
		return; }
	motif_widget			*toggle_button;
	int				k;
	vertically_scrolled_item	*vsi = (vertically_scrolled_item *) v_o->findTheVSIat(0, NULL);
					// can easily be negative:
	int				top_of_leg = v_o->vertical_distance_above_item(vsi);
	DS_scroll_graph			*theLegendWidget = (DS_scroll_graph *) v_o->get_this();
	int				kmax = theLegendWidget->number_of_visible_units();

	DBG_NOINDENT(get_key() << "->redraw_the_cute_grey_lines: vsi at "
			<< ((void *) vsi) << "; the Legend widget says we have "
			<< kmax << " legends.\n");
	for(	k = 0;
		(k < kmax) &&
		(toggle_button = (motif_widget *) theLegendWidget->children[k]);
		k++
	  ) {
		if(vsi) {
			if(!DS_line::separation_gc) {
				XGCValues	gcvalue;

				gcvalue.fill_style = FillSolid;
				gcvalue.foreground = ResourceData.grey_color;
				gcvalue.line_width = 1;
				DS_line::separation_gc = new DS_gc(
					XtDisplay(widget),
						XtWindow(widget),
						GCForeground |
						GCLineWidth |
						GCFillStyle,
						&gcvalue); }
			top_of_leg += vsi->get_vertical_size_in_pixels_in(v_o);
			XDrawLine(Defdisplay,
				XtWindow(widget),
					// As per Stuart's request:
					// DS_object::gethilite()->getGC()
				DS_object::separation_gc->getGC(),
				0, top_of_leg,
				getwidth(), top_of_leg);
			if(top_of_leg >= getheight()) break;
			vsi = v_o->get_next_vsi(vsi); } } }

void DS_draw_graph::cleargraph() {
	if(isVisible()) {
		XClearArea(Defdisplay,
			// PIXMAP
			XtWindow(widget),
			0,             0,
			getwidth(),    getheight(),
			FALSE);
		redraw_the_cute_grey_lines(); } }

int DS_draw_graph::the_horizontal_scrollbar_is_out_of_sync(infoList* who) {
	if(!get_Hscrollbar()) {
		return 0; }
	if(isVisible()) {
		static int		x_max, actual_x_max, x_slider_size, actual_x_slider_size, page_inc, actual_page_inc,
					value, actual_value, x_min, actual_x_min;
		static CTime_base	t1, t2, timeunit = Siblings->timemark->get_time_unit();
		infoNode*		tl;
		infoList		*theDelegates = NULL, *theEnforcers = NULL;

		if(refresh_info::ON) {
			(*who) << (tl = new infoNode(get_key() + get_key()
				+ "::the_horizontal_scrollbar_is_out_of_sync() (server) "));
			theDelegates = &tl->payload.theListOfServers;
			theEnforcers = &tl->payload.theListOfAgents; }
		if(aButtonIsPressed()) {
			XtVaGetValues(get_Hscrollbar()->widget, XmNvalue, &actual_value, NULL);
			value = (int) (0.5 + (Siblings->dwtimestart() - theTimeAssociatedWithTheMinPosOfTheHorizScrollbar)
										/ timeunit);
			if(value != actual_value) {
				DBG_NOINDENT(get_key() << "->the_horizontal_scrollbar_is_out_of_sync(): value = "
					<< value << " while actual_value = "
					<< actual_value << ", so: YES\n");
				if(who) {
					tl->payload.reason << " value = " << value << " while actual_value = "
						<< actual_value << ", so: YES."; }
				return 1; }
			else {
				if(who) delete tl;
				return 0; } }

		XtVaGetValues(	get_Hscrollbar()->widget,
				XmNmaximum,		&actual_x_max,
				XmNminimum,		&actual_x_min,
				XmNvalue,		&actual_value,
				XmNsliderSize,		&actual_x_slider_size,
				XmNpageIncrement, 	&actual_page_inc,
				NULL);
		evaluate_horizontal_scrollbar_parameters(x_min, x_max, value, x_slider_size, page_inc, t1, t2);
		if(	x_min != actual_x_min
			|| x_max != actual_x_max
			|| value != actual_value
			|| x_slider_size != actual_x_slider_size
			|| actual_page_inc != page_inc) {
			DBG_NOINDENT(get_key() << "->the_horizontal_scrollbar_is_out_of_sync(): one or more horiz. "
				"scrollbar params are out of date, so: YES\n");
			if(who) {
				tl->payload.reason << " one or more horiz. scrollbar params are out of date, so: YES."; }
			return 1; }
		if(who) delete tl;
		return 0; }
	return 0; }


	/* SYNC AGENT responsible for fixing discrepancy between (1) actual position of
	* the vertical scrollbar and (2) what Siblings reports as the (desired) y_offset.
	*
	* This function does not make any sense unless the DS_scroll_graph owns both
	* a vertical scrollbar and a VSI_owner, for otherwise there is no way to figure
	* out how to size the scrollbar nor what to scroll.
	* */
void DS_draw_graph::do_the_vertical_scrollbar_thing(infoList* who) {
	VSI_owner		*v_o;
	motif_widget		*vsb;
	static int		y_max, y_slider_size, theValue;
	static infoNode*	tl;

	if(!(vsb = get_Vscrollbar(&v_o))) {
		return; }

	if(who) {
		tl = new infoNode(get_key() + "::do_the_vertical_scrollbar_thing() (agent)"); }

	XtVaGetValues(vsb->widget, XmNvalue, &theValue, NULL);
	if(theValue != v_o->y_offset_in_pixels()) {
		if(who) {
			tl->payload.reason << "according to Siblings, the value is " << Cstring(theValue) << ". "; }
		v_o->set_vertical_offset_to(theValue); }

	v_o->evaluate_vertical_scrollbar_parameters(y_max, y_slider_size);
	if(who) {
		tl->payload.reason << " evaluate_vertical_scrollbar_params:\n\twidget reports y_max = "
			<< Cstring(y_max) << ", y_slider_size " << Cstring(y_slider_size)
			<< ";\n\tSiblings reports y_offset should be "
			<< Cstring(v_o->y_offset_in_pixels()) << ". "; }
	DBG_NOINDENT("...updating scrollbar w/y_max = " << y_max <<
		", min = 0, value = " << v_o->y_offset_in_pixels() <<
		", sliderSize = " << y_slider_size << endl);
	XtVaSetValues(	vsb->widget,
		XmNmaximum,		y_max,
		XmNminimum,		0,
		XmNvalue,		v_o->y_offset_in_pixels(),
		XmNsliderSize,		y_slider_size,
		XmNincrement,		ACTVERUNIT,
		XmNpageIncrement,	y_slider_size,
		NULL); }

	/* SYNC SERVER responsible for detecting discrepancy between (1) actual position of
	* the vertical scrollbar and (2) Siblings->y_offset_in_pixels_for_ds_graph(this) */
int DS_draw_graph::the_vertical_scrollbar_is_out_of_sync(infoList* who) {
	VSI_owner		*v_o;
	motif_widget		*vsb;
	static int		y_max, y_slider_size;

	// Only non-trivial implementation is in DS_lndraw class. DS_lndraw::get_Vscrollbar() sets
	// v_o to its legend_display cast as a VSI_owner, so everybody's happy.
	if(!(vsb = get_Vscrollbar(&v_o))) {
		return 0; }

	if(isVisible()) {
		int			yo = v_o->y_offset_in_pixels();
		int			actual_y_max, actual_slider_size, actual_slider_value, y_slider_size, y_max;
		infoNode*		tl;

		DBG_INDENT(get_key() << "->the_vertical_scrollbar_is_out_of_sync START\n");
		if(who) {
			tl = new infoNode(get_key() + "::the_vertical_scrollbar_is_out_of_sync() (server)"); }

		XtVaGetValues(	vsb->widget,
				XmNvalue,		&actual_slider_value,
				NULL);
		if(yo != actual_slider_value) {
			DBG_NOINDENT(get_key() << "->the_vertical_scrollbar_is_out_of_sync(BUTTON PRESSED): value = " << yo
				<< " while actual_value = " << actual_slider_value << ", so: YES\n");
			if(who) {
				tl->payload.reason << " value = " << yo << " while actual_value = " << actual_slider_value
					<< ", so: YES. "; }
			DBG_UNINDENT("...DONE (out of sync).\n");
			return 1; }

		v_o->evaluate_vertical_scrollbar_parameters(y_max, y_slider_size);
		XtVaGetValues(vsb->widget,
				XmNmaximum,		&actual_y_max,
				XmNvalue,		&actual_slider_value,
				XmNsliderSize,		&actual_slider_size,
				NULL);
		if(	actual_y_max != y_max
			|| actual_slider_value != yo
			|| actual_slider_size != y_slider_size) {
			DBG_NOINDENT(get_key() << "->the_vertical_scrollbar_is_out_of_sync(): (max, val, siz) = " << actual_y_max
				<< " " << actual_slider_value << " " << actual_slider_size << " vs. theo. "
				<< y_max << " " << yo << " " << y_slider_size << ", so: YES\n");
			if(who) {
				tl->payload.reason << " (max, val, siz) = " << Cstring(actual_y_max) << " "
					<< Cstring(actual_slider_value) << " "
					<< Cstring(actual_slider_size) << " vs. theo. " << Cstring(y_max) << " "
					<< Cstring(yo) << " " << Cstring(y_slider_size) << ", so: YES"; }
			DBG_UNINDENT("...DONE (out of sync).\n");
			return 1; }
		else {
			if(who) delete tl;
			DBG_UNINDENT("...DONE (in sync).\n");
			return 0; } }
	return 0; }

void DS_draw_graph::evaluate_horizontal_scrollbar_parameters(
		int		&min_value,
		int		&max_value,
		int		&value,
		int		&slider_size,
		int		&page_inc,
		CTime_base	&theMinimumTime,
		CTime_base	&theMaximumTime) {
	static CTime_base		theTimeOfTheLastEvent,
					theStartTimeOfThePlan,
					theEndTimeOfThePlan,
					theTimeOfTheFirstEvent;
	CTime_base			theUnitOfTime = Siblings->timemark->get_time_unit();

	// We only want to use FirstEvent if some activities are present,
	// because otherwise we get today's date:
	if(model_intfc::FirstEvent(theTimeOfTheFirstEvent)) {
		model_intfc::LastEvent(theTimeOfTheLastEvent);
		if(Siblings->dwtimestart() < theTimeOfTheFirstEvent) {
			theStartTimeOfThePlan = Siblings->dwtimestart(); }
		else {
			theStartTimeOfThePlan = theTimeOfTheFirstEvent; }
		if((Siblings->dwtimestart() + Siblings->dwtimespan()) > theTimeOfTheLastEvent) {
			theEndTimeOfThePlan = Siblings->dwtimestart() + Siblings->dwtimespan(); }
		else {
			theEndTimeOfThePlan = theTimeOfTheLastEvent; } }
	else {
		theStartTimeOfThePlan = Siblings->dwtimestart();
		theEndTimeOfThePlan = Siblings->dwtimestart() + Siblings->dwtimespan(); }
	theMinimumTime = theStartTimeOfThePlan - Siblings->timemark->get_time_delta();
	theMaximumTime = theEndTimeOfThePlan + Siblings->timemark->get_time_delta();

	min_value = 0;
	max_value = (int) (0.5 + (theMaximumTime - theMinimumTime) / theUnitOfTime);
	// this is the amount by which we 'cheat' in sizing the scrollbar, so we can ALWAYS move forward and
	// backward in time:
	value = (int) (0.5 + (Siblings->dwtimestart() - theMinimumTime)  / theUnitOfTime);
	slider_size = (int) (0.5 + Siblings->dwtimespan() / theUnitOfTime);
	// page_inc = (int) ((Siblings->timemark->get_time_delta()) / theUnitOfTime);
	page_inc = slider_size;

	// Avoid Motif errors:
	if(slider_size < 1)
		slider_size = 1;
	if(page_inc < 1)
		page_inc = 1;
	if(max_value < value + slider_size)
		max_value = value + slider_size; }
