#ifndef UI_messagewarndialog_H
#define UI_messagewarndialog_H

#include "UI_motif_widget.H"

///
class UI_messagewarndialog : public motif_widget
	{
protected:
///
	motif_widget		* messageWarnDialog ;
///
	motif_widget		* messageWarnForm ;
///
	motif_widget		* msgWarnButtonForm;
///
	motif_widget		*msgWarnOKButton;
///
	motif_widget		*msgWarnCancelButton;
///
	motif_widget		*msgWarnSW;
///
	motif_widget		*msgWarnText;
///
	motif_widget		*msgWarnLabel;
///
	motif_widget		*msgWarnOptionsBox;

///
	static void		msgWarnButtonActivateCallback(Widget, callback_stuff * , void * ) ;
				/// OK processing
	void			(*FunctionToCall)(const Cstring &);
				/// Cancel processing
	void			(*CancelAction)(void);
    
public:
	/// arg. is list of options (encoded as String_nodes):
        UI_messagewarndialog(const List &, bool include_cancel);
        ///
        ~UI_messagewarndialog();

///
	static void initialize(const Cstring & windowTitle,
			const Cstring	&dialogTitle,
			const Cstring	&message,
			const List	&button_labels,		// list of String_nodes with toggle button text
			void		(*)(const Cstring &),	// function that will be called by the OK callback
								// with the text of the active toggle button
			bool		include_cancel_button = false,
			void		(*)(void) = NULL // called by the Cancel callback
			);
	static void enable_ok();
	static void disable_ok();
	} ;

class UI_preferences : public motif_widget
	{
public:
///
	motif_widget		* paned_window , * OKButton , * ApplyButton , * CancelButton ;
///
	motif_widget		* remodeling_box , * drag_children_box ;
///
	motif_widget		* no_remodeling_button , * remodeling_button ;
///
	motif_widget		* horizontal_button , * vertical_button , * unrestricted_button ;
        ///
        motif_widget            * snap_to_grid;
///
	motif_widget		* together_button , * separate_button ;
///
	UI_preferences() ;
///
	~UI_preferences() {}
///
	static void		initialize() ;
///
	static void		DoneButtonCallback( Widget , callback_stuff * , void * ) ;
	} ;

#endif
