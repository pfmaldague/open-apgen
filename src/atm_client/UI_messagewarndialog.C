#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "ACT_exec.H"
#include <ActivityInstance.H>
#include "UI_exec.H"
#include "UI_GeneralMotifInclude.H"
#include "UI_messagewarndialog.H"
#include "action_request.H"
#include "Prefs.H"
	                // in UI_motif_widget.C:
extern resource_data    ResourceData ;

			// in main.C:
extern motif_widget*	MW ;

#ifdef GUI
extern XEvent*		get_event() ;
extern XtAppContext	Context ;
#endif

			// in UI_ds_timeline.C:

extern "C" {
	extern void disable_signals( int k ) ;
	extern void enable_signals() ; }

UI_messagewarndialog	*message_warning_dialog = NULL ;
UI_preferences		*preferences_dialog = NULL ;

#define SET_BACKGROUND_TO_GREY  XtSetArg(args[ac], XmNbackground, ResourceData.grey_color ) ; ac++;

void UI_messagewarndialog::enable_ok() {
	if(message_warning_dialog) {
		message_warning_dialog->msgWarnOKButton->set_sensitive(TRUE); } }

void UI_messagewarndialog::disable_ok() {
	if(message_warning_dialog) {
		message_warning_dialog->msgWarnOKButton->set_sensitive(FALSE); } }

UI_messagewarndialog::UI_messagewarndialog(const List & options, bool include_cancel)
	: motif_widget("Warning", xmDialogShellWidgetClass, MW, NULL, 0, FALSE),
	FunctionToCall(NULL) ,
	msgWarnCancelButton(NULL) {
#ifdef GUI
	if( GUI_Flag ) {
	Arg      	args[56];
	Cardinal 	ac=0;
	Boolean  	argok=False;
	motif_widget	*button , *frame , *label , *scrolled_win , *form ;
	List_iterator	opts( options ) ;
	String_node	*N ;

	messageWarnDialog = new motif_widget(
		"Message Warning Paned Window" ,
		xmPanedWindowWidgetClass ,	this ,
		args ,				ac ,
		FALSE ) ;

	XtSetArg( args[ac] , XmNheight ,	600 ) ;			ac++ ;
	XtSetValues( widget , args , ac ) ;

	messageWarnForm = new motif_widget( "Message Warning Form" ,
		messageWarnDialog , form_position( 5 ) , TRUE ) ;

	ac = 0;
	XtSetArg(args[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
	XtSetArg(args[ac], XmNeditable, False); ac++;
	XtSetArg(args[ac], XmNwordWrap, True); ac++;
	XtSetArg(args[ac], XmNscrollHorizontal, False); ac++;
	SET_BACKGROUND_TO_GREY ;
	msgWarnText = new motif_widget(
		"msgWarnText",
		xmScrolledTextWidgetClass ,	messageWarnForm ,
		args ,				ac ,
		TRUE ) ;

	scrolled_win = new motif_widget( msgWarnText , XtParent( msgWarnText->widget ) , xmScrolledWindowWidgetClass ) ;

	ac = 0 ;
	XtSetArg(args[ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	msgWarnLabel = new motif_widget(
		"msgWarnLabel",
		xmLabelWidgetClass ,	messageWarnForm,
		args, ac ,
		TRUE ) ;
	*msgWarnLabel = "Message:" ;
	
	ac = 0;
	XtSetArg( args[ac] , XmNwidth ,		400 ) ;			ac++ ;
	XtSetArg( args[ac] , XmNheight ,	100 ) ;			ac++ ;
	XtSetValues( XtParent( msgWarnText->widget ) , args , ac ) ;

	*messageWarnForm ^ 10 ^ *msgWarnLabel ^ *scrolled_win ^ *messageWarnForm ^ endform ;
	*messageWarnForm < 10 < *msgWarnLabel < 10 < *messageWarnForm < endform ;
	*messageWarnForm < 10 < *scrolled_win < 10 < *messageWarnForm < endform ;

	delete scrolled_win ;
	scrolled_win = NULL ;

	form = new motif_widget( "form" , messageWarnDialog , form_position( 40 ) ) ;
	frame = new motif_widget( "frame" ,
		xmFrameWidgetClass ,	form ,
		NULL ,			0 ,
		TRUE ) ;
	label = new motif_widget( "These are your options:" ,
		xmLabelWidgetClass ,	form ) ;

	*form ^ 10 ^ *label ^ endform ;
	*form ^ 5 ^ *frame ^ 5 ^ *form ^ endform ;
	*form < 10 < *label < 10 < form_position(12) < 5 < *frame < 5 < *form < endform ;

	ac = 0;
	XtSetArg( args[ac] , XmNheight , 100 ) ; ac++ ;
	// added this in the hope of seeing Linux behave better: (didn't help)
	XtSetArg( args[ac] , XmNradioBehavior , True ) ; ac++ ;
	msgWarnOptionsBox = new motif_widget(
		"msgWarnOptionsBox",
		xmRadioBoxWidgetClass , frame ,
		args ,			ac ,
		TRUE ) ;

	while((N = (String_node*) opts())) {
		button = new motif_widget( N->get_key() ,
			xmToggleButtonWidgetClass ,	msgWarnOptionsBox ,
			NULL ,				0 ,
			TRUE ) ;
		if( N == ( String_node * ) options.first_node() ) {
			*button = "1" ; } }
	int num_positions;
	if(include_cancel) {
		num_positions = 7; }
	else {
		num_positions = 5; }
	msgWarnButtonForm = new motif_widget(
			"msgWarnButtonForm",
			messageWarnDialog,
			form_position(num_positions),
			TRUE);
	
	ac = 0 ;
	msgWarnOKButton = new motif_widget(
		"msgWarnOKButton",
		xmPushButtonWidgetClass,	msgWarnButtonForm,
		args,				ac,
		TRUE);
	*msgWarnOKButton = "OK";
	msgWarnOKButton->add_callback(
		UI_messagewarndialog::msgWarnButtonActivateCallback,
		XmNactivateCallback,
		this);
	if(include_cancel) {
		ac = 0 ;
		msgWarnCancelButton = new motif_widget(
			"msgWarnCancelButton",
			xmPushButtonWidgetClass,	msgWarnButtonForm,
			args,				ac,
			TRUE);
		*msgWarnCancelButton = "Cancel";
		msgWarnCancelButton->add_callback(
			UI_messagewarndialog::msgWarnButtonActivateCallback,
			XmNactivateCallback,
			this);
		form_position(4) < *msgWarnCancelButton < form_position(6) < endform;
		form_position(1) < *msgWarnOKButton < form_position(3) < endform;
		*msgWarnButtonForm ^ *msgWarnCancelButton ^ *msgWarnButtonForm ^ endform; }
	else {
		form_position(2) < *msgWarnOKButton < form_position(3) < endform; }
	*msgWarnButtonForm ^ *msgWarnOKButton ^ *msgWarnButtonForm ^ endform;
#ifdef HAVE_MACOS
	msgWarnButtonForm->fix_height(24, msgWarnOKButton->widget);
#else
	msgWarnButtonForm->fix_height(msgWarnOKButton->widget);
#endif /* HAVE_MACOS */
	messageWarnDialog->manage();
	}
#endif /* GUI */
}

UI_messagewarndialog::~UI_messagewarndialog() {
	destroy_widgets(); }

void UI_messagewarndialog::initialize(
		const Cstring&	windowTitle,
		const Cstring&	title,
		const Cstring&	message,
		const List&	list_of_options,
		void (*func_to_call) (const Cstring&),
		bool include_cancel_button,
		void (*cancel_func) (void)) {
#ifdef GUI
	if( GUI_Flag ) {
	Widget		shell ;
	int		i = 0 ;

	// Changed behavior: ALWAYS construct a new widget. A bug in the Linux Motif library prevents
	// correct operation of the radivoidbox.
	if( message_warning_dialog )
		delete message_warning_dialog ;
	message_warning_dialog = new UI_messagewarndialog(list_of_options, include_cancel_button) ;

	*message_warning_dialog->msgWarnText = message ;
	*message_warning_dialog->msgWarnLabel = title ;
	shell = message_warning_dialog->widget ;
	XtVaSetValues ( shell ,
		XmNtitle , ( char * ) * windowTitle ,
		NULL ) ;

	if( list_of_options.get_length() ) {
		String_node	*N ;
		List_iterator	opts( list_of_options ) ;
		motif_widget	*button ;

		while((N = (String_node *) opts())) {
			button = (motif_widget *) message_warning_dialog->msgWarnOptionsBox->children[i];
			button->set_label(N->get_key());
			button->manage();
			i++; }
		for(	;
			i < message_warning_dialog->msgWarnOptionsBox->children.get_length() ;
			i++ ) {
			( ( motif_widget * ) message_warning_dialog->msgWarnOptionsBox->children[i] )->unmanage() ; }
		message_warning_dialog->FunctionToCall = func_to_call ;
		if((button = (motif_widget*) message_warning_dialog->msgWarnOptionsBox->children.first_node())) {
			// select first option
			*button = "1" ; } }
	else {
		for(	;
			i < message_warning_dialog->msgWarnOptionsBox->children.get_length() ;
			i++ )
			( ( motif_widget * ) message_warning_dialog->msgWarnOptionsBox->children[i] )->unmanage() ;
		message_warning_dialog->FunctionToCall = NULL ; }
	if(include_cancel_button) {
		message_warning_dialog->CancelAction = cancel_func; }
	else {
		message_warning_dialog->CancelAction = NULL; }
	message_warning_dialog->messageWarnDialog->manage() ;
	}
#endif
}


void UI_messagewarndialog::msgWarnButtonActivateCallback( Widget , callback_stuff * clientData , void * ) {
	UI_messagewarndialog	*obj = (UI_messagewarndialog *) clientData->data ;
	motif_widget		*theButton = clientData->parent;
	motif_widget		*N;
	List_iterator		buttons(obj->msgWarnOptionsBox->children);

	obj->messageWarnDialog->unmanage();
	if(theButton == obj->msgWarnCancelButton) {
		if(obj->CancelAction) {
			obj->CancelAction(); }
		return; }
	while((N = (motif_widget*) buttons()))
		if( N->get_text() == "SET" )
			break;

	if( N ) {
		obj->FunctionToCall( N->get_label() ) ; } }

UI_preferences::UI_preferences()
	: motif_widget( "Preferences" , xmDialogShellWidgetClass , MW , NULL , 0 , FALSE ) {
#ifdef GUI
	if( GUI_Flag ) {
	Arg      	args[56];
	Cardinal 	ac=0;
	Boolean  	argok=False;
	motif_widget	*frame1 , *frame2 , *frame3 , *label , *form1, *form2, *form3 ;
	motif_widget	*restriction_box ;

	paned_window = new motif_widget(
		"Drag options paned window" ,
		xmPanedWindowWidgetClass ,	this ,
		args ,				ac ,
		FALSE ) ;

	// XtSetArg( args[ac] , XmNheight ,	600 ) ;		ac++ ;
	// XtSetValues( widget , args , ac ) ;

	form1 = new motif_widget( "Drag Options Form" ,
		paned_window , form_position( 5 ) , TRUE ) ;

	ac = 0 ;
	XtSetArg(args[ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	label = new motif_widget(
		"Label",
		xmLabelWidgetClass ,	form1 ,
		args,			ac ,
		TRUE ) ;
	*label = "The Following Settings Apply When Dragging Activities:" ;
	*form1 < *label < *form1 < endform ;
	*form1 ^ 5 ^ *label ^ endform ;

	// form1 = new motif_widget( "Drag Options Form 2" , paned_window , form_position( 5 ) , TRUE ) ;
	frame1 = new motif_widget( "Frame" ,
		xmFrameWidgetClass ,		form1 ,
		NULL ,				0 ,
		TRUE ) ;
	*form1 < *frame1 < endform ;
	*label ^ 5 ^ *frame1 ^ 5 ^ *form1 ^ endform ;
	drag_children_box = new motif_widget( "radio_box" ,
		xmRadioBoxWidgetClass ,	frame1 ,
		NULL ,			0 ,
		TRUE ) ;
	together_button = new motif_widget( "Drag Children Along" ,
		xmToggleButtonWidgetClass ,		drag_children_box ,
		NULL ,					0 ,
		TRUE ) ;

	separate_button = new motif_widget( "Leave Children Alone" ,
		xmToggleButtonWidgetClass ,		drag_children_box ,
		NULL ,					0 ,
		TRUE ) ;

	if(Preferences().GetSetPreference("DragChildren", "TRUE") == "TRUE")
	  *together_button = "1";
	else
	  *separate_button = "1";

	frame2 = new motif_widget( "Frame" ,
		xmFrameWidgetClass ,		form1 ,
		NULL ,				0 ,
		TRUE ) ;
	*frame1 < 5 < *frame2 < endform ;
	*label ^ 5 ^ *frame2 ^ 5 ^ *form1 ^ endform ;
	restriction_box = new motif_widget( "radio_box" ,
		xmRadioBoxWidgetClass ,		frame2 ,
		NULL ,				0 ,
		TRUE ) ;
	horizontal_button = new motif_widget( "Horizontal Motion Only" ,
		xmToggleButtonWidgetClass ,	restriction_box ,
		NULL ,				0 ,
		TRUE ) ;
	vertical_button = new motif_widget( "Vertical Motion Only" ,
		xmToggleButtonWidgetClass ,	restriction_box ,
		NULL ,				0 ,
		TRUE ) ;
	unrestricted_button = new motif_widget( "Unrestricted Motion" ,
		xmToggleButtonWidgetClass ,	restriction_box ,
		NULL ,				0 ,
		TRUE ) ;

	string horizontal = Preferences().GetSetPreference("HorizontalDragEnabled", "TRUE");
	string vertical = Preferences().GetSetPreference("VerticalDragEnabled", "TRUE");

	//this should take care of things
	if(horizontal == "TRUE")
	{
	  if(vertical == "TRUE")
	    *unrestricted_button = "1";
	  else
	    *horizontal_button = "1";
	}
	else if(vertical == "TRUE")
	  *vertical_button = "1";

	frame3 = new motif_widget( "Frame" ,
		xmFrameWidgetClass ,	form1 ,
		NULL ,			0 ,
		TRUE ) ;
	*frame2 < 5 < *frame3 < 5 < *form1 < endform ;
	*label ^ 5 ^ *frame3 ^ 5 ^ *form1 ^ endform ;
	remodeling_box = new motif_widget( "radio_box" ,
		xmRadioBoxWidgetClass ,	frame3 ,
		NULL ,			0 ,
		TRUE ) ;

	string remodel = Preferences().GetSetPreference("RemodelAfterDrag", "FALSE");
	no_remodeling_button = new motif_widget( "Do Not Remodel" ,
		xmToggleButtonWidgetClass ,	remodeling_box ,
		NULL ,				0 ,
		TRUE ) ;
	remodeling_button = new motif_widget( "Remodel After Move" ,
		xmToggleButtonWidgetClass ,	remodeling_box ,
		NULL ,				0 ,
		TRUE ) ;

	if(remodel == "TRUE")
	  *remodeling_button = "1" ;
	else
	  *no_remodeling_button = "1";

	form2 = new motif_widget("Other Preferences" ,
				 paned_window, form_position(11), TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNalignment, XmALIGNMENT_CENTER); ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;

	motif_widget* otherLabel = new motif_widget("Other Label",
						    xmLabelWidgetClass, form2,
						    args, ac, TRUE);
	*otherLabel = "Other preferences:";

	*form2 < *otherLabel < *form2 < endform;
	*form2 ^ 5 ^ *otherLabel ^ endform;

	motif_widget* frame4 = new motif_widget("Other frame",
						xmFrameWidgetClass, form2, NULL, 0, TRUE);

	*form2 < *frame4 < *form2 < endform;
	*otherLabel ^ 5 ^ *frame4 ^ 5 ^ *form2 ^ endform;


	ac = 0;
	XtSetArg(args[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;

	snap_to_grid = new motif_widget("Snap to Grid", 
				  xmToggleButtonWidgetClass, frame4,
				  args, ac, TRUE);
	if(Preferences().GetSetPreference("SnapToGrid", "TRUE") == "TRUE") {
	  // debug
	  // cout << "preferences popup: snap-to-grid was found to be true\n";
	  *snap_to_grid = "1"; }
	// debug
	// else {
	//   cout << "preferences popup: snap-to-grid was found to be true\n"; }

	form3 = new motif_widget( "Preference Actions" ,
		paned_window , form_position( 11 ) , TRUE ) ;
	OKButton = new motif_widget( "OK" ,
		xmPushButtonWidgetClass ,	form3 ,
		NULL ,				0 ,
		TRUE ) ;
	form_position( 2 ) < *OKButton < form_position( 3 ) < endform ;
	*form3 ^ *OKButton ^ *form3 ^ endform ;
	ApplyButton = new motif_widget( "Apply" ,
		xmPushButtonWidgetClass ,	form3 ,
		NULL ,				0 ,
		TRUE ) ;
	form_position( 5 ) < *ApplyButton < form_position( 6 ) < endform ;
	*form3 ^ *ApplyButton ^ *form3 ^ endform ;
	CancelButton = new motif_widget( "Cancel" ,
		xmPushButtonWidgetClass ,	form3 ,
		NULL ,				0 ,
		TRUE ) ;
	form_position( 8 ) < *CancelButton < form_position( 9 ) < endform ;
	*form3 ^ *CancelButton ^ *form3 ^ endform ;
#ifdef HAVE_MACOS
	form3->fix_height(24, CancelButton->widget ) ;
#else
	form3->fix_height( CancelButton->widget ) ;
#endif /* HAVE_MACOS */
	CancelButton->add_callback( DoneButtonCallback , XmNactivateCallback , this ) ;
	ApplyButton->add_callback( DoneButtonCallback , XmNactivateCallback , this ) ;
	OKButton->add_callback( DoneButtonCallback , XmNactivateCallback , this ) ;
	
	}
#endif /* GUI */
}

void UI_preferences::DoneButtonCallback( Widget , callback_stuff * client_data , void * ) {
	UI_preferences		*obj = ( UI_preferences * ) client_data->data ;

	if(	client_data->parent == obj->OKButton ||
		client_data->parent == obj->ApplyButton ) {
		DRAG_CHILDRENrequest	*request ;

		if(preferences_dialog->remodeling_button->get_text() == "SET")
		  Preferences().SetPreference("RemodelAfterDrag", "TRUE");
		else
		  Preferences().SetPreference("RemodelAfterDrag", "FALSE");

		if(( preferences_dialog->horizontal_button->get_text() == "SET" ) ||
		   ( preferences_dialog->unrestricted_button->get_text() == "SET" ))
		  Preferences().SetPreference("HorizontalDragEnabled", "TRUE");
		else
		  Preferences().SetPreference("HorizontalDragEnabled", "FALSE");

		if(( preferences_dialog->vertical_button->get_text() == "SET" ) ||
		   ( preferences_dialog->unrestricted_button->get_text() == "SET" ))
		  Preferences().SetPreference("VerticalDragEnabled", "TRUE");
		else
		  Preferences().SetPreference("VerticalDragEnabled", "FALSE");

		if(preferences_dialog->snap_to_grid->get_text() == "SET")
		  Preferences().SetPreference("SnapToGrid", "TRUE");
		else
		  Preferences().SetPreference("SnapToGrid", "FALSE");

		bool dragChildren = (Preferences().GetSetPreference("DragChildren", "TRUE") == "TRUE");

		bool should_drag = (preferences_dialog->together_button->get_text() == "SET" );
		if( should_drag)
		  Preferences().SetPreference("DragChildren", "TRUE");
		else
		  Preferences().SetPreference("DragChildren", "FALSE");

		if( dragChildren != should_drag ) 
		{
		  request = new DRAG_CHILDRENrequest( should_drag ) ;
		  request->process(); 
		  delete request;
		}

		Preferences().SavePreferences();
	}
	if( client_data->parent != obj->ApplyButton )
		preferences_dialog->paned_window->unmanage() ; }

void UI_preferences::initialize() {
	if( !preferences_dialog ) preferences_dialog = new UI_preferences() ;

	preferences_dialog->paned_window->manage() ; }
