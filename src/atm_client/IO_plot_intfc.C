#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "IO_plot_intfc.H"
#include "C_string.H"
// #include "C_types.H"
#include "UI_GeneralMotifInclude.H"

#ifdef   SUN
#define FAST_POLYGON_FILL
#endif

using namespace std;

int PLOTdebug = 0;	//i.e. debug turned off

extern motif_widget * TopLevelWidget;	// in main.C

// GLOBALS:
#ifdef GUI

extern resource_data ResourceData;     // in UI_motif_widget.C

	R0Data theR0Data;

	static Arg args[20];

	GC	cursorGC = 0 ,
		bitmapGC = 0 ,
		legendGC[ 2 ] = { 0, 0 } ,
		canvasGC[4 ] = { 0, 0, 0, 0 } ,		//96-11-11 DSG:
		// 2002-06-30 PFM: fillGC[ 9 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		fillGC[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	sr_xm_BitmapStruct symbol[10];

//97-12-08 DSG:  (char *) silliness keeps HP10 aCC quiet:
#	define XtNdoDebug	(char *)"PLOT doDebug"
#	define XtCDoDebug	(char *)"DoDebug"
#	define XtNbell	 	(char *)"PLOT bell"
#	define XtCBell	 	(char *)"Bell"
#	define XtNdestroy	(char *)"PLOT destroy"
#	define XtCDestroy	(char *)"Destroy"
#	define XtNonTop		(char *)"PLOT onTop"
#	define XtCOnTop		(char *)"OnTop"
#	define XtNkeepOnTop	(char *)"PLOT keepOnTop"
#	define XtCKeepOnTop	(char *)"KeepOnTop"
// #	define XtNfontList	(char *)"PLOT fontList"
// #	define XtCFontList	(char *)"FontList"
#	define XtNcolor1	(char *)"PLOT color1"
#	define XtCColor1	(char *)"Color1"
#	define XtNcolor2	(char *)"PLOT color2"
#	define XtCColor2	(char *)"Color2"
#	define XtNcolor3	(char *)"PLOT color3"
#	define XtCColor3	(char *)"Color3"
#	define XtNcolor4	(char *)"PLOT color4"
#	define XtCColor4	(char *)"Color4"
#	define XtNcolor5	(char *)"PLOT longCursorColor"
#	define XtCColor5	(char *)"LongCursorColor"
#	define XtNincrement1	(char *)"PLOT increment1"
#	define XtCIncrement1	(char *)"Increment1"
#	define XtNincrement2	(char *)"PLOT increment2"
#	define XtCIncrement2	(char *)"Increment2"
#	define XtNincrement3	(char *)"PLOT increment3"
#	define XtCIncrement3	(char *)"Increment3"

	// in motif_widget.C:
	extern XtAppContext Context;

	static XtResource R0Resources[] =	 {
		{ XtNbell, XtCBell, XmRBoolean, sizeof( Boolean ) ,
		XtOffset( R0DataPtr, bell ), XmRImmediate, ( char * ) FALSE	} ,
		{ XtNdoDebug, XtCDoDebug, XmRBoolean, sizeof( Boolean ) ,
		XtOffset( R0DataPtr, debug ), XmRImmediate, ( char * ) FALSE	} ,
		{ XtNonTop, XtCOnTop, XmRBoolean, sizeof( Boolean ) ,
		XtOffset( R0DataPtr, ontop ), XmRImmediate, ( char * ) TRUE	} ,
		{ XtNdestroy, XtCDestroy, XmRBoolean, sizeof( Boolean ) ,
		XtOffset( R0DataPtr, destroy ), XmRImmediate, ( char * ) TRUE  } ,
		{ XtNkeepOnTop, XtCKeepOnTop, XmRBoolean, sizeof( Boolean ) ,
		XtOffset( R0DataPtr, keepontop ), XmRImmediate, ( char * ) FALSE	} ,

		// BUG FIX PFM 8/6/97: static version of APGEN crashes unser Solaris
		// Instead of defining a string resource, get a real font from resources;
		// Use the style of resource definition in UI_motif_widget.C.
		// This guarantees (?) that we get some kind of font, instead of taking a
		// chance when calling XQueryFont() with a font name, which would return
		// NULL (and does in the Solaris environment when trying to find font 'Fixed')

		// { XtNfontList, XtCFontList, XmRString, sizeof( String ) ,
		// XtOffset( R0DataPtr, font ), XmRString, "Fixed"	} ,

		//97-12-08 DSG:  (char *) silliness keeps HP10 aCC quiet:
		{ (char *)"plotFont", (char *)"PlotFont", XtRFontStruct ,
		sizeof( XFontStruct* ), XtOffset( R0DataPtr, font ) ,
		XtRString, ( char * ) "fixed" } ,

		{ XtNcolor1, XtCColor1, XmRPixel, sizeof( Pixel ) ,
		XtOffset( R0DataPtr, color1 ), XmRString, ( char * ) "AliceBlue"  } ,
		{ XtNcolor2, XtCColor2, XmRPixel, sizeof( Pixel ) ,
		XtOffset( R0DataPtr, color2 ), XmRString, ( char * ) "Gray90"	} ,
		{ XtNcolor3, XtCColor3, XmRPixel, sizeof( Pixel ) ,
		XtOffset( R0DataPtr, color3 ), XmRString, ( char * ) "Gray85"	} ,
		{ XtNcolor4, XtCColor4, XmRPixel, sizeof( Pixel ) ,
		XtOffset( R0DataPtr, color4 ), XmRString, ( char * ) "Gray80"	} ,
		{ XtNcolor5, XtCColor5, XmRPixel, sizeof( Pixel ) ,
		XtOffset( R0DataPtr, color5 ), XmRString, ( char * ) "White"	} ,
		{ XtNincrement1, XtCIncrement1, XmRInt, sizeof( int ) ,
		XtOffset( R0DataPtr, inc1 ), XmRString, ( char * ) "30"	} ,
		{ XtNincrement2, XtCIncrement2, XmRInt, sizeof( int ) ,
		XtOffset( R0DataPtr, inc2 ), XmRString, ( char * ) "30"	} ,
		{ XtNincrement3, XtCIncrement3, XmRInt, sizeof( int ) ,
		XtOffset( R0DataPtr, inc3 ), XmRString, ( char * ) "30"	}
		};
	
	static Cursor	cursor[5];
#	include "X11/cursorfont.h"

	static RData		theRData;
	XFontStruct *gfont = NULL, *inputfont = NULL;

#	define XtNfontList	(char *)"PLOT fontList"
#	define XtCFontList	(char *)"FontList"

	static XtResource RResources[] =
		{
		{ XtNfontList, XtCFontList, XmRString, sizeof( String ) ,
		XtOffset( RDataPtr, font ), XmRString, ( char * ) "Fixed"	} ,
		{ XtNcolor5, XtCColor5, XmRPixel, sizeof( Pixel ) ,
		XtOffset( RDataPtr, long_cursor_color ), XmRString, ( char * ) "yellow" }
		};

#else
	static void * args = 0;
#endif

#ifdef GUI

sr_GRDisplay DisplayStructure;

//static int n = 0;

static int CursorGCdefined = 0;
static int MainGCdefined = 0;
static int FillGCdefined = 0;
#endif

int sr_get_font_width() {
#ifdef GUI
	if ( GUI_Flag ) {
		return DisplayStructure.FontWidth;
	}
	else {
		return 9;
	}
#else
	return 9;
#endif
	}


void sr_xm_initialize_DisplayStructure() {
#ifdef GUI
if ( GUI_Flag ){
if ( PLOTdebug )
	cerr << "sr_xm_initialize_DisplayStructure: display = " << ( void * ) DisplayStructure.display << endl;
	if ( DisplayStructure.display ) return;

	DisplayStructure.display = XtDisplay( TopLevelWidget->widget );

	XtGetApplicationResources(TopLevelWidget->widget, (XtPointer) &theR0Data,
		R0Resources, XtNumber(R0Resources), NULL, 0);

	// BUG FIX 8/6/97 PFM
	// DisplayStructure.font = XLoadQueryFont( DisplayStructure.display, theR0Data.font );
	DisplayStructure.font = theR0Data.font;
	DisplayStructure.font_list = XmFontListCreate(DisplayStructure.font, XmSTRING_DEFAULT_CHARSET);
	DisplayStructure.FontHeight = DisplayStructure.font->max_bounds.ascent + DisplayStructure.font->max_bounds.descent;
//	DisplayStructure.FontWidth = DisplayStructure.font->per_char[ 'h' - DisplayStructure.font->min_char_or_byte2 ].width;
	//above fails on Sun, so do "worst case", as for FontHeight above
	DisplayStructure.FontWidth = DisplayStructure.font->max_bounds.width;

	DisplayStructure.bell	= theR0Data.bell;
	DisplayStructure.screen	= DefaultScreen( /*Xlib.h deficiency causes aCC warning*/
						DisplayStructure.display );
	DisplayStructure.cmap	= DefaultColormap( /*Xlib.h deficiency causes aCC warning*/
						DisplayStructure.display ,
						DisplayStructure.screen );
	DisplayStructure.context	= XtDisplayToApplicationContext( DisplayStructure.display );

	XtGetApplicationResources(TopLevelWidget->widget, (XtPointer) &theRData, RResources, XtNumber(RResources), NULL, 0);

	if ( ( ! inputfont ) && ( inputfont = XLoadQueryFont( DisplayStructure.display, theRData.font ) ) )
		{
		XmFontListFree(DisplayStructure.font_list);
		DisplayStructure.font_list = XmFontListCreate(inputfont, XmSTRING_DEFAULT_CHARSET);
		DisplayStructure.font = inputfont;
		}

	if ( ! gfont ) gfont = XLoadQueryFont( DisplayStructure.display, "Fixed" );

	cursor[0] = XCreateFontCursor( DisplayStructure.display, XC_left_ptr );
	cursor[1] = XCreateFontCursor( DisplayStructure.display, XC_watch );
	cursor[2] = XCreateFontCursor( DisplayStructure.display, XC_question_arrow );
	cursor[3] = XCreateFontCursor( DisplayStructure.display, XC_sb_right_arrow );
	cursor[4] = XCreateFontCursor( DisplayStructure.display, XC_gumby );
if ( PLOTdebug )
	cerr << "sr_xm_initialize_DisplayStructure: DONE; display now = " << ( void * ) DisplayStructure.display << endl;
}
#endif
	}

void sr_xm_initialize_gcs( Widget legend, Widget canvas )
	{
#ifdef GUI
if ( GUI_Flag ){
//	static char	dashes[] = { 3, 7, 1, 7 };	//original, 18 pixels
  	static char	dashes[] = { 2, 4, 2, 4 };	//DSG 96-11-08,12 pixels
	Drawable	drawable = RootWindow( /*Xlib.h deficiency causes aCC warning*/
						DisplayStructure.display ,
						DisplayStructure.screen );
	int		depth = DefaultDepthOfScreen( XtScreen( TopLevelWidget->widget ) );
	XGCValues	gcval;
	XtGCMask	cursor_mask =	GCBackground | GCForeground | GCLineWidth  | GCFunction;
	XtGCMask	valuemask  = GCFont;
	XtGCMask	gcmask  = GCBackground | GCForeground | GCLineWidth  | GCFont;
if ( PLOTdebug )
	cerr << "sr_xm_initialize_gcs: MainGCdefined = " << MainGCdefined << endl;

	if ( ! MainGCdefined )
		{

		if ( ! XtIsRealized( legend ) )
			{
			if ( PLOTdebug )
				cerr << "sr_xm_initialize_gcs:  unrealized legend Widget\n";
			return;
			}
		if ( ! XtIsRealized( canvas ) )
			{
			if ( PLOTdebug )
				cerr << "sr_xm_initialize_gcs:  unrealized canvas Widget\n";
			return;
			}

		MainGCdefined = 1;

		gcval.font = DisplayStructure.font->fid;	  /* Font fid=>  Font ID  */

		XtVaGetValues( legend ,
			XmNbackground, &gcval.background ,
			XmNforeground, &gcval.foreground ,
			NULL );

		gcval.line_width = 1;
		legendGC[0] = XtAllocateGC( legend, depth, gcmask, &gcval ,
			0, 0 );

		gcval.line_width = 2;
		legendGC[1] = XtAllocateGC( legend, depth, gcmask, &gcval ,
			0, 0 );
		XSetFont( DisplayStructure.display, legendGC[0], DisplayStructure.font->fid );
		XSetFont( DisplayStructure.display, legendGC[1], DisplayStructure.font->fid );

		XGetGCValues( DisplayStructure.display, legendGC[0], valuemask, & gcval );

		XtVaGetValues( canvas ,
			XmNbackground, &gcval.background ,
			XmNforeground, &gcval.foreground ,
			NULL );

		gcval.line_width = 1;

		canvasGC[0] = XCreateGC( DisplayStructure.display, XtWindow( canvas ), gcmask, &gcval );
		canvasGC[2] = XCreateGC( DisplayStructure.display, XtWindow( canvas ), gcmask, &gcval );
		XSetDashes( DisplayStructure.display, canvasGC[2], 0, dashes ,XtNumber( dashes ) );
		XSetLineAttributes( DisplayStructure.display, canvasGC[2], 1, LineDoubleDash, CapButt, JoinMiter );
		gcval.line_width = 3;	//DSG 96-10-23 changed BOLD 2 -> 3
		canvasGC[1] = XCreateGC( DisplayStructure.display, XtWindow( canvas ), gcmask, &gcval );
		canvasGC[3] = XCreateGC( DisplayStructure.display, XtWindow( canvas ), gcmask, &gcval );
		XSetDashes( DisplayStructure.display, canvasGC[3], 0, dashes ,XtNumber( dashes ) );
		XSetLineAttributes( DisplayStructure.display, canvasGC[3], 2, LineDoubleDash, CapButt, JoinMiter );
		}

	load_pixmaps( canvas, depth, drawable );

	load_bitmaps( canvas, depth, drawable );
if ( PLOTdebug )
	cerr << "sr_xm_initialize_gcs: CursorGCdefined = " << CursorGCdefined << endl;
	if ( !CursorGCdefined )
		{
		CursorGCdefined = 1;

		gcval.line_width = 1;
		gcval.function	= GXxor;
		XtVaGetValues( canvas ,
			XmNbackground, & gcval.background ,
			NULL );
		gcval.foreground = theRData.long_cursor_color ^ gcval.background;
		cursorGC = XCreateGC( DisplayStructure.display, XtWindow( canvas ) ,
			cursor_mask, &gcval );
		}
if ( PLOTdebug )
	cerr << "sr_xm_initialize_gcs: DONE\n";
}
#endif
	}

//Set drawing-area window attributes so get expose event even when shrink
//  window (see O'Reilly V 6A, pp. 346-7, for explanation plus code fragment):
void sr_xm_initialize_gravity( Widget drawarea )
{
#ifdef GUI
    Display* dis = XtDisplay(drawarea);
    Window   win = XtWindow(drawarea);

    //only does it if possible!
    if (dis && win)
    {
	//check bit gravity first; only set if need to change its value
	XWindowAttributes x_attrs;
	XGetWindowAttributes(dis, win, &x_attrs);
	if (x_attrs.bit_gravity != ForgetGravity)
	{
            XSetWindowAttributes c_attrs;
            c_attrs.bit_gravity = ForgetGravity;
            XChangeWindowAttributes (dis, win, CWBitGravity, &c_attrs);
	}
    }
#endif
}

void sr_xm_draw_lines( int which_gc, motif_widget * drawing, sr_IntPoint *lines ,
		int numlines, int bold, int dashed )
	{
#ifdef GUI
if ( GUI_Flag ){
	XPoint * newlines = ( XPoint * ) malloc( numlines * sizeof( XPoint ) );
	int j;

if ( PLOTdebug )
	cerr << "\nxm_draw_lines: drawing " << numlines << " lines...\n";

	for ( j = 0; j < numlines; j++ )
		{
if ( PLOTdebug )
	cerr << " j = " << j << " x = " << lines[j].x << " y = lines[j].y\n";
		newlines[j].x = lines[j].x;	//int->short overflow MAY occur
		newlines[j].y = lines[j].y;	//int->short overflow MAY occur
		}

if ( PLOTdebug )
	cerr << "\nxm_draw_lines: DONE drawing " << numlines << " lines\n\n";

	XDrawLines( XtDisplay( drawing->widget ), XtWindow( drawing->widget ) ,
		( which_gc == 1 ) ? legendGC[bold] : canvasGC[bold + ( 2 * dashed )] ,
		newlines, numlines, CoordModeOrigin );

	free( ( char * ) newlines );
}
#endif
	}

// draws a set of disconnected line segments in drawing area
void sr_xm_draw_line_segments( int which_gc, motif_widget * drawing ,
	sr_Segment *lines, int numlines, int bold, int dashed )
	{
	if ( !numlines ) {
		// efence doesn't like malloc(0):
		return; }
#ifdef GUI
if ( GUI_Flag ){

	XSegment *newlines = ( XSegment * ) malloc( numlines * sizeof( XSegment ) );
	int j;

if ( PLOTdebug )
	cerr << "xm_draw " << numlines << " line_segments...\n";

	for ( j = 0; j < numlines; j++ )
		{
if ( PLOTdebug )
	cerr << "draw_line_seg: x1 " << lines[j].x1 << " y1 " << lines[j].y1 << " x2 " <<
		lines[j].x2 << " y2 " << lines[j].y2 << "\n";

		newlines[j].x1 = /* MIN( */ lines[j].x1 /*, 0x7FFF ) */;
		newlines[j].y1 = /* MIN( */ lines[j].y1 /*, 0x7FFF ) */;
		newlines[j].x2 = /* MIN( */ lines[j].x2 /*, 0x7FFF ) */;
		newlines[j].y2 = /* MIN( */ lines[j].y2 /*, 0x7FFF ) */;
		}

	if ( numlines == 1 )
		XDrawLine( XtDisplay( drawing->widget ), XtWindow( drawing->widget ) ,
			( which_gc == 1 )? legendGC[bold] : canvasGC[bold] ,
			newlines[0].x1, newlines[0].y1 ,
			newlines[0].x2, newlines[0].y2 );
	else
		XDrawSegments( XtDisplay( drawing->widget ), XtWindow( drawing->widget ) ,
			( which_gc == 1 ) ? legendGC[bold]:canvasGC[bold+( 2 * dashed )] ,
				newlines, numlines );

	free( ( char * ) newlines );
	newlines = NULL;
if ( PLOTdebug )
	cerr << "xm... DONE drawing " << numlines << " line_segments\n";
}
#endif
	}

	// converts a "citibank" truncated rectangle to a polygon (CALLER MUST
	//   ALLOCATE output polygon "points", with exactly 4 elements)
void sr_xm_citibank_to_polygon(sr_CitiBank acitibank, sr_IntPoint* points)
{
    //pixel coordinates, in X standard form (X increases to right,Y incr. down)
    points[0].x = acitibank.x1;
    points[0].y = acitibank.y1;
    points[1].x = acitibank.x2;
    points[1].y = acitibank.y2;
    points[2].x = points[1].x;
    points[3].x = points[0].x;

    if (acitibank.rect_low)    //draw rectangle from segment to LOW-Y direction
    {
	if (points[0].y <= points[1].y)
	{
	    if (acitibank.rect_height < points[0].y)
	        points[3].y= points[2].y= points[0].y - acitibank.rect_height;
	    else
	        points[3].y = points[2].y = 0;	//negative Y illegal
	}
	else
	{
	    if (acitibank.rect_height < points[1].y)
	        points[3].y= points[2].y= points[1].y - acitibank.rect_height;
	    else
	        points[3].y = points[2].y = 0;	//negative Y illegal
	}
    }
    else		//i.e. draw rectangle from segment to HIGH-Y direction
    {
	if (points[0].y >= points[1].y)
	    points[3].y = points[2].y = points[0].y + acitibank.rect_height;
	else
	    points[3].y = points[2].y = points[1].y + acitibank.rect_height;
    }
}

	// draws a single filled polygon (general, but cannot intersect itself)
void sr_xm_fill_polygon(motif_widget* drawing,
			sr_IntPoint* points, int numpoints,
			int convex, int style) {
#ifdef GUI
    if (GUI_Flag) {
	if(style == STANDARD) {
	    return; }

	if(style > TELEMETRY) {
	    style = TELEMETRY; }

#ifndef   FAST_POLYGON_FILL
	//better algorithms, but only for SOME quadrilaterals
	sr_Rectangle rect;	//the current rectangle (construct then render)
	int upper_left;	//which point number is rectangle origin
	int i;

	//check first for actual rectangle (keep rectangle drawing
	//  algorithm in sync with sr_xm_fill_rectangle())
	if ((numpoints == 4)
	 && (points[0].x == points[1].x) && (points[2].x == points[3].x)
	 && (points[0].y == points[3].y) && (points[1].y == points[2].y))
	{
	    if (points[0].x <= points[3].x)
		if (points[0].y <= points[1].y)
		    upper_left = 0;
		else
		    upper_left = 1;
	    else
		if (points[0].y <= points[1].y)
		    upper_left = 3;
		else
		    upper_left = 2;

	    rect.x = points[upper_left].x;
	    rect.y = points[upper_left].y;
	    if (upper_left <= 1)
	        rect.width = 1 + points[3].x - points[0].x;
	    else
		rect.width = 1 + points[0].x - points[3].x;
	    if ((upper_left == 0) || (upper_left == 3))
	        rect.height = 1 + points[1].y - points[0].y;
	    else
	        rect.height = 1 + points[0].y - points[1].y;

	    XFillRectangle(XtDisplay( drawing->widget ),
	    	XtWindow(drawing->widget), fillGC[style],
		rect.x, rect.y, rect.width, rect.height);
	}
	else if ((numpoints == 4)
	      && (points[0].y == points[1].y) && (points[2].y == points[3].y)
	      && (points[0].x == points[3].x) && (points[1].x == points[2].x))
	{
	    //tested this case, with upper_left==0:  OK
	    if (points[0].y <= points[3].y)
		if (points[0].x <= points[1].x)
		    upper_left = 0;
		else
		    upper_left = 1;
	    else
		if (points[0].x <= points[1].x)
		    upper_left = 3;
		else
		    upper_left = 2;

	    rect.x = points[upper_left].x;
	    rect.y = points[upper_left].y;
	    if (upper_left <= 1)
	        rect.height = 1 + points[3].y - points[0].y;
	    else
		rect.height = 1 + points[0].y - points[3].y;
	    if ((upper_left == 0) || (upper_left == 3))
	        rect.width = 1 + points[1].x - points[0].x;
	    else
	        rect.width = 1 + points[0].x - points[1].x;

	    XFillRectangle(XtDisplay( drawing->widget ),
	    	XtWindow(drawing->widget), fillGC[style],
		rect.x, rect.y, rect.width, rect.height);
	}

	//check for "citibank" truncated-rectangle (for brevity, ONLY the cases
	//  we know are passed in -- this is for EFFICIENCY; the "else" is OK!
	else if ((numpoints == 4)
	      && (points[2].y == points[3].y)
	      && (points[0].x == points[3].x) && (points[1].x == points[2].x)
	      && (points[0].x <= points[1].x) && (points[0].y <= points[3].y))
	{
	    //the "<" condition works; so we shouldn't need "<=":
	    for (i = points[0].x; i < points[1].x; i++)
	    {
	        rect.x = i;
	        rect.width = 1;
		//want double arithmetic, then force to int at the last step:
	        rect.y = points[0].y
	            + (int)ceil(((double)points[1].y - points[0].y)
	              * (((double)i - points[0].x)
		        / ((double)points[1].x - points[0].x)));
	        rect.height = points[3].y - rect.y;

	        XFillRectangle(XtDisplay( drawing->widget ),
	    	    XtWindow(drawing->widget), fillGC[style],
		    rect.x, rect.y, rect.width, rect.height);
	    }
	}

	//else, fall through to default (and general) polygon-fill
	else
#endif /* FAST_POLYGON_FILL */
	{
	    XPoint* newpoints = (XPoint*)malloc(numpoints * sizeof(XPoint));
	    int j;

	    for ( j = 0; j < numpoints; j++ )
	    {
	        newpoints[j].x = points[j].x;	//int->short overflow MAY occur
	        newpoints[j].y = points[j].y;	//int->short overflow MAY occur
	    }

	    XFillPolygon(XtDisplay(drawing->widget), XtWindow(drawing->widget),
		  fillGC[style], newpoints, numpoints,
		  (convex ? Convex : Nonconvex), CoordModeOrigin);

	    free((char*)newpoints); } }
#endif
}

	// draws a single filled rectangle without a border in drawing area
void	sr_xm_fill_rectangle( motif_widget * drawing, sr_Rectangle rect, int style ) {
#ifdef GUI
    if ( GUI_Flag ){
	if ( style == STANDARD )
	    return;

	if ( style > TELEMETRY )
	    style = TELEMETRY;
	XFillRectangle( XtDisplay( drawing->widget ), XtWindow( drawing->widget ) , fillGC[style], rect.x, rect.y, rect.width, rect.height );
    }
#endif
}

// draws a single filled rectangle without a border in drawing area with a specific color
void sr_xm_fill_rectangle_with_color(motif_widget* drawing, sr_Rectangle rect, char* colorName) {
#ifdef GUI
    if (GUI_Flag) {
	XColor color;
	XGCValues savedGC;

	XGetGCValues(XtDisplay(drawing->widget), fillGC[SOLID], GCForeground, &savedGC);	// save to restore color lat
	XLookupColor(XtDisplay(drawing->widget), DisplayStructure.cmap, colorName, &color, &color);	// ignore status return

	XSetForeground(XtDisplay(drawing->widget), fillGC[SOLID], color.pixel);
	XFillRectangle(XtDisplay(drawing->widget), XtWindow(drawing->widget), fillGC[SOLID], rect.x, rect.y, rect.width, rect.height);

	XSetForeground(XtDisplay(drawing->widget), fillGC[SOLID], savedGC.foreground);
    }
#endif
}

	// draws a set of rectangles with a border in drawing area
void	sr_xm_draw_rectangles( int which_gc ,
		motif_widget * drawing, sr_Rectangle * rects ,
		int numrects, int bold, int dash )
	{
#ifdef GUI
if ( GUI_Flag ){
	XRectangle *newrects = ( XRectangle * ) malloc( numrects * sizeof( XRectangle ) );
	int j;

	for ( j = 0; j < numrects; j++ )
		{
		if ( rects[j].x < 0 )
			{
			if ( ((int)rects[j].width) >= -rects[j].x )
				{
				rects[j].width += rects[j].x + 1;
				rects[j].x = -1;
				}
			else	/* box is not visible */
				{
				rects[j].x = -10;
				rects[j].width = 5;
				}

			}
	/* QUESTIONABLE */
		newrects[j].x	= /* MIN( */ rects[j].x /* ,	0xFFF ) */;
		newrects[j].y	= /* MIN( */ rects[j].y /* ,	0xFFF ) */;
							/*  ^^^^ = 7FFF */

		newrects[j].width  = /* MIN( */ rects[j].width /*,  0xFFF ) */;
		newrects[j].height = /* MIN( */ rects[j].height /*, 0xFFF ) */;
		}

	XDrawRectangles( XtDisplay( drawing->widget ), XtWindow( drawing->widget ) ,
		(  which_gc == 1 ) ? legendGC[bold] : canvasGC[bold + 2 * dash] ,
		newrects, numrects );

	free( ( char * ) newrects );
}
#endif
	}

	// draw text in drawing area at( x, y )
void	sr_xm_draw_text( int which_gc, motif_widget * drawing, int x, int y, char * text )
	{
#ifdef GUI
if ( GUI_Flag ){
if ( PLOTdebug )
	cerr << "sr_xm_draw_text...\n";
	if ( ! XtIsRealized( drawing->widget ) ) /* probably in iconic state */
		return;

	XDrawImageString( XtDisplay( drawing->widget ), XtWindow( drawing->widget ) ,
		(  which_gc == 1 )? legendGC[0] : canvasGC[0] ,
		x, y, text, strlen( text ) );
}
#endif
	}

	// draws a bitmap symbol in drawing area
	// 1 = 5x6 triangle, reference bottom
	// 2 = 5x9 triangle, reference upper right
void	sr_xm_draw_bitmap( motif_widget * drawing, int id, int xloc, int yloc ,
		int write_flag )
	{
#ifdef GUI
if ( GUI_Flag ){
	int	x = xloc - symbol[ id ].x_hot, y = yloc - symbol[ id ].y_hot;

if ( PLOTdebug )
	cerr << "sr_xm_draw_bitmap...\n";
	if ( write_flag )
		XCopyArea( XtDisplay( drawing->widget ), symbol[ id ].pixmap ,
			XtWindow( drawing->widget ), bitmapGC, 0, 0 ,
			symbol[ id ].width, symbol[ id ].height, x, y );
	else
		XClearArea( XtDisplay( drawing->widget ), XtWindow( drawing->widget ), x, y ,
			symbol[ id ].width + 1, symbol[ id ].height + 1, 1 );
}
#endif
	}

	// clears a rectangle or entire drawing area( 0, 0, 0, 0 )
	// and declare expose event if exposure is TRUE
void	sr_xm_clear_drawing( /* Plot_window * pw, */
		motif_widget *drawing, sr_Rectangle rect, int exposure )
	{
#ifdef GUI
if ( GUI_Flag ){
	XRectangle	xrect;

if ( PLOTdebug )
	cerr << "sr_xm_clear_drawing...\n";
	if ( rect.x < 0 )
		{
		if ( ((int)rect.width) >= -rect.x )
			{
			rect.width += rect.x + 1;
			rect.x = -1;
			}
		else	/* box is not visible */
			{
			rect.x = -10;
			rect.width = 5;
			}
		}
	/* QUESTIONABLE */
	xrect.x	= /* MIN( */ rect.x /* ,	0xFFF ) */;
	xrect.y	= /* MIN( */ rect.y /* ,	0xFFF ) */;
				/*	^^^^^^ = 0x7FFF */

	/* QUESTIONABLE */
	xrect.width  = /* MIN( */ rect.width  /*, 0xFFF ) */;
	xrect.height = /* MIN( */ rect.height /*, 0xFFF ) */;

//	clear_long_cursor( & pw->DrawingStructure );

	XClearArea( XtDisplay( drawing->widget ), XtWindow( drawing->widget ) ,
		xrect.x, xrect.y ,
		xrect.width, xrect.height ,
		exposure );
}
#endif
	}

	// sets focus for text input to parent
void	sr_xm_set_focus( motif_widget * parent )
	{
#ifdef GUI
if ( GUI_Flag ){
if ( PLOTdebug )
        cerr << "sr_xm_set_focus\n";

	XmProcessTraversal( parent->widget, XmTRAVERSE_CURRENT );
        XtSetKeyboardFocus( parent->widget, None );

}
#endif
	}

int	sr_xm_width( motif_widget * W ) {
#	ifdef GUI
if ( GUI_Flag ){
		Dimension	width;
		if ( sr_xm_is_iconic( W->widget ) ) return 600;
		XtVaGetValues( W->widget, XmNwidth, &width, NULL );
		return width; }
else {
		return 600; }
#	else
		return 600;
#	endif
	}

int	sr_xm_height( motif_widget * W ) {
#	ifdef GUI
if ( GUI_Flag ){
		Dimension	height;
		if ( sr_xm_is_iconic( W->widget ) ) return 600;
		XtVaGetValues( W->widget, XmNheight, &height, NULL );
		return height; }
else {
		return 600; }
#	else
		return 600;
#	endif
	}

#ifdef GUI
void load_pixmaps( Widget widget, int depth, Drawable drawable )
	{

if ( GUI_Flag ){

	XtGCMask	gcmask = GCForeground | GCFillStyle |
				 GCBackground | GCTile;
	XGCValues	gcval;
	int		d4 = 4, d8 = 8, d10 = 10;
	unsigned long	bg, fg;

	static unsigned char vert_bits[]  = { 0x44, 0x44, 0x44, 0x44  };
	static unsigned char horz1_bits[] = { 0x00, 0x01, 0xff, 0x10  };
//	static unsigned char horz2_bits[] = { 0xff, 0x01, 0x00, 0x10  };

	//hatch,diag1,diag2 are designed for 16x16 pattern (but 10x10 repeat),
	//  with every 5th of the possible diagonals having its bits turned on
	static unsigned char hatch_bits[] = { 0x11, 0x02, 0x0a, 0x01, 0x84 ,
					      0x00, 0x4a, 0x00, 0x31, 0x00 ,
					      0x30, 0x02, 0x48, 0x01, 0x84 ,
					      0x00, 0x42, 0x01, 0x21, 0x02 };

	static unsigned char diag1_bits[] = { 0x10, 0x02, 0x08, 0x01, 0x84 ,
					      0x00, 0x42, 0x00, 0x21, 0x00 ,
					      0x10, 0x02, 0x08, 0x01, 0x84 ,
					      0x00, 0x42, 0x00, 0x21, 0x00 };

//	static unsigned char diag2_bits[] = { 0x21, 0x00, 0x42, 0x00, 0x84 ,
//					      0x00, 0x08, 0x01, 0x10, 0x02 ,
//					      0x21, 0x00, 0x42, 0x00, 0x84 ,
//					      0x00, 0x08, 0x01, 0x10, 0x02 };
//	//97-06-05 DSG modify to get 2 neighboring diagonals on, next 3 off,...
//	static unsigned char diag2_bits[] = { 0x63, 0x00, 0xc6, 0x00, 0x8c ,
//					      0x01, 0x18, 0x03, 0x31, 0x06 ,
//					      0x63, 0x00, 0xc6, 0x00, 0x8c ,
//					      0x01, 0x18, 0x03, 0x31, 0x06 };
	//97-06-05 DSG modify to get 3 neighboring diagonals on, next 2 off,...
	static unsigned char diag2_bits[] = { 0xe7, 0x00, 0xce, 0x01, 0x9c ,
					      0x03, 0x39, 0x07, 0x73, 0x0e ,
					      0xe7, 0x00, 0xce, 0x01, 0x9c ,
					      0x03, 0x39, 0x07, 0x73, 0x0e };

	static unsigned char solid_bits[]  = 	{ 0xff, 0xff, 0xff, 0xff };

if ( PLOTdebug )
        cerr << "load_pixmaps\n";
	if ( FillGCdefined ) return;
	FillGCdefined = 1;

	XtVaGetValues( widget ,
		XmNbackground, &gcval.background ,
		XmNforeground, &gcval.foreground ,
		NULL );

	bg = gcval.background;	/* Input here for color background.	*/
	fg = gcval.foreground;
	gcval.fill_style = FillTiled;

	/*================ DO FILL TILES DEFINITION ====================*/
	/* STANDARD ,HATCHED ,VERTICAL ,DIAGONAL ,HORIZONTAL ,SOLID,   */
	/*   DIAG_OVER_MAX, DIAG_UNDER_MIN				*/

	gcval.tile = XCreatePixmapFromBitmapData( DisplayStructure.display, drawable ,
		( char * ) hatch_bits, d10, d10, fg, bg, depth );
	fillGC[HATCHED] = XtAllocateGC( widget, depth, gcmask, &gcval ,
		0, 0 );

	gcval.tile = XCreatePixmapFromBitmapData( DisplayStructure.display, drawable ,
		( char * ) vert_bits, d8, d4, fg, bg, depth );
	fillGC[VERTICAL] = XtAllocateGC( widget, depth, gcmask, &gcval ,
		0, 0 );

	gcval.tile = XCreatePixmapFromBitmapData( DisplayStructure.display, drawable ,
		( char * ) diag1_bits, d10, d10, fg, bg, depth );
	fillGC[DIAGONAL] = XtAllocateGC( widget, depth, gcmask, &gcval ,
		0, 0 );

	gcval.tile = XCreatePixmapFromBitmapData( DisplayStructure.display, drawable ,
		( char * ) horz1_bits, d8, d4, fg, bg, depth );
	fillGC[HORIZONTAL] = XtAllocateGC( widget, depth, gcmask, &gcval ,
		0, 0 );

	//*** DSG 96-10-22 horrible kludge makes solid RED instead of fg color
	//DSG 97-04-17 use ok_color (Resource value "OK", not out-of-bounds)
	gcval.tile = XCreatePixmapFromBitmapData( DisplayStructure.display, drawable ,
		( char * ) solid_bits, d8, d4, /* fg */ ResourceData.ok_color, bg, depth );
	fillGC[SOLID] = XtAllocateGC( widget, depth, gcmask ,
		&gcval, 0, 0 );

	gcval.tile = XCreatePixmapFromBitmapData(DisplayStructure.display, drawable,
		(char *) solid_bits, d8, d4, /* fg */ ResourceData.fuzzy_color, bg, depth);
	fillGC[FUZZY] = XtAllocateGC(widget, depth, gcmask, &gcval, 0, 0);

	gcval.tile = XCreatePixmapFromBitmapData(DisplayStructure.display, drawable,
		(char *) solid_bits, d8, d4, /* fg */ ResourceData.telemetry_color, bg, depth);
	fillGC[TELEMETRY] = XtAllocateGC(widget, depth, gcmask, &gcval, 0, 0);

	//*** DSG 97-04-17 another kludge, use this for out-of-bounds Res.value
	gcval.tile = XCreatePixmapFromBitmapData( DisplayStructure.display, drawable ,
	//	( char * ) diag2_bits, d10, d10, fg, bg, depth );
	//	( char * ) solid_bits, d8, d4, /* fg */ ResourceData.not_ok_color, bg, depth );
	//DSG 97-04-20 change from solid to diagonal-hatch (PFM likes):
	//DSG 97-05-14 change color name
	// PFM 2015-09-08 revert
	// PFM 2015-09-09 revert back - changed my mind!
		( char * ) diag2_bits, d10, d10, /* fg */ ResourceData.over_max_color, bg, depth );
	//	( char * ) solid_bits, d8, d4, /* fg */ ResourceData.over_max_color, bg, depth );

	//fillGC[UNDEFINED_STYLE] = XtAllocateGC( widget, depth, gcmask ,
	//DSG 97-05-14 change name (over-maximum out-of-bounds):
	fillGC[DIAG_OVER_MAX] = XtAllocateGC( widget, depth, gcmask , &gcval, 0, 0 );

//	//extra (unnamed) fill style is ALTERNATE horizontal style; fill logic
//	//  chooses this if it is "better" than standard "HORIZONTAL" fill
//	gcval.tile = XCreatePixmapFromBitmapData( DisplayStructure.display, drawable ,
//		( char * ) horz2_bits, d8, d4, fg, bg, depth );
//	fillGC[7] = XtAllocateGC( widget, depth, gcmask, &gcval ,
//		0, 0 );

	//*** DSG 97-05-14 another kludge, use this for under-min out-of-bounds
	gcval.tile = XCreatePixmapFromBitmapData( DisplayStructure.display, drawable ,
		( char * ) diag2_bits, d10, d10, /* fg */ ResourceData.under_min_color, bg, depth );
	fillGC[DIAG_UNDER_MIN] = XtAllocateGC( widget, depth, gcmask , &gcval, 0, 0 ); }
	}
#endif


#ifdef GUI
void load_bitmaps( Widget widget, int depth, Drawable drawable )
	{

if ( GUI_Flag ){

	Pixmap		bitmap;
	XtGCMask	gcmask = GCBackground | GCForeground;
	XGCValues	gcval;

if ( PLOTdebug )
        cerr << "load_bitmaps\n";
	static unsigned char	arrow[] = 	{
		0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10 ,
		0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x11, 0x01 ,
		0x11, 0x01, 0x92, 0x00, 0x92, 0x00, 0x54, 0x00, 0x54 ,
		0x00, 0x38, 0x00, 0x38, 0x00, 0x10, 0x00, 0x10, 0x00 	};
	static unsigned char	icycle[] = 	{
		0xef, 0x01, 0xef, 0x01, 0xef, 0x01, 0xee, 0x00, 0xee ,
		0x00, 0xee, 0x00, 0xfe, 0x00, 0x7c, 0x00, 0x7c, 0x00 ,
		0x7c, 0x00, 0x7c, 0x00, 0x38, 0x00, 0x38, 0x00, 0x38 ,
		0x00, 0x38, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00 	};
	static char	small_triangle[] = 	{
		0x1f, 0x1f, 0x0e, 0x0e, 0x04, 0x04 	};
	static char	large_triangle[] = 	{
		0x1f, 0x1f, 0x1f, 0x0e, 0x0e, 0x0e, 0x04, 0x04, 0x04 	};
	static char	hi_bits[] = 	{
		0x07, 0x07, 0x07, 0x07, 0x01, 0x01, 0x01, 0x01, 0x01 ,
		0x01, 0x01, 0x01, 0x01, 0x01 	};
	static char	lo_bits[] = 	{
		0x07, 0x07, 0x07, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04 ,
		0x04 	};


	XtVaGetValues( widget ,
		XmNbackground, &gcval.background ,
		XmNforeground, &gcval.foreground ,
		NULL );

	bitmapGC = XtAllocateGC( widget, depth, gcmask, &gcval, 0 ,0 );

	/*======> DO SYMBOLS	*/

	symbol[1].width  = 5;
	symbol[1].height = 6;
	symbol[1].x_hot  = 2;
	symbol[1].y_hot  = 5;
	bitmap = XCreateBitmapFromData( DisplayStructure.display, drawable ,
			small_triangle, symbol[1].width ,symbol[1].height );

	symbol[1].pixmap = XCreatePixmap( DisplayStructure.display, drawable ,
			symbol[1].width, symbol[1].height, depth ); 
	XCopyPlane( DisplayStructure.display, bitmap, symbol[1].pixmap, bitmapGC ,
			0, 0, symbol[1].width, symbol[1].height, 0, 0, 1 );


	symbol[2].width  = 5;
	symbol[2].height = 9;
	symbol[2].x_hot  = 4;
	symbol[2].y_hot  = 0;
	bitmap = XCreateBitmapFromData( DisplayStructure.display, drawable ,
			large_triangle, symbol[2].width ,symbol[2].height );

	symbol[2].pixmap = XCreatePixmap( DisplayStructure.display, drawable ,
			symbol[2].width, symbol[2].height, depth ); 
	XCopyPlane( DisplayStructure.display, bitmap, symbol[2].pixmap, bitmapGC ,
			0, 0, symbol[2].width, symbol[2].height, 0, 0, 1 );

	symbol[3].width  = 9;
	symbol[3].height = 18;
	symbol[3].x_hot  = 4;
	symbol[3].y_hot  = 17;
	bitmap = XCreateBitmapFromData( DisplayStructure.display, drawable ,
			( char * ) arrow, symbol[3].width ,symbol[3].height );

	symbol[3].pixmap = XCreatePixmap( DisplayStructure.display, drawable ,
			symbol[3].width, symbol[3].height, depth ); 
	XCopyPlane( DisplayStructure.display, bitmap, symbol[3].pixmap, bitmapGC ,
			0, 0, symbol[3].width, symbol[3].height, 0, 0, 1 );

	symbol[4].width  = 9;
	symbol[4].height = 18;
	symbol[4].x_hot  = 4;
	symbol[4].y_hot  = 0;
	bitmap = XCreateBitmapFromData( DisplayStructure.display, drawable ,
			( char * ) icycle, symbol[4].width ,symbol[4].height );

	symbol[4].pixmap = XCreatePixmap( DisplayStructure.display, drawable ,
			symbol[4].width, symbol[4].height, depth ); 
	XCopyPlane( DisplayStructure.display, bitmap, symbol[4].pixmap, bitmapGC ,
			0, 0, symbol[4].width, symbol[4].height, 0, 0, 1 );

	symbol[5].width  = 3;
	symbol[5].height = 14;
	symbol[5].x_hot  = 0;
	symbol[5].y_hot  = 10;
	bitmap = XCreateBitmapFromData( DisplayStructure.display, drawable ,
			hi_bits, symbol[5].width ,symbol[5].height );

	symbol[5].pixmap = XCreatePixmap( DisplayStructure.display, drawable ,
			symbol[5].width, symbol[5].height, depth ); 
	XCopyPlane( DisplayStructure.display, bitmap, symbol[5].pixmap, bitmapGC ,
			0, 0, symbol[5].width, symbol[5].height, 0, 0, 1 );

	symbol[6].width  = 3;
	symbol[6].height = 10;
	symbol[6].x_hot  = 2;
	symbol[6].y_hot  = 6;
	bitmap = XCreateBitmapFromData( DisplayStructure.display, drawable ,
			lo_bits, symbol[6].width ,symbol[6].height );

	symbol[6].pixmap = XCreatePixmap( DisplayStructure.display, drawable ,
			symbol[6].width, symbol[6].height, depth ); 
	XCopyPlane( DisplayStructure.display, bitmap, symbol[6].pixmap, bitmapGC ,
			0, 0, symbol[6].width, symbol[6].height, 0, 0, 1 );

	symbol[7].width  = 3;
	symbol[7].height = 10;
	symbol[7].x_hot  = 2;
	symbol[7].y_hot  = 6;
	bitmap = XCreateBitmapFromData( DisplayStructure.display, drawable ,
			lo_bits, symbol[7].width ,symbol[7].height );

	symbol[7].pixmap = XCreatePixmap( DisplayStructure.display, drawable ,
			symbol[7].width, symbol[7].height, depth ); 
	XCopyPlane( DisplayStructure.display, bitmap, symbol[7].pixmap, bitmapGC ,
			0, 0, symbol[7].width, symbol[7].height, 0, 0, 1 );

	XFreePixmap( DisplayStructure.display, bitmap );
}
	}
#endif

sr_Point sr_xm_string_size( const Cstring & text, int & ascent )
	{
	Dimension	width = 0, height = 0;
#ifdef GUI
if ( GUI_Flag ){
	XmString	string = XmStringCreateSimple( text() );

if ( PLOTdebug )
	cerr << "xm_string_size...\n";
	XmStringExtent( DisplayStructure.font_list, string, &width, &height );
	ascent = gfont->ascent;

	XmStringFree( string );
}
#endif
	return sr_Point( width, height );
	}

int sr_xm_get_font_height()
	{
#ifdef GUI
if ( GUI_Flag ){
	return DisplayStructure.FontHeight;
}
else
	return 1;
#else
	return 1;
#endif
	}

int sr_xm_get_font_width()
	{
#ifdef GUI
if ( GUI_Flag ){
	return DisplayStructure.FontWidth;
}
else
	return 1;
#else
	return 1;
#endif
	}

void sr_xm_free_all_GCs()
	{
#ifdef GUI
if ( GUI_Flag ){
	int i;

if ( PLOTdebug )
	cerr << "sr_xm_free_all_GCs...\n";
	if ( CursorGCdefined )
		XFreeGC( DisplayStructure.display, cursorGC );
	if ( FillGCdefined )
		{
		XFreeGC( DisplayStructure.display, fillGC[HATCHED] );
		XFreeGC( DisplayStructure.display, fillGC[VERTICAL] );
		XFreeGC( DisplayStructure.display, fillGC[DIAGONAL] );
		XFreeGC( DisplayStructure.display, fillGC[HORIZONTAL] );
		XFreeGC( DisplayStructure.display, fillGC[SOLID] );
		XFreeGC( DisplayStructure.display, fillGC[FUZZY] );
//		XFreeGC( DisplayStructure.display, fillGC[UNDEFINED_STYLE] );
//		XFreeGC( DisplayStructure.display, fillGC[7] );
		XFreeGC( DisplayStructure.display, fillGC[DIAG_OVER_MAX] );
		XFreeGC( DisplayStructure.display, fillGC[DIAG_UNDER_MIN] );
		}
	if ( MainGCdefined )
		{
		for( i = 0; i < 4; i++ )
			XFreeGC( DisplayStructure.display, canvasGC[i] );
		XFreeGC( DisplayStructure.display, legendGC[0] );
		XFreeGC( DisplayStructure.display, legendGC[1] );
		}
	if ( gfont )
		XFreeFont( DisplayStructure.display, gfont );
	if ( inputfont )
		XFreeFont( DisplayStructure.display, inputfont );
	gfont = NULL;
	inputfont = NULL;
}
#endif
	}

void sr_xm_update_screen()
        {
#ifdef GUI
if ( GUI_Flag ){
        while ( XtAppPending( DisplayStructure.context ) )
                XtAppProcessEvent( DisplayStructure.context, XtIMAll );
}
#endif
	}

sr_Point sr_xm_cursor_position( void * e )
	{
	sr_Point  position;
#ifdef GUI
if ( GUI_Flag ){
	XEvent * cursor_event = ( XEvent * ) e;

	position.x = cursor_event->xbutton.x;
	position.y = cursor_event->xbutton.y;
}
#endif
	return position;
	}

void sr_xm_cursor_style( int on_off, sr_drawing_structure * ds )
	{
#ifdef GUI
if ( GUI_Flag ){

	int j;
if ( PLOTdebug )
	cerr << "sr_xm_cursor_style...\n";
	if ( ! on_off )
		{
		for ( j = 0; j < ds->drawing_count; j++ )
			{
			XtRemoveEventHandler( ds->canvas[j], ExposureMask ,
				FALSE, ( XtEventHandler ) cursorExpose ,
				( char * ) ds );
			XtRemoveEventHandler( ds->canvas[j], PointerMotionMask ,
				FALSE, ( XtEventHandler ) cursor_handler ,
				( char * ) ds );
			}
		clear_long_cursor( ds );
		}
	else
		{
		for ( j = 0; j < ds->drawing_count; j++ )
			{
			XtAddEventHandler( ds->canvas[j], ExposureMask ,
				FALSE, ( XtEventHandler ) cursorExpose ,
				( char * ) ds );
			XtAddEventHandler( ds->canvas[j], PointerMotionMask ,
				FALSE, ( XtEventHandler ) cursor_handler ,
				( char * ) ds );
			}
		}
}
#endif
	}

#ifdef GUI
void cursor_handler( Widget, char * data, XEvent * event )
	{
if ( GUI_Flag ){

	int j;
	sr_drawing_structure * ds = ( sr_drawing_structure * ) data;
if ( PLOTdebug )
	cerr << "cursor_handler...\n";
	if ( ds->old_x )
		{
		for ( j = 0; j < ds->drawing_count; j++ )
		draw_cursor( ds->canvas[j], ds->old_x );
		}

	if ( event->xbutton.x > 0 )
		{
		ds->old_x = event->xbutton.x;
		for ( j = 0; j < ds->drawing_count; j++ )
			draw_cursor( ds->canvas[j], ds->old_x );
		}
}
	}



void cursorExpose( Widget widget, char * data, XEvent * event )
	{
if ( GUI_Flag ){
	sr_drawing_structure * ds = ( sr_drawing_structure * ) data;
	int       x = event->xexpose.x, y = event->xexpose.y;

if ( PLOTdebug )
	cerr << "cursorExpose...\n";
	if ( ds->old_x > x && ds->old_x < ( x + event->xexpose.width ) )
		{
		XDrawLine( DisplayStructure.display, XtWindow( widget ), cursorGC ,
			ds->old_x, y, ds->old_x, y + event->xexpose.height );
		}
}

	}
#endif



void draw_cursor( Widget widget, int x )
	{
#ifdef GUI
if ( GUI_Flag ){

	Dimension       height;
if ( PLOTdebug )
        cerr << "draw_cursor...\n";


	XtSetArg( args[0], XmNheight, &height );
	XtGetValues( widget, args, 1 );
	XDrawLine( DisplayStructure.display, XtWindow( widget ), cursorGC, x ,0 ,x, height );
	XmUpdateDisplay( widget );

}
#endif
	}


void clear_long_cursor( sr_drawing_structure * ds )
	{
#ifdef GUI
if ( GUI_Flag ){

if ( PLOTdebug )
        cerr << "clear_long_cursor...\n";
	if ( ds->old_x > 0 )
		{
		int j;

		for ( j = 0; j < ds->drawing_count; j++ )
			draw_cursor( ds->canvas[j], ds->old_x );
		ds->old_x = 0;
		}
}
#endif
	}


void windowResizeEVENT( Widget resized, callback_stuff * client_data, void * )
        {
#ifdef GUI
if ( GUI_Flag ){

        Widget          cliparea = NULL;
        Dimension       width;

        XtVaGetValues( resized, XmNclipWindow, &cliparea, NULL );
        XtVaGetValues( cliparea, XmNwidth, &width, NULL );

// debug SILLY
// if ( PLOTdebug )
        cerr << "windowResizeEVENT for \"" << XtName( resized ) << "\"... width: " << width <<
        	"; widget affected: \"" << XtName( ( Widget ) client_data->data ) << "\"...\n";

        XtVaSetValues( ( Widget ) client_data->data, XmNwidth, width, NULL );

}
#endif
        }


sr_Rectangle sr_xm_get_rect_from_expose( void * e )
	{
	sr_Rectangle       rect;

#ifdef GUI
if ( GUI_Flag ){

	XEvent  *event = ( XEvent * ) e;

if ( PLOTdebug )
        cerr << "sr_xm_get_rect_from_expose...\n";
	rect.x = event->xexpose.x;
	rect.y = event->xexpose.y;
	rect.width = event->xexpose.width;
	rect.height = event->xexpose.height;
}
#endif
	return rect;
	}

int sr_xm_is_iconic( Widget w )
	{
	Boolean b = TRUE;
#ifdef GUI
if ( GUI_Flag ){

	Widget first_vendor_ancestor = w;

if ( PLOTdebug )
        cerr << "sr_xm_is_iconic...\n";
	while ( ! XtIsVendorShell( first_vendor_ancestor ) )
		first_vendor_ancestor = XtParent( first_vendor_ancestor );
	XtVaGetValues( first_vendor_ancestor, XmNiconic, &b, NULL );
}
#endif
	return ( int ) b;
	}
