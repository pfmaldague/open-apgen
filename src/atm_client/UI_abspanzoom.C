#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "action_request.H"
#include <ActivityInstance.H>
#include "UI_abspanzoom.H"
#include "UI_mainwindow.H"
#include "UTL_time.H"
#include "UI_GeneralMotifInclude.H"

// GLOBALS:
CTime			real_time_update_interval(1, 0, true);
CTime			modeled_time_update_interval(3600, 0, true);
UI_abspanzoom*		absPanZoomDialog = NULL;

// EXTERNS:
			// in main.C:
extern int		GUI_Flag;
extern	motif_widget*	MW;
	                // in UI_motif_widget.C:
extern resource_data    ResourceData;


UI_abspanzoom::UI_abspanzoom(const char * /* name */ )
	: motif_widget( "Absolute Pan/Zoom" ,
		xmDialogShellWidgetClass ,	MW ,
		NULL ,				0 ,
		// causes the window to sometimes appear as a tiny box:
		// TRUE )
		FALSE ) {
	create(); }

UI_abspanzoom::~UI_abspanzoom()
	{; }

void UI_abspanzoom::initialize ( const Cstring & startTime, const Cstring & duration ) {
	if( !absPanZoomDialog ) {
		absPanZoomDialog = new UI_abspanzoom( "AbsolutePanZoomDialog" );
		absPanZoomDialog->mainWindow = UI_mainwindow::mainWindowObject; }
	*absPanZoomDialog->_apzStartTimeTF = startTime;
	*absPanZoomDialog->_apzTimeSpanTF = duration; }

void UI_abspanzoom::adTimeSystemChange (Widget, callback_stuff * , void * )
	{; }

void UI_abspanzoom::apzApplyButtonActivate (Widget, callback_stuff * , void * ) {
	applyChanges(); }

void UI_abspanzoom::apzCancelButtonActivate (Widget, callback_stuff * , void * ) {
	_absPanZoomDialog->unmanage(); }

void UI_abspanzoom::apzOKButtonActivate (Widget, callback_stuff * , void * ) {
	if (applyChanges ()) {
	    _absPanZoomDialog->unmanage(); } }

int UI_abspanzoom::applyChanges () {
	NEW_HORIZONrequest	*request;
	Cstring			startTimeString(_apzStartTimeTF->get_text());
	Cstring			durationString(_apzTimeSpanTF->get_text());
	Cstring			realString(_rtuiTF->get_text());
	Cstring			modeledString(_mtuiTF->get_text());
	Cstring			theAD;
	Node			*N;
	List			theChosenADs;
	List_iterator		acs(theChosenADs);

	UI_mainwindow::findAllSelectedADs(theChosenADs);
	while((N = acs())) {
		request = new NEW_HORIZONrequest( N->get_key() ,
				startTimeString , durationString , realString , modeledString );
		if( request->process() != apgen::RETURN_STATUS::SUCCESS ) {
			return 0; } }
	return 1; }

void UI_abspanzoom::create() {
#ifdef GUI
	if( GUI_Flag ) {
	Arg      	args[19];
	Cardinal	count;
	Boolean  	argok;
	motif_widget	*_w;
	motif_widget	*form;

	count = 0;
	argok = False;

	_w = _absPanZoomDialog = new motif_widget( get_key() ,
		// xmFormWidgetClass, this,
		xmPanedWindowWidgetClass, this,
		NULL ,		0 ,
		FALSE );

	form = new motif_widget( "form" ,
		xmFormWidgetClass,      _w,
                args,                   count ,
                TRUE );

	count = 0;
	XtSetArg ( args[count], XmNfractionBase, 60 ); count++;
	_apzButtonForm = new motif_widget  ( "apzButtonForm",
	        xmFormWidgetClass,	_w, 
	        args,			count ,
		TRUE );

	count = 0;
	_apzOKButton = new motif_widget  ( "OK",
		xmPushButtonWidgetClass, _apzButtonForm, 
		args,			count ,
		TRUE );

	_apzOKButton->add_callback( apzOKButtonActivateCallback,
	                XmNactivateCallback,
			this );

	count = 0;
	_apzApplyButton = new motif_widget  ( "Apply",
		xmPushButtonWidgetClass,	_apzButtonForm, 
		args,				count ,
		TRUE );

	_apzApplyButton->add_callback( apzApplyButtonActivateCallback ,
	                XmNactivateCallback,
			this );

	count = 0;
	_apzCancelButton = new motif_widget  ( "Cancel",
		xmPushButtonWidgetClass, _apzButtonForm, 
		args, count ,
		TRUE );

	_apzCancelButton->add_callback( apzCancelButtonActivateCallback ,
	                XmNactivateCallback,
			this );

	form_position( 40 ) < *_apzCancelButton < form_position( 59 ) < endform;
	form_position( 20 ) < *_apzApplyButton < form_position( 39 ) < endform;
	form_position( 0 ) < *_apzOKButton < form_position( 19 ) < endform;
	*_apzButtonForm ^ *_apzCancelButton ^ *_apzButtonForm ^ endform;
	*_apzButtonForm ^ *_apzApplyButton ^ *_apzButtonForm ^ endform;
	*_apzButtonForm ^ *_apzOKButton ^ *_apzButtonForm ^ endform;

#ifdef HAVE_MACOS
	_apzButtonForm->fix_height(40, _apzOKButton->widget);
#else
	_apzButtonForm->fix_height( _apzOKButton->widget );
#endif /* HAVE_MACOS */
	count = 0;
	_apzTimeZonePM = new motif_widget(
	        "apzTimeZonePM",
		xmPulldownMenuWidgetClass , form  ,
		args, count ,
		TRUE );

	count = 0;
	_apzUtcButton = new motif_widget  ( "UTC",
		xmPushButtonWidgetClass, _apzTimeZonePM, 
		args, count ,
		TRUE );

	_apzUtcButton->add_callback(
	                adTimeSystemChangeCallback,
	                XmNactivateCallback,
	                ( void * ) 1 );
	count = 0;
	XtSetArg ( args[count], XmNsensitive, False ); count++;
	_apzPstButton = new motif_widget  ( "PST",
	                                    xmPushButtonWidgetClass,
	                                    _apzTimeZonePM, 
	                                    args, 
	                                    count  ,
		TRUE);
	_apzPstButton->add_callback(
                        adTimeSystemChangeCallback,
                        XmNactivateCallback,
                        ( void * ) 2 );

	count = 0;
	XtSetArg ( args[count], XmNsensitive, False ); count++;
	_apzPdtButton = new motif_widget  ( "PDT",
	                                    xmPushButtonWidgetClass,
	                                    _apzTimeZonePM, 
	                                    args, 
	                                    count  ,
		TRUE);
	_apzPdtButton->add_callback(
	                adTimeSystemChangeCallback,
	                XmNactivateCallback,
	                ( void * ) 3 );

	count = 0;
	XtSetArg ( args[count], XmNsensitive, False ); count++;
	_apzMstButton = new motif_widget  ( "MST",
	                                    xmPushButtonWidgetClass,
	                                    _apzTimeZonePM, 
	                                    args, 
	                                    count  ,
		TRUE);
	_apzMstButton->add_callback(
	                adTimeSystemChangeCallback,
	                XmNactivateCallback,
	                ( void * ) 4 );

	count = 0;
	XtSetArg ( args[count], XmNsensitive, False ); count++;
	_apzMdtButton = new motif_widget  ( "MDT",
	                                    xmPushButtonWidgetClass,
	                                    _apzTimeZonePM, 
	                                    args, 
	                                    count  ,
		TRUE);
	_apzMdtButton->add_callback(
	                adTimeSystemChangeCallback,
	                XmNactivateCallback,
	                ( void * ) 5 );

	count = 0;
	XtSetArg ( args[count], XmNsensitive, False ); count++;
	_apzCstButton = new motif_widget  ( "CST",
	                                    xmPushButtonWidgetClass,
	                                    _apzTimeZonePM, 
	                                    args, 
	                                    count  ,
		TRUE);
	_apzCstButton->add_callback(
	                adTimeSystemChangeCallback,
	                XmNactivateCallback,
	                ( void * ) 6 );

	count = 0;
	XtSetArg ( args[count], XmNsensitive, False ); count++;
	_apzCdtButton = new motif_widget  ( "CDT",
	                                    xmPushButtonWidgetClass,
	                                    _apzTimeZonePM, 
	                                    args, 
	                                    count  ,
		TRUE);
	_apzCdtButton->add_callback(
	                adTimeSystemChangeCallback,
	                XmNactivateCallback,
	                ( void * ) 7 );
	count = 0;
	XtSetArg ( args[count], XmNsensitive, False ); count++;
	_apzEstButton = new motif_widget  ( "EST",
	                                    xmPushButtonWidgetClass,
	                                    _apzTimeZonePM, 
	                                    args, 
	                                    count  ,
		TRUE);
	_apzEstButton->add_callback(
	                adTimeSystemChangeCallback,
	                XmNactivateCallback,
	                ( void * ) 7 );

	count = 0;
	XtSetArg ( args[count], XmNsensitive, False ); count++;
	_apzEdtButton = new motif_widget  ( "EDT",
		xmPushButtonWidgetClass,	_apzTimeZonePM, 
		args, count ,
		TRUE );
	_apzEstButton->add_callback(
	                adTimeSystemChangeCallback,
	                XmNactivateCallback,
	                ( void * ) 8 );

	count = 0;
	XtSetArg ( args[count], XmNalignment, XmALIGNMENT_BEGINNING ); count++;
	_apzStartTimeLabel = new motif_widget  ( "Starting Time:",
		xmLabelWidgetClass, form , 
		args, count ,
		TRUE );

	count = 0;
	XtSetArg ( args[count], XmNborderWidth, 0 ); count++;
	XtSetArg ( args[count], XmNnavigationType, XmTAB_GROUP ); count++;
	XtSetArg ( args[count], XmNsubMenuId, _apzTimeZonePM->widget ); count++;
	_apzTimeZoneOM = new motif_widget( "apzTimeZoneOM" ,
		xmOptionMenuWidgetClass ,	form , 
		args, count ,
		TRUE );

	_apzStartTimeTF = new single_line_text_widget( "pzStartTimeTF" , form , Cstring( EDITABLE ) , TRUE );

	count = 0;
	XtSetArg ( args[count], XmNalignment, XmALIGNMENT_BEGINNING ); count++;
	_apzTimeSpanLabel = new motif_widget  ( "Time Span:",
			xmLabelWidgetClass,	form , 
			args,			count ,
			TRUE );
	_apzTimeSpanTF = new single_line_text_widget( "apzTimeSpanTF" , form , Cstring( EDITABLE ) , TRUE );
	*_apzTimeSpanTF = "001T00:00:00";

	count = 0;
	XtSetArg ( args[count], XmNalignment, XmALIGNMENT_BEGINNING ); count++;
	_rtuiLabel = new motif_widget  ( "Real Time Update Interval:",
			xmLabelWidgetClass,	form , 
			args,			count ,
			TRUE );
	_rtuiTF = new single_line_text_widget( "_rtuiTF" , form , Cstring( EDITABLE ) , TRUE );
	*_rtuiTF = "000T00:00:01";

	count = 0;
	XtSetArg ( args[count], XmNalignment, XmALIGNMENT_BEGINNING ); count++;
	_mtuiLabel = new motif_widget  ( "Modeled Time Update Interval:",
			xmLabelWidgetClass,	form , 
			args,			count ,
			TRUE );
	_mtuiTF = new single_line_text_widget( "_mtuiTF" , form , Cstring( EDITABLE ) , TRUE );
	*_mtuiTF = "000T01:00:00";

	*form < 10 < *_apzStartTimeLabel			< *form  < endform;
	*form < 10 < *_apzStartTimeTF	< 10 < *_apzTimeZoneOM < 10 < *form  < endform;
	*form < 10 < *_apzTimeSpanLabel	< *form  < endform;
	*form < 10 < *_apzTimeSpanTF	< 10 < *form < endform;
	*form < 10 < *_rtuiLabel	< *form  < endform;
	*form < 10 < *_rtuiTF		< 10 < *form  < endform;
	*form < 10 < *_mtuiLabel	< *form  < endform;
	*form < 10 < *_mtuiTF		< 10 < *form  < endform;
	*form ^ *_apzStartTimeLabel ^ *_apzStartTimeTF ^ *_apzTimeSpanLabel ^ *_apzTimeSpanTF ^
		*_rtuiLabel ^ *_rtuiTF ^ *_mtuiLabel ^ *_mtuiTF ^ *form ^ endform;
	*_apzStartTimeLabel ^ *_apzTimeZoneOM ^ *_apzTimeSpanLabel ^ endform;
	}
#endif
	}

void UI_abspanzoom::adTimeSystemChangeCallback (Widget w,  callback_stuff *  clientData,  void *  callData) { 
	UI_abspanzoom* obj = (UI_abspanzoom *) clientData->data;

	obj->adTimeSystemChange(w, clientData , callData); }

void UI_abspanzoom::apzApplyButtonActivateCallback (Widget w,  callback_stuff *  clientData,  void *  callData) { 
	UI_abspanzoom* obj = (UI_abspanzoom *) clientData->data;

	obj->apzApplyButtonActivate(w, clientData , callData); }

void UI_abspanzoom::apzCancelButtonActivateCallback (Widget w,  callback_stuff *  clientData,  void *  callData) { 
	UI_abspanzoom* obj = (UI_abspanzoom *) clientData->data;

	obj->apzCancelButtonActivate(w, clientData , callData); }

void UI_abspanzoom::apzOKButtonActivateCallback (Widget w,  callback_stuff *  clientData,  void *  callData) { 
	UI_abspanzoom* obj = (UI_abspanzoom *) clientData->data;

	obj->apzOKButtonActivate(w, clientData , callData); }
