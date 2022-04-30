#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "ACT_sys.H"
#include <ActivityInstance.H>
#include "apDEBUG.H"
#include "UI_activitydisplay.H"
#include "UI_resourcedisplay.H"
#include "UI_mainwindow.H"
#include "UI_GeneralMotifInclude.H"
#include "apcoreWaiter.H"

using namespace std;

                        // in UI_motif_widget.C:
extern resource_data    ResourceData ;



			// in main.C:
extern int		GUI_Flag ;

#define SET_BACKGROUND_TO_GREY  XtSetArg( args[ac]  , XmNbackground, ResourceData.grey_color ) ; ac++;

// STATICS:
static Arg      		args[256] ;
static Cardinal 		ac = 0 ;
static Boolean  		argok = False ;

extern void SET_BACKGROUND_COLOR(Widget, ArgList, Cardinal*, Pixel);
extern XtPointer CONVERT(Widget, const char *, const char *, int, Boolean *);

void resource_display::uninitialize () {
	// resource display is being closed; set to unselected
	resourceSystem->removeAllResources ();

	// 98-03-20 DSG also make sure background points and mode are reset
	// setControlInfo( YSCROLL_AUTO_WINDOW, 0 ) ;
	resourceSystem->mw_objects.clear() ; 
	udef_intfc::something_happened() += 1 ; }


void resource_display::sensitizeLegendsSelectedButtons() {
	if( GUI_Flag ) {
		XtVaSetValues ( _rdUpArrowButton->widget ,XmNsensitive,True,NULL);
		XtVaSetValues ( _rdDownArrowButton->widget ,XmNsensitive,True,NULL); } }

void resource_display::desensitizeLegendsSelectedButtons() {
	if( GUI_Flag ) {
		XtVaSetValues ( _rdUpArrowButton->widget ,XmNsensitive,False,NULL);
		XtVaSetValues ( _rdDownArrowButton->widget ,XmNsensitive,False,NULL); } }


int resource_display::LegendsSelectedButtonsAreSensitive() {
	return _rdUpArrowButton->is_sensitive(); }

void resource_display::purge () {
	// PFM
	// purge r.d. (like uninitialize(), but leave selected status alone)
	// 97-11-25 DSG change so all RDs are reselected (as if they had been
	//   newly created) -- this results in RDs acting as if newly created
	// 97-12-02 DSG also make sure background points and mode are reset
	resourceSystem->removeAllResources ();

}

int resource_display::numResourcesInDisplay () {
	return resourceSystem->mw_objects.get_length();
}

apgen::RETURN_STATUS resource_display::addResourceToDisplay(
		const Cstring &res,
		Cstring &any_errors) {
	return resourceSystem->addResource(res, any_errors);
}

apgen::RETURN_STATUS resource_display::removeResourceFromDisplay(const Cstring &theResource) {
	apgen::RETURN_STATUS	status;

	status = resourceSystem->removeResource(theResource);

	return status;
}

void resource_display::setControlInfo (const Cstring &theResource , RD_YSCROLL_MODE mode,int resolution) {
	resourceSystem->setyscrollmode (theResource , mode);
	resourceSystem->setModelingResolution (theResource , resolution);
}

void resource_display::getControlInfo (const Cstring &theResource , RD_YSCROLL_MODE &mode, int &resolution) {
	mode = resourceSystem->getyscrollmode(theResource);
	resolution = resourceSystem->getModelingResolution(theResource);
}

void resource_display::yabsscrollzoom(const Cstring &theResource , double min , double span) {
	MW_object	*res_plot = (MW_object *) resourceSystem->mw_objects.find(theResource);

	// we need to change this so the first selected MW_object is scrolled/zoomed
	if(res_plot && res_plot->getyscrollmode() == YSCROLL_MANUAL) {
		res_plot->yabspanzoom(min, span);
	}
}

apgen::RETURN_STATUS resource_display::getZoomScrollInfo(const Cstring& theResource, Cstring& min, Cstring& span) {
	List_iterator		res_plots( resourceSystem->mw_objects);
	MW_object		*res_plot;

	while((res_plot = (MW_object *) res_plots())) {
		if(res_plot->get_selection()) {
			return res_plot->ygetminspan(min, span); 
		} 
	}
	return apgen::RETURN_STATUS::FAIL;
}

#ifdef GUI
static Arg* get_args() {
	static Arg      ADargs[1];
	XtSetArg(ADargs[0], XmNpaneMaximum, 3000);
	return ADargs; }

#else
static void* get_args() {
	return NULL; }
#endif

resource_display::resource_display( activity_display * AD , const Cstring & name ) 
	: motif_widget(name,
		xmFrameWidgetClass,	UI_mainwindow::mainWindowObject->_panedWindow,
		get_args(),		1,
		FALSE),
	activitySystem(AD->activitySystem) {
	if(GUI_Flag) {
	ac = 0;
	if (APcloptions::theCmdLineOptions().roomy_display ) {
	    XtSetArg( args[ac]  , XmNshadowThickness, 0 ) ; ac++ ; }
	else {
		XtSetArg( args[ac]  , XmNshadowThickness, 2 ) ; ac++ ;
		XtSetArg( args[ac]  , XmNshadowType , XmSHADOW_IN ) ; ac++ ; }

	// Was 100
	XtSetArg( args[ac]  , XmNheight, 120  ) ; ac++ ;
	XtSetValues( widget , args , ac ) ;

	// Theory: by default, the form we are defining next will span the entire width of
	// the APGEN display. Specifying the width is guaranteed to be wrong if the user resizes
	// the main window anyway. Ah, but wait: it's true that we build all these widgets before
	// they are ever managed... OK, we'll leave the width there for now.
	ac = 0;
	XtSetArg( args[ac]  , XmNresizePolicy , XmRESIZE_GROW ) ; ac++ ;
	// XtSetArg( args[ac]  , XmNwidth, 523 ) ; ac++ ;
	// Was 98
	XtSetArg( args[ac]  , XmNheight, 118 ) ; ac++ ;
	SET_BACKGROUND_TO_GREY
	} // if( GUI_Flag )
	_resourceDisplayForm = new motif_widget(
		"resourceDisplayForm",
		xmFormWidgetClass ,	this ,
		args,			ac ,
		TRUE ) ;

	if( GUI_Flag ) {

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNhighlightThickness, 0); ac++;
	XtSetArg(args[ac], XmNfont, ResourceData.small_font ) ; ac++ ;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 2); ac++;
	XtSetArg(args[ac], XmNwidth, 100); ac++;
	XtSetArg(args[ac], XmNheight, 15); ac++;
	SET_BACKGROUND_TO_GREY
	XtSetArg(args[ac], XmNforeground, 
		CONVERT( widget , "Black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	_rdSelAllButton = new motif_widget( Cstring( "RD" ) + AD->myNumber + "_selectAll" ,
		xmPushButtonWidgetClass ,	_resourceDisplayForm,
		args ,				ac ,
		TRUE );
	*_rdSelAllButton = "Select All" ;
	_rdSelAllButton->add_callback( resource_display::rdSelAllButtonActivateCallback,
		XmNactivateCallback ,
		this ) ;
	
	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNhighlightThickness, 0); ac++;
	XtSetArg(args[ac], XmNarrowDirection, XmARROW_DOWN); ac++;
	XtSetArg(args[ac], XmNx, 153); ac++;
	XtSetArg(args[ac], XmNwidth, 47); ac++;
	XtSetArg(args[ac], XmNheight, 15); ac++;
	SET_BACKGROUND_TO_GREY
	XtSetArg(args[ac], XmNforeground, 
		CONVERT( widget , "Black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	_rdDownArrowButton = new motif_widget( Cstring( "RD" ) + AD->myNumber + "_downArrow" ,
		xmArrowButtonWidgetClass ,	_resourceDisplayForm,
		args ,				ac ,
		TRUE );
	_rdDownArrowButton->add_callback( resource_display::rdDownArrowActivateCallback,
		XmNactivateCallback,
		this );
	
	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	SET_BACKGROUND_TO_GREY
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNhighlightThickness, 0); ac++;
	XtSetArg(args[ac], XmNx, 104); ac++;
	XtSetArg(args[ac], XmNwidth, 47); ac++;
	XtSetArg(args[ac], XmNheight, 15); ac++;
	XtSetArg(args[ac], XmNforeground, 
		CONVERT( widget , "Black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	_rdUpArrowButton = new motif_widget( Cstring( "RD" ) + AD->myNumber + "_upArrow" ,
		xmArrowButtonWidgetClass ,	_resourceDisplayForm,
		args ,				ac ,
		TRUE );
	_rdUpArrowButton->add_callback( resource_display::rdUpArrowActivateCallback,
		XmNactivateCallback,
		this );
	
	*_rdSelAllButton ^ 2 ^ *_resourceDisplayForm ^ endform ;
	*_rdUpArrowButton ^ 2 ^ *_resourceDisplayForm ^ endform ;
	*_rdDownArrowButton ^ 2 ^ *_resourceDisplayForm ^ endform ;
	*_resourceDisplayForm < 2 < *_rdSelAllButton < 2 < *_rdUpArrowButton < 2 < *_rdDownArrowButton < endform ;

	} // if( GUI_Flag )
}

void resource_display::rdDownArrowActivateCallback( Widget , callback_stuff *client_data , void * ) {
	resource_display	*obj = ( resource_display * ) client_data->data ;

	obj->resourceSystem->vlegendswap( SWAPDOWN ) ; }

void resource_display::rdUpArrowActivateCallback( Widget, callback_stuff * client_data , void * ) {
	resource_display	*obj = ( resource_display * ) client_data->data ;

	obj->resourceSystem->vlegendswap (SWAPUP) ; }

void resource_display::rdSelAllButtonActivateCallback( Widget , callback_stuff *clientData , XtPointer ) {
	resource_display	*obj = ( resource_display * ) clientData->data ;

	obj->resourceSystem->selectalllegends( 1 ) ; }




void	resource_display::initialize() {
	initialize_res_sys(); }

// analogous to activity_display::initialize_act_sys():
void resource_display::initialize_res_sys() {
	// The RD_sys object should instantiate itself as DrawingArea widget
	// that leaves room on the left for the resource legend.
	resourceSystem = new RD_sys( activitySystem , _resourceDisplayForm ) ;
	*activitySystem->get_siblings() << new sibling( resourceSystem ) ; }

void resource_display::manage() {
	motif_widget::manage() ;
	resourceSystem->manage() ; }
