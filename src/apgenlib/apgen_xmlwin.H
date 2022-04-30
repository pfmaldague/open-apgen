#ifndef _APGEN_XML_WIN_H_
#define _APGEN_XML_WIN_H_

#include <gtkmm.h>
#include <memory>
#include <map>
#include "flexval.H"

using namespace std;


class apgen_xmlwin : public Gtk::Window {
public:
	apgen_xmlwin();
	virtual ~apgen_xmlwin();

	// HELPER METHODS
	void					fill_Table();
	void					fill_ButtonBox();
	void					fill_TreeModel(
							Gtk::TreeModel::Children::iterator	iter,
							flexval&				theValue);
	void					fillWithData(bool first_time);


	static apgen_xmlwin*&			theXMLwin();
	// static bool				never_shown;

	// PRE-REQUISITE CLASSES
	class ModelColumns_style_1 : public Gtk::TreeModel::ColumnRecord {
	  public:
		ModelColumns_style_1()
			{ add(m_col_keyword); add(m_col_value); }

		Gtk::TreeModelColumn<Glib::ustring> m_col_keyword;
		Gtk::TreeModelColumn<bool> m_col_value; };

	ModelColumns_style_1		KeywordValueColumns;

	// VBOX hierarchy
	Gtk::VBox					m_VBox;
	    Gtk::Table					m_Table;
		Gtk::Label				m_fname_label;
		Gtk::Entry				m_fname_entry;
		Gtk::CheckButton			m_toggle_partial;
	    Gtk::Table					m_Table2;
		Gtk::Label				m_begin_label;
		Gtk::Entry				m_begin_entry;
		Gtk::Label				m_end_label;
		Gtk::Entry				m_end_entry;
	    // FRAMES for instance description and attribute/parameter window
	    Gtk::Frame					m_Frame;
		// scrolled window to store the Tree of attributes and parameters
		Gtk::ScrolledWindow			m_ScrolledWindow;
		    // Now the child of m_ScrolledWindow, a Tree View, and its Tree Store
		    Gtk::TreeView			m_TreeView;
		    Glib::RefPtr<Gtk::TreeStore>	m_refTreeModel;

		    // For handling selection events
		    Glib::RefPtr<Gtk::TreeSelection>	refTreeSelection;

	    // a button box to hold the apply/cancel/ok buttons
	    Gtk::HButtonBox				m_ButtonBox;
		Gtk::Button				m_Button_OK,
	    						m_Button_Apply,
							m_Button_Cancel;

	bool					select_function(
							const Glib::RefPtr<Gtk::TreeModel>&,
							const Gtk::TreePath&,
							bool);

	unique_ptr<Gtk::CellRendererToggle>	m_renderer;

	// To assist selection callback
	void					set_description_sensitivity(bool);
	void					set_range_sensitivity(bool);
	void					set_name_sensitivity(bool);

	//Signal handlers
	void					on_button_cancel();
	void					on_button_apply();
	void					on_button_ok();
	void					on_selection_changed();
	void					on_cell_toggled(const Glib::ustring& path);
	void					on_partial_toggled();

	void					propagate_new_value(Gtk::TreeModel::iterator iter, bool newval);
	void					notify_apgen();
	void					hide_all() {
		hide();
	}
	void					collect_indices_recursively(
							Gtk::TreeModel::iterator,
							list<string>&,
							vector<string>&);
	flexval					get_data();
};

#endif //_APGEN_XML_WIN_H_
