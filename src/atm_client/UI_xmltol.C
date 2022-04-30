#if	HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef GUTTED

#ifdef have_xmltol

// useful includes for generating and filtering XML TOLs:
#include <libxml++/libxml++.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/catalog.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include <dirent.h>
#include <sys/types.h>

#include <action_request.H>
#include <UI_xmltol.H>

xmltol_popup*	xmltol_popup::_xmltolPopupDialogShell = NULL;

xmltol_popup::xmltol_popup(
		const Cstring&	name,                   // will be name of top widget
		motif_widget*	parent_widget,          // parent of this widget
		const Cstring&	text_for_popup          // text (multiline) to display
		)
	: motif_widget(	name,
			xmDialogShellWidgetClass,
			parent_widget,
			NULL,
			0,
			/* should manage = */ false),
		data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()),
		top_widget(NULL),
		AllActsVisible(false),
		S(CTime(0, 0, false)),
		E(CTime(0, 0, false)) {
	const char*	APGEN_XSLT_FILTERS = getenv("APGEN_XSLT_FILTERS");
	const char*	APGEN_TOL_SCHEMA_FILE = getenv("APGEN_TOL_SCHEMA_FILE");
	const char*	filter_errors = NULL;

	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, &data_for_window_closing);
	paned_window			= new motif_widget(
						"sr_paned",
						xmPanedWindowWidgetClass,
						this,
						NULL,
						0,
						false);

	// first pane
	motif_widget* form_for_top	= new motif_widget(
						"form_for_top",
						paned_window,
						form_position(2),
						true);
	top_widget			= new motif_widget(
						"Enter start and end times:",
						xmLabelWidgetClass,
						form_for_top);
	*form_for_top < *top_widget < *form_for_top < endform;
	*form_for_top ^ *top_widget ^ *form_for_top ^ endform;
#ifdef HAVE_MACOS
	form_for_top->fix_height(25, form_for_top->widget);
#else
	form_for_top->fix_height(form_for_top->widget);
#endif /* HAVE_MACOS */

	// second pane
	form_for_start_and_end_times	= new motif_widget(
						"form_for_start_and_end_times",
						paned_window,
						form_position(2),
						true);

	label				= new motif_widget(
						"Start time: ",
						xmLabelWidgetClass,
						form_for_start_and_end_times);


	start_time			= new single_line_text_widget(
						"start_time",
						form_for_start_and_end_times,
						EDITABLE,
						true);


	label2				= new motif_widget(
						"End time: ",
						xmLabelWidgetClass,
						form_for_start_and_end_times);

	end_time			= new single_line_text_widget(
						"end_time",
						form_for_start_and_end_times,
						EDITABLE,
						true);


	*form_for_start_and_end_times < *label < *start_time < *form_for_start_and_end_times < endform;
	*form_for_start_and_end_times < *label2 < *end_time < *form_for_start_and_end_times < endform;
	*form_for_start_and_end_times ^ 5 ^ *label ^ 15 ^ *label2 ^ endform;
	*form_for_start_and_end_times ^ *start_time ^ *end_time ^ *form_for_start_and_end_times ^ endform;

#ifdef HAVE_MACOS
	form_for_start_and_end_times->fix_height(74, form_for_start_and_end_times->widget);
#else
	form_for_start_and_end_times->fix_height(form_for_start_and_end_times->widget);
#endif /* HAVE_MACOS */

	// third pane
	filter_form			= new  motif_widget(
						"filter_form",
						paned_window,
						form_position(2),
						true);
	if(!APGEN_XSLT_FILTERS) {
		filter_label			= new motif_widget(
						"(APGEN_XSLT_FILTERS env. var. undefined)",
						xmLabelWidgetClass,
						filter_form); }
	else {
		filter_label			= new motif_widget(
						"Optional filters from APGEN_XSLT_FILTERS:",
						xmLabelWidgetClass,
						filter_form); }
	blist		some_props(compare_function(compare_bstringnodes, false));
	some_props << new bstringnode(SCROLLBAR_DISPLAY_POLICY_STATIC);
	// multiple selection disallowed:
	some_props << new bstringnode(SELECTION_POLICY_SINGLE_SELECT);
	filters				= new scrolled_list_widget(
						"filters",
						filter_form,
						5,
						some_props,
						true);

	/* Populate the filters - use opendir() and readdir(): */
	if(APGEN_XSLT_FILTERS) {
		DIR*		dir;
		struct dirent*	entry;

		if(!(dir = opendir(APGEN_XSLT_FILTERS))) {
			filter_errors = "No such directory"; }
		else {
			while((entry = readdir(dir))) {
				if(entry->d_type == DT_REG) {
					// regular file
					const char*	entry_name = entry->d_name;
					filter_list << new emptySymbol(entry_name);
					*filters << entry_name; } }
			closedir(dir); } }

	*filter_form ^ *filter_label ^ *filters ^ *filter_form ^ endform;
	*filter_form < *filter_label < *filter_form < endform;
	*filter_form < *filters < *filter_form < endform;

	// fourth pane
	timesystem_form			= new  motif_widget(
						"timesystem_form",
						paned_window,
						form_position(2),
						true);
	timesystem_label		= new motif_widget(
						"Time System (optional)",
						xmLabelWidgetClass,
						timesystem_form);

	// blist		some_props(compare_function(compare_bstringnodes, false));
	// some_props << new bstringnode(SCROLLBAR_DISPLAY_POLICY_STATIC);

	// multiple selection disallowed:
	// some_props << new bstringnode(SELECTION_POLICY_SINGLE_SELECT);

	timesystems			= new scrolled_list_widget(
						"timesystems",
						timesystem_form,
						5,
						some_props,
						true);

	/* Populate the time systems */
	stringslist	tsyst;
	stringslist::iterator	iter(tsyst);
	emptySymbol*	es;

	globalData::get_all_timesystems(tsyst);
	while((es = iter())) {
		timesystem_list << new emptySymbol(es->get_key());
		*timesystems << *es->get_key(); }

	*timesystem_form ^ *timesystem_label ^ *timesystems ^ *timesystem_form ^ endform;
	*timesystem_form < *timesystem_label < *timesystem_form < endform;
	*timesystem_form < *timesystems < *timesystem_form < endform;

	// fifth pane
	schema_form			= new motif_widget(
						"form_for_schema",
						paned_window,
						form_position(5),
						true);

	if(!getenv("APGEN_TOL_SCHEMA_FILE")) {
		schema_label		= new motif_widget(
						"(APGEN_TOL_SCHEMA_FILE env. var. undefined)",
						xmLabelWidgetClass,
						schema_form); }
	else {
		schema_label		= new motif_widget(
						"(from APGEN_TOL_SCHEMA_FILE env. var.)",
						xmLabelWidgetClass,
						schema_form); }
	schema_button			= new motif_widget(
						"Include ref. to schema:",
						xmToggleButtonWidgetClass,
						schema_form);
	schema_name			= new single_line_text_widget(
						"SchemaName",
						schema_form,
						EDITABLE,
						true);
	visibility_button		= new motif_widget(
						"All acts. visible",
						xmToggleButtonWidgetClass,
						schema_form);
	*schema_name = "xmltol.xsd";
	*schema_form ^ *schema_label ^ endform;
	*schema_form < *schema_label < *schema_form < endform;
	*schema_form < *schema_button < *schema_name < *visibility_button < *schema_form < endform;
	*schema_label ^ *schema_button ^ *schema_form ^ endform;
	*schema_label ^ *visibility_button ^ *schema_form ^ endform;
	*schema_label ^ *schema_name ^ *schema_form ^ endform;

	schema_button->add_callback(SchemaButtonCallback, XmNvalueChangedCallback, this);
	visibility_button->add_callback(VisibilityButtonCallback, XmNvalueChangedCallback, this);

	// sixth pane
	file_name_form			= new motif_widget(
						"file_name_form",
						paned_window,
						form_position(2),
						true);
	file_name_label			= new motif_widget(
						"File Name: ",
						xmLabelWidgetClass,
						file_name_form);
	fileNameWidget			= new single_line_text_widget(
						"FileNameWidget",
						file_name_form,
						EDITABLE,
						true);

	*file_name_form < *file_name_label < *fileNameWidget < *file_name_form < endform;
	*file_name_form ^ *file_name_label ^ *file_name_form ^ endform;
	*fileNameWidget = "apgen-tol.xml";

	// seventh pane
	form_for_buttons		= new motif_widget(
						"form_for_buttons",
						paned_window,
						form_position(5),
						true);
	motif_widget* button		= new motif_widget(
						"OK",
						xmPushButtonWidgetClass,
						form_for_buttons);

	*form_for_buttons ^ *button ^ *form_for_buttons ^ endform;
	form_position(1) < *button < form_position(2) < endform;
	button->add_callback(OKButtonCallback, XmNactivateCallback, this);

	button				= new motif_widget(
						"Cancel",
						xmPushButtonWidgetClass,
						form_for_buttons);
	*form_for_buttons ^ *button ^ *form_for_buttons ^ endform;
	form_position(3) < *button < form_position(4) < endform;
	button->add_callback(CancelButtonCallback, XmNactivateCallback, this);

#ifdef HAVE_MACOS
	file_name_form->fix_height(32, fileNameWidget->widget);
	form_for_buttons->fix_height(32, button->widget);
#else
	file_name_form->fix_height(fileNameWidget->widget);
	form_for_buttons->fix_height(button->widget);
#endif /* HAVE_MACOS */
	update_time_widgets(); }

xmltol_popup::~xmltol_popup() {}

void xmltol_popup::update_time_widgets() {
	DBG_NOINDENT("xmltol_popup: OLD start " << S.to_string() << ", OLD end " << E.to_string() << endl);
	update_times(S, E);
	DBG_NOINDENT("xmltol_popup: NEW start " << S.to_string() << ", NEW end " << E.to_string() << endl);
	*start_time = S.to_string();
	*end_time = E.to_string(); }

void xmltol_popup::update_times(CTime& st, CTime& en) {
	model_intfc::FirstEvent(st);
	model_intfc::LastEvent(en); }

void xmltol_popup::CancelButtonCallback(Widget, callback_stuff* client_data, void*) {
	xmltol_popup*	obj = (xmltol_popup*) client_data->data;

	obj->paned_window->unmanage(); }

void xmltol_popup::OKButtonCallback(Widget, callback_stuff* client_data, void*) {
	xmltol_popup*	obj = (xmltol_popup*) client_data->data;

	obj->OKcallback(); }

void xmltol_popup::SchemaButtonCallback(Widget, callback_stuff* client_data, void*) {
	xmltol_popup*	obj = (xmltol_popup*) client_data->data;

	obj->schema_callback(); }

void xmltol_popup::VisibilityButtonCallback(Widget, callback_stuff* client_data, void*) {
	xmltol_popup*	obj = (xmltol_popup*) client_data->data;

	obj->visibility_callback();
}

void xmltol_popup::schema_callback() {
	if(schema_button->get_text() == "SET") {
		;
	} else {
		;
	}
}

void xmltol_popup::visibility_callback() {
	if(visibility_button->get_text() == "SET") {
		AllActsVisible = true; }
	else {
		AllActsVisible = false;
	}
}

void xmltol_popup::OKcallback() {
	WRITE_XMLTOLrequest*	theRequest;
	Cstring			new_start = start_time->get_text();
	Cstring			new_end = end_time->get_text();
	Cstring			error_msg;
	Cstring			theOptionalFilter;
	List			theStrips;
	strintslist		sel;
	strint_node*		N;
	const char*		APGEN_XSLT_FILTERS = getenv("APGEN_XSLT_FILTERS");
	Cstring			theOptionalSchema;
	Cstring			theOptionalTimesystem;

	filters->get_selected_items(sel);

	// should only be zero or one selection
	if((N = sel.first_node())) {
		Cstring selection = filter_list[N->payload - 1]->get_key();
		theOptionalFilter = APGEN_XSLT_FILTERS;
		int l = strlen(APGEN_XSLT_FILTERS);
		if(APGEN_XSLT_FILTERS[l - 1] != '/') {
			theOptionalFilter << "/";
		}
		theOptionalFilter << selection;
	}
	sel.clear();
	timesystems->get_selected_items(sel);
	if((N = sel.first_node())) {
		theOptionalTimesystem = N->get_key();
	}
	if(schema_button->get_text() == "SET") {
		theOptionalSchema = schema_name->get_text();
	}
	theRequest = new WRITE_XMLTOLrequest(
		fileNameWidget->get_text(),
		new_start,
		new_end,
		theOptionalFilter,
		theOptionalSchema,
		theOptionalTimesystem,
		AllActsVisible);
	theRequest->process();
	delete theRequest;
	paned_window->unmanage();
}



#endif /* have_xmltol */
#endif /* GUTTED */
