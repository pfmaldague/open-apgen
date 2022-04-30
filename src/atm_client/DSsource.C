#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include "apDEBUG.H"
#include <ActivityInstance.H>
#include "ACT_sys.H"

using namespace std;

DS_line_owner::DS_line_owner(Dsource* ds) : this_sort_of(ds) {
	// debug
	// cout << "new ds line owner for " << ds->get_key() << endl;
}

DS_line_owner::DS_line_owner() : this_sort_of(NULL) {
	// debug
	// cout << "new ds line owner for NULL!!\n";
}

void	derivedDataDS::addDerivedData(Dsource* ds, Cstring& errs) {
	if(ds->is_a_violation()) {
		ds->dataForDerivedClasses = new derivedDataVio(ds);
	} else if(ds->is_an_activity()) {
		ds->dataForDerivedClasses = new derivedDataDS(ds);
	} else {
		errs << "Error constructing activity instance - higher-order constructor failed\n";
	}
}

void	derivedDataDS::destroyDerivedData(Dsource *ds) {
	// no need to cast; destructor is virtual.
	if(ds->dataForDerivedClasses) {
		delete ds->dataForDerivedClasses;
		ds->dataForDerivedClasses = NULL;
	}
}

DS_gc *derivedDataDS::get_tilepattern(DS_lndraw *graph_obj) const {
	int pattern_value = 0;
	int color_value = 1;

	if(((ActivityInstance*)source)->Object->defines_property("color")) {
		const TypedValue& color_val = (*((ActivityInstance*)source)->Object)["color"];
		color_value = color_val.get_int();
	}
	if(((ActivityInstance*)source)->Object->defines_property("pattern")) {
		const TypedValue& pattern_val = (*((ActivityInstance*)source)->Object)["pattern"];
		pattern_value = pattern_val.get_int();
	}

	return graph_obj->gettilemap(
			pattern_value,
			color_value);
}

void	DS_line_owner::instantiate_graphics_for_act_sys(ACT_sys *act_sis) {
	static int	sx, ex;

	if(!act_sis->isVisible()) {
		return;
	}
	if(act_sis == ACT_sys::theHopper) {
		if(!this_sort_of->is_unscheduled()) {
			return;
		}
	} else  if(this_sort_of->is_unscheduled()) {
		return;
	}

	if(!this_sort_of->is_active()) {
		return;
	}

	if(find_instance_in(act_sis)) {
		return;
	} // the request is already displayed

	((derivedDataDS *) this_sort_of->dataForDerivedClasses)->compute_start_and_end_times_in_pixels(act_sis, sx, ex);
	act_sis->lnobjects << create_new_ds_line(
				((derivedDataDS *) this_sort_of->dataForDerivedClasses)->get_tilepattern(act_sis),
				(void *) XtWindow(act_sis->widget),
				sx,
				ex,
				LNDRAWWIDTH,
				this_sort_of,
				act_sis);
}

DS_line	*DS_line_owner::find_instance_in(ACT_sys *which_act_sis){
	Pointer_node	*ptrnode;
	DS_line		*line;

	if(!which_act_sis) return NULL ;
	for(    ptrnode = (Pointer_node *) theListOfPointersToDsLines.first_node();
		ptrnode;
		ptrnode = (Pointer_node *) ptrnode->next_node()) {
		if((line = (DS_line *) ptrnode->get_ptr())) {
			if(which_act_sis == (ACT_sys *) line->graphobj)
				return line; } }
	return NULL; }

void	derivedDataDS::handle_legend(LegendObject *){
	actsis_iterator	act_sisses;
	ACT_sys		*actsis;

	while((actsis = act_sisses())) {
		if(	source->is_active()
			|| (
				(source->is_decomposed() || source->is_abstracted())
				&& actsis->is_a_giant_window()
			   )
		  ) {
			// OBSERVER PATTERN
			instantiate_graphics_for_act_sys(actsis) ; } } }

void derivedDataDS::compute_start_and_end_times_in_pixels(
		DS_time_scroll_graph *in_this_actsis,
		int &sx,
		int &ex) {
	static CTime		T1, T2;
	static double		S1, S2, S3;

	S1 =	(
			((T1 = source->getetime()) - (T2 = in_this_actsis->get_siblings()->dwtimestart()))
				.convert_to_double_use_with_caution()
			/ (S3 = in_this_actsis->timeperhpixel())
		) + 0.5;
	sx = (int) S1;
	S2 = ((T1 + source->get_timespan() - T2).convert_to_double_use_with_caution() / S3) + 0.5;
	ex = (int) S2; }

DS_line	*DS_line_owner::create_new_ds_line(	DS_gc	*theGC,
						void	*theWindow,
						int sx, int ex,
						int LineWidth,
						Dsource* thisSoToSpeak,
						ACT_sys *actsis ) {
	DBG_NOINDENT(thisSoToSpeak->identify() << "create_new_ds_line called\n");
	// debug
	// cerr << "ds line for " << thisSoToSpeak->get_key() << " is given color " << theGC->cid << endl;
	DS_line		*line = new DS_line(
				*theGC,
				(Drawable) theWindow,
				sx,
				ex,
				LineWidth,
				thisSoToSpeak,
				actsis);

	line->settitle(thisSoToSpeak);
	return line; }

void derivedDataDS::instantiate_all_graphics() {
	actsis_iterator	act_sisses;
	ACT_sys		*actsis;

	DBG_INDENT("derivedDataDS::instantiate_all_graphics()\n");

	if(!source->get_legend()) {
		DBG_NOINDENT( "theLegend is NULL; attaching to generic legend.\n" );
		// last chance. Note: method does nothing if theLegend is already the right thing.
		source->switch_to_legend(LegendObject::theGenericLegend(source->get_APFplanname()));
	}

	while((actsis = act_sisses())) {
		// Note that the ACT_sys will not do anything if it is 'not visible',
		// since in that case it is not clear that it get basic info about itself
		// such as height, width etc.

		// We removed the 'update' flag, because that should always be done
		// separately (and presumably at a high level).

		// The function will do nothing and return if (1) the actsis is not visible, or
		// (2) if a DS_line already exists for this:
		if(source->is_unscheduled()) {
			DBG_NOINDENT("Unscheduled case...\n");
			if(actsis == ACT_sys::theHopper) {
				DBG_NOINDENT("(hopper case) Calling instantiate_graphics_for_act_sys...\n");
				instantiate_graphics_for_act_sys(actsis);
			}
		}
		else {
			DBG_NOINDENT( "Scheduled case...\n" ) ;
			if(actsis != ACT_sys::theHopper) {
				DBG_NOINDENT("(regular case) Calling instantiate_graphics_for_act_sys...\n");
				instantiate_graphics_for_act_sys(actsis);
			}
		}
	}
	DBG_UNINDENT( "instantiate_all DONE.\n" );
}

template class tlist<alpha_void, backptr<ActivityInstance>::ptr2p2b, short>;


