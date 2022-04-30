#ifndef DB_H
#define DB_H
#include "ACT_exec.H"
// #include "ActivityType.H"

// class eventCatcher;

/* NOTES by achase 1/09/2002
 *
 * The idea here is to create a layer of convenience functions
 * with which to query and modify the APGEN Database at a high level
 * since the database is sort of global right now, having a database class
 * didn't seem to make sense.  We are using a namespace to prevent namespace
 * collisions.  To use one of these functions, include this header file and
 * call DB::functionName or type "using namespace DB;" to call all of them directly
 * of you can just say "using namespace DB::myfunction" to make that name local.
 * 
 * As this group of functions develops, it will probably make sense to 
 * split the functions into different files and maybe even create different
 * classes for the various types (i.e. DB_instance instead of ACT_req).  Then
 * we should say that everything in the UI and in action_request calls these things
 * and this can call things beneath.  This should help with our include files and
 * other things.  The aim is to be incremental, but to actually get somewhere
 *
 * IMPORTANT REQUIREMENTS:
 * 	- this header file should not "include the world"
 *	- start simple, keep refactoring the API to keep it minimal and orthogonal
 */

// #include "UTL_stl.H"

/* PFM NOTE: The above is commented out because we apply the
 *
 * 	"Principle of Maximal Transparency"
 * 	===================================
 *
 * An educated but non-clairvoyant reader (EBNCR) shall be able to
 * make sense of source code without consulting external, non-standard
 * header files.
 */

/* UTL_stl.H defines StringVect as vector<string>, so we do that explicitly
 * here. We may assume that the EBNCR knows the STL. */

#include <vector>
#include <string>
#include <algorithm>

using std::string;
using std::vector;
using std::pair;

typedef vector<string> StringVect;


/* This partially violates the principle of maximal transparency; however
 * the impact is small since we are not using the details of the class
 * definition. */
// class CTime;

// Original name was DB, but that's awfully generic... maybe someone else will use that!
namespace apgenDB {

/* We need to organize the DB capabilities as to their scope. We are
 * clearly dealing with several broad categories:
 *
 * 	- system-global queries: get act subsystems, get res subsystems, get legends
 * 	- act-system manipulation functions (get all selected instances)
 * 	- instance-specific manipulation functions (do something to one instance)
 */

// Explain what the function does. Does it fill names with unquoted subsystem names?
void GetSubSystemNames(StringVect *names); 

//subSystem names without quotes (compare case-sensitive)
void GetActivityTypeNamesForSubSystem(const string& subSystem, StringVect *names);

//version2 of getActivityTypeNames
void GetInstantiatedActivity_TypeNames(StringVect *names);

	//
	// default parameters are used; id is set to its actual value,
	// which may not agree with the one supplied by the client if
	// it's already in use
	//
ActivityInstance*
CreateActivity(		const Cstring&		type,
			const Cstring&		name,
			const Cstring&		legend,
			const CTime_base&	time,
			const pairslist&	optional_kwd_val_pairs,
			Cstring&		id);

	//
	// same as above, but instantiates the activity
	//
void
CreateActivityInPlan(	const Cstring&	type,
			const Cstring&		name,
			const Cstring&		legend,
			const CTime_base&	time,
			const pairslist&	optional_kwd_val_pairs,
			Cstring&		id);

void ClearSelectedActivities();

/**only works on active activities currently (not abstracted, decomposed, or pending)
 *returns 0 if it can't find it
 */   
int SelectActivity(const string& id);

//returns 0 if it can't find it -- it being the instance with that id?
int GetStartTime(const string& id, CTime_base* startTime);

// Does this get all of them? The visible ones? The selected ones?

void GetActivityIDs(StringVect *names);

//Retrieve All Activity IDs of given type
void GetActivityIDs_ofType(const string& type, StringVect *names);

// When is it appropriate to use this vs. "CUT_ACTIVITYrequest::process()"?
void DeleteActivity(const string& id);

void GetLegendNames(StringVect *names);

void DeleteEmptyLegend(const string& legendName);

// What is a "resource display"? Is it something in the GUI?
void GetResourceDisplayNames(StringVect* names);


ActivityInstance*	GetActivityForID(const string& id); }

#endif
