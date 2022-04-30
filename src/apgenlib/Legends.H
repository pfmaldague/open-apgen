#ifndef LEGENDS_H
#define LEGENDS_H

#include "APdata.H"

	/* A class that implements the "abstract" legend object; there is only one for each legend
	 in the core database. It provides support for the motif_widgets that implement the
	 graphic instantiation of legends on a specific ACT_sys display object. */
class LegendObject: public bstringnode, public back_pointer {
protected:
				// Are we selected?
	int			the_selection;
	// Constructor (private); normally, a factory method is used (see below)
	LegendObject(	
		const Cstring	&ltext,		// text of the legend
		const Cstring	&filename,	// file in which legend was defined (or NEW)
		int		preferred_height);// defaults to 32
public:
	// Copy constructor (unused but should be there by default)
	LegendObject(const LegendObject &);

	// destructor
	~LegendObject();

				// mandatory (pure virtual in Node)
	Node			*copy() { return new LegendObject(*this); }
				// the usual
	const Cstring		&get_key() const { return bstringnode::get_key(); }
				// File in which defined, or "New". Set by the constructor.
	Cstring			apf;
				/* This height defaults to 32 (pixels) and is saved/restored to/from
				 APF files. */
	int			preferredHeight;

				// for use in plotting
	virtual int		get_official_height() const;
				//
	virtual void		adjust_height(int new_height);
				// access method for GUI to use
	virtual void		set_selection(int k) { the_selection = k; }
				// access method for GUI to use
	virtual int		get_selection() const { return the_selection; }

				/* List of (smart) bpointernodes that point to the ACT_reqs
				 under this legend. Note that for consistency, these ACT_reqs
				 should mark which ACT_sys(es) they should be displayed in. */
	tlist<alpha_void, smart_actptr>	ActivityPointers;
				/* convenience constructor for default legend, used e. g.
				when constructing the first activity instance */
	static LegendObject*	theGenericLegend(const Cstring &desiredPlanName);

		// Factory methods for external use.

				/*
				This method is set to the default constructor by create_subsystems(),
				but can be overridden by apcore clients such as ATM_client.
				param filename	file in which legend was defined (or NEW)
				param preferred_height	defaults to 32
				*/
	static LegendObject	*(*theLegendObjectConstructorHandle)(
		const Cstring	&ltext,		
		const Cstring	&filename,	
		int		preferred_height);
				/* Default factory method, implemented in the core.
				param ltext	text of the legend
				param filename	file in which legend was defined (or NEW)
				param preferred_height	defaults to 32
				*/
	static LegendObject	*theLegendObjectConstructor(
		const Cstring	&ltext,	
		const Cstring	&filename,
		int		preferred_height);
				/* Convenience function; invokes the factory if one
				was installed, else returns NULL.
				param ltext	text of the legend
				param filename	file in which legend was defined (or NEW)
				param preferred_height	defaults to 32
				*/
	static LegendObject	*LegendObjectFactory(
		const Cstring	&ltext,	
		const Cstring	&filename,
		int		preferred_height);
				//
	};

#endif /* LEGENDS_H */
