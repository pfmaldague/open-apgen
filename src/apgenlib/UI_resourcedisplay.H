#ifndef UI_resourcedisplaybx_H
#define UI_resourcedisplaybx_H

#include "UI_motif_widget.H"
#include "RD_sys.H"


///
class	ACT_sys ;
class	activity_display ;

class resource_display: public motif_widget {
public:
///
	resource_display( activity_display * , const Cstring & ) ;
///
	~resource_display() {}

protected:
				/** The top-level form on which RD_sys and associated
				 * widgets (legend, scrollbar) will be laid out.*/
	motif_widget		*_resourceDisplayForm;

				///
	motif_widget		*_rdSelAllButton , *_rdDownArrowButton , *_rdUpArrowButton ;
				///
	static void		rdSelAllButtonActivateCallback(	Widget , callback_stuff * , void * ) ;
				///
	static void		rdDownArrowActivateCallback(	Widget , callback_stuff * , void * ) ;
				///
	static void		rdUpArrowActivateCallback(	Widget , callback_stuff * , void * ) ;

				/// Node-mandated function.
	Node			*copy() { return new motif_widget( *this ) ; }

	/// Callback to interface with Motif.
	static void rdSelectedTBValueChangedCallback( Widget , callback_stuff * , void * ) ;

public:

				/// Top-level object in a functional (not display) sense.
	RD_sys			*resourceSystem;
				/** The ACT_sys "in charge" that the RD_sys is associated
				 * with. */
	ACT_sys			*activitySystem ;

				///
	void			initialize() ;
				///
	void			uninitialize() ;
				///
	void			manage() ;
				///
	int			numResourcesInDisplay() ;
				///
	apgen::RETURN_STATUS		addResourceToDisplay ( const Cstring & , Cstring &errorsIfAny ) ;
				///
	apgen::RETURN_STATUS		removeResourceFromDisplay( const Cstring &theResource ) ;
				///
	void			setControlInfo(	const Cstring	&theResource ,
						RD_YSCROLL_MODE	mode ,
						int		resolution ) ;
				///
	void			getControlInfo( const Cstring &theResource , RD_YSCROLL_MODE &mode ,int &resolution ) ;

				/**The next 2 methods are not quite symmetric (hence the name difference):
				 * yabsscrollzoom does the action (silent if it cannot); getZoomScrollInfo
				 * gets the defining info, and reports whether it was able to do so */
	void			yabsscrollzoom(
					const Cstring	&theResource ,
					double		min ,
					double		span ) ;

				/** Returns the parameters that define optional zooming/panning within
				 * a resource display. Not totally clear that we want this. In any case,
				 * there is no analogous thing in the ACT world, so it's OK to keep this
				 * method around. */
	apgen::RETURN_STATUS		getZoomScrollInfo(	const Cstring	&theResource ,
							Cstring		&min ,
							Cstring		&span ) ;

				/// Emulate activity_display class.
	void			initialize_res_sys() ;

				///
	void			sensitizeLegendsSelectedButtons() ;
				///
	void			desensitizeLegendsSelectedButtons() ;
				///
	int			LegendsSelectedButtonsAreSensitive() ;
				///
	void			purge () ; } ;

#endif
