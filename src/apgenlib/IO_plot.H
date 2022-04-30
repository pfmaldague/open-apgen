/*
 * PROJECT: APGEN
 * FILE NAME: IO_plot.H
 *
 * ABSTRACT
 *
 * This file defines the hardcopy plotting classes for Hardcopy Activity
 *   Display (HAD).
 *
 *DATE    NAME              VERSION           REASON FOR CHANGE
 *------------------------------------------------------------------------
 *6/02/95 D. Glicksberg      1.0              Initial design
 *6/12/95 D. Glicksberg			      Add legends list,header,trailer
 *6/13/95 D. Glicksberg			      Add attributes list to private
 *6/15/95 D. Glicksberg			      Add 2 args to generate_plot
 *7/05/95 D. Glicksberg			      Change IO_comm.H to correct .h
 *
 *
 *
 */

#ifndef IOPLOT_H
#define IOPLOT_H

#include "C_list.H"
#include "UTL_time.H"
#include <iostream>
#include <fstream>


class RD_sys ;
class MW_object ;

//Holder for miscellaneous info needed to do a Resource Plot (1 per RD_sys)
struct res_plot_info {
			/// handle on current MW_object
	MW_object	*cur_mw_object ;			
	int		vpage;				//vertical-page # plot is on
	int		vert_space_left;		//top of plot (in full points)
	int		adj_res_height;			//plot height (in full points)
	};

class ACT_sys;		//forward reference avoids circular include "ACT_def.H"

class IO_plotter {
private:
    ///
    static const char *header[];	///PostScript header  for APGEN plot job
    static const char *trailer[];	///PostScript trailer for APGEN plot job
protected:
public:
    IO_plotter();
    ///
    ~IO_plotter();

    // Call to set dimensions of the PostScript plot page
    void setPlotPageDimensions(double width, double height, double leftMargin = 0.50, double topMargin = 0.50);

    // Call to turn on or off the plot summary
    void showPlotSummary(bool showSummary);	// true if you want it.  defaults to true.

    // UI calls this method when the user wants to print (plot) a HAD
    apgen::RETURN_STATUS generate_plot(const Cstring & theFileName,
				ACT_sys		*selected_activity_system,
				RD_sys		*theResSys ,
				List&		list_of_selected_resource_displays, // pointers to MW_objects now
				TIMEZONE	timezone_button,
				bool		plot_resource_displays,
				bool		plot_all_legends,
				bool		send_to_printer,
				int		pages_per_display_width,
				int		page_format_choice,
				char		*page_header,
				char		*page_subhead ) ;
    ///
    void plot_header_page(ACT_sys* selected_activity_system,
			  std::ofstream& fout, int nlegends, int nresources,
			  int pages_per_display_width, int vertical_page_count,
			  CTime plotstart, CTime plotspan, TIMEZONE timezone_button,
			  res_plot_info* res_plots, List& list_of_selected_resource_systems);
    ///
    void plot_timescale(std::ofstream& fout, CTime pagestart, CTime pagespan, CTime majors,
			CTime minors, CTime offset, CTime sec_per_pixel,
			TIMEZONE timezone_button);
    ///
    void plot_violations( std::ofstream& fout, CTime pagestart, CTime pageend, CTime sec_per_pixel);
    ///
    void plot_legends(List& list_of_legends, std::ofstream& fout, int vpage);
    ///
    void plot_activities(ACT_sys* selected_activity_system,
			 List& list_of_legends, std::ofstream& fout, int vpage,
 			 CTime plotStartTime, CTime plotEndTime,
			 CTime pagestart, CTime pageend, CTime sec_per_pixel, int page);
    ///
    void plot_resources(int nresources,
			res_plot_info* res_plots, std::ofstream& fout, int vpage,
			int pages_per_display_width, int page);
    ///
    void wrap_up_one_page(int vertical_page_count, int page, int vpage,
			  char* page_header, char* page_subhead, std::ofstream& fout,
			  int pages_per_display_width,
			  CTime pagestart, TIMEZONE timezone_button, CTime pageend,
			  List& list_of_legends, const Cstring& print_time); };

#endif
