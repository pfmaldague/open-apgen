#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <AP_exp_eval.H>
#include <ActivityInstance.H>
#include <fileReader.H>
// #include <grammar_intfc.H>
#include <Prefs.H>
#include <UI_save.H>
#include <UI_mainwindow.H>

Cstring&		last_native_file() {
	static Cstring c;
	return c;
}

// start capturing here
			// the "Save File(s)" panel:
UI_save*		UI_save::ui_save			= NULL;
			// the "Export Data" panel:
UI_export*		UI_export::ui_export			= NULL;
			// the "Enter Password" panel:
pwd_popup*		pwd_popup::passwordDisplay		= NULL;

bool			UI_export::there_is_work_to_do		= false;

void			SaveDirectivesCallback(	Widget,
						callback_stuff* client_data,
						void*);
extern motif_widget*	MW;

UI_save::UI_save(
		const Cstring & name,
		motif_widget* parent_widget)
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget, NULL, 0, FALSE),
		directive_options_popup(NULL),
		data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()) {
	motif_widget*	form1;
	motif_widget*	form2;
	motif_widget*	form3;
	motif_widget*	form4;
	motif_widget*	button;

	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, &data_for_window_closing);

	paned_window	= new motif_widget("paned", xmPanedWindowWidgetClass, this, NULL, 0, FALSE);
	form1		= new motif_widget("form1", paned_window, form_position(60), true);
	frame1		= new motif_widget("frame1", xmFrameWidgetClass, form1, NULL, 0, FALSE);
	radiobox	= new motif_widget("RadioBox1", xmRadioBoxWidgetClass, frame1);

	save_in_separate_files	= new motif_widget("Save in separate files", xmToggleButtonWidgetClass, radiobox);
	save_in_one_file	= new motif_widget("Save in a single file", xmToggleButtonWidgetClass, radiobox);
	save_apf_only		= new motif_widget("Save Plan only (APF format)", xmToggleButtonWidgetClass, radiobox);
	// save_plan_xml_only	= new motif_widget("Save Plan only (XML format)", xmToggleButtonWidgetClass, radiobox);
	save_legend_layout	= new motif_widget("Save Legend layout", xmToggleButtonWidgetClass, radiobox);

	save_in_separate_files->add_callback(HowManyFilesCallback, XmNvalueChangedCallback, this);
	save_in_one_file->add_callback(HowManyFilesCallback, XmNvalueChangedCallback, this);
	save_apf_only->add_callback(HowManyFilesCallback, XmNvalueChangedCallback, this);
	// save_plan_xml_only->add_callback(HowManyFilesCallback, XmNvalueChangedCallback, this);
	save_legend_layout->add_callback(HowManyFilesCallback, XmNvalueChangedCallback, this);

	*save_apf_only = 1;	// sets this button as the default

	frame2 = new motif_widget("Frame2", xmFrameWidgetClass, form1, NULL, 0, FALSE);
	radiobox2 = new motif_widget("RadioBox2", xmRadioBoxWidgetClass, frame2);

	frame3 = new motif_widget("Frame3", xmFrameWidgetClass, form1, NULL, 0, FALSE);
	form4 = new motif_widget("Form4", xmFormWidgetClass, frame3);
	tag_label = new motif_widget("Layout Tag:", xmLabelWidgetClass, form4);
	tag_text = new single_line_text_widget("APF name", form4, EDITABLE);
	*form4 ^ *tag_label ^ *form4 ^ endform;
	*form4 ^ *tag_text ^ *form4 ^ endform;
	*form4 < *tag_label < *tag_text < *form4 < endform;
	tag_label->set_sensitive(FALSE);
	tag_text->set_sensitive(FALSE);

	save_everything = new motif_widget("Save all activities", xmToggleButtonWidgetClass, radiobox2);
	do_not_save_everything = new motif_widget("Exclude some activities", xmToggleButtonWidgetClass, radiobox2);
	save_everything->add_callback(SaveEverything, XmNvalueChangedCallback, this);
	do_not_save_everything->add_callback(SaveEverything, XmNvalueChangedCallback, this);
	*save_everything = 1;	// sets this button as the default

	*form1 < *frame1 < form_position(30) < *frame2 < *form1 < endform;
	*form1 < *frame1 < form_position(30) < *frame3 < *form1 < endform;
	*form1 ^ *frame1 ^ *form1 ^ endform;
	*form1 ^ *frame2 ^ form_position(40) ^ *frame3 ^ *form1 ^ endform;
#ifdef HAVE_MACOS
	form1->fix_height(155, frame1->widget);
#else
	form1->fix_height(frame1->widget);
#endif /* HAVE_MACOS */

	form1 = new motif_widget("form1B", paned_window, form_position(6), true);

	apf_label = new motif_widget("APF File:", xmLabelWidgetClass, form1);
	apf_text = new single_line_text_widget("APF name", form1, EDITABLE);
	directiveOptions_apf = new motif_widget("APF Options", xmPushButtonWidgetClass, form1);
	directiveOptions_apf->add_callback(SaveDirectivesCallback, XmNactivateCallback, this);

	*form1 < *apf_label < form_position(1) < *apf_text < form_position(4) < *directiveOptions_apf < *form1 < endform;
	*form1 ^ *apf_text ^ endform;
	*form1 ^ *apf_text ^ endform;
	*form1 ^ *directiveOptions_apf ^ endform;
	*form1 ^ 4 ^ *apf_label ^ endform;
	*apf_text = "apgen.apf";

	aaf_label = new motif_widget("AAF File:", xmLabelWidgetClass, form1);
	aaf_text = new single_line_text_widget("AAF name", form1, EDITABLE);
	directiveOptions_aaf = new motif_widget("AAF Options", xmPushButtonWidgetClass, form1);
	directiveOptions_aaf->add_callback(SaveDirectivesCallback, XmNactivateCallback, this);
	*form1 < *aaf_label < form_position(1) < *aaf_text < form_position(4) < *directiveOptions_aaf < *form1 < endform;
	*apf_text ^ 2 ^ *aaf_text ^ endform;
	*apf_text ^ 4 ^ *aaf_label ^ endform;
	*apf_text ^ 2 ^ *directiveOptions_aaf ^ endform;
	*aaf_text = "apgen.aaf";
	aaf_text->set_sensitive(FALSE);
	aaf_label->set_sensitive(FALSE);
	directiveOptions_aaf->set_sensitive(FALSE);

	mix_label = new motif_widget("Mixed File:", xmLabelWidgetClass, form1);
	mix_text = new single_line_text_widget("Mixed name", form1, EDITABLE);
	directiveOptions_mix = new motif_widget("Mix Options", xmPushButtonWidgetClass, form1);
	directiveOptions_mix->add_callback(SaveDirectivesCallback, XmNactivateCallback, this);
	*form1 < *mix_label < form_position(1) < *mix_text < form_position(4) < *directiveOptions_mix < *form1 < endform;
	*aaf_text ^ 2 ^ *mix_text ^ *form1 ^ endform;
	*aaf_text ^ 2 ^ *directiveOptions_mix ^ endform;
	*aaf_text ^ 4 ^ *mix_label ^ endform;
	mix_text->set_sensitive(FALSE);
	mix_label->set_sensitive(FALSE);
	directiveOptions_mix->set_sensitive(FALSE);
	*mix_text = "apgen.mix";

	form3 = new  motif_widget("form3", paned_window, form_position(2), true);
	exclude_label = new motif_widget("Directives, Epochs, TimeSystems, Globals and Activities\nfrom selected source(s) will NOT be saved:", xmLabelWidgetClass, form3);
	exclude_label->set_sensitive(FALSE);
	files_to_exclude = new scrolled_list_widget("file_list", form3, 5, true);
	files_to_exclude->add_property(SELECTION_POLICY_MULTIPLE_SELECT);
	files_to_exclude->set_sensitive(FALSE);
	*form3 ^ *exclude_label ^ *files_to_exclude ^ *form3 ^ endform;
	*form3 < *exclude_label < *form3 < endform;
	*form3 < *files_to_exclude < *form3 < endform;

	form2 = new  motif_widget("form2", paned_window, form_position(5), true);
	OKbutton = new motif_widget("Save", xmPushButtonWidgetClass, form2);
	*form2 ^ *OKbutton ^ *form2 ^ endform;
	form_position(1) < *OKbutton < form_position(2) < endform;
	OKbutton->add_callback(OKButtonCallback, XmNactivateCallback, this);
	button = new motif_widget("Cancel", xmPushButtonWidgetClass, form2);
	*form2 ^ *button ^ *form2 ^ endform;
	form_position(3) < *button < form_position(4) < endform;
	button->add_callback(CancelButtonCallback, XmNactivateCallback, this);
#ifdef HAVE_MACOS
	form2->fix_height(32, OKbutton->widget);
#else
	form2->fix_height(OKbutton->widget);
#endif /* HAVE_MACOS */

	//Get/Set Preference will only set the preference if it is not already set
	Preferences().GetSetPreference("APFGlobals", "None");
	Preferences().GetSetPreference("APFEpochs", "Comments");
	Preferences().GetSetPreference("APFTimeSystems", "Comments");
	Preferences().GetSetPreference("APFLegends", "Code");
	Preferences().GetSetPreference("APFWindowSize", "None");
	Preferences().GetSetPreference("APFTimeParameters", "None");

	Preferences().GetSetPreference("AAFGlobals", "Code");
	Preferences().GetSetPreference("AAFEpochs", "Code");
	Preferences().GetSetPreference("AAFTimeSystems", "Code");
	Preferences().GetSetPreference("AAFLegends", "None");
	Preferences().GetSetPreference("AAFWindowSize", "None");
	Preferences().GetSetPreference("AAFTimeParameters", "None");

	Preferences().GetSetPreference("MIXGlobals", "Code");
	Preferences().GetSetPreference("MIXEpochs", "Code");
	Preferences().GetSetPreference("MIXTimeSystems", "Code");
	Preferences().GetSetPreference("MIXLegends", "Code");
	Preferences().GetSetPreference("MIXWindowSize", "None");
	Preferences().GetSetPreference("MIXTimeParameters", "None");
}

void UI_save::initialize() {
	if(!ui_save)
		ui_save = new UI_save("Save Files", MW);

	// cerr << "UI_save::initialize(): ListOfAllFileNames has " << eval_intfc::ListOfAllFileNames().get_length() << " element(s)\n";

	ui_save->updateMainList(eval_intfc::ListOfAllFileNames());
	if(last_native_file().length())
		*ui_save->apf_text = last_native_file();
	ui_save->frame1->manage();
	ui_save->frame2->manage();
	ui_save->frame3->manage();
	ui_save->paned_window->manage();
}

void UI_save::CancelButtonCallback(Widget, callback_stuff * client_data, void *) {
	UI_save		*obj = (UI_save *) client_data->data;

	obj->paned_window->unmanage();
	if(obj->directive_options_popup) {
		obj->directive_options_popup->user_has_made_a_choice = 1;
		obj->directive_options_popup->paned_window->unmanage(); } }

int get_button_index_from_string(const string& val)
{
  if(val == "Comments")
    return 0;
  else if(val == "Code")
    return 1;
  else
    return 2;
}

int get_button_index_from_pointer(void * v) {
	long		i = (long) v;

	// buttons are in the order COMMENTS (i = 1), CODE (i = 2), NONE (i = 0)
	i = i - 1;
	if(i < 0) i = 2;
	return (int) i; }

void *get_pointer_from_button_index(int i) {
	i = i + 1;
	if(i > 2) i = 0;
	long d = i;
	return (void *) d; }

Action_request::save_option get_pointer_from_string(const string& val) {
  if(val == "Comments") {
    return Action_request::INCLUDE_AS_COMMENTS; }
  if(val == "Code") {
    return Action_request::INCLUDE_AS_CODE; }
  return Action_request::INCLUDE_NOT_AT_ALL; }

void 
UI_save::GetSaveOptionsListFromPrefs(tlist<alpha_string, optionNode>* listToSet) const
{
  string pref;
  Preferences().GetPreference("APFGlobals", &pref);
  (*listToSet) << new optionNode("APF GLOBALS",	get_pointer_from_string(pref)); 	
  Preferences().GetPreference("APFEpochs", &pref);
  (*listToSet) << new optionNode("APF EPOCHS",	get_pointer_from_string(pref));
  Preferences().GetPreference("APFTimeSystems", &pref);
  (*listToSet) << new optionNode("APF TIME_SYSTEMS",	get_pointer_from_string(pref));
  Preferences().GetPreference("APFLegends", &pref);
  (*listToSet) << new optionNode("APF LEGENDS",	get_pointer_from_string(pref));
  Preferences().GetPreference("APFWindowSize", &pref);
  (*listToSet) << new optionNode("APF WIN_SIZE",	get_pointer_from_string(pref));
  Preferences().GetPreference("APFTimeParameters", &pref);
  (*listToSet) << new optionNode("APF TIME_PARAMS",	get_pointer_from_string(pref));
  
  Preferences().GetPreference("AAFGlobals", &pref);
  (*listToSet) << new optionNode("AAF GLOBALS",	get_pointer_from_string(pref));
  Preferences().GetPreference("AAFEpochs", &pref);
  (*listToSet) << new optionNode("AAF EPOCHS",	get_pointer_from_string(pref));
  Preferences().GetPreference("AAFTimeSystems", &pref);
  (*listToSet) << new optionNode("AAF TIME_SYSTEMS",	get_pointer_from_string(pref));
  Preferences().GetPreference("AAFLegends", &pref);
  (*listToSet) << new optionNode("AAF LEGENDS",	get_pointer_from_string(pref));
  Preferences().GetPreference("AAFWindowSize", &pref);
  (*listToSet) << new optionNode("AAF WIN_SIZE",	get_pointer_from_string(pref));
  Preferences().GetPreference("AAFTimeParameters", &pref);
  (*listToSet) << new optionNode("AAF TIME_PARAMS",	get_pointer_from_string(pref));
  
  Preferences().GetPreference("MIXGlobals", &pref);
  (*listToSet) << new optionNode("MIX GLOBALS",	get_pointer_from_string(pref));
  Preferences().GetPreference("MIXEpochs", &pref);
  (*listToSet) << new optionNode("MIX EPOCHS",	get_pointer_from_string(pref));
  Preferences().GetPreference("MIXTimeSystems", &pref);
  (*listToSet) << new optionNode("MIX TIME_SYSTEMS",	get_pointer_from_string(pref));
  Preferences().GetPreference("MIXLegends", &pref);
  (*listToSet) << new optionNode("MIX LEGENDS",	get_pointer_from_string(pref));
  Preferences().GetPreference("MIXWindowSize", &pref);
  (*listToSet) << new optionNode("MIX WIN_SIZE",	get_pointer_from_string(pref));
  Preferences().GetPreference("MIXTimeParameters", &pref);
  (*listToSet) << new optionNode("MIX TIME_PARAMS",	get_pointer_from_string(pref));
}

void UI_save::OKButtonCallback(Widget, callback_stuff * client_data, void *) {
	UI_save			*obj = (UI_save *) client_data->data;
	SAVE_FILErequest	*request = NULL;
	Cstring			empty;
	strintslist		selected_files;
	strintslist::iterator	the_selected_files(selected_files);
	strint_node*		p;
	tlist<alpha_string, optionNode>	aCopyOfTheOptionsList;

	obj->GetSaveOptionsListFromPrefs(&aCopyOfTheOptionsList);

	obj->paned_window->unmanage();

	((scrolled_list_widget *) obj->files_to_exclude)->get_selected_items(selected_files);
	DBG_NOINDENT("UI_save::OKButtonCallBack: " << selected_files.get_length() << " files excluded...\n");

	stringslist		string_args;

	while((p = the_selected_files())) {
		string_args << new emptySymbol(p->get_key()); }

	if(obj->save_in_separate_files->get_text() == "SET") {
		last_native_file() = obj->apf_text->get_text();
		request = new SAVE_FILErequest(
				obj->aaf_text->get_text(),
				apgen::FileType::FT_AAF,
				string_args,
				aCopyOfTheOptionsList);
		request->process();
		delete request;
		Cstring apf_file_name(obj->apf_text->get_text());
		char first_char = apf_file_name[0];
		if(first_char != '\'' && first_char != '"') {
			apf_file_name = addQuotes(apf_file_name); }
		request = new SAVE_FILErequest(
				apf_file_name,
				apgen::FileType::FT_APF,
				string_args,
				aCopyOfTheOptionsList);
		request->process();
		delete request;
	} else if(obj->save_in_one_file->get_text() == "SET") {
		request = new SAVE_FILErequest(
				obj->mix_text->get_text(),
				apgen::FileType::FT_MIXED,
				string_args,
				aCopyOfTheOptionsList);
		request->process();
		delete request;
	} else if(obj->save_apf_only->get_text() == "SET") {
		Cstring apf_file_name(obj->apf_text->get_text());
		char first_char = apf_file_name[0];
		if(first_char != '\'' && first_char != '"') {
			apf_file_name = addQuotes(apf_file_name); }
		last_native_file() = apf_file_name;
		request = new SAVE_FILErequest(
				apf_file_name,
				apgen::FileType::FT_APF,
				string_args,
				aCopyOfTheOptionsList);
		request->process();
		delete request;
	}
#ifdef OBSOLETE
	else if(obj->save_plan_xml_only->get_text() == "SET") {
		last_native_file() = obj->apf_text->get_text();
		request = new SAVE_FILErequest(
				obj->apf_text->get_text(),
				apgen::FileType::FT_XML,
				string_args,
				aCopyOfTheOptionsList);
		request->process();
		delete request;
	}
#endif /* OBSOLETE */
	else if(obj->save_legend_layout->get_text() == "SET") {
		// last_native_file() = obj->apf_text->get_text();
		// fake it: selected files contains the tag
		string_args.clear();
		string_args << new emptySymbol(obj->tag_text->get_text());
		request = new SAVE_FILErequest(
				obj->apf_text->get_text(),
				apgen::FileType::FT_LAYOUT,
				string_args,
				aCopyOfTheOptionsList);
		request->process();
		delete request;
	}
}


void SaveDirectivesCallback(Widget, callback_stuff *client_data, void *) 
{
#ifdef GUI
  if(GUI_Flag) 
  {
    string which_file;
    
    if(client_data->parent == UI_save::ui_save->directiveOptions_apf) 
    {
      which_file = "APF"; 
    }
    else if(client_data->parent == UI_save::ui_save->directiveOptions_aaf) 
    {
      which_file = "AAF"; 
    }
    else if(client_data->parent == UI_save::ui_save->directiveOptions_mix) 
    {
      which_file = "MIX"; 
    }
    
    if(! UI_save::ui_save->directive_options_popup) 
    {
      UI_save::ui_save->directive_options_popup = new directive_options(UI_save::ui_save); 
    }
    
    UI_save::ui_save->directive_options_popup->initialize(which_file);

    // New style:
    // UI_save::ui_save->directive_options_popup->wait_for_user_action(UI_save::ui_save->directive_options_popup);
    return; 
  
  }
#endif
}

directive_options::directive_options(motif_widget *par)
		: motif_widget("Directive Options", xmDialogShellWidgetClass, par, NULL, 0, FALSE),
		theDataForWindowClosing(NULL, CancelButtonCallback, NULL, this) {
#ifdef GUI
	if(GUI_Flag) {
	motif_widget			*labels[6], *form1, *form2, *button,
					*button1, *button2, *button3,
					*frames[6], *frame1, *frame2, *frame3;

	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, (void *) &theDataForWindowClosing);

	paned_window = new motif_widget("paned", xmPanedWindowWidgetClass, this, NULL, 0, FALSE);
	form1 = new motif_widget("form1", paned_window, form_position(6), true);
	top_label = new motif_widget("Directives will be included as follows:", xmLabelWidgetClass, form1);

	labels[0] = new motif_widget("Globals", xmLabelWidgetClass, form1);
	frames[0] = new motif_widget("frame1", xmFrameWidgetClass, form1);
	radioboxes[0] = new motif_widget("radiobox1", xmRadioBoxWidgetClass, frames[0]);
	button1 = new motif_widget("As Comments",	xmToggleButtonWidgetClass, radioboxes[0]);
	button2 = new motif_widget("As Code",		xmToggleButtonWidgetClass, radioboxes[0]);
	button3 = new motif_widget("Not Included",	xmToggleButtonWidgetClass, radioboxes[0]);

	labels[1] = new motif_widget("Epochs", xmLabelWidgetClass, form1);
	frames[1] = new motif_widget("frame1", xmFrameWidgetClass, form1);
	radioboxes[1] = new motif_widget("radiobox2", xmRadioBoxWidgetClass, frames[1]);
	button1 = new motif_widget("As Comments",	xmToggleButtonWidgetClass, radioboxes[1]);
	button2 = new motif_widget("As Code",		xmToggleButtonWidgetClass, radioboxes[1]);
	button3 = new motif_widget("Not Included",	xmToggleButtonWidgetClass, radioboxes[1]);

	labels[2] = new motif_widget("Time Systems", xmLabelWidgetClass, form1);
	frames[2] = new motif_widget("frame1", xmFrameWidgetClass, form1);
	radioboxes[2] = new motif_widget("radiobox3", xmRadioBoxWidgetClass, frames[2]);
	button1 = new motif_widget("As Comments",	xmToggleButtonWidgetClass, radioboxes[2]);
	button2 = new motif_widget("As Code",		xmToggleButtonWidgetClass, radioboxes[2]);
	button3 = new motif_widget("Not Included",	xmToggleButtonWidgetClass, radioboxes[2]);

	labels[3] = new motif_widget("Legends", xmLabelWidgetClass, form1);
	frames[3] = new motif_widget("frame1", xmFrameWidgetClass, form1);
	radioboxes[3] = new motif_widget("radiobox4", xmRadioBoxWidgetClass, frames[3]);
	button1 = new motif_widget("As Comments",	xmToggleButtonWidgetClass, radioboxes[3]);
	button2 = new motif_widget("As Code",		xmToggleButtonWidgetClass, radioboxes[3]);
	button3 = new motif_widget("Not Included",	xmToggleButtonWidgetClass, radioboxes[3]);

	labels[4] = new motif_widget("Time Params.", xmLabelWidgetClass, form1);
	frames[4] = new motif_widget("frame1", xmFrameWidgetClass, form1);
	radioboxes[4] = new motif_widget("radiobox5", xmRadioBoxWidgetClass, frames[4]);
	button1 = new motif_widget("As Comments",	xmToggleButtonWidgetClass, radioboxes[4]);
	button2 = new motif_widget("As Code",		xmToggleButtonWidgetClass, radioboxes[4]);
	button3 = new motif_widget("Not Included",	xmToggleButtonWidgetClass, radioboxes[4]);

	labels[5] = new motif_widget("Window Size", xmLabelWidgetClass, form1);
	frames[5] = new motif_widget("frame1", xmFrameWidgetClass, form1);
	radioboxes[5] = new motif_widget("radiobox6", xmRadioBoxWidgetClass, frames[5]);
	button1 = new motif_widget("As Comments",	xmToggleButtonWidgetClass, radioboxes[5]);
	button2 = new motif_widget("As Code",		xmToggleButtonWidgetClass, radioboxes[5]);
	button3 = new motif_widget("Not Included",	xmToggleButtonWidgetClass, radioboxes[5]);

	*form1 < *top_label < *form1 < endform;

	*form1	< *labels[0] < form_position(1)
		< *labels[1] < form_position(2)
		< *labels[2] < form_position(3)
		< *labels[3] < form_position(4)
		< *labels[4] < form_position(5)
		< *labels[5] < *form1 < endform;

	*form1	< *frames[0] < form_position(1)
		< *frames[1] < form_position(2)
		< *frames[2] < form_position(3)
		< *frames[3] < form_position(4)
		< *frames[4] < form_position(5)
		< *frames[5] < *form1 < endform;

	*form1 ^ *top_label ^ endform;
	*top_label ^ 10 ^ *labels[0] ^ *frames[0] ^ *form1 ^ endform;
	*top_label ^ 10 ^ *labels[1] ^ *frames[1] ^ *form1 ^ endform;
	*top_label ^ 10 ^ *labels[2] ^ *frames[2] ^ *form1 ^ endform;
	*top_label ^ 10 ^ *labels[3] ^ *frames[3] ^ *form1 ^ endform;
	*top_label ^ 10 ^ *labels[4] ^ *frames[4] ^ *form1 ^ endform;
	*top_label ^ 10 ^ *labels[5] ^ *frames[5] ^ *form1 ^ endform;

	form2 = new  motif_widget("form2", paned_window, form_position(5), true);
	OKbutton = new motif_widget("OK", xmPushButtonWidgetClass, form2);
	*form2 ^ *OKbutton ^ *form2 ^ endform;
	form_position(1) < *OKbutton < form_position(2) < endform;
	OKbutton->add_callback(OKButtonCallback, XmNactivateCallback, this);
	button = new motif_widget("Cancel", xmPushButtonWidgetClass, form2);
	*form2 ^ *button ^ *form2 ^ endform;
	form_position(3) < *button < form_position(4) < endform;
	button->add_callback(CancelButtonCallback, XmNactivateCallback, this);
#ifdef HAVE_MACOS
	form2->fix_height(32, OKbutton->widget);
#else
	form2->fix_height(OKbutton->widget);
#endif /* HAVE_MACOS */
	}
#endif /* GUI */
	}

void 
directive_options::initialize(const string& fileType)
{
  Type = fileType;

  Cstring panel_title = Type + " Option Selection Panel";

  string globals;
  Preferences().GetPreference(Type+"Globals", &globals);
  string epochs;
  Preferences().GetPreference(Type+"Epochs", &epochs);
  string timesystems;
  Preferences().GetPreference(Type+"TimeSystems", &timesystems);
  string legends;
  Preferences().GetPreference(Type+"Legends", &legends);
  string timeparams;
  Preferences().GetPreference(Type+"TimeParameters", &timeparams);
  string winsize;
  Preferences().GetPreference(Type+"WindowSize", &winsize);
#ifdef GUI 
  XtVaSetValues(UI_save::ui_save->directive_options_popup->widget, XmNtitle, *panel_title, NULL);
  *UI_save::ui_save->directive_options_popup->top_label = Cstring("The ") + Type + " file will contain directives formatted as follows:";
  *((motif_widget *) UI_save::ui_save->directive_options_popup->radioboxes[0]->children[get_button_index_from_string(globals)])	= "2";
  *((motif_widget *) UI_save::ui_save->directive_options_popup->radioboxes[1]->children[get_button_index_from_string(epochs)])	= "2";
  *((motif_widget *) UI_save::ui_save->directive_options_popup->radioboxes[2]->children[get_button_index_from_string(timesystems)])	= "2";
  *((motif_widget *) UI_save::ui_save->directive_options_popup->radioboxes[3]->children[get_button_index_from_string(legends) ])	= "2";
  *((motif_widget *) UI_save::ui_save->directive_options_popup->radioboxes[4]->children[get_button_index_from_string(timeparams) ])	= "2";
  *((motif_widget *) UI_save::ui_save->directive_options_popup->radioboxes[5]->children[get_button_index_from_string(winsize) ])	= "2";
  
  UI_save::ui_save->directive_options_popup->paned_window->manage();
#endif /* ifdef GUI */
}

void directive_options::OKButtonCallback(Widget, callback_stuff * client_data, void *) {
#ifdef GUI
  directive_options		*obj = (directive_options *) client_data->data;
  
  /*
   *	OK, the user has made up her mind. Time to collect options...
   */

  if(((motif_widget *) obj->radioboxes[0]->children[0])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"Globals", "Comments"); 
  }
  else if(((motif_widget *) obj->radioboxes[0]->children[1])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"Globals", "Code");
  }
  else if(((motif_widget *) obj->radioboxes[0]->children[2])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"Globals", "None");
  }
  
  if(((motif_widget *) obj->radioboxes[1]->children[0])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"Epochs", "Comments");
  }
  else if(((motif_widget *) obj->radioboxes[1]->children[1])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"Epochs", "Code");
  }
  else if(((motif_widget *) obj->radioboxes[1]->children[2])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"Epochs", "None");
  }
  
  if(((motif_widget *) obj->radioboxes[2]->children[0])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"TimeSystems", "Comments");
  }
  else if(((motif_widget *) obj->radioboxes[2]->children[1])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"TimeSystems", "Code");
  }
  else if(((motif_widget *) obj->radioboxes[2]->children[2])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"TimeSystems", "None");
  }
  
  if(((motif_widget *) obj->radioboxes[3]->children[0])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"Legends", "Comments");
  }
  else if(((motif_widget *) obj->radioboxes[3]->children[1])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"Legends", "Code");
  }
  else if(((motif_widget *) obj->radioboxes[3]->children[2])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"Legends", "None");
  }
  
  if(((motif_widget *) obj->radioboxes[4]->children[0])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"TimeParameters", "Comments");
  }
  else if(((motif_widget *) obj->radioboxes[4]->children[1])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"TimeParameters", "Code");
  }
  else if(((motif_widget *) obj->radioboxes[4]->children[2])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"TimeParameters", "None");
  }
  
  if(((motif_widget *) obj->radioboxes[5]->children[0])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"WindowSize", "Comments");
  }
  else if(((motif_widget *) obj->radioboxes[5]->children[1])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"WindowSize", "Code");
  }
  else if(((motif_widget *) obj->radioboxes[5]->children[2])->get_text() == "SET") 
  {
    Preferences().SetPreference(obj->Type+"WindowSize", "None");
  } 
  
  Preferences().SavePreferences();

  obj->paned_window->unmanage();

  obj->user_has_made_a_choice = 2;
#endif /* ifdef GUI */
}

void directive_options::CancelButtonCallback(Widget, callback_stuff * client_data, void *) {
	directive_options		*obj = (directive_options *) client_data->data;

	obj->paned_window->unmanage();
	obj->user_has_made_a_choice = 1; 
}

void UI_save::SaveEverything(Widget, callback_stuff * client_data, void *) {
	UI_save		*obj = (UI_save *) client_data->data;

	if(obj->save_everything->get_text() == "SET") {
		((scrolled_list_widget *) obj->files_to_exclude)->deselect_all();
		obj->files_to_exclude->set_sensitive(FALSE);
		obj->exclude_label->set_sensitive(FALSE); }
	else if(obj->do_not_save_everything->get_text() == "SET") {
		obj->files_to_exclude->set_sensitive(TRUE);
		obj->exclude_label->set_sensitive(TRUE); } }

void UI_save::desensitize_file_list() {
	*save_everything = 2; }

void UI_save::HowManyFilesCallback(Widget, callback_stuff * client_data, void *) {
	UI_save		*obj = (UI_save *) client_data->data;

	if(obj->save_in_separate_files->get_text() == "SET") {
		obj->mix_text->set_sensitive(FALSE);
		obj->mix_label->set_sensitive(FALSE);
		obj->directiveOptions_mix->set_sensitive(FALSE);
		obj->aaf_text->set_sensitive(TRUE);
		obj->aaf_label->set_sensitive(TRUE);
		obj->directiveOptions_aaf->set_sensitive(TRUE);
		obj->apf_text->set_sensitive(TRUE);
		obj->apf_label->set_sensitive(TRUE);
		obj->tag_label->set_sensitive(FALSE);
		obj->tag_text->set_sensitive(FALSE);
		*obj->apf_text = "apgen.apf";
		obj->directiveOptions_apf->set_sensitive(TRUE); }
	else if(obj->save_in_one_file->get_text() == "SET") {
		obj->mix_text->set_sensitive(TRUE);
		obj->mix_label->set_sensitive(TRUE);
		obj->directiveOptions_mix->set_sensitive(TRUE);
		obj->aaf_text->set_sensitive(FALSE);
		obj->aaf_label->set_sensitive(FALSE);
		obj->directiveOptions_aaf->set_sensitive(FALSE);
		obj->apf_text->set_sensitive(FALSE);
		obj->apf_label->set_sensitive(FALSE);
		obj->tag_label->set_sensitive(FALSE);
		obj->tag_text->set_sensitive(FALSE);
		obj->directiveOptions_apf->set_sensitive(FALSE); }
	else if(obj->save_apf_only->get_text() == "SET") {
		obj->mix_text->set_sensitive(FALSE);
		obj->mix_label->set_sensitive(FALSE);
		obj->directiveOptions_mix->set_sensitive(FALSE);
		obj->aaf_text->set_sensitive(FALSE);
		obj->aaf_label->set_sensitive(FALSE);
		obj->directiveOptions_aaf->set_sensitive(FALSE);
		obj->apf_text->set_sensitive(TRUE);
		obj->apf_label->set_sensitive(TRUE);
		obj->tag_label->set_sensitive(FALSE);
		obj->tag_text->set_sensitive(FALSE);
		*obj->apf_text = "apgen.apf";
		obj->directiveOptions_apf->set_sensitive(TRUE); }
#ifdef OBSOLETE
	else if(obj->save_plan_xml_only->get_text() == "SET") {
		obj->mix_text->set_sensitive(FALSE);
		obj->mix_label->set_sensitive(FALSE);
		obj->directiveOptions_mix->set_sensitive(FALSE);
		obj->aaf_text->set_sensitive(FALSE);
		obj->aaf_label->set_sensitive(FALSE);
		obj->directiveOptions_aaf->set_sensitive(FALSE);
		obj->apf_text->set_sensitive(TRUE);
		obj->apf_label->set_sensitive(TRUE);
		obj->tag_label->set_sensitive(FALSE);
		obj->tag_text->set_sensitive(FALSE);
		*obj->apf_text = "apgen.xml";
		obj->directiveOptions_apf->set_sensitive(TRUE); }
#endif /* OBSOLETE */
	else if(obj->save_legend_layout->get_text() == "SET") {
		obj->mix_text->set_sensitive(FALSE);
		obj->mix_label->set_sensitive(FALSE);
		obj->directiveOptions_mix->set_sensitive(FALSE);
		obj->aaf_text->set_sensitive(FALSE);
		obj->aaf_label->set_sensitive(FALSE);
		obj->directiveOptions_aaf->set_sensitive(FALSE);
		obj->apf_text->set_sensitive(TRUE);
		obj->apf_label->set_sensitive(TRUE);
		obj->directiveOptions_apf->set_sensitive(TRUE);
		obj->files_to_exclude->set_sensitive(FALSE);
		obj->do_not_save_everything->set_sensitive(FALSE);
		obj->save_everything->set_sensitive(FALSE);
		obj->tag_label->set_sensitive(TRUE);
		obj->tag_text->set_sensitive(TRUE);
		*obj->apf_text = "layout.apf";
		}
	}

void UI_save::updateMainList(const stringslist& L) {
	emptySymbol*		N;
	stringslist::iterator	files(L);
	int			the_index = 0;

	files_to_exclude->clear();
	while((N = files.next())) {
		*((scrolled_list_widget *) files_to_exclude) << N->get_key(); }
	while((N = files.next())) {
		the_index++;	// Motif starts at 1 !!
		if(fileReader::listOfFilesReadThroughSeqReview().find(N->get_key())) {
			((scrolled_list_widget *) files_to_exclude)->select_pos(the_index); } }
	if(fileReader::listOfFilesReadThroughSeqReview().get_length()) {
		*do_not_save_everything = "2"; }
	else {
		*save_everything = "2"; } }

void pwd_popup::check_passwd(Widget w, callback_stuff* clientData, void* callData) {
}


pwd_popup::pwd_popup(
		const Cstring&	name			//will be name of top widget
		)
	: motif_widget(	name,
			xmDialogShellWidgetClass,
			MW,
			NULL, 0,
			false),		//i.e. should NOT [be] manage[d]
	  gotThePassword(false),
	  data_for_window_closing(
			NULL,
			CancelButtonCallback,
			NULL,
			get_this_widget()) {
#ifdef GUI
	if(GUI_Flag) {

	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, &data_for_window_closing);

	int n = 0;
	Arg args[20];

	XtSetArg(args[n], XmNtitle, "Password for TMS access"); n++;
	XtSetValues(widget, args, n);

	pwdPopupPanedWindow = new motif_widget(
		"pwd_popup_paned",
		xmPanedWindowWidgetClass,
		this,
		NULL, 0,
		false);

	/* Create the top-level Form (child of DialogShell) and all of its children.
	 * !!!FUTURE need better way to specify common resources (width, height,
	 *  background, fontList, etc.) -- perhaps group together as defined STYLE
	 *  (as in BX); OR, use fallback resources for default style (see unused
	 *  fallbacks[] in UI_motif_widget.C); OR, create method with lots of args
	 *  for widget type and common resources, plus "args, n" for any others. */


	// For password entry:
	pwdPopupRowColumn = new motif_widget("pwdPopupRowColumn", xmRowColumnWidgetClass, pwdPopupPanedWindow);
	n = 0;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetValues(pwdPopupRowColumn->widget, args, n);

	pwdPopupLabel = new motif_widget("pwdPopupLabel", xmLabelWidgetClass, pwdPopupRowColumn);
	*pwdPopupLabel = "Password:";

	pwdPopupTxt = new single_line_text_widget(
		"pwdPopupTxt",
		pwdPopupRowColumn,
		EDITABLE,
		true);
	pwdPopupTxt->add_callback(
		pwd_popup::check_passwd, XmNmodifyVerifyCallback, this);
	pwdPopupTxt->add_callback(
		pwd_popup::check_passwd, XmNactivateCallback, this);

// #ifdef PREMATURE

	//Form fraction base of 39 is optimal for 1-button Form (APGEN BX usage)
	pwdPopupButtonForm = new motif_widget(
		"pwdPopupButtonForm",
		pwdPopupPanedWindow,
		form_position(39),
		true);

	pwdPopupOKButton = new motif_widget(
		"pwdPopupOKButton",
		xmPushButtonWidgetClass,
		pwdPopupButtonForm);
	*pwdPopupButtonForm ^ *pwdPopupOKButton ^ *pwdPopupButtonForm ^ endform;
	form_position(10) < *pwdPopupOKButton < form_position(29) < endform;
	*pwdPopupOKButton = Cstring("OK");
	pwdPopupOKButton->add_callback(
		pwd_popup::OKButtonCallback,
		XmNactivateCallback,
		this);

#ifdef HAVE_MACOS
	pwdPopupButtonForm->fix_height(32, pwdPopupOKButton->widget);
#else
	pwdPopupButtonForm->fix_height(pwdPopupOKButton->widget);
#endif /* HAVE_MACOS */

// #endif /* PREMATURE */
	}
#endif /* GUI */
}

pwd_popup::~pwd_popup() {}

void pwd_popup::CancelButtonCallback(Widget, callback_stuff* clientData, void*) {
	pwd_popup* obj0 = (pwd_popup*) clientData->data;

	obj0->pwdPopupPanedWindow->unmanage();
}

void pwd_popup::OKButtonCallback(Widget, callback_stuff* clientData, void*) {
	pwd_popup* obj0 = (pwd_popup*) clientData->data;


	obj0->pwdPopupPanedWindow->unmanage();
}

UI_export::UI_export(
		const Cstring&	name,
		motif_widget*	parent_widget)
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget, NULL, 0, FALSE),
		passwd(NULL),
		how_option(EXPORT_DATArequest::JSON_FILE),
		data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()) {
	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, &data_for_window_closing);

	// NOTE: the constructor is only called once, so the following code will execute at most once:

	paned_window			= new motif_widget("paned", xmPanedWindowWidgetClass, this, NULL, 0, FALSE);
	motif_widget*	form1		= new motif_widget("form1", paned_window, form_position(100), true);
	frame1				= new motif_widget("frame1", xmFrameWidgetClass, form1, NULL, 0, FALSE);
	motif_widget*	radio_form	= new motif_widget("form17", frame1, form_position(100), true);
	motif_widget*	checkbox	= new motif_widget("RadioBox1", xmCheckBoxWidgetClass, radio_form);
	motif_widget*	radiobox	= new motif_widget("RadioBox2", xmRadioBoxWidgetClass, radio_form);

	export_act_defs			= new motif_widget("Export activity interactions", xmToggleButtonWidgetClass, checkbox);
	export_acts			= new motif_widget("Export activity instances", xmToggleButtonWidgetClass, checkbox);
	export_res_def			= new motif_widget("Export resource definitions", xmToggleButtonWidgetClass, checkbox);
	export_res_hist			= new motif_widget("Export resource histories", xmToggleButtonWidgetClass, checkbox);

	export_via_json			= new motif_widget("Export Jason Files", xmToggleButtonWidgetClass, radiobox);
	export_via_xdr			= new motif_widget("Export XDR Files", xmToggleButtonWidgetClass, radiobox);

	export_via_curl->add_callback(HowToExportCallback, XmNvalueChangedCallback, this);
	export_via_json->add_callback(HowToExportCallback, XmNvalueChangedCallback, this);
	export_via_xdr->add_callback(HowToExportCallback, XmNvalueChangedCallback, this);

#ifdef UNNECESSARY
	export_act_defs->add_callback(WhatToExportCallback, XmNvalueChangedCallback, this);
	export_acts->add_callback(WhatToExportCallback, XmNvalueChangedCallback, this);
	export_res_def->add_callback(WhatToExportCallback, XmNvalueChangedCallback, this);
	export_res_hist->add_callback(WhatToExportCallback, XmNvalueChangedCallback, this);
#endif /* UNNECESSARY */

	*export_act_defs = 1;
	*export_acts = 1;
	*export_res_def = 1;
	*export_res_hist = 1;

	*export_via_curl = 1;

	*radio_form < *radiobox < 10 < *checkbox < *radio_form < endform;

	motif_widget*	form2		= new motif_widget("form2", xmFormWidgetClass, paned_window, NULL, 0, TRUE);
	frame2				= new motif_widget("Frame2", xmFrameWidgetClass, form2, NULL, 0, FALSE);
	motif_widget*	form3		= new motif_widget("Form3", frame2, form_position(15), true);

	// URL: "https://<server>:<port number>/tms/v4/scn/<server change number>/timeline/data/<schema>/"
	motif_widget*	tag_label	= new motif_widget("URL: https://", xmLabelWidgetClass, form3);
	url_text_server			= new single_line_text_widget("URL", form3, EDITABLE);
	motif_widget*	tag_label1	= new motif_widget(":", xmLabelWidgetClass, form3);
	url_text_port			= new single_line_text_widget("URL2", form3, EDITABLE);
	motif_widget*	tag_label2	= new motif_widget("/tms/v4/scn/", xmLabelWidgetClass, form3);
	url_text_scn			= new single_line_text_widget("URL3", form3, EDITABLE);
	motif_widget*	tag_label3	= new motif_widget("/timeline/(meta)data/", xmLabelWidgetClass, form3);
	url_text_schema			= new single_line_text_widget("URL4", form3, EDITABLE);

	*url_text_server = "seqrdev2.jpl.nasa.gov";
	*url_text_port = "8443";
	*url_text_scn = "latest";
	const char*	usr = getenv("USER");
	Cstring		Usr(usr);
	Usr << "1";
	*url_text_schema = *Usr;

	*form3 ^ *tag_label ^ *form3 ^ endform;
	*form3 ^ *tag_label1 ^ *form3 ^ endform;
	*form3 ^ *tag_label2 ^ *form3 ^ endform;
	*form3 ^ *tag_label3 ^ *form3 ^ endform;
	*form3 ^ *url_text_server ^ *form3 ^ endform;
	*form3 ^ *url_text_port ^ *form3 ^ endform;
	*form3 ^ *url_text_scn ^ *form3 ^ endform;
	*form3 ^ *url_text_schema ^ *form3 ^ endform;

	// URL: "https://<server>:<port number>/tms/v4/scn/<server change number>/timeline/data/<schema>/"

	*form3 < *tag_label < *url_text_server < *tag_label1 < *url_text_port < *tag_label2 < *url_text_scn < *tag_label3 < *url_text_schema < *form3 < endform;

	tag_label->set_sensitive(TRUE);
	url_text_server->set_sensitive(TRUE);
	url_text_port->set_sensitive(TRUE);
	url_text_scn->set_sensitive(TRUE);
	url_text_schema->set_sensitive(TRUE);

	frame3				= new motif_widget("Frame3", xmFrameWidgetClass, form2, NULL, 0, FALSE);
	motif_widget*	form4		= new motif_widget("Form4", frame3, form_position(15), true);

	// URL: "https://<server>:<port number>/tms/v4/scn/<server change number>/timeline/data/<schema>/"
	motif_widget*	tag_label4	= new motif_widget("Namespace", xmLabelWidgetClass, form4);
	Cstring	default_ns("/test/");
	default_ns << usr << "/";
	namespace_text			= new single_line_text_widget("Namespace", form4, EDITABLE);
	motif_widget*	tag_label5	= new motif_widget("activity directory:", xmLabelWidgetClass, form4);
	directory_text			= new single_line_text_widget("Directory", form4, EDITABLE);

	*namespace_text = *default_ns;
	*directory_text = "activities";

	*form4 < *tag_label4 < *namespace_text < *tag_label5 < *directory_text < endform;
	*form4 ^ *tag_label4 ^ endform;
	*form4 ^ *namespace_text ^ endform;
	*form4 ^ *tag_label5 ^ endform;
	*form4 ^ *directory_text ^ endform;

	*form1 < *frame1 < *form1 < endform;
	*form2 < *frame2 < *form2 < endform;
	*form2 < *frame3 < *form2 < endform;
	*form1 ^ *frame1 ^ *form1 ^ endform;
	*form2 ^ *frame2 ^ 5 ^ *frame3 ^ *form2 ^ endform;

	motif_widget*	form5		= new  motif_widget("form5", paned_window, form_position(20), true);
	motif_widget*	OKbutton	= new motif_widget("Save", xmPushButtonWidgetClass, form5);
	*form5 ^ *OKbutton ^ *form5 ^ endform;
	form_position(4) < *OKbutton < form_position(8) < endform;
	OKbutton->add_callback(OKButtonCallback, XmNactivateCallback, this);
	motif_widget*	button		= new motif_widget("Cancel", xmPushButtonWidgetClass, form5);
	*form5 ^ *button ^ *form5 ^ endform;
	form_position(12) < *button < form_position(16) < endform;
	button->add_callback(CancelButtonCallback, XmNactivateCallback, this);
#ifdef HAVE_MACOS
	// form1->fix_height(155, frame1->widget);
	// form2->fix_height(40, frame2->widget);
	form5->fix_height(32, OKbutton->widget);
#else
	form1->fix_height(frame1->widget);
	// form2->fix_height(frame2->widget);
	form5->fix_height(OKbutton->widget);
#endif /* HAVE_MACOS */
	}

void UI_export::initialize() {
	if(!ui_export) {
		ui_export = new UI_export("Export to JSON", MW);
	}
	DBG_NOINDENT("    UI_export::initialize()\n");
	ui_export->frame1->manage();
	ui_export->frame2->manage();
	ui_export->frame3->manage();
	ui_export->paned_window->manage();
}

void UI_export::WhatToExportCallback(Widget, callback_stuff* client_data, void*) {
	UI_export*	obj = (UI_export*) client_data->data;

	if(obj->export_acts->get_text() == "SET") {
		; }
}

void UI_export::HowToExportCallback(Widget, callback_stuff* client_data, void*) {
	UI_export*	obj = (UI_export*) client_data->data;

	if(obj->export_via_json->get_text() == "SET") {
		obj->how_option = EXPORT_DATArequest::JSON_FILE;
	}
}


void UI_export::OKButtonCallback(Widget, callback_stuff * client_data, void *) {
	UI_export*	obj = (UI_export*) client_data->data;

	obj->paned_window->unmanage();
}

void UI_export::CancelButtonCallback(Widget, callback_stuff * client_data, void *) {
	UI_export*	obj = (UI_export*) client_data->data;

	obj->paned_window->unmanage();
}
