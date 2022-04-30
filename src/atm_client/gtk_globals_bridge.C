#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include "AP_exp_eval.H"
#include "gtk_bridge.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "UI_mainwindow.H"

#ifdef GTK_EDITOR

string&		glS::theGlobalName() {
	static string S;
	return S;
}

string&		glS::theGlobalValue() {
	static string T;
	return T;
}

bool		glS::gtk_globwin_data_available = false;

void glS::grab_gtk_globwin_data() {
	EDIT_GLOBALrequest*	theRequest;
	Cstring			global_name;

	// for now we don't mess with filter, schema, time systems etc.
	theRequest = new EDIT_GLOBALrequest(
		theGlobalName(),
		theGlobalValue()
		);
	theRequest->process();
	Cstring msg("Done creating EDITGLOBAL request.");
	UI_mainwindow::setStatusBarMessage(msg);
}

//
// Here we need to deal with the fact that the main APGenX window
// runs in Motif, while the global editor and other GTK panels run
// in the "GTK loop", which is completely separate and therefore
// runs in a separate thread.
//
// The solution is that whichever GTK panel is created first plays
// the role of a main window. Launching a panel takes two steps:
//
//	- creating a GTK object
//	- invoking the object's run() method
//
// Creating multiple GTK objects is not a problem. The problem is
// that only one object's run method can be invoked. When a second
// panel is needed, the existing panel can invoke the new panel's
// show() method, which will bring it up.
//
// There is a single display_gtk_window() method; it takes one
// argument, a gS::WinObj panel identifier. The very first time
// this method is invoked, it creates a new thread - the GTK
// thread. The GTK thread is provided with the panel identifier.
// It creates the required panel and invokes its run() method.
//
// At that point, there are two possibilities:
//
// 	1. the user closes the GTK panel just created,
// 	   e. g. by clicking OK or Cancel
//
// 	2. the user wants to invoke a second GTK panel
//
// In case 1, the GTK thread call to the first panel's run()
// method returns. But we do not let the GTK thread terminate,
// because of the possibility that another GTK panel might
// be requested by the user. Instead, the thread tries to
// unlock a lock that is held by the main APGenX process;
// it will only be released when the main APGenX process either
//
// 	a. terminates, and wants to join the GTK thread
//
// 	b. requests a new (or the same) GTK panel
//
// In case (a), the GTK thread returns; in case (b), the GTK
// thread creates the requested panel if necessary or reuses
// the panel already created, then invokes its run() method.
//
// In case 2, the APGenX engine needs to get the attention of
// the GTK thread.
//
apgen::RETURN_STATUS	glS::fire_up_globwin(Cstring&) {
    {
	lock_guard<mutex> lock1(*gS::get_gtk_mutex());

	if(glS::panel_active) {

	    //
	    // We ignore the request
	    //
	    return apgen::RETURN_STATUS::SUCCESS;
	}
    }


    static gS::WinObj	winObj = gS::WinObj::GLOBAL;

    if(!gS::theGtkThread) {
	gS::theGtkThread = new thread(&gS::display_gtk_window, winObj);
    } else {

	//
	// The GTK thread already exists, so we start by locking
	// the gtk mutex:
	//

	// debug
	// cerr << "start_gtk_editor(): locking the gtk mutex...";

	bool must_restart_gtk = false;

	{
	    lock_guard<mutex> lock(*gS::get_gtk_mutex());
	

	    // debug
	    // cerr << " got it.\n";
	    must_restart_gtk = gS::gtk_subsystem_inactive;

	    //
	    // Prevent another request from trying to join
	    //
	    if(must_restart_gtk) {
		gS::gtk_subsystem_inactive = false;
	    }
	}

	if(must_restart_gtk) {
	    gS::theGtkThread->join();
	    delete gS::theGtkThread;
	    gS::theGtkThread
		= new thread(&gS::display_gtk_window, winObj);
	} else {

	    glS::panel_requested = true;
	}
    }
    return apgen::RETURN_STATUS::SUCCESS;
}

void glS::get_the_globals(gS::ISL& all_globs) {
	gS::gtk_editor_array& theArray = all_globs->get_array();

    //
    // First we need to iterate over all the globals. The GlobalType
    // only has one task, which is its constructor.
    //
    task*	glob_constr = Behavior::GlobalType().tasks[0];

    //
    // The global values are stored in GlobalObject. The most convenient
    // way to access the value of global no. i is to use
    //
    //   TypedValue& glob_val = (*behaving_object::GlobalObject)[i];
    // We do this over a loop, using glob_constr->get_varindex().
    //
    std::map<Cstring, int>::const_iterator iter;

    list<string>			empty_list;

    //
    // The integer that keeps track of rows in the Gtk widget.
    //
    int					path_index = 0;

    //
    // varindex is a map with the global name as an index,
    // so they will be listed in alphanumeric order
    //
    for(	iter = glob_constr->get_varindex().begin();
		iter != glob_constr->get_varindex().end();
		iter++) {
	string		name = *iter->first;

	//
	// This is the index into the global constructor, which
	// usually not in alphabetic order; it is determined by
	// the order in which globals are defined in the adaptation.
	//
	int			index = iter->second;
	TypedValue&		value = (*behaving_object::GlobalObject())[index];

	if(value.is_string()) {
		theArray.push_back(
			new gS::de_facto_string(
				name,
				"",
				empty_list,
				gS::add_quotes_to(*value.get_string())));
	} else if(value.is_array()) {
	    if(value.get_array().get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
		std::map<string, string>	names;
		std::map<string, string>	types;
		std::map<string, list<string> >	ranges;
		std::map<string, string>	descriptions;
		string				path_prefix;
		char				buf[70];

		//
		// path_prefix is the path attached by Gtk
		// to this particular global entry.
		//
		sprintf(buf, "%d", index);
		path_prefix = buf;

		names[path_prefix] = name;

		value.extract_array_info(
		    -1,		// indentation (none in this case)

		    path_prefix,// path to parameter or sub-element whose value
				// this is; the path to every array element starts
				// with this prefix

		    names,	// map from paths of array elements to their labels;
				//   paths start with the prefix; colon is the separator
				//   (example: "1:3:0")

		    types,	// map from paths to either "struct" or "list"
				// (only if element is an array)

		    ranges	// presently unused because we don't capture ranges
		    );
		theArray.push_back(
		    gS::handle_documented_array(
			names,
			types,
			descriptions,
			ranges,
			value.get_array(),
			path_prefix));
	    } else {

		//
		// List (possibly empty)
		//
		theArray.push_back(
		    gS::handle_documented_list(
			name,			// global name
			string(),		// description
			list<string>(),		// range
			value.get_array()	// the array to be documented
			));
	    }
	} else if(value.is_boolean()) {
		Cstring temp = value.to_string();
		list<string> the_range;
		the_range.push_back(string("TRUE"));
		the_range.push_back(string("FALSE"));
		theArray.push_back(
			new gS::de_facto_string(
				name,
				string(),
				the_range,
				string(*temp)));
	} else {
		Cstring temp = value.to_string();
		theArray.push_back(
			new gS::de_facto_string(
				name,
				"",
				empty_list,
				string(*temp)));
	}
	path_index++;
    }
}


#endif /* GTK_EDITOR */
