#if HAVE_CONFIG_H
#include <config.h>
#endif
// APGEN IO_plot_util.C, adapted from:
//
//	PROJECT:	SEQ_GEN
//	SUBSYSTEM:	User Interface
//	FILE NAME:	U_panel.C (original), plot_util.C (SEQ_REVIEW)
//	DESCRIPTION:
//
//	 
//  Date     Revision	Name		Reason for change
//------------------------------------------------------------------------
//  3/25/91	000	R. Brill	Original Design
//  7/27/93	v19	R. Oliphant	Software Maintenance/Upgrade
//  4/95	v21	P. Maldague	SEQ_REVIEW adaptation
//

#ifdef   UNUSED_CODE
// #include <sys/param.h>			// To get MAX
//
// PFM 12/13/96: Solaris include doesn't define MAX; so we define it here:

#define MAX(a,b) (((a)>(b))?(a):(b))

#endif /*UNUSED_CODE*/

#include "IO_plot_util.H"
#include "apDEBUG.H"

#ifdef GUI
#	include "UI_GeneralMotifInclude.H"
#endif

using namespace std;

static const DeviceColor DEFAULT_COLOR = {(char*) "DarkGreen", 0, 128, 0};

// #define apDEBUG

Plotter_slice::Plotter_slice( /* ofstream& f , */
	int x_off , int x_wid , double x_scl ,
	int y_off , int y_hgt , double y_scl ) {
    //Store data members.
    /* fout = f ; */
    x_offset = x_off ;
    x_width  = x_wid ;
    x_scale = x_scl ;
    y_offset = y_off ;
    y_height = y_hgt ;
    y_scale = y_scl ;

    //Figure out transformation to this Plotter_slice, as specified by args.


    //Save old graphics context, then write transform to PostScript output.
    fout.undefine() ;	//reset PS output string before first write
    fout << "gsave\n" ;
    fout << x_off << " " << y_off << " translate\n" ;
    //fout << x_scl << " " << y_scl << " scale\n" ;
    //NO!  Scaling is done inline at each method, where actual coordinates are
    //  scaled.  Must be done this way to avoid undesirable effects of scaling
    //  the CTM, i.e. line widths are scaled also.

    //Set clip region (in transformed coordinates).
    fout << "newpath "		//Clockwise rectangle
     << 0 << " " << 0 << " moveto "
     << x_wid << " " << 0 << " lineto "
     << x_wid << " " << y_hgt << " lineto "
     << 0 << " " << y_hgt << " lineto closepath clip\n" ; }

//Do work that we would want actual destructor to do, but can't because cannot
//  return anything from the destructor!
Cstring & Plotter_slice::destructor()
{
    fcopy.undefine() ;
    fcopy << "grestore\n" ;
    return fcopy ;
}

//Get the accumulated string of PostScript output, and reset it to empty.
Cstring & Plotter_slice::get_output()
{
    fcopy.undefine() ;
    fcopy = fout() ;
    fout.undefine() ;
    return fcopy ;
}

//Draw numlines disconnected line segments.
void Plotter_slice::draw_line_segments(
	sr_Segment * lines ,
	const int numlines ,
	const int bold ,
	const int dashed )
{
    char linetype[16] ;

    DBG_NOINDENT( "Plotter_slice::draw_line_segments( lines, numlines=" << numlines
         << ", bold=" << bold << ", dashed=" << dashed
         << ")" << endl ) ;

    if ( bold )
	if ( dashed )
	    strcpy( linetype , " bolddashline\n" ) ;
	else
	    strcpy( linetype , " boldline\n" ) ;
    else
	if ( dashed )
	    strcpy( linetype , " dashline\n" ) ;
	else
	    strcpy( linetype , " line\n" ) ;

    //loop to draw each line segment
    double x_scl = x_sc() ;
    double y_scl = y_sc() ;
    for ( int i = 0 ; i < numlines ; i++ )
    {
	fout << ( x_scl * lines[i].x1 ) << " "
	     << ( y_scl * lines[i].y1 ) << " "
	     << ( x_scl * lines[i].x2 ) << " "
	     << ( y_scl * lines[i].y2 ) << (char *)linetype ;
    }
}

//Draw (numpoints-1) connected line segments, i.e. a polyline.
void Plotter_slice::draw_lines(
	sr_IntPoint * points ,
	const int numpoints ,
	const int bold ,
	const int dashed ) {
    char linetype[16] ;

    DBG_NOINDENT( "Plotter_slice::draw_lines( points, numpoints=" << numpoints
         << ", bold=" << bold << ", dashed=" << dashed
         << ")" << endl ) ;

    if ( bold )
	if ( dashed )
	    strcpy( linetype , " bolddashline\n" ) ;
	else
	    strcpy( linetype , " boldline\n" ) ;
    else
	if ( dashed )
	    strcpy( linetype , " dashline\n" ) ;
	else
	    strcpy( linetype , " line\n" ) ;

    //loop to draw each polyline segment
    double x_scl = x_sc() ;
    double y_scl = y_sc() ;
    for ( int i = 0 ; i < numpoints - 1 ; i++ ) {
	fout << ( x_scl * points[i].x )   << " "
	     << ( y_scl * points[i].y )   << " "
	     << ( x_scl * points[i+1].x ) << " "
	     << ( y_scl * points[i+1].y ) << (char *)linetype ; } }

//Draw (numpoints-1) connected line segments, i.e. a polyline, and fill in underneath
void Plotter_slice::fillToBaseline(sr_IntPoint * points, const int numpoints, DeviceColor* color, bool fillAboveLine) {
  const DeviceColor* drawColor = (color == 0 ? &DEFAULT_COLOR : color);
  
  //loop to draw each polyline segment
  double xScale = x_sc();
  double yScale = y_sc();
  
  fout << "newpath % fillToBaseline\n";
  if (fillAboveLine)
    fout << xScale * points[0].x << " 65535.0 moveto % left base of filled polygon\n";	// 65535 is an arbitrary big number
  else
    fout << xScale * points[0].x << " 0 moveto % left base of filled polygon\n";
  
  for (int i = 0; i < numpoints; i++) {	// loop for each point on the polyline
    if (i > 0 && points[i].x == points[i - 1].x && points[i].y == points[i - 1].y)	// skip identical points in line
      continue;
    else
      fout << xScale * points[i].x << " " << yScale * points[i].y << " lineto\n";
  }

  if (fillAboveLine)
    fout << xScale * points[numpoints - 1].x  << " 65535.0 lineto % back to the base line for the right edge\n";
  else
    fout << xScale * points[numpoints - 1].x  << " 0 lineto % back to the base line for the right edge\n";
  
  fout << "closepath % complete the region to fill\n";
  fout << drawColor->red << " 255 div "
       << drawColor->green << " 255 div "
       << drawColor->blue << " 255 div setrgbcolor % set color for the fill to " << drawColor->name << "\n";
  fout << "fill\n";
  fout << "0 0 0 setrgbcolor % end fillToBaseline; set color back to black\n";
}

void Plotter_slice::fillRegionWithPattern(sr_IntPoint * points, const int numpoints, DeviceColor* color, const char* patternName, bool fillAboveLine) {
  const DeviceColor* drawColor = (color == 0 ? &DEFAULT_COLOR : color);
  
  //loop to draw each polyline segment
  double xScale = x_sc();
  double yScale = y_sc();
  
  fout << "gsave newpath % fillRegionWithPattern\n";
  if (fillAboveLine)
    fout << xScale * points[0].x << " 65535.0 moveto % left base of filled polygon\n";	// 65535 is an arbitrary big number
  else
    fout << xScale * points[0].x << " 0 moveto % left base of filled polygon\n";
  
  for (int i = 0; i < numpoints; i++) {	// loop for each point on the polyline
    if (i > 0 && points[i].x == points[i - 1].x && points[i].y == points[i - 1].y)	// skip identical points in line
      continue;
    else
      fout << xScale * points[i].x << " " << yScale * points[i].y << " lineto\n";
  }

  if (fillAboveLine)
    fout << xScale * points[numpoints - 1].x  << " 65535.0 lineto % back to the base line for the right edge\n";
  else
    fout << xScale * points[numpoints - 1].x  << " 0 lineto % back to the base line for the right edge\n";
  
  fout << "closepath clip % complete the region to fill\n";
  fout << drawColor->red << " 255 div "
       << drawColor->green << " 255 div "
       << drawColor->blue << " 255 div setrgbcolor % set color for the fill to " << drawColor->name << "\n";
  fout << "/" << patternName << " 0 0 " << width() << " " << height() << " rectanglefill\n";
  fout << "0 0 0 setrgbcolor % end fillToBaseline; set color back to black\n";
  fout << "grestore\n";
}

void Plotter_slice::draw_text( const int xx , const int yy , char* text ) {
    DBG_NOINDENT( "Plotter_slice::draw_text(" << xx << ", " << yy << ", " << text
         << ")" << endl ) ;

    fout << "(" << text << ") "
         << ( x_sc() * xx ) << " " << ( y_sc() * yy ) << " text\n" ; }

#if 0
// -----------------------------------------------------------------------
//	Fill CitiBank shape (no appropriate term found in dictionary).

void Plotter_slice::fill_citibank(sr_CitiBank acitibank, const int style) {
  //"CitiBank" truncated-rectangle always uses exactly 4 points

  DeviceColor* drawColor = &DEFAULT_COLOR;
  sr_IntPoint points[4];
  
  sr_xm_citibank_to_polygon(acitibank, points);	// convert to my coordinates

  fout << "newpath % start Plotter_slice::fill_citibank\n";
  fout << points[0].x << " " << points[0].y << " moveto\n";
  for (int i = 1; i < 4; i++)
    fout << points[i].x << " " << points[i].y << " lineto\n";
  fout << "closepath\n" ;
  fout << drawColor->red << " 255 div "
       << drawColor->green << " 255 div "
       << drawColor->blue << " 255 div setrgbcolor % set color for the fill to " << drawColor->name << "\n";
  
  fout << "0 setgray\n";
  fout << "fill % end Plotter_slice::fill_citibank\n";
}

// -----------------------------------------------------------------------
//	The width of an X rectangle is 1 less than expected.  Hence
//	the "-1" below.

void Plotter_slice::fill_rectangle( sr_Rectangle arect , const int style ) {
  // ignore style
  DeviceColor* drawColor = &DEFAULT_COLOR;
  
  fout << drawColor->red << " 255 div "
       << drawColor->green << " 255 div "
       << drawColor->blue << " 255 div setrgbcolor % set color for the fill to " << drawColor->name << "\n";
  fout << arect.x << " " << arect.y << " " << arect.width << " " << arect.height << " rectfill\n";
  fout << "0 0 0 setrgbcolor\n";
}

// -----------------------------------------------------------------------
//	The width of an X rectangle is 1 less than expected.  Hence
//	the "-1" below.

void Plotter_slice::draw_rectangles(sr_Rectangle* rects, const int numrects, const int bold,const int dashed) {
#if 0
  sr_Segment line ;
  
  for (int i = 0 ; i < numrects ; i++) {
    rects[i].x += x ;
    rects[i].y += y ;
    if (rects[i].x >= x) {
      plotter.draw_rectangles(rects+i, 1, bold, dashed) ;
    }
    else {
      line.x1 = x ;
      line.y1 = rects[i].y ;
      line.x2 = rects[i].x ;
      if(rects[i].width > 1)
	line.x2 += rects[i].width - 1 ;
      line.y2 = line.y1 ;
      plotter.draw_line_segments(&line, 1, bold, dashed) ;
      line.x1 = line.x2 ;
      line.y1 += rects[i].height ; 
      plotter.draw_line_segments(&line, 1, bold, dashed) ;
      line.x2 = x ;
      line.y2 = line.y1 ;
      plotter.draw_line_segments(&line, 1, bold, dashed) ;
    }
  }
#endif
  for (int i = 0; i < numrects; i++) {
    fout << "gsave % Plotter_slice::draw_rectangles [" << i << "]\n";
    if (bold)
      fout << "1.44 setlinewidth\n";
    else
      fout << "0.48 setlinewidth\n";
    if (dashed)
      fout << "[2 4 2 4] 0 setdash\n";
    //else
    //  fout << "[] 0 setdash\n";
    fout << rects[i].x << " " << rects[i].y << " " << rects[i].width << " " << rects[i].height << " rectstroke\n";
    fout << "grestore\n";
  }
}
#endif

#ifdef   UNUSED_FUNC
// -----------------------------------------------------------------------

// =========> Postscript code all appears below ==========================

const int	Plotter::WIDTH  = 576 ; 		// 8"
const int	Plotter::HEIGHT = 756 ; 		// 10.5"

const char * Plotter::header[] = 	{
	"%!SEQ_GEN Plot File" ,
		// change the next 4 lines if the plotter changes
	// "90 rotate" ,
	// "1.42 -.46 scale" ,
	"0 30 translate" ,
	"/Helvetica findfont [1 0 0 1 0 0] makefont" ,
	// "/Times-Roman findfont [.706 0 0 -2.16 0 0] makefont" ,
		// undo scaling for fonts
	"8 scalefont" ,
	"setfont" ,
	"" ,
	"/rectangle 		%stack:x y w h => ---" ,
	"	{newpath" ,
	"	4 2 roll" ,
	"	moveto" ,
	"	1 index 0 rlineto" ,
	"	0 exch rlineto" ,
	"	neg 0 rlineto" ,
	"closepath" ,
	"stroke	} def" ,
	"" ,
	"/rectanglefill		%stack:x y w h s => ---" ,
	"	{newpath" ,
	"setgray" ,
	"	4 2 roll" ,
	"	moveto" ,
	"	1 index 0 rlineto" ,
	"	0 exch rlineto" ,
	"	neg 0 rlineto" ,
	"closepath" ,
	"fill" ,
	"0 setgray	} def" ,
	"" ,
	"/line			%stack:x1 y1 x2 y2 => ---" ,
	"	{newpath" ,
	"	moveto" ,
	"	lineto" ,
	"stroke	} def" ,
	"" ,
	"/dashed_line			%stack:x1 y1 x2 y2 => ---" ,
	"	{[6 2 1 2] 0 setdash" ,
	"	newpath" ,
	"	moveto" ,
	"	lineto" ,
	"	stroke" ,
	"[] 0 setdash	} def" ,
	"" ,
	"/text			%stack: string x y => ---" ,
	"	{moveto" ,
	"show	} def" ,
	"" ,
	"0.75 setlinewidth" ,
	0
	} ;

// -----------------------------------------------------------------------
//

void Plotter::draw_line_segments(
	sr_Segment * lines ,
	const int numlines ,
	const int bold ,
	const int dashed )
	{
	const char * routine ;

	if( ! dashed )
		routine = " line" ;
	else
		routine = " dashed_line" ;

	if( bold )
		{
		for ( int i = 0 ; i < numlines ; i++ )
			{
			OutputDevice << "1.25 setlinewidth\n" << lines[i].x1 << " " << ( HEIGHT - lines[i].y1 ) << " " <<
			    lines[i].x2 << " " << ( HEIGHT - lines[i].y2 ) << routine << "\n0.75 setlinewidth" << "\n" ;
			if( lines[i].x2 > WIDTH ) break ;
			}
		}
	else
		{
		for ( int i = 0 ; i < numlines ; i++ )
			{
			OutputDevice << lines[i].x1 << " " << ( HEIGHT - lines[i].y1 ) << " " <<
			    lines[i].x2 << " " << ( HEIGHT - lines[i].y2 ) << routine << "\n" ;
			if( lines[i].x2 > WIDTH ) break ;
			}
		}
	}

void Plotter::draw_lines(
	sr_IntPoint * points ,
	const int numpoints ,
	const int bold ,
	const int )
	{
	if( bold )
		{
		for ( int i = 0 ; i < numpoints - 1 ; i++ )
			{
			if( points[i].x > WIDTH ) break ;
			OutputDevice << "1.25 setlinewidth\n" << points[i].x << " " << ( HEIGHT - points[i].y ) << " " <<
			    points[i+1].x << " " << ( HEIGHT - points[i+1].y ) << " line\n0.75 setlinewidth" << "\n" ;
			}
		}
	else
		{
		for ( int i = 0 ; i < numpoints - 1 ; i++ )
			{
			if( points[i].x > WIDTH ) break ;
			OutputDevice << points[i].x << " " << ( HEIGHT - points[i].y ) << " " <<
			    points[i+1].x << " " << ( HEIGHT - points[i+1].y ) << " line" << "\n" ;
			}
		}
	}


// -----------------------------------------------------------------------
//	The width of an X rectangle is 1 less than expected.  Hence
//	the "-1"s below.

void Plotter::draw_rectangles(
	sr_Rectangle* rects ,
	const int numrects ,
	const int ,
	const int )
	{
	int	w , h ;

	for ( int i = 0 ; i < numrects ; i++ )
		{

		// I think this is nonsense... PFM
		//w = 0 ;
		//if ( rects[i].width > 1 )
		//	w = ( int ) rects[i].width - 1 ;
		//h = ( int ) rects[i].height - 1 ;
		//

		w = ( int ) rects[i].width ;
		h = ( int ) rects[i].height ;

		OutputDevice << rects[i].x << " " <<  ( HEIGHT - rects[i].y - h ) << " " <<
		w << " " << h << " rectangle" << "\n" ;
		}
	}

// -----------------------------------------------------------------------
//	Fill CitiBank shape (no appropriate term found in dictionary).

void Plotter::fill_citibank( sr_CitiBank /*acitibank*/, const int /*style*/ )
{
	;
}

// -----------------------------------------------------------------------
//	The width of an X rectangle is 1 less than expected.  Hence
//	the "-1"s below.

void Plotter::fill_rectangle( sr_Rectangle arect , const int style )
	{
	int	w = 0 , h ;
	float	s = 1.0 - ( float ) style * 0.15 ;

	if ( arect.width > 1 )
		w = ( int ) arect.width - 1 ;
	h = ( int ) arect.height - 1 ;

	OutputDevice << arect.x << " " << ( HEIGHT - arect.y - h ) << " " << w << " " <<
		h << " " << s << " rectanglefill " << "\n" ;
	}

// -----------------------------------------------------------------------
//

void Plotter::draw_text( const int x , const int y , char* text )
	{
	//							GUESS ??? :
	OutputDevice << "( " << text << " ) " <<  x << " " << ( HEIGHT - y + 5 ) << " text" << "\n" ;
	}

// -----------------------------------------------------------------------
//

int Plotter::width()
	{
	return WIDTH ;
	}

// -----------------------------------------------------------------------
//

int Plotter::height()
	{
	return HEIGHT ;
	}

// -----------------------------------------------------------------------
//

void Plotter::show_page()
	{
	OutputDevice << "copypage erasepage" << "\n" ;
	}

// -----------------------------------------------------------------------

Plotter::Plotter( file * f )
	: OutputDevice( f )
	{
	for ( int i = 0 ; header[i] ; i++ )
		OutputDevice << header[i] << "\n" ;
	}
#endif /*UNUSED_FUNC*/
// -----------------------------------------------------------------------
//Rest of plot_screen methods are inlined in .H file

void	plot_screen::fill_citibank(sr_CitiBank acitibank, const int style) {
	//"CitiBank" truncated-rectangle always uses exactly 4 points
	sr_IntPoint points[4];

	sr_xm_citibank_to_polygon(acitibank, points);
	sr_xm_fill_polygon(WidgetToDrawOn, points, 4, 1, style); }

int	plot_screen::width() {
	return sr_xm_width( WidgetToDrawOn ) ;
	}

int	plot_screen::height() {
	// return sr_xm_height( WidgetToDrawOn ) ;
	return height_of_screen ;
	}
