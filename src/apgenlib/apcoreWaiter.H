#ifndef _APCOREWAITER_H_
#define _APCOREWAITER_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <APdata.H>
#include <APbasic.H>

class Action_request;

class apcoreWaiter {
private:
	apcoreWaiter();
	~apcoreWaiter() {
	}

public:
	enum class RequestStatus: char {
		PENDING,
		SUCCESS,
		FAIL
	};

	static apcoreWaiter&	theWaiter();

				// to support apcore_server
	mutex			usingAPcore;
	condition_variable	signalRequestReady;
	condition_variable	signalApcoreReady;
	mutex			theThread;
	RequestStatus		status;
	Action_request*&	getTheCommand();
	Action_request*		ar;
	void			setTheCommandTo(Action_request *ar);

	Cstring			errors;
};

// Let's also add a class to hold a bunch of command-line options:

class APcloptions {
public:
	APcloptions() :
		ARGC(0),
		ARGV(NULL),
		AUTO_DELAY(1.000),
		AutomaticRemodelOnFileRead(true),
		CMD_DELTA(0),
		aafLog(false),
		constraints_active(true),
		debug_execute(false),
		debug_grammar(false),
		activity_report(false),
		disable_discrepancy_popup_panel(true),
		GTKdebug(false),
		TOLdebug(false),
		generate_cplusplus(false),
		hard_timing_errors(false),
		hold_off_exec_call(0),
		hopper_option(false),
		libudef_flag(true),
		ModelWarnings(true),
		ModelErrorsFound(false),
		parallel_tol(false),
		profile(false),
		RedetailOptionEnabled(true),
		refresh_info_requested(false),
		roomy_display(false),
		show_functions(false),
		show_function_calls(false),
		TMSdebug(false),
		validateFlag(false),
		we_are_an_editor_server(false),
		ExitFlag(0),
		test(""),
		testing_the_effectiveness_of_the_low_level_error_handling_mechanism(0),
		XMLRPC_PORT_APcore(0),
		XMLRPC_PORT_atm(0)
		{}
	~APcloptions() {}

	static APcloptions &theCmdLineOptions();
	bool		AutomaticRemodelOnFileRead;
	bool		aafLog;
	bool		constraints_active;
	bool		activity_report;
	bool		generate_cplusplus;
	bool		disable_discrepancy_popup_panel;
	bool		GTKdebug;
	bool		TOLdebug;
	Cstring		theResourceToDebug;
	bool		hard_timing_errors;
	bool		hold_off_exec_call;
	char		hopper_option;
	bool		libudef_flag;
	bool		parallel_tol;
	bool		profile;
	bool		RedetailOptionEnabled;
	bool		refresh_info_requested;
			// to yield a roomier (but uglier) display,via skinnier
			//   borders and bars (see UI_mainwindowbx.C):
	char		roomy_display;
	bool		show_functions;
	bool		show_function_calls;
	bool		TMSdebug;
	bool		validateFlag;
	bool		we_are_an_editor_server;
	bool		debug_execute;
	bool		debug_grammar;
	Cstring		theConfigFile;
	Cstring		test;
	stringslist	testoptions;

	int		XMLRPC_PORT_APcore;
	int		XMLRPC_PORT_atm;


	int		ARGC;
	char**		ARGV;
	int		testing_the_effectiveness_of_the_low_level_error_handling_mechanism;
			// extra pixels to make sure activity labels are far enough away from surrounding box:
			// (used in UI_ds_draw.C)
	int		CMD_DELTA;

			// Number of milliseconds to wait before hitting "OK"
			// on pause panel in auto mode:
	double		AUTO_DELAY;

			// to answer queries (see UI_mainwindow.C):
	int		ExitFlag;


	Cstring	 	udef_file;
	Cstring	 	libaaf_file;
	bool		ModelWarnings;
	bool		ModelErrorsFound;
	Cstring		session_id;
	Cstring		opsrev_url;
	Cstring		opsrev_lib;
	Cstring		seqr_lib;
	Cstring		seqr_NS;
	static int	weShouldWorkBatch();
	static bool	theWorkProcBatch(void * theWorkProcData);
	static stringslist& FilesToRead();
	static pairslist& FilesToWrite();
	static void	getFlags(stringtlist& L);
};

#endif /* _APCOREWAITER_H_ */
