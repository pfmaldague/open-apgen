#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <iostream>
#include <locale.h>

#include <gtkmm/main.h>
#include <gtkmm/style.h>

#include "gtk_bridge.H"
#include "apgen_editor.H"
#include "apgen_xmlwin.H"
#include "apgen_globwin.H"
#include "apgen_mainwin.H"

bool& copy_of_GTKdebug() {
	static bool b;
	return b;
}


mutex* gS::get_gtk_mutex() {
	static mutex  gtk_editor_mutex;
	return &gtk_editor_mutex;
}

//
// The purpose of the following globals is to define an
// interaction protocol between the main APGenX engine
// and the GTK thread. Access to these globals is
// controlled by *gS::get_gtk_mutex(). Both the engine
// and the GTK thread lock this mutex to access the
// protocol data and release it after reading and/or
// writing data.
//
// A second mutex, *gS::instruction_mutex(), is used to
// put the GTK thread on hold when no panel is currently
// active.
//
// See README.md for more information.
//

bool				gS::quit_requested = false;

				// assumed until proven false:
bool				gtk_editor_write_enabled = true;

extern bool			gtk_xmlwin_active;

static bool			GtkEditorDebug = false;
static bool			should_get_color = true;

double				editor_intfc::theDurationMultiplier = 1.0;

extern "C" {
	time_conv_func		TimeConversionFunc = NULL;
	dur_conv_func		DurConversionFunc = NULL;
	dur_update_func		DurUpdateFunc = NULL;
	colorfinder_func	ColorFunc = NULL;

	//
	// ParamParsingFunc is set to parameter_list_parser,
	// a function implemented in atm_client/gtk_editor_bridge.C
	// which collects information from the apgen_params panel
	// and sends it to the core.
	//
	param_parsing_func	ParamParsingFunc = NULL;

	char*			(*TypeDefFunc)() = NULL;
	char*			(*ParamDefFunc)() = NULL;

	char*			gtk_error_to_report = NULL;
} /* extern "C" */

using namespace Gtk;

std::map<string, editor_config>& editor_intfc::get_default_choices() {
	static std::map<string, editor_config> theMap;
	
	return theMap;
}

apgen_editor*&	apgen_editor::theWindow() {
	static apgen_editor* w = NULL;
	return w;
}

// bool		apgen_editor::never_shown = true;

void apgen_editor::prepareToShow() {

    //
    // STEP 1. Start the Gtk system - done by the caller.
    //

    //
    // STEP 2. Clean up edited data that may have been left over
    // 	       from a previous session
    //
    editor_intfc::theDurationMultiplier = 1.0;
    gS::gtk_editor_array& currentvals = editor_intfc::get_interface().theCurrentValues;
    gS::gtk_editor_array::iterator i;
    currentvals.erase(currentvals.begin(), currentvals.end());

    //
    // STEP 3. Clean up current path-indexed pointers to edited data.
    //	       theEditedValues will be filled again by
    //	       apgen_editor::fill_TreeModel().
    //
    std::map<string, gS::ISL>& editedvals
		= editor_intfc::get_interface().theEditedValues;

    //
    // NOTE: Don't delete the pointees. We store pointers that
    // reference items in theCurrentValues.
    //
    editedvals.erase(editedvals.begin(), editedvals.end());

    //
    // STEP 4. Copy the 'official' values, stored in Everything,
    // into theCurrentValues. Note that these official values
    // were computed by fire_up_gtk_editor().
    //
    gS::gtk_editor_array& officialvals = editor_intfc::get_interface().Everything;

    for(	i = officialvals.begin();
		i != officialvals.end();
		i++) {

	//
	// copy() actually copies the contents, not just the pointer.
	//
	gS::indexed_string_or_list* isl = (*i)->copy();
	currentvals.push_back(gS::ISL(isl));
    }


    // debug
    // cerr << "display_gtk_window(): creating GTK editor\n";

    //
    // STEP 5. Create the editor and fill it with activity data
    //
    if(!theWindow()) {

	theWindow() = new apgen_editor;

	//
	// Boolean arg is whether to only fill parameters
	//
	theWindow()->fillWithActivityData(/* first time = */ true, /* params only = */ false);
    } else {
	theWindow()->clear_tables();

	//
	// 2nd Boolean arg is whether to only fill parameters
	//
	theWindow()->fillWithActivityData(false, false);
    }


    //
    // STEP 6. Invoke the gtk main loop. This call blocks until the user
    // quits, which is why we need a separate thread to run this code.
    //

    //
    // This is going to be the main problem - we must implement the
    // re-initialization of the widget through a callback, because this
    // loop will last forever.
    //
    // Actually, there is no callback available for this. The only way I
    // can think of is to have a timer ping apgen repeatedly for whether
    // or not there is a new activity to be edited. If so, the timer
    // should  hide the editor window; this kicks Gtk out of the Main
    // loop. Come to think of it, we can keep the code pretty much as it
    // was - just re-initialize the window! Of course it would be less
    // wasteful to reset all the appropriate widgets.
    //
}
void apgen_editor::set_legends_in_combo(apgen_editor& AE) {
	AE.m_legend_combo.set_popdown_strings(editor_intfc::get_interface().theLegends);
}

void apgen_editor::fill_Table(int &current_row) {
	m_Table.attach(	m_name_label,
			0,		1,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(m_name_entry,
			1,		3,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
		
	m_Table.attach(	m_ID_label,
			3,		4,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_ID_entry,
			4,		6,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	current_row++;

	m_type_label.set_text(editor_intfc::get_interface().type_label.c_str());
	m_Table.attach(	m_type_label,
			0,		1,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_type_entry,
			1,		3,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_type_button,
			4,		6,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_type_button.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_editor::on_button_show_def));
	current_row++;

	m_Table.attach(	m_start_label,
			0,		1,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_start_entry,
			1,		6,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	current_row++;
	m_Table.attach(	m_duration_label,
			0,		1,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_duration_entry,
			1,		3,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);

	m_Table.attach(	m_epoch_label,
			3,		4,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);

	mr_epoch = Gtk::ListStore::create(mc_epoch);
	m_epoch_combo.set_model(mr_epoch);
	list<string>::iterator	oneEpoch;
	for(	oneEpoch = editor_intfc::get_interface().theEpochs.begin();
		oneEpoch != editor_intfc::get_interface().theEpochs.end();
		oneEpoch++) {
		Gtk::TreeModel::Row row = *(mr_epoch->append());
		row[mc_epoch.m_col_string] = oneEpoch->c_str();
	}
	// m_epoch_combo.pack_start(mc_epoch.m_col_string);
	m_epoch_combo.set_text_column(mc_epoch.m_col_string);
	// m_epoch_combo.set_popdown_strings(editor_intfc::get_interface().theEpochs);
	// m_epoch_combo.set_value_in_list(false, true);

	m_epoch_combo.get_entry()->signal_changed().connect(
				sigc::mem_fun(	*this,
						&apgen_editor::on_epoch_selected));

	m_Table.attach(	m_epoch_combo,
			4,		6,
			current_row,	current_row + 1,
			Gtk::FILL, 	Gtk::FILL,
			5,		5);
	current_row++;

	m_Table.attach(	m_duration_formula_label,
			0,		1,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_duration_formula_entry,
			1,		6,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	current_row++;

	set_legends_in_combo(*this);
	m_legend_combo.set_value_in_list(false, true);

	m_Table.attach(	m_legend_label,
			0,		1,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_legend_combo,
			1,		3,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);

	list<string> theColors;
	int c_index = 0;
	const char *cname;
	while(1) {
		cname = ColorFunc(c_index);
		if(!cname) {
			break;
		}
		theColors.push_back(string(cname));
		c_index++;
	}
	if(!c_index) {
		should_get_color = false;
		m_color_combo.set_sensitive(false);
	} else {
		should_get_color = true;
		m_color_combo.set_sensitive(true);
		m_color_combo.set_popdown_strings(theColors);
		m_color_combo.set_value_in_list(false, true);
	}

	m_Table.attach(	m_color_label,
			3,		4,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_color_combo,
			4,		6,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	current_row++;

	m_toggle_dur_timesystem.signal_toggled().connect(
			sigc::mem_fun(*this, &apgen_editor::on_dur_toggled));
	m_toggle_redetail_on_apply.signal_toggled().connect(
			sigc::mem_fun(*this, &apgen_editor::on_redetail_toggled));

	m_Table.attach(	m_toggle_dur_timesystem,
			0,		4,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_toggle_redetail_on_apply,
			4,		6,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	current_row++;

	//
	// Create the description:
	//
	m_descr_buffer = Gtk::TextBuffer::create();
	m_TextView.set_buffer(m_descr_buffer);
	m_TextView.set_wrap_mode(Gtk::WRAP_WORD);
	DescriptionWindow.add(m_TextView);
	DescriptionWindow.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	m_Table.attach(	m_descr_label,
			0,		3,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	current_row++;

	//
	// Edit policy
	//
	m_type_entry.set_editable(false);
	m_ID_entry.set_editable(false);
	if(editor_intfc::get_interface().instance_duration_formula.length()) {
		m_duration_entry.set_editable(false);
	} else {
		m_duration_entry.set_editable(true);
	}
	m_duration_formula_entry.set_editable(false);
	m_epoch_combo.get_entry()->set_editable(false);
	timeSystemInUse = editor_intfc::get_interface().theEpochs.front().c_str();
	m_legend_combo.get_entry()->set_editable(false);
	m_color_combo.get_entry()->set_editable(false);
}

void apgen_editor::fill_Table2(int &current_row) {
	m_ParShowButton.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_editor::on_button_show_param));

	ParDescr_buffer = Gtk::TextBuffer::create();
	ParDescr_TextView.set_buffer(ParDescr_buffer);
	ParDescr_TextView.set_wrap_mode(Gtk::WRAP_WORD);
	ParDescrWin.add(ParDescr_TextView);

	//
	// Only show the scrollbars when they are necessary:
	//
	ParDescrWin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	//
	// STEP 4. Add Description Window to Frame
	//
	m_ParDescrValue.add(ParDescrWin);
}

//
// This method is static to allow reuse by global_editor.
//
// To understand this method, one must keep in mind that
// there are three distinct hierarchies at work here:
//
// - Otree, the original tree structure of the internal data
//
// - Dtree, the tree of strings that is displayed by the editor 
//
// - Itree, the virtual internal tree of activity-related values
//
// All three trees contain nodes that are (keyword, value) pairs.
// All three are stored in a map. Each node (_both_ keyword and value)
// are stored in a location indexed by its path. The path has the form
//
// 	label1:label2:...:labelN
//
// where labeln is the keyword of the n-th node in a path leading from
// the root of the tree to the node under consideration.
//
// The input parameters are as follows:
//
//	name		description
//	----		-----------
// 	the_tree_model	says where the GTK row should be stored
// 	this_row	specifies the row of the tree we are filling
// 	key_val_col 	class that specifies the column structure of
// 	  		the tree
// 	row_data	the (label, value) information for this row
//
// The output is implicit; it consists of
//
// 	- new information stored in the_tree_model
// 	- a new node in the editedvals map. The index of the node will
// 	  be the path to this node (in the sense of GTK) and the value
// 	  will be row_data.
//
void apgen_editor::fill_TreeModel(
		Glib::RefPtr<Gtk::TreeStore>&		the_tree_model,
		Gtk::TreeModel::Children::iterator	this_row,
		ModelColumns_style_1&			key_val_col,
		gS::ISL&				row_data,
		std::map<string, gS::ISL>&		editedvals) {
	const Gtk::TreeModel::Row&	row_to_fill(*this_row);
	static buf_struct		out_string = {NULL, 0, 0};
	gS::compact_int			infinite("INFINITE");
	gS::compact_int			finite(128);
	gS::compact_int*		ml = NULL;
	string				thePath = the_tree_model->get_string(this_row);
	string				theValue;
	const string&			theKey = row_data->get_key();

	if(row_data->is_list()) {
		ml = &finite;
	} else {
		ml = &infinite;
	}
	initialize_buf_struct(&out_string);

	//
	// For the value of the item, we always transfer_to_stream the stuff in row_data.
	// If it's a string, there is no limit. If it's an array, we impose a
	// limit and append three dots.
	//
	if(row_data->transfer_to_stream(&out_string, *ml)) {

		//
		// output has been truncated
		//
		concatenate(&out_string, "...");
	}
	theValue = out_string.buf;
	if(GtkEditorDebug) {
		cout << "fill_TreeModel: setting TreeModel[ " << thePath << " ] = "
			<< theValue << endl;
	}

	row_to_fill[key_val_col.m_col_keyword] = theKey.c_str();
	row_to_fill[key_val_col.m_col_value] = theValue.c_str();

	if(GtkEditorDebug) {
		cout << "Inserting pointer to " << theValue
			<< " into edited value at " << thePath << endl;
	}
	editedvals[thePath] = row_data;

	//
	// we are done if ISL was an array, but now we need to handle the case
	// of a list
	//

	if(row_data->is_list()) {

		//
		// recursively add the children
		//
		for(	int i = 0;
			i < row_data->get_array().size();
			i++) {
			Gtk::TreeModel::Children	theChildren = row_to_fill.children();
			Gtk::TreeModel::Children::iterator iter = the_tree_model->append(theChildren);
			// string			unquoted((*i)->get_key());

			// gS::remove_quotes_from(unquoted);
			fill_TreeModel(
					the_tree_model,
					iter,
					key_val_col,
					// unquoted,
					row_data->get_array()[i],
					editedvals);
		}
	}
}

void apgen_editor::fill_ButtonBox() {
	m_ButtonBox.set_layout(		Gtk::BUTTONBOX_SPREAD);
	m_ButtonBox.set_spacing(	20);
	m_ButtonBox.set_border_width(	5);

	m_ButtonBox.pack_start(	m_Button_OK,		Gtk::PACK_SHRINK);
	m_ButtonBox.pack_start(	m_Button_Apply,		Gtk::PACK_SHRINK);
	m_ButtonBox.pack_start(	m_Button_Cancel,	Gtk::PACK_SHRINK);

	// m_ButtonBox.set_layout(Gtk::BUTTONBOX_END);

	m_Button_Cancel.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_editor::on_button_cancel));
	m_Button_Apply.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_editor::on_button_apply));
	m_Button_OK.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_editor::on_button_ok));
}

	/* Idea: we could have bold labels in the table for distinguishing between
	 * attributes and parameters.
	 *
	 * How do you set the font for a label? Well, it's going to be painful. Starting
	 * with http://www.gtkmm.org/gtkmm2/docs/reference/html/classGtk_1_1Label.html#_details
	 * we are led to the following namespace::class::methods:
	 *
	 *
	 * substep 1.
	 *
	 * 	Use the following method to retrieve the Label's layout object:
	 *
	 *		Glib::RefPtr<Pango::Layout>	Gtk::Label::get_layout()
	 *
	 *	Once we have the Layout, we can ask for the context (next substep).
	 *
	 * substep 2.
	 *
	 * 	Use the following method to retrieve the layout's context object:
	 *
	 *		Glib::RefPtr<Pango::Context>	Pango::Layout::get_context() const
	 *
	 * 	Once we have the context, we can ask for the Font Description (next substep)
	 *
	 * substep 3.
	 *
	 * 	Use the following method to get the font from the context:
	 *
	 *		FontDescription Pango::Context::get_font_description() const
	 *
	 * substep 4.
	 *
	 * 	Use the following method to set the weight (e. g. to bold)
	 *
	 *		void		Pango::FontDescription::set_weight(Weight w)
	 *
	 *	Note that Pango::Weight as an enum with the following values:
	 * 
	 *		WEIGHT_ULTRALIGHT
	 *		WEIGHT_LIGHT
	 *		WEIGHT_NORMAL
	 *		WEIGHT_BOLD
	 *		WEIGHT_ULTRABOLD
	 *		WEIGHT_HEAVY
	 *
	 * 	We now have the FontDescription object we want.
	 *
	 * substep 5.
	 *
	 * 	Create a Pango::AttrFontDesc object (derived from the Pango::Attribute class)
	 * 	using a void constructor, then use the following method to set its font
	 * 	description:
	 *
	 *		void		Pango::AttrFontDesc::set_desc(const FontDescription& desc)
	 *
	 * substep 6.
	 *
	 *
	 * 	Create a Pango::AttrList object using the default constructor, and use the
	 * 	following method to insert the attribute we just created:
	 *
	 *		void		Pango::AttrList::insert(Attribute& attr)
	 *
	 * substep 7.
	 *
	 * 	Use the following method to apply the AttrList object we just created to the Label:
	 *
	 * 		void		Gtk::Label::set_attributes(Pango::AttrList&     attrs) 
	 *
	 * That's it! Simple, no??
	 */

void apgen_editor::set_description_sensitivity(bool T) {
	if(!T) {
		ParDescr_buffer->set_text("");
	}
	// Unfortunately, once the sensitivity has been set to false there seems to be no turning back!
	// ParDescrWin.set_sensitive(T);
	m_ParDescrLabel1.set_sensitive(T);
}

void apgen_editor::set_range_sensitivity(bool T) {
	m_ParRangeLabel.set_sensitive(T);
	m_ParRangeValue.set_sensitive(T);
	if(!T) {

		// Crashes the editor:
		// m_ParRangeValue.get_list()->children().erase(
		// 	m_ParRangeValue.get_list()->children().begin(),
		// 	m_ParRangeValue.get_list()->children().end());

		m_ParRangeValue.get_entry()->set_text("");
	}
}

void apgen_editor::set_name_sensitivity(bool T) {
	m_ParNameLabel.set_sensitive(T);
	m_ParNameValue.set_sensitive(T);
	if(!T) {
		m_ParNameValue.set_text("");
	}
}

apgen_error::apgen_error()
	: m_Button_Cancel("Cancel") {
	string	Title("APGEN Editor Error");

	set_title(Title.c_str());
	set_border_width(5);
	set_default_size(400, 300);
	add(m_VBox);
	m_ButtonBox.add(m_Button_Cancel);
	m_VBox.pack_start(m_Frame,	Gtk::PACK_EXPAND_WIDGET);
	m_VBox.pack_start(m_ButtonBox,	Gtk::PACK_SHRINK);
	m_VBox.set_homogeneous(false);
	m_buffer = Gtk::TextBuffer::create();
	m_TextView.set_buffer(m_buffer);
	m_TextView.set_wrap_mode(Gtk::WRAP_WORD);
	m_ScrolledWindow.add(m_TextView);
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	m_Frame.add(m_ScrolledWindow);
	m_Button_Cancel.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_error::on_button_cancel));
	show_all_children();
}

apgen_error::~apgen_error() {
}

void apgen_error::set_txt_to(const string &a) {
	m_buffer->set_text(a.c_str());
}

void apgen_error::on_button_cancel() {
	hide();
}

apgen_type::apgen_type()
	: m_Button_Cancel("Cancel") {
	string	Title("APGEN Type Definition");

	set_title(Title.c_str());
	set_border_width(5);
	set_default_size(600, 800);
	add(m_VBox);
	m_ButtonBox.add(m_Button_Cancel);
	m_VBox.pack_start(m_Frame,	Gtk::PACK_EXPAND_WIDGET);
	m_VBox.pack_start(m_ButtonBox,	Gtk::PACK_SHRINK);
	m_VBox.set_homogeneous(false);
	m_buffer = Gtk::TextBuffer::create();
	m_TextView.set_buffer(m_buffer);
	m_TextView.set_wrap_mode(Gtk::WRAP_WORD);
	m_ScrolledWindow.add(m_TextView);
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	m_Frame.add(m_ScrolledWindow);
	m_Button_Cancel.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_type::on_button_cancel));
	show_all_children();
}

apgen_type::~apgen_type() {
}

void apgen_type::set_txt_to(const string &a) {
	m_buffer->set_text(a.c_str());
}

void apgen_type::on_button_cancel() {
	hide();
}

//
// Special purpose window; this came as a result of
// an InSight requirement and may not be generic
// enough to be of interest.
//
apgen_param::apgen_param()
	: m_Button_OK("OK"),
		m_Button_Apply("Apply"),
		m_Button_Cancel("Cancel") {
	string	Title("APGEN Parameter Definitions");

	set_title(Title.c_str());
	set_border_width(5);
	set_default_size(600, 800);
	add(m_VBox);
	m_ButtonBox.set_layout(Gtk::BUTTONBOX_SPREAD);
	m_ButtonBox.pack_start(m_Button_OK, Gtk::PACK_SHRINK);
	m_ButtonBox.pack_start(m_Button_Apply, Gtk::PACK_SHRINK);
	m_ButtonBox.pack_start(m_Button_Cancel, Gtk::PACK_SHRINK);
	m_VBox.pack_start(m_Frame,	Gtk::PACK_EXPAND_WIDGET);
	m_VBox.pack_start(m_ButtonBox,	Gtk::PACK_SHRINK);
	m_VBox.set_homogeneous(false);
	m_buffer = Gtk::TextBuffer::create();
	m_TextView.set_buffer(m_buffer);
	m_TextView.set_wrap_mode(Gtk::WRAP_WORD);
	m_ScrolledWindow.add(m_TextView);
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	m_Frame.add(m_ScrolledWindow);
	m_Button_OK.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_param::on_button_ok));
	m_Button_Apply.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_param::on_button_apply));
	m_Button_Cancel.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_param::on_button_cancel));
	show_all_children();
}

apgen_param::~apgen_param() {
}

void apgen_param::set_txt_to(const string &a) {
	m_buffer->set_text(a.c_str());
}

void apgen_param::on_button_ok() {
	on_button_apply();
	hide();
}

void apgen_param::on_button_apply() {
	string edited_params = m_buffer->get_text();
	char* the_params = strdup(edited_params.c_str());
	char* errors;

	//
	// See comments on ParamParsingFunc at the top
	// of this file.
	//
	if(ParamParsingFunc(the_params, &errors)) {
		string the_errors = errors;
		apgen_editor::theWindow()->report_error(the_errors);
		free(errors);
	}
	free(the_params);

	string	path = "2";
	Gtk::TreeModel::iterator	iter =
				apgen_editor::theWindow()->m_refTreeModel->get_iter(path);
	apgen_editor::theWindow()->m_refTreeModel->erase(iter);
	path = "1";
	iter = apgen_editor::theWindow()->m_refTreeModel->get_iter(path);
	apgen_editor::theWindow()->m_refTreeModel->erase(iter);
	path = "0";
	iter = apgen_editor::theWindow()->m_refTreeModel->get_iter(path);
	apgen_editor::theWindow()->m_refTreeModel->erase(iter);
	
	apgen_editor::theWindow()->clear_tables();

	apgen_editor::theWindow()->fillWithActivityData(false, true);
}

void apgen_param::on_button_cancel() {
	hide();
}

apgen_editor::apgen_editor()
	: m_Button_Cancel("Cancel"),
		m_Button_Apply("Apply"),
		m_Button_OK("OK"),
		m_type_button("Show Definition"),
		m_name_label("Name: "),
		m_ID_label("ID: "),
		m_type_label("Type: "),
		m_start_label("Start: "),
		m_epoch_label("Time Sys.: "),
		m_duration_label("Duration: "),
		m_duration_formula_label("Dur. Formula: "),
		m_legend_label("Legend: "),
		m_color_label("Color: "),
		m_toggle_dur_timesystem("Allow non-UTC Duration (Editor only)"),
		m_toggle_redetail_on_apply("Regen Children on OK"),
		m_descr_label("Activity Description: "),
		m_renderer(new Gtk::CellRendererText()),
		m_ParNameLabel("Name: "),
		m_ParRangeLabel("Value/Range: "),
		m_ParEdit(" Params.: "),
		m_ParShowButton("Show As Text"),
		m_ParDescrLabel1("Parameter/Attribute Description:"),
		m_Table(6, 4, true), // args: (int) rows, (int) columns, (bool) is_homogeneous
		theAPerror(NULL),
		theAPparam(NULL),
		theAPtype(NULL) {
	int	current_row = 0;
	string	Title("APGEN Editor Version ");

	//
	// Set basic parameters
	//

	//
	// defined in config.h:
	//
	Title = Title + VERSION;
	set_title(Title.c_str());
	set_border_width(5);
	set_default_size(600, 700);

	//
	// Handle font problems, if possible - turns out the only
	// thing that works on the Mac is .gtkrc-2.0 in the user's
	// home directory. On Linux, it also works for colors, but
	// not for font size...
	//
	Glib::RefPtr<Style> the_style { get_style() };
	Pango::FontDescription FD { the_style->get_font() };
	int font_size = FD.get_size();

	//
	// "absolute" means "device units"
	//
	if(!FD.get_size_is_absolute()) {
		font_size = font_size / PANGO_SCALE;
	}

	//
	// Attempts to set font size this way have failed:
	//
	// cout << "apgen editor: font size = " << font_size << "\n";
	// FD.set_absolute_size(font_size);
	// the_style->set_font(FD);
	// set_style(the_style);

	//
	// Recall that we defined the following widgets as class members:
	// 	VBox		m_VBox
	// 	ScrolledWindow	m_ScrolledWindow
	// 	ScrolledWindow	DescriptionWindow
	//	HButtonBox	m_ButtonBox
	//	Button		m_ButtonQuit
	//

	//
	// STEP 1. Add the vertical box (already initialized as a data member)
	// to this window
	//
	add(m_VBox);

	//
	// STEP 2. Define m_VBox's 1st child, a Table. This table contains
	// all the top items of the editor: labels and text fields for
	// activity name, ID, start time, duration, epoch selection, color,
	// description.
	//
	fill_Table(current_row);

	//
	// STEP 3. Define TreeView and add it to the ScrolledWindow. This is
	// the main tree of attributes and parameters, editable by the user.
	//
	m_ScrolledWindow.add(m_TreeView);

	//
	// Only show the scrollbars when they are necessary:
	//
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	//
	// STEP 4. Add Description Window to Frame. This window will
	// be used to display information relative to specific
	// parameters.
	//
	m_Frame.add(DescriptionWindow);
	m_Frame2.add(m_ScrolledWindow);

	//
	// STEP 5. Combine the description and tree windows into the paned widget.
	//
	m_VPaned.pack1(	m_Frame,	/* resize */ true, /* shrink */ true	);
	m_VPaned.pack2(m_Frame2,	/* resize */ true, /* shrink */ false	);
	m_VPaned.set_position(40);

	//
	// STEP 5bis. Descriptions and ranges
	//
	current_row = 0;
	fill_Table2(current_row);

    	m_HBox2.pack_start(m_ParNameLabel,	Gtk::PACK_SHRINK);
	m_HBox2.pack_start(m_ParNameValue,	Gtk::PACK_EXPAND_WIDGET);
	m_HBox2.pack_start(m_ParRangeLabel,	Gtk::PACK_SHRINK);
	m_HBox2.pack_start(m_ParRangeValue,	Gtk::PACK_EXPAND_WIDGET);
	m_HBox2.pack_start(m_ParEdit,		Gtk::PACK_SHRINK);
	m_HBox2.pack_start(m_ParShowButton,	Gtk::PACK_EXPAND_WIDGET);
	m_HBox2.set_homogeneous(false);

	m_HBox3.pack_start(m_ParDescrLabel1,	Gtk::PACK_EXPAND_PADDING);

	m_HBox4.pack_start(m_ParDescrValue,	Gtk::PACK_EXPAND_WIDGET);

	//
	// STEP 6. Collect all the pieces into the main VBox
	//
	m_VBox.pack_start(m_Table,		Gtk::PACK_SHRINK	);
	m_VBox.pack_start(m_VPaned, 		Gtk::PACK_EXPAND_WIDGET	);
	m_VBox.pack_start(m_HBox2,		Gtk::PACK_SHRINK, /* padding = */ 5	);
	m_VBox.pack_start(m_HBox3,		Gtk::PACK_SHRINK, /* padding = */ 5	);
	m_VBox.pack_start(m_HBox4,		Gtk::PACK_SHRINK	);
	m_VBox.pack_start(m_ButtonBox,		Gtk::PACK_SHRINK	);
	m_VBox.set_homogeneous(false);

	set_name_sensitivity(false);
	set_range_sensitivity(false);
	set_description_sensitivity(false);

	m_ParNameValue.set_editable(false);

	mr_ParRangeValue = Gtk::ListStore::create(mc_ParRangeValue);
	m_ParRangeValue.set_model(mr_ParRangeValue);
	m_ParRangeValue.get_entry()->set_editable(false);
	m_ParRangeValue.get_entry()->signal_changed().connect(
			sigc::mem_fun(	*this,
					&apgen_editor::on_par_range_selected));

	ParDescr_TextView.set_editable(false);

	//
	// STEP 7. Fill the button box with buttons.
	//
	fill_ButtonBox();

	//
	// STEP 8. Important Details for the tree: create the model for it.
	//

	// Sub-step 8.A. Create model.

	/********************************************************************************
	 *										*
	 * 				TREE MODEL					*
	 * 										*
	 *******************************************************************************/

	// m_refTreeModel = Gtk::TreeStore::create(KeywordValueColumns);
	// m_TreeView.set_model(m_refTreeModel);

	// Sub-step 8.B. Set up the selection callback.

	//
	// Define the selection callback as per the following URL:
	// http://www.gtkmm.org/gtkmm2/docs/tutorial/html/ch08s04.html#id2844095
	//
	refTreeSelection = m_TreeView.get_selection();
	refTreeSelection->signal_changed().connect(
		sigc::mem_fun(*this, &apgen_editor::on_selection_changed));
	refTreeSelection->set_select_function(
				sigc::mem_fun(
					*this,
					&apgen_editor::select_function));

	//
	// all done for the construction of the editor
	//
}

void delete_gtk_editor() {
	if(apgen_editor::theWindow()) {
		delete apgen_editor::theWindow();
		apgen_editor::theWindow() = NULL;
	}
}

void apgen_editor::clear_tables() {

	//
	// this method does not exist - hopefully
	// clearing the tree model is enough:
	//
	//	m_TreeView.clear();
	//
	m_refTreeModel->clear();
	m_epoch_combo.get_model().clear();
	m_ParRangeValue.get_model().clear();
}

//
// This method fills the main tree of attribute and parameter
// information with the required values. If params_only is
// true, that's all it does. Else, it also fills all the
// fields at the top of the editor window: name, ID etc.
//
void apgen_editor::fillWithActivityData(
		bool first_time_called,
		bool params_only) {

	// debug
	// cerr << "fillWithActivityData() start; first time: "
	// 	<< first_time_called << "\n";

	if(!params_only) {

		//
		// do all the easy stuff:
		//
		m_name_entry.set_text(editor_intfc::get_interface().instance_name.c_str());
		m_ID_entry.set_text(editor_intfc::get_interface().instance_id.c_str());
		m_type_entry.set_text(editor_intfc::get_interface().type_name.c_str());
		m_start_entry.set_text(editor_intfc::get_interface().instance_start.c_str());
		m_duration_entry.set_text(editor_intfc::get_interface().instance_duration.c_str());
		m_duration_formula_entry.set_text(editor_intfc::get_interface().instance_duration_formula.c_str());
		m_legend_combo.get_entry()->set_text(editor_intfc::get_interface().instance_legend.c_str());
		m_color_combo.get_entry()->set_text(editor_intfc::get_interface().instance_color.c_str());
		m_descr_buffer->set_text(editor_intfc::get_interface().instance_description.c_str());
	}

	if(first_time_called) {
		m_refTreeModel = Gtk::TreeStore::create(KeywordValueColumns);
		m_TreeView.set_model(m_refTreeModel);
	}


	//
	// Sub-step 8.C. Fill the TreeView's model with Attributes.
	//
	// NOTE: the calls to foll_TreeModel() below will cause pointers to all tree nodes
	// below the top level in theCurrentValues to be stored in a string-based map
	// (theEditedValues) that will be used to update the appearance of the tree
	// as we go.
	//

	//
	// Here we need to define all the attributes that apply to the instance being edited.
	// For the time being we've hard-coded a few instances. More generally, we need
	// to extract the actual attributes from the editor interface object. Let's at
	// least see if we can list everything that was put in there by gtk_bridge.C.
	//
	gS::gtk_editor_array&	attrs(editor_intfc::get_interface().theCurrentValues[0]->get_array());
	gS::gtk_editor_array&	parms(editor_intfc::get_interface().theCurrentValues[1]->get_array());
	gS::gtk_editor_array&	locls(editor_intfc::get_interface().theCurrentValues[2]->get_array());

	if(GtkEditorDebug) {
		static buf_struct	B = {NULL, 0, 0};

		//
		// iterate over theAttributes, a gtk_editor_array (vector)
		//
		cout << "\nAttributes:\n";
		for(int i = 0; i < attrs.size(); i++) {
			gS::compact_int	ml(128);
			cout << attrs[i]->get_key() << ": ";
			initialize_buf_struct(&B);
			if(attrs[i]->transfer_to_stream(&B, ml)) {

				//
				// output has been truncated
				//
				concatenate(&B, "...");
				cout << B.buf;
			} else {
				cout << B.buf;
			}
		}
		
		cout.flush();
	}

	/********************************************************************************
	 *										*
	 * 				TREE MODEL 1: ATTRIBUTES			*
	 * 										*
	 *******************************************************************************/

	// Create a (first) new row.
	Gtk::TreeModel::Row top_level_row = *(m_refTreeModel->append());
	top_level_row[KeywordValueColumns.m_col_keyword] = "Attributes";
	top_level_row[KeywordValueColumns.m_col_value] = "";

	for(int i = 0; i < attrs.size(); i++) {
		Gtk::TreeModel::Children		theChildren = top_level_row.children();
		Gtk::TreeModel::Children::iterator	this_row = m_refTreeModel->append(theChildren);

		//
		// Store the appropriate data in this row and
		// add children row(s) as necessary
		//
		fill_TreeModel(
			m_refTreeModel,
			this_row,
			KeywordValueColumns,
			attrs[i],
			editor_intfc::get_interface().theEditedValues);
	}

	gtk_editor_write_enabled = true;

	if(gtk_editor_write_enabled) {
		m_Button_OK.set_sensitive(true);
		m_toggle_redetail_on_apply.set_sensitive(true);
		m_Button_Apply.set_sensitive(true);
	} else {
		m_Button_OK.set_sensitive(false);
		m_toggle_redetail_on_apply.set_sensitive(false);
		m_Button_Apply.set_sensitive(false);
	}

	if(GtkEditorDebug) {
		static buf_struct	B = {NULL, 0, 0};

		//
		// iterate over theAttributes, a gtk_editor_array (map)
		//
		cout << "\nParameters:\n";
		for(int k = 0; k < parms.size(); k++) {
			gS::compact_int	ml(128);

			cout << parms[k]->get_key() << ": ";
			initialize_buf_struct(&B);
			if(parms[k]->transfer_to_stream(&B, ml)) {
				cout << B.buf;

				//
				// output has been truncated
				//
				cout << "...";
			} else {
				cout << B.buf;
			}
			cout << endl;
		}
		cout.flush();
	}


	/********************************************************************************
	 *										*
	 * 				TREE MODEL 2: PARAMETERS			*
	 * 										*
	 *******************************************************************************/

	// Create a (second) new row.
	top_level_row = *(m_refTreeModel->append());
	top_level_row[KeywordValueColumns.m_col_keyword] = "Parameters";
	top_level_row[KeywordValueColumns.m_col_value] = "";

	for(int k = 0; k < parms.size(); k++) {
		Gtk::TreeModel::Children		theChildren = top_level_row.children();
		Gtk::TreeModel::Children::iterator	this_row = m_refTreeModel->append(theChildren);

		//
		// Store the appropriate data in this row and add children
		// row(s) as necessary. We pass the iterator rather than the
		// row so we can get the path easily.
		//
		fill_TreeModel(	m_refTreeModel,
				this_row,
				KeywordValueColumns,
				parms[k],
				editor_intfc::get_interface().theEditedValues);
	}

	if(GtkEditorDebug) {
		static buf_struct B = {NULL, 0, 0};
		// iterate over theAttributes, a gtk_editor_array (map)
		cout << "\nLocal Variables:\n";
		for(int k = 0; k < locls.size(); k++) {
			gS::compact_int	ml(128);
			cout << locls[k]->get_key() << ": ";
			initialize_buf_struct(&B);
			if(locls[k]->transfer_to_stream(&B, ml)) {
				// output has been truncated
				cout << B.buf;
				cout << "...";
			} else {
				cout << B.buf;
			}
			cout << endl;
		}
		cout.flush();
	}

	/********************************************************************************
	 *										*
	 * 				TREE MODEL 3: LOCAL VARIABLES			*
	 * 										*
	 *******************************************************************************/

	// Create a (third) new row. NOTE: this goes beyond requirements. May have to take it out.
	top_level_row = *(m_refTreeModel->append());
	top_level_row[KeywordValueColumns.m_col_keyword] = "Local Variables";
	top_level_row[KeywordValueColumns.m_col_value] = "";

	for(int j = 0; j < locls.size(); j++) {
		Gtk::TreeModel::Children		theChildren = top_level_row.children();
		Gtk::TreeModel::Children::iterator	this_row = m_refTreeModel->append(theChildren);

		// Store the appropriate data in this row and add children
		// row(s) as necessary. We pass the iterator rather than the
		// row so we can get the path easily.
		fill_TreeModel(	m_refTreeModel,
				this_row,
				KeywordValueColumns,
				locls[j],
				editor_intfc::get_interface().theEditedValues);
	}

	/********************************************************************************
	 *										*
	 * 				TREE VIEW					*
	 * 										*
	 *******************************************************************************/

	// STEP 9. Add the TreeView's 2 view columns.

	if(first_time_called) {

		//
		// First column is standard.
		//
		m_TreeView.append_column("Name", KeywordValueColumns.m_col_keyword);

		//
		// For the 2nd column, we want explicit control over which rows are
		// editable.
		//

		Gtk::TreeViewColumn*	const m_column = new Gtk::TreeViewColumn(
						"Value",
						*Gtk::manage(m_renderer.get()));
		m_TreeView.append_column(*Gtk::manage(m_column));
		m_column->add_attribute(	m_renderer->property_text(),
						KeywordValueColumns.m_col_value);
		m_renderer->property_editable() = true;
		m_renderer->signal_edited().connect(sigc::mem_fun(
						*this,
						&apgen_editor::on_cell_edited));
	}

	// STEP 10. Expand both attributes and parameters at the top level.

	Gtk::TreePath m_path("0");
	m_TreeView.expand_row(m_path, /* open_all */ false);
	m_path = Gtk::TreePath("1");
	m_TreeView.expand_row(m_path, /* open_all */ false);
	m_path = Gtk::TreePath("2");
	m_TreeView.expand_row(m_path, /* open_all */ false);

	if(!params_only) {
		/* Search the tree of default choices with the activity type
		 * as an index */
		std::map<string, editor_config>::iterator
			editor_config_map_it = editor_intfc::get_default_choices().find(
					editor_intfc::get_interface().type_name);

		if(editor_config_map_it != editor_intfc::get_default_choices().end()) {
			if(editor_config_map_it->second.redetail_needed == true) {
				eS::gtk_editor_requests_redetailing = true;
				m_toggle_redetail_on_apply.set_active(true);
			} else {
				m_toggle_redetail_on_apply.set_active(false);
			}
		} else {
			eS::gtk_editor_requests_redetailing = false;
			m_toggle_redetail_on_apply.set_active(false);
		}

		if(default_time_system) {
			string		deftim(default_time_system);
	
			list<string>&	the_epochs = editor_intfc::get_interface().theEpochs;
			list<string>::iterator it;
	
			for(it = the_epochs.begin(); it != the_epochs.end(); it++) {
				if((*it) == deftim) {
					// found it
					m_epoch_combo.get_entry()->set_text(deftim.c_str());
					carry_out_time_conversions(deftim.c_str());
					break;
				}
			}
		}
	}

	show_all_children();
}

apgen_editor::~apgen_editor() {
	// debug
	// cerr << "~apgen_editor(): creating GTK editor - locking mutex...";

	lock_guard<mutex> lock1(*gS::get_gtk_mutex());

	// debug
	// cerr << " got it; releasing it.\n";

	eS::panel_active = false;
}

bool apgen_editor::select_function(	const Glib::RefPtr<Gtk::TreeModel>& model,
					const Gtk::TreeModel::Path& path,
					bool) {
	const Gtk::TreeModel::iterator	iter = model->get_iter(path);
	Gtk::TreeModel::Row		row = *iter;
	Glib::ustring			theKey = row[KeywordValueColumns.m_col_keyword];

	// only allow leaf nodes to be edited
	if(	theKey != "Attributes"
		&& theKey != "Parameters"
		&& iter->children().empty()) {
		m_renderer->property_editable() = true;
	} else {
		m_renderer->property_editable() = false;
	}
	return true;
}

void apgen_editor::on_button_cancel() {

	// debug
	// cerr << "on_button_cancel(): hiding the GTK editor - locking mutex...";

	lock_guard<mutex> lock1(*gS::get_gtk_mutex());

	// debug
	// cerr << " got it.\n";

    	// gtk_editor_active = false;

	//
	// This may cause run() to return in display_gtk_window():
	//
	hide_all();
	// debug
	// cerr << "on_button_cancel(): releasing the lock.\n";
}

void apgen_editor::notify_apgen(bool apply_was_used) {

    if(gtk_editor_write_enabled) {

	Gtk::TreeNodeChildren the_children = m_refTreeModel->children();

	//
	// Collect info from the GTK panel and
	// stuff it into the interface
	//
	editor_intfc::get_interface().instance_name = m_name_entry.get_text();
	editor_intfc::get_interface().instance_start = m_start_entry.get_text();

	if(editor_intfc::get_interface().instance_duration_formula.length() == 0) {
		editor_intfc::get_interface().instance_duration
			= m_duration_entry.get_text();
	}

	editor_intfc::get_interface().instance_legend =
		m_legend_combo.get_entry()->get_text();

	if(should_get_color) {
		editor_intfc::get_interface().instance_color =
			m_color_combo.get_entry()->get_text(); }

	editor_intfc::get_interface().instance_description =
		m_descr_buffer->get_text();

	{
		// debug
		// cerr << "notify_apgen(): editor data available - locking mutex...";

		lock_guard<mutex> lock1(*gS::get_gtk_mutex());

		// debug
		// cerr << " got it.\n";

		if(copy_of_GTKdebug()) {
			cerr << "notify_apgen: got gtk_editor...\n";
		}
		eS::gtk_editor_data_available = true;
		// gtk_editor_user_pressed_apply = apply_was_used;

		if(copy_of_GTKdebug()) {
			cerr << "notify_apgen: unlocking gtk_editor.\n";
		}
	}
	gS::send_unblocking_event_to_motif();

	// debug
	// cerr << "notify_apgen(): released the lock and sent synthetic event.\n";

    }
    if(!apply_was_used) {
	hide_all();
    }
}

void apgen_editor::on_button_apply() {
	notify_apgen(true);
}

void apgen_editor::on_button_ok() {
	notify_apgen(false);

	// debug
	// cerr << "on_button_ok(): hiding the activity editor; locking mutex...";

	lock_guard<mutex> lock1(*gS::get_gtk_mutex());

	// debug
	// cerr << " got it.\n";

    	// gtk_editor_active = false;

	// debug
	// cerr << "on_button_ok(): released the lock.\n";
}

//
// Invoked when the user clicks on a tree node that
// now becomes selected. Other things in the panel
// need to reflect this choice.
//
void apgen_editor::on_selection_changed() {
    TreeModel::iterator iter = refTreeSelection->get_selected();

    //
    // If anything is selected:
    //
    if(iter) {
	TreeModel::Row	row = *iter;
	std::map<string, gS::ISL>::iterator C;

	//
	// This is where we are going to grab all
	// the data that needs to be displayed as
	// a result of the user selecting this
	// particular tree element:
	//
	std::map<string, gS::ISL>& editedvals
				= editor_intfc::get_interface().theEditedValues;

	//
	// Do something with the row. To do this we use Row::operator []
	//
	Glib::ustring thePath = m_refTreeModel->get_string(iter);
	Glib::ustring theValue = row[KeywordValueColumns.m_col_value];

	//
	// How do we get descriptions? Remember that the descriptions are
	// stored in the Description member of the indexed_string_or_list_class,
	// and that the gtk_editor_array structure(s) passed by the core
	// to the gtk editor is a vector of pointers to indexed_string_or_list
	// objects. Not only that, but when the gtk editor initializes itself,
	// it stores pointers to these objects into a map indexed by the path:
	//
	// 	editedvals[thePath] = row_data;
	//
	C = editedvals.find(thePath);
	if(C != editedvals.end()) {
	    set_name_sensitivity(true);
	    m_ParNameValue.set_text((*C).second->Key.c_str());
	    if((*C).second->Description.length()) {
		set_description_sensitivity(true);
		ParDescr_buffer->set_text((*C).second->Description.c_str());
	    } else {
		ParDescr_buffer->set_text("");
		set_description_sensitivity(false);
	    }

	    //
	    // added PFM 08/03/2012 as Steve Wissler noticed
	    // "growing list"
	    //
	    mr_ParRangeValue->clear();
	    if(!(*C).second->Range.empty()) {
		set_range_sensitivity(true);
		PathToWhichRangeApplies = (*C).first;

		list<string>&		the_range((*C).second->Range);
		list<string>::iterator	an_item;
		for(	an_item = the_range.begin();
			an_item != the_range.end();
			an_item++) {
		    Gtk::ListStore::Row	row = *(mr_ParRangeValue->append());
		    row[mc_ParRangeValue.m_col_string] = an_item->c_str();
		}
		m_ParRangeValue.set_text_column(mc_ParRangeValue.m_col_string);

		// m_ParRangeValue.set_popdown_strings((*C).second->Range);
		// m_ParRangeValue.set_value_in_list(false, true);
		m_ParRangeValue.get_entry()->set_text(theValue);

	    } else {
		set_range_sensitivity(false);
	    }
	}
	else {
	    set_name_sensitivity(false);
	    set_range_sensitivity(false);
	    set_description_sensitivity(false);
	}
    }
}

//
// Invoked when the user enters a new definition of a
// tree node (which is restricted to be a leaf; intermediate
// nodes are displayed with a short content string that is
// computed by the editor and is not editable by the user.)
//
// Note added Dec. 2019: this method was reworked to invoke a static
// method that is generic enough for another panel (the Globals Editor)
// to use it also. All the specifics required by the algorithms are now
// collected by the static method's API:
//
// 	- original parameters:
// 		path
// 		new_text
// 	- editedvals:
// 		this is a map which provides the value string to
// 	  	be displayed for every Path (in the sense of the gtk tree
// 	  	model) in the tree. Fortunately, the algorithm is completely
// 	  	generic in the sense that it does not matter what the
// 	  	top-level labels are. These labels are as shown below.
//
// 	  		apgen_editor:
// 	  			Attributes
// 	  			Parameters
// 	  			Local Variables
// 	  		glolal_editor:
// 	  			Globals
//
// What allows the algorithm to ignore the label of the top-level paths
// is that the top-level rows only have a 'key' entry; the value is the
// empty string, which does not need to be updated.
//
void apgen_editor::on_cell_edited(
			const Glib::ustring& path,
			const Glib::ustring& edited_text) {
	Gtk::TreeModel::Path		tree_path(path);

	//
	// This is where we are going to stuff the values
	// extracted from the gtk panel. Remember, though,
	// that all these collected values are strings
	// only. It is only when pushing Apply or OK that
	// the editor will communicate these strings to
	// the core, which will then parse them into bona
	// fide TypedValue objects.
	//
	std::map<string, gS::ISL>&		editedvals
		    = editor_intfc::get_interface().theEditedValues;

	//
	// Invoke the generic static method
	//
	UpdateTreeModel(
		m_refTreeModel,
		tree_path,
		edited_text,
		editedvals,
		KeywordValueColumns);

	char	*theNewValue, *theErrors;
	if(	DurUpdateFunc(&theNewValue, &theErrors)
		&& !theErrors) {
		Glib::ustring text(theNewValue);

		m_duration_entry.set_text(text);
		free(theNewValue);
		if(m_toggle_dur_timesystem.get_active()) {
			update_the_duration(timeSystemInUse);
		}
	}
	if(theErrors) {

		//
		// need to lock, set error flag
		//
		report_error(string(theErrors));
		free(theErrors);
	}
}

//
// Generic static method, usable by global_editor
//
void apgen_editor::UpdateTreeModel(
		Glib::RefPtr<Gtk::TreeStore>&	ref_tree_model,
		Gtk::TreeModel::Path&		the_tree_path,
		const Glib::ustring&		new_text,
		std::map<string, gS::ISL>&	values,
		ModelColumns_style_1&		kw_columns) {

	bool				we_have_a_leaf = true;
	TreeModel::iterator		iter;
	Glib::ustring			thePath;
	Glib::ustring			theValue;
	Gtk::TreeModel::Row		row;

	//
	// We propagate the change made by the user up the array chain.
	// We have a path to the cell, so finding the chain is not a problem.
	// Need to keep a global map<string, string> indexed by the path.
	//

	//
	// 1. Set the text for the item that was just edited. NOTE: this takes
	// care of the visual appearance as well as the tree model, but does
	// NOT update theCurrentValues.
	//

	//
	// 2. Recursively update parent rows. What is clearly needed is a way to
	// reconstruct the "string appearance" (including truncation) of any node
	// in the tree of displayed attributes and parameters.
	//
	// Clearly, to update an internal node of the tree, we want to use method
	//
	// 	indexed_string_or_list::transfer_to_stream(ostream &s, int &max_L)
	//
	// But that method takes advantage of the fact that, if the node in question
	// is an array, it has pointers (through a vector) to all the subnodes it
	// needs. Thus, by definition, we need to store the temporary values in a
	// structure similar to that used for the 'official' quantities being edited.
	//
	std::map<string, gS::ISL>::iterator C;
	while(1) {
		iter = ref_tree_model->get_iter(the_tree_path);
		row = *iter;
		thePath = ref_tree_model->get_string(iter);

		//
		// thePath is a colon-separated list of indices
		//
		if(thePath.find(":") == string::npos) {

			//
			// No colon found
			//

			//
			// We have reached the top level (no colon)
			// (this is a row that says "Attributes", "Parameters"
			// or "Local Variables".)
			//
			// If the cell that was just edited belonged to a
			// parameter, we should update the duration in case
			// it depends on the parameters.
			//
			break;
		}

		//
		// get the interface node for this row
		//
		C = values.find(thePath);
		if(C != values.end()) {
			if(we_have_a_leaf) {

				//
				// only applies to the first iteration
				//
				we_have_a_leaf = false;

				//
				// update theCurrentValues
				//
				(*C).second->set_string_to(new_text);
				row[kw_columns.m_col_value] = new_text;
			} else {

				//
				// We are dealing with an internal node of
				// the tree model. We need to update the string
				// that represents the value at this spot in
				// the tree; we do this by invoking the
				// transfer_to_stream() method.
				//
				gS::compact_int		ml(128);
				static buf_struct	out_string;

				initialize_buf_struct(&out_string);
				if((*C).second->transfer_to_stream(&out_string, ml)) {
					concatenate(&out_string, "..."); }
				theValue = out_string.buf;
				row[kw_columns.m_col_value] = theValue;
			}
		} else {

			//
			// Internal error... "should never happen"
			//
			cerr << "Hmmm... path " << thePath.c_str()
				<< " not found in theEditedValues.\n";
		}

		//
		// bug in TreePath::up(): it returns true when it should
		// return false; therefore, we will not rely on the returned value.
		//
		// while(the_tree_path.up())
		the_tree_path.up();
	}
}

void apgen_editor::update_the_duration(const Glib::ustring &selectedTimeSystem) {
	const char	*theNewDurationString, *any_errors_found;

	if(!DurConversionFunc) {

		//
		// we got an error... need to display it!
		//
		cerr << "Duration conversion utility not available!\n";
	} else if(!DurConversionFunc(
				m_duration_entry.get_text().c_str(),
				selectedTimeSystem.c_str(),
				&theNewDurationString,
				&editor_intfc::theDurationMultiplier,
				&any_errors_found)) {
		// we got an error... need to display it!
		cerr << "apgen_editor::on_epoch_selected(): ERROR -- "
			<< any_errors_found << endl;
	} else {
		m_duration_entry.set_text(theNewDurationString);
	}
}

void apgen_editor::on_epoch_selected() {
	Gtk::Entry*	pEntry = m_epoch_combo.get_entry();

	if(pEntry) {
		int	redetail_option_status = m_toggle_redetail_on_apply.get_active();

		Glib::ustring text = pEntry->get_text();
		carry_out_time_conversions(text);
		editor_intfc::get_default_choices()[editor_intfc::get_interface().type_name] = editor_config(
				redetail_option_status, text);
	}
}

void apgen_editor::carry_out_time_conversions(const Glib::ustring &text) {
	timeSystemInUse = text;
	// debug
	// cerr << "carry_out_time_conversions: text = " << text << endl;
	if(!(text.empty())) {	// (Note from gtkmm documentation:) We seem to get 2 signals,
				//  one when the text is empty.
		const char	*theNewStartTimeString, *any_errors_found;

		// 1. Handle start time
		if(!TimeConversionFunc) {
			// we got an error... need to display it!
			cerr << "Time conversion utility not available!\n";
		}
		if(!TimeConversionFunc(	m_start_entry.get_text().c_str(),
						text.c_str(),
						&theNewStartTimeString,
						&any_errors_found)) {
			// we got an error... need to display it!
			cerr << "apgen_editor::on_epoch_selected(): ERROR -- "
				<< any_errors_found << endl;
		} else {
			m_start_entry.set_text(theNewStartTimeString);
		}
		// 2. Handle duration if appropriate
		if(m_toggle_dur_timesystem.get_active()) {
			update_the_duration(text);
		} else {
			update_the_duration("UTC");
		}
	}
}

void apgen_editor::on_dur_toggled() {
	if(m_toggle_dur_timesystem.get_active()) {
		update_the_duration(m_epoch_combo.get_entry()->get_text());
	} else {
		update_the_duration("UTC");
	}
}

void apgen_editor::on_redetail_toggled() {
	Glib::ustring		theEpochChoice("UTC");
	Gtk::Entry		*pEntry = m_epoch_combo.get_entry();

	if(pEntry) {
		theEpochChoice = pEntry->get_text();
	}
	if(m_toggle_redetail_on_apply.get_active()) {
		editor_intfc::get_default_choices()[editor_intfc::get_interface().type_name] = editor_config(1, theEpochChoice);
		if(gtk_editor_write_enabled) {
			m_Button_Apply.set_sensitive(true);
			eS::gtk_editor_requests_redetailing = true;
		}
	} else {
		editor_intfc::get_default_choices()[editor_intfc::get_interface().type_name] = editor_config(0, theEpochChoice);
		if(gtk_editor_write_enabled) {
			m_Button_Apply.set_sensitive(true);
		}
		eS::gtk_editor_requests_redetailing = false;
	}
}

void apgen_editor::report_error(const string& msg) {
	if(!theAPerror) {
		theAPerror = new apgen_error();
	}
	theAPerror->set_txt_to(msg.c_str());
	theAPerror->show();
}

void apgen_editor::on_par_range_selected() {
	Gtk::Entry* pEntry = m_ParRangeValue.get_entry();
	if(pEntry) {
		Glib::ustring	text = pEntry->get_text();
		if(!text.empty()) {
			on_cell_edited(PathToWhichRangeApplies.c_str(), text);
		}
	}
}

void apgen_editor::on_button_show_def() {
	if(!theAPtype) {
		theAPtype = new apgen_type();
	}
	theAPtype->set_txt_to(TypeDefFunc());
	theAPtype->show();
}


//
// This capability was designed to support the InSight
// mission. Although useful, it is rather specific and
// it's not clear that we need to keep it.
//
void apgen_editor::on_button_show_param() {
	if(!theAPparam) {
		theAPparam = new apgen_param();
	}
	theAPparam->set_txt_to(ParamDefFunc());
	theAPparam->show();
}
