#if HAVE_CONFIG_H
#include <config.h>
#endif

 // timing stuff
 #include <cstdio>
 #include <ctime>
 #include <sys/time.h>

#include <iostream>
#include <unistd.h>
using namespace std;
#include "apDEBUG.H"
#include "APdata.H"
#include "apcoreWaiter.H"



extern "C" {
	extern void create_subsystems(const char*);
} // extern "C"

//
// Remnant of pthread-based design:
//
int	thread_main(int, char**);

int main(int argc, char** argv) {
	int		ret_val;

	if(argc == 2 && !strcmp(argv[1], "-version")) {
		cout << get_apgen_version_build_platform() << "\n";
		return 0;
	}

	//
	// We used to launch a thread here, but not anymore;
	// thread_main is a regular function call now. However,
	// the APcore executable launches thread_main as a
	// genuine thread. In that case, inter-thread communications
	// via atomic variables can easily be set up to establish
	// a connection between XmlRpc messages from APcore clients
	// and the ongoing thread_main process, e. g. while the
	// latter is performing a modeling or scheduling task.
	//
	ret_val = thread_main(argc, argv);

	if(!ret_val) {
		return 0;
	} else {
		return -1;
	}
}
