#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "AP_exp_eval.H"	// for most instance-related data and pE_extra
#include <ActivityInstance.H>
#include "APmodel.H"		// for model_intfc
#include "action_request.H"	// for EDIT_ACTIVITYrequest
#include "UI_mainwindow.H"
#include "gtk_bridge.H"

#ifdef GTK_EDITOR



bool		xS::gtk_xmlwin_data_available = false;

flexval&	xS::theXmlWinData() {
	static flexval F;
	return F;
}

string&		xS::theXmlStartTime() {
	static string s;
	return s;
}

string&		xS::theXmlEndTime() {
	static string s;
	return s;
}

vector<string>&	xS::theResults() {
	static vector<string> v;
	return v;
}

string&		xS::theFileName() {
	static string s;
	return s;
}

bool&		xS::partial_xmltol() {
	static bool b = true;
	return b;
}

void xS::grab_gtk_xmlwin_data() {
	WRITE_XMLTOLrequest*	theRequest;
	Cstring			theOptionalFilter;
	strintslist		sel;
	strint_node*		N;
	const char*		APGEN_XSLT_FILTERS = getenv("APGEN_XSLT_FILTERS");
	Cstring			theOptionalSchema;
	Cstring			theOptionalTimesystem;
	bool			AllActsVisible = !xS::partial_xmltol();

	//
	// for now we don't mess with filter, schema, time systems etc.
	//
	theRequest = new WRITE_XMLTOLrequest(
		xS::theFileName().c_str(),
		xS::theXmlStartTime().c_str(),
		xS::theXmlEndTime().c_str(),
		theOptionalFilter,
		theOptionalSchema,
		theOptionalTimesystem,
		AllActsVisible,
		xS::theResults());
	theRequest->process();
	Cstring msg("Done creating XMLTOL request.");
	UI_mainwindow::setStatusBarMessage(msg);
}

//
// When you want to pop up a new gTK window, start a new pthread
// with display_gtk_window() as its function argument:
//
apgen::RETURN_STATUS xS::fire_up_xmlwin(
		Cstring&	any_errors) {
    {
	lock_guard<mutex> lock1(*gS::get_gtk_mutex());

	//
	// need to get all resource subsystems
	//
	if(panel_active) {

	    //
	    // We ignore the request
	    //
	    return apgen::RETURN_STATUS::SUCCESS;
	}

	panel_active = true;
    }
    
    theXmlWinData().clear();
    eval_intfc::get_resource_subsystems(theXmlWinData());
    CTime st, en;
    model_intfc::FirstEvent(st);
    model_intfc::LastEvent(en);
    theXmlStartTime() = *st.to_string();
    theXmlEndTime() = *en.to_string();

    static gS::WinObj	winObj = gS::WinObj::XMLTOL;

    if(!gS::theGtkThread) {
	gS::theGtkThread = new thread(&gS::display_gtk_window, winObj);
	
    } else {

	//
	// The GTK thread already exists, so we start by locking
	// the gtk mutex:
	//

	// debug
	// cerr << "fire_up_xmlwin(): locking the gtk mutex...";

	bool must_restart_gtk = false;

	{
	    lock_guard<mutex> lock(*gS::get_gtk_mutex());
	

	    // debug
	    // cerr << " got it.\n";
	    must_restart_gtk = gS::gtk_subsystem_inactive;

	    //
	    // Prevent an other request from trying to join:
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
	    xS::panel_requested = true;
	}
    }
    return apgen::RETURN_STATUS::SUCCESS;
}

#endif /* ifdef GTK_EDITOR */
