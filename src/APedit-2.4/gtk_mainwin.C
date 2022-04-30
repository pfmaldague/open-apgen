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

apgen_mainwin*&	apgen_mainwin::theMainWindow() {
	static apgen_mainwin* am = NULL;
	return am;
}

bool				gS::gtk_subsystem_inactive = true;
static Gtk::Main*		gS_main_object = NULL;

void gS::initializeGtkSubsystem() {

			// not sure whether we really need to emulate main():
	int		argc = 1;
	char*		theFirstString = (char *) malloc(strlen("APedit") + 1);
	char**		string_address = &theFirstString;

	strcpy(theFirstString, "APedit");

	gS::gtk_subsystem_inactive = false;

	//
	// NOTE: if one day you want to let the user specify this
	// stuff in a file, use the Gtk::RC constructor with a
	// string containing the name of the rc file as an argument.
	//

	const char*	home_dir = getenv("HOME");
	Cstring		rc_file_name(home_dir);
	rc_file_name << "/.gtkrc-2.0";
	FILE*		rc_file = fopen(*rc_file_name, "r");
	if(!rc_file) {
		cout << "gtk editor: " << rc_file_name << " not found, using built-in defaults\n";

		Gtk::RC::parse_string(
			"gtk-font-name = \"Sans 10\"\n"
			"style \"apgen_grey\"\n"
			"{\n"
			  "fg[NORMAL] = { 0, 0, 0 }\n"
			  "bg[NORMAL] = { 0.125, 0.698, 0.675 }\n"

			  //
			  // for testing:
			  //
			  // "bg[NORMAL] = { 1.000, 0.000, 0.000 }\n"

			  "base[NORMAL] = {0.8, 0.8, 0.8 }\n"
			  "text[NORMAL] = {0, 0, 0 }\n"
			  "fg[INSENSITIVE] = { 0, 0, 0 }\n"
			  "bg[INSENSITIVE] = { 0.100, 0.449, 0.438 }\n"
			  "base[INSENSITIVE] = {0.6, 0.6, 0.6 }\n"
			  "bg[PRELIGHT] = { 0.125, 0.698, 0.675 }\n"
			  "fg[ACTIVE] = { 0, 0, 0 }\n"
			"}\n"
			"widget \"*\" style \"apgen_grey\"\n"
  			);
	} else {
		cout << "gtk editor: getting defaults from " << rc_file_name << "\n";

		struct stat FileStats;
		stat(*rc_file_name, &FileStats);
		long int content_size = FileStats.st_size;
		char* content_string = (char*) malloc(content_size + 1);
		content_string[content_size] = '\0';
		fread(content_string, 1, content_size, rc_file);
		fclose(rc_file);
		Gtk::RC::parse_string(content_string);
		free(content_string);
	}


	gS_main_object = new Gtk::Main(&argc, &string_address);

	free(theFirstString);
}

apgen_mainwin::apgen_mainwin()
	: m_VBox_Sub1(false, 10),
		m_Button("Cancel") {
    set_title("APGenX GTK main window");
    set_border_width(0);

    add(m_VBox1);

    m_VBox1.pack_start(m_MenuBar, Gtk::PACK_SHRINK);
    {
	Gtk::MenuItem*	editItem = Gtk::manage(new Gtk::MenuItem("Edit"));

	Gtk::Menu*	editMenu = Gtk::manage(new Gtk::Menu());
	Gtk::MenuItem*	editActivity = Gtk::manage(new Gtk::MenuItem("Activity"));
	editMenu->append(*editActivity);
	editActivity->show();
	Gtk::MenuItem*	editGlobals = Gtk::manage(new Gtk::MenuItem("Globals"));
	editMenu->append(*editGlobals);

	//
	// example from the GNOME developer guide:
	//
	//	m_button1.signal_clicked().connect(
	//		sigc::bind<Glib::ustring>(
	//			sigc::mem_fun(
	//				*this,
	//				&HelloWorld::on_button_clicked
	//			),
	//			"button 1"
	//		)
	//	);
	//
	editActivity->signal_activate().connect(
		sigc::bind<Glib::ustring>(
			sigc::mem_fun(*this, &apgen_mainwin::on_menu_item_picked),
			"editActivity"
		)
	);
	editGlobals->signal_activate().connect(
		sigc::bind<Glib::ustring>(
			sigc::mem_fun(*this, &apgen_mainwin::on_menu_item_picked),
			"editGlobals"
		)
	);

	editItem->set_submenu(*editMenu);
	m_MenuBar.append(*editItem);
	editGlobals->show();
	editItem->show();
    }
    {
	Gtk::MenuItem*	exportItem = Gtk::manage(new Gtk::MenuItem("Export"));

	Gtk::Menu*	exportMenu = Gtk::manage(new Gtk::Menu());
	Gtk::MenuItem*	exportXMLTOL = Gtk::manage(new Gtk::MenuItem("XMLTOL"));
	exportMenu->append(*exportXMLTOL);

	exportXMLTOL->signal_activate().connect(
		sigc::bind<Glib::ustring>(
			sigc::mem_fun(*this, &apgen_mainwin::on_menu_item_picked),
			"exportXMLTOL"
		)
	);

	exportXMLTOL->show();

	exportItem->set_submenu(*exportMenu);
	m_MenuBar.append(*exportItem);
	exportItem->show();
    }
    m_VBox1.pack_start(m_Separator, Gtk::PACK_SHRINK);

    m_ButtonBox.set_layout(Gtk::BUTTONBOX_SPREAD);
    m_ButtonBox.set_spacing(20);
    m_ButtonBox.set_border_width(5);
    m_ButtonBox.add(m_Button);

    m_VBox1.pack_start(m_ButtonBox, Gtk::PACK_SHRINK);

    m_Button.signal_clicked().connect(sigc::mem_fun(*this, &apgen_mainwin::on_button_clicked));

    m_Button.property_can_default() = true;
    m_Button.grab_default();

    show_all();
    theMainWindow() = this;

    //
    // Connect the timeout
    //
    timeoutConn = Glib::signal_timeout().connect(
			sigc::ptr_fun(&on_timeout),
			// debug: should be 100
			1000 /* milliseconds */);
}

apgen_mainwin::~apgen_mainwin() {}

void apgen_mainwin::on_button_clicked() {
	std::map<void*, Gtk::Window*>::iterator iter;
	for(	iter = subwindows.begin();
		iter != subwindows.end();
		iter++) {
	    iter->second->hide_all();
	}
	hide_all();
}

void apgen_mainwin::on_menu_item_picked(Glib::ustring buttonID) {
	//
	// placeholder
	//
	cerr << "on_menu_item_picked: id = " << buttonID << "\n";
}

bool apgen_mainwin::on_timeout() {

	//
	// mutex logic
	//
	bool	must_quit = false;
	bool	must_display_apgen_editor = false;
	bool	must_display_global_editor = false;
	bool	must_display_xmlwin = false;

	{
	    // debug
	    // cerr << "apgen_mainwin::on_timeout: locking gtk mutex...";

	    lock_guard<mutex>	lock1(*gS::get_gtk_mutex());

	    // debug
	    // cerr << " got it.\n";

	    if(gS::quit_requested) {
		must_quit = true;
	    } else if(eS::panel_requested) {
		must_display_apgen_editor = true;
		eS::panel_requested = false;
	    } else if(glS::panel_requested) {
		must_display_global_editor = true;
		glS::panel_requested = false;
	    } else if(xS::panel_requested) {
		must_display_xmlwin = true;
		xS::panel_requested = false;
	    }

	    if(must_quit) {

		//
		// It's OK to set this now; the important thing is that
		// run() will return if this is the last active window
		//
		glS::panel_active = false;

		//
		// this will cause Main::run() to return in display_gtk_window()
		// if no other GTK panel is active
		//
		theMainWindow()->hide_all();

		std::map<void*, Gtk::Window*>::iterator iter;
		for(	iter = theMainWindow()->subwindows.begin();
			iter != theMainWindow()->subwindows.end();
			iter++) {
		    iter->second->hide_all();
		}

		//
		// disconnect the timer:
		//
		return false;
	    }
	}

	if(must_display_apgen_editor) {

	    // cerr << "editor requested!\n";

	    //
	    // We mimic what gS::display_gtk_window() does
	    //
	    apgen_editor::prepareToShow();
	    apgen_editor::theWindow()->show();
	    theMainWindow()->subwindows[apgen_editor::theWindow()]
		= apgen_editor::theWindow();
	} else if(must_display_global_editor) {

	    // cerr << "global editor requested!\n";

	    //
	    // We mimic what gS::display_gtk_window() does
	    //
	    if(!global_editor::theGlobalwin()) {
		global_editor::theGlobalwin() =  new global_editor;
	    } else {
		global_editor::theGlobalwin()->fillWithData(false);
	    }
	    global_editor::theGlobalwin()->show();
	    theMainWindow()->subwindows[global_editor::theGlobalwin()] = global_editor::theGlobalwin();

	} else if(must_display_xmlwin) {

	    // cerr << "xml panel editor requested!\n";

	    //
	    // We mimic what gS::display_gtk_window() does
	    //
	    if(!apgen_xmlwin::theXMLwin()) {
		apgen_xmlwin::theXMLwin() =  new apgen_xmlwin;
	    } else {
		apgen_xmlwin::theXMLwin()->fillWithData(false);
	    }
	    apgen_xmlwin::theXMLwin()->show();
	    theMainWindow()->subwindows[apgen_xmlwin::theXMLwin()] = apgen_xmlwin::theXMLwin();
	}


	//
	// debug
	//
	// static int d = 0;
	// cerr << "mainwin timeout count: " << (++d) << "\n";
	return true;
}


//
// The APGenX engine uses this method to display the requested GTK
// panel. It is used for all GTK panels. The method creates the main
// GTK window first, then sends a message to it requesting the desired
// panel.
//
// The method runs in a thread that is separate from the main APGenX
// engine (modeling) thread. The method blocks forever, until the
// engine sends it the "quit request".
//
// If the GTK main window is deactivated by the user, the call to
// Gtk::Main::run( the main window ) returns, but this method does not.
// Instead, it embarks on a while loop and waits for a new request from
// the engine. This new request can be a request to quit, in case
// the method returns, allowing the engine to join the GTK thread, or
// a request for a GTK panel, in which case the method calls the run()
// method again to display the main window and sends it an appropriate
// request.
//
void gS::display_gtk_window(gS::WinObj which) {

	//
	// STEP 1A. Initialize GTK (if necessary)
	//

	//
	// Simple test to tell us for sure that we've never
	// been called before:
	//
	if(!gS_main_object) {

		// debug
		// cerr << "display_gtk_window(): creating main GTK object\n";

		//
		// Creates Gtk::Main
		//
		initializeGtkSubsystem();
	}
	{
	    lock_guard<mutex> lock1(*get_gtk_mutex());
	    switch(which) {
		case WinObj::EDITOR:
		    eS::panel_requested = true;
		    break;
		case WinObj::GLOBAL:
		    glS::panel_requested = true;
		    break;
		case WinObj::XMLTOL:
		    xS::panel_requested = true;
		    break;
		default:
		    break;
	    }
	}

	if(!apgen_mainwin::theMainWindow()) {
	    apgen_mainwin::theMainWindow() = new apgen_mainwin;
	}

	Gtk::Main::run(*apgen_mainwin::theMainWindow());

	//
	// Inefficient, but I don't know how to restart the GTK subsystem
	// with previously created windows. Just invoking run() again leads
	// to unresponsive windows.
	//
	std::map<void*, Gtk::Window*>::iterator iter;
	for(	iter = apgen_mainwin::theMainWindow()->subwindows.begin();
		iter != apgen_mainwin::theMainWindow()->subwindows.end();
		iter++) {
	    delete iter->second;
	}
	delete apgen_mainwin::theMainWindow();
	// delete gS_main_object;
	// gS_main_object = NULL;
	apgen_mainwin::theMainWindow() = NULL;
	apgen_editor::theWindow() = NULL;
	global_editor::theGlobalwin() = NULL;
	apgen_xmlwin::theXMLwin() = NULL;

	//
	// Notify the APGenX engine that the GTK thread
	// should terminate
	//
	{
	    lock_guard<mutex> lock1(*get_gtk_mutex());
	    gS::gtk_subsystem_inactive = true;
	}
}
