#ifndef _UI_save_H_
#define _UI_save_H_

#include <action_request.H>
#include "UI_motif_widget.H"
#include "UI_GeneralMotifInclude.H"
#include "IO_ApgenDataOptions.H"



class directive_options: public motif_widget, public user_waiter 
{
  
  // The type of these options: 'AAF', 'APF', or 'MIX'
  string Type;
  
public:
  directive_options(motif_widget *parent_widget);
  ~directive_options() {}
  
  motif_widget*		top_form;
  motif_widget*		top_label;
  motif_widget*		paned_window;
  motif_widget*		Parent;
  motif_widget*		apply_button;
  motif_widget*		cancel_button;
  motif_widget*		OKbutton;
  motif_widget*		radioboxes[6];
  
  callback_stuff	theDataForWindowClosing;
  callback_proc		ok_proc;
			/* Initializes the panel with a string specifying 'AAF', 'APF' or 'MIX'.
			 * This method loads the values from the preferences and displays the form */
  void			initialize(const string& type);
  static void		CancelButtonCallback(Widget, callback_stuff *, void *);
  static void		OKButtonCallback(Widget, callback_stuff *, void *);
  static void		ApplyButtonCallback(Widget, callback_stuff *, void *);
  static void		UpdateChoice(Widget, callback_stuff *, void *);
  virtual void		OKcallback() {} 
};

//The save file panel.
class UI_save: public motif_widget {

				//to get the old style options from prefs
	void			GetSaveOptionsListFromPrefs(tlist<alpha_string, optionNode>* listToSet) const;

	motif_widget*		paned_window;
	motif_widget*		apf_text;
	motif_widget*		apf_label;
	motif_widget*		aaf_text;
	motif_widget*		aaf_label;
	motif_widget*		mix_text;
	motif_widget*		mix_label;
	motif_widget*		radiobox;
	motif_widget*		save_in_separate_files;
	motif_widget*		save_in_one_file;
	motif_widget*		save_apf_only;
	// motif_widget*	save_plan_xml_only;
	motif_widget*		save_legend_layout;
	motif_widget*		OKbutton;
	motif_widget*		radiobox2;
	motif_widget*		save_everything;
	motif_widget*		frame1;
	motif_widget*		frame2;
	motif_widget*		frame3;
	motif_widget*		do_not_save_everything;
	motif_widget*		files_to_exclude;
	motif_widget*		exclude_label;
	motif_widget*		tag_label;
	motif_widget*		tag_text;
	callback_stuff          data_for_window_closing;

public:
  //constructor
  UI_save(
	  const Cstring&	name_of_the_top_widget_of_this_object,
	  motif_widget*		parent_of_this_widget
	 );
  //destructor
  ~UI_save() {}

  static UI_save*	ui_save;
  static void		initialize();
  static void		CancelButtonCallback(Widget, callback_stuff *, void *);
  static void		OKButtonCallback(Widget, callback_stuff *, void *);
  static void		HowManyFilesCallback(Widget, callback_stuff *, void *);
  static void		SaveEverything(Widget, callback_stuff *, void *);
  
			// updates files_to_exclude:
  void			updateMainList(const stringslist&);
  void			desensitize_file_list(); 
			//aaf options button
  motif_widget*		directiveOptions_aaf;
			//apf options button
  motif_widget*		directiveOptions_apf;
			//mix_options button
  motif_widget*		directiveOptions_mix;
			//the directive options popup
  directive_options*	directive_options_popup;
};

class pwd_popup: public motif_widget {
public:
    pwd_popup(
    	const Cstring& name			// will be name of top widget
    	);
    ~pwd_popup();

    // UTILITY:

    Node*	copy() { return new pwd_popup(*this); }


    // MOTIF CALLBACKS:

    static void		CancelButtonCallback(Widget, callback_stuff*, void*);
    static void		OKButtonCallback(Widget, callback_stuff*, void*);
    static void		check_passwd(Widget, callback_stuff*, void*);

    /* widget hierarchy (indent 1 space per level of parentage,as in an outline)
     * of POINTERS to motif_widget's (PFM reasons:  need explicit "new" to
     * control timing w/r/t X Widget hierarchy construction; need deletion of
     * Widgets to happen via that hierarchy (could get conflicts if automatic
     * destruction of each motif_widget when it went out of scope) */
    motif_widget*		pwdPopupButtonForm;
    // motif_widget*		pwdPopupCancelButton;
    motif_widget*		pwdPopupOKButton;
    motif_widget*		pwdPopupRowColumn;
    motif_widget*		pwdPopupLabel;		// fixed label
    motif_widget*		pwdPopupTxt;		// text with info

    motif_widget*		pwdPopupPanedWindow;
    static pwd_popup*		passwordDisplay;
    bool			gotThePassword;

    callback_stuff		data_for_window_closing; };

//The export file panel.
class UI_export: public motif_widget {

				//to get the old style options from prefs
	void			GetSaveOptionsListFromPrefs(tlist<alpha_string, optionNode>* listToSet) const;

	callback_stuff          data_for_window_closing;

public:
  //constructor
  UI_export(
	  const Cstring&	name_of_the_top_widget_of_this_object,
	  motif_widget*		parent_of_this_widget
	 );

  //destructor
  ~UI_export() {}

//  pthread_mutex_t			export_mutex;
//  pthread_mutex_t			main_mutex;
//  pthread_cond_t			export_cond;
//  pthread_cond_t			main_cond;
//  pthread_t				theThread;
  static bool				there_is_work_to_do;
  char*					passwd;
  
  EXPORT_DATArequest::how_to_export	how_option;

  motif_widget*				export_acts;
  motif_widget*				export_act_defs;
  motif_widget*				export_res_def;
  motif_widget*				export_res_hist;

  motif_widget*				export_via_curl;
  motif_widget*				export_via_json;
  motif_widget*				export_via_xdr;

  motif_widget*				paned_window;
  motif_widget*				frame1;
  motif_widget*				frame2;
  motif_widget*				frame3;
  motif_widget*				url_text_server;
  motif_widget*				url_text_port;
  motif_widget*				url_text_scn;
  motif_widget*				url_text_schema;
  motif_widget*				namespace_text;
  motif_widget*				directory_text;

  static UI_export*	ui_export;
  static void		initialize();
  static void		WhatToExportCallback(Widget, callback_stuff*, void*);
  static void		HowToExportCallback(Widget, callback_stuff*, void*);
  static void		CancelButtonCallback(Widget, callback_stuff*, void*);
  static void		OKButtonCallback(Widget, callback_stuff*, void*);
  };  

#endif /* _UI_save_H_ */
