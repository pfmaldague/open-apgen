/*	PROJECT		: APGEN
 *	SUBSYSTEM	: UI (non-BX Motif)
 *	FILE NAME	: UI_GeneralMotifInclude.H
 *
 *
 *	ABSTRACT:
 *	
 *	General Motif Header file (used by Pierre's Motif wrapper classes),
 *	adapted from SEQ_REVIEW file GeneralMotifInclude.H
 *
 */

#ifndef GUI_GENERAL_INCLUDE
#define GUI_GENERAL_INCLUDE

//
extern "C" {

// bold PFM 5/19/99:
// #define GUI

#ifdef GUI
	/*

	General Motif include file. Contains zillions of defines, lots of
	structure definitions and a bunch of function prototypes.

	*/

#include <Xm/Xm.h>

	/*

	Standard Motif Widget Headers

	*/

#include <Xm/ArrowB.h>
#include <Xm/CascadeBG.h>
#include <Xm/CascadeB.h>
#include <Xm/Command.h>
#include <Xm/DialogS.h>
#include <Xm/DrawingA.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/MainW.h>
#include <Xm/MenuShell.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
// FOR HEAVENS SAKE DO NOT INCLUDE THIS -- ALL HELL BREAKS LOOSE WITH THE ...P.h FILES
// AND THE CENTERLINE COMPILER
// #include <Xm/SashP.h>
#include <Xm/Scale.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/List.h>

#else
typedef int Pixel ;
typedef int XFontStruct ;

#endif
// GUI

typedef struct 
        {
        Pixel           grey_color, red_color, green_color, black_color;
	Pixel		papaya_color, peach_color, peach_dark_color,
			white_color, ok_color, not_ok_color,
			over_max_color, under_min_color, fuzzy_color, telemetry_color;
        XFontStruct     *main_font;
        XFontStruct     *secondary_font;
        XFontStruct     *small_font;
	XFontStruct	*time_and_act_font;
	XFontStruct	*legend_font;
        } resource_data;


}
#endif
// GUI_GENERAL_INCLUDE
