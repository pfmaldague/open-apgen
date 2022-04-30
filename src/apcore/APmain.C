#if HAVE_CONFIG_H
#include <config.h>
#endif

/*
 * This file has been reworked extensively as part
 * of the implementation of CR 100789
 */
#include "perf_util.H"

#include <unistd.h>

#ifdef OLDUDEF

extern "C" {
    extern void clear_all_user_caches();
    extern void clean_up_all_user_caches();
}

#else

    #include <dlfcn.h>

#endif /* OLDUDEF */

#include <time.h>
#include <fstream>
#include <stdlib.h> // for getenv, among others...
#include <memory>
#include <pwd.h>
#include <unistd.h>
#include <thread>
#include <mutex>

#include <aafReader.H>
#include "ACT_exec.H"
#include <ActivityInstance.H>
#include <RES_eval.H>
#include "action_request.H"
#include "apcoreWaiter.H"
#include "APerrors.H"
#include "AP_exp_eval.H"
#include "C_global.H"
#include "C_list.H"
#include "EventLoop.H"
#include "Prefs.H"
#include <xmlrpc_api.H>

using namespace apgen;

extern "C" {
#include "concat_util.h"
} // extern "C"

using namespace std;

// in apcore/APdata/APdata.C:
extern const char* get_apgen_version_build_platform();
extern int print_version_build_platform();
extern void report_profile_info();

// in apcore/parser_support/class_generation.C:
extern void clean_up_files_generated_during_consolidation();
extern bool create_files_generated_during_consolidation(Cstring& errors);

// in apcore/interpreter/BuiltInFunctions.C:
void register_all_internal_functions();

// in APdata/test.C:
extern const char**	help_test();

// thread-related globals:

mutex&	using_lists() {
	static mutex	p;
	return p;
}

condition_variable&	done_using_lists() {
	static condition_variable  c;
	return c;
}

// would be nice to install the following as handles... maybe later

bool		gtk_editor_needs_lists = false;

//
// need globals because main() can be located in one of several files
//
std::clock_t	perf_report::startcputime;
std::clock_t	perf_report::lastcputime;
double		perf_report::wall_at_start;

unsigned long int perf_report::MemoryUsed() {
	size_t peak_resident_mem;
	struct rusage rusage;
	getrusage( RUSAGE_SELF, &rusage );
#if defined(__APPLE__) && defined(__MACH__)
	    peak_resident_mem = (size_t)rusage.ru_maxrss;
#else
	    peak_resident_mem = (size_t)(rusage.ru_maxrss * 1024L);
#endif
	return peak_resident_mem;
}


aoString& theErrorMsg() {
	static aoString aos;
	return aos;
}

//
// debug
//
extern map<Cstring, int>& STATS_set();
extern map<Cstring, int>& STATS_settable();

void perf_report::profile() {

	cout << "\nProcess " << ::getpid() << " --\n";

	// debug
	// cout << "size of TypedValue: " << sizeof(TypedValue) << "\n";

	cout << "CPU: " << cpu_duration() << " s\n";
	double wall2 = wall_time();
	cout << "Wall: " << wall2 - wall_at_start << " s\n";
	cout << MemoryUsed() << " bytes of resident memory used\n" << std::flush;

#ifdef NO_STATISTICS
	//
	// Resource filtering statistics
	//
	cout << "\nState SET statistics:\n";
	map<Cstring, int>::iterator	iter;
	int				tot = 0;
	for(iter = STATS_set().begin(); iter != STATS_set().end(); iter++) {
		tot += iter->second;
		cout << "\t" << iter->first << ": " << iter->second << "\n";
	}
	cout << "Total: " << tot << "\n" << std::flush;
	tot = 0;
	cout << "\nSettable SET statistics:\n";
	for(iter = STATS_settable().begin(); iter != STATS_settable().end(); iter++) {
		tot += iter->second;
		cout << "\t" << iter->first << ": " << iter->second << "\n";
	}
	cout << "Total: " << tot << "\n" << std::flush;
#endif /* NO_STATISTICS */

	//
	// Non-standard character report
	//
	if(aafReader::IllegalChars()) {
	    FILE*	report = fopen("non-std-chars.txt", "w");
	    if(report) {
		std::string Out { aafReader::IllegalChars()->str() };
		fwrite(Out.c_str(), 1, Out.size(), report);
		fclose(report);
	    }
	    delete aafReader::IllegalChars();
	    aafReader::IllegalChars() = NULL;
	}
}

mutex&	using_editor_data() {
	static mutex	p;
	return p;
}

bool		message_center_needs_lists = false;

mutex &using_messaging_data() {
	static mutex	p;
	return p;
}

// extern declarations:

//
// We need points to functions defined in the User
// Library, whether linked dynamically or not.
//
#ifdef OLDUDEF
extern "C" {
    extern void		register_all_user_defined_functions();
    extern void		last_call();
}
extern void		clear_all_user_caches();
#endif /* OLDUDEF */

//
// Declaration 'extern "C"' are mandatory in order to
// be able to access these functions via the dlsym
// interface
//
extern "C" {
	void		(*register_userdef)(void) = NULL;
	void		(*last_call_before_exit)(void) = NULL;
	void		(*clear_user_cache)(void) = NULL;
	void		(*clean_up_user_cache)(void) = NULL;
	char*		(*tentative_title)(void) = NULL;

	// in ACT_exec.C:
	extern void	delete_subsystems();
} // extern "C"

		// in APdata.C
extern bool	&notify_control();
		// in APdata/APdata.C
extern bool	debug_event_loop; // false by default

			// for ACT_sys:
#define PRESSED 33
#define RELEASED 34


static blist	&arguments_that_make_no_sense() {
	static blist B(compare_function(compare_bstringnodes, false));
	return B;
}

static List	&preloaded_files() {
	static List	L;
	return L;
}

void print_all_options() {
	cout << "Valid options:\n";
	cout << "    -aaf-log           (will cause the APGen log to contain XCMDs instead of the usual action requests)\n";
	cout << "    -activity-report   (outputs a JSON file describing interactions between activities, resources and globals)\n";
	cout << "    -auto <delay>      (will cause the PAUSE panel to automatically move on after <delay> seconds)\n";
	cout << "    -catch             (defines an event catcher for all parameter change events; testing mostly)\n";
	cout << "    -cfg <file_name>   (read action_request::is_shared behavior from <file>; used when seqtalk is on)\n";
	cout << "    -debug-constraints (enables messages to stdout from constraint subsystem)\n";
	cout << "    -debug-events      (prints helpful messages for debugging the event loop)\n";
	cout << "    -debug-execute     (prints helpful messages for debugging instructions)\n";
	cout << "    -debug-grammar     (prints helpful messages for debugging the adaptation grammar)\n";
	cout << "    -debug-tol         (creates tol_debug.txt file w/debugging info regarding TOL generation)\n";
	cout << "    -disable-popup     (enables the warning popup when instance durations disagree with computed values)\n";
	cout << "    -enable-popup      (enables the warning popup when instance durations disagree with computed values)\n";
	cout << "    -errortest         (testing only. Removes 'try' in Group Activity action request, which triggers an unhandled\n";
	cout << "                        exception if there is an error in the creation section of the parent request's template.)\n";
	cout << "    -extrapix <number> (makes boxes bigger by 'number' pixels around activity labels)\n";
	cout << "    -generate-cplusplus (generate a C++ source file that wraps abstract resource usage statements)\n";
	cout << "    -hard-timing-errors (make it an error to schedule an activity with start time < now)\n";
	cout << "    -help              (prints this list)\n";
	cout << "    -hopper            (EXPERIMENTAL - enabled a \"hopper\" window for holding activities that are to be scheduled)\n";
	cout << "    -libaaf            (tells apgen to use the specified <file> as its AAF library)\n";
	cout << "    -libudef <file>    (tells apgen to use the specified <file> as its user-defined library)\n";
	cout << "    -log     <file>    (tells apgen to use the specified <file> as its log file instead of apgen.log)\n";
	cout << "    -maxmodeltime <T>  (tells apgen to stop modeling when time <T> is reached)\n";
	cout << "    -messaging <hostname:port> <lib> (connects to a messaging bus at <hostname:port> using <lib>.so)\n";
	cout << "    -mode <mode-name>  (OBSOLETE; must be preceded by -talk opt.; default mode-name is \"interactive\")\n";
	cout << "    -noconstraints     (turns off constraint modeling)\n";
	cout << "    -noeventbuffering  (turn off buffering of X Events; seems to help with refresh problems on Hummingbird)\n";
	cout << "    -nogui             (runs apgen without a GUI)\n";
	cout << "    -nolibudef         (apgen won't try to load a user-defined library)\n";
	cout << "    -noredetail        (apgen will use regen_children instead of redetail)\n";
	cout << "    -noremodel         (suppresses auto. remodel after reading file)\n";
	cout << "    -nomodelwarnings   (suppresses modeling warnings to stdout for out-of-order events)\n";
	cout << "    -parallel-tol <on/off> (enables/disables parallel (XML)TOL output)\n";
	cout << "    -persistent-scripts (forces execution of scripts to continue through errors)\n";
	cout << "    -preload <file>    (tells apgen to load library <file> before libudef; can be used repeatedly)\n";
	cout << "    -ramreport         (tells apgen to issue a RAM usage report after each REMODEL)\n";
	cout << "    -recompute-durations (switch default behavior to letting AAF duration formulas override APF values when in conflict)\n";
	cout << "    -redetail-enabled  (restore old \"redetail\" option instead of (re)gen_children)\n";
	cout << "    -read <file-name>  (will open the specified file as part of startup)\n";
	cout << "                       (note: this option can be used multiple times)\n";
	cout << "    -refreshinfo       (prints event-related messages to standard error; useful for detecting GUI synchronization problems)\n";
	cout << "    -roomy             (slightly less elegant GUI, provides more room for activity and resource displays)\n";
	cout << "    -restore-id        (restore pre-version 7.1 behavior in terms of defining ID of new activity instances - for testing mostly)\n";
	cout << "    -seqr-NS <file>    (namespace to be used with the SEQR library)\n";
	cout << "    -server            (EXPERIMENTAL - turns APGEN into a server that accepts commands through SOAP)\n";
	cout << "    -session           (helper mode only - specifies the unique ID of the session we are helping)\n";
	cout << "    -showlib           (will list user-defined functions contained in the current user-defined library)\n";
	cout << "    -showfuncs         (will list user-defined functions and AAF functions together with the number of times they were called)\n";
	cout << "    -tabs              (will cause list elements to be separated by tabs instead of spaces in APF output)\n";
	cout << "    -test <testprog> [<arg> [<arg> ... ] ]  (will run the indicated testprog with the arguments supplied, then exit)\n";
	// cout << "    -udef-verbose <file> (tells apgen to write a msg to stdout each time a function in the specified <file> is called)\n";
	cout << "    -version           (returns apgen version)\n";
	cout << "    -write <file>      (saves internal objects into a comprehensive mixed file named <file>)\n";
	cout << "    -xmlrpc            (turns on an XmlRpc server that allows client programs to operate apgen remotely)\n";
	}

void*&	theUdefHandle() {
	static void*	Udef = NULL;
	return Udef;
}

void*&	theOpsRevHandle() {
	static void*	OpsRev = NULL;
	return OpsRev;
}

void*&	theSeqrHandle() {
	static void*	Seqr = NULL;
	return Seqr;
}

extern "C" {
	typedef void (*simple_get_handle)(const char* URL, char** result);
	simple_get_handle	theSimpleGetHandle = NULL;
	} // extern "C"

void	dummy_catcher(const char *act_id, const char* param) {
	cerr << "caught param notification from act " << act_id << ", param " << param << endl;
}

int APmain1(int argc, char **argv, Cstring &any_errors) {
	int		i;
	bool		printall = false;

	/*	>>>>>>>> PART 1 <<<<<<<<
	 *
	 */

	Preferences().Init();
	// in case we are not the first thread to pass through this:
	APcloptions::theCmdLineOptions().udef_file = NULL;

	/*	>>>>>>>> PART 2 <<<<<<<<
	 *
	 *	send the executable title to stdout
	 */

	APcloptions::theCmdLineOptions().ARGC = argc;
	APcloptions::theCmdLineOptions().ARGV = argv;

	/*	>>>>>>>> PART 3 <<<<<<<<
	 *
	 *	command-line argument processing. Do this BEFORE setting the sockets...
	 *	otherwise, can't register them with X!! Also do this BEFORE registering
	 *	user-defined functions, since command-line arguments may specify what
	 *	library to use.
	 */


	i = 1;
	while(i < argc) {
		if(!strcmp(argv[i], "-aaf-log")) {
			APcloptions::theCmdLineOptions().aafLog = true;
		} else if(!strcmp(argv[i], "-activity-report")) {
			APcloptions::theCmdLineOptions().activity_report = true;
		} else if(!strcmp(argv[i], "-auto")) {
			if(i < (argc - 1)) {
				sscanf(argv[i + 1], "%lf", &APcloptions::theCmdLineOptions().AUTO_DELAY);
			} else {
				any_errors = "option -auto should be followed by a float. Exiting.\n";
				return -2;
			}
			i++;
		} else if(!strcmp(argv[i], "-catch")) {
			ActivityInstance::eventCatcher = dummy_catcher;
		} else if(!strcmp(argv[i], "-cfg")) {
			if(i < (argc - 1))
				APcloptions::theCmdLineOptions().theConfigFile = argv[ i + 1 ];
			else {
				any_errors = "option -cfg should be followed by a file name. Exiting.\n";
				return -2;
			}
			i++;
		} else if(!strcmp(argv[i], "-debug-events")) {
			debug_event_loop = true;
		} else if(!strcmp(argv[i], "-debug-execute")) {
			APcloptions::theCmdLineOptions().debug_execute = true;
		} else if(!strcmp(argv[i], "-debug-grammar")) {
			APcloptions::theCmdLineOptions().debug_grammar = true;
		} else if(!strcmp(argv[i], "-debug-constraints")) {
			// CONdebug = true;
		} else if(!strcmp(argv[i], "-debug-editor")) {
			APcloptions::theCmdLineOptions().GTKdebug = true;
		} else if(!strcmp(argv[i], "-debug-tol")) {
			APcloptions::theCmdLineOptions().TOLdebug = true;
			if(i < (argc - 1))
				APcloptions::theCmdLineOptions().theResourceToDebug = argv[ i + 1 ];
			else {
				any_errors = "option -debug-tol should be followed by a resource name. Exiting.\n";
				return -2;
			}
			i++;
		} else if(!strcmp(argv[i], "-disable-popup")) {
			APcloptions::theCmdLineOptions().disable_discrepancy_popup_panel = true;
		} else if(!strcmp(argv[i], "-enable-popup")) {
			APcloptions::theCmdLineOptions().disable_discrepancy_popup_panel = false;
		} else if(!strcmp(argv[i], "-errortest")) {
			APcloptions::theCmdLineOptions().testing_the_effectiveness_of_the_low_level_error_handling_mechanism = 1;
		} else if(!strcmp(argv[i], "-xmlrpc")) {
#ifdef HAVE_XMLRPC
			if(i < (argc - 1)) {
				sscanf(argv[i + 1], "%d", &APcloptions::theCmdLineOptions().XMLRPC_PORT_atm);
			} else {
				any_errors << "option -xmlrpc should be followed by a port number. Exiting.\n";
				return -2;
			}
			i += 1;
#else
			any_errors << "option -xmlrpc is not supported in this version of apcore, " 							"which was compiled without the XmlRpc library. Exiting.\n";
			return -2;
#endif /* HAVE_XMLRPC */
		} else if(!strcmp(argv[i], "-extrapix")) {
			if(i < (argc - 1)) {
				sscanf(argv[i + 1], "%d", &APcloptions::theCmdLineOptions().CMD_DELTA);
			} else {
				any_errors << "option -extrapix should be followed by an integer. Exiting.\n";
				return -2;
			}
			i++;
		} else if(!strcmp(argv[i], "-generate-cplusplus")) {
			APcloptions::theCmdLineOptions().generate_cplusplus = true;
			if(!create_files_generated_during_consolidation(any_errors)) {
				return -2;
			}
		} else if(! strcmp(argv[i], "-hard-timing-errors")) {
			APcloptions::theCmdLineOptions().hard_timing_errors = true;
		} else if(! strcmp(argv[i], "-help")) {
			printall = true;
		} else if(! strcmp(argv[i], "-hopper")) {
			APcloptions::theCmdLineOptions().hopper_option = true;
		} else if(!strcmp(argv[i], "-libaaf")) {
			if(i == argc - 1) {
				any_errors = "option -libaaf should be followed by a library file name. Exiting.\n";
				return -2;
			} else {
				APcloptions::theCmdLineOptions().libaaf_file = argv[i + 1];
			}
			i++;
		} else if(!strcmp(argv[i], "-libudef")) {
			if(i == argc - 1) {
				any_errors = "option -libudef should be followed by a library file name. Exiting.\n";
				return -2;
			} else {
				APcloptions::theCmdLineOptions().udef_file = argv[i + 1];
			}
			i++;
		} else if(!strcmp(argv[i], "-log")) {
			if(i == argc - 1) {
				any_errors = "option -log should be followed by a file name. Exiting.\n";
				return -2;
			} else {
				extern void set_log_file_to(const char *);
				set_log_file_to(argv[i + 1]);
			}
			i++;
		} else if(!strcmp(argv[i], "-maxmodeltime")) {
			if(i == argc - 1) {
				any_errors = "option -maxmodeltime should be followed by a time. Exiting.\n";
				return -2;
			} else {
				CTime_base theMaxModelTime(argv[i + 1]);
				eval_intfc::MaxModelTime() = theMaxModelTime;
			}
			i++;
		} else if(!strcmp(argv[i], "-noconstraints")) {
			APcloptions::theCmdLineOptions().constraints_active = false;
		} else if(!strcmp(argv[i], "-noeventbuffering")) {
			;
		} else if(!strcmp(argv[i], "-nogui")) {
			/* no-op */;
		} else if(!strcmp(argv[i], "-nolibudef")) {
			APcloptions::theCmdLineOptions().libudef_flag = false;
		} else if(!strcmp(argv[i], "-nomodelwarnings")) {
			APcloptions::theCmdLineOptions().ModelWarnings = false;
		} else if(!strcmp(argv[i], "-opsrev-url")) {
			if(i == argc - 1) {
				any_errors = "option -opsrev-url should be followed by a URL. Exiting.\n";
				return -2;
			} else {
				APcloptions::theCmdLineOptions().opsrev_url = argv[i + 1];
			}
			i++;
		} else if(!strcmp(argv[i], "-opsrev-lib")) {
			if(i == argc - 1) {
				any_errors = "option -opsrev-lib should be followed by a library file name. Exiting.\n";
				return -2;
			} else {
				APcloptions::theCmdLineOptions().opsrev_lib = argv[i + 1];
			}
			i++;
		} else if(!strcmp(argv[i], "-persistent-scripts")) {
			Action_request::persistent_scripts_enabled() = true;
		} else if(!strcmp(argv[i], "-noremodel")) {
			APcloptions::theCmdLineOptions().AutomaticRemodelOnFileRead = false;
		} else if(!strcmp(argv[i], "-noredetail")) {
			APcloptions::theCmdLineOptions().RedetailOptionEnabled = false;
		} else if(!strcmp(argv[i], "-parallel-tol")) {
			if(i == argc - 1) {
				any_errors = "option -parallel-tol should be followed by on/off. Exiting.\n";
				return -2;
			} else if(!strcmp(argv[i + 1], "on")) {
				APcloptions::theCmdLineOptions().parallel_tol = true;
			} else if(!strcmp(argv[i + 1], "off")) {
				APcloptions::theCmdLineOptions().parallel_tol = false;
			} else {
				any_errors = "option -parallel-tol should be followed by on/off. Exiting.\n";
				return -2;
			}
			i++;
		} else if(!strcmp(argv[i], "-preload")) {
			if(i == argc - 1) {
				any_errors = "option -preload should be followed by a library name. Exiting.\n";
				return -2;
			} else {
				preloaded_files() << new String_node(argv[i + 1]);
			}
			i++;
		} else if(!strcmp(argv[i], "-profile")) {
			APcloptions::theCmdLineOptions().profile = true;
		} else if(!strcmp(argv[i], "-ramreport")) {
			Action_request::RAMreport = true;
		} else if(!strcmp(argv[i], "-recompute-durations")) {
			ACT_exec::recompute_durations = true;
		} else if(!strcmp(argv[i], "-redetail-enabled")) {
			APcloptions::theCmdLineOptions().RedetailOptionEnabled = true;
		} else if(!strcmp(argv[i], "-read")) {
			if(i < (argc - 1))
				APcloptions::FilesToRead() << new emptySymbol(argv[ i + 1 ]);
			else {
				any_errors = "option -read should be followed by a file name. Exiting.\n";
				return -2;
			}
			i++;
		} else if(!strcmp(argv[i], "-refreshinfo")) {
			APcloptions::theCmdLineOptions().refresh_info_requested = true;
		} else if(!strcmp(argv[i], "-roomy")) {
			APcloptions::theCmdLineOptions().roomy_display = true;
		} else if(!strcmp(argv[i], "-restore-id")) {
			eval_intfc::restore_previous_id_behavior = true;
		} else if(!strcmp(argv[i], "-seqr-lib")) {
			if(i == argc - 1) {
				any_errors = "option -seqr-lib should be followed by a library file name. Exiting.\n";
				return -2;
			} else {
				APcloptions::theCmdLineOptions().seqr_lib = argv[i + 1];
			}
			i++;
		} else if(!strcmp(argv[i], "-seqr-NS")) {
			if(i == argc - 1) {
				any_errors = "option -seqr-NS should be followed by a namespace. Exiting.\n";
				return -2;
			} else {
				APcloptions::theCmdLineOptions().seqr_NS = argv[i + 1];
			}
			i++;
		} else if(!strcmp(argv[i], "-server")) {
			notify_control() = true;
			/* APcloptions::theCmdLineOptions().server_mode = true; */
		} else if(!strcmp(argv[i], "-session")) {
			if(i < (argc - 1)) {
				APcloptions::theCmdLineOptions().session_id = Cstring(argv[i + 1]);
			} else {
				any_errors = "option -session should be followed by a unique id. Exiting.\n";
				return -2;
			}
			i++;
		} else if(!strcmp(argv[i], "-showlib")) {
			// postpone until after libudef has been loaded...
			APcloptions::theCmdLineOptions().show_functions = true;
		} else if(!strcmp(argv[i], "-showfuncs")) {
			// postpone until after udef functions are unregistered...
			APcloptions::theCmdLineOptions().show_function_calls = true;
		} else if(!strcmp(argv[i], "-tabs")) {
			TypedValue::tabs = true;
		} else if(!strcmp(argv[i], "-test")) {
			bool enough_args = false;

			while(++i < argc) {
				if(!enough_args) {
					enough_args = true;
					APcloptions::theCmdLineOptions().test = Cstring(argv[i]);
				}
				else {
					APcloptions::theCmdLineOptions().testoptions << new emptySymbol(argv[i]);
				}
			}
			if(!enough_args) {
				any_errors = "option -test should be followed by a unique test name and optional args.\n";
				any_errors << "Valid test names:\n";
				const char**	theNames = help_test();
				const char*	which_name = theNames[0];
				while(which_name) {
					any_errors << "\t" << which_name << "\n";
					which_name = *(++theNames);
				}
				return -2;
			}
			i++;
#ifdef OBSOLETE
		} else if(!strcmp(argv[i], "-udef-verbose")) {
			if(i == argc - 1) {
				any_errors = "option -udef-verbose should be followed by the name of a file containing functions to track. Exiting.\n";
				return -2;
			}
			i++;
			if(Register_udef_function_filter(argv[i])) {
				any_errors = "File ";
				any_errors << argv[i] << " does not exist. Exiting.\n";
				return -2;
			}
#endif /* OBSOLETE */
		} else if(!strcmp(argv[i], "-validate")) {
			int		ll;
			char		*c;

			if(i == argc - 1) {
				any_errors =  "option -validate should be followed by validation options (ACLPT). Exiting.\n";
				return -3;
			}
			c = argv[++i];
			for(ll = 0; ll < strlen(c); ll++) {
				if(c[ll] == 'A') {
					Validating |= VALID_ACT;
				} else if(c[ll] == 'C') {
					Validating |= VALID_CALLBACK;
				} else if(c[ll] == 'G') {
					Validating |= VALID_GRAPH;
				} else if(c[ll] == 'L') {
					Validating |= VALID_LIST;
				} else if(c[ll] == 'O') {
					Validating |= VALID_GOBJ;
				} else if(c[ll] == 'P') {
					Validating |= VALID_POINTER;
				} else if(c[ll] == 'T') {
					Validating |= VALID_TIMEPTR;
				}
			}
			cerr << "setting validation flag to " << Validating << endl;

			/*
			 * First attempt, doesn't do much except traverse live lists:
			 * validateFlag = 1;
			 */
		} else if(!strcmp(argv[i], "-write")) {
			if(i < (argc - 1))
				APcloptions::FilesToWrite() << new symNode(argv[ i + 1 ], "");
			else {
				any_errors = "option -write should be followed by a file name. Exiting.\n";
				return -2;
			}
			i++;
		} else if(!strcmp(argv[i], "-declarations")) {
			if(i < (argc - 1))
				APcloptions::FilesToWrite() << new symNode(argv[ i + 1 ], "declarations");
			else {
				any_errors = "option -declarations should be followed by a file name. Exiting.\n";
				return -2;
			}
			i++;
		} else {
			arguments_that_make_no_sense() << new bstringnode(argv[i]);
		}
		i++;
	}

	if(printall) {
		print_all_options();
		return 0;
	}
	return 0;
}

int APmain2(Cstring& any_errors, bool this_is_ATM) {
	int		i;

	/*	>>>>>>>> PART 4 <<<<<<<<
	 *
	 *	Read certain options (udef) from environment variables
	 *	if not specified on the command line.
	 */

	if(!APcloptions::theCmdLineOptions().udef_file.is_defined()) {
	       char *udef_file2	= getenv("APGEN_LIBUDEF");
	       if(udef_file2) {
		       APcloptions::theCmdLineOptions().udef_file = (const char *) udef_file2;
	       }
	}

#ifdef OLDUDEF

	// This call (implemented in libudef) will send UDEF info to standard output:
	register_all_user_defined_functions();
	clear_user_cache = (void (*)(void)) clear_all_user_caches;
	clean_up_user_cache = (void (*)(void)) clean_up_all_user_caches;
#else

	/*	>>>>>>>> PART 5 <<<<<<<<
	 *
	 *	register any functions defined in the user-defined library.
	 *	Also attach the OpsRev access libary if the appropriate option
	 *	was specified by the user.
	 */
	if(	APcloptions::theCmdLineOptions().opsrev_url.is_defined()
		&& !APcloptions::theCmdLineOptions().opsrev_lib.is_defined()) {
		cerr << "Ops Rev URL is specified but not the Ops Rev library. Cannot access Ops Rev database.\n";
		APcloptions::theCmdLineOptions().opsrev_url.undefine();
	}

	if(	APcloptions::theCmdLineOptions().opsrev_lib.is_defined()
		&& !APcloptions::theCmdLineOptions().opsrev_url.is_defined()) {
		APcloptions::theCmdLineOptions().opsrev_url = "https://docweb.jpl.nasa.gov/app/ehm/ehm.json";
		cout << "Using default Ops Rev URL " << APcloptions::theCmdLineOptions().opsrev_url << "\n";
	}

	if(APcloptions::theCmdLineOptions().libudef_flag) {
		int		preloaded_files_have_been_loaded = 0;
		Node		*N;
		bool		should_proceed = true;

		while((N = preloaded_files().first_node())) {
			// theUdefHandle() = dlopen(*N->get_key(), RTLD_NOW | RTLD_GLOBAL);
			theUdefHandle() = dlopen(*N->get_key(), RTLD_LAZY);
			if(!theUdefHandle()) {
				const char *theDlOpenError = dlerror();
				theErrorMsg().clear();
				theErrorMsg() << "\nHaving trouble opening preloaded library \"" << N->get_key() << "\".\n";
				theErrorMsg() << "Error message from dlerror:\n";
				theErrorMsg() << theDlOpenError << "\n";
				theErrorMsg() << "\nWill run with built-in functions only.\n\n";
				break;
			}
			delete N;
		}

		if(preloaded_files().get_length()) {
			should_proceed = false;
		}
		if(should_proceed) {
			if(!APcloptions::theCmdLineOptions().udef_file.is_defined()) {
#ifdef HAVE_MACOS
				APcloptions::theCmdLineOptions().udef_file = "libudef.dylib";
#else
				APcloptions::theCmdLineOptions().udef_file = "libudef.so";
#endif /* HAVE_MACOS */
				}

			// Do this, otherwise the program will crash if there are any unresolved symbols in libudef.so
			// theUdefHandle() = dlopen(udef_file, RTLD_NOW | RTLD_GLOBAL);
			theUdefHandle() = dlopen(*APcloptions::theCmdLineOptions().udef_file, RTLD_LAZY);
			if(!theUdefHandle()) {
				const char *theDlOpenError = dlerror();
				cerr << "Not using a udef library, for the following reason:\n"
					<< theDlOpenError
					<< endl;
				should_proceed = false;
			}
		}
		if(should_proceed) {
			register_userdef = (void (*)(void)) dlsym(
						theUdefHandle(),
						"register_all_user_defined_functions");
			clear_user_cache = (void (*)(void)) dlsym(
						theUdefHandle(),
						"clear_all_user_caches");
			clean_up_user_cache = (void (*)(void)) dlsym(
						theUdefHandle(),
						"clean_up_all_user_caches");
			last_call_before_exit = (void (*)(void)) dlsym(
						theUdefHandle(),
						"last_call");
			if(!register_userdef
			    || !clear_user_cache
			    || !clean_up_user_cache
			  ) {
				const char *theDlOpenError = dlerror();

				if(!theDlOpenError) {
					theDlOpenError = "(none supplied)";
				}
				theErrorMsg().clear();
				theErrorMsg() << "udef registration and/or cache clearing function not found; error msg from dlerror:\n"
					<< theDlOpenError << "\n";
				theErrorMsg() << "\nClosing the udef library. Will run with built-in functions only.\n";
				theErrorMsg() << "Your udef library may be using old headers, in which case recompiling the library will help.\n\n";
				dlclose(theUdefHandle());
				theUdefHandle() = NULL;
				should_proceed = false;
			}
		}
		if(should_proceed) {
			const char	*user_lib_version = NULL;
			register_userdef();
			if((tentative_title = (char *(*)(void)) dlsym(theUdefHandle(), "print_user_lib_title_out"))) {
				user_lib_version = tentative_title();
			} else {
				//for backward compatibility
				char	**titlePtr = (char**) dlsym(theUdefHandle(), "User_lib_Title");

				if(titlePtr) {
					user_lib_version = *titlePtr;
				} else {
					user_lib_version = "unknown library";
				}
			}
			udef_intfc::set_user_lib_version_to(user_lib_version);
		}
	}

	any_errors = theErrorMsg().str();
	if(!any_errors.length()) {
		cout << "    udef library \""
			<< udef_intfc::get_user_lib_version() << "\"\n";
	}

#endif /* OLDUDEF */

	if(APcloptions::theCmdLineOptions().show_functions) {
		aaf_intfc::show_all_funcs();
	}

	/*	>>>>>>>> PART 6 <<<<<<<<
	 *
	 *	send the Copyright notice to stdout
	 */

	// OK to print in atm mode
	if(this_is_ATM) {
		cout << "\nCopyright (C) 1994-2022, California Institute of Technology."
			<< " ALL RIGHTS RESERVED."
	    		<< " U.S. Government sponsorship acknowledged."
			<< " Any commercial use must be negotiated with the Office Of Technology"
			<< " Transfer at the California Institute of Technology."
			<< " The technical data in this document is controlled under the U.S. Export"
			<< " Regulations, release to foreign persons may require an export authorization.\n";
	}

	// Let's check that all arguments were understood. Note that the top-level widget constructor
	// strips all the options it does understand from argv[]; therefore, the only ones left should
	// be the ones we took care of ourselves:
	static bool there_are_bad_options = false;

	for(i = 0; i < APcloptions::theCmdLineOptions().ARGC; i++) {
		if(arguments_that_make_no_sense().find(APcloptions::theCmdLineOptions().ARGV[i])) {
			if(!there_are_bad_options) {
				there_are_bad_options = true;
				cerr << "The following options were not understood:\n";
			}
			cerr << "    " << APcloptions::theCmdLineOptions().ARGV[i] << endl;
		}
	}
	if(there_are_bad_options) {
		cerr << "Try '" << APcloptions::theCmdLineOptions().ARGV[0] << " -help' for a list of valid options.\n";
		return -1;
	}

#ifdef HAVE_XMLRPC
	if(APcloptions::theCmdLineOptions().XMLRPC_PORT_atm > 0) {
		xmlrpc_api::init_server(APcloptions::theCmdLineOptions().XMLRPC_PORT_atm);
	}
#endif /* HAVE_XMLRPC */

	return any_errors.length() ? 1 : 0;
}

/* NOTE: APmain3 is only used in apbatch, APcore and GmsecServer. The atm_client
 * application uses its own version.
 *
 * NOTE: no error possible (!)
 *
 * NOTE: getting the password is unreasonable for APcore
 * */
int APmain3(bool XmlRpc_protocol) {
	bool			useApReady = XmlRpc_protocol;
	mutex&			useApcore(apcoreWaiter::theWaiter().usingAPcore);
	condition_variable&	sigReqReady(apcoreWaiter::theWaiter().signalRequestReady);
	condition_variable&	sigApReady(apcoreWaiter::theWaiter().signalApcoreReady);


	/*	>>>>>>>> PART 10 <<<<<<<<
	 *
	 *	>>>>>>> THE MAIN LOOP <<<<<<
	 */
	if(!theErrorMsg().is_empty()) {
		errors::Post("APcore initialization errors", theErrorMsg().str());
	}

	while(1) {

		//
		// SECTION 1:	see if we should exit. We don't just "exit(1)" because we want the
		// 		destructors to clean up.

		if(APcloptions::theCmdLineOptions().ExitFlag) {
			if(APcloptions::theCmdLineOptions().ExitFlag == 1) {

				//
				// Not necessary: request goes on the History list after processing.
				//
				// delete request;
      				break;
			} else if(APcloptions::theCmdLineOptions().ExitFlag == 2) {

				//
				// fast exit
				//
				perf_report::profile();
				if(APcloptions::theCmdLineOptions().ModelErrorsFound) {
					_exit(-1);
				} else {

					delete_subsystems();

					_exit(0);
				}
			}
		}

		// SECTION 2:	see if we should do some work

		// we are apbatch (atm_client uses its own version of APmain3())
		if(APcloptions::weShouldWorkBatch()) {
			APcloptions::theWorkProcBatch(NULL);
		} else {
			unique_ptr<QUITrequest> request { new QUITrequest(false) };
			request->process();

			//
			// This is wrong - QUITrequests sets ExitFlag,
			// which we must process at the top of this loop
			//
			// break;
		}
	}
	// 		<<<<<<< POOL NIAM EHT >>>>>>

	// do not forget...
	APcloptions::theCmdLineOptions().ExitFlag = 0;
	if(APcloptions::theCmdLineOptions().ModelErrorsFound) {
		return -1;
	} else {
		return 0;
	}
}

// no error possible (!)
int APmain4() {
			// in ACT_req.C:
	extern void	report_high_level_node_sizes();

	delete_subsystems();

#ifdef OLDUDEF

	last_call();

#	else

	if(last_call_before_exit) {
	    last_call_before_exit();
	}

	if(theUdefHandle()) {
	    dlclose(theUdefHandle());
	    theUdefHandle() = NULL;
	}

#	endif /* OLDUDEF */
	// odd that we would give this priority...
	theErrorMsg().clear();

	return 0;
}

using namespace std;

// EXTERNS:

extern stringtlist&	FileNamesWithInstancesAndOrNonLayoutDirectives();
static int		theWorkProcHasNotBeenCalled = 0;
static int		theFileSemaphore = 0;

int	APcloptions::weShouldWorkBatch() {

	if(udef_intfc::something_happened() <= 0) {
		// We have already been called and nothing significant happened
		return 0;
	}
	udef_intfc::something_happened()--;

	// Little trick ensures that we don't waste time figuring out whether or not to work if we already
	// found out we ought to and yet haven't:
	if(theWorkProcHasNotBeenCalled) {
		// We return 0 because the WorkProc was registered but never called so it WILL!
		// Just BE PATIENT!!
		return 0;
	}
	if((!theFileSemaphore) && FilesToRead().get_length() + FilesToWrite().get_length()) {
		theWorkProcHasNotBeenCalled = 1;
		return 1;
	}

	return 0;
}

void write_one_file(parsedExp& pe, FILE* outfile, bool& first, const Cstring& option) {
	parsedExp		item;
	pEsys::InputFile*	iF
		= dynamic_cast<pEsys::InputFile*>(pe.object());
	
	//
	// There may have been errors:
	//

	if(!iF) {
		return;
	}
	aoString	aos;
	if(first) {
		first = false;
		iF->write_to_stream(&aos, 0, option);
	} else {
		iF->write_to_stream_no_header(&aos, 0, option);
	}
	Cstring		outstring = aos.str();
	fwrite(*outstring, 1, outstring.length(), outfile);
}

bool APcloptions::theWorkProcBatch(void *) {
	emptySymbol*	N;
	symNode*	sN;

	theWorkProcHasNotBeenCalled = 0;
	/* The rationale for using theFileSemaphore is that a script may contain a PAUSE statement
	 * which causes the action_request to 'complete', i. e. to return, even though not all
	 * commands have been executed. In that case, we will pass through here again, but since
	 * we are already in the midst of processing input files we should not scan the loop again...  */
	if(!theFileSemaphore) {
	    while((N = FilesToRead().first_node())) {
		theFileSemaphore = 1;
		// Check if file is already opened
		// 96-08-07 DSG reversed following for FR021:  user chooses to read or not:
		// PFM: we won't check. Assume the user knows what he/she's doing.

		unique_ptr<OPEN_FILErequest> request { new OPEN_FILErequest(
				compiler_intfc::FILE_NAME,
				N->get_key()) };

		try {
	        	request->process();
		} catch(eval_error Err) {
			FilesToRead().clear();
			break;
		}
		if(request->get_status() != apgen::RETURN_STATUS::SUCCESS) {
			FilesToRead().clear();
			break;
		}

		delete N;
	    }
	    bool first_file = true;
	    if((sN = FilesToWrite().first_node())) {
		FILE* outfile = fopen(*sN->get_key(), "w");
		if(!outfile) {
			cerr << "Cannot open output file " << sN->get_key() << "\n";
			delete sN;
			return true;
		}
		for(int m = 0; m < aafReader::consolidated_files().size(); m++) {
			write_one_file(aafReader::consolidated_files()[m],
					outfile,
					first_file,
					sN->payload);
		}
		for(int m = 0; m < aafReader::input_files().size(); m++) {
			write_one_file(aafReader::input_files()[m],
					outfile,
					first_file,
					sN->payload);
		}
		fclose(outfile);
		delete sN;
	    }
	    theFileSemaphore = 0;
	}
	return true;
}

extern "C" {
	extern void create_subsystems(const char*);
}

class subsystems_creator {
public:
	subsystems_creator() {

		//
		// create the engine's global objects
		//
		create_subsystems( /* resource factory prefix = */ "");
	}
	~subsystems_creator() {

		//
		// Make sure this is run even if errors
		// were found:
		//
		delete_subsystems();
	}
};

//
// thread_main is called as a regular function by
// the main() of both apbatch and atm. However,
// the APcore executable (an XmlRpc server) launches
// thread_main as a regular thread, separate from
// the main execution thread which services calls
// from XmlRpc clients. This architecture lends
// itself to a simple implementation of an external
// debugger or fault-injection program.
//
int thread_main(int argc, char** argv) {
    try {
	Cstring any_errors;
	extern int APmain1(int, char **, Cstring &any_errors);
	extern int APmain2(Cstring& any_errors, bool ATM);
	extern int APmain3(bool);
	extern int APmain4();
	extern char APGEN_build_date[];
	int retval;

	//
	// start timers so CPU time usage can be reported upon exit
	//
	perf_report::initialize();

	subsystems_creator Creator;

	//
	// process command-line options
	//
	if((retval = APmain1(argc, argv, any_errors))) {
		if(retval == -10) {
			// -version option was used
			return 0;
		}
		std::cerr << any_errors;
		return -1;
	}


	cout << PACKAGE << " " << VERSION << APGEN_build_date << endl;

	register_all_internal_functions();

	//
	// load the user-defined library, if any
	//
	if(APmain2(any_errors, false)) {
		std::cerr << any_errors;
		perf_report::profile();
		return -1;
	}

	//
	// the main loop
	//
	if(APmain3(false)) {
		perf_report::profile();

		//
		// Abstract Resource CPU time usage report
		//
		if(APcloptions::theCmdLineOptions().profile) {
			report_profile_info();
		}
		return -1;
	}

	//
	// Abstract Resource CPU time usage report
	//
	if(APcloptions::theCmdLineOptions().profile) {
		report_profile_info();
	}

	//
	// wrapping up - calls delete_subsystems():
	//
	if(APmain4()) {
		perf_report::profile();
		return -1;
	}

	perf_report::profile();

	//
	// This was not included in delete_subsystems() because profile()
	// needs it:
	//
	Cstring::delete_permanent_strings();

	if(APcloptions::theCmdLineOptions().generate_cplusplus) {
		clean_up_files_generated_during_consolidation();
	}

	return 0;
    } catch(eval_error Err) {
	cerr << "Error in system initialization:\n"
	    << Err.msg << "\n";
	return -1;

    }
    return 0;
}
