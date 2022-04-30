#if HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef GUI
#	include <X11/StringDefs.h>
#	include <Xm/Form.h>
#	include <Xm/PushB.h>
#	include <Xm/Separator.h>
#	include <Xm/TextF.h>
#	include <Xm/Label.h>
#endif

#include "C_list.H"
#include "UI_resabsscrollzoombx.H"
#include "UI_mainwindow.H"

//
// Common constant and pixmap declarations.
//
#include "UI_GeneralMotifInclude.H"

                        // in main.C:
extern int		GUI_Flag ;

                        // in UI_motif_widget.C:
extern resource_data    ResourceData ;

#define SET_BACKGROUND_TO_GREY  XtSetArg(args[ac], XmNbackground, ResourceData.grey_color ) ; ac++;



//
// Convenience functions from utilities file.
//
#ifdef GUI
extern XtPointer CONVERT(Widget, const char *, const char *, int, Boolean *);
extern void RegisterBxConverters(XtAppContext);
extern XtPointer DOUBLE(double);
extern XtPointer SINGLE(float);
extern void MENU_POST(Widget, XtPointer, XEvent *, Boolean *);
extern Pixmap XPM_PIXMAP(Widget, char**);
extern void SET_BACKGROUND_COLOR(Widget, ArgList, Cardinal*, Pixel);
#endif

UI_resabsscrollzoombx::UI_resabsscrollzoombx(const char *name, Widget parent) : 
    UIComponent(name) {
    
    // Begin user code block <alt_constructor> 
    // End user code block <alt_constructor> 
    create(parent); }

//
// Class constructor.
//
UI_resabsscrollzoombx::UI_resabsscrollzoombx(const char *name) : UIComponent(name) {
    
    // Begin user code block <constructor> 
    // End user code block <constructor> 
}

//
// Minimal Destructor. Base class destroys widgets.
//
UI_resabsscrollzoombx::~UI_resabsscrollzoombx() {
    
    // Begin user code block <destructor> 
    // End user code block <destructor> 
    delete _clientDataStructs;
}

//
// Handle creation of all widgets in the class.
//
void UI_resabsscrollzoombx::create(Widget ) {
#ifdef GUI
	if( GUI_Flag ) {
    Arg      args[56];
    Cardinal ac=0;
    Boolean  argok=False;
	Widget	_resAbsScrollZoomDialogShell ;
    
	ac = 0;
	XtSetArg(args[ac], XmNtitle, "Absolute Scroll/Zoom"); ac++;
	XtSetArg(args[ac], XmNwidth, 260); ac++;
	XtSetArg(args[ac], XmNheight, 200); ac++;
	_resAbsScrollZoomDialogShell = XtCreatePopupShell(
		"resAbsScrollZoomDialogShell" ,
		xmDialogShellWidgetClass ,      UI_mainwindow::mainWindowObject->widget ,
		args ,                          ac ) ;

    ac = 0;
    XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_GROW); ac++;
    XtSetArg(args[ac], XmNx, 0); ac++;
    XtSetArg(args[ac], XmNy, 505); ac++;
    XtSetArg(args[ac], XmNwidth, 260); ac++;
    XtSetArg(args[ac], XmNheight, 200); ac++;
    _UI_resabsscrollzoombx = XmCreateForm( _resAbsScrollZoomDialogShell ,
        _name,
        args, 
        ac);
    
    _w = _UI_resabsscrollzoombx;
    //
    // Install callback to guard against unexpected widget destruction..
    //
    installDestroyHandler();
    
    ac = 0;
    XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_GROW); ac++;
    XtSetArg(args[ac], XmNfractionBase, 59); ac++;
    XtSetArg(args[ac], XmNx, 10); ac++;
    XtSetArg(args[ac], XmNy, 160); ac++;
    XtSetArg(args[ac], XmNwidth, 240); ac++;
    XtSetArg(args[ac], XmNheight, 30); ac++;
    _raszButtonForm = XmCreateForm(_UI_resabsscrollzoombx,
        (char *)"raszButtonForm",
        args, 
        ac);
    XtManageChild(_raszButtonForm);
    
    ac = 0;
    XtSetArg(args[ac], XmNlabelString, 
        CONVERT( _resAbsScrollZoomDialogShell , "OK", 
        XmRXmString, 0, &argok)); if (argok) ac++;
    XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
    XtSetArg(args[ac], XmNwidth, 77); ac++;
    XtSetArg(args[ac], XmNheight, 30); ac++;
    _raszOKButton = XmCreatePushButton(_raszButtonForm,
        (char *)"raszOKButton",
        args, 
        ac);
    XtManageChild(_raszOKButton);
    _clientDataStructs = new UICallbackStruct;
    _clientDataStructs->object = this;
    _clientDataStructs->client_data = (XtPointer)0;
    XtAddCallback(_raszOKButton,
        XmNactivateCallback,
        UI_resabsscrollzoombx::raszOKButtonActivateCallback,
        (XtPointer)_clientDataStructs);
    
    ac = 0;
    XtSetArg(args[ac], XmNlabelString, 
        CONVERT( _resAbsScrollZoomDialogShell , "Apply", 
        XmRXmString, 0, &argok)); if (argok) ac++;
    XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
    XtSetArg(args[ac], XmNwidth, 78); ac++;
    XtSetArg(args[ac], XmNheight, 30); ac++;
    _raszApplyButton = XmCreatePushButton(_raszButtonForm,
        (char *)"raszApplyButton",
        args, 
        ac);
    XtManageChild(_raszApplyButton);
    _clientDataStructs = new UICallbackStruct;
    _clientDataStructs->object = this;
    _clientDataStructs->client_data = (XtPointer)0;
    XtAddCallback(_raszApplyButton,
        XmNactivateCallback,
        UI_resabsscrollzoombx::raszApplyButtonActivateCallback,
        (XtPointer)_clientDataStructs);
    
    ac = 0;
    XtSetArg(args[ac], XmNlabelString, 
        CONVERT( _resAbsScrollZoomDialogShell , "Cancel", 
        XmRXmString, 0, &argok)); if (argok) ac++;
    XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
    XtSetArg(args[ac], XmNwidth, 77); ac++;
    XtSetArg(args[ac], XmNheight, 30); ac++;
    _raszCancelButton = XmCreatePushButton(_raszButtonForm,
        (char *)"raszCancelButton",
        args, 
        ac);
    XtManageChild(_raszCancelButton);
    _clientDataStructs = new UICallbackStruct;
    _clientDataStructs->object = this;
    _clientDataStructs->client_data = (XtPointer)0;
    XtAddCallback(_raszCancelButton,
        XmNactivateCallback,
        UI_resabsscrollzoombx::raszCancelButtonActivateCallback,
        (XtPointer)_clientDataStructs);
    
    ac = 0;
    XtSetArg(args[ac], XmNwidth, 240); ac++;
    XtSetArg(args[ac], XmNheight, 20); ac++;
    _raszSeparator = XmCreateSeparator(_UI_resabsscrollzoombx,
        (char *)"raszSeparator",
        args, 
        ac);
    XtManageChild(_raszSeparator);
    
    ac = 0;
    XtSetArg(args[ac], XmNvalue, "100.0"); ac++;
    XtSetArg(args[ac], XmNwidth, 240); ac++;
    XtSetArg(args[ac], XmNheight, 36); ac++;
	SET_BACKGROUND_TO_GREY ;
    _raszTimeSpanTF = XmCreateTextField(_UI_resabsscrollzoombx,
        (char *)"raszTimeSpanTF",
        args, 
        ac);
    XtManageChild(_raszTimeSpanTF);
    
    ac = 0;
    XtSetArg(args[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
    XtSetArg(args[ac], XmNlabelString, 
        CONVERT( _resAbsScrollZoomDialogShell , "Value Range:", 
        XmRXmString, 0, &argok)); if (argok) ac++;
    XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
    XtSetArg(args[ac], XmNwidth, 240); ac++;
    XtSetArg(args[ac], XmNheight, 30); ac++;
    _raszTimeSpanLabel = XmCreateLabel(_UI_resabsscrollzoombx,
        (char *)"raszTimeSpanLabel",
        args, 
        ac);
    XtManageChild(_raszTimeSpanLabel);
    
    ac = 0;
    XtSetArg(args[ac], XmNvalue, "0.0"); ac++;
    XtSetArg(args[ac], XmNwidth, 240); ac++;
    XtSetArg(args[ac], XmNheight, 36); ac++;
	SET_BACKGROUND_TO_GREY ;
    _raszStartTimeTF = XmCreateTextField(_UI_resabsscrollzoombx,
        (char *)"raszStartTimeTF",
        args, 
        ac);
    XtManageChild(_raszStartTimeTF);
    
    ac = 0;
    XtSetArg(args[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
    XtSetArg(args[ac], XmNlabelString, 
        CONVERT( _resAbsScrollZoomDialogShell , "Minimum Value:", 
        XmRXmString, 0, &argok)); if (argok) ac++;
    XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
    XtSetArg(args[ac], XmNwidth, 240); ac++;
    XtSetArg(args[ac], XmNheight, 30); ac++;
    _raszStartTimeLabel = XmCreateLabel(_UI_resabsscrollzoombx,
        (char *)"raszStartTimeLabel",
        args, 
        ac);
    XtManageChild(_raszStartTimeLabel);
    
    ac = 0;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_NONE); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 10); ac++;
    XtSetArg(args[ac], XmNleftOffset, 10); ac++;
    XtSetArg(args[ac], XmNrightOffset, 10); ac++;
    XtSetValues(_raszButtonForm, args, ac);
    
    ac = 0;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, 0); ac++;
    XtSetArg(args[ac], XmNrightPosition, 19); ac++;
    XtSetArg(args[ac], XmNtopOffset, 0); ac++;
    XtSetArg(args[ac], XmNrightOffset, 0); ac++;
    XtSetValues(_raszOKButton, args, ac);
    
    ac = 0;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, 20); ac++;
    XtSetArg(args[ac], XmNrightPosition, 39); ac++;
    XtSetArg(args[ac], XmNtopOffset, 0); ac++;
    XtSetArg(args[ac], XmNrightOffset, 0); ac++;
    XtSetValues(_raszApplyButton, args, ac);
    
    ac = 0;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, 40); ac++;
    XtSetArg(args[ac], XmNrightPosition, 59); ac++;
    XtSetArg(args[ac], XmNtopOffset, 0); ac++;
    XtSetArg(args[ac], XmNrightOffset, 0); ac++;
    XtSetValues(_raszCancelButton, args, ac);
    
    ac = 0;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_NONE); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNbottomWidget, _raszButtonForm); ac++;
    XtSetArg(args[ac], XmNbottomOffset, -1); ac++;
    XtSetArg(args[ac], XmNleftOffset, 10); ac++;
    XtSetArg(args[ac], XmNrightOffset, 10); ac++;
    XtSetValues(_raszSeparator, args, ac);
    
    ac = 0;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNtopWidget, _raszTimeSpanLabel); ac++;
    XtSetArg(args[ac], XmNtopOffset, 0); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 0); ac++;
    XtSetArg(args[ac], XmNleftOffset, 10); ac++;
    XtSetArg(args[ac], XmNrightOffset, 10); ac++;
    XtSetValues(_raszTimeSpanTF, args, ac);
    
    ac = 0;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNtopWidget, _raszStartTimeTF); ac++;
    XtSetArg(args[ac], XmNtopOffset, 0); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 0); ac++;
    XtSetArg(args[ac], XmNleftOffset, 10); ac++;
    XtSetArg(args[ac], XmNrightOffset, 10); ac++;
    XtSetValues(_raszTimeSpanLabel, args, ac);
    
    ac = 0;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_NONE); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNtopWidget, _raszStartTimeLabel); ac++;
    XtSetArg(args[ac], XmNtopOffset, 0); ac++;
    XtSetArg(args[ac], XmNbottomOffset, 0); ac++;
    XtSetArg(args[ac], XmNleftOffset, 10); ac++;
    XtSetArg(args[ac], XmNrightOffset, 10); ac++;
    XtSetValues(_raszStartTimeTF, args, ac);
    
    ac = 0;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(args[ac], XmNtopOffset, 10); ac++;
    XtSetArg(args[ac], XmNleftOffset, 10); ac++;
    XtSetArg(args[ac], XmNrightOffset, 10); ac++;
    XtSetValues(_raszStartTimeLabel, args, ac);

	ac = 0;
	XtSetArg(args[ac], XmNx, 0); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 260); ac++;
	XtSetArg(args[ac], XmNheight, 200); ac++;
	XtSetValues( baseWidget() , args, ac);
    
    // Begin user code block <endcreate> 
    // End user code block <endcreate> 
	}
#endif
}

//
// Classname access.
//
const char *const UI_resabsscrollzoombx::className()
{
    return ("UI_resabsscrollzoombx");
}

void UI_resabsscrollzoombx::raszOKButtonActivateCallback(Widget w, 
		XtPointer clientData, XtPointer callData)
{
    UICallbackStruct *data = (UICallbackStruct *) clientData;
    UI_resabsscrollzoombx *obj = (UI_resabsscrollzoombx *)data->object;
    obj->raszOKButtonActivate(w, data->client_data, callData);
}

void UI_resabsscrollzoombx::raszOKButtonActivate(Widget, XtPointer, XtPointer)
{
    // Empty virtual function. Called from raszOKButtonActivateCallback.
    // Derived classes can override.
}

void UI_resabsscrollzoombx::raszApplyButtonActivateCallback(Widget w, 
		XtPointer clientData, XtPointer callData)
{
    UICallbackStruct *data = (UICallbackStruct *) clientData;
    UI_resabsscrollzoombx *obj = (UI_resabsscrollzoombx *)data->object;
    obj->raszApplyButtonActivate(w, data->client_data, callData);
}

void UI_resabsscrollzoombx::raszApplyButtonActivate(Widget, XtPointer, XtPointer)
{
    // Empty virtual function. Called from raszApplyButtonActivateCallback.
    // Derived classes can override.
}

void UI_resabsscrollzoombx::raszCancelButtonActivateCallback(Widget w, 
		XtPointer clientData, XtPointer callData)
{
    UICallbackStruct *data = (UICallbackStruct *) clientData;
    UI_resabsscrollzoombx *obj = (UI_resabsscrollzoombx *)data->object;
    obj->raszCancelButtonActivate(w, data->client_data, callData);
}

void UI_resabsscrollzoombx::raszCancelButtonActivate(Widget, XtPointer, XtPointer)
{
    // Empty virtual function. Called from raszCancelButtonActivateCallback.
    // Derived classes can override.
}

// Begin user code block <tail> 
// End user code block <tail> 
