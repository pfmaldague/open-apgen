#ifndef RES_EXEC_H
#define RES_EXEC_H

#include <APmodel.H>
#include <Multiterator.H>

class	Action_request;
class	EventLoop;

namespace RES_exec {
// private:
	// static int		one_event_at_a_time;
	// static int		real_time_flag;
// public:
	// RES_exec();
	// ~RES_exec();

	apgen::RETURN_STATUS	checkForEmptyArrayedResources(Cstring& any_errors);

	//
	// Modeling
	//
	void			update_all_epochs();

	// only called when destroying the resource subsystem
	void			clear_all_lists();

	//
	// Utilities
	//
	// void			get_all_resource_names(stringslist&);
	// void			get_all_resource_container_names(stringslist&);
	void			print_to_C(aoString &);
	void			print_to_H(aoString &);
	void			report_history();
	// friend class		model_intfc;
};

#include <EventLoop.H>

#endif
