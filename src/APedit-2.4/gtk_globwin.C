#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <iostream>
#include <locale.h>

#include <gtkmm/main.h>
#include <gtkmm/main.h>
#include <gtkmm/style.h>

#include "apgen_globwin.H"
#include "apgen_xmlwin.H"
#include "gtk_bridge.H" // for add_quotes_to()

global_editor*& global_editor::theGlobalwin() {
	static global_editor* g = NULL;
	return g;
}


global_editor::global_editor()
	: m_VBox(Gtk::ORIENTATION_VERTICAL),
		m_Button_Cancel("Cancel"),
		m_Button_Apply("Apply"),
		m_Button_OK("OK"),
		m_renderer(new Gtk::CellRendererText) {
	string	Title("APGEN Globals Editor");

	Title = Title + VERSION;
	set_title(Title.c_str());
	set_border_width(5);
	set_default_size(300, 800);

	add(m_VBox);

	m_ScrolledWindow.add(m_TreeView);
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	m_Frame.add(m_ScrolledWindow);

	m_ButtonBox.add(m_Button_OK);
	m_ButtonBox.add(m_Button_Apply);
	m_ButtonBox.add(m_Button_Cancel);

	m_Button_OK.signal_clicked().connect(
		sigc::mem_fun(*this, &global_editor::on_button_ok));
	m_Button_Apply.signal_clicked().connect(
		sigc::mem_fun(*this, &global_editor::on_button_apply));
	m_Button_Cancel.signal_clicked().connect(
		sigc::mem_fun(*this, &global_editor::on_button_cancel));

	m_VBox.pack_start(m_Frame, Gtk::PACK_EXPAND_WIDGET);
	m_VBox.pack_start(m_ButtonBox, Gtk::PACK_SHRINK);
	m_VBox.set_homogeneous(false);

	fillWithData(true);

	refTreeSelection = m_TreeView.get_selection();
	refTreeSelection->signal_changed().connect(
		sigc::mem_fun(*this, &global_editor::on_selection_changed));

	show_all_children();
}

global_editor::~global_editor() {
}

std::map<string, gS::ISL>& global_editor::all_globs() {
	static std::map<string, gS::ISL> AG;
	return AG;
}

void global_editor::fillWithData(bool first_time) {
    m_refTreeModel = Gtk::TreeStore::create(KeywordValueColumns);
    m_TreeView.set_model(m_refTreeModel);

    Gtk::TreeModel::Row top_level_row = *(m_refTreeModel->append());
    top_level_row[KeywordValueColumns.m_col_keyword] = "Globals";
    top_level_row[KeywordValueColumns.m_col_value] = "";

    if(first_time) {
	list<string>			empty_list;

	//
	// Structure to pass global names and values to the editor.
	// Remember: gtk_editor_array is a typedef for vector<ISL>
	// and ISL is a typedef for smart_ptr<indexed_string_or_list>
	//
	gS::indexed_string_or_list*	globs_ptr = new gS::de_facto_array(
					"Globals",	// name
					"",		// description
					empty_list,	// range
					true		// has labels
					);
	gS::ISL isl(globs_ptr);
	all_globs()["Globals"] = isl;
    }
    gS::ISL&			arrayPtr = all_globs()["Globals"];
    gS::gtk_editor_array&	theArray = arrayPtr->get_array();

    //
    // emulate the activity editor to clear data
    //
    theArray.erase(theArray.begin(), theArray.end());

    //
    // We could fill the array here, but that would require access
    // to the TypedValues inside the globals constructor, and as
    // of now the APedit files are independent of the APGenX core.
    //
    // So, we delegate to the bridge:
    //
    glS::get_the_globals(arrayPtr);

    //
    // Now fill the gtk structure
    //
    for(int i = 0; i < theArray.size(); i++) {
	Gtk::TreeModel::Children		theChildren = top_level_row.children();
	Gtk::TreeModel::Children::iterator	this_row = m_refTreeModel->append(theChildren);
	apgen_editor::fill_TreeModel(
			m_refTreeModel,
			this_row,
			KeywordValueColumns,
			theArray[i],
			all_globs());
    }

    if(first_time) {
    	m_TreeView.append_column("Name", KeywordValueColumns.m_col_keyword);
	Gtk::TreeViewColumn* const m_column = new Gtk::TreeViewColumn(
				"Value", *Gtk::manage(m_renderer.get()));
	m_TreeView.append_column(*Gtk::manage(m_column));
	m_column->add_attribute(m_renderer->property_text(),
				KeywordValueColumns.m_col_value);
	m_renderer->property_editable() = true;
	m_renderer->signal_edited().connect(sigc::mem_fun(
				*this,
				&global_editor::on_cell_edited));
    }
}


//
// Responsible for updating editor components
// that react to which global or which global
// element is selected: description, unit and
// range.
//
void global_editor::on_selection_changed() {
}

//
// Responsible for updating the value of global data kept by the editor.
// These values will be communicated to the core when the user hits the
// Apply or OK button. Until then, all manipulations of 'value' data is
// done in terms of strings.
//
// We reuse the generic static method provided by apgen_editor.
//
void global_editor::on_cell_edited(
			const Glib::ustring& path,
			const Glib::ustring& edited_text) {
	Gtk::TreeModel::Path	tree_path(path);

	apgen_editor::UpdateTreeModel(
		m_refTreeModel,
		tree_path,
		edited_text,
		all_globs(),
		KeywordValueColumns);
}

//
// Responsible for configuring the rendered
// properly, given what part of the tree is
// being selected. Only tree nodes can be
// edited; intermediate nodes display a
// summary of the content of the nodes
// below them. The summary is evaluated by
// the editor and should not be edited
// directly.
//
bool global_editor::select_function(
		const Glib::RefPtr<Gtk::TreeModel>&	model,
		const Gtk::TreePath&			path,
		bool) {
	const Gtk::TreeModel::iterator	iter = model->get_iter(path);
	Gtk::TreeModel::Row		row = *iter;
	Glib::ustring			theKey = row[KeywordValueColumns.m_col_keyword];

	//
	// only allow leaf nodes to be edited
	//
	if(	theKey != "Globals"
		&& iter->children().empty()) {
		m_renderer->property_editable() = true;
	} else {
		m_renderer->property_editable() = false;
	}
	return true;
}

void global_editor::notify_apgen(bool apply_was_used) {
}

void global_editor::on_button_cancel() {
	glS::panel_active = false;
	hide_all();
}

void global_editor::on_button_apply() {
	notify_apgen(true);
}

void global_editor::on_button_ok() {
	notify_apgen(false);
	glS::panel_active = false;
	hide_all();
}
