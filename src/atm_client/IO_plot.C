#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define apDEBUG
#include "apDEBUG.H"

#include <ActivityInstance.H>
#include "ACT_sys.H"
#include "APcolors.H"
// #include "CON_exec.H"
// #include "Constraint.H"
#include "CON_sys.H"
#include "RD_sys.H"
#include "RES_eval.H"
#include "IO_plot.H"

#include "IO_activitylayout.H"

#include <map>
#include <algorithm>
#include <limits.h>
#include <fstream>

using namespace std;

//EXTERNS
extern const char*			get_apgen_version_build_platform();
					// in ACT_io.C:

IO_plotter::IO_plotter() {}

IO_plotter::~IO_plotter() {}

//local constants (FUTURE move into .H if use in multiple methods); names
//are self-explanatory; units are points (1/72 inch);font size in header;
//want 1/2" margins EXCEPT 7/6" at top (2/3" for hole punch, 1/2" title)
    const double PLOTTER_RESOLUTION_IN_POINTS =	0.05;	//"pixel" is 1/20 point

//WARNING:  life will be easier if ALL of following constants are INTEGERS,
//	    even though they are virtually all expressed as doubles!
double LANDSCAPE_FULL_WIDTH =	10.0*72;	//10.0" on 11.0" page
double LANDSCAPE_PLOT_WIDTH =	LANDSCAPE_FULL_WIDTH;//No reserved area
//  const double LANDSCAPE_PLOT_HEIGHT=	LANDSCAPE_FULL_HEIGHT-7.; //for ID info
//  const double LANDSCAPE_PLOT_HEIGHT=		492.0;	//6+5/6" on  8.5" page
double LANDSCAPE_PLOT_HEIGHT=	492;	// 6 5/6" on  8.5" page
double LANDSCAPE_FULL_HEIGHT=	LANDSCAPE_PLOT_HEIGHT+7.;	//ID in margin

const double TIMESCALE_REGION_HEIGHT =	48;	//2/3"(4 typewtr lines)
const double VIOLATIONS_REGION_HEIGHT = 12;	//1/6"(1 typewtr line)
const double LEGENDS_REGION_WIDTH = 120;	//1+2/3" (8pt: ~24 chr)
const double LANDSCAPE_HEADER_Y_OFFSET = -18;	//ABOVE Y=0 at top line
const double LANDSCAPE_SUBHEAD_Y_OFFSET = -4;	//Subheader nearer line
double ACTIVITY_PLOT_TOTAL_HEIGHT = LANDSCAPE_PLOT_HEIGHT - TIMESCALE_REGION_HEIGHT - VIOLATIONS_REGION_HEIGHT;	// Available for legends
const double ACTIVITY_PLOT_REGION_HEIGHT =	24;	//1/3"(2 typewtr lines)
int    ACTIVITY_PLOT_REGIONS_PAGE = (int) (ACTIVITY_PLOT_TOTAL_HEIGHT / ACTIVITY_PLOT_REGION_HEIGHT);	// Max legends/page

double pageWidth = 8.5 * 72;	// default to letter page
double pageHeight = 11.0 * 72;	// default to letter page
int pageNumber = 1;	// current page number of plot

double leftMargin = 36;	// left margin in points
double topMargin = 36;	// top margin in points

void IO_plotter::setPlotPageDimensions(double width, double height, double left, double top) {
    // make sure dimensions are landscape oriented
    if (width > height) {
	pageWidth = width * 72;
	pageHeight = height * 72;
    }
    else {
	pageWidth = height * 72;
	pageHeight = width * 72;
    }

    //for now set margins for what they are regardless of whether we are printing landscape or not
    leftMargin = left * 72;
    topMargin = top * 72;

    LANDSCAPE_FULL_WIDTH = pageWidth - (2 * leftMargin);
    LANDSCAPE_PLOT_WIDTH = LANDSCAPE_FULL_WIDTH;
    LANDSCAPE_PLOT_HEIGHT = pageHeight - (2 * topMargin) - (18+12);	// 18 for main title, 12 for subtitle
    LANDSCAPE_FULL_HEIGHT = LANDSCAPE_PLOT_HEIGHT + 7;
    ACTIVITY_PLOT_TOTAL_HEIGHT = LANDSCAPE_PLOT_HEIGHT - TIMESCALE_REGION_HEIGHT - VIOLATIONS_REGION_HEIGHT;
    ACTIVITY_PLOT_REGIONS_PAGE = (int) (ACTIVITY_PLOT_TOTAL_HEIGHT / ACTIVITY_PLOT_REGION_HEIGHT);
}

//  const double TIMESCALE_REGION_HEIGHT =	36.;	//1/2"(3 typewtr lines)
//  const double TIMESCALE_LINE_Y_OFFSET =	36.;	//Offset inside region
    const double TIMESCALE_LINE_Y_OFFSET =	48.;	//Offset inside region
    const double TIMESCALE_MAJOR_TICK_HEIGHT =	8.;	//
    const double TIMESCALE_MINOR_TICK_HEIGHT =	4.;	//
//  const double TIMESCALE_DATE_Y_POSITION =	12.;	//First  line date+time
//  const double TIMESCALE_TIME_Y_POSITION =	24.;	//Second line date+time
//  const double TIMESCALE_EXTRA_Y_POSITION =	32.;	//Extra line(ticks use)
//  const double TIMESCALE_ALTDATE_Y_POSITION =	12.;	//Extra line(ticks use)
//  const double TIMESCALE_DATE_Y_POSITION =	24.;	//First  line date+time
//  const double TIMESCALE_TIME_Y_POSITION =	36.;	//Second line date+time
    const double TIMESCALE_SUPDATE_Y_POSITION =	10.;	//First  line date+time
    const double TIMESCALE_ALTDATE_Y_POSITION =	19.;	//Second line date+time
    const double TIMESCALE_DATE_Y_POSITION =	28.;	//Third  line date+time
    const double TIMESCALE_TIME_Y_POSITION =	37.;	//Fourth line date+time
//  const double TIMESCALE_MIN_X_SEPARATION =	60.;	//5/6" (8pt: ~12 chars)
    const double TIMESCALE_MIN_X_SEPARATION =	59.;	//this avoids roundoff
    const double TIMESCALE_DATETIME_X_OFFSET =	-20.;	//Offset from majortick

//  const double TIMECORNER_LINE1_Y_POSITION =	10.;	//Corner box:  1st line
//  const double TIMECORNER_LINE2_Y_POSITION =	20.;	//Corner box:  2nd line
//  const double TIMECORNER_LINE3_Y_POSITION =	34.;	//Corner box:  3rd line
//  const double TIMECORNER_LINE4_Y_POSITION =	44.;	//Corner box:  4th line
    const double TIMECORNER_LINE1_Y_POSITION =	 8.;	//Corner box:  1st line
    const double TIMECORNER_LINE2_Y_POSITION =	17.;	//Corner box:  2nd line
    const double TIMECORNER_LINETO_Y_POSITION =	26.;	//Corner: new "to" line
    const double TIMECORNER_LINE3_Y_POSITION =	35.;	//Corner box:  3rd line
    const double TIMECORNER_LINE4_Y_POSITION =	44.;	//Corner box:  4th line

    const double LEGEND_X_OFFSET =		 2.;	//Move legend right
    const double LEGEND_Y_OFFSET =		 2.;	//Center legend in box

    const double VIOLATIONS_TEXT_Y_POSITION = 	 9.;	//Text position in area
    const double VIOLATIONS_ERROR_HEIGHT = VIOLATIONS_REGION_HEIGHT;	//error mark
    const double VIOLATIONS_WARNING_HEIGHT = 0.5 * VIOLATIONS_REGION_HEIGHT;	//warning mark height

    const double ACTIVITY_BAR_HEIGHT =		 10.;	//<1/2 of plot region
    const double ACTIVITY_NAME_Y_OFFSET =	 -4.;	//Name offset from bar

bool print_header_page = true;

void IO_plotter::showPlotSummary(bool showSummary) {print_header_page = showSummary; }

inline int legendPixelsToPoints(double pixels) {
  // converts vertical height of the legend from pixels to PostScript points.
  // The conversion is done assuming that the default number of pixels in a legend
  // (32) converts to the default number of points on the plot page (24)
  return (int) ((pixels / ACTVERUNIT) * ACTIVITY_PLOT_REGION_HEIGHT);
}

//  const double TIMEZONE_STRING_OFFSET=LEGEND_X_OFFSET+90.;//Upper left corner
    const double TIMEZONE_STRING_OFFSET=LEGEND_X_OFFSET+88.;//Upper left corner

//static local variables
    //alphabet array is 1-indexed, so blank up front; "*" at end for overflow
    static char page_letters[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ*";
    static int page_letters_count = strlen(page_letters) - 2; //ignore blank,*

//APGEN PostScript header "IO_plotter::header[]" moved to IO_plotheader.C

//APGEN PostScript trailer "IO_plotter::trailer[]" moved to IO_plotheader.C


apgen::RETURN_STATUS IO_plotter::generate_plot(
	const Cstring		&theFileName,
	ACT_sys			*selected_activity_system,
	RD_sys			*theOneAndOnlyResSys ,
	List			&list_of_selected_resource_systems,
	TIMEZONE		timezone_button,
	bool			plot_resource_displays,
	bool			plot_all_legends,
	bool			/* send_to_printer */,
	int			pages_per_display_width,
	int			/* page_format_choice */,
	char			*page_header,
	char			*page_subhead ) {
#ifdef GUI
    if (GUI_Flag) {
	CTime			plotstart;			//selected ACT_sys starttime
	CTime			plotspan;			//selected ACT_sys timespan
	int			page;				//current plot-page number (split by time span)
	int			vpage;				//current vertical-page number(legend overflow)
	int			vertical_page_count;		//count of vertically-stacked rows of pages
	int			vert_space_left;		//room on this page for res. plots(full points)
	CTime			pagestart;			//current plot-page starttime
	CTime			pagespan;			//current plot-page timespan
	CTime			pageend;			//current plot-page endtime (use to print time)
	CTime			majors, minors, offset;		//major-interval,minor-interval,major-offset
	int			psline;				//current PS line# (e.g. in header or trailer)
	CTime			sec_per_pixel;			//seconds per PostScript plot unit
	List			list_of_legends;		//use for selected OR all legends
	int			nlegends;			//count of legends to be plotted
	int			nresources;			//count of Resource Displays to be plotted
	List_iterator		res_iterator(list_of_selected_resource_systems);
	Pointer_node		*resnode;			//iterator and current node in RDs to plot
 	MW_object		*theObject;			//RD_sys pointed to by current node
	int			cur_res_height;			//current RD_sys height (pixels on display)
	int	 		adj_res_height;			//current RD_sys height (full points on plot)
	res_plot_info		*res_plots;			//allocated array of plot info, per RD_sys
	int			nplot;				//resource plot counter

	//Get current time (used for "print time" for ALL pages)
	time_t t;
	time(&t);
	Cstring print_time = ctime(&t);

	// setPlotPageDimensions(8.5, 11.0);	// set default values in case caller never did

	//Print input args for debug
	DBG_INDENT("IO_plotter::generate_plot for selected_activity_system:" << endl);
	DBG_NOINDENT("  timezone_button.zone= "
		     << timezone_to_abbrev_string(timezone_button.zone)
		     << "  timezone_button.epoch= " << timezone_button.epoch << endl);
#   ifdef   apDEBUG
	if (*timezone_button.theOrigin > CTime(0, 0, false)) {
	    DBG_NOINDENT("  timezone_button.scale= " << timezone_button.scale
			 << "  timezone_button.origin= " << timezone_button.theOrigin->to_string()
			 << "(UTC)" << endl);
	}
#   endif
	DBG_NOINDENT("  plot_resource_displays= "
		     <<   (plot_resource_displays ? "TRUE" : "FALSE") << endl);
	DBG_NOINDENT("  plot_all_legends= "
		     <<   (plot_all_legends ? "TRUE" : "FALSE") << endl);
	DBG_NOINDENT("  pages_per_display_width= " << pages_per_display_width << endl);
	DBG_NOINDENT("  page_header= " << page_header << endl);
	DBG_NOINDENT("  page_subhead= " << page_subhead << endl);

	//Get and store the start time & time span of the selected activity system
	selected_activity_system->getviewtime(plotstart, plotspan);

	DBG_NOINDENT("IO_plotter::generate_plot got start time, span for sel. ACT_sys:"
		     << endl);
	DBG_NOINDENT("  plotstart= " << plotstart.to_string() << " (UTC) "
		     << " = " << plotstart.to_string(timezone_button)
		     << " (timezone_button)" << endl);
	DBG_NOINDENT("  plotspan = " << plotspan.to_string() << endl);

	//Get and store ALL or SELECTED legends (implemented as SINGLE CALL)
	selected_activity_system->get_all_or_selected_legends(plot_all_legends, list_of_legends);
	nlegends = list_of_legends.get_length();

	DBG_NOINDENT("IO_plotter::generate_plot found number of legends to plot=" << nlegends << endl);

	//Note:  could split plot calculations (here down to just above fout open)
	//       out into a method, for better program organization.

	//Count how many vertically-stacked rows of pages are needed (nominally 1).
	//  At this point, this value only includes the Activity Display.  Also,
	//  compute space left for Resource Display(s), if any, on last page.

	// /* *** DEBUG *** */ showPlotSummary(false); setPlotMargins(1.0, 0.5); setPlotPageDimensions(60.0, 36.0);

	pageNumber = vertical_page_count = 1;
	if (nlegends == 0) {	//zero legends -- may want this to plot ONLY resources
	    vert_space_left = (int)ACTIVITY_PLOT_TOTAL_HEIGHT;	//entire rest of page is leftover
	    DBG_NOINDENT( "No legends; setting vert_space_left to " << vert_space_left << endl );
	}
	else {
	    List_iterator	legendIterator(list_of_legends);
	    LegendObject	*legendNode = 0;
	    int			verticalSpace = 0;

	    // Iterative sum the size of all legends and increment the page count as needed
	    for (int i = 0; i < nlegends; i++) {
		legendNode = (LegendObject *) ((Pointer_node *) legendIterator.next())->get_ptr();
		int nodeSpace = legendPixelsToPoints(legendNode->get_official_height());
		verticalSpace += nodeSpace;
		if (verticalSpace >= ACTIVITY_PLOT_TOTAL_HEIGHT) {
				// we just crossed a page boundary.  Legend entries are not split across pages.
		    verticalSpace = nodeSpace;	// this goes on the next page
		    vertical_page_count++;
		}
	    }
	    DBG_NOINDENT( "After processing " << list_of_legends.get_length() << " legend(s), vertical_page_count = "
			  << vertical_page_count << "; verticalSpace = " << verticalSpace << endl );

	    // leftover is unused legend-regions PLUS any always-leftover remainder
	    // vert_space_left = (int) ((ACTIVITY_PLOT_TOTAL_HEIGHT - verticalSpace) +
	    // 		     (ACTIVITY_PLOT_TOTAL_HEIGHT - (ACTIVITY_PLOT_REGION_HEIGHT * ACTIVITY_PLOT_REGIONS_PAGE;
	    // well... sounds fishy; let's just go with the nominal amount of space left:
	    vert_space_left = (int) ( ACTIVITY_PLOT_TOTAL_HEIGHT - verticalSpace );
	    DBG_NOINDENT( "After subtracting vertical Space from ACTIVITY_PLOT_TOTAL_HEIGHT = " << ACTIVITY_PLOT_TOTAL_HEIGHT << endl );
	    DBG_NOINDENT( "    vert_space_left = " << vert_space_left << endl );
	}
	if (vert_space_left < 0 /*shouldn't happen*/) {
	    DBG_NOINDENT( "vert_space_left = " << vert_space_left << ", < 0... setting to 0 instead\n" );
	    vert_space_left = 0;
	}

	//Figure out which Resource Displays are to be plotted, and their height.
	//  Note that list already contains ONLY RD_sys of RDs that are both
	//  managed and selected,and thus no further winnowing of list is required.
	res_plots = NULL;
	DBG_NOINDENT( "Beginning processing of " << list_of_selected_resource_systems.get_length() << " resource plot(s)...\n" );
	if (plot_resource_displays) {
	    nresources = list_of_selected_resource_systems.get_length();
	    if (nresources) {
		res_plots = new res_plot_info[nresources];
		nplot = 0;
		while ((resnode = (Pointer_node*) res_iterator.next())) {
		    Cstring any_errors; // hopefully there aren't any

		    //Compute height of current RD, proportional to activity
		    //  display regions (i.e. legend height).  This ensures that
		    //  the proportionality chosen by the user on-screen will be
		    //  matched on the plot (with the sole limitation that the RD
		    //  plot cannot exceed a single full page).
		    theObject = (MW_object*) resnode->get_ptr();

		    // make sure undisplayed data is updated for the plot
		    theObject->refresh_all_internal_data(any_errors);

		    // The right way to get the height of an abstract object is to query its height
		    // in a VSI_owner.
		    cur_res_height = (int) theObject->get_vertical_size_in_abstract_units_in(
											     theOneAndOnlyResSys->legend_display );
		    adj_res_height = (int)(cur_res_height * (ACTIVITY_PLOT_REGION_HEIGHT / ACTVERUNIT));
#				ifdef   apDEBUG
		    COUT << "IO_plotter::generate_plot RES_to_plot #="
			 << resnode->get_index()
			 << " has height=" << cur_res_height << " (pixels), i.e.="
			 << adj_res_height << " (full points)" << endl;
		    COUT << "  available on vpage=" << vertical_page_count
			 << " is vert_space_left=" << vert_space_left
			 << " (full points)" << endl;
#				endif
	
				//Handle the aforementioned sole limitation.
		    if (adj_res_height > ACTIVITY_PLOT_TOTAL_HEIGHT)
			adj_res_height = (int)ACTIVITY_PLOT_TOTAL_HEIGHT;
		    if (adj_res_height < 1)
			adj_res_height = 1;	//absolute minimum
	
		    //Now, place it on the current page, or on the next if need be.
		    if (adj_res_height > vert_space_left) {
			vertical_page_count++;
			vert_space_left = (int) ACTIVITY_PLOT_TOTAL_HEIGHT - adj_res_height;
		    }
		    else {	//fits on current page
			vert_space_left -= adj_res_height;
		    }

		    DBG_NOINDENT( "IO_plotter::generate_plot RES_to_plot #="
				  << resnode->get_index() << " has limited height="
				  << adj_res_height << " (full points)" << endl );
		    DBG_NOINDENT( "  fits on vpage=" << vertical_page_count
				  << " leaving vert_space_left=" << vert_space_left
				  << " (full points)" << endl );
	
		    //Save computed per-plot info for later.  Last 2 elements are
		    //  doubles which have integer values.
	
		    DBG_NOINDENT( "Stuffing data into res_plots[" << nplot << "]:\n" );
		    DBG_NOINDENT( "    vert page count = " << vertical_page_count << endl );
		    DBG_NOINDENT( "    vert space left = " << vert_space_left << endl );
		    DBG_NOINDENT( "    vert space left (top of plot) = " << (vert_space_left + adj_res_height) << endl );
		    DBG_NOINDENT( "    adj_res_height = " << adj_res_height << endl );
		    res_plots[nplot].cur_mw_object = theObject;
		    res_plots[nplot].vpage = vertical_page_count;
		    res_plots[nplot].vert_space_left = vert_space_left + adj_res_height; //plot TOP
		    res_plots[nplot].adj_res_height = adj_res_height; //and height
	
		    nplot++;
		}
	    }
	}
	else { //plot Activity Display (including its subsidiary regions) ONLY
	    DBG_NOINDENT( "No resources to plot.\n" );	
	    nresources = 0;
	}

	DBG_NOINDENT("IO_plotter::generate_plot found number of resources to plot=" << nresources << endl);

	//Open and setup output file
	std::ofstream fout(*theFileName);

	if (! fout) {
	    DBG_UNINDENT("IO_plotter FAIL: cannot open file \"" << *theFileName << "\"" << endl);
	    return apgen::RETURN_STATUS::FAIL;
	}

	fout.precision(6);	//HARDCODED precision, reaffirms doc. default

	//Start building print job (output all-pages header)
	for (psline = 0; header[ psline ]; psline++)
	    fout << header[psline] << endl;

	fout << "/pageWidth " << pageWidth << " def" << endl;
	fout << "/pageHeight " << pageHeight << " def" << endl;
	fout << "/leftMargin " << leftMargin << " def" << endl;
	// fout << "/topMargin " << topMargin << " def" << endl;
	fout << "/topMargin " << topMargin + (18+12) << " def" << endl;	// (18+12) for title and subtitle

	if (print_header_page) {
	    plot_header_page(	selected_activity_system,
				fout,
				nlegends,
				nresources,
				pages_per_display_width,
				vertical_page_count,
				plotstart,
				plotspan,
				timezone_button,
				res_plots,
				list_of_selected_resource_systems );
	    //Use standard page-completer to finish header page; page==vpage==0
	    //  signals wrap_up_one_page() to just do portion needed for this page.
	    pagestart = plotstart;
	    pageend = plotstart + plotspan;
	    wrap_up_one_page(	vertical_page_count,
				0,
				0,
				page_header,
				page_subhead,
				fout,
				pages_per_display_width,
				pagestart,
				timezone_button,
				pageend,
				list_of_legends,
				print_time );
	}

	//Loop for each page (outer loop across time; inner loop across legends)

	sec_per_pixel = plotspan / (pages_per_display_width * (LANDSCAPE_PLOT_WIDTH - LEGENDS_REGION_WIDTH));
	pagespan = plotspan / (double) pages_per_display_width;
	pagestart = plotstart - pagespan;
	pageend   = plotstart;

	for (page = 1; page <= pages_per_display_width; page++) {
	    DBG_NOINDENT("IO_plotter::generate_plot: START page # " << page << endl);

	    //Compute page's time span (algorithm differs if page_format_choice>0)
	    pagestart += pagespan;
	    pageend   += pagespan;

	    for (vpage = 1; vpage <= vertical_page_count; vpage++) {
		DBG_NOINDENT("IO_plotter::generate_plot: START vpage # " << vpage << endl);
		//Page initialization
		fout << "%%Page: " << pageNumber << " " << pageNumber << "\nStartPage" << endl; pageNumber++;

		//Plot timeline (same on every vert-stacked page); however, this
		//  does not print corner area with start+end times and timezone,
		//  which are printed by wrap_up_one_page(), so can use BOLD font.
		plot_timescale(fout, pagestart, pagespan, majors, minors, offset, sec_per_pixel, timezone_button);

		//Plot constraint violations, if Constraints are "On".  Plot on
		//  every page, both for uniformity of appearance (and fits nicely
		//  below timescale),and because a violation may relate to activity
		//  and/or resource information on any/all of vert-stacked pages.


		// if (APcloptions::theCmdLineOptions().constraints_active && CON_exec::violations().get_length() /*1+ violations*/)
		  //   plot_violations(fout, pagestart, pageend, sec_per_pixel);

		//else, space is allocated but unused(for uniformity of appearance)

		//FUTURE: may be able to restrict plot_legends and plot_activities
		//        to pages 1-M, and plot_resources to pages M-N, via "if"s

		if (nlegends) {
		    plot_legends( list_of_legends , fout, vpage );
		    plot_activities(selected_activity_system, list_of_legends,
				    fout, vpage,
				    plotstart, plotstart + plotspan,
				    pagestart, pageend,
				    sec_per_pixel, page );
		}
		//else, zero legends, so no legends or activities to plot

		if ( nresources ) {
		    plot_resources(	nresources,
					res_plots,
					fout,
					vpage,
					pages_per_display_width,
					page); }
		//else, zero RDs, or user wants to plot AD only

		wrap_up_one_page(vertical_page_count, page, vpage, page_header, page_subhead,
				 fout, pages_per_display_width, pagestart, timezone_button,
				 pageend, list_of_legends, print_time);
		DBG_NOINDENT("IO_plotter::generate_plot: END vpage # " << vpage << endl);
	    }

	    DBG_NOINDENT("IO_plotter::generate_plot: END page # " << page << endl);
	}

	for (psline = 0; trailer[ psline ]; psline++) {
	    fout << trailer[ psline ] << endl; }
	fout << "%%Trailer\n%%Pages: " << pageNumber - 1 << "\n%%EOF" << endl;

	DBG_UNINDENT("IO_plotter::generate_plot: SUCCESS,"
		     << " fout.precision()= " << fout.precision() << endl);
	//fout automatically closes, since local to this method
	delete (res_plots);
    }
#endif
    return apgen::RETURN_STATUS::SUCCESS;
}

//Plot single header page for the 1+ pages of the plot.
void IO_plotter::plot_header_page(
		ACT_sys		*selected_activity_system,
		ofstream	& fout,
		int		nlegends,
		int		nresources,
		int		pages_per_display_width,
		int		vertical_page_count,
		CTime		plotstart,
		CTime		plotspan,
		TIMEZONE	timezone_button,
		res_plot_info	*res_plots,
		List		& list_of_selected_resource_systems ) {

#ifdef GUI
    if (GUI_Flag) {

	stringtlist::iterator	fileNameListIterator(eval_intfc::ListOfAllFileNames());
	emptySymbol*		fileNamePtr;	//iterator and current node in source files
	List_iterator		res_iterator(list_of_selected_resource_systems);
	Pointer_node*		resnode;	//iterator and current node in RDs to plot
	int nplot, ntotlines, nsellines, i;
	double x, y, yi;
	Cstring tot_report, sel_report, full, left, right;
	CTime plotend = plotstart + plotspan;

	// fout << "StartPage" << endl;
	fout << "%%Page: " << pageNumber << " " << pageNumber << "\nStartHeader" << endl; pageNumber++;
	x = y = 10.0;	//top-left-corner start location for information
	yi = 10.0;		//line-spacing increment

	//General header line
	fout << "(APGEN Header Page, " << get_apgen_version_build_platform() << ") "
	     << x << " " << y << " text" << endl;

	y += 2 * yi;

	string version(get_apgen_version());
	
        string::size_type idx = version.find('\n');
	idx++;//skip newline
	string::size_type idx2 = version.find('\n', idx);

	while (1) {
	    string chunk = version.substr(idx, idx2 - idx);
	    fout << "(" << chunk << ") " << x << " " << y << " text" << endl;
	    y += yi;

	    if (idx2 == string::npos)
		break;

	    idx = idx2 + 1;
	    idx2 = version.find('\n', idx); }
	
	//Basic stats (number of pages, legends, resources) plus time info
	fout << "(Plot has " << nlegends << " Legends and " << nresources << " Resources on " << vertical_page_count << " vertical ";
	if (vertical_page_count > 1)
	    fout << "(A-"
		 << page_letters[(vertical_page_count <= page_letters_count)
 				? vertical_page_count		//prints letter
				: (page_letters_count+1)]	//prints "*"
		 << ") ";
	fout << "by " << pages_per_display_width << " horizontal pages) " << x << " " << y << " text" << endl;
	y += 2 * yi;
	Cstring scet_time;
	plotstart.time_to_complete_SCET_string(scet_time, timezone_button);
	fout << "(Plot Start=" << scet_time << ") " << x << " " << y << " text" << endl;
	y += yi;
	plotend.time_to_complete_SCET_string(scet_time, timezone_button);
	fout << "(Plot End  =" << scet_time << ") " << x << " " << y << " text" << endl;
	y += yi;
	fout << "(Plot Timespan=" << plotspan.to_string() << ") " << x << " " << y << " text" << endl;
	y += 2 * yi;

	//File origin (relies on ListOfAllFileNames, which DOESN'T include AAF's)
	fout << "(Plot uses information from these "
	     << eval_intfc::ListOfAllFileNames().get_length()
	     << " non-AAF files, possibly after modification:) "
	     << x << " " << y << " text" << endl;
	while ((fileNamePtr = fileNameListIterator())) {
	    y += yi;
	    fout << "( " << fileNamePtr->get_key() << ") " << x << " " << y << " text" << endl;
	}
	y += 2 * yi;

	//Constraint violation information
	if (APcloptions::theCmdLineOptions().constraints_active) {
	    //Print full "total" report (always 1-3 lines)
	    ntotlines = selected_activity_system->constraint_display->report_total(tot_report);
	    full = tot_report;
	    for (i = 0; i < ntotlines; i++) {
		left = full / "\n";
		right = "\n" / full;
		fout << "(" << left << ") " << x << " " << y << " text" << endl;
		full = right;
		y += yi;
	    }
	    y += yi;

	    //Print up to 3 lines (i.e. only the summary) of the "selected" report,
	    //  which is chosen to match the entire plot's time span
	    nsellines = selected_activity_system->constraint_display->report_in_span(plotstart, plotspan, sel_report);
	    if (nsellines > 3)
		nsellines = 3;
	    full = "Selected" / sel_report; //allows replacement of fixed 1st word
	    for (i = 0; i < nsellines; i++)
		{
		    left = full / "\n";
		    right = "\n" / full;
		    fout << "(" << (i ? "" : "Plotted") << left << ") "
			 << x << " " << y << " text" << endl;
		    full = right;
		    y += yi;
		}
	    y += yi;
	}
	else {
	    fout << "(Constraint processing is turned Off) "
		 << x << " " << y << " text" << endl;
	    y += 2 * yi;
	}

	//Per-resource-plot information (plotted range, any MIN and MAX, etc.)
	nplot = 0;
	while ((resnode = (Pointer_node*) res_iterator.next())) {
	    fout << "(Resource plot #" << (nplot + 1) << " is on ";
	    if (vertical_page_count == 1)
		fout << "the only vertical page";
	    else
		fout << "vertical page "
		     << page_letters[(res_plots[nplot].vpage <= page_letters_count)
				    ? res_plots[nplot].vpage	//prints letter
				    : (page_letters_count+1)];	//prints "*"
	    fout << ") " << x << " " << y << " text" << endl;

	    //List_iterator limwobjects(*res_plots[nplot].cur_res_sys->mw_objects);
	    //MW_object* mwobjectnode;
	    //while (mwobjectnode = (MW_object*)(limwobjects.next())) {; }

	    y += yi;
	    nplot++;
	}

	//Skip this, since now use wrap_up_one_page() to finish header page.
	//fout << "EndPage" << endl << "%" << endl; //Resets setfont,setdash,etc

    }
#endif

}

//Plot timescale for THIS vert-stacked page. Note that time range and timezone,
//  which are in the upper left corner, are handled by wrap_up_one_page().
void IO_plotter::plot_timescale(ofstream & fout,
	CTime pagestart, CTime pagespan, CTime majors, CTime minors,
	CTime offset, CTime sec_per_pixel, TIMEZONE timezone_button) {

#ifdef GUI
    if (GUI_Flag) {

    double w_previous = -LANDSCAPE_PLOT_WIDTH; //1 page to LEFT of page
    double w, w_majors, w_minors, w_offset, w_minor_offset;
    int len, n, n_majors;
    CTime majortime;
    Cstring datetime;	//easier to do substrings than char*
    Cstring paddatetime;
    Cstring blanks = "                                ";
    Cstring dateonly;
    Cstring timeonly;
    Cstring fullalt;
    Cstring altdate;
    Cstring supdate;
    bool subsec_majors;

    DBG_INDENT("IO_plotter::plot_timescale for THIS vert-stacked page:" << endl);

    //Set up clipping for timescale region (no xform however)
    fout << "gsave" << endl;
    fout << "newpath "		//Clockwise rectangle
     << LEGENDS_REGION_WIDTH << " " << 0 << " moveto "
     << LANDSCAPE_PLOT_WIDTH << " " << 0 << " lineto "
     << LANDSCAPE_PLOT_WIDTH << " "
     << TIMESCALE_REGION_HEIGHT << " lineto "
     << LEGENDS_REGION_WIDTH << " "
     << TIMESCALE_REGION_HEIGHT << " lineto closepath clip"
     << endl;

	//Find out major-interval, minor-interval (which ALWAYS divides
	//  evenly into major), & offset of 1st major mark from left edge

	/*
	**	NOTE: this function results in hanging the system sometimes
	**	when the pagespan is negative. Actually this almost certainly
	**	occurs when the GUI uses the results to build the time ruler,
	**	NOT when doing plots. Just in case we will return promptly
	**	when this occurs...
	*/

	if (determineTimemarkLayout(	pagestart, pagespan,
					timezone_button,
    					majors, minors, offset) != apgen::RETURN_STATUS::SUCCESS)
		return;

    //Find out if major interval is less than 1 second (this triggers special
    //  logic for tick labeling).  Note that major interval as returned by
    //  determineTimemarkLayout() is "sensible", so get 2 sec,1 sec,0.5 sec,...
    if (majors <= CTime(0, 999, true))	// 1 sec, w/safety for roundoff error
	subsec_majors = true;
    else
	subsec_majors = false;
    //Convert these into graphic units "pixels"
    w_majors = majors / sec_per_pixel;
    w_minors = minors / sec_per_pixel;
    w_offset = offset / sec_per_pixel;
    //Compute offset of first minor mark from left edge
    w_minor_offset = w_offset - (w_minors * floor(w_offset / w_minors));

    //Draw the horizontal timeline line
    fout <<        LEGENDS_REGION_WIDTH
     << " " << TIMESCALE_LINE_Y_OFFSET
     << " " << LANDSCAPE_PLOT_WIDTH
     << " " << TIMESCALE_LINE_Y_OFFSET << " line" << endl;
    //Draw & count the major ticks, starting with ONE PRIOR to computed
    //  first one (this handles boundary case where major tick EXACTLY
    //  at left edge is not found; no harm since clips if negative)
    n_majors = 0;
    for (w = LEGENDS_REGION_WIDTH + w_offset - w_majors;
	  w <= LANDSCAPE_PLOT_WIDTH;
	  w += w_majors)
    {
	n_majors++;
	fout << w << " " << TIMESCALE_LINE_Y_OFFSET << " " << w << " "
	     << (TIMESCALE_LINE_Y_OFFSET - TIMESCALE_MAJOR_TICK_HEIGHT)
	     << " line" << endl;
    }
    //Draw the minor ticks, starting with first minor mark
    //  (OK to overwrite major ticks, as long as B&W only!)
    for (w = LEGENDS_REGION_WIDTH + w_minor_offset;
	  w <= LANDSCAPE_PLOT_WIDTH;
	  w += w_minors)
	fout << w << " " << TIMESCALE_LINE_Y_OFFSET << " " << w << " "
	     << (TIMESCALE_LINE_Y_OFFSET - TIMESCALE_MINOR_TICK_HEIGHT)
	     << " line" << endl;

    //Compute and print the date+time for each major tick, including
    //  the "ONE PRIOR" one drawn above, which is needed both to handle
    //  aforementioned boundary condition and to handle print spillover.
    //97-10-05 change so print spillover(even from tick beyond edge)IS printed!
    //for (n = 0; n < n_majors; n++)
    for (n = -1; n <= n_majors; n++) {
	majortime = pagestart + offset + (majors * (double)(n - 1));
	//if subsec_majors, keep millisec->21 chars;else round to sec->17 chars
	//  (if have an Epoch, length will depend on Epoch name and 2nd arg)
	datetime = majortime.to_string(timezone_button, subsec_majors);
	if ((timezone_button.zone != TIMEZONE_EPOCH) && (! subsec_majors)) {   //nominal case: ordinary timezone and span
	    dateonly = datetime.substr(0,8);
	    timeonly = datetime.substr(9,8);
	    majortime.time_to_word_date_SCET_string(fullalt, timezone_button); //->17 chars
	    altdate  = fullalt.substr(0,2) + fullalt.substr(4,4) + fullalt.substr(9,2); //->8 chars
	    supdate  = "";
	}	//new top line unused for non-Epoch situation
	else if ((timezone_button.zone != TIMEZONE_EPOCH) && (subsec_majors))	//ordinary timezone but span of few sec
	{
	    altdate  = datetime.substr(0,8);
	    dateonly = datetime.substr(9,8);
	    timeonly = datetime.substr(17,4);	//implicit 4-blank pad on right
	    majortime.time_to_word_date_SCET_string(fullalt, timezone_button); //->17 chars
	    supdate  = fullalt.substr(0,2) + fullalt.substr(4,4) + fullalt.substr(9,2); //->8 chars
	}
	else				//epoch, with span small or large
	{
	    //Add 4 blanks to end to keep millisec on its own line (if needed).
	    if (subsec_majors)
		datetime << blanks.substr(0, 4);
	    //Fit in 8 chars per line (up to 32 chars maximum, with truncation
	    //  of beginning of string -- i.e. beginning of Epoch name).  Note
	    //  minimum of 1-char Epoch + sign + 8-char time = 10-char minimum.
	    len = datetime.length();
	    if (len >= 32)	//keep only last 32 chars
		paddatetime = datetime.substr(len-32, 32);
	    else		//left-blank-pad to exactly 32 chars
		paddatetime = blanks.substr(0, 32-len) + datetime;
	    //Logic can now make use of constant 32-char length of paddatetime.
	    timeonly = paddatetime.substr(24, 8);
	    dateonly = paddatetime.substr(16, 8);
	    if (len > 16)
		altdate = paddatetime.substr(8, 8);
	    else
		altdate = "";
	    if (len > 24)
		supdate = paddatetime.substr(0, 8);
	    else
		supdate = "";
	}
	//Compute major-tick-centered position of 8-char date(or time)
	w = LEGENDS_REGION_WIDTH + w_offset + TIMESCALE_DATETIME_X_OFFSET + ((n-1) * w_majors);
	//Only output date+time if NONE of it goes outside left or right clip
	//  (i.e. region edges) AND if minimum X-distance between date+time
	//  labels is maintained.
	//97-10-05 change so spilling outside clip OK; do maintain min X-dist.
	//if ((w >= LEGENDS_REGION_WIDTH)
	//  && (w <= LANDSCAPE_PLOT_WIDTH +2.*TIMESCALE_DATETIME_X_OFFSET)
	//  && ((w - w_previous) >= TIMESCALE_MIN_X_SEPARATION))
	//97-10-29 changed TIMESCALE_MIN_X_SEPARATION so it was not evenly
	//  divisible into the timeline width; this prevents roundoff errors
	//  from occurring in the common situation of 5 or 10 major units
	//  evenly fitting the span; does NOT prevent roundoff in less "common"
	//  situations (spans that the user is less likely to choose).
	if ((w - w_previous) >= TIMESCALE_MIN_X_SEPARATION)
	{
	    if (supdate.has_value())
	        fout << "(" << (*(supdate))  << ") " << w << " " << TIMESCALE_SUPDATE_Y_POSITION << " text" << endl;
	    if (altdate.has_value())
	        fout << "(" << (*(altdate))  << ") " << w << " " << TIMESCALE_ALTDATE_Y_POSITION << " text" << endl;
	    //dateonly and timeonly ALWAYS contain a non-null string
	    fout << "(" << (*(dateonly)) << ") " << w << " " << TIMESCALE_DATE_Y_POSITION << " text" << endl;
	    fout << "(" << (*(timeonly)) << ") " << w << " " << TIMESCALE_TIME_Y_POSITION << " text" << endl;
	    w_previous = w; } }	//Save, so can space next date properly

    //Restore pre-clip (for timescale region) context
    fout << "grestore" << endl;

    DBG_UNINDENT("IO_plotter::plot_timescale: DONE" << endl);

    }
#endif
}

// Hopper design: for now we should stick to legends of regular ACT_sysses only
//Plot legends for THIS vert-stacked page
void IO_plotter::plot_legends(List &list_of_legends, ofstream &fout, int vpage) {

#ifdef GUI
  if (GUI_Flag) {

    List_iterator	legendList(list_of_legends);
    LegendObject	*legendNode;
    Pointer_node	*legendPointer;
    double x = LEGEND_X_OFFSET;	//moves just past box edge
    double y = TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT;	// start of legend space
    int verticalSpace = 0;
    int currentPage = 1;

    DBG_INDENT("IO_plotter::plot_legends for THIS vert-stacked page:" << endl);

    //Set up clipping for legends region (no xform however)
    fout << "gsave % begin legend clip region" << endl;
    fout << "newpath " << endl;	//Clockwise rectangle
    fout << 0 << " " << (TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT) << " moveto " << endl;
    fout << LEGENDS_REGION_WIDTH << " " << (TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT) << " lineto " << endl;
    fout << LEGENDS_REGION_WIDTH << " " << LANDSCAPE_PLOT_HEIGHT << " lineto " << endl;
    fout << 0 << " " << LANDSCAPE_PLOT_HEIGHT << " lineto" << endl;
    fout << "closepath clip" << endl;

    fout << "/Helvetica findfont [1.00 0 0 -1.00 0 0] makefont 8 scalefont setfont" << endl;

    // plot legends for this page
    while (vpage >= currentPage && (legendPointer = (Pointer_node *) legendList.next())) {
      legendNode = (LegendObject *) legendPointer->get_ptr();
      int nodeSpace = legendPixelsToPoints(legendNode->get_official_height());	// determine this legend entry height in points

      // Hopper design: skip if not a legend of a regular ACT_sys
      // Restrict to legends printed on THIS vertically-stacked page

      if (verticalSpace + nodeSpace > ACTIVITY_PLOT_TOTAL_HEIGHT) {
	// we're about to cross a page boundary.  Legend entries are not split across pages.  Put this one on the next page.
	verticalSpace = 0;
	currentPage++;
      }

      if (vpage == currentPage) {	// plot the current page until we run over to the next
	fout << "(" << legendNode->get_key() << ") " << x << " " << (y + (0.5 * nodeSpace) + LEGEND_Y_OFFSET) << " text" << endl;
	y += nodeSpace;	// move to end of this legend entry (start of next)
      }

      verticalSpace += nodeSpace;	// remember where we are

    }

    //Restore pre-clip (for legends region) context
    fout << "grestore % end legend clip region" << endl;

    DBG_UNINDENT("IO_plotter::plot_legends: DONE" << endl);
  }
#endif
}

//Plot constraint violations within time span of this horiz-split page (like
//  the timescale, this is identical on ALL vert-stacked pages).  Directly
//  consults time of violations (NOT their graphic DS_line representations on
//  the screen) for maximum precision.
void IO_plotter::plot_violations(ofstream& fout, CTime pagestart, CTime pageend, CTime sec_per_pixel) {

// #ifdef GUI
#ifdef GUTTED
    if (GUI_Flag) {

	List_iterator	violst(CON_exec::violations());
	Violation_info	* vionode;
	double		x, y, h;

	//Set up clipping for violation region (no xform however)
	fout << "gsave" << endl;
	fout << "newpath "		//Clockwise rectangle
	     << LEGENDS_REGION_WIDTH << " " << TIMESCALE_REGION_HEIGHT << " moveto "
	     << LANDSCAPE_PLOT_WIDTH << " " << TIMESCALE_REGION_HEIGHT << " lineto "
	     << LANDSCAPE_PLOT_WIDTH << " "
	     << (TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT) << " lineto "
	     << LEGENDS_REGION_WIDTH << " "
	     << (TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT)
	     << " lineto closepath clip" << endl;

	//Loop through list of violations, finding those within page's time window.
	while ((vionode = (Violation_info *) violst.next()))
	    if ((vionode->getetime() >= pagestart) && (vionode->getetime() <= pageend)) {
		x = LEGENDS_REGION_WIDTH + ((vionode->getetime() - pagestart) / sec_per_pixel);
		y = TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT;
		h = ((vionode->severity == ERROR) ? VIOLATIONS_ERROR_HEIGHT : VIOLATIONS_WARNING_HEIGHT);

		//Draw the computed violation mark.
		fout << x << "\t" << y << "\t" << x << "\t" << (y - h) << "\tline" << endl;
	    }

	//Restore pre-clip (for constraint-violation plot region) context
	fout << "grestore" << endl;

    }
#endif
}

struct SortVertically : public std::binary_function<ActivityLayoutInfo*, ActivityLayoutInfo*, bool> {
    bool operator()(const ActivityLayoutInfo* layout1, const ActivityLayoutInfo* layout2) {
	return layout1->getY() < layout2->getY();
    }
};

// Plot activities for THIS vert-stacked page, within time span of this horiz-
// split page.  Directly consults start and end time of activities (NOT their
// graphic DS_line representations on the screen) for maximum precision.

ActivityLayout::ActivityLayout(
			       ActivityInfoList& activities,	// a list of composite rectangles to place on the page
			       double regionHeight		// height of writable region in units
			       ) : overlays(), height(regionHeight) {

    // For each item in the list, calculate values, convert to ActivityLayoutInfo, attempt to add it to the list,
    // if a spot is found, set Y value, add to (date sorted list).  If no spot, construct a new overlay and repeat until a spot is found.

    for (int i = 0; i < activities.size(); i++) {
	ActivityDataList* fragments = &(activities[i]->getActivityDataList());
	if (fragments == 0 || fragments->size() == 0)
	    continue;	// nothing in this one

	ActivityData* data = (*fragments)[0];
	double xstart = data->getStart();
	double xend = data->getEnd();
	double height = data->getHeight();

	if (xstart > xend) {
	    double temp = xstart;
	    xstart = xend;
	    xend = temp;
	}

	for (int j = 1; j < fragments->size(); j++) {
	    // construct minimum enclosing rectangle of all fragments
	    data = (*fragments)[j];
	    double dstart = data->getStart();
	    double dend = data->getEnd();
	    if (dstart > dend) {
		double temp = dstart;
		dstart = dend;
		dend = temp;
	    }

	    if (dstart < xstart)
		xstart = dstart;
	    if (xend < dend)
		xend = dend;
	    height += data->getHeight();
	}

	if (height <= 0 || (xend - xstart) <= 0)
	    continue;	// it's there but has no size
	
	// construct new ActivityLayoutInfo with this region and place it somewhere

	ActivityLayoutInfo* layoutInfo = new ActivityLayoutInfo(xstart, 0, xend-xstart, height, activities[i]->activity);
	
	// Each overlay is maintained in x sorted order.  !!! NO -- we need to modify this code to make it sorted !!!
	// 1. skip entries in the current overlay until we find some that intersect the new objects x coordinates
	// 2. compute the Y location of the top of the last region that intersects the new region.
	// 3. if the space remaining is larger than the space needed for the new object, add it to the list at the appropriate x
	// 4. if there is no space, move to the next overlay and repeat 1,2,3
	// 5. if the list of overlays is exhausted, create a new list, add it to the list of overlays and add the new object to it.
	// Kludge: every odd overlay list has it's first entry offset by half it's height in a crude attempt to make overlapping regions visible

	ActivityOverlayList* currentOverlayList;

	if (overlays.size() == 0) {
	    ActivityOverlayList* overlayList = new ActivityOverlayList;
	    overlayList->push_back(layoutInfo);	// add to this list of regions on the new overlay
	    overlays.push_back(overlayList);	// add to end of list of all overlays
	    continue;	// move onto next set of fragments
	}

	bool done = false;

	for (int currentOverlay = 0; !done && currentOverlay < overlays.size(); currentOverlay++) {
	    ActivityOverlayList intersections;
	    currentOverlayList = overlays[currentOverlay];

	    // locate regions that intersects the new region
	    for (int j = 0; j < currentOverlayList->size(); j++) {
		ActivityLayoutInfo* region = (*currentOverlayList)[j];
		if (xstart >= (region->getX() + region->getWidth()))
		    continue;	// the new region starts past the end of this region in the list
		if (xend < region->getX())
		    continue;	// the new region ends before this region
		intersections.push_back(region);
	    }

	    if (intersections.size() == 0) {
		// nothing intersects the new region in this overlay
		if ((currentOverlay & 1) && (height + height/2.0) < regionHeight)
		    layoutInfo->setY(height / 2.0);	// odd overlays are offset from the origin unless they have a narrow space
		// find location to insert this
		currentOverlayList->push_back(layoutInfo);
		done = true;
		break;	// move onto next set of fragments
	    }

	    // find spot to put the new region.
	    // sort the list of intersecting regions in Y order and find a free spot

	    // sort by height
	    sort(intersections.begin(), intersections.end(), SortVertically());

	    // find a slot between [0, the sorted objects in the list, the top of the range] that's big enough to accomodate the new region
	    double maxY = 0;

	    // check to see if it fits at the bottom
	    if (intersections[0]->getY() > height) {
		// it goes at the bottom
		layoutInfo->setY(0);
		done = true;
	    }
	    else
		maxY = intersections[0]->getY() + intersections[0]->getHeight();

	    // look for space between regions
	    if (!done) {
		for (int j = 1; j < intersections.size(); j++) {
		    // since regions in any one overlay do not intersect, we can use the Y and height without worrying about overlaps
		    if (intersections[j]->getY() - maxY >= height) {
			// we have a big enough gap.  place this new region at the bottom end of the gap.
			layoutInfo->setY(maxY);
			done = true;
			break;
		    }
		    else
			maxY = intersections[j]->getY() + intersections[j]->getHeight();
		}
	    }

	    // check for space at the top of the region
	    if (!done && ((regionHeight - maxY) > height)) {
		layoutInfo->setY(maxY);
		done = true;
	    }

	    // add this to the list at the proper point if there is a spot
	    if (done) {
		currentOverlayList->push_back(layoutInfo);	// place at end
	    }
	}

	if (!done) {
	    // There was no place to put this new region.  We have to put it in a new overlay.
	    ActivityOverlayList* overlayList = new ActivityOverlayList;
	    if ((overlays.size() % 2 == 1)  && (height + height/2.0) < regionHeight)
		layoutInfo->setY(height / 2.0);	// offset odd numbered overlays unless the space is too narrow
	    overlayList->push_back(layoutInfo);	// add to this list of regions on the new overlay
	    overlays.push_back(overlayList);	// add to end of list of all overlays
	}
    }
}

ActivityLayout::~ActivityLayout() {
    for (int i = 0; i < overlays.size(); i++)
	delete overlays[i];
}


class EnumeratorInfo {

    // setup values to plot a bar entity
public:
    ActivityInstance* request;
    double	x, y, w, h;
    int		ps_bitmap;
    // TypedSymbol	*pattern_value;
    Cstring	actinpattern;
    // TypedSymbol	*color_value;
    RGBvaluedef color;

public:

    RGBvaluedef& getDefaultColor() {
	static RGBvaluedef defaultColor = {(char*) "Black", 0, 0, 0};
	return defaultColor;
    }

    EnumeratorInfo(ActivityLayoutInfo& activityLayoutInfo) : actinpattern("0"), color(getDefaultColor()) {

	// assume the coordinate system has been change such that a y of zero is the bottom of the legend bars space

	request = dynamic_cast<ActivityInstance*>(activityLayoutInfo.getActivity());
	assert(request);
	try {
		const TypedValue &pattern_value = (*request->Object)["pattern"];
		actinpattern = (int) pattern_value.get_double(); }
	catch(eval_error) {}
	try {
		const TypedValue &color_value = (*request->Object)["color"];
		color = Patterncolors[color_value.get_int() - 1]; }
	catch(eval_error) {}

	x = activityLayoutInfo.getX();
	y = activityLayoutInfo.getY();
	w = activityLayoutInfo.getWidth();
	h = activityLayoutInfo.getHeight();

	// Bitmap numbers range from 1-64, corresponding to pattern numbers 0-63 (in APF syntax, and internal to APGEN)
	sscanf(actinpattern(), "%d", &ps_bitmap);
	ps_bitmap++;
    }

};

class PlotEnumerator : public ActivityEnumerator {
private:
    ofstream &fout;
    double sec_per_pixel;
    double xorigin, xcutoff;
    double xstart, xend;

public:
    PlotEnumerator(ofstream &ostream, CTime secPerPix, CTime pstart, CTime pend, CTime origin, CTime cutoff) : fout(ostream) {
	sec_per_pixel = secPerPix.convert_to_double_use_with_caution();
	xorigin = origin.convert_to_double_use_with_caution();	// bar must land, even partially, between xorigin and xcutoff
	xcutoff = cutoff.convert_to_double_use_with_caution();
	xstart = pstart.convert_to_double_use_with_caution();
	xend = pend.convert_to_double_use_with_caution();
    }

    ~PlotEnumerator() {};

    void action(ActivityLayoutInfo& activityLayoutInfo) {
	EnumeratorInfo info(activityLayoutInfo);

	if ((info.x+info.w) <= xorigin || info.x >= xcutoff
	    || (info.request->getetime() + info.request->get_timespan()).convert_to_double_use_with_caution() <= xorigin
	    || info.request->getetime().convert_to_double_use_with_caution() >= xcutoff)
	    return;	// if nothing of this bar (region) shows on the plot, do nothing

	// assume the coordinate system has been change such that a y of zero is the bottom of the legend bars space

	// PostScript does the clipping to the boundary of the active region
	fout << info.color.red << " 255 div "
	     << info.color.green << " 255 div "
	     << info.color.blue << " 255 div "
	     << " setrgbcolor % " << info.color.colorname << endl;
	fout << "/PS_BITMAP_" << info.ps_bitmap
	     << " " << (info.request->getetime().convert_to_double_use_with_caution() - xorigin) / sec_per_pixel
	     << " " << info.y
	    // << " " << ((info.request->getetime() + info.request->get_timespan()).convert_to_double_use_with_caution() - xorigin) / sec_per_pixel
	     << " " << info.request->get_timespan().convert_to_double_use_with_caution() / sec_per_pixel
	     << " " << ACTIVITY_BAR_HEIGHT
	     << " rectanglefilldraw" << endl;

	return;
    }

};

class TextEnumerator : public ActivityEnumerator {
private:
    ofstream &fout;
    double sec_per_pixel;
    double xorigin;	// leftmost time on this plot
    double xcutoff;	// rightmost time on the plot
    double xstart, xend;	// start and end of the whole plot as opposed to just this page

public:
    TextEnumerator(ofstream &ostream, CTime secPerPix, CTime pstart, CTime pend, CTime origin, CTime cutoff) : fout(ostream) {
	sec_per_pixel = secPerPix.convert_to_double_use_with_caution();
	xorigin = origin.convert_to_double_use_with_caution();	// plotstart
	xcutoff = cutoff.convert_to_double_use_with_caution();	// plotend
	xstart = pstart.convert_to_double_use_with_caution();
	xend = pend.convert_to_double_use_with_caution();
    }

    ~TextEnumerator() {};

    void action(ActivityLayoutInfo& activityLayoutInfo) {
	EnumeratorInfo info(activityLayoutInfo);

	Cstring text = info.request->get_key();

	if (info.x + info.w <= xorigin ||
	    info.x          >= xcutoff ||
	    info.request->getetime().convert_to_double_use_with_caution() >= xcutoff)
	    return;	// if nothing of this bar (region) shows on the plot, do nothing

	// assume the coordinate system has been change such that a y of zero is the bottom of the legend bars space

	double textEnd = info.request->getetime().convert_to_double_use_with_caution() + (sec_per_pixel * 8 * (info.request->get_key().length()));

	if (info.x < xstart) {	// keep label on page
	    fout << "(" << text << ") "
		 << 0 << " "
		 << ACTIVITY_BAR_HEIGHT + (info.y - ACTIVITY_NAME_Y_OFFSET)
		 << " text" << endl;
	    fout // show has already placed us at the end of the string
		<< ((info.request->getetime() + info.request->get_timespan()).convert_to_double_use_with_caution() - xorigin) / sec_per_pixel
		<< " " <<  ACTIVITY_BAR_HEIGHT + (info.y - ACTIVITY_NAME_Y_OFFSET) << " lineto "
		<< ((info.request->getetime() + info.request->get_timespan()).convert_to_double_use_with_caution() - xorigin) / sec_per_pixel
		<< " " <<  ACTIVITY_BAR_HEIGHT + info.y << " lineto "
		<< "0 setlinewidth stroke" << endl;
	}
	else if (textEnd > xend) {	// text ends at right hand side of last plot page
	    fout << (info.x - xorigin) / sec_per_pixel << " " << info.y + ACTIVITY_BAR_HEIGHT << " moveto "
		 << (info.request->getetime().convert_to_double_use_with_caution() - xorigin) / sec_per_pixel
		 << " " << info.y + ACTIVITY_BAR_HEIGHT << " lineto "
		 << (info.request->getetime().convert_to_double_use_with_caution() - xorigin) / sec_per_pixel << " " << info.y << " lineto "
		 << "0 setlinewidth stroke" << endl;
	    fout << "(" << text << ") "
		 << ((xcutoff - (sec_per_pixel * 8 * (info.request->get_key().length()))) - xorigin) / sec_per_pixel << " "
		 << ACTIVITY_BAR_HEIGHT + (info.y - ACTIVITY_NAME_Y_OFFSET)
		 << " text" << endl;
	}
	else {
	    fout << (info.x - xorigin) / sec_per_pixel << " "
		 << info.y + info.h - (ACTIVITY_PLOT_REGION_HEIGHT - ACTIVITY_BAR_HEIGHT - 8) << " moveto "
		 << (info.x - xorigin) / sec_per_pixel << " " << info.y  << " lineto "
		 << "0 setlinewidth stroke" << endl;
	    fout << "(" << text << ") "
		 << (info.x - xorigin) / sec_per_pixel << " "
		 << ACTIVITY_BAR_HEIGHT + (info.y - ACTIVITY_NAME_Y_OFFSET)
		 << " text" << endl;
	}

	return;
    }

};

typedef map<LegendObject*, ActivityLayout*> LegendActivityLayouts;

void IO_plotter::plot_activities(
				 ACT_sys	*selected_activity_system,
				 List		&list_of_legends,
				 ofstream	&fout,
				 int		vpage,
				 CTime		plotStartTime,
				 CTime		plotEndTime,
				 CTime		pagestart,
				 CTime		pageend,
				 CTime		sec_per_pixel,
				 int		page) {
    List_iterator	legendList(list_of_legends);
    Pointer_node *legendPointer;	//current pointer node
    LegendObject *legendNode;	//current legend node
    LegendActivityLayouts legendLayouts;

    int verticalSpace = 0;
    int currentPage = 1;
    double y = 0;
    const double yBase = TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT;

    //Set up clipping for activity-instance plot region (no xform however)
    fout << "gsave % begin plot activities" << endl;
    fout << "newpath" << endl;
    fout << LEGENDS_REGION_WIDTH << " " << yBase << " moveto " << endl;
    fout << LANDSCAPE_PLOT_WIDTH << " " << yBase << " lineto " << endl;
    fout << LANDSCAPE_PLOT_WIDTH << " " << LANDSCAPE_PLOT_HEIGHT << " lineto " << endl;
    fout << LEGENDS_REGION_WIDTH << " " << LANDSCAPE_PLOT_HEIGHT << " lineto" << endl;
    fout << "closepath clip" << endl;

    // fout << "/Helvetica findfont [1.00 0 0 -1.00 0 0] makefont 8 scalefont setfont" << endl;
    fout << "/Helvetica findfont [1.00 0 0 1.00 0 0] makefont 8 scalefont setfont" << endl;

    // plot each activity
    while (vpage >= currentPage && (legendPointer = (Pointer_node *) legendList.next())) {
	legendNode = (LegendObject *) legendPointer->get_ptr();
	int nodeSpace = legendPixelsToPoints(legendNode->get_official_height());	// determine this legend entry height in points

	if (nodeSpace > ACTIVITY_PLOT_TOTAL_HEIGHT)	// make sure the whole legend fits on the page
	    nodeSpace = (int) ACTIVITY_PLOT_TOTAL_HEIGHT;

	if (verticalSpace + nodeSpace > ACTIVITY_PLOT_TOTAL_HEIGHT) {
	    // we're about to cross a page boundary.  Legend entries are not split across pages.  Put this one on the next page.
	    verticalSpace = 0;
	    currentPage++;
	}

	if (vpage == currentPage) {
	    // draw activities

						// list of activities in this legend entry
	    slist<alpha_void, smart_actptr>::iterator	activityList(legendNode->ActivityPointers);
						// used to search the list to count how many activities will show
	    slist<alpha_void, smart_actptr>::iterator	visibleList(legendNode->ActivityPointers);
	    ActivityInstance*				request = NULL;
	    smart_actptr*				activityPointer = NULL;
	    ActivityInfoList*				activityInfoList = NULL;

	    int activityCount = legendNode->ActivityPointers.get_length();

	    if (activityCount > 0) {	// only do something if there is something there
		ActivityLayout* activeLayout;
		activityInfoList = new ActivityInfoList;

		// see if we have already done this (multi-part plot)
		LegendActivityLayouts::iterator iter = legendLayouts.find(legendNode);
		if (iter == legendLayouts.end()) {
		    // this is a new entry

		    // Note: width is in time, height is in pixels
		
		    while ((activityPointer = visibleList.next())) {
			request = (*activityPointer).operator->();

			// Check that instance is currently displayed (as opposed to invisible
			// in Paste buffer or due to abstraction, etc.)

			if (request->is_active() && !request->is_unscheduled() ) {
			
			
			    // Add this activity to the list of ones to be shown

			    ActivityInfo* info = new ActivityInfo;
			    ActivityDataList* dataList = &(info->getActivityDataList());

			    info->setAutoDelete(true);
			    info->activity = request;
			    double etime = request->getetime().convert_to_double_use_with_caution();

			    // bar
			    dataList->push_back(
						new ActivityData(
								 etime,
								 etime + request->get_timespan().convert_to_double_use_with_caution(),
								 ACTIVITY_BAR_HEIGHT
								 )
						);

			    // text
			    if (etime < plotStartTime.convert_to_double_use_with_caution())	// force label to left hand side of plot
				dataList->push_back(
						    new ActivityData(
								     plotStartTime.convert_to_double_use_with_caution(),
								     (plotStartTime + (sec_per_pixel * 8 * (request->get_key().length()))).convert_to_double_use_with_caution(),
								     8
								     )
						    );
			    else if (etime + (sec_per_pixel * 8 * (request->get_key().length())).convert_to_double_use_with_caution() > plotEndTime.convert_to_double_use_with_caution())
				// force label to end at the right side of the plot
				dataList->push_back(
						    new ActivityData(
								     (plotEndTime - (sec_per_pixel * 8 * (request->get_key().length()))).convert_to_double_use_with_caution(),
								     plotEndTime.convert_to_double_use_with_caution(),
								     8
								     )
						    );
			    else	// it fits where it is
				dataList->push_back(
						    new ActivityData(
								     etime,
								     etime + (sec_per_pixel * 8 * (request->get_key().length())).convert_to_double_use_with_caution(),
								     8
								     )
						    );

			    // remaining space
			    dataList->push_back(
						new ActivityData(
								 etime,
								 etime + request->get_timespan().convert_to_double_use_with_caution(),
								 ACTIVITY_PLOT_REGION_HEIGHT - ACTIVITY_BAR_HEIGHT - 8
								 )
						);

			    activityInfoList->push_back(info);
			}
		    }

		    // Let ActivityLayout work on it now

		    ActivityLayout* activityLayout = new ActivityLayout(*activityInfoList, nodeSpace);

		    // record for next use (multi-part plot)
		    activeLayout = activityLayout;
		    legendLayouts.insert(LegendActivityLayouts::value_type(legendNode, activeLayout));
		}
		else {
		    // this is a multi-part plotter page and this is at least the second piece
		    // therefore, we already have the information we need

		    activeLayout = iter->second;
		}

		fout << "\n% Legend:  " << legendNode->get_key() << endl;	// just for some debug info in the PS file

		// move origin to the legend activity area we are going to use and set up the clip

		fout << "gsave" << endl;
		fout << "newpath" << endl;	//Clockwise rectangle
		fout << LEGENDS_REGION_WIDTH << " " << yBase + verticalSpace << " moveto" << endl;
		fout << LANDSCAPE_PLOT_WIDTH << " " << yBase + verticalSpace << " lineto" << endl;
		fout << LANDSCAPE_PLOT_WIDTH << " " << yBase + verticalSpace + nodeSpace << " lineto" << endl;
		fout << LEGENDS_REGION_WIDTH << " " << yBase + verticalSpace + nodeSpace << " lineto" << endl;
		fout << "closepath clip" << endl;
		fout << LEGENDS_REGION_WIDTH << " " << yBase + verticalSpace + nodeSpace << " translate" << endl;
		fout << "1 -1 scale" << endl;	// flip to 'normal' orientation
		fout << endl;	// just some visual blank space for debugging

		// draw activities on the plot page
		
		PlotEnumerator plotEnumerator(fout, sec_per_pixel, plotStartTime, plotEndTime, pagestart, pageend);	// object to draw each bar (minus text)
		activeLayout->enumerate(plotEnumerator);
		
		// Now overwrite the bars (if necessary) with text.  Thus the bars do not obliterate the name of the bar
		// but text may still overwrite other text.
		
		fout << "0 0 0 setrgbcolor % back to black for the text" << endl;
		TextEnumerator textEnumerator(fout, sec_per_pixel, plotStartTime, plotEndTime, pagestart, pageend);	// object to draw each bar text
		activeLayout->enumerate(textEnumerator);
		
		fout << "grestore" << endl;	// restore original clipping region
	    }
	    if (activityInfoList != 0)
		delete activityInfoList;
	    activityInfoList = 0;
	}

	verticalSpace += nodeSpace;	// move to next legend entry
    }

    //Restore pre-clip (for activity-instance plot region) context
    fout << "grestore % end plot activities" << endl << endl;

    // cleanup
    for (LegendActivityLayouts::iterator iter = legendLayouts.begin(); iter != legendLayouts.end(); iter++)
	delete iter->second;

}

//Plot resources for THIS vert-stacked page, within time span of this horiz-
//  split page.  Unlike activity and violation plotting, this re-uses resource
//  plotting code, and routes to PostScript device rather than X/Motif device.
void IO_plotter::plot_resources(	int		nresources,
					res_plot_info	*res_plots,
					ofstream	&fout,
					int		vpage,
					int		pages_per_display_width,
					int		page) {
	int i;
	int boundary;
	Cstring temp_fout;
	bool firstBoundary = true;	// remember when we need to put in a bolder dividing line to separate from activities

	for (i = 0; i < nresources; i++) {
		if (res_plots[i].vpage == vpage ) {	//plot is on current vert-page
			//plot current resource in space allocated,including boundary lines
	
			//top boundary line
			boundary = (int) LANDSCAPE_PLOT_HEIGHT - res_plots[i].vert_space_left;
			if (firstBoundary) {
			  // draw fat separator
			  firstBoundary = false;
			  fout << "gsave 2 setlinewidth newpath 0 " << boundary << " moveto "
			       << LANDSCAPE_PLOT_WIDTH << " " << boundary << " lineto closepath stroke grestore\n";
			}
			else {
			  fout << 0 << " " << boundary << " " << LANDSCAPE_PLOT_WIDTH << " " << boundary << " " << " line" << endl;
			}
		
			//Plot the legend and canvas; Device constructor/destructor will do
			//  all necessary transformations and clipping, so no need here.
			//  All values cast as int's *SHOULD BE* doubles with int values,
			//  so that no information is lost.
			temp_fout.undefine();
			res_plots[i].cur_mw_object->plot_graphics(
						temp_fout,
						0,
						(int) LEGENDS_REGION_WIDTH,
						boundary,
						(int) LEGENDS_REGION_WIDTH,
						(int) (LANDSCAPE_PLOT_WIDTH - LEGENDS_REGION_WIDTH),
						res_plots[i].adj_res_height,
						pages_per_display_width,
						page,
						PLOTTER_RESOLUTION_IN_POINTS);
			fout << temp_fout << endl;
	
			//bottom boundary line
			boundary += res_plots[i].adj_res_height;
			fout << 0 << " " << boundary << " " << LANDSCAPE_PLOT_WIDTH << " " << boundary << " " << " line" << endl;
			}
		}
}

//Finish up current vert/horiz page, including all bold text, page titles and
//  identification, and page and region borders (kept here so that FUTURE may
//  allow fancier border contexts, e.g. dotted or colored lines).  Also do
//  limited finish work for the header page (signaled by page==vpage==0).
void IO_plotter::wrap_up_one_page(int vertical_page_count, int page,
				   int vpage, char * page_header, char * page_subhead,
				   ofstream & fout, int pages_per_display_width, CTime pagestart,
				   TIMEZONE timezone_button, CTime pageend, List & list_of_legends,
				  const Cstring & print_time) {
    int i;
    bool is_header_page;

    if ((page == 0) || (vpage == 0)) {	//page,vpage are 1-indexed, so use 0's to mark header page
	is_header_page = true;
	page  = 0;
	vpage = 0;
    }
    else
	is_header_page = false;

    DBG_INDENT("IO_plotter::wrap_up_one_page for Page " << page
	       << page_letters[(vpage <= page_letters_count) ? vpage /* prints letter */ : (page_letters_count+1)] //"*"
	       << endl);

    //Plot identification and summary information (includes page number)
    if (vertical_page_count == 1) {
	fout << "(" << "Page " << page << " of " << pages_per_display_width
	     << "              " << get_apgen_version_build_platform()	;
    }
    else {
	fout << "(" << "Page " << page
	     << page_letters[(vpage <= page_letters_count) ? vpage /* prints letter */ : (page_letters_count+1)] //"*"
	     << " of " << pages_per_display_width << "A-"
	     << page_letters[(vertical_page_count <= page_letters_count) ? vertical_page_count /* letter */ : (page_letters_count+1)] //"*"
	     << "          " << get_apgen_version_build_platform();
    }
    for (i = 0; i < (93 - (int)strlen(get_apgen_version_build_platform())); i++)
	fout << " ";
    fout << "Printed " << print_time << ")" << 0 << " "
	 << LANDSCAPE_FULL_HEIGHT << " text" << endl;

    if (!is_header_page) {	//non-header pages ONLY
	//Print start & end times and timezone (no need for block, or to clip);
	//  switch first to BOLD FONT (same size, PostScript taken from header)
	fout << "/Courier-Bold findfont [1.00 0 0 -1.00 0 0] makefont"
	     << " 8 scalefont setfont" << endl;
	Cstring scet;
	pagestart.time_to_word_date_SCET_string(scet, timezone_button);
	fout << "(" << scet << ") " << LEGEND_X_OFFSET << " "
	     << TIMECORNER_LINE1_Y_POSITION << " text" << endl;
	fout << "(" << pagestart.to_string(timezone_button, false)
	     << ") " << LEGEND_X_OFFSET << " "
	     << TIMECORNER_LINE2_Y_POSITION << " text" << endl;
	fout << "(" << timezone_to_abbrev_string(timezone_button.zone) << ") "
	     << TIMEZONE_STRING_OFFSET << " "
	     << TIMECORNER_LINE2_Y_POSITION << " text" << endl;

	fout << "( to) "
	     << LEGEND_X_OFFSET << " "
	     << TIMECORNER_LINETO_Y_POSITION << " text" << endl;

	pageend.time_to_word_date_SCET_string(scet, timezone_button);
	fout << "(" << scet << ") " << LEGEND_X_OFFSET << " "
	     << TIMECORNER_LINE3_Y_POSITION << " text" << endl;
	fout << "(" << pageend.to_string(timezone_button, false)
	     << ") " << LEGEND_X_OFFSET << " "
	     << TIMECORNER_LINE4_Y_POSITION << " text" << endl;
	fout << "(" << timezone_to_abbrev_string(timezone_button.zone) << ") "
	     << TIMEZONE_STRING_OFFSET << " "
	     << TIMECORNER_LINE4_Y_POSITION << " text" << endl;

	//Constraint Violations area title also in BOLD (if constraints active)
	if (APcloptions::theCmdLineOptions().constraints_active) {
	    // fout << "( Constraint Violations:) "

	// fout << "( Violations: " << CON_exec::violations().get_length() << ")"
	// 	 << LEGEND_X_OFFSET << " "
	// 	 << (TIMESCALE_REGION_HEIGHT + VIOLATIONS_TEXT_Y_POSITION)
	// 	 << " text" << endl;
	}
    }

    //Page header & subheader (passed in by caller)
    // Note (18+12) reference elsewhere refers to corresponding values here
    fout << "/Helvetica findfont [1.00 0 0 -1.00 0 0] makefont" << " 18 scalefont setfont" << endl;
    fout << "(" << page_header  << ") " << (LANDSCAPE_PLOT_WIDTH / 2.) << " " << LANDSCAPE_HEADER_Y_OFFSET  << " xcentertext" << endl;
    fout << "/Helvetica findfont [1.00 0 0 -1.00 0 0] makefont" << " 12 scalefont setfont" << endl;
    fout << "(" << page_subhead << ") " << (LANDSCAPE_PLOT_WIDTH / 2.) << " " << LANDSCAPE_SUBHEAD_Y_OFFSET << " xcentertext" << endl;

    // Reset font back to the one we want for legend text
    // fout << "/Helvetica-Bold findfont [1.00 0 0 -1.00 0 0] makefont 8 scalefont setfont" << endl;

    //Page termination (includes page border, region borders, legend boxes)
    //Dotted lines look nice, & allow seeing ticks & box edges (or lack
    //  thereof) on boundaries, but they don't copy well, so leave as solid
    //  fout << "[0.24 1.44] 0 setdash" << endl;	//Use for all of these
    fout << 0 << " " << 0 << " " << LANDSCAPE_PLOT_WIDTH	//Page border
	 << " " << LANDSCAPE_PLOT_HEIGHT << " " << " rectangle" << endl;

    if (! is_header_page) {	//non-header pages ONLY
	fout << 0 << " " << TIMESCALE_REGION_HEIGHT	//Timescale area border
	     << " " << LANDSCAPE_PLOT_WIDTH << " " << TIMESCALE_REGION_HEIGHT
	     << " " << " line" << endl;
	fout << LEGENDS_REGION_WIDTH << " " << 0	//Legends area border
	     << " " << LEGENDS_REGION_WIDTH << " " << LANDSCAPE_PLOT_HEIGHT
	     << " " << " line" << endl;
	//Violations area border (title area only if constraints inactive):
	fout << 0 << " "
	     << (TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT)
	     << " "
	     << (APcloptions::theCmdLineOptions().constraints_active ? LANDSCAPE_PLOT_WIDTH : LEGENDS_REGION_WIDTH)
	     << " "
	     << (TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT)
	     << " " << " line" << endl;

	// Legends dividers draw box BOTTOMS, for this vertically-stacked page
	{
	    List_iterator	legendList(list_of_legends);
	    LegendObject	*legendNode;
	    Pointer_node	*legendPointer;
	    int verticalSpace = 0;
	    int currentPage = 1;
	    double y = 0;
	    const double yBase = TIMESCALE_REGION_HEIGHT + VIOLATIONS_REGION_HEIGHT;

	    // Skip legend entries until we get the ones for this page then
	    // plot legend BOTTOM lines for this legend entry
	    while (vpage >= currentPage && (legendPointer = (Pointer_node *) legendList.next())) {
		legendNode = (LegendObject *) legendPointer->get_ptr();
		int nodeSpace = legendPixelsToPoints(legendNode->get_official_height());	// determine this legend entry height in points
	
		if (verticalSpace + nodeSpace > ACTIVITY_PLOT_TOTAL_HEIGHT) {
		    // we're about to cross a page boundary.  Legend entries are not split across pages.  Put this one on the next page.
		    verticalSpace = 0;
		    currentPage++;
		}
	
		if (vpage == currentPage) {
		    // draw lines until we cross another page boundary
		    double yBottom = yBase + y + nodeSpace;
		    fout << "0 " << yBottom << " " << LEGENDS_REGION_WIDTH << " " << yBottom << " line % " << legendNode->get_key() << endl;
		    fout << "0 " << yBottom << " " << LANDSCAPE_PLOT_WIDTH << " " << yBottom << " thinline % " << legendNode->get_key() << endl;
		    y += nodeSpace;	// move to end of this legend entry (start of next)
		}
	
		verticalSpace += nodeSpace;	// remember where we are
	
	    }
	}
    }

    //Actual page termination
    fout << "EndPage" << endl << endl; //Resets setfont,setdash,etc

    DBG_UNINDENT("IO_plotter::wrap_up_one_page: DONE" << endl);
}
