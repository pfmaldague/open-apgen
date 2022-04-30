#ifndef IO_PLOT_INTFC_H
#define IO_PLOT_INTFC_H

#include "UI_motif_widget.H"

#ifdef GUI
#	include "UI_GeneralMotifInclude.H"
#else

typedef unsigned int u_int;
#endif

//class Plot_window;

#define USE_CANVAS_GC 0
#define USE_LEGEND_GC 1


struct sr_drawing_structure {
	Widget	canvas[5];
	Widget	legend[5];
	int	old_x;
	int	drawing_count; };


struct sr_Segment {
	int	x1, y1, x2, y2; };

struct sr_Point	{ //"class sr_Point":  do NOT change to int--compatibility with X
	sr_Point() :x(0), y(0) {}
	sr_Point(short i, short j) :x(i), y(j) {}
	short	x, y; };

struct sr_IntPoint {
	int	x, y; };


struct sr_Rectangle {
	int	x, y;
	u_int	width, height; };


struct sr_CitiBank {
	int	x1, y1, x2, y2;
	int	rect_height;
	Boolean	rect_low; };


enum sr_DRAW_STYLE
	{	/* STANDARD is ignored without any filling. */
//	STANDARD, HATCHED, VERTICAL, DIAGONAL, HORIZONTAL, SOLID, UNDEFINED_STYLE
//97-05-14 DSG:  rename UNDEFINED_STYLE to better name, and add another style:
	STANDARD, HATCHED, VERTICAL, DIAGONAL, HORIZONTAL, SOLID,
	DIAG_OVER_MAX, DIAG_UNDER_MIN, FUZZY, TELEMETRY
	};


void clear_long_cursor(sr_drawing_structure *);


#ifdef GUI

typedef struct
	{
	Pixmap	pixmap;
	Position	x_hot, y_hot;
	Dimension	width, height;
	} sr_xm_BitmapStruct;
#endif


class sr_GRDisplay
	{
public:
	sr_GRDisplay()
		:display(0),
		screen(0),
		bell(0)
		{}

	~sr_GRDisplay() {}

#	ifdef GUI

	Display *	display;	/**	pointer to display structure	*/
	int		screen;		/**	screen number for display (i.e., 0) */
	int		bell;		/**	flag for bell being audible	*/
	Colormap	cmap;		/**	Storage area for colormap	*/
	XmFontList	font_list;	/**	Main default fontlist		*/
	XFontStruct *	font;		/**	Main default font		*/
	XtAppContext	context;	/**	Default application context	*/
	int		FontHeight;

	int		FontWidth;
#	else
	void * display;
	void * screen;
	void *bell;
#	endif
	};		/** Graphics Display structure for application.	*/

extern sr_GRDisplay DisplayStructure;

#ifdef GUI

typedef struct	{
	Boolean debug, bell, ontop, keepontop, destroy;
	// BUG FIX 8/6/97 PFM
	// char	*font;
	XFontStruct * font;
	Pixel	color1, color2, color3, color4, color5;
	int	inc1, inc2, inc3;
	} R0Data, *R0DataPtr;


extern R0Data theR0Data;


typedef struct	{
	char	*font;
	Pixel long_cursor_color;
	} RData, *RDataPtr;

void load_pixmaps(Widget widget, int depth, Drawable drawable);
void load_bitmaps(Widget widget, int depth, Drawable drawable);

#endif

void		sr_xm_initialize_DisplayStructure();
void		sr_xm_initialize_gcs(Widget legend, Widget canvas);
void		sr_xm_initialize_gravity(Widget drawarea);	// DSG 96-10-22
void		sr_xm_draw_lines(int which_gc, motif_widget * drawing, sr_IntPoint *lines,
			int numlines, int bold, int dashed);
void		sr_xm_draw_line_segments(int which_gc, motif_widget * drawing,
			sr_Segment *lines, int numlines, int bold, int dashed);
void		sr_xm_citibank_to_polygon(sr_CitiBank, //DSG 96-11-08 and -21
			sr_IntPoint *);			 //	caller MUST alloc 2nd arg
void		sr_xm_fill_polygon(motif_widget* drawing, sr_IntPoint* points,
			int numpoints, int convex, int style); // DSG 96-11-08
void		sr_xm_fill_rectangle(motif_widget * drawing, sr_Rectangle rect, int style);
void		sr_xm_draw_rectangles(int which_gc,
			motif_widget *drawing, sr_Rectangle *rects,
			int numrects, int bold, int dash);
void		sr_xm_draw_text(int which_gc, motif_widget * drawing, int x, int y, char * text);
void		sr_xm_draw_bitmap(motif_widget *drawing, int id, int xloc, int yloc,
			int write_flag);
void		sr_xm_clear_drawing( /* Plot_window * , */
			motif_widget *drawing, sr_Rectangle rect, int exposure);
void		sr_xm_set_focus(motif_widget *parent);
int		sr_xm_width(motif_widget *W);
int		sr_xm_height(motif_widget *W);	// DSG 96-10-07
sr_Rectangle	sr_xm_get_rect_from_expose(void *);
sr_Point	sr_xm_cursor_position(void *);
sr_Point	sr_xm_string_size(const Cstring &, int &);
int		sr_xm_get_font_height();
int		sr_xm_get_font_width();
void		sr_xm_free_all_GCs();
void		sr_xm_update_screen();
void		sr_xm_cursor_style(int on_off, sr_drawing_structure *ds);
int		sr_get_font_width();

#ifdef GUI

void cursor_handler(Widget, char * data, XEvent * event);
void cursorExpose(Widget widget, char * data, XEvent * event);

#endif

void draw_cursor(Widget widget, int x);
void windowResizeEVENT(Widget resized, callback_stuff *client_data, void *call_data);
int sr_xm_is_iconic(Widget);

#endif
