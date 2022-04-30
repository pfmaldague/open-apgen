#ifndef _INC_GTK_BRIDGE_H
#define _INC_GTK_BRIDGE_H

//
// The purpose of this header is to help establish an interface
// bridge between the APGenX core, which uses traditional APGenX
// core classes, and the gtk GUI components, which exclusively
// use the STL and the GTK-- toolkit.
//
// Originally, there was only one GTK-based component, the
// activity editor. Over time, more components showed up: the
// XMLTOL output panel, and the Globals Editor.
//
// The main files supported by this header are
//
//   in APedit-2.4:
//
//	gtk_interface.C
//	gtk_editwin.C
// 	gtk_xmlwin.C
// 	gtk_globwin.C
//
//   in atm_client:
//
// 	gtk_editor_bridge.C
// 	gtk_xmlwin_bridge.C
// 	gtk_globals_bridge.C
//
// This header provides the STL side of the bridge. The .C files also includes
// headers from the core. We do not include any core files here,
// so that this header file can also 'safely' be included in the editor
// (GTK--) code.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <string>
#include <vector>
#include <list>
#include <map>
#include <thread>
#include <mutex>
#include <string.h>
#include "flexval.H"
#include "smartptr.H"


class ListOVal;

extern "C" {
#	include <concat_util.h>
}

using namespace std;



//
// gS stands for "gtkSubsystem"
//
namespace gS {
    extern thread*	theGtkThread;

    enum class WinObj: char {
	EDITOR,
	GLOBAL,
	XMLTOL
    };

    // void		create_gtkSubsystem_main_object();
    void		initializeGtkSubsystem();
    void		display_gtk_window(WinObj);

    mutex*		get_gtk_mutex();
    extern bool		quit_requested;
    extern bool		gtk_subsystem_inactive;

    //
    // Used to kick the Motif Loop out of its slumber
    // when the editor is finished and Motif is supposed
    // to pick up the activity data. This is necessary
    // because the Motif Loop blocks on calls to
    // XtAppNextEvent() until some event occurs.
    //
    void		send_unblocking_event_to_motif();

    class indexed_string_or_list;

    typedef smart_ptr<indexed_string_or_list> ISL;
    typedef vector<ISL> gtk_editor_array;

    string	add_quotes_to(const string &s);
    void	remove_quotes_from(string &s);

    class compact_int {
    public:
	int		i;
	bool		is_infinite;
	compact_int( int j ) :i(j), is_infinite(false) {}
	compact_int( const char* ) :i(0), is_infinite(true) {}
	compact_int(const compact_int &C)
		:i(C.i), is_infinite(C.is_infinite) {}
	~compact_int() {}
	operator int() {return i;}
	bool operator < (int k) {
		return (is_infinite ? false : i < k);
	}
	bool operator <= (int k) {
		return (is_infinite ? false : i <= k);
	}
	bool operator == (int k) {
		return (is_infinite ? false : i == k);
	}
	bool operator != (int k) {
		return (is_infinite ? true : i != k);
	}
	bool operator > (int k) {
		return (is_infinite ? true : i > k);
	}
	bool operator >= (int k) {
		return (is_infinite ? true : i >= k);
	}
	compact_int &operator -=(int K) {
		if(!is_infinite) i -= K;
		return *this;
	}
	compact_int &operator --() {
		if(!is_infinite) i--;
		return *this;
	}
	compact_int &operator --(int) {
		if(!is_infinite) i--;
		return *this;
	}
	compact_int &operator = (int L) {
		i=L;
		is_infinite=false;
		return *this;
	}
    };

    class indexed_string_or_list {
    public:
	string			Key;
	string			Description;
	list<string>		Range;

	//
	// smart_ptr support:
	//
	int			ref_count;
	int			ref() {
		ref_count++;
		return ref_count;
	}
	int			deref() {
		ref_count--;
		return ref_count;
	}


	indexed_string_or_list()
		: ref_count(0) {}
	indexed_string_or_list(	const string &theKey,
				const string &theDescription,
				const list<string> &theRange)
		: Key( theKey ),
			ref_count(0),
			Description( theDescription ),
			Range( theRange ) {}
	virtual ~indexed_string_or_list() {}
	virtual bool			is_string() {
		return false;
	}
	virtual bool			is_list() {
		return false;
	}
	virtual bool			has_labels() const {
		return false;
	}
	const string&			get_key() const {
		return Key;
	}
	const string&			get_descr() const {
		return Description;
	}
	virtual void			set_string_to( const string & ) {}

	//
	// pure virtual methods
	//
	virtual const string&		get_string() const = 0;
	virtual gtk_editor_array&	get_array() = 0;

	virtual indexed_string_or_list*	copy() const = 0;
	virtual int			length() = 0;

	//
	// returns 0 if OK, 1 if truncation took place
	//
	virtual int			transfer_to_stream(buf_struct *s, compact_int &max_length ) = 0;
    };

    class de_facto_string: public indexed_string_or_list {
	string			contents;
    public:
	de_facto_string(	const string &k,
				const string &descr,
				const list<string> &range,
				const string &c )
		: indexed_string_or_list(k, descr, range),
		contents( c ) {}
	~de_facto_string() {}

	bool			is_string() {
		return true;
	}
	const string&		get_string() const {
		return contents;
	}
	void			set_string_to(const string& S) {
		contents = S;
	}

	//
	// pure virtual methods
	//
	indexed_string_or_list*	copy() const;
	virtual gtk_editor_array& get_array() {
		throw(string("Don't call get_array on a string!!"));
	}
	int			length() {
		return contents.length();
	}
	int			transfer_to_stream(buf_struct *s, compact_int &max_length );
    };

    class de_facto_array: public indexed_string_or_list {
	gtk_editor_array	contents;
	bool			HasLabels;
    public:
	de_facto_array(	const string &k,
			const string &descr,
			const list<string> &range,
			bool label_flag )
		: indexed_string_or_list(k, descr, range),
			HasLabels( label_flag) {
	}

	~de_facto_array(); // memory leak... {}

	bool			is_list() {
		return true;
	}

	bool			has_labels() const {
		return HasLabels;
	}

	//
	// Not const, so client is allowed to push_back on contents.
	//
	gtk_editor_array&	get_array() {
		return contents;
	}

	const string&		get_string() const {
		throw(string("Don't call get_string() on a de_facto_array!"));
	}

	//
	// pure virtual methods in base class
	//
	indexed_string_or_list*	copy() const;
	int			transfer_to_stream(buf_struct *s, compact_int &max_length );
	int			length();
    };

    de_facto_array*	handle_array_value(
				const string&			theKey,
				const string&			theParentDescr,
				const list<string>&		theParentRange,
				ListOVal&			AParray,
				bool				describe_children = false);

    de_facto_array*	handle_documented_array(
				map<string, string>&		names,
				map<string, string>&		types,
				map<string, string>&		descriptions,
				map<string, list<string> >&	ranges,
				ListOVal&			AParray,
				const string&			path_prefix);

    de_facto_array*	handle_documented_list(
				const string&			theKey,
				const string&			theParentDescr,
				const list<string>&		theParentRange,
				ListOVal&			AParray);
} // namespace gS

//
// eS stands for "editor Subsystem"
//
namespace eS {

    extern bool		panel_requested;
    extern bool		panel_active;

    extern bool		gtk_editor_data_available;
    extern bool		gtk_editor_requests_redetailing;
    extern bool		gtk_editor_requests_error_display;

    apgen::RETURN_STATUS fire_up_gtk_editor(Cstring&);
    void		grab_gtk_editor_data();
} // namespace eS

//
// namespace for Globals editor window
//
namespace glS {
    extern void		get_the_globals(gS::ISL& allGlobalsVars);

    extern bool		gtk_globwin_data_available;
    extern bool		panel_requested;
    extern bool		panel_active;

    void		grab_gtk_globwin_data();

    string&		theGlobalName();
    string&		theGlobalValue();

    apgen::RETURN_STATUS fire_up_globwin(Cstring&);
}

//
// namespace for XMLTOL output window
//
namespace xS {
    flexval&		theXmlWinData();
    string&		theXmlStartTime();
    string&		theXmlEndTime();

    extern bool		gtk_xmlwin_data_available;
    extern bool		panel_requested;
    extern bool		panel_active;

    void		grab_gtk_xmlwin_data();

    vector<string>&	theResults();
    string&		theFileName();
    string&		theStartTime();
    string&		theEndTime();
    bool&		partial_xmltol();

    apgen::RETURN_STATUS fire_up_xmlwin(Cstring&);
}

extern thread*		get_gtk_thread();
extern void		delete_gtk_editor();
extern bool&		copy_of_GTKdebug();

class editor_config {
public:
	editor_config(bool i, string s)
		: redetail_needed(i),
		time_system(s) {}
	editor_config(const editor_config &ec)
		: redetail_needed(ec.redetail_needed),
		time_system(ec.time_system) {}
	editor_config()
		: redetail_needed(false),
		time_system("") {}
	~editor_config() {}

	bool		redetail_needed;
	string		time_system;
};

class editor_intfc {
	editor_intfc() {}
	~editor_intfc() {}
public:
	static map<string, editor_config>&	get_default_choices();
	static editor_intfc&			get_interface();
	static double				theDurationMultiplier;


	//
	// The global maps into attributes/parameters will be inserted
	// Originally, there were two separate ones (one for attributes,
	// one for parameters.) Now we use a simple convention:
	//
	//    - 1 tree for everything, mirrored in theCurrentValues
	//
	//    - within each tree, 3 top-level arrays: "Attributes",
	// 	"Parameters", "Local Variables"
	//
	// theCurrentValues is a copy of Everything that holds currently edited
	// values. When the user finally hits OK or Apply, the contents of
	// theCurrentValues are stored in the official Everything structure.
	// It remains to collect the leaves of Everything into TypedValues and
	// stick those into an action request.
	//
	gS::gtk_editor_array Everything, theCurrentValues;

				// lists containing the legends and the epochs
	list<string>		theLegends, theEpochs;

				/* for the editor to stick edited values in, with the path
				 * string (example: 1:2:2:4:0) as an index. All nodes are
				 * stored. Leave nodes should have their values set directly
				 * by the editor. Inner nodes values are derived from them
				 * and should be processed with the 'transfer_to_stream' method. */
	map<string, gS::ISL>	theEditedValues;

				// actual legend
	string			instance_legend;
				// actual color
	string			instance_color;

				// the name used to denote the instance type (e. g. type, chameleon)
	string			type_label;

				// the type of the instance (e. g. downlink, cpu_on)
	string			type_name;

				// the various types available (note: unused)
	list<string>		theTypes;

				// name etc.
	string			instance_name, instance_id, instance_start, instance_duration,
				instance_duration_formula, instance_description;
};


extern "C" {

	//
	// 1. Utilities for conversion between time systems
	//
	typedef int		(*time_conv_func)(
					const char *the_orig_time_string,
					const char *the_epoch_or_time_system,
					const char **the_converted_time,
					const char **any_errors );
	typedef int		(*dur_conv_func)(
					const char *the_orig_dur_string,
					const char *the_epoch_or_time_system,
					const char **the_converted_dur,
					double *the_multiplier,
					const char **any_errors );
	typedef const char	*(*colorfinder_func)(int i);
	typedef int		(*param_parsing_func)(const char* paramlist, char** errors);

	extern time_conv_func	TimeConversionFunc;
	extern dur_conv_func	DurConversionFunc;
	extern colorfinder_func	ColorFunc;
	extern param_parsing_func ParamParsingFunc;

	extern char		*(*TypeDefFunc)();
	extern char		*(*ParamDefFunc)();
	extern char		*default_time_system;

	//
	// 2. Utilities for recomputing duration (future: other attributes?) based
	//    on parameter values edited by the user
	//
	typedef int		(*dur_update_func)(
					char **new_value,
					char **any_errors);
	extern dur_update_func	DurUpdateFunc;

} /* extern "C" */

#endif /* _INC_GTK_BRIDGE_H */
