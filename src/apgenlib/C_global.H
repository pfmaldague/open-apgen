#ifndef	C_GLOBAL_H
#define	C_GLOBAL_H

#include <string.h>
#include <math.h>
#include <ctype.h>
#include "C_string.H" 

extern double sin(), cos(), exp(), log(), log10(), tan(), atan(), sqrt();
extern double pow(), atan2();

//
// Constants that are similar to those in <limits.h> and <float.h> go here; make
// sure the names are distinct from defines in these or other standard .h files.
//
// DSG 96-11-20:  changed from "DBL_DIG" to "DOUBLE_DIG" (to avoid confusion
// with similar define in <float.h>); moved from C_string.H to C_global.H
// approximate digits of precision in a double (Sun = 17; HP DBL_DIG is 15)
//
// PFM 01-01-9: decreased DOUBLE_DIG from 17 (the absolute upper bound of what a
// double can do) to 14 to avoid the "34.5999999999999999999992" syndrome.
//
// PFM JUN-30-2020: re-instated 15 because of concerns about TOL output consistency
//
#define DOUBLE_DIG 15

namespace apgen {

//
// Basic data types:
//
enum class DATA_TYPE: char {

	//
	// FLOATING used to be FLOAT, which confused Bill Gates. (Not an issue anymore, thanks Linus!)
	// BOOLEAN looks like something that might already mean something, so we don't use it.
	// AP_ACTIVITY and AP_RESOURCE used to support C++ code generation and compilation.
	//
	ARRAY, BOOL_TYPE, DURATION, FLOATING, INTEGER, STRING, TIME, UNINITIALIZED, INSTANCE
};


//
// Basic file types:
//
enum class FileType: char {
	FT_NONE,
	FT_TOL,
	FT_APF,
	FT_AAF,
	FT_MIXED,
	FT_SASF,
	FT_DYN_C,
	FT_LAYOUT,
	FT_XML,
	FT_RPC,
	FT_JSON
};

// Distinguishes between regular (simple) resource,
// array-type resources, and signals
enum class RES_CONTAINER_TYPE: char {
	// default: simple resource
	SIMPLE,
	// array of simple resources
	ARRAY,
	// no resource (signal)
	SIGNAL
};

enum class RES_CLASS: char {
	CONSUMABLE,
	NONCONSUMABLE,
	SETTABLE,
	STATE,
	EXTERNAL,
	ASSOCIATIVE,
	ASSOCIATIVE_CONSUMABLE,
	INTEGRAL,
	SIGNAL
};

enum class RES_VAL_TYPE: char {
	DON_T_CARE,
	SET,
	RESET,
	START_USING,
	END_USING
};

enum class STATE_STATE: char {
	RESET,
	SET,
	USE_SET_PENDING,
	USE_RESET_PENDING
};

//
// Enumerations for general return codes from various methods
//
enum class RETURN_STATUS: char {
	FAIL = -1,			// the selected method/function failed
	SUCCESS = 1,			// the selected method/function succeeded
	INTERRUPTED_OP = 19		// the operation was interrupted (e. g., modeling)
};

const Cstring&	RETURN_STATUS_to_Cstring(const RETURN_STATUS &);


//
//	Possible values for the "invisibility" of an act instance
//
enum class act_visibility_state: char {
	// instantiated, visible, participates in modeling
	VISIBILITY_REGULAR,
	// instantiated, invisible, participates in modeling if decomp. is nonexclusive
	VISIBILITY_DECOMPOSED,
	// instantiated, invisible, participates in modeling if decomp. is nonexclusive
	VISIBILITY_ABSTRACTED,
	// non-instantiated yet
	VISIBILITY_BRAND_NEW,
	// transient state, used to request children regeneration
	VISIBILITY_NEWCHILDREN,
	// in the cut/paste clipboard (not instantiated, not visible, not modeled)
	VISIBILITY_ON_CLIPBOARD
};

//
// Possible resource usage types
//
enum class USAGE_TYPE: char {
	// standard usage (from/to or at, depending on the resource)
	USE = 0,
	// sets the profile to the given value (state resources only)
	SET = 1,
	// resets the profile to the original value (state resources only)
	RESET = 2
};

// Possible attribute types
enum class ATTR_TYPE: char {
	// core attributes
	ATTR_ACTIVITY_CORE,
	// non-core attributes
	ATTR_ACTIVITY_OTHER,
	// for old-style SASF attributes:
	ATTR_ACTIVITY_OLD_FASHIONED,
	// attribute used for resources (e. g. Units)
	ATTR_RESOURCE,
	// for directives (?)
	ATTR_DIRECTIVE,
	// for constraints (?)
	ATTR_CONSTRAINT
};

//      Possible values for downtype:
enum class DECOMP_STATEMENT_TYPE: char {
	ACTIVITY_CALL,
	ACTIVITY_SPAWN,
	ACTIVITY_CONSTRUCTOR,
	ACTIVITY_DELAYED	// for unresolved child names
};

//
// This enum should be modernized. It should
// allow for global modeling and decomposition
// functions, and for modeling and decomposition
// methods as well.
//
// Note: a function is global, a method appears
// inside an activity type or abstract resource
// specification.
//
// A more uniform approach would be to lump all
// act. type / abs. res. sections into a common
// format that identifies the section as a method
// and gives it a 'standard' name:
//
// 	"attributes"
// 	"creation"
// 	"destruction"
// 	"decomposition"
// 	"nonexclusive_decomposition"
// 	"modeling"
// 	"resource usage"
//
// The remaining ones are used to identify specific
// portions of a type / abs. res. definition, and
// they are special to the APGenX DSL. They should
// not be confused with methods.
//
enum class METHOD_TYPE: char {
	ATTRIBUTES,
//	AUTODECOMP,		obsolete
	CONCUREXP,
	CREATION,
	DECOMPOSITION,
	DESTRUCTION,
	FUNCTION,
//	MODELED_DECOMPOSITION,	obsolete
	MODELING,
	NONE,
	NONEXCLDECOMP,
	PARAMETERS,
	PROFILE,
//	RESOLUTION,		obsolete
	RESUSAGE,
	STATES,
	USAGE
};

Cstring spell(DATA_TYPE typ);
Cstring spellC(DATA_TYPE typ);
Cstring spell(FileType typ);
Cstring spell(ATTR_TYPE typ);
Cstring spell(RES_CLASS typ);
Cstring spell(METHOD_TYPE typ);
Cstring spell(USAGE_TYPE typ);
Cstring spell(DECOMP_STATEMENT_TYPE typ);

} // namespace apgen

// These functions provide string to data-type conversion and visa versa.

// non-hopper act_sys displays:
#define MAX_NUM_REG_ACTSYS		3
#define TOTAL_NUM_ACTSYS		4

#define	MAX_SIGNIFICANT_DIGITS		DOUBLE_DIG

// implemented in APmain.C:
unsigned long int MemoryUsed();

#endif
