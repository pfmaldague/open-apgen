#ifndef _AP_MODEL_H
#define _AP_MODEL_H

#include <GLOBdata.H>

class Action_request;
class OPEN_FILErequest;

class model_intfc {
public:
	model_intfc() {}
	~model_intfc() {}

	static void			init_model(
						time_saver&			master);

	//
	// The list argument holds optional TOL / XMLTOL output requests,
	// which need to be processed in a separate thread while modeling.
	//
	static void			doModel(time_saver& master);

	//
	// In principle, a script that ends with a scheduling pass might
	// need to process TOL and XMLTOL output requests, just like
	// REMODEL. In practice, however, scheduling scripts only produce
	// small (partial) XMLTOL files, if any; parallel processing is
	// not a requirement for those.
	//
	static void			doSchedule(
						bool				use_selection_list,
						bool				incremental,
						slist<alpha_void, dumb_actptr>&	top_kids,
						time_saver&			master);

	static void		 	doUnschedule(
						int				all_flag,
						time_saver&			master);

	static void			purge();

	//
	// called at the end of the scheduling process:
	//
	static void			PurgeSchedulingEvents();


	// returns true if there is one, 0 otherwise; consults activities
	static bool			FirstEvent(
						CTime_base&,
						int print_flag = 0);

	static bool			LastEvent(
						CTime_base &);
	//
	// returns true if there is one, false otherwise;
	// consults activities
	//
	static bool			HasEvents();

	static void			add_timevent(
						mEvent*&,
						apgen::METHOD_TYPE original_section);
};

#endif /* _AP_MODEL_H */
