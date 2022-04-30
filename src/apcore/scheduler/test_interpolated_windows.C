#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <action_request.H>
#include <ActivityInstance.H>
#include <AP_exp_eval.H>

static const char*	aaf_text = "apgen version \"foo\"\n\
resource rA: consumable float\n\
    begin\n\
        parameters\n\
	    x: float default to 0.0;\n\
	profile\n\
	    0.0;\n\
	usage\n\
	    -x;\n\
    end resource rA\n\
\n\
resource rB: consumable float\n\
    begin\n\
        parameters\n\
	    x: float default to 0.0;\n\
	profile\n\
	    0.0;\n\
	usage\n\
	    -x;\n\
    end resource rB\n\
\n\
resource rC: consumable float\n\
    begin\n\
        parameters\n\
	    x: float default to 0.0;\n\
	profile\n\
	    0.0;\n\
	usage\n\
	    -x;\n\
    end resource rC\n\
\n\
activity type s\n\
    begin\n\
        attributes\n\
	    \"Duration\" = 1:0:0;\n\
	decomposition\n\
	    windows: array default to [];\n\
	    get_windows((rA.currentval() > 0.3 || rB.currentval() > 0.3) && rC.currentval() <= 0.0)\n\
	    	for [\"min\" = 0:0:1, \"max\" = 2T00:00:00];\n\
    end activity type s\n\
\n\
activity type useA\n\
    begin\n\
	attributes\n\
	    \"Duration\" = 3T00:00:00;\n\
	resource usage\n\
	    T: time default to start;\n\
	    delta: duration default to 0:5:0;\n\
	    while(T <= finish) {\n\
		    # period = 1 day     \n\
		    # offset = 0         \n\
		    use rA(sin(PI * 2.0 * ((T - start) / 1T00:00:00)));\n\
		    T = T + delta;\n\
	    }\n\
    end activity type useA\n\
\n";

static int usage(char* s) {
	fprintf(stderr, "usage: %s [aaf]", s);
	return -1;
}

int test_interp_windows(int argc, char* argv[]) {
	if(argc == 2) {
		if(!strcmp(argv[1], "aaf")) {
			// placeholder
			cout << aaf_text;
		}
	}

	/* first we are going to manufacture a PC that resembles a realistic
	 * scheduling Criterion.
	 */
	parsedExp	PC;

	/* we need to construct a set of currentval() expressions that
	 * interact via relational operators, then boolean operations
	 */
	// step 1: build a currentval node
	return 0;
}
