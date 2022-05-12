#ifndef ACTION_REQUEST_H
#define ACTION_REQUEST_H

#include <fstream>
#include <string>

#include "APdata.H"
#include "UTL_stl.H"
#include "APbasic.H"
#include "ParsedExpressionSystem.H"
#include "CompilerIntfc.H"
#include "perf_util.H"

using std::string;

class thread_intfc;

static char NO_POUND_MARKER[2] =  { (char) 29,'\0' }; // unprintable char "^]"
#define END_MSG_MARKER (char) 27 // unprintable char '^['
#define POUND "#"
 
class Log {
	// fstream *fout; // BKL changed from ofstream fstream 9-25-97
	// PFM 10/1/97
	std::ofstream *fout; // PFm changed it back
	// std::fstream *tempf; BKL added 9-25-97
public:
	Log();
	~Log();

// to replace global variable:
	static Log&		LOG();
	static aoString&	theList();

	Log&			operator << (const Cstring & S)
					{ return (*this) << *S; }
	Log&			operator << (const char*);
	Log&			operator << (char);
	Log&			operator << (short);
	Log&			operator << (int);
	Log&			operator << (long);
	Log&			operator << (float);
	Log&			operator << (double);
	Log&			operator << (unsigned char);
	Log&			operator << (unsigned short);
	Log&			operator << (unsigned int);
	Log&			operator << (unsigned long);
	Log&			operator << (void*);
	Log&			operator << (std::streambuf*);
	Log&			operator << (std::ostream & (*func) (std::ostream &));
	Log&			operator << (std::ios & (*func) (std::ios &));
	char*			output();
	friend void		set_log_file_to(const char *L); }; // added by BKL

Cstring add_pound_sign(const Cstring& S);
static bool serveropt() {
	return false;
}

class OPEN_FILErequest;

class level_handler {
public:
	level_handler();
	~level_handler();
};

class Action_request : public baseC<alpha_string, Action_request, OPEN_FILErequest*> {
private:
	Cstring Modifier;
protected:
	Cstring		commandType;
	Cstring		commandText;
	apgen::RETURN_STATUS theStatus;

	/* Local copy of execution_level at Action_request construction time.
	 * This is what really determines whether or not this Action_request
	 * should be commented out in the log file.
	 *
	 * NOTE: Adam did this. The previous method, which used the run-time
	 * value of execution_level, was flawed. */
	bool		CommentInLog;
	bool		shouldCaptureErrors;
	Cstring		*captured;
public:

	typedef enum save_option {
		INCLUDE_AS_CODE,
		INCLUDE_AS_COMMENTS,
		INCLUDE_NOT_AT_ALL } save_option;

	Action_request(const Cstring &cmd);
	Action_request(const Action_request & AA);
	virtual ~Action_request() {
		if(list) {
			list->remove_node(this);
		}
		list = NULL;
	}
  
	Action_request*			get_this() {
		return this;
	}
	const Action_request*		get_this() const {
		return this;
	}

	const Cstring&			get_key() const {
		return commandType;
	}

	static void			executeRequests(
		       			    slist<
						alpha_string,
						Action_request,
						OPEN_FILErequest*>&
							the_commands);

	void				set_error_string_to(Cstring *);
	Cstring				get_command_text_with_modifier();
	virtual const Cstring&		get_command_text();
	virtual apgen::RETURN_STATUS	get_pointers() {
		return apgen::RETURN_STATUS::SUCCESS;
	}
	apgen::RETURN_STATUS		process();
	bool				validate();
	virtual void 			process_middle(TypedValue* = NULL) {
		return;
	}

	//
	// Overridden 8/28/2000 to avoid double entries in LOG when using CMDrequest
	//
	virtual void			common_process_first();
	virtual void			common_process_last();

	virtual void			to_script(Cstring& outString) const = 0;

	//
	// Does what it says, and also displays the error(s) to the user if
	// the script level is 0 (i. e. we are not in the middle of a script)
	//
	void				add_error(
						const Cstring& str);
  
	//
	// Same as above, but reports a list of errors - for example
	// because the user is setting the "persistent script" option
	//
	// NOTE: the call empties the list, getting it
	// ready for the the next batch of mistakes...
	//
	void				add_error(
						stringslist& l);

	//
	// Errors are dumped into the master error list at end of processing
	//
	stringslist			local_errors;
	Cstring				get_local_errors();
	static Cstring			get_error_message();

	void				add_id_to_list(
						const Cstring&);
	Cntnr<alpha_string, ActivityInstance*>*
					find_tag_from_real_or_symbolic_id(
						const Cstring& s);
	static int			script_execution_level;
	static bool&			persistent_scripts_enabled();
	static bool&			unconsolidated_files_exist();
	static bool			RAMreport;
	void				set_status(
						apgen::RETURN_STATUS status) {
		theStatus = status;
	}
	apgen::RETURN_STATUS		get_status() const {
		return theStatus;
	} 
	void				set_modifier(
						const Cstring& modifier) {
		Modifier = modifier;
	}
	Cstring				get_modifier() const {
		return Modifier;
	}

	static TypedValue		process_an_AAF_request(
						const Cstring&	requestName,
						ListOVal&	arguments);
	static void			set_to_selection(
						TypedValue*);
};

typedef slist<alpha_string, Action_request, OPEN_FILErequest*> commandslist;

// base class for owners of activity lists:
class ACTIVITY_LISTrequest : public Action_request {
protected:
	slist<alpha_void, dumb_actptr>	list_of_act_pointers;
	stringslist			list_of_act_names;
	bool				All;
public:
	ACTIVITY_LISTrequest(
			const Cstring& type,
			const stringslist& list_of_names,
			bool all = false)
		: Action_request(type),
			All(all),
			list_of_act_names(list_of_names) {}
	ACTIVITY_LISTrequest(
			const Cstring& type,
			const slist<alpha_void, dumb_actptr>& list_of_pointers,
			bool all = false)
		: Action_request(type),
			All(all),
			list_of_act_pointers(list_of_pointers) {}
	ACTIVITY_LISTrequest(
			const Cstring& type,
			Cstring& errors,
			ListOVal* args);
	ACTIVITY_LISTrequest(const ACTIVITY_LISTrequest& AL)
		: Action_request(AL),
			list_of_act_pointers(AL.list_of_act_pointers),
			list_of_act_names(AL.list_of_act_names),
			All(AL.All) {}
	~ACTIVITY_LISTrequest() {}

			// to assist derived classes' get_cmd_text():
	void		append_act_list_to_cmd_text(
				Cstring& outString,
				bool quoted = false) const;
			// converts from act names to act pointers:
	apgen::RETURN_STATUS extract_pointers(
				bool& act_list_contains_noneditable_activities);
			// calls extract_pointers and takes appropriate action
	apgen::RETURN_STATUS get_pointers();
	void		set_selection();
	void		removeUnwantedDescendants();
};

class ABSTRACT_ACTIVITYrequest : public ACTIVITY_LISTrequest {
private:
	bool Fully;
	slist<alpha_void, dumb_actptr> ptrs_to_acts_to_abstract;
public:
	ABSTRACT_ACTIVITYrequest(const stringslist& list_of_names, bool full = false);
	ABSTRACT_ACTIVITYrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers, bool full = false);
	ABSTRACT_ACTIVITYrequest(Cstring& errors, ListOVal*);
	ABSTRACT_ACTIVITYrequest(const ABSTRACT_ACTIVITYrequest & AA)
		: ACTIVITY_LISTrequest(AA),
		Fully(AA.Fully),
		ptrs_to_acts_to_abstract(AA.ptrs_to_acts_to_abstract)
		{}
	~ABSTRACT_ACTIVITYrequest() {}

	Action_request*	copy() { return new ABSTRACT_ACTIVITYrequest(*this); }
	const Cstring&	get_command_text();

	void		to_script(Cstring& s) const {
				s << "xcmd(\"ABSTRACTACTIVITY\", [\"activities\" = [";
				append_act_list_to_cmd_text(s, true);
				s << "], \"Fully\" = ";
				if(Fully) {
					s << "true"; }
				else {
					s << "false"; }
				s << "]);\n"; }
	void		process_middle(TypedValue*);
private:
	void		process_middle_private(bool report_errors); };

class ABSTRACT_ALLrequest : public Action_request {
public:
	ABSTRACT_ALLrequest();
	ABSTRACT_ALLrequest(Cstring& errors, ListOVal*);
	~ABSTRACT_ALLrequest() {}

	Action_request*	copy() { return new ABSTRACT_ALLrequest(); }
	const Cstring&	get_command_text();

	void		to_script(Cstring& s) const {
				s << "xcmd(\"ABSTRACTALL\", []);\n"; }
	void		process_middle(TypedValue*); };

class ABSTRACT_ALL_QUIETrequest : public Action_request {
public:
	ABSTRACT_ALL_QUIETrequest();
	ABSTRACT_ALL_QUIETrequest(Cstring& errors, ListOVal*);
	~ABSTRACT_ALL_QUIETrequest() {}

	Action_request*	copy() { return new ABSTRACT_ALL_QUIETrequest(); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
				s << "xcmd(\"ABSTRACTALL_QUIET\", []);\n"; }
	void		process_middle(TypedValue*);
private:
	void		process_middle_private();
	bool		more_left; };

class ADD_RESOURCErequest : public Action_request {
public:
	ADD_RESOURCErequest(const stringslist& ar_list);
	ADD_RESOURCErequest(Cstring& errors, ListOVal*);
	~ADD_RESOURCErequest() {}

	Action_request* copy() { return new ADD_RESOURCErequest(list); }

	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
				emptySymbol *es = list.first_node();
				s << "xcmd(\"ADDRESOURCE\", [\"resource\" = "
					<< addQuotes(es->get_key())
					<< "\"display\" = " 
					<< addQuotes(es->next_node()->get_key())
					<< "]);\n"; }
	void		process_middle(TypedValue*);
	stringslist	list;
	};

class BUILDrequest: public Action_request {
public:
	BUILDrequest();
	BUILDrequest(Cstring& errors, ListOVal* args);
	~BUILDrequest() {}
	Action_request* copy() { return new BUILDrequest(); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {}
	void		process_middle(TypedValue*);
};

class CLOSE_ACT_DISPLAYrequest : public Action_request {
// BKL-1-20-98 totally messed with this entire class !!!!!!!!
// for scripting capability
public:
	CLOSE_ACT_DISPLAYrequest(const stringslist& alist);
	CLOSE_ACT_DISPLAYrequest(Cstring& errors, ListOVal* args);
	~CLOSE_ACT_DISPLAYrequest() {}

	Action_request* copy() { return new CLOSE_ACT_DISPLAYrequest(my_list); }

	stringslist my_list; // BKL-1-20-98
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const;
	void		process_middle(TypedValue*); };

class CLOSE_RES_DISPLAYrequest : public Action_request {
// BKL-12/97 totally messed with this entire class !!!!!!!!
// for scripting capability
public:
	CLOSE_RES_DISPLAYrequest(const stringslist& alist);
	CLOSE_RES_DISPLAYrequest(const StringVect& resesToClose);
	CLOSE_RES_DISPLAYrequest(Cstring& errors, ListOVal* args);
	~CLOSE_RES_DISPLAYrequest() {}

	Action_request* copy() { return new CLOSE_RES_DISPLAYrequest(my_list); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const;
	stringslist	my_list; // BKL-12/97
	void		process_middle(TypedValue*); };

class COPY_ACTIVITYrequest : public ACTIVITY_LISTrequest {
public:
	// not real OO, but easy to remember... pass pointer to list of pointers,
	/// reference to list of names:
	COPY_ACTIVITYrequest(const stringslist& list_of_names);
	COPY_ACTIVITYrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers);
	COPY_ACTIVITYrequest(const COPY_ACTIVITYrequest& CA)
		: ACTIVITY_LISTrequest(CA)
		{}
	COPY_ACTIVITYrequest(Cstring& errors, ListOVal* args);
	~COPY_ACTIVITYrequest() {}

	Action_request* copy() { return new COPY_ACTIVITYrequest(*this); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const;
	void		process_middle(TypedValue*); };


class XCMDrequest: public ACTIVITY_LISTrequest {
	Cstring	request;
public:
	XCMDrequest(const stringslist& list_of_names, const Cstring& req);
	XCMDrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers, const Cstring& req);
	XCMDrequest(const XCMDrequest& x)
		: ACTIVITY_LISTrequest(x),
		request(x.request) {}
	~XCMDrequest() {}

	Action_request*	copy() { return new XCMDrequest(*this); }
	const Cstring&	get_command_text();
	apgen::RETURN_STATUS get_pointers();
	void		to_script(Cstring& s) const {
				s << "xcmd(" << addQuotes(request) << ", [\"activities\" = [";
				append_act_list_to_cmd_text(s, true);
				s << "]]);\n"; }
	void		process_middle(TypedValue*); };

class CUT_ACTIVITYrequest : public ACTIVITY_LISTrequest {
public:
	CUT_ACTIVITYrequest(const stringslist& list_of_names);
	CUT_ACTIVITYrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers);
	CUT_ACTIVITYrequest(Cstring& errors, ListOVal* args);
	CUT_ACTIVITYrequest(const CUT_ACTIVITYrequest& CA)
		: ACTIVITY_LISTrequest(CA),
		please_delete(CA.please_delete)
		{}
	~CUT_ACTIVITYrequest() {}

	Action_request*	copy() { return new CUT_ACTIVITYrequest(*this); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
				s << "xcmd(\"CUTACTIVITY\", [\"activities\" = [";
				append_act_list_to_cmd_text(s, true);
				s << "], \"delete\" = ";
				if(please_delete) {
					s << "true"; }
				else {
					s << "false"; }
				s << "]);\n"; }
	bool		please_delete;
	void		process_middle(TypedValue*); };

class DELETE_ACTIVITYrequest : public ACTIVITY_LISTrequest {
public:
	// not real OO, but easy to remember... pass pointer to list of pointers,
	/// reference to list of names:
	DELETE_ACTIVITYrequest(const stringslist& list_of_names);
	DELETE_ACTIVITYrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers);
	DELETE_ACTIVITYrequest(Cstring& errors, ListOVal*);
	DELETE_ACTIVITYrequest(const DELETE_ACTIVITYrequest & CA)
		: ACTIVITY_LISTrequest(CA)
		{}
	~DELETE_ACTIVITYrequest() {}

	Action_request*		copy() { return new DELETE_ACTIVITYrequest(*this); }
	const Cstring&		get_command_text();
	void			to_script(Cstring& s) const {
					s << "xcmd(\"DELETEACTIVITY\", [\"activities\" = [";
					append_act_list_to_cmd_text(s, true);
					s << "]]);\n"; }
	void			process_middle(TypedValue*); };

class DEBUGrequest : public Action_request {
	Cstring		on;
public:
	DEBUGrequest(const Cstring& l_name);
	DEBUGrequest(Cstring& errors, ListOVal* args);
	~DEBUGrequest() {}

	Action_request *copy() { return new DEBUGrequest(on); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
				s << "xcmd(\"DEBUG\", [\"on\" = "
					<< addQuotes(on)
					<< "]);\n"; }
	void		process_middle(TypedValue*); };

class DELETE_LEGENDrequest : public Action_request {
	Cstring		name;
public:
	DELETE_LEGENDrequest(const Cstring& l_name);
	~DELETE_LEGENDrequest() {}

	Action_request *copy() { return new DELETE_LEGENDrequest(name); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
				s << "xcmd(\"DELETELEGEND\", [\"name\" = "
					<< addQuotes(name) << "]);\n"; }
	void		process_middle(TypedValue*); };


class DETAIL_ACTIVITYrequest : public ACTIVITY_LISTrequest {
	bool				New, Fully;
	tlist<alpha_void, dumb_actptr>	ptrs_to_acts_to_decompose;
	int				ResolutionChoice;
public:
	DETAIL_ACTIVITYrequest(const stringslist& list_of_names, bool NewDetailize, int which_res,
		int secret_message = 0, bool full = false);
	DETAIL_ACTIVITYrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers, bool NewDetailize, int which_res,
		int secret_message = 0, bool full = false);
	DETAIL_ACTIVITYrequest(Cstring& errors, ListOVal*);
	DETAIL_ACTIVITYrequest(const DETAIL_ACTIVITYrequest &DA)
		: ACTIVITY_LISTrequest(DA),
		New(DA.New),
		Fully(DA.Fully),
		ptrs_to_acts_to_decompose(DA.ptrs_to_acts_to_decompose)
		{}

	~DETAIL_ACTIVITYrequest() {}

	Action_request *copy() { return new DETAIL_ACTIVITYrequest(*this); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(\"DETAILACTIVITY\", [\"activities\" = [";
		append_act_list_to_cmd_text(s, true);
		s << "], \"redetail\" = ";
		if(New) {
			s << "true";
		} else {
			s << "false";
		}
		s << ", \"full\" = ";
		if(Fully) {
			s << "true";
		} else {
			s << "false";
		}
		s << "]);\n";
	}
	void		process_middle(TypedValue*);
private:
	void		process_middle_private(bool report_errors);
};

class DELETE_ALL_DESCENDANTSrequest : public Action_request {
public:
	DELETE_ALL_DESCENDANTSrequest();
	DELETE_ALL_DESCENDANTSrequest(Cstring& errors, ListOVal*);
	~DELETE_ALL_DESCENDANTSrequest() {}

	Action_request *copy() { return new DELETE_ALL_DESCENDANTSrequest(); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
				s << "xcmd(\"DELETEALLDESCENDANTS\", []);\n"; }
	void		process_middle(TypedValue*); };


class DRAG_CHILDRENrequest : public Action_request {
	int		on;
public:
	DRAG_CHILDRENrequest(int);
	~DRAG_CHILDRENrequest() {}

	Action_request *copy() { return new DRAG_CHILDRENrequest(on); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
				s << "xcmd(\"DRAGCHILDREN\", [\"on\" = "
					<< on << "]);\n"; }
	void		process_middle(TypedValue*); };


class DETAIL_ALLrequest : public Action_request {
	int		New;
public:
	DETAIL_ALLrequest(int NewDetailize);
	DETAIL_ALLrequest(Cstring& errors, ListOVal*);
	~DETAIL_ALLrequest() {}

	Action_request*	copy() { return new DETAIL_ALLrequest(New); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
				s << "xcmd(\"DETAILALL\", [\"new\" = "
					<< New << "]);\n"; }
	void		process_middle(TypedValue*); };

class DETAIL_ALL_QUIETrequest : public Action_request {
	int		New;
public:
	DETAIL_ALL_QUIETrequest(int NewDetailize);
	DETAIL_ALL_QUIETrequest(Cstring& errors, ListOVal*);
	~DETAIL_ALL_QUIETrequest() {}

	Action_request *copy() { return new DETAIL_ALL_QUIETrequest(New); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
				s << "xcmd(\"DETAILALL_QUIET\", [\"new\" = "
					<< New << "]);\n"; }
	void		process_middle(TypedValue*);
private:
	void		process_middle_private();
	bool		more_left;
};

class EDIT_ACTIVITYrequest : public Action_request {
	pairslist	assign_list;
	int		carry;
	Cstring		nodeID;	// unique ID of ACT_req being acted upon (DSG 97-06-18)
public:
	EDIT_ACTIVITYrequest(const pairslist& elist, int n, const Cstring& stringID);
	EDIT_ACTIVITYrequest(Cstring& errors, ListOVal*);
	~EDIT_ACTIVITYrequest() {}

	Action_request*	copy() {
		return new EDIT_ACTIVITYrequest(assign_list, carry, nodeID);
	}
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const;
	void		process_middle(TypedValue*);
};

class EDIT_GLOBALrequest : public Action_request {
	Cstring		global_name;
	Cstring		global_value;
public:
	EDIT_GLOBALrequest(const Cstring& name, const Cstring& val);
	EDIT_GLOBALrequest(Cstring& errors, ListOVal*);
	~EDIT_GLOBALrequest() {}

	Action_request*	copy() {
		return new EDIT_GLOBALrequest(global_name, global_value);
	}
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const;
	void		process_middle(TypedValue*);
};

class FILE_CONSOLIDATErequest : public Action_request {
	stringslist		filelist;
	Cstring			name;
public:
	FILE_CONSOLIDATErequest(const stringslist& flist, const Cstring& cname);
	FILE_CONSOLIDATErequest(Cstring& errors, ListOVal*);
	~FILE_CONSOLIDATErequest() {}

	Action_request*	copy() { return new FILE_CONSOLIDATErequest(filelist, name); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const;
	void		process_middle(TypedValue*); };

class GRAB_IDrequest : public Action_request {
private:
	Cstring		id_name;
public:
	GRAB_IDrequest(const Cstring& c)
		: Action_request("GRAB"),
		id_name(c) {}
	GRAB_IDrequest(const GRAB_IDrequest& GI)
		: Action_request(GI),
		id_name(GI.id_name) {}
	~GRAB_IDrequest() {}

	Action_request*	copy() { return new GRAB_IDrequest(*this); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class GROUP_ACTIVITIESrequest : public ACTIVITY_LISTrequest {
	Cstring			theNewParentId, theNewParentName, theNewChameleonName,
				theTemplateName, theChameleonTypeName;
public:
	/* Not real OO, but easy to remember... pass pointer to list of pointers, reference to list of names.
	 * If req_flag == 0 and cham_flag == 0, then n is the ID of an activity to be chosen as the parent. 
	 * Else, only one of the 2 flags should be set; in either case, n is the desired parent name. */
	GROUP_ACTIVITIESrequest(const stringslist& list_of_names, const Cstring& n, int req_flag, int cham_flag);
	// The second type of list; same conventions for the other parameters.
	GROUP_ACTIVITIESrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers, const Cstring& n, int req_flag, int cham_flag);
	// Similar to the above but with a requested activity type (or template, or chameleon type as appropriate)
	GROUP_ACTIVITIESrequest(const stringslist& list_of_names, const Cstring& n, const Cstring& Type, int req_flag, int cham_flag);
	// Same but with the other type of list.
	GROUP_ACTIVITIESrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers,	const Cstring& n, const Cstring& Type, int req_flag, int cham_flag);
	// Used when the parent is a pre-existing activity (could be a request or a chameleon)
	GROUP_ACTIVITIESrequest(const stringslist& list_of_names, const Cstring& theId);
	// Same with the other type of list.
	GROUP_ACTIVITIESrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers, const Cstring& theId);
	// The infamous copy constructor.
	GROUP_ACTIVITIESrequest(const GROUP_ACTIVITIESrequest & CA)
		: ACTIVITY_LISTrequest(CA),
		theNewParentId(CA.theNewParentId),
		theNewParentName(CA.theNewParentName),
		theNewChameleonName(CA.theNewChameleonName),
		theTemplateName(CA.theTemplateName),
		theChameleonTypeName(CA.theChameleonTypeName)
		{}

	~GROUP_ACTIVITIESrequest() {}

	Action_request*	copy() { return new GROUP_ACTIVITIESrequest(*this); }
	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };

class UNGROUP_ACTIVITIESrequest : public ACTIVITY_LISTrequest {
public:
	// not real OO, but easy to remember... pass pointer to list of pointers,
	// reference to list of names:
	UNGROUP_ACTIVITIESrequest(const stringslist& list_of_names);
	UNGROUP_ACTIVITIESrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers);
	UNGROUP_ACTIVITIESrequest(const UNGROUP_ACTIVITIESrequest& CA)
		: ACTIVITY_LISTrequest(CA)
		{}
	UNGROUP_ACTIVITIESrequest(Cstring& errors, ListOVal* args);
	~UNGROUP_ACTIVITIESrequest() {}

	Action_request*	copy() { return new UNGROUP_ACTIVITIESrequest(*this); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };

class UNFREEZE_RESOURCESrequest : public Action_request {
public:
	UNFREEZE_RESOURCESrequest();
	~UNFREEZE_RESOURCESrequest() {}

	Action_request *copy() { return new UNFREEZE_RESOURCESrequest(); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class MOVE_ACTIVITYrequest : public ACTIVITY_LISTrequest {
	CTime_base		act_time, act_duration;
	Cstring		act_legend;
public:
	MOVE_ACTIVITYrequest(const stringslist& list_of_names, const CTime_base &time,
	                 const CTime_base& duration, const Cstring& legend);
	MOVE_ACTIVITYrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers, const CTime_base& time,
	                 const CTime_base& duration, const Cstring& legend);
	MOVE_ACTIVITYrequest(const MOVE_ACTIVITYrequest& MA)
		: ACTIVITY_LISTrequest(MA),
		act_time(MA.act_time),
		act_duration(MA.act_duration),
		act_legend(MA.act_legend)
		{}

	~MOVE_ACTIVITYrequest() {}

	Action_request *copy() { return new MOVE_ACTIVITYrequest(*this); }
	const Cstring & get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*);
	const CTime_base & get_time();
	const CTime_base & get_duration();
	apgen::RETURN_STATUS	get_pointers();
	const Cstring & get_legend(); };

class MOVE_LEGENDrequest : public Action_request {
	Cstring name;
	int legend_index;
public:
	MOVE_LEGENDrequest(const Cstring &l_name, int n);
	~MOVE_LEGENDrequest() {}

	Action_request *copy() { return new MOVE_LEGENDrequest(name, legend_index); }
	const Cstring & get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class NEW_ACTIVITYrequest : public Action_request {
	Cstring		act_type, act_id, act_legend, act_name, act_time;
	pairslist	params_as_kwd_val_pairs;
public:
	NEW_ACTIVITYrequest(const Cstring &type, const Cstring & the_name, const Cstring &id, 
	                const Cstring &time, const Cstring &legend);
	NEW_ACTIVITYrequest(
			const Cstring&	type,
			const Cstring&	the_name,
			const Cstring&	id, 
	                const Cstring&	time,
			const Cstring&	legend,
	                const pairslist& parameters_keyword_value_pairs);
	NEW_ACTIVITYrequest(Cstring& errors, ListOVal*);
	~NEW_ACTIVITYrequest() {}

	Action_request*	copy() {
		return new NEW_ACTIVITYrequest(
				act_type,
				act_name,
				act_id,
				act_time,
				act_legend,
				params_as_kwd_val_pairs);
	}
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);
	const Cstring&	get_type();
	const Cstring&	get_name();
	const Cstring&	get_id();
	const Cstring&	get_time() const;
	const Cstring&	get_legend();
};


class NEW_ACTIVITIESrequest : public Action_request {
	Cstring		Type, Legend, StartTime, Period;
	int		NumTimes;
public:
	NEW_ACTIVITIESrequest(	const Cstring &type,
				const Cstring &startTime,
				const Cstring &period, 
				int num_times,
				const Cstring &legend);
	NEW_ACTIVITIESrequest(Cstring& errors, ListOVal*);
        ~NEW_ACTIVITIESrequest() {};

	Action_request *copy() { return new NEW_ACTIVITIESrequest(Type, StartTime,
							Period, NumTimes, Legend);};
	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };



class NEW_ACT_DISPLAYrequest : public Action_request {
public:
CTime_base startTime; /// BKL-1-20-98
CTime_base duration; // BKL-1-20-98
// BKL-1-20-98 totally messed with this entire class !!!!!!!!
/// for scripting capability

	NEW_ACT_DISPLAYrequest(CTime_base start_time, CTime_base dur_time);
	~NEW_ACT_DISPLAYrequest() {}

	Action_request *copy() { return new NEW_ACT_DISPLAYrequest(startTime, duration); }
	const Cstring & get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class NEW_LEGENDrequest : public Action_request {

	Cstring legend;

	int	legend_height;
public:


	NEW_LEGENDrequest(const Cstring &legend_name, int theHeight);

	~NEW_LEGENDrequest() {}


	Action_request *copy() { return new NEW_LEGENDrequest(legend, legend_height); }


	const Cstring & get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class NEW_RES_DISPLAYrequest : public Action_request {
public:
	// constructor
	NEW_RES_DISPLAYrequest();
	// destructor
	~NEW_RES_DISPLAYrequest() {}

	Action_request *copy() { return new NEW_RES_DISPLAYrequest(); }

	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class SELECT_RES_LEGENDrequest: public Action_request {
public:
	int	on_or_off;
	Cstring	which_legend;
	Cstring	which_ad;
	// constructor
	SELECT_RES_LEGENDrequest(const Cstring &which_ad, const Cstring &which_legend, int on_or_off);
	// destructor
	~SELECT_RES_LEGENDrequest() {}

	Action_request *copy() { return new SELECT_RES_LEGENDrequest(which_ad, which_legend, on_or_off); }

	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class SELECT_ACT_LEGENDrequest: public Action_request {
public:
	int	on_or_off;
	Cstring	which_legend;
	// constructor
	SELECT_ACT_LEGENDrequest(const Cstring &whichLegend, int on_or_off);
	// destructor
	~SELECT_ACT_LEGENDrequest() {}

	Action_request *copy() { return new SELECT_ACT_LEGENDrequest(which_legend, on_or_off); }

	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class SELECT_ACT_DISPLAYrequest: public Action_request {
public:
	int	on_or_off;
	Cstring	which_display; // A1, A2, A3 ... hopper?
	// constructor
	SELECT_ACT_DISPLAYrequest(const Cstring &whichDisplay, int on_or_off);
	// destructor
	~SELECT_ACT_DISPLAYrequest() {}

	Action_request *copy() { return new SELECT_ACT_DISPLAYrequest(which_display, on_or_off); }

	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class NEW_HORIZONrequest: public Action_request {
public:
	CTime_base		start_time, end_time;

	Cstring		Cstart_time, Cduration, Creal, Cmodeled, whichDisplay;

	NEW_HORIZONrequest(const Cstring &theActsis,
		const Cstring &S, const Cstring &D, const Cstring &R, const Cstring &M)
		: Action_request("NEWHORIZON"),
		start_time(CTime_base(0, 0, false)), end_time(CTime_base(0, 0, false)),
		Cstart_time(S), Cduration(D), Creal(R), Cmodeled(M), whichDisplay(theActsis) {}

	NEW_HORIZONrequest(CTime_base S, CTime_base E)
		: Action_request("NEWHORIZON"), start_time(S), end_time(E), whichDisplay("A1") {}

	NEW_HORIZONrequest(Cstring& errors, ListOVal*);

	NEW_HORIZONrequest(const Cstring &theActsis, CTime_base S, CTime_base E)
		: Action_request("NEWHORIZON"), start_time(S), end_time(E), whichDisplay(theActsis) {}

	NEW_HORIZONrequest(const NEW_HORIZONrequest & nh)
		: Action_request(nh), start_time(nh.start_time), end_time(nh.end_time),
		whichDisplay(nh.whichDisplay) {}

	~NEW_HORIZONrequest() {}


	Action_request		*copy() { return new NEW_HORIZONrequest(*this); }


	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };

class OPEN_FILErequest : public Action_request {
	Cstring				filename;
	Cstring				file_contents;
	compiler_intfc::source_type	the_data_source;
public:
	OPEN_FILErequest(	compiler_intfc::source_type	theSource,
				const Cstring&			theData);
	OPEN_FILErequest(const OPEN_FILErequest& OF)
		: Action_request(OF),
			the_data_source(OF.the_data_source),
			filename(OF.filename),
			file_contents(OF.file_contents),
			the_file_has_commands(OF.the_file_has_commands),
			ListOfCommands(OF.ListOfCommands) {
		}
	// OPEN_FILErequest(const Cstring& fname, const List& act_requests);
	OPEN_FILErequest(Cstring& errors, ListOVal*);
	~OPEN_FILErequest() {};

	Action_request*		copy() {
		return new OPEN_FILErequest(*this);
	}
	const Cstring&		get_command_text();
	void			to_script(Cstring& s) const {
					s << "xcmd(" << addQuotes(commandType) << ", [";
				s << "\"name\" = " << addQuotes(filename) << ", "
					<< "\"content\" = " << addQuotes(file_contents);
				s << "]);\n";
	}
	void			process_middle(TypedValue*);
	bool			the_file_has_commands;
	const Cstring&		get_file_contents() {
		return file_contents;
	}
	commandslist		ListOfCommands;
				// to store the IDs of activities
				// resulting from opening this file: 
	stringtlist		IDs;
	pairtlist		id_variables;
};

class PASTE_ACTIVITYrequest : public ACTIVITY_LISTrequest {
	Cstring		act_time;
	Cstring		act_legend;
public:

	PASTE_ACTIVITYrequest(const stringslist& list_of_names, const Cstring &time, const Cstring &legend);
	PASTE_ACTIVITYrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers, const Cstring& time, const Cstring& legend);
	PASTE_ACTIVITYrequest(const PASTE_ACTIVITYrequest& PA)
		: ACTIVITY_LISTrequest(PA),
		act_time(PA.act_time),
		act_legend(PA.act_legend),
		please_delete(PA.please_delete)
		{}
	~PASTE_ACTIVITYrequest() {}


	Action_request *copy() { return new PASTE_ACTIVITYrequest(*this); }
	const Cstring	&get_command_text();
	apgen::RETURN_STATUS	get_pointers();
	void		process_middle(TypedValue*);
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	bool		please_delete;
	CTime_base	get_time() const;
	const Cstring & get_legend(); };


class PRINTrequest : public Action_request {
public:
	PRINTrequest();
	~PRINTrequest() {}

	Action_request *copy() { return new PRINTrequest(); }
	const Cstring & get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n"; }
	void		process_middle(TypedValue*); };


class PURGErequest : public Action_request {

private:
///Purge Plan only
	bool PlanOnly;
public:
	PURGErequest(bool planOnly = false);
	~PURGErequest() {}

	Action_request* copy() {
		return new PURGErequest();
	}
	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);
};

class QUITrequest : public Action_request {
public:
	QUITrequest(bool fast = false);
	QUITrequest(Cstring& errors, ListOVal*);
	~QUITrequest() {}

	Action_request* copy() { return new QUITrequest(fast_option); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);
	bool		fast_option;
};

class REMODELrequest : public Action_request {
public:
	int			real_time;
	bool			partial;
	bool			from_start;
	CTime_base		to_time;

	//
	// Obsolete - we combine the output requests separately,
	// now, and do not execute them in parallel with remodel.
	// For now, we keep this list around; it might be useful
	// in deciding whether to try multithreading in a different
	// way.
	//
	slist<alpha_string, Action_request, OPEN_FILErequest*> tol_requests;
	REMODELrequest(
		bool Partial,
		CTime_base ToTime,
		bool FromStart,
		int = 0);
	REMODELrequest(Cstring& errors, ListOVal*);
	~REMODELrequest() {
	}

	Action_request* copy() {
		return new REMODELrequest(partial, to_time, real_time);
	}
	static REMODELrequest*	usually_this;
	const Cstring&		get_command_text();
	void			to_script(
					Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", [";

		if(partial) {
			s << "\"partial\" = true, \"to_time\" = ";
			s << to_time.to_string();
		} else {
			s << "false";
		}
		if(from_start) {
			s << "\"from_start\" = true";
		}
		s << "]);\n";
	}
	void			process_middle(TypedValue*);
};


class REMOVE_RESOURCErequest : public Action_request {
public:
	REMOVE_RESOURCErequest();
	~REMOVE_RESOURCErequest() {}

	Action_request *copy() { return new REMOVE_RESOURCErequest(); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);
};

class RESOURCE_SCROLLrequest : public Action_request {
public:
	Cstring		theMinimum, theSpan;
	RESOURCE_SCROLLrequest(const Cstring& the_m, const Cstring& the_s)
		: Action_request("RESOURCESCROLL"),
		theMinimum(the_m), theSpan(the_s) {}

	~RESOURCE_SCROLLrequest() {}

	Action_request*	copy() { return new RESOURCE_SCROLLrequest(theMinimum, theSpan); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*); };

typedef Cntnr<alpha_string, Action_request::save_option> optionNode;

class SAVE_FILErequest : public Action_request {
	Cstring			file_name;
	apgen::FileType		theFileType;
	stringslist		exclude;

				/*
				 *	New-style save options: the blist contains btags with the following keys:
				 *
				 *		APF Epochs
				 *		APF TimeSystems
				 *		APF OtherGlobals
				 *		APF Legends
				 *		APF TimeParameters
				 *		APF WindowSize
				 *
				 */

	tlist<alpha_string, optionNode>	saveOptions;
public:

	SAVE_FILErequest(
			const Cstring&				the_file_name,
			const apgen::FileType&			the_type_of_the_file,
			const stringslist&			excl,
			tlist<alpha_string, optionNode>&	some_options);
	SAVE_FILErequest(Cstring& errors, ListOVal*);
	~SAVE_FILErequest() {};

	Action_request*		copy() {
				return new SAVE_FILErequest(file_name, theFileType, exclude,
					saveOptions); }
	const Cstring&		get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	const stringslist&	get_exclusions() { return exclude; }
	const Cstring&		get_filename() { return file_name; }
	void			process_middle(TypedValue*);
};

class SAVE_PARTIAL_FILErequest : public Action_request {
	stringslist filelist;
	Cstring name;
public:
	SAVE_PARTIAL_FILErequest(const stringslist& flist, const Cstring& wname);
	SAVE_PARTIAL_FILErequest(Cstring& errors, ListOVal*);
	~SAVE_PARTIAL_FILErequest() {}

	Action_request *copy() { return new SAVE_PARTIAL_FILErequest(filelist, name); }
	const Cstring & get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);
};


//
// The following action request has been re-purposed to export
// reports on
//
// 	- interactions between activity types, resources and
//	  globals
//
//	- function declarations (AAF functions only)
//
class EXPORT_DATArequest : public Action_request {
public:
	typedef enum {
			ACTIVITY_INTERACTIONS,
			ACTIVITY_INSTANCES,
			FUNCTION_DECLARATIONS,
			GLOBALS
			} what_to_export;

	//
	// Unused but kept for compatibility
	//
	typedef enum _how_to_export  {
				JSON_FILE
				} how_to_export;
	how_to_export	How;

	what_to_export	What;
	Cstring		Filename;

	EXPORT_DATArequest(
			what_to_export W,
			how_to_export H,
			const Cstring& filename)
		: Action_request("EXPORTDATA"), What(W), How(H), Filename(filename) {}
	~EXPORT_DATArequest() {}

	Action_request*	copy() {
		return new EXPORT_DATArequest(
				What, How,
				Filename);
	};
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);
};


class PAUSErequest: public Action_request {
public:
	Cstring		Text;
	PAUSErequest() : Action_request("PAUSE")
		{}
	PAUSErequest(const Cstring &rem) : Action_request("PAUSE"), Text(rem)
		{}
	~PAUSErequest() {}

	Action_request		*copy() { return new PAUSErequest(); }
	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*); };


class REGEN_CHILDRENrequest : public ACTIVITY_LISTrequest {
public:
	REGEN_CHILDRENrequest(
			const stringslist& list_of_names,
			bool a)
		: ACTIVITY_LISTrequest("REGENCHILDREN", list_of_names, a) {}
	REGEN_CHILDRENrequest(
			const slist<alpha_void, dumb_actptr>& list_of_pointers,
			bool a)
		: ACTIVITY_LISTrequest("REGENCHILDREN", list_of_pointers, a) {}
	REGEN_CHILDRENrequest(Cstring& errors, ListOVal* args);
	REGEN_CHILDRENrequest(const REGEN_CHILDRENrequest& RC)
		: ACTIVITY_LISTrequest(RC) {}
	~REGEN_CHILDRENrequest() {}

	Action_request		*copy() { return new REGEN_CHILDRENrequest(*this); }
	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	apgen::RETURN_STATUS	get_pointers();
	void		process_middle(TypedValue*); };


class SCHEDULE_ACTrequest : public ACTIVITY_LISTrequest {
public:
	bool			incremental;

	SCHEDULE_ACTrequest(
			const stringslist& list_of_names,
			bool a,
			bool incr = false)
		: ACTIVITY_LISTrequest("SCHEDULEACTIVITY", list_of_names, a),
			incremental(incr) {}
	SCHEDULE_ACTrequest(
			const slist<alpha_void, dumb_actptr>& list_of_pointers,
			bool a,
			bool r,
			bool incr = false)
		: ACTIVITY_LISTrequest("SCHEDULEACTIVITY", list_of_pointers, a),
			incremental(incr) {}
	SCHEDULE_ACTrequest(Cstring& errors, ListOVal* args);
	SCHEDULE_ACTrequest(const SCHEDULE_ACTrequest& SA)
		: ACTIVITY_LISTrequest(SA),
			incremental(SA.incremental) {}
	~SCHEDULE_ACTrequest() {}

	Action_request*		copy() {
		return new SCHEDULE_ACTrequest(*this);
	}
	const Cstring&		get_command_text();
	void			to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	apgen::RETURN_STATUS	get_pointers();
	void			process_middle(TypedValue*);
};


class UI_ACTIVITYrequest : public Action_request {
private:
	const string EventHandlerName;
	const string ActivityID;
public:
	UI_ACTIVITYrequest(const string& eventHandlerName, const string activityID)
		: Action_request("UIACTIVITY"),
		  EventHandlerName(eventHandlerName),
		  ActivityID(activityID) {}
	~UI_ACTIVITYrequest() {}

	Action_request*	copy() { return new UI_ACTIVITYrequest(*this); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*); };

class UI_GLOBALrequest : public Action_request
{
private:
	const string EventHandlerName;
	const string LegendName;
	const long X;
	const long Y;
	const CTime_base Time;
public:
	UI_GLOBALrequest(const string& eventHandlerName, const string& legendName, long x, long y, const CTime_base& time)
		: Action_request("UIGLOBAL"),
		  EventHandlerName(eventHandlerName),
		  LegendName(legendName),
		  X(x),
		  Y(y),
		  Time(time) {}

	~UI_GLOBALrequest() {}

	Action_request*	copy() { return new UI_GLOBALrequest(*this); }
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*); };

class UNSCHEDULE_ACTrequest : public ACTIVITY_LISTrequest {

	int		all;
public:
	UNSCHEDULE_ACTrequest(const stringslist& list_of_names, int A)
		: ACTIVITY_LISTrequest("UNSCHEDULEACTIVITY", list_of_names),
		all(A) {}

	UNSCHEDULE_ACTrequest(const slist<alpha_void, dumb_actptr>& list_of_pointers, int A)
		:  ACTIVITY_LISTrequest("UNSCHEDULEACTIVITY", list_of_pointers),
		all(A) {}

	UNSCHEDULE_ACTrequest(Cstring& errors, ListOVal* args);

	UNSCHEDULE_ACTrequest(UNSCHEDULE_ACTrequest &UA)
		: ACTIVITY_LISTrequest(UA),
		all(UA.all) {}

	~UNSCHEDULE_ACTrequest() {}


	Action_request*	copy() { return new UNSCHEDULE_ACTrequest(*this); }


	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	apgen::RETURN_STATUS	get_pointers();
	void		process_middle(TypedValue*); };


class WRITE_TOLrequest : public Action_request {

	//
	// NOTE: the comments below are almost certainly obsolete.
	// Check by looking carefully at cmd_grammar.y.
	//

	//
	// expressions:
	//
			// NOTE: CFileName is ALWAYS quoted (single or double quotes)
	Cstring		Cstart_time, Cend_time, CFileName;

	//
	// evaluated values:
	//
	CTime_base	StartTime, EndTime;

			// NOTE: FileName ALWAYS unquoted because
			// that's how it's evaluated
	Cstring		FileName;

	//
	// constants:
	//
	stringslist	filters;
	Cstring		format;

public:

	//
	// To hold optional XMLTOL request to be executed in parallel:
	//
	slist<alpha_string, Action_request, OPEN_FILErequest*> xmltol_requests;

	bool		running;

	WRITE_TOLrequest(
		const Cstring &desired_filename,
		const Cstring &t1,
		const Cstring &t2,
		const stringslist& filters,
		const Cstring &optional_format);
	WRITE_TOLrequest(Cstring& errors, ListOVal*);
	~WRITE_TOLrequest() {}

	Action_request* copy() {
		return new WRITE_TOLrequest(
				CFileName,
				Cstart_time,
				Cend_time,
				filters,
				format);
	}

	//
	// Returns true if OK, false if errors found
	//
	bool		evaluate_arguments();
	const CTime_base& get_start() const {
		return StartTime;
	}
	const CTime_base& get_end() const {
		return EndTime;
	}
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);

	//
	// Provides a uniform handle for executing the
	// request that works both in single-threaded
	// and multi-threaded environments
	//
	static void	execute(
				thread_intfc* /* ,
				bool evaluate_args = true */ );
	static WRITE_TOLrequest*&	in_process();
};

class WRITE_XMLTOLrequest : public Action_request {

	//
	// strings:
	//
	Cstring		Cstart_time, Cend_time, CFileName;

	//
	// evaluated values:
	//
	CTime_base	StartTime, EndTime;

public:
	//
	// NOTE: FileName ALWAYS _un_quoted because that's how it's evaluated
	//
	Cstring		FileName;
private:

	//
	// constants:
	//
	Cstring		filter;

	Cstring		schema;
	Cstring		timesystem;
	bool		AllActsVisible;
	vector<string>	which_resources;

public:

	//
	// Indicates whether this request is to be processed
	// concurrently with a REMODEL request:
	//
	bool		running;

	WRITE_XMLTOLrequest(
		const Cstring&		desired_filename,
		const Cstring&		t1,
		const Cstring&		t2,
		const Cstring&		optional_filter,
		const Cstring&		optional_schema,
		const Cstring&		optional_timesystem,
		const bool		all_acts_visible,
		const vector<string>&	resources);
	WRITE_XMLTOLrequest(Cstring& errors, ListOVal*);
	~WRITE_XMLTOLrequest() {}

	Action_request* copy() { return new WRITE_XMLTOLrequest(
					CFileName,
					Cstart_time,
					Cend_time,
					filter,
					schema,
					timesystem,
					AllActsVisible,
					which_resources);
	}

	//
	// Returns true if OK, false if errors found
	//
	bool		evaluate_arguments();
	const CTime_base& get_start() const {
		return StartTime;
	}
	const CTime_base& get_end() const {
		return EndTime;
	}
	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);

	//
	// Provides a uniform handle for executing the
	// request that works both in single-threaded
	// and multi-threaded environments
	//
	static void	execute(
				thread_intfc* /* ,
				bool evaluate_args = true */ );
	static WRITE_XMLTOLrequest*&	in_process();
};

class WRITE_JSONrequest : public Action_request {

	//
	// strings:
	//
	Cstring		Cstart_time, Cend_time, CFileName;

	//
	// evaluated values:
	//
	CTime_base	StartTime, EndTime;

	//
	// NOTE: FileName ALWAYS _un_quoted because that's how it's evaluated
	//
	Cstring		FileName;

	vector<string>	which_resources;

public:
	WRITE_JSONrequest(
		const Cstring&		desired_filename,
		const Cstring&		t1,
		const Cstring&		t2,
		const vector<string>&	resources);
	WRITE_JSONrequest(Cstring& errors, ListOVal*);
	~WRITE_JSONrequest() {}

	Action_request* copy() { return new WRITE_JSONrequest(
					CFileName,
					Cstart_time,
					Cend_time,
					which_resources);
	}

	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);
};

class WRITE_SASFrequest : public Action_request {

	//
	// expressions:
	//
	Cstring		desired_file_name_expr;
	Cstring		start_time_expr, end_time_expr;

	//
	// evaluated values:
	//
	CTime_base	theStartTime, theEndTime;

	Cstring		desired_file_name;

	//
	// constants:
	//
			//
			// these are values of the SASF "file" attribute
			// for which activities will be included in the output.
			//
	stringtlist	original_sasf_file_attributes;

			//
			// 0 or 1; 1 if one is to include acts that start at the end time
			//
	int		includeActivitiesThatStartAtEndTime;
public:

	WRITE_SASFrequest(	const stringslist& symbolic_fnames,
				const Cstring &startExpr,
				const Cstring &endExpr,
				const Cstring &actual_fnameExpr,
				int theInclusionFlag);
	WRITE_SASFrequest(Cstring& errors, ListOVal*);

	~WRITE_SASFrequest() {}

	Action_request		*copy();

	const Cstring	&get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);
};

class FIND_RESOURCErequest : public Action_request {

	stringslist	list; // BKL-1/98

// BKL-1/98 totally messed with this entire class !!!!!!!!
// for scripting capability

public:

	FIND_RESOURCErequest(const stringslist& ar_list);

	~FIND_RESOURCErequest() {}


	Action_request *copy() { return new FIND_RESOURCErequest(list); }


	const Cstring&	get_command_text();
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	void		process_middle(TypedValue*);
};

class FREEZErequest: public Action_request {

	vector<string>	resources;
public:
	FREEZErequest(const vector<string>& res_list);
	~FREEZErequest() {}

	Action_request* copy() {
		return new FREEZErequest(resources);
	}
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	const Cstring&	get_command_text();
	void		process_middle(TypedValue*);
};


class UNFREEZErequest: public Action_request {

	vector<string>	resources;
public:
	UNFREEZErequest(const vector<string>& res_list);
	~UNFREEZErequest() {}

	Action_request* copy() {
		return new UNFREEZErequest(resources);
	}
	void		to_script(Cstring& s) const {
		s << "xcmd(" << addQuotes(commandType) << ", []);\n";
	}
	const Cstring&	get_command_text();
	void		process_middle(TypedValue*);
};



#endif /* ACTION_REQUEST_H */
