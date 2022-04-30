/*
 *	Project:	APGEN
 *	Subsystem:	User Interface
 *	File Name:	XM.h
 *
 *	Description:	User Interface in X using Motif Routines
 *
 *	Date		Revision	Author		Reason for change
 *-----------------------------------------------------------------------
 *	10/1/94    0.0       I. Lin		Original Release
 */

#ifndef U_XM_H
#define U_XM_H

#include <sys/types.h>

#ifdef _WIN32
///
extern "C" {
#endif

#include <Xm/Xm.h>

#ifdef _WIN32
}
#endif

///
typedef struct 
{
    Display       *display;	/*  pointer to display structure	     */
    int	        screen;	/*  screen number for display (i.e., 0)	*/
    int	        bell;	     /*  flag for bell being audible		*/
    Colormap      cmap;	     /*  Storage area for colormap		     */
    XmFontList    font;		/*  Main default fontlist		     */
    XtAppContext  context;	/*  Default applicaton context		*/
} ///
 GRDisplay;		/* GRaphics Display structure for application.	*/

/*margin for top APGEN window*/
#define SHELLVMARGIN 20
#define SHELLHMARGIN 20

#define BITMAPDEPTH  1

/**structure for bitmap*/
typedef struct
{
    int width ;
    int height ;
    char* bitmapdata ;
} ///
 BITMAPDEF ;


#ifndef _NO_PROTO

///
extern void   xm_main_loop(Widget);
///
extern Widget xm_create_main_window(char*,char*,char*,char*) ;
///
extern Widget xm_create_popup_window(Widget,char*) ;
///
extern void   xm_post_window(Widget);
///
extern void   xm_sense(Widget) ;
///
extern void   xm_insense(Widget) ;
///
extern void   xm_unpost_window(Widget);
///
extern GRDisplay* xdw ;

/*xlib calls
extern GC     XCreateGC(Display*,Drawable,unsigned long,XGCValues*) ;
extern XFreeGC(Display*,GC) ;
extern XDrawLine(Display*,Drawable,GC,int,int,int,int) ;

xt calls
extern Widget XtCreateManagedWidget(char*,WidgetClass,Widget,ArgList,Cardinal) ;
extern void   XtAddCallback(Widget,char*,XtCallbackProc,XtPointer) ;
extern void   XtSetValues(Widget,Arglist,Cardinal) ;
extern Screen XtScreen(Widget) ;
extern Display* XtDisplay(Widget) ;
extern Widget XtParent(Widget) ;

*/

#else

///
extern void   xm_main_loop();
///
extern Widget xm_create_main_window() ;
///
extern Widget xm_create_popup_window() ;
///
extern void   xm_post_window();
///
extern void   xm_sense() ;
///
extern void   xm_insense() ;
///
extern void   xm_unpost_window();


#endif
/*end of ifndef _NO_PROTO*/





#endif
/*end of ifndef U_XM_H*/

