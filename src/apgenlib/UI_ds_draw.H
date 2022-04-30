/*
 * PROJECT: APGEN
 * FILE NAME: UI_ds_draw.H
 *
 * ABSTRACT
 *
 * This file defines the basic drawing element by using X window system.
 *
 *DATE    NAME              VERSION           REASON FOR CHANGE
 *------------------------------------------------------------------------
 *9/30/94 I. Lin             1.0              Initial design
 *
 * Source Code Control:    @(#) /seq/DEV21_0/apgen/lib_incl_local/UI_ds_draw.H ver. 
 *
 */
#ifndef DSDRAW_H
#define DSDRAW_H

#ifdef GUI

extern "C"
	{

#define NeedFunctionPrototypes 1

/*Xlib include fles*/
#include	<X11/X.h>
#include	<X11/Xlib.h>
#include	<X11/Xutil.h>
#include	<X11/Xos.h>
#include	<X11/Xatom.h>
#include        <X11/cursorfont.h>
#include        <X11/Intrinsic.h>
#include        <X11/keysym.h>

#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include "XM.h"
/*extern Arg args[]; DSG 96-07-05 not needed*/
extern char* APbcolor;

	}
#endif


#include "UI_dsconfig.h"
#include "DSsource.H"
#include "Legends.H"
#include "Rsource.H"
#include "UTL_time.H"

#define DIRECTSEL   0
#define INDIRECTSEL 1


#define WITHSELECTION 1
#define WITHOUTSELECTION 2

extern int LNDRAWWIDTH;

typedef struct
	{
	int patternid;
	int colorid;
	} 
 DS_id;

class DS_draw_graph;
class DS_pixmap_board;
class ACT_sys;
class CON_sys;

#ifndef GUI
typedef void * Drawable;
#endif

//
// Graphic Context class
//
class DS_gc : public Node {

#ifdef GUI

protected:
	struct gp {
		GC		gc;
		int		ct; };
	gp		*gptr;
public:
	static Display	*dp;
	int		id;
	int		cid;
	// int		ct;
	DS_gc(	Display* display,
		Drawable drawable,
		unsigned long valuemask,
		XGCValues* value);
	DS_gc(const DS_gc & org);
	~DS_gc();

	Node_type		node_type() const { return UNKNOWN_EXTENSION; }
	const Cstring&		get_key() const {
		return Cstring::null();
	}
	void			changeGC(unsigned long valuemask, XGCValues* value)
					{ XChangeGC(dp,gptr->gc,valuemask,value); }
				// more object-oriented:
	void			adopt_new_gc(const DS_gc &);
	void			switchGC(DS_gc& gc);
	GC			getGC();
	static Display		*getdisplay() { return dp; }
#else
public:
	// dummy class:
	DS_gc() {}
	DS_gc(const DS_gc &) {}
	~DS_gc() {}
#endif
	virtual Node		*copy() { return new DS_gc(*this); } };

class DS_draw {
public:
	DS_draw(Drawable draw) { drawon=draw; }

	virtual void		draw()  {; }

	Drawable		drawon;
};

class DS_object : public DS_draw, public DS_gc, public back_pointer {
private:
				//alternate graphic context, used for highlighting (e.g. when selected)
	static DS_gc		*hilitegc;
protected:
	int			new_cm_x, new_cm_y;
				//Pointer to the data object this DS_object representing
				// PFM: more strongly typed (used to be void *)
	Dsource*		sourceobj;	//DSG 96-06-07, was Act_req*
public:
	DS_object(DS_gc &gc, Drawable draw, DS_draw_graph *obj);
	DS_object(DS_gc &gc, Drawable draw, Dsource* source, DS_draw_graph *obj);
	// This will exercise the back_pointer copy contructor:
	DS_object(const DS_object &o) : DS_draw(o.drawon), DS_gc(o), back_pointer(o) {
		sourceobj = o.sourceobj;
		graphobj = o.graphobj;
	}
	~DS_object();
	// DATA MEMBERS
				// used for activities too short to see:
	static DS_gc		*short_act_gc;
				// used for indicating overlapping activities:
	static DS_gc		*busy_gc;
				// used to draw separation lines between legends:
	static DS_gc		*separation_gc;
				// used to draw the 'unused' portion of an activity request timeline:
	static DS_gc		*grey_gc;
				//Pointer to the graph object this DS_object belongs to
	DS_draw_graph		*graphobj;
	static DS_pixmap_board 	*bk_board;
	// METHODS
	void			setto(int x,int y) { new_cm_x=x; new_cm_y=y; }    
	int			getselection(); // PFM { return selected; }
	virtual int		is_selected(int, int) { return 0; }
	virtual void		draw() {; }
				//move to right
	virtual void		hmove(int) {; }
	virtual void		resize(float,float) {; }

				//
				// Pointer to the data source object
				//
	//DSG 96-06-07 changed from Act_req* to Dsource*, in these 2 methods:
	Dsource*		getsource() {
		return sourceobj;
	}
	static void		sethilite(DS_gc * hgc) { hilitegc = hgc; }
	static DS_gc		*gethilite();
	static DS_gc		*get_separation_gc();
	static DS_gc		*get_grey_gc();
	}; 

class DS_point : public DS_object {
protected:
	int	x;
	int	y;
public:
#ifdef GUI
	DS_point(DS_gc& gc, Drawable draw, int x_coor, int y_coor)
	    : DS_object(gc, draw, NULL) { x=x_coor; y=y_coor; }
#else
	DS_point() {}
#endif
	~DS_point() {; }

	void	draw();
	int	getx() { return x; }
	int	gety() { return y; } };

class DS_text_base : public DS_object {
#ifdef GUI
protected:
				//Font ID
	Font			fontid;
	Cstring			fontname;
	Widget			parentid;
	int			chksetdefont(const Cstring &); //1-SUCCESS
	static XFontStruct*	deffstruct;
	static char*		def_fontname;
	static Font		def_fontid;
	static int		def_fontLoaded;
	static int		single_char_width;
	XFontStruct*		curfstruct;
public:
	DS_text_base(DS_gc & gc, Drawable draw, Widget id_different, DS_draw_graph * obj)
	    : DS_object(gc, draw, obj)  { parentid = id_different; chksetdefont(""); } 
	DS_text_base(const DS_text_base & dst)
		: DS_object(dst), parentid(dst.parentid), fontname(dst.fontname), fontid(dst.fontid)  {}
	~DS_text_base() {
		if (curfstruct != deffstruct)
			XFreeFont(getdisplay(), curfstruct); }
	virtual Cstring		get_id() = 0;
	int			is_selected(int, int);
	void			setparentid(Widget id_diff) { parentid = id_diff; }
	void			changefont(const Cstring & newfont) { fontname = newfont; }
	virtual const Cstring	&getText() const = 0;
	virtual void		settext(const Cstring & newin) = 0;
	virtual void		draw();
				// for drawing a highlighted activity:
	virtual void		draw(int);
	              
	void			setfont(Font newfid);
				// Find out the width in pixel of the string under the current font
	int			strwidth(const char* instr) {
					// there used to be a '+ 1', which seems fishy
					// (the string only has strlen() chars)
					return XTextWidth(curfstruct, instr, strlen(instr)) + single_char_width; }
				//Find out the font descent in pixel of the string under the current font
	int			strdescent() {
					return curfstruct->descent; }
	int			strascent()  { return curfstruct->ascent; }
	virtual int		width() { return strwidth(*getText()); }
	int			height() { return strdescent(); }
	int			totalheight() { return strdescent()+strascent(); }
	static void		classinit() { 
					// Cstring undefined;
					// def_fontname=strcpy((char*)(malloc(strlen(DEFTEXTFONT)+1)),DEFTEXTFONT); 
					def_fontLoaded=0; }
#else
	// dummy class:
public:
	DS_text_base() {}
	~DS_text_base() {}
#endif
};
	// Used to draw time values in time rulers
class DS_text : public DS_text_base {
public:
	// constructor 1
	DS_text(
#		ifdef GUI
		DS_gc & gc, Drawable draw, Widget id_different, DS_draw_graph *obj
#		endif
		)
	    : DS_text_base(
#		ifdef GUI
		gc, draw, id_different, obj
#		endif
			)  {} 
	// constructor 2
	DS_text(const DS_text & dst)
		: DS_text_base(
#		ifdef GUI
			dst
#		endif
		), tdata(dst.tdata) {}
	// destructor
	~DS_text() {}
				// the text to draw
	Cstring			tdata;
				// Hmmm... may be used when validating.
	Cstring			get_id() { return Cstring("DS_text = ") + tdata; }
				// data is public, but who cares?
	void			settext(const Cstring & newin) { tdata = newin; }
				// data is public, but who cares?
	const Cstring &		getText() const { return tdata; }
	};

	/** Used to draw activity intances. The 'smart' qualifier refers to
	* the fact that (1) parent instance is consulted through a pointer;
	* (2) text display will vary depending on whether the user wants
	* to see name, type or ID.
	*/
class DS_smart_text : public DS_text_base {
				// Points to the instance
	Dsource*		theSourceObject;
				// Actual text may be a truncated version of the real text.
	Cstring			theActualText;
public:
	// constructor
#	ifdef GUI
	DS_smart_text(Dsource* ds, DS_gc & gc, Drawable draw, Widget id_different, DS_draw_graph *obj)
		: DS_text_base(gc, draw, id_different, obj),
		theSourceObject(ds) {
		settext(getText()); }
#	else
	DS_smart_text() {}
#	endif
	// destructor
	~DS_smart_text() {}
				// Hmm... wonder if this is ever called in a non-trivial context?
	virtual void		setsource(Dsource* ds) { theSourceObject = ds; }
				// for telling theWorkProc that we need to update the display.
	int			is_out_of_sync();
				// draws.
	void			draw();
				// draws.
	void			draw(int);
				// for validating, I think.
	Cstring			get_id() { return Cstring("DS_text = ") + getText(); }
				//
	const Cstring &		getText() const;
				//
	void			settext(const Cstring &t) { theActualText = t; }
				//
	void			update_text_if_necessary();
				// Flag used to figure out what the user wants to see on top of instances.
	static int		showTheActivityName; };

// used to mean that a legend is not visible:
// #define MINUS_INFINITY -16000

class DS_line : public DS_object {
protected:
			// line start and end coordinates (in pixels),plus clipped start and end
			//  (as int's, these DO NOT have enough precision to support scaling!!!)
	int		start_x;
	int		end_x;
	int		y_pos();
	virtual int	y_start() { return y_pos(); }
	virtual int	y_end() { return y_pos(); }
			//line width in pixels
	int		width;
			//label text (NULL if unused)
	DS_smart_text	*linename;         

	void		refresh_coordinates();
// We want to restrict access to constructors and destructors, because we need to
// enforce consistency with the list of redraw windows maintained by the WorkProc:
// public:

// THEORY: can make constructors/destructors public because THEY take care of the window nodes.
// protected:
// public:
			// This method itself updates the win_nodes; no need for friends:
	virtual void		newhcoor(int s_x_coor, int e_x_coor);

// needs to call refresh_coordinates() and KNOW WHAT IT IS DOING:
friend bool theWorkProcCplusplus(void *);

friend class DS_line_manager;
friend class DS_line_owner;
friend class derivedDataDS;
friend class derivedDataVio;

	DS_line(	DS_gc		& gc,
			Drawable	draw,
			int		s_x_coor,
	        	int		e_x_coor,
			int		W,
			Dsource* source,
			DS_draw_graph	*obj);
	DS_line(	DS_gc		& gc,
			Drawable	draw,
			int		s_x_coor,
	        	int		e_x_coor
			);
	DS_line(const DS_line & dsl);
	~DS_line();

	Node		*copy() { return new DS_line(*this); }
public:
	void		draw();
	int		is_visible();
	int		is_selected(int x, int y);
	Cstring		get_linename();
	int		is_out_of_sync();
			// CHANGED PFM 1/27/99
	void		getposition(int& sx,int& sy,int& ex,int& ey) 
				{ sx = start_x; sy = y_pos(); ex = end_x; ey = sy; }
protected:
	int		get_vertical_extents_of_container(int & top, int & bottom);
			// For drawing hierarchies of chameleons.
	void		recursively_draw_descendants(Dsource *, ACT_sys *);
	Cstring		get_id();
			//timeloc is used by DS_timemark and CON_sys, but not (yet) by ACT_sys
			//  (has enough precision to support scaling operations properly)
	CTime		timeloc;
	int		getstartx() { return start_x; }
	int		getendx() { return end_x; }
	void		settitle(Dsource * /* const Cstring & titlestring, DS_gc * gc */);
			// Used to store offset for disambiguation.
	int		theStaggeringOffset;
			// The coordinate at which line is actually drawn.
	int		the_y_coord;
};

class DS_line_for_constraints: public DS_line {
public:
	DS_line_for_constraints(	DS_gc		& gc,
			Drawable	D,
			int		s_x_coor,
	        	int		e_x_coor,
			int		W,
			Dsource*	source,
			DS_draw_graph*	obj) 
		: DS_line(gc, D, s_x_coor, e_x_coor, W, source, obj) {
			timeloc = source->getetime();
	}

			//
			// special:
			//
	void		newhcoor(int s_x_coor, int e_x_coor);
	};

class DS_simple_line : public DS_object {
#ifdef GUI
protected:
			//
			// line start and end coordinates (in pixels),plus clipped start and end
			//  (as int's, these DO NOT have enough precision to support scaling!!!)
			//
	int		start_x;
	int		end_x;
	int		start_y;
	int		end_y;

			//
			// line width in pixels
			//
	int		width;
public:
	DS_simple_line(	DS_gc		& gc,
			Drawable	D,
			int		s_x_coor,
			int		s_y_coor,
	        	int		e_x_coor,
			int		e_y_coor,
			int		W)
		: DS_object(gc, D, NULL, NULL),
		start_x(s_x_coor),
		start_y(s_y_coor),
		end_x(e_x_coor),
		end_y(e_y_coor),
		width(W)
		{}
	DS_simple_line(	DS_gc		& gc,
			Drawable	D,
			int		s_x_coor, int		s_y_coor,
	        	int		e_x_coor, int	e_y_coor
			)
		: DS_object(gc, D, NULL, NULL),
		start_x(s_x_coor),
		start_y(s_y_coor),
		end_x(e_x_coor),
		end_y(e_y_coor),
		width(LNDRAWWIDTH)
		{}
	~DS_simple_line();
	Cstring		get_linename() { return Cstring(); }
	Cstring		get_id() { return Cstring("Simple Line..."); }

	void		draw();

			// the busy_line is a 1-pixel-wide grey line that is drawn just under
			// an activity wherever it overlaps with another activity:
	// void		draw(int whether_to_update_busy_line);
	void		hmove(int rx);
	int		is_selected(int x, int y);
	int		is_visible() { return 1; }
	void		setwidth(int swidth) { width = swidth; }
	void		resize(float, float);
	void		vset(int vy) { start_y = vy; end_y = vy; }
	void		newcoor(int s_x_coor, int s_y_coor, int e_x_coor, int e_y_coor)
				{ start_x = s_x_coor; start_y = s_y_coor;
				end_x = e_x_coor; end_y = e_y_coor; }
	void		newhcoor(int s_x_coor, int e_x_coor)
	    			{ start_x = s_x_coor; end_x = e_x_coor; }
	void		getposition(int& sx,int& sy,int& ex,int& ey) 
				{ sx = start_x; sy = start_y; ex = end_x; ey = end_y; }
			//timeloc is used by DS_timemark and CON_sys, but not (yet) by ACT_sys
			//  (has enough precision to support scaling operations properly)
	CTime		timeloc;
	int		getstartx() { return start_x; }
	void		settitle(const Cstring & titlestring, DS_gc * gc);
			// clears the area formerly occupied by the DS_line:
	// void		cleardisplay();
	Node*		copy();
#else
	// dummy class:
public:
	DS_simple_line() {}
	~DS_simple_line() {}
#endif
};

class DS_rectangle : public DS_object
	{
#ifdef GUI
protected:
	int		start_x;
	int		start_y;
	int		width;
	int		height;
public:
	DS_rectangle(DS_gc& gc, Drawable draw, int s_x_coor, int s_y_coor, int swidth, int sheight)
		: DS_object(gc, draw, NULL),
		start_x(s_x_coor),
		start_y(s_y_coor),
		width(swidth),
		height(sheight)
		{}
	~DS_rectangle() {; }

	void		draw();
	void		draw(DS_gc& newgc) { switchGC(newgc); draw(); }    
#else
	// dummy class:
public:
	DS_rectangle() {}
	~DS_rectangle() {}
#endif
	};

class DS_pixmap : public DS_object
	{
#ifdef GUI
protected:
	int depth;
	BITMAPDEF* bitmapdata;
	int x,y;
	int clearflg;
	Pixmap bimage;
public:
	DS_pixmap(DS_gc& gc,Drawable draw,BITMAPDEF* bdata,int depval);
	~DS_pixmap() { cleardraw(); }

	Cstring		get_id() { return "a Pixmap..."; }
	void		draw();
	Pixmap		getmap();
	void		redraw();
	void		cleardraw() { if (clearflg){ XFreePixmap(getdisplay(),bimage); 
	                                  clearflg=0;} }
	void		setloc(int x_cor,int y_cor) { x=x_cor; y=y_cor; }
#else
	// dummy class:
public:
	DS_pixmap() {}
	~DS_pixmap() {}
#endif
	};

/*	This class represents a drawarea with collection of bitmaps to chose from.
	ABSTRACT:
	This class when constructed, reads an array of BITMAP data and create
	a List of DS_pixmap objects with the bitmap(pixmap of depth 1)). Then 
	it creates a pixmap that can contain all the bitmap(pixmap of depth 1).
	When display() is called, it create a pop-up display if it is not already
	created, or it just pops up the display.
	Undisplay() is just pops the display down.
	*/

#define PATTERNGAP 6

class DS_pixmap_board {
#ifdef GUI
protected:
	Widget		Parentid;
	Widget		Bboard;
	Widget		Bmap;
			//This is for EXPOSE and RESIZE
	Pixmap		Storing_under;
			//List of DS_pixmap
	List		Patterns;
			//Number of bitmaps of one column of the display square
	int		Dimension;
	int		Npattern;
			//Normal display characteristics
	DS_gc*		Normal_GC;
			//Highlite display characteristics
	DS_gc*		Highlite_GC;
			//Color palette List of Pointer_nodes points to DS_gc objects
	List		Colorpalette;
			//board size
	int		Boardwidth,Boardheight;
			//Max size of bitmap patterns
	int		Maxwidth,Maxheight;
			//bitmap selected
	int		Selected;
			//Color selected
	int		Colorid;
			//display created flag
	int		DScreated;
			//Display,Screen,Depth
	Display		* Defdisplay;
	int		Defscreenum;
	int		Defdepth;
	Colormap	Defcolormap;
			//The current display id
	DS_id		did;
public:
	DS_pixmap_board(Widget parent,BITMAPDEF* defsource,int npattern);
	~DS_pixmap_board();
	void		selcolor(int sid);
	void		selpattern(int pid) { Selected=pid; } 
	Pixmap		getselpattern(); 
	int		getmaxwidth() { return Maxwidth; }
	int		getmaxheight() { return Maxheight; }
	DS_gc		*gethilite() { return Highlite_GC; }
	int		getselectid() { return Selected; }
	DS_id&		getdid() { did.patternid=Selected; did.colorid=Colorid;
	                   return(did); }       
#else
	// dummy class:
public:
	
	DS_pixmap_board() {}
	~DS_pixmap_board() {}
#endif
	};

class rsource_display_ext: public rsource_ext {
public:
	rsource_display_ext() : resolution(0,0,true) {}
	~rsource_display_ext() {}
	void notify_clients(void *);
	CTime_base& get_resolution() {
		return resolution;
	}
	CTime_base resolution;
};

#endif
