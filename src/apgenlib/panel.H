#ifndef PANEL_H
#define PANEL_H

#include "UI_GeneralMotifInclude.H"
#include "UI_motif_widget.H"

enum PANEL_TYPE {	FILE_SELECTION_DIALOG ,
			FILE_SELECTION_DIALOG_W_NAME_AND_DESCR ,
			BASIC_DIALOG ,
			PROMPT_DIALOG ,
			PANED_WINDOW ,
			PANED_WINDOW_W_LIST ,
			INFORMATION_PANEL ,
			UNDEFINED };

enum SELECTION_WIDGET_TYPE { NONE , COMBO_BOX , SELECTION_BOX , LIST_WIDGET };

class panel {
public:
	panel( Cstring );
	~panel();

	void 				create_widgets( PANEL_TYPE ,
						Cstring * pattern_text = NULL ,
			 			Cstring * filename_label = NULL ,
						Cstring * ok_button_label = NULL ,
						Cstring * cancel_button_label = NULL ,
				 		Cstring * help_button_label = NULL ,
						Cstring * add_button_label = NULL );
	// MOTIF CALLBACKS:

	static void			Apply( Widget , callback_stuff * , void * );
	static void			Cancel( Widget , callback_stuff * , void * );
	static void			Catalog( Widget , callback_stuff * , void * );
	static void			Add( Widget , callback_stuff * , void * );

	// callbacks:

	virtual void			add( void * ) {; }
	virtual void			apply() = 0;
	virtual void			cancel() { PopupWidget->unmanage(); }
	// WIDGETS:

	scrolled_list_widget		*SelectionList;
	motif_widget			*PopupWidget , *DialogWidget ,
					*FormWidget ,
	//				*Box ,
	//				*ComboBoxEditWidget ,
					*NameWidget ,
					*DescriptionWidget ,
					*AddButton ,
					*MoreButton ,
					*ContinueButton ,
					*InfoWidget;
	static callback_stuff		data_for_closing_window;

	static int			first_time;
	void				UpdateSelectionList( int include_NONE = 0 );

					// for catalog type objects, and also for any other panels that
					// let the user choose from a list:
	List *				Selections;

	Cstring				ExtractFilename();

	static void			UnmanageAllPanels();
	static SELECTION_WIDGET_TYPE		SelectionWidgetType;
	static Cstring				WidgetName;

protected:

	Node *				get_selected_item();
	static PANEL_TYPE		type;
	};

#endif
