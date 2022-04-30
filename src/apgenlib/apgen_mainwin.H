#ifndef APGEN_GTK_MAIN_H
#define APGEN_GTK_MAIN_H

#include <gtkmm.h>
#include <memory>
#include <map>
#include "gtk_bridge.H"

using namespace std;

//
// The following is a simple GTK main window intended to provide a parent
// for all the GTK panels that are part of the APGenX GUI. Initially, I
// was hoping to have a "democratic design" in which each panel can play
// the role of a main window, but that is incompatible with the hierarchical
// design of gtkmm.
//
// Whenever the user requests a GTK panel for the first time, this window
// will be created automatically, and the GTK panel will be created as one
// of its children.
//
// The user can hide this window, but then all its children will be hidden.
// Activating a GTK panel again will cause the window to reappear. This
// simplifies the design of the interface between the GTK subsystem
// and the APGenX engine; all interactions pass through the main window.
//
// The actual code is inspired from the example
//
// 	/usr/share/doc/gtkmm24-devel-2.24.5/gtk-demo/example_menus.cc
//
// and in particular the class called Example_Menus.
//
class apgen_mainwin : public Gtk::Window {
public:
	apgen_mainwin();
	apgen_mainwin(const apgen_mainwin&) = delete;
	virtual ~apgen_mainwin();

	static apgen_mainwin*& theMainWindow();


	//
	// Members
	//
	Gtk::VBox			m_VBox1, m_VBox_Sub1;
	Gtk::MenuBar			m_MenuBar;
	Gtk::HSeparator			m_Separator;
	Gtk::HButtonBox			m_ButtonBox;
	Gtk::Button			m_Button;

	//
	// Callbacks
	//
	void				on_button_clicked();
	void				on_menu_item_picked(Glib::ustring which);

	static bool			on_timeout();
	sigc::connection		timeoutConn;

	std::map<void*, Gtk::Window*>	subwindows;
};

#endif //APGEN_GTK_MAIN_H
