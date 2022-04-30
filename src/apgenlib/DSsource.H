#ifndef _DS_SOURCE_H
#define _DS_SOURCE_H

#include "APdata.H"
class ACT_sys;
class DS_gc;
class DS_time_scroll_graph;
class DS_line;
class DS_lndraw;

class DS_line_owner {
protected:
	List		theListOfPointersToDsLines;
	Dsource*	this_sort_of;
public:
	DS_line_owner(Dsource* ds); 
	DS_line_owner();
	~DS_line_owner() {}
	DS_line			*find_instance_in(ACT_sys *which_act_sis);
	DS_line			*get_representative_in(DS_time_scroll_graph *d);
	DS_line			*create_new_ds_line(	DS_gc	*,
							void	*theDrawable,
							int	sx,
							int	ex,
							int	LineWidth,
							Dsource* thisSoToSpeak,
							ACT_sys * );
	void			update_ds_line_gc();
	void			update_ds_line_coordinates();
	void			instantiate_graphics_for_act_sys(ACT_sys *);

				//
				// Pretty smart method -- knows about giant
				// windows and stuff
				//
	int			isDisplayedCorrectlyIn(DS_time_scroll_graph *);
	friend class DS_line;
};

class derivedDataDS: public derivedData, public DS_line_owner {
public:
	derivedDataDS(Dsource *s)
		: derivedData(s),
		DS_line_owner(s)
		{}
	derivedDataDS(const derivedData &DD)
		: derivedData(DD),
		DS_line_owner(DD.source)
		{}
	~derivedDataDS() {
		this_sort_of->dataForDerivedClasses->handle_instantiation(false, false); }

				//
				// Generates windows for all ds lines
				//
	void			handle_selection(bool selected);

	void			compute_start_and_end_times_in_pixels(
					DS_time_scroll_graph	*in_this_actsis,
					int			&start_coord,
					int			&end_coord);
	DS_line			*find_instance_in(ACT_sys *);
	void			handle_legend(LegendObject *);
	DS_gc			*get_tilepattern(DS_lndraw *) const;
	const Cstring		&label_to_use_on_display() const;

	void			handle_position() {
					update_ds_line_coordinates(); }

	void			handle_appearance() {
					update_ds_line_gc(); }

				//
				// Old delete_ds_lines method was split between
				// delete_active_ds_lines which deletes all of
				// them, and delete_all_ds_lines which does what
				// it says
				//
	void			handle_instantiation(bool present, bool active_only);

				//
				// not virtual:
				//
	void			instantiate_all_graphics();

	static void		addDerivedData(Dsource* S, Cstring& errs);
	static void		destroyDerivedData(Dsource* S);
};

class derivedDataVio: public derivedData, public DS_line_owner {
public:
	derivedDataVio(Dsource *s)
		: derivedData(s),
		DS_line_owner(s)
		{}
	derivedDataVio(const derivedData &DD)
		: derivedData(DD),
		DS_line_owner(DD.source)
		{}
	~derivedDataVio() {
		handle_instantiation(false, false); }

	void			handle_selection(bool) {}
	void			compute_start_and_end_times_in_pixels(
					DS_time_scroll_graph	*in_this_actsis,
					int			&start_coord,
					int			&end_coord) {}
	DS_line			*find_instance_in(ACT_sys *) { return NULL; }
	void			handle_legend(LegendObject *) {}
	DS_gc			*get_tilepattern(DS_lndraw *) const { return NULL; }
	const Cstring&		label_to_use_on_display() const {
		return Cstring::null();
	}
	void			handle_position() {}
	void			handle_appearance() {}
	void			handle_instantiation(bool present, bool active_only);
	void			instantiate_all_graphics() {}
};

#endif /* _DS_SOURCE_H */
