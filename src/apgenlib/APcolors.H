#if HAVE_CONFIG_H
#include <config.h>
#endif
/*
 *	Project:	SEQ_ADAPT, modified for APGEN
 *	Subsystem:	User Interface
 *	File Name:
 *
 *	Description: This file define the APGEN configuration data
 * **KEEP PostScript PATTERNS IN IO_plotheader.C IN SYNC WITH this file!!!**
 *
 *	Date		Revision	Author		Reason for change
 *-----------------------------------------------------------------------
 * 9/31/94     V1       I. Lin         Original Release
 * 11/20/95		D. Glicksberg	Fix AP_BITMAP_22; comments added
 * 1/30/96		D. Glicksberg	Use UI_dsconfig.h new defines; comments
 *
 *   Source Code Control: @(#) /seq/DEV21_0/apgen/src/APconfig.c ver. 
 */

#include "C_string.H"

#include "UI_dsconfig.h"  /*contains color & pattern counts, defaults, etc.*/
extern RGBvaluedef Patterncolors[];
extern long int find_color_id(const Cstring &colorname);

