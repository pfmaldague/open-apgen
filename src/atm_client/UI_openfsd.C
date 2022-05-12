#if HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef GUI
#include <Xm/FileSB.h>
#endif

#include <unistd.h>
// #include <strstream.h>

#include "ACT_exec.H"
#include <ActivityInstance.H>
#include "UI_utility.H"
#include "UI_openfsd.H"
#include "UI_exec.H"
#include "UI_GeneralMotifInclude.H"
#include "UI_motif_widget.H"
#include "UI_messagewarndialog.H"
#include "action_request.H"

			// used in file output:
Cstring			pendingInputFile ;

			// in action_request.C:
extern strinttlist&	FileNamesWithInstancesAndOrNonLayoutDirectives();

static tlist<alpha_void, Cntnr<alpha_void, CTime_base> > ourPrivateList;

                        // in UI_motif_widget.C:
extern resource_data    ResourceData ;
extern UI_exec*		UI_subsystem ;


#ifdef GUI
extern XtPointer CONVERT(Widget, const char *, const char *, int, Boolean *);
extern void		RegisterBxConverters(XtAppContext);
extern XtPointer	DOUBLE(double);
extern XtPointer	SINGLE(float);
extern void		MENU_POST(Widget, XtPointer, XEvent *, Boolean *);
extern Pixmap		XPM_PIXMAP(Widget, char**);
extern void		SET_BACKGROUND_COLOR(Widget, ArgList, Cardinal*, Pixel);

			// in main.C:
extern int		GUI_Flag ;
#endif
extern motif_widget	*MW ;

bool			UI_openfsd::disabled = false;

void disable_discrepancy_popup() {
	ACT_exec::dur_discrepancy_popup_disabled = true;
	UI_openfsd::disabled = true; }

UI_openfsd::UI_openfsd(const char *name ) : motif_widget( name , xmPopupShellWidgetClass , MW ) {
#ifdef GUI
	if( GUI_Flag ) {
		create( MW->widget ) ;
	}
#endif
	}

#ifdef GUI
void UI_openfsd::create( Widget ) {
	if( GUI_Flag ) {
	Arg      	args[256];
	Cardinal 	ac=0;
	Boolean  	argok=False;

	ac = 0 ;
	XtSetArg(args[ac], XmNautoUnmanage, False ) ; ac++ ;
	XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_NONE) ; ac++ ;
	XtSetArg(args[ac], XmNdialogTitle, 
		CONVERT( widget , "Open File", 
		XmRXmString, 0, &argok)); if (argok) ac++;
        XtSetArg(args[ac], XmNheight, 640); ac++;
        XtSetArg(args[ac], XmNwidth, 450); ac++;
	XtSetArg(args[ac], XmNlistVisibleItemCount, 6); ac++;
	XtSetArg(args[ac], XmNapplyLabelString, 
		CONVERT( widget , "Apply", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNcancelLabelString, 
		CONVERT( widget , "Cancel", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	sel_box = new motif_widget( "File Selection Box" ,
		xmFileSelectionBoxWidgetClass , this ,
		args ,				ac ,
		TRUE ) ;

	sel_box->add_callback( openFSDOkCallback , XmNokCallback , this ) ;
	sel_box->add_callback( openFSDOkCallback , XmNcancelCallback , this ) ;

	// for Solaris to work properly:

	Widget scroll_w = XtParent( XmFileSelectionBoxGetChild( sel_box->widget , XmDIALOG_LIST ) ) ;
	Widget h_scroll ;

	XtVaGetValues( scroll_w , XmNhorizontalScrollBar , & h_scroll , NULL ) ;
	XtManageChild( h_scroll ) ;

	// end of Solaris addition
	}
}
#endif // ifdef GUI

void open_anyway_CB( const Cstring & toggle_button_text ) {
	if( toggle_button_text == "Open File Anyway" ) {
		OPEN_FILErequest *request;

		request = new OPEN_FILErequest(compiler_intfc::FILE_NAME, pendingInputFile);
		if (request) {
			request->process(); } }
	else
		pendingInputFile.undefine() ; }

void restore_durations_CB(const Cstring &toggle_button_text) {
	slist<alpha_void, Cntnr<alpha_void, CTime_base> >::iterator	suspects(ourPrivateList);
	Cntnr<alpha_void, CTime_base>*					ptp;
	const char*							which_button;

	if(ACT_exec::recompute_durations) {
		which_button = "Restore APF Durations"; }
	else {
		which_button = "Enforce formulas in the adaptation file(s)" ; }
	if(toggle_button_text == which_button) {
		ActivityInstance*	act;
		TypedValue		old_time(apgen::DATA_TYPE::TIME);

		while((ptp = suspects())) {
			act = (ActivityInstance*) ptp->Key.Num;
			old_time = ptp->payload;
#ifdef PREMATURE
			// check whether the symbol table has it, then invoke assign_w_side_effects
			act->Object->side_effects("span", old_time, apgen::METHOD_TYPE::METHOD_ATTRIBUTES);
#endif /* PREMATURE */
		}
	}
	ourPrivateList.clear();
}

#ifdef GUI
void UI_openfsd::openFSDOkCallback( Widget w, callback_stuff *clientData , XtPointer callData) {
	if( GUI_Flag ) {
	UI_openfsd*	obj = ( UI_openfsd * ) clientData->data ;

	obj->openFSDOk( w , clientData->data , callData ) ;
	}
}

void UI_openfsd::openFSDOk( Widget , XtPointer , XtPointer cbs ) {
	if( GUI_Flag ) {
	char*					filename ;
	XmFileSelectionBoxCallbackStruct* 	CBS = ( XmFileSelectionBoxCallbackStruct  * ) cbs ;

	//	if(CBS->reason != XmCR_APPLY)
	  unmanage() ;
	if( CBS->reason == XmCR_CANCEL ) {
		return ; }
	if( !XmStringGetLtoR( CBS->value , XmSTRING_DEFAULT_CHARSET , &filename ) ) {
		return ; }
	if( *filename == '\0' ) {
		// NOTE: should protect against signals...
		XtFree( filename ) ;
		return ; }

	// Check if file is already opened
	// 96-08-07 DSG reversed following for FR021:  user chooses to read or not:
	// PFM: we won't check. Assume the user knows what he/she's doing.
	pendingInputFile = filename ;

	// It's OK to repeatedly read a layout file; don't warn user in that case
	if(FileNamesWithInstancesAndOrNonLayoutDirectives().find(filename)) {
		List		buttons;

		buttons << new String_node( "Open File Anyway" ) ;
		buttons << new String_node( "Forget It" ) ;
		UI_messagewarndialog::initialize (
			"Open File" ,
			"Duplicate Input File" ,
			"The selected file has already been opened, or "
				"the filename is already in use.  Re-reading a "
				"file may yield duplicates of activity instances." ,
			buttons ,
			open_anyway_CB
			) ; }
	else {
		OPEN_FILErequest*	request;

		request = new OPEN_FILErequest(compiler_intfc::FILE_NAME, filename);
		request->process() ;
		pendingInputFile.undefine() ;
		if( ACT_exec::ACT_subsystem().execAgent()->act_instances_with_discrepant_durations.get_length() ) {
			pop_the_discrepancy_panel() ; } }
	XtFree ( filename ) ; }
}

#endif

void UI_openfsd::pop_the_discrepancy_panel() {
	List								buttons;
	Cntnr<alpha_void, CTime_base>*					ptp ;
	slist<alpha_void, Cntnr<alpha_void, CTime_base> >::iterator	theCopies(ourPrivateList) ;

	if(disabled) {
		ACT_exec::ACT_subsystem().execAgent()->act_instances_with_discrepant_durations.clear();
		return; }

	ourPrivateList.clear();
	ourPrivateList << ACT_exec::ACT_subsystem().execAgent()->act_instances_with_discrepant_durations;
	if(ACT_exec::recompute_durations) {
		buttons << new String_node( "Keep values computed from Duration formulas" );
		buttons << new String_node( "Restore APF Durations" );
		UI_messagewarndialog::initialize (
			"Discrepancies Found" ,
			"Discrepancies in Activity Duration(s)" ,
			"The selected file contains one or more act. instance(s) whose duration "
				"is specified by a formula in their type definition. "
				"The durations have been modified to reflect the type definition(s); "
				"since you selected the -recompute-durations command-line option, "
				"you probably want to keep things the way they are now (first option below).\n\n"
				"Select the second option below only if you want to let the duration values "
				"in the APF override the type definitions." ,
			buttons ,
			restore_durations_CB
			); }
	else { // the default
		buttons << new String_node( "Keep Durations specified in the APF" );
		buttons << new String_node( "Enforce formulas in the adaptation file(s)" );
		UI_messagewarndialog::initialize (
			"Discrepancies Found",
			"Discrepancies in Activity Duration(s)",
			"The selected file contains one or more act. instance(s) whose duration "
				"is specified by a formula in their type definition. "
				"The durations have not been modified to reflect the type definition(s); "
				"you probably want to keep things the way they are now (first option below).\n\n"
				"Select the second option below only if you want to enforce the duration formulas "
				"specified in the adaptation.",
			buttons,
			restore_durations_CB
			); }
	}

#define SET_BACKGROUND_TO_GREY  XtSetArg(args[ac], XmNbackground, ResourceData.grey_color ) ; ac++;

UI_openfsd		*openFSD = NULL ;

UI_openfsd::~UI_openfsd ()
	{}

void UI_openfsd::initialize () {
	if( !openFSD )
		openFSD = new UI_openfsd( "File Open Dialog" ) ;
	openFSD->mainWindow = NULL ;
	openFSD->setBackgroundColors() ; }

void UI_openfsd::setBackgroundColors () {
#ifdef GUI
	if( GUI_Flag ) {
	Widget	w,parent;

	// DON'T CHANGE COLOR HERE; JUST CHANGE THE BACKGROUND:
	w = XmFileSelectionBoxGetChild ( sel_box->widget , XmDIALOG_DIR_LIST ) ;
	XtVaSetValues( w , XmNbackground , ResourceData.grey_color , NULL ) ;
	parent = XtParent(w);	// get the form parent of this FSD area
	XtVaGetValues (parent,XmNhorizontalScrollBar,&w,NULL);
	XmChangeColor (w, ResourceData.grey_color );
	XtVaGetValues (parent,XmNverticalScrollBar,&w,NULL);
	XmChangeColor (w, ResourceData.grey_color );

	w = XmFileSelectionBoxGetChild ( sel_box->widget ,XmDIALOG_FILTER_TEXT);
	XtVaSetValues( w , XmNbackground , ResourceData.grey_color , NULL ) ;

	w = XmFileSelectionBoxGetChild ( sel_box->widget ,XmDIALOG_LIST);
	XtVaSetValues( w , XmNbackground , ResourceData.grey_color , NULL ) ;
	XtVaSetValues( w , XmNscrollBarDisplayPolicy , XmSTATIC , NULL ) ;
	parent = XtParent(w);	// get the form parent of this FSD area
	XtVaGetValues (parent,XmNhorizontalScrollBar,&w,NULL);
	XmChangeColor (w, ResourceData.grey_color );
	XtVaGetValues (parent,XmNverticalScrollBar,&w,NULL);
	XmChangeColor (w, ResourceData.grey_color );

	w = XmFileSelectionBoxGetChild ( sel_box->widget ,XmDIALOG_TEXT);
	XtVaSetValues( w , XmNbackground , ResourceData.grey_color , NULL ) ;
	}
#endif
}
