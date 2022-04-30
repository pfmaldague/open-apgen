#if HAVE_CONFIG_H
#include <config.h>
#endif

// define apDEBUG

#include <assert.h>
#include "apDEBUG.H"

#include "ACT_exec.H"
#include <ActivityInstance.H>
#include "ACT_sys.H"
// #include "ActivityType.H"
#include "action_request.H"
#include "APmodel.H"
#include "fileReader.H"
#include "IO_write.H"
#include "RD_sys.H"
#include "UI_activitydisplay.H"
#include "UI_ds_timeline.H"
#include "UI_exec.H"
#include "UI_mainwindow.H"
#include "UI_messagewarndialog.H"
#include "UI_popups.H"
#include "DB.H"
#include "Prefs.H"
#include <unistd.h>	// for getcwd()
#include <sys/stat.h>	// for stat()

#ifdef GUI
#	include <Xm/PushB.h>
#endif

// GLOBALS:

tol_popup               *_tolPopupDialogShell;
con_popup		*_conPopupDialogShell;
rd_popup                *_rdPopupDialogShell;
UI_rdBackgroundPts	*_rdBackgroundPtsPopupDialogShell;
UI_rdOptions		*_rdOptionsPopupDialogShell;
int			UI_rdOptions::saved_mode = 0;
int			UI_rdBackgroundPts::saved_resolution = 0;
void			*UI_rdBackgroundPts::current_RES_sys;
UI_newLegend		*_nlPopupDialogShell;

int			user_waiter::we_are_in_automatic_mode = 0;

int			user_waiter::user_has_made_a_choice;

using namespace std;

extern "C" {
	extern void enable_signals();
	extern void disable_signals(int k); }

			// in IO_seqtalk.C:
extern void		bell();

			// the "Write Partial Plan File" panel:
UI_partial_save		*ui_partial_save = NULL;

                        //the "Export Data" panel
// UI_export_data          *ui_export_data = NULL;

			// the "Consolidate Plan File(s)" panel:
UI_integrate		*ui_integrate = NULL;


// EXTERNS:
			// in UI_openfsd.C:
extern stringtlist&	FileNamesWithInstancesAndOrNonLayoutDirectives();

			// in UI_motif_widget.C:
extern resource_data    ResourceData;

			// in main.C:
#ifdef GUI
XEvent*			get_event();
extern XtAppContext	Context;
#endif
extern motif_widget*	MW;

			// in UI_exec.C:
extern UI_exec*		UI_subsystem;

			// in grammar_intfc.C:
extern Cstring&		last_native_file();
// extern Cstring		normalizeString (const Cstring&, char c);
extern void		removeQuotes(Cstring& s);

// The following global method makes it possible for the user to close
// a window using the standard menu at the top left of the window:
extern void AllowWindowClosingFromMotifMenu(Widget, callback_proc, void*);


// X-argument Globals (re-use again and again):
static int n;
#ifdef GUI
static Arg args[20];
#else
static void *args[20];
#endif

static unsigned long		theAutomaticDelay = 1000;

// to avoid importing the world into main():
void	set_automatic_mode(double theDelay) {
	theAutomaticDelay = (unsigned long) (1000 * theDelay);
	user_waiter::we_are_in_automatic_mode = 1; }

rd_popup::rd_popup(
		const Cstring &	name,			//will be name of top widget
		motif_widget	*parent_widget,	//parent of this widget
		const Cstring &	text_for_popup)	//text (multiline) to display
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget, NULL, 0,
			false),		//i.e. should NOT [be] manage[d]
	  data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()) {
#ifdef GUI
	if(GUI_Flag) {
	textMultiline = Cstring(text_for_popup);
	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, &data_for_window_closing);

	n = 0;
	XtSetArg(args[n], XmNtitle, "Resource Value"); n++;
	// doesn't work:
	// XtSetArg(args[n], XtNwindowGroup, XtWindow(MW->widget)); n++;
	XtSetValues(widget, args, n);

	//Create the top-level Form (child of DialogShell) and all of its children.
	//!!!FUTURE need better way to specify common resources (width, height,
	//  background, fontList, etc.) -- perhaps group together as defined STYLE
	//  (as in BX); OR, use fallback resources for default style (see unused
	//  fallbacks[] in UI_motif_widget.C); OR, create method with lots of args
	//  for widget type and common resources, plus "args, n" for any others.

	rdPopupForm = new motif_widget("rdPopupForm", this, form_position(1), false);
	//Set all related colors (top & bottom shadows, select, and foreground):
	//Set all other resources (AND override just-calculated foreground color):

	rdPopupLabel = new motif_widget("rdPopupLabel", xmLabelWidgetClass, rdPopupForm);
	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	XtSetArg(args[n], XmNwidth, 300); n++;
#ifdef HAVE_EUROPA
	/* The planner will add info about the five most "costly" */
	/*  culprits, so allow for a larger widget. */
	/*  --wedgingt@ptolemy.arc.nasa.gov 2002 Apr 29 */
	XtSetArg(args[n], XmNheight, 100); n++;
#else
	XtSetArg(args[n], XmNheight, 30); n++;
#endif
	XtSetValues(rdPopupLabel->widget, args, n);
	//too long by 6 char:"Exact Time and Displayed Value (Remodel May Change):"
	XtVaSetValues(rdPopupLabel->widget, XtVaTypedArg, XmNlabelString,
			XmRString, "Selected Time and Displayed Value:",
			strlen("Selected Time and Displayed Value:")+1, NULL);
	*rdPopupForm ^ 15 ^ *rdPopupLabel ^ endform;	//10 looks too short
	*rdPopupForm < 10 < *rdPopupLabel < 10 < *rdPopupForm < endform;

#ifdef HAVE_EUROPA
	/* As above. --wedgingt@ptolemy.arc.nasa.gov */
	rdPopupTextMultiline = new multi_line_text_widget(
		"rdPopupTextMultiline", rdPopupForm, 8 /* lines HARDCODED */,
		NONEDITABLE, true);
#else
	rdPopupTextMultiline = new multi_line_text_widget(
		"rdPopupTextMultiline", rdPopupForm, 3 /* lines HARDCODED */,
		NONEDITABLE, true);
#endif
	XtVaSetValues(rdPopupTextMultiline->widget, XmNbackground, (Pixel) ResourceData.grey_color, NULL);
	n = 0;
	XtSetArg(args[n], XmNwidth, 300); n++;
	XtSetArg(args[n], XmNtraversalOn, False); n++; //TAB group can't focus here
	XtSetArg(args[n], XmNvalue, text_for_popup()); n++;
	XtSetValues(rdPopupTextMultiline->widget, args, n);
	*rdPopupLabel ^ 5 ^ *rdPopupTextMultiline ^ endform; //0 is too crowded
	*rdPopupForm < 10 < *rdPopupTextMultiline < 10 < *rdPopupForm < endform;

	rdPopupSeparator = new motif_widget("rdPopupSeparator", xmSeparatorWidgetClass, rdPopupForm);
	n = 0;
	XtSetArg(args[n], XmNwidth, 300); n++;
	XtSetArg(args[n], XmNheight, 20); n++;
	XtSetValues(rdPopupSeparator->widget, args, n);
	//Can't seem to find way to NOT attach separator to text widget,so do it...
	*rdPopupTextMultiline ^ *rdPopupSeparator ^ endform;
	*rdPopupForm < 10 < *rdPopupSeparator < 10 < *rdPopupForm < endform;

	//Form fraction base of 39 is optimal for 1-button Form (APGEN BX usage)
	rdPopupButtonForm = new motif_widget("rdPopupButtonForm", rdPopupForm, form_position(39), true);
	n = 0;
	XtSetArg(args[n], XmNwidth, 300); n++;
	XtSetArg(args[n], XmNheight, 30); n++;
	XtSetValues(rdPopupButtonForm->widget, args, n);
	//Bracketing with add_property() is REQUIRED to allow a sub-Form to relate
	//  to a Form using the attachment/offset/position operators:
	rdPopupButtonForm->add_property(CHILD_OF_A_FORM);
	*rdPopupSeparator ^ *rdPopupButtonForm ^ endform;
	*rdPopupForm < 10 < *rdPopupButtonForm < 10 < *rdPopupForm < endform;
	rdPopupButtonForm->add_property(FORM_PARENT);

	//traversal off for eligible widgets above in TAB group,so this is default:
	rdPopupCancelButton = new motif_widget("rdPopupCancelButton", xmPushButtonWidgetClass, rdPopupButtonForm);
	*rdPopupButtonForm ^ *rdPopupCancelButton ^ *rdPopupButtonForm ^ endform;
	form_position(10) < *rdPopupCancelButton < form_position(29) < endform;
	*rdPopupCancelButton = Cstring("Dismiss");		// ??? looks goofy...
	rdPopupCancelButton->add_callback(rd_popup::CancelButtonCallback, XmNactivateCallback, this);

//  If had Delete button, would be similar to above, except put Cancel at 0-19
//    and Delete at 20-39 in the 39-fraction-base ButtonForm

	//Since cannot seem to attach Separator and ButtonForm to bottom of parent
	//  Form (so excess space appears above the separator, as in APGEN-BX-style
	//  dialogs to date), instead create DummyForm to absorb excess space at
	//  the bottom of parent Form (not ideal, but preserves ButtonForm shape).
	rdPopupDummyForm = new motif_widget("rdPopupDummyForm", rdPopupForm, form_position(1), true);
	n = 0;
	XtSetArg(args[n], XmNwidth, 300); n++;
	//Do NOT set height!!!
	XtSetValues(rdPopupDummyForm->widget, args, n);
	//Bracketing with add_property() is REQUIRED to allow a *TWO* sub-Forms to
	//  relate to a Form using the attachment/offset/position operators:
	rdPopupButtonForm->add_property(CHILD_OF_A_FORM);
	rdPopupDummyForm->add_property(CHILD_OF_A_FORM);
	*rdPopupButtonForm ^ *rdPopupDummyForm ^ 10 ^ *rdPopupForm ^ endform;
	*rdPopupForm < 10 < *rdPopupDummyForm < 10 < *rdPopupForm < endform;
	rdPopupButtonForm->add_property(FORM_PARENT);
	rdPopupDummyForm->add_property(FORM_PARENT);

	//  rdPopupForm->manage();	//NO! wait until popup is needed!!!
	}
#endif
	}

rd_popup::~rd_popup() {
	//PFM:  DO NOT do explicit object delete()s! since that would interfere
	//      with the destruction of Widgets via the Widget hierarchy!
	; }

void rd_popup::CancelButtonCallback(Widget, callback_stuff* clientData, void*) {
	rd_popup* obj0 = (rd_popup*) clientData->data;
	obj0->rdPopupForm->unmanage(); }



con_popup::con_popup(
		const Cstring&	name,			//will be name of top widget
		motif_widget	*parent_widget,		//parent of this widget
		const Cstring&	text_for_popup 		//text (multiline) to display
		)
	: motif_widget(	name,
			xmDialogShellWidgetClass,
			parent_widget,
			NULL, 0,
			false),		//i.e. should NOT [be] manage[d]
	  data_for_window_closing(
			NULL,
			CancelButtonCallback,
			NULL,
			get_this_widget()) {
#ifdef GUI
	if(GUI_Flag) {
	motif_widget	*scrwin;

	textScrolled = Cstring(text_for_popup);
	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, &data_for_window_closing);

	n = 0;
	XtSetArg(args[n], XmNtitle, "Constraint Violations"); n++;
	XtSetValues(widget, args, n);

	paned_window = new motif_widget(
		"con_popup_paned",
		xmPanedWindowWidgetClass,
		this,
		NULL, 0,
		false);

	//
	// Create the top-level Form (child of DialogShell) and all of its children.
	// !!!FUTURE need better way to specify common resources (width, height,
	//  background, fontList, etc.) -- perhaps group together as defined STYLE
	//  (as in BX); OR, use fallback resources for default style (see unused
	//  fallbacks[] in UI_motif_widget.C); OR, create method with lots of args
	//  for widget type and common resources, plus "args, n" for any others.
	//


	//
	// For Total Violations info (1 line MINIMUM, 3 lines MAXIMUM)
	//
	conPopupTextMultiline = new multi_line_text_widget(
		"conPopupTextMultiline", paned_window, 3 /* visible lines HARDCODED */,
		NONEDITABLE, true);
	n = 0;
	XtSetArg(args[n], XmNtraversalOn, FALSE); n++;
	XtSetValues(conPopupTextMultiline->widget, args, n);

	//
	// This assignment truncates the text...
	//
	*((motif_widget *) conPopupTextMultiline) = text_for_popup;

	conPopupForm = new motif_widget("conPopupForm", paned_window, form_position(1), TRUE);

	conPopupLabel = new motif_widget("conPopupLabel",
		xmLabelWidgetClass, conPopupForm);
	*conPopupLabel = "Information on Selected Violations:";
	*conPopupForm < 10 < *conPopupLabel < 10 < *conPopupForm < endform;

	//
	// For Selected Violations info (3 lines MINIMUM, no fixed MAXIMUM).  Note
	//  that, since this is a COMPOUND widget, the motif_widget really "wraps"
	//  the Text widget, so must use XtParent() to refer to the containing
	//  ScrolledWindow widget!
	//
	n = 0;
	XtSetArg(args[n], XmNrows, 15); n++;
	XtSetArg(args[n], XmNcolumns, 40); n++;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNeditable, FALSE); n++;
	XtSetArg(args[n], XmNtraversalOn, FALSE); n++;
	XtSetArg(args[n], XmNscrollHorizontal, FALSE); n++;
	XtSetArg(args[n], XmNwordWrap, TRUE); n++;
	conPopupTextScrolled = new motif_widget("conPopupTextScrolled",
		xmScrolledTextWidgetClass,	conPopupForm,
		args,				n,
		true);

	//
	// This assignment should make the _entire_ text visible:
	//
	*conPopupTextScrolled = text_for_popup;

	scrwin = new motif_widget(
		conPopupTextScrolled,
		XtParent(conPopupTextScrolled->widget),
		xmScrolledWindowWidgetClass);
	*conPopupForm < 10 < *scrwin < 10 < *conPopupForm < endform;

	*conPopupForm  ^ 10 ^ *conPopupLabel ^ 10 ^ *scrwin ^ *conPopupForm ^ endform;

	//
	// Don't understand this:
	//
	delete scrwin;
	scrwin = NULL;

#ifdef OBSOLETE
	moveToNextPreviousForm = new motif_widget(
		"conPopupNextPrevForm",
		paned_window,
		form_position(69),
		true);

	conPopupPrevButton = new motif_widget(
		"conPopupPrevButton",
		xmPushButtonWidgetClass,
		moveToNextPreviousForm);
	conPopupNextButton = new motif_widget(
		"conPopupNextButton",
		xmPushButtonWidgetClass,
		moveToNextPreviousForm);
	*moveToNextPreviousForm ^ *conPopupPrevButton ^ *moveToNextPreviousForm ^ endform;
	*moveToNextPreviousForm ^ *conPopupNextButton ^ *moveToNextPreviousForm ^ endform;
	form_position(10) < *conPopupPrevButton < form_position(29) < endform;
	form_position(40) < *conPopupNextButton < form_position(59) < endform;
	*conPopupPrevButton = Cstring("Previous");
	conPopupPrevButton->add_callback(
		con_popup::NextPrevButtonCallback,
		XmNactivateCallback,
		this);
	*conPopupNextButton = Cstring("Next");
	conPopupNextButton->add_callback(
		con_popup::NextPrevButtonCallback,
		XmNactivateCallback,
		this);
#endif /* OBSOLETE */

	//Form fraction base of 39 is optimal for 1-button Form (APGEN BX usage)
	conPopupButtonForm = new motif_widget(
		"conPopupButtonForm",
		paned_window,
		form_position(39),
		true);

	conPopupCancelButton = new motif_widget(
		"conPopupCancelButton",
		xmPushButtonWidgetClass,
		conPopupButtonForm);
	*conPopupButtonForm ^ *conPopupCancelButton ^ *conPopupButtonForm ^ endform;
	form_position(10) < *conPopupCancelButton < form_position(29) < endform;
	*conPopupCancelButton = Cstring("Dismiss");
	conPopupCancelButton->add_callback(
		con_popup::CancelButtonCallback,
		XmNactivateCallback,
		this);

#ifdef HAVE_MACOS
	conPopupButtonForm->fix_height(32, conPopupCancelButton->widget);
#else
	conPopupButtonForm->fix_height(conPopupCancelButton->widget);
#endif /* HAVE_MACOS */
	}
#endif /* GUI */
}

con_popup::~con_popup()
{
	//PFM:  DO NOT do explicit object delete()s! since that would interfere
	//      with the destruction of Widgets via the Widget hierarchy!
	;
}

void con_popup::set_centertime(CTime t) {
	centertime = t;
}

void con_popup::NextPrevButtonCallback(Widget, callback_stuff *client_data, void *) {
	con_popup* obj0 = (con_popup*) client_data->data;

}

void con_popup::CancelButtonCallback(Widget, callback_stuff* clientData, void*) {
	con_popup* obj0 = (con_popup*) clientData->data;

	obj0->paned_window->unmanage();
}

tol_popup::tol_popup(
	const Cstring&  name,                   //will be name of top widget
	motif_widget	*parent_widget,          //parent of this widget
	const Cstring&  Txt //text (multiline) to display
	)
	: seq_review_popup(name, parent_widget, Txt) {
	motif_widget	*form3, *form4, *form2, *button, *label, *form5;
	Cstring		one_line, rest_of_the_file;
	blist		some_props(compare_function(compare_bstringnodes, false));

	form3 = new  motif_widget("form3", paned_window, form_position(2), true);
	label = new motif_widget("Choose zero or more filters: ", xmLabelWidgetClass, form3);
	// multiple selection allowed:
	some_props << new bstringnode(SCROLLBAR_DISPLAY_POLICY_STATIC);
	filters = new scrolled_list_widget("filters", form3, 5, some_props, true);
	*form3 ^ *label ^ *filters ^ *form3 ^ endform;
	*form3 < *label < *form3 < endform;
	*form3 < *filters < *form3 < endform;

	form4 = new  motif_widget("form4", paned_window, form_position(2), true);
	label = new motif_widget("Choose one format: ", xmLabelWidgetClass, form4);
	// multiple selection NOT allowed:
	formats = new scrolled_list_widget("formats", form4, 5, true);
	*form4 ^ *label ^ *formats ^ *form4 ^ endform;
	*form4 < *label < *form4 < endform;
	*form4 < *formats < *form4 < endform;

	form5 = new motif_widget("form5", paned_window, form_position(2), true);
	label = new motif_widget("File Name: ", xmLabelWidgetClass, form5);
	fileNameWidget = new single_line_text_widget("FileNameWidget", form5, EDITABLE, true);
	*form5 < *label < *fileNameWidget < *form5 < endform;
	*form5 ^ *label ^ *form5 ^ endform;
#ifdef HAVE_MACOS
	form5->fix_height(32, fileNameWidget->widget);
#else
	form5->fix_height(fileNameWidget->widget);
#endif /* HAVE_MACOS */
	*fileNameWidget = "apgen.tol";

	form2 = new motif_widget("tol_form2", paned_window, form_position(5), true);
	button = new motif_widget("OK", xmPushButtonWidgetClass, form2);
	*form2 ^ *button ^ *form2 ^ endform;
	form_position(1) < *button < form_position(2) < endform;
	button->add_callback(OKButtonCallback, XmNactivateCallback, this);
	button = new motif_widget("Cancel", xmPushButtonWidgetClass, form2);
	*form2 ^ *button ^ *form2 ^ endform;
	form_position(3) < *button < form_position(4) < endform;
	button->add_callback(CancelButtonCallback, XmNactivateCallback, this);
#ifdef HAVE_MACOS
	form2->fix_height(32, button->widget);
#else
	form2->fix_height(button->widget);
#endif /* HAVE_MACOS */

	/* NOTE: conv_file is the result of reading the seq_review configuration file into seq_review;
	 * 	 the seq_review_popup constructor is where the conversion took place (assuming seq_review was available). */
	rest_of_the_file = conv_file;
	/* NOTE: formats and strips appropriate for the TOL have been extracted by seq_review from its own
	 *	 configuration file. Each line in the conv_file represents wither a tol format or a tol strip.
	 *	 The seq_review name for the strip or format comes first (no embedded blanks), then a blank space,
	 *	 then a comment. The comment is what is displayed to the apgen user; the seq_review name is what
	 *	 will be used to tell seq_review what should be done to the tol. Note that thanks to the format of
	 *	 the configuration file, there are NO EMBEDDED NEWLINES in the comment; any newlines have been escaped
	 *	 as \n. This is ugly in case there WERE any newlines in the original description text, but guarantees
	 *	 that the present mechanism AND the Motif item selection mechanisms both work without a hitch. */
	*formats << Cstring("Raw Format");
	format_list << new String_node("DEFAULT");
	while((! hopeless) && (rest_of_the_file & "\n")) {
		one_line = rest_of_the_file / "\n";
		rest_of_the_file = "\n" / rest_of_the_file;
		if(one_line & "TOL_STRIP") {
			*filters << (" " / one_line);
			filter_list << new String_node(one_line / " "); }
		else if(one_line & "TOL_FORMAT") {
			*formats << (" " / one_line);
			format_list << new String_node(one_line / " "); } }
	update_time_widgets(); }

tol_popup::~tol_popup()
	{}

void tol_popup::OKcallback() {
	WRITE_TOLrequest*	theRequest;
	Cstring			new_start = start_time->get_text();
	Cstring			new_end = end_time->get_text();
	Cstring			error_msg;
	Cstring			theOptionalFormat;
	stringslist		theStrips;

	if(!hopeless) {
		strintslist	sel;
		strint_node	*N;

		formats->get_selected_items(sel);

		if(sel.get_length()) {
			N = sel.first_node();

			// Rationale: what is displayed is the DESCRIPTION of the format; the filter_list contains
			// the ACTUAL NAME of the format to be applied. Ditto for strips.
			// if(N->get_key() != "Raw Format")

			// NOTE: we DO INCLUDE the raw format; we can only read the TOL action request command
			//	 text if the format is present.
			theOptionalFormat = format_list[N->payload - 1]->get_key(); }
		else
			theOptionalFormat = "DEFAULT";
		filters->get_selected_items(sel);

		for(	N = sel.first_node();
			N;
			N = N->next_node())
			theStrips << new emptySymbol(Cstring("CATALOG STRIP ") + filter_list[N->payload - 1]->get_key()); }
	theRequest = new WRITE_TOLrequest(
			addQuotes(fileNameWidget->get_text()),
			new_start, new_end,
			theStrips, theOptionalFormat);
	theRequest->process();
	delete theRequest;
	paned_window->unmanage();
}

void seq_review_popup::OKButtonCallback(Widget, callback_stuff * client_data, void *) {
	seq_review_popup	*obj = (seq_review_popup *) client_data->data;

	obj->OKcallback(); }

seq_review_popup::seq_review_popup(
	const Cstring	&name,                   //will be name of top widget
	motif_widget	*parent_widget,          //parent of this widget
	const Cstring	&  			  //text (multiline) to display
	)
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget, NULL, 0,
			false),		//i.e. should NOT [be] manage[d]
	data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()),
	hopeless(0),
	top_widget(NULL),
	S(CTime(0, 0, false)),
	E(CTime(0, 0, false)) {
#ifdef GUI
	if(GUI_Flag) {
	int		method = 0;
	Cstring		error_msg;
	motif_widget	*label, *label2;
	stringslist	dummy_list;

	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, & data_for_window_closing);

	paned_window = new motif_widget("sr_paned", xmPanedWindowWidgetClass, this, NULL, 0, false);

	if(fileReader::theFileReader().read_file_from_seq_review("seq_review.cfg", conv_file, error_msg, "", dummy_list,
		"catalog script convert_to_apgen_format\n") != apgen::RETURN_STATUS::SUCCESS) {
		top_widget = new multi_line_text_widget("top", paned_window, 4, NONEDITABLE, true);
		XtVaSetValues(top_widget->widget, XmNscrollVertical, TRUE, NULL);
		top_widget->add_property(WRAP_TEXT);
		*top_widget = error_msg;
		hopeless = 1; }
	else {
		top_widget = new motif_widget("Enter start and end times:", xmLabelWidgetClass, paned_window); }
	top_form = new  motif_widget("top_form", paned_window, form_position(11), true);
	label = new motif_widget("Start time: ", xmLabelWidgetClass, top_form);
	start_time = new single_line_text_widget("start_time", top_form, EDITABLE, true);
	label2 = new motif_widget("End time: ", xmLabelWidgetClass, top_form);
	end_time = new single_line_text_widget("end_time", top_form, EDITABLE, true);
	*top_form < *label < *start_time < *top_form < endform;
	*top_form < *label2 < *end_time < *top_form < endform;
	*top_form ^ *start_time ^ *end_time ^ *top_form ^ endform;
	*top_form ^ 5 ^ *label ^ 5 ^ *label2 ^ *top_form ^ endform;
#ifdef HAVE_MACOS
	top_form->fix_height(74, top_form->widget);
#else
	top_form->fix_height(top_form->widget);
#endif /* HAVE_MACOS */
	}
#endif /* GUI */
	}

seq_review_popup::~seq_review_popup() { }

void seq_review_popup::update_time_widgets() {
	update_times(S, E);
	*start_time = S.to_string();
	*end_time = E.to_string(); }

void tol_popup::update_times(CTime & st, CTime & en) {
	model_intfc::FirstEvent(st);
	model_intfc::LastEvent(en); }

void seq_review_popup::CancelButtonCallback(Widget, callback_stuff *client_data, void *) {
	seq_review_popup	*obj = (seq_review_popup *) client_data->data;

	obj->paned_window->unmanage(); }

popup_list::popup_list(
	const Cstring&  name,				//will be name of top widget
	motif_widget	*parent_widget,			//parent of this widget
	callback_proc	proc_to_call,
	void		*data,
	long		apply_button_desired,
	int		hold_creation,				// defaults to 0
	void		(*user_waiter_timeoutproc)(void *),	// defaults to 0
	void		*user_waiter_data
	)
		: motif_widget(name, xmDialogShellWidgetClass, parent_widget, NULL, 0,
			false),		// i.e. should NOT [be] manage[d]
		data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()),
		ok_proc(proc_to_call),
		Parent(parent_widget),
		Data(data),
		apply_button((motif_widget*) apply_button_desired),
		theTypeOfPopup(0),
		listOfSubsystems(true),
		user_waiter(user_waiter_timeoutproc, user_waiter_data) {
#ifdef GUI
	if(GUI_Flag) {
	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, & data_for_window_closing);
	n = 0;
	XtSetArg(args[n], XmNautoUnmanage, FALSE); n++;
	paned_window = new motif_widget("paned",
		xmPanedWindowWidgetClass,	this,
		args,				n,
		false);
	if(!hold_creation) {
		Create(); }
	}
#endif
	}

void popup_list::Create(int) {
#ifdef GUI
    if(GUI_Flag) {
	motif_widget	*form2;

	scrolled_list = new  scrolled_list_widget("list", paned_window, 5, false);
	scrolled_list->add_callback(UpdateChoice, XmNsingleSelectionCallback, this);
	chosen_text = new single_line_text_widget("", paned_window, NONEDITABLE, true);
#ifdef HAVE_MACOS
	chosen_text->fix_height(32, chosen_text->widget);
#else
	chosen_text->fix_height(chosen_text->widget);
#endif /* HAVE_MACOS */
	form2 = new  motif_widget("form2", paned_window, form_position(19), true);
	OKbutton = new motif_widget("OK", xmPushButtonWidgetClass, form2);
	*form2 ^ *OKbutton ^ *form2 ^ endform;
	form_position(1) < *OKbutton < form_position(6) < endform;
	OKbutton->add_callback(OKButtonCallback, XmNactivateCallback, this);
	cancel_button = new motif_widget("Cancel", xmPushButtonWidgetClass, form2);
	*form2 ^ *cancel_button ^ *form2 ^ endform;
	form_position(13) < *cancel_button < form_position(18) < endform;
	cancel_button->add_callback(CancelButtonCallback, XmNactivateCallback, this);
	if(apply_button) {
		apply_button = new motif_widget("Apply", xmPushButtonWidgetClass, form2);
		*form2 ^ *apply_button ^ *form2 ^ endform;
		form_position(7) < *apply_button < form_position(12) < endform;
		apply_button->add_callback(ApplyButtonCallback, XmNactivateCallback, this); }
#ifdef HAVE_MACOS
	form2->fix_height(32, OKbutton->widget);
#else
	form2->fix_height(OKbutton->widget);
#endif /* HAVE_MACOS */
	XtManageChild(scrolled_list->widget); }
#endif
	}

popup_list::~popup_list() {
	listOfSubsystems.clear(); }

void popup_list::UpdateChoice(Widget, callback_stuff * client_data, void *) {
	popup_list*	PL = (popup_list *) client_data->data;
	strintslist	sel;
	strint_node*	N;

	PL->scrolled_list->get_selected_items(sel);
	if(sel.get_length() != 1) return;
	N = sel.first_node();
	*((motif_widget *) PL->chosen_text) = N->get_key(); }

void popup_list::CancelButtonCallback(Widget, callback_stuff * client_data, void *) {
	popup_list	*obj = (popup_list *) client_data->data;

	obj->user_has_made_a_choice = 1;
	obj->paned_window->unmanage(); }

void popup_list::OKButtonCallback(Widget, callback_stuff * client_data, void *) {
	popup_list	*obj = (popup_list *) client_data->data;

	obj->user_has_made_a_choice = 1;
	obj->paned_window->unmanage();
	obj->ok_proc(NULL, client_data, obj->Data); }

void popup_list::ApplyButtonCallback(Widget, callback_stuff * client_data, void *) {
	popup_list	*obj = (popup_list *) client_data->data;

	obj->user_has_made_a_choice = 1;
	obj->ok_proc(NULL, client_data, obj->Data); }

bool add_resource_popup::subsystems_are_out_of_sync() {
	tlistNode*		theTag;

	assert(theTypeOfPopup == 2);
	tlisttlist	theActualSubsystems(true);
	emptySymbol*	theRes;
	int		Nres = 0;

	compute_resource_subsystems(theActualSubsystems, Nres);
	if(listOfSubsystems.get_length() != theActualSubsystems.get_length()) {
		return true; }

	tlistslist::iterator		subs(listOfSubsystems);

	while((theTag = subs())) {
		tlistNode*	theMatch = theActualSubsystems.find(theTag->get_key());

		if(!theMatch) {
			return true; }
		if(theTag->payload.get_length() != theMatch->payload.get_length()) {
			return true; }
		stringslist::iterator	obj(theTag->payload);
		while((theRes = obj())) {
			emptySymbol* matchingRes = theMatch->payload.find(theRes->get_key());

			if(!matchingRes) {
				return true; } } }
	// we tried, but couldn't find any diffs
	return false; }

bool new_act_popup::subsystems_are_out_of_sync() {
	bool			chameleons_only = false, templates_only = false;
	tlistNode*		theTag;
	emptySymbol*		theAct;
	emptySymbol*		bstr;

	/* Perhaps unfortunately, the popup_list class supports at least 4 different
	 * popup panels with rather different characteristics and requirements. Here
	 * is a list of the popup types and their requirements:
	 *
	 * 	theTypeOfPopup = 0: simple popup, does not feature subsystems, no worry.
	 *
	 * 	theTypeOfPopup = 1: "create new activity" popup. Not very complicated,
	 * 		but we need to provide the user with a clear indication of whether
	 * 		an activity type is a chameleon or not.
	 *
	 * 	theTypeOfPopup = 2: "display resource". Not too complicated; obviously
	 * 		only concrete resources should be displayed.
	 * 	theTypeOfPopup = 3: "group activity/ies". This is the complex case. The
	 * 		panel features a radiobox containing 4 buttons:
	 *
	 *		"Select Existing Activity"
	 *		"Create New Activity"
	 *		"Create New Request"
	 *		"Create New Chameleon"
	 */
	if(!theTypeOfPopup) {
		// simple popup
		return false; }
	assert(theTypeOfPopup != 2);
	if(	theTypeOfPopup == 1
		|| (	(theTypeOfPopup == 3)
			&& (createNewActivity->get_text() == "SET"))
	  ) {
		// we are OK
		; }
	else if(theTypeOfPopup == 3
		&& (useExistingActivity->get_text() == "SET")) {
		if(	listOfSubsystems.get_length() == 1
			&& (	(theTag = listOfSubsystems.first_node())
			    	&& theTag->get_key() == "<Default>")
			&& (	theTag->payload.get_length() == 1
				&& (bstr = theTag->payload.first_node())
				&& bstr->get_key() == "generic")
		  ) {
			// OK
			return false; }
		else {
			return true; } }
	else if(theTypeOfPopup == 3
		&& (createNewChameleon->get_text() == "SET")) {
		chameleons_only = true; }
	else if(theTypeOfPopup == 3
		&& (createNewRequest->get_text() == "SET")) {
		templates_only = true; }
	else {
		// should never happen
		assert(0); }

	tlisttlist		theActualSubsystems;
	int			Nacts = 0;

	/* we don't care if there are no activity types - still need
	 * to check */
	compute_activity_subsystems(theActualSubsystems, Nacts, chameleons_only, templates_only);
	if(listOfSubsystems.get_length() != theActualSubsystems.get_length()) {
		return true; }

	tlistslist::iterator		subs(listOfSubsystems);

	while((theTag = subs())) {
		tlistNode*	theMatch = theActualSubsystems.find(theTag->get_key());

		if(!theMatch) {
			return true; }
		if(theTag->payload.get_length() != theMatch->payload.get_length()) {
			return true; }
		stringtlist::iterator	obj(theTag->payload);
		while((theAct = obj())) {
			emptySymbol* matchingAct = theMatch->payload.find(theAct->get_key());

			if(!matchingAct) {
				return true; } } }
	// we tried, but couldn't find any diffs
	return false; }

apgen::RETURN_STATUS new_act_popup::compute_activity_subsystems(
		tlisttlist&	list_to_fill,
		int&		Nacts,
		bool		chameleons_only,
		bool		templates_only) {
	tlistNode*	theTag;
	Behavior*	beh;

	Nacts = 0;
	list_to_fill.clear();
	list_to_fill << (theTag = new tlistNode("<Default>"));
	theTag->payload << new emptySymbol("generic");

	for(int k = 0; k < Behavior::ClassTypes().size(); k++) {
		beh = Behavior::ClassTypes()[k];
		if(beh->realm == "activity") {
			Cstring		subName("<Default>");
			if(beh->tasks[0]->prog) {
#ifdef PREMATURE
				// search the symbol table instead
				pEsys::Statement* inst = beh->tasks[0]->prog->find("subsystem");
				if(inst)  {
					subName = inst->get_rhs();
					removeQuotes(subName);
				}
#endif /* PREMATURE */
			}
			if(!(theTag = list_to_fill.find(subName))) {
				list_to_fill << (theTag = new tlistNode(subName));
			}
			Nacts++;
			if(!theTag->payload.find(beh->name)) {
				theTag->payload << new emptySymbol(beh->name);
			}
		}
	}
	if(!Nacts) {
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS add_resource_popup::compute_resource_subsystems(
		tlistslist&	list_to_fill,
		int&		Nres) {
	stringslist		resourceList;
	stringslist::iterator	iterator(resourceList);
	emptySymbol*		resourceNode;
	Rsource*		resource;
	tlistNode*		theTag;

	Nres = 0;
	list_to_fill.clear();
	list_to_fill << new tlistNode("<Default>");
	Rsource::get_all_resource_names(resourceList);
	while((resourceNode = iterator())) {
		resource = eval_intfc::FindResource(resourceNode->get_key());

		if (!resource->is_hidden()) {
			Cstring		subName("<Default>");

			if(resource->Object->defines_property("subsystem")) {
				subName = (*resource->Object)["subsystem"].get_string();
			}
			if(!(theTag = list_to_fill.find(subName))) {
				list_to_fill << (theTag = new tlistNode(subName));
			}
			Nres++;
			theTag->payload << new emptySymbol(resource->name);
		}
	}
	if(!Nres) {
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS add_resource_popup::fill_in_subsystems() {
	int		Nres = 0;
	// case: resources
	assert(theTypeOfPopup == 2);
	if(compute_resource_subsystems(listOfSubsystems, Nres) != apgen::RETURN_STATUS::SUCCESS) {
			return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS new_act_popup::fill_in_subsystems() {
	compare_function cmp1(compare_bstringnodes, false);
	bool	chameleons_only = false, templates_only = false;

	/* Perhaps unfortunately, the popup_list class supports at least 4 different
	 * popup panels with rather different characteristics and requirements. Here
	 * is a list of the popup types and their requirements:
	 *
	 * 	theTypeOfPopup = 0: simple popup, does not feature subsystems, no worry.
	 *
	 * 	theTypeOfPopup = 1: "create new activity" popup. Not very complicated,
	 * 		but we need to provide the user with a clear indication of whether
	 * 		an activity type is a chameleon or not.
	 *
	 * 	theTypeOfPopup = 2: "display resource". Not too complicated; obviously
	 * 		only concrete resources should be displayed.
	 *
	 * 	theTypeOfPopup = 3: "group activity/ies". This is the complex case. The
	 * 		panel features a radiobox containing 4 buttons:
	 *
	 *		"Select Existing Activity"
	 *		"Create New Activity"
	 *		"Create New Request"
	 *		"Create New Chameleon"
	 */
	if(!theTypeOfPopup) {
		// simple popup
		return apgen::RETURN_STATUS::SUCCESS; }

	/* case 1: regular activity types: either simple "create new act" or "Group act" with new
	 * 	   activity creation. Note that we have the following possibilities (buttons in
	 * 	   the radiobox):
	 * 	   	createNewActivity
	 * 	   	useExistingActivity
	 *		createNewRequest
	 *		createNewChameleon
	 */
	if(theTypeOfPopup == 1) {
		int		Nacts = 0;
		if(compute_activity_subsystems(listOfSubsystems, Nacts, chameleons_only, templates_only) != apgen::RETURN_STATUS::SUCCESS) {
			return apgen::RETURN_STATUS::FAIL; } }

	// case 2: resources
	assert(theTypeOfPopup != 2);
	// case 3: activity templates/chameleons
	if(theTypeOfPopup == 3) {
		int		Nacts = 0;
		tlistNode*	theDefaultTag;


		if(useExistingActivity->get_text() == "SET") {
			*user_instructions = "Select an existing activity, then hit OK"; }
		else {
			*user_instructions =
				"Select a Subsystem from the list below,\nthen a type from the second list."; }

		if(useExistingActivity->get_text() == "SET") {
			// we should change the 'instructions' so they are not confusing in this case...
			// OK: done.
			scrolled_list->set_sensitive(false);
			scrolled_list2->set_sensitive(false);
			listOfSubsystems.clear();
			listOfSubsystems << (theDefaultTag = new tlistNode("<Default>"));
			theDefaultTag->payload << new emptySymbol("generic");

			// To do this right, use Adam's trick of registering event handlers with DS_lndraw.

			// the OK button will be sensitized after you select something (see Event method below):
			OKbutton->set_sensitive(false);
			chosen_text->set_sensitive(false);
			return apgen::RETURN_STATUS::SUCCESS; }
		else if(createNewActivity->get_text() == "SET") {
			scrolled_list->set_sensitive(true);
			scrolled_list2->set_sensitive(true);
			OKbutton->set_sensitive(true);
			chosen_text->set_sensitive(true);
			compute_activity_subsystems(listOfSubsystems, Nacts, chameleons_only, templates_only); }
		else if(createNewChameleon->get_text() == "SET") {
			scrolled_list->set_sensitive(true);
			scrolled_list2->set_sensitive(true);
			OKbutton->set_sensitive(true);
			chosen_text->set_sensitive(true);
			*(motif_widget *)chosen_text = "None";
			chameleons_only = true;
			compute_activity_subsystems(listOfSubsystems, Nacts, chameleons_only, templates_only); }
		else if(createNewRequest->get_text() == "SET") {
			scrolled_list->set_sensitive(true);
			scrolled_list2->set_sensitive(true);
			OKbutton->set_sensitive(true);
			chosen_text->set_sensitive(true);
			*(motif_widget *) chosen_text = "None";
			templates_only = true;
			compute_activity_subsystems(listOfSubsystems, Nacts, chameleons_only, templates_only); }
		if(!Nacts) {
			return apgen::RETURN_STATUS::FAIL; } }
	return apgen::RETURN_STATUS::SUCCESS; }

#ifdef GUI
void	user_waiter_proc(XtPointer client_data, XtIntervalId * /* id */) {
	user_waiter	*me = (user_waiter *) client_data;

	user_waiter::user_has_made_a_choice = 1;
	if(me->timeout_proc) {
		me->timeout_proc(me->theData); }
	// change for 7.4.7:
	// UI_mainwindow::send_a_dummy_event_to_unblock_XtAppNextEvent();

	// NOTE: we may have to invent some new way of doing this...
	}
#endif

void user_waiter::wait_for_user_action(user_waiter *me) {
#ifdef GUI
	if(GUI_Flag) {
	XEvent		*event;
 
	user_has_made_a_choice = 0;
	if(me && me->i_am_a_pause_popup() && we_are_in_automatic_mode) {
		if(me->this_is_the_last_time()) {
			we_are_in_automatic_mode = 0; }
		else {
			XtAppAddTimeOut(Context, theAutomaticDelay, user_waiter_proc, me); } }
	}
#endif
	}

void popup_list::updateMainList(stringtlist& L, bool order_the_list) {
	emptySymbol*		N;
	stringslist::iterator	l(L);

	if(order_the_list) {
		L.order(); }
	((scrolled_list_widget *) scrolled_list)->clear();
	while((N = l.next())) {
		*((scrolled_list_widget *) scrolled_list) << N->get_key();
		if(N == L.first_node())
			*((motif_widget *) chosen_text) = N->get_key(); } }

void pause_popup::Create(int) {
	if(GUI_Flag) {
	motif_widget	*label, *form2;

	n = 0;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNscrollHorizontal, False); n++;
	XtSetArg(args[n], XmNscrollVertical, True); n++;
	XtSetArg(args[n], XmNwordWrap, True); n++;
	XtSetArg(args[n], XmNeditable, False); n++;
	XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
	XtSetArg(args[n], XmNheight, 180); n++;
	XtSetArg(args[n], XmNwidth, 450); n++;
	multi = new motif_widget("multi", xmScrolledTextWidgetClass, paned_window, args, n, true);

	label = new motif_widget("Press Continue to execute command below\nPress Cancel to abort the script",
			xmLabelWidgetClass, paned_window);
#ifdef HAVE_MACOS
	label->fix_height(32, label->widget);
#else
	label->fix_height(label->widget);
#endif /* HAVE_MACOS */

	chosen_text = new single_line_text_widget("", paned_window, NONEDITABLE, true);
#ifdef HAVE_MACOS
	chosen_text->fix_height(32, chosen_text->widget);
#else
	chosen_text->fix_height(chosen_text->widget);
#endif /* HAVE_MACOS */

	form2 = new  motif_widget("form2", paned_window, form_position(19), true);
	cancel_button = new motif_widget("Cancel", xmPushButtonWidgetClass, form2);
	OKbutton = new motif_widget("Continue", xmPushButtonWidgetClass, form2);
	*form2 ^ *cancel_button ^ *form2 ^ endform;
	*form2 ^ *OKbutton ^ *form2 ^ endform;
	form_position(1) < *OKbutton < form_position(6) < endform;
	form_position(13) < *cancel_button < form_position(18) < endform;
	// cancel_button->add_callback(CancelButtonCallback, XmNactivateCallback, this);
	OKbutton->add_callback(OKButtonCallback, XmNactivateCallback, this);
	// Note: callback will have to tell which button was pressed
	cancel_button->add_callback(OKButtonCallback, XmNactivateCallback, this);
#ifdef HAVE_MACOS
	form2->fix_height(32, OKbutton->widget);
#else
	form2->fix_height(OKbutton->widget);
#endif /* HAVE_MACOS */
	} }

sasf_popup::sasf_popup(const Cstring & name, motif_widget *parent_widget)
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget, NULL, 0, false),
		data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()),
		top_widget(NULL),
		files(NULL) {
	if(GUI_Flag) {
	motif_widget		*form5, *form2, *form3, *button, *label,
				*frame1, *label1, *label2, *label3, *radiobox, *separator;
	blist			some_properties(compare_function(compare_bstringnodes, false));

	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, &data_for_window_closing);

	paned_window = new motif_widget("sasf_paned", xmPanedWindowWidgetClass, this, NULL, 0, false);

	/*
	 * We create 5 panes, each with its own form.
	 */

	top_form = new motif_widget("top_form", paned_window, form_position(2), true);
		label = new motif_widget("Start time: ", xmLabelWidgetClass, top_form);
		start_time = new single_line_text_widget("start_time", top_form, EDITABLE, true);
		label2 = new motif_widget("End time: ", xmLabelWidgetClass, top_form);
		end_time = new single_line_text_widget("end_time", top_form, EDITABLE, true);
		*top_form ^ *start_time ^ *end_time ^ *top_form ^ endform ;
		*top_form ^ 5 ^ *label ^ 15 ^ *label2 ^ 5 ^ endform ;
		*label < *start_time < *top_form < endform;
		*label2 < *end_time < *top_form < endform ;
#ifdef HAVE_MACOS
		top_form->fix_height(70, top_form->widget);
#else
		top_form->fix_height( top_form->widget ) ;
#endif /* HAVE_MACOS */

	form2 = new  motif_widget( "form2" , paned_window , form_position( 11 ) , true ) ;
		frame1 = new motif_widget( "frame1" , xmFrameWidgetClass , form2 ) ;
		radiobox = new motif_widget( "Radiobox" , xmRadioBoxWidgetClass , frame1 ) ;
		// to be completed...
		includeExactEndTime = new motif_widget( "Include Acts. that start at End Time" ,
			xmToggleButtonWidgetClass ,	radiobox ) ;
		excludeExactEndTime = new motif_widget( "Exclude Acts. that start at End Time" ,
			xmToggleButtonWidgetClass ,	radiobox ) ;
		*excludeExactEndTime = 1 ;
		*form2 ^ *frame1 ^ *form2 ^ endform;
#ifdef HAVE_MACOS
		form2->fix_height(64, top_form->widget);
#else
		form2->fix_height( top_form->widget ) ;
#endif /* HAVE_MACOS */

	form3 = new  motif_widget( "form3" , paned_window , form_position( 11 ) , true ) ;
		label = new motif_widget( "Choose output file(s) by selecting\n"
			"item and editing info below:\n"
			"(* means file already exists and will be overwritten)" , xmLabelWidgetClass , form3 ) ;

		separator = new motif_widget( "sep1" , xmSeparatorWidgetClass , form3 ) ; 

		// multiple selection NOT allowed:
		some_properties << new bstringnode( SELECTION_POLICY_SINGLE_SELECT ) ;
		some_properties << new bstringnode( SCROLLBAR_DISPLAY_POLICY_STATIC ) ;
		some_properties << new bstringnode( SCROLLBAR_PLACEMENT_BOTTOM_LEFT ) ;
		files = new scrolled_list_widget( "files" , form3 , 5 , some_properties , true ) ;

		n = 0 ;
		XtSetArg( args[n] , XmNselectionPolicy , XmSINGLE_SELECT ) ; n++ ;
		actual_files = new slave_list_widget( "Slave" , form3 , files , args , n ) ;
		whether_selected = new slave_list_widget( "Slave2" , form3 , files , args , n ) ;

		label1 = new motif_widget( "File Name Attr." , xmLabelWidgetClass , form3 ) ; 
		label2 = new motif_widget( "Actual File Name" , xmLabelWidgetClass , form3 ) ; 
		label3 = new motif_widget( "Sel." , xmLabelWidgetClass , form3 ) ; 

		*form3 < *label < *form3 < endform ;
		*form3 < *separator < *form3 < endform ;
		*form3 < *label1 < form_position( 4 ) < *label2 < form_position( 9 ) < *label3 < *form3 < endform ;
		*form3 ^ *label ^ *separator ^ *label1 ^ *files ^ *form3 ^ endform ;
		*separator ^ *label2 ^ *files ;
		*separator ^ *label3 ^ *files ;
		*label2 ^ *actual_files ^ *form3 ^ endform ;
		*label2 ^ *whether_selected ^ *form3 ^ endform ;
		form_position( 0 ) < *files < form_position( 4 ) < *actual_files < form_position( 9 )
			< *whether_selected < form_position( 11 ) < endform ;

		files->add_callback( SASFcallback , XmNsingleSelectionCallback , this ) ;
		actual_files->add_callback( SASFcallback , XmNsingleSelectionCallback , this ) ;
		whether_selected->add_callback( SASFcallback , XmNsingleSelectionCallback , this ) ;

	form5 = new motif_widget("form5", paned_window, form_position(11), true);
		label = new motif_widget( "Actual File Name: " , xmLabelWidgetClass , form5 ) ;
		fileNameWidget = new single_line_text_widget("FileNameWidget", form5, EDITABLE, true);
		// do this FIRST!!
		*fileNameWidget = "apgen.sasf" ;
		fileNameWidget->add_callback( SASFcallback , XmNvalueChangedCallback , this ) ;
		selected_button = new motif_widget( "toggleButton" , xmToggleButtonWidgetClass , form5 ) ;
		selected_button->set_label( "Select" ) ;
		*selected_button = "0" ; // do this BEFORE callback is attached... safer!
		form_position( 0 ) < *label
			< form_position( 4 ) < *fileNameWidget
			< form_position( 9 ) < *selected_button
			< *form5 < endform ;
		*form5 ^ 3 ^ *label ^ endform ;
		*form5 ^ *fileNameWidget ^ *form5 ^ endform ;
#ifdef HAVE_MACOS
		form5->fix_height(32, form5->widget);
#else
		form5->fix_height(form5->widget);
#endif /* HAVE_MACOS */
		fileNameWidget->set_sensitive(FALSE);
		selected_button->set_sensitive(FALSE);
		selected_button->add_callback(SASFcallback, XmNvalueChangedCallback, this);

	motif_widget*	sasf_form2;
	motif_widget*	cancelbutton;
	sasf_form2 = new  motif_widget("sasf_form2", paned_window, form_position(5), true);
		cancelbutton = new motif_widget("Cancel", xmPushButtonWidgetClass, sasf_form2);
		*sasf_form2 ^ *cancelbutton ^ *sasf_form2 ^ endform;
		form_position(3) < *cancelbutton < form_position(4) < endform;
		cancelbutton->add_callback(CancelButtonCallback, XmNactivateCallback, this);
		button = new motif_widget("OK", xmPushButtonWidgetClass, sasf_form2);
		*sasf_form2 ^ *button ^ *sasf_form2 ^ endform;
		form_position(1) < *button < form_position(2) < endform;
		button->add_callback(OKButtonCallback, XmNactivateCallback, this);
#ifdef HAVE_MACOS
		sasf_form2->fix_height(32, button->widget);
#else
		sasf_form2->fix_height(button->widget);
#endif /* HAVE_MACOS */

	// the seq_review popup already did this, but in my opinion it may have chosen its own method,
	// even though it's virtual:
	update_time_widgets() ;
	}
}

void	sasf_popup::SASFcallback(Widget, callback_stuff *client_data, void *call_data) {
	if(GUI_Flag) {
	sasf_popup	*object = ( sasf_popup * ) client_data->data ;
	motif_widget	*callback_source = client_data->parent ;

	if(	   callback_source == object->files
		|| callback_source == object->actual_files
		|| callback_source == object->whether_selected ) {
		XmListCallbackStruct	*cbs = (XmListCallbackStruct *) call_data;
		int			item_pos = cbs->item_position;
		ref_node		*FILE_REF;
		ref_node		*SEL_REF;

		if(	callback_source == object->actual_files
			|| callback_source == object->whether_selected) {
			item_pos += (object->actual_files->index_of_item_at_the_top - 1); }
		object->thePositionOfTheSelectedItem = item_pos;
		FILE_REF = (ref_node *) object->actual_files->theItems[item_pos - 1];
		SEL_REF = (ref_node *) object->whether_selected->theItems[item_pos - 1];
		if(FILE_REF && SEL_REF) {
			Cstring		theFileNameToUse(FILE_REF->get_key());

			if(theFileNameToUse & "* ")
				theFileNameToUse = "* " / theFileNameToUse;
			object->fileNameWidget->set_sensitive(TRUE);
			object->selected_button->set_sensitive( TRUE ) ;
			*object->fileNameWidget = theFileNameToUse ;
			if( SEL_REF->get_key() == "X" )
				*object->selected_button = "1" ;
			else
				*object->selected_button = "0" ; }  }
	else if( callback_source == object->selected_button ) {
		ref_node		*SEL_REF = ( ref_node * ) object->whether_selected->theItems[
						object->thePositionOfTheSelectedItem - 1] ;

		if( SEL_REF ) {
			if( callback_source->get_text() == "SET" )
				SEL_REF->Substring = "X" ;
			else
				SEL_REF->Substring = " " ;
			object->whether_selected->redisplay_all() ; } }
	else if( callback_source == object->fileNameWidget ) {

		ref_node		*FILE_REF = ( ref_node * ) object->actual_files->theItems[
						object->thePositionOfTheSelectedItem - 1] ;

		if( FILE_REF ) {
			struct stat		FileStats ;
			Cstring			theFileName = object->fileNameWidget->get_text() ;

			if ( stat( *theFileName , & FileStats ) )			// file does NOT exist
				FILE_REF->Substring = theFileName ;
			else
				FILE_REF->Substring = Cstring( "* " ) + theFileName ;	// file DOES exist
			object->actual_files->redisplay_all() ; } } } }

void sasf_popup::update_times(CTime &theStart, CTime &theEnd) {
	TypedValue& theStartTime = globalData::get_symbol("SASF_BEGIN_TIME");
	TypedValue& theEndTime = globalData::get_symbol("SASF_CUTOFF_TIME");

	theStart = theStartTime.get_time_or_duration();
	theEnd = theEndTime.get_time_or_duration();
	if( theStart == CTime(0, 0, false) )
		model_intfc::FirstEvent( theStart ) ;
	if( theEnd == CTime(0, 0, false) )
		model_intfc::LastEvent( theEnd ) ;
}

sasf_popup::~sasf_popup() {}

void sasf_popup::update_time_widgets() {
	update_times(S, E);
	*start_time = S.to_string();
	*end_time = E.to_string(); }

void sasf_popup::OKButtonCallback(Widget, callback_stuff *client_data, void *) {
	sasf_popup	*obj = (sasf_popup *) client_data->data;

	obj->OKcallback(); }

void sasf_popup::CancelButtonCallback(Widget, callback_stuff *client_data, void *) {
	sasf_popup	*obj = (sasf_popup *) client_data->data;

	obj->paned_window->unmanage(); }

int sasf_popup::create_sasf_file_list(Cstring &errorsFound) {

	//
	// This list is compiled from the values of the "SASF file" attributes of
	// any (non-request) activities that have their "SASF" attribute set. By default,
	// an activity that has a value specified for its "SASF" attribute
	// but not for its "SASF file" attribute is associated with the
	// "default sasf file name". This default used to be "apgen.sasf",
	// but now (Dec. '98) it is user-definable.
	//
  
	// Clean up any previous file info
	actual_files->clear() ;
	files->clear() ;
	whether_selected->clear() ;

	StringVect fileNames;
	if(!IO_writer::create_sasf_file_list(&fileNames, errorsFound)) {
		fileNames.push_back(*errorsFound);
	}
  
	//Second, update the displayed list using the new internal list:
  
	StringVect::iterator currentFileName = fileNames.begin();
	Cstring sasf_file_name;
	struct stat FileStats;

	while(currentFileName != fileNames.end()) {
		if(*currentFileName == "<Default>") {
			sasf_file_name = "apgen.sasf";
		} else {
			sasf_file_name = currentFileName->c_str();
		}
		if(!stat(*sasf_file_name, &FileStats)) { //file exists, so set "overwrite" flag
			string	temp("* ");

			temp += *sasf_file_name;
			sasf_file_name = temp;
		}
		*actual_files << sasf_file_name;
		*files << currentFileName->c_str();
		// always use symbolic name (from sasf file attribute value, if any) in first col.
		*whether_selected << "X" ;// default is "selected"
		currentFileName++;
	}

	return fileNames.size();
}

void sasf_popup::OKcallback() {
	Cstring			theMessage;
	Cstring			new_start = start_time->get_text();
	Cstring			new_end = end_time->get_text();
	listtlist		TrulyDistinctFileNames;
	ref_node		*N;
	listNode		*theListTag, *theNewListTag;
	int			i;
	WRITE_SASFrequest	*theRequest;
	int			inclusionFlag;

	if( includeExactEndTime->get_text() == "SET" ) {
		inclusionFlag = 1; }
	else {
		inclusionFlag = 0 ; }

	// Second, consolidate the lists of activities associated with DISTINCT ACTUAL FILE NAMES:
	for( i = 1 , N = ( ref_node * ) files->theItems.first_node() ;
	     N ;
	     N = ( ref_node * ) N->next_node() , i++ ) {
		Cstring		theTrueFileName( actual_files->theItems[i - 1]->get_key() ) ;
		Cstring		theSelection( whether_selected->theItems[i - 1]->get_key() ) ;
 
		if(theSelection == "X") {
			if(theTrueFileName & "* ")
				theTrueFileName = "* " / theTrueFileName ;
			if(!(theNewListTag = TrulyDistinctFileNames.find(theTrueFileName))) {
				theNewListTag = new listNode(theTrueFileName);
				TrulyDistinctFileNames << theNewListTag; }
			// OLD STYLE:
			// theNewListTag->get_list() << theListTag->get_list() ; empty rhs into lhs
			// NEW STYLE:
			theNewListTag->payload << new emptySymbol(N->get_key());
		}
	}

	if(TrulyDistinctFileNames.get_length()) {
		for( theListTag = TrulyDistinctFileNames.first_node();
		     theListTag;
		     theListTag = theListTag->next_node(), i++) {
			theRequest = new WRITE_SASFrequest(theListTag->payload, new_start, new_end,
					theListTag->get_key(), inclusionFlag);
			theRequest->process();
			delete theRequest;
		}
	}
	paned_window->unmanage();
}

void pause_popup::auto_callback(void *V) {
	pause_popup	*obj = (pause_popup *)V;

	obj->paned_window->unmanage(); }


void explanation_popup::Create(int) {}

void explanation_popup::UpdateExplanation( Widget, callback_stuff *client_data, void *call_data) {}

void explanation_popup::CancelExplanation( Widget, callback_stuff *client_data, void *) {}

int pause_popup::i_am_a_pause_popup() {
	return 1; }

int pause_popup::this_is_the_last_time() {
	return chosen_text->get_text() == ""; }

popup_scrolled_text::popup_scrolled_text(
	const Cstring&  name,			//will be name of top widget
	motif_widget*	parent_widget,		//parent of this widget
	callback_proc	proc_to_call,
	void *		data
	)
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget, NULL, 0,
			FALSE),		//i.e. should NOT [be] manage[d]
	data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()),
	ok_proc(proc_to_call),
	Parent(parent_widget),
	Data(data)
	{}

// this used to be in the constructor. However, the problem is that virtual method add_more_panels()
// always refer to the base class inside the base class constructor, which is a real pain... hence,
// we use an explicit "create" method, which does what we want, i. e., use the appropriate add_more_panels() method.
void popup_scrolled_text::create() {
#ifdef GUI
	if(GUI_Flag) {
	motif_widget	*form1, *form2, *multi;

	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, & data_for_window_closing);

	n = 0;
	XtSetArg(args[n], XmNautoUnmanage, FALSE); n++;
	paned_window = new motif_widget("paned",
		xmPanedWindowWidgetClass,	this,
		args,				n,
		FALSE);

		// try this new style:
		n = 0;
		// XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
		multi = new motif_widget("multi text",
			xmScrolledWindowWidgetClass,	paned_window,
			args,				n,
			TRUE);

		n = 0;
		XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
		XtSetArg(args[n], XmNscrollHorizontal, False); n++;
		XtSetArg(args[n], XmNscrollVertical, True); n++;
		XtSetArg(args[n], XmNwordWrap, True); n++;
		XtSetArg(args[n], XmNeditable, False); n++;
		XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
		XtSetArg(args[n], XmNheight, 500); n++;
		XtSetArg(args[n], XmNwidth, 500); n++;
		chosen_text = new motif_widget("multi", xmTextWidgetClass, multi, args, n, TRUE);

		// (instead of the old style:)
		// chosen_text = new scrolled_text_widget("multi text", paned_window, 5 /* visible lines HARDCODED */,
		//	EDITABLE, TRUE);

	// allows derived classes to construct more complex object:
	add_more_panels();
	form2 = new  motif_widget("form2", paned_window, form_position(5), true);
	ok_button = new motif_widget("OK", xmPushButtonWidgetClass, form2);
	*form2 ^ *ok_button ^ *form2 ^ endform;
	form_position(1) < *ok_button < form_position(2) < endform;
	ok_button->add_callback(OKButtonCallback, XmNactivateCallback, this);
	cancel_button = new motif_widget("Cancel", xmPushButtonWidgetClass, form2);
	*form2 ^ *cancel_button ^ *form2 ^ endform;
	form_position(3) < *cancel_button < form_position(4) < endform;
	cancel_button->add_callback(CancelButtonCallback, XmNactivateCallback, this);
#ifdef HAVE_MACOS
	form2->fix_height(32, ok_button->widget);
#else
	form2->fix_height(ok_button->widget);
#endif /* HAVE_MACOS */
	}
#endif /* GUI */
	}

popup_scrolled_text::~popup_scrolled_text()
	{
	}

void popup_scrolled_text::CancelButtonCallback(Widget, callback_stuff * client_data, void *) {
	popup_scrolled_text * obj = (popup_scrolled_text *) client_data->data;

	obj->paned_window->unmanage(); }

void popup_scrolled_text::OKButtonCallback(Widget, callback_stuff * client_data, void *) {
	popup_scrolled_text * obj = (popup_scrolled_text *) client_data->data;

	obj->paned_window->unmanage();
	if(obj->ok_proc)
		obj->ok_proc(NULL, client_data, obj->Data); }

text_and_list_popup::text_and_list_popup(
	const Cstring & name,
	motif_widget * parent_widget,
	callback_proc CancelButtonCallback,
	callback_proc OKButtonCallback,
	const Cstring & TFlabel,
	const Cstring & Listlabel,
	const Cstring & OKlabel)
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget, NULL, 0, FALSE),
	    data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()) {
#ifdef GUI
	if(GUI_Flag) {
	motif_widget	*form1, *form2, *button, *label;

	AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, & data_for_window_closing);
	paned_window = new motif_widget("paned", xmPanedWindowWidgetClass, this, NULL, 0, FALSE);

	form1 = new  motif_widget("form1", paned_window, form_position(2), true);

	label = new motif_widget(TFlabel, xmLabelWidgetClass, form1);
	name_of_new_plan_file = new single_line_text_widget("filename", form1, EDITABLE);
	*form1 ^ *label ^ *name_of_new_plan_file ^ endform;
	*form1 < *label < endform;
	*form1 < *name_of_new_plan_file < *form1 < endform;

	label = new motif_widget(Listlabel, xmLabelWidgetClass, form1);
	*name_of_new_plan_file ^ *label ^ endform;
	*form1 < *label < endform;
	file_list = new scrolled_list_widget("SaveListScrolled", form1, 5, true);
	*label ^ *file_list ^ *form1 ^ endform;
	*form1 < *file_list < *form1 < endform;

	form2 = new  motif_widget("form2", paned_window, form_position(5), true);
	    OKbutton = new motif_widget(OKlabel, xmPushButtonWidgetClass, form2);
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

void text_and_list_popup::updateMainList(const stringslist& L) {
	emptySymbol*		N;
	stringslist::iterator	l(L);

	((scrolled_list_widget *) file_list)->clear();
	while((N = l.next()))
		*((scrolled_list_widget *) file_list) << N->get_key(); }


UI_integrate::UI_integrate(
	const Cstring & name,
	motif_widget * parent_widget,
	const Cstring & TFlabel,
	const Cstring & Listlabel,
	const Cstring & OKlabel)
	: text_and_list_popup(name, parent_widget, CancelButtonCallback, OKButtonCallback, TFlabel, Listlabel, OKlabel)
	{}

void UI_integrate::initialize() {
	if(! ui_integrate)
		ui_integrate = new UI_integrate("Consolidate Plan Files", MW,
			"New Plan File:", "Use Data From:", "Consolidate");
	ui_integrate->file_list->add_property(SELECTION_POLICY_MULTIPLE_SELECT);
	ui_integrate->updateMainList(FileNamesWithInstancesAndOrNonLayoutDirectives());
	ui_integrate->paned_window->manage(); }

void UI_integrate::CancelButtonCallback(Widget, callback_stuff * client_data, void *) {
	    text_and_list_popup	*obj = (text_and_list_popup *) client_data->data;

	    obj->paned_window->unmanage(); }

// NOTE: we need to invent an action request for the following callback.
void UI_integrate::OKButtonCallback(Widget, callback_stuff * client_data, void *) {
	    text_and_list_popup		*obj = (text_and_list_popup *) client_data->data;

	    obj->paned_window->unmanage();

	strintslist			this_list_better_not_be_empty;
	strint_node			*N, *M;
	emptySymbol*			n;
	FILE_CONSOLIDATErequest*	request;
	stringslist			list_arg;

	((scrolled_list_widget *) obj->file_list)->get_selected_items(this_list_better_not_be_empty);
	for(	N = this_list_better_not_be_empty.first_node();
		N;
		N = M) {
		M = N->next_node();
		if((n = FileNamesWithInstancesAndOrNonLayoutDirectives().find(N->get_key()))) {
			delete n; }
		else if((n = eval_intfc::ListOfAllFileNames().find(N->get_key()))) {
			delete n; }
		list_arg << new emptySymbol(N->get_key()); }
	request = new FILE_CONSOLIDATErequest(list_arg, obj->name_of_new_plan_file->get_text());
	request->process();
	delete request;
}

UI_partial_save::UI_partial_save(
	const Cstring	&name,
	motif_widget	*parent_widget,
	const Cstring	&TFlabel,
	const Cstring	&Listlabel,
	const Cstring	&OKlabel)
	: text_and_list_popup(name, parent_widget, CancelButtonCallback, OKButtonCallback, TFlabel, Listlabel, OKlabel)
	{}

void UI_partial_save::initialize() {
	if(! ui_partial_save)
		ui_partial_save = new UI_partial_save("Write Partial Plan File", MW,
			"New Plan File:", "Use Data From:", "Save");
	ui_partial_save->file_list->add_property(SELECTION_POLICY_MULTIPLE_SELECT);
	ui_partial_save->updateMainList(FileNamesWithInstancesAndOrNonLayoutDirectives());
	ui_partial_save->paned_window->manage(); }

void UI_partial_save::CancelButtonCallback(Widget, callback_stuff * client_data, void *) {
	    text_and_list_popup		*obj = (text_and_list_popup *) client_data->data;

	    obj->paned_window->unmanage(); }

void UI_partial_save::OKButtonCallback(Widget, callback_stuff * client_data, void *) {
	SAVE_PARTIAL_FILErequest	*request = NULL;
	text_and_list_popup		*obj = (text_and_list_popup *) client_data->data;
	strintslist			this_list_better_not_be_empty;
	strintslist::iterator		this_list_iter(this_list_better_not_be_empty);
	strint_node*			N;
	stringslist			new_list;

	obj->paned_window->unmanage();

	((scrolled_list_widget *) obj->file_list)->get_selected_items(this_list_better_not_be_empty);

	while((N = this_list_iter())) {
		new_list << new emptySymbol(N->get_key()); }
	request = new SAVE_PARTIAL_FILErequest(
		new_list,
		obj->name_of_new_plan_file->get_text());
	request->process();
	delete request;
}


UI_new_activities::UI_new_activities(motif_widget *parent_widget)
  : motif_widget("New Activities", xmDialogShellWidgetClass, parent_widget, NULL, 0, FALSE),
    Data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget())
{
#ifdef GUI
  if(GUI_Flag)
  {
    AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, & Data_for_window_closing);

    //make a Paned_window
    Paned_window = new motif_widget("paned", xmPanedWindowWidgetClass, this, NULL, 0, FALSE);

    motif_widget *label = new motif_widget("Select a Subsystem from the list below,\nthen an activity from the second list.",
					    xmLabelWidgetClass, Paned_window, NULL, 0, FALSE); 
#ifdef HAVE_MACOS
    label->fix_height(60, label->widget);
#else
    label->fix_height(label->widget);
#endif /* HAVE_MACOS */
    Subsystem_list = new scrolled_list_widget("list", Paned_window, 5, false);
    Subsystem_list->add_callback(SubsystemSelect, XmNsingleSelectionCallback, this);
    ActivityTypes_list = new scrolled_list_widget("list2", Paned_window, 5, false);
    ActivityTypes_list->add_callback(ActivityTypeSelect, XmNsingleSelectionCallback, this);
    
    //start time, NumTimes and Period -- insensitive until an activity type is selected
    motif_widget *form1 = new motif_widget("form1", Paned_window, form_position(19), true);
    Label_start = new motif_widget("Start Time:", xmLabelWidgetClass, form1, NULL, 0, FALSE);
    *form1 < *Label_start < endform;
    Start_time = new single_line_text_widget("", form1, EDITABLE, false);
    *form1 < *Start_time < *form1 < endform;
    Label_times = new motif_widget("Num Activities:", xmLabelWidgetClass, form1, NULL, 0, FALSE);
    *form1 < *Label_times < endform;
    Num_times = new single_line_text_widget("", form1, EDITABLE, false);
    *form1 < *Num_times < *form1 < endform;
    Label_period = new motif_widget("Period:", xmLabelWidgetClass, form1, NULL, 0, FALSE);
    *form1 < *Label_period < endform;
    Period = new single_line_text_widget("", form1, EDITABLE, false);
    *form1 < *Period < *form1 < endform;
    *form1 ^ *Label_start ^ *Start_time ^ *Label_times ^ *Num_times ^ *Label_period ^ *Period ^ *form1 ^ endform;
#ifdef HAVE_MACOS
    form1->fix_height(170, form1->widget);
#else
    form1->fix_height(form1->widget);
#endif /* HAVE_MACOS */

    //form within paned_window, buttons
    motif_widget *form2 = new  motif_widget("form2", Paned_window, form_position(19), true);
    OKButton = new motif_widget("OK", xmPushButtonWidgetClass, form2, NULL, 0, FALSE);
    *form2 ^ *OKButton ^ *form2 ^ endform;
    form_position(1) < *OKButton < form_position(6) < endform;
    OKButton->add_callback(OKButtonCallback, XmNactivateCallback, this);
    motif_widget *cancel_button = new motif_widget("Cancel", xmPushButtonWidgetClass, form2, NULL, 0, FALSE);
    *form2 ^ *cancel_button ^ *form2 ^ endform;
    form_position(13) < *cancel_button < form_position(18) < endform;
    cancel_button->add_callback(CancelButtonCallback, XmNactivateCallback, this);
    ApplyButton = new motif_widget("Apply", xmPushButtonWidgetClass, form2, NULL, 0, FALSE);
    *form2 ^ *ApplyButton ^ *form2 ^ endform;
    form_position(7) < *ApplyButton < form_position(12) < endform;
    ApplyButton->add_callback(ApplyButtonCallback, XmNactivateCallback, this); 
#ifdef HAVE_MACOS
    form2->fix_height(30, OKButton->widget);
#else
    form2->fix_height(OKButton->widget);
#endif /* HAVE_MACOS */

    Subsystem_list->manage();
    ActivityTypes_list->manage();
    label->manage();
    OKButton->manage();
    cancel_button->manage();
    ApplyButton->manage();
    Label_start->manage();
    Start_time->manage();
    Label_times->manage();
    Num_times->manage();
    Label_period->manage();
    Period->manage();
    Paned_window->manage();
    
    actsis_iterator act_sisses;

    ACT_sys* act_sis;

    CTime startTime;
    if((act_sis = act_sisses())) {
      startTime = act_sis->GetSnappedTimeForPosition(10);
      Start_time->SetText(*startTime.to_string()); }
    else
    { Start_time->SetText("Now"); }

    Period->SetText("01:00:00");
    Num_times->SetText("1");

    InstancePanelUnInit(); }
#endif
}

UI_new_activities::~UI_new_activities()
{
}

void
UI_new_activities::SubsystemSelect(Widget, callback_stuff * client_data, void *) {
  UI_new_activities*	instance = (UI_new_activities *) client_data->data;
  strintslist		sel;

  instance->Subsystem_list->get_selected_items(sel);

  //clear the instance panel because there wont be an activity type selected anymore
  instance->InstancePanelUnInit();

  int len = sel.get_length();
  if(len != 1) {
    instance->ClearActivityTypes();
    return; }

  strint_node* N = sel.first_node();
  Cstring subSystem = N->get_key();

  instance->SetActivityTypes(*subSystem); }

void
UI_new_activities::ActivityTypeSelect(Widget, callback_stuff * client_data, void *)
{
  UI_new_activities* instance = (UI_new_activities *) client_data->data;

  strintslist sel;

  instance->ActivityTypes_list->get_selected_items(sel);

  int len = sel.get_length();
  if(len != 1) {
    instance->ActivityType = "";
    instance->InstancePanelUnInit();
    return; }
  
  instance->InstancePanelInit();

  strint_node* N = sel.first_node();
  
  instance->ActivityType = N->get_key();

  return;
}

void
UI_new_activities::InstancePanelInit()
{
  Label_start->set_sensitive(true);
  Start_time->set_sensitive(true);
  //find a better default start time
  Label_times->set_sensitive(true);
  Num_times->set_sensitive(true);
  Label_period->set_sensitive(true);
  Period->set_sensitive(true);
  Num_times->SetText("1");
  Period->SetText("01:00:00");
  OKButton->set_sensitive(true);
  ApplyButton->set_sensitive(true);
}

void
UI_new_activities::InstancePanelUnInit()
{
  Label_start->set_sensitive(false);
  Start_time->set_sensitive(false);
  Label_times->set_sensitive(false);
  Num_times->set_sensitive(false);
  Label_period->set_sensitive(false);
  Period->set_sensitive(false);
  OKButton->set_sensitive(false);
  ApplyButton->set_sensitive(false);
}

void 
UI_new_activities::CancelButtonCallback(Widget, callback_stuff * client_data, void *) 
{
  UI_new_activities* instance = (UI_new_activities *) client_data->data;

  instance->Hide();
}

void
UI_new_activities::ApplyButtonCallback(Widget, callback_stuff * client_data, void *) 
{
  UI_new_activities* instance = (UI_new_activities *) client_data->data;
  
  instance->ApplyCurrentValues();
}


void 
UI_new_activities::OKButtonCallback(Widget, callback_stuff * client_data, void *) {
  UI_new_activities *instance = (UI_new_activities *) client_data->data;
  
  instance->ApplyCurrentValues();

  instance->Hide(); }

void
UI_new_activities::Show() {
  Paned_window->manage();

  SetSubsystems(); }

void
UI_new_activities::Hide() {
  Paned_window->unmanage(); }

void
UI_new_activities::SetSubsystems() {
  StringVect subSystems;
  
  // apgenDB::GetSubSystemNames(&subSystems);

  Subsystem_list->SetList(subSystems);

  //selects the first one (indices start at 1)
  Subsystem_list->select_pos(1);

  SetActivityTypes(subSystems[0]); }

void
UI_new_activities::SetActivityTypes(const string &subSystemName) {
  StringVect activityTypes;

  // apgenDB::GetActivityTypeNamesForSubSystem(subSystemName, &activityTypes);

  ActivityTypes_list->SetList(activityTypes);

  //because we shouldn't have anything set yet
  InstancePanelUnInit(); }

void
UI_new_activities::ClearActivityTypes() {
	StringVect activityTypes;
	ActivityTypes_list->SetList(activityTypes);
}

void
UI_new_activities::ApplyCurrentValues() {
	Cstring startTime = Start_time->get_text();
	Cstring period = Period->get_text();
	Cstring legend = "Generic_Activities";
	Cstring num_Times = Num_times->get_text();
	const char* num_TimesStr = *num_Times;
	int numTimes = strtol(num_TimesStr, NULL, 10);

	NEW_ACTIVITIESrequest* newActivities = new NEW_ACTIVITIESrequest(
						*ActivityType,
						*startTime,
						*period,
						numTimes,
						*legend);

	apgen::RETURN_STATUS status = newActivities->process();
	delete newActivities;
}

Cstring	UI_rdBackgroundPts::theSavedResource;

UI_rdBackgroundPts::UI_rdBackgroundPts(
	const Cstring&	name,			//will be name of top widget
	motif_widget*	parent_widget,		//parent of this widget
	callback_proc	proc_to_call		//call when OK pushed
		 )
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget,
			NULL, 0, FALSE),
	  data_for_window_closing(NULL, rdbCancelButtonCallback, NULL, get_this_widget()),
	  ok_proc(proc_to_call) {
#ifdef GUI
	if(GUI_Flag) {
	AllowWindowClosingFromMotifMenu(widget, rdbCancelButtonCallback, &data_for_window_closing);

	//build the sub-widgets that are common to this base class

	n = 0;
	XtSetArg(args[n], XmNresizePolicy, XmRESIZE_GROW); n++;
	//explicitly set width to handle critical element (the slider); extra
	//  30 overhead is for 10-pixel borders and other border elements
	XtSetArg(args[n], XmNwidth, (MAX_BACKGROUND_PTS/5 + 28 + 30)); n++;
	XtSetArg(args[n], XmNheight, 145); n++; //EMPIRICAL,fixes messed attachments
	_UI_rdBackgroundPts = new motif_widget(
		"Background Points",
		xmFormWidgetClass,	this,
		args, n,
		FALSE);

	n = 0;
	XtSetArg(args[n], XmNresizePolicy, XmRESIZE_GROW); n++;
	XtSetArg(args[n], XmNfractionBase, 59); n++;
	_rdBackgroundPtsButtonForm = new motif_widget(
		"rdBackgroundPtsButtonForm",
		xmFormWidgetClass,	_UI_rdBackgroundPts,
		args, n,
		true);

	_rdBackgroundPtsButtonForm->add_property(CHILD_OF_A_FORM);
	*_UI_rdBackgroundPts < 10 < *_rdBackgroundPtsButtonForm < 10 < *_UI_rdBackgroundPts < endform;
	*_rdBackgroundPtsButtonForm ^ 10 ^ *_UI_rdBackgroundPts ^ endform;
	_rdBackgroundPtsButtonForm->add_property(FORM_PARENT);

	n = 0;
	XtSetArg(args[n], XmNnavigationType, XmTAB_GROUP); n++;
	_rdBackgroundPtsOKButton = new motif_widget(
		"rdBackgroundPtsOKButton",
		xmPushButtonWidgetClass, _rdBackgroundPtsButtonForm,
		args, n,
		true);
	*_rdBackgroundPtsOKButton = "OK";
	_rdBackgroundPtsOKButton->add_callback(
		UI_rdBackgroundPts::rdbOKButtonCallback, XmNactivateCallback, this);

	form_position(20) < *_rdBackgroundPtsOKButton < form_position(39) < endform;
	*_rdBackgroundPtsButtonForm ^ *_rdBackgroundPtsOKButton ^ *_rdBackgroundPtsButtonForm ^ endform;

	n = 0;
	XtSetArg(args[n], XmNnavigationType, XmTAB_GROUP); n++;
	_rdBackgroundPtsCancelButton = new motif_widget(
		"rdBackgroundPtsCancelButton",
		xmPushButtonWidgetClass, _rdBackgroundPtsButtonForm,
		args, n,
		true);
	*_rdBackgroundPtsCancelButton = "Cancel";
	_rdBackgroundPtsCancelButton->add_callback(
		UI_rdBackgroundPts::rdbCancelButtonCallback, XmNactivateCallback, this);

	*_rdBackgroundPtsButtonForm ^ *_rdBackgroundPtsCancelButton ^ *_rdBackgroundPtsButtonForm ^ endform;
	form_position(40) < *_rdBackgroundPtsCancelButton < form_position(59) < endform;

	n = 0;
	XtSetArg(args[n], XmNnavigationType, XmTAB_GROUP); n++;
	_rdBackgroundPtsResetButton = new motif_widget(
		"rdBackgroundPtsResetButton",
		xmPushButtonWidgetClass, _rdBackgroundPtsButtonForm,
		args, n,
		true);
	*_rdBackgroundPtsResetButton = "Reset";
	_rdBackgroundPtsResetButton->add_callback(
	UI_rdBackgroundPts::rdbResetButtonCallback, XmNactivateCallback, this);

	*_rdBackgroundPtsButtonForm ^ *_rdBackgroundPtsResetButton ^ *_rdBackgroundPtsButtonForm ^ endform;
	form_position(0) < *_rdBackgroundPtsResetButton < form_position(19) < endform;

	n = 0;
	XtSetArg(args[n], XmNminimum, 0); n++;
	XtSetArg(args[n], XmNmaximum, MAX_BACKGROUND_PTS); n++;
	XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
	XtSetArg(args[n], XmNshowValue, True); n++;
	//explicitly set width so 5 units exactly per pixel of
	//  slider movement (plus 28 pixels slider overhead)
	XtSetArg(args[n], XmNwidth, (MAX_BACKGROUND_PTS/5 + 28)); n++;
	XtSetArg(args[n], XmNheight, 37); n++;
	_rdBackgroundPtsResScale = new motif_widget(
		"rdBackgroundPtsResScale",
		xmScaleWidgetClass, _UI_rdBackgroundPts,
		args, n,
		true);

	n = 0;
	_rdBackgroundPtsSeparator1 = new motif_widget(
		"rdBackgroundPtsSeparator",
		xmSeparatorWidgetClass, _UI_rdBackgroundPts,
		args, n,
		true);

	*_UI_rdBackgroundPts < 10 < *_rdBackgroundPtsResScale < 10 < *_UI_rdBackgroundPts < endform;
	*_UI_rdBackgroundPts < 10 < *_rdBackgroundPtsSeparator1 < 10 < *_UI_rdBackgroundPts < endform;
	*_rdBackgroundPtsResScale ^ *_rdBackgroundPtsSeparator1 ^ *_rdBackgroundPtsButtonForm ^ endform;

	//add extra sub-widgets (if any)
	create();
	}
#endif
	  }

void UI_rdBackgroundPts::create() {
#ifdef GUI
	if(GUI_Flag) {
	//set title for this popup
	n = 0;
	XtSetArg(args[n], XmNtitle, "Background Points"); n++;
	XtSetValues(widget, args, n);

	//complete the attachments (derived classes will add more sub-widgets here)
	*_UI_rdBackgroundPts ^ 10 ^ *_rdBackgroundPtsResScale ^ endform;
	}
#endif
	}

void UI_rdBackgroundPts::initialize(void* cur_RES_sys, int res, const Cstring &theResource) {
	//store RES_sys handle, so know which one is being worked with!
	current_RES_sys = cur_RES_sys;

	theSavedResource = theResource;
	//store for RESET
	saved_resolution = res;

	//reset to just-saved (i.e. passed) value
	_rdBackgroundPtsPopupDialogShell->resetInfo(); }

void UI_rdBackgroundPts::resetInfo() {
#ifdef GUI
	if(GUI_Flag) {
	//reset slider based upon saved value
	XtVaSetValues(_rdBackgroundPtsResScale->widget,
		XmNvalue, saved_resolution,
		NULL);
	}
#endif
	}

//next 3 callbacks are used by this (the base class) and by derived classes
void UI_rdBackgroundPts::rdbResetButtonCallback (Widget, callback_stuff * clientData, void *) {
	UI_rdBackgroundPts* eobject = (UI_rdBackgroundPts*) clientData->data;

	eobject->resetInfo(); }

void UI_rdBackgroundPts::rdbCancelButtonCallback (Widget, callback_stuff * clientData, void *) {
	UI_rdBackgroundPts* eobject = (UI_rdBackgroundPts*) clientData->data;

	eobject->_UI_rdBackgroundPts->unmanage(); }

void UI_rdBackgroundPts::rdbOKButtonCallback (Widget, callback_stuff * clientData, void *)
{
	UI_rdBackgroundPts* eobject = (UI_rdBackgroundPts*) clientData->data;

	eobject->_UI_rdBackgroundPts->unmanage();

	eobject->ok_proc(NULL, clientData, eobject->current_RES_sys);
}

void UI_rdBackgroundPts::rdbOKCallback (Widget, callback_stuff * clientData, void * ok_data) {
#ifdef GUI
	if(GUI_Flag) {
	int		resolution;

	UI_rdBackgroundPts* eobject = (UI_rdBackgroundPts*) clientData->data;
	RD_sys* dobject = (RD_sys*) ok_data;

	XmScaleGetValue (eobject->_rdBackgroundPtsResScale->widget, &resolution);
	dobject->setModelingResolution(theSavedResource, resolution);
	}
#endif
	}


UI_rdOptions::UI_rdOptions(
	const Cstring&	name,			//will be name of top widget
	motif_widget	*	parent_widget,		//parent of this widget
	callback_proc	proc_to_call		//call when OK pushed
		 )
	: UI_rdBackgroundPts(name, parent_widget, proc_to_call) {
	//most of work has already been done by UI_rdBackgroundPts constructor

	//add extra sub-widgets for this derived class
	create(); }

void UI_rdOptions::create() {
#ifdef GUI
	if(GUI_Flag) {
	//set title for this popup
	n = 0;
	XtSetArg(args[n], XmNtitle, "Resource Display Control Options"); n++;
	XtSetValues(widget, args, n);

	//set correct height for this popup
	n = 0;
	XtSetArg(args[n], XmNheight, 300); n++; //EMPIRICAL,fixes messed attachments
	XtSetValues(_UI_rdBackgroundPts->widget, args, n);

	//complete the attachments (derived classes will add more sub-widgets here)

	n = 0;
	// XtSetArg(args[n], XmNx, 10); n++;
	// XtSetArg(args[n], XmNy, 142); n++;
	// XtSetArg(args[n], XmNwidth, 212); n++;
	// XtSetArg(args[n], XmNheight, 20); n++;
	_rdOptionsSeparator2 = new motif_widget(
		"rdOptionsSeparator2",
		xmSeparatorWidgetClass, _UI_rdBackgroundPts,
		args, n,
		true);

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	// XtSetArg(args[n], XmNx, 10); n++;
	// XtSetArg(args[n], XmNy, 162); n++;
	// XtSetArg(args[n], XmNwidth, 212); n++;
	// XtSetArg(args[n], XmNheight, 27); n++;
	_rdOptionsResLabel = new motif_widget(
		"rdOptionsResLabel",
		xmLabelWidgetClass, _UI_rdBackgroundPts,
		args, n,
		true);
	*_rdOptionsResLabel = "Background Resolution:";

	*_UI_rdBackgroundPts < 10 < *_rdOptionsSeparator2 < 10 < *_UI_rdBackgroundPts < endform;
	*_UI_rdBackgroundPts < 10 < *_rdOptionsResLabel < 10 < *_UI_rdBackgroundPts < endform;
	*_rdOptionsSeparator2 ^ * _rdOptionsResLabel ^ *_rdBackgroundPtsResScale ^ endform;

	n = 0;
	XtSetArg(args[n], XmNisHomogeneous, False); n++;
	XtSetArg(args[n], XmNnavigationType, XmNONE); n++;
	XtSetArg(args[n], XmNmarginHeight, 0); n++;
	_rdOptionsControlRB = new motif_widget(
		"rdOptionsControlRB",
		xmRadioBoxWidgetClass,	_UI_rdBackgroundPts,
		args, n,
		true);

	n = 0;
	XtSetArg(args[n], XmNsensitive, True); n++;
	XtSetArg(args[n], XmNtraversalOn, False); n++;
	XtSetArg(args[n], XmNhighlightThickness, 0); n++;
	_rdOptionsManualTB = new motif_widget(
	"rdOptionsManualTB",
		xmToggleButtonWidgetClass, _rdOptionsControlRB,
		args, n,
		true);
	_rdOptionsManualTB->set_label("Manual");

	n = 0;
	XtSetArg(args[n], XmNhighlightThickness, 0); n++;
	XtSetArg(args[n], XmNtraversalOn, False); n++;
	XtSetArg(args[n], XmNset, True); n++;
	_rdOptionsAutoWindowTB = new motif_widget(
	"rdOptionsAutoWindowTB",
		xmToggleButtonWidgetClass, _rdOptionsControlRB,
		args, n,
		true);
	_rdOptionsAutoWindowTB->set_label("Auto Window");

	n = 0;
	XtSetArg(args[n], XmNsensitive, True); n++;
	XtSetArg(args[n], XmNtraversalOn, False); n++;
	XtSetArg(args[n], XmNhighlightThickness, 0); n++;
	_rdOptionsAutoPlanTB = new motif_widget(
	"rdOptionsAutoPlanTB",
		xmToggleButtonWidgetClass, _rdOptionsControlRB,
		args, n,
		true);
	_rdOptionsAutoPlanTB->set_label("Auto Plan");

	n = 0;
	XtSetArg(args[n], XmNsensitive, False); n++;
	XtSetArg(args[n], XmNhighlightThickness, 0); n++;
	_rdOptionsAutoRangeTB = new motif_widget(
	"rdOptionsAutoRangeTB",
		xmToggleButtonWidgetClass, _rdOptionsControlRB,
		args, n,
		true);
	_rdOptionsAutoRangeTB->set_label("Auto Range");

	*_UI_rdBackgroundPts < 10 < *_rdOptionsControlRB < 10 < *_UI_rdBackgroundPts < endform;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	_rdOptionsControlLabel = new motif_widget(
	"rdOptionsControlLabel",
		xmLabelWidgetClass, _UI_rdBackgroundPts,
		args, n,
		true);
	*_rdOptionsControlLabel = "Control:";

	*_UI_rdBackgroundPts < 10 < *_rdOptionsControlLabel < 10 < *_UI_rdBackgroundPts < endform;
	*_UI_rdBackgroundPts ^ 10 ^ *_rdOptionsControlLabel ^ *_rdOptionsControlRB ^ *_rdOptionsSeparator2 ^ endform;
	}
#endif
	}

void UI_rdOptions::initialize(RD_YSCROLL_MODE mode, int res) {
	//no RD_sys handle, since UI_rdOptions works globally on SELECTED RDs
	current_RES_sys = NULL;

	//store for RESET
	saved_mode = mode;
	saved_resolution = res;

	//reset to just-saved (i.e. passed) values
	_rdOptionsPopupDialogShell->resetInfo(); }

void UI_rdOptions::resetInfo() {
#ifdef GUI
	if(GUI_Flag) {
	//reset slider based upon saved value
	XtVaSetValues(_rdBackgroundPtsResScale->widget,
		XmNvalue, saved_resolution,
		NULL);

	//reset radio buttons based on saved value
	XtVaSetValues (_rdOptionsManualTB->widget,XmNset,False,NULL);
	XtVaSetValues (_rdOptionsAutoWindowTB->widget,XmNset,False,NULL);
	XtVaSetValues (_rdOptionsAutoPlanTB->widget,XmNset,False,NULL);
	XtVaSetValues (_rdOptionsAutoRangeTB->widget,XmNset,False,NULL);
	switch (saved_mode) {
		case (YSCROLL_MANUAL):
			XtVaSetValues (_rdOptionsManualTB->widget,XmNset,True,NULL);
			XtVaSetValues (_rdOptionsControlRB->widget,XmNmenuHistory,
			    _rdOptionsManualTB->widget,NULL);
			break;
		case (YSCROLL_AUTO_WINDOW):
			XtVaSetValues (_rdOptionsAutoWindowTB->widget,XmNset,True,NULL);
			XtVaSetValues (_rdOptionsControlRB->widget,XmNmenuHistory,
			    _rdOptionsAutoWindowTB->widget,NULL);
			break;
		case (YSCROLL_AUTO_PLAN):
			XtVaSetValues (_rdOptionsAutoPlanTB->widget,XmNset,True,NULL);
			XtVaSetValues (_rdOptionsControlRB->widget,XmNmenuHistory,
			    _rdOptionsAutoPlanTB->widget,NULL);
			break;
		case (YSCROLL_AUTO_RANGE):
			XtVaSetValues (_rdOptionsAutoRangeTB->widget,XmNset,True,NULL);
			XtVaSetValues (_rdOptionsControlRB->widget,XmNmenuHistory,
			    _rdOptionsAutoRangeTB->widget,NULL);
			break; }
	}
#endif
	}

void UI_rdOptions::rdoOKCallback (Widget, callback_stuff * clientData, void * /*ok_data*/) {
#ifdef GUI
	if(GUI_Flag) {
	int			resolution;
	RD_YSCROLL_MODE		mode;

	UI_rdOptions* eobject = (UI_rdOptions*) clientData->data;

	//get the resolution from the slider
	XmScaleGetValue (eobject->_rdBackgroundPtsResScale->widget, &resolution);

	//get the mode from the radio box buttons
	if (eobject->_rdOptionsManualTB->get_text() == "SET")
		mode = YSCROLL_MANUAL;
	else if (eobject->_rdOptionsAutoWindowTB->get_text() == "SET")
		mode = YSCROLL_AUTO_WINDOW;
	else if (eobject->_rdOptionsAutoPlanTB->get_text() == "SET")
		mode = YSCROLL_AUTO_PLAN;
	else if (eobject->_rdOptionsAutoRangeTB->get_text() == "SET")
		mode = YSCROLL_AUTO_RANGE;

	//set the mode and resolution of the SELECTED RDs
	UI_subsystem->setRDControlInfo (mode, resolution);
	}
#endif
	}


UI_newLegend::UI_newLegend(
	const Cstring&	name,			//will be name of top widget
	motif_widget*	parent_widget,		//parent of this widget
	callback_proc	proc_to_call		//call when OK pushed
		 )
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget,
			NULL, 0, false),
	  data_for_window_closing(NULL, nlCancelButtonActivate, NULL, get_this_widget()),
	  ok_proc(proc_to_call) {
#ifdef GUI
	if(GUI_Flag) {
	AllowWindowClosingFromMotifMenu(widget, nlCancelButtonActivate, &data_for_window_closing);

	n = 0;
	XtSetArg(args[n], XmNwidth, 300); n++;
	_UI_newLegend = new motif_widget(
		"New Legend",
		xmFormWidgetClass,	this,
		args,			n,
		false);

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	_nlLabel = new motif_widget(
		"Legend Text:",
		xmLabelWidgetClass, _UI_newLegend, 
		args, n,
		true);

	_nlLegendTF = new single_line_text_widget("nlLegendTF", _UI_newLegend, EDITABLE, true);

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	LegendHeightLabel = new motif_widget(
		"Legend Height:",
		xmLabelWidgetClass,	_UI_newLegend, 
		args,			n,
		true);

	LegendHeightTextField = new single_line_text_widget("LegendHeightTextField", _UI_newLegend, EDITABLE, true);
	*LegendHeightTextField = "32";
	n = 0;
	_nlSeparator = new motif_widget(
		"nlSeparator",
		xmSeparatorWidgetClass, _UI_newLegend,
		args, n,
		true);

	*_UI_newLegend < 10 < *_nlLabel			< 10 < *_UI_newLegend < endform;
	*_UI_newLegend < 10 < *_nlLegendTF		< 10 < *_UI_newLegend < endform;
	*_UI_newLegend < 10 < *LegendHeightLabel	< 10 < *_UI_newLegend < endform;
	*_UI_newLegend < 10 < *LegendHeightTextField	< 10 < *_UI_newLegend < endform;
	*_UI_newLegend < 10 < *_nlSeparator		< 10 < *_UI_newLegend < endform;
	*_UI_newLegend ^ 10 ^ *_nlLabel ^ *_nlLegendTF ^ 10 ^ *LegendHeightLabel ^ *LegendHeightTextField ^ 5 ^ *_nlSeparator ^ endform;

	n = 0;
	XtSetArg(args[n], XmNresizePolicy, XmRESIZE_GROW); n++;
	XtSetArg(args[n], XmNfractionBase, 59); n++;
	_nlButtonForm = new motif_widget(
		"nlButtonForm",
		xmFormWidgetClass, _UI_newLegend,
		args, n,
		true);

	_nlButtonForm->add_property(CHILD_OF_A_FORM);
	*_UI_newLegend < 10 < *_nlButtonForm < 10 < *_UI_newLegend < endform;
	*_nlSeparator ^ 5 ^ *_nlButtonForm ^ 10 ^ *_UI_newLegend ^ endform;
	_nlButtonForm->add_property(FORM_PARENT);

	n = 0;
	XtSetArg(args[n], XmNnavigationType, XmTAB_GROUP); n++;
	_nlOKButton = new motif_widget(
		"nlOKButton",
		xmPushButtonWidgetClass, _nlButtonForm,
		args, n,
		true);
	*_nlOKButton = "OK";
	_nlOKButton->add_callback(UI_newLegend::nlOKButtonActivate, XmNactivateCallback, this);

	form_position(0) < *_nlOKButton < form_position(19) < endform;
	*_nlButtonForm ^ *_nlOKButton ^ *_nlButtonForm ^ endform;

	n = 0;
	XtSetArg(args[n], XmNnavigationType, XmTAB_GROUP); n++;
	_nlApplyButton = new motif_widget(
		"nlApplyButton",
		xmPushButtonWidgetClass, _nlButtonForm,
		args, n,
		true);
	*_nlApplyButton = "Apply";
	_nlApplyButton->add_callback(UI_newLegend::nlApplyButtonActivate, XmNactivateCallback, this);

	*_nlButtonForm ^ *_nlApplyButton ^ *_nlButtonForm ^ endform;
	form_position(20) < *_nlApplyButton < form_position(39) < endform;

	n = 0;
	XtSetArg(args[n], XmNnavigationType, XmTAB_GROUP); n++;
	_nlCancelButton = new motif_widget(
		"nlCancelButton",
		xmPushButtonWidgetClass, _nlButtonForm,
		args, n,
		true);
	*_nlCancelButton = "Cancel";
	_nlCancelButton->add_callback(UI_newLegend::nlCancelButtonActivate, XmNactivateCallback, this);

	*_nlButtonForm ^ *_nlCancelButton ^ *_nlButtonForm ^ endform;
	form_position(40) < *_nlCancelButton < form_position(59) < endform;
	}
#endif
	}

void UI_newLegend::initialize () {
	; }	//nothing to do

void UI_newLegend::nlApplyButtonActivate (Widget, callback_stuff * clientData, void *) {
	UI_newLegend* eobject = (UI_newLegend*) clientData->data;

	eobject->applyChanges(); }

void UI_newLegend::nlCancelButtonActivate (Widget, callback_stuff * clientData, void *) {
	UI_newLegend* eobject = (UI_newLegend*) clientData->data;

	eobject->_UI_newLegend->unmanage(); }

void UI_newLegend::nlOKButtonActivate (Widget, callback_stuff * clientData, void *) {
	UI_newLegend		*eobject = (UI_newLegend*) clientData->data;

	if (eobject->applyChanges() == apgen::RETURN_STATUS::SUCCESS) {
		eobject->_UI_newLegend->unmanage();
		eobject->ok_proc(NULL, clientData, NULL); } }

void UI_newLegend::nlOKCallback (Widget, callback_stuff *, void *) {
	; }	//nothing to do

apgen::RETURN_STATUS UI_newLegend::applyChanges () {
	NEW_LEGENDrequest	*request;
	Cstring			legendText(_nlLegendTF->get_text());
	Cstring			legendHeight(LegendHeightTextField->get_text());
	int			leg_height;
	const char		*c = *legendHeight;

	if(!*c) {
		bell();
		return apgen::RETURN_STATUS::FAIL; }
	while(*c) {
		if(!isdigit(*c)) {
			bell();
			return apgen::RETURN_STATUS::FAIL; }
		c++; }
	if(!legendText.length()) {
			bell();
			return apgen::RETURN_STATUS::FAIL; }
	while(*c) {
		if(!isdigit(*c)) {
			bell();
			return apgen::RETURN_STATUS::FAIL; }
		c++; }

	sscanf(*legendHeight, "%d", &leg_height);
	if(leg_height > 2000)
		leg_height = 2000;
	if(leg_height < ACTVERUNIT) {
		if(leg_height < (ACTFLATTENED + ACTSQUISHED) / 2)
			leg_height = ACTFLATTENED;
		else if(leg_height < (ACTVERUNIT + ACTSQUISHED) / 2)
			leg_height = ACTSQUISHED;
		else
			leg_height = ACTVERUNIT; }

	//action request checks for bad values (i.e. AFTER normalization)
	request = new NEW_LEGENDrequest(legendText, leg_height);
	apgen::RETURN_STATUS retval = request->process();
	delete request;
	return retval;
}

add_resource_popup::add_resource_popup(
	const Cstring	&name,			//will be name of top widget
	motif_widget	*parent_widget,		//parent of this widget
	callback_proc	proc_to_call,
	void		*data)
	: popup_list(name, parent_widget, proc_to_call, data,
			/* apply button */ 1, /* hold creation */ 1) {
	Create(); }

add_resource_popup::~add_resource_popup() {
	listOfSubsystems.clear(); }

void add_resource_popup::Create(int) {
#ifdef GUI
    if(GUI_Flag) {
	motif_widget		*label;

	XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++;
	label = new motif_widget("Select a Subsystem from the list below,\nthen a resource from the second list.",
		xmLabelWidgetClass, paned_window, args, n, true); 
#ifdef HAVE_MACOS
	label->fix_height(32, label->widget);
#else
	label->fix_height(label->widget);
#endif /* HAVE_MACOS */

	theTypeOfPopup = 2;
	fill_in_subsystems();
	// this is the list that will be used for subsystems:
	scrolled_list2 = new scrolled_list_widget("list2", paned_window, 5, false);
	scrolled_list2->add_callback(UpdateSubsystem, XmNsingleSelectionCallback, this);
	popup_list::Create();
	updateAllLists();
	}
#endif /* GUI */
    }

void add_resource_popup::UpdateSubsystem(Widget, callback_stuff *client_data, void *call_data) {
#ifdef GUI
	if(GUI_Flag) {
	add_resource_popup	*obj = (add_resource_popup *) client_data->data;
	XmListCallbackStruct	*cbs = (XmListCallbackStruct *) call_data;
	int			thePosition = -1, selection_flag = 0;
 
	thePosition = cbs->item_position;
	selection_flag = XmListPosSelected(client_data->parent->widget, thePosition);
	obj->updateLowerList(thePosition, selection_flag);
	}
#endif
	}

// Update the 2 list widgets based on the contents of listOfSubsystems.
void add_resource_popup::updateAllLists() {
#ifdef GUI
	if(GUI_Flag) {
	tlistNode*	theTag;
	int		Nres;

	if(XtIsRealized(scrolled_list->widget)) {

		// debug
		// cerr << "add_resource_popup: upper list is realized; unrealizing it\n";

		XtUnrealizeWidget(scrolled_list->widget); }
	compute_resource_subsystems(listOfSubsystems, Nres);
	((scrolled_list_widget *) scrolled_list2)->clear();
	for(	theTag = listOfSubsystems.earliest_node();
		theTag;
		theTag = theTag->following_node()) {
		*((scrolled_list_widget *) scrolled_list2) << theTag->get_key(); }
	((scrolled_list_widget *) scrolled_list2)->select_pos(1);
	// populate lower panel
	updateLowerList(1, 1);
	XtManageChild(scrolled_list2->widget);
	}
#endif
	}

void add_resource_popup::updateLowerList(int pos, int sel) {
#ifdef GUI
	if(GUI_Flag) {

	int k = 0;
	tlistNode* tl = NULL;
	for(tl = listOfSubsystems.first_node();
	    tl;
	    tl = tl->next_node()) {
		if(k == pos - 1) {
			break;
		}
		k++;
	}
	tlistNode*	theTag = tl;

	if(XtIsRealized(scrolled_list->widget)) {

		// debug
		// cerr << "add_resource_popup: lower list is realized; unrealizing it\n";

		XtUnrealizeWidget(scrolled_list->widget); }
	((scrolled_list_widget *) scrolled_list)->clear();
	*((motif_widget *) chosen_text) = "";
	if(!sel) {
		return; }
	if(theTag) {
		stringtlist::iterator	l(theTag->payload);
		emptySymbol*		N;

		for(N = theTag->payload.earliest_node();
		    N;
		    N = N->following_node()) {
			*((scrolled_list_widget *) scrolled_list) << N->get_key(); } }
	XtManageChild(scrolled_list->widget);
	}
#endif
	}

new_act_popup::new_act_popup(
	const Cstring	&name,			//will be name of top widget
	motif_widget	*parent_widget,	//parent of this widget
	callback_proc	proc_to_call,
	void		*data,
	long		apply_button_needed)
	: popup_list(name, parent_widget, proc_to_call, data,
			/* apply button */ apply_button_needed, /* hold creation */ 1)
	{}

new_act_popup::~new_act_popup()
	{}

/*
 * flag = 1: create a new activity
 * flag = 2: group activity/ies
 */
void new_act_popup::Create(int act_or_parent) {
#ifdef GUI
	if(GUI_Flag) {
	motif_widget	*frame, *radio_buttons, *options;

	XtVaSetValues(widget, XmNkeyboardFocusPolicy, XmEXPLICIT, NULL);
	if(act_or_parent == 2) {
		// group activity/ies
		theTypeOfPopup = 3;
		frame = new motif_widget("theFrame", xmFrameWidgetClass, paned_window);
			radiobox = new motif_widget("Radiobox", xmRadioBoxWidgetClass, frame);
			useExistingActivity = new motif_widget("Select Existing Activity",
				xmToggleButtonWidgetClass,	radiobox);
			useExistingActivity->add_callback(creation_method_callback, XmNvalueChangedCallback, this);
			XtVaSetValues(useExistingActivity->widget, XmNtraversalOn, False, NULL);
			createNewActivity = new motif_widget("Create New Activity",
				xmToggleButtonWidgetClass,	radiobox);
			createNewActivity->add_callback(creation_method_callback, XmNvalueChangedCallback, this);
			XtVaSetValues(createNewActivity->widget, XmNtraversalOn, False, NULL);
			createNewRequest = new motif_widget("Create New Request",
				xmToggleButtonWidgetClass,	radiobox);
			createNewRequest->add_callback(creation_method_callback, XmNvalueChangedCallback, this);
			XtVaSetValues(createNewRequest->widget, XmNtraversalOn, False, NULL);
			createNewChameleon = new motif_widget("Create New Chameleon",
				xmToggleButtonWidgetClass,	radiobox);
			createNewChameleon->add_callback(creation_method_callback, XmNvalueChangedCallback, this);
			XtVaSetValues(createNewChameleon->widget, XmNtraversalOn, False, NULL);
			EventRegistry::Register().Subscribe("UI_ACTIVITY_SELECTED", "SelectionSignal", this);
			EventRegistry::Register().Subscribe("UI_ACTIVITY_UNSELECTED", "UnselectionSignal", this);
			}
	else {
		// regular activity
		theTypeOfPopup = 1;
		// get_the_radiobox() will be used to test what kind of panel we got:
		radiobox = NULL; }

	XtSetArg(args[n], XmNalignment, XmALIGNMENT_CENTER); n++;
	user_instructions = new motif_widget("Select a Subsystem from the list below,\nthen a type from the second list.",
		xmLabelWidgetClass, paned_window, args, n, true); 
#ifdef HAVE_MACOS
	user_instructions->fix_height(32, user_instructions->widget);
#else
	user_instructions->fix_height(user_instructions->widget);
#endif /* HAVE_MACOS */

	scrolled_list2 = new scrolled_list_widget("list2", paned_window, 5, false);
	scrolled_list2->add_callback(UpdateSubsystem, XmNsingleSelectionCallback, this);

	// Continue with the type selection list:
	popup_list::Create();
	updateAllLists();
	}
#endif /* GUI */
	}

void new_act_popup::HandleEvent(const string &EventType, const TypedValuePtrVect &theArgs) {
	TypedValuePtrVect::const_iterator citer = theArgs.begin();

#	ifdef JUST_DEBUGGING
	cout << "new_act_popup::Event printing its arguments:\n";
	while(citer != theArgs.end()) {
		TypedValue	*val = *citer;
		cout << "    ";
		val->print(cout);
		cout << endl;
		citer++;
	}
	cout << "new_act_popup: done.\n";
#	endif
	if(EventType == "UI_ACTIVITY_SELECTED") {
		OKbutton->set_sensitive(TRUE);
	} else {
		OKbutton->set_sensitive(FALSE);
	}
}

void new_act_popup::UpdateSubsystem(Widget, callback_stuff *client_data, void *call_data) {
#ifdef GUI
	if(GUI_Flag) {
	new_act_popup		*obj = (new_act_popup *) client_data->data;
	XmListCallbackStruct	*cbs = (XmListCallbackStruct *) call_data;
	int			thePosition = -1, selection_flag = 0;
 
	thePosition = cbs->item_position;
	selection_flag = XmListPosSelected(client_data->parent->widget, thePosition);
	obj->updateLowerList(thePosition, selection_flag);
	}
#endif
	}

void new_act_popup::updateLowerList(int pos, int sel) {
#ifdef GUI
	if(GUI_Flag) {

	int k = 0;
	tlistNode* tl = NULL;
	for(tl = listOfSubsystems.first_node();
	    tl;
	    tl = tl->next_node()) {
		if(k == pos - 1) {
			break;
		}
		k++;
	}
	tlistNode*	theTag = tl;

	if(XtIsRealized(scrolled_list->widget)) {

		// debug
		// cerr << "new_act_popup: lower list is realized; unrealizing it\n";

		XtUnrealizeWidget(scrolled_list->widget); }

	((scrolled_list_widget *) scrolled_list)->clear();
	*((motif_widget *) chosen_text) = "";
	if(!sel) {
		return; }
	if(theTag) {
		emptySymbol*		N;

		for(N = theTag->payload.earliest_node();
		    N;
		    N = N->following_node()) {
			*((scrolled_list_widget *) scrolled_list) << N->get_key(); } }
	XtManageChild(scrolled_list->widget);
	}
#endif
	}

/* Update the 2 list widgets based on the contents of listOfSubsystems,
 * then call updateLowerList() to populate the lower (activity type)
 * list. */
void new_act_popup::updateAllLists() {
#ifdef GUI
	if(GUI_Flag) {
	tlistNode*	theTag;
	bool		chameleons_only = false, templates_only = false;
	int		Nacts;

	if(theTypeOfPopup == 3
		&& (createNewChameleon->get_text() == "SET")) {
		chameleons_only = true; }
	else if(theTypeOfPopup == 3
		&& (createNewRequest->get_text() == "SET")) {
		templates_only = true; }

	if(XtIsRealized(scrolled_list2->widget)) {

		// debug
		// cerr << "new_act_popup: upper list is realized; unrealizing it\n";

		XtUnrealizeWidget(scrolled_list2->widget); }

	((scrolled_list_widget *) scrolled_list2)->clear();
	compute_activity_subsystems(listOfSubsystems, Nacts, chameleons_only, templates_only);
	for(	theTag = listOfSubsystems.earliest_node();
		theTag;
		theTag = theTag->following_node()) {
		*((scrolled_list_widget *) scrolled_list2) << theTag->get_key(); }
	((scrolled_list_widget *) scrolled_list2)->select_pos(1);
	// populate lower panel
	updateLowerList(1, 1);
	XtManageChild(scrolled_list2->widget);
	}
#endif
	}

void new_act_popup::creation_method_callback(Widget, callback_stuff * client_data, void *call_data) {
#ifdef GUI
	if(GUI_Flag) {
	new_act_popup			*obj = (new_act_popup *) client_data->data;
	XmToggleButtonCallbackStruct	*toggle_state =
		    				(XmToggleButtonCallbackStruct *) call_data;

	if(toggle_state->set) {
		obj->fill_in_subsystems();
		obj->updateAllLists(); }
		}
#endif
	}

hopper_popup::hopper_popup(	const Cstring	&name,
				motif_widget	*parent_widget)
		: motif_widget("Hopper Window", xmDialogShellWidgetClass, parent_widget, NULL, 0, FALSE),
		data_for_window_closing(NULL, CancelButtonCallback, NULL, get_this_widget()),
		AD(NULL) {
	n = 0;
	_panedWindow = new motif_widget("hopper_paned", xmPanedWindowWidgetClass, this, args, n, false);

#ifdef GUI
	if(GUI_Flag) {
		AllowWindowClosingFromMotifMenu(widget, CancelButtonCallback, &data_for_window_closing);


		n = 0;
		XtSetArg(args[n], XmNwidth, 900); n++;
		_menuBar = new motif_widget("menuBar", xmMenubarWidgetClass, _panedWindow, args, n, true);
#ifdef HAVE_MACOS
		_menuBar->fix_height(32, _menuBar->widget);
#else
		_menuBar->fix_height(_menuBar->widget);
#endif /* HAVE_MACOS */

		_fileButton = new motif_widget("File", xmCascadeButtonWidgetClass, _menuBar);
		_fileButtonPM = new motif_widget("fileButtonPM", xmPulldownMenuWidgetClass, _menuBar);
		XtSetArg(args[n], XmNsubMenuId, _fileButtonPM->widget); n++;
		XtSetValues(_fileButton->widget, args, n);

		_viewButton = new motif_widget("View", xmCascadeButtonWidgetClass, _menuBar);
		_viewButtonPM = new motif_widget("viewButtonPM", xmPulldownMenuWidgetClass, _menuBar);
		XtSetArg(args[n], XmNsubMenuId, _viewButtonPM->widget); n++;
		XtSetValues(_viewButton->widget, args, n);

		_zoomButton = new motif_widget("Zoom", xmCascadeButtonWidgetClass, _menuBar);
		_zoomButtonPM = new motif_widget("zoomButtonPM", xmPulldownMenuWidgetClass, _menuBar);
		XtSetArg(args[n], XmNsubMenuId, _zoomButtonPM->widget); n++;
		XtSetValues(_zoomButton->widget, args, n);

		_fileCloseButton = new motif_widget(		"Close Window",
								xmPushButtonWidgetClass,
								_fileButtonPM);
		_fileCloseButton->add_callback(		hopper_popup::fileCloseCallback,
							XmNactivateCallback,
							this);
		_hierarchyColorsButton = new motif_widget(	"Hierarchy-based Colors",
								xmPushButtonWidgetClass,
								_viewButtonPM);
		_hierarchyColorsButton->add_callback(	hopper_popup::viewCallback,
							XmNactivateCallback,
							this);
		_origColorsButton = new motif_widget(		"Original Colors",
								xmPushButtonWidgetClass,
								_viewButtonPM);
		_origColorsButton->add_callback(	hopper_popup::viewCallback,
							XmNactivateCallback,
							this);
		_hierarchySelectionOnButton = new motif_widget(	"Select Hierarchy",
								xmPushButtonWidgetClass,
								_viewButtonPM);
		_hierarchySelectionOnButton->add_callback(
							hopper_popup::viewCallback,
							XmNactivateCallback,
							this);
		_downSelectionOnButton = new motif_widget(	"Select Descendants",
								xmPushButtonWidgetClass,
								_viewButtonPM);
		_downSelectionOnButton->add_callback(	hopper_popup::viewCallback,
							XmNactivateCallback,
							this);
		_hierarchySelectionOffButton = new motif_widget(
								"Select Single Act.",
								xmPushButtonWidgetClass,
								_viewButtonPM);
		_hierarchySelectionOffButton->add_callback(
							hopper_popup::viewCallback,
							XmNactivateCallback,
							this);

		_origColorsButton->set_sensitive(false);
		_hierarchySelectionOffButton->set_sensitive(false);

		_zoomInButton = new motif_widget("Zoom In", xmPushButtonWidgetClass, _zoomButtonPM);
		_zoomInButton->add_callback(hopper_popup::zoomCallback, XmNactivateCallback, this);
		_zoomOutButton = new motif_widget("Zoom Out", xmPushButtonWidgetClass, _zoomButtonPM);
		_zoomOutButton->add_callback(hopper_popup::zoomCallback, XmNactivateCallback, this);
		_zoomToFitButton = new motif_widget("Zoom To Fit", xmPushButtonWidgetClass, _zoomButtonPM);
		_zoomToFitButton->add_callback(hopper_popup::zoomCallback, XmNactivateCallback, this);

		// the activity_display is already a frame:
		// _hopperFrame = new motif_widget("hopper_frame", xmFrameWidgetClass, _panedWindow);
		}
#endif /* GUI */
	// We must do this because the hopper may be used even in no-gui mode.
	AD = new activity_display("HopperActDisplay", _panedWindow, 484, 334);
	AD->initialize("0Hopper");
}

hopper_popup::~hopper_popup() {}

void hopper_popup::viewCallback(Widget, callback_stuff *client_data, void *) {
#ifdef GUI
	hopper_popup	*obj = (hopper_popup *) client_data->data;
	motif_widget	*origin = client_data->parent;
#endif
}

void hopper_popup::zoomCallback(Widget, callback_stuff *client_data, void *) {
#ifdef GUI
	if(GUI_Flag) {
	hopper_popup	*obj = (hopper_popup *) client_data->data;
	motif_widget	*origin = client_data->parent;

	/*
	* Reminder: the hopper constructor is invoked first (by UI_mainwindow::initialize_hopper())
	* then an activity_display is created as a child of paned_window. By the time this callback
	* is involved, the full-fledged ACT-sys is available as ((activity_display *) AD)->activitySystem.
	*/

	NEW_HORIZONrequest	*request;

	Cstring whichSys;

	whichSys = "H1";

	if(origin == obj->_zoomInButton || origin == obj->_zoomOutButton) {
		CTime			oldDuration, newDuration;
		CTime			oldStartTime, newStartTime;
		double			factor = 1.25;

		if(origin == obj->_zoomInButton) {
			factor = 0.8; }
		obj->AD->getZoomPanInfo(oldStartTime, oldDuration);
		newDuration = oldDuration * factor;
		newStartTime = oldStartTime + ((oldDuration - newDuration) / 2.0);
		if(newStartTime < CTime(0, 0, false))
			newStartTime = CTime(0, 0, false);
		// about 10 years:
		if(newDuration > CTime(315360000, 0, true))
			newDuration = CTime(315360000, 0, true);
		// min size set to 1 --> DELETE: tenth of a :ETELED <-- millisecond:
		else if(newDuration < CTime(0, 1, true))
			newDuration = CTime(0, 1, true);
		// DO THIS FIRST!
		obj->AD->activitySystem->never_scrolled = 0;

		request = new NEW_HORIZONrequest(whichSys, newStartTime, newStartTime + newDuration);
		request->process();
		delete request;
	} else if(origin == obj->_zoomToFitButton) {
		CTime	uno(0, 0, false), due(0, 0, false);

		if(model_intfc::FirstEvent(uno) && model_intfc::LastEvent(due)) {
			request = new NEW_HORIZONrequest(whichSys, uno, due);
			request->process();
			delete request;
		}
	}
}
#endif /* ifdef GUI */
}

void hopper_popup::CancelButtonCallback(Widget, callback_stuff *client_data, void *) {
	hopper_popup	*obj = (hopper_popup *) client_data->data;
	// do something here?
	obj->_panedWindow->unmanage(); }

void hopper_popup::OKButtonCallback(Widget, callback_stuff *client_data, void *) {
	hopper_popup	*obj = (hopper_popup *) client_data->data;

	obj->OKcallback(); }

void hopper_popup::OKcallback() {}


void hopper_popup::fileCloseCallback(Widget, callback_stuff *client_data, void *) {
	hopper_popup	*obj = (hopper_popup *) client_data->data;
	obj->_panedWindow->unmanage(); }
