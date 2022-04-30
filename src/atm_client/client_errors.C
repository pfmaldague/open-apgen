#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "APerrors.H"
#include <ActivityInstance.H>
#include "client_errors.H"
#include "UI_mainwindow.H"
#include "UI_GeneralMotifInclude.H"
#include "UI_motif_widget.H"
#include "action_request.H" // Added by BKL 9-19-97

using namespace std;

Cstring add_pound_sign(const Cstring & S);

extern resource_data    ResourceData;
extern int		GUI_Flag;

int client_errors::Processing = 0;
int client_errors::MoreProcessing = 0;

char*			client_errors::theTitle = NULL;

client_errors		*theErrorPanel = NULL;
client_errors		*theMessagePanel = NULL;
int			first_error_panel_appearance = 1;
int			first_message_panel_appearance = 1;

// STATICS:

// END OF STATICS

client_errors::client_errors()
	: panel("error_popup") {
	Cstring continue_string("Continue");
	Cstring more_string("More...");

	create_widgets(INFORMATION_PANEL, & continue_string, & more_string); }

client_errors::~client_errors() {}

void client_errors::set_title_to(const Cstring& aNewTitle) {
	if(theTitle && aNewTitle == theTitle) {
		return; }
	if(theTitle) {
		free(theTitle); }
	theTitle = (char *) malloc(strlen(*aNewTitle) + 1);
	strcpy(theTitle, *aNewTitle);
	if(!first_error_panel_appearance) {
        	XtVaSetValues(theErrorPanel->DialogWidget->widget,
			XtVaTypedArg, XmNtitle, XmRString, theTitle, strlen(theTitle) + 1, NULL); } }

void client_errors::ExpectMore() {
	if(first_message_panel_appearance) {
		first_message_panel_appearance = 0;
		theMessagePanel = new client_errors();
		theMessagePanel->ContinueButton->add_callback(error_cancel, XmNactivateCallback, (void *) theMessagePanel);
		theMessagePanel->MoreButton->add_callback(error_add, XmNactivateCallback, (void *) theMessagePanel); }
	theMessagePanel->MoreButton->set_sensitive(True); }

void client_errors::DisplayMessage(const Cstring &title, const Cstring &msg) {
	if(GUI_Flag) {
	Widget  scroll_bar;

	if(first_message_panel_appearance) {
		first_message_panel_appearance = 0;
		theMessagePanel = new client_errors();
		theMessagePanel->ContinueButton->add_callback(error_cancel, XmNactivateCallback, (void *) theMessagePanel);
		theMessagePanel->MoreButton->add_callback(error_add, XmNactivateCallback, (void *) theMessagePanel); }

        XtVaSetValues(theMessagePanel->DialogWidget->widget,
		XtVaTypedArg, XmNtitle, XmRString, *title, title.length() + 1, NULL);
	XtVaSetValues(theMessagePanel->PopupWidget->widget,	XmNbackground, ResourceData.ok_color, NULL);
	XtVaSetValues(theMessagePanel->InfoWidget->widget,	XmNbackground, ResourceData.grey_color, NULL);
	XtVaSetValues(theMessagePanel->FormWidget->widget,	XmNbackground, ResourceData.ok_color, NULL);
	XtVaSetValues(theMessagePanel->PopupWidget->widget,	XmNforeground, ResourceData.black_color, NULL);
	XtVaSetValues(theMessagePanel->InfoWidget->widget,	XmNforeground, ResourceData.black_color, NULL);
	XtVaSetValues(theMessagePanel->FormWidget->widget,	XmNforeground, ResourceData.black_color, NULL);

        XtVaGetValues(XtParent(theMessagePanel->InfoWidget->widget), XmNverticalScrollBar, & scroll_bar, NULL);
        if(scroll_bar) XmChangeColor(scroll_bar, ResourceData.ok_color);

	*theMessagePanel->InfoWidget = msg;
	// debug
	// cout << "client errors: setting accumulate errors to true\n";
	errors::accumulate_errors(true);
	theMessagePanel->PopupWidget->manage();
	theMessagePanel->MoreButton->set_sensitive(False); } }

void client_errors::error_cancel(Widget, callback_stuff * client_data, void *) {
	client_errors		*obj = (client_errors *) client_data->data;

	if(obj == theErrorPanel) {
		errors::theErrorStack().clear();
		Processing = 0;
		MoreProcessing = 0; }
	// debug
	// cout << "client errors: setting accumulate errors to false\n";
	errors::accumulate_errors(false);
	obj->PopupWidget->unmanage(); }

void client_errors::error_add(Widget, callback_stuff * client_data, void *) {
	client_errors	*obj = (client_errors *) client_data->data;
	Cstring		new_stuff(get_error_message());

	*obj->InfoWidget = new_stuff;
	obj->MoreButton->set_sensitive(False);
	MoreProcessing = 0; }


// include file sets default for int argument:
Cstring client_errors::get_error_message() {
        strint_node* N;
	Cstring long_line("\n-------------------------------------------------------\n");
	int msg_length = 0;
	char* c;
	int i;

        for(	N = errors::theErrorStack().first_node();
		N;
		N = N->next_node()
           ) {
		if(N->payload != COMBINED_ERROR)
	                msg_length += N->get_key().length() + long_line.length();
		else	// N already ends with a long line...
			msg_length += N->get_key().length(); }

	c = (char *) malloc(msg_length+1); // Kluge added by BKL to deal with seq_review barrowed code difference
        for(i=0; i<msg_length; i++) c[i] = '#'; // filler
	c[i]='\0';
        Cstring ret((const char*) c);
	free(c);
	c = (char *) *ret; // BKL replaced ".start" with "*" and overloaded operator under Cstring

        for(	N = errors::theErrorStack().first_node();
		N;
		N = N->next_node()
           ) {
		if(N->payload != COMBINED_ERROR) {
	                strcpy(c , *N->get_key());
			c += N->get_key().length();
	                strcpy(c , *long_line);
			c += long_line.length();
		} else {
			// N already ends with a long line...
	                strcpy(c , *N->get_key());
			c += N->get_key().length();
		}
	}

	*c = '\0';

        for(    i = errors::theErrorStack().get_length();
		i >= 0;
		i--)
		delete errors::theErrorStack().last_node();

        return ret;
}

int client_errors::SeriousErrorsFound() {
	ref_node * N  = (ref_node *) errors::theErrorStack().first_node();

	for(; N; N = (ref_node *) N->next_node()) {
		if(N->get_id() == SERIOUS_ERROR) {
			return 1;
		}
	}
	return 0;
}
