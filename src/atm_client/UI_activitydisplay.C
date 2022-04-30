#if HAVE_CONFIG_H
#include <config.h>
#endif
// #define apDEBUG

#include "apDEBUG.H"

#include "ACT_sys.H"
#include <ActivityInstance.H>
#include "action_request.H"
#include "APmodel.H"
#include "CON_sys.H"
#include "UI_activitydisplay.H"
#include "UI_exec.H"
#include "UI_ds_timeline.H"
#include "UI_ds_draw.H"
#include "UI_mainwindow.H"
#include "UI_GeneralMotifInclude.H"     // for resource_data
#include "UI_resourcedisplay.H"
#include "apcoreWaiter.H"

#define BXC_LEGEND_WIDTH_P2 202
#define BXC_LEGEND_WIDTH 200


// EXTERNS:

			// in UI_exec.C:
extern UI_exec*		UI_subsystem;

	                // in UI_motif_widget.C:
extern resource_data    ResourceData;

			// C-style STATICS:
static Arg		args[256];
static Cardinal		ac = 0;
#ifdef GUI
static Boolean		argok = False;
extern XtPointer CONVERT(Widget, const char *, const char *, int, char *);
#endif
			// Forward Declaration(s):
void			fill_list_with_complete_set_of_time_labels(List & theTimeLabelList);

static int		adNumber = 0;

void activity_display::clear() {
	if(activitySystem) {
		activitySystem->cleargraph();
		// not necessary:
		// activitySystem->legend_display->get_this()->cleargraph();
		} }

apgen::RETURN_STATUS activity_display::getZoomPanInfo (CTime & startTime , CTime & duration) {
	apgen::RETURN_STATUS	r;
	CTime	t1 , t2;

	r = activitySystem->getviewtime(t1 , t2);
	startTime = t1;
	duration = t2;
	return r;
}

ACT_sys *activity_display::getActivitySystem () {
	return activitySystem; }

void activity_display::sensitizeLegendsSelectedButtons () {
#ifdef GUI
	if(GUI_Flag) {
		XtVaSetValues (_adUpArrowButton->widget ,XmNsensitive,True,NULL);
		XtVaSetValues (_adDownArrowButton->widget ,XmNsensitive,True,NULL);
	}
#endif
	}

void activity_display::desensitizeLegendsSelectedButtons () {
#ifdef GUI
	if(GUI_Flag) {
		XtVaSetValues (_adUpArrowButton->widget ,XmNsensitive,False,NULL);
		XtVaSetValues (_adDownArrowButton->widget ,XmNsensitive,False,NULL);
	}
#endif
	}

int activity_display::LegendsSelectedButtonsAreSensitive() {
	return _adUpArrowButton->is_sensitive(); }

void activity_display::deleteSelectedLegends() {
	activitySystem->modify_selected_legends(0); }

void activity_display::flattenSelectedLegends() {
	activitySystem->modify_selected_legends(4); }

void activity_display::squishSelectedLegends() {
	activitySystem->modify_selected_legends(1); }

void activity_display::unsquishSelectedLegends() {
	activitySystem->modify_selected_legends(2); }

void activity_display::expandSelectedLegends() {
	activitySystem->modify_selected_legends(3); }

void activity_display::unselectAllLegends() {
	activitySystem->selectalllegends(0); }

static int disable_action_request_for_AD_selection = 0;

void activity_display::makeSelectedFromGui(int selected) {
	SELECT_ACT_DISPLAYrequest	*request;
	// prevent action request from messing up our algorithm
	disable_action_request_for_AD_selection = 1;
	request = new SELECT_ACT_DISPLAYrequest(getADName(), selected);
	// this will log the request, but will not duplicate what we are doing here
	request->process();
	disable_action_request_for_AD_selection = 0; }

void activity_display::makeSelectedFromActionRequest(int selected) {
	if(!disable_action_request_for_AD_selection) {
		if(!activitySystem->get_hopper_id().length()) {
			if(selected) {
				*_adSelectedTB = "1"; }
			else {
				*_adSelectedTB = "0"; } } } }

int activity_display::isSelected() {
	if(_adSelectedTB->get_text() == "SET")
		return 1;
	return 0; }

Cstring
activity_display::getADName() const
{
	Cstring digit = ("ACT_system" / activitySystem->get_key());
	
	int index = atoi(*digit);

	if(index == 6)
	{
		return "G1";
	}
	else if(index == 4)
	{
		return "H1";
	}
	else
	{
		return Cstring("A") + digit;
	}
}

void activity_display::adSelectCallback(Widget , callback_stuff *clientData , XtPointer) {
	activity_display		*obj = (activity_display *) clientData->data;

	if(obj->_adSelectedTB->get_text() == "SET") {
		obj->makeSelectedFromGui(1); }
	else if(obj->_adSelectedTB->get_text() == "NOT SET") {
		obj->makeSelectedFromGui(0); }
	}

void activity_display::adSelAllButtonActivateCallback(Widget , callback_stuff *clientData , XtPointer) {
	activity_display		*obj = (activity_display *) clientData->data;

	obj->activitySystem->selectalllegends(1); }

void activity_display::adTimeSystemChangeCallback(Widget, callback_stuff *clientData , void *) {
	Cstring			newTime = clientData->parent->get_text();
	activity_display	*obj = (activity_display *) clientData->data;

	if(newTime == "UTC")
		obj->activitySystem->timezone = TIMEZONE_UTC;
	else if(newTime == "PST")
		obj->activitySystem->timezone = TIMEZONE_PST;
	else if(newTime == "PDT")
		obj->activitySystem->timezone = TIMEZONE_PDT;
	else if(newTime == "MST")
		obj->activitySystem->timezone = TIMEZONE_MST;
	else if(newTime == "MDT")
		obj->activitySystem->timezone = TIMEZONE_MDT;
	else if(newTime == "CST")
		obj->activitySystem->timezone = TIMEZONE_CST;
	else if(newTime == "CDT")
		obj->activitySystem->timezone = TIMEZONE_CDT;
	else if(newTime == "EST")
		obj->activitySystem->timezone = TIMEZONE_EST;
	else if(newTime == "EDT")
		obj->activitySystem->timezone = TIMEZONE_EDT;
	else {

		obj->activitySystem->timezone = TIMEZONE_EPOCH;
		obj->activitySystem->timezone.epoch = newTime;
		try {
			TypedValue& tdv = globalData::get_symbol(newTime);
			if(globalData::isAnEpoch(newTime)) {
				*obj->activitySystem->timezone.theOrigin = tdv.get_time_or_duration();
				obj->activitySystem->timezone.scale = 1.0;
			} else if(globalData::isATimeSystem(newTime)) {
				ListOVal*	LV = &tdv.get_array();
				ArrayElement*	element = LV->find("origin");

				*obj->activitySystem->timezone.theOrigin = element->Val().get_time_or_duration();
				element = LV->find("scale");
				obj->activitySystem->timezone.scale = element->Val().get_double();
			}
		} catch(eval_error Err) {
		}
	}
	obj->activitySystem->get_siblings()->timemark->configure_graphics(NULL);
	obj->activitySystem->constraint_display->configure_graphics(NULL); }

void activity_display::adDownArrowActivateCallback(Widget , callback_stuff *client_data , void *) {
	activity_display	*obj = (activity_display *) client_data->data;

	// debug
	// std::cout << "adDownArrowActivateCallback: calling vlegendswap(SWAPDOWN)\n";
	obj->activitySystem->vlegendswap(SWAPDOWN); }

void activity_display::adUpArrowActivateCallback(Widget, callback_stuff * client_data , void *) {
	activity_display	*obj = (activity_display *) client_data->data;

	// debug
	// std::cout << "adUpArrowActivateCallback: calling vlegendswap(SWAPUP)\n";
	obj->activitySystem->vlegendswap(SWAPUP); }

	/* Creates a very simple hidden pane (just a form.) We need an additional argument to
	 * indicate how tall, wide we want the display to be; requirements differ between
	 * "regular" activity displays (part of the main window) and embedded displays (hopper,
	 * activity type editor).
	 *
	 * NOTE: UI_mainwindow::initialize() is the one and only client of this constructor. */

#ifdef GUI
static Arg* get_args() {
	static Arg      ADargs[1];
	XtSetArg(ADargs[0], XmNpaneMaximum, 3000);
	return ADargs; }

#else
static void* get_args() {
	return NULL; }
#endif
activity_display::activity_display(
		const Cstring	&name ,
		motif_widget	*parent_to_use ,
		int		wid ,			// set to 484 by UI_mainwindow::initialize()
		int		hgt			// set to 334 by UI_mainwindow::initialize()
		) :
		motif_widget(
			name,
			xmFrameWidgetClass,		// we are a Frame!
			parent_to_use,
			// NULL,			0,
			get_args(),			1,
			FALSE),
		myNumber(++adNumber),
		adViolationCountLabel(NULL),
		siblings(NULL) {
	CTime		theTime, theDuration((int) ONEDAY, 0, true);
	static int	whichNumber = 0;

	UI_mainwindow::activityDisplay[whichNumber++] = this;
	model_intfc::FirstEvent(theTime);
	siblings = new list_of_siblings((activity_display *) get_this_widget(), theTime, theDuration);
	ac = 0;
#ifdef GUI
	if(GUI_Flag) {
		if (APcloptions::theCmdLineOptions().roomy_display) {
	    		XtSetArg(args[ac] , XmNshadowThickness , 0); ac++; }
		else {
	    		XtSetArg(args[ac] , XmNshadowThickness , 2); ac++;
	    		XtSetArg(args[ac] , XmNshadowType , XmSHADOW_IN); ac++;  }
		XtSetArg(args[ac] , XmNwidth , wid); ac++;
		XtSetArg(args[ac] , XmNheight , hgt); ac++;
		XtSetValues(widget , args , ac);
		XtVaSetValues(widget , XmNpaneMinimum , 105 , NULL);

		// XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_GROW); ac++;
		ac = 0;
		if (APcloptions::theCmdLineOptions().roomy_display) {
	    		XtSetArg(args[ac] , XmNshadowThickness , 0); ac++; }
		else {
	    		XtSetArg(args[ac] , XmNshadowThickness , 2); ac++;
	    		XtSetArg(args[ac] , XmNshadowType , XmSHADOW_IN); ac++;  }
		XtSetArg(args[ac] , XmNwidth , wid); ac++;
		XtSetArg(args[ac], XmNheight, hgt - 2); ac++;
		XtSetArg(args[ac], XmNbackground, ResourceData.peach_dark_color); ac++;
		XtSetArg(args[ac], XmNforeground, ResourceData.black_color); ac++; }
#endif
	// This form spans the whole width of the APGEN activity display:
	_activityDisplayForm = new motif_widget(
		"activityDisplayForm" ,
		xmFormWidgetClass ,	this ,
		args ,			ac ,
		TRUE); }

activity_display::~activity_display() {
	if(siblings) delete siblings; }

void fill_list_with_complete_set_of_time_labels(List &theTimeLabelList) {
	ListOVal		globs;
	ArrayElement*		bp;

	theTimeLabelList.clear();
	theTimeLabelList << new String_node("UTC");
	theTimeLabelList << new String_node("PST");
	theTimeLabelList << new String_node("PDT");
	theTimeLabelList << new String_node("MST");
	theTimeLabelList << new String_node("MDT");
	theTimeLabelList << new String_node("CST");
	theTimeLabelList << new String_node("CDT");
	theTimeLabelList << new String_node("EST");
	theTimeLabelList << new String_node("EDT");

	globalData::copy_symbols(globs);
	for(int i = 0; i < globs.get_length(); i++) {
		bp = globs[i];
		if(globalData::isAnEpoch(bp->get_key())) {
			theTimeLabelList << new String_node(bp->get_key());
		} else if(globalData::isATimeSystem(bp->get_key())) {
			theTimeLabelList << new String_node(bp->get_key());
		}
	}
}

void activity_display::manage() {
	if(!thePopupName.length()) {
		List		list_of_time_system_options;

		fill_list_with_complete_set_of_time_labels(list_of_time_system_options);
		define_time_system_widgets(list_of_time_system_options); }
	DBG_NOINDENT("activity_display " << get_key() << "->manage() called; invoking motif_widget::manage().\n");
	motif_widget::manage(); }

int activity_display::isVisible() {
#ifdef GUI
	if(GUI_Flag) {
		// This ugly logic is required because we manage ONE of the activity_displays
		// so as to give the apgen main window the right size... THAT activity_display
		// is managed while its child (a form) is NOT. The OTHER activity_displays are
		// NOT managed, while their form children ARE... Just think about it!
		return is_managed() && ((motif_widget *) children[0])->is_managed();
	}
	else {
		return 0;
	}
#else
	return 0;
#endif
	}

// hides the pane after it has been revealed

void activity_display::uninitialize () {
	// the activity display is selected
	if(!activitySystem->get_hopper_id().length()) {
		*_adSelectedTB = "0"; } }

// reveals the pane

void activity_display::initialize(const Cstring &popupName) {
	thePopupName = popupName;
#ifdef GUI
	if(GUI_Flag) {
	static int TBcount = 0;

	ac = 0;
	XtSetArg(args[ac] , XmNhighlightThickness ,	0); ac++;
	XtSetArg(args[ac] , XmNlabelString ,		CONVERT(widget , "", XmRXmString , 0, &argok)); if(argok) ac++;
	XtSetArg(args[ac] , XmNspacing ,		0); ac++;
	XtSetArg(args[ac] , XmNindicatorSize ,		15); ac++;
	XtSetArg(args[ac] , XmNbackground ,		ResourceData.peach_dark_color); ac++;
	_adSelectedTB = new motif_widget(/* As per Dennis' request:
					     Cstring("adSelectedTB") + UI_mainwindow::mainWindowObject->_panedWindow->children.get_length() */
		Cstring("ADtoggleButton") + ++TBcount ,
		xmToggleButtonWidgetClass ,	_activityDisplayForm,
		args ,				ac ,
		TRUE);
	*_adSelectedTB = "0";
/*	if(popupName.length()) {
		// we KNOW that our parent is the paned_window of a hopper_popup
		// NO! We now have two types of hopper containers (the other is the
		// activity editor). Let the client (UI_mainwindow::initialize_hopper(s)
		// or whoever) handle the AD pointer.
		// hopper_popup		*aHopper = (hopper_popup *) myParent->myParent;

		// aHopper->AD = this;
		_adSelectedTB->set_sensitive(FALSE); }
          All ADs are selectable!!*/  


	_adSelectedTB->add_callback(activity_display::adSelectCallback, XmNvalueChangedCallback , this);

	adTimeSystemLabel = NULL;
	adPointerTimeLabel = NULL;

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNhighlightThickness, 0); ac++;
	XtSetArg(args[ac], XmNfont, ResourceData.small_font); ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 2); ac++;
	XtSetArg(args[ac], XmNy, 315); ac++;
	XtSetArg(args[ac], XmNwidth, 100); ac++;
	XtSetArg(args[ac], XmNheight, 15); ac++;
	XtSetArg(args[ac], XmNbackground, ResourceData.peach_color); ac++;
	XtSetArg(args[ac], XmNforeground, 
		CONVERT(widget , "Black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	_adSelAllButton = new motif_widget(Cstring("AD") + myNumber + "_selectAll" ,
		xmPushButtonWidgetClass ,	_activityDisplayForm,
		args ,				ac ,
		TRUE);
	*_adSelAllButton = "Select All";
	_adSelAllButton->add_callback(activity_display::adSelAllButtonActivateCallback,
		XmNactivateCallback ,
		this);
	
	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNbackground, ResourceData.peach_color); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNhighlightThickness, 0); ac++;
	XtSetArg(args[ac], XmNarrowDirection, XmARROW_DOWN); ac++;
	XtSetArg(args[ac], XmNx, 153); ac++;
	XtSetArg(args[ac], XmNy, 315); ac++;
	XtSetArg(args[ac], XmNwidth, 47); ac++;
	XtSetArg(args[ac], XmNheight, 15); ac++;
	XtSetArg(args[ac], XmNforeground, 
		CONVERT(widget , "Black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	_adDownArrowButton = new motif_widget(Cstring("AD") + myNumber + "_downArrow" ,
		xmArrowButtonWidgetClass ,	_activityDisplayForm,
		args ,				ac ,
		TRUE);
	_adDownArrowButton->add_callback(activity_display::adDownArrowActivateCallback,
		XmNactivateCallback,
		this);
	
	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNbackground, ResourceData.peach_color); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNhighlightThickness, 0); ac++;
	XtSetArg(args[ac], XmNx, 104); ac++;
	XtSetArg(args[ac], XmNy, 315); ac++;
	XtSetArg(args[ac], XmNwidth, 47); ac++;
	XtSetArg(args[ac], XmNheight, 15); ac++;
	XtSetArg(args[ac], XmNforeground, 
		CONVERT(widget , "Black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	_adUpArrowButton = new motif_widget(Cstring("AD") + myNumber + "_upArrow" ,
		xmArrowButtonWidgetClass ,	_activityDisplayForm,
		args ,				ac ,
		TRUE);
	_adUpArrowButton->add_callback(activity_display::adUpArrowActivateCallback,
		XmNactivateCallback,
		this);
	
	*_adSelectedTB < *_activityDisplayForm < endform;
	*_activityDisplayForm ^ 2 ^ *_adSelectedTB ^ endform;

	*_adSelAllButton ^ 2 ^ *_activityDisplayForm ^ endform;
	*_adUpArrowButton ^ 2 ^ *_activityDisplayForm ^ endform;
	*_adDownArrowButton ^ 2 ^ *_activityDisplayForm ^ endform;
	*_activityDisplayForm < 2 < *_adSelAllButton < 2 < *_adUpArrowButton < 2 < *_adDownArrowButton < endform;
	}
#endif
	
	initialize_act_sys();
}

void activity_display::initialize_act_sys() {
	int		j;
	List		list_of_time_system_options;

	// This will be destroyed when the widget is managed, so let's not define all the buttons:
	list_of_time_system_options << new String_node("UTC");

	if(thePopupName & "Hopper") {
		activitySystem = new ACT_hopper(
			Cstring("ACT_system") + myNumber ,
			_activityDisplayForm ,
			thePopupName ,
			this,
			siblings);
	} else {
		activitySystem = new ACT_sys(
			Cstring("ACT_system") + myNumber ,
			_activityDisplayForm ,
			this,
			siblings);
	}

#	ifdef GUI
	if(GUI_Flag) {
	// Hopper design DONE: should switch this to after the ACT_sys is defined
	define_time_system_widgets(list_of_time_system_options);
	}	
#endif

	// Hopper design: Should not do this for Hoppers?
	if(!thePopupName.length()) {
		resourceDisplay = new resource_display(this , get_key() + "_resourceDisplay_0");
		resourceDisplay->initialize();
	}
}

void activity_display::reset_time_system_widgets() {
	List		the_names;

	fill_list_with_complete_set_of_time_labels(the_names);

	define_time_system_widgets(the_names); }

void activity_display::define_time_system_widgets(const List &copy_of_all_time_labels) {
#ifdef GUI
	if(GUI_Flag) {
	motif_widget	*Button;
	String_node	*N;

	// Hopper design DONE: should skip time system creation; always use "relative to start"
	if(!adPointerTimeLabel) {
		XmFontListEntry		entry = XmFontListEntryCreate((char*)"tag1", XmFONT_IS_FONT, ResourceData.legend_font);
		XmFontList		fontList = XmFontListAppendEntry(NULL , entry);

        	// XtFree((char *) entry);

		if(activitySystem->is_a_hopper()) {
			// Hopper design: borrow from UI_mainwindow::time_system_ok()

			// use the existing time origin for the epoch origin
			DBG_NOINDENT("UI_activitydisplay: setting hopper timezone to newly-defined Epoch.\n");
			activitySystem->timezone = TIMEZONE_UTC;

			}
		else {
			ac = 0;
			XtSetArg(args[ac] , XmNbackground,	ResourceData.peach_dark_color	); ac++;
			XtSetArg(args[ac] , XmNforeground,	ResourceData.black_color	); ac++;
			XtSetArg(args[ac] , XmNfontList ,	fontList			); ac++;
			XtSetArg(args[ac] , XmNrecomputeSize ,	False				); ac++;
			XtSetArg(args[ac] , XmNalignment ,	XmALIGNMENT_BEGINNING		); ac++;
			adTimeSystemLabel = new motif_widget("adTimeSystemLabel_ta_da_daa_daaa" ,
				xmLabelWidgetClass ,		_activityDisplayForm ,
				args ,				ac ,
				TRUE);
			*adTimeSystemLabel = "UTC";

			activitySystem->timezone = TIMEZONE_UTC;

			ac = 0;
			XtSetArg(args[ac] , XmNbackground,	ResourceData.peach_dark_color	); ac++;
			XtSetArg(args[ac] , XmNforeground,	ResourceData.black_color	); ac++;
			XtSetArg(args[ac] , XmNfontList ,	fontList			); ac++;
			XtSetArg(args[ac] , XmNrecomputeSize ,	False				); ac++;
			// provide a string that gives the Widget its correct size:
			adViolationLabel = new motif_widget("Violations:" ,
				xmLabelWidgetClass ,		_activityDisplayForm ,
				args ,				ac ,
				TRUE);

			ac = 0;
			XtSetArg(args[ac] , XmNbackground,	ResourceData.peach_dark_color	); ac++;
			XtSetArg(args[ac] , XmNforeground,	ResourceData.black_color	); ac++;
			XtSetArg(args[ac] , XmNfontList ,	fontList			); ac++;
			XtSetArg(args[ac] , XmNrecomputeSize ,	True				); ac++;
			// provide a string that gives the Widget its correct size:
			adViolationCountLabel = new motif_widget("0" ,
				xmLabelWidgetClass ,		_activityDisplayForm ,
				args ,				ac ,
				TRUE); }

		ac = 0;
		XtSetArg(args[ac] , XmNbackground,	ResourceData.peach_dark_color	); ac++;
		XtSetArg(args[ac] , XmNforeground,	ResourceData.black_color	); ac++;
		XtSetArg(args[ac] , XmNfontList ,	fontList			); ac++;
		XtSetArg(args[ac] , XmNrecomputeSize ,	False				); ac++;
		// provide a string that gives the Widget its correct size:
		adPointerTimeLabel = new motif_widget("00-001T00:00:00.003 plus extra " ,
			xmLabelWidgetClass ,		_activityDisplayForm ,
			args ,				ac ,
			TRUE);
		// 23 spaces to allow for time display:
		*adPointerTimeLabel = "";

		if(activitySystem->is_a_hopper()) {
			*_activityDisplayForm ^ *adPointerTimeLabel ^ endform;
			*_activityDisplayForm < 2 < *adPointerTimeLabel < endform; }
		else {
			*_activityDisplayForm ^ *adTimeSystemLabel ^ *adPointerTimeLabel ^ *adViolationLabel ^ endform;
			*adPointerTimeLabel ^ *adViolationCountLabel ^ endform;
			*_activityDisplayForm < 2 < *adPointerTimeLabel < endform;
			*_activityDisplayForm < 2 < *adTimeSystemLabel < endform;
			*_activityDisplayForm < 2 < *adViolationLabel < 5 < *adViolationCountLabel < endform; }
		// XmFontListEntryCreate("tag1" , XmFONT_IS_FONT , ResourceData.legend_font);
		// XmFontListAppendEntry(NULL , entry);
		XmFontListFree(fontList);
		XmFontListEntryFree(&entry);
		}
	}
#endif
	}

void	activity_display::adjust_time_system_label(const Cstring &new_time_label) {
	// Hopper design DONE: remove this; no change allowed!
	if(adTimeSystemLabel) {
		*adTimeSystemLabel = new_time_label; } }

int	activity_display::get_time_system_label(Cstring &theLabel) {
	if(adTimeSystemLabel) {
		theLabel = adTimeSystemLabel->get_text();
		return 1; }
	return 0; }

void	activity_display::update_pointer_time_label(CTime theNewTime) {
	*adPointerTimeLabel = theNewTime.to_string(); }

void	activity_display::update_time_system_label() {}
