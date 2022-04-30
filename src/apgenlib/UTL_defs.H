#ifndef UTLDEFSH
#define UTLDEFSH

#include "C_string.H"

#ifdef MOVED_TO_C_GLOBAL_H

/* PFM: FT_MIXED added to reflect the fact that apf's and aaf's can now be mixed.
 * Added FT_DYN_C to allow compiling resources as external dynamic libraries.
 * Added the initialization to powers of 2 to enable multiple affiliations. */

#define APGENfileType int

#define FT_NONE		0
#define	FT_TOL		1
#define FT_APF		2
#define FT_AAF		4
#define FT_MIXED	8
#define FT_SASF		16
#define FT_DYN_C	32
#define FT_LAYOUT	64
#define FT_XML		128

#endif /* MOVED_TO_C_GLOBAL_H */

// changed to V3.1 1/15/99:
// changed to V3.2 2/28/99:
// changed to V3.3 8/31/99:
// removed 7/13/2004. Do this in the configure script.
// #define APGEN_VERSION "V3.3"

#define DEFAULT_COLOR       5
#define GENERICCOLOR        "Dodger Blue"
#define DEFAULT_PATTERN     0
#define DEFAULT_PATTERN_STR "0"
#define DEFAULTACTREQPIXEL  10
#define ACTREQDEFTIMESPAN  100
#define ACTREQDEFSTIME      0
#define GENERICLEGEND       "Generic_Activities"
#define GENERICTYPE         "generic"
#define GENERICID           "Activity"

// Resource Display Y-scrolling mode enum (YSCROLL_AUTO_WINDOW is the DEFAULT).
// Modes _AUTO_PLAN and _AUTO_RANGE may not be used initially.  Only _MANUAL
/// enables the scrollbar (and zoom menus) and Y-scrolling control by the user.
enum RD_YSCROLL_MODE
        {
//	YSCROLL_NORESOURCES=    -1,     //Scrollbar DISABLED when NO Resources
//					are displayed in a Resource Display
	YSCROLL_MANUAL=		0,	//Manual (user) control of displayed
					//  Y-range of resource values
	YSCROLL_AUTO_WINDOW=    1,      //Automatic Y-range:  range of Resource
					//  values within time window, +margin
	YSCROLL_AUTO_PLAN=      2,      //Automatic Y-range:  range of Resource
					//  values across entire plan, +margin
	YSCROLL_AUTO_RANGE=     3       //Automatic Y-range:  largest possible
					//  range of Resource values, +margin
	} ;

#endif
