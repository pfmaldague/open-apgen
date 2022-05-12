#if HAVE_CONFIG_H
#include <config.h>
#endif
// #define apDEBUG

#include "apDEBUG.H"

#ifdef GUI
#include <X11/cursorfont.h>
#include <Xm/DialogS.h>
#include <Xm/PushB.h>
#endif

#include "ACT_sys.H"
#include "ActivityInstance.H"
#include "APerrors.H"
#include "APmodel.H"
#include "C_string.H"
#include "CMD_exec.H"
#include "CON_sys.H"
#include "DB.H"
#include "IO_write.H"
#include "UI_abspanzoom.H"
#include "UI_activitydisplay.H"
#include "UI_exec.H"
#include "UI_mainwindow.H"
#include "UI_messagewarndialog.H"
#include "UI_openfsd.H"
#include "UI_printdialog.H"
#include "UI_resabsscrollzoom.H"
#include "UI_resourcedisplay.H"
#include "UI_save.H"

#ifdef have_xmltol
#	include "UI_xmltol.H"
#endif /* have_xmltol */

#include "UTL_time.H"
#include "apcoreWaiter.H"

using namespace std;


// STATICS:
#ifdef GUI
static Arg      			args[56];
#endif
static int				warned_about_exit = 0;

	    		/*
	    		 * MAX_NUM_ACTIVITY_DISPLAYS is set to 5.
	    		 * MAX_NUM_REG_ACTSYS is set to 3.
	    		 */
activity_display*			UI_mainwindow::activityDisplay[TOTAL_NUM_ACTSYS];

// GLOBALS:

UI_mainwindow*				UI_mainwindow::mainWindowObject = NULL;
int					UI_mainwindow::weFoundSomethingToDoInADelayedManner = 0;
int					UI_mainwindow::thisEventIsSynthetic = 0;

popup_list				*time_system_selection_panel = NULL;
popup_list				*resource_popup = NULL;
popup_list				*script_popup = NULL;
popup_scrolled_text			*resource_def_popup = NULL;
hopper_popup                            *theHopperPopup = NULL;

pwd_popup*				UI_mainwindow::passwordDisplay = NULL;

// EXTERNS:

extern UI_abspanzoom			*absPanZoomDialog;
extern UI_openfsd			*openFSD;
extern UI_printdialog			*printDialog;
extern UI_newLegend			*_nlPopupDialogShell;
extern UI_rdOptions			*_rdOptionsPopupDialogShell;
extern UI_resabsscrollzoom		*resAbsScrollZoomDialog;

	    		// in UI_ds_timeline.C:
extern int				SelectionFrozen;
extern void				set_type_of_new_act_to(const Cstring &);

	    		// in UI_exec.C:
extern UI_exec				*UI_subsystem;

	    		// in main.C:
extern motif_widget*			TopLevelWidget;
extern motif_widget*			MW;

	    		// in ACT_io.C:
extern int				ConstraintsActive;

	    		// in IO_seqtalk.C:
extern int              		theOutPort;
extern Cstring          		theName;
extern int              		send_message(
						const Cstring & header_prefix,
						const Cstring & message_body,
						int port = theOutPort);

	    		// in UI_popups.C:
extern tol_popup*			_tolPopupDialogShell;

extern const char*			get_apgen_version_build_platform();


#ifdef GTK_EDITOR

#include "gtk_bridge.H"

#endif /* GTK_EDITOR */

extern "C" {
    // in server.c:
    extern void disable_signals(int);
    extern void enable_signals();

    // in APmain.C:
    typedef void (*simple_get_handle)(const char* URL, char** result);
    extern simple_get_handle	theSimpleGetHandle;
}



#define INCLUDE_ANNOYING_MESSAGE

sasf_popup	*&UI_mainwindow::sasf_popup_instance() {
    static sasf_popup *foo = NULL;
    return foo; }

/////////////////////////////////////////////////////////
// Constructors, Destructors, and Initializing Methods //
/////////////////////////////////////////////////////////

UI_mainwindow::UI_mainwindow(const char *name, motif_widget *parent)
    : UI_mainwindowbx(name, parent) {
#ifdef GUI
    // UI_mainwindowbx method that creates the base widgets (menubar, toolbar, status bar, paned window):
    create(parent);
#endif
    // UI_mainwindow method that creates the ACT_sys- and RD_sys-related widgets:
    initialize(); }

UI_mainwindow::~UI_mainwindow()
    {}

void UI_mainwindow::initialize() {
    int			i;
    activity_display	*AD;


    mainWindowObject = this;
#ifdef GUI
    if(GUI_Flag) {
	DS_text::classinit();
    }
#endif

    for(i = 0; i < MAX_NUM_REG_ACTSYS; i++) {
	// Note: the activity_display constructor will stick a pointer to its object in the UI_mainwindow::activityDisplay[] array
	AD = new activity_display(Cstring("activityDisplay_") + i, _panedWindow, 484, 334);
	AD->initialize(Cstring());
    }

    // trick to avoid collapsing the entire window:
    ((motif_widget *) activityDisplay[0]->children[0])->unmanage();
    activityDisplay[0]->manage();

}

void UI_mainwindow::initialize_hopper() {

    /* The containers should create their own activity_display objects. Note that the
     * activity_display constructor is very simple (it just builds a form). The actual
     * work involved in creating the display is done by the activity_display::initialize()
     * method.  */
	// create the two "hopper containers"
    theHopperPopup = new hopper_popup("Hopper", TopLevelWidget);
}

class auto_flag_raiser {
    int		theInternalFlag;
    infoList*	L;
public:
    auto_flag_raiser(infoList* l);
    ~auto_flag_raiser();
    Cstring		reason;
    void		defuse(); };

void	auto_flag_raiser::defuse() {
	theInternalFlag = 0;
	if(L && L->last_node()) {
	    delete L->last_node(); } }

auto_flag_raiser::auto_flag_raiser(infoList* l)
    : L(l) {
	// DBG_INDENT("UI_mainwindow::isOutOfSync() starts\n");
	theInternalFlag = 1; }

auto_flag_raiser::~auto_flag_raiser() {
    if(theInternalFlag) {
#		ifdef apDEBUG
	if(!UI_mainwindow::weFoundSomethingToDoInADelayedManner) {
	    assert(reason.length() > 2);
	    DBG_NOINDENT("UI_mainwindow::isOutOfSync() found something to do in delayed mode: " << reason << "\n"); }
	else {
	    DBG_NOINDENT("UI_mainwindow::isOutOfSync() previously found something to do in delayed mode.\n"); }
#		endif
	UI_mainwindow::weFoundSomethingToDoInADelayedManner = 1; }
    // too verbose:
    // else {
    // 	DBG_UNINDENT("UI_mainwindow::isOutOfSync() ends (nothing found)\n"); }
    }

int UI_mainwindow::isOutOfSync(infoList* who) {
    static int		i, j;
    int			number_of_act_displays_visible = 0;
    int			number_of_act_displays_selected = 0;
    int			number_of_res_displays_visible[3] = { 0, 0, 0 };
    int			number_of_res_displays_selected[3] = { 0, 0, 0 };
    int			there_is_at_least_one_sel_act_disp_w_less_than_max_num_of_res_disp = 0;
    int			there_is_at_least_one_selected_res_display_w_a_resource = 0;
    int			there_is_at_least_one_selected_res_display_w_o_a_resource = 0;
    int			at_least_one_res_legend_is_selected = 0;
    List_iterator		all_legs(Dsource::theLegends());
    LegendObject		*theLeg;
    int			at_least_one_legend_is_selected = 0;
    infoNode*		tl = NULL;
    auto_flag_raiser	AFR(who);
    btag			*a_hopper_tag;
    ACT_hopper		*a_hopper;

    if(who) {
	tl = new infoNode("mainwindow (server)");
	(*who) << tl; }
    if(GUI_Flag) {
    if(weFoundSomethingToDoInADelayedManner) {
	AFR.reason = " we found something to do in a delayed manner flag is set.\n";
	return 1; }
    if(ACT_exec::ACT_subsystem().execAgent()->act_instances_with_discrepant_durations.get_length()) {
	AFR.reason = " there are activity instances with a duration discrepancy.\n";
	return 1; }
    while((theLeg = (LegendObject *) all_legs())) {
	// Hopper design: should restrict the iteration to non-Hopper ACT_sisses at this point
	if(theLeg->get_selection()) {
	    at_least_one_legend_is_selected = 1;
	    break; } }
    // We need a good way to loop through hoppers. OK, from the
    // ACT_hopper constructor, here is a good choice:
    for(	a_hopper_tag = (btag *)ACT_sys::Hoppers().first_node();
	a_hopper_tag;
	a_hopper_tag = (btag *)a_hopper_tag->next_node()) {
	a_hopper = (ACT_hopper *)a_hopper_tag->get_pointer();
	if(a_hopper->get_AD_parent()->isVisible()) {
	    number_of_act_displays_visible++;
	}
    }
    for (i = 0; i < MAX_NUM_REG_ACTSYS; ++i) {
	if(	activityDisplay[i]->isVisible()) {
	    number_of_act_displays_visible++;
	    // only if it's not a hopper;
	    if(i < MAX_NUM_REG_ACTSYS) {
	    	if (activityDisplay[i]->isSelected() && activityDisplay[i]->resourceDisplay->is_managed()) {
	    		MW_object	*mwo;
	    		List_iterator	theResourceLegends(activityDisplay[i]->resourceDisplay
	    					->resourceSystem->legend_display
	    					->get_blist_of_vertically_scrolled_items());

	    		while((mwo = (MW_object *) theResourceLegends())) {
	    			if(mwo->get_selection()) {
	    				at_least_one_res_legend_is_selected = 1;
	    				break; } }
	    		if(at_least_one_res_legend_is_selected) {
	    			if(!activityDisplay[i]->resourceDisplay->LegendsSelectedButtonsAreSensitive()) {
	    				AFR.reason = "AD window has unsync'ed buttons";
	    				return 1; } }
	    		else {
	    			if(activityDisplay[i]->resourceDisplay->LegendsSelectedButtonsAreSensitive()) {
	    				AFR.reason = "RD window has unsync'ed buttons";
	    				return 1; } }
	    		number_of_res_displays_selected[i] = 1;
	    		number_of_res_displays_visible[i] = 1; }
	    	if(at_least_one_legend_is_selected) {
	    		if(!mainWindowObject->_deleteSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to activate delete selected legends button.\n";
	    			return 1; }
	    		if(!mainWindowObject->_squishSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to activate squish selected legends button.\n";
	    			return 1; }
	    		if(!mainWindowObject->_flattenSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to activate flatten selected legends button.\n";
	    			return 1; }
	    		if(!mainWindowObject->_expandSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to activate expand selected legends button.\n";
	    			return 1; }
	    		if(!mainWindowObject->_unexpandSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to activate unexpand selected legends button.\n";
	    			return 1; }
	    		if(!mainWindowObject->_unselectAllLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to activate unselect selected legends button.\n";
	    			return 1; }
	    		if(!mainWindowObject->_unsquishSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to activate unselect selected legends button.\n";
	    			return 1; }
	    		if(!activityDisplay[i]->LegendsSelectedButtonsAreSensitive()) {
	    			AFR.reason = " need to activate unselect selected legends button.\n";
	    			return 1;
	    		}
	    	}
	    	else {
	    		if(mainWindowObject->_deleteSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to deactivate unselect selected legends button.\n";
	    			return 1; }
	    		if(mainWindowObject->_squishSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to deactivate unselect selected legends button.\n";
	    			return 1; }
	    		if(mainWindowObject->_flattenSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to deactivate unselect selected legends button.\n";
	    			return 1; }
	    		if(mainWindowObject->_unsquishSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to deactivate unselect selected legends button.\n";
	    			return 1; }
	    		if(mainWindowObject->_expandSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to deactivate unselect selected legends button.\n";
	    			return 1; }
	    		if(mainWindowObject->_unexpandSelectedLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to deactivate unselect selected legends button.\n";
	    			return 1; }
	    		if(mainWindowObject->_unselectAllLegendsButton->is_sensitive()) {
	    			AFR.reason = " need to deactivate unselect selected legends button.\n";
	    			return 1; }
	    		if(activityDisplay[i]->LegendsSelectedButtonsAreSensitive()) {
	    			AFR.reason = " need to deactivate unselect selected legends button.\n";
	    			return 1;
	    		}
	    	}
	    	if(activityDisplay[i]->isSelected()) {
	    		number_of_act_displays_selected++;
	    	}
	    }
	}
    }
    for (i = 0; i < MAX_NUM_REG_ACTSYS; ++i) {
	if(	activityDisplay[i]->isVisible() && activityDisplay[i]->isSelected()) {
	    if(number_of_res_displays_visible[i] == 0) {
	    	there_is_at_least_one_sel_act_disp_w_less_than_max_num_of_res_disp = 1;
	    	if((!mainWindowObject->_newRDToolbarButton->is_sensitive())
	    	    || (!mainWindowObject->_newResourceDisplayButton->is_sensitive())) {
	    		AFR.reason = " New RD requested.\n";
	    		return 1;
	    	}
	    }
	    if(number_of_res_displays_selected[i] > 0) {
	    	// We need to count the number of selected MW_objects
	    	List_iterator	resource_plots(activityDisplay[i]->
	    					resourceDisplay->
	    					resourceSystem->
	    					legend_display->
	    					get_blist_of_vertically_scrolled_items());
	    	MW_object	*mo;

	    	while((mo = (MW_object *) resource_plots())) {
	    		if(mo->get_selection()) {
	    			there_is_at_least_one_selected_res_display_w_a_resource = 1;
	    			break;
	    		}
	    	}
	    	there_is_at_least_one_selected_res_display_w_o_a_resource = 1;
	    }
	}
    }
    if(!there_is_at_least_one_sel_act_disp_w_less_than_max_num_of_res_disp) {
	if(	(mainWindowObject->_newRDToolbarButton->is_sensitive())
	    || (mainWindowObject->_newResourceDisplayButton->is_sensitive())) {
	    	AFR.reason = " New RD requested.\n";
	    	return 1;
	}
    }
    if(there_is_at_least_one_selected_res_display_w_a_resource) {
	if(!mainWindowObject->_removeResourceButton->is_sensitive()) {
	    AFR.reason = " RD deletion requested.\n";
	    return 1;
	}
    } else {
	if(mainWindowObject->_removeResourceButton->is_sensitive()) {
	    AFR.reason = " RD deletion requested.\n";
	    return 1;
	}
    }
    if(there_is_at_least_one_selected_res_display_w_o_a_resource) {
	if(!mainWindowObject->_addResourceButton->is_sensitive()) {
	    AFR.reason = " RD unselected.\n";
	    return 1;
	}
    } else {
	if(mainWindowObject->_addResourceButton->is_sensitive()) {
	    AFR.reason = " RD unselected.\n";
	    return 1;
	}
    }
    if(number_of_res_displays_selected[0] + number_of_res_displays_selected[1] + number_of_res_displays_selected[2]) {
	int			ad_i, rd_i;

	    if(!mainWindowObject->_addResourceButton->is_sensitive()) {
	    AFR.reason = " RD unselected.\n";
	    return 1; }
	    if(	(!mainWindowObject->_closeSRDToolbarButton->is_sensitive())
	    || (!mainWindowObject->_closeSelectedRDsButton->is_sensitive())
	    ) {
	    AFR.reason = " RD unselected.\n";
	    return 1; }
	if(at_least_one_res_legend_is_selected) {
	    if(!mainWindowObject->_resOptionsButton->is_sensitive()) {
	    	AFR.reason = " RD options selected.\n";
	    	return 1; } }
	else {
	    if(mainWindowObject->_resOptionsButton->is_sensitive()) {
	    	AFR.reason = " RD options selected.\n";
	    	return 1; } }
	UI_mainwindow::findFirstSelectedRD(ad_i, rd_i, IN_MANUAL_MODE);
	if(ad_i < 0) {
	    /// nothing in manual mode...
	    if(mainWindowObject->_resAbsScrollZoomButton->is_sensitive()) {
	    	AFR.reason = " RD options selected.\n";
	    	return 1; } }
	else {
	    /// at least one RD in manual mode...
	    if(!mainWindowObject->_resAbsScrollZoomButton->is_sensitive()) {
	    	AFR.reason = " RD options selected.\n";
	    	return 1;
	    }
	}
    } else {
	    if(	mainWindowObject->_closeSRDToolbarButton->is_sensitive()
	    || mainWindowObject->_closeSelectedRDsButton->is_sensitive()
	    || mainWindowObject->_addResourceButton->is_sensitive()
	    || mainWindowObject->_resOptionsButton->is_sensitive()
	    || mainWindowObject->_resAbsScrollZoomButton->is_sensitive()
	    ) {
	    AFR.reason = " RD options selected.\n";
	    return 1;
	}
    }
    if(number_of_act_displays_visible) {
	    if((! mainWindowObject->_newActivityButton->is_sensitive()) 
	   || (!mainWindowObject->_newActivitiesButton->is_sensitive())) {
	    AFR.reason = " AD selected.\n";
	    return 1;
	}
    } else {
	    if((mainWindowObject->_newActivityButton->is_sensitive()) 
	   ||(mainWindowObject->_newActivitiesButton->is_sensitive())) {
	    AFR.reason = " AD selected.\n";
	    return 1;
	}
    }
    if(number_of_act_displays_selected) {
	if(	(!mainWindowObject->_zoominToolbarButton->is_sensitive())
	    || (!mainWindowObject->_zoomoutToolbarButton->is_sensitive())
	    || (!mainWindowObject->_closeSADToolbarButton->is_sensitive())
 			|| (!mainWindowObject->_printToolbarButton->is_sensitive())
	    || (!mainWindowObject->_closeSelectedADsButton->is_sensitive())
	    || (!mainWindowObject->_printButton->is_sensitive())
	    || (!mainWindowObject->_zoominButton->is_sensitive())
	    || (!mainWindowObject->_zoomoutButton->is_sensitive())
	    || (!mainWindowObject->_absolutePanZoomButton->is_sensitive())
	    || (!mainWindowObject->_timeSystemButton->is_sensitive())
     		/* || (!mainWindowObject->_activityDisplayControlButton->is_sensitive())
	     * || (!mainWindowObject->_remodelOneStepAtATimeButton->is_sensitive())
	     */
	  ) {
	    AFR.reason = " AD present.\n";
	    return 1;
	}
    } else {
	if(	(mainWindowObject->_zoominToolbarButton->is_sensitive())
	    || (mainWindowObject->_zoomoutToolbarButton->is_sensitive())
	    || (mainWindowObject->_closeSADToolbarButton->is_sensitive())
 			|| (mainWindowObject->_printToolbarButton->is_sensitive())
	    || (mainWindowObject->_closeSelectedADsButton->is_sensitive())
	    || (mainWindowObject->_printButton->is_sensitive())
	    || (mainWindowObject->_zoominButton->is_sensitive())
	    || (mainWindowObject->_zoomoutButton->is_sensitive())
	    || (mainWindowObject->_absolutePanZoomButton->is_sensitive())
	    || (mainWindowObject->_timeSystemButton->is_sensitive())
	    /* || (mainWindowObject->_activityDisplayControlButton->is_sensitive())
	     * || (mainWindowObject->_remodelOneStepAtATimeButton->is_sensitive())
	     */
	  ) {
	    AFR.reason = " AD present.\n";
	    return 1;
	}
    }
    if(number_of_act_displays_visible >= 3) {
	    if(	mainWindowObject->_newADToolbarButton->is_sensitive()
	    	|| mainWindowObject->_newActivityDisplayButton->is_sensitive()) {
	    AFR.reason = " AD present.\n";
	    return 1; } }
    if(eval_intfc::get_act_lists().get_clipboard_length() && !SelectionFrozen) {
	if(	(!mainWindowObject->_pasteToolbarButton->is_sensitive())
	    || (!mainWindowObject->_pasteButton->is_sensitive())) {
	    AFR.reason = " AD present.\n";
	    return 1; } }
    else {
	if(	mainWindowObject->_pasteToolbarButton->is_sensitive()
	    || mainWindowObject->_pasteButton->is_sensitive()) {
	    AFR.reason = " AD present.\n";
	    return 1; } }
    if(eval_intfc::get_act_lists().get_active_length()) {
	    if(	(!mainWindowObject->_integratePlanFilesButton->is_sensitive())
	    	|| (!mainWindowObject->_writeIntegratedPlanFileButton->is_sensitive())
	    	|| (!mainWindowObject->_genTOLButton->is_sensitive())
	    	|| (!mainWindowObject->_genXMLTOLButton->is_sensitive())
	    	|| (!mainWindowObject->_genCsourceButton->is_sensitive())
	    	|| (!mainWindowObject->_genSASFButton->is_sensitive())) {
	    AFR.reason = " AD present.\n";
	    return 1; } }
    else {
	    if(	mainWindowObject->_integratePlanFilesButton->is_sensitive()
	    	|| mainWindowObject->_writeIntegratedPlanFileButton->is_sensitive()
	    || mainWindowObject->_exportDataButton->is_sensitive()
	    	|| mainWindowObject->_genTOLButton->is_sensitive()
	    	|| mainWindowObject->_genXMLTOLButton->is_sensitive()
	    	|| mainWindowObject->_genCsourceButton->is_sensitive()
	    	|| mainWindowObject->_genSASFButton->is_sensitive()) {
	    AFR.reason = " AD present.\n";
	    return 1; } }
    if(SelectionFrozen) {
	if(	mainWindowObject->_abstractAllButton->is_sensitive()
	    || mainWindowObject->_detailAllButton->is_sensitive()
	    || mainWindowObject->_redetailAllButton->is_sensitive()) {
	    AFR.reason = " AD present.\n";
	    return 1; } }
    else {
	if(	(!mainWindowObject->_abstractAllButton->is_sensitive())
	    || (!mainWindowObject->_detailAllButton->is_sensitive())
	    || (!mainWindowObject->_redetailAllButton->is_sensitive())) {
	    AFR.reason = " AD present.\n";
	    return 1; } }
    if(ACT_exec::there_are_active_selections()) {
	if(SelectionFrozen) {
	    if(	(!mainWindowObject->_activityUnlockSelection->is_sensitive())
	    	|| mainWindowObject->_activityLockSelection->is_sensitive()
	    		|| mainWindowObject->_abstractActivityButton->is_sensitive()
	    	|| mainWindowObject->_fullyAbstractActivityButton->is_sensitive()
	    		|| mainWindowObject->_detailActivityButton->is_sensitive()
	    	|| mainWindowObject->_fullyDetailActivityButton->is_sensitive()
	    		|| mainWindowObject->_redetailActivityButton->is_sensitive()
	    		|| mainWindowObject->_groupActivitiesButton->is_sensitive()
	    		|| mainWindowObject->_ungroupActivitiesButton->is_sensitive()
	    	|| mainWindowObject->_scheduleSelectionButton->is_sensitive()
	    	|| mainWindowObject->_unscheduleButton->is_sensitive()) {
	    	AFR.reason = " active selection present.\n";
	    	return 1; } }
	else {
	    if(	mainWindowObject->_activityUnlockSelection->is_sensitive()
	    	|| (!mainWindowObject->_activityLockSelection->is_sensitive())
	    		|| (!mainWindowObject->_abstractActivityButton->is_sensitive())
	    		|| (!mainWindowObject->_fullyAbstractActivityButton->is_sensitive())
	    		|| (!mainWindowObject->_detailActivityButton->is_sensitive())
	    		|| (!mainWindowObject->_fullyDetailActivityButton->is_sensitive())
	    		|| (!mainWindowObject->_redetailActivityButton->is_sensitive())
	    		|| (!mainWindowObject->_groupActivitiesButton->is_sensitive())
	    		|| (!mainWindowObject->_ungroupActivitiesButton->is_sensitive())
	    	|| (!mainWindowObject->_scheduleSelectionButton->is_sensitive())
	    	|| (!mainWindowObject->_unscheduleButton->is_sensitive())) {
	    	AFR.reason = " active selection present.\n";
	    	return 1; } }
	if(	(!mainWindowObject->_cutToolbarButton->is_sensitive())
	    || (!mainWindowObject->_cutButton->is_sensitive())
	    || (!mainWindowObject->_copyToolbarButton->is_sensitive())
	    || (!mainWindowObject->_copyButton->is_sensitive())
	    	|| (!mainWindowObject->_editActToolbarButton->is_sensitive())
	    	|| (!mainWindowObject->_editActivityButton->is_sensitive())
#			ifdef GTK_EDITOR_CLIENT
	    	|| (!mainWindowObject->_gtkEditActivityButton->is_sensitive())
#			endif /* GTK_EDITOR_CLIENT */
	    ) {
	    	AFR.reason = " active selection present.\n";
	    	return 1; } }
    else {
	if(	mainWindowObject->_activityUnlockSelection->is_sensitive()
	    || mainWindowObject->_activityLockSelection->is_sensitive()
	    	|| mainWindowObject->_abstractActivityButton->is_sensitive()
	    	|| mainWindowObject->_fullyAbstractActivityButton->is_sensitive()
	    	|| mainWindowObject->_detailActivityButton->is_sensitive()
	    	|| mainWindowObject->_fullyDetailActivityButton->is_sensitive()
	    	|| mainWindowObject->_redetailActivityButton->is_sensitive()
	    || mainWindowObject->_cutToolbarButton->is_sensitive()
	    || mainWindowObject->_cutButton->is_sensitive()
	    || mainWindowObject->_copyToolbarButton->is_sensitive()
	    || mainWindowObject->_copyButton->is_sensitive()
	    	|| mainWindowObject->_editActToolbarButton->is_sensitive()
	    || mainWindowObject->_scheduleSelectionButton->is_sensitive()
	    || mainWindowObject->_unscheduleButton->is_sensitive()
	    	|| mainWindowObject->_editActivityButton->is_sensitive()
#			ifdef GTK_EDITOR_CLIENT
	    	|| mainWindowObject->_gtkEditActivityButton->is_sensitive()
#			endif /* GTK_EDITOR_CLIENT */
	    ) {
	    AFR.reason = " active selection present.\n";
	    return 1; } }
    }
    // This used to be inside the GUI_Flag test; we most likely don't call this function if
    // there is no GUI, but in any case it's better to defuse here!
    AFR.defuse();
    return 0; }

void UI_mainwindow::bringToolbarButtonsInSync() {
    int			number_of_act_displays_selected = 0;
    int			number_of_act_displays_visible = 0;
    int			number_of_res_displays_visible[3] = { 0, 0, 0 };
    int			number_of_res_displays_selected[3] = { 0, 0, 0 };
    int			there_is_at_least_one_selected_res_display_w_o_a_resource = 0;
    int			at_least_one_res_legend_is_selected = 0;
    int			there_is_at_least_one_sel_act_disp_w_less_than_max_num_of_res_disp = 0;
    int			at_least_one_legend_is_selected = 0;
    int			i, j;
    List_iterator		all_legs(Dsource::theLegends());
    LegendObject		*theLeg;
    btag			*a_hopper_tag;
    ACT_hopper		*a_hopper;

    while((theLeg = (LegendObject *) all_legs())) {
	if(theLeg->get_selection()) {
	    at_least_one_legend_is_selected = 1;
	    break; } }

    weFoundSomethingToDoInADelayedManner = 0;
    if(ACT_exec::ACT_subsystem().execAgent()->act_instances_with_discrepant_durations.get_length()) {
	UI_openfsd::pop_the_discrepancy_panel(); }
    // We need a good way to loop through hoppers. OK, from the
    // ACT_hopper constructor, here is a good choice:
    for(	a_hopper_tag = (btag *)ACT_sys::Hoppers().first_node();
	a_hopper_tag;
	a_hopper_tag = (btag *)a_hopper_tag->next_node()) {
	a_hopper = (ACT_hopper *)a_hopper_tag->get_pointer();
	if(a_hopper->get_AD_parent()->isVisible()) {
	    number_of_act_displays_visible++;
	}
    }
    for (i = 0; i < TOTAL_NUM_ACTSYS; ++i) {
	if(activityDisplay[i]->isVisible()) {
	    number_of_act_displays_visible++;
	    if(at_least_one_legend_is_selected) {
	    	mainWindowObject->_deleteSelectedLegendsButton->set_sensitive(TRUE);
	    	mainWindowObject->_squishSelectedLegendsButton->set_sensitive(TRUE);
	    	mainWindowObject->_flattenSelectedLegendsButton->set_sensitive(TRUE);
	    	mainWindowObject->_expandSelectedLegendsButton->set_sensitive(TRUE);
	    	mainWindowObject->_unexpandSelectedLegendsButton->set_sensitive(TRUE);
	    	mainWindowObject->_unselectAllLegendsButton->set_sensitive(TRUE);
	    	mainWindowObject->_unsquishSelectedLegendsButton->set_sensitive(TRUE);
	    	activityDisplay[i]->sensitizeLegendsSelectedButtons();
	    } else {
	    	mainWindowObject->_deleteSelectedLegendsButton->set_sensitive(FALSE);
	    	mainWindowObject->_squishSelectedLegendsButton->set_sensitive(FALSE);
	    	mainWindowObject->_flattenSelectedLegendsButton->set_sensitive(FALSE);
	    	mainWindowObject->_expandSelectedLegendsButton->set_sensitive(FALSE);
	    	mainWindowObject->_unexpandSelectedLegendsButton->set_sensitive(FALSE);
	    	mainWindowObject->_unsquishSelectedLegendsButton->set_sensitive(FALSE);
	    	mainWindowObject->_unselectAllLegendsButton->set_sensitive(FALSE);
	    	activityDisplay[i]->desensitizeLegendsSelectedButtons();
	    }
	    if(activityDisplay[i]->isSelected()) {
	    	number_of_act_displays_selected++;
	    }
	    // debug
	    // cerr << "UI_mainwindow::bringToolbarButtonsInSync(): checking resource display...";
	    if ((i < MAX_NUM_REG_ACTSYS) && activityDisplay[i]->resourceDisplay->is_managed() &&
	    	activityDisplay[i]->isSelected()) {
	    	MW_object	*mwo;
	    	List_iterator	theResourceLegends(activityDisplay[i]->resourceDisplay->resourceSystem->legend_display->get_blist_of_vertically_scrolled_items());

	    	while((mwo = (MW_object *) theResourceLegends())) {
	    		if(mwo->get_selection()) {
	    			at_least_one_res_legend_is_selected = 1;
	    			break;
	    		}
	    	}
	    	// debug
	    	// cerr << " IS managed.\n";
	    	number_of_res_displays_visible[i]++;
	    	number_of_res_displays_selected[i]++;
	    	there_is_at_least_one_selected_res_display_w_o_a_resource = 1;
	    	if(at_least_one_res_legend_is_selected) {
	    		activityDisplay[i]->resourceDisplay->sensitizeLegendsSelectedButtons();
	    	} else {
	    		activityDisplay[i]->resourceDisplay->desensitizeLegendsSelectedButtons();
	    	}
	    } else {
	    	// debug
	    	// cerr << " NOT managed.\n";
	    }
	}
    }
    for (i = 0; i < MAX_NUM_REG_ACTSYS; ++i) {
	if(activityDisplay[i]->isVisible() && activityDisplay[i]->isSelected()) {
	    if(number_of_res_displays_visible[i] == 0) {
	    	there_is_at_least_one_sel_act_disp_w_less_than_max_num_of_res_disp = 1;
	    	mainWindowObject->_newResourceDisplayButton->set_sensitive(TRUE);
	    	mainWindowObject->_newRDToolbarButton->set_sensitive(TRUE);
	    }
	}
    }
    if(!there_is_at_least_one_sel_act_disp_w_less_than_max_num_of_res_disp) {
	mainWindowObject->_newResourceDisplayButton->set_sensitive(FALSE);
	mainWindowObject->_newRDToolbarButton->set_sensitive(FALSE);
    }
    if(at_least_one_res_legend_is_selected) {
	mainWindowObject->_resOptionsButton->set_sensitive(TRUE);
	mainWindowObject->_removeResourceButton->set_sensitive(TRUE);
    } else {
	mainWindowObject->_resOptionsButton->set_sensitive(FALSE);
	mainWindowObject->_removeResourceButton->set_sensitive(FALSE); }
    if(there_is_at_least_one_selected_res_display_w_o_a_resource) {
	// debug
	// cerr << "Sensitizing add resource button.\n";
	mainWindowObject->_addResourceButton->set_sensitive(TRUE);
    } else {
	// debug
	// cerr << "De-sensitizing add resource button.\n";
	mainWindowObject->_addResourceButton->set_sensitive(FALSE);
    }
    if(number_of_act_displays_visible) {
       		mainWindowObject->_newActivityButton->set_sensitive(TRUE); 
	mainWindowObject->_newActivitiesButton->set_sensitive(TRUE);
    } else {
       		mainWindowObject->_newActivitiesButton->set_sensitive(FALSE); 
       		mainWindowObject->_newActivityButton->set_sensitive(FALSE);
    }
    if(number_of_act_displays_selected) {
     	mainWindowObject->_zoominToolbarButton->set_sensitive(TRUE);
	mainWindowObject->_zoomoutToolbarButton->set_sensitive(TRUE);
     	mainWindowObject->_closeSADToolbarButton->set_sensitive(TRUE);
 		mainWindowObject->_printToolbarButton->set_sensitive(TRUE);
	mainWindowObject->_closeSelectedADsButton->set_sensitive(TRUE);
	mainWindowObject->_printButton->set_sensitive(TRUE);
	mainWindowObject->_zoominButton->set_sensitive(TRUE);
	mainWindowObject->_zoomoutButton->set_sensitive(TRUE);
	mainWindowObject->_absolutePanZoomButton->set_sensitive(TRUE);
	mainWindowObject->_timeSystemButton->set_sensitive(TRUE);
	/* mainWindowObject->_activityDisplayControlButton->set_sensitive(TRUE);
	 * mainWindowObject->_remodelOneStepAtATimeButton->set_sensitive(TRUE);
	 */
    } else {
	mainWindowObject->_zoominToolbarButton->set_sensitive(FALSE);
	mainWindowObject->_zoomoutToolbarButton->set_sensitive(FALSE);
	mainWindowObject->_closeSADToolbarButton->set_sensitive(FALSE);
 		mainWindowObject->_printToolbarButton->set_sensitive(FALSE);
	mainWindowObject->_closeSelectedADsButton->set_sensitive(FALSE);
	mainWindowObject->_printButton->set_sensitive(FALSE);
	mainWindowObject->_zoominButton->set_sensitive(FALSE);
	mainWindowObject->_zoomoutButton->set_sensitive(FALSE);
	mainWindowObject->_absolutePanZoomButton->set_sensitive(FALSE);
	mainWindowObject->_timeSystemButton->set_sensitive(FALSE);
	// mainWindowObject->_activityDisplayControlButton->set_sensitive(FALSE);
	// mainWindowObject->_remodelOneStepAtATimeButton->set_sensitive(FALSE);
    }
    if(number_of_res_displays_selected[0] + number_of_res_displays_selected[1] + number_of_res_displays_selected[2]) {
	int			ad_i, rd_i;

	    mainWindowObject->_closeSRDToolbarButton->set_sensitive(TRUE);
	    mainWindowObject->_closeSelectedRDsButton->set_sensitive(TRUE);

	UI_mainwindow::findFirstSelectedRD(ad_i, rd_i, IN_MANUAL_MODE);
	if(ad_i < 0) {
	    /// nothing in manual mode...
	    mainWindowObject->_resZoominButton->set_sensitive(FALSE);
	    mainWindowObject->_resZoomoutButton->set_sensitive(FALSE);
	    mainWindowObject->_resAbsScrollZoomButton->set_sensitive(FALSE); }
	else {
	    /// at least one RD in manual mode...
	    mainWindowObject->_resZoominButton->set_sensitive(TRUE);
	    mainWindowObject->_resZoomoutButton->set_sensitive(TRUE);
	    mainWindowObject->_resAbsScrollZoomButton->set_sensitive(TRUE); }
	UI_mainwindow::findFirstSelectedRD(ad_i, rd_i, WITHOUT_RESOURCE); }
    else {
	    mainWindowObject->_closeSRDToolbarButton->set_sensitive(FALSE);
	    mainWindowObject->_closeSelectedRDsButton->set_sensitive(FALSE);
	mainWindowObject->_resOptionsButton->set_sensitive(FALSE);
	mainWindowObject->_resZoominButton->set_sensitive(FALSE);
	mainWindowObject->_resZoomoutButton->set_sensitive(FALSE);
	mainWindowObject->_resAbsScrollZoomButton->set_sensitive(FALSE); }
    if(number_of_act_displays_visible >= 3) {
	    mainWindowObject->_newADToolbarButton->set_sensitive(FALSE);
	    mainWindowObject->_newActivityDisplayButton->set_sensitive(FALSE); }
    else {
	    mainWindowObject->_newADToolbarButton->set_sensitive(TRUE);
	    mainWindowObject->_newActivityDisplayButton->set_sensitive(TRUE); }
    if(eval_intfc::get_act_lists().get_clipboard_length() && !SelectionFrozen) {
	mainWindowObject->_pasteToolbarButton->set_sensitive(TRUE);
	mainWindowObject->_pasteButton->set_sensitive(TRUE); }
    else {
	mainWindowObject->_pasteToolbarButton->set_sensitive(FALSE);
	mainWindowObject->_pasteButton->set_sensitive(FALSE); }
    if(eval_intfc::get_act_lists().get_active_length()) {
	    mainWindowObject->_integratePlanFilesButton->set_sensitive(TRUE);
	    mainWindowObject->_writeIntegratedPlanFileButton->set_sensitive(TRUE);
	mainWindowObject->_exportDataButton->set_sensitive(TRUE);
	    mainWindowObject->_genTOLButton->set_sensitive(TRUE);
	    mainWindowObject->_genXMLTOLButton->set_sensitive(TRUE);
	    mainWindowObject->_genCsourceButton->set_sensitive(TRUE);
	    mainWindowObject->_genSASFButton->set_sensitive(TRUE); }
    else {
	    mainWindowObject->_integratePlanFilesButton->set_sensitive(FALSE);
	    mainWindowObject->_writeIntegratedPlanFileButton->set_sensitive(FALSE);
	mainWindowObject->_exportDataButton->set_sensitive(FALSE);
	    mainWindowObject->_genTOLButton->set_sensitive(FALSE);
	    mainWindowObject->_genXMLTOLButton->set_sensitive(FALSE);
	    mainWindowObject->_genCsourceButton->set_sensitive(FALSE);
	    mainWindowObject->_genSASFButton->set_sensitive(FALSE); }
    if(SelectionFrozen) {
	mainWindowObject->_abstractAllButton->set_sensitive(FALSE);
	mainWindowObject->_detailAllButton->set_sensitive(FALSE);
	mainWindowObject->_redetailAllButton->set_sensitive(FALSE); }
    else {
	mainWindowObject->_abstractAllButton->set_sensitive(TRUE);
	mainWindowObject->_detailAllButton->set_sensitive(TRUE);
	mainWindowObject->_redetailAllButton->set_sensitive(TRUE); }
    if(ACT_exec::there_are_active_selections()) {
	if(SelectionFrozen) {
	    mainWindowObject->_activityUnlockSelection->set_sensitive(TRUE);
	    mainWindowObject->_activityLockSelection->set_sensitive(FALSE);
	    	mainWindowObject->_abstractActivityButton->set_sensitive(FALSE);
	    	mainWindowObject->_fullyAbstractActivityButton->set_sensitive(FALSE);
	    	mainWindowObject->_detailActivityButton->set_sensitive(FALSE);
	    	mainWindowObject->_fullyDetailActivityButton->set_sensitive(FALSE);
	    	mainWindowObject->_redetailActivityButton->set_sensitive(FALSE);
	    	mainWindowObject->_groupActivitiesButton->set_sensitive(FALSE);
	    	mainWindowObject->_ungroupActivitiesButton->set_sensitive(FALSE);
	    mainWindowObject->_scheduleSelectionButton->set_sensitive(FALSE);
	    mainWindowObject->_unscheduleButton->set_sensitive(FALSE); }
	else {
	    mainWindowObject->_activityUnlockSelection->set_sensitive(FALSE);
	    mainWindowObject->_activityLockSelection->set_sensitive(TRUE);
	    	mainWindowObject->_abstractActivityButton->set_sensitive(TRUE);
	    	mainWindowObject->_fullyAbstractActivityButton->set_sensitive(TRUE);
	    	mainWindowObject->_detailActivityButton->set_sensitive(TRUE);
	    	mainWindowObject->_fullyDetailActivityButton->set_sensitive(TRUE);
	    	mainWindowObject->_redetailActivityButton->set_sensitive(TRUE);
	    	mainWindowObject->_groupActivitiesButton->set_sensitive(TRUE);
	    	mainWindowObject->_ungroupActivitiesButton->set_sensitive(TRUE);
	    mainWindowObject->_scheduleSelectionButton->set_sensitive(TRUE);
	    mainWindowObject->_unscheduleButton->set_sensitive(TRUE); }
	mainWindowObject->_cutToolbarButton->set_sensitive(TRUE);
	mainWindowObject->_cutButton->set_sensitive(TRUE);
	mainWindowObject->_copyToolbarButton->set_sensitive(TRUE);
	mainWindowObject->_copyButton->set_sensitive(TRUE);
	    mainWindowObject->_editActToolbarButton->set_sensitive(TRUE);
	    mainWindowObject->_editActivityButton->set_sensitive(TRUE);
#		ifdef GTK_EDITOR_CLIENT
	    mainWindowObject->_gtkEditActivityButton->set_sensitive(TRUE);
#		endif /* GTK_EDITOR_CLIENT */
	}
    else {
	mainWindowObject->_activityUnlockSelection->set_sensitive(FALSE);
	mainWindowObject->_activityLockSelection->set_sensitive(FALSE);
	    mainWindowObject->_abstractActivityButton->set_sensitive(FALSE);
	    mainWindowObject->_fullyAbstractActivityButton->set_sensitive(FALSE);
	    mainWindowObject->_detailActivityButton->set_sensitive(FALSE);
	    mainWindowObject->_fullyDetailActivityButton->set_sensitive(FALSE);
	    mainWindowObject->_redetailActivityButton->set_sensitive(FALSE);
	mainWindowObject->_cutToolbarButton->set_sensitive(FALSE);
	mainWindowObject->_cutButton->set_sensitive(FALSE);
	mainWindowObject->_copyToolbarButton->set_sensitive(FALSE);
	mainWindowObject->_copyButton->set_sensitive(FALSE);
	    mainWindowObject->_editActToolbarButton->set_sensitive(FALSE);
	    mainWindowObject->_groupActivitiesButton->set_sensitive(FALSE);
	    mainWindowObject->_ungroupActivitiesButton->set_sensitive(FALSE);
	    mainWindowObject->_editActivityButton->set_sensitive(FALSE);
	mainWindowObject->_scheduleSelectionButton->set_sensitive(FALSE);
	mainWindowObject->_unscheduleButton->set_sensitive(FALSE);
#		ifdef GTK_EDITOR_CLIENT
	    mainWindowObject->_gtkEditActivityButton->set_sensitive(FALSE);
#		endif /* GTK_EDITOR_CLIENT */
	}
}

void UI_mainwindow::send_a_dummy_event_to_unblock_XtAppNextEvent() {
    if(GUI_Flag) {
	XEvent		theEvent;

	theEvent.type = ButtonPress;
	UI_mainwindow::thisEventIsSynthetic = 1;
	// KEEP WINDOW
	XSendEvent(XtDisplay(mainWindowObject->widget),
	    XtWindow(mainWindowObject->widget),
	    False,
	    ButtonPressMask,
	    &theEvent);
    }
}

void UI_mainwindow::hopperButtonActivateCallback(Widget, callback_stuff *Stuff, void *) {
    if(Stuff->parent == mainWindowObject->_giantButton) {
	cerr << "APGEN INTERNAL ERROR -- no giant window found.\n";
	return;
    } else if(Stuff->parent == mainWindowObject->_hopperButton) {
	activity_display	*AD;
	ACT_hopper		*a_hopper;
	btag			*theHopperTag;

	theHopperTag = (btag *)ACT_sys::Hoppers().find("0Hopper");
	if(!theHopperTag) {
	    cerr << "APGEN INTERNAL ERROR -- no hopper found.\n";
	    return; }
	a_hopper = (ACT_hopper *)theHopperTag->get_pointer();
	AD = a_hopper->get_AD_parent();
	AD->manage();
	AD->myParent->manage();
    }
}

void UI_mainwindow::printButtonActivateCallback(Widget, callback_stuff *, void *) {
    ACT_sys		*activitySystem;
    int			adIndex = findFirstSelectedAD();

    if (adIndex < 0) {
    displayMessage("Print","Print Message",
	    "There is no selected activity display."); }
    else {
    //97-09-17 DSG -- no longer check for 0 legends selected -- OK!

    activitySystem = activityDisplay[adIndex]->getActivitySystem();

    //Create the UI_printdialog, if hasn't been yet:
    if(! printDialog)
	printDialog = new UI_printdialog("Print Selected Display(s)");

    //UI_printdialog::initialize() not yet called, but OK init. for now.
    printDialog->manage();
    }
}

apgen::RETURN_STATUS UI_mainwindow::printButtonUpdateSelections() {
    ACT_sys		*activitySystem;
    List		managedSelectedResourceSystemList; //of Pointer_node's
    int			j;

    int adIndex = findFirstSelectedAD();
    if (adIndex < 0) {
    displayMessage("Print","Print Message", "There is no selected activity display.");

    return apgen::RETURN_STATUS::FAIL; }
    //97-09-17 DSG -- no longer check for 0 legends selected -- OK!

    activitySystem = activityDisplay[adIndex]->getActivitySystem();

    if ((activityDisplay[adIndex]->resourceDisplay->is_managed())
	      && (activityDisplay[adIndex]->resourceDisplay->is_managed())) {
	RD_sys		*res_sis = activityDisplay[adIndex]->resourceDisplay->resourceSystem;
	List_iterator	theMWobjects(res_sis->legend_display->get_blist_of_vertically_scrolled_items());
	MW_object	*mo;

	while((mo = (MW_object *) theMWobjects())) {
	    if(mo->get_selection()) {
	    	managedSelectedResourceSystemList << new Pointer_node(mo, NULL); } } }
    UI_printdialog::initialize(	activitySystem,
	    		activityDisplay[adIndex]->resourceDisplay->resourceSystem,
	    		managedSelectedResourceSystemList,
	    		activitySystem->timezone);
    return apgen::RETURN_STATUS::SUCCESS; }

void UI_mainwindow::chooseResourceToDisplay(Widget, callback_stuff *, void *) {
    Rsource*		theResource = eval_intfc::FindResource(
	    			resource_popup->chosen_text->get_text());

    if(theResource) {
	aoString	s;
	char*		contents;

	if(!resource_def_popup) {
	    resource_def_popup = new popup_scrolled_text("Resource Definition", MW, NewResourceDefOK, MW);
	    resource_def_popup->create();
	}
	theResource->Type.to_stream(&s, 0);
	*resource_def_popup->chosen_text = s.str();
	resource_def_popup->paned_window->manage();
    }
}

void UI_mainwindow::chooseScriptToExecute(Widget, callback_stuff*, void*) {
    slist<alpha_void, dumb_actptr>	active_selections;
    XCMDrequest*			actreq;

    ACT_exec::get_active_selections(active_selections);
    actreq = new XCMDrequest(active_selections, script_popup->chosen_text->get_text());
    actreq->process();
    delete actreq;
    script_popup->paned_window->unmanage();
}

void UI_mainwindow::NewResourceDefOK(Widget, callback_stuff *client_data, void *) {}

void UI_mainwindow::resourceDefButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
#ifdef GUI
    if(GUI_Flag) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;
    Cntnr<alpha_void, Rsource*>*					ptr;
    tlist<alpha_void, Cntnr<alpha_void, Rsource*> >		theListOfResources;
    slist<alpha_void, Cntnr<alpha_void, Rsource*> >::iterator	theResources(theListOfResources);
    Rsource		*res;

    if(!resource_popup) {
	resource_popup = new popup_list("Resources", MW, chooseResourceToDisplay, obj, 0);
	XtVaSetValues(resource_popup->chosen_text->widget,
	    XmNeditable,           False,  //FR088:  user can't edit/add types
	    NULL);
    } else {
	resource_popup->scrolled_list->clear();
    }
    eval_intfc::get_list_of_ptrs_to_resources(theListOfResources);
    *((motif_widget *) resource_popup->chosen_text) = "";
    resource_popup->scrolled_list->clear();
    while((ptr = theResources())) {
	res = ptr->payload;
	(*resource_popup->scrolled_list) << res->name;
    }
    resource_popup->paned_window->manage();
    // obj->setStatusBarMessage ("A future version will actually DO something.");
}
#endif
}

void UI_mainwindow::aafScriptButtonActivateCallback(Widget, callback_stuff* client_data, void*) {
#ifdef GUI
    if(GUI_Flag) {
    UI_mainwindow*	obj = (UI_mainwindow *) client_data->data;

    task*		maybe_a_script = NULL;
    Behavior*	global_type = &Behavior::GlobalType();
    vector<task*>	the_scripts;

    for(int i = 0; i < global_type->tasks.size(); i++) {
	maybe_a_script = global_type->tasks[i];
	if(maybe_a_script->script_flag) {
	    the_scripts.push_back(maybe_a_script);
	}
    }

    if(!script_popup) {
	script_popup = new popup_list(
	    "AAF Scripts",
	    MW,
	    chooseScriptToExecute,
	    obj,
	    0);
	XtVaSetValues(script_popup->chosen_text->widget,
	    XmNeditable,           False,
	    NULL);
    } else {
	script_popup->scrolled_list->clear();
    }
    *((motif_widget *) script_popup->chosen_text) = "";
    script_popup->scrolled_list->clear();
    for(int i = 0; i < the_scripts.size(); i++) {
	(*script_popup->scrolled_list) << the_scripts[i]->name;
    }
    script_popup->paned_window->manage();
    }
#endif
}
void UI_mainwindow::editGlobalsButtonActivateCallback(Widget, callback_stuff* client_data, void*) {
#ifdef GUI
    if(GUI_Flag) {
    // UI_mainwindow*	obj = (UI_mainwindow *) client_data->data;

    //
    // we need to launch the globals editor
    //
    Cstring				errors;

#ifdef GTK_EDITOR
    if(glS::fire_up_globwin(errors) != apgen::RETURN_STATUS::SUCCESS) {
	displayMessage("Global Editor panel", "Global Editor panel error", errors);
    }
#endif /* GTK_EDITOR */

    }
#endif
}

void UI_mainwindow::zoominButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	* obj = (UI_mainwindow *) client_data->data;

    obj->zoomSelectedDisplays(0.8);
    obj->setStatusBarMessage("Zoomed in on selected display(s)."); }

void UI_mainwindow::zoomoutButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	* obj = (UI_mainwindow *) client_data->data;

    obj->zoomSelectedDisplays(1.25);
    obj->setStatusBarMessage("Zoomed out on selected display(s)."); }

void UI_mainwindow::absolutePanZoomButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    CTime		startTime, duration;
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;


    // Find the first activity display that is selected (it is
    // guaranteed that there is at least one).

    /*
     * We really need ALL selected displays.
     */
    int adIndex = findFirstSelectedAD();
    //should have adIndex>=0 here,since can't AbsPanZoom without 1+ AD selected

    activityDisplay[adIndex]->getZoomPanInfo(startTime, duration);
    UI_abspanzoom::initialize(
	startTime.to_string(),
	duration.to_string());
    absPanZoomDialog->_absPanZoomDialog->manage(); }

void UI_mainwindow::time_system_ok(Widget, callback_stuff *, void * data) {
#ifdef GUI
    if(GUI_Flag) {
    strintslist		this_list_better_not_be_empty;
    strint_node*		N;
    Cstring			epoch_name;
 
    ((scrolled_list_widget *) time_system_selection_panel->scrolled_list)->get_selected_items(
	this_list_better_not_be_empty);
    if((N = this_list_better_not_be_empty.first_node())) {
	int				adIndex;
	activity_display		*AD;
	ACT_sys			*actsis; 

	for (adIndex = 0; adIndex < TOTAL_NUM_ACTSYS; ++adIndex) {
	AD = activityDisplay[adIndex];
	if (	AD->isVisible() &&
	    AD->isSelected()) {
	    int			flag = 0;

	    actsis = AD->activitySystem;
	    epoch_name = N->get_key();
	    if(N->get_key() == "UTC") { flag = 1; actsis->timezone.zone = TIMEZONE_UTC; }
	    else if( N->get_key() == "PST") { flag = 1; actsis->timezone.zone = TIMEZONE_PST;  }
	    else if( N->get_key() == "PDT") { flag = 1; actsis->timezone.zone = TIMEZONE_PDT;  }
	    else if( N->get_key() == "MST") { flag = 1; actsis->timezone.zone = TIMEZONE_MST;  }
	    else if( N->get_key() == "MDT") { flag = 1; actsis->timezone.zone = TIMEZONE_MDT;  }
	    else if( N->get_key() == "CST") { flag = 1; actsis->timezone.zone = TIMEZONE_CST;  }
	    else if( N->get_key() == "CDT") { flag = 1; actsis->timezone.zone = TIMEZONE_CDT;  }
	    else if( N->get_key() == "EST") { flag = 1; actsis->timezone.zone = TIMEZONE_EST;  }
	    else if( N->get_key() == "EDT") { flag = 1; actsis->timezone.zone = TIMEZONE_EDT;  }
	    if(flag) {
	    	actsis->timezone.epoch = "";
	    	*actsis->timezone.theOrigin = CTime(0, 0, false);
	    	actsis->timezone.scale = 1.;
	    	// AD->_adTimeSystemPM->set_sensitive(TRUE);
	    } else {
	    	epoch_name = N->get_key() / " = ";
	    	try {
	    		TypedValue& tdv = globalData::get_symbol(epoch_name);
	    		actsis->timezone.zone = TIMEZONE_EPOCH;
	    		actsis->timezone.epoch = epoch_name;
	    		if(globalData::isAnEpoch(epoch_name)) {
	    			*actsis->timezone.theOrigin = tdv.get_time_or_duration();
	    			actsis->timezone.scale = 1.0;
	    		} else if(globalData::isATimeSystem(epoch_name)) {
	    			ListOVal*	LV = &tdv.get_array();
	    			ArrayElement*	element = LV->find("origin");

	    			*actsis->timezone.theOrigin = element->Val().get_time_or_duration();
	    			element = LV->find("scale");
	    			actsis->timezone.scale = element->Val().get_double();
	    		}
	    		// AD->_adTimeSystemPM->set_sensitive(FALSE);
	    	} catch(eval_error Err) {
	    	}
	    }
	    AD->adjust_time_system_label(epoch_name);
	    actsis->get_siblings()->timemark->configure_graphics(NULL);
	    actsis->constraint_display->configure_graphics(NULL); } } }
    }
#endif
    }

void UI_mainwindow::update_all_time_system_labels() {
#ifdef GUI
    if(GUI_Flag) {
    int			i;

    for (i = 0; i < MAX_NUM_REG_ACTSYS; ++i)
	activityDisplay[i]->update_time_system_label();
    }
#endif
    }

void UI_mainwindow::timeSystemButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow		*obj = (UI_mainwindow *) client_data->data;
    ListOVal		globs;
    ArrayElement*		bs;
    stringtlist		epochs_to_display;
    Cstring			temp;
 
    epochs_to_display << new emptySymbol("UTC");
    epochs_to_display << new emptySymbol("PST");
    epochs_to_display << new emptySymbol("PDT");
    epochs_to_display << new emptySymbol("MST");
    epochs_to_display << new emptySymbol("MDT");
    epochs_to_display << new emptySymbol("CST");
    epochs_to_display << new emptySymbol("CDT");
    epochs_to_display << new emptySymbol("EST");
    epochs_to_display << new emptySymbol("EDT");
    globalData::copy_symbols(globs);
    for(int i = 0; i < globs.get_length(); i++) {
	bs = globs[i];
	if(globalData::isAnEpoch(bs->get_key())
	   || globalData::isATimeSystem(bs->get_key())) {
	    if(!(bs->get_key() & "Hopper:")) {
	    	try {
	    		TypedValue& tdv = globalData::get_symbol(bs->get_key());
	    		temp.undefine();
	    		temp << bs->get_key() << " = " << tdv.to_string();
	    		epochs_to_display << new emptySymbol(temp);
	    	} catch(eval_error Err) {
	    	}
	    }
	}
    }

    // Find the first activity display that is selected (it is
    // guaranteed that there is at least one).
    int adIndex = findFirstSelectedAD();

    // Hopper design: should prevent changing the time system of a hopper
    if(!time_system_selection_panel) {
	time_system_selection_panel = new popup_list(
	    "Time System Selection Panel",
	    mainWindowObject,
	    time_system_ok,
	    (void *) activityDisplay[adIndex],
	    1	// include apply button
	    );
	/* The popup_list::Create(int) method enables the scrolled
	 * list explicitly so there is nothing we need to do */
	}

    // do not forget this, otherwise the panel will be permanently attached to one ACT_sys...
    else
	time_system_selection_panel->Data = (void *) activityDisplay[adIndex];
    time_system_selection_panel->updateMainList(epochs_to_display);
    time_system_selection_panel->paned_window->manage();
    user_waiter::wait_for_user_action(time_system_selection_panel); }

// BKL-1-20-98 changed to action request
void UI_mainwindow::closeSADButtonActivateCallback(Widget, callback_stuff * /*client_data*/, void *) {
    CLOSE_ACT_DISPLAYrequest*	request;

	CMD_exec::cmd_subsystem().clear_lists();

	request = new CLOSE_ACT_DISPLAYrequest(CMD_exec::cmd_subsystem().get_strlist());
    request->process();
    delete request;
	CMD_exec::cmd_subsystem().clear_lists();
}

void UI_mainwindow::cutButtonActivateCallback(Widget, callback_stuff *, void *) {
    CUT_ACTIVITYrequest*		request;
    slist<alpha_void, dumb_actptr>	active_selections;

    ACT_exec::get_active_selections(active_selections);
    request = new CUT_ACTIVITYrequest(active_selections);
    request->process();
    delete request;

    UI_mainwindow::setStatusBarMessage("Cut activity instance(s).");
}

void dummy_proc(Widget, callback_stuff*, void*) {}

void UI_mainwindow::ungroupActivitiesCallback(Widget, callback_stuff *client_data, void *) {
    slist<alpha_void, dumb_actptr>	selection_list;
    UNGROUP_ACTIVITIESrequest*	request;

    ACT_exec::get_active_selections(selection_list);
    request = new UNGROUP_ACTIVITIESrequest(selection_list);
    request->process();
    delete request;
}

void UI_mainwindow::unfreezeButtonActivateCallback(Widget, callback_stuff* client_data, void*) {
    UNFREEZE_RESOURCESrequest*	request = new UNFREEZE_RESOURCESrequest;
    request->process();
    delete request;
}

void UI_mainwindow::copyButtonActivateCallback(Widget, callback_stuff * /*client_data*/, void *) {
    COPY_ACTIVITYrequest*		request;
    slist<alpha_void, dumb_actptr>	selection_copy;

    ACT_exec::get_active_selections(selection_copy);
    request = new COPY_ACTIVITYrequest(selection_copy);
    request->process();
    delete request;

    UI_mainwindow::setStatusBarMessage("Copied activity instance(s).");
    // FIXES PART OF FR 1.1-8:
    // PFM 2/3/98: this seems no longer necessary
    // UI_subsystem->notify(ActivityCreation); // 2_3_98 NOTIFY TO IGNORE
}

void UI_mainwindow::pasteButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;

    ACT_sys::enter_pastemode();
    // DONE BY THE PASTEACTIVITY action_request NOW:
    // UI_subsystem->addNewItem ();
    obj->setStatusBarMessage("Pasted an activity instance."); }

void UI_mainwindow::aboutApgenButtonActivateCallback(Widget, callback_stuff * client_data,  void *) {
    UI_mainwindow*	obj = (UI_mainwindow *) client_data->data;
    Cstring		titles = get_apgen_version_build_platform();

    // new udef scheme: need to extract this dynamically
    // titles << "\nlinked with " << User_lib_Title;
    // titles << "\nlinked with unknown udef library";
    titles << "\nlinked with " << udef_intfc::get_user_lib_version();

    displayMessage("About APGEN", titles,
	"Copyright (C) 1994-2022 California Institute of Technology.\n"
	"U. S. Government sponsorship is acknowledged.\n\n"
	"Engineers:\tDennis Page, Pierre Maldague\n\n"
	"Organization: Mission Services and Applications (MS&A)\n"
	"\tTelecom and Mission Ops Directorate (TMOD)\n"
	"\tJet Propulsion Laboratory (JPL)\n"
	"\tNational Aeronautics and Space Administration\n\t    (NASA)\n\n"
	"Thanks to:\tAdam Chase, Alvin Chong, David S. Glicksberg,\n"
	"\tBryan Lambert, Chris Lawler, Imin Lin,\n"
	"\tWalter Z. Pan, Thomas W. Starbird,\n"
	"\tand last but not least Rudy V. Valdez.\n"
	); }

void UI_mainwindow::openButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow* obj = (UI_mainwindow*) client_data->data;

    UI_openfsd::initialize();
    // openFSD->sel_box->manage();
    openFSD->manage(); }

void UI_mainwindow::savePlanFilesButtonActivateCallback(Widget, callback_stuff *, XtPointer) {
    UI_save::initialize(); }

void UI_mainwindow::integratePlanFilesButtonActivateCallback(Widget, callback_stuff *, XtPointer) {
    UI_integrate::initialize(); }

void UI_mainwindow::writeIntegratedPlanFileButtonActivateCallback(Widget, callback_stuff *, XtPointer) {
    UI_partial_save::initialize(); }

void UI_mainwindow::exportDataButtonActivateCallback(Widget, callback_stuff *, XtPointer) {
    EXPORT_DATArequest* request;

    request = new EXPORT_DATArequest(
	    EXPORT_DATArequest::ACTIVITY_INTERACTIONS,
	    EXPORT_DATArequest::JSON_FILE,
	    "act_interactions.json"
	    );
    request->process();
    delete request;
    setStatusBarMessage("activity interactions -> ActInteract.json");
}

extern "C" {

/* Keep in mind: the following is called from within the export thread.
 * We _must_ activate the password GUI panel, then put ourselves to sleep
 * while the user is entering information. */
char*	getPasswordFromGUI() {
    return UI_export::ui_export->passwd; }

} // extern "C"

// BKL-1-20-98 replaced inner code with action request
void UI_mainwindow::newADButtonActivateCallback(Widget, callback_stuff *, void *) {
    NEW_ACT_DISPLAYrequest* request;

    request = new NEW_ACT_DISPLAYrequest(CTime(0, 0, false), CTime(0, 0, true));
    request->process();
    delete request;
}


void UI_mainwindow::newRDButtonActivateCallback(Widget, callback_stuff *,  void * ) {
	NEW_RES_DISPLAYrequest *request;
    request = new NEW_RES_DISPLAYrequest();
    request->process();
    delete request;
}
// BKL-11/97 END OF WHAT I ADDED

void UI_mainwindow::clearActivityDisplays() {
#ifdef GUI
    if(GUI_Flag) {
    int i = - 1;
    DBG_INDENT("UI_mainwindow::clearActivityDisplays: START\n");
    while (++i < MAX_NUM_REG_ACTSYS) {
	deallocateResourceDisplays(i);
	if(activityDisplay[i]->isVisible()) {
	    activityDisplay[i]->clear(); } }
    DBG_UNINDENT("UI_mainwindow::clearActivityDisplays: START\n");
    }
#endif
}

/////////////////////////////////////////////
// Display Allocation/Deallocation Methods //
/////////////////////////////////////////////

// CHANGED (PFM): no need for strings; double values are available:
// void UI_mainwindow::allocateActivityDisplay (char *startTime,char *duration)
// last arg defaults to 0:
void UI_mainwindow::allocateActivityDisplay(CTime startTime, CTime duration, int freeze_start_time) {
    if(GUI_Flag) {
    int			i;
    static int		first_time_alloc = 1;

    DBG_INDENT("UI_mainwindow::allocateActivityDisplay: START\n");

    i = 0;
    while (i < MAX_NUM_REG_ACTSYS) {
	if (! activityDisplay[i]->isVisible()) {
	    if (startTime > CTime(0, 0, false) && duration > CTime(0, 0, true)) {
	    	activityDisplay[i]->activitySystem->get_siblings()->request_a_change_in_time_parameters(
	    							startTime,
	    							duration); }
	    activityDisplay[i]->manage();
	    if(freeze_start_time)
	    	activityDisplay[i]->activitySystem->never_scrolled = 0;
	    if(first_time_alloc && ! i) {
	    	first_time_alloc = 0;
	    	((motif_widget *) activityDisplay[0]->children[0])->manage(); }
	    *activityDisplay[i]->_adSelectedTB = "1"; // Does not invoke the callback
	    DBG_UNINDENT("UI_mainwindow::allocateActivityDisplay: END\n");
	    return; }
	++i; }
    DBG_UNINDENT("UI_mainwindow::allocateActivityDisplay: END\n");
    }
    }

int UI_mainwindow::allocateResourceDisplay(int adIndex) {
    if(GUI_Flag) {
    if (! activityDisplay[adIndex]->resourceDisplay->is_managed()) {
	activityDisplay[adIndex]->resourceDisplay->manage(); }
    }
    return 0; }	//Always succeeds.

void UI_mainwindow::deallocateActivityDisplays() {
    if(GUI_Flag) {
    int activeDisplayCount = 0;

    for (int i = 0; i < MAX_NUM_REG_ACTSYS; ++i) {
	if (activityDisplay[i]->isVisible()) {
	    if (activityDisplay[i]->isSelected()) {
	    	activityDisplay[i]->unmanage();
	    	activityDisplay[i]->uninitialize();
	    	deallocateResourceDisplays(i); }
	    else
	    	activeDisplayCount++; } } }
}

void UI_mainwindow::deallocateResourceDisplays(int adIndex) {
    if(GUI_Flag) {

    if (activityDisplay[adIndex]->resourceDisplay->is_managed()) {
	activityDisplay [adIndex]->resourceDisplay->unmanage();
	activityDisplay [adIndex]->resourceDisplay->uninitialize(); }
    }
    }

void UI_mainwindow::deallocateSelectedResourceDisplays(int adIndex) {
#ifdef GUI
    if(GUI_Flag) {
    int i;

    if (activityDisplay[adIndex]->resourceDisplay->is_managed()) {
	    activityDisplay[adIndex]->resourceDisplay->unmanage();
	    activityDisplay[adIndex]->resourceDisplay->uninitialize();
	} }
#endif
    }

// Internal Utility Methods //
//////////////////////////////

void UI_mainwindow::purge() {
    if(GUI_Flag) {
	    int			adIndex;

	DBG_INDENT("UI_mainwindow::purge START...\n");  // 2_3_98 NOTIFY TO IGNORE

	mainWindowObject->purgeResourceDisplays();
	SelectionFrozen = 0;
	set_type_of_new_act_to("generic");
	DBG_UNINDENT("UI_mainwindow::purge END\n"); } }

int UI_mainwindow::get_default_timesystem(Cstring &s) {
    if(GUI_Flag) {
	int	i = findFirstSelectedAD();

	if(i >= 0) {
	    return activityDisplay[i]->get_time_system_label(s); } }
    return 0; }

int UI_mainwindow::findFirstSelectedAD() {
    if(GUI_Flag) {
    static int		i;

    for(i = 0; i < TOTAL_NUM_ACTSYS; ++i) {
	if(	activityDisplay[i]->isVisible() &&
	    activityDisplay[i]->isSelected()) {
	    return i; } } }
    //get here only if no AD(s), or have AD(s) but none are selected
    return -1; }	//callers must check for this before using as an index!

int UI_mainwindow::findFirstVisibleAD() {
#ifdef GUI
    if(GUI_Flag) {
    static int		i;

    for (i = 0; i < TOTAL_NUM_ACTSYS; ++i) {
	if(	activityDisplay[i]->isVisible()) {
	    return i; } }
    }
#endif
    //get here only if no visible AD(s)
    return -1; }	//callers must check for this before using as an index!

void UI_mainwindow::findAllSelectedADs(List &L) {
    if(GUI_Flag) {
    static int		i;

    for (i = 0; i < TOTAL_NUM_ACTSYS; ++i) {
	if(	activityDisplay[i]->isVisible() &&
	    activityDisplay[i]->isSelected()) {
	    L << new String_node(Cstring("A") + Cstring(i + 1)); } }
    }
    }

    /* last argument defaults to 0 (see .H); when nonzero, it means the caller wants
     * the first Resource Display that is in manual mode.
     *
     * Design problem for scrolled resources: we need to specify the actual resource,
     * not just the first selected RD_sys. Solution: grab the first selected MW_object.
     * Note that a fair amount of reshuffling must take place here. For instance, we
     * should no longer have a selection button for RD_sys objects; we should select
     * individual resource plots through the toggle button in the resource display's
     * legend.  */
void UI_mainwindow::findFirstSelectedRD(int &adIndex, int &rdIndex, resourceDisplayStatus theStatus) {
    if(GUI_Flag) {
    for(int i = 0; i < MAX_NUM_REG_ACTSYS; ++i) {
	if(	activityDisplay[i]->isVisible()
	    && activityDisplay[i]->isSelected()) {
	    if(activityDisplay[i]->resourceDisplay->is_managed()) {
	    	RD_sys		*res_sis = activityDisplay[i]->resourceDisplay->resourceSystem;
	    	MW_object	*res_plot;
	    	List_iterator	the_res_plots(res_sis->mw_objects);
	    	int		pass = 0;

	    	/* We should be able to query the status of the MW_object directly.
	    	 * The available methods are
	    	 *
	    	 * 	RD_YSCROLL_MODE MW_object::getyscrollmode()
	    	 * 	int MW_object::getModelingResolution(const CTime&)
	    	 *
	    	 * Also, there should be a method for testing whether a MW_object is selected.
	    	 * The available data member is
	    	 * 
	    	 * 	int vertically_scrolled_item::selection
	    	 */
	    	while((res_plot = (MW_object *) the_res_plots())) {
	    		if(	(theStatus & IN_MANUAL_MODE)
	    			&& res_plot->get_selection()
	    			&& res_plot->getyscrollmode() == YSCROLL_MANUAL) {
	    			pass = 1;
	    			break; } }
	    	if(pass) {
	    		adIndex = i;
	    		rdIndex = 0;
	    		return; } } } }
    }
    //get here only if NO selected RD(s) among active (visible) AD(s)
    adIndex = -1;  //callers must check for (-1) before use these as indexes
    rdIndex = -1; }

int UI_mainwindow::findNthCreatedAD(int n) {
    static int		i;

    for (i = 0; i < MAX_NUM_REG_ACTSYS; ++i) {
	n--;
	if (n == 0) return (i); }
    //get here only if no AD has been created yet
    return -1; }

void UI_mainwindow::setStatusBarMessage(const Cstring & message) {
    *_statusBarLabel = message;
}

void UI_mainwindow::appendStatusBarMessage(const Cstring & message) {
    Cstring	old_text = _statusBarLabel->get_text();

    *_statusBarLabel = old_text + message;
}

void UI_mainwindow::zoomSelectedDisplays(double factor) {
    CTime			oldDuration, newDuration;
    CTime			oldStartTime, newStartTime;
    NEW_HORIZONrequest*	request;

    for (int i = 0; i < TOTAL_NUM_ACTSYS; ++i) {
	if (activityDisplay[i]->isVisible() && activityDisplay[i]->isSelected()) {
	    activityDisplay[i]->getZoomPanInfo(oldStartTime, oldDuration);
	    newDuration = oldDuration * factor;
	    newStartTime = oldStartTime + ((oldDuration - newDuration) / 2.0);
	    if (newStartTime < CTime(0, 0, false))
	    	newStartTime = CTime(0, 0, false);
	    // about 10 years:
	    if (newDuration > CTime(315360000, 0, true))
	    	newDuration = CTime(315360000, 0, true);
	    // min size set to 1 --> DELETE: tenth of a :ETELED <-- millisecond:
	    else if(newDuration < CTime(0, 1, true))
	    	newDuration = CTime(0, 1, true);
	    // DO THIS FIRST!
	    activityDisplay[i]->activitySystem->never_scrolled = 0;
	    request = new NEW_HORIZONrequest(Cstring("A") + Cstring(i + 1), newStartTime, newStartTime + newDuration);
	        request->process();
	    delete request;
	}
    }
}

//////////////////////////////
// External Utility Methods //
//////////////////////////////

void UI_mainwindow::changeCursor(int cursorIndex) {
    // Question Mark cursor added PFM 4/14/96
    static Display	*apgenDisplay;
    static Window	apgenWindow;
    static Cursor	cursor[3];
    static int	initialized = 0;

    if(!initialized) {
	apgenDisplay = XtDisplay(mainWindowObject->widget);
	// KEEP WINDOW
	apgenWindow = XtWindow(mainWindowObject->widget);
	cursor[0] = XCreateFontCursor(apgenDisplay, XC_watch);
	cursor[1] = XCreateFontCursor(apgenDisplay, XC_crosshair);
	cursor[2] = XCreateFontCursor(apgenDisplay, XC_question_arrow);
	initialized = 1; }

    if(cursorIndex >= 0 && cursorIndex < 3)
	XDefineCursor(apgenDisplay, apgenWindow, cursor[cursorIndex]);
    else
	XUndefineCursor(apgenDisplay, apgenWindow);
    XFlush(apgenDisplay); }


int UI_mainwindow::countManualSelectedRDs() {
    RD_YSCROLL_MODE	mode;
    int			count_of_selected_resource_displays_in_manual_mode = 0;

    for (int adIndex = 0;
	adIndex < MAX_NUM_REG_ACTSYS;
	++adIndex) {
	if (	activityDisplay[adIndex]->isVisible()
	    && activityDisplay[adIndex]->resourceDisplay->is_managed()) {
	    RD_sys		*res_sis = activityDisplay[adIndex]->resourceDisplay->resourceSystem;
	    MW_object	*res_plot;
	    List_iterator	the_res_plots(res_sis->mw_objects);

	    /* RD_YSCROLL_MODE MW_object::getyscrollmode()
	     * 	int MW_object::getModelingResolution(const CTime&)
	     * 	int vertically_scrolled_item::selection
	     */
	    while((res_plot = (MW_object *) the_res_plots())) {
	    	if(res_plot->getyscrollmode() == YSCROLL_MANUAL
	    	    && res_plot->get_selection()) {
	    		count_of_selected_resource_displays_in_manual_mode++; } } } }
    return count_of_selected_resource_displays_in_manual_mode; }


void UI_mainwindow::panSelectedDisplays(CTime startTime) {
    CTime		current_duration;

    for (int i = 0; i < TOTAL_NUM_ACTSYS; ++i) {
	if (	activityDisplay[i]->isVisible() &&
	    activityDisplay[i]->isSelected()) {
	    current_duration = activityDisplay[i]->activitySystem->get_siblings()->dwtimespan();
	    activityDisplay[i]->activitySystem->get_siblings()->request_a_change_in_time_parameters(
	    		startTime - current_duration / 2., current_duration);
	    activityDisplay[i]->activitySystem->never_scrolled = 0; } }
    }

void UI_mainwindow::zoomPanDisplay(int which, CTime startTime, CTime duration) {
    activityDisplay[which]->activitySystem->get_siblings()->request_a_change_in_time_parameters(startTime, duration);
    activityDisplay[which]->activitySystem->never_scrolled = 0; }

void UI_mainwindow::zoomPanHopper(CTime startTime, CTime duration) {
	theHopperPopup->AD->activitySystem->get_siblings()->request_a_change_in_time_parameters(startTime, duration); }


void UI_mainwindow::setApplicationSize(int width, int height) {
    Position		x, y;
    Widget			mainWindowShell = XtParent(mainWindowObject->widget);
    Screen			*screen = XtScreen(mainWindowShell);

    if (width == 0 || width > WidthOfScreen(screen) - 20) {
	width = WidthOfScreen(screen) - 20; }

    if (height == 0 || height > HeightOfScreen(screen) - 60) {
	height = HeightOfScreen(screen) - 60; }

    x = WidthOfScreen(screen) / 2 - width / 2;
    y = HeightOfScreen(screen) / 2 - height / 2;

    XtVaSetValues(mainWindowShell,
	XmNx,		x,
	XmNy,		y,
	XmNheight,	height,
	XmNwidth,	width,
	NULL); }

void UI_mainwindow::getApplicationSize(int &width, int &height) {
    if(GUI_Flag) {
	Widget			mainWindowShell = XtParent(mainWindowObject->widget);
	Position		w, h;

	XtVaGetValues(mainWindowShell,
	    XmNheight,	&h,
	    XmNwidth,	&w,
	    NULL);
	width = (int) w;
	height = (int) h; }
    else {
	width = 600;
	height = 480; } }


/////////////////////////
// C Wrapper Functions //
/////////////////////////
void UI_mainwindow::genTOLButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;

    if(! _tolPopupDialogShell) {
	_tolPopupDialogShell = new tol_popup("Generate TOL (Time-Ordered Listing)", MW, "UNINITIALIZED WIDGET!!!");
    }
    _tolPopupDialogShell->update_time_widgets();
    _tolPopupDialogShell->paned_window->manage();
}

void UI_mainwindow::genXMLTOLButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    Cstring		errors;

#ifdef GTK_EDITOR
    if(xS::fire_up_xmlwin(errors) != apgen::RETURN_STATUS::SUCCESS) {
	displayMessage("XMLTOL panel", "XMLTOL panel error", errors);
    }
#endif /* GTK_EDITOR */
}

void UI_mainwindow::genCsourceButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    }

void UI_mainwindow::apgenHelpButtonActivateCallback(Widget w, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;

    displayMessage ("APGEN Help", "On-Line Help",
	"Integrated on-line help is not yet supported.  However, the "
	"APGEN on-line User's Guide and related documents are published as "
	"World Wide Web (WWW) documents located at "
	"http://seq-www.jpl.nasa.gov/docs/apgen"); 
}

void	
theUniversalCallback(Widget w, callback_stuff *CBS, void *V) 
{

    // Signal to theWorkProc() that the user has done something "significant" and it'd better
    // figure out what really needs to be done to bring the core in sync with the GUI.
    udef_intfc::something_happened() += 1;
    try {
	CBS->function(w, CBS, V); }
    catch(eval_error E1) {
	UI_mainwindow::displayMessage("Unhandled Exception", "Low-level Error Warning",
	    		       Cstring("A low-level evaluation error occurred. This error should have been handled"
	    				" at a higher level, but was not; this is a deficiency of the Apgen software. Please"
	    				" send a copy of apgen.log together with any files you read to the Apgen Cog. E. for"
	    				" further analysis and corrective action. Proceed with caution: save while you are"
	    				" ahead and prepare to die. FYI, here is the low-level error message:\n")
	    		       + E1.msg); }
}


void UI_mainwindow::displayMessage(
	const Cstring &windowTitle,
	const Cstring &messageTitle,
	const Cstring &message) {
	errors::Post(windowTitle, messageTitle + Cstring("\n") + message); }

void UI_mainwindow::displayMessage(const Cstring &windowTitle, const Cstring &messageTitle, const List &L) {
    String_node	*N;
    int		tot_len = 0;
    char		*c, *d;

    for(	N = (String_node *) L.first_node();
	N;
	N = (String_node *) N->next_node())
	tot_len += N->get_key().length();
    c = (char *) malloc(tot_len + 1);
    d = c;
    for(	N = (String_node *) L.first_node();
	N;
	N = (String_node *) N->next_node()) {
	strcpy(d, (char *) *N->get_key());
	d += N->get_key().length(); }
    *d = '\0';
    errors::Post(windowTitle, messageTitle + Cstring("\n") + Cstring(c));
    // PURIFY caught this:
    free(c); }

void UI_mainwindow::newLegendButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;

	if(! _nlPopupDialogShell)
	    _nlPopupDialogShell = new UI_newLegend("New Legend", obj, UI_newLegend::nlOKCallback);

    _nlPopupDialogShell->initialize();

    if (! (_nlPopupDialogShell->_UI_newLegend->is_managed()))
	_nlPopupDialogShell->_UI_newLegend->manage(); }

void UI_mainwindow::deleteSelectedLegendsButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;
    int		adIndex = obj->findNthCreatedAD(1);

    activityDisplay[adIndex]->deleteSelectedLegends(); }

void UI_mainwindow::flattenSelectedLegendsButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;
    int		adIndex = obj->findNthCreatedAD(1);

    activityDisplay[adIndex]->flattenSelectedLegends(); }

void UI_mainwindow::squishSelectedLegendsButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;
    int		adIndex = obj->findNthCreatedAD(1);

    activityDisplay[adIndex]->squishSelectedLegends(); }

void UI_mainwindow::expandSelectedLegendsButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;
    int		adIndex = obj->findNthCreatedAD(1);

    activityDisplay[adIndex]->expandSelectedLegends(); }

void UI_mainwindow::unselectAllLegendsButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;
    int		adIndex = obj->findNthCreatedAD(1);

    activityDisplay[adIndex]->unselectAllLegends(); }

void UI_mainwindow::unsquishSelectedLegendsButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;
    int		adIndex = obj->findNthCreatedAD(1);

    activityDisplay[adIndex]->unsquishSelectedLegends(); }


void quit_anyway_CB(const Cstring & which_button) {
    if(which_button == "Quit anyway"){
	QUITrequest* request = new QUITrequest();
	request->process();
	delete request;
    } else if(which_button == "Do not quit now but do not show this warning again") {
	warned_about_exit = 1;
    }
}

void UI_mainwindow::seqrButtonActivateCallback(Widget, callback_stuff* client_data, void*) {
}

void UI_mainwindow::opsrevButtonActivateCallback(Widget, callback_stuff* client_data, void*) {
    if(theSimpleGetHandle) {
	char*	result;
	theSimpleGetHandle(*APcloptions::theCmdLineOptions().opsrev_url, &result);
	// cout << "Result from Ops Rev URL " << APcloptions::theCmdLineOptions().opsrev_url
	//	<< ":\n" << result << "\n";
	} }

void UI_mainwindow::fastquitButtonActivateCallback(Widget, callback_stuff*, void*) {
    QUITrequest* request = new QUITrequest( /* fast = */ true);
    request->process();
    delete request;
}

void UI_mainwindow::quitButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	*obj = (UI_mainwindow *) client_data->data;

    if((!warned_about_exit) && eval_intfc::get_act_lists().get_active_length()) {
	List		buttons;

	buttons << new String_node("Quit anyway");
	buttons << new String_node("Do not quit now");
	buttons << new String_node("Do not quit now but do not show this warning again");
	UI_messagewarndialog::initialize(
	    "Quit",
	    	"Possible Unsaved Files",
	    "There may be unsaved files.  If you wish to save "
	    "some or all files, use the \"Save Plan File(s)...\" "
	    "command.  This warning will occur every time you try to quit, unless "
	    "you select the third option below.",
	    buttons,
	    quit_anyway_CB
	    );
    } else {
	// PFM >> 
	// exit(0);
	QUITrequest* request = new QUITrequest();
	request->process();
	delete request;
	// << PFM
    }
}

void UI_mainwindow::editActivityButtonActivateCallback(Widget, callback_stuff *, void *) {
#ifdef GTK_EDITOR

    //
    // I think this comment is obsolete:
    //
    // When double-clicking on an activity, the callback is invoked with NULL arguments from
    // within DS_lndraw::Event(). Beware.
    //

    //
    // obsolete
    //
    // slist<alpha_void, dumb_actptr>	copy_of_sel_pointers;

    Cstring				errors;

    //
    // useless? I think so
    //
    // ACT_exec::get_active_selections(copy_of_sel_pointers);

    if(eS::fire_up_gtk_editor(errors) != apgen::RETURN_STATUS::SUCCESS) {
	displayMessage("New Editor", "Editor Error", errors);
    }
#endif /* GTK_EDITOR */
    }

void UI_mainwindow::activityDefinitionsButtonActivateCallback(Widget, callback_stuff *, void *) {
    if(DS_lndraw::actsys_in_which_last_click_occurred)
	DS_lndraw::actsys_in_which_last_click_occurred->send_to_back(DS_lndraw::mv_x, DS_lndraw::mv_y); }


void UI_mainwindow::closeSRDButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	* obj = (UI_mainwindow *) client_data->data;
    Cstring temp;
    int i, j;

	// BKL-12/97 ADDED THE FOLLOWING:
	CMD_exec::cmd_subsystem().clear_lists();

    // NO NEED TO CHECK IF ANY, BUTTONS WOULD NOT ALLOW US TO GET HERE !!!!

    // build the list of displays to de-allocate and send them to be processed
	for(i = 0, j = 0; i < MAX_NUM_REG_ACTSYS; ++i) {
	  if(UI_mainwindow::activityDisplay[i]->isVisible() && UI_mainwindow::activityDisplay[i]->isSelected()) {
	j++; // count visible ACT displays
	  // count visible RES displays
	      if (UI_mainwindow::activityDisplay[i]->resourceDisplay->is_managed()) {

	temp = "A";
	temp = temp + j + "R1";
	        CMD_exec::cmd_subsystem().add_string_node_to_list(*temp); } } }

	CLOSE_RES_DISPLAYrequest* request;
    request = new CLOSE_RES_DISPLAYrequest(CMD_exec::cmd_subsystem().get_strlist());
    request->process();
    delete request;
    CMD_exec::cmd_subsystem().clear_lists();
	// BKL-12/97 END OF WHAT I ADDED
}

void UI_mainwindow::removeResourceButtonActivateCallback(Widget, callback_stuff * /* client_data */, void *) {
    REMOVE_RESOURCErequest* request;

    request = new REMOVE_RESOURCErequest();
    request->process();
    delete request;
}

void UI_mainwindow::remodelButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow*	obj = (UI_mainwindow *) client_data->data;
    Cstring		errorMsg;
    REMODELrequest*	rr;

    if(	client_data->parent == mainWindowObject->_remodelButton
	|| client_data->parent == mainWindowObject->_remodelToolbarButton) {
	UI_mainwindow::changeCursor(0);
    }
    CTime_base	to_time;
    rr = new REMODELrequest(false, to_time, false);
    rr->process();
    delete rr;
    changeCursor(-1);
}

void UI_mainwindow::resOptionsButtonActivateCallback(
	Widget,
	callback_stuff* client_data,
	void*) {
    UI_mainwindow*		obj = (UI_mainwindow*) client_data->data;
    RD_YSCROLL_MODE		mode;
    int			resolution;

    apgen::RETURN_STATUS status = obj->getRDControlInfo(
	    mode,
	    resolution);

	if(! _rdOptionsPopupDialogShell)
	    _rdOptionsPopupDialogShell = new UI_rdOptions(
	        			"rdOptionsPopupDialogShell",
	    			obj,
	    			UI_rdOptions::rdoOKCallback);

    _rdOptionsPopupDialogShell->initialize(mode, resolution);

    if(_rdOptionsPopupDialogShell->_UI_rdBackgroundPts->is_managed())
	_rdOptionsPopupDialogShell->_UI_rdBackgroundPts->unmanage();
    _rdOptionsPopupDialogShell->_UI_rdBackgroundPts->manage();
}

void UI_mainwindow::setRDControlInfo(RD_YSCROLL_MODE mode, int resolution) {
#ifdef GUI
    if(GUI_Flag) {
    for (int adIndex = 0; adIndex < MAX_NUM_REG_ACTSYS; ++adIndex) {
	    if (activityDisplay[adIndex]->isVisible()) {
	    RD_sys		*res_sis = activityDisplay[adIndex]->resourceDisplay->resourceSystem;
	    MW_object	*res_plot;
	    List_iterator	the_res_plots(res_sis->mw_objects);

#	    		ifdef apDEBUG
	    	COUT << "UI_mainwindow::setRDControlInfo: searching for sel. RD in AD # " << adIndex << "...\n";
#	    		endif
	    // We need to convert the resolution info from an integer to a duration... no, setModelingResolution does it!
	    while((res_plot = (MW_object *) the_res_plots())) {
	    	if(res_plot->get_selection()) {
	    		res_plot->setyscrollmode(mode);
	    		res_plot->setModelingResolution(
	    				resolution,
	    				res_sis->get_siblings()->dwtimespan());
	    	}
	    }
	}
    }
    }
#endif
}

apgen::RETURN_STATUS UI_mainwindow::getRDControlInfo(RD_YSCROLL_MODE &mode, int &resolution) {
#ifdef GUI
    if(GUI_Flag) {
    int	adIndex;

    for (adIndex = 0; adIndex < MAX_NUM_REG_ACTSYS; ++adIndex) {
	    if (activityDisplay[adIndex]->isVisible()) {
	    RD_sys		*res_sis = activityDisplay[adIndex]->resourceDisplay->resourceSystem;
	    MW_object	*res_plot;
	    List_iterator	the_res_plots(res_sis->mw_objects);

#	    		ifdef apDEBUG
	    	COUT << "UI_mainwindow::setRDControlInfo: searching for sel. RD in AD # " << adIndex << "...\n";
#	    		endif
	    // We need to convert the resolution info from an integer to a duration... no, setModelingResolution does it!
	    while((res_plot = (MW_object *) the_res_plots())) {
	    	if(res_plot->get_selection()) {
	    		mode = res_plot->getyscrollmode();
	    		resolution = res_plot->getModelingResolution(res_sis->get_siblings()->dwtimespan());
	    		return apgen::RETURN_STATUS::SUCCESS;
	    	}
	    }
	}
    }
    } else {
	return apgen::RETURN_STATUS::FAIL;
    }
#endif
    return apgen::RETURN_STATUS::FAIL;
}

void purge_anyway(const Cstring& which_button) {

    if(which_button == "Purge Anyway") {
	PURGErequest *request = new PURGErequest();
	request->process();
	delete request;
    }
    if(which_button == "Purge Plan Only"){
	PURGErequest *request = new PURGErequest(true);
	request->process();
	delete request;
    }
}

void UI_mainwindow::purgeButtonActivateCallback(Widget, callback_stuff *,  void *) {
    List	purge_list;

    purge_list << new String_node("Purge Anyway");
    purge_list << new String_node("Do Not Purge At This Time");
    purge_list << new String_node("Purge Plan Only");
    UI_messagewarndialog::initialize(
	"Purge",
	"Purge Re-initializes APGEN",
	"Purge deletes all resource definitions, "
	    "constraint definitions, activity type "
	"definitions, activity instances, and "
	    "legends. Only the empty displays "
	"will remain after Purge.",
	purge_list,
	purge_anyway
       ); }

void UI_mainwindow::purgeResourceDisplays() {
#ifdef GUI
    if(GUI_Flag) {
    for (int adIndex = 0; adIndex < MAX_NUM_REG_ACTSYS; ++adIndex) {
	if (activityDisplay[adIndex]->isVisible()) {
	        if (activityDisplay[adIndex]->resourceDisplay->is_managed())
	    	activityDisplay [adIndex]-> resourceDisplay->purge(); } }
    }
#endif
    }

void UI_mainwindow::resAbsScrollZoomButtonActivateCallback(Widget, callback_stuff * client_data, void *) {
    UI_mainwindow	* obj = (UI_mainwindow *) client_data->data;
    int			adIndex;
    int			rdIndex;
    RD_YSCROLL_MODE		mode;
    int			resolution;
    Cstring			min("0.0"), span("0.0");
    apgen::RETURN_STATUS	status = apgen::RETURN_STATUS::FAIL;

    for(adIndex = 0; adIndex < MAX_NUM_REG_ACTSYS; ++adIndex) {
	if (	activityDisplay[adIndex]->isVisible()
	    && activityDisplay[adIndex]->resourceDisplay->is_managed()) {
	    	RD_sys		*res_sis = activityDisplay[adIndex]->resourceDisplay->resourceSystem;
	    	MW_object	*res_plot;
	    	List_iterator	the_res_plots(res_sis->mw_objects);

	    	// go to first selected resource display
	    	// We need to convert the resolution info from an integer to a duration... no, setModelingResolution does it!
	    	while((res_plot = (MW_object *) the_res_plots())) {
	    		if(res_plot->get_selection()) {
	    			// how to we get min and span information from a resource plot?
	    			// answer:
	    			status = res_plot->ygetminspan(min, span);
	    			if(status == apgen::RETURN_STATUS::SUCCESS)
	    				break; } } } }

    //Put up the dialog box
    UI_resabsscrollzoom::initialize(min, span);
    resAbsScrollZoomDialog->manage(); }

void UI_mainwindow::zoomScrollSelResourceDisplays(double min, double span) {
#ifdef GUI
    if(GUI_Flag) {
    RD_YSCROLL_MODE		mode;
    int			resolution;
    int			done = 0;

    //Loop thru all active (open) Activity Displays, looking for ...
    for(int adIndex = 0; adIndex < MAX_NUM_REG_ACTSYS; ++adIndex) {
	    if(	activityDisplay[adIndex]->isVisible()
	    && activityDisplay[adIndex]->isSelected()
	        && activityDisplay[adIndex]->resourceDisplay->is_managed()) {
	//... all active Resource Displays, that are selected AND are in
	//  manual (i.e. scrollable/zoomable) mode
	    RD_sys		*res_sis = activityDisplay[adIndex]->resourceDisplay->resourceSystem;
	    MW_object	*res_plot;
	    List_iterator	the_res_plots(res_sis->mw_objects);

	    // go to first selected resource display
	    // We need to convert the resolution info from an integer to a duration... no, setModelingResolution does it!
	    while((res_plot = (MW_object *) the_res_plots())) {
	    	if(res_plot->get_selection()) {
	    		mode  = res_plot->getyscrollmode();
	    		resolution = res_plot->getModelingResolution(res_sis->get_siblings()->dwtimespan());
	        		if (mode == YSCROLL_MANUAL) {
	                		res_plot->yabspanzoom(min, span);
	    			res_plot->setStaleFlag(true);
	    			udef_intfc::something_happened() += 1;
	    			done = 1;
	    			break; } } } }
    if(done) break; }
    }
#endif
    }

void UI_mainwindow::genSASFButtonActivateCallback(Widget, callback_stuff * /* client_data */,  void *) {
    Cstring	problems;
    int	integer_returned;

    if(!sasf_popup_instance()) {
	sasf_popup_instance() = new sasf_popup("Generate SASF", MW); }
    sasf_popup_instance()->update_time_widgets();
    integer_returned = sasf_popup_instance()->create_sasf_file_list(problems);
    if(problems.length()) {
	displayMessage("Generate SASF",
	    	"Problem found",
	    	problems); }
    else if(integer_returned) {
	sasf_popup_instance()->paned_window->manage(); }

    else {	/* 0 files in SASF list, so no instances will go to any SASF */
	displayMessage("Generate SASF",
	    	"Nothing to Write",
	    	"No activity instances(s) have the \"SASF\" attribute set, so none can be output."); } }

void UI_mainwindow::activityLockButtonActivateCallback(Widget, callback_stuff * client_data,  void *) {
    UI_mainwindow	* obj = (UI_mainwindow *) client_data->data;

    if(client_data->parent == obj->_activityUnlockSelection) {
	SelectionFrozen = 0; }
    else if(client_data->parent == obj->_activityLockSelection) {
	SelectionFrozen = 1; } }

void UI_mainwindow::preferencesButtonActivateCallback(Widget, callback_stuff *,  void *) {
    UI_preferences::initialize(); }

void UI_mainwindow::scheduleButtonActivateCallback(Widget, callback_stuff * client_data,  void *) {
#ifdef GUI
    Cstring				cause = client_data->parent->get_text();
    SCHEDULE_ACTrequest*		request1;
    UNSCHEDULE_ACTrequest*		request2;
    stringslist			empty_list;
    slist<alpha_void, dumb_actptr>	copy_of_sel_pointers;

    ACT_exec::get_active_selections(copy_of_sel_pointers);
    if(cause == "Schedule All") {
	// 2nd argument is for 'all'
	request1 = new SCHEDULE_ACTrequest(empty_list, 1, 0);
	request1->process();
	delete request1;
    } else if(cause == "Schedule All in Real Time") {
	// 2nd argument is for 'all'
	request1 = new SCHEDULE_ACTrequest(empty_list, 1, 1);
	request1->process();
	delete request1;
    } else if(cause == "Schedule One Step at a Time") {
	// 2nd argument is for 'all', 3rd arg is for 'real-time'
	request1 = new SCHEDULE_ACTrequest(empty_list, 1, 2);
	request1->process();
	delete request1;
    } else if(cause == "Schedule Selection")   {
	// 2nd argument is for 'not all'
	request1 = new SCHEDULE_ACTrequest(copy_of_sel_pointers, 0, 0);
	request1->process();
	delete request1;
    } else if(cause == "Unschedule Selection") {
	// 2nd argument is for 'not all'
	request2 = new UNSCHEDULE_ACTrequest(copy_of_sel_pointers, 0);
	request2->process();
	delete request2;
    } else if(cause == "Unschedule All") {
	// 2nd argument is for 'all'
	request2 = new UNSCHEDULE_ACTrequest(empty_list, 1);
	request2->process();
	delete request2;
    }
#endif
}
	
void UI_mainwindow::detailActivityButtonActivateCallback(Widget, callback_stuff *clientData, XtPointer) {
#ifdef GUI
    UI_mainwindow*			obj;
    static int			redetail = 0, use_selection = 0;
    slist<alpha_void, dumb_actptr>	copy_of_sel_pointers;
    Cstring				errors;
    static callback_stuff*		savedClientData;

    ACT_exec::get_active_selections(copy_of_sel_pointers);
    if(clientData) {
	savedClientData = clientData;
	obj = (UI_mainwindow *) clientData->data;
	/*
	 * First, we need to make sure that either there are no resolutions in the set to be detailed,
	 * or that there is only one.
	 */
	redetail = 0;
	use_selection = 0;
	if(clientData->parent == obj->_detailActivityButton) {
	    use_selection = 1;
	    redetail = 0;
	}
	if(clientData->parent == obj->_fullyDetailActivityButton) {
	    use_selection = 1;
	    redetail = 0;
	} else if(clientData->parent == obj->_detailAllButton) {
	    use_selection = 0;
	    redetail = 0;
	} else if(clientData->parent == obj->_redetailActivityButton) {
	    use_selection = 1;
	    redetail = 1;
	} else if(clientData->parent == obj->_redetailAllButton) {
	    use_selection = 0;
	    redetail = 1;
	} else if(clientData->parent == obj->_deleteAllDescButton) {
	    use_selection = 0;
	    redetail = 0;
	}
    } else {
	clientData = savedClientData;
    }
    obj = (UI_mainwindow *) clientData->data;

    if(	clientData->parent == obj->_detailActivityButton
	|| clientData->parent == obj->_fullyDetailActivityButton
	|| clientData->parent == obj->_redetailActivityButton) {
	if(!copy_of_sel_pointers.get_length()) {
	    return;
	}
    }

    if(clientData->parent == obj->_detailActivityButton) {
	DETAIL_ACTIVITYrequest	*request = new DETAIL_ACTIVITYrequest(
	    				copy_of_sel_pointers,
	    				/* NewDetailize */ 0,
	    				/* resolution */ 0);

	request->process();
	delete request;
    } else if(clientData->parent == obj->_fullyDetailActivityButton) {
	DETAIL_ACTIVITYrequest	*request = new DETAIL_ACTIVITYrequest(
	    				copy_of_sel_pointers,
	    				/* NewDetailize */ 0,
	    				/* resolution */ 0,
	    				/* secret */ 0,
	    				/* fully */ true);

	request->process();
	delete request;
    } else if(clientData->parent == obj->_detailAllButton) {
	DETAIL_ALLrequest	*request = new DETAIL_ALLrequest(
	    				/* NewDetailize */ 0);

	request->process();
	delete request;
    } else if(clientData->parent == obj->_detailAllRecursivelyButton) {
	DETAIL_ALL_QUIETrequest	*request = new DETAIL_ALL_QUIETrequest(
	    			/* NewDetailize */ 0);

	request->process();
	delete request;
    } else if(clientData->parent == obj->_redetailActivityButton) {
	if(APcloptions::theCmdLineOptions().RedetailOptionEnabled) {
	    DETAIL_ACTIVITYrequest	*request = new DETAIL_ACTIVITYrequest(
	    				copy_of_sel_pointers,
	    				/* NewDetailize */ 1,
	    				/* resolution */ 0);
	    request->process();
	    delete request;
	} else {
	    REGEN_CHILDRENrequest	*request = new REGEN_CHILDRENrequest(
	    				copy_of_sel_pointers,
	    				/* All */ 0);
	    request->process();
	    delete request;
	}
    } else if(clientData->parent == obj->_redetailAllButton) {
	if(APcloptions::theCmdLineOptions().RedetailOptionEnabled) {
	    DETAIL_ALLrequest	*request = new DETAIL_ALLrequest(
	    				/* NewDetailize */ 1);
	    request->process();
	    delete request;
	} else {
	    REGEN_CHILDRENrequest	*request = new REGEN_CHILDRENrequest(
	    				copy_of_sel_pointers,
	    				/* All */ 1);
	    request->process();
	    delete request;
	}
    } else if(clientData->parent == obj->_redetailAllRecursivelyButton) {
	if(APcloptions::theCmdLineOptions().RedetailOptionEnabled) {
	    DETAIL_ALL_QUIETrequest	*request = new DETAIL_ALL_QUIETrequest(
	    				/* NewDetailize */ 1);
	    request->process();
	    delete request;
	} else {
	    REGEN_CHILDRENrequest	*request = new REGEN_CHILDRENrequest(
	    				copy_of_sel_pointers,
	    				/* All */ 2);
	    request->process();
	    delete request;
	}
    } else if(clientData->parent == obj->_deleteAllDescButton) {
	DELETE_ALL_DESCENDANTSrequest	*request = new DELETE_ALL_DESCENDANTSrequest();

	request->process();
	delete request;
    }
#endif
}

void UI_mainwindow::abstractActivityButtonActivateCallback(Widget, 
	callback_stuff *clientData, XtPointer) {
#ifdef GUI
    UI_mainwindow	*obj = (UI_mainwindow *) clientData->data;

    if(clientData->parent == obj->_abstractActivityButton) {
	slist<alpha_void, dumb_actptr>	selection_list;
	ABSTRACT_ACTIVITYrequest*	request;

	ACT_exec::get_active_selections(selection_list);
	request = new ABSTRACT_ACTIVITYrequest(selection_list);

	request->process();
	delete request;
    } else if(clientData->parent == obj->_fullyAbstractActivityButton) {
	slist<alpha_void, dumb_actptr>	selection_list;
	ABSTRACT_ACTIVITYrequest*	request;

	ACT_exec::get_active_selections(selection_list);
	request = new ABSTRACT_ACTIVITYrequest(selection_list, true);

	request->process();
	delete request;
    } else if(clientData->parent == obj->_abstractAllButton) {
	ABSTRACT_ALLrequest*	request = new ABSTRACT_ALLrequest();

	request->process();
	delete request;
    } else if(clientData->parent == obj->_abstractAllRecursivelyButton) {
	ABSTRACT_ALL_QUIETrequest*	request = new ABSTRACT_ALL_QUIETrequest();

	request->process();
	delete request;
    }
#endif
}

void	UI_mainwindow::PURGEhandler(Action_request *ar, int w) {
    PURGErequest	*p = (PURGErequest *) ar;
    if(GUI_Flag) {
	if(!w) {
	    }
	else {
	    mainWindowObject->clearActivityDisplays();
	    }
    }
    }

void	UI_mainwindow::OPEN_FILEhandler(Action_request *ar, int w) {
    OPEN_FILErequest	*of = (OPEN_FILErequest *) ar;
    if(GUI_Flag) {
	if(!w) {
	    }
	else {
	    }
    }
    }

void	UI_mainwindow::ENABLE_SCHEDULINGhandler(Action_request *ar, int w) {
    if(GUI_Flag) {
	if(w) {
	    UI_mainwindow::mainWindowObject->_scheduleAllButton->set_sensitive(TRUE);
	    UI_mainwindow::mainWindowObject->_scheduleSelectionButton->set_sensitive(TRUE);
	    UI_mainwindow::mainWindowObject->_scheduleRealTimeButton->set_sensitive(TRUE);
	    UI_mainwindow::mainWindowObject->_scheduleOneStepAtATimeButton->set_sensitive(TRUE);
	    UI_mainwindow::mainWindowObject->_unscheduleButton->set_sensitive(TRUE);
	    }
	else {
	    UI_mainwindow::mainWindowObject->_scheduleAllButton->set_sensitive(FALSE);
	    UI_mainwindow::mainWindowObject->_scheduleSelectionButton->set_sensitive(FALSE);
	    UI_mainwindow::mainWindowObject->_scheduleRealTimeButton->set_sensitive(FALSE);
	    UI_mainwindow::mainWindowObject->_scheduleOneStepAtATimeButton->set_sensitive(FALSE);
	    UI_mainwindow::mainWindowObject->_unscheduleButton->set_sensitive(FALSE);
	    UI_mainwindow::mainWindowObject->_unscheduleAllButton->set_sensitive(FALSE);
	    }
	}
    }

void UI_mainwindow::RES_LAYOUThandler(
	    		const Cstring&		layout_id,
	    		const vector<Cstring>&	res_names,
	    		const vector<int>&	res_display_heights,
	    		const Cstring&		source_file,
	    		int			line_number) {
    RD_sys::reorder_legends_as_per(
	    layout_id,
	    res_names,
	    res_display_heights,
	    source_file,
	    line_number);
}

void UI_mainwindow::ACT_DISPLAY_HORIZONhandler(
	    		const CTime_base&	start,
	    		const CTime_base&	duration) {
    allocateActivityDisplay(start, duration, 1);
}

void UI_mainwindow::DISPLAY_SIZEhandler(
	    		const int&		x,
	    		const int&		y) {
    setApplicationSize(x, y);
}

void	UI_mainwindow::SAVE_FILEhandler(Action_request *ar, int w) {
    SAVE_FILErequest	*sf = (SAVE_FILErequest *) ar;
    if(GUI_Flag) {
	if(!w) {
	    }
	else {
	    IO_writer	writer;
	    int		ad_index = findFirstSelectedAD();
	    emptySymbol*	N;

	    N = sf->get_exclusions().first_node();
	    if(ad_index < 0) {
	    	sf->set_status(apgen::RETURN_STATUS::FAIL);
	    	sf->add_error("No display selected.\n");
	    	return; }
	    if(!N) {
	    	sf->set_status(apgen::RETURN_STATUS::FAIL);
	    	sf->add_error("SAVE FILE error: layout tag is missing.\n");
	    	return; }
	    ofstream fout(*sf->get_filename());
	    if(!fout) {
	    	sf->set_status(apgen::RETURN_STATUS::FAIL);
	    	sf->add_error(Cstring("Cannot open file ") + sf->get_filename() + " for output.\n"); }
	    else {
	    	fout << "APGEN version \"" << get_apgen_version_build_platform() << "\"\n\n\n";
	    	if(activityDisplay[ad_index]->activitySystem
	    	  && activityDisplay[ad_index]->resourceDisplay->resourceSystem) {
	    		activityDisplay[ad_index]->activitySystem->WriteLegendLayoutToStream(
	    								fout,
	    								N->get_key());
	    	  	activityDisplay[ad_index]->resourceDisplay->resourceSystem->WriteLegendLayoutToStream(
	    								fout,
	    								N->get_key()); } } } } }

void	UI_mainwindow::SCHEDULEhandler(Action_request *ar, int w) {
    SCHEDULE_ACTrequest	*of = (SCHEDULE_ACTrequest *) ar;
    if(GUI_Flag) {
	if(!w) {
	    ; }
	else {
	    // debug
	    // cout << "REMODELhandler: calling RD_sys::remodelingHasOccurred().\n";
	    RD_sys::remodelingHasOccurred(); } } }

void	UI_mainwindow::UNSCHEDULEhandler(Action_request *ar, int w) {
    UNSCHEDULE_ACTrequest	*of = (UNSCHEDULE_ACTrequest *) ar;
    if(GUI_Flag) {
	if(!w) {
	    ; }
	else {
	    // debug
	    // cout << "REMODELhandler: calling RD_sys::remodelingHasOccurred().\n";
	    RD_sys::remodelingHasOccurred(); } } }

void	UI_mainwindow::REMODELhandler(Action_request *ar, int w) {
    REMODELrequest	*of = (REMODELrequest *) ar;
    if(GUI_Flag) {
	if(!w) {
	    ; }
	else {
	    // debug
	    // cout << "REMODELhandler: calling RD_sys::remodelingHasOccurred().\n";
	    RD_sys::remodelingHasOccurred(); } } }

void	UI_mainwindow::COMMONhandler(Action_request *ar, int w) {
    if(GUI_Flag) {
	if(!w) {
	    changeCursor(0); }
	else {
	    changeCursor(-1); }
	XFlush(XtDisplay(UI_mainwindow::mainWindowObject->widget)); } }

void	UI_mainwindow::ABSTRACT_ALLhandler(Action_request *ar, int w) {
    Cstring msg;

    if(w == 0) {
	int c = eval_intfc::get_act_lists().get_active_length();

	msg = "Will look through ";
	msg << c << " activities and abstract them...";
	UI_mainwindow::mainWindowObject->setStatusBarMessage((char *) *msg);
	UI_mainwindow::mainWindowObject->setStatusBarMessage((char *) *msg);
	XFlush(XtDisplay(UI_mainwindow::mainWindowObject->widget)); }
    else if(w == 1) {
	UI_mainwindow::mainWindowObject->appendStatusBarMessage(" done."); } }

void	UI_mainwindow::DETAIL_ALLhandler(Action_request *ar, int w) {
    DETAIL_ALLrequest	*da = (DETAIL_ALLrequest *) ar;

    if(w == 0) {
	Cstring		msg("Will look through ");
	int		d = 0, c = eval_intfc::get_act_lists().get_active_length();

	msg << c;
	msg << " activities and decompose them...";
	UI_mainwindow::mainWindowObject->setStatusBarMessage((char *) *msg);
	XFlush(XtDisplay(UI_mainwindow::mainWindowObject->widget)); }
    else {
	UI_mainwindow::mainWindowObject->appendStatusBarMessage(" done."); } }

void pause_ok(Widget, callback_stuff *data, void *) {
    pause_popup		*obj = (pause_popup *) data->data;
    motif_widget		*orig = data->parent;

    obj->paned_window->unmanage(); }

void	UI_mainwindow::PAUSEhandler(Action_request *ar, int w) {
#ifdef PREMATURE
    if(GUI_Flag) {
	static pause_popup	*pause_panel = NULL;
	    	// NOTE: common_process_first throws the Action_request
	    	//	 into the History list, so that the next action
	    	//	 request is always the first (left) in the list...
	Action_request		*ar = next_command;
	Cstring			noquot(Text);

	removeQuotes(noquot);
	if(!pause_panel) {
	    pause_panel = new pause_popup(
	    	"Pause",
	    	UI_mainwindow::mainWindowObject,
	    	pause_ok
	    	);
	    pause_panel->create(); }
	*pause_panel->multi = noquot;
	if(ar) {
	    *( (motif_widget *) pause_panel->chosen_text ) = ar->get_command_text_with_modifier(); }
	else {
	    *( (motif_widget *) pause_panel->chosen_text ) = ""; }
	pause_panel->paned_window->manage();
	RequestQueue::disable_queue("PAUSE", 314159265);
	user_waiter::wait_for_user_action(pause_panel);
	return; }
#endif /* PREMATURE */
    }

void	UI_mainwindow::DELETE_ALL_DESCENDANTShandler(Action_request *ar, int w) {}

void	UI_mainwindow::CLOSE_ACT_DISPLAYhandler(Action_request *ar, int w) {
    if( GUI_Flag ) {
    CLOSE_ACT_DISPLAYrequest* This = (CLOSE_ACT_DISPLAYrequest *) ar;
    Cstring			temp , errmsg;
    emptySymbol		*string_node1=NULL;
    String_node*		aidx_node=NULL;
    int			i, j, act_idx, a_idx, disp_found;
    List			aidx_list;
    
    string_node1 = This->my_list.first_node();

    if (string_node1) {
	aidx_list.clear();
	while (string_node1) {
	    temp = string_node1->get_key();
	    removeQuotes( temp );
    
	    // verify format and limits on An
	    // 1st character must be the "A"
	    if (temp[0] != 'A') {
	    	 This->my_list.clear();
	    	aidx_list.clear();
	    	/**** THIS ERROR SHOULD ONLY OCCUR FROM THE SCRIPT, SO DISPLAY IT NOW ****/
	    	errmsg << "Error: Activity Display cannot be de-allocated" << "Ambiguous display name: "
	    		<< temp;
	    	This->add_error(errmsg);
	    	This->set_status(apgen::RETURN_STATUS::FAIL);
	    	return; }

	    for (i=1; temp[i]!='\0' && isdigit(temp[i]); i++) {}
	    if ((i==1) || (temp[i]!='\0')) {
	    	 This->my_list.clear();
	    	aidx_list.clear();
	    	/**** THIS ERROR SHOULD ONLY OCCUR FROM THE SCRIPT, SO DISPLAY IT NOW ****/
	    	errmsg << "Error: Activity Display cannot be de-allocated; " << "Ambiguous display name: "
	    		<< temp;
	    	This->add_error(errmsg);
	    	This->set_status(apgen::RETURN_STATUS::FAIL);
	    	return; }

	    // pull out the indexes
	    sscanf(*temp,"A%d",&act_idx);

	    // now check limit for activity display
	    if (act_idx > MAX_NUM_REG_ACTSYS) {
	    	 This->my_list.clear();
	    	aidx_list.clear();
	    	/**** THIS ERROR SHOULD ONLY OCCUR FROM THE SCRIPT, SO DISPLAY IT NOW ****/
	    	errmsg << "Error: Activity Display cannot be de-allocated" <<
	    		            "Activity display number is above limit: " <<
	    		            temp;
	    	This->add_error(errmsg);
	    	This->set_status(apgen::RETURN_STATUS::FAIL);
	    	return; }

	    // Logic is for example: if a display is the second visible, even if
	    // it's the third actual display, then it's == A2
	    // Then look for AD to be allocated and visible,
	    for (a_idx = 0, j=0, disp_found = 0; a_idx < MAX_NUM_REG_ACTSYS && !disp_found; a_idx++) {
	    	if (UI_mainwindow::activityDisplay[a_idx]->isVisible()) {
	    		j++;
	    		if (j == act_idx) {
	    			disp_found = 1;
	    			break; } } }
	
	    // error if not
	    if (!disp_found) {
	    	 This->my_list.clear();
	    	aidx_list.clear();
	    	/**** THIS ERROR SHOULD ONLY OCCUR FROM THE SCRIPT, SO DISPLAY IT NOW ****/
	    	errmsg << "Error: Activity Display cannot be de-allocated" <<
	    	            "Display is not visible: "<< temp;
	    	This->add_error(errmsg);
	    	This->set_status(apgen::RETURN_STATUS::FAIL);
	    	return; }

	    // --a_idx;
	    aidx_list << new String_node(a_idx); // save actual index

	    string_node1 = string_node1->next_node(); }
    
	// there is a list to deallocate, do it!
	aidx_node = (String_node *) aidx_list.first_node();
	string_node1 = This->my_list.first_node();
	while (aidx_node) {
	    temp = aidx_node->get_key();
	    removeQuotes( temp );
	    sscanf(*temp,"%d",&a_idx);
	    temp = string_node1->get_key(); // get him last, he is for error display only, if necc.
	    removeQuotes( temp );

	    if (!UI_mainwindow::activityDisplay[a_idx]->isVisible()) {
	    	// this error condition could only possibly happen after
	    	// at least one window has been successfully closed
	    	UI_mainwindow::setStatusBarMessage ("Closed resource display(s) up to point of error.");
    
	    	 This->my_list.clear();
	    	aidx_list.clear();

	    	/**** THIS ERROR SHOULD ONLY OCCUR FROM THE SCRIPT, SO DISPLAY IT NOW ****/
	    	errmsg << "Error: Activity display cannot be de-allocated; " <<
       		                    "Display is not visible: ",
	    		   Cstring(temp) + Cstring(" (May have been closed more than once in same command)");
	    	This->add_error(errmsg);
	    	This->set_status(apgen::RETURN_STATUS::FAIL);
	    	return; }

	    if (!UI_mainwindow::activityDisplay[a_idx]->isSelected()) {
	    	// ignore the request
	    	return; }

	    i = a_idx;

	    UI_mainwindow::activityDisplay[i]->unmanage ();
	    UI_mainwindow::activityDisplay[i]->uninitialize ();
	    UI_mainwindow::deallocateResourceDisplays (i);

	    aidx_node = (String_node *)aidx_node->next_node();
	    string_node1 = string_node1->next_node(); }

	UI_mainwindow::setStatusBarMessage ("Closed specified activity display(s) and "
	    "any associated resource display(s).");
	 This->my_list.clear();
	aidx_list.clear(); }
    else {
	/* List of displays to close is empty. UI_mainwindow interprets
	 * this to mean "close all selected ADs". */
	UI_mainwindow::deallocateActivityDisplays ();
	UI_mainwindow::setStatusBarMessage ("Closed selected activity display(s) and "
	    "any associated resource display(s)."); }

    return; } }

void	UI_mainwindow::CLOSE_RES_DISPLAYhandler(Action_request *ar, int w) {
    if( GUI_Flag ) {
    int	a_idx;

    for (a_idx = 0; a_idx < MAX_NUM_REG_ACTSYS; a_idx++) {
	if(	UI_mainwindow::activityDisplay[a_idx]->isVisible() &&
	    UI_mainwindow::activityDisplay[a_idx]->isSelected() &&
	    UI_mainwindow::activityDisplay[a_idx]->resourceDisplay->is_managed() ) {

	    UI_mainwindow::activityDisplay [a_idx]->resourceDisplay->unmanage ();
	    UI_mainwindow::activityDisplay [a_idx]->resourceDisplay->uninitialize (); } }
    UI_mainwindow::setStatusBarMessage ("Closed selected resource display(s).");

    return; } }

void	UI_mainwindow::NEW_ACT_DISPLAYhandler(Action_request *ar, int w) {
    if(GUI_Flag) {
	NEW_ACT_DISPLAYrequest	*This = (NEW_ACT_DISPLAYrequest *) ar;

	UI_mainwindow::allocateActivityDisplay(This->startTime, This->duration);
	UI_mainwindow::setStatusBarMessage ("Opened new activity display.");
	} }

void	UI_mainwindow::SELECT_RES_LEGENDhandler(Action_request *ar, int w) {
    SELECT_RES_LEGENDrequest *srl = (SELECT_RES_LEGENDrequest *) ar;

    if(GUI_Flag) {
	Cstring		theDisplayIndex = ("A" / srl->which_ad);
	MW_object	*mo;
	int		i;

	if(!isdigit((*theDisplayIndex)[0])) {
	    Cstring	theError("display ID ");

	    theError << srl->which_ad << " is not of the form An with n = 1, 2, 3.";
	    srl->add_error(theError);
	    srl->set_status(apgen::RETURN_STATUS::FAIL);
	    return; }
	i = atoi(*theDisplayIndex);
	if(i < 1 || i > 3) {
	    Cstring	theError("Invalid index in display ID ");

	    theError << srl->which_ad << ". Should be 1, 2 or 3.";
	    srl->add_error(theError);
	    srl->set_status(apgen::RETURN_STATUS::FAIL);
	    return; }
	mo = (MW_object *) UI_mainwindow::activityDisplay[i - 1]->resourceDisplay->resourceSystem->legend_display
	    	->get_blist_of_vertically_scrolled_items().find(srl->which_legend);
	if(!mo) {
	    Cstring				theError("Resource ");

	    theError << srl->which_legend << " cannot be selected in AD " << srl->which_ad
	    	<< " because it doesn't seem to exist.\n";
	    srl->add_error(theError);
	    srl->set_status(apgen::RETURN_STATUS::FAIL);
	    return; }
	mo->selection = srl->on_or_off; } }

void	UI_mainwindow::NEW_RES_DISPLAYhandler(Action_request *ar, int w) {
    if(GUI_Flag) {
	Cstring			errmsg;
	int			i, a_idx;

	DBG_INDENT( "NEW_RES_DISPLAYrequest::process_middle() START\n" );
	for (a_idx = 0; a_idx < MAX_NUM_REG_ACTSYS;  a_idx++) {
	    if(	UI_mainwindow::activityDisplay[a_idx]->isVisible() &&
	    	UI_mainwindow::activityDisplay[a_idx]->isSelected()) {
	    		break;
	    }
	}
    
	if(a_idx >= MAX_NUM_REG_ACTSYS) {
	    NEW_ACT_DISPLAYrequest *request;

	    a_idx = 0;
	    request = new NEW_ACT_DISPLAYrequest(CTime(0, 0, false), CTime(0, 0, true));
	    request->process();
	    delete request;
	}
	if(UI_mainwindow::activityDisplay[a_idx]->resourceDisplay->is_managed()) {
	    // ignore redundant request
	    return;
	}

	// allocate next resource display. This call always succeeds (i >= 0)
	i = UI_mainwindow::allocateResourceDisplay(a_idx);

	UI_mainwindow::setStatusBarMessage( "Opened new resource display." );
    }
}

void	UI_mainwindow::ADD_RESOURCEhandler(Action_request *ar, int w) {
    ADD_RESOURCErequest*	add = (ADD_RESOURCErequest *) ar;
    if(GUI_Flag) {
    Cstring			the_resource_name;
    Cstring			the_displ_num;
    Cstring			errmsg;
    Cstring			temp;
    emptySymbol*		string_node1;
    int			there_is_a_displ_num = 0;
    int			i, j, act_window, res_window;
    int			displ_found = 0;
    apgen::RETURN_STATUS	status;
    
    string_node1 = add->list.first_node();
    the_resource_name = string_node1->get_key();
    //removeQuotes( the_resource_name );	//DSG 98-02-12 need to keep quotes inside array-index brackets
    string_node1 = string_node1->next_node();
    if(!string_node1) {
	add->add_error( "Incorrect syntax in ADD_RESOURCErequest." );
	add->set_status(apgen::RETURN_STATUS::FAIL);
	return; }
    the_displ_num = string_node1->get_key();
    removeQuotes(the_displ_num);
    there_is_a_displ_num = 1;
    add->list.clear();
    // verify format and limits on AnRm
    // 1st character must be the "A"
    if(the_displ_num[0] != 'A') {
	/**** THIS ERROR SHOULD ONLY OCCUR FROM THE SCRIPT, SO DISPLAY IT NOW ****/
	errmsg << "Error: Resource display cannot be populated; " << "Ambiguous display specification: "
	    << the_displ_num;
	add->add_error(errmsg);
	add->set_status(apgen::RETURN_STATUS::FAIL);
	return; }

    for(i = 1; the_displ_num[i] != '\0' && isdigit(the_displ_num[i]); i++) {}
    if((i == 1 ) || (the_displ_num[i] != 'R')) {
	errmsg << "Error: Resource display cannot be populated; " << "Ambiguous display specification: "
	    << the_displ_num;
	add->add_error(errmsg);
	add->set_status(apgen::RETURN_STATUS::FAIL);
	return; }

    for(i = i + 1, j = i; the_displ_num[i] != '\0' && isdigit(the_displ_num[i]); i++) {}

    if(i == j || the_displ_num[i] != '\0') {
	errmsg << "Error: ambiguous display specification: " 
	       	<< the_displ_num;
	add->add_error(errmsg);
	add->set_status(apgen::RETURN_STATUS::FAIL);
	return; }

    // pull out the indexes
    sscanf(*the_displ_num, "A%dR%d", &act_window, &res_window);
    for(i = 0; i < MAX_NUM_REG_ACTSYS; i++) {
	if (UI_mainwindow::activityDisplay[i]->isVisible()) {
	    if( (i + 1) == act_window ) {
	    	if( UI_mainwindow::activityDisplay[i]->resourceDisplay->is_managed() ) {
	    		displ_found = 1;
	    		break; } } } }

    if(!displ_found) {
	errmsg << "No resource display available; " << "Cannot Display Resource " << temp;
	add->add_error(errmsg);
	add->set_status(apgen::RETURN_STATUS::FAIL);
	return; }

    temp = the_resource_name + " " + "(" + the_displ_num + ") ";

    add->set_status(UI_mainwindow::activityDisplay[i]->resourceDisplay->addResourceToDisplay(
	    the_resource_name,
	    errmsg));
    if(add->get_status() != apgen::RETURN_STATUS::SUCCESS) {
	errmsg << " (This error occurred while trying to Display Resource " << temp << ")";
	add->add_error(errmsg);
	return; } } }

void	UI_mainwindow::REMOVE_RESOURCEhandler(Action_request *ar, int w) {
    REMOVE_RESOURCErequest	*rr = (REMOVE_RESOURCErequest *) ar;
    if(GUI_Flag) {
    Cstring                 the_displ_num;
    Cstring                 temp;
    emptySymbol*		string_node1;
    int                     there_is_a_displ_num=0;
    int     		i , j , m, n, act_window, res_window;
    stringslist		reslist;
    
    for (i = 0; i < MAX_NUM_REG_ACTSYS; i++) {
	if (	UI_mainwindow::activityDisplay[i]->isVisible()
	    && UI_mainwindow::activityDisplay[i]->isSelected()
	    && UI_mainwindow::activityDisplay[i]->resourceDisplay->is_managed()) {

	    VSI_owner*	v_o = UI_mainwindow::activityDisplay[i]->resourceDisplay->resourceSystem->legend_display;
	    List_iterator	theResLegendObjects(v_o->get_blist_of_vertically_scrolled_items());
	    MW_object*	mo;
	    stringslist::iterator the_selected_res_plots(reslist);

	    while((mo = (MW_object *) theResLegendObjects())) {
	    	if(mo->get_selection()) {
	    		// we need to change this so we get the MW_objects instead
	    		reslist << new emptySymbol(mo->get_key()); } }
	    while((string_node1 = the_selected_res_plots())) {
	    	rr->set_status(
	    		UI_mainwindow::activityDisplay[i]->resourceDisplay
	    			->removeResourceFromDisplay(
	    				string_node1->get_key()));
	    	if(rr->get_status() != apgen::RETURN_STATUS::SUCCESS) {
	    		break; } }
	    reslist.clear(); }
	if(rr->get_status() != apgen::RETURN_STATUS::SUCCESS) {
	    break; } }
    if (rr->get_status() != apgen::RETURN_STATUS::SUCCESS) {
	Cstring		temp("Unable to Remove Resource ");

	temp << string_node1->get_key();
	temp << "from Display ";
	temp << (i + 1) << ".\n";
	rr->add_error(temp);
	return; } } }

void	UI_mainwindow::RESOURCE_SCROLLhandler(Action_request *ar, int w) {
    RESOURCE_SCROLLrequest	*rs = (RESOURCE_SCROLLrequest *) ar;
    double			dmin, dspan;

    try { is_a_valid_number(rs->theMinimum, dmin); }
    catch(eval_error Err) {
	Cstring		theMessage("Minimum value \"");

	theMessage << rs->theMinimum << "\" is improperly formatted; error:\n";
	theMessage << Err.msg << "Examples of "
	    	<< "legal minimum values are -7 and 4.3.";
	rs->add_error(theMessage);
	rs->set_status(apgen::RETURN_STATUS::FAIL);
	return; }

    // Check for improper span-value type (must be integer or floating)

    try { is_a_valid_number(rs->theSpan, dspan); }
    catch(eval_error Err) {
	Cstring theMessage("Maximum value \"");

	theMessage << rs->theSpan << "\" is improperly formatted; error:\n";
	theMessage << Err.msg << "Examples of "
	    	<< "legal maximum values are -7 and 4.3.";
	rs->add_error(theMessage);
	rs->set_status(apgen::RETURN_STATUS::FAIL);
	    return; }

    if(dspan <= 0.) {
	rs->add_error( "The span is less than or equal to zero. It must be positive." );
	rs->set_status(apgen::RETURN_STATUS::FAIL);
	return; }
    mainWindowObject->zoomScrollSelResourceDisplays(dmin, dspan); }

void	UI_mainwindow::FIND_RESOURCEhandler(Action_request *ar, int w) {}

void	UI_mainwindow::NEW_HORIZONhandler(Action_request *ar, int w) {
    NEW_HORIZONrequest	*nh = (NEW_HORIZONrequest *) ar;

    if(nh->whichDisplay[0] == 'A') {
	zoomPanDisplay(
	    (int) (nh->whichDisplay[1] - '1'),
	    nh->start_time, nh->end_time - nh->start_time); }
    else if(nh->whichDisplay[0] == 'G') {
    } else if(nh->whichDisplay[0] == 'H') {
	zoomPanHopper(nh->start_time, nh->end_time - nh->start_time); }
    // hopefully we never get here
    }

refresh_info::refresh_info() : actsis_count(0),
	mainwin_count(0)
	{}

Cstring remove_internal_quotes(Cstring& a) {
    const char* c = *a;
    char* d = strdup(c);
    char* e = d;
    while(*e) {
	if(*e == '"') {
	    *e = '_'; }
	else if(*e == '[') {
	    *e = '_'; }
	else if(*e == ']') {
	    *e = '_'; }
	else if(*e == '-') {
	    *e = '_'; }
	else if(*e == ' ') {
	    *e = '_'; }
	e++; }
    Cstring f(d);
    free(d);
    return f;
}
