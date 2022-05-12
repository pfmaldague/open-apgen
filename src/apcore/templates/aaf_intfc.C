#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define apDEBUG
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// debug
#include <assert.h>

#include "C_string.H"
#include "ActivityInstance.H"
#include "AAFlibrary.H"
#include <APbasic.H>
#include <flexval.H>
#include <AbstractResource.H>

using namespace std;

// AAF compiler-related statics, in the aaf_intfc class:
tlist<alpha_string, bfuncnode>& aaf_intfc::internalAndUdefFunctions() {
	static tlist<alpha_string, bfuncnode> F;
	return F;
}

tlist<alpha_string, Cntnr<alpha_string, ActivityInstance*> >&	aaf_intfc::actIDs() {
	static tlist<alpha_string, Cntnr<alpha_string, ActivityInstance*> > B;
	return B;
}

//
// For user-defined functions
//
void udef_intfc::add_to_Func_list(
		const char*		theName,
		generic_function	theProc,
		apgen::DATA_TYPE	dt) {
    Cstring		theOrigin("user-defined");
    Cstring		S(theName), T(theName);

    aaf_intfc::internalAndUdefFunctions()
	<< new bfuncnode(theName, triplet(theProc, theOrigin, dt));

    // debug
    bfuncnode*	bf = aaf_intfc::internalAndUdefFunctions().find(theName);
    assert(bf->get_key() == theName);

    T.to_upper();
    if(T != S && !aaf_intfc::internalAndUdefFunctions().find(T)) {
	aaf_intfc::internalAndUdefFunctions() << new bfuncnode(T, triplet(theProc, theOrigin, dt)); }
}

void udef_intfc::unregister_all_functions(bool print) {
    bfuncnode* bf;
    if(print) {

	//
	// Document UDEF functions
	//
	cout << "      function name                         |  calls\n";
	cout << "------------------------------------------- | ------\n";
	stringtlist	Upper;
	for(	bf = aaf_intfc::internalAndUdefFunctions().latest_node();
	    	bf;
	    	bf = bf->preceding_node()) {
	    Cstring T(bf->get_key());
	    T.to_upper();
	    if(!Upper.find(T)) {
		Upper << new emptySymbol(T);
	    }
	}
	for(	bf = aaf_intfc::internalAndUdefFunctions().earliest_node();
	    	bf;
	    	bf = bf->following_node()) {
	    if(bf->payload.Origin == "user-defined" && !Upper.find(bf->get_key())) {
		cout << bf->get_key();
		for(int k = 0; k < 50 - bf->get_key().length(); k++) {
			cout << " ";
		}
		cout << " | " << bf->payload.count << "\n";
	    }
	}
	cout << "\n";

	//
	// Now document AAF functions
	//
	map<Cstring, int>&	the_tasks = Behavior::GlobalType().taskindex;
	map<Cstring, int>::const_iterator iter;
	for(	iter = the_tasks.begin();
		iter != the_tasks.end();
		iter++) {
	    if(iter->first != "constructor") {
		cout << iter->first;
		for(int k = 0; k < 50 - iter->first.length(); k++) {
		    cout << " ";
		}
		cout << " | " << Behavior::GlobalType().tasks[iter->second]->calls() << "\n";
	    }
	}
	cout.flush();
    }
    aaf_intfc::internalAndUdefFunctions().clear();
}

//
// For internal and AAF-defined functions
//
void	udef_intfc::add_to_Func_list(
		const char*		theName,
		generic_function	theProc,
		const Cstring&		theOrigin,
		apgen::DATA_TYPE	dt) {
	Cstring S(theName), T(theName);

	aaf_intfc::internalAndUdefFunctions() << new bfuncnode(theName, triplet(theProc, theOrigin, dt));
	T.to_upper();
	aaf_intfc::internalAndUdefFunctions() << new bfuncnode(T, triplet(theProc, theOrigin, dt));
}

void aaf_intfc::show_all_funcs() {
	bfuncnode*	bf;
	clist		AllFuncs(compare_function(compare_bstringnodes, false));
	clist		capitalized(compare_function(compare_bstringnodes, false));
	Cstring		T;
	List_iterator	all_the_functions_in_order(AllFuncs);
	bstringnode	*bp;
	int		maxlength = 0;

	cout << "\nAvailable functions:\n";
	// list all-caps names last...
	for(	bf = aaf_intfc::internalAndUdefFunctions().latest_node();
		bf;
		bf = bf->preceding_node()) {
		T = bf->get_key();
		T.to_upper();
		if(!capitalized.find(T)) {
			capitalized << new bstringnode(T);
			if(!(bf->get_key() == "local_function")) {
				AllFuncs << new bstringnode(bf->get_key());
				if(maxlength < bf->get_key().length())
					maxlength = bf->get_key().length();
			}
		}
	}
	while((bp = (bstringnode *) all_the_functions_in_order())) {
		int		i = 6 + maxlength + 4, j;

		bf = aaf_intfc::internalAndUdefFunctions().find(bp->get_key());
		cout << "      " << bp->get_key();
		j = 6 + bp->get_key().length();
		while(j++ < i)
			cout << " ";
		if(bf->payload.Origin == "internal") {
			cout << "(built-in)\n";
		} else {
			cout << "(user)\n";
		}
	}
	cout << endl;
}

