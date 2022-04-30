// APcoreClient.C: based originally on HelloClient.cpp, A simple xmlrpc client.
// Usage: APcoreClient serverHost serverPort
// Link against xmlrpc lib
#include "XmlRpc.H"
#include <iostream>
#include <stdio.h>
#include <sys/timeb.h>

#include <xmlrpc_intfc.H>

extern "C" {
#include <concat_util.h>
} // extern "C"

#include "UTL_time_base.H"
// #include "APdata.H"

#include <time.h>
#include <stdlib.h>


using namespace XmlRpc;

static const char *theClientName = NULL;
static int thePortNumber = -1;

static XmlRpcClient &client() {
	static XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client; }

static bool shouldTerminate = true;

static int usage(const char* Pname, const char *A) {
	std::cerr << "Usage: " << Pname << A << " [serverHost serverPort] [-v] [-t] [-log] -res <resource-name>\n";
	return -1; }

time_t my_timegm (struct tm *tm) {
	time_t ret, checking;
	struct tm localtimestr;

	/* returns the number of seconds corresponding to what's in tm,
	 * but unfortunately the time zone is taken into account... and
	 * there is no analog for UTC on the Solaris platform.
	 */
	ret = mktime(tm);
	// if tz is 7 hours off, then ret is too big by 7 hours
	gmtime_r(&ret, &localtimestr);
	// now localtimestr reflects the erroneous 7 extra hours. But
	// first we correct the @#$#@$%^^* daylight savings time bit that will
	// mess everything up:
	localtimestr.tm_isdst = -1;
	checking = mktime(&localtimestr);
	// finally we correct ret by subtracting the difference between checking and ret
	ret = ret - (checking - ret);
	return ret; }

void fill_time_API(CTime_base& v, XmlRpc::XmlRpcValue& x) {
	struct tm		TM((struct tm) x);
	time_t			int_time = my_timegm(&TM);

	v = CTime_base(int_time, 0, false); }

int ap_getresource(int argc, char* argv[], const char* Pname) {
	int		i;
	XmlRpcValue	oneArg;
	XmlRpcValue	noArgs, Args, result;
	char		*HOSTINFO = getenv("APCOREHOST");
	char		*PORTINFO = getenv("APCOREPORT");
	int		hostarg = 1;
	bool		reportTime = false, Log = false, Verbose = false, allFlag = false;
	int		niceTime = 0;
	double		startSeconds, endSeconds;
	char		*theResource = NULL;
	const char*	cmd;

	/* usage */
	for(i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-v")) {
			Verbose = true;
		} else if(!strcmp(argv[i], "-log")) {
			Log = true;
		} else if(!strcmp(argv[i], "-nice")) {
			niceTime = 1;
		} else if(!strcmp(argv[i], "-scet")) {
			niceTime = 2;
		} else if(!strcmp(argv[i], "-t")) {
			reportTime = true;
		} else if(!strcmp(argv[i], "-res")) {
			if(i < (argc - 1)) {
				i++;
				theResource = argv[i];
			} else {
				return usage(Pname, argv[0]);
			}
		} else if(!strcmp(argv[i], "-all")) {
			allFlag = true;
		} else {
			return usage(Pname, argv[0]);
		}
	}
	if((!allFlag) && (!theResource)) {
		return usage(Pname, argv[0]);
	}
	// XmlRpc::setVerbosity(5);

	if(!theClientName) {
		if((!HOSTINFO) || (!PORTINFO)) {
			if(!HOSTINFO) {
				std::cerr << "host info not supplied; please set "
					"APCOREHOST or pass host/port as cmd line args\n";
			}
			if(!PORTINFO) {
				std::cerr << "port info not supplied; please set "
					"APCOREPORT or pass host/port as cmd line args\n";
			}
			exit(-1);
		}
		theClientName = HOSTINFO;
		thePortNumber = atoi(PORTINFO);
	}

	if(allFlag) {
		cmd = "getResourceTypesPSI";
	}
	else {
		cmd = "getResourceInTmFormat";
		oneArg[0] = theResource;
	}
	if(Verbose) {
		std::cout << "sending this message:\n" << cmd << oneArg << "\n";
	}
	if(reportTime) {
		struct timeb T;
		ftime(&T);
		startSeconds = (double) T.time;
		startSeconds += 0.001 * (double) T.millitm;
	}
	if(!strcmp(cmd, "getResourceInTmFormat")) {
	    if(niceTime > 0) {
		if(client().execute(cmd, oneArg, result)) {
			if(result.getType() == XmlRpcValue::TypeArray) {
				int	n = result.size();

				std::cout << "\ngetResourceInTmFormat " << theResource << " result:\n";
				for(i = 0; i < n; i++) {
					XmlRpcValue	v = result[i];
					if(v.getType() == XmlRpcValue::TypeArray) {
						if(niceTime == 2) {
							// SCET format
							TypedValue	tv;

							xml2typedval_API(v[0], tv);
							cout << "    " << tv.to_string();
						} else {
							cout << "    " << v[0];
						}
						cout << "  " << v[1] << endl;
					}
				}
			} else {
				std::cout << "\n Error in getResourceInTimeFmt.\n";
			}
		}
	    } else if(client().execute("getResource", oneArg, result)) {
		if(reportTime) {
			struct timeb T;
			ftime(&T);
			endSeconds = (double) T.time;
			endSeconds += 0.001 * (double) T.millitm;
			std::cout << "seconds elapsed = " << (endSeconds - startSeconds) << std::endl;
		}
		std::cout << "\ngetResource " << theResource << " result:\n" << result << std::endl;
	    } else {
		std::cout << "\n Error in getResource.\n" << std::endl;
	    }
	} else {
		if(client().execute(cmd, oneArg, result)) {
			std::cout << cmd << " result = " << result << "\n";
		} else {
			std::cout << "\n Error in " << cmd << ".\n";
		}
	}

	if(Log) {
		if(client().execute("getLogMessage", noArgs, result)) {
			std::cout << "\ngetLogMessage result: " << result << std::endl;
		} else {
			std::cout << "\nError calling 'getLogMessage'\n\n";
			return -1;
		}
	}
	return 0;
}
