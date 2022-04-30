/*
 *	Project:	APGEN
 *	Subsystem:	User Interface (Display Subsystem)
 *	File Name:	UI_dsconfig.h
 *
 *	Description: This file defines (Activity) Display Subsystem constants
 *		     and typedefs, PLUS some color/pattern constants
 *
 *	Date		Revision	Author		Reason for change
 *-----------------------------------------------------------------------
 * ??/??/94     V1       		I. Lin		Original Release
 * 1/30/96				D. Glicksberg	move color/pattern
 *				defines from ACT_def.H and UI_activityeditor.H
 * 4/01/96                              D. Glicksberg   add TMTXT2VPOS
 * 6/03/96				D. Glicksberg	fit in contraint area
 * 7/08/96				D. Glicksberg	constraint sensitivity
 * 11/06/96				D. Glicksberg	resource axis tick
 *				lengths; mark unused constants as such
 *
 */


#ifndef dsconfig_h
#define dsconfig_h


/* UNUSED:  #define TMHSCRSTEP       24 **Timeline horizontal scroll increment*/

/* UNUSED:  #define TMHGRPNUM         4 **Timeline horizontal major groups   */

/* UNUSED:  #define TMHGZNUM        (TMHSCRSTEP/TMHGRPNUM)		     */

/* Following 3 constants were 0.85, 0.65, 0.35 in pre-constraint days:	     */
#define TMVPOS          (0.975)         /*Timemark vertical position ratio   */
#define TMTXTVPOS       (0.775)         /*Timemark text vertical position    */
#define TMTXT2VPOS      (0.400)         /*Timemark supplemental text vert pos*/

#define ACTVERUNIT       32             /*Vertical unit:  32 pixel per unit (NORMAL)   */
#define ACTSQUISHED      26             /*Vertical unit:  26 pixel per unit (SQUISHED) */
#define ACTFLATTENED     13             /*Vertical unit:  13 pixel per unit (SQUISHED) */
#define ACTEXPANDED      100            /*Vertical unit: 100 pixel per unit (EXPANDED) */

#define RESVERUNIT	26		/* Vertical unit for resource display toggle button */

#define ACTLEGNDWIDTH    200            /*Legend width of Activity display   */

// #define	MAXTOTALWIDTH	4000		/*Hugest conceivable width  in pixels*/
// #define	MAXTOTALHEIGHT	4000		/*Hugest conceivable height in pixels*/

/* Following 3 constants are NOT used, since BX doesn't know about them:     */
#define TIMEMARKHEIGHT   40             /*Timemark height (50 pre-contraints)*/
#define CONSTRAINTHEIGHT 10		/*Constraint area height	     */
#define SCROLLBARWIDTH   15             /*Scrollbar width		     */

#define CONLINEWIDTH	  1		/*Constraint violation line width    */

//
// The following was defined as 2, which is too tiny on modern displays
//
#define CONSENSITIVITY    8		/*Constraint violation selection     */
					/*  sensitivity (pixels on each side)*/

#define TMLBIG            2             /*Timemark thick line width (pixels) */
#define TMLBHEIGHT        4
#define TMLSMALL          1             /*Timemark thin line width (pixels)  */
#define TMLSHEIGHT        2

#define RESMAJORTICKLEN   6		/*Resource display axis:  major and  */
#define RESMINORTICKLEN   3		/*  minor tick lengths (pixels)      */

typedef struct {
	char* colorname ;
	unsigned short red ;
	unsigned short green ;
	unsigned short blue ;
	}
	RGBvaluedef ;

/* UNUSED:  #define COLORBTNHEIGHT 30   **Color palette button height        */
/* UNUSED:  #define COLORBTNWIDTH  80   **Color palette button width         */

#define MAXLEGENDDSTRLEN   180          /*Maximum legend display string      */
                                        /*        length, in pixels          */


/* Color & Pattern counts and defaults colocated here (used for "ds" display */
/*   and elsewhere, so most sensibly belong in this file OR new separate one)*/
/* SEE ALSO ACT_sys.H for duplicates of some of these!!!		     */
#define NUM_OF_COLORS   16              /*Pixmap board count of colors       */
#define NUM_OF_PATTERNS 64              /*Pixmap board count of patterns     */
#define DEFAULT_COLOR    5		/*Default color index (1-based)      */
#define DEFAULT_PATTERN     0		/*Default pattern index (0-based)    */
#define DEFAULT_PATTERN_STR "0"

#endif
