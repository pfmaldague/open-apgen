#ifndef OPENFSDCLASSDERIVED_H
#define OPENFSDCLASSDERIVED_H

#include "UI_motif_widget.H"

///
class UI_mainwindow ;

class UI_openfsd: public motif_widget {
public:
///
	UI_openfsd( const char * ) ;
///
	~UI_openfsd() ;

///
	static void		initialize() ;
				///
	static bool		disabled;
///
	static void		openFSDOkCallback( Widget , callback_stuff * , XtPointer) ;

///
	void			openFSDOk(Widget, XtPointer, XtPointer) ;
///
	void			create(Widget) ;
///
	void			setBackgroundColors () ;
///
	static void		pop_the_discrepancy_panel () ;

///
	motif_widget		*sel_box ;
///
	UI_mainwindow		*mainWindow ; } ;

#endif

