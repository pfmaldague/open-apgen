#ifndef ACTIVITYDISPLAYCLASSDERIVED_H
#define ACTIVITYDISPLAYCLASSDERIVED_H

#include "C_global.H"
#include "UI_motif_widget.H"
#include "UTL_time.H"
#include "UTL_defs.H"

///
class ACT_sys ;
class UI_activityeditor ;
class commbox ;
class resource_display ;
class list_of_siblings ;

class UI_mainwindow;


class activity_display : public motif_widget {
protected:
///
	TIMEZONE		timezone;

public:

	// creates a Frame as child of UI_mainwindow::mainWindowObject->_panedWindow,
	// then a form (_activityDisplayForm) as a child of the frame. This form is used
	/// to position all the DS_graph objects that make up an activity display:
	activity_display( const Cstring & , motif_widget *my_parent , int theWidth , int theHeight ) ;
///
	~activity_display () ;

				// The most important class member; implemented
				/// as a motif_widget of the xmDrawingAreaClass:
	ACT_sys			*activitySystem ;

///
	int			isVisible() ;

///
	list_of_siblings	*siblings ;
///
	Cstring			thePopupName ;
///
	void			initialize_act_sys() ;

///
	resource_display	*resourceDisplay ;

				/// used to query redundant flag; now: checks toggle button
	int			isSelected() ;

				/// creates an ACT_sys:
	void			initialize( const Cstring &the_name ) ;

				/// deletes the ACT_sys:
	void			uninitialize() ;

				/// used by UI_mainwindow::clearActivityDisplays():
	void			clear() ;

///
	apgen::RETURN_STATUS		getZoomPanInfo ( CTime & startTime , CTime & duration);
///
	Cstring                 getADName() const;
///
	void			newActivity ( char *activityType ) ;
///
	ACT_sys			*getActivitySystem() ;
///
	void			sensitizeLegendsSelectedButtons() ;
///
	int			LegendsSelectedButtonsAreSensitive() ;
///
	void			desensitizeLegendsSelectedButtons() ;
///
	void			deleteSelectedLegends() ;
///
	void			squishSelectedLegends() ;
///
	void			flattenSelectedLegends() ;
///
	void			unsquishSelectedLegends() ;
///
	void			expandSelectedLegends() ;
///
	void			unselectAllLegends() ;
				/// For GUI callback.
	void			makeSelectedFromGui( int ) ;
				/// For action request.
	void			makeSelectedFromActionRequest( int ) ;

///
	motif_widget		*adViolationLabel , *adViolationCountLabel ;
///
	motif_widget		*_adTimeSystemForm , *adPointerTimeLabel , *adTimeSystemLabel ;
				/** , * _adTimeSystemPM , * _adTimeSystemOM					OBSOLETE */

	void			update_time_system_label();
	int			get_time_system_label(Cstring &l);
///
	void			update_pointer_time_label( CTime the_new_time ) ;
///
	void			define_time_system_widgets( const List & list_of_all_time_system_names ) ;
///
	void			reset_time_system_widgets() ;
///
	void			adjust_time_system_label( const Cstring & new_time_label ) ;
				// virtual function was overloaded for the sole benefit of THIS class, which needs to update its
				/// time system buttons:
	void			manage() ;

				/// old bx stuff:
	motif_widget		* _activityDisplayForm , * _adConstraintDA , * _adSelectedTB ,
				* _adSelAllButton , * _adDownArrowButton , * _adUpArrowButton , * _adLegendForm ,
				* _adVertScrollBar , * _adHorizScrollBar , * _adPlottingAreaDA , * _adTimelineDA;

				///
	int			myNumber ;
private: 
///
	static void	adTimeSystemChangeCallback(	Widget , callback_stuff * , void * ) ;
///
	static void	adSelectCallback(		Widget , callback_stuff * , void * ) ;
///
	static void	adSelAllButtonActivateCallback(	Widget , callback_stuff * , void * ) ;
///
	static void	adDownArrowActivateCallback(	Widget , callback_stuff * , void * ) ;
///
	static void	adUpArrowActivateCallback(	Widget , callback_stuff * , void * ) ;
	} ;
#endif

