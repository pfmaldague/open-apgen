#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "ACT_sys.H"
#include <ActivityInstance.H>
#include "UI_ds_draw.H"
#include "UI_exec.H"
#include "UI_GeneralMotifInclude.H"
#include "UI_mainwindow.H"
#include "UI_mainwindowbx.H"
#include "UI_resourcedisplay.H"
#include "tooltip.h"
#include "apcoreWaiter.H"

#include "apcoreWaiter.H"

#include "jpl.H"

#ifdef GUI
static Arg      args[56];
static Cardinal ac=0;
static Boolean  argok=False;
#endif

// STATICS:
motif_widget*		UI_mainwindowbx::_statusBarLabel = NULL;

// EXTERNS:

			// in UI_motif_widget.C:
extern resource_data    ResourceData;

			// in UI_exec.C:
extern UI_exec*		UI_subsystem;

// Common constant and pixmap declarations.
#ifdef GUI

extern XtPointer CONVERT(Widget, const char *, const char *, int, Boolean *);

// autoconf-style:

#ifdef USES_XPM_H
	// only Solaris 2.5 seems to have it here:
#	include <xpm.h>
#else
#	include <Xm/XpmP.h>
#endif

#ifdef HAVE_XPMATTR_21
	typedef XpmAttributes_21 theOfficialXpmAttributes;
#else
	typedef XpmAttributes theOfficialXpmAttributes;
#endif


#endif // ifdef GUI

#include "toolbar.H"

#ifdef GUI

Pixmap XPM_PIXMAP(Widget w, char **pixmapName)  {
	theOfficialXpmAttributes	attributes;
	int				argcnt;
	Arg				args[10];
	Pixmap				pixmap;
	Pixmap				shape;
	int				returnValue;
    
	argcnt = 0;
	XtSetArg(args[argcnt], XmNdepth, &(attributes.depth)); argcnt++;
	XtSetArg(args[argcnt], XmNcolormap, &(attributes.colormap)); argcnt++;
	XtGetValues(w, args, argcnt);

	attributes.visual = DefaultVisual(XtDisplay(w), DefaultScreen(XtDisplay(w)));
	attributes.valuemask = (XpmDepth | XpmColormap | XpmVisual);
    
	XpmCreatePixmapFromData(XtDisplay(w),
				 DefaultRootWindow(XtDisplay(w)),
				 pixmapName, &pixmap, &shape,
				 &attributes);
	return pixmap; }
#endif

UI_mainwindowbx::UI_mainwindowbx(const char *name, motif_widget * parent) : 
	motif_widget(name, xmMainWindowWidgetClass, parent, NULL, 0, TRUE)
#ifdef GUI
	, data_for_window_closing(NULL, UI_mainwindow::quitButtonActivateCallback, NULL, get_this_widget())
#endif // ifdef GUI
	{
#ifdef GUI
	  if(GUI_Flag)
	  {
		  AllowWindowClosingFromMotifMenu(widget, UI_mainwindow::quitButtonActivateCallback, &data_for_window_closing);
	  }
	// in UI_mainwindow.C now:
	// create(parent);
#endif // ifdef GUI
	}

UI_mainwindowbx::~UI_mainwindowbx() 
	{ }
#ifdef GUI
void UI_mainwindowbx::create(motif_widget *  parent) {
  if(GUI_Flag)
  {
	Widget	separator1, separator2, separator3, separator4, separator5;
	Widget	separator6, separator7, separator8, separator9, separator10;
	Widget	separator11, separator12, separator13, separator14, separator15;
	Widget	separator16, separator17, separator18, separator19, separator20;
	Widget	separator21;

	ac = 0;

	XtSetArg(args[ac], XmNwidth, 628); ac++;
	XtSetArg(args[ac], XmNheight, 33); ac++;
	_menuBar = new motif_widget(
		"menuBar",
		xmMenubarWidgetClass, this,
		args, ac,
		TRUE);

	//
	// FILE MENU
	//

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'F'); ac++;
	_fileButton = new motif_widget(
		"File",
		xmCascadeButtonWidgetClass,	_menuBar,
		args,	ac,
		TRUE);

	_fileButtonPM = new motif_widget(
		"fileButtonPM",
		xmPulldownMenuWidgetClass,	_menuBar,
		NULL,				0,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'O'); ac++;
	_openButton = new motif_widget(
		"Open File...", 
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac ,
		TRUE);
	_openButton->add_callback(
		UI_mainwindow::openButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator6 = XmCreateSeparator(_fileButtonPM->widget,
		(char *) "separator6",
		args, 
		ac);
	
	XtManageChild(separator6);

	ac = 0;
	// XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNmnemonic, 'S'); ac++;
	_savePlanFilesButton = new motif_widget(
		"Save File(s)...",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_savePlanFilesButton->add_callback(
		UI_mainwindow::savePlanFilesButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNmnemonic, 'C'); ac++;
	_integratePlanFilesButton = new motif_widget(
		"Consolidate Plan File(s)...",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_integratePlanFilesButton->add_callback(
		UI_mainwindow::integratePlanFilesButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent->widget, "Write Partial Plan File...", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNmnemonic, 'W'); ac++;
	_writeIntegratedPlanFileButton = new motif_widget(
		"Write Partial Plan File...",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_writeIntegratedPlanFileButton->add_callback(
		UI_mainwindow::writeIntegratedPlanFileButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator20 = XmCreateSeparator(_fileButtonPM->widget,
		(char *) "separator20",
		args, 
		ac);
	
	XtManageChild(separator20);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent->widget, "Export Data...", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNmnemonic, 'x'); ac++;
	_exportDataButton = new motif_widget(
		"Export Data...",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_exportDataButton->add_callback(
		UI_mainwindow::exportDataButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator7 = XmCreateSeparator(_fileButtonPM->widget,
		(char *) "separator7",
		args, 
		ac);

	XtManageChild(separator7);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNmnemonic, 'F'); ac++;
	_genSASFButton = new motif_widget(
		"Generate SASF...",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_genSASFButton->add_callback(
		UI_mainwindow::genSASFButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNmnemonic, 'T'); ac++;
	_genTOLButton = new motif_widget(
		"Generate TOL...",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_genTOLButton->add_callback(
		UI_mainwindow::genTOLButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	_genXMLTOLButton = new motif_widget(
		"Generate XML TOL...",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_genXMLTOLButton->add_callback(
		UI_mainwindow::genXMLTOLButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNmnemonic, 'T'); ac++;
	_genCsourceButton = new motif_widget(
		"Generate C source...",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_genCsourceButton->add_callback(
		UI_mainwindow::genCsourceButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator14 = XmCreateSeparator(_fileButtonPM->widget,
		(char *) "separator14",
		args, 
		ac);

	XtManageChild(separator14);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNmnemonic, 'P'); ac++;
	_printButton = new motif_widget(
		"Print...",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_printButton->add_callback(
		UI_mainwindow::printButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator15 = XmCreateSeparator(_fileButtonPM->widget,
		(char *) "separator15",
		args, 
		ac);

	XtManageChild(separator15);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, True); ac++;
	XtSetArg(args[ac], XmNmnemonic, 'u'); ac++;
	_purgeButton = new motif_widget(
		"Purge",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_purgeButton->add_callback(
		UI_mainwindow::purgeButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator12 = XmCreateSeparator(_fileButtonPM->widget,
		(char *) "separator12",
		args, 
		ac);

	XtManageChild(separator12);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'Q'); ac++;
	XtSetArg(args[ac], XmNaccelerator, "Ctrl<Key>C"); ac++;
	XtSetArg(args[ac], XmNacceleratorText, 
		CONVERT(parent->widget, "Ctrl+C", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	_quitButton = new motif_widget(
		"Quit",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_quitButton->add_callback(
		UI_mainwindow::quitButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'J'); ac++;
	_fast_quitButton = new motif_widget(
		"Fast Quit",
		xmPushButtonWidgetClass,	_fileButtonPM,
		args,				ac,
		TRUE);
	_fast_quitButton->add_callback(
		UI_mainwindow::fastquitButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsubMenuId, _fileButtonPM->widget); ac++;
	XtSetValues(_fileButton->widget, args, ac);


	//
	// EDIT MENU
	//

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
	_editButton = new motif_widget(
		"Edit",
		xmCascadeButtonWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	_editButtonPM = new motif_widget(
		"editButtonPM",
		xmPulldownMenuWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNmnemonic, 'C'); ac++;
	XtSetArg(args[ac], XmNaccelerator, "Meta<Key>X"); ac++;
	XtSetArg(args[ac], XmNacceleratorText, 
		CONVERT(parent->widget, "Meta+X", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	_cutButton = new motif_widget(
		"Cut Selection",
		xmPushButtonWidgetClass,	_editButtonPM,
		args,				ac,
		TRUE);
	_cutButton->add_callback(
		UI_mainwindow::cutButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'y'); ac++;
	XtSetArg(args[ac], XmNaccelerator, "Meta<Key>C"); ac++;
	XtSetArg(args[ac], XmNacceleratorText, 
		CONVERT(parent->widget, "Meta+C", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	_copyButton = new motif_widget(
		"Copy Selection",
		xmPushButtonWidgetClass,	_editButtonPM,
		args,				ac,
		TRUE);
	_copyButton->set_sensitive(FALSE);
	_copyButton->add_callback(
		UI_mainwindow::copyButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'P'); ac++;
	XtSetArg(args[ac], XmNaccelerator, "Meta<Key>V"); ac++;
	XtSetArg(args[ac], XmNacceleratorText, 
		CONVERT(parent->widget, "Meta+V", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	_pasteButton = new motif_widget(
		"Paste Selection",
		xmPushButtonWidgetClass,	_editButtonPM,
		args,				ac,
		TRUE);
	_pasteButton->set_sensitive(FALSE);
	_pasteButton->add_callback(
		UI_mainwindow::pasteButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator1 = XmCreateSeparator(_editButtonPM->widget,
		(char *) "separator1",
		args, 
		ac);

	XtManageChild(separator1);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'L'); ac++;
	_activityLockSelection = new motif_widget(
		"Lock Selection",
		xmPushButtonWidgetClass,	 _editButtonPM,
		args,				ac,
		TRUE);
	_activityLockSelection->set_sensitive(FALSE);
	_activityLockSelection->add_callback(
		UI_mainwindow::activityLockButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'U'); ac++;
	_activityUnlockSelection = new motif_widget(
		"Unlock Selection",
		xmPushButtonWidgetClass,	 _editButtonPM,
		args,				ac,
		TRUE);
	_activityUnlockSelection->set_sensitive(FALSE);
	_activityUnlockSelection->add_callback(
		UI_mainwindow::activityLockButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator1 = XmCreateSeparator(_editButtonPM->widget,
		(char *) "separator1",
		args,				ac);

	XtManageChild(separator1);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'B'); ac++;
	_activityDefinitionsButton = new motif_widget(
		"Send Activity to Back",
		xmPushButtonWidgetClass,	_editButtonPM,
		args,				ac,
		TRUE);
	_activityDefinitionsButton->set_sensitive(FALSE);
	_activityDefinitionsButton->add_callback(
		UI_mainwindow::activityDefinitionsButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator1 = XmCreateSeparator(_editButtonPM->widget,
		(char *) "separator1",
		args,		ac);

	XtManageChild(separator1);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'P'); ac++;
	_preferences = new motif_widget(
		"Preferences...",
		xmPushButtonWidgetClass,	_editButtonPM,
		args,				ac,
		TRUE);
	_preferences->add_callback(
		UI_mainwindow::preferencesButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator10 = XmCreateSeparator(_editButtonPM->widget,
		(char *) "separator11",
		args, 
		ac);
	XtManageChild(separator10);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'z'); ac++;
	motif_widget* aButton = new motif_widget(
		"Edit Globals",
		xmPushButtonWidgetClass,	_editButtonPM,
		args,				ac,
		TRUE);
	aButton->add_callback(
		UI_mainwindow::editGlobalsButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsubMenuId, _editButtonPM->widget); ac++;
	XtSetValues(_editButton->widget, args, ac);


	//
	// ACTIVITY MENU
	//

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'A'); ac++;
	_activityButton = new motif_widget(
		"Activity",
		xmCascadeButtonWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNtearOffModel, XmTEAR_OFF_DISABLED); ac++;
	_activityButtonPM = new motif_widget(
		"activityButtonPM",
		xmPulldownMenuWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'N'); ac++;
	_newActivityButton = new motif_widget(
		"New Activity...",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args, ac,
		TRUE);
	_newActivityButton->set_sensitive(FALSE);
	_newActivityButton->add_callback(
		UI_exec::AddNewActivity,
		XmNactivateCallback,
		(void *) "by itself");

	ac = 0;
	_newActivitiesButton = new motif_widget(
		"New Activities...",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args, ac,
		TRUE);
	_newActivitiesButton->set_sensitive(FALSE);
	_newActivitiesButton->add_callback(
		UI_exec::AddNewActivities,
		XmNactivateCallback,
		this);

	ac = 0;
	separator2 = XmCreateSeparator(_activityButtonPM->widget,
		(char *) "separator",
		args, 
		ac);

	XtManageChild(separator2);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'A'); ac++;
	_abstractActivityButton = new motif_widget(
		"Abstract Selection",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_abstractActivityButton->add_callback(
		UI_mainwindow::abstractActivityButtonActivateCallback,
		XmNactivateCallback,
		this);
	_abstractActivityButton->set_sensitive(FALSE);

	ac = 0;
		_fullyAbstractActivityButton = new motif_widget(
		"Fully Abstract Selection" ,
		xmPushButtonWidgetClass ,	_activityButtonPM ,
		args ,				ac ,
		TRUE ) ;
	_fullyAbstractActivityButton->add_callback(
		UI_mainwindow::abstractActivityButtonActivateCallback,
		XmNactivateCallback,
		this ) ;
	_fullyAbstractActivityButton->set_sensitive( FALSE ) ;

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'D'); ac++;
	_detailActivityButton = new motif_widget(
		"Detail Selection",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_detailActivityButton->add_callback(
		UI_mainwindow::detailActivityButtonActivateCallback,
		XmNactivateCallback,
		this);
	_detailActivityButton->set_sensitive(FALSE);

	ac = 0;
	_fullyDetailActivityButton = new motif_widget(
		"Fully Detail Selection" ,
		xmPushButtonWidgetClass ,	_activityButtonPM ,
		args ,				ac ,
		TRUE ) ;
	_fullyDetailActivityButton->add_callback(
		UI_mainwindow::detailActivityButtonActivateCallback,
		XmNactivateCallback,
		this ) ;
	_fullyDetailActivityButton->set_sensitive( FALSE ) ;

	XtSetArg(args[ac], XmNmnemonic, 'R'); ac++;
	if(APcloptions::theCmdLineOptions().RedetailOptionEnabled) {
		_redetailActivityButton = new motif_widget(
			"Redetail Selection",
			xmPushButtonWidgetClass,	_activityButtonPM,
			args,				ac,
			TRUE); }
	else {
		_redetailActivityButton = new motif_widget(
			"(Re)gen Children",
			xmPushButtonWidgetClass,	_activityButtonPM,
			args,				ac,
			TRUE); }
	_redetailActivityButton->add_callback(
		UI_mainwindow::detailActivityButtonActivateCallback,
		XmNactivateCallback,
		this);
	_redetailActivityButton->set_sensitive(FALSE);

	ac = 0;
	separator11 = XmCreateSeparator(_activityButtonPM->widget,
		(char *) "separator11",
		args, 
		ac);

	XtManageChild(separator11);
 
	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 's'); ac++;
	_abstractAllButton = new motif_widget(
		"Abstract All",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_abstractAllButton->add_callback(
		UI_mainwindow::abstractActivityButtonActivateCallback,
		XmNactivateCallback,
		this);
 
	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 's'); ac++;
	_abstractAllRecursivelyButton = new motif_widget(
		"Abstract All Recursively",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_abstractAllRecursivelyButton->add_callback(
		UI_mainwindow::abstractActivityButtonActivateCallback,
		XmNactivateCallback,
		this);
 
	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'm'); ac++;
	_detailAllButton = new motif_widget(
		"Detail All",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_detailAllButton->add_callback(
		UI_mainwindow::detailActivityButtonActivateCallback,
		XmNactivateCallback,
		this);
 
	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'm'); ac++;
	_detailAllRecursivelyButton = new motif_widget(
		"Detail All Recursively",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_detailAllRecursivelyButton->add_callback(
		UI_mainwindow::detailActivityButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'q'); ac++;
	if(APcloptions::theCmdLineOptions().RedetailOptionEnabled) {
		_redetailAllButton = new motif_widget(
			"Redetail All",
			xmPushButtonWidgetClass,	_activityButtonPM,
			args,				ac,
			TRUE);
		_redetailAllRecursivelyButton = new motif_widget(
			"Redetail All Recursively",
			xmPushButtonWidgetClass,	_activityButtonPM,
			args,				ac,
			TRUE);
		}
	else {
		_redetailAllButton = new motif_widget(
			"Regen All Children",
			xmPushButtonWidgetClass,	_activityButtonPM,
			args,				ac,
			TRUE);
		_redetailAllRecursivelyButton = new motif_widget(
			"Regen All Recursively",
			xmPushButtonWidgetClass,	_activityButtonPM,
			args,				ac,
			TRUE);
		}
	_redetailAllButton->add_callback(
		UI_mainwindow::detailActivityButtonActivateCallback,
		XmNactivateCallback,
		this);
	_redetailAllRecursivelyButton->add_callback(
		UI_mainwindow::detailActivityButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator11 = XmCreateSeparator(_activityButtonPM->widget,
		(char *) "separator11",
		args, 
		ac);

	XtManageChild(separator11);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'c'); ac++;
	_deleteAllDescButton = new motif_widget(
		"Delete All Descendants",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_deleteAllDescButton->add_callback(
		UI_mainwindow::detailActivityButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator11 = XmCreateSeparator(_activityButtonPM->widget,
		(char *) "separator11",
		args, ac);

	XtManageChild(separator11);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
	_editActivityButton = new motif_widget(
		"Edit Selection...",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_editActivityButton->set_sensitive(FALSE);
	_editActivityButton->add_callback(
		UI_mainwindow::editActivityButtonActivateCallback,
		XmNactivateCallback,
		this);
#ifdef GTK_EDITOR_CLIENT
	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'w'); ac++;
	XtSetArg(args[ac], XmNaccelerator, "Meta<Key>W"); ac++;
	XtSetArg(args[ac], XmNacceleratorText, 
		CONVERT(parent->widget, "Meta+W", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	_gtkEditActivityButton = new motif_widget(
		"New Editor...",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_gtkEditActivityButton->set_sensitive(FALSE);
	_gtkEditActivityButton->add_callback(
		UI_mainwindow::editActivityButtonActivateCallback,
		XmNactivateCallback,
		this);
#endif /* GTK_EDITOR_CLIENT */

	/*	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'B'); ac++;
	_activityDesignButton = new motif_widget(
		"Design New Type or Cyclic",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_activityDesignButton->set_sensitive(TRUE);
	_activityDesignButton->add_callback(
		UI_mainwindow::activityDesignButtonActivateCallback,
		XmNactivateCallback,
		this);
	*/

	ac = 0;
	separator11 = XmCreateSeparator(_activityButtonPM->widget,
		(char *) "separator11",
		args, ac);

	XtManageChild(separator11);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
	_showActivityNameButton = new motif_widget(
		"Show Activity Name",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_showActivityNameButton->set_sensitive(FALSE);
	_showActivityNameButton->add_callback(
		UI_mainwindow::showActivityNameOrTypeActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
	_showActivityLabelButton = new motif_widget(
		"Show Label if avail.",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_showActivityLabelButton->set_sensitive(TRUE);
	_showActivityLabelButton->add_callback(
		UI_mainwindow::showActivityNameOrTypeActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
	_showActivityTypeButton = new motif_widget(
		"Show Activity Type",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_showActivityTypeButton->set_sensitive(TRUE);
	_showActivityTypeButton->add_callback(
		UI_mainwindow::showActivityNameOrTypeActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
	_showActivityIDButton = new motif_widget(
		"Show Activity ID",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_showActivityIDButton->set_sensitive(TRUE);
	_showActivityIDButton->add_callback(
		UI_mainwindow::showActivityNameOrTypeActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator11 = XmCreateSeparator(_activityButtonPM->widget,
		(char *) "separator11",
		args, ac);
	XtManageChild(separator11);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
	_groupActivitiesButton = new motif_widget(
		"Group Activities",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_groupActivitiesButton->set_sensitive(TRUE);
	// _groupActivitiesButton->add_callback(UI_mainwindow::groupActivitiesCallback, XmNactivateCallback, this);
	_groupActivitiesButton->add_callback(UI_exec::AddNewActivity, XmNactivateCallback, (void *) "parent of a group");

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
	_ungroupActivitiesButton = new motif_widget(
		"Ungroup Activities",
		xmPushButtonWidgetClass,	_activityButtonPM,
		args,				ac,
		TRUE);
	_ungroupActivitiesButton->set_sensitive(TRUE);
	_ungroupActivitiesButton->add_callback(UI_mainwindow::ungroupActivitiesCallback, XmNactivateCallback, this);

	ac = 0;
	XtSetArg(args[ac], XmNsubMenuId, _activityButtonPM->widget); ac++;
	XtSetValues(_activityButton->widget, args, ac);


	//
	// RESOURCE MENU
	//

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'R'); ac++;
	_resourceButton = new motif_widget(
		"Resource",
		xmCascadeButtonWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNtearOffModel, XmTEAR_OFF_DISABLED); ac++;
	_resourceButtonPM = new motif_widget(
		"resourceButtonPM",
		xmPulldownMenuWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'A'); ac++;
	_addResourceButton = new motif_widget(
		"Add Resource to Selected Display...",
		xmPushButtonWidgetClass,	_resourceButtonPM,
		args,				ac,
		TRUE);
	_addResourceButton->set_sensitive(FALSE);
	_addResourceButton->add_callback(
		UI_exec::AddResource,
		XmNactivateCallback,
		NULL);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'R'); ac++;
	_removeResourceButton = new motif_widget(
		"Remove Resource(s) from Selected Display(s)",
		xmPushButtonWidgetClass,	_resourceButtonPM,
		args,				ac,
		TRUE);
	_removeResourceButton->set_sensitive(FALSE);
	_removeResourceButton->add_callback(
		UI_mainwindow::removeResourceButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator8 = XmCreateSeparator(_resourceButtonPM->widget,
		(char *) "separator8",
		args, 
		ac);
	XtManageChild(separator8);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'm'); ac++;
	_remodelButton = new motif_widget(
		"Remodel",
		xmPushButtonWidgetClass,	_resourceButtonPM,
		args,				ac,
		TRUE);
	_remodelButton->add_callback(
		UI_mainwindow::remodelButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'U'); ac++;
	_unfreezeButton = new motif_widget(
		"Unfreeze resources",
		xmPushButtonWidgetClass,	_resourceButtonPM,
		args,				ac,
		TRUE);
	_unfreezeButton->add_callback(
		UI_mainwindow::unfreezeButtonActivateCallback,
		XmNactivateCallback,
		this);
	_unfreezeButton->set_sensitive(TRUE);

	ac = 0;
	separator9 = XmCreateSeparator(_resourceButtonPM->widget,
		(char *) "separator9",
		args, 
		ac);
	XtManageChild(separator9);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'D'); ac++;
	aButton = new motif_widget(
		"Resource Definitions...",
		xmPushButtonWidgetClass,	_resourceButtonPM,
		args,				ac,
		TRUE);
	aButton->add_callback(
		UI_mainwindow::resourceDefButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator10 = XmCreateSeparator(_resourceButtonPM->widget,
		(char *) "separator10",
		args, 
		ac);
	XtManageChild(separator10);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'X'); ac++;
	aButton = new motif_widget(
		"Execute AAF Script...",
		xmPushButtonWidgetClass,	_resourceButtonPM,
		args,				ac,
		TRUE);
	aButton->add_callback(
		UI_mainwindow::aafScriptButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsubMenuId, _resourceButtonPM->widget); ac++;
	XtSetValues(_resourceButton->widget, args, ac);
  }
  create_part_2(parent); 
}

// split the create() function to make the optimizing compiler's task easier:

void UI_mainwindowbx::create_part_2(motif_widget * Parent) {

	Widget		parent = Parent->widget;
	Widget		separator1, separator2, separator3, separator4, separator5,
			separator6, separator7, separator8, separator9, separator10,
			separator11;
	motif_widget	*_jplLabel, *_statusBarForm;

	if(GUI_Flag)
	{

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'S'); ac++;
	_schedulingButton = new motif_widget(
		"Scheduling",
		xmCascadeButtonWidgetClass,	_menuBar,
		args,		ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNtearOffModel, XmTEAR_OFF_DISABLED); ac++;
	_schedulingButtonPM = new motif_widget(
		"scheduleButtonPM",
		xmPulldownMenuWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'A'); ac++;
	_scheduleAllButton = new motif_widget(
		"Schedule All",
		xmPushButtonWidgetClass,	_schedulingButtonPM,
		args,				ac,
		TRUE);
	_scheduleAllButton->set_sensitive(TRUE);
	_scheduleAllButton->add_callback(
		UI_mainwindow::scheduleButtonActivateCallback,
		XmNactivateCallback,
		(void *) 0);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'S'); ac++;
	_scheduleSelectionButton = new motif_widget(
		"Schedule Selection",
		xmPushButtonWidgetClass,	_schedulingButtonPM,
		args,				ac,
		TRUE);
	_scheduleSelectionButton->set_sensitive(FALSE);
	_scheduleSelectionButton->add_callback(
		UI_mainwindow::scheduleButtonActivateCallback,
		XmNactivateCallback,
		(void *) 3);


	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'U'); ac++;
	_unscheduleButton = new motif_widget(
		"Unschedule Selection",
		xmPushButtonWidgetClass,	_schedulingButtonPM,
		args,				ac,
		TRUE);
	_unscheduleButton->set_sensitive(FALSE);
	_unscheduleButton->add_callback(
		UI_mainwindow::scheduleButtonActivateCallback,
		XmNactivateCallback,
		(void *) 1);

	ac = 0;
	// XtSetArg(args[ac], XmNmnemonic, 'I'); ac++;
	XtSetArg(args[ac], XmNmnemonic, 'n'); ac++;
	_unscheduleAllButton = new motif_widget(
		// "Improve Whole Schedule",
		"Unschedule All",
		xmPushButtonWidgetClass,	_schedulingButtonPM,
		args,				ac,
		TRUE);
	_unscheduleAllButton->set_sensitive(TRUE);
	_unscheduleAllButton->add_callback(
		UI_mainwindow::scheduleButtonActivateCallback,
		XmNactivateCallback,
		(void *) 4);

	ac = 0;
	XtSetArg(args[ac], XmNsubMenuId, _schedulingButtonPM->widget); ac++;
	XtSetValues(_schedulingButton->widget, args, ac);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'c'); ac++;
	actDisplayButton = new motif_widget(
		"Act Display",
		xmCascadeButtonWidgetClass,	_menuBar,
		args, ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'e'); ac++;
	resDisplayButton = new motif_widget(
		"Res Display",
		xmCascadeButtonWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNtearOffModel, XmTEAR_OFF_DISABLED); ac++;
	actDisplayButtonPM = new motif_widget(
		"actDisplayButtonPM",
		xmPulldownMenuWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNtearOffModel, XmTEAR_OFF_DISABLED); ac++;
	resDisplayButtonPM = new motif_widget(
		"resDisplayButtonPM",
		xmPulldownMenuWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'A'); ac++;
	_newActivityDisplayButton = new motif_widget(
		"New Activity Display",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_newActivityDisplayButton->add_callback(
		UI_mainwindow::newADButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'R'); ac++;
	_newResourceDisplayButton = new motif_widget(
		"New Resource Display",
		xmCascadeButtonWidgetClass,	resDisplayButtonPM,
		args, ac,
		TRUE);
	_newResourceDisplayButton->set_sensitive(FALSE);
	_newResourceDisplayButton->add_callback(
		UI_mainwindow::newRDButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'L'); ac++;
	_newLegendButton = new motif_widget(
		"New Legend...",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_newLegendButton->add_callback(
		UI_mainwindow::newLegendButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator2 = XmCreateSeparator(actDisplayButtonPM->widget,
		(char *) "separator2",
		args, ac);
	XtManageChild(separator2);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'C'); ac++;
	_closeSelectedADsButton = new motif_widget(
		"Close Selected Activity Display(s)",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_closeSelectedADsButton->set_sensitive(FALSE);
	_closeSelectedADsButton->add_callback(
		UI_mainwindow::closeSADButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'S'); ac++;
	_closeSelectedRDsButton = new motif_widget(
		"Close Resource Display(s)",
		xmCascadeButtonWidgetClass,	resDisplayButtonPM,
		args, ac,
		TRUE);
	_closeSelectedRDsButton->set_sensitive(FALSE);
	_closeSelectedRDsButton->add_callback(
		UI_mainwindow::closeSRDButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'E'); ac++;
	_deleteSelectedLegendsButton = new motif_widget(
		"Delete Selected Empty Legend(s)",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_deleteSelectedLegendsButton->set_sensitive(FALSE);
	_deleteSelectedLegendsButton->add_callback(
		UI_mainwindow::deleteSelectedLegendsButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'q'); ac++;
	_squishSelectedLegendsButton = new motif_widget(
		"Squish Selected Legend(s)",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_squishSelectedLegendsButton->set_sensitive(FALSE);
	_squishSelectedLegendsButton->add_callback(
		UI_mainwindow::squishSelectedLegendsButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'f'); ac++;
	_flattenSelectedLegendsButton = new motif_widget(
		"Flatten Selected Legend(s)",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_flattenSelectedLegendsButton->set_sensitive(FALSE);
	_flattenSelectedLegendsButton->add_callback(
		UI_mainwindow::flattenSelectedLegendsButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'U'); ac++;
	_unsquishSelectedLegendsButton = new motif_widget(
		"Unsquish Selected Legend(s)",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_unsquishSelectedLegendsButton->set_sensitive(FALSE);
	_unsquishSelectedLegendsButton->add_callback(
		UI_mainwindow::unsquishSelectedLegendsButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'q'); ac++;
	_expandSelectedLegendsButton = new motif_widget(
		"Expand Selected Legend(s)",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_expandSelectedLegendsButton->set_sensitive(FALSE);
	_expandSelectedLegendsButton->add_callback(
		UI_mainwindow::expandSelectedLegendsButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'U'); ac++;
	_unexpandSelectedLegendsButton = new motif_widget(
		"Unexpand Selected Legend(s)",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_unexpandSelectedLegendsButton->set_sensitive(FALSE);
	// NOTE: unexpand == unsquish
	_unexpandSelectedLegendsButton->add_callback(
		UI_mainwindow::unsquishSelectedLegendsButtonActivateCallback,
		XmNactivateCallback,
		this);


	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'l'); ac++;
	_unselectAllLegendsButton = new motif_widget(
		"Unselect All Selected Legend(s)",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_unselectAllLegendsButton->set_sensitive(FALSE);
	_unselectAllLegendsButton->add_callback(
		UI_mainwindow::unselectAllLegendsButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator4 = XmCreateSeparator(actDisplayButtonPM->widget,
		(char *) "separator4",
		args, ac);
	XtManageChild(separator4);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'I'); ac++;
	_zoominButton = new motif_widget(
		"Zoom In",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_zoominButton->add_callback(
		UI_mainwindow::zoominButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'O'); ac++;
	_zoomoutButton = new motif_widget(
		"Zoom Out",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_zoomoutButton->add_callback(
		UI_mainwindow::zoomoutButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'P'); ac++;
	_absolutePanZoomButton = new motif_widget(
		"Absolute Pan/Zoom...",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_absolutePanZoomButton->add_callback(
		UI_mainwindow::absolutePanZoomButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator9 = XmCreateSeparator(actDisplayButtonPM->widget,
		(char *) "separator9",
		args,				ac);
	XtManageChild(separator9);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'T'); ac++;
	_timeSystemButton = new motif_widget(
		"Time System...",
		xmCascadeButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_timeSystemButton->add_callback(
		UI_mainwindow::timeSystemButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator9 = XmCreateSeparator(actDisplayButtonPM->widget,
		(char *) "separator9",
		args,		ac);
	XtManageChild(separator9);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'G'); ac++;
	XtSetArg(args[ac], XmNaccelerator, "Meta<Key>G"); ac++;
	XtSetArg(args[ac], XmNacceleratorText, 
		CONVERT(parent, "Meta+G", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	_giantButton = new motif_widget(
		"Show Giant Window",
		xmPushButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_giantButton->add_callback(
		UI_mainwindow::hopperButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'T'); ac++;
	_hopperButton = new motif_widget(
		"Show Hopper",
		xmPushButtonWidgetClass,	actDisplayButtonPM,
		args,				ac,
		TRUE);
	_hopperButton->add_callback(
		UI_mainwindow::hopperButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'p'); ac++;
	_resOptionsButton = new motif_widget(
		"Options...",
		xmCascadeButtonWidgetClass,	resDisplayButtonPM,
		args,				ac,
		TRUE);
	_resOptionsButton->add_callback(
		UI_mainwindow::resOptionsButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	separator10 = XmCreateSeparator(resDisplayButtonPM->widget,
		(char *) "separator10",
		args, 
		ac);
	XtManageChild(separator10);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'I'); ac++;
	_resZoominButton = new motif_widget(
		"Zoom In",
		xmCascadeButtonWidgetClass,	resDisplayButtonPM,
		args,				ac,
		TRUE);
	_resZoominButton->set_sensitive(FALSE);

#ifdef OBSOLETE
	_resZoominButton->add_callback(
		UI_mainwindow::resZoominButtonActivateCallback,
		XmNactivateCallback,
		this);
#endif /* OBSOLETE */

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'O'); ac++;
	_resZoomoutButton = new motif_widget(
		"Zoom Out",
		xmCascadeButtonWidgetClass,	resDisplayButtonPM,
		args,				ac,
		TRUE);
	_resZoomoutButton->set_sensitive(FALSE);
#ifdef OBSOLETE
	_resZoomoutButton->add_callback(
		UI_mainwindow::resZoomoutButtonActivateCallback,
		XmNactivateCallback,
		this);
#endif /* OBSOLETE */

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'Z'); ac++;
	_resAbsScrollZoomButton = new motif_widget(
		"Absolute Scroll/Zoom...",
		xmCascadeButtonWidgetClass,	resDisplayButtonPM,
		args,				ac,
		TRUE);
	_resAbsScrollZoomButton->set_sensitive(FALSE);
	_resAbsScrollZoomButton->add_callback(
		UI_mainwindow::resAbsScrollZoomButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsubMenuId, actDisplayButtonPM->widget); ac++;
	XtSetValues(actDisplayButton->widget, args, ac);

	ac = 0;
	XtSetArg(args[ac], XmNsubMenuId, resDisplayButtonPM->widget); ac++;
	XtSetValues(resDisplayButton->widget, args, ac);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'H'); ac++;
	_helpButton = new motif_widget(
		"Help",
		xmCascadeButtonWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNtearOffModel, XmTEAR_OFF_DISABLED); ac++;
	_helpButtonPM = new motif_widget(
		"helpButtonPM",
		xmPulldownMenuWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'H'); ac++;
	_apgenHelpButton = new motif_widget(
		"APGEN Help...",
		xmPushButtonWidgetClass,	_helpButtonPM,
		args, ac,
		TRUE);
	_apgenHelpButton->add_callback(
		UI_mainwindow::apgenHelpButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'S'); ac++;
	_searchForHelpOnButton = new motif_widget(
		"Search for Help on...",
		xmPushButtonWidgetClass,	_helpButtonPM,
		args, ac,
		TRUE);
	_searchForHelpOnButton->set_sensitive(FALSE);

	ac = 0;
	separator3 = XmCreateSeparator(_helpButtonPM->widget,
		(char *) "separator3",
		args, 
		ac);
	XtManageChild(separator3);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'A'); ac++;
	_aboutApgenButton = new motif_widget(
		"About APGEN...",
		xmPushButtonWidgetClass,	_helpButtonPM,
		args, ac,
		TRUE);
	_aboutApgenButton->add_callback(
		UI_mainwindow::aboutApgenButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNsubMenuId, _helpButtonPM->widget); ac++;
	XtSetValues(_helpButton->widget, args, ac);


#ifdef INCLUDE_ADEF

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'C'); ac++;
	_componentsButton = new motif_widget(
		"Components...",
		xmPushButtonWidgetClass,	_helpButtonPM,
		args, ac,
		TRUE);
	_componentsButton->add_callback(
		UI_mainwindow::aboutComponentsButtonActivateCallback,
		XmNactivateCallback,
		this);

#endif

#ifdef HAVE_EUROPA
	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'P'); ac++;
	_planButton = new motif_widget(
		"Planning",
		xmCascadeButtonWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNtearOffModel, XmTEAR_OFF_DISABLED); ac++;
	_planButtonPM = new motif_widget(
		"planButtonPM",
		xmPulldownMenuWidgetClass,	_menuBar,
		args,				ac,
		TRUE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'I'); ac++;
	_plannerOnButton = new motif_widget(
		"Planner On",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_plannerOnButton->add_callback(
		UI_mainwindow::planInitButtonActivateCallback,
		XmNactivateCallback,
		this);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'I'); ac++;
	_plannerOffButton = new motif_widget(
		"Planner Off",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_plannerOffButton->add_callback(
		UI_mainwindow::planInitButtonActivateCallback,
		XmNactivateCallback,
		this);
	_plannerOffButton->set_sensitive(FALSE);

	_planInitButton = _plannerOnButton;

	/*
	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'I'); ac++;
	_planInitButton = new motif_widget(
		"Plan Init",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_planInitButton->add_callback(
		UI_mainwindow::planInitButtonActivateCallback,
		XmNactivateCallback,
		this);
	_planInitButton->set_sensitive(FALSE);
	*/

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'P'); ac++;
	_planAllButton = new motif_widget(
		"Plan All",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_planAllButton->add_callback(
		UI_mainwindow::planAllButtonActivateCallback,
		XmNactivateCallback,
		this);
	_planAllButton->set_sensitive(FALSE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'G'); ac++;
	_planGoalButton = new motif_widget(
		"Plan Selected Goal(s)",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_planGoalButton->add_callback(
		UI_mainwindow::planGoalButtonActivateCallback,
		XmNactivateCallback,
		this);
	_planGoalButton->set_sensitive(FALSE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'U'); ac++;
	_unplanGoalButton = new motif_widget(
		"UnPlan Selected Goal(s)",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_unplanGoalButton->add_callback(
		UI_mainwindow::unplanGoalButtonActivateCallback,
		XmNactivateCallback,
		this);
	_unplanGoalButton->set_sensitive(FALSE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'R'); ac++;
	_fixResourcesButton = new motif_widget(
		"Fix Resources",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_fixResourcesButton->add_callback(
		UI_mainwindow::fixResourceButtonActivateCallback,
		XmNactivateCallback,
		this);
	_fixResourcesButton->set_sensitive(FALSE);

	ac = 0;
	XtSetArg(args[ac], XmNmnemonic, 'D'); ac++;
	_displayParentsButton = new motif_widget(
		"Display Parents",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_displayParentsButton->add_callback(
		UI_mainwindow::displayParentsButtonActivateCallback,
		XmNactivateCallback,
		this);
	_displayParentsButton->set_sensitive(FALSE);

	ac = 0;
/*	XtSetArg(args[ac], XmNmnemonic, 'U'); ac++;
	_undoPlanningButton = new motif_widget(
		"Undo Latest Planning",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_undoPlanningButton->add_callback(
		UI_mainwindow::undoPlanningButtonActivateCallback,
		XmNactivateCallback,
		this);
	_undoPlanningButton->set_sensitive(FALSE);
*/
	ac = 0;
	_placeSelectedGoalButton = new motif_widget(
		"Place Selected Goal",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);
	_placeSelectedGoalButton->add_callback(
		UI_mainwindow::placeSelectedGoalButtonActivateCallback,
		XmNactivateCallback,
		this);
	_placeSelectedGoalButton->set_sensitive(FALSE);

	ac = 0;
	_setRelaxedModeButton = new motif_widget(
		"Set Relaxed Enforcement Mode",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_setRelaxedModeButton->add_callback(
	    UI_mainwindow::setRelaxedModeButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_setRelaxedModeButton->set_sensitive(FALSE);

	ac = 0;
	_setFullModeButton = new motif_widget(
		"Set Full Enforcement Mode",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_setFullModeButton->add_callback(
	    UI_mainwindow::setFullModeButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_setFullModeButton->set_sensitive(FALSE);

	ac = 0;
	_clearConstraintsButton = new motif_widget(
		"Clear All Constraints",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_clearConstraintsButton->add_callback(
	    UI_mainwindow::clearConstraintsButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_clearConstraintsButton->set_sensitive(FALSE);


//pin-related buttons

	ac = 0;
	_pinSelectedButton = new motif_widget(
		"Pin Selected",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_pinSelectedButton->add_callback(
	    UI_mainwindow::pinSelectedButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_pinSelectedButton->set_sensitive(FALSE);

	ac = 0;
	_unpinSelectedButton = new motif_widget(
		"UnPin Selected",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_unpinSelectedButton->add_callback(
	    UI_mainwindow::unpinSelectedButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_unpinSelectedButton->set_sensitive(FALSE);

	ac = 0;
	_unpinAllButton = new motif_widget(
		"UnPin All",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_unpinAllButton->add_callback(
	    UI_mainwindow::unpinAllButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_unpinAllButton->set_sensitive(FALSE);

	ac = 0;

	_selectPlannedActivitiesButton = new motif_widget(
		"Select All Planned Activities",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_selectPlannedActivitiesButton->add_callback(
	    UI_mainwindow::selectPlannedActivitiesButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_selectPlannedActivitiesButton->set_sensitive(FALSE);

	//
	ac = 0;

	_addHeatingContraintsButton = new motif_widget(
		"Add Heating Constraints",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_addHeatingContraintsButton->add_callback(
	    UI_mainwindow::addHeatingConstraintsButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_addHeatingContraintsButton->set_sensitive(FALSE);
	//
	ac = 0;

	_removeHeatingContraintsButton = new motif_widget(
		"Remove Heating Constraints",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_removeHeatingContraintsButton->add_callback(
	    UI_mainwindow::removeHeatingConstraintsButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_removeHeatingContraintsButton->set_sensitive(FALSE);
	//
	ac = 0;

	_regenerateHeaterActivitiesButton = new motif_widget(
		"Regenerate Heater Activities",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_regenerateHeaterActivitiesButton->add_callback(
	    UI_mainwindow::regenerateHeaterActivitiesButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_regenerateHeaterActivitiesButton->set_sensitive(FALSE);
	//
	ac = 0;

	_updateSelectedHeaterActivitiesButton = new motif_widget(
		"Update Selected Heater Activities",
		xmPushButtonWidgetClass,	_planButtonPM,
		args, ac,
		TRUE);

	_updateSelectedHeaterActivitiesButton->add_callback(
	    UI_mainwindow::updateSelectedHeaterActivitiesButtonActivateCallback,
	    XmNactivateCallback,
	    this);

	_updateSelectedHeaterActivitiesButton->set_sensitive(FALSE);
	//

	ac = 0;
	XtSetArg(args[ac], XmNsubMenuId, _planButtonPM->widget); ac++;
	XtSetValues(_planButton->widget, args, ac);

#endif

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	if (APcloptions::theCmdLineOptions().roomy_display)
	{
	    //Setting the following resources results in more space for ADs and
	    //  RDs at the expense of appearance:
	    XtSetArg(args[ac], XmNmarginHeight, 1); ac++; //default==3; works
	    XtSetArg(args[ac], XmNmarginWidth, 1); ac++;  //default==3; works
	    XtSetArg(args[ac], XmNsashHeight, 6); ac++;	  //default==10; works
	    //XtSetArg(args[ac],XmNseparatorOn,False);ac++;//no,need visual cue
	    XtSetArg(args[ac], XmNspacing, 4); ac++; 	  //default==8; works
	}
	}
	_panedWindow = new motif_widget(
		"panedWindow",
		xmPanedWindowWidgetClass,	this,
		args,				ac,
		TRUE);
	if(GUI_Flag)
	{
	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_GROW); ac++;
	XtSetArg(args[ac], XmNx, 3); ac++;
	XtSetArg(args[ac], XmNy, 3); ac++;
	// XtSetArg(args[ac], XmNpaneMaximum, 3000); ac++;
	XtSetArg(args[ac], XmNwidth, 700); ac++;
	if (APcloptions::theCmdLineOptions().roomy_display)
	    { XtSetArg(args[ac], XmNheight, 32); ac++; }
	else // original
	    { XtSetArg(args[ac], XmNheight, 40); ac++; }
	_toolbarForm = new motif_widget(
		"toolbarForm",
		xmFormWidgetClass,	_panedWindow,
		args,			ac,
		TRUE);
#ifdef HAVE_MACOS
	_toolbarForm->fix_height(40, _toolbarForm->widget);
#else
	_toolbarForm->fix_height(_toolbarForm->widget);
#endif /* HAVE_MACOS */

	_statusBarForm = new motif_widget(
		"statusBarForm",
		xmFormWidgetClass,	this,
		NULL,			0,
		TRUE);
	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 1); ac++;
	XtSetArg(args[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;
	XtSetArg(args[ac], XmNmarginLeft, 3); ac++;
	XtSetArg(args[ac], XmNbackground, ResourceData.grey_color); ac++;
	// if (APcloptions::theCmdLineOptions().roomy_display) {
	XtSetArg(args[ac], XmNheight, 18); ac++;
	//	}
	// else original
	//     { XtSetArg(args[ac], XmNheight, 25); ac++; }
	_statusBarLabel = new motif_widget(
		"statusBarLabel",
		xmLabelWidgetClass,	_statusBarForm,
		args,			ac,
		TRUE);
	*_statusBarLabel = "";

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, True); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, (char **) jpl_logo));  ac++;
	XtSetArg(args[ac], XmNwidth, 69); ac++;
	XtSetArg(args[ac], XmNheight, 18); ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	_jplLabel = new motif_widget(
		"JPL_label",
		xmLabelWidgetClass,	_statusBarForm,
		args,			ac,
		TRUE);
	*_statusBarForm < *_statusBarLabel < 69 < *_statusBarForm < endform;
	*_jplLabel < *_statusBarForm < endform;
	*_statusBarForm ^ *_statusBarLabel ^ *_statusBarForm ^ endform;
	*_statusBarForm ^ *_jplLabel ^ *_statusBarForm ^ endform;

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, print_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, printoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 550); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_printToolbarButton = new motif_widget(
		"printToolbarButton",
		xmPushButtonWidgetClass, _toolbarForm,
		args, ac,
		TRUE);

	_printToolbarButton->add_callback(
		UI_mainwindow::printButtonActivateCallback,
		XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_printToolbarButton->widget);


	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, remodel_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, remodeloff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 500); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_remodelToolbarButton = new motif_widget(
		"remodelToolbarButton",
		xmPushButtonWidgetClass,	_toolbarForm,
		args, ac,
		TRUE);
	_remodelToolbarButton->add_callback(
		UI_mainwindow::remodelButtonActivateCallback,
		XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_remodelToolbarButton->widget);


	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, closeSRD_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, closeSRDoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 380); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_closeSRDToolbarButton = new motif_widget(
		"closeSRDToolbarButton",
		xmPushButtonWidgetClass,_toolbarForm,
		args, 
		ac, TRUE);
	_closeSRDToolbarButton->add_callback(
		UI_mainwindow::closeSRDButtonActivateCallback,
		XmNactivateCallback,
		this);

		//add tooltip
	xmAddTooltip(_closeSRDToolbarButton->widget);

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, newRD_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, newRDoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 340); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_newRDToolbarButton = new motif_widget(
		"newRDToolbarButton",
		xmPushButtonWidgetClass,_toolbarForm,
		args, 
		ac, TRUE);
	_newRDToolbarButton->add_callback(
		UI_mainwindow::newRDButtonActivateCallback,
		XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_newRDToolbarButton->widget);


	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, zoomout_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, zoomoutoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 290); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_zoomoutToolbarButton = new motif_widget(
		"zoomoutToolbarButton",
		xmPushButtonWidgetClass,_toolbarForm,
		args, 
		ac, TRUE);
	_zoomoutToolbarButton->add_callback(
		UI_mainwindow::zoomoutButtonActivateCallback,
		XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_zoomoutToolbarButton->widget);

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, zoomin_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, zoominoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 250); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_zoominToolbarButton = new motif_widget(
		"zoominToolbarButton",
		xmPushButtonWidgetClass,_toolbarForm,
		args, 
		ac, TRUE);
	_zoominToolbarButton->add_callback(
		UI_mainwindow::zoominButtonActivateCallback,
		XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_zoominToolbarButton->widget);

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, closeSD_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, closeSDoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 210); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_closeSADToolbarButton = new motif_widget(
		"closeSADToolbarButton",
		xmPushButtonWidgetClass,_toolbarForm,
		args, 
		ac, TRUE);
	_closeSADToolbarButton->add_callback(
		UI_mainwindow::closeSADButtonActivateCallback,
		XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_closeSADToolbarButton->widget);

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, newAD_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, newADoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 170); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_newADToolbarButton = new motif_widget(
		"newADToolbarButton",
		xmPushButtonWidgetClass,_toolbarForm,
		args, 
		ac, TRUE);
	_newADToolbarButton->add_callback(
		UI_mainwindow::newADButtonActivateCallback,
		XmNactivateCallback,
		this);
	
	//add tooltip
	xmAddTooltip(_newADToolbarButton->widget);

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, actedit_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, acteditoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 120); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_editActToolbarButton = new motif_widget(
		"editActToolbarButton",
		xmPushButtonWidgetClass,_toolbarForm,
		args, 
		ac, TRUE);
	_editActToolbarButton->add_callback(
		UI_mainwindow::editActivityButtonActivateCallback,
		XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_editActToolbarButton->widget);

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, pastexpm_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, pasteGOxpm_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 80); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_pasteToolbarButton = new motif_widget(
		"pasteToolbarButton",
		xmPushButtonWidgetClass,_toolbarForm,
		args, 
		ac, TRUE);
	_pasteToolbarButton->add_callback(
		UI_mainwindow::pasteButtonActivateCallback,
	XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_pasteToolbarButton->widget);

	ac = 0;
	XtSetArg(args[ac], XmNborderWidth, 0); ac++;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, copy_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, copyoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNx, 40); ac++;
	XtSetArg(args[ac], XmNy, 0); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_copyToolbarButton = new motif_widget(
		"copyToolbarButton",
		xmPushButtonWidgetClass,_toolbarForm,
		args, 
		ac, TRUE);
	_copyToolbarButton->add_callback(
		UI_mainwindow::copyButtonActivateCallback,
		XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_copyToolbarButton->widget);

	ac = 0;
	XtSetArg(args[ac], XmNsensitive, False); ac++;
	XtSetArg(args[ac], XmNhighlightColor, 
		CONVERT(parent, "black", 
		XmRPixel, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNlabelType, XmPIXMAP); ac++;
	XtSetArg(args[ac], XmNlabelPixmap, XPM_PIXMAP(parent, cut_icon));  ac++;
	XtSetArg(args[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(parent, cutoff_icon));  ac++;
	XtSetArg(args[ac], XmNlabelString, 
		CONVERT(parent, "", 
		XmRXmString, 0, &argok)); if (argok) ac++;
	XtSetArg(args[ac], XmNrecomputeSize, False); ac++;
	XtSetArg(args[ac], XmNwidth, 40); ac++;
	XtSetArg(args[ac], XmNheight, 40); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	_cutToolbarButton = new motif_widget(
		"cutToolbarButton",
		xmPushButtonWidgetClass, _toolbarForm,
		args, ac,
		TRUE);
	_cutToolbarButton->add_callback(
		UI_mainwindow::cutButtonActivateCallback,
		XmNactivateCallback,
		this);

	//add tooltip
	xmAddTooltip(_cutToolbarButton->widget);

	ac = 0;
	XtSetArg(args[ac], XmNworkWindow, _panedWindow->widget); ac++;
	XtSetArg(args[ac], XmNmenuBar, _menuBar->widget); ac++;
	// XtSetArg(args[ac], XmNmessageWindow, _statusBarLabel->widget); ac++;
	XtSetArg(args[ac], XmNmessageWindow, _statusBarForm->widget); ac++;
	XtSetValues(widget, args, ac);

	ac = 0;
	XtSetArg(args[ac], XmNmenuHelpWidget, _helpButton->widget); ac++;
	XtSetValues(_menuBar->widget, args, ac);

	*_cutToolbarButton < *_copyToolbarButton < *_pasteToolbarButton < *_editActToolbarButton <
		10 < *_newADToolbarButton < *_closeSADToolbarButton < *_zoominToolbarButton < *_zoomoutToolbarButton <
		10 < *_newRDToolbarButton < *_closeSRDToolbarButton < /* *_rdZoomInToolbarButton <
		*_rdZoomOutToolbarButton < */ *_remodelToolbarButton < 10 < *_printToolbarButton 
#ifdef HAVE_EUROPA

		< 10 < *_plannerStateToolbarButton < *_plannerModeToolbarButton
#endif
		< endform;
	*_toolbarForm ^ *_cutToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_copyToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_pasteToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_editActToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_newADToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_closeSADToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_zoominToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_zoomoutToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_newRDToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_closeSRDToolbarButton ^ *_toolbarForm ^ endform;
	// *_toolbarForm ^ *_rdZoomInToolbarButton ^ *_toolbarForm ^ endform;
	// *_toolbarForm ^ *_rdZoomOutToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_remodelToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_printToolbarButton ^ *_toolbarForm ^ endform; 

#ifdef HAVE_EUROPA
	
	*_toolbarForm ^ *_plannerStateToolbarButton ^ *_toolbarForm ^ endform;
	*_toolbarForm ^ *_plannerModeToolbarButton ^ *_toolbarForm ^ endform;

#endif

}}

void UI_mainwindow::showActivityNameOrTypeActivateCallback(Widget, 
		callback_stuff * clientData, XtPointer) {
	if(GUI_Flag) {
	UI_mainwindow		*obj = (UI_mainwindow *) clientData->data;
	List_iterator		the_actsisses(ACT_sys::Registration());
	ACT_sys			*actsis;
	Pointer_node		*ptr;

	// Force a refresh:
	while((ptr = (Pointer_node *) the_actsisses())) {
		actsis = (ACT_sys *) ptr->get_ptr();
		actsis->cleargraph(); }
	if(clientData->parent == obj->_showActivityNameButton) {
		DS_smart_text::showTheActivityName = 1;
		obj->_showActivityNameButton->set_sensitive(FALSE);
		obj->_showActivityTypeButton->set_sensitive(TRUE);
		obj->_showActivityIDButton->set_sensitive(TRUE);
		obj->_showActivityLabelButton->set_sensitive(TRUE);
		}
	else if(clientData->parent == obj->_showActivityTypeButton) {
		DS_smart_text::showTheActivityName = 0;
		obj->_showActivityNameButton->set_sensitive(TRUE);
		obj->_showActivityIDButton->set_sensitive(TRUE);
		obj->_showActivityLabelButton->set_sensitive(TRUE);
		obj->_showActivityTypeButton->set_sensitive(FALSE); }
	else if(clientData->parent == obj->_showActivityIDButton) {
		DS_smart_text::showTheActivityName = -1;
		obj->_showActivityIDButton->set_sensitive(FALSE);
		obj->_showActivityLabelButton->set_sensitive(TRUE);
		obj->_showActivityNameButton->set_sensitive(TRUE);
		obj->_showActivityTypeButton->set_sensitive(TRUE); }
	else if(clientData->parent == obj->_showActivityLabelButton) {
		DS_smart_text::showTheActivityName = 2;
		obj->_showActivityIDButton->set_sensitive(TRUE);
		obj->_showActivityLabelButton->set_sensitive(FALSE);
		obj->_showActivityNameButton->set_sensitive(TRUE);
		obj->_showActivityTypeButton->set_sensitive(TRUE); }
		} }


#ifdef HAVE_EUROPA

void 
UI_mainwindowbx::SetPlannerButton(bool on)
{
#ifdef GUI

	if(GUI_Flag)
	{

		int ac = 0;
		Arg arg[2];

		if(on)
		{
			XtSetArg(arg[ac], XmNlabelPixmap, XPM_PIXMAP(myParent->widget, planner_on)); ac++;
			XtSetArg(arg[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(myParent->widget, planner_on)); ac++;

		}
		else
		{
			XtSetArg(arg[ac], XmNlabelPixmap, XPM_PIXMAP(myParent->widget, planner_off)); ac++;
			XtSetArg(arg[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(myParent->widget, planner_off)); ac++;

		}

		XtSetValues(_plannerStateToolbarButton->widget, arg, ac);
	}

#endif
}

void
UI_mainwindowbx::SetRelaxedButton(bool on)
{

#ifdef GUI

	if(GUI_Flag)
	{

		int ac = 0;
		Arg arg[2];

		if(on)
		{
			XtSetArg(arg[ac], XmNlabelPixmap, XPM_PIXMAP(myParent->widget, relaxed_on)); ac++;
			XtSetArg(arg[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(myParent->widget, relaxed_on)); ac++;

		}
		else
		{
			XtSetArg(arg[ac], XmNlabelPixmap, XPM_PIXMAP(myParent->widget, relaxed_off)); ac++;
			XtSetArg(arg[ac], XmNlabelInsensitivePixmap, XPM_PIXMAP(myParent->widget, relaxed_off)); ac++;

		}

		XtSetValues(_plannerModeToolbarButton->widget, arg, ac);

	}
#endif
}

#endif //HAVE_EUROPA


#endif // ifdef GUI
