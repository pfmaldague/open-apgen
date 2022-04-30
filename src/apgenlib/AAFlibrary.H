#ifndef _AAFLIBRARY_H_
#define _AAFLIBRARY_H_
#include "C_list.H"

// this file contains the basic interface to the AAF library

extern void*&	theAAFlibHandle();
extern blist&	theAAFActivityFactories();
extern List&	theListOfAbstractResourceInstances();

namespace AAFlibrary {

extern "C" {
	typedef int			(*global_initializer)(void*, char** errors);
	typedef void			(*global_function_init)(void* ptr_to_blist, char** errors);
	typedef void*			(*activity_factory)(const char* act_type_name, void* ptr_to_act_req, void* blist_of_factories, char** any_errors);
	typedef int			(*factory_initializer)(void* blist_of_factories);
	typedef int			(*thread_stimulant)(void* thread_to_resume, char** errors);
	typedef void			(*absres_destructor)(void);
	extern global_initializer	initialize_all_globals;
	extern global_function_init	initialize_all_functions;
	extern activity_factory		AAF_activity_factory;
	extern factory_initializer	initialize_factories;
	extern thread_stimulant		resume_thread;
	extern absres_destructor	the_abs_res_destructor;
} // extern "C"

} // namespace AAFlibrary

#endif /* _AAFLIBRARY_H_ */
