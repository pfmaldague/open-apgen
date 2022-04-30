#ifndef UI_POPUPS_H
#define UI_POPUPS_H

#include "ACT_sys.H"
#include <action_request.H>
#include "UI_motif_widget.H"
#include "UI_GeneralMotifInclude.H"
#include "UTL_time.H"
#include "IO_ApgenDataOptions.H"
#include "EventRegistry.H"

class activity_display;
class PAUSErequest;

                        // in UI_motif_widget.C:
extern resource_data    ResourceData;

//Maximum reasonable resource background points should be approx. == screen
//  width in pixels; any more points, and extra resolution is wasted.  Since
//  this is used in UI (Background Points slider), want a nice round number.
#define MAX_BACKGROUND_PTS 1000

class rd_popup: public motif_widget {
public:
    rd_popup(
    	const Cstring	&name,			//will be name of top widget
    	motif_widget	*parent_widget,		//parent of this widget
	const Cstring	&text_for_popup		//text (multiline) to display
    	);
    ~rd_popup();

    // UTILITY:

    Node		*copy() { return new rd_popup(*this); }


    // MOTIF CALLBACKS:

    static void		CancelButtonCallback(Widget, callback_stuff*, void*);

    /*widget hierarchy (indent 1 space per level of parentage,as in an outline)
     *  of POINTERS to motif_widget's (PFM reasons:  need explicit "new" to
     *  control timing w/r/t X Widget hierarchy construction; need deletion of
     *  Widgets to happen via that hierarchy (could get conflicts if automatic
     *  destruction of each motif_widget when it went out of scope) */
    motif_widget		*rdPopupForm;		//child of DialogShell; un/manage THIS!
    motif_widget		*rdPopupLabel;		//fixed label
    multi_line_text_widget	*rdPopupTextMultiline;	//multiline text with info
    motif_widget		*rdPopupSeparator;	//separator(for APGEN std look)
    motif_widget		*rdPopupButtonForm;	//Form to contain Button(s)
    motif_widget		*rdPopupCancelButton;	//Dismisses popup
    motif_widget		*rdPopupDummyForm;	//Form to absorb excess space

    Cstring			textMultiline;

    callback_stuff		data_for_window_closing; };

class con_popup: public motif_widget {
public:
    con_popup(
    	const Cstring	&name,			//will be name of top widget
    	motif_widget	*parent_widget,		//parent of this widget
	const Cstring	&text_for_popup		//text (multiline) to display
    	);
    ~con_popup();

    // UTILITY:
    Node*		copy() { return new con_popup(*this); }


    // MOTIF CALLBACKS:

    static void		CancelButtonCallback(Widget, callback_stuff *, void *);
    static void		NextPrevButtonCallback(Widget, callback_stuff *, void *);

    /* Widget hierarchy (indent 1 space per level of parentage, as in an outline)
     *  of POINTERS to motif_widget's (PFM reasons:  need explicit "new" to
     *  control timing w/r/t X Widget hierarchy construction; need deletion of
     *  Widgets to happen via that hierarchy (could get conflicts if automatic
     *  destruction of each motif_widget when it went out of scope) */
				// parent of all that stuff; allows sensible resizing
    motif_widget		*paned_window;
    				// 3-line total-violations info
    multi_line_text_widget	*conPopupTextMultiline;
    motif_widget		*conPopupForm;		//container for more stuff
    motif_widget		*conPopupLabel;	//fixed label
    motif_widget		*conPopupTextScrolled; //vert-scrolling multiline info
    motif_widget		*conPopupSeparator;	//separator(for APGEN std look)

				//Form to contain Button(s)
    motif_widget		*conPopupButtonForm;
    				//Dismisses popup
    motif_widget		*conPopupCancelButton;
    				// Moves to previous violation (if any)
    motif_widget		*conPopupPrevButton;
    				// Moves to next violation (if any)
    motif_widget		*conPopupNextButton;
    				// Holds next, prev buttons
    motif_widget		*moveToNextPreviousForm;

	Cstring			textScrolled;
				// sets the centertime
	void			set_centertime(CTime);

    				/* Remembers where in the constraint display the
				* user clicked for info */
	CTime			centertime;

    				/* To allow user to close window by clicking on
				 * the cross in the upper right corner */
	callback_stuff		data_for_window_closing; };

class seq_review_popup: public motif_widget {
public:
	seq_review_popup(
		const Cstring	&name,                   //will be name of top widget
        	motif_widget	*parent_widget,          //parent of this widget
        	const Cstring	&text_for_popup          //text (multiline) to display
		);
	~seq_review_popup();
	motif_widget		*top_form, *paned_window, *top_widget;
	motif_widget		*start_time, *end_time;
				// 0 if seq_review + its cfg file can be found, 1 otherwise:
	int			hopeless;
				// original start, end times:
	CTime			S, E;
	Cstring			conv_file;

	callback_stuff		data_for_window_closing;

	static void		CancelButtonCallback(Widget, callback_stuff*, void*);
	static void		OKButtonCallback(Widget, callback_stuff*, void*);
	virtual void		OKcallback() {}
	virtual void		update_times(CTime & ST, CTime & EN) = 0;
	void			update_time_widgets(); };

class tol_popup: public seq_review_popup {
public:
	tol_popup(
		const Cstring	&name,                   //will be name of top widget
        	motif_widget	*parent_widget,          //parent of this widget
        	const Cstring	&text_for_popup          //text (multiline) to display
		);
	~tol_popup();
	scrolled_list_widget	*filters;
	scrolled_list_widget	*formats;
	List			format_list;
	List			filter_list;
	motif_widget		*fileNameWidget;
	void			update_times(CTime &ST, CTime &EN);

	virtual void		OKcallback(); };

class sasf_popup: public /* seq_review_popup */ motif_widget {
public:
	motif_widget		*top_form, *paned_window, *top_widget;
	motif_widget		*start_time, *end_time;
				// original start, end times:
	CTime			S, E;
	callback_stuff		data_for_window_closing;

	sasf_popup(
		const Cstring	&name,                   //will be name of top widget
        	motif_widget	*parent_widget           //parent of this widget
		);
	~sasf_popup();
	scrolled_list_widget    *files;		// the file list as displayed to the user.
	slave_list_widget    	*actual_files;		// the actual file list as displayed to the user.
	slave_list_widget    	*whether_selected;	// the selection status as displayed to the user.
	motif_widget		*fileNameWidget, *selected_button, *UpdateButton;
	motif_widget		*includeExactEndTime, *excludeExactEndTime;
	int			thePositionOfTheSelectedItem;

	int			create_sasf_file_list(Cstring &allErrors);
	void			update_times(CTime & ST, CTime & EN);
	virtual void		OKcallback();
	static void		SASFcallback(Widget, callback_stuff * client_data, void *);
	static void		CancelButtonCallback(Widget, callback_stuff*, void*);
	static void		OKButtonCallback(Widget, callback_stuff*, void*);
	void			update_time_widgets(); };

class popup_list: public motif_widget, public user_waiter {
public:
	popup_list(
		const Cstring	&name,			// will be name of top widget
        	motif_widget	*parent_widget,		// parent of this widget
		callback_proc	proc_to_call,		// call when OK pushed
		void		*data,
		long		apply_button_desired,
		int		hold_creation = 0,
		void		(*UWaiter_func)(void *) = NULL,
		void		*UWaiter_data = NULL
		);
	~popup_list();

				// apply_button is optional and initialized to NULL:
	motif_widget		*top_form, *paned_window, *Parent, *apply_button, *OKbutton;
	motif_widget		*cancel_button;
	scrolled_list_widget	*scrolled_list;
	single_line_text_widget	*chosen_text;
				/*
				 * Values:
				 * 	1	new_activity (regular)
				 * 	2	add_resource
				 * 	3	new_activity (select parent)
				 * 	0	the default: all other cases (not worried about subsystems)
				 */
	int			theTypeOfPopup;

	virtual void		Create(int theFlag = 0);
	virtual motif_widget*	get_the_radiobox() { return NULL; }

	callback_stuff		data_for_window_closing;
	callback_proc		ok_proc;
	tlisttlist		listOfSubsystems;

	virtual void		updateAllLists() {}
	void			updateMainList(stringtlist&, bool please_order = false);
	void*			Data;
	static void		CancelButtonCallback(Widget, callback_stuff *, void *);
	static void		OKButtonCallback(Widget, callback_stuff *, void *);
	static void		ApplyButtonCallback(Widget, callback_stuff *, void *);
	static void		UpdateChoice(Widget, callback_stuff *, void *);
	virtual void		OKcallback() {}
	};

class add_resource_popup: public popup_list {
public:
	add_resource_popup(
		const Cstring&	name,			// will be name of top widget
        	motif_widget*	parent_widget,	// parent of this widget
		callback_proc	proc_to_call,		// call when OK pushed
		void*		data);
	~add_resource_popup();
				// for listing subsystems:
	motif_widget*		scrolled_list2;
	apgen::RETURN_STATUS	fill_in_subsystems();
				// checks the list of Taglist_objects
	bool			subsystems_are_out_of_sync();
				// computes the list of Taglist_objects for resources
	apgen::RETURN_STATUS	compute_resource_subsystems(
					tlistslist& list_to_fill,
					int& Nres);
	static void		UpdateSubsystem(Widget, callback_stuff *, void *);
	void			updateLowerList(int pos, int pos_selected);
	void			updateAllLists();
	void			Create(int theFlag = 0);
	};

class pause_popup: public popup_list {
public:
	static void	auto_callback(void *);
	pause_popup(
			const Cstring	&name,			// will be name of top widget
        		motif_widget	*parent_widget,	// parent of this widget
			callback_proc	proc_to_call) :	// call when OK pushed
		popup_list(	name,
				parent_widget,
				proc_to_call,
				NULL,
				0,
				/* hold creation */ 1,
				auto_callback,	// in case we are in automatic mode
				this
				) {}
	~pause_popup() {}
	motif_widget		*multi;
	int			i_am_a_pause_popup();
	int			this_is_the_last_time();
	void			Create(int theFlag = 0);
	callback_proc		cancel_proc; };

class explanation_popup: public popup_list {
public:
	explanation_popup(
		const Cstring	&name,			// will be name of top widget
        	motif_widget	*parent_widget) :	// parent of this widget
		popup_list(name, parent_widget, NULL, NULL, 0, /* hold creation */ 1) {}
	~explanation_popup() {}
	motif_widget		*multi;
	List			theExplanations;
	Cstring			theExplainee;
	static void		UpdateExplanation(Widget, callback_stuff *, void *);
	static void		CancelExplanation(Widget, callback_stuff *, void *);
	void			set_explanations(const List&);
	void			Create(int theFlag = 0); };

	/* This class implements 2 popup panels: the "Define New Activity" panel and
	 * the "Group Activities" panel. Which of the two is created depends not on
	 * the constructor (there is only one), but on the argument to the create()
	 * method, which can be 1 (def new act) or 2 (group acts). 
	 *
	 * The create method in turn sets theTypeOfPopup, a member of the base popup_list
	 * class, to 3 for a "group activities" panel and to 1 for a "define new act."
	 * panel. 
	 *
	 * For the 'group activities' panel, one of the options is for the user to select
	 * an activity as the parent of the new group. We want the OK button to only light
	 * up after the user has made the selection. To achieve this, we base the class on
	 * EventRegistryClient class, which means we have to implement the Event method.
	 */
class new_act_popup: public popup_list, public EventRegistryClient {
public:
	new_act_popup(
		const Cstring	&name,			// will be name of top widget
        	motif_widget	*parent_widget,	// parent of this widget
		callback_proc	proc_to_call,		// call when OK pushed
		void		*data,
		long		apply_button_needed);
	~new_act_popup();

	motif_widget		*useExistingActivity, *createNewActivity, *createNewRequest, *createNewChameleon;
	motif_widget		*radiobox, *scrolled_list2, *user_instructions;
	apgen::RETURN_STATUS		fill_in_subsystems();
				// checks the list of tlistNodes
	bool			subsystems_are_out_of_sync();
				// computes the list of tlistNodes for activities
	apgen::RETURN_STATUS	compute_activity_subsystems(
					tlisttlist& list_to_fill, int& Nacts,
					bool cham_only, bool templ_only);
				// static
	static void		UpdateSubsystem(Widget, callback_stuff *, void *);
				// non-static
	void			updateLowerList(int pos, int pos_selected);
				// Used in UI_exec.C:
	motif_widget		*get_the_radiobox() { return radiobox; }
	void			updateAllLists();
				// Creates either a "New Activity" or a "Group Activities" panel:
	void			Create(int theFlag);
				// Pure virtual function of the EventRegistryClient interface.
	void			HandleEvent(const string&, const TypedValuePtrVect&);
				// Callback used when user changes the state of the radiobox.
	static void		creation_method_callback(Widget, callback_stuff *, void *); };

class popup_scrolled_text: public motif_widget {
public:
	popup_scrolled_text(
		const Cstring	&name,			// will be name of top widget
        	motif_widget	*parent_widget,		// parent of this widget
		callback_proc	proc_to_call,		// call when OK pushed
		void		*data
		);
	~popup_scrolled_text();
	motif_widget		*top_form, *paned_window, *Parent;
	motif_widget		*ok_button, *cancel_button, *chosen_text;

	callback_stuff		data_for_window_closing;
	callback_proc		ok_proc;
	void			*Data;
	static void		CancelButtonCallback(Widget, callback_stuff *, void *);
	static void		OKButtonCallback(Widget, callback_stuff *, void *);
	virtual void		OKcallback() {}
	virtual void		add_more_panels() {}
	virtual void		create(); };

class text_and_list_popup: public motif_widget {
public:
        text_and_list_popup(
                const Cstring	&name_of_the_top_widget_of_this_object,
                motif_widget	*parent_of_this_widget,
		callback_proc	Cancel_callback_func,
		callback_proc	OK_callback_func,
		const Cstring	&TextFieldLabel,
		const Cstring	&ListLabel,
		const Cstring	&OKbuttonLabel
               );
        ~text_and_list_popup() {}
        motif_widget            *paned_window;
        motif_widget            *name_of_new_plan_file, * file_list;
	motif_widget		*OKbutton;

        callback_stuff          data_for_window_closing;
	void			updateMainList(const stringslist&); };

class UI_integrate: public text_and_list_popup {
public:
        UI_integrate(
                const Cstring	&name_of_the_top_widget_of_this_object,
                motif_widget	*parent_of_this_widget,
		const Cstring	&TextFieldLabel,
		const Cstring	&ListLabel,
		const Cstring	&OKbuttonLabel
               );
        ~UI_integrate() {}
	static void		initialize();
        static void             CancelButtonCallback(Widget, callback_stuff *, void *);
        static void             OKButtonCallback(Widget, callback_stuff *, void *);
        };

//
class UI_partial_save: public text_and_list_popup
        {
public:
        UI_partial_save(
                const Cstring	&name_of_the_top_widget_of_this_object,
                motif_widget	*parent_of_this_widget,
		const Cstring	&TextFieldLabel,
		const Cstring	&ListLabel,
		const Cstring	&OKbuttonLabel
               );
        ~UI_partial_save() {}
	static void		initialize();
        static void             CancelButtonCallback(Widget, callback_stuff *, void *);
        static void             OKButtonCallback(Widget, callback_stuff *, void *);
        };


class IO_ApgenSoDataClient;

class UI_new_activities: public motif_widget {
  motif_widget* Paned_window;
  callback_stuff Data_for_window_closing;

  void SetSubsystems();
  void SetActivityTypes(const string& subSystemName);
  void ClearActivityTypes();
  void InstancePanelInit();
  void InstancePanelUnInit();

  scrolled_list_widget* Subsystem_list;
  scrolled_list_widget* ActivityTypes_list;

  motif_widget *Label_start;
  single_line_text_widget* Start_time;
  motif_widget *Label_times;
  single_line_text_widget* Num_times;
  motif_widget *Label_period;
  single_line_text_widget* Period;
  motif_widget* OKButton;
  motif_widget* ApplyButton;
  void ApplyCurrentValues();
  Cstring ActivityType;

public:

  UI_new_activities(motif_widget *parent_of_this_widget);
  ~UI_new_activities();

  static void SubsystemSelect(Widget, callback_stuff *, void *);
  static void ActivityTypeSelect(Widget, callback_stuff *, void *);
  static void CancelButtonCallback(Widget, callback_stuff *, void *);
  static void OKButtonCallback(Widget, callback_stuff *, void *);
  static void ApplyButtonCallback(Widget, callback_stuff *, void *);
  void Show();
  void Hide();
  void ACT_sysInput(ACT_sys* which, const XEvent* event);

};


class UI_rdBackgroundPts : public motif_widget {
protected:
	virtual void	resetInfo();

			// Holds the name of the resource of interest
	static Cstring	theSavedResource;

			// Holds the previous value of the resolution for the resource of interest
	static int	saved_resolution;
	static void	*current_RES_sys;

public:
	virtual void	create();
	UI_rdBackgroundPts(
	    	const Cstring	&name,			//will be name of top widget
	    	motif_widget	*parent_widget,		//parent of this widget
		callback_proc	proc_to_call		//call when OK pushed
	    	);
	~UI_rdBackgroundPts() {}

	static void initialize(void *cur_RES_sys, int res, const Cstring& the_resource_name);

	//widget hierarchy (indent 1 space per level of parentage,as in an outline)
	motif_widget	*_UI_rdBackgroundPts; //DialogShell child;un/manage it
	motif_widget	*_rdBackgroundPtsButtonForm;
	motif_widget	*_rdBackgroundPtsOKButton;
	motif_widget	*_rdBackgroundPtsCancelButton;
	motif_widget	*_rdBackgroundPtsResetButton;
	motif_widget	*_rdBackgroundPtsSeparator1;
	motif_widget	*_rdBackgroundPtsResScale;

	callback_stuff	data_for_window_closing;
	callback_proc	ok_proc;

	static void rdbResetButtonCallback(Widget, callback_stuff *, void *);
	static void rdbCancelButtonCallback(Widget, callback_stuff *, void *);
	static void rdbOKButtonCallback(Widget, callback_stuff *, void *);
	static void rdbOKCallback(Widget, callback_stuff *, void *); };
	
class UI_rdOptions : public UI_rdBackgroundPts {
protected:
	void resetInfo();
	static int		saved_mode;
public:
	void		create();
    UI_rdOptions(
    	const Cstring	&name,			//will be name of top widget
    	motif_widget	*parent_widget,		//parent of this widget
	callback_proc	proc_to_call		//call when OK pushed
    	);
    ~UI_rdOptions() {}

    static void initialize(RD_YSCROLL_MODE mode, int res);

    motif_widget	 *_rdOptionsResLabel;
    motif_widget	 *_rdOptionsSeparator2;
    motif_widget	 *_rdOptionsControlRB;
    motif_widget	 *_rdOptionsManualTB;
    motif_widget	 *_rdOptionsAutoWindowTB;
    motif_widget	 *_rdOptionsAutoPlanTB;
    motif_widget	 *_rdOptionsAutoRangeTB;
    motif_widget	 *_rdOptionsControlLabel;

    static void rdoOKCallback(Widget, callback_stuff *, void *); };

class UI_newLegend : public motif_widget {
protected:
	static void	nlApplyButtonActivate(Widget, callback_stuff *, void *);
	static void	nlCancelButtonActivate(Widget, callback_stuff *, void *);
	static void	nlOKButtonActivate(Widget, callback_stuff *, void *);

apgen::RETURN_STATUS	applyChanges ();

	callback_stuff	data_for_window_closing;
	callback_proc	ok_proc;

public:
	UI_newLegend(
	    	const Cstring	&name,			//will be name of top widget
	    	motif_widget	*parent_widget,	//parent of this widget
		callback_proc	proc_to_call		//call when OK pushed
	    	);
	~UI_newLegend()	{}

	static void initialize ();

	//widget hierarchy (indent 1 space per level of parentage,as in an outline)
	motif_widget		*_UI_newLegend; //DialogShell child;un/manage it
	motif_widget		*_nlLabel;
	motif_widget		*_nlLegendTF;
	motif_widget		*_nlSeparator;
	motif_widget		*_nlButtonForm;
	motif_widget		*_nlCancelButton;
	motif_widget		*_nlApplyButton;
	motif_widget		*_nlOKButton;
	motif_widget		*LegendHeightLabel, *LegendHeightTextField;

    static void	nlOKCallback(Widget, callback_stuff *, void *); };

class hopper_popup: public motif_widget {
public:
	motif_widget		*_menuBar, *_exportButton, *_exportButtonPM,
				*_toFileButton, *_toSubsystemButton,
				*_importButton, *_importButtonPM, *_fromFileButton, *_fromSubsystemButton,
				*_deleteLegendButton, *_newLegendButton,
				*_viewButton, *_viewButtonPM, *_zoomButton, *_zoomButtonPM,
				*_zoomInButton, *_zoomOutButton, *_zoomToFitButton,
				*_origColorsButton, *_hierarchyColorsButton,
				*_hierarchySelectionOnButton, *_hierarchySelectionOffButton, *_downSelectionOnButton,
				*_panedWindow, *_toolBarForm, *_hopperFrame, *_hopperForm,
				*_fileButton, *_fileButtonPM, *_fileCloseButton;

	callback_stuff		data_for_window_closing;

	hopper_popup(	const Cstring&	name,
			motif_widget*	parent_widget);
	~hopper_popup();

	activity_display	*AD;

	static void		CancelButtonCallback(Widget, callback_stuff *, void *);
	static void		OKButtonCallback(Widget, callback_stuff *, void *);

	static void		fileCloseCallback(Widget, callback_stuff *, void *);
	static void		viewCallback(Widget, callback_stuff *, void *);
	static void		zoomCallback(Widget, callback_stuff *, void *);
	void			OKcallback(); };
#endif
