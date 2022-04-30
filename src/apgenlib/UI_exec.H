#ifndef UI_EXEC_H
#define UI_EXEC_H

// #include "IO_protocols.H"
#include "UI_motif_widget.H"
#include "UTL_defs.H"
#include "UTL_time.H"

class UI_activityeditor ;
class commbox ;

#ifndef _WIN32
#	ifndef linux
/* typedef unsigned char * XmString ; */
#	endif
#endif

//
class UI_exec
	{
public:
	UI_exec() {}
	~UI_exec() {}

	static void	enable_scheduling();
	static void	disable_scheduling();
	void		scheduleActivityButtonActivate(int which_option);
	void		OpenActivityDisplay(CTime startTime, CTime duration);
	void		changeCursor(int);
	void		addNewItem();
	void		addNewActivity(const Cstring &);
	void		setRDControlInfo(RD_YSCROLL_MODE mode, int resolution);
	void		displayMessage(const Cstring &windowTitle, const Cstring &messageTitle, const Cstring &message);
	void            displayMessage ( const Cstring & windowTitle, const Cstring & messageTitle, const List & messages ) ;
	void		zoomScrollSelResourceDisplays( double dmin , double dspan ) ;
	apgen::RETURN_STATUS	printButtonUpdateSelections() ;
	static void	update_all_time_systems() ;

	static void	synchronize() ;
	static void	unsynchronize() ;
			/// origin = 1 for mail, 2 for activity editor (new type def.)
	void		read_mail( const Cstring & , int origin ) ;

	/// CALLBACKS:
	static void	AddNewActivity( Widget , callback_stuff * , void * ) ;
	static void	AddNewActivityOK( Widget , callback_stuff * , void * ) ;
	static void     AddNewActivities(Widget, callback_stuff *, void *);
	static void	AddResource( Widget , callback_stuff * , void * ) ;
	static void	AddResourceOK( Widget , callback_stuff * , void * ) ;

//     Find Activity Panel Execute
	static void	FindActivity( Widget, Cstring TAS);
	static void	FindActivityOK( Widget , callback_stuff * , void * ) ;
};

#endif
