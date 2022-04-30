#ifndef ACT_SYS_H
#define ACT_SYS_H

#include <iostream>

#include "apcoreWaiter.H"
#include "C_global.H"
#include "C_list.H"
#include "C_string.H"
#include "DSsource.H"
#include "RES_exceptions.H"
#include "UI_motif_widget.H"
#include "UTL_defs.H"
#include "UTL_time.H"
#include "UI_ds_timeline.H"

class ACT_type;
class ACT_exec;
class activity_display;
class commbox;
class CON_sys;
class DS_graph;
class DS_lndraw;
class IO_writer;
class RES_resource;
class UI_mainwindow;


#define SWAPUP   1
#define SWAPDOWN 2

#define				CLICK_INTERVAL 250

//click context enum
enum context_for_click_event {
  OTHER,
  SECOND_CLICK_OF_A_FAST_PAIR,
  SELECT_W_FIRST_BUTTON,
  INITIATE_AN_ACTIVITY_DRAG_ACTION,
  RESHUFFLE_ACTIVITY_STACK,
  DISAMBIGUATE_ACTIVITY_STACK 
};

class VerticalLine {
	CTime Time;
	bool OutOfSync;
	DS_lndraw* Draw;

public:
	VerticalLine(CTime time, DS_lndraw* draw);
	CTime getTime() const;
	bool isOutOfSync() const;
	void setOutOfSync();
	void update_graphics();
	void refreshBlock(int ul_x1, int ul_y1, int lr_x2, int lr_y2);
};


typedef std::map<int, VerticalLine> VerticalLineMap;

class VerticalLineManager {
private:
	VerticalLineMap VerticalLines;
	bool OutOfSync;
public:
	VerticalLineManager();
	int addAVerticalLine(int handle, const CTime& lineTime, DS_lndraw* draw);
	void removeAVerticalLine(int lineHandle);
	bool isOutOfSync() const;
	void setOutOfSync();
	void update_graphics();
	void refreshBlock(int ul_x1, int ul_y1, int lr_x2, int lr_y2); };

extern bool	theWorkProcCplusplus(void *);

class ACT_sys : public DS_lndraw, public synchro::fixer {
protected:
	activity_display*	AD_parent;

				// for Hopper design:
	Cstring			HopperID;

				// blist of bpointernodes to DS_lines that need refreshing:
	blist			thePointersToTheNewDsLines;

				/*
				 * Used by ActivateTheDelayedWorkProc(), TheDelayedWorkProcHasBeenActivated() and ResetTheDelayedWorkProc() to
				 * communicate between theWorkProc(), which moves things around and detects the need for a global change, and
				 * theDelayedWorkProc(), which implements global changes once every half-second at the most so we don't keep
				 * flashing changes forever... */
	int			delayed_flag;

public:

	ACT_sys(	const Cstring		&name,
			motif_widget		*parent_assumed_to_be_a_form,
			activity_display	*myAD,
			list_of_siblings	*);
	~ACT_sys();
	activity_display*	get_AD_parent() const { return AD_parent; }

	void			cleargraph();
	void			manage();
	int			is_horizontal() { return 1; }
	int			is_vertical() { return 1; }

				// Used by theWorkProc():
	// void			activateTheDelayedWorkProc();
	// int			TheDelayedWorkProcHasBeenActivated();
	// void			resetTheDelayedWorkProc();
	int			NumberOfLinesActuallyDrawn;
	int			activeCount, decomposedCount, abstractedCount;
	//			already implemented in DS_graph:
	// const Cstring&	detectorName() const { return get_key(); }
	const Cstring&		fixerName() const { return get_key(); }

	static synchro::problem* checkActivityCount(synchro::detector*);
	static void		fixActivityCount(synchro::problem*);
	static synchro::problem* checkForActivityListProblem(synchro::detector*);
	virtual synchro::problem* detectActivityListProblem();

	static int		verticallyEnabled;
				// Pointer to hoppers if they exist.
	static ACT_sys		*theHopper;
	static void		purge();
	static blist		getTheRelevantActSisses(const Cstring &HID);
				/* Extracts a hopper name from the string, if applicable.
				 * For a non-"" hopper name, the string has to be of the
				 * form "<number>Hopper:" */
	static Cstring		extractHopper(const Cstring &HID);

				// called by DS_line::draw().
	void			remove_pointer_to_line(DS_line *);

				// called by DS_line::newhcoor() and ~DS_line(). 
	void			update_windows(
					int old_sx, int old_ex, int y_top, int y_bottom,
					Lego *);

				/* called by DS_line::DS_line() (several constructors), and also by DS_line_owner::generate_windows()
				 * (which is itself called by Dsource::select() and Dsource::unselect()). */
	void			add_window_for(DS_line *);

				// used as a signal that it's ok to set the viewing window:
	bool			never_scrolled, never_configured;

				/** difference with DS_graph::draw() is that this
				 * one tells all siblings to draw as well:
				 8 (used to be in DS_lndraw) */
	void			update_graphics(callback_stuff *);
	virtual void		configure_graphics(callback_stuff *);

				// Create a display object after ACT request is given
	friend DS_line		*CreateNewDSline(ACT_sys *actsis, Dsource *newACT, DS_gc *gc);

	friend bool		theWorkProcCplusplus(void *);
	friend void		theDelayedWorkProc();
	friend int		weShouldWork();

	//*****DISPLAY    

	apgen::RETURN_STATUS	getviewtime(CTime & starttime, CTime & timespan);

	void			modify_selected_legends(int);

				//Support hard copy plotting:
	void			get_all_or_selected_legends(Boolean get_all_legends, List & new_list);

				// Supports writing mixed files when updating seqtalk participant
	static List		&get_all_legend_names();

	virtual const Cstring&	get_hopper_id() const {
		return Cstring::null();
	}

				// unused but kept around, just in case:
	void			sel_paste_legend(CTime	&stm, int y);

	//****Utilities
	virtual synchro::problem* detectSyncProblem();
				/** Checks that the list of pointers to top-level activities
				 * only contains pointers to activities without parent
				 * (only meaningful for giant windows) */
	virtual void		check_top_level_pointers() {}
	virtual void		clear_top_level_pointers() {}

				// !!
	Node			*copy() { return NULL; }

	CON_sys			*constraint_display;

	TIMEZONE		timezone;

	friend void create_activity_subsystem();
	friend void delete_activity_subsystem();
	friend void UpdateDSline(ACT_sys *, DS_line *, const DS_gc &);
	friend void UpdateDSline(ACT_sys *, DS_line *);
	friend void  Purge_model();
	friend class ACT_exec;
	friend class DS_line_owner;


	// statics are always there (batch or not):
	// static ACT_exec		*ACT_subsystem;

				//List of dumb pointers to ACT_sys
	static List		&Registration();
				// blist of btags pointing to ACT_sys, indexed by hopper
				// name (if of nonzero length)
	static blist		&Hoppers();

				// Master blist of Lego; it is an alphabetically ordered list
				// (its nodes are Lego, derived from bstringnode):
	// static blist		theLegends;

	VerticalLineManager     VertLineManager;

				//returns a line handle
	static int              addAVerticalLine(CTime& lineTime);
				//takes a line handle
	static int              removeAVerticalLine(int lineHandle);

	static void		addALegend(Lego *LO);
	static List_iterator	*getLegendListIterator();
	static Lego		*findTheLegend(const Cstring &t);

	//****Editing
				//Remove the selected activity requests to a CUT/COPY buffer 
	static apgen::RETURN_STATUS	sel_remove(	const List & selection_copy,
						const unsigned char copyflg,
						Cstring &any_errors);
				/** This has been turned into a non-static method so we can use different kinds
				 * of ACT-sisses. See notes in ACT_edit.C about this method.
				 *
				 * Modified so that is now takes an int flag, = 1 to schedule an act,
				 * = -1 to unschedule.  */
	// void			paste_activity(	const Cstring		&start_time_as_a_string,
	// 					const Cstring		&legend_name,
	// 	       				act_scheduled_state	change_status_to,
	// 					List			&list_of_created_IDs) throw(eval_error);
	// void			paste_one_instance(	Dsource		*above,
	// 						Dsource		*a,
	// 						int		leg_shift,
	// 						const CTime	&paste_start,
	// 						act_scheduled_state scheduleflag,
	// 						int		&updateflg);


				// The following functions are mainly used to pass info to other ACT_sys's in
				// the Registration list. This should be done through a 'list_of_peers' class:
	static void		enter_pastemode();
	static void		exit_pastemode();
	static void		enter_defmode();
	static void		exit_defmode();

				// global transfer methods:
	static void		TransferAllActivities(
						List& plan_files_to_include,
						aoString &,
						apgen::FileType	FileType,
						Cstring&	outputFileName,
						CTime start, CTime end);
	static apgen::RETURN_STATUS TransferSpecifiedSasfActivities(
						const List &list_of_ptrs_to_activities,
						const blist &list_of_symbolic_filenames,
						aoString &,
						CTime start_time, CTime end_time,
						int inclusion_flag,
						Cstring &error_msg);

				// list of btags:
	static int		transferAllLegends(blist &);

				// For writing legends into a regular APF
	static void		WriteAllLegendsToStream(List &list_of_plan_files, aoString &Stream);
				// For writing legend layout into a special APF
	void			WriteLegendLayoutToStream(
					std::ostream &Stream,
					const Cstring &LayoutTag);
				// To support execution of legend layout directives (ACT_exec::executeNewDirectives)
	static void		reorder_legends_as_per(const Cstring &layout_id, List &pointers_to_legends);
	static const Cstring	&getplanfilename();

				//**** called after reading a file:
	static void		SetViewTimeAndUpdateObjectsForAllRegisteredDisplays();

	//*****debug
	static void		dump(); };

class ACT_hopper: public ACT_sys {
public:
	// Constructor
	ACT_hopper(	const Cstring		&name,
			motif_widget		*parent_assumed_to_be_a_form,
			const Cstring		&theHopperName,
			activity_display	*myAD,
			list_of_siblings	*);
	// Destructor
	~ACT_hopper() {}

	bool			is_a_hopper() { return true; }
	const Cstring&		get_hopper_id() const { return HopperID; }
};

class actsis_iterator {
	Pointer_node		*ptr;
	btag			*theTag;
public:
	actsis_iterator();
	~actsis_iterator();

	ACT_sys		*operator ()(); };

#endif
