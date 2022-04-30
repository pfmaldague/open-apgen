#ifndef UI_ABSPANZOOM_H
#define UI_ABSPANZOOM_H

#include "UI_motif_widget.H"

///
class UI_mainwindow;

class UI_abspanzoom : public motif_widget
	{
protected:

///
	void			adTimeSystemChange( Widget , callback_stuff *, void *);
///
	void			apzApplyButtonActivate( Widget , callback_stuff * , void * );
///
	void			apzCancelButtonActivate( Widget , callback_stuff * , void * );
///
	void			apzOKButtonActivate( Widget , callback_stuff * , void * );

///
	int			applyChanges ();
///
	void			create() ;

///
	UI_mainwindow		*mainWindow;

private:

public:

///
	UI_abspanzoom( const char * ) ;
///
	~UI_abspanzoom() ;
///
	static void initialize ( const Cstring &startTime , const Cstring &duration ) ;

	/// widgets:
	motif_widget	*_absPanZoomDialog;
///
	motif_widget	*_apzApplyButton ;
///
	motif_widget	*_apzButtonForm ;
///
	motif_widget	*_apzCancelButton ;
///
	motif_widget	*_apzCdtButton ;
///
	motif_widget	*_apzCstButton ;
///
	motif_widget	*_apzEdtButton ;
///
	motif_widget	*_apzEstButton ;
///
	motif_widget	*_apzMdtButton ;
///
	motif_widget	*_apzMstButton ;
///
	motif_widget	*_apzOKButton ;
///
	motif_widget	*_apzPdtButton ;
///
	motif_widget	*_apzPstButton ;
///
	motif_widget	*_apzStartTimeLabel ;
///
	motif_widget	*_apzStartTimeTF ;
///
	motif_widget	*_rtuiTF ;
///
	motif_widget	*_mtuiTF ;
///
	motif_widget	*_rtuiLabel ;
///
	motif_widget	*_mtuiLabel ;
///
	motif_widget	*_apzTimeSpanLabel ;
///
	motif_widget	*_apzTimeSpanTF ;
///
	motif_widget	*_apzTimeZoneOM ;
///
	motif_widget	*_apzTimeZonePM ;
///
	motif_widget	*_apzUtcButton ;

	/// callbacks:

	static void adTimeSystemChangeCallback( Widget , callback_stuff * , void * ) ;
///
	static void apzApplyButtonActivateCallback( Widget , callback_stuff * , void * ) ;
///
	static void apzCancelButtonActivateCallback( Widget , callback_stuff * , void * ) ;
///
	static void apzOKButtonActivateCallback ( Widget , callback_stuff * , void *);
	} ;
#endif

