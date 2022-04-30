#if HAVE_CONFIG_H
#include <config.h>
#endif
/*	PROJECT		: APGEN
 *	SUBSYSTEM	: UI (non-BX Motif)
 *	FILE NAME	: UI_motif_widget.C
 *
 *
 *	ABSTRACT:
 *	
 *	Code file for Pierre's Motif wrapper classes, adapted from SEQ_REVIEW
 *
 */

// #define GUI

#include "APdata.H"
#include "RES_exceptions.H"
#include "UI_motif_widget.H"
#include "UI_GeneralMotifInclude.H"
#include "apcoreWaiter.H"

#ifdef GUI
	// for global method AllowWindowClosingFromMotifMenu():
#	include <Xm/AtomMgr.h>
#	include <Xm/Protocols.h>

	// STATICS:
	//   xmTopLevelWidgetClass of method motif_widget::construct_widget())

	static int		sn = 0;
	static Arg		sargs[10];
#else
	static int		sn = 0;
	static void*		sargs[10];
#endif /* ifdef GUI */

using namespace std;

			// main window:
int			GUI_Flag = 1;
extern motif_widget*	MW;		//defined and used in main.C
extern const char*	get_apgen_version_build_platform();
int			iconic_flag = 0;

// GLOBALS:

resource_data ResourceData;

#ifdef GUI

	//97-12-08 DSG:  (char*) silliness keeps HP10 aCC quiet:
    XtResource	motif_resources[19] = {
	{ (char*)"textBackgroundColor", (char*)"TextBackgroundColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, grey_color),
                XmRString, (char *) "Grey80"
                },
	{ (char*)"redColor", (char*)"RedColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, red_color),
                XmRString, (char *) "Red"
                },
	{ (char*)"greenColor", (char*)"GreenColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, green_color),
                XmRString, (char *) "Green"
                },
	{ (char*)"blackColor", (char*)"BlackColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, black_color),
                XmRString, (char *) "Black"
                },
	{ (char*)"displayBackgroundColor", (char*)"DisplayBackgroundColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, papaya_color),
                XmRString, (char *) "PapayaWhip"
                },
	{ (char*)"legendTimelineBackgroundColor", (char*)"LegendTimelineBackgroundColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, peach_color),
                XmRString, (char *) "PeachPuff1"
                },
	{ (char*)"timeSystemBackgroundColor", (char*)"TimeSystemBackgroundColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, peach_dark_color),
                XmRString, (char *) "PeachPuff2"
                },
	{ (char*)"constraintBackgroundColor", (char*)"ConstraintBackgroundColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, white_color),
                XmRString, (char *) "White"
                },
	{ (char*)"resourceOKFillColor", (char*)"ResourceOKFillColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, ok_color),
                XmRString, (char *) "LightSeaGreen"
                },
	{ (char*)"resourceFUZZYFillColor", (char*)"ResourceFUZZYFillColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, fuzzy_color),
                XmRString, (char *) "Yellow"
                },
	{ (char*)"resourceTELEMETRYFillColor", (char*)"ResourceTELEMETRYFillColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, telemetry_color),
                XmRString, (char *) "Blue"
                },
	{ (char*)"resourceNotOKFillColor", (char*)"ResourceNotOKFillColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, not_ok_color),
                XmRString, (char *) "Red"
                },
	{ (char*)"resourceOverMaximumFillColor", (char*)"ResourceOverMaximumFillColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, over_max_color),
                XmRString, (char *) "Red"
                },
	{ (char*)"resourceUnderMinimumFillColor", (char*)"ResourceUnderMinimumFillColor", XtRPixel,
                sizeof(Pixel), XtOffsetOf(resource_data, under_min_color),
                XmRString, (char *) "Red"
                },
	{ (char*)"mainFont", (char*)"MainFont", XtRFontStruct,
                sizeof(XFontStruct*), XtOffsetOf(resource_data, main_font),
                XtRString, (char *) "-*-helvetica-medium-r-*-*-*-140-*-*-*-*-iso8859-1"
                },
	{ (char*)"secondaryFont", (char*)"SecondaryFont", XtRFontStruct,
                sizeof(XFontStruct*), XtOffsetOf(resource_data, secondary_font),
                XtRString, (char *) "-*-helvetica-medium-r-*-*-*-140-*-*-*-*-iso8859-1"
                },
	{ (char*)"time_and_act_Font", (char*)"Time_and_act_Font", XtRFontStruct,
                sizeof(XFontStruct*), XtOffsetOf(resource_data, time_and_act_font),
                XtRString, (char *) "*misc*fixed-medium-r-semicondensed*13*"
                },
	{ (char*)"legend_Font", (char*)"Legend_Font", XtRFontStruct,
                sizeof(XFontStruct*), XtOffsetOf(resource_data, legend_font),
                XtRString, (char *) "-*-times-medium-r-*-*-*-140-*-*-*-*-*-*"
                },
	{ (char*)"smallerFont", (char*)"SmallerFont", XtRFontStruct,
                sizeof(XFontStruct*), XtOffsetOf(resource_data, small_font),
                XtRString, (char *) "-*-helvetica-medium-r-*-*-*-120-*-*-*-*-iso8859-1"
                } };
#endif	/* ifdef GUI */

END_OP endform;

// #define DEBUG_MOTIF

#ifdef DEBUG_MOTIF
	int	lev = 0, L;
#endif


			// in main.C:
extern motif_widget	*TopLevelWidget;	//DSG 96-05-08 define in main.C
#ifdef GUI
	extern XtAppContext	Context;
#endif
extern "C" {
  extern int		ARGC;
extern char		**ARGV;
};

			// in some BX utility program:
extern XtPointer	CONVERT(Widget, const char *, const char *, int, Boolean *);

WidgetClass xmScrolledListWidgetClass		= (WidgetClass) "sr_ScrolledList";
WidgetClass xmScrolledTextWidgetClass		= (WidgetClass) "sr_ScrolledText";
WidgetClass xmRadioBoxWidgetClass		= (WidgetClass) "sr_RadioBox";
WidgetClass xmCheckBoxWidgetClass		= (WidgetClass) "sr_CheckBox";
WidgetClass xmOptionMenuWidgetClass		= (WidgetClass) "sr_OptionMenu";
WidgetClass xmPulldownMenuWidgetClass		= (WidgetClass) "sr_PulldownMenu";
WidgetClass xmTopLevelWidgetClass		= (WidgetClass) "sr_TopLevel";
WidgetClass xmFileSelectionDialogWidgetClass	= (WidgetClass) "sr_FileSelectionDialog";
WidgetClass xmSelectionDialogWidgetClass	= (WidgetClass) "sr_SelectionDialog";
WidgetClass xmPromptDialogWidgetClass		= (WidgetClass) "sr_PromptDialog";
WidgetClass xmPopupShellWidgetClass		= (WidgetClass) "sr_PopupShell";
WidgetClass xmMenubarWidgetClass		= (WidgetClass) "sr_Menubar";
WidgetClass xmPopupMenuWidgetClass		= (WidgetClass) "sr_PopupMenu";
WidgetClass xmFormDialogWidgetClass		= (WidgetClass) "sr_FormDialog";

#ifndef GUI

	WidgetClass xmTextWidgetClass			= (WidgetClass) "sr_Text";
	WidgetClass xmFormWidgetClass			= (WidgetClass) "sr_Form";
	WidgetClass xmLabelWidgetClass			= (WidgetClass) "sr_Label";
	WidgetClass xmDialogShellWidgetClass		= (WidgetClass) "sr_DialogShell";
	WidgetClass xmMenuShellWidgetClass		= (WidgetClass) "sr_MenuShell";
	//ALL below were previously "sr_PanedWin" (no "dow"!), surely this was an
	//  editing glitch or an oversight?
	WidgetClass xmPanedWindowWidgetClass		= (WidgetClass) "sr_PanedWindow";
	WidgetClass xmPushButtonWidgetClass		= (WidgetClass) "sr_PushButton";
	WidgetClass xmDrawingAreaWidgetClass		= (WidgetClass) "sr_DrawingArea";
	WidgetClass xmScrolledWindowWidgetClass		= (WidgetClass) "sr_ScrolledWindow";
	WidgetClass xmToggleButtonWidgetClass		= (WidgetClass) "sr_ToggleButton";
	WidgetClass xmScaleWidgetClass			= (WidgetClass) "sr_Scale";
	WidgetClass xmFrameWidgetClass			= (WidgetClass) "sr_Frame";
	WidgetClass xmScrollBarWidgetClass		= (WidgetClass) "sr_ScrollBar";
	WidgetClass xmSeparatorWidgetClass		= (WidgetClass) "sr_Separator";
	WidgetClass xmRowColumnWidgetClass		= (WidgetClass) "sr_RowColumn";
	WidgetClass xmCascadeButtonWidgetClass		= (WidgetClass) "sr_CascadeButton";
	WidgetClass xmMainWindowWidgetClass		= (WidgetClass) "sr_MainWindow";

	long ExposureMask = 1;
	long StructureNotifyMask =2;
	long EnterWindowMask = 4;
	long LeaveWindowMask = 8;

	char		*XtName(_WidgetRec*) { return "undefined"; }

	const char	*XmNactivateCallback = "HI";
	const char	*XmNmenuHistory = "HI";
	const char	*XmNvalueChangedCallback = "HI";
	const char	*XmNsingleSelectionCallback = "HI";
	const char	*XmNlosingFocusCallback = "HI";
	const char	*XmNokCallback = "HI";
	const char	*XmNbrowseSelectionCallback = "HI";
	const char	*XmNdefaultActionCallback = "HI";

#endif	/* ifndef GUI */

static Cstring		set_string("SET"), not_set_string("NOT SET");

#ifdef GUI
				// in main.C:
	extern String		fallbacks[];

#endif	/* ifdef GUI */

// END OF STATICS

			// for resize_scrolled_list_widget:
blist			widget_indexed_list_of_scrolled_list_widgets(compare_function(compare_bpointernodes, false));

// GLOBAL METHODS FOLLOW:

void load_resources(motif_widget *W) {
#ifdef GUI
	if(GUI_Flag) {
	XtGetApplicationResources(W->widget, &ResourceData, motif_resources, XtNumber(motif_resources), NULL, 0);
	}
#endif
}

const char* spellEtype(int e) {
	if(e == 2) return "KeyPress";
	if(e == 3) return "KeyRelease";
	if(e == 4) return "ButtonPress";
	if(e == 5) return "ButtonRelease";
	if(e == 6) return "MotionNotify";
	if(e == 7) return "EnterNotify";
	if(e == 8) return "LeaveNotify";
	if(e == 9) return "FocusIn";
	if(e == 10) return "FocusOut";
	if(e == 11) return "KeymapNotify";
	if(e == 12) return "Expose";
	if(e == 13) return "GraphicsExpose";
	if(e == 14) return "NoExpose";
	if(e == 15) return "VisibilityNotify";
	if(e == 16) return "CreateNotify";
	if(e == 17) return "DestroyNotify";
	if(e == 18) return "UnmapNotify";
	if(e == 19) return "MapNotify";
	if(e == 20) return "MapRequest";
	if(e == 21) return "ReparentNotify";
	if(e == 22) return "ConfigureNotify";
	if(e == 23) return "ConfigureRequest";
	if(e == 24) return "GravityNotify";
	if(e == 25) return "ResizeRequest";
	if(e == 26) return "CirculateNotify";
	if(e == 27) return "CirculateRequest";
	if(e == 28) return "PropertyNotify";
	if(e == 29) return "SelectionClear";
	if(e == 30) return "SelectionRequest";
	if(e == 31) return "SelectionNotify";
	if(e == 32) return "ColormapNotify";
	if(e == 33) return "ClientMessage";
	if(e == 34) return "MappingNotify";
	return "Unknown event"; }


void fix_action_area_height(Widget w, callback_stuff *client_data, void *) {
#ifdef GUI
    if(GUI_Flag) {
#ifdef HAVE_MACOS
	Dimension h;
	Widget OneOfTheButtons = (Widget) client_data->data;

	// if(! OneOfTheButtons) OneOfTheButtons = w;
	if(OneOfTheButtons) {
	
		XtVaGetValues(OneOfTheButtons, XmNheight, &h, NULL);
#		ifdef DEBUG_MOTIF
			for(L = 0; L < lev; L++) cerr << "  ";
			cerr << "fix_action_area_height(... callback_stuff * ...): widget \"" << XtName(w)
				<< "\", height " << h << "...\n";
#		endif

		// debug
		// cerr << "fix_action_area_height: " << XtName(w) << " wishes for height = "
		// 	<< client_data->height << "; actual widget height is " << h << "\n";
		XtVaSetValues(	w,
				XmNpaneMaximum, (Dimension) client_data->height,
				XmNpaneMinimum, (Dimension) client_data->height,
				NULL);
	  	XtRemoveEventHandler(
			w,
			StructureNotifyMask,
			FALSE,
			(XtEventHandler) fix_action_area_height,
			(void *) client_data);
		}
#else
	  Dimension h;
	  Widget OneOfTheButtons = (Widget) client_data->data;
	  
	  if(! OneOfTheButtons) OneOfTheButtons = w;
	  XtVaGetValues(OneOfTheButtons, XmNheight, &h, NULL);
#	ifdef DEBUG_MOTIF
	  for(L = 0; L < lev; L++) cerr << "  ";
	  cerr << "fix_action_area_height(... callback_stuff * ...): widget \"" << XtName(w)
	       << "\", height " << h << "...\n";
#	endif
	  XtVaSetValues(w,
			 XmNpaneMaximum, h,
			 XmNpaneMinimum, h,
		NULL);
	  XtRemoveEventHandler(
			       w,
			       StructureNotifyMask,
			       FALSE,
		(XtEventHandler) fix_action_area_height,
			       (void *) client_data);
#endif /* HAVE_MACOS */
	}
#endif /* GUI */
}

int sr_is_composite(WidgetClass c) {
  //ADD widget class names here, for ALL composite widgets:
  if((c == xmScrolledListWidgetClass)
      || (c == xmScrolledTextWidgetClass))
    return 1;
  return 0; }

// The following global method makes it possible for the user to close
// a window using the standard menu at the top left of the window:

void AllowWindowClosingFromMotifMenu(
				     Widget		w,
				     callback_proc	proc_to_call_when_closing_window,
				     void		*client_data) {
#ifdef GUI
  if(GUI_Flag) {
    // Motif manual p. 581
    Widget shell;
    Atom WM_DELETE_WINDOW;
    
    if(XtIsVendorShell(w))
      shell = w;
    else
      shell = XtParent(w);
    
    WM_DELETE_WINDOW = XmInternAtom(XtDisplay(w), (char *) "WM_DELETE_WINDOW", False);
    XmAddWMProtocolCallback(
			    shell,
			    WM_DELETE_WINDOW,
			    (XtCallbackProc) proc_to_call_when_closing_window,
			    client_data);
  }
#endif
}

void SetLabelToStringContainingNewline(Widget w, char *s) {
#ifdef GUI
  if(GUI_Flag) {
    XmFontListEntry	entry = XmFontListEntryCreate((char *) "tag1", XmFONT_IS_FONT, ResourceData.main_font);
	XmFontList	fontList = XmFontListAppendEntry(NULL, entry);
	
	XtFree((char *) entry);
	XmString str = XmStringCreateLtoR(
		s,
		(char *) "tag1");
	XtVaSetValues(w,
		XmNfontList,                   fontList,
		XmNlabelString,                str,
		NULL);
	XmFontListFree(fontList);
	XmStringFree(str);
	}
#endif
}

Cstring class_to_string(const WidgetClass &MC) {
	Cstring	temp;

#ifdef GUI
	if(GUI_Flag) {
		if(MC == applicationShellWidgetClass) temp = "applicationShellWidgetClass";
		else if(MC == topLevelShellWidgetClass) temp = "topLevelShellWidgetClass";
		else if(MC == transientShellWidgetClass) temp = "transientShellWidgetClass";
		else if(MC == xmArrowButtonWidgetClass) temp = "xmArrowButtonWidgetClass";
		else if(MC == xmCascadeButtonWidgetClass) temp = "xmCascadeButtonWidgetClass";
		else if(MC == xmDialogShellWidgetClass) temp = "xmDialogShellWidgetClass";
		else if(MC == xmDrawingAreaWidgetClass) temp = "xmDrawingAreaWidgetClass";
		else if(MC == xmFileSelectionBoxWidgetClass) temp = "xmFileSelectionBoxWidgetClass";
		else if(MC == xmFileSelectionDialogWidgetClass) temp = "xmFileSelectionDialogWidgetClass";
		else if(MC == xmFormDialogWidgetClass) temp = "xmFormDialogWidgetClass";
		else if(MC == xmFormWidgetClass) temp = "xmFormWidgetClass";
		else if(MC == xmFrameWidgetClass) temp = "xmFrameWidgetClass";
		else if(MC == xmLabelWidgetClass) temp = "xmLabelWidgetClass";
		else if(MC == xmMainWindowWidgetClass) temp = "xmMainWindowWidgetClass";
		else if(MC == xmMenubarWidgetClass) temp = "xmMenubarWidgetClass";
		else if(MC == xmOptionMenuWidgetClass) temp = "xmOptionMenuWidgetClass";
		else if(MC == xmPanedWindowWidgetClass ) temp = "xmPanedWindowWidgetClass ";
		else if(MC == xmPopupMenuWidgetClass) temp = "xmPopupMenuWidgetClass";
		else if(MC == xmPopupShellWidgetClass) temp = "xmPopupShellWidgetClass";
		else if(MC == xmPromptDialogWidgetClass) temp = "xmPromptDialogWidgetClass";
		else if(MC == xmPulldownMenuWidgetClass) temp = "xmPulldownMenuWidgetClass";
		else if(MC == xmPushButtonWidgetClass) temp = "xmPushButtonWidgetClass";
		else if(MC == xmRadioBoxWidgetClass) temp = "xmRadioBoxWidgetClass";
		else if(MC == xmCheckBoxWidgetClass) temp = "xmRadioBoxWidgetClass";
		else if(MC == xmRowColumnWidgetClass) temp = "xmRowColumnWidgetClass";
		else if(MC == xmScaleWidgetClass) temp = "xmScaleWidgetClass";
		else if(MC == xmScrollBarWidgetClass) temp = "xmScrollBarWidgetClass";
		else if(MC == xmScrolledListWidgetClass) temp = "xmScrolledListWidgetClass";
		else if(MC == xmScrolledWindowWidgetClass) temp = "xmScrolledWindowWidgetClass";
		else if(MC == xmSelectionDialogWidgetClass) temp = "xmSelectionDialogWidgetClass";
		else if(MC == xmSeparatorWidgetClass) temp = "xmSeparatorWidgetClass";
		else if(MC == xmTextFieldWidgetClass) temp = "xmTextFieldWidgetClass";
		else if(MC == xmTextWidgetClass) temp = "xmTextWidgetClass";
		else if(MC == xmToggleButtonWidgetClass) temp = "xmToggleButtonWidgetClass";
		else if(MC == xmTopLevelWidgetClass) temp = "xmTopLevelWidgetClass";
		else  temp = "Unknown motif class";
	}
#endif
	return temp; }

//CONSTRUCTORS, DESTRUCTOR, and CLASS METHODS:


motif_widget::motif_widget(motif_widget *prnt)
	: widget(NULL),
	ref_node(""),
	RightOffset(0),
	BottomOffset(0),
	IsChildOfAForm(0),
	myParent(prnt),
	managedState(false)
	{}

motif_widget::motif_widget(motif_widget *parnt, Widget w, const WidgetClass wc)
	: ref_node(XtName(w)),
	widget(w),
	RightOffset(0),
	BottomOffset(0),
	IsChildOfAForm(0),
	MotifClass(wc),
	myParent(parnt),
	managedState(false)
	{}

motif_widget::motif_widget(const motif_widget & w)
	: ref_node(w),
	widget(w.widget),
	RightOffset(w.RightOffset),
	BottomOffset(w.BottomOffset),
	IsChildOfAForm(w.IsChildOfAForm),
	myParent(w.myParent),
	managedState(false)
	{}

// Last arg. in following func. defaults to 1:
motif_widget::motif_widget(
	const Cstring		&widget_name,
	motif_widget		*widget_parent,
	const form_position	&P,
	bool			should_manage)
	: ref_node(widget_name),
	MotifClass(xmFormWidgetClass),
	RightOffset(0),
	BottomOffset(0),
	IsChildOfAForm(0),
	myParent(widget_parent),
	managedState(should_manage) {
#ifdef GUI
	if(GUI_Flag) {
	widget = XtVaCreateWidget(
		widget_name(),
		xmFormWidgetClass,	widget_parent->widget,
		XmNfractionBase,	P.pos,
		NULL
		);
#	ifdef DEBUG_MOTIF
		cerr << "motif_widget::motif_widget(form_position): creating form \"" << XtName(widget) << "\", should_manage: " <<
			should_manage << "...\n";
#	endif
	if(should_manage) {
#		ifdef DEBUG_MOTIF
		cerr << "motif_widget::motif_widget: managing \"" << XtName(widget) << "\"...\n";
#		endif
		XtManageChild(widget); }
	if(widget_parent) {
#		ifdef DEBUG_MOTIF
		cerr << "motif_widget::motif_widget: adding child to parent \"" << XtName(widget_parent->widget) <<
			"\" which now has " << (widget_parent->children.get_length() + 1) << " child(ren)...\n";
#		endif
		widget_parent->children << this; }
#	ifdef DEBUG_MOTIF
	else {
		cerr << "motif_widget::motif_widget: \"" << XtName(widget) << "\" HAS NO PARENT\n"; }
#	endif
	}
#endif
}

motif_widget::motif_widget(
	const Cstring		&widget_name,
	const WidgetClass	motif_class,
	motif_widget		*widget_parent)
	: ref_node(widget_name),
	MotifClass(motif_class),
	RightOffset(0),
	BottomOffset(0),
	IsChildOfAForm(0),
	myParent(widget_parent),
	managedState(true) {
	construct_widget(widget_parent, NULL, 0, true); }

// Last arg. in following func. defaults to 0:

motif_widget::motif_widget(
	const Cstring		&widget_name,
	const WidgetClass	motif_class,
	motif_widget		*parent,
	void			*A,
	int			n,
	bool			manage)
	: ref_node(widget_name),
	MotifClass(motif_class),
	RightOffset(0),
	BottomOffset(0),
	IsChildOfAForm(0),
	myParent(parent),
	managedState(manage) {
	construct_widget(parent, A, n, manage);
#	ifdef DEBUG_MOTIF
	cerr << "motif_widget::motif_widget: DONE creating \"" << XtName(widget) << "\"...\n";
#	endif
	}

motif_widget::motif_widget(
	void			(*setargs) (void*, int&),
	const Cstring&		widget_name,
	const WidgetClass	motif_class,
	motif_widget*		parent,
	bool			manage)
	: ref_node(widget_name),
	MotifClass(motif_class),
	RightOffset(0),
	BottomOffset(0),
	IsChildOfAForm(0),
	myParent(parent),
	managedState(manage) {
#	ifdef GUI
	if(GUI_Flag) {
	  sn = 0;
	setargs(sargs, sn);
	}
#	endif
	construct_widget(parent, sargs, sn, manage);
#	ifdef DEBUG_MOTIF
	cerr << "motif_widget::motif_widget: DONE creating \"" << XtName(widget) << "\"...\n";
#	endif
	}

void motif_widget::set_menu_position(void* e) {
#ifdef GUI
	if(GUI_Flag) {
	XEvent		*event = (XEvent *) e;

	XmMenuPosition(widget, (XButtonPressedEvent *) event);
	}
#endif
}

void motif_widget::construct_widget(motif_widget *widget_parent, void *args, int num_args, bool should_manage) {
#ifdef GUI
	if(GUI_Flag) {
	if(MotifClass == xmTopLevelWidgetClass) {
		if(iconic_flag) {
	                widget = XtVaAppInitialize(
	                        &Context,
	                        (char *) *Substring,			//	Name of resource file is defined here
	                        NULL,			0,
	                        &ARGC,			ARGV,
				fallbacks,
	                        XmNtitle,		(char*) get_apgen_version_build_platform(),
	                        XmNiconic,		True,
	                        XmNdeleteResponse,	XmDO_NOTHING,
	                        NULL); }
	        else
	                widget = XtVaAppInitialize(
	                        &Context,
	                        (char *) *Substring,		//      Name of resource file is defined here
	                        // options,		XtNumber(options),
				NULL,			0,
	                        & ARGC,		ARGV,
	                        fallbacks,
	                        XmNtitle,              (char*) get_apgen_version_build_platform(),
	                        XmNdeleteResponse,     XmDO_NOTHING,
	                        NULL);
		}
	else if(MotifClass == xmMainWindowWidgetClass) {
		Arg	*motif_args = (Arg *) args;

		if(! num_args)
			widget = XtVaCreateWidget(
				(char *) *Substring,
				MotifClass,			widget_parent->widget,
				// XmNcommandWindowLocation,	XmCOMMAND_BELOW_WORKSPACE,
				NULL);
		else {
			// XtSetArg(motif_args[num_args], XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE); num_args++;
			widget = XtCreateWidget(
				(char *) *Substring,
				MotifClass,		widget_parent->widget,
				motif_args,		num_args
				); } }
	else if(MotifClass == xmDialogShellWidgetClass) {
		Position	x, y;
		Arg		*this_arg = (Arg *) args;
		int		N = num_args;

		if(!this_arg) {
			this_arg = sargs;
			N = 0; }

		XtVaGetValues(MW->widget, XmNx, &x, XmNy, &y, NULL);

                XtSetArg(this_arg[N], XmNx,                      x + 100); N++;
                XtSetArg(this_arg[N], XmNy,                      y); N++;
                XtSetArg(this_arg[N], XmNallowShellResize,       True); N++;
                XtSetArg(this_arg[N], XmNdeleteResponse,         XmDO_NOTHING); N++;
		if(! widget_parent) {
	                widget = XmCreateDialogShell(MW->widget, (char *) *Substring, this_arg, N);
			widget_parent = MW; }
		else
	                widget = XmCreateDialogShell(widget_parent->widget, (char *) *Substring, this_arg, N); }
	else if(MotifClass == xmPromptDialogWidgetClass) {
		Position x, y;

		XtVaGetValues(MW->widget, XmNx, &x, XmNy, &y, NULL);

		sn = 0;
		XtSetArg(sargs[sn], XmNx,                      x + 100); sn++;
		XtSetArg(sargs[sn], XmNy,                      y); sn++;
		XtSetArg(sargs[sn], XmNallowShellResize,       True); sn++;
		XtSetArg(sargs[sn], XmNautoUnmanage,           False); sn++;
		XtSetArg(sargs[sn], XmNdeleteResponse,         XmDO_NOTHING); sn++;
		widget = XmCreatePromptDialog(MW->widget, (char *) *Substring, sargs, sn);
		widget_parent = MW; }
	else if(MotifClass == xmPopupShellWidgetClass) {
		if(widget_parent == MW) {	// ONLY used by UI_openfsd.C
			Position	x, y;

			XtVaGetValues(MW->widget, XmNx, &x, XmNy, &y, NULL);
			widget = XtVaCreatePopupShell((char *) *Substring,
				transientShellWidgetClass,	widget_parent->widget,
				XmNwidth,				410,
				XmNheight,				640,
				XmNx,					x + 20,
				XmNy,					y + 20,
				// XmNdeleteResponse,		XmDO_NOTHING,
				NULL); }
		else {							// used by Plot_window
			Cstring save(Substring);

#			ifdef DEBUG_MOTIF
			cerr << "construct_widget: creating topLevelShell for Plot window \"" << save << "\"...\n";
#			endif
			Substring = Cstring("PLOT window");
			sn = 0;
			XtSetArg(sargs[sn], XmNdeleteResponse, XmDO_NOTHING); sn++;
			XtSetArg(sargs[sn], XmNtitle, save()); sn++;
			XtSetArg(sargs[sn], XmNallowShellResize, True); sn++;
			XtSetArg(sargs[sn], XmNwidth, 600); sn++;
			if(iconic_flag) {
				XtSetArg(sargs[ sn ], XmNiconic, True); sn++; }
			else {
				XtSetArg(sargs[ sn ], XmNiconic, False); sn++; }
			widget = XtCreatePopupShell((char *) *Substring, topLevelShellWidgetClass,
                        	TopLevelWidget->widget, sargs, sn);
			widget_parent = TopLevelWidget; } }
	else if(MotifClass == xmPopupMenuWidgetClass) {
		// In seq_review, a menu pops up when the 3rd button is pushed
		// in a state panel DrawingArea widget. Without the line, it
		// was IMPOSSIBLE to get the callback to work!!
		// (Thanks to Dang Le for pointing out the fix):
		XtSetArg(sargs[0], XmNpopupEnabled, False);
		widget = XmCreatePopupMenu(widget_parent->widget,
			(char *) *Substring,
			sargs, 1); }
	else if(MotifClass == xmFileSelectionDialogWidgetClass) {
		widget = XmCreateFileSelectionDialog(widget_parent->widget,
			(char *) *Substring, (Arg *) args, num_args); }
	else if(MotifClass == xmFileSelectionBoxWidgetClass) {
		widget = XmCreateFileSelectionBox(widget_parent->widget,
			(char *) *Substring, (Arg *) args, num_args); }
	else if(MotifClass == xmSelectionDialogWidgetClass) {
		widget = XmCreateSelectionDialog(widget_parent->widget,
			(char *) *Substring,	// prompt
			(Arg *) args,
			num_args); }
	else if(MotifClass == xmPanedWindowWidgetClass) {
		if(! num_args)
			widget = XtVaCreateWidget(
				(char *) *Substring,
				MotifClass,			widget_parent->widget,
				XmNallowResize,		True,
				NULL);
		else
			{
			XtSetArg(((Arg *) args)[num_args], XmNallowResize, True);
			num_args++;
			widget = XtCreateWidget(
				(char *) *Substring,
				MotifClass,		widget_parent->widget,
				(Arg *) args,	num_args
				); } }
	else if(MotifClass == xmSeparatorWidgetClass) {
		widget = XmCreateSeparator(widget_parent->widget, (char *) *Substring, (Arg *) args, num_args); }
	else if(MotifClass == xmOptionMenuWidgetClass) {
		widget = XmCreateOptionMenu(widget_parent->widget, (char *) *Substring, (Arg *) args, num_args); }
	else if(MotifClass == xmPulldownMenuWidgetClass) {
		widget = XmCreatePulldownMenu(widget_parent->widget, (char *) *Substring, (Arg *) args, num_args); }
	else if(MotifClass == xmMenubarWidgetClass) {
		widget = XmCreateMenuBar( widget_parent->widget, (char *) *Substring, (Arg *) args, num_args); }
	else if(MotifClass == xmScrolledListWidgetClass) {
		widget = XmCreateScrolledList(widget_parent->widget, (char *) *Substring, (Arg *) args, num_args); }
	else if(MotifClass == xmScrolledTextWidgetClass) {
			widget = XmCreateScrolledText(widget_parent->widget,
					(char *) *Substring, (Arg *) args, num_args); }
	else if(MotifClass == xmRadioBoxWidgetClass) {
		widget = XmCreateRadioBox(widget_parent->widget, (char *) *Substring, (Arg *) args, num_args); }
	else if(MotifClass == xmCheckBoxWidgetClass) {
		widget = XmCreateSimpleCheckBox(widget_parent->widget, (char *) *Substring, (Arg *) args, num_args); }
	else {
		widget = XtCreateWidget(
			(char *) *Substring,
			MotifClass,		widget_parent->widget,
			(Arg *) args,	num_args
			); }

#	ifdef DEBUG_MOTIF
		cerr << "motif_widget::construct_widget: creating " << class_to_string(MotifClass) << " \"" <<
			XtName(widget) << "\" @ " << (void *) this << "...\n";
#	endif

	if(should_manage && MotifClass != xmPulldownMenuWidgetClass) {
#		ifdef DEBUG_MOTIF
			cerr << "motif_widget::construct_widget: XtManageChild called for \"" <<
				XtName(widget) << "\"...\n";
#		endif
		XtManageChild(widget); }
	}
#endif

	if(widget_parent) {
#		ifdef DEBUG_MOTIF
			cerr << "motif_widget::construct_widget: adding child to parent \"" << XtName(widget_parent->widget) <<
			"\" which now has " << (widget_parent->children.get_length() + 1) << " child(ren)...\n";
#		endif
		widget_parent->children << this; }

#ifdef GUI
	if(GUI_Flag) {
#	ifdef DEBUG_MOTIF
	  cerr << "motif_widget::construct_widget: WIDGET \"" << Substring << "\" HAS NO PARENT\n";

	  cerr << "                     xmWidgetClass \"" << class_to_string(MotifClass) << "\"\n";
	  cerr << "                     WINDOW at " << XtWindowOfObject(widget) << endl;
#	endif

	if(		MotifClass == xmTextWidgetClass ||
			MotifClass == xmTextFieldWidgetClass) {
		XtVaSetValues(widget, XmNbackground, ResourceData.grey_color, NULL);
		XtVaSetValues(widget, XmNeditable, False, NULL); }
	else if(MotifClass == xmScrolledListWidgetClass ||
		 MotifClass == xmScrolledTextWidgetClass) {
		Widget	scroll_bar;

		XtVaGetValues(XtParent(widget), XmNverticalScrollBar, & scroll_bar, NULL);
		if(scroll_bar) XmChangeColor(scroll_bar, ResourceData.grey_color);
		XtVaGetValues(XtParent(widget), XmNhorizontalScrollBar, & scroll_bar, NULL);
		if(scroll_bar) XmChangeColor(scroll_bar, ResourceData.grey_color);
		XtVaSetValues(widget, XmNbackground, ResourceData.grey_color, NULL); }
	else if(MotifClass == xmListWidgetClass) {
		XtVaSetValues(widget, XmNbackground, ResourceData.grey_color, NULL); }
	}
#endif
}

motif_widget::~motif_widget()
	{}

void motif_widget::remove_widgets() {
	motif_widget * N;
#ifdef DEBUG_MOTIF
lev++; for(L = 0; L < lev; L++) cerr << "  ";
cerr << "motif_widget::remove_widgets: widget \"" << Substring << "\" removing widgets from " <<
	children.get_length() << " child(ren)...\n";
#endif

	for(	N = (motif_widget *) children.first_node();
		N;
		N = (motif_widget *) N->next_node())
		N->remove_widgets();
	widget = NULL;
#ifdef DEBUG_MOTIF
for(L = 0; L < lev; L++) cerr << "  ";
cerr << "motif_widget::remove_widgets: done removing widgets for widget \"" << Substring << "\"...\n";
lev--;
#endif
	}

void motif_widget::destroy_widgets(int level) {
	motif_widget	* N;
	List_iterator	C(children);

	detach_callbacks_and_event_handlers();
#ifdef DEBUG_MOTIF
	lev++; for(L = 0; L < lev; L++) cerr << "  ";
	cerr << Substring << "->destroy_widgets(" << level << "): done detaching...\n";
#endif
	if(level)
		{
		while((N = (motif_widget *) C()))
			N->destroy_widgets(level - 1);
		}
	else if(widget)
		{
#ifdef DEBUG_MOTIF
	for(L = 0; L < lev; L++) cerr << "  ";
	cerr << "destroy_widgets: destroying widget \"" << Substring << "\"...\n";
#endif
#ifdef GUI
	if(GUI_Flag) {
		XtDestroyWidget(widget);
	}
#endif
		// Make sure no one uses the children widgets since they
		// are in the process of being demolished:
		remove_widgets();
		}
#ifdef DEBUG_MOTIF
	cerr << Substring << "->destroy_widgets: DONE\n";
	lev--;
#endif
	}

void	theDefaultUniversalCallback(Widget w, callback_stuff* CBS, void* V) {
	udef_intfc::something_happened() += 1;

	try {
		CBS->function(w, CBS, V);
	}
	catch(eval_error E1) {
		cerr << "A low-level evaluation error occurred. This error should have been handled"
			" at a higher level, but was not; this is a deficiency of the Apgen software. Please"
			" send a copy of apgen.log together with any files you read to the Apgen Cog. E. for"
			" further analysis and corrective action. Proceed with caution: save while you are"
			" ahead and prepare to die. FYI, here is the low-level error message:\n";
		cerr << E1.msg << endl;
	}
}

void motif_widget::detach_callbacks_and_event_handlers() {
	motif_widget*	N;

#	ifdef DEBUG_MOTIF
	lev++; for(L = 0; L < lev; L++) cerr << "  ";
	cerr << "detach_callbacks_and_event...: detaching callbacks for \"" << Substring <<
		"\" and " << children.get_length() << " child(ren)...\n";
#	endif


	for(	N = (motif_widget *) children.first_node();
		N;
		N = (motif_widget *) N->next_node())
		N->detach_callbacks_and_event_handlers();

	callbacks.clear();
#	ifdef DEBUG_MOTIF
	for(L = 0; L < lev; L++) cerr << "  ";
	cerr << "detach_callbacks_and_event_handlers: DONE for \"" << Substring << "\"...\n";
	lev--;
#	endif
	}

void motif_widget::add_callback(callback_proc f, const char *xmnReason, void *user_data) {
#	ifdef GUI
	if(GUI_Flag) {
#	ifdef DEBUG_MOTIF
	cerr << "adding callback func. @ " << (void *) f << " to \"" << Substring << "\", reason \"" <<
		xmnReason << "\", data @ " << (void *) user_data << "...\n";
#	endif
	if(widget) {
		callback_stuff*		CBS = new callback_stuff(this, f, xmnReason, user_data);
		callback_proc		theCallbackToUse = theDefaultUniversalCallback;

#		ifdef DEBUG_MOTIF
		cerr << "    callback_data @ " << (void *) CBS << endl;
#		endif

		XtAddCallback(widget, xmnReason,
			(XtCallbackProc) /* f */ theCallbackToUse,
			CBS);
		callbacks << CBS; }
	}
#endif
}

void motif_widget::add_event_handler(callback_proc f, long mask, void * data) {
#	ifdef GUI
	if(GUI_Flag) {
#	ifdef DEBUG_MOTIF
	cerr << "adding event handler to \"" << Substring << "\", function @ " <<
		(void *) f << ", data @ " << (void *) data;
#	endif
	if(widget) {
		callback_stuff *H = new callback_stuff(this, f, "", data, mask);

		XtAddEventHandler(
			widget,
			mask,
			FALSE,
			(XtEventHandler) f,
			H);
#		ifdef DEBUG_MOTIF
		cerr << ", callback_data @ " << (void *) H << endl;
#		endif
		callbacks << H; }
	}
#endif
}

#ifdef HAVE_MACOS
void motif_widget::add_event_handler(callback_proc f, long mask, int desired_height, void *data) {
#	ifdef GUI
	if(GUI_Flag) {
#	ifdef DEBUG_MOTIF
	cerr << "adding event handler to \"" << Substring << "\", function @ " <<
		(void *) f << ", data @ " << (void *) data;
#	endif
	if(widget) {
		callback_stuff *H = new callback_stuff(
						this, 
						f, 
						"", 
						desired_height, 
						data);

		XtAddEventHandler(
			widget,
			mask,
			FALSE,
			(XtEventHandler) f,
			H);
#		ifdef DEBUG_MOTIF
		cerr << ", callback_data @ " << (void *) H << endl;
#		endif
		callbacks << H; }
	}
#endif
}

#endif /* HAVE_MACOS */
#ifdef GUI

#include <X11/keysym.h>

static int buffersize = 10;
static char buffer[ 10 ];
static KeySym keysym;
static XComposeStatus compose;

void StandardKeyboardAndMouseEventHandler(Widget w, void * client_data, XEvent * event) {
	udef_intfc::something_happened() += 1;

	if(GUI_Flag) {
	callback_stuff * H = (callback_stuff *) client_data;
	int make_the_call = FALSE;
	int button_event = FALSE;
	unsigned char ModifierByte = (event->xbutton.state & 0xFF);

#	ifdef DEBUG_MOTIF
	cerr << "StandardKeyboardAndMouseEventHandler(" << XtName(w) << ")... action = " << H->action
		<< ", state = " << event->xbutton.state << "...\n";
#	endif

	if(event->xany.type == ButtonPress)
		{
		button_event = TRUE;
		switch(H->action)
			{
			case BTTN1_DN:
			case SHFT_BTTN1_DN:
			case CTRL_BTTN1_DN:
			case META_BTTN1_DN:
			case BTTN2_DN:
			case SHFT_BTTN2_DN:
			case CTRL_BTTN2_DN:
			case META_BTTN2_DN:
			case BTTN3_DN:
			case SHFT_BTTN3_DN:
			case CTRL_BTTN3_DN:
			case META_BTTN3_DN:
#				ifdef DEBUG_MOTIF
				cerr << "std. handler: type = button press...\n";
#				endif
				break;
			default:
#				ifdef DEBUG_MOTIF
				cerr << "std. handler: type = button press, action = " << H->action << "... not doing anything.\n";
#				endif
				return;
			}
		}
	else if(event->xany.type == ButtonRelease)
		{
		button_event = TRUE;
		switch(H->action)
			{
			case BTTN2_UP:
			case BTTN3_UP:
			case SHFT_BTTN2_UP:
			case SHFT_BTTN3_UP:
			case CTRL_BTTN2_UP:
			case CTRL_BTTN3_UP:
			case META_BTTN2_UP:
			case META_BTTN3_UP:
#				ifdef DEBUG_MOTIF
				cerr << "std. handler: type = button release...\n";
#				endif
				break;
			default:
#				ifdef DEBUG_MOTIF
				cerr << "std. handler: type = button release, action = " << H->action << "... not doing anything.\n";
#				endif
				return;
			}
		}
	else if(event->xany.type == KeyPress)
		{
		XKeyEvent * xkey = (XKeyEvent *) event;
		// XLib Programming manual p. 270
		int count = XLookupString(xkey, buffer, buffersize, &keysym, &compose);

		// NOTE: only one modifier will be retained...
		H->modifier = NO_MODIFIER;

		if(ModifierByte & 1)		H->modifier = SHIFT_MODIFIER;
		else if(ModifierByte & 4)	H->modifier = CONTROL_MODIFIER;
		else if(ModifierByte & 8)	H->modifier = ALT_MODIFIER;

		if(keysym == XK_Left)		H->string = "LEFT_ARROW";
		else if(keysym == XK_Up)	H->string = "UP_ARROW";
		else if(keysym == XK_Right)	H->string = "RIGHT_ARROW";
		else if(keysym == XK_Down)	H->string = "DOWN_ARROW";
		else				H->string = (char *) buffer;

		if(H->function)
			H->function(w, (callback_stuff *) client_data, event);
		return;
		}

	if(button_event && (event->xbutton.button == Button1))
		{
#		ifdef DEBUG_MOTIF
		cerr << "std. handler: BUTTON 1\n";
#		endif
		switch(H->action) {
			case SHFT_BTTN1_DN:
				if(ModifierByte & 1)
					make_the_call = TRUE;
				break;
			case CTRL_BTTN1_DN:
				if(ModifierByte & 4)
					make_the_call = TRUE;
				break;
			case META_BTTN1_DN:
				if(ModifierByte & 8)
					make_the_call = TRUE;
				break;
			// modified 11/2/98 PFM
			case BTTN1_DN:
				if((ModifierByte & 13) == 0)
				make_the_call = TRUE;
				break;
			// pacify the compiler
			default:
				break; } }
	else if(button_event && (event->xbutton.button == Button2))
		{
#		ifdef DEBUG_MOTIF
		cerr << "std. handler: BUTTON 2\n";
#		endif
		switch(H->action) {
			case BTTN2_DN:
			case BTTN2_UP:
				if((ModifierByte & 13) == 0)
					make_the_call = TRUE;
				break;
			case SHFT_BTTN2_DN:
			case SHFT_BTTN2_UP:
				if((ModifierByte & 13) == 1)
					make_the_call = TRUE;
				break;
			case CTRL_BTTN2_DN:
			case CTRL_BTTN2_UP:
				if((ModifierByte & 13) == 4)
					make_the_call = TRUE;
				break;
			case META_BTTN2_DN:
			case META_BTTN2_UP:
				if((ModifierByte & 13) == 8)
					make_the_call = TRUE;
				break;
			// pacify the compiler
			default:
				break; }
		}
	else if(button_event && (event->xbutton.button == Button3)) {
#		ifdef DEBUG_MOTIF
		cerr << "std. handler: BUTTON 3\n";
#		endif
		switch(H->action) {
			case BTTN3_DN:
			case BTTN3_UP:
				if((ModifierByte & 13) == 0)
					make_the_call = TRUE;
				break;
			case SHFT_BTTN3_DN:
			case SHFT_BTTN3_UP:
				if((ModifierByte & 13) == 1)
					make_the_call = TRUE;
				break;
			case CTRL_BTTN3_DN:
			case CTRL_BTTN3_UP:
				if((ModifierByte & 13) == 4)
					make_the_call = TRUE;
				break;
			case META_BTTN3_DN:
			case META_BTTN3_UP:
				if((ModifierByte & 13) == 8)
					make_the_call = TRUE;
				break;
			// pacify the compiler
			default:
				break; } }
	else {
#		ifdef DEBUG_MOTIF
		cerr << "std. handler: !? UNKNOWN BUTTON\n";
#		endif
		}
#	ifdef DEBUG_MOTIF
	cerr << "std. handler: make_the_call = " << make_the_call << "...\n";
#	endif
	if(make_the_call && H->function)
		H->function(w, (callback_stuff *) client_data, event); 
}
}
#endif

void motif_widget::add_keyboard_and_mouse_handler(callback_proc f, KBD_AND_MOUSE_ACTION C, MODIFIER M, void * data) {
	if(GUI_Flag) {
	callback_proc g = NULL;

#	ifdef DEBUG_MOTIF
	cerr << "adding kbd_and_mouse event handler to \"" << Substring << "\", function @ " <<
		(void *) f << ", action = " << C << ", data @ " << (void *) data << "...\n";
#	endif
	if(widget) {
		callback_stuff		*H = new callback_stuff(this, f, "", data, C, M);
		long			mask = 0;

		switch(C) {
			case EXPOSE:
				mask = ExposureMask;
				g = f;
				break;
			case RESIZE:
				mask = StructureNotifyMask;
				g = f;
				break;
			case FOCUS:
				mask = EnterWindowMask | LeaveWindowMask;
				g = f;
				break;
			case MOTION:
				mask = PointerMotionMask;
				g = f;
				break;
			case BTTN1_DN:
			case SHFT_BTTN1_DN:
			case CTRL_BTTN1_DN:
			case META_BTTN1_DN:
			case BTTN2_DN:
			case SHFT_BTTN2_DN:
			case CTRL_BTTN2_DN:
			case META_BTTN2_DN:
			case BTTN3_DN:
			case SHFT_BTTN3_DN:
			case CTRL_BTTN3_DN:
			case META_BTTN3_DN:
				g = (callback_proc) StandardKeyboardAndMouseEventHandler;
				mask = ButtonPressMask;
				break;
			case BTTN2_UP:
			case SHFT_BTTN2_UP:
			case CTRL_BTTN2_UP:
			case META_BTTN2_UP:
			case BTTN3_UP:
			case SHFT_BTTN3_UP:
			case CTRL_BTTN3_UP:
			case META_BTTN3_UP:
				g = (callback_proc) StandardKeyboardAndMouseEventHandler;
				mask = ButtonReleaseMask;
				break;
			case KEY_PRESS:
				g = (callback_proc) StandardKeyboardAndMouseEventHandler;
				mask = KeyPressMask;
				break;
			default:
#				ifdef DEBUG_MOTIF
				cerr << "add_kbd...: unknown action\n";
#				endif
				break; }

		if(g) {
#			ifdef DEBUG_MOTIF
			cerr << "add_kbd...: adding handler, mask = " << mask << "...\n";
#			endif
			XtAddEventHandler(
				widget,
				mask,
				FALSE,
				(XtEventHandler) g,
				H);
			callbacks << H; }
		else
			delete H; }
	}
}

#ifdef HAVE_MACOS
void motif_widget::fix_height(int desired_height, Widget handler_data /* = 0 (in header file) */ ) {
	if(GUI_Flag) {
	add_event_handler(fix_action_area_height, StructureNotifyMask, desired_height, (void *) handler_data);
	}
}

#else
void motif_widget::fix_height(Widget handler_data) {
	if(GUI_Flag) {
	add_event_handler(fix_action_area_height, StructureNotifyMask, (void *) handler_data);
	}
}
#endif /* HAVE_MACOS */

callback_stuff::callback_stuff(motif_widget * p, callback_proc f, const char * xmnReason, void * u_data)
	: ref_node(xmnReason),
	original_reason(xmnReason),
	data(u_data),
	function(f),
	parent(p),
#ifdef HAVE_MACOS
	height(0),
#endif /* HAVE_MACOS */
	callback_type(CALLBACK_TYPE),
	Mask(0),
	modifier(NO_MODIFIER),
	action(NO_ACTION)
	{}

#ifdef HAVE_MACOS
callback_stuff::callback_stuff(motif_widget *p, callback_proc f, const char *xmnReason, int desiredHeight, void *u_data)
	: ref_node(xmnReason),
	original_reason(xmnReason),
	data(u_data),
	function(f),
	parent(p),
	height(desiredHeight),
	callback_type(CALLBACK_TYPE),
	Mask(0),
	modifier(NO_MODIFIER),
	action(NO_ACTION)
	{}
#endif /* HAVE_MACOS */

callback_stuff::callback_stuff(motif_widget * p, callback_proc f, const char * xmnReason, void * u_data, long mask)
	: ref_node(xmnReason),
	original_reason(xmnReason),
	data(u_data),
	function(f),
	parent(p),
#ifdef HAVE_MACOS
	height(0),
#endif /* HAVE_MACOS */
	callback_type(EVENT_HANDLER_TYPE),
	Mask(mask),
	modifier(NO_MODIFIER),
	action(NO_ACTION)
	{}

callback_stuff::callback_stuff(motif_widget * p, callback_proc f, const char *, void * udat, KBD_AND_MOUSE_ACTION ac, MODIFIER mod)
	: ref_node("kbd_and_mouse_handler"),
	data(udat),
	function(f),
	parent(p),
#ifdef HAVE_MACOS
	height(0),
#endif /* HAVE_MACOS */
	callback_type(KBD_AND_MOUSE_HANDLER_TYPE),
	Mask(0),
	modifier(mod),
	original_reason(NULL),
	action(ac)
	{}

callback_stuff::callback_stuff(const callback_stuff & CS)
	: ref_node(CS),
	original_reason(CS.original_reason),
	data(CS.data),
	function(CS.function),
	parent(CS.parent),
#ifdef HAVE_MACOS
	height(0),
#endif /* HAVE_MACOS */
	callback_type(CS.callback_type),
	Mask(CS.Mask),
	modifier(CS.modifier),
	action(CS.action)
	{}

callback_stuff::~callback_stuff() {
	if(GUI_Flag) {
#	ifdef DEBUG_MOTIF
	if(parent) {
	for(L = 0; L < lev; L++) cerr << "  ";
	cerr << "removing callback for \"" << parent->Substring << "\" @ " << (void *) function
		<< ", reason \"" << Substring << "\", data @ " << (void *) data << ", type " <<
		(int) callback_type << "...\n"; }
#	endif
	if(parent && parent->widget) {
		if(callback_type == CALLBACK_TYPE && original_reason && strcmp(original_reason,"")){
			XtRemoveCallback(
				parent->widget,
				// THIS DOES NOT WORK!!
				// (char *) *Substring,
				original_reason,
				(XtCallbackProc) function,
				// THIS DOES NOT WORK!!
				// data);
				this);
		}
		else if(callback_type == EVENT_HANDLER_TYPE){
			XtRemoveEventHandler(
				parent->widget,
				XtAllEvents,
				TRUE,
				(XtEventHandler) function,
				this);
		}
		else if(callback_type == KBD_AND_MOUSE_HANDLER_TYPE) {
			callback_proc g = NULL;

			switch(action) {
				case EXPOSE:
				case RESIZE:
				case FOCUS:
				case MOTION:
					g = function;
					break;
				case BTTN1_DN:
				case SHFT_BTTN1_DN:
				case CTRL_BTTN1_DN:
				case META_BTTN1_DN:
				case BTTN2_DN:
				case SHFT_BTTN2_DN:
				case CTRL_BTTN2_DN:
				case META_BTTN2_DN:
				case BTTN3_DN:
				case SHFT_BTTN3_DN:
				case CTRL_BTTN3_DN:
				case META_BTTN3_DN:
				case BTTN2_UP:
				case SHFT_BTTN2_UP:
				case CTRL_BTTN2_UP:
				case META_BTTN2_UP:
					g = (callback_proc) StandardKeyboardAndMouseEventHandler;
					break;
				default:
					break; }
			XtRemoveEventHandler(
				parent->widget,
				XtAllEvents,
				TRUE,
				(XtEventHandler) g,
				this); } }
	}
}

motif_widget &motif_widget::operator = (const Cstring &Label) {
	if(GUI_Flag) {
	XmTextPosition		thePos;

#	ifdef DEBUG_MOTIF
	cerr << "motif_widget::operator =: widget \"" << Substring << "\" = " << Label << endl;
#	endif
	if(widget) {
		if(	MotifClass == xmTextFieldWidgetClass) {
			int		n;

			if((n = Label.OccurrencesOf("\n"))) {
				const char	*t = *Label;
				char		*temp = (char *) malloc(Label.length() + 2 * n + 1);
				char		*u = temp;
		
				while(*t) {
					if(*t == '\n') {
						*u++ = '\\';
						*u++ = 'n'; }
					else {
						*u++ = *t; }
					t++; }
				*u = '\0';
				XmTextSetString(widget, temp);
				free(temp); }
			else {
				XmTextSetString(widget, (char *) *Label); }
			thePos = XmTextGetLastPosition(widget);
			XmTextSetInsertionPosition(widget, thePos);
			XmTextShowPosition(widget, thePos); }
		else if(MotifClass == xmTextWidgetClass ||
			 MotifClass == xmScrolledTextWidgetClass) {
			XmTextSetString(widget, (char *) *Label); }
		else if(MotifClass == xmLabelWidgetClass) {
			if(Label.OccurrencesOf("\n"))
				SetLabelToStringContainingNewline(widget, (char *) *Label);
			else {
				XmString temp_str;

				temp_str = XmStringCreateLocalized((char *) *Label);
				XtVaSetValues(widget, XmNlabelString, temp_str, NULL);
				XmStringFree(temp_str); } }
		else if(MotifClass == xmFileSelectionDialogWidgetClass)
			XmTextSetString(XmFileSelectionBoxGetChild(widget, XmDIALOG_TEXT), (char *) *Label);
		else if(MotifClass == xmFileSelectionBoxWidgetClass)
			XmTextSetString(XmFileSelectionBoxGetChild(widget, XmDIALOG_TEXT), (char *) *Label);
		else if(MotifClass == xmPromptDialogWidgetClass) {
			XmString temp_str;

			temp_str = XmStringCreateLocalized((char *) *Label);
			XtVaSetValues(widget, XmNselectionLabelString, temp_str, NULL);
			XmStringFree(temp_str); }
		else if(MotifClass == xmCascadeButtonWidgetClass) {
			XmString temp_str;

			temp_str = XmStringCreateLocalized((char *) *Label);
			XtVaSetValues(widget, XmNlabelString, temp_str, NULL);
			XmStringFree(temp_str); }
		else if(MotifClass == xmPushButtonWidgetClass) {
			XmString temp_str;

			temp_str = XmStringCreateLocalized((char *) *Label);
			XtVaSetValues(widget, XmNlabelString, temp_str, NULL);
			XmStringFree(temp_str); }
		else if(MotifClass == xmToggleButtonWidgetClass) {
			// reset and Notify callbacks
			if(Label == "-1") XmToggleButtonSetState(widget, False, True);
			// reset
			else if(Label == "0") XmToggleButtonSetState(widget, False, False);
			// set
			else if(Label == "1") XmToggleButtonSetState(widget, True, False);
			// set and Notify callbacks
			else if(Label == "2") XmToggleButtonSetState(widget, True, True); } } }
	return *this; }

void motif_widget::set_label(const Cstring & text) {
	if(GUI_Flag) {
	XmString temp_str;

	temp_str = XmStringCreateLocalized((char *) *text);
	XtVaSetValues(widget, XmNlabelString, temp_str, NULL);
	XmStringFree(temp_str); } }

motif_widget & motif_widget:: operator < (motif_widget & M) {
	if(GUI_Flag) {
	Widget w = widget, m_w = M.widget;

	if(sr_is_composite(MotifClass)) w = XtParent(w);
	if(sr_is_composite(M.MotifClass)) m_w = XtParent(m_w);
	if(M.IsChildOfAForm || M.MotifClass != xmFormWidgetClass)
		{
#		ifdef DEBUG_MOTIF
		cerr << "Attaching left of \"" << XtName(m_w) << "\" to \"" << XtName(w) << "\"...\n";
#		endif
		if(IsChildOfAForm || MotifClass != xmFormWidgetClass)
			XtVaSetValues(m_w,
				XmNleftAttachment,	XmATTACH_WIDGET,
				XmNleftWidget,		widget,
				NULL);
		else
			XtVaSetValues(m_w,
				XmNleftAttachment,	XmATTACH_FORM,
				NULL);
		if(RightOffset)
			{
			XtVaSetValues(m_w,
				XmNleftOffset,		RightOffset,
				NULL);
			RightOffset = 0;
			}
		return M;
		}
	else
		{
#		ifdef DEBUG_MOTIF
		cerr << "Attaching right of \"" << XtName(w) << "\" to form \"" << XtName(m_w) << "\"...\n";
#		endif
		XtVaSetValues(w,
			XmNrightAttachment,	XmATTACH_FORM,
			NULL);
		if(RightOffset)
			{
			XtVaSetValues(w,
				XmNrightOffset,	RightOffset,
				NULL);
			RightOffset = 0;
			}
		return * this; } }
	return *this; }

motif_widget & motif_widget:: operator < (int i) {
	RightOffset = i;
#	ifdef DEBUG_MOTIF
	cerr << "offsetting right of \"" << XtName(widget) << "\" by " << i << "...\n";
#	endif
	return *this; }

motif_widget & operator < (int i, motif_widget & MW) {
	if(GUI_Flag) {
	Widget m_w = MW.widget;

	if(m_w) {
		if(sr_is_composite(MW.MotifClass)) m_w = XtParent(m_w);
		XtVaSetValues(m_w, XmNleftOffset, i, NULL);
#	ifdef DEBUG_MOTIF
	cerr << "offsetting left of \"" << XtName(m_w) << "\" by " << i << "...\n";
#	endif
		}
	}
	return MW; 
}

motif_widget & motif_widget:: operator < (const form_position & fp) {
	if(GUI_Flag) {
	Widget w = widget;
	if(w) {
		if(sr_is_composite(MotifClass)) w = XtParent(w);
		XtVaSetValues(w,
			XmNrightAttachment,	XmATTACH_POSITION,
			XmNrightPosition,	fp.pos,
			NULL);
#	ifdef DEBUG_MOTIF
	cerr << "Attaching right of \"" << XtName(w) << "\" to position " << fp.pos << "...\n";
#	endif
		}
	}
	return *this; 
}

motif_widget & motif_widget:: operator ^ (const form_position & fp) {
	if(GUI_Flag) {
	Widget w = widget;
	if(w)
		{
		if(sr_is_composite(MotifClass)) w = XtParent(w);
		XtVaSetValues(w,
			XmNbottomAttachment,	XmATTACH_POSITION,
			XmNbottomPosition,	fp.pos,
			NULL);
#	ifdef DEBUG_MOTIF
	cerr << "Attaching bottom of \"" << XtName(w) << "\" to position " << fp.pos << "...\n";
#	endif
		} }
	return *this; }

motif_widget & form_position:: operator < (motif_widget & M) {
	if(GUI_Flag) {
	Widget m_w = M.widget;

	if(m_w) {
		if(sr_is_composite(M.MotifClass)) m_w = XtParent(m_w);
		XtVaSetValues(m_w,
			XmNleftAttachment,	XmATTACH_POSITION,
			XmNleftPosition,	pos,
			NULL);
#		ifdef DEBUG_MOTIF
		cerr << "Attaching left of \"" << XtName(m_w) << "\" to position " << pos << "...\n";
#		endif
		} }
	return M; }

motif_widget & form_position:: operator ^ (motif_widget & M) {
#	ifdef GUI
	if(GUI_Flag) {
	Widget m_w = M.widget;

	if(m_w) {
		if(sr_is_composite(M.MotifClass)) m_w = XtParent(m_w);
		XtVaSetValues(m_w,
			XmNtopAttachment,	XmATTACH_POSITION,
			XmNtopPosition,	pos,
			NULL);
#		ifdef DEBUG_MOTIF
		cerr << "Attaching top of \"" << XtName(m_w) << "\" to position " << pos << "...\n";
#		endif
		}
	}
#endif
	return M; 
}

motif_widget & motif_widget:: operator ^ (motif_widget & M) {
#	ifdef GUI
	if(GUI_Flag) {
	Widget w = widget, m_w = M.widget;

	if(sr_is_composite(MotifClass))  w = XtParent(w);
	if(sr_is_composite(M.MotifClass)) m_w = XtParent(m_w);
	if(M.MotifClass != xmFormWidgetClass || M.IsChildOfAForm) {
		if(MotifClass != xmFormWidgetClass || IsChildOfAForm) {
#			ifdef DEBUG_MOTIF
			cerr << "Attaching top of \"" << XtName(m_w) << "\" to bottom of \"" << XtName(w) <<
				"\", offset " << BottomOffset << "...\n";
#			endif
			XtVaSetValues(m_w,
				XmNtopAttachment,	XmATTACH_WIDGET,
				XmNtopWidget,		widget,
				NULL); }
		else {
#ifdef DEBUG_MOTIF
cerr << "Attaching top of \"" << XtName(m_w) << "\" to form \"" << XtName(w) << "\"...\n";
#endif
			XtVaSetValues(m_w,
				XmNtopAttachment,	XmATTACH_FORM,
				NULL); }
		if(BottomOffset) {
			XtVaSetValues(m_w,
				XmNtopOffset,		BottomOffset,
				NULL);
			BottomOffset = 0; }
		return M; }
	else {
#		ifdef DEBUG_MOTIF
		cerr << "Attaching \"" << XtName(w) << "\" to bottom of form \"" << XtName(m_w) << "\", offset "
			<< BottomOffset << "...\n";
#		endif
		XtVaSetValues(w,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);
		if(BottomOffset) {
			XtVaSetValues(w,
				XmNbottomOffset,	BottomOffset,
				NULL);
			BottomOffset = 0; }
		return *this; }
	}
	else {
		return *this;
	}
#	else
	return *this;
#	endif
}

motif_widget & motif_widget:: operator ^ (int i) {
	BottomOffset = i;
#	ifdef DEBUG_MOTIF
	cerr << "Bottom Offset of widget \"" << XtName(widget) << "\" = " << i << endl;
#	endif
	return *this; }

void motif_widget::add_property(const Cstring & P) {
#	ifdef GUI
	if(GUI_Flag) {

	if(P == CHILD_OF_A_FORM)
		IsChildOfAForm = 1;
	else if(P == FORM_PARENT)
		IsChildOfAForm = 0;
	if(widget) {
		if(P == EDITABLE) {
			XtVaSetValues(widget, XmNeditable, True, NULL); }
		else if(P == NONEDITABLE) {
			XtVaSetValues(widget, XmNeditable, False, NULL); }
		else if(P == INITIAL_FOCUS)
			XmProcessTraversal(widget, XmTRAVERSE_CURRENT);
		else if(P == SELECTION_POLICY_MULTIPLE_SELECT)
			XtVaSetValues(widget, XmNselectionPolicy, XmMULTIPLE_SELECT, NULL);
		else if(P == SCROLLBAR_DISPLAY_POLICY_AS_NEEDED)
			XtVaSetValues(widget, XmNscrollBarDisplayPolicy, XmAS_NEEDED, NULL);
		else if(P == WRAP_TEXT) {
			XtVaSetValues(widget, XmNscrollHorizontal, FALSE, NULL);
			XtVaSetValues(widget, XmNwordWrap, True, NULL); }
#		ifdef DEBUG_MOTIF
		cerr << "motif_widget::add_property << (" << P << ") for widget \"" << XtName(widget) << "\"\n";
#		endif
		}
	}
#endif
}

int motif_widget::is_sensitive() {
	static int		are_we_sensitive;
#ifdef GUI
	if(GUI_Flag) {
	are_we_sensitive = XtIsSensitive(widget);
#	ifdef DEBUG_MOTIF
	// cerr << "motif_widget::is_sensitive() for widget \"" << XtName(widget) << "\" = " << are_we_sensitive << "\n";
#	endif
	}
	else {
		are_we_sensitive = 0;
	}
#else
	are_we_sensitive = 0;
#endif
	return are_we_sensitive; 
}

void motif_widget::set_sensitive(int value) {
#	ifdef GUI
	if(GUI_Flag) {
	XtSetSensitive(widget, value);
#	ifdef DEBUG_MOTIF
	cerr << "motif_widget::set_sensitive(" << value << ") for widget \"" << XtName(widget) << "\"...\n";
#	endif
	}
#endif
}

void motif_widget::manage() {
#	ifdef GUI
	if(GUI_Flag) {
#	ifdef DEBUG_MOTIF
		cerr << "motif_widget::manage: XtManageChild called for \"" <<
			XtName(widget) << "\"...\n";
#	endif
	XtManageChild(widget);
	}
#endif
	managedState = true; 
}

void motif_widget::unmanage() {
#	ifdef GUI
	if(GUI_Flag) {
	XtUnmanageChild(widget);
#	ifdef DEBUG_MOTIF
	cerr << "motif_widget::unmanage calling XtUnmanageChild for widget \"" << XtName(widget) << "\"...\n";
#	endif
	}
#endif
	managedState = false; 
}

Cstring motif_widget::get_label() {
#	ifdef GUI
	if(GUI_Flag) {
	if(MotifClass == xmToggleButtonWidgetClass) {
		char * string = NULL;
		XmString label;
		Cstring ret;

		XtVaGetValues(widget, XmNlabelString, & label, NULL);
		XmStringGetLtoR(label, XmFONTLIST_DEFAULT_TAG, & string);
		XmStringFree(label);
		ret = Cstring(string);
		if(string) XtFree(string);
		return ret; }
	return get_text();
	}
	else {
		return Cstring();
	}
#	else
	return Cstring();
#	endif
}

Cstring motif_widget::get_text() {
	char	*string = NULL;

	if(GUI_Flag) {

		if(	 MotifClass == xmTextWidgetClass ||
			 MotifClass == xmTextFieldWidgetClass ||
			 MotifClass == xmScrolledTextWidgetClass)
			string = XmTextGetString(widget);
		else if(MotifClass == xmLabelWidgetClass ||
			 MotifClass == xmPushButtonWidgetClass ||
			 MotifClass == xmCascadeButtonWidgetClass) {
			XmString label;
	
			XtVaGetValues(widget, XmNlabelString, & label, NULL);
			XmStringGetLtoR(label, XmFONTLIST_DEFAULT_TAG, & string);
			XmStringFree(label); }
		else if(MotifClass == xmFileSelectionBoxWidgetClass)
			string = XmTextGetString(XmFileSelectionBoxGetChild(widget, XmDIALOG_TEXT));
		else if(MotifClass == xmFileSelectionDialogWidgetClass)
			string = XmTextGetString(XmFileSelectionBoxGetChild(widget, XmDIALOG_TEXT));
		else if(MotifClass == xmToggleButtonWidgetClass) {
			if(XmToggleButtonGetState(widget)) return set_string;
			else return not_set_string; }
		else {
			cerr << "?? get_text() applied to widget \"" << get_key() << "\", class \"" <<
				class_to_string(MotifClass) << "\"...\n"; }
		Cstring ret(string);
		if(string) XtFree(string);
		return ret; }
	return Cstring(); }

/*
 * we now use our own flag because X doesn't tell the truth:
 */
#ifdef OBSOLETE
bool motif_widget::is_managed() {
	if(widget && XtIsManaged(widget))
		return 1;
	else 
		return 0;
#endif

bool motif_widget::is_managed() {
#	ifdef DEBUG_MOTIF
	// cerr << "IsManaged(\"" << XtName(widget) << "\") = " << managedState << "\n";
#endif
	return managedState; }


label_widget::label_widget(const Cstring & name, motif_widget *parent, const Cstring & P, bool should_manage)
	: motif_widget(name, xmLabelWidgetClass, parent, NULL, 0, should_manage) {
#ifdef GUI
	if(GUI_Flag) {
	if(P == AT_BEGINNING)
		XtVaSetValues(widget, XmNalignment, XmALIGNMENT_BEGINNING, NULL);
#	ifdef DEBUG_MOTIF
	cerr << "DONE creating label_widget \"" << XtName(widget) << "\", should_manage = " << should_manage << "...\n";
#	endif
	}
#endif
}

single_line_text_widget::single_line_text_widget(const Cstring & name, motif_widget * parent,
	const Cstring & editable, bool should_manage)
	: motif_widget(
#ifdef GUI
			name, xmTextFieldWidgetClass, parent, NULL, 0, should_manage
#else
			NULL
#endif
			) {
#ifdef GUI
	if(GUI_Flag) {
	if(editable == EDITABLE) {
		XtVaSetValues(widget, XmNeditable, True, NULL);
		XtVaSetValues(widget, XmNnavigationType, XmTAB_GROUP, NULL); }
	else {
		XtVaSetValues(widget, XmNeditable, False, NULL);
		XtVaSetValues(widget, XmNnavigationType, XmNONE, NULL); }
#	ifdef DEBUG_MOTIF
	cerr << "DONE creating single_line_text_widget \"" << XtName(widget) << "\", should_manage = " << should_manage << "...\n";
#	endif
	}
#endif
}
 
void
single_line_text_widget::SetText(const char* text) {
#ifdef GUI
  //why does set string need a non-const char*?

  char* non_const = new char[strlen(text) + 1];
  strcpy(non_const, text);
  XmTextSetString(widget, non_const);
  XmTextPosition thePos = XmTextGetLastPosition(widget);
  XmTextShowPosition(widget, thePos);
  delete [] non_const;

  //would it write over it?
#endif
}

Cstring
single_line_text_widget::GetText() {
#ifdef GUI
	if(GUI_Flag) {
	return (Cstring)XmTextGetString(widget);
	}
#endif
	return Cstring();
}

multi_line_text_widget::multi_line_text_widget(const Cstring & name, motif_widget * parent, int rows,
	const Cstring & editable, bool should_manage)
	: motif_widget(name, xmTextWidgetClass, parent, NULL, 0, should_manage) {
#ifdef GUI
	if(GUI_Flag) {
	XtVaSetValues(widget, XmNeditMode, XmMULTI_LINE_EDIT, XmNrows, rows, NULL);
	if(editable == EDITABLE) {
		XtVaSetValues(widget, XmNeditable, True, NULL);
		XtVaSetValues(widget, XmNnavigationType, XmTAB_GROUP, NULL); }
	else {
		XtVaSetValues(widget, XmNeditable, False, NULL);
		XtVaSetValues(widget, XmNnavigationType, XmNONE, NULL); }
#	ifdef DEBUG_MOTIF
	cerr << "DONE creating multi_line_text_widget \"" << XtName(widget) << "\", should_manage = " << should_manage << "...\n";
#	endif
	}
#endif
}

scrolled_text_widget::scrolled_text_widget(const Cstring &name, motif_widget *parent, int vis_rows,
	const Cstring &editable, bool should_manage)
	: motif_widget(name, xmScrolledTextWidgetClass, parent, NULL, 0, should_manage) {
#ifdef GUI
	if(GUI_Flag) {
	//Note that only Text widget values are set here; parent ScrolledWidget
	//  values are set upon creation, in motif_widget::construct_widget()
	XtVaSetValues(widget,
		XmNrows,		((vis_rows > 2) ? vis_rows : 2),
		NULL);
	if(editable == EDITABLE) {
		XtVaSetValues(widget,
			XmNeditable,	True,
			XmNeditMode,	XmMULTI_LINE_EDIT, 
			NULL);
		// add_callback((callback_proc) XmProcessTraversal, XmNactivateCallback, (void *) XmTRAVERSE_NEXT);
		}
	else
		XtVaSetValues(widget,
			XmNeditable, False,
			XmNeditMode,	XmMULTI_LINE_EDIT, 
			NULL);
#	ifdef DEBUG_MOTIF
	cerr << "DONE creating scrolled_text_widget \"" << XtName(widget) << "\", should_manage = " << should_manage << "...\n";
#	endif
	}
#endif
}

void scrolled_list_widget::SingleSelection(Widget, callback_stuff * client_data, void * call_data) {
#ifdef GUI
	if(GUI_Flag) {
	Pointer_node			*ptr;
	scrolled_list_widget		*object = (scrolled_list_widget *) client_data->data;
	List_iterator			theSlaves(object->smart_pointers_to_slave_lists);
	XmListCallbackStruct		*cbs = (XmListCallbackStruct *) call_data;
	int				selected;

	selected = XmListPosSelected(object->widget, cbs->item_position);
	while((ptr = (Pointer_node *) theSlaves())) {
		slave_list_widget	*theSlave = (slave_list_widget *) ptr->get_ptr();

		if(selected) {
			XmListSelectPos(	// last arg. is whether to notify the callback(s)
				theSlave->widget,
				cbs->item_position - (theSlave->index_of_item_at_the_top - 1),
				FALSE); }
		else {
			XmListDeselectPos(
				theSlave->widget,
				cbs->item_position - (theSlave->index_of_item_at_the_top - 1)); } }
	}
#endif
}

#ifdef GUI
void resize_scrolled_list_widget(Widget w, XEvent *, String *, Cardinal *) {
	if(GUI_Flag) {
	pointer_to_pointer	*ptp = (pointer_to_pointer *)
					widget_indexed_list_of_scrolled_list_widgets.find((void *) w);
	if(ptp) {
		scrolled_list_widget		*theList = (scrolled_list_widget *) ptp->PTR;
		Pointer_node			*ptr;
		List_iterator			theSlaves(theList->smart_pointers_to_slave_lists);
		int				thePositionOfTheTopItem;

		XtVaGetValues(w, XmNtopItemPosition, &thePositionOfTheTopItem, NULL);
		while((ptr = (Pointer_node *) theSlaves())) {
			slave_list_widget	*theSlave = (slave_list_widget *) ptr->get_ptr();

			theSlave->index_of_item_at_the_top = thePositionOfTheTopItem;

			theSlave->redisplay_all(); } } 
	}
}
#endif

scrolled_list_widget::scrolled_list_widget(const Cstring & name, motif_widget * parent, int visible_lines, bool should_manage)
	: motif_widget(name, xmScrolledListWidgetClass, parent, NULL, 0, should_manage) {
	v_blist				L(v_compare_v_bstringnodes);

#ifdef GUI
	if(GUI_Flag) {
	Widget		parent_window = XtParent(widget), scrollB;;
	XtActionsRec	rec;

	XtVaSetValues(widget,
		XmNscrollingPolicy,		XmAUTOMATIC,
		XmNselectionPolicy,		XmSINGLE_SELECT,
		XmNscrollBarDisplayPolicy,	XmAS_NEEDED,
		NULL);
	if(visible_lines > 0)
		XtVaSetValues(widget,
			XmNvisibleItemCount,		visible_lines,
			NULL);
	XtVaGetValues(parent_window, XmNverticalScrollBar, &scrollB, NULL);
	XtAddCallback(scrollB, XmNvalueChangedCallback,	standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNdragCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNincrementCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNdecrementCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNpageIncrementCallback,	standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNpageDecrementCallback,	standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNtoTopCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNtoBottomCallback,		standardResponseToScrolling, this);

	add_callback(SingleSelection, XmNsingleSelectionCallback, this);

	// Motif manual p. 243
	rec.string = (char*) "resize";
	rec.proc = resize_scrolled_list_widget;
	XtAppAddActions(Context, &rec, 1);
	XtOverrideTranslations(widget, XtParseTranslationTable("<Configure>: resize()"));
	widget_indexed_list_of_scrolled_list_widgets << new pointer_to_pointer((void *) widget, this);

#	ifdef DEBUG_MOTIF
	cerr << "DONE creating scrolled_list_widget(1) \"" << XtName(widget) << "\", should_manage = " << should_manage << "...\n";
#	endif
	}
#endif
}

/*
  New Constructor 8/29/03
  new variable passed SELECTION_TYPE
  Possible values "MULTIPLE_SELECT" or "SINGLE_SELECT", corresponding to a
  single select list or a multiselect list

scrolled_list_widget::scrolled_list_widget(const Cstring & name, motif_widget * parent, int visible_lines, int should_manage, int SELECTION_TYPE)
	: motif_widget(name, xmScrolledListWidgetClass, parent, NULL, 0, should_manage) {
	v_blist				L(v_compare_v_bstringnodes);

#ifdef GUI
	if(GUI_Flag) {
	Widget		parent_window = XtParent(widget), scrollB;;
	XtActionsRec	rec;

	if(SELECTION_TYPE == SINGLE_SELECT)
	{
	XtVaSetValues(widget,
		XmNscrollingPolicy,		XmAUTOMATIC,
		XmNselectionPolicy,		XmSINGLE_SELECT,
		XmNscrollBarDisplayPolicy,	XmAS_NEEDED,
		NULL);
	}
        else if(SELECTION_TYPE == MULTIPLE_SELECT)
	{
	XtVaSetValues(widget,
		XmNscrollingPolicy,		XmAUTOMATIC,
		XmNselectionPolicy,		XmMULTIPLE_SELECT,
		XmNscrollBarDisplayPolicy,	XmAS_NEEDED,
		NULL);
	}

	if(visible_lines > 0)
		XtVaSetValues(widget,
			XmNvisibleItemCount,		visible_lines,
			NULL);
	XtVaGetValues(parent_window, XmNverticalScrollBar, &scrollB, NULL);
	XtAddCallback(scrollB, XmNvalueChangedCallback,	standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNdragCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNincrementCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNdecrementCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNpageIncrementCallback,	standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNpageDecrementCallback,	standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNtoTopCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNtoBottomCallback,		standardResponseToScrolling, this);

	add_callback(SingleSelection, XmNsingleSelectionCallback, this);

	// Motif manual p. 243
	rec.string = "resize";
	rec.proc = resize_scrolled_list_widget;
	XtAppAddActions(Context, &rec, 1);
	XtOverrideTranslations(widget, XtParseTranslationTable("<Configure>: resize()"));
	widget_indexed_list_of_scrolled_list_widgets << new pointer_to_pointer((void *) widget, this);

#	ifdef DEBUG_MOTIF
	cerr << "DONE creating scrolled_list_widget(1) \"" << XtName(widget) << "\", should_manage = " << should_manage << "...\n";
#	endif
	}
#endif
}
*/

 void scrolled_list_widget::standardResponseToScrolling(Widget w, void * client_data, void * call_data) {
#ifdef GUI
   if(GUI_Flag) {
     XmScrollBarCallbackStruct	*cbs = (XmScrollBarCallbackStruct *) call_data;
     scrolled_list_widget		*object = (scrolled_list_widget *) client_data;
     Pointer_node			*ptr;
     List_iterator			theSlaves(object->smart_pointers_to_slave_lists);
     
     while((ptr = (Pointer_node *) theSlaves())) {
       slave_list_widget	*theSlave = (slave_list_widget *) ptr->get_ptr();

       theSlave->index_of_item_at_the_top = 1 + cbs->value;
       theSlave->redisplay_all(); }
   }
#endif
 }
 
 
scrolled_list_widget::~scrolled_list_widget() {
}
 
void scrolled_list_widget::detach_callbacks_and_event_handlers() {
#ifdef GUI
	if(GUI_Flag) {
	Widget		scrollB;

	XtVaGetValues(XtParent(widget), XmNverticalScrollBar, &scrollB, NULL);
	XtRemoveCallback(scrollB, XmNvalueChangedCallback, standardResponseToScrolling, this);
	XtRemoveCallback(scrollB, XmNdragCallback, standardResponseToScrolling, this);
	XtRemoveCallback(scrollB, XmNincrementCallback, standardResponseToScrolling, this);
	XtRemoveCallback(scrollB, XmNdecrementCallback, standardResponseToScrolling, this);
	XtRemoveCallback(scrollB, XmNpageIncrementCallback, standardResponseToScrolling, this);
	XtRemoveCallback(scrollB, XmNpageDecrementCallback, standardResponseToScrolling, this);
	XtRemoveCallback(scrollB, XmNtoTopCallback, standardResponseToScrolling, this);
	XtRemoveCallback(scrollB, XmNtoBottomCallback, standardResponseToScrolling, this);
	}
#endif
	motif_widget::detach_callbacks_and_event_handlers(); 
}


scrolled_list_widget::scrolled_list_widget(
					   const Cstring&	name,
					   motif_widget*	parent,
					   int			visible_lines,
					   const blist&		optional_props,
					   bool			should_manage)
		: motif_widget(parent) {
	Widget				scrollB;
	static _validation_tag		*T;

	Substring = name;
	MotifClass = xmScrolledListWidgetClass;

#ifdef GUI
	if(GUI_Flag) {
	XtActionsRec	rec;

	sn = 0;
	if(! optional_props.find(SELECTION_POLICY_SINGLE_SELECT)) {
		XtSetArg(sargs[sn], XmNselectionPolicy, XmMULTIPLE_SELECT); sn++; }
	else
		XtSetArg(sargs[sn], XmNselectionPolicy, XmSINGLE_SELECT); sn++;
	XtSetArg(sargs[sn], XmNscrollingPolicy, XmAUTOMATIC); sn++;
	if(optional_props.find(SCROLLBAR_DISPLAY_POLICY_STATIC)) {
		XtSetArg(sargs[sn], XmNscrollBarDisplayPolicy, XmAS_NEEDED); sn++; }
	if(optional_props.find(SCROLLBAR_PLACEMENT_BOTTOM_LEFT)) {
		XtSetArg(sargs[sn], XmNscrollBarPlacement, XmBOTTOM_LEFT); sn++; }
	if(visible_lines > 0) {
		XtSetArg(sargs[sn], XmNvisibleItemCount, visible_lines); sn++; }
	construct_widget(parent, sargs, sn, should_manage);
	XtVaGetValues(XtParent(widget), XmNverticalScrollBar, &scrollB, NULL);
	XtAddCallback(scrollB, XmNvalueChangedCallback,	standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNdragCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNincrementCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNdecrementCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNpageIncrementCallback,	standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNpageDecrementCallback,	standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNtoTopCallback,		standardResponseToScrolling, this);
	XtAddCallback(scrollB, XmNtoBottomCallback,		standardResponseToScrolling, this);

	add_callback(SingleSelection, XmNsingleSelectionCallback, this);

	// Motif manual p. 243
	rec.string = (char *) "resize";
	rec.proc = resize_scrolled_list_widget;
	XtAppAddActions(Context, &rec, 1);
	XtOverrideTranslations(widget, XtParseTranslationTable("<Configure>: resize()"));
	widget_indexed_list_of_scrolled_list_widgets << new pointer_to_pointer((void *) widget, this);

#	ifdef DEBUG_MOTIF
	cerr << "DONE creating scrolled_list_widget(2) \"" << XtName(widget) << "\", should_manage = " << should_manage << "...\n";
#	endif
	}
#endif
}

void scrolled_list_widget::operator << (const Cstring & s) {
	theItems << new ref_node(s);
#ifdef GUI
	if(GUI_Flag) {
	XmString	str = XmStringCreateLocalized(s());
	int		vizItemCount;
	Pointer_node	*ptr;
	List_iterator	theSlaves(smart_pointers_to_slave_lists);

	XtVaGetValues(widget, XmNvisibleItemCount, &vizItemCount, NULL);
	XmListAddItem(widget, str, 0);
	XmListDeselectAllItems(widget);
	XmStringFree(str);
	while((ptr = (Pointer_node *) theSlaves())) {
		slave_list_widget	* theSlave = (slave_list_widget *) ptr->get_ptr();

		XtVaSetValues(theSlave->widget, XmNvisibleItemCount, vizItemCount, NULL); }
	/*
	 * We need an easy way to select the item just added (for Dennis).
	 */
#	ifdef DEBUG_MOTIF
	cerr << "scrolled_list_widget::operator <<: widget \"" << XtName(widget) << "\" = " << s << endl;
#	endif
	}
#endif
}

void
scrolled_list_widget::SetList(const StringVect& strVect)
{
  clear();
  //there should be a way to do this with for_each
  StringVect::const_iterator iter = strVect.begin();
  
  while(iter != strVect.end())
  {
    //should << the string in the StringVect
    operator<<(*iter);
    iter++;
  }
  
}
 
void scrolled_list_widget::clear() {
	List_iterator			theSlaves(smart_pointers_to_slave_lists);
	Pointer_node			*ptr;
	slave_list_widget		*theSlave;

	while((ptr = (Pointer_node *) theSlaves())) {
		theSlave = (slave_list_widget *) ptr->get_ptr();
		theSlave->clear(); }

	theItems.clear();
#ifdef GUI
	if(GUI_Flag) {
	if(widget) {
		XmListDeleteAllItems(widget);
#		ifdef DEBUG_MOTIF
		cerr << "scrolled_list_widget::clear(): widget = \"" << XtName(widget) << "\"\n";
#		endif
		}
	}
#endif
}

void scrolled_list_widget::redisplay_all() {
#ifdef GUI
	if(GUI_Flag) {

	if(widget) {
		List_iterator			theSlaves(smart_pointers_to_slave_lists);
		Pointer_node			*ptr;
		slave_list_widget		*theSlave;
		int				i;
		strintslist			theSelectedItems;
		strint_node*			theSelectedItem;

		get_selected_items(theSelectedItems);
		XmListDeleteAllItems(widget);
#		ifdef DEBUG_MOTIF
		cerr << "scrolled_list_widget::redisplay_all(): widget = \"" << XtName(widget) << "\"\n";
#		endif
		for(i = 0; i < theItems.get_length(); i++) {
			XmString	str = XmStringCreateLocalized((char *) *theItems[i]->get_key());

 			XmListAddItem(widget, str, 0);
			XmStringFree(str); }
		XmListDeselectAllItems(widget);
		if((theSelectedItem = theSelectedItems.first_node())) {
			int	desired_selected_position = theSelectedItem->payload;

			if(desired_selected_position >= 1 && desired_selected_position <= theItems.get_length()) {
				// last arg. is whether to notify the callback(s)
				XmListSelectPos(widget, desired_selected_position, FALSE); } }
	while((ptr = (Pointer_node *) theSlaves())) {
		theSlave = (slave_list_widget *) ptr->get_ptr();
		theSlave->redisplay_all(); } }
	}
#endif
}

// stuffs ref_nodes in the list, with Substring = text of selection and ID = BASIC-style index:
int scrolled_list_widget::get_selected_items(strintslist& l) {
	int		N = 0;
#ifdef GUI
	if(GUI_Flag) {
	int		i;
	int		*pos_list;
	XmStringTable	st;
	char		*string;

	l.clear();
	XtVaGetValues(widget, XmNselectedItemCount, &N, XmNselectedItems, &st, NULL);
	XmListGetSelectedPos(widget, & pos_list, & N);
	for(i = 0; i < N; i++) {
		XmStringGetLtoR(st[i], XmFONTLIST_DEFAULT_TAG, & string);

		// debug
		// printf("list_widget::get_selected_items: N = %d, string = %s, pos = %d\n", N, string, pos_list[i]);
		l << new strint_node(string, pos_list[i]);
                XtFree(string); }
	if(N)
		XtFree((char *) pos_list);
	// Motif Ref. Manual p. 527: don't free the selected items!
	}
#endif
	return N; 
}

Cstring scrolled_list_widget::get_item_number(int n) {
	Cstring		result;

#ifdef GUI
	if(GUI_Flag) {
	int		how_many;
	XmStringTable	st;
	char		*string;

	XtVaGetValues(widget, XmNitems, &st, XmNitemCount, &how_many, NULL);
	if(n > how_many) return result;
	if(n <= 0) return result;
	XmStringGetLtoR(st[n - 1], XmFONTLIST_DEFAULT_TAG, & string);
	result = string;
	XtFree(string);
	}
#endif
	// note: we could have used theItems, but this was written before theItems was (were?) added:
	return result; 
}

Cstring slave_list_widget::get_item_number(int n) {
	if(n > 0 && theItems.get_length() >= n)
		return theItems[n - 1]->get_key();
	else return Cstring(""); }

void scrolled_list_widget::deselect_all()
	{
#ifdef GUI
	if(GUI_Flag) {
	XmListDeselectAllItems(widget);
	}
#endif
}

// Uses a BASIC-style index:
void scrolled_list_widget::delete_item(int i)
	{
#ifdef GUI
	if(GUI_Flag) {
	XmListDeletePos(widget, i);
	}
#endif
}

void scrolled_list_widget::select_bottom_pos(int notify) {
#ifdef GUI
	if(GUI_Flag) {
	List_iterator		theSlaves(smart_pointers_to_slave_lists);
	Pointer_node		*ptr;
	int			how_many;

	XtVaGetValues(widget, XmNitemCount, &how_many, NULL);
	XmListSetBottomPos(widget, 0);
	XmListSelectPos(widget, how_many, notify);
	while((ptr = (Pointer_node *) theSlaves())) {
		slave_list_widget	*theSlave = (slave_list_widget *) ptr->get_ptr();

		/*
		 * Note: 0 specifies the last item in the list
		 */
		XtVaGetValues(theSlave->widget, XmNitemCount, &how_many, NULL);
		XmListSetBottomPos(theSlave->widget, 0);
		/*
		 * Note: 0 means "don't notify the callback(s)"
		 */
		XmListSelectPos(theSlave->widget, how_many, notify); }
	}
#endif
}

void scrolled_list_widget::reset_horiz_pos()
	{
#ifdef GUI
	if(GUI_Flag) {
	XmListSetHorizPos(widget, 0);
	}
#endif
}

void scrolled_list_widget::set_pos(int I) {
#ifdef GUI
	if(GUI_Flag) {
	XmListSelectPos(widget, I, False); /* do NOT Notify the callback(s) */
	XmListSetPos(widget, I);
	
	}
#endif
}

void scrolled_list_widget::select_pos(int I) {
#ifdef GUI
	if(GUI_Flag) {
	List_iterator			theSlaves(smart_pointers_to_slave_lists);
	Pointer_node			*ptr;

	XmListSelectPos(widget, I, False); /* do NOT Notify the callback(s) */
	while((ptr = (Pointer_node *) theSlaves())) {
		slave_list_widget	*theSlave = (slave_list_widget *) ptr->get_ptr();
		XmListSelectPos(	// last arg. is whether to notify the callback(s)
			theSlave->widget,
			I - (theSlave->index_of_item_at_the_top - 1),
			FALSE); }
	}
#endif
}

void scrolled_list_widget::setSelectionType(int T)
{
#ifdef GUI
	if(GUI_Flag) {
		Widget		parent_window = XtParent(widget), scrollB;;
		XtActionsRec	rec;


		if(T == MULTIPLE_SELECT)
		{
			XtVaSetValues(widget,
				       XmNscrollingPolicy,		XmAUTOMATIC,
				       XmNselectionPolicy,		XmEXTENDED_SELECT,
				       XmNscrollBarDisplayPolicy,	XmAS_NEEDED,
				       NULL);

		}

		else if(T == SINGLE_SELECT)
		{
			XtVaSetValues(widget,
				       XmNscrollingPolicy,		XmAUTOMATIC,
				       XmNselectionPolicy,		XmSINGLE_SELECT,
				       XmNscrollBarDisplayPolicy,	XmAS_NEEDED,
				       NULL);

		}

		else
		{
			cerr << "Invalid List Selection Type in scrolled_list_widget::setSelectionType(int)" << endl;
		}
	}
#endif
}

void slave_list_widget::SingleSelection(Widget, callback_stuff * client_data, void * call_data) {
#ifdef GUI
	if(GUI_Flag) {
	slave_list_widget		*object = (slave_list_widget *) client_data->data;
	Pointer_node			*ptr = (Pointer_node *) object->list_of_pointer_to_the_master.first_node();
	List_iterator			*theSlaves;
	XmListCallbackStruct		*cbs = (XmListCallbackStruct *) call_data;
	scrolled_list_widget		*theMaster;
	int				selected;

	if(! ptr) return;
	theMaster = (scrolled_list_widget *) ptr->get_ptr();
	// check whether selected
	selected = XmListPosSelected(object->widget, cbs->item_position);
	if(selected) {
		XmListSelectPos(	// last arg. is whether to notify the callback(s)
			theMaster->widget,
			cbs->item_position + (object->index_of_item_at_the_top - 1),
			FALSE); }
	else {
		XmListDeselectPos(
			theMaster->widget,
			cbs->item_position + (object->index_of_item_at_the_top - 1)); }
	theSlaves = new List_iterator(theMaster->smart_pointers_to_slave_lists);
	while((ptr = (Pointer_node *) (*theSlaves)())) {
		slave_list_widget	*theSlave = (slave_list_widget *) ptr->get_ptr();

		if(theSlave != object) {
			if(selected) {
				XmListSelectPos(	// last arg. is whether to notify the callback(s)
					theSlave->widget,
					cbs->item_position,
					FALSE); }
			else {
				XmListDeselectPos(
					theSlave->widget,
					cbs->item_position); } } }
	delete theSlaves;
	}
#endif
}

slave_list_widget::slave_list_widget(const Cstring & name, motif_widget * parent, scrolled_list_widget * master,
		void * args, int no_args)
	: motif_widget(
#ifdef GUI
			name, xmListWidgetClass, parent, args, no_args, true
#else
			NULL
#endif
			),
	index_of_item_at_the_top(0) {
#ifdef GUI
	if(GUI_Flag) {
#       ifdef DEBUG_MOTIF
        cerr << "DONE creating scrolled_list_widget(1) \"" << XtName(widget) << "\"...\n";
#       endif
	if(master) {
		// SMART pointer:
		list_of_pointer_to_the_master << new Pointer_node(master, master);
		// SMART pointer:
		master->smart_pointers_to_slave_lists << new Pointer_node(this, this); }

	add_callback(SingleSelection, XmNsingleSelectionCallback, this);
	}
#endif
}

void slave_list_widget::clear() {
#ifdef GUI
	if(GUI_Flag) {
	if(widget) {
		XmListDeleteAllItems(widget);
#		ifdef DEBUG_MOTIF
		cerr << "slave_list_widget::clear(): widget = \"" << XtName(widget) << "\"\n";
#		endif
		}
	}
#endif
	theItems.clear();
	index_of_item_at_the_top = 0; 
}

void slave_list_widget::redisplay_all() {
#	ifdef GUI
	if(GUI_Flag) {
	int			i, vizItemCount;
	int			number_of_items_to_display = theItems.get_length() - (index_of_item_at_the_top - 1);
	Pointer_node		*ptr = (Pointer_node *) list_of_pointer_to_the_master.first_node();
	scrolled_list_widget	*theMaster;
	strintslist		theSelectedItems;
	strint_node*		theSelectedItem;

	if(! ptr) return;
	theMaster = (scrolled_list_widget *) ptr->get_ptr();
	theMaster->get_selected_items(theSelectedItems);
	if(! index_of_item_at_the_top) {
		number_of_items_to_display = theItems.get_length();
		index_of_item_at_the_top = 1; }
	XtVaGetValues(widget, XmNvisibleItemCount, &vizItemCount, NULL);

	if(number_of_items_to_display > vizItemCount)
		number_of_items_to_display = vizItemCount;

	XmListDeleteAllItems(widget);
	for(i = 0; i < number_of_items_to_display; i++) {
		XmString str = XmStringCreateLocalized((char *) *theItems[index_of_item_at_the_top - 1 + i]->get_key());

 		XmListAddItem(widget, str, 0);
		XmStringFree(str); }
	XmListDeselectAllItems(widget);
	if((theSelectedItem = theSelectedItems.first_node())) {
		int	desired_selected_position = theSelectedItem->payload - (index_of_item_at_the_top - 1);

		if(desired_selected_position >= 1 && desired_selected_position <= number_of_items_to_display) {
			// last arg. is whether to notify the callback(s)
			XmListSelectPos(widget, desired_selected_position, FALSE); } }
	}
#endif
}

void slave_list_widget::operator << (const Cstring & s) {
	if(GUI_Flag) {
	int		current_list_size = theItems.get_length();
	int		vizItemCount;
	int		index_of_new_item = 1 + current_list_size - (index_of_item_at_the_top - 1);

	theItems << new ref_node(s);
	if(! index_of_item_at_the_top) {
		index_of_new_item = 1; // this is the very first item...
		index_of_item_at_the_top = 1; }
	XtVaGetValues(widget, XmNvisibleItemCount, &vizItemCount, NULL);
	if(index_of_new_item <= vizItemCount) {
		XmString str = XmStringCreateLocalized((char *) *s);

		XmListAddItem(widget, str, 0);
		XmListDeselectAllItems(widget);
		XmStringFree(str); }
#	ifdef DEBUG_MOTIF
	cerr << "slave_list_widget::operator <<: widget \"" << XtName(widget) << "\" = " << s << endl;
#	endif
	} }
