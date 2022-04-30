#if HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef GUI
/*
 * srComboBox.c - This is an attempt to try and mimick the idea of a combox
 * box from Microsoft Windows. It consists of a label , text and
 *		scrollable list widget.
 *	
 *	Author:		Mekki EL Boushi
 *	Date:		12/1/94
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
/* #include <Xm/DialogS.h> */
#include <Xm/ArrowB.h>
#include <Xm/TextF.h>
#include <Xm/List.h>
#include <Xm/LabelP.h>
#include <string.h>
#include <ctype.h>

#include "srComboBoxP.h"

#ifndef WIN32
#else
#define strcasecmp _stricmp
#endif

#include <stdio.h>

/* Make sure to define function prototyping*/
#ifndef FUNCPROTO
#define FUNCPROTO
#endif

#ifdef DEBUG
#define LOG( p1 ) fprintf( stderr , p1 ) ;
#define LOG2( p1 , p2 ) fprintf( stderr , p1 , p2 ) ;
#else
#define LOG( p1 )
#define LOG2( p1 , p2 )
#endif

#define offset( field ) XtOffsetOf( XmsrComboBoxRec , field )
static XtResource resources[] = {
	{
	XmNeditable , XmCEditable , XmRBoolean , sizeof( Boolean ) , 
	offset( combobox.Editable ) , XmRString , "False"
	} , 
	{
	XmNsorted , XmCSorted , XmRBoolean , sizeof( Boolean ) ,
	offset( combobox.Sorted ) , XmRString , "False"
	} , 
	{
	XmNvisibleItemCount , XmCVisibleItemCount , XmRInt , sizeof( int ) , 
	offset( combobox.VisibleItemCount ) , XmRImmediate , ( XtPointer ) 8
	} , 
	{
	XmNfontList , XmCFontList , XmRFontList , sizeof( XmFontList ) , 
	offset( combobox.Font ) , XmRImmediate , NULL
	} , 
	{
	XmNselectionCallback , XmCSelectionCallback , XmRCallback , 
	sizeof( XtCallbackList ) , 
	offset( combobox.SelectionCBL ) , XmRCallback , NULL
	} , 
	{
	XmNdropDownCallback , XmCDropDownCallback , XmRCallback , 
	sizeof( XtCallbackList ) , 
	offset( combobox.DropDownCBL ) , XmRCallback , NULL
	} , 
	{
	XmNpersistentDropDown , XmCPersistentDropDown , XmRBoolean , 
	sizeof( Boolean ) , 
	offset( combobox.Persistent ) , XmRString , "False"
	} , 
	{
	XmNtwmHandlingOn , XmCTwmHandlingOn , XmRBoolean , sizeof( Boolean ) , 
	/* PFM CHANGE >>> */
	offset( combobox.TwmHandlingOn ) , XmRString , "False"
	/* <<< PFM CHANGE
	offset( combobox.TwmHandlingOn ) , XmRString , "True"
	(CHANGED BACK...) */
	} , 
	{
	XmNshowLabel , XmCShowLabel , XmRBoolean , sizeof( Boolean ) , 
	offset( combobox.ShowLabel ) , XmRString , "False"
	} , 
	{
	XmNdropDownOffset , XmCDropDownOffset , XmRPosition , 
	sizeof( Position ) , offset( combobox.DropDownOffset ) , 
	XmRImmediate , ( XtPointer ) -1
	} , 
	{
	XmNborderWidth , XmCBorderWidth , XmRDimension , sizeof( Dimension ) , 
	offset( core.border_width ) , XmRImmediate , ( XtPointer ) 0
	} ,
	{
	XmNdropDownCursor , XmCDropDownCursor , XmRCursor , sizeof( Cursor ) ,
	offset( combobox.ArrowCursor ) , XmRString , "center_ptr"
	}
	} ; /* resources[] */

static void Initialize( Widget , XmsrComboBoxWidget , ArgList , 
					Cardinal * ) ;
static void Destroy( XmsrComboBoxWidget ) ;
static void Resize( XmsrComboBoxWidget ) ;
static Boolean SetValues( XmsrComboBoxWidget , XmsrComboBoxWidget , 
				XmsrComboBoxWidget , ArgList , Cardinal * ) ;
static void GetValuesAlmost( XmsrComboBoxWidget , ArgList , Cardinal * ) ;
static XtGeometryResult QueryGeometry( XmsrComboBoxWidget , XtWidgetGeometry * , 
					XtWidgetGeometry * ) ;
static XtGeometryResult GeometryManager( XmsrComboBoxWidget , XtWidgetGeometry * , 
			XtWidgetGeometry * ) ;
static void		ClassInit() ;
static void ShowHideDropDownList( XmsrComboBoxWidget w , XEvent *event , 
			Boolean Show ) ;
static void ShellCallback( Widget w , XtPointer cbw , 
			XEvent *event , Boolean *ContDispatch ) ;
/*
 * Class Definition
 */
XmsrComboBoxClassRec xmsrComboBoxClassRec = {
	{ /*** core-Klasse ***/
	/* superclass				*/	( WidgetClass ) &xmManagerClassRec , 
	/* class_name				*/	"XmsrComboBox" ,
	/* widget_size				*/	sizeof( XmsrComboBoxRec ) ,
	/* class_initialize 			*/	( XtProc ) ClassInit ,
	/* class_part_initialize		*/	NULL ,
	/* class_inited 			*/	False ,
	/* initialize				*/	( XtInitProc ) Initialize ,
	/* initialize_hook			*/	NULL ,
	/* realize				*/	XtInheritRealize ,
	/* actions				*/	NULL ,
	/* num_actions				*/	0 ,
	/* resources				*/	resources ,
	/* num_resources			*/	XtNumber( resources ) ,
	/* xrm_class				*/	NULLQUARK ,
	/* compress_motion			*/	True ,
	/* compress_exposure 			*/	XtExposeCompressMultiple ,
	/* compress_enterleave			*/	True ,
	/* visible_interest			*/	False ,
	/* destroy				*/	( XtWidgetProc ) Destroy ,
	/* resize				*/	( XtWidgetProc ) Resize ,
	/* expose				*/	NULL ,
	/* set_values				*/	( XtSetValuesFunc ) SetValues ,
	/* set_values_hook			*/	NULL ,
	/* set_values_almost			*/	XtInheritSetValuesAlmost ,
	/* get_values_hook			*/	( XtArgsProc ) GetValuesAlmost ,
	/* accept_focus				*/	NULL ,
	/* version				*/	XtVersion ,
	/* callback_private 			*/	NULL ,
	/* tm_table				*/	NULL ,
	/* query_geometry			*/	( XtGeometryHandler ) QueryGeometry ,
	/* display_accelerator			*/	XtInheritDisplayAccelerator ,
	/* extension 				*/	NULL
	} , 
	{ /*** composite Class ***/
	/* geometry_manager			*/	( XtGeometryHandler ) GeometryManager ,
	/* change_managed			*/	XtInheritChangeManaged ,
	/* insert_child				*/	XtInheritInsertChild ,
	/* delete_child				*/	XtInheritDeleteChild ,
	/* extension				*/	NULL
	} , 
	{ /*** constraint Class ***/
	/* resources				*/	NULL ,
	/* num_resources			*/	0 ,
	/* constraint_size			*/	sizeof( XmManagerConstraintPart ) ,
	/* initialize				*/	NULL ,
	/* destroy				*/	NULL ,
	/* set_values				*/	NULL ,
	/* extension				*/	NULL
	} , 
	{ /*** xmManager Class ***/
	/* translations				*/	XtInheritTranslations ,
	/* syn_resources			*/	NULL ,
	/* num_syn_resources			*/	0 ,
	/* syn_constraint_resources		*/	NULL ,
	/* num_syn_constraint_resources		*/	0 ,
	/* parent_process			*/	XmInheritParentProcess ,
	/* extension				*/	NULL
	} , 
	{ /*** combobox Class ***/
	/*					*/	0
	}
	} ; /* xmsrComboBoxClassRec */

WidgetClass xmsrComboBoxWidgetClass = ( WidgetClass ) &xmsrComboBoxClassRec ;

/* --------------------------------------------------------------------
 * Translation-Table
 */
static char newEditTranslations[] =
	"Alt<Key>osfDown:		srComboBox-Manager( show-hide-list )	\n\
	Meta<Key>osfDown:		srComboBox-Manager( show-hide-list )	\n\
	Alt<Key>osfUp:			srComboBox-Manager( hide-list )		\n\
	Meta<Key>osfUp:			srComboBox-Manager( hide-list )		\n\
	<Key>osfUp:			srComboBox-Manager( up )		\n\
	<Key>osfDown:			srComboBox-Manager( down )		\n\
	<Key>osfCancel:			srComboBox-Manager( cancel )		\n\
	<Key>Return:			srComboBox-Manager( activate ) activate()"
		;
static char newEditTranslationsNE[] =
	"<Key>osfBeginLine:		srComboBox-Manager( top )		\n\
	<Key>osfEndLine:		srComboBox-Manager( bottom )				"
		;
#ifdef NODRAGNDROP
static char newListTranslations[] =
	"<Btn2Down>:			srComboBox-Manager( no-operation )	" ;
#endif
	
static void CBoxManager( Widget w , XEvent *event , String *params , 
			Cardinal *num_params ) ;

static XtActionsRec actions[] = {
	{ "srComboBox-Manager" , CBoxManager } ,
	{ NULL , NULL }
	} ; /* actions */

	
static XtTranslations NewEditTranslations , NewEditTranslationsNE , 
			NewListTranslations ;

static XtConvertArgRec ConverterScreenConvertArg[] = {
	{ XtBaseOffset , ( XtPointer ) NULL + XtOffset( Widget , core.screen ) , 
	sizeof( Screen * ) }
	} ;

static void ClassInit()
	{
	NewEditTranslations =
			XtParseTranslationTable( newEditTranslations ) ;
	NewEditTranslationsNE =
			XtParseTranslationTable( newEditTranslationsNE ) ;
#ifdef NODRAGNDROP
	NewListTranslations =
			XtParseTranslationTable( newListTranslations ) ;
#endif
/*
	XtAddConverter( XtRString , XtRBitmap , 
			XmuCvtStringToBitmap , 
			ConverterScreenConvertArg , 
			XtNumber( ConverterScreenConvertArg ) ) ;
				*/
	} /* ClassInit */

/* PFM CHANGE >>> ... CHANGE BACK */
static Window GetDecorationWindow( XmsrComboBoxWidget w )
	{
	Window Root , Parent , AWindow ;
	Window *Children ;
	unsigned int NumChildren ;
	
	Parent = XtWindow( ( Widget ) w ) ;
	do
		{
		AWindow = Parent ;
		XQueryTree( XtDisplay( ( Widget ) w ) , AWindow , 
			&Root , &Parent , &Children , &NumChildren ) ;
		XFree( ( char * ) Children ) ;
		}
		while ( Parent != Root ) ;
	return AWindow ;
	} /* GetDecorationWindow */
/* <<< PFM CHANGE */

static Boolean DelayedFocusOutWorkProc( XtPointer pClientData ) ;

static void Destroy( XmsrComboBoxWidget w )
	{
/* fprintf( stderr , "Destroy: %08X\n" , w->core.window ) ;*/
	if ( w->combobox.ConvertBitmapToPixmap )
	XFreePixmap( XtDisplay( ( Widget ) w ) , 
			w->combobox.LabelPixmap ) ;
	if ( w->combobox.ConvertBitmapToPixmapInsensitive )
	XFreePixmap( XtDisplay( ( Widget ) w ) , 
			w->combobox.LabelInsensitivePixmap ) ;
	/* SIMPLIFY
	if ( w->combobox.PendingFocusOut )
		XtRemoveWorkProc( w->combobox.WorkProcID ) ;
	*/
	XtRemoveEventHandler( w->combobox.MyNextShell , 
		StructureNotifyMask | FocusChangeMask | ExposureMask , 
		False , ( XtEventHandler ) ShellCallback , 
		( XtPointer ) w ) ;
	} /* Destroy */

static void DefaultGeometry(
			XmsrComboBoxWidget w , 
			Dimension *TotalWidth , 
			Dimension *TotalHeight , 
			Dimension *EditCtrlWidth , 
			Dimension *LabelCtrlWidth )
	{
	XtWidgetGeometry EditGeom , ArrowGeom , LabelGeom ;
	
	XtQueryGeometry( w->combobox.EditCtrl , NULL , &EditGeom ) ;
	XtQueryGeometry( w->combobox.ArrowCtrl , NULL , &ArrowGeom ) ;
	XtQueryGeometry( w->combobox.LabelCtrl , NULL , &LabelGeom ) ;
	*TotalWidth = EditGeom.width + ArrowGeom.width ;
	if ( w->combobox.ShowLabel )
	*TotalWidth += LabelGeom.width ;
	if ( w->combobox.Editable ) *TotalWidth += ArrowGeom.width/2 ;
	*TotalHeight = EditGeom.height ;
	*EditCtrlWidth = EditGeom.width ;
	*LabelCtrlWidth = LabelGeom.width ;
	} /* DefaultGeometry */

static int WidgetToScreen( Widget w )
	{
	Screen *screen ;
	Display *display ;
	int NumScreens , i ;
	
	screen = XtScreen( w ) ; NumScreens = ScreenCount( XtDisplay( w ) ) ;
	display = DisplayOfScreen( screen ) ;
	for ( i = 0 ; i < NumScreens ; ++i )
	if ( ScreenOfDisplay( display , i ) == screen )
			return i ;
	XtError( "WidgetToScreen: data structures are destroyed." ) ;
	// to make compiler happy:
	return(-1);
	} /* WidgetToScreen */

static void DoDropDownLayout( XmsrComboBoxWidget w )
	{
	Position abs_x , abs_y ;
	Dimension ArrowWidth , ListWidth , ListHeight , LabelWidth ;
	Window Decoration ;
	XWindowChanges WindowChanges ;

	if ( !w->combobox.ListVisible )
		{
		return ;
		}

	XtVaGetValues( w->combobox.ArrowCtrl , 
		XmNwidth , &ArrowWidth , NULL ) ;
	XtTranslateCoords( ( Widget ) w , 0 , w->core.height , &abs_x , &abs_y ) ;
	if ( w->combobox.DropDownOffset < 0 )
	w->combobox.DropDownOffset = ArrowWidth + 2 ;
	ListWidth = w->core.width - w->combobox.DropDownOffset - 2 ;
	abs_x += w->combobox.DropDownOffset ;
	if ( w->combobox.ShowLabel )
		{
		XtVaGetValues( w->combobox.LabelCtrl , XmNwidth , &LabelWidth , NULL ) ;
		ListWidth -= LabelWidth ;
		abs_x += LabelWidth ;
		}
	if ( ListWidth < 20 ) ListWidth = 20 ;
	XtVaGetValues( w->combobox.ListCtrl , XmNheight , &ListHeight , NULL ) ;
	XtConfigureWidget( w->combobox.PopupShell , 
		abs_x , abs_y , ListWidth , ListHeight , 1 ) ;

	/* TEST */
	if ( XtIsRealized( ( Widget ) w ) )
		{
		WindowChanges.sibling = GetDecorationWindow( w ) ;
		/* This change works w/mwmbut not dnp's: */
		/* WindowChanges.sibling =  XtWindow( ( Widget ) w ) ; */
		WindowChanges.stack_mode = Above ;
		XReconfigureWMWindow( XtDisplay( ( Widget ) w ) , 
			XtWindow( w->combobox.PopupShell ) , 
			WidgetToScreen( w->combobox.PopupShell ) , 
			CWSibling | CWStackMode , &WindowChanges ) ;
		}
	} /* DoDropDownLayout */

static void DoLayout( XmsrComboBoxWidget w )
	{
	Dimension EditCtrlWidth , ArrowCtrlWidth , LabelCtrlWidth ;
	Dimension srComboBoxHeight ;
	Dimension BorderWidth ;
	Dimension HighlightThickness ;
	Position EditX ;
	
	srComboBoxHeight = w->core.height ;
	XtVaGetValues( w->combobox.ArrowCtrl ,
			XmNwidth , &ArrowCtrlWidth , NULL ) ;
	XtVaGetValues( w->combobox.LabelCtrl , 
			XmNwidth , &LabelCtrlWidth , NULL ) ;
	EditCtrlWidth = w->core.width - ArrowCtrlWidth ;
	if ( w->combobox.Editable ) EditCtrlWidth -= ArrowCtrlWidth/2 ;
	if ( w->combobox.ShowLabel )
		{
		EditX = LabelCtrlWidth ;
		EditCtrlWidth -= LabelCtrlWidth ;
		}
	else
		EditX = 0 ;
	if ( EditCtrlWidth < 20 ) EditCtrlWidth = 20 ;
	XtVaGetValues( w->combobox.EditCtrl , 
			XmNborderWidth , &BorderWidth , 
			XmNhighlightThickness , &HighlightThickness , 
			NULL ) ;
	XtConfigureWidget( w->combobox.EditCtrl , 
			EditX , 0 , 
			EditCtrlWidth , srComboBoxHeight , BorderWidth ) ;
	XtVaGetValues( w->combobox.ArrowCtrl , 
			XtNborderWidth , &BorderWidth , NULL ) ;
	XtConfigureWidget( w->combobox.ArrowCtrl , 
			w->core.width-ArrowCtrlWidth , HighlightThickness , 
			ArrowCtrlWidth , 
			srComboBoxHeight - 2 * HighlightThickness , 
			BorderWidth ) ;
	if ( w->combobox.ShowLabel )
		{
		XtVaGetValues( w->combobox.LabelCtrl , 
				XmNborderWidth , &BorderWidth , 
				NULL ) ;
		XtConfigureWidget( w->combobox.LabelCtrl , 
			0 , 0 , LabelCtrlWidth , srComboBoxHeight , 
			BorderWidth ) ;
		}
	if ( w->combobox.ListVisible )
		{
		DoDropDownLayout( w ) ;
		}
	} /* DoLayout */

static XtGeometryResult QueryGeometry( XmsrComboBoxWidget w , 
			XtWidgetGeometry *Request , 
					XtWidgetGeometry *Reply )
	{
	XtGeometryResult result = XtGeometryYes ;
	Dimension minW , minH , editW , labelW ;
	
	Request->request_mode &= CWWidth | CWHeight ;
	if ( Request->request_mode == 0 ) return result ;

	DefaultGeometry( w , &minW , &minH , &editW , &labelW ) ;

	if ( Request->request_mode & CWWidth )
		{
		if ( Request->width < minW )
			{
			result = XtGeometryAlmost ;
			Reply->width = minW ;
			Reply->request_mode |= CWWidth ;
			}
		}
	if ( Request->request_mode & CWHeight )
		{
		if ( Request->height < minH )
			{
			result = XtGeometryAlmost ;
			Reply->height = minH ;
			Reply->request_mode |= CWHeight ;
			}
		}
	return result ;
	} /* QueryGeometry */

static void Resize( XmsrComboBoxWidget w )
	{
	DoLayout( w ) ;
	} /* Resize */

static void ShellCallback( Widget w , XtPointer pClientData , 
			XEvent *event , Boolean *ContDispatch )
	{
	XmsrComboBoxWidget cbw = ( XmsrComboBoxWidget ) pClientData ;
	XWindowChanges WindowChanges ;
	
	switch ( event->type )
		{
		case ConfigureNotify:
		case CirculateNotify:
			DoDropDownLayout( ( XmsrComboBoxWidget ) cbw ) ;
			break ;
		case FocusOut:
			LOG2( "ShellCallback: FocusOut , mode: %i\n" ,
				event->xfocus.mode ) ;
			/* SIMPLIFY
			if ( cbw->combobox.Persistent )
				cbw->combobox.IgnoreFocusOut = True ;
			else if ( ( event->xfocus.mode == NotifyGrab ) &&
				cbw->combobox.ListVisible )
				cbw->combobox.IgnoreFocusOut = True ;
			*/
			break ;
		case UnmapNotify:
			ShowHideDropDownList( ( XmsrComboBoxWidget ) cbw , 
				event , False ) ;
			break ;
		case Expose:
			if( cbw->combobox.ListVisible )
				{
				WindowChanges.sibling =  XtWindow( ( Widget ) cbw ) ;
				WindowChanges.stack_mode = Above ;
				XReconfigureWMWindow( XtDisplay( ( Widget ) cbw ) , 
					XtWindow( cbw->combobox.PopupShell ) , 
					WidgetToScreen( cbw->combobox.PopupShell ) , 
					CWSibling | CWStackMode , &WindowChanges ) ;
				}
			break ;
		}
	*ContDispatch = True ;
	} /* ShellCallback */

static void OverrideShellCallback( Widget w , XtPointer pClientData ,
	XEvent *event , Boolean *ContDispatch )

	{
	XmsrComboBoxWidget cbw = ( XmsrComboBoxWidget ) pClientData ;

	switch ( event->type )
		{
		case EnterNotify:
			/*	SIMPLIFY
			LOG2( "OverrideShellCallback: EnterNotify , PendingFO: %s\n" , 
				cbw->combobox.PendingFocusOut ? "True" : "False" ) ;
			if ( cbw->combobox.PendingFocusOut )
				cbw->combobox.IgnoreFocusOut = True ;

			if ( cbw->combobox.TwmHandlingOn )
				cbw->combobox.PendingOverrideInOut = True ;
			*/
			break ;
		case LeaveNotify:
			/* SIMPLIFY
			LOG( "OverrideShellCallback: LeaveNotify\n" ) ;
			if ( cbw->combobox.TwmHandlingOn )
				cbw->combobox.PendingOverrideInOut = True ;
			*/
			break ;
		}
	} /* OverrideShellCallback */

static void ArrowCrossingCallback( Widget w , XtPointer pClientData ,
			XEvent *event , Boolean *ContDispatch )
	{
	XmsrComboBoxWidget cbw = ( XmsrComboBoxWidget ) pClientData ;
	switch ( event->type )
		{
		case LeaveNotify:
			/* SIMPLIFY
			LOG2( "ArrowCrossingCallback: LeaveNotify , mode: %i\n" , 
				event->xcrossing.mode ) ;
			if ( event->xcrossing.mode == NotifyGrab )
				cbw->combobox.IgnoreFocusOut = True ;
			else
				cbw->combobox.IgnoreFocusOut = False ;
			*/
			break ;
		}
	} /* ArrowCrossingCallback */

static void HelpCallback( Widget w , XtPointer cbw , XtPointer CallData )
	{
	XtCallCallbacks( ( Widget ) cbw , XmNhelpCallback , CallData ) ;
	} /* HelpCallback */

static XtGeometryResult GeometryManager(
			XmsrComboBoxWidget w , 
			XtWidgetGeometry *Request , 
			XtWidgetGeometry *Reply )
	{
	return XtGeometryNo ; /* Was ICH hier vorgegeben habe , gilt! */
	} /* GeometryManager */

#define BOTTOMSHADOWCOLOR 0x0001
#define TOPSHADOWCOLOR			0x0002
#define FOREGROUND			0x0004
#define BACKGROUND			0x0008

static struct { String Resource ; int Flag ; }
	ColorResources[] = {
			{ XmNbottomShadowColor , BOTTOMSHADOWCOLOR } , 
			{ XmNtopShadowColor , TOPSHADOWCOLOR } , 
			{ XmNforeground , FOREGROUND } , 
			{ XmNbackground , BACKGROUND }
	} ;

static void SR_UpdateColors( XmsrComboBoxWidget w , int flags )
	{
	Pixel Color ;
	int i , size = XtNumber( ColorResources ) ;
	Widget ScrolledWin , ScrollBar ;
	
	ScrolledWin = XtParent( w->combobox.ListCtrl ) ;
	XtVaGetValues( ScrolledWin , XmNverticalScrollBar , &ScrollBar , NULL ) ;
	for ( i=0 ; i<size ; i++ )
	if ( flags & ColorResources[i].Flag )
		{
		XtVaGetValues( ( Widget ) w , ColorResources[i].Resource , &Color ,
			NULL ) ;
		XtVaSetValues( w->combobox.ListCtrl , 
			ColorResources[i].Resource , Color , NULL ) ;
		XtVaSetValues( ScrolledWin , 
			ColorResources[i].Resource , Color , NULL ) ;
		XtVaSetValues( ScrollBar , 
			ColorResources[i].Resource , Color , NULL ) ;
		XtVaSetValues( w->combobox.EditCtrl , 
			ColorResources[i].Resource , Color , NULL ) ;
		XtVaSetValues( w->combobox.LabelCtrl , 
			ColorResources[i].Resource , Color , NULL ) ;
		XtVaSetValues( w->combobox.ArrowCtrl , 
			ColorResources[i].Resource , Color , NULL ) ;
		if ( ColorResources[i].Flag & BACKGROUND )
			XtVaSetValues( ScrollBar , XmNtroughColor , Color , NULL ) ;
		}
	} /* SR_UpdateColors */

typedef enum { EDITCTRL , LISTCTRL , LABELCTRL } CHILDCTRL ;
typedef struct {
	String rsc ;
	CHILDCTRL ctrl ;
	enum { RO , RW , RWS , RWL , RWI } dir ;
	} MIRROR ;

static MIRROR MirroredResources[] = {
	{ XmNitems , 				LISTCTRL , RWI } , /* Urgs! */
	{ XmNitemCount , 			LISTCTRL , RW } , /* dto. */
	{ XmNlistMarginHeight , 		LISTCTRL , RW } , 
	{ XmNlistMarginWidth , 			LISTCTRL , RW } , 
	{ XmNlistSpacing , 			LISTCTRL , RW } , 
	{ XmNstringDirection , 			LISTCTRL , RO } , /* Naja? */
	{ XmNtopItemPosition , 			LISTCTRL , RO } , 
	
	{ XmNblinkRate , 			EDITCTRL , RW } , 
	{ XmNcolumns , 				EDITCTRL , RW } , 
	{ XmNcursorPosition , 			EDITCTRL , RW } , 
	{ XmNcursorPositionVisible , 		EDITCTRL , RW } , 
	{ XmNmarginHeight , 			EDITCTRL , RW } , 
	{ XmNmarginWidth , 			EDITCTRL , RW } , 
	{ XmNmaxLength , 			EDITCTRL , RW } , 
	{ XmNselectThreshold , 			EDITCTRL , RW } , 
	{ XmNvalue , 				EDITCTRL , RWS } , 
	
	{ XmNalignment , 			LABELCTRL , RW } , 
	{ XmNlabelPixmap , 			LABELCTRL , RW } , 
	{ XmNlabelInsensitivePixmap ,		LABELCTRL , RW } , 
	{ XmNlabelString , 			LABELCTRL , RW } , 
	{ XmNlabelType , 			LABELCTRL , RW } , 
	{ XmNlabelMarginBottom , 		LABELCTRL , RWL } , /* !!! */
	{ XmNlabelMarginHeight , 		LABELCTRL , RWL } , /* !!! */
	{ XmNlabelMarginLeft , 			LABELCTRL , RWL } , /* !!! */
	{ XmNlabelMarginRight , 		LABELCTRL , RWL } , /* !!! */
	{ XmNlabelMarginTop , 			LABELCTRL , RWL } , /* !!! */
	{ XmNlabelMarginWidth , 		LABELCTRL , RWL } , /* !!! */
	{ XmNlabelFontList ,			LABELCTRL , RWL } , /* !!! */
	} ;

typedef struct {
	char *from , *to ;
	} TRANSFORMATION ;

static TRANSFORMATION Transformations[] = {
	{ XmNlabelMarginBottom ,	XmNmarginBottom } , 
	{ XmNlabelMarginHeight ,	XmNmarginHeight } , 
	{ XmNlabelMarginLeft ,		XmNmarginLeft } , 
	{ XmNlabelMarginRight ,		XmNmarginRight } , 
	{ XmNlabelMarginTop ,		XmNmarginTop } , 
	{ XmNlabelMarginWidth ,		XmNmarginWidth } , 
	{ XmNlabelFontList ,		XmNfontList } ,
	} ;

static Boolean SetValues( XmsrComboBoxWidget current , XmsrComboBoxWidget req , 
		XmsrComboBoxWidget new , 
		ArgList args , Cardinal *NumArgs )
	{
	Boolean Update = False ;
	int j, k, MirrorSize = XtNumber( MirroredResources ) ;
	int TransformationSize = XtNumber( Transformations ) ;
	unsigned int i ;
	Arg arg ;
	int Flags ;
	
	new->combobox.Editable = current->combobox.Editable ;
	new->combobox.ListCtrl = current->combobox.ListCtrl ;
	new->combobox.EditCtrl = current->combobox.EditCtrl ;
	new->combobox.LabelCtrl = current->combobox.LabelCtrl ;
	
	if ( current->combobox.VisibleItemCount !=
		new->combobox.VisibleItemCount )
		{
		XtVaSetValues( new->combobox.ListCtrl , 
			XmNvisibleItemCount , new->combobox.VisibleItemCount , 
				NULL ) ;
		Update = True ;
		}
	if ( current->combobox.Font != new->combobox.Font )
		{
		XtVaSetValues( new->combobox.ListCtrl , 
			XmNfontList , new->combobox.Font , NULL ) ;
		XtVaSetValues( new->combobox.EditCtrl , 
			XmNfontList , new->combobox.Font , NULL ) ;
		XtVaSetValues( new->combobox.LabelCtrl , 
			XmNfontList , new->combobox.Font , NULL ) ;
		}
	
	Flags = 0 ;
	if ( new->manager.top_shadow_color !=
		current->manager.top_shadow_color ) Flags |= TOPSHADOWCOLOR ;
	if ( new->manager.bottom_shadow_color !=
		current->manager.bottom_shadow_color ) Flags |= BOTTOMSHADOWCOLOR ;
	if ( new->manager.foreground !=
		current->manager.foreground ) Flags |= FOREGROUND ;
	if ( new->core.background_pixel !=
		current->core.background_pixel ) Flags |= BACKGROUND ;
	if ( Flags ) { SR_UpdateColors( new , Flags ) ; Update = True ; }
	
	
	if ( new->combobox.ArrowCursor != current->combobox.ArrowCursor )
		{
		if ( new->combobox.ListVisible )
			XDefineCursor( XtDisplay( new->combobox.PopupShell ) , 
				XtWindow( new->combobox.PopupShell ) , 
				new->combobox.ArrowCursor ) ;
		}
	for ( i = 0 ; i < *NumArgs ; i++ )
		{
		for ( j = 0 ; j < MirrorSize ; j++ )
			{
			if ( ( strcmp( args[i].name , MirroredResources[j].rsc ) == 0 ) )
				{
				switch ( MirroredResources[j].dir )
					{
					case RW:
						XtSetValues( MirroredResources[j].ctrl == LISTCTRL ?
							new->combobox.ListCtrl : 
							( MirroredResources[j].ctrl == EDITCTRL ?
							new->combobox.EditCtrl :
							new->combobox.LabelCtrl ) , 
							&( args[i] ) , 1 ) ;
						break ;
					case RWS:
						if ( strcmp( args[i].name , XmNvalue ) == 0 )
							{
							if ( new->combobox.Editable )
								XtSetValues( new->combobox.EditCtrl , 
								&( args[i] ) , 1 ) ;
							}
						break ;
					case RWL:
						for ( k = 0 ; k < TransformationSize ; k++ )
							if ( strcmp( args[i].name , Transformations[k].from ) == 0 )
								{
								arg.value = args[i].value ;
								arg.name = Transformations[k].to ;
								XtSetValues( new->combobox.LabelCtrl , 
									&arg , 1 ) ;
								break ;
								}
					case RWI:
						for ( k = 0 ; k < *NumArgs ; k++ )
							if ( strcmp( args[k].name , XmNitemCount ) == 0 )
								{
								XtVaSetValues( new->combobox.ListCtrl , 
									XmNitems , args[i].value , 
									XmNitemCount , args[k].value , 
									NULL ) ;
								break ;
								}
						break ;
					case RO:
						break ;
					} /* case write mode */
				goto ScanForNextResource ;
				} /* if entry found */
			} /* for every mirrored entry */
		ScanForNextResource: ;
		} /* for every Arg */
	
	return Update ;
	} /* SetValues */

static void GetValuesAlmost( XmsrComboBoxWidget w , ArgList args , 
			Cardinal *NumArgs )
	{
	int j, k, MirrorSize = XtNumber( MirroredResources ) ;
	int TransformationSize = XtNumber( Transformations ) ;
	unsigned int i ;
	Arg arg ;
	
	for ( i = 0 ; i < *NumArgs ; i++ )
		{
		for ( j = 0 ; j < MirrorSize ; j++ )
			{
			if ( strcmp( args[i].name , MirroredResources[j].rsc ) == 0 )	
				{
				switch ( MirroredResources[j].dir )
					{
					case RO:
					case RW:
					case RWS:
					case RWI:
						XtGetValues( MirroredResources[j].ctrl == LISTCTRL ?
							w->combobox.ListCtrl :
							MirroredResources[j].ctrl == EDITCTRL ?
							w->combobox.EditCtrl :
							w->combobox.LabelCtrl , 
							&( args[i] ) , 1 ) ;
						break ;
					case RWL:
						for ( k = 0 ; k < TransformationSize ; k++ )
							if ( strcmp( args[i].name , Transformations[k].from ) == 0 )
								{
								arg.value = args[i].value ;
								arg.name = Transformations[k].to ;
								XtGetValues( w->combobox.LabelCtrl , 
									( ArgList ) &arg , 1 ) ;
								break ;
								}
						break ;
					} /* case read mode */
				} /* if entry found */
			} /* for every mirrored entry */
		} /* for every Arg */
	} /* GetValuesAlmost */

static void ShowHideDropDownList( XmsrComboBoxWidget w , XEvent *event , 
			Boolean Show )
	{
	XmsrComboBoxDropDownCallbackStruct info ;
	if ( Show == w->combobox.ListVisible ) return ;
	w->combobox.ListVisible = Show ;
	if ( Show )
		{
		DoDropDownLayout( w ) ;
		info.reason = XmCR_SHOW_LIST ;
		info.event = event ;
		XtCallCallbacks( ( Widget ) w , XmNdropDownCallback , 
			( XtPointer ) &info ) ;
		XDefineCursor( XtDisplay( w->combobox.PopupShell ) , 
			XtWindow( w->combobox.PopupShell ) , 
			w->combobox.ArrowCursor ) ;
		XtPopup( w->combobox.PopupShell , XtGrabNone ) ;
		XtVaSetValues( w->combobox.ArrowCtrl , 
			XmNarrowDirection , XmARROW_UP , NULL ) ;
		}
	else
		{ 
		XtPopdown( w->combobox.PopupShell ) ;
		XtVaSetValues( w->combobox.ArrowCtrl , 
			XmNarrowDirection , XmARROW_DOWN , NULL ) ;
		info.reason = XmCR_HIDE_LIST ;
		info.event = event ;
		XtCallCallbacks( ( Widget ) w , XmNdropDownCallback , 
			( XtPointer ) &info ) ;
		}
	} /* ShowHideDropDownList */

static void ArrowCallback( Widget w , XtPointer pClientData , 
			XmAnyCallbackStruct *info )
	{
	XmsrComboBoxWidget cbw = ( XmsrComboBoxWidget ) XtParent( w ) ;
	
	switch ( info->reason )
		{
		case XmCR_ARM:
			LOG( "ArrowCallback: XmCR_ARM\n" ) ;
			XmProcessTraversal( cbw->combobox.EditCtrl , XmTRAVERSE_CURRENT ) ;
			/* SIMPLIFY
			if ( cbw->combobox.TwmHandlingOn && cbw->combobox.ListVisible )
				{
				cbw->combobox.IgnoreFocusOut = True ;
				}
			*/
			break ;

		case XmCR_ACTIVATE:
			XmProcessTraversal( cbw->combobox.EditCtrl , XmTRAVERSE_CURRENT ) ;
			ShowHideDropDownList( cbw , info->event , 
				( Boolean )( !cbw->combobox.ListVisible ) ) ;
			break ;

		}
	} /* ArrowCallback */

static Boolean DelayedFocusOutWorkProc( XtPointer pClientData )
	{
	XmsrComboBoxWidget cbw = ( XmsrComboBoxWidget ) pClientData ;

	/* SIMPLIFY
	LOG2( "DelayedFocusOutWorkProc: IgnoreFocusOut: %s\n" , 
		cbw->combobox.IgnoreFocusOut ? "True" : "False" ) ;
	if ( !cbw->combobox.IgnoreFocusOut )
		{
		ShowHideDropDownList( cbw , &( cbw->combobox.xevent ) , False ) ;
		}
	cbw->combobox.IgnoreFocusOut = False ;
	cbw->combobox.PendingFocusOut = False ;
	*/
	return True ;
	} /* DelayedFocusOutWorkProc */

static void EditFocusCallback( Widget w , XtPointer pClientData , 
			XmAnyCallbackStruct *info )
	{
	XmsrComboBoxWidget cbw = ( XmsrComboBoxWidget ) XtParent( w ) ;
	
	if ( info->reason == XmCR_LOSING_FOCUS )
		{

		/* SIMPLIFY
		LOG2( "EditFocusCallback: PendingFocusOut: %s , " , 
			cbw->combobox.PendingFocusOut ? "True" : "False" ) ;
		LOG2( "PendingOverrideInOut: %s\n" ,
			cbw->combobox.PendingOverrideInOut ? "True" : "False" ) ;
		if (	!cbw->combobox.PendingFocusOut &&
			!cbw->combobox.PendingOverrideInOut )
			{
		*/

			if ( info->event )
				cbw->combobox.xevent = *info->event ;

		/* SIMPLIFY
			cbw->combobox.WorkProcID = XtAppAddWorkProc( 
				XtWidgetToApplicationContext( ( Widget ) cbw ) ,
				( XtWorkProc ) DelayedFocusOutWorkProc ,
				( XtPointer ) cbw ) ;
			cbw->combobox.PendingFocusOut = True ;
			}
		cbw->combobox.PendingOverrideInOut = False ;
		*/

		}
	} /* EditFocusCallback */

static int SetSelectionPos( XmsrComboBoxWidget w , int Index , Boolean Notify )
	{
	Widget ListBox = w->combobox.ListCtrl ;
	int ItemCount ; 
	int TopItem , VisibleItems ;
	
	XtVaGetValues( ListBox , XmNitemCount , &ItemCount , 
				XmNtopItemPosition , &TopItem , 
				XmNvisibleItemCount , &VisibleItems ,
			NULL ) ;
	if ( Index < 1 ) Index = 1 ;
	if ( Index > ItemCount ) Index = ItemCount ;
	if ( Index != 0 && ItemCount != 0 )
		{
		if ( Index < TopItem )
			XmListSetPos( ListBox , Index ) ;
		if ( Index >= TopItem + VisibleItems )
			XmListSetBottomPos( ListBox , Index ) ;
		XmListSelectPos( ListBox , Index , Notify ) ;
		return Index ;
		}
	else
		return 0 ;
	} /* SetSelectionPos */

static void TransferToEditCtrl( XmsrComboBoxWidget w , int SelectionIndex )
	{
	Widget ListBox = w->combobox.ListCtrl ;
	XmStringTable Items ;
	char *pItemText ;
	
	XtVaGetValues( ListBox , XmNitems , &Items , NULL ) ;
	SelectionIndex = SetSelectionPos( w , SelectionIndex , False ) ;
	if ( SelectionIndex > 0 )
		{
		XmStringGetLtoR( Items[SelectionIndex-1] , 
			XmSTRING_DEFAULT_CHARSET , &pItemText ) ;
		w->combobox.PassVerification = True ;
		XmTextFieldSetString( w->combobox.EditCtrl , pItemText ) ;
		XtFree( pItemText ) ;
		}
	} /* TransferToEditCtrl */

static void CallSelectionCBL( XmsrComboBoxWidget w , XEvent *Event )
	{
	XmsrComboBoxSelectionCallbackStruct info ;
	XmStringTable Items ;
	
	info.reason = XmCR_SINGLE_SELECT ;
	info.event = Event ;
	info.index = XmsrComboBoxGetSelectedPos( ( Widget ) w ) ;
	if ( info.index == 0 ) return ;
	XtVaGetValues( w->combobox.ListCtrl , XmNitems , &Items , NULL ) ;
	info.value = Items[info.index-1] ;
	XtCallCallbacks( ( Widget ) w , XmNselectionCallback , ( XtPointer ) &info ) ;
	} /* CallSelectionCBL */

static void CBoxManager( Widget w , XEvent *Event , String *params , 
			Cardinal *num_params )
	{
	XmsrComboBoxWidget cbw = ( XmsrComboBoxWidget ) XtParent( w ) ;
	Widget ListBox = cbw->combobox.ListCtrl ;
	int *SelectionList ;
	int SelectionCount ;
	int SelectionIndex ;
	int ItemCount ; 
	char opt ;
	switch ( *( params[0] ) )
		{
		case 's': /* show-hide-list */
			ShowHideDropDownList( cbw , Event , 
				( Boolean )( !cbw->combobox.ListVisible ) ) ;
			break ;
		case 'h': /* hide-list */
			ShowHideDropDownList( cbw , Event , False ) ;
			break ;
		case 'u': /* up */
		case 'd': /* down */
		case 't': /* top */
		case 'b': /* bottom */
			opt = *( params[0] ) ;
			XtVaGetValues( ListBox , XmNitemCount , &ItemCount , NULL ) ;
			if (	XmListGetSelectedPos( ListBox ,
				&SelectionList , &SelectionCount ) )
				{
				SelectionIndex = *SelectionList ;
				XtFree( ( char * )SelectionList ) ;
				switch ( opt )
					{
					case 'u': SelectionIndex-- ;		break ;
					case 'd': SelectionIndex++ ;		break ;
					case 't': SelectionIndex = 1 ;		break ;
					case 'b': SelectionIndex = ItemCount ;	break ;
					}
				}
			else
				{
				if ( opt == 'b' ) SelectionIndex = ItemCount ;
				else SelectionIndex = 1 ;
				}
			TransferToEditCtrl( cbw , SelectionIndex ) ;
			CallSelectionCBL( cbw , Event ) ;
			break ;
		case 'a': /* Return = activate */
		case 'S': /* Selection */
			if ( !cbw->combobox.ListVisible ) break ;
			XtVaGetValues( ListBox , XmNitemCount , &ItemCount , NULL ) ;
			if ( ItemCount == 0 ) break ;
			if (	XmListGetSelectedPos( ListBox ,
				&SelectionList , &SelectionCount ) )
				{
				SelectionIndex = *SelectionList ;
				XtFree( ( char * )SelectionList ) ;
				}
			else
				SelectionIndex = 1 ;
			TransferToEditCtrl( cbw , SelectionIndex ) ;
			CallSelectionCBL( cbw , Event ) ;
			ShowHideDropDownList( cbw , Event , ( Boolean )
				( *( params[0] ) == 'S' ? True : False ) ) ;
			break ;
		case 'c': /* Cancel */
			if ( cbw->combobox.ListVisible )
				{
				ShowHideDropDownList( cbw , Event , False ) ;
				}
			else
				XtCallActionProc( cbw->combobox.EditCtrl , 
					"process-cancel" , Event , NULL , 0 ) ;
			break ;
		case 'n': /* no-operation */
			break ;
		}
	} /* CBoxManager */

static void ListSelectionCallback( Widget w , XtPointer pClientData , 
			XmAnyCallbackStruct *info )
	{
	String paramsMouse[1] = { "a" } , paramsKeyboard[1] = { "S" } ;
	Cardinal NumParams = 1 ;

	if ( info->event == NULL )
		CBoxManager( XtParent( XtParent( w ) ) , info->event , 
			paramsKeyboard , &NumParams ) ;
	else
		{
		XmsrComboBoxWidget cbw ;
		CBoxManager( XtParent( XtParent( w ) ) , info->event , 
			paramsMouse , &NumParams ) ;
		cbw = ( XmsrComboBoxWidget ) XtParent( XtParent( XtParent( w ) ) ) ;
		XmProcessTraversal( cbw->combobox.EditCtrl , 
			XmTRAVERSE_CURRENT ) ;
		}
	} /* ListSelectionCallback */

static void EditVerifyCallback( Widget w , XtPointer pClientData , 
			XmTextVerifyCallbackStruct *info )
	{
	XmsrComboBoxWidget cbw = ( XmsrComboBoxWidget ) XtParent( w ) ;

	if ( cbw->combobox.PassVerification )
		{
		cbw->combobox.PassVerification = False ;
		info->doit = True ;
		return ;
		}
	if ( !cbw->combobox.Editable )
		{
		Widget ListBox = cbw->combobox.ListCtrl ;
		char WarpCharLow , WarpCharHigh ;
		XmString Item ;
		XmStringTable Items ;
		int *SelectionList ;
		int SelectionCount ;
		int i , ItemCount , Start , End ;
		char *pItem ;
		Boolean Ignore ;
	
		info->doit = False ;
		if ( ( info->text == NULL ) ||
			( info->text->length == 0 ) ) return ;
		if ( info->text->length > 1 )
			{
			fprintf( stderr , ":%s:\n" , info->text->ptr ) ;
			Item = XmStringCreateLocalized( info->text->ptr ) ;
			XmsrComboBoxSelectItem( ( Widget ) cbw , Item , True ) ;
			XmStringFree( Item ) ;
			}
		else
			{
			WarpCharLow = tolower( *( info->text->ptr ) ) ;
			WarpCharHigh = toupper( WarpCharLow ) ;

			XtVaGetValues( ListBox , XmNitemCount , &ItemCount , 
				XmNitems , &Items , NULL ) ;
			if ( ItemCount < 1 ) return ;
			if (	XmListGetSelectedPos( ListBox , 
				&SelectionList , &SelectionCount ) )
				{
				Start = *SelectionList ; i = Start + 1 ;
				XtFree( ( char * )SelectionList ) ;
				}
			else
				i = Start = 1 ;
		
			if ( i > ItemCount ) i = 1 ;
			Ignore = True ;
			while ( i != Start || Ignore )
				{
				Ignore = False ;
				XmStringGetLtoR( Items[i-1] , XmSTRING_DEFAULT_CHARSET , 
					&pItem ) ;
				if ( ( strchr( pItem , WarpCharLow ) == pItem ) ||
					( strchr( pItem , WarpCharHigh ) == pItem ) )
					{
					XtFree( pItem ) ;
						TransferToEditCtrl( cbw , i ) ;
						CallSelectionCBL( cbw , info->event ) ;
						break ;
					}
				XtFree( pItem ) ;
				if ( ++i > ItemCount ) i = 1 ;
				}
			}
		}
	} /* EditVerifyCallback */

static void EditChangedCallback( Widget w , XtPointer pClientDate , 
			XmAnyCallbackStruct *info )
	{
	XmsrComboBoxWidget cbw = ( XmsrComboBoxWidget ) XtParent( w ) ;
	XmStringTable Items ;
	int ItemCount , i ;
	XmString EditStr ;
	char* EditLine ;
	
	XtVaGetValues( cbw->combobox.EditCtrl , XmNvalue , &EditLine , NULL ) ;
	XtVaGetValues(	cbw->combobox.ListCtrl , 
			XmNitemCount , &ItemCount , 
			XmNitems , &Items , 
			NULL ) ;
	EditStr = XmStringCreateLocalized( EditLine ) ;
	XtVaSetValues( cbw->combobox.ListCtrl , XmNselectedItemCount , 0 , NULL ) ;
	if ( ItemCount < 1 )
		{
		/* CHANGE 8-9-95-1 >>> Purify caught these memory leaks: */
		XmStringFree( EditStr ) ;
		XtFree( EditLine ) ;
		/* <<< CHANGE 8-9-95-1 */
		return ;
		}
	for ( i = 0 ; i < ItemCount ; i++ )
		if ( XmStringCompare( Items[i] , EditStr ) )
			{
			SetSelectionPos( cbw , i+1 , False ) ;
			break ;
			}
	XmStringFree( EditStr ) ;
	/* CHANGE 8-8-95-1 >>> Purify caught this memory leak: */
	XtFree( EditLine ) ;
	/* <<< CHANGE 8-8-95-1 */
	} /* EditChangedCallback */

static void MakeNameAndClass( Widget w , char *NameBuff , char *ClassBuff )
	{
	Widget Parent = XtParent( w ) ;
	
	if ( Parent ) MakeNameAndClass( Parent , NameBuff , ClassBuff ) ;
	if ( XtIsSubclass( w , applicationShellWidgetClass ) )
		{
		String AppName , AppClass ;
		XtGetApplicationNameAndClass( 
			XtDisplayOfObject( w ) , &AppName , &AppClass ) ;
		strcpy( NameBuff , AppName ) ;
		strcpy( ClassBuff , AppClass ) ;
		}
	else
		{
		strcat( NameBuff , "." ) ;
		strcat( NameBuff , XtName( w ) ) ;
		strcat( ClassBuff , "." ) ;
		strcat( ClassBuff , ( ( CoreClassRec * ) XtClass( w ) )->core_class.class_name ) ;
		}
	} /* MakeNameAndClass */

static Boolean FetchResource( Widget w , 
			char *FullName , char *FullClass , 
			char *RscName , char *RscClass , 
					XrmValue *RscValue , 
					String *RepresentationType )
	{
	Boolean ok ;
	char *EndOfName = FullName + strlen( FullName ) ;
	char *EndOfClass = FullClass + strlen( FullClass ) ;
	
	strcat( FullName , "." ) ; strcat( FullName , RscName ) ;
	strcat( FullClass , "." ) ; strcat( FullClass , RscClass ) ;
	ok = XrmGetResource( 
	XtDatabase( XtDisplayOfObject( w ) ) , 
	FullName , FullClass , RepresentationType , RscValue ) ;
	*EndOfName = 0 ; *EndOfClass = 0 ;
	return ok ;
	} /* FetchResource */

static Boolean FetchIntResource( Widget w , char *FullName , char *FullClass , 
	char *RscName , char *RscClass , int *pInt )
	{
	XrmValue RscValue , RscDest ;
	String RepresentationType ;
	
	if ( FetchResource( w , FullName , FullClass , 
			RscName , RscClass , 
			&RscValue , &RepresentationType ) )
		{
		RscDest.size = sizeof( int ) ;
		RscDest.addr = ( XtPointer ) pInt ;
		if ( XtConvertAndStore( w , RepresentationType , &RscValue , 
			XtRInt , &RscDest ) )
			return True ;
		}
	return False ;
	} /* FetchIntResource */

static Boolean FetchShortResource( Widget w , 
	char *FullName , char *FullClass , 
	char *RscName , char *RscClass , 
	short *pShort )
	{
	XrmValue RscValue , RscDest ;
	String RepresentationType ;
	
	if ( FetchResource( w , FullName , FullClass , 
		RscName , RscClass , &RscValue , &RepresentationType ) )
		{
		RscDest.size = sizeof( short ) ;
		RscDest.addr = ( XtPointer ) pShort ;
		if ( XtConvertAndStore( w , RepresentationType , &RscValue , 
			XtRShort , &RscDest ) )
			return True ;
		}
	return False ;
	} /* FetchShortResource */

static Boolean FetchLabelTypeResource( Widget w , 
			char *FullName , char *FullClass , 
			char *RscName , char *RscClass , 
					unsigned char *pUChar )
	{
	XrmValue RscValue , RscDest ;
	String RepresentationType ;
	int AInt ;
	
	if ( FetchResource( w , FullName , FullClass , 
		RscName , RscClass , 
		&RscValue , &RepresentationType ) )
		{
		if ( strcasecmp( ( char * ) RscValue.addr , "XmPIXMAP" ) == 0 )
			*pUChar = XmPIXMAP ;
		else
			*pUChar = XmSTRING ;
		return True ;
		}
	return False ;
	} /* FetchLabelTypeResource */

static Boolean FetchDimensionResource( Widget w , 
			char *FullName , char *FullClass , 
			char *RscName , char *RscClass , 
					Dimension *pDimension )
	{
	XrmValue RscValue , RscDest ;
	String RepresentationType ;
	
	if ( FetchResource( w , FullName , FullClass , 
		RscName , RscClass , 
		&RscValue , &RepresentationType ) )
		{
		RscDest.size = sizeof( Dimension ) ;
		RscDest.addr = ( XtPointer ) pDimension ;
		if ( XtConvertAndStore( w , RepresentationType , &RscValue , 
			XtRDimension , &RscDest ) )
			return True ;
		}
	return False ;
	} /* FetchDimensionResource */

static Boolean FetchXmStringResource( Widget w , 
	char *FullName , char *FullClass , 
	char *RscName , char *RscClass , 
	XmString *pString )
	{
	XrmValue RscValue ;
	String RepresentationType ;
	
	if ( FetchResource( w , FullName , FullClass , 
		RscName , RscClass , 
		&RscValue , &RepresentationType ) )
		{
		*pString = XmCvtCTToXmString( ( char * ) RscValue.addr ) ;
		return True ;
		}
	return False ;
	} /* FetchXmStringResource */

static Boolean FetchXmStringTableResource( Widget w , 
	char *FullName , char *FullClass , 
	char *RscName , char *RscClass , 
	XmStringTable *pStringTable , 
	int *pTableSize )
	{
	XrmValue RscValue ;
	String RepresentationType ;
	String TmpList , p , pStart ;
	int Entries , Entry ;
	
	if ( FetchResource( w , FullName , FullClass , 
		RscName , RscClass , 
		&RscValue , &RepresentationType ) ) {
	TmpList = XtNewString( ( String )RscValue.addr ) ;
	if ( TmpList == NULL ) return False ;
	if ( *TmpList == 0 ) { XtFree( TmpList ) ; return False ; }
	Entries = 1 ; p = TmpList ;
	while ( *p )
		if ( *p++ == ',' ) ++Entries ;
	*pStringTable = ( XmStringTable ) XtMalloc( Entries * sizeof( XmString ) ) ;
	
	p = TmpList ;
	for ( Entry = 0 ; Entry < Entries ; ++Entry )
		{
		pStart = p ;
		while ( ( *p != 0 ) && ( *p != ',' ) ) ++p ;
		*p++ = 0 ;
		( *pStringTable )[Entry] = ( XmString )
		XmStringCreateLocalized( pStart ) ;
		}
	XtFree( TmpList ) ;
	*pTableSize = Entries ;
	return True ;
	}
	return False ;
	} /* FetchXmStringTableResource */

static Boolean FetchXmFontListResource( Widget w , 
	char *FullName , char *FullClass , 
	char *RscName , char *RscClass , 
	XmFontList *pFontList )
	{
	XrmValue RscValue , RscDest ;
	String RepresentationType ;
	
	if ( FetchResource( w , FullName , FullClass , 
		RscName , RscClass , 
		&RscValue , &RepresentationType ) )
		{
		RscDest.size = sizeof( XmFontList ) ;
		RscDest.addr = ( XtPointer ) pFontList ;
		if ( XtConvertAndStore( w , RepresentationType , &RscValue , 
			XmRFontList , &RscDest ) )
			return True ;
		}
	return False ;
	} /* FetchXmFontListResource */

static Boolean FetchPixmapResource( Widget w , 
			char *FullName , char *FullClass , 
			char *RscName , char *RscClass ,
				Pixmap *pPixmap )
	{
	XrmValue RscValue , RscDest ;
	String RepresentationType ;
	
	if ( FetchResource( w , FullName , FullClass , 
		RscName , RscClass , 
		&RscValue , &RepresentationType ) )
		{
		RscDest.size = sizeof( Pixmap ) ;
		RscDest.addr = ( XtPointer ) pPixmap ;
		if ( XtConvertAndStore( w , RepresentationType , &RscValue , 
			XtRBitmap , &RscDest ) )
			return True ;
		}
	return False ;
	} /* FetchPixmapResource */

typedef struct {
	String Name , Class ;
	enum { RInt , RShort , RLType , RDimension , 
	RXmString , RPixmap , RXmFontList ,
	RXmStringTable , RXmItemCount } Converter ;
	} RESOURCEMIRROR ;

static RESOURCEMIRROR ResourceMirror[] = {
	{ XmNblinkRate , 			XmCBlinkRate , 				RInt , } , 
	{ XmNcolumns , 				XmCColumns , 				RShort , } , 
	{ XmNmaxLength , 			XmCMaxLength , 				RInt , } , 
	{ XmNmarginHeight , 			XmCMarginHeight , 			RDimension } , 
	{ XmNmarginWidth , 			XmCMarginWidth , 			RDimension } ,
	{ XmNselectThreshold , 			XmCSelectThreshold , 			RInt } , 
	
	{ XmNlistMarginHeight , 		XmCListMarginHeight , 			RDimension } , 
	{ XmNlistMarginWidth , 			XmCListMarginWidth , 			RDimension } , 
	{ XmNlistSpacing , 			XmCListSpacing , 			RDimension } ,
	{ XmNitems , 				XmCItems , 				RXmStringTable } ,
	{ XmNitemCount , 			XmCItemCount , 				RXmItemCount } ,
	
	{ XmNlabelString , 			XmCLabelString , 			RXmString } , 
	{ XmNlabelMarginBottom , 		XmCLabelMarginBottom , 			RDimension } , 
	{ XmNlabelMarginHeight , 		XmCLabelMarginHeight , 			RDimension } , 
	{ XmNlabelMarginLeft , 			XmCLabelMarginLeft , 			RDimension } , 
	{ XmNlabelMarginRight , 		XmCLabelMarginRight , 			RDimension } , 
	{ XmNlabelMarginTop , 			XmCLabelMarginTop , 			RDimension } , 
	{ XmNlabelMarginWidth , 		XmCLabelMarginWidth , 			RDimension } , 
	{ XmNlabelPixmap , 			XmCLabelPixmap , 			RPixmap } ,
	{ XmNlabelInsensitivePixmap ,		XmCLabelInsensitivePixmap ,		RPixmap } , 
	{ XmNlabelType , 			XmCLabelType , 				RLType } , 
	{ XmNlabelFontList ,			XmCLabelFontList , 			RXmFontList } , 
	} ;

static void InitMirrorResources( XmsrComboBoxWidget w )
	{
	char FullName[1024] , FullClass[1024] ;
	int AInt , TableSize ;
	short AShort ;
	unsigned char AUChar ;
	Dimension ADimension ;
	XmString AString ;
	XmStringTable AStringTable ;
	Pixmap APixmap ;
	XmFontList AFontList ;
	XrmValue RscValue ;
	int i , size = XtNumber( ResourceMirror ) ;
	
	FullName[0] = 0 ; FullClass[0] = 0 ;
	MakeNameAndClass( ( Widget ) w , FullName , FullClass ) ;

	for ( i=0 ; i < size ; i++ )
		{
		switch ( ResourceMirror[i].Converter )
			{
			case RInt:
				if ( FetchIntResource( ( Widget ) w , 
					FullName , FullClass , 
					ResourceMirror[i].Name , ResourceMirror[i].Class , 
					&AInt ) )
					XtVaSetValues( ( Widget ) w , ResourceMirror[i].Name , 
					AInt , NULL ) ;
				break ;
			case RXmItemCount:
				if ( FetchIntResource( ( Widget ) w , 
					FullName , FullClass , 
					ResourceMirror[i].Name , ResourceMirror[i].Class , 
					&AInt ) && ( AInt != 0 ) )
					XtVaSetValues( ( Widget ) w , ResourceMirror[i].Name , 
					AInt , NULL ) ;
					break ;
			case RShort:
				if ( FetchShortResource( ( Widget ) w , 
					FullName , FullClass , 
					ResourceMirror[i].Name , ResourceMirror[i].Class , 
					&AShort ) )
					XtVaSetValues( ( Widget ) w , ResourceMirror[i].Name , 
					AShort , NULL ) ;
				break ;
			case RLType:
				if ( FetchLabelTypeResource( ( Widget ) w , 
					FullName , FullClass , 
					ResourceMirror[i].Name , ResourceMirror[i].Class , 
					&AUChar ) )
						XtVaSetValues( ( Widget ) w , ResourceMirror[i].Name , 
						AUChar , NULL ) ;
					break ;
			case RDimension:
				if ( FetchDimensionResource( ( Widget ) w , 
					FullName , FullClass , 
					ResourceMirror[i].Name , ResourceMirror[i].Class , 
					&ADimension ) )
					XtVaSetValues( ( Widget ) w , ResourceMirror[i].Name , 
						ADimension , NULL ) ;
				break ;
			case RXmString:
				if ( FetchXmStringResource( ( Widget ) w , 
					FullName , FullClass , 
					ResourceMirror[i].Name , ResourceMirror[i].Class , 
					&AString ) )
					XtVaSetValues( ( Widget ) w , ResourceMirror[i].Name , 
						AString , NULL ) ;
				break ;
			case RXmStringTable:
				if ( FetchXmStringTableResource( ( Widget ) w ,
					FullName , FullClass ,
					ResourceMirror[i].Name , ResourceMirror[i].Class ,
					&AStringTable , &TableSize ) )
					{
					XtVaSetValues( ( Widget ) w , 
						XmNitems , ( XtPointer ) AStringTable , 
						XmNitemCount , TableSize , NULL ) ;
					}
				break ;
			case RPixmap:
				if ( FetchPixmapResource( ( Widget ) w , 
					FullName , FullClass , 
					ResourceMirror[i].Name , ResourceMirror[i].Class , 
					&APixmap ) )
					{
					XtVaSetValues( ( Widget ) w , ResourceMirror[i].Name , 
						APixmap , NULL ) ;
					if ( strcmp( ResourceMirror[i].Name , XmNlabelPixmap ) == 0 )
						w->combobox.ConvertBitmapToPixmap = True ;
					else
						w->combobox.ConvertBitmapToPixmapInsensitive = True ;
					}
				break ;
			case RXmFontList:
				if ( FetchXmFontListResource( ( Widget ) w ,
					FullName , FullClass ,
					ResourceMirror[i].Name , ResourceMirror[i].Class ,
					&AFontList ) )
					XtVaSetValues( ( Widget ) w , ResourceMirror[i].Name ,
						AFontList , NULL ) ;
				break ;
			}
		}
	} /* InitMirrorResources */

static Pixmap BitmapToPixmap( XmsrComboBoxWidget w , 
	String Resource , GC ColorGC )
	{
	Pixmap LabelPixmap , LabelBitmap ;
	Display *display = XtDisplay( w ) ;
	Window root ;
	int PixX , PixY ;
	unsigned int PixW , PixH , PixBW , PixDepth ;

	XtVaGetValues( w->combobox.LabelCtrl , Resource , &LabelBitmap , NULL ) ;
	XGetGeometry( display , LabelBitmap , &root , 
			&PixX , &PixY , &PixW , &PixH , &PixBW , &PixDepth ) ;
	LabelPixmap = XCreatePixmap( 
			display , RootWindowOfScreen( XtScreen( w ) ) , 
			PixW , PixH , 
			( w->combobox.LabelCtrl )->core.depth ) ;
	XCopyPlane( display , LabelBitmap , LabelPixmap , 
			ColorGC , 0 , 0 , PixW , PixH , 0 , 0 , 1 ) ;
	XtVaSetValues( w->combobox.LabelCtrl , Resource , LabelPixmap , NULL ) ;
	XFreePixmap( display , LabelBitmap ) ;
	return LabelPixmap ;
	} /* BitmapToPixmap */

static void Initialize( Widget request , XmsrComboBoxWidget new , 
	ArgList wargs , Cardinal *ArgCount )
	{
	Dimension width , height , dummy ;
	Widget w ;
	Arg args[10] ;
	int n = 0 ;
	XmString xmstr ;
	Pixel BackgroundColor ;
	
	XtAppAddActions( XtWidgetToApplicationContext( ( Widget ) new ) , 
		actions , XtNumber( actions ) ) ;
	
	new->combobox.ConvertBitmapToPixmap = False ;
	new->combobox.ConvertBitmapToPixmapInsensitive = False ;
	
	w = ( Widget ) new ;
	while ( !XtIsSubclass( w , shellWidgetClass ) )
		w = XtParent( w ) ;
	new->combobox.MyNextShell = w ;
	XtAddEventHandler( w , StructureNotifyMask | FocusChangeMask | ExposureMask , 
		False , ( XtEventHandler ) ShellCallback , ( XtPointer ) new ) ;

	new->combobox.EditCtrl = XtVaCreateManagedWidget( 
		"edit" ,
		xmTextFieldWidgetClass , ( Widget ) new ,
		XmNverifyBell , False , 
		NULL ) ;
	XtAddCallback( new->combobox.EditCtrl , XmNlosingFocusCallback , 
			( XtCallbackProc ) EditFocusCallback , NULL ) ;
	XtAddCallback( new->combobox.EditCtrl , XmNmodifyVerifyCallback , 
			( XtCallbackProc ) EditVerifyCallback , NULL ) ;
	XtAddCallback( new->combobox.EditCtrl , XmNvalueChangedCallback , 
			( XtCallbackProc ) EditChangedCallback , NULL ) ;
	XtAddCallback( new->combobox.EditCtrl , XmNhelpCallback , 
			( XtCallbackProc ) HelpCallback , ( XtPointer ) new ) ;
	XtOverrideTranslations( new->combobox.EditCtrl , NewEditTranslations ) ;
	if ( !new->combobox.Editable )
		XtOverrideTranslations( new->combobox.EditCtrl , NewEditTranslationsNE ) ;
#ifdef NODRAGNDROP
	XtOverrideTranslations( new->combobox.EditCtrl , 
			NewListTranslations ) ; /* Btn2Dwn aus! */
#endif

/* --- */
	new->combobox.ArrowCtrl = XtVaCreateManagedWidget( 
		"arrow" ,
		xmArrowButtonWidgetClass ,	( Widget ) new , 
		XmNarrowDirection ,		XmARROW_DOWN , 
		XmNtraversalOn , 		False , 
		XmNnavigationType ,		XmNONE ,
		XmNborderWidth ,		0 , 
		XmNhighlightThickness ,		0 , 
		NULL ) ;
	XmRemoveTabGroup( new->combobox.ArrowCtrl ) ;
	XtAddEventHandler( new->combobox.ArrowCtrl , EnterWindowMask | LeaveWindowMask ,
			False , ( XtEventHandler ) ArrowCrossingCallback , ( XtPointer ) new ) ;
	XtAddCallback( new->combobox.ArrowCtrl , XmNactivateCallback , 
			( XtCallbackProc ) ArrowCallback , NULL ) ;
	XtAddCallback( new->combobox.ArrowCtrl , XmNarmCallback , 
			( XtCallbackProc ) ArrowCallback , NULL ) ;
	XtAddCallback( new->combobox.ArrowCtrl , XmNhelpCallback , 
			( XtCallbackProc ) HelpCallback , ( XtPointer ) new ) ;
/* --- */
	new->combobox.LabelCtrl = XtVaCreateWidget( 
		"label" ,
		xmLabelWidgetClass ,	( Widget ) new , 
		XmNstringDirection ,	new->manager.string_direction , 
		NULL ) ;
	if ( new->combobox.ShowLabel )
		{
		XtManageChild( ( Widget ) new->combobox.LabelCtrl ) ;
		XtAddCallback( new->combobox.LabelCtrl , 
				XmNhelpCallback , 
				( XtCallbackProc ) HelpCallback , 
				( XtPointer ) new ) ;
		}

/* PFM CHANGE >>> (avoid strange pop-through windows lingering around when application window is lowered) */
	new->combobox.PopupShell = XtVaCreateWidget( 
		"combobox_shell" ,
		vendorShellWidgetClass ,	( Widget ) new ,
		/* The following class is a better choice for mwm but not for PC-based
		   servers such as DENNIS PAGE's! */
		/* transientShellWidgetClass ,	( Widget ) new , */
		/* XmNmwmDecorations ,		False , */
		/* With mwm the following line can be commented out: */
		XmNoverrideRedirect ,		True ,
		XmNsaveUnder ,			True ,
		XmNallowShellResize ,		True , 
		NULL ) ;

	XtAddEventHandler( new->combobox.PopupShell , EnterWindowMask | LeaveWindowMask , 
		False , ( XtEventHandler ) OverrideShellCallback , ( XtPointer ) new ) ;

	XtSetArg( args[n] , XmNselectionPolicy , XmBROWSE_SELECT ) ; n++ ;
	XtSetArg( args[n] , XmNhighlightThickness , 0 ) ; n++ ;
	XtSetArg( args[n] , XmNautomaticSelection , False ) ; n++ ;
	XtSetArg( args[n] , XmNscrollBarDisplayPolicy , XmSTATIC ) ; n++ ;
	XtSetArg( args[n] , XmNlistSizePolicy , XmVARIABLE ) ; n++ ;
	XtSetArg( args[n] , XmNvisibleItemCount , new->combobox.VisibleItemCount ) ; n++ ;

	new->combobox.ListCtrl = XmCreateScrolledList(
		new->combobox.PopupShell , "list" , 
		args , n ) ;
	XtVaSetValues( new->combobox.ListCtrl , 
			XmNtraversalOn , False , NULL ) ;
	XtVaSetValues( XtParent( new->combobox.ListCtrl ) , 
			XmNtraversalOn , False , NULL ) ;
			
	XtManageChild( new->combobox.ListCtrl ) ;
	XtAddCallback( new->combobox.ListCtrl , 
			XmNsingleSelectionCallback , 
				( XtCallbackProc ) ListSelectionCallback , NULL ) ;
	XtAddCallback( new->combobox.ListCtrl , 
			XmNbrowseSelectionCallback , 
				( XtCallbackProc ) ListSelectionCallback , NULL ) ;
	XtAddCallback( new->combobox.ListCtrl , 
			XmNdefaultActionCallback , 
				( XtCallbackProc ) ListSelectionCallback , NULL ) ;
	XtAddCallback( new->combobox.ListCtrl , 
			XmNhelpCallback , 
				( XtCallbackProc ) HelpCallback , 
				( XtPointer ) new ) ;
#ifdef NODRAGNDROP
	XtOverrideTranslations( new->combobox.ListCtrl , 
			NewListTranslations ) ;
#endif
	InitMirrorResources( new ) ;
	SR_UpdateColors( new , -1 ) ;
	SetValues( new , new , new , wargs , ArgCount ) ;
	
	if ( new->combobox.ConvertBitmapToPixmap )
		new->combobox.LabelPixmap =
			BitmapToPixmap( new , XmNlabelPixmap , 
			( ( XmLabelRec * ) new->combobox.LabelCtrl )->
				label.normal_GC ) ;
	if ( new->combobox.ConvertBitmapToPixmapInsensitive )
		new->combobox.LabelInsensitivePixmap =
			BitmapToPixmap( new , XmNlabelInsensitivePixmap , 
			( ( XmLabelRec * ) new->combobox.LabelCtrl )->
				label.insensitive_GC ) ;
	
	DefaultGeometry( new , &width , &height , &dummy , &dummy ) ;
	if ( new->core.width == 0 )
		new->core.width = width ;
	if ( new->core.height == 0 )
		new->core.height = height ;

	if ( new->combobox.Font == NULL )
		{
		XtVaGetValues( new->combobox.EditCtrl , 
			XmNfontList , &new->combobox.Font , NULL ) ;
		XtVaSetValues( new->combobox.ListCtrl , 
			XmNfontList , new->combobox.Font , NULL ) ;
		XtVaSetValues( new->combobox.LabelCtrl , 
			XmNfontList , new->combobox.Font , NULL ) ;
		}
	else
		{
		XtVaSetValues( new->combobox.ListCtrl , 
			XmNfontList , new->combobox.Font , NULL ) ;
		XtVaSetValues( new->combobox.EditCtrl , 
			XmNfontList , new->combobox.Font , NULL ) ;
		XtVaSetValues( new->combobox.LabelCtrl , 
			XmNfontList , new->combobox.Font , NULL ) ;
		}
	
	new->combobox.ListVisible = False ;
	/* SIMPLIFY
	new->combobox.IgnoreFocusOut = False ;
	new->combobox.PendingFocusOut = False ;
	new->combobox.PendingOverrideInOut = False ;
	*/
	
	new->combobox.PassVerification = False ;

	DoLayout( new ) ;
	} /* Initialize */

Widget XmsrComboBoxGetEditWidget( Widget w )
	{
	return ( ( XmsrComboBoxWidget ) w )->combobox.EditCtrl ;
	} /* XmsrComboBoxGetEditWidget */

Widget XmsrComboBoxGetListWidget( Widget w )
	{
	return ( ( XmsrComboBoxWidget ) w )->combobox.ListCtrl ;
	} /* XmsrComboBoxGetListWidget */

Widget XmsrComboBoxGetLabelWidget( Widget w )
	{
	return ( ( XmsrComboBoxWidget ) w )->combobox.LabelCtrl ;
	} /* XmsrComboBoxGetLabelWidget */

static void UpdatesrComboBox( XmsrComboBoxWidget w , int Index , Boolean Deleted )
	{
	int OldIndex , ItemCount ;
	
	OldIndex = XmsrComboBoxGetSelectedPos( ( Widget ) w ) ;
	if ( OldIndex == Index )
		{
		if ( Deleted )
			{
			XtVaGetValues( w->combobox.ListCtrl , 
				XmNitemCount , &ItemCount , NULL ) ;
			if ( ItemCount != 0 )
				{
				if ( Index >= ItemCount ) Index = ItemCount ;
					SetSelectionPos( w , Index , False ) ;
				}
			}
		}
	if ( !w->combobox.Editable )
		{
		TransferToEditCtrl( w , Index ) ;
		}
	} /* UpdatesrComboBox */


static int FindSortedItemPos( XmsrComboBoxWidget w , XmString item )
	{
	Widget ListBox = w->combobox.ListCtrl ;
	XmStringTable Items ;
	int ItemCount , i , BestFit ;
	char *pItemText , *pCompareText ;
	
	XtVaGetValues( ListBox , XmNitems , &Items , 
			XmNitemCount , &ItemCount , NULL ) ;
	XmStringGetLtoR( item , XmSTRING_DEFAULT_CHARSET , &pCompareText ) ;

	BestFit = 0 ;
	for ( i = 0 ; i < ItemCount ; i++ )
		{
		XmStringGetLtoR( Items[i] , XmSTRING_DEFAULT_CHARSET , &pItemText ) ;
		if ( strcmp( pItemText , pCompareText ) > 0 )
			{
			XtFree( pItemText ) ;
			BestFit = i+1 ;
			break ;
			}
		XtFree( pItemText ) ;
		}
	XtFree( pCompareText ) ;
	return BestFit ;
	} /* FindSortedItemPos */

static Boolean ChecksrComboBox( Widget w , char *pFuncName )
	{
	char buff[256] ;
	char *pWName ;
	
	if ( XmIssrComboBox( w ) ) return False ;
	pWName = XrmQuarkToString( w->core.xrm_name ) ;
	sprintf( buff , 
		"Warning: %s called on widget named %s which is \
		not a descendent of class XmsrComboBox!" , 
		pFuncName , pWName ) ;
	XtWarning( buff ) ;
	return True ;
	} /* ChecksrComboBox */

#define ListBox ( ( ( XmsrComboBoxWidget ) w )->combobox.ListCtrl )
#define EditBox ( ( ( XmsrComboBoxWidget ) w )->combobox.EditCtrl )
#define srComboBox ( ( XmsrComboBoxWidget ) w )

void XmsrComboBoxAddItem( Widget w , XmString item , int pos )
	{
	int OldIndex = XmsrComboBoxGetSelectedPos( w ) ;
	
	if ( ChecksrComboBox( w , "XmsrComboBoxAddItem" ) ) return ;
	if ( srComboBox->combobox.Sorted )
		pos = FindSortedItemPos( srComboBox , item ) ;
	XmListAddItem( ListBox , item , pos ) ;
	if ( OldIndex != XmsrComboBoxGetSelectedPos( w ) )
		SetSelectionPos( srComboBox , OldIndex , False ) ;
	} /* XmsrComboBoxAddItem */

void XmsrComboBoxAddItems( Widget w , XmString *items , int item_count , int pos )
	{
	int OldIndex = XmsrComboBoxGetSelectedPos( w ) ;

	if ( ChecksrComboBox( w , "XmsrComboBoxAddItems" ) ) return ;
		XmListAddItems( ListBox , items , item_count , pos ) ;
	if ( OldIndex != XmsrComboBoxGetSelectedPos( w ) )
		SetSelectionPos( srComboBox , OldIndex , False ) ;
	} /* XmsrComboBoxAddItems */

void XmsrComboBoxAddItemUnselected( Widget w , XmString item , int pos )
	{ XmListAddItemUnselected( ListBox , item , pos ) ; }

void XmsrComboBoxDeleteItem( Widget w , XmString item )
	{
	int Index = XmListItemPos( ListBox , item ) ;

	if ( ChecksrComboBox( w , "XmsrComboBoxDeleteItem" ) ) return ;
	if ( Index ) XmsrComboBoxDeletePos( w , Index ) ;
	} /* XmsrComboBoxDeleteItem */

void XmsrComboBoxDeleteItems( Widget w , XmString *items , int item_count )
	{
	int i ;

	if ( ChecksrComboBox( w , "XmsrComboBoxDeleteItems" ) ) return ;
	for ( i = 0 ; i < item_count ; i++ )
	XmListDeleteItem( w , items[i] ) ;
	} /* XmsrComboBoxDeleteItems */

void XmsrComboBoxDeletePos( Widget w , int pos )
	{
	int OldIndex = XmsrComboBoxGetSelectedPos( w ) ;
	
	if ( ChecksrComboBox( w , "XmsrComboBoxDeletePos" ) ) return ;
	XmListDeletePos( ListBox , pos ) ;
	if ( pos == OldIndex ) UpdatesrComboBox( srComboBox , pos , True ) ;
	} /* XmsrComboBoxDeletePos */

void XmsrComboBoxDeleteItemsPos( Widget w , int item_count , int pos )
	{
	int i ;

	if ( ChecksrComboBox( w , "XmsrComboBoxDeleteItemsPos" ) ) return ;
	for ( i = 0 ; i < item_count ; i++ )
		XmsrComboBoxDeletePos( w , pos++ ) ;
	} /* XmsrComboBoxDeleteItemsPos */

void XmsrComboBoxDeleteAllItems( Widget w )
	{
	if ( ChecksrComboBox( w , "XmsrComboBoxAllDeleteItems" ) ) return ;
	XmListDeleteAllItems( ListBox ) ;
	UpdatesrComboBox( srComboBox , 0 , True ) ;
	} /* XmsrComboBoxDeleteAllItems */

void XmsrComboBoxReplaceItems( Widget w , XmString *old_items , int item_count , XmString *new_items )
	{
	if ( ChecksrComboBox( w , "XmsrComboBoxReplaceItems" ) ) return ;
	XmListReplaceItems( ListBox , old_items , item_count , new_items ) ;
	UpdatesrComboBox( srComboBox , XmsrComboBoxGetSelectedPos( w ) , False ) ;
	} /* XmsrComboBoxReplaceItems */

void XmsrComboBoxReplaceItemsPos( Widget w , XmString *new_items , int item_count , int position )
	{
	int OldIndex = XmsrComboBoxGetSelectedPos( w ) ;

	if ( ChecksrComboBox( w , "XmsrComboBoxReplaceItemsPos" ) ) return ;
	XmListReplaceItemsPos( ListBox , new_items , item_count , position ) ;
	if ( ( OldIndex >= position ) && ( OldIndex < position + item_count ) )
	UpdatesrComboBox( srComboBox , OldIndex , False ) ;
	} /* XmsrComboBoxReplaceItemsPos */

Boolean XmsrComboBoxItemExists( Widget w , XmString item )
	{
	if ( ChecksrComboBox( w , "XmsrComboBoxItemExists" ) ) return False ;
	return XmListItemExists( ListBox , item ) ;
	} /* XmsrComboBoxItemExists */

int XmsrComboBoxItemPos( Widget w , XmString item )
	{
	if ( ChecksrComboBox( w , "XmsrComboBoxItemPos" ) ) return 0 ;
	return XmListItemPos( ListBox , item ) ;
	} /* XmsrComboBoxItemPos */

Boolean XmsrComboBoxGetMatchPos( Widget w , XmString item , int **pos_list , int *pos_count )
	{
	if ( ChecksrComboBox( w , "XmsrComboBoxGetMatchPos" ) ) return False ;
	return XmListGetMatchPos( ListBox , item , pos_list , pos_count ) ;
	} /* XmsrComboBoxGetMatchPos */

void XmsrComboBoxSelectPos( Widget w , int pos , Boolean notify )
	{
	int index ;
	
	if ( ChecksrComboBox( w , "XmsrComboBoxSelectPos" ) ) return ;
	index = SetSelectionPos( srComboBox , pos , notify ) ;
	if ( index ) TransferToEditCtrl( srComboBox , index ) ;
	} /* XmsrComboBoxSelectPos */

void XmsrComboBoxSelectItem( Widget w , XmString item , Boolean notify )
	{
	int index ;
	
	if ( ChecksrComboBox( w , "XmsrComboBoxSelectItem" ) ) return ;
	XmListSelectItem( ListBox , item , notify ) ;
	index = SetSelectionPos( srComboBox , XmsrComboBoxGetSelectedPos( w ) , False ) ;
	if ( index ) TransferToEditCtrl( srComboBox , index ) ;
	} /* XmsrComboBoxSelectItem */

int XmsrComboBoxGetSelectedPos( Widget w )
	{
	int *SelectionList , SelectionCount , SelectionIndex ;
	
	if ( ChecksrComboBox( w , "XmsrComboBoxGetSelectedPos" ) ) return 0 ;
	if ( XmListGetSelectedPos( ListBox ,
			&SelectionList , &SelectionCount ) ) {
	SelectionIndex = *SelectionList ;
	XtFree( ( char * )SelectionList ) ;
	} else SelectionIndex = 0 ;
	return SelectionIndex ;
	} /* XmsrComboBoxGetSelectedPos */

void XmsrComboBoxClearSelection( Widget w , Time time )
	{
	XmTextFieldClearSelection( EditBox , time ) ;
	} /* XmsrComboBoxClearSelection */

Boolean XmsrComboBoxCopy( Widget w , Time time )
	{
	return XmTextFieldCopy( EditBox , time ) ;
	} /* XmsrComboBoxCopy */

Boolean XmsrComboBoxCut( Widget w , Time time )
	{
	return XmTextFieldCut( EditBox , time ) ;
	} /* XmsrComboBoxCut */

XmTextPosition XmsrComboBoxGetInsertionPosition( Widget w )
	{
	return XmTextFieldGetInsertionPosition( EditBox ) ;
	} /* XmsrComboBoxGetInsertionPosition */

XmTextPosition XmsrComboBoxGetLastPosition( Widget w )
	{
	return XmTextFieldGetLastPosition( EditBox ) ;
	} /* XmsrComboBoxGetLastPosition */

int XmsrComboBoxGetMaxLength( Widget w )
	{
	return XmTextFieldGetMaxLength( EditBox ) ;
	} /* XmsrComboBoxGetMaxLength */

char * XmsrComboBoxGetSelection( Widget w )
	{
	return XmTextFieldGetSelection( EditBox ) ;
	} /* XmsrComboBoxGetSelection */

Boolean XmsrComboBoxGetSelectionPosition( Widget w , XmTextPosition *left , 
	XmTextPosition *right )
	{
	return XmTextFieldGetSelectionPosition( EditBox , left , right ) ;
	} /* XmsrComboBoxGetSelectionPosition */

char * XmsrComboBoxGetString( Widget w )
	{
	return XmTextFieldGetString( EditBox ) ;
	} /* XmsrComboBoxGetString */

void XmsrComboBoxInsert( Widget w , XmTextPosition position , char *value )
	{
	XmTextFieldInsert( EditBox , position , value ) ;
	} /* XmsrComboBoxInsert */

Boolean XmsrComboBoxPaste( Widget w )
	{
	return XmTextFieldPaste( EditBox ) ;
	} /* XmsrComboBoxPaste */

Boolean XmsrComboBoxRemove( Widget w )
	{
	return XmTextFieldRemove( EditBox ) ;
	} /* XmsrComboBoxRemove */

void XmsrComboBoxReplace( Widget w , XmTextPosition from_pos , 
			XmTextPosition to_pos , char *value )
	{
	XmTextFieldReplace( EditBox , from_pos , to_pos , value ) ;
	} /* XmsrComboBoxReplace */

void XmsrComboBoxSetAddMode( Widget w , Boolean state )
	{
	XmTextFieldSetAddMode( EditBox , state ) ;
	} /* XmsrComboBoxSetAddMode */

void XmsrComboBoxSetHighlight( Widget w , XmTextPosition left , 
			XmTextPosition right , XmHighlightMode mode )
	{
	XmTextFieldSetHighlight( EditBox , left , right , mode ) ;
	} /* XmsrComboBoxSetHighlight */

void XmsrComboBoxSetInsertionPosition( Widget w , XmTextPosition position )
	{
	XmTextFieldSetInsertionPosition( EditBox , position ) ;
	} /* XmsrComboBoxSetInsertionPosition */

void XmsrComboBoxSetMaxLength( Widget w , int max_length )
	{
	XmTextFieldSetMaxLength( EditBox , max_length ) ;
	} /* XmsrComboBoxSetMaxLength */

void XmsrComboBoxSetSelection( Widget w , XmTextPosition first , 
		XmTextPosition last , Time time )
	{
	XmTextFieldSetSelection( EditBox , first , last , time ) ;
	} /* XmsrComboBoxSetSelection */

void XmsrComboBoxSetString( Widget w , char *value )
	{
	if ( ( value == NULL ) || ( *value == 0 ) )
		{
		XtVaSetValues( w , XmNvalue , "" , NULL ) ;
		/* PFM: don't understand why following line wasn't included in the original code: */
		XmTextFieldSetString( EditBox , "" ) ;
		}
	else
		XmTextFieldSetString( EditBox , value ) ;
	} /* XmsrComboBoxSetString */

void XmsrComboBoxShowPosition( Widget w , XmTextPosition position )
	{
	XmTextFieldShowPosition( EditBox , position ) ;
	} /* XmsrComboBoxShowPosition */

/* Drop Down List*/
void XmsrComboBoxShowList( Widget w )
	{
	if ( ChecksrComboBox( w , "XmsrComboBoxShowList" ) ) return ;
	ShowHideDropDownList( ( XmsrComboBoxWidget ) w , NULL , False ) ;
	} /* XmsrComboBoxShowList */

void XmsrComboBoxHideList( Widget w )
	{
	if ( ChecksrComboBox( w , "XmsrComboBoxHideList" ) ) return ;
	ShowHideDropDownList( ( XmsrComboBoxWidget ) w , NULL , True ) ;
	} /* XmsrComboBoxShowList */
#endif
