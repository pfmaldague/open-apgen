#ifndef _APGEN_GLOB_EDITOR_H_
#define _APGEN_GLOB_EDITOR_H_

//
// Like the other GTK-based GUI components, the Globals Editor
// consists of the following elements:
//
//   - a GUI-oriented class, global_editor; it is defined here
//     and implemented in APedit-2.4/gtk_globwin.C
//
//   - an interface-flavored interface, glS; it is defined in
//     gtk_bridge.H and implemented in APedit-2.4/gtk_interface.C
//
//   - an interface with the Motif GUI; it is implemented in
//     atm_client/gtk_globals_bridge.C
//

#include <memory>
#include <map>
#include "gtk_bridge.H"
#include "apgen_editor.H"	// for ModelColumns_style_1 and fill_TreeModel()
#include <gtkmm.h>

// using namespace std;

class global_editor: public Gtk::Window {
public:
	global_editor();
	virtual ~global_editor();

	//
	// One day, we will have a Gtk main window. Until we do, the first
	// GTK panel that is created acts as a de facto main window. This
	// map contains pointers to all the peer-level windows that were
	// created; it is the responsibility of the de-facto main window
	// to delete all the sub-windows it created.
	//
	std::map<void*, Gtk::Window*>	subwindows;


	//
	// This static structure holds all the global data. The
	// map will contain a single item whose key is "Globals"
	// and whose value is an ISL containing all the globals
	// in serialized form.
	//
	static std::map<string, gS::ISL>&		all_globs();

	//
	// Fills all_globs with current values of the globals. Since
	// the gtk code does not import core classes, the task of
	// collecting the data is delegated to the global editor
	// bridge.
	//
	void						fillWithData(bool first_time);

	//
	// Some graphics work only needs to be done once, when
	// creating the global editor panel:
	//
	// static bool					never_shown;

	static global_editor*&				theGlobalwin();

protected:

	//
	// Widget hierarchy
	//
	Gtk::VBox					m_VBox;
	    Gtk::Frame					m_Frame;
		Gtk::ScrolledWindow			m_ScrolledWindow;
		    Gtk::TreeView			m_TreeView;
		    Glib::RefPtr<Gtk::TreeStore>	m_refTreeModel;
		    Glib::RefPtr<Gtk::TreeSelection>	refTreeSelection;
		Gtk::HButtonBox				m_ButtonBox;
		    Gtk::Button				m_Button_OK,
							m_Button_Apply,
							m_Button_Cancel;

	//
	// This structure is the same as that used by the
	// global_editor, which will allow us to share some
	// code:
	//
	ModelColumns_style_1				KeywordValueColumns;

	unique_ptr<Gtk::CellRendererText>		m_renderer;

	//
	// Responsible for updating the value of
	// global data kept by the editor; these
	// values will be communicated to the core
	// when the user hits the Apply or OK button.
	//
	void						on_cell_edited(
							    const Glib::ustring& path,
							    const Glib::ustring& new_text);

	//
	// Responsible for updating editor components
	// that react to which global or which global
	// element is selected: description, unit and
	// range.
	//
	void						on_selection_changed();

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
	bool						select_function(
							    const Glib::RefPtr<Gtk::TreeModel>&,
							    const Gtk::TreePath&,
							    bool);

	void						notify_apgen(bool apply);

	void						hide_all() {
		hide();
	}

	void						on_button_cancel();
	void						on_button_apply();
	void						on_button_ok();

};



#endif /* _APGEN_GLOB_EDITOR_H_ */
