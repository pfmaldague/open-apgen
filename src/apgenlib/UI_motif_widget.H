/*	PROJECT		: APGEN
 *	SUBSYSTEM	: UI (non-BX Motif)
 *	FILE NAME	: UI_motif_widget.H
 *
 *
 *	ABSTRACT:
 *	
 *	Header file for Pierre's Motif wrapper classes, adapted from SEQ_REVIEW
 *	file motif_widget.H
 *
 */

#ifndef MOTIF_WIDGET_H
#define MOTIF_WIDGET_H

// NOTE: MAKE SURE MOTIF HAS ALREADY BEEN INCLUDED

// new (and bold) PFM 5/19/99:
// #define GUI

#include "C_list.H"
#include "UI_MotifPreview.H"
#include "UTL_stl.H"
#include "APbasic.H"

// Newly added constants for SELECTION_TYPE scrolled_list_widget() 8/29/03
const int MULTIPLE_SELECT = 0;
const int SINGLE_SELECT = 1;

extern const char*	spellEtype(int e);
extern int		GUI_Flag;


typedef Cnode0<alpha_string, stringtlist> tlistNode;
typedef tlist<alpha_string, tlistNode> tlisttlist;
typedef slist<alpha_string, tlistNode> tlistslist;

class infoPLD;
typedef Cnode0<alpha_string, infoPLD>  infoNode;
typedef slist<alpha_string, infoNode> infoList;

class infoPLD {
public:
	infoPLD() {}
	infoPLD(const Cstring& theReason, const infoList& servers, const infoList& agents)
		: reason(theReason),
			theListOfServers(servers),
			theListOfAgents(agents) {}
	~infoPLD() {}

	Cstring		reason;
	infoList	theListOfServers;
	infoList	theListOfAgents;
};

enum KBD_AND_MOUSE_ACTION
{
	EXPOSE = 1, RESIZE, FOCUS, MOTION,
	BTTN1_DN, SHFT_BTTN1_DN, CTRL_BTTN1_DN, META_BTTN1_DN,
	BTTN1_UP, SHFT_BTTN1_UP, CTRL_BTTN1_UP, META_BTTN1_UP, // added by BKL 9-26-97
	BTTN2_DN, SHFT_BTTN2_DN, CTRL_BTTN2_DN, META_BTTN2_DN,
	BTTN2_UP, SHFT_BTTN2_UP, CTRL_BTTN2_UP, META_BTTN2_UP,
	BTTN3_DN, SHFT_BTTN3_DN, CTRL_BTTN3_DN, META_BTTN3_DN,
	BTTN3_UP, SHFT_BTTN3_UP, CTRL_BTTN3_UP, META_BTTN3_UP,
	KEY_PRESS, NO_ACTION
};

extern void AllowWindowClosingFromMotifMenu(Widget, callback_proc, void*);

class END_OP
{
public:
	END_OP() {}
	~END_OP() {}
};

//in UI_motif_widget.C:
extern END_OP endform;

// instead of an enum, we now use bstringnodes to convey properties, so they can be
// placed in lists...

// legal property values:
#define	EDITABLE				"mwEDITABLE"
#define NONEDITABLE 				"mwNONEDITABLE"
#define	INITIAL_FOCUS 				"mwINITIAL_FOCUS"
#define WRAP_TEXT				"mwWRAP_TEXT"
#define	AT_BEGINNING				"mwAT_BEGINNING"
#define CENTERED				"mwCENTERED"
#define AT_END					"mwAT_END"
#define	SCROLLBAR_PLACEMENT_BOTTOM_LEFT		"mwSCROLLBAR_PLACEMENT_BOTTOM_LEFT"
#define	SELECTION_POLICY_MULTIPLE_SELECT	"mwSELECTION_POLICY_MULTIPLE_SELECT"
#define	SELECTION_POLICY_SINGLE_SELECT		"mwSELECTION_POLICY_SINGLE_SELECT"
#define	SCROLLBAR_DISPLAY_POLICY_AS_NEEDED	"mwSCROLLBAR_DISPLAY_POLICY_AS_NEEDED"
#define	SCROLLBAR_DISPLAY_POLICY_STATIC		"mwSCROLLBAR_DISPLAY_POLICY_STATIC"
#define	CHILD_OF_A_FORM				"mwCHILD_OF_A_FORM" 
#define FORM_PARENT				"mwFORM_PARENT"

class motif_widget;
class scrolled_list_widget;

class form_position
{
public:
	form_position(int x) : pos(x) {}
	form_position() : pos( 0 ) {}
	form_position( const form_position & fq ) : pos( fq.pos ) {}
	~form_position() {}
	int		pos;
	motif_widget & operator < ( motif_widget & );
	motif_widget & operator ^ ( motif_widget & );
	void operator < ( const END_OP & ) {}
	void operator ^ ( const END_OP & ) {}
	form_position & operator = ( int i ) { pos = i; return * this; }
};

enum TYPE_OF_CALLBACK
{
	CALLBACK_TYPE, EVENT_HANDLER_TYPE, KBD_AND_MOUSE_HANDLER_TYPE
};

enum	MODIFIER
{
	NO_MODIFIER = 0, SHIFT_MODIFIER = 1,
	CONTROL_MODIFIER = 2, ALT_MODIFIER = 4
};

class callback_stuff: public ref_node {
public:
	callback_stuff(motif_widget*, callback_proc, const char*, void*);					// CALLBACK_TYPE
#ifdef HAVE_MACOS
	callback_stuff(motif_widget*, callback_proc, const char*, int, void*);					// FIX_HEIGHT
#endif /* HAVE_MACOS */
	callback_stuff(motif_widget*, callback_proc, const char*, void*, long);					// EVENT_HANDLER_TYPE
	callback_stuff(motif_widget*, callback_proc, const char*, void*, KBD_AND_MOUSE_ACTION, MODIFIER);	// KBD...
	callback_stuff(const callback_stuff&);
	~callback_stuff();

				// we need this, because callbacks cannot be detached
				// unless the reason agrees exactly with that used when adding:
	const char		*original_reason;
	Node			*copy() {return new callback_stuff(*this); }
#ifdef HAVE_MACOS
	int			height;
#endif /* HAVE_MACOS */
	callback_proc		function;
	void			*data;
	motif_widget		*parent;
	TYPE_OF_CALLBACK	callback_type;
	long			Mask;		// for event handlers
	KBD_AND_MOUSE_ACTION	action;	// for keyboard/mouse handling
	MODIFIER		modifier;	// for keyboard input
	Cstring			string; };

class motif_widget: public ref_node {
public:
	motif_widget(motif_widget *parnt);
	motif_widget(motif_widget *parnt, Widget widgt, const WidgetClass motif_class);
	motif_widget(const motif_widget&);
	// for forms:
	motif_widget(const Cstring &	widget_name,
		motif_widget *		widget_parent,
		const form_position &	pos,
		bool			should_manage = true);
	// long version:
	motif_widget(
		const Cstring &		widget_name,
		const WidgetClass	motif_class,
		motif_widget *		widget_parent,
		void *			arguments,
		int			number_of_arguments,
		bool			should_manage = false);
	// long version with setup client for base class constructors:
	motif_widget(
					// change order of the parameters to avoid ambiguity in panel.C:
		void			(*set_client)( void*, int& ),
		const Cstring&		widget_name,
		const WidgetClass	motif_class,
		motif_widget*		widget_parent,
		bool			should_manage = false);
	// simple version (always manages):
	motif_widget(
		const Cstring &		widget_name,
		const WidgetClass	motif_class,
		motif_widget *		widget_parent );

	virtual	~motif_widget();
	
	motif_widget	*myParent;
	void		construct_widget(motif_widget* parent, void* args, int n,
				 bool should_manage );
	Node		*copy() { return new motif_widget( *this ); }
	void		add_callback( callback_proc, const char* Reason, void* data );
	void		add_event_handler(callback_proc, long mask, void* data);
#ifdef HAVE_MACOS
	void		add_event_handler(callback_proc, long mask, int desired_height, void* data);
#endif /* HAVE_MACOS */
	void		add_keyboard_and_mouse_handler(
				/* kbd_and_mouse_event_handler */ callback_proc,
				KBD_AND_MOUSE_ACTION, MODIFIER, void* data);
	void		detach_callbacks_and_event_handlers();
	void		remove_widgets();
			// int was added to debug Solaris problem (Xtmalloc error while exiting)
	void		destroy_widgets( int = 0 );
	void		set_sensitive( int value );
	int		is_sensitive();
	virtual void	manage();
	void		unmanage();
	void		set_menu_position( void * event ); // for popup menus
	void		fix_height(
#ifdef HAVE_MACOS
				int desired_height,
#endif /* HAVE_MACOS */
				Widget sizing_widget = NULL );

	Cstring		get_text();	// returns various things for various widgets
	Cstring		get_label();	// identical to get_text, except for toggle buttons
	void		set_label(const Cstring&);	// sets text of toggle buttons (operator = sets or resets)
			/**
			 * You'd think that we can query the X system to find out whether a widget
			 * is managed, but this does NOT work. To compensate, we explicitly keep track:
			 */
	bool		is_managed();
	bool		managedState;

	motif_widget	*get_this_widget() { return this; }
	Widget		widget;
	List		children;	// nodes should be motif_widgets
	List		callbacks;	// nodes should be Tags pointing to
				//   callback_stuff data
	WidgetClass	MotifClass;

	int		RightOffset, BottomOffset;
	char		IsChildOfAForm;

	virtual void	clear() {};

	// set text to r.h.s.:
	motif_widget & operator = ( const Cstring & );
	// attach left edge of second widget to right edge of first widget:
	motif_widget & operator < ( motif_widget & );
	// set offset of left widget:
	motif_widget & operator < ( int );
	// set position of widget:
	motif_widget & operator < ( const form_position & );
	// set offset of right widget:
	friend motif_widget & operator < ( int, motif_widget & );
	// terminate line without compiler warnings:
	void operator < ( const END_OP & ) {}
	void operator ^ ( const END_OP & ) {}
	// attach top edge of second widget to bottom edge of first widget:
	motif_widget & operator ^ ( motif_widget & );
	// attach top to position
	motif_widget & operator ^ ( const form_position & );
	// set vertical offset of widget:
	motif_widget & operator ^ ( int );
	// set vertical offset of widget:
	friend motif_widget & operator ^ ( int, motif_widget & );
	// to set initial focus:
	void add_property( const Cstring & );
	// to get rid of compiler complaints:
	void operator ! () {} };

class label_widget: public motif_widget {
public:
	label_widget(const Cstring& name, motif_widget* parent,
		const Cstring & alignment, bool should_manage = true);
	label_widget(const motif_widget& M) : motif_widget(M) {}
	~label_widget() {} };

class single_line_text_widget: public motif_widget 
{
public:
  //
  single_line_text_widget(const Cstring& name, motif_widget* parent,
			  const Cstring & editable,
			  bool should_manage = true);
  //
  single_line_text_widget(const motif_widget& M) : motif_widget(M) {}
  //
  ~single_line_text_widget() {}
  //
  void SetText(const char* stringValue);
  Cstring GetText();
 };

class multi_line_text_widget: public motif_widget {
public:
	multi_line_text_widget(
		const Cstring& name,
		motif_widget* parent,
		int rows,
		const Cstring & editable,
		bool should_manage = true);
	multi_line_text_widget(const motif_widget& M) : motif_widget(M) {}
	~multi_line_text_widget() {} };

class scrolled_text_widget: public motif_widget {
public:
	scrolled_text_widget(
		const Cstring& name,
		motif_widget* parent,
		int vis_rows,
		const Cstring & editable,
		bool should_manage = true);
	scrolled_text_widget(const motif_widget& M) : motif_widget(M) {}
	~scrolled_text_widget() {} };

class scrolled_list_widget: public motif_widget , public back_pointer {
public:
	scrolled_list_widget(const Cstring& name, motif_widget* parnt, int vis_lines, bool mng = true);
	scrolled_list_widget(const Cstring& name, motif_widget* parnt, int vis_lines, const blist &motif_properties, bool mng);
	scrolled_list_widget(const motif_widget& M) : motif_widget(M) {}
	~scrolled_list_widget();

	List		smart_pointers_to_slave_lists;
	static void	standardResponseToScrolling( Widget , void * , void * );
	static void	SingleSelection( Widget , callback_stuff * , void * );
	void		clear();

			/* Populates the list (after clearing) with ref_nodes; Substring = selected
			 *   item text, ID = selected item position.  Returns the list length. */
	int		get_selected_items(strintslist&);
			// Returns the Ith item if any:
	Cstring		get_item_number( int I );
	virtual Cstring	get_id() { return get_key(); }

	void		deselect_all();
	void		delete_item( int );
	void		select_bottom_pos( int notify = 0 );
	void		reset_horiz_pos();
	void		set_pos(int I);
	void		select_pos( int I );

			// base method overridden because we need to detach a non-motif-widget-callback:
	void		detach_callbacks_and_event_handlers();
			// List of ref_nodes, for convenience:
	List		theItems;
	void		redisplay_all();

	void		operator << ( bstringnode *P );
	void		operator << (const Cstring&); 
        void            SetList(const StringVect& strVect);

	void            setSelectionType(int T); };


class slave_list_widget: public motif_widget, public back_pointer {
public:
	slave_list_widget( const Cstring & name , motif_widget * parent , scrolled_list_widget * master ,
		void * theArguments , int no_args );
	slave_list_widget( const motif_widget &MW ) : motif_widget(MW) {}
	~slave_list_widget() {}
			// list of ref_nodes. This list is always the COMPLETE list of ALL the items,
			// whether visible or not...
	List		theItems;
	virtual Cstring	get_id() { return get_key(); }
			// Returns the Ith item if any:
	Cstring		get_item_number(int I);
			// list of 0 or 1 node:
	List		list_of_pointer_to_the_master;
			// 0 means nothing displayed (empty list)
			// 1 means first item in theItems is displayed at the top of the list
			// etc...
	int		index_of_item_at_the_top;

			// This is the built-in callback that will notify the master and any other slave(s).
			// The client of this widget can of course add her own callbacks:
	static void	SingleSelection( Widget , callback_stuff * , void * );

	// void		document_self( const char * ) const;
	void		clear();
	void		redisplay_all();
	void		set_pos(int I);
	void		select_pos( int I );

	void		operator << ( const Cstring & ); };

class user_waiter {
public:
	void			(*timeout_proc)(void *);
	void			*theData;
	user_waiter(void (*P)(void *) = NULL, void *callback_data = NULL)
		:timeout_proc(P),
       		theData(callback_data) {}
	~user_waiter() {}
	static int		user_has_made_a_choice;
	virtual int		i_am_a_pause_popup() { return 0; }
	virtual int		this_is_the_last_time() { return 1; }
	static int		we_are_in_automatic_mode;
	static void		wait_for_user_action(user_waiter *); };

#endif
