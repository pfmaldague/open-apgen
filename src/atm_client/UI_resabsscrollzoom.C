#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "RES_eval.H"	// for eval_error
#include <ActivityInstance.H>
#include "UI_resabsscrollzoom.H"
#include "UI_mainwindow.H"
#include "action_request.H" // for RESOURCE_SCROLLrequest

UI_resabsscrollzoom	*resAbsScrollZoomDialog = NULL ;

UI_resabsscrollzoom::UI_resabsscrollzoom(const char *name, Widget parent)
	: UI_resabsscrollzoombx(name, parent) {
	create( parent ) ; }

UI_resabsscrollzoom::UI_resabsscrollzoom(const char *name)
	: UI_resabsscrollzoombx(name) {
	create( NULL ) ; }

UI_resabsscrollzoom::~UI_resabsscrollzoom() { ; }

// Classname access.
const char *const UI_resabsscrollzoom::className() {
    return("UI_resabsscrollzoom"); }

void UI_resabsscrollzoom::initialize(Cstring min, Cstring span) {
	if(GUI_Flag) {
	if(!resAbsScrollZoomDialog)
		resAbsScrollZoomDialog = new UI_resabsscrollzoom( "Resource Absolute Scroll and Zoom" ) ;
	resAbsScrollZoomDialog->mainWindow = UI_mainwindow::mainWindowObject ;
	XtVaSetValues( resAbsScrollZoomDialog->_raszStartTimeTF , XmNvalue, *min , NULL ) ;
	XtVaSetValues( resAbsScrollZoomDialog->_raszTimeSpanTF ,  XmNvalue, *span , NULL ) ; } }

void UI_resabsscrollzoom::raszOKButtonActivate(Widget, XtPointer, XtPointer) {
	if( applyChanges() )
		this->unmanage() ; }

void UI_resabsscrollzoom::raszApplyButtonActivate(Widget, XtPointer, XtPointer) {
    applyChanges(); }

void UI_resabsscrollzoom::raszCancelButtonActivate(Widget, XtPointer, XtPointer) {
    this->unmanage(); }

Boolean UI_resabsscrollzoom::applyChanges () {
	if(GUI_Flag) {
	char			*min;
	char			*span;
	RESOURCE_SCROLLrequest	*request;

	XtVaGetValues(_raszStartTimeTF, XmNvalue, &min, NULL);
	XtVaGetValues(_raszTimeSpanTF,  XmNvalue, &span, NULL);

	request = new RESOURCE_SCROLLrequest(min, span);
	XtFree(min);
	XtFree(span);
	if(request->process() == apgen::RETURN_STATUS::SUCCESS) {
		delete request;
		return TRUE;
	} else {
		delete request;
		return FALSE;
	}
}
	return TRUE; }
