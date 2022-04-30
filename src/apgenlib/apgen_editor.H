#ifndef APGEN_GTK_EDITOR_H
#define APGEN_GTK_EDITOR_H

#include <gtkmm.h>
#include <memory>
#include <map>
#include "gtk_bridge.H"

using namespace std;

class apgen_error: public Gtk::Window {
public:
	apgen_error();
	virtual ~apgen_error();
	void					set_txt_to(const string &);
protected:
	Gtk::VBox				m_VBox;
	  Gtk::Frame				m_Frame;
	    Gtk::ScrolledWindow			m_ScrolledWindow;
	  Gtk::HButtonBox			m_ButtonBox;
	    Gtk::Button				m_Button_Cancel;
	Glib::RefPtr<Gtk::TextBuffer>		m_buffer ;
	Gtk::TextView				m_TextView ;
	void					on_button_cancel();
} ;

class apgen_type: public Gtk::Window {
public:
	apgen_type();
	virtual ~apgen_type();
	void					set_txt_to(const string &);
protected:
	Gtk::VBox				m_VBox;
	  Gtk::Frame				m_Frame;
	    Gtk::ScrolledWindow			m_ScrolledWindow;
	  Gtk::HButtonBox			m_ButtonBox;
	Gtk::Button				m_Button_Cancel;
	Glib::RefPtr<Gtk::TextBuffer>		m_buffer ;
	Gtk::TextView				m_TextView ;
	void					on_button_cancel();
};

class ModelColumns_style_1 : public Gtk::TreeModel::ColumnRecord {
  public:
	ModelColumns_style_1() {
		add(m_col_keyword);
		add(m_col_value);
	}

	Gtk::TreeModelColumn<Glib::ustring> m_col_keyword;
	Gtk::TreeModelColumn<Glib::ustring> m_col_value;
};

class apgen_param: public Gtk::Window {
public:
	apgen_param();
	virtual ~apgen_param();
	void					set_txt_to(const string &);
protected:
	Gtk::VBox				m_VBox;
	  Gtk::Frame				m_Frame;
	    Gtk::ScrolledWindow			m_ScrolledWindow;
	  Gtk::HButtonBox			m_ButtonBox;
	Gtk::Button				m_Button_Cancel;
	Gtk::Button				m_Button_Apply;
	Gtk::Button				m_Button_OK;
	Glib::RefPtr<Gtk::TextBuffer>		m_buffer ;
	Gtk::TextView				m_TextView ;
	void					on_button_ok();
	void					on_button_apply();
	void					on_button_cancel();
};

class apgen_editor : public Gtk::Window {
public:
	apgen_editor();
	apgen_editor(const apgen_editor&) = delete;
	virtual ~apgen_editor();

	void					fillWithActivityData(
							bool first_time,
							bool params_only = false);
	void					clear_tables();
	void					set_legends_in_combo(apgen_editor&);

	void					report_error(const string &);
protected:

	//
	// Auxiliary windows
	//
	apgen_error*				theAPerror;
	apgen_type*				theAPtype;
	apgen_param*				theAPparam;

	Glib::ustring				timeSystemInUse;

	// HELPER METHODS
	void					fill_Table(int &the_current_row);
	void					fill_Table2(int &the_current_row);
	void					fill_ButtonBox();
public:

	//
	// the editor window
	//
	static apgen_editor*&			theWindow();

	//
	// Generic, static methods that can be used by other panels:
	//
	static void				fill_TreeModel(
							Glib::RefPtr<Gtk::TreeStore>&		the_tree_model,
							Gtk::TreeModel::Children::iterator	iter,
							ModelColumns_style_1&			key_val_col,
							gS::ISL&				the_value,
							std::map<string, gS::ISL>&		all_values);

	static void				UpdateTreeModel(
							Glib::RefPtr<Gtk::TreeStore>&		the_tree_model,
							Gtk::TreeModel::Path&			the_tree_path,
							const Glib::ustring&			edited_text,
							std::map<string, gS::ISL>&		all_values,
							ModelColumns_style_1&			key_val_col);

	//
	// Does everything required to display the editor, except
	// show the GTK panel. This can be done in two ways: invoke
	// GtkMain::run() if no panel is active, or theWindow()->show()
	// if another panel is available to make the call.
	//
	static void				prepareToShow();

	// static bool				never_shown;
protected:

	//
	// PRE-REQUISITE CLASSES
	//
	class MC_ParRangeValue : public Gtk::TreeModel::ColumnRecord {
	public:
		MC_ParRangeValue() { add(m_col_string); }

		Gtk::TreeModelColumn<Glib::ustring> m_col_string; };

	class MC_epoch : public Gtk::TreeModel::ColumnRecord {
	public:
		MC_epoch() { add(m_col_string); }

		Gtk::TreeModelColumn<Glib::ustring> m_col_string; };

	//
	// This structure is the same as that used by the
	// global_editor, which will allow us to share some
	// code:
	//
	ModelColumns_style_1		KeywordValueColumns;

	MC_ParRangeValue		mc_ParRangeValue;
	MC_epoch			mc_epoch;

	// for legends, epochs etc.:
	list<string>				m_epochs;

	// VBOX
	Gtk::VBox				m_VBox;
	  // HBOX
	  Gtk::HBox				m_HBox;
	    // TABLE 1 for 'standard' items like name, type etc.
	    Gtk::Table				m_Table;

	  // PANED for instance descr. + tree window
	  Gtk::VPaned				m_VPaned;
	    // FRAMES for instance description and attribute/parameter window
	    Gtk::Frame				m_Frame, m_Frame2;
	      // multi-line widget to hold the instance description
	      Gtk::ScrolledWindow		DescriptionWindow;
	    // scrolled window to store the Tree of attributes and parameters
	    Gtk::ScrolledWindow			m_ScrolledWindow;

	    Gtk::HBox				m_HBox2, m_HBox3, m_HBox4;
	      Gtk::Label			m_ParNameLabel;
	      Gtk::Entry			m_ParNameValue;
	      Gtk::Label			m_ParRangeLabel;
	      Gtk::ComboBoxEntry		m_ParRangeValue;
	      Gtk::Label			m_ParEdit;
	      Gtk::Button			m_ParShowButton;
	      Glib::RefPtr<Gtk::ListStore>	mr_ParRangeValue; // refTreeModel
	      std::string			PathToWhichRangeApplies;
	      Gtk::Label			m_ParDescrLabel1;
	      Gtk::Label			m_ParDescrLabel2;
	      Gtk::Frame			m_ParDescrValue;
	        Gtk::ScrolledWindow		ParDescrWin;
		  Glib::RefPtr<Gtk::TextBuffer>	ParDescr_buffer ;
        	  Gtk::TextView			ParDescr_TextView ;

	  // Finally, a button box to hold the apply/cancel/ok buttons
	  Gtk::HButtonBox				m_ButtonBox;


	/* Now the children of m_Table, arranged row-wise. */
	/* ROW 1 */
	Gtk::Label				m_name_label ;
	Gtk::Entry				m_name_entry ;
	Gtk::Label				m_ID_label ;
	Gtk::Entry				m_ID_entry ;
	/* ROW 2 */
	Gtk::Label				m_type_label;
	Gtk::Entry				m_type_entry;
	Gtk::Button				m_type_button;
	/* ROW 3 */
	Gtk::Label				m_start_label ;
	Gtk::Entry				m_start_entry ;
	Gtk::Label				m_epoch_label ;
	Gtk::ComboBoxEntry			m_epoch_combo ;
	Glib::RefPtr<Gtk::ListStore>		mr_epoch; // refTreeModel
	/* ROW 4 */
	Gtk::Label				m_duration_label ;
	Gtk::Entry				m_duration_entry ;
	Gtk::Label				m_duration_formula_label ;
	Gtk::Entry				m_duration_formula_entry ;
	/* ROW 5 */
	Gtk::Label				m_legend_label ;
	Gtk::Combo				m_legend_combo ;
	Gtk::Label				m_color_label ;
	Gtk::Combo				m_color_combo ;
	/* ROW 6 */
	Gtk::Label				m_descr_label ;
	/* ROW 7 */

	// Description window, not in the table (goes into m_Frame)
	Glib::RefPtr<Gtk::TextBuffer>		m_descr_buffer ;
	Gtk::TextView				m_TextView ;

	// Now the child of m_ScrolledWindow, a Tree View, and its Tree Store
	Gtk::TreeView				m_TreeView;
	Glib::RefPtr<Gtk::TreeStore>		m_refTreeModel;


	// BOTTOM BUTTONS
	Gtk::Button				m_Button_OK ,
	    					m_Button_Apply ,
						m_Button_Cancel ;
	Gtk::CheckButton			m_toggle_dur_timesystem,
						m_toggle_redetail_on_apply;

	// RENDERER for the Value Column of the tree
	unique_ptr<Gtk::CellRendererText>	m_renderer ;

	// To assist selection callback
	void					set_description_sensitivity(bool);
	void					set_range_sensitivity(bool);
	void					set_name_sensitivity(bool);

	void					update_the_duration(const Glib::ustring &new_timesystem);
	void					carry_out_time_conversions(const Glib::ustring &new_timesystem);

	//
	// For handling selection events
	//
	Glib::RefPtr<Gtk::TreeSelection>	refTreeSelection ;

	//
	// CALLBACKS (a.k.a. signal handlers)
	//

	void					on_cell_edited(
							const Glib::ustring& path,
							const Glib::ustring& new_text ) ;

	bool					select_function(
							const Glib::RefPtr<Gtk::TreeModel> &,
							const Gtk::TreePath &,
							bool ) ;

	void					on_selection_changed() ;
	void					on_par_range_selected();
	void					on_epoch_selected();

	void					notify_apgen(bool was_it_ok);

	//
	// Hides all windows associated with the editor:
	//
	void					hide_all() {
		if(theAPerror) {
			theAPerror->hide();
		}
		if(theAPtype) {
			theAPtype->hide();
		}
		if(theAPparam) {
			theAPparam->hide();
		}
		hide();
	}

	//
	// Button signal handlers:
	//
	void					on_button_cancel() ;
	void					on_button_apply() ;
	void					on_button_ok() ;

	void					on_button_show_def();
	void					on_button_show_param();

	void					on_dur_toggled();

	void					on_redetail_toggled();

	friend class apgen_param;
} ;

#endif //APGEN_GTK_EDITOR_H
