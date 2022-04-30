#if HAVE_CONFIG_H
#include <config.h>
#endif
// #define apDEBUG

#include "apDEBUG.H"

#include "ActivityInstance.H"
#include "ACT_sys.H"
#include "ACT_exec.H"
#include "C_string.H"
#include "CON_sys.H"
// #include "IO_protocols.H"
#include "UI_ds_timeline.H"
#include "UI_ds_draw.H"
#include "UI_exec.H"
#include "UI_activitydisplay.H" // BKL-1-20-98
#include "UI_mainwindow.H"
#include "UI_popups.H"
#include "UTL_time.H"
#include "UI_resourcedisplay.H" // BKL-1-20-98
#include "UI_messagewarndialog.H"
#include "action_request.H"

using namespace std;

// GLOBALS:

UI_exec			*UI_subsystem;

			// List of bstringnodes:
// extern blist		ListOfPlanFileNames;
extern stringtlist&	FileNamesWithInstancesAndOrNonLayoutDirectives();

slist<alpha_void, dumb_actptr>& tentativeDescendants() {
		static slist<alpha_void, dumb_actptr>	q;
		return q; }

			// the "Add Resource" panel:
add_resource_popup	*add_resource_panel = NULL;

			// the "Define New Activity" panel:
popup_list		*def_new_activity = NULL;
new_act_popup		*def_new_parent = NULL;
const char		*contextForRequestingTheNewActivity = "";
UI_new_activities       *ui_new_activities = NULL;
           
int			SyncFlag = 0;

// EXTERNS:
			// in UI_ds_timeline.C:
extern void		set_type_of_new_act_to(const Cstring &);

			// in main.C:
extern motif_widget*	TopLevelWidget;
extern motif_widget*	MW;

			// in UI_mainwindow.C:
			// bad programming, but no time for better code:
ActivityInstance*		activity_request_that_needs_to_be_re_resolved = NULL;

	/*
	 * This method is called from 2 places:
	 *
	 * 	- as a callback for UI_mainwindowbx::_newActivityButton; in that
	 * 	  case client_data->data is the const string "by itself"
	 *
	 * 	- within the UI_mainwindow::groupActivitiesCallback() method; in that
	 * 	  case client_data->data is the const string "parent of a group"
	 */
void
UI_exec::AddNewActivities(Widget, callback_stuff *client_data, void *) {
  if(!ui_new_activities)
    ui_new_activities = new UI_new_activities(MW);
  
  ui_new_activities->Show(); }


void UI_exec::AddNewActivity(Widget, callback_stuff *client_data, void *) {
	stringtlist		activityTypeList;
	const char		*theHiddenMessage = (const char *) client_data->data;

	/*
	 * 'message' for UI_ds_timeline, which will catch the 'click' event that will
	 * position the new activity and take appropriate action:
	 */
	contextForRequestingTheNewActivity = theHiddenMessage;
	if(!strcmp(theHiddenMessage, "by itself")) {
		/* This is the simple case: we want to create a new activity by itself.
		 * First see if there are any non-generic types available; if not, we
		 * don't need to pop up the type selection box: */
		ACT_exec::ACT_subsystem().get_all_type_names(activityTypeList);
		if(!activityTypeList.get_length()) {
			UI_subsystem->addNewActivity("");
			return;
		}

		/* OK, we have types available; let's then pop  up the appropriate box.
		 * The function that will be invoked when the user clicks OK is UI_exec::AddNewActivityOK();
		 * see below. */

		if(!def_new_activity) {
			def_new_activity = new new_act_popup(
				"Define New Activity", MW, AddNewActivityOK, &UI_subsystem, 1);
			/* create() provides the personality of the popup panel based on the flag.
			 * flag = 1 for regular activity creation */
			def_new_activity->Create(1); }
		else {
			def_new_activity->updateAllLists(); }
		if(def_new_parent) {
			/* do this because of bad interference between regular 'def new act'
			 * and the request-type def-new-act; this way, there is no ambiguity... */
			def_new_parent->paned_window->unmanage(); }
		def_new_activity->paned_window->manage(); }
	else if(!strcmp(theHiddenMessage, "parent of a group")) {
		dumb_actptr*					selnode;
		slist<alpha_void, dumb_actptr>		selection_list;
		slist<alpha_void, dumb_actptr>::iterator	to_inspect(selection_list);

		ACT_exec::get_active_selections(selection_list);
		tentativeDescendants().clear();
		while((selnode = to_inspect())) {
			ActivityInstance* act = selnode->payload;
			tentativeDescendants() << new dumb_actptr(act, act); }

		if(!def_new_parent) {
			def_new_parent = new new_act_popup(
				"Define New Parent" , MW , AddNewActivityOK , &UI_subsystem , 0 );
			/* create() provides the personality of the popup panel based on the flag.
			 * flag = 2 for parent-type activity creation. create() should store its
			 * parameter in a permanent location, so that fill_in_subsystems() knows what
			 * to do. */
			def_new_parent->Create(2);
			/* will select the right radiobox button; only do this the first time around,
			 * so the user doesn't go nuts: */
			*def_new_parent->createNewRequest = 2; }
		/* We need to make the following call aware of what 'state' the radiobox is in, so
		 * that any lists that are updated are consistent with it. */
		def_new_parent->fill_in_subsystems();
		def_new_parent->updateAllLists();
		if( def_new_activity ) {
			/* do this because of bad interference between regular 'def new act' and
			 * the request-type def-new-act; this way, there is no ambiguity... */
			def_new_activity->paned_window->unmanage(); }
		def_new_parent->paned_window->manage(); } }

void UI_exec::AddNewActivityOK(Widget, callback_stuff *ClientData, void *) {
	new_act_popup		*thePopup = ( new_act_popup * ) ClientData->data;
				// little trick to find out whether we are dealing with 'define new parent' or 'define new activity'
	motif_widget		*theRadiobox = thePopup->get_the_radiobox();
	Cstring			theType = thePopup->chosen_text->get_text();
	GROUP_ACTIVITIESrequest	*request;

	if(theType & " (Chameleon)") {
		theType = theType / " (Chameleon)"; }
	if(theRadiobox) {
		// we are dealing with 'define new parent'

		if(thePopup->useExistingActivity->get_text() == "SET") {
			slist<alpha_void, dumb_actptr>	selection_list;
			dumb_actptr*				ptr;
			ActivityInstance*			theA;

			if(ACT_exec::number_of_active_selections() != 1) {
				/* this is 'impossible' (HA HA) */
				return; }
			ACT_exec::get_active_selections(selection_list);
			ptr = selection_list.last_node();
			theA = ptr->payload;
			/* Recall (heh heh) that tentativeDescendants is managed by the group activities
			 * callback in UI_mainwindow.C...
			 */
			request = new GROUP_ACTIVITIESrequest(tentativeDescendants(), theA->get_unique_id(), 0, 0);
			request->process();
			/* By the time the code ends up here, the user has ALREADY selected an activity
			 * to be the parent of the group. All we need to do is pick it up...
			 */
			return; }
		else if(thePopup->createNewRequest->get_text() == "SET") {
			/* We quietly collect the existing selected activities
			 * and put them under the umbrella of the requested request... */

			if( theType == "None" ) {
				request = new GROUP_ACTIVITIESrequest(tentativeDescendants(), "Request", 1, 0); }
			else {
				request = new GROUP_ACTIVITIESrequest(tentativeDescendants(), "Request", theType, 1, 0); }
			request->process();
			return; }
		else if(thePopup->createNewChameleon->get_text() == "SET") {
			/* We quietly collect the existing selected activities
			 * and put them under the umbrella of the requested request... */

			if(theType == "None") {
				request = new GROUP_ACTIVITIESrequest(tentativeDescendants(), "Parent", 0, 1); }
			else {
				request = new GROUP_ACTIVITIESrequest(tentativeDescendants(), "Parent", theType, 0, 1); }
			request->process();
			return; }
		/* The third case is the one in which the parent is a new activity. The DS_lndraw::input()
		 * method will consult the 'contextForRequestingTheNewActivity' global variable and will
		 * invoke the appropriate action request. We just fall out of the 'if' in this case. */
		}
	/* The purpose of this function is to allow the AddNewActivity() callback (just
	 * above this function) to call addNewActivity directly with a zero-length activity type name.
	 *
	 * Note that we should be REALLY careful about what we stick into the 'chosen text' field.
	 * In particular, there should always be an option that keeps the parent as generic as
	 * possible. */
	UI_subsystem->addNewActivity( theType ); }

void UI_exec::addNewActivity ( const Cstring &activityType) {
	ACT_sys		*act_sys;
	Pointer_node	*ptr;

	/* This is where we set the stage for the user to click on a location and
	 * indicate where the new activity is to start: */
	if((! (ptr = (Pointer_node *) ACT_sys::Registration().first_node())) ||
		! (act_sys = (ACT_sys *) ptr->get_ptr()))
		return;

	DBG_INDENT("UI_exec::addNewActivity( " << activityType << " )... START\n");
	set_type_of_new_act_to(activityType);
	act_sys->enter_defmode();
	/* Once we have reached this point, the cursor turns into a question mark, prompting
	 * the user to click somewhere so as to define the start time of the parent activity.
	 *
	 * The button press event caused by the user will be caught by DS_lndraw::input,
	 * how the action request(s) are generated for creating the new activity. Note that
	 * the define_new_activity_starting_at() method needs to find out whether we are
	 * defining a really new activity by itself or whether it needs to group activities
	 * under the umbrella of the new parent; the mechanism it uses to tell which case
	 * is appropriate is to consult the contextForRequestingTheNewActivity, which is
	 * set by the AddNewActivity callback early in the group activities process. */
	DBG_UNINDENT("UI_exec::addNewActivity: END\n"); }

void UI_exec::update_all_time_systems() {
	UI_mainwindow::update_all_time_system_labels(); }

void UI_exec::AddResource(Widget, callback_stuff *, void *) {

	if(!add_resource_panel) {
	        add_resource_panel = new add_resource_popup(
			"Add Resource", MW, AddResourceOK, &UI_subsystem);
	}

	// 2 means resources
	if(add_resource_panel->fill_in_subsystems() != apgen::RETURN_STATUS::SUCCESS) {
		UI_subsystem->displayMessage("Add Resource(s) To Selected Display(s)",
			"Resource Instances",
			"No concrete non-hidden resources have been defined.");
	} else {
		add_resource_panel->updateAllLists();
	}
	add_resource_panel->paned_window->manage();
}

void UI_exec::AddResourceOK(Widget, callback_stuff *, void *) {
        int 		adIndex;
	int		dsp_found = 0;
	Cstring		which_one(add_resource_panel->chosen_text->get_text());
	Cstring         display_num;

	if(which_one.length()) {
		int			i;
                ADD_RESOURCErequest*	request;
		stringslist		theList;

		//  get valid i,j (i.e. not -1), because Add Resource is
		//  not sensitive unless have 1+ RD selected!

		/*************************************************************************************/
		/* although you have the resource name, you'll want to pass a display number         */
		/* to be recorded on the log/script                                                  */
		/*************************************************************************************/

                /* tally visible displays as you search for first selected RD */
                for (i = 0, adIndex = 1, dsp_found = 0; i < MAX_NUM_REG_ACTSYS && !dsp_found; i++, adIndex++) {
	                if (	UI_mainwindow::activityDisplay[i]->isVisible() 
	                	&& UI_mainwindow::activityDisplay[i]->isSelected()) {
	                	if (UI_mainwindow::activityDisplay[i]->resourceDisplay->is_managed()) {
					dsp_found = 1;
					break; } } }

                if(!dsp_found) {
                	adIndex = -1; }
                else {
                	i = adIndex; }

                theList << new emptySymbol(which_one);

		if(i >= 0) { // if a display is available
			display_num = "A";
			display_num = display_num + i + "R1";
			theList << new emptySymbol(display_num);

                	request = new ADD_RESOURCErequest(theList);
			request->process(); } }
	else {
		UI_subsystem->displayMessage (
			"Add Resource",
                	"Selection Error",
                	"No resource was selected."); } }

static int secret_paste_enabler = 0;

void UI_exec::enable_scheduling() {
}

void UI_exec::disable_scheduling() {
}

apgen::RETURN_STATUS UI_exec::printButtonUpdateSelections () {
	return UI_mainwindow::mainWindowObject->printButtonUpdateSelections(); }

void UI_exec::OpenActivityDisplay(CTime startTime, CTime duration) {
	UI_mainwindow::mainWindowObject->allocateActivityDisplay(startTime, duration, 1); }

void UI_exec::changeCursor(int i) {
	UI_mainwindow::mainWindowObject->changeCursor(i); }

void    UI_exec::addNewItem() {
	if(!FileNamesWithInstancesAndOrNonLayoutDirectives().find("New"))
		FileNamesWithInstancesAndOrNonLayoutDirectives() << new emptySymbol("New");
	if(!eval_intfc::ListOfAllFileNames().find("New"))
		eval_intfc::ListOfAllFileNames() << new emptySymbol("New"); }

void	UI_exec::setRDControlInfo(RD_YSCROLL_MODE mode, int resolution) {
	UI_mainwindow::mainWindowObject->setRDControlInfo(mode, resolution); }

void UI_exec::displayMessage(const Cstring &windowTitle, const Cstring &messageTitle, const List &messages) {
	UI_mainwindow::mainWindowObject->displayMessage(windowTitle, messageTitle, messages); }

void UI_exec::displayMessage(const Cstring &windowTitle, const Cstring &messageTitle, const Cstring &message) {
	UI_mainwindow::mainWindowObject->displayMessage(windowTitle, messageTitle, message); }

void UI_exec::zoomScrollSelResourceDisplays(double min, double span) {
	UI_mainwindow::mainWindowObject->zoomScrollSelResourceDisplays(min, span); }

void	UI_exec::synchronize() {
	}

void	UI_exec::unsynchronize() {
	}
