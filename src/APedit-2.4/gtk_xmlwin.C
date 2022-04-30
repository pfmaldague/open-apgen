#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <iostream>
#include <locale.h>

#include <gtkmm/main.h>
#include <gtkmm/main.h>
#include <gtkmm/style.h>

#include <apgen_xmlwin.H>
#include <apgen_globwin.H>
#include <gtk_bridge.H> // for add_quotes_to()

#include <flexval.H>

apgen_xmlwin*&	apgen_xmlwin::theXMLwin() {
	static apgen_xmlwin* w = NULL;
	return w;
}

// bool		apgen_xmlwin::never_shown = true;

apgen_xmlwin::apgen_xmlwin()
	:	m_VBox(Gtk::ORIENTATION_VERTICAL),
		m_Table(3, 6, true),
		m_fname_label("File name:"),
		m_begin_label("Start:"),
		m_end_label("End:"),
		m_toggle_partial("Partial"),
		m_Button_Cancel("Cancel"),
		m_Button_Apply("Apply"),
		m_Button_OK("OK"),
		m_renderer(new Gtk::CellRendererToggle) {
	string	Title("APGEN XMLTOL Options ");

	//
	// defined in config.h:
	//
	Title = Title + VERSION;
	set_title(Title.c_str());
	set_border_width(5);
	set_default_size(300, 700);

	//
	// Recall that we defined the following widgets as class members:
	// 	VBox		m_VBox
	// 	ScrolledWindow	m_ScrolledWindow
	//	HButtonBox	m_ButtonBox
	//	Button		m_ButtonQuit
	//

	// STEP 1. Add the vertical box (already initialized as a data member) to this window

	add(m_VBox);

	// STEP 2. Define TreeView and add it to the ScrolledWindow.
	m_ScrolledWindow.add(m_TreeView);

		// Only show the scrollbars when they are necessary:
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	m_Frame.add(m_ScrolledWindow);

	m_VBox.pack_start(m_Table,	Gtk::PACK_SHRINK);
	m_VBox.pack_start(m_Frame,	Gtk::PACK_EXPAND_WIDGET);
	m_VBox.pack_start(m_ButtonBox,	Gtk::PACK_SHRINK);
	m_VBox.set_homogeneous(false);

	set_name_sensitivity(false);
	set_range_sensitivity(false);
	set_description_sensitivity(false);

	fill_Table();
	fill_ButtonBox();


	m_toggle_partial.signal_toggled().connect(
		sigc::mem_fun(*this, &apgen_xmlwin::on_partial_toggled));

	m_toggle_partial.set_active(true);
	/* We probably won't need this:
	 * refTreeSelection->set_select_function(
	 * 			sigc::mem_fun(
	 * 				*this,
	 * 				&apgen_xmlwin::select_function));
	 */

	fillWithData(true);
	// never_shown = false;
	show_all_children();
}

apgen_xmlwin::~apgen_xmlwin() {
	xS::panel_active = false;
}

void apgen_xmlwin::on_partial_toggled() {
	if(m_toggle_partial.get_active()) {
		m_TreeView.set_sensitive(true);
		xS::partial_xmltol() = true;
		// cout << "partial ON\n";
	}
	else {
		m_TreeView.set_sensitive(false);
		xS::partial_xmltol() = false;
		// cout << "partial OFF\n";
	}
}

void apgen_xmlwin::on_selection_changed() {
}

flexval	apgen_xmlwin::get_data() {
	return xS::theXmlWinData();
}

void apgen_xmlwin::fill_Table() {
	int	current_row = 0;

	m_Table.attach(	m_fname_label,
			0,		2,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(m_fname_entry,
			2,		6,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	current_row++;
	m_Table.attach(	m_toggle_partial,
			0,		3,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_fname_entry.set_text("apgen-tol.xml");
	current_row++;
	m_Table.attach(	m_begin_label,
			0,		1,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_begin_entry,
			1,		3,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_begin_entry.set_text(xS::theXmlStartTime());
	m_Table.attach(	m_end_label,
			3,		4,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_Table.attach(	m_end_entry,
			4,		6,
			current_row,	current_row + 1,
			Gtk::FILL,	Gtk::FILL,
			5,		5);
	m_end_entry.set_text(xS::theXmlEndTime());
}

// assumes the flexval is a struct
void apgen_xmlwin::fill_TreeModel(
		Gtk::TreeModel::Children::iterator	given_row,
		flexval&				row_data) {
	const Gtk::TreeModel::Row&	row_to_fill(*given_row);

	assert(row_data.getType() == flexval::TypeStruct);

	Gtk::TreeModel::Children			theChildren = row_to_fill.children();
	std::map<std::string, flexval>&			theData(row_data.get_struct());
	std::map<std::string, flexval>::iterator	iter;

	for(iter = theData.begin(); iter != theData.end(); iter++) {
		Gtk::TreeModel::Children::iterator	row_iter = m_refTreeModel->append(theChildren);
		const Gtk::TreeModel::Row&		this_row(*row_iter);

		this_row[KeywordValueColumns.m_col_keyword] = iter->first;
		if(iter->second.getType() == flexval::TypeStruct) {
			this_row[KeywordValueColumns.m_col_value] = false;
			fill_TreeModel(this_row, iter->second);
		} else if(iter->second.getType() == flexval::TypeBool) {
			this_row[KeywordValueColumns.m_col_value] = (bool) iter->second;
		}
	}
}

void apgen_xmlwin::on_button_cancel() {
	hide_all();
}

void apgen_xmlwin::on_button_apply() {
	notify_apgen();
}

void apgen_xmlwin::on_button_ok() {
	notify_apgen();
	hide_all();
}

void apgen_xmlwin::fill_ButtonBox() {
	m_ButtonBox.set_layout(		Gtk::BUTTONBOX_SPREAD);
	m_ButtonBox.set_spacing(	20);
	m_ButtonBox.set_border_width(	5);

	m_ButtonBox.pack_start(	m_Button_OK,		Gtk::PACK_SHRINK);
	m_ButtonBox.pack_start(	m_Button_Apply,		Gtk::PACK_SHRINK);
	m_ButtonBox.pack_start(	m_Button_Cancel,	Gtk::PACK_SHRINK);

	// m_ButtonBox.set_layout(Gtk::BUTTONBOX_END);

	m_Button_Cancel.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_xmlwin::on_button_cancel));
	m_Button_Apply.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_xmlwin::on_button_apply));
	m_Button_OK.signal_clicked().connect(
			sigc::mem_fun(*this, &apgen_xmlwin::on_button_ok));
}

void apgen_xmlwin::fillWithData(bool first_time) {

	// debug
	// cerr << "apgen_xmlwin::fillWithData()\n";

	m_refTreeModel = Gtk::TreeStore::create(KeywordValueColumns);
	m_TreeView.set_model(m_refTreeModel);

	Gtk::TreeModel::Row	top_level_row = *(m_refTreeModel->append());
	top_level_row[KeywordValueColumns.m_col_keyword] = "subsystems";
	top_level_row[KeywordValueColumns.m_col_value] = false;
	fill_TreeModel(top_level_row, get_data()["subsystems"]);

	if(first_time) {
		m_TreeView.append_column("Name", KeywordValueColumns.m_col_keyword);

		//
		// We don't do this:
		//
		// m_TreeView.append_column_editable("Included", KeywordValueColumns.m_col_value);
		//
		// Instead, we follow the example of gtk_thread.C and use a renderer:
		//
		Gtk::TreeViewColumn	*const m_column = new Gtk::TreeViewColumn(
						"Included",
						*Gtk::manage(m_renderer.get()));
		m_TreeView.append_column(*Gtk::manage(m_column));
		m_column->add_attribute(	m_renderer->property_active(),
						KeywordValueColumns.m_col_value);
		// m_renderer->set_activatable(true);
		m_renderer->signal_toggled().connect(sigc::mem_fun(*this, &apgen_xmlwin::on_cell_toggled));
	}
}

void apgen_xmlwin::on_cell_toggled(const Glib::ustring& path) {
	Gtk::TreeModel::Path					the_tree_path(path);
	Gtk::TreeModel::iterator				iter;
	Gtk::TreeModel::Row					row;
	bool							the_new_value = false;

	iter = m_refTreeModel->get_iter(the_tree_path);
	row = *iter;
	the_new_value = !row[KeywordValueColumns.m_col_value];

	propagate_new_value(iter, the_new_value);
}

void apgen_xmlwin::propagate_new_value(Gtk::TreeModel::iterator	iter, bool newval) {
	Gtk::TreeModel::Row	row = *iter;

	row[KeywordValueColumns.m_col_value] = newval;

	// debug
	// Glib::ustring		thePath = m_refTreeModel->get_string(row);
	// cout << "apgen_xmlwin::propagate_new_value: path = " << thePath << ", val "
	// 	<< row[KeywordValueColumns.m_col_value] << "\n";

	Gtk::TreeModel::Children		theChildren = row.children();
	Gtk::TreeModel::Children::iterator	child;
	for(child = theChildren.begin(); child != theChildren.end(); child++) {
		propagate_new_value(child, newval);
	}
}


void apgen_xmlwin::set_name_sensitivity(bool T) {
}

void apgen_xmlwin::set_range_sensitivity(bool T) {
}

void apgen_xmlwin::set_description_sensitivity(bool T) {
}

bool apgen_xmlwin::select_function(	const Glib::RefPtr<Gtk::TreeModel>& model,
					const Gtk::TreeModel::Path& path, bool) {
	return true;
}

void apgen_xmlwin::collect_indices_recursively(
		Gtk::TreeModel::Children::iterator	iter,
		list<string>&				pieces,
		vector<string>&				results) {
	Gtk::TreeModel::Children		theChildren = iter->children();
	Gtk::TreeModel::Children::iterator	child = theChildren.begin();
	if(child == theChildren.end()) {
		Gtk::TreeModel::Row	row = *iter;
		if(row[KeywordValueColumns.m_col_value]) {
			list<string>::iterator	it;
			ostringstream		os;
			for(it = pieces.begin(); it != pieces.end(); it++) {
				os << *it;
			}
			string piece = gS::add_quotes_to(os.str());
			results.push_back(piece);
		}
	} else {
		for(; child != theChildren.end(); child++) {
			Gtk::TreeModel::Row	row = *child;
			string			c;
			row.get_value(0, c);
			pieces.push_back(c);
			collect_indices_recursively(child, pieces, results);
			pieces.pop_back();
		}
	}
}

void apgen_xmlwin::notify_apgen() {
	Gtk::TreeModel::Children		top_level = m_refTreeModel->children();
	Gtk::TreeModel::Children::iterator	top_level_iter = top_level.begin();
	Gtk::TreeModel::Children::iterator	subsystem;
	Gtk::TreeModel::Children::iterator	resource;


	{
		lock_guard<mutex>	lock1(*gS::get_gtk_mutex());
		if(m_toggle_partial.get_active()) {
			xS::theResults().clear();
			assert(top_level_iter != top_level.end());
			Gtk::TreeModel::Children	subsys = top_level_iter->children();
			for(subsystem = subsys.begin(); subsystem != subsys.end(); subsystem++) {
				Gtk::TreeModel::Children	resources = subsystem->children();
				for(resource = resources.begin(); resource != resources.end(); resource++) {
					string			resname;
					list<string>		pieces;
					resource->get_value(0, resname);
					pieces.push_back(resname);
					collect_indices_recursively(resource, pieces, xS::theResults());
				}
			}
		} else {
			xS::theResults().clear();
		}

		xS::theFileName() = m_fname_entry.get_text();
		xS::theXmlStartTime() = m_begin_entry.get_text();
		xS::theXmlEndTime() = m_end_entry.get_text();

		// debug
		cerr << "apgen_xmlwin::notify_apgen(): setting gtk_xmlwin_data_available to true\n";

		xS::gtk_xmlwin_data_available = true;
	}
}
