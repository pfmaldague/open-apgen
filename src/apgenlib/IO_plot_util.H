#ifndef IO_PLOT_UTIL_H
#define IO_PLOT_UTIL_H

#include "C_string.H"
// #include "C_types.H"
#include "UI_motif_widget.H"
// #include "IO_file.H" // for FileOrPrinter and file

#include "IO_plot_intfc.H"

// class Plot_window ;

// for "get_device_type()":
#define MOTIF_DEVICE		1
#define POSTSCRIPT_DEVICE	2

// =======================================================================

// Color construct for devices.  Color values run 0-255.
typedef struct {
  char *name;	// name of this color
  unsigned char red, green, blue;	// RGB values for this color (0-255)
} DeviceColor;


/** This class represents an output device.  It is used by code which
 *  may write either to the screen or to a plotter file.  It is not
 *  intended for direct use, but only as an abstract base class. */
class Device {
public:
			///
  virtual void		draw_line_segments(sr_Segment* lines, const int numline, const int bold, const int dashed)	= 0 ;

  			///
  virtual void		draw_lines(sr_IntPoint* lines, const int numline, const int bold, const int dashed)	= 0 ;

  			///
  virtual void		draw_rectangles(sr_Rectangle* rects, const int numrects, const int bold, const int dashed)	= 0 ;
  			///
  virtual void		fill_rectangle(sr_Rectangle arect, const int style)	= 0 ;

  			///
  virtual void		fill_citibank(sr_CitiBank acitibank, const int style)	= 0 ;

  			///
  virtual void		fillToBaseline(	sr_IntPoint	*points,
		  			const int	numline,
					DeviceColor	*color = 0,
					bool fillAboveLine = true ) {}

  			///
  virtual void		fillRegionWithPattern(
		  			sr_IntPoint	*points,
				     	const int	numline,
				     	DeviceColor	*color = 0,
				     	const char*	patternName = "PS_BITMAP_7",
				     	bool		fillAboveLine = true)		{}
  			///
  virtual void		draw_text( const int x, const int y, char* text )		= 0 ;
  			///
  virtual void		draw_bitmap( const int, const int, const int, const int )	{}
  			///
  virtual void		clear_drawing( sr_Rectangle , const int )			{}
  			///
  virtual int		width()				= 0 ;
  			///
  virtual int		height()		= 0 ; //DSG 96-10-07
  			///
  virtual int		top() = 0 ;
  			///
  virtual double	x_sc()			= 0 ; //DSG 97-09-28
  			///
  virtual double	y_sc()			= 0 ; //DSG 97-09-28
  			///
  virtual int		get_device_type() = 0 ;
} ;

// =======================================================================

///	This class represents an X subwindow.
class plot_screen : public Device {
public:

	/** Note: the widget to draw on is more complex now, because we
	 *  are using a single DrawArea widget at the RD_sys level to
	 *  plot (potentially) many individual resources.
	 *
	 *  We will continue to assign the plot_screen to RD_sys (or
	 *  MW_widgetdraw, even though one could assign one to each
	 *  MW_object instead. Assiging to RD_sys makes things more
	 *  consistent with the design of ACT_sys, which is in charge
	 *  of redrawing each band of the display in sync with the
	 *  LegendObjects displayed (as toggle buttons) on the DS_legend.
	 */
	plot_screen( motif_widget * w, int wi , int y , int h , int which_gc )
		: WidgetToDrawOn(w),
		IndexOfGCToUse(which_gc) ,
		width_of_screen( wi ) ,
		top_of_screen( y ) ,
		height_of_screen( h )
		{}
	/// Destructor
	~plot_screen() {}

				///
	void			draw_line_segments(	sr_Segment *lines,
							const int numlines, 
							const int bold,
							const int dashed) {
					sr_xm_draw_line_segments(
							IndexOfGCToUse,
							WidgetToDrawOn,
							lines,
							numlines,
							bold,
							dashed) ;
					}
				///
	void			draw_lines(		sr_IntPoint * points,
							const int numpoints, 
							const int bold,
							const int dashed) {
					sr_xm_draw_lines(
							IndexOfGCToUse,
							WidgetToDrawOn,
							points,
							numpoints,
							bold,
							dashed) ;
					}
				///
	void			draw_rectangles(	sr_Rectangle* rects,
							const int numrects, 
							const int bold,
							const int dashed) {
					sr_xm_draw_rectangles(
							IndexOfGCToUse,
							WidgetToDrawOn,
							rects,
							numrects,
							bold,
							dashed) ;
					}
				///
	void			fill_rectangle(		sr_Rectangle arect,
							const int style) {
					sr_xm_fill_rectangle(
						WidgetToDrawOn,
						arect,
						style) ;
					}

				/// Unlike most methods of this class, fill_citibank() is NOT inlined.
	void			fill_citibank(	sr_CitiBank	acitibank,
						const int	style) ;
				///
	void			draw_text(	const int x,
						const int y,
						char* text) {
					sr_xm_draw_text(
						IndexOfGCToUse,
						WidgetToDrawOn,
						x,
						y,
						text) ;
					}
				///
	void			draw_bitmap(	const int symbol,
						const int x,
						const int y, 
						const int write_flag) {
					sr_xm_draw_bitmap(
						WidgetToDrawOn,
						symbol,
						x,
						y,
						write_flag) ;
					}
				///
	void			clear_drawing(sr_Rectangle rect, const int exposure) {
					sr_xm_clear_drawing(	WidgetToDrawOn,
								rect,
								exposure) ;
					}

				///
	motif_widget		*get_widget()
					{ return WidgetToDrawOn ; }

				/** Extracts the width info from the motif_widget using
				 * sr_xm_width() */
	int			width() ;
				/** Extracts the height info from the motif_widget using
				 * sr_xm_height() */
	int			height() ;

				///
	int			top() { return top_of_screen ; }
				///
	double			x_sc() { return 1.0 ; }
				///
	double			y_sc() { return 1.0 ; }
				///
	int			get_device_type()
					{ return MOTIF_DEVICE ; }
private:
				///
	motif_widget		*WidgetToDrawOn ;
				///
	int			IndexOfGCToUse ;	// 1 means 'use GC appropriate for Legend'
							// 0 means 'use GC appropriate for Canvas'
				/** We have a design choice concerning how top y and height are computed:
				 * we can include that info in the constructor signature, or we can store
				 * a pointer to the MW_object and query it. Let's assume for the time
				 * being that the constructor can easily be provided by the client.  */
	int			top_of_screen, height_of_screen , width_of_screen ;
	} ;

// =======================================================================
//	This class represents a plotter and plotter-language.  It creates
//	an output file appropriate to those two items.  All of the code
//	peculiar to the plotter-language resides in this class.

// enum FILE_OR_PRINTER { FILE_TYPE, PRINTER_TYPE } ;
#ifdef UNUSED_FUNC
class FileOrPrinter ;

	/// This class represents a PostScript output device (e. g. a printer)
class Plotter : public Device
	{
public:
	/// Constructor
	Plotter(file * File) ;
				///
	void			draw_line_segments(sr_Segment *,const int, const int, const int) ;
				///
	void			draw_lines(sr_IntPoint *,const int, const int, const int) ;
				///
	void			draw_rectangles(sr_Rectangle*, const int, const int, const int) ;
				///
	void			fill_rectangle(sr_Rectangle arect, const int style) ;
				///
	void			fill_citibank(sr_CitiBank acitibank, const int style) ;
				///
	void			draw_text(const int x, const int y, char* text) ;
				///
	void			show_page() ;
				///
	int			width() ;
				/// DSG 96-10-07
	int			height() ;
				///
	int			top() { return 0 ; }
				/// DSG 97-09-28
	double			x_sc() { return 1.0 ; }
				/// DSG 97-09-28
	double			y_sc() { return 1.0 ; }
				///
	int			get_device_type() { return POSTSCRIPT_DEVICE ; }
				///
	void			write_filename(const Cstring &) ;
private:
				///
	FileOrPrinter		OutputDevice ;
				///
	static const int	WIDTH, HEIGHT ;
				///
	static const char *	header[] ;
	} ;
#endif

// =======================================================================

/**	This class represents a portion of a plotter page.  It converts
 * 	from local coordinates to page coordinates and clips when necessary. */
class Plotter_slice : public Device {
public:
	///
  Plotter_slice(/* ofstream& f, */
		int x_off, int x_wid, double x_scl,
		int y_off, int y_hgt, double y_scl);
  ///
  ~Plotter_slice() {};
 		/// 
  void		draw_line_segments(sr_Segment *,const int, const int, const int);
  		///
  void		draw_lines(sr_IntPoint *,const int, const int, const int);
  		///
  void		draw_rectangles(sr_Rectangle*, const int, const int, const int) {}
  		///
  void		fill_rectangle(sr_Rectangle /*arect*/, const int /*style*/) {}
  		///
  void		fill_citibank(sr_CitiBank /*acitibank*/, const int /*style*/) {}
  		///
  void fillToBaseline(sr_IntPoint* points, const int numline, DeviceColor* color = 0, bool fillAboveLine = true);
  		///
  void fillRegionWithPattern(sr_IntPoint* points,
			     const int numline,
			     DeviceColor* color = 0,
			     const char* patternName = "PS_BITMAP_4",
			     bool fillAboveLine = true);
  		///
  void		draw_text(const int xx, const int yy, char* text);
  		///
  int		width() { return x_width; }
  		///
  int		height() { return y_height; }
  		///
  int		top() { return 0 ; }
  		///
  double		x_sc() { return x_scale; }
  		///
  double		y_sc() { return y_scale; }
  		///
  int		get_device_type() { return POSTSCRIPT_DEVICE; }

  //these methods are peculiar to this Device type:
 
  		///
  int		x_off() { return x_offset; }
  		///
  int		y_off() { return y_offset; }
  		///
  Cstring &	get_output();
  		/// What want destructor to do, but can't
  Cstring &	destructor();
private:
  			/// LOCAL, but get_output() allows caller to copy
  Cstring		fout;
  			///
  Cstring		fcopy;
  			///
  int		x_offset, y_offset;
  			///
  int		x_width, y_height;
  			///
  double	x_scale, y_scale;
};

#endif
