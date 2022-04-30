#if HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef GUI
#	include <Xm/AtomMgr.h>
#	include <Xm/Protocols.h>
#endif

#include "C_list.H"
#include "UI_GeneralMotifInclude.H"
#include "UI_mainwindow.H"

#include "panel.H"

// #define DEBUG_MOTIF

#ifdef DEBUG_MOTIF
extern int lev , L ;
#endif

int			panel::first_time (1);
PANEL_TYPE		panel::type = UNDEFINED ;
SELECTION_WIDGET_TYPE	panel::SelectionWidgetType = NONE ;
callback_stuff		panel::data_for_closing_window( NULL , panel::Cancel , NULL , NULL );
Cstring			panel::WidgetName( "APanel" );

panel::panel( Cstring /* name */ )
	: Selections( NULL )
	{}

// String pointers default to NULL:
void panel::create_widgets(
		PANEL_TYPE requested_type ,
		Cstring * OptionalString1 ,
		Cstring * OptionalString2 ,
		Cstring * OkButtonString ,
		Cstring * CancelButtonString ,
		Cstring * HelpButtonString ,
		Cstring * AddButtonString
	) {
#ifdef GUI
	if( GUI_Flag ) {
	XmString temp1 ;
	int n ;
	Arg args[15] ;
	Position x , y ;
	callback_proc proc_to_call_when_closing_window = NULL ;
	Widget shell ;

	type = requested_type ;

	XtVaGetValues( UI_mainwindow::mainWindowObject->widget , XmNx , &x , XmNy , &y , NULL ) ;

	data_for_closing_window.data = this ;
	proc_to_call_when_closing_window = Cancel ;

	if( type == BASIC_DIALOG ) {
		PopupWidget = new motif_widget( WidgetName , xmDialogShellWidgetClass , NULL ) ;
		shell = PopupWidget->widget ;

		FormWidget = new motif_widget( "FormWidget" , PopupWidget , form_position( 50 ) , TRUE ) ; }
	else if(	type == FILE_SELECTION_DIALOG ||
			type == FILE_SELECTION_DIALOG_W_NAME_AND_DESCR ) {
		XmString temp1 , temp2 , temp3 , temp4 , temp5 ;

		n = 0 ;
		if( OptionalString1 ) {
			temp1 = XmStringCreateLocalized( (char *) *(*OptionalString1) ) ;
			XtSetArg( args[n] , XmNpattern , temp1 ) ; n++ ; }
		if( OptionalString2 ) {
			temp2 = XmStringCreateLocalized( (char *) *(*OptionalString2) ) ;
			XtSetArg( args[n] , XmNselectionLabelString , temp2 ) ; n++ ; }
		if( OkButtonString ) {
			temp3 = XmStringCreateLocalized( (char *) *(*OkButtonString) ) ;
			XtSetArg( args[n] , XmNokLabelString , temp3 ) ; n++ ; }
		if( CancelButtonString ) {
			temp4 = XmStringCreateLocalized( (char *) *(*CancelButtonString) ) ;
			XtSetArg( args[n] , XmNcancelLabelString , temp4 ) ; n++ ; }
		if( HelpButtonString ) {
			temp5 = XmStringCreateLocalized( (char *) *(*HelpButtonString) ) ;
			XtSetArg( args[n] , XmNhelpLabelString , temp5 ) ; n++ ; }
		XtSetArg( args[n] , XmNdeleteResponse ,		XmDO_NOTHING ) ; n++ ;
		PopupWidget = new motif_widget( WidgetName ,
			xmFileSelectionDialogWidgetClass ,	UI_mainwindow::mainWindowObject,
			args ,					n ,
			FALSE ) ;

		shell = XtParent( PopupWidget->widget ) ;

		FormWidget = new motif_widget( "FormWidget" , PopupWidget , form_position( 50 ) , FALSE ) ;

		if( OptionalString1 ) XmStringFree( temp1 ) ;
		if( OptionalString2 ) XmStringFree( temp2 ) ;

		if( OkButtonString )
			XmStringFree( temp3 ) ;
		else
			XtUnmanageChild( XmSelectionBoxGetChild( PopupWidget->widget , XmDIALOG_OK_BUTTON ) ) ;

		if( CancelButtonString )
			XmStringFree( temp4 ) ;
		else
			XtUnmanageChild( XmSelectionBoxGetChild( PopupWidget->widget , XmDIALOG_CANCEL_BUTTON ) ) ;

		if( HelpButtonString )
			XmStringFree( temp5 ) ;
		else
			XtUnmanageChild( XmSelectionBoxGetChild( PopupWidget->widget , XmDIALOG_HELP_BUTTON ) ) ;

		PopupWidget->add_callback( Apply , XmNokCallback , ( void * ) this ) ;
		PopupWidget->add_callback( Cancel , XmNcancelCallback, ( void * ) this ) ;
		PopupWidget->add_callback( Catalog , XmNhelpCallback, ( void * ) this ) ;

		if(type == FILE_SELECTION_DIALOG_W_NAME_AND_DESCR) {
			motif_widget *NameLabel , *DescriptionLabel ;

			NameLabel = new motif_widget("Name: ", xmLabelWidgetClass, FormWidget);
			*FormWidget ^ 5 ^ *NameLabel;
			*FormWidget < *NameLabel;
			NameWidget = new motif_widget("NameWidget", xmTextWidgetClass, FormWidget);
			*NameLabel < *NameWidget;
			*FormWidget ^ *NameWidget < *FormWidget;

			DescriptionLabel = new motif_widget("Description: ", xmLabelWidgetClass, FormWidget);
			*NameWidget ^ 5 ^ *DescriptionLabel;
			*FormWidget < *DescriptionLabel;

			DescriptionWidget = new motif_widget("DescriptionWidget", xmTextWidgetClass, FormWidget);
			*DescriptionLabel < *DescriptionWidget < *FormWidget;
			*NameWidget ^ *DescriptionWidget;
			if(!AddButtonString) {
				//DT 4/3/96  The follow two line is rem out for the HELP button in the define_descriptor screen
				//*DescriptionLabel ^ *FormWidget;
				//*DescriptionWidget ^ *FormWidget;
				}
			else {
				AddButton = new motif_widget(*(*AddButtonString), xmPushButtonWidgetClass, FormWidget);
				form_position(20) < *AddButton < form_position(30);
				*DescriptionWidget ^ 10 ^ *AddButton;
				AddButton->add_callback(Add, XmNactivateCallback, (void*) this); }
			FormWidget->manage(); }
		PopupWidget->manage();
		proc_to_call_when_closing_window = Cancel; }
	else if(type == PANED_WINDOW) {
		DialogWidget = new motif_widget(WidgetName, xmDialogShellWidgetClass, NULL, 0, FALSE);

		shell = DialogWidget->widget;

		PopupWidget = new motif_widget("PanedWindow", xmPanedWindowWidgetClass, DialogWidget, NULL, 0, FALSE);
		proc_to_call_when_closing_window = Cancel; }
	else if(type == PANED_WINDOW_W_LIST) {
		DialogWidget = new motif_widget(WidgetName, xmDialogShellWidgetClass, NULL, 0, FALSE);

		shell = DialogWidget->widget;

		PopupWidget = new motif_widget("PanedWindow", xmPanedWindowWidgetClass, DialogWidget, NULL, 0, FALSE);

		FormWidget = new motif_widget("FormWidget", PopupWidget, form_position(50), TRUE);

		SelectionList = new scrolled_list_widget("Select A Catalog Item", FormWidget, 10, TRUE);
		*FormWidget ^ *SelectionList;
		*FormWidget < *SelectionList < *FormWidget;
		SelectionList->add_callback(Catalog, XmNsingleSelectionCallback, (void*) this);
		SelectionWidgetType = LIST_WIDGET;
		proc_to_call_when_closing_window = Cancel; }
	else if( type == PROMPT_DIALOG ) {
		PopupWidget = new motif_widget( WidgetName , xmPromptDialogWidgetClass , NULL ) ;
		if( OptionalString1 ) *PopupWidget = *OptionalString1 ;

		shell = XtParent( PopupWidget->widget ) ;

		if( OkButtonString )
			{
			temp1 = XmStringCreateLocalized( (char *) *(*OkButtonString) ) ;
			XtVaSetValues( PopupWidget->widget , XmNokLabelString , temp1 , NULL ) ;
			XmStringFree( temp1 ) ;
			}
		if( CancelButtonString )
			{
			temp1 = XmStringCreateLocalized( (char *) *(*CancelButtonString) ) ;
			XtVaSetValues( PopupWidget->widget , XmNcancelLabelString , temp1 , NULL ) ;
			PopupWidget->add_callback( Cancel , XmNcancelCallback , ( void * ) this ) ;
			XmStringFree( temp1 ) ;
			}
		else
			XtUnmanageChild( XmSelectionBoxGetChild( PopupWidget->widget , XmDIALOG_CANCEL_BUTTON ) ) ;
		if( HelpButtonString )
			{
			temp1 = XmStringCreateLocalized( (char *) *(*HelpButtonString) ) ;
			XtVaSetValues( PopupWidget->widget , XmNhelpLabelString , temp1 , NULL ) ;
			PopupWidget->add_callback( Add , XmNhelpCallback , ( void * ) this ) ;
			XmStringFree( temp1 ) ;
			}
		else
			XtUnmanageChild( XmSelectionBoxGetChild( PopupWidget->widget , XmDIALOG_HELP_BUTTON ) ) ;
		PopupWidget->add_callback( Apply , XmNokCallback , ( void * ) this ) ;
		proc_to_call_when_closing_window = Apply ; }
	else if( type == INFORMATION_PANEL ) {
		int x1 , x2 ;

		DialogWidget = new motif_widget( WidgetName , xmDialogShellWidgetClass , NULL , NULL , 0 , FALSE ) ;

		shell = DialogWidget->widget ;

		PopupWidget = new motif_widget( "PanedWindow" , xmPanedWindowWidgetClass , DialogWidget , NULL , 0 , FALSE ) ;

		n = 0 ;
		XtSetArg( args[n] , XmNrows ,           	10 ) ;			n++ ;
		XtSetArg( args[n] , XmNcolumns ,         	81 ) ;			n++ ;
		XtSetArg( args[n] , XmNwordWrap ,	   	True ) ;		n++ ;
		XtSetArg( args[n] , XmNscrollHorizontal ,	False ) ;		n++ ;
		XtSetArg( args[n] , XmNeditable ,		False ) ;               n++ ;
		XtSetArg( args[n] , XmNeditMode ,		XmMULTI_LINE_EDIT ) ;   n++ ;
		XtSetArg( args[n] , XmNheight ,			250 ) ;			n++ ;
		XtSetArg( args[n] , XmNcursorPositionVisible ,	False ) ;		n++ ;

		InfoWidget = new motif_widget( "text_w",
			xmScrolledTextWidgetClass ,	PopupWidget ,
			args ,				n ,
			TRUE ) ;

		FormWidget = new motif_widget( "Error" , PopupWidget , form_position( 50 ) , TRUE ) ;

		if( OptionalString2 )
			{
			x1 = 10 ; x2 = 22 ;
			}
		else
			{
			x1 = 19 ; x2 = 31 ;
			}

		ContinueButton = new motif_widget( "ContinueButton" , xmPushButtonWidgetClass , FormWidget ) ;
		*ContinueButton = *OptionalString1;
		form_position( x1 ) < *ContinueButton < form_position(x2);
		*ContinueButton ^ *FormWidget;

		if( OptionalString2 ) {
			MoreButton = new motif_widget( "ContinueButton" , xmPushButtonWidgetClass , FormWidget ) ;
			! ( form_position( 30 ) < *MoreButton < form_position( 42 ) ) ;
			! ( *MoreButton ^ *FormWidget ) ;
			*MoreButton = *OptionalString2 ;
	// BKL REMOVED		MoreButton->add_callback( Add , XmNactivateCallback , ( void * ) this ) ;
			}
	// BKL REMOVED  ContinueButton->add_callback( Cancel , XmNactivateCallback , ( void * ) this ) ;
#ifdef HAVE_MACOS
		FormWidget->fix_height(32, ContinueButton->widget);
#else
		FormWidget->fix_height( ContinueButton->widget ) ;
#endif /* HAVE_MACOS */
		proc_to_call_when_closing_window = Cancel ; }
	AllowWindowClosingFromMotifMenu( shell , proc_to_call_when_closing_window , & data_for_closing_window ) ;
	}
#endif
}

panel::~panel()
	{}

void panel::Apply( Widget , callback_stuff * client_data , void * ) {
	panel	*p = ( panel * ) client_data->data ;

	p->apply() ; }

void panel::Cancel( Widget , callback_stuff * client_data , void * ) {
	panel	*p = ( panel * ) client_data->data ;

	p->cancel() ; }

void panel::Catalog( Widget , callback_stuff * client_data , void * call_data ) {}


void panel::Add( Widget , callback_stuff * client_data , void * call_data ) {
	panel	*p = ( panel * ) client_data->data ;

	p->add( call_data ) ; }

Cstring panel::ExtractFilename() {
        Cstring		InputString ;
#ifdef GUI
	if( GUI_Flag ) {
	Widget		text = XmSelectionBoxGetChild( PopupWidget->widget , XmDIALOG_TEXT ) ;
	char		*filename = XmTextGetString( text ) ;

	InputString = Cstring( filename ) ;
	XtFree( filename ) ;
	}
#endif
	return InputString ; 
}
