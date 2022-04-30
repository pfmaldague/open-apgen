#if HAVE_CONFIG_H
#include <config.h>
#endif
// #define apDEBUG
#include "apDEBUG.H"

#include "ActivityInstance.H" /*DSG 96-07-11 for Dsource */
#include "ACT_sys.H"
#include "UI_dsconfig.h"
#include "UI_ds_draw.H"
#include "UI_ds_timeline.H"
#include "UI_GeneralMotifInclude.H"
#include "UI_mainwindow.H"		// for refresh_info
#include "apcoreWaiter.H"

using namespace std;

#ifdef GUI
int		DS_text_base::def_fontLoaded;
Font		DS_text_base::def_fontid;
XFontStruct	*DS_text_base::deffstruct;
char		*DS_text_base::def_fontname;
int		DS_text_base::single_char_width = 0;

DS_gc		*DS_object::hilitegc = NULL;
DS_gc		*DS_object::short_act_gc = NULL;
DS_gc		*DS_object::busy_gc = NULL;
DS_gc		*DS_object::separation_gc = NULL;
DS_gc		*DS_object::grey_gc = NULL;
Display		*DS_gc::dp = NULL;
#endif

int		DS_smart_text::showTheActivityName = 1;

#define SHORTEST_VISIBLE_ACTIVITY 3

extern RGBvaluedef  Patterncolors[];
#ifdef GUI
extern BITMAPDEF AP_Bitmap_pool[];
#endif

				// in UI_motif_widget.C:
extern resource_data		ResourceData;

				// in main.C:
extern refresh_info		refreshInfo;

static int int_sqrt(int n) {
	int i;
	for (i=0;i<=n;i++)
	   if (i*i>n)
		 return i;  
	return 0; }

#ifdef GUI	
DS_gc::DS_gc(Display* display,Drawable drawable,unsigned long valuemask,
	     XGCValues *value)
	: id(0), cid(0) {
	if(GUI_Flag) {
	DBG_NOINDENT("    DS_gc(\"" << get_key() << "\")::DS_gc\n"; cout.flush());
	if(! drawable) {
		cerr << "GC PANIC!!\n";
		exit(- 1); }
	gptr=new gp;
	gptr->gc = XCreateGC(display,drawable,valuemask,value);
	if(! dp) dp = display;
	gptr->ct=1; 
	}
}

DS_gc::DS_gc(const DS_gc& org)
	: id(org.id),
	cid(org.cid) {
	if(GUI_Flag) {
	org.gptr->ct++;
	gptr=org.gptr; } }

void DS_gc::adopt_new_gc(const DS_gc & new_gc) {
	if(GUI_Flag) {
	gptr->ct--;
	if (!gptr->ct) {
		XFreeGC(dp, gptr->gc);
		delete gptr; }
	new_gc.gptr->ct++;
	gptr = new_gc.gptr; 
	id = new_gc.id;
	cid = new_gc.cid; }
}

//Destructor only destructs the GC
DS_gc::~DS_gc() {
	if(GUI_Flag) {
	gptr->ct--;
	if (!gptr->ct) {
	 XFreeGC(dp,gptr->gc);
	 delete gptr; } 
	}
}

GC DS_gc::getGC() {
	if(GUI_Flag) {
		return gptr->gc;
	}
	return GC();
}


void DS_gc::switchGC(DS_gc& gc) {
	if(GUI_Flag) {
	gptr->ct--;
	if (!gptr->ct) {
		XFreeGC(dp,gptr->gc);
		delete gptr; }    
	gptr=gc.gptr;
	gptr->ct++; 
	id = gc.id;
	cid = gc.cid;
	}
}

DS_object::DS_object(DS_gc & gc, Drawable draw, DS_draw_graph *obj)
	: DS_gc(gc),
	DS_draw(draw), sourceobj(NULL) {
	if(GUI_Flag) {
	graphobj = obj; 
	}
}

DS_object::DS_object(DS_gc & gc, Drawable draw, Dsource * s, DS_draw_graph * g)
	: DS_gc(gc),
	DS_draw(draw), sourceobj(s), graphobj(g) {}

int DS_object::getselection() {
	if(GUI_Flag) {
	if(sourceobj && sourceobj->is_selected())
		return 1;
	else
		return /* selected */ 0; 
	}
	return 0;
}

DS_object::~DS_object() {}

DS_gc *DS_object::gethilite() {
	//if(GUI_Flag) {
	if(! hilitegc) {
					// in UI_motif_widget.C:
		extern resource_data	ResourceData;
		XGCValues		gcvalue;

        	//create hilite GC
                gcvalue.line_width = 1;
                gcvalue.fill_style = FillSolid;
                gcvalue.foreground = ResourceData.black_color;
                // gcvalue.foreground = BlackPixel(
                //              XtDisplay(widget),
                //              XDefaultScreen(XtDisplay(widget)));
                hilitegc = new DS_gc(	DS_graph::Defdisplay,
					DefaultRootWindow(DS_graph::Defdisplay),
					GCForeground |
						GCLineWidth |
						GCFillStyle,
					&gcvalue); }
	return hilitegc; 
	//}
}
#endif


int DS_line::y_pos() {
#ifdef GUI
	if(GUI_Flag) {
	int		y_top, y_bottom;

	/* get_vertical_extents_of_container() returns the ...
	 */
	if(get_vertical_extents_of_container(y_top, y_bottom)) {
		static int	theOffset = 5; // (int) (0.2 * ACTVERUNIT);
		int             roomAvailableForStaggering = y_bottom - y_top - ACTSQUISHED;
		int		theAmount;

		theAmount = y_bottom - theOffset;
		if(	(roomAvailableForStaggering > 0)
			&& theStaggeringOffset
		 ) {
			static int	theStaggeringIncrement = 5;

			if(roomAvailableForStaggering > 50) {
				// Dont'a work (?? -- until now...):
				theStaggeringIncrement = roomAvailableForStaggering / (graphobj->theMaximumStaggeringOffset + 1);
				// theStaggeringIncrement = roomAvailableForStaggering / 10;
				if(theStaggeringIncrement > 26)
					theStaggeringIncrement = 26; }
			theAmount -= (theStaggeringOffset * theStaggeringIncrement) % roomAvailableForStaggering; }
		return theAmount; }
	}
#endif
	return -20; }

int DS_line::is_selected(int x, int y) {
#ifdef GUI
	if(GUI_Flag) {
	int	adjusted_end = (end_x < start_x + SHORTEST_VISIBLE_ACTIVITY) ?
					start_x + SHORTEST_VISIBLE_ACTIVITY :
					end_x;
	int	y_coord = y_pos();

	//check if the incoming pixel is within the line
	if(	(x >= start_x && x <= adjusted_end)
		&&
	 	(
			y >= y_coord - (width >> 1) &&
			y <= y_coord + (width >> 1)
		)
	 ) {
		DBG_NOINDENT("DS_line(" << get_linename() << ")::is_selected == 1\n");
		return 1; }
	// maybe we clicked in the text area:
	if (	is_visible()
		&& linename // if this != NULL we are dealing with an ActivityInstance
		&& sourceobj
		&& (sourceobj->get_legend()->get_official_height() > ACTFLATTENED)
		&& linename->is_selected(x, y)) {
		DBG_NOINDENT("DS_line(" << get_linename() << ")::is_selected == 1\n");
		return 1; } }
#endif
	return 0; }

int DS_line::is_out_of_sync() {
	DS_gc		*the_tilepattern;
	if(linename && linename->is_out_of_sync()) {
		DBG_NOINDENT("DS line for " << sourceobj->identify()
				<< " is out of sync because the label is wrong.\n");
		return 1;
	}
	return 0;
}

		// in IO_seqtalk:
extern int 	UpdateBusyLine;

void DS_line::recursively_draw_descendants(Dsource *who, ACT_sys *onWhat) {
#ifdef GUI
	slist<alpha_void, smart_actptr>::iterator	the_kids_0(who->get_down_iterator());
	tlist<timeptr<ActivityInstance>, smart_actref, ActivityInstance*>	theSubactivities(true);
	slist<timeptr<ActivityInstance>, smart_actref, ActivityInstance*>::iterator the_kids(theSubactivities);
	smart_actptr*					ptr;
	ActivityInstance*				kid;
	smart_actref*					tptr;
	int						w = onWhat->getwidth();
	DS_gc*						the_tilepattern;

	while((ptr = the_kids_0())) {
		kid = ptr->BP;
		theSubactivities << new smart_actref(kid);
	}
	while((tptr = the_kids())) {
		kid = tptr->getKey().mC;
		if((the_tilepattern = ((derivedDataDS *) kid->dataForDerivedClasses)->get_tilepattern(onWhat))) {
			int			ks, ke;
			GC			theGC;

			((derivedDataDS *) kid->dataForDerivedClasses)->compute_start_and_end_times_in_pixels(onWhat, ks, ke);
			if (ks < 0)
			    ks = - 2;
			if (ke > w)
			    ke = w + 50;
			theGC = the_tilepattern->getGC();
			/*
			 * Should draw cute little line; will do that later...
			 */
			XDrawLine(getdisplay(), drawon, theGC, ks, the_y_coord, ke, the_y_coord);
			XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
				ks,	the_y_coord - (width >> 1),
				ks,	the_y_coord + (width >> 1) - 1);
		}
	}
#endif /* ifdef GUI */
}

		// if UpdateBusyLine is set, ALL we do is update the busy line parameters;
		// a later call to draw() with UpdateBusyLine unset is required to draw the line.
void DS_line::draw() {
#ifdef GUI
    if(GUI_Flag) {
	int		w;
	int		clipped_start_x = start_x;
	int		clipped_end_x = end_x;
	DS_graph*	graphsource = (DS_graph *) graphobj;

	// This needs to be done even if the line is NOT visible!!
	if(UpdateBusyLine) {
		if(sourceobj && linename) {
			// modify start, end so as to differentiate between adjacent activities:
			theStaggeringOffset = graphsource->update_busy_line(
				sourceobj,
				clipped_start_x - 1,
				clipped_end_x + 1,
				clipped_start_x + linename->width());
		}
		return;
	}

	graphobj->remove_pointer_to_line(this);

	if(!is_visible()) {
		// Even so, let's update the name if necessary, because otherwise we'll get lots
		// of spurious redraw requests...
		if(linename) {
			linename->update_text_if_necessary();
		}
		if(refresh_info::ON && sourceobj) {
			int		y_top, y_bottom;

			get_vertical_extents_of_container(y_top, y_bottom);
			refreshInfo.add_level_2_trigger(sourceobj->get_key(), Cstring("DS_line not visible; y_pos = ") + Cstring(y_pos())
					+ " while extents = " + Cstring(y_top) + ", " + Cstring(y_bottom));
		}
		return;
	}

	if(graphsource) {
		w = graphsource->getwidth();
		/*
		 * Prevent length overflow. Note that we compute this even in the case of an ACT_composite
		 * because we need to erase the previous garbage.
		 */
		if(start_x < 0)
		    clipped_start_x = - 2;
		if(end_x > w)
		    clipped_end_x = w + ((end_x - w) > 50 ? 50 : end_x - w);
	}
	/*
	 * This is really a test to see whether we are drawing an activity (linename != NULL)
	 * or a 'simple' line (e. g. a rule violation; can't think of anything else).
	 */
	if(linename) {
		int	add_extralines_please = 1;
			/* In the case of an ACT_composite, getGC should return the grey GC. */
		GC	theGC = getGC();

		the_y_coord = y_pos();
		if(clipped_end_x < start_x + SHORTEST_VISIBLE_ACTIVITY) {
			theGC = short_act_gc->getGC();
			clipped_end_x = clipped_start_x + SHORTEST_VISIBLE_ACTIVITY;
		}

		if(refresh_info::ON && sourceobj) {
			int		y_top = 0, y_bottom = 0;

			get_vertical_extents_of_container(y_top, y_bottom);
			refreshInfo.add_level_2_trigger(sourceobj->get_key(), Cstring("drawing DS_line on ") + graphsource->get_key()
					+ "; y_pos = " + Cstring(y_pos())
					+ ", extents = " + Cstring(y_top) + ", " + Cstring(y_bottom)
					+ ", start " + Cstring(clipped_start_x) + ", span " + Cstring(clipped_end_x - clipped_start_x));
		}

		// if the sourceobj IS selected then we don't need to erase anything because
		// the linename takes up more space...
		if(sourceobj && !sourceobj->is_selected()) {
			// clean up possible highlight line:
			XClearArea(getdisplay(), drawon,
				clipped_start_x, the_y_coord - (width >> 1) - 1,
				clipped_end_x - clipped_start_x, 1, FALSE);
		}

		// DRAW TEXT FIRST because it erases part of the DS_line:
		if(sourceobj->get_legend()->get_official_height() > ACTFLATTENED) {
			// height() returns text DESCENT only; width is defined as
			// LNDRAWWIDTH = LINEWIDTH = 10 pixels:
			linename->setto(clipped_start_x + 2, the_y_coord - (linename->height() + (width >> 1)) - 3);

			if(sourceobj && sourceobj->is_selected()) {
				linename->draw(1);
			} else {
				linename->draw();
			}
		} else {
			add_extralines_please = 0;
		}
		XDrawLine(getdisplay(), drawon, theGC, clipped_start_x, the_y_coord, clipped_end_x, the_y_coord);
		if(sourceobj) {
		    if(sourceobj->is_selected()) {
			//draw a high lite line on the top
			if(gethilite()->getGC()) {
				XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
					clipped_start_x, 	the_y_coord - (width >> 1) - 1,
					clipped_end_x - 1,	the_y_coord - (width >> 1) - 1);

				XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
					clipped_start_x,	the_y_coord + (width >> 1) - 1,
					clipped_end_x - 1,	the_y_coord + (width >> 1) - 1);
			  
				XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
					clipped_start_x,	the_y_coord - (width >> 1) - 1,
					clipped_start_x,	the_y_coord + (width >> 1) - 1);
			  
				XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
					clipped_end_x - 1,	the_y_coord - (width >> 1) - 1,
					clipped_end_x - 1,	the_y_coord + (width >> 1)); }
#			ifdef 	apDEBUG
			else {
				DBG_NOINDENT("DS_line::draw ERROR; null highlight GC; obj @ " << (void *) this << "\n"); }
#			endif
		    } else if(add_extralines_please) {
				int	middle_y = the_y_coord - (width >> 1) - (linename->totalheight() >> 1) - 1;

				XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
					clipped_start_x,	middle_y,
					clipped_start_x,	the_y_coord + (width >> 1));
				XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
					clipped_start_x,	middle_y,
					clipped_start_x + 2,	middle_y);
		    }
		}
		graphobj->add_vertical_cursor_if_necessary(
			clipped_start_x, the_y_coord - (width >> 1),	// upper left
			clipped_end_x, the_y_coord + (width >> 1));	// lower right
	} else {	// simpler types of lines (ticmarks etc.) will use this:
		XDrawLine(getdisplay(), drawon, getGC(), clipped_start_x,
			y_start(), clipped_end_x, y_end());
	}
    }
#endif
}


void DS_line::settitle(Dsource * theSource) {
	if (linename)
		linename->setsource(theSource);
	else {
		linename = new DS_smart_text(theSource, *gethilite(), drawon, 0, graphobj); }
	linename->setto(start_x, y_pos() - (linename->height() + (width >> 1)) - 1); }

DS_simple_line::~DS_simple_line()  {
}

void DS_simple_line::draw() {
	if(GUI_Flag) {
	int		w;
	int		clipped_start_x = start_x;
	int		clipped_end_x = end_x;
	DS_graph	*graphsource = (DS_graph *) graphobj;

	if(! is_visible()) return;
	if(graphsource) {
		w = graphsource->getwidth();
		// h = graphsource->getheight();
		//Prevent length overflow
		if (start_x < 0)
		    clipped_start_x = - 2;
		if (end_x > w)
		    clipped_end_x = w + ((end_x - w) > 50 ? 50 : end_x - w); }
	XDrawLine(getdisplay(), drawon, getGC(), clipped_start_x,
		start_y, clipped_end_x, end_y); 
	}
}

void DS_simple_line::hmove(int rdistance) {
	if(GUI_Flag) {
	DBG_INDENT("DS_simple_line::hmove(dist = " << rdistance <<
		"): START (note: will not draw; only updating start_x, end_x...)\n");

	// FIXES PART OF FR 1.1-8:
	// cleardisplay();

	//redraw the new line
	start_x+=rdistance;
	end_x+=rdistance;

	// FIXES PART OF FR 1.1-8:
	// draw();

	DBG_UNINDENT("DS_simple_line::hmove: END\n"); 
	}
}

void DS_simple_line::resize(float xratio,float yratio) {
	if(GUI_Flag) {
	start_x=int((float)start_x*xratio);
	end_x=int((float)end_x*xratio);
	start_y=int((float)start_y*yratio);
	end_y=int((float)end_y*yratio); 
	}
}

void DS_simple_line::settitle(const Cstring & titlestring, DS_gc * /* gc */) {
	; }

Node* DS_simple_line::copy() {
	//if(GUI_Flag) {
	return new DS_simple_line(* ((DS_gc *) this), drawon, start_x,
		       start_y, end_x, end_y, width); 
	//}
}

int DS_simple_line::is_selected(int x, int y) {
	if(GUI_Flag) {
	int	adjusted_end = (end_x < start_x + SHORTEST_VISIBLE_ACTIVITY) ?
					start_x + SHORTEST_VISIBLE_ACTIVITY :
					end_x;

	//check if the incoming pixel is within the line
	if(	(x >= start_x && x <= adjusted_end) &&
	 	(
			y >= start_y - (width >> 1) &&
			y <= end_y + (width >> 1)
		)
	 ) {
		return 1; }
	// maybe we clicked in the text area:
	if (is_visible())
		return 1; }
	return 0; }

//Always return SUCCESS now
int DS_text_base::chksetdefont(const Cstring &) {
	if(GUI_Flag) {
	int ret_val=1;

	if (!def_fontLoaded) {   
		deffstruct = ResourceData.time_and_act_font;
		def_fontid = deffstruct->fid;
		single_char_width = XTextWidth(deffstruct, "g", 1);
		// def_fontid=XLoadFont(getdisplay(),def_fontname);
		// deffstruct=XQueryFont(getdisplay(),def_fontid);
		def_fontLoaded = 1; }
	fontid = def_fontid;
	setfont(fontid);	
	curfstruct = deffstruct;
	return ret_val; 
	}
return 1; }


//Set up the Font
void DS_text_base::setfont(Font newid) {
	if(GUI_Flag) {
	XGCValues values;
	
	values.font=newid;
	changeGC(GCFont,&values); } }

DS_pixmap::DS_pixmap(DS_gc& gc,Drawable draw,BITMAPDEF* bdata,int depval)
	: DS_object(gc, draw, NULL) { 
	if(GUI_Flag) {
	depth=depval;
	bitmapdata=bdata;
	clearflg=0; } }

//Turns the bitmap data into pixmap of depth 1 and draw
void DS_pixmap::draw() {
	if(GUI_Flag) {
	clearflg=1;
	bimage=XCreateBitmapFromData(getdisplay(),drawon,bitmapdata->bitmapdata,
				    bitmapdata->width,bitmapdata->height);
	XCopyPlane(getdisplay(),bimage,drawon,getGC(),0,0,bitmapdata->width,
	       bitmapdata->height,x,y,1); } }


//Return the selected Pixmap
Pixmap DS_pixmap::getmap() {
	if(GUI_Flag) {
	if (clearflg != 0) {
		return bimage; } }
	return (Pixmap) 0; }



DS_pixmap_board::DS_pixmap_board(
	Widget parent,
	BITMAPDEF *defsource,
	int npattern) {
	if(GUI_Flag) {
	int		nitems;
	int		ncolors;
	int		nhorizontal=0;
	int		nvertical=0;
	XGCValues	boardGCval;
	DS_pixmap	*cpixnode;
	DS_simple_line		*lnode;
	XColor		exact_def;
	DS_gc		*gcpointer;

	Parentid = parent;
	Selected = 1;
	Npattern = npattern;
	Maxwidth = 0;
	Maxheight = 0;
	DScreated = 0;
	Defdisplay = XtDisplay(Parentid);
	Defscreenum = XDefaultScreen(Defdisplay);
	Defdepth = XDefaultDepth(Defdisplay, Defscreenum); 
	  

	//step1: find how many elements per one column

	Dimension = int_sqrt(npattern);
	
	//step2: find out the max width and max height of item
	//       and also create the pattern list

	for (nitems=0;nitems<npattern;nitems++)
		{
		if (defsource[nitems].width>Maxwidth)
		    Maxwidth=defsource[nitems].width;
		if (defsource[nitems].height>Maxheight)
		    Maxheight=defsource[nitems].height;	
		}

	//size of the drawarea
	Boardwidth=Maxwidth*Dimension+PATTERNGAP*(Dimension-1);
	Boardheight=Maxheight*Dimension+PATTERNGAP*(Dimension-1);
	
	//step3:Create the pixmap board

	Storing_under=XCreatePixmap(Defdisplay,
		DefaultRootWindow(Defdisplay), /*Xlib.h deficiency causes aCC warning*/
		Boardwidth,Boardheight,Defdepth);
				
	//Create the normal display GC
	boardGCval.background=WhitePixel(/*Xlib.h deficiency causes aCC warning*/
					Defdisplay,Defscreenum);
	boardGCval.foreground=WhitePixel(/*Xlib.h deficiency causes aCC warning*/
					Defdisplay,Defscreenum);
	Normal_GC=new DS_gc(Defdisplay,Storing_under,(GCForeground|
			GCBackground),&boardGCval);

	//Clean the board

	XFillRectangle(Defdisplay,Storing_under,Normal_GC->getGC(),
		   0,0,Boardwidth+1,Boardheight+1);
	delete(Normal_GC);
 
	boardGCval.foreground=BlackPixel(/*Xlib.h deficiency causes aCC warning*/
					Defdisplay,Defscreenum);   
	Normal_GC=new DS_gc(Defdisplay,Storing_under,(GCForeground|
			GCBackground),&boardGCval);

	//Draw the grids

	for (nitems=1;nitems<Dimension;nitems++)
		{
		lnode = new DS_simple_line(	*Normal_GC,(Drawable)Storing_under,
				(int) ((Maxwidth + PATTERNGAP) * nitems - .6667 * PATTERNGAP),
				0,
				(int) ((Maxwidth + PATTERNGAP) * nitems - .6667 * PATTERNGAP),
				Boardheight,
				LNDRAWWIDTH);
		lnode->draw();
		delete(lnode);
		lnode = new DS_simple_line(* Normal_GC,
				(Drawable) Storing_under,
				0,
				(int) ((Maxheight + PATTERNGAP) * nitems - .6667 * PATTERNGAP),
				Boardwidth,
				(int) ((Maxheight + PATTERNGAP) * nitems - .6667 * PATTERNGAP),
				LNDRAWWIDTH);
		lnode->draw();
		delete(lnode);   
		}    
	//Create pattern list and draw the patterns into the board
	for (nitems=0;nitems<npattern;nitems++)
		{
		cpixnode=new DS_pixmap(*Normal_GC,(Drawable)Storing_under,
				defsource++,Defdepth);
		Patterns.insert_node(cpixnode);
		//set the draw location
		if (nhorizontal>=Dimension)
			{
			nhorizontal=0;
			nvertical++;
			} 
		cpixnode->setloc((Maxwidth+PATTERNGAP)*nhorizontal,
			 (Maxheight+PATTERNGAP)*nvertical); 
		cpixnode->draw();
		//no need to save the pixmap around
		//cpixnode->cleardraw();
		nhorizontal++;    
		}
	// memory leak:
	delete Normal_GC;
	//Prepare the Highlite color
	Defcolormap = XDefaultColormap(Defdisplay, Defscreenum);
	for(ncolors = 0; ncolors < NUM_OF_COLORS; ncolors++)
		{
		if(XParseColor(Defdisplay,Defcolormap,Patterncolors[ncolors].colorname,
			   &exact_def) &&
		XAllocColor(Defdisplay,Defcolormap,&exact_def))
		boardGCval.foreground=exact_def.pixel;
		//Create the highlite display GC
		gcpointer=new DS_gc(Defdisplay,
			DefaultRootWindow(Defdisplay), /*Xlib.h deficiency causes aCC warning*/
			(GCForeground|GCBackground),&boardGCval);
		// DUMB POINTER (PFM)
		Colorpalette.insert_node(new Pointer_node((void*)gcpointer, NULL));
		if (ncolors==0)
		    //Default value
		    Highlite_GC=gcpointer; } } }

DS_pixmap_board::~DS_pixmap_board() {
	if(GUI_Flag) {
	Pointer_node *	ptr;
	List_iterator	pal(Colorpalette);

	while((ptr = (Pointer_node *) pal.next()))
		delete (DS_gc *) ptr->get_ptr(); } }

void DS_pixmap_board::selcolor(int sid) {
	if(GUI_Flag) {
	Pointer_node* pnode=(Pointer_node*)(Colorpalette[sid-1]);
	DS_gc* pgc=(DS_gc*)(pnode->get_ptr());
	
	Colorid=sid;
	Highlite_GC=pgc; } }

Pixmap DS_pixmap_board::getselpattern() {
	if(GUI_Flag) {
	if (Selected>=0) {
		return ((DS_pixmap*)(Patterns[Selected]))->getmap(); } }
	return ((Pixmap)0); }

void DS_point::draw() {
	XDrawPoint(getdisplay(), drawon, getGC(), x, y); }

void DS_rectangle::draw() {
	if(GUI_Flag) {
	XDrawRectangle(getdisplay(), drawon, getGC(), start_x, start_y, width, height); } }

int DS_text_base::is_selected(int x, int y) {
	if(	(
			x >=  new_cm_x - 1 &&
			x <= new_cm_x + width() - 1
		) &&
		(
			y >= new_cm_y - strascent() &&
			y <= new_cm_y + strdescent()
		)
	 ) {
		return 1; }
	return 0; }

int DS_line::is_visible() {
	int		w, h;
	DS_graph	*graphsource = (DS_graph *) graphobj;

	if(graphsource) {
		w = graphsource->getwidth();
		// FIXES FR 14 FOR GOOD AND THIS TIME I MEAN IT:
		// h = graphsource->getheight();
		h = graphsource->getheight() + ACTVERUNIT;
		if(end_x < 0 || start_x > w || y_start() > h || y_end() < 0) {
			DBG_NOINDENT("DS_line(" << get_linename() << ")::is_visible: "
				<< start_x << ", " << end_x << "\n");
			return 0; } }
	return 1; }

void DS_smart_text::draw() {
	DS_text_base::draw();
	graphobj->add_vertical_cursor_if_necessary(
				new_cm_x - 1,
				new_cm_y - strascent() - 2 * APcloptions::theCmdLineOptions().CMD_DELTA,
				new_cm_x + width() + 2 * APcloptions::theCmdLineOptions().CMD_DELTA - 1,
				new_cm_y - strascent() + totalheight() + 2
				);
	// Reconciles local and 'core' strings:
	settext(getText()); }

void DS_smart_text::draw(int) {
	DS_text_base::draw(1);
	graphobj->add_vertical_cursor_if_necessary(
				new_cm_x - 1,
				new_cm_y - strascent() - 2 * APcloptions::theCmdLineOptions().CMD_DELTA,
				new_cm_x + width() + 2 * APcloptions::theCmdLineOptions().CMD_DELTA - 1,
				new_cm_y - strascent() + totalheight() + 2
				);
	// Reconciles local and 'core' strings:
	settext(getText()); }

void DS_smart_text::update_text_if_necessary() {
	if(getText() != theActualText) {
		settext(getText()); } }

int DS_smart_text::is_out_of_sync() {
	ActivityInstance*	act = dynamic_cast<ActivityInstance*>(theSourceObject);
	assert(act);
	if(showTheActivityName == 1) {
		return theSourceObject->get_key() != theActualText;
	} else if(showTheActivityName == 0) {
		return act->Object->Task.Type.name != theActualText;
	} else if(showTheActivityName == -1) {
		return theSourceObject->get_unique_id() != theActualText;
	}
	return 0;
}

const Cstring & DS_smart_text::getText() const {
	ActivityInstance*	act = dynamic_cast<ActivityInstance*>(theSourceObject);
	assert(act);
	if(showTheActivityName == 1) {
		return theSourceObject->get_key();
	} else if(showTheActivityName == 2) {
		return ((derivedDataDS *) theSourceObject->dataForDerivedClasses)->label_to_use_on_display();
	} else if(showTheActivityName == -1) {
		return theSourceObject->get_unique_id();
	}
	return act->Object->Task.Type.name;
}

void DS_text_base::draw() {
	if(GUI_Flag) {
	XClearArea(	getdisplay(),	drawon,
			new_cm_x - 1,	new_cm_y - strascent() - 2 * APcloptions::theCmdLineOptions().CMD_DELTA,
			width() + 2 * APcloptions::theCmdLineOptions().CMD_DELTA, totalheight() + 2 + 2 * APcloptions::theCmdLineOptions().CMD_DELTA,
			FALSE);
	XDrawString(getdisplay(), drawon, getGC(), new_cm_x + 2 + APcloptions::theCmdLineOptions().CMD_DELTA, new_cm_y - APcloptions::theCmdLineOptions().CMD_DELTA,
		*getText(), getText().length()); 
	}
}

void DS_text_base::draw(int) {
	XClearArea(	getdisplay(),	drawon,
			new_cm_x - 1,	new_cm_y - strascent() - 2 * APcloptions::theCmdLineOptions().CMD_DELTA,
			width() + 2 * APcloptions::theCmdLineOptions().CMD_DELTA, totalheight() + 2 + 2 * APcloptions::theCmdLineOptions().CMD_DELTA,
			FALSE);

/*	(new_cm_x, new_cm_y - strascent())		(mew_cm_x + width() - 2, new_cm_y - strascent())



	(new_cm_x, new_cm_y - strascent())		(mew_cm_x + width() - 2, new_cm_y - strascent()
			+ totalheight() + 2						+ totalheight() + 2) */

	XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
		new_cm_x,					new_cm_y - strascent() - 2 * APcloptions::theCmdLineOptions().CMD_DELTA,
		new_cm_x + width() + 2 * APcloptions::theCmdLineOptions().CMD_DELTA - 2,	new_cm_y - strascent() - 2 * APcloptions::theCmdLineOptions().CMD_DELTA);
	XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
		new_cm_x + width() + 2 * APcloptions::theCmdLineOptions().CMD_DELTA - 2,	new_cm_y - strascent() - 2 * APcloptions::theCmdLineOptions().CMD_DELTA,
		new_cm_x + width() + 2 * APcloptions::theCmdLineOptions().CMD_DELTA - 2,	new_cm_y - strascent() + totalheight() + 1);
	XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
		new_cm_x + width() + 2 * APcloptions::theCmdLineOptions().CMD_DELTA - 1,	new_cm_y - strascent() + totalheight() + 1,
		new_cm_x - 1,					new_cm_y - strascent() + totalheight() + 1);
	XDrawLine(getdisplay(), drawon, gethilite()->getGC(),
		new_cm_x,					new_cm_y - strascent() + totalheight() + 1,
		new_cm_x,					new_cm_y - strascent() - 2 * APcloptions::theCmdLineOptions().CMD_DELTA);
	XDrawString(getdisplay(), drawon, getGC(), new_cm_x + 2 + APcloptions::theCmdLineOptions().CMD_DELTA, new_cm_y - APcloptions::theCmdLineOptions().CMD_DELTA,
			*getText(), strlen(*getText())); 
	}

Cstring DS_line::get_linename() {
	if(linename) return linename->getText();
	return ""; }

Cstring DS_line::get_id() {
	if(linename)
		return get_linename();
	else if(sourceobj) {
		return Cstring("DS_line for ") + sourceobj->get_unique_id(); }
	return Cstring(); }

void	DS_line::newhcoor(int s_x_coor, int e_x_coor) {
	int			old_sx = start_x;
	static int		top_y, bottom_y;
	int			old_ex = start_x + linename->width() + 2 * APcloptions::theCmdLineOptions().CMD_DELTA;

	DBG_INDENT("DS_line(" << get_linename() << ")::newhcoor(" << s_x_coor << ", "
		<< e_x_coor << ") START\n");
	if(old_ex < end_x)
		old_ex = end_x;
	if(old_ex < old_sx + SHORTEST_VISIBLE_ACTIVITY)
		old_ex = old_sx + SHORTEST_VISIBLE_ACTIVITY;
	if(old_ex < s_x_coor + SHORTEST_VISIBLE_ACTIVITY)
		old_ex = s_x_coor + SHORTEST_VISIBLE_ACTIVITY;
	if(old_ex < e_x_coor)
		old_ex = e_x_coor;

	start_x = s_x_coor;
	end_x = e_x_coor;
	// we want the window to cover the whole interval...
	if(start_x < old_sx)
		old_sx = start_x;

	// OBSERVER PATTERN
	if(graphobj && get_vertical_extents_of_container(top_y, bottom_y)) {
		// debug
		// if(graphobj->is_a_giant_window()) {
		// 	cerr << "update_windows from newhcoor\n"; }
		DBG_NOINDENT("calling " << graphobj->get_key() << "->update_windows...\n");
		graphobj->update_windows(	old_sx,
						old_ex,
						top_y,
						bottom_y,
						(Lego *) getsource()->get_legend()); }
	else {
		DBG_NOINDENT("vertical container not visible; not updating windows.\n"); }
	DBG_UNINDENT("DS_line::newhcoor DONE\n"); }

	// Hopper design: if this line has the wrong idea about its type of ACT_sys, it's too late!
int	DS_line::get_vertical_extents_of_container(int &top_y, int & bottom_y) {
	Lego			*theObj;

	// Avoid UMR's:
	top_y = 0;
	bottom_y = 0;
	if(sourceobj && (theObj = (Lego *) sourceobj->get_legend())) {
		DS_lndraw		*l_d = (DS_lndraw *) graphobj;

		int			theLegendSize =
						theObj->get_vertical_size_in_pixels_in(l_d->legend_display);
		list_of_siblings	*theSiblings;

		top_y = l_d->legend_display->vertical_distance_above_item(theObj->vthing);
		bottom_y = top_y + theLegendSize;

		DBG_NOINDENT(sourceobj->get_unique_id() << ": legend \"" << theObj->get_key()
				<< "\" says it's " << top_y << " pixels from the top.\n");

		if(top_y > ((DS_lndraw *) graphobj)->getheight()) {
			DBG_NOINDENT(sourceobj->get_unique_id() << ": no vert. extents (too far down)\n");
			return 0; } }
	else {
		if(refresh_info::ON && sourceobj) {
			refreshInfo.add_level_2_trigger(sourceobj->get_key(),
					" container extents cannot be computed: no legend_object"); }
		// Must be violation stuff; always visible:
		return 1; }

	DBG_NOINDENT(sourceobj->get_unique_id() << ": vert. extents ("
			<< top_y << ", " << bottom_y << ")\n");
	return 1; }

DS_line::DS_line(	DS_gc		& gc,
			Drawable	draw,
			int		s_x_coor,
	        	int		e_x_coor,
			int		W,
			Dsource		*source,
			DS_draw_graph	*obj)
		: DS_object(
#ifdef GUI
			gc, draw, source, obj
#endif
			),
		start_x(s_x_coor),
		end_x(e_x_coor),
		width(W),
		linename(NULL),
		theStaggeringOffset(0) {
		// debug
		// cerr << "DS line constructor: cid is " << cid << endl;
#ifdef GUI
	if(GUI_Flag) {
		if(obj) obj->add_window_for(this);
		// Safe, because a pointer to this can't possibly exist (if
		// it does, we are in so much trouble that it's not even
		// worth contemplating our fate!!):
		if(source) {
				DBG_NOINDENT("    DS_line::DS_line @ " << ((void *) this)
					<< ": creating DS_line with source = "
					<< (source ?
						source->get_unique_id() :
						Cstring("constraint vio."))
					<< endl);
			((derivedDataDS *) source->dataForDerivedClasses)->theListOfPointersToDsLines
				<< new Pointer_node(this, this); }
	}
#endif
}

DS_line::DS_line(	DS_gc		& gc,
			Drawable	draw,
			int		s_x_coor,
	        	int		e_x_coor
			)
		: DS_object(
#ifdef GUI
			gc, draw, NULL, NULL
#endif
			),
		start_x(s_x_coor),
		end_x(e_x_coor),
		width(LNDRAWWIDTH),
		linename(NULL),
		theStaggeringOffset(0) {
		// debug
		// cerr << "DS line constructor: cid is " << cid << endl;
#ifdef GUI
	if(GUI_Flag) {
	DBG_NOINDENT("    DS_line::DS_line: creating DS_line @ "
			<< ((void *) this) << " with NO SOURCE OBJECT\n");
	if(graphobj) graphobj->add_window_for(this);
	}
#endif
}

DS_line::DS_line(const DS_line & dsl)
		: DS_object(dsl),
		start_x(dsl.start_x),
		end_x(dsl.end_x),
		width(LNDRAWWIDTH),
		linename(NULL),
		theStaggeringOffset(0) {
#ifdef GUI
	if(GUI_Flag) {
	if(graphobj) graphobj->add_window_for(this);
	if(dsl.linename) linename = new DS_smart_text(*dsl.linename);
	}
#endif
}

DS_line::~DS_line() {
	int	top_y, bottom_y;

	if(graphobj && get_vertical_extents_of_container(top_y, bottom_y)) {
		if(linename) {
			int			end_including_text = start_x + linename->width()
									+ 2 * APcloptions::theCmdLineOptions().CMD_DELTA;
			int			top_including_text = top_y;
			static LegendObject	*LO;

			if(end_x > end_including_text)
				end_including_text = end_x;
			/* Check because the ACT_composite destructor may be invoked after
			 * all the children have been deleted; in that case the request
			 * won't be able to find its legend. The question is of course whether
			 * we need to clean up the grey areas, which are not claimed by any
			 * children... oh well. */
			if((LO = getsource()->get_legend())) {
				// debug
				// if(graphobj->is_a_giant_window()) {
				// 	cerr << "update_windows from DS_line destructor\n"; }
				graphobj->update_windows(	start_x,
								end_including_text,
								top_y,
								bottom_y,
								(Lego *) LO); }
			delete linename; } }
	else if(linename) {
		delete linename; } }

void DS_line::refresh_coordinates() {
	Dsource	*request = (Dsource *) sourceobj;
	ACT_sys		*actsis = (ACT_sys *) graphobj;
	CTime		T1, T2, T3;
	double		S1;

	if(!request) return;
	if(!graphobj->isVisible()) return;
	start_x = (int) (0.5 + ((T1 = request->getetime()) -
		(T2 = actsis->get_siblings()->dwtimestart())).convert_to_double_use_with_caution()
		/ (S1 = actsis->timeperhpixel()));
	end_x = (int) (0.5 + (T1 + request->get_timespan() -
				T2).convert_to_double_use_with_caution() / S1); }
