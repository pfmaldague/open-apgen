#ifndef _UI_XML_TOL_H_
#define _UI_XML_TOL_H_

#include <APbasic.H>
#include <APmodel.H>
#include <UI_GeneralMotifInclude.H>
#include <UI_motif_widget.H>
#include <UTL_time.H>

// inspired from seq_review_popup and tol_popup
class xmltol_popup: public motif_widget {
public:
	xmltol_popup(
		const Cstring&	name,                   // will be name of top widget
        	motif_widget*	parent_widget,          // parent of this widget
        	const Cstring&	text_for_popup          // text (multiline) to display
		);
	~xmltol_popup();

	motif_widget*		paned_window;
	motif_widget*		top_widget;
	motif_widget*		form_for_start_and_end_times;
	motif_widget*		start_time;
	motif_widget*		end_time;
	motif_widget*		label;
	motif_widget*		label2;
	motif_widget*		filter_form;
	motif_widget*		filter_label;
	motif_widget*		timesystem_form;
	motif_widget*		timesystem_label;
	motif_widget*		schema_form;
	motif_widget*		schema_label;
	motif_widget*		schema_button;
	motif_widget*		visibility_button;
	motif_widget*		schema_name_label;
	motif_widget*		schema_name;
	motif_widget*		file_name_form;
	motif_widget*		file_name_label;
	motif_widget*		fileNameWidget;
	motif_widget*		form_for_buttons;
	CTime			S, E;
	callback_stuff		data_for_window_closing;

	scrolled_list_widget*	filters;
	stringtlist		filter_list;
	scrolled_list_widget*	timesystems;
	stringslist		timesystem_list;
	bool			AllActsVisible;

	static void		CancelButtonCallback(Widget, callback_stuff*, void*);
	static void		OKButtonCallback(Widget, callback_stuff*, void*);
	static void		SchemaButtonCallback(Widget, callback_stuff*, void*);
	static void		VisibilityButtonCallback(Widget, callback_stuff*, void*);
	virtual void		OKcallback();
	virtual void		schema_callback();
	virtual void		visibility_callback();
	virtual void		update_times(CTime& ST, CTime& EN);
	void			update_time_widgets();

	static xmltol_popup*	_xmltolPopupDialogShell;
};

#endif /* _UI_XML_TOL_H_ */
