#ifndef RES_EXCEPTIONS_H
#define RES_EXCEPTIONS_H

#include "C_string.H"

class eval_error {
public:
	eval_error(const Cstring& err);
	eval_error(const eval_error& ee);
	~eval_error() {}

	static bool&	thread_unsafe_error();
	static Cstring&	thread_unsafe_msg();

	//
	// Sets thread_unsafe_error to true and
	// thread_unsafe_msg to the given string
	//
	static void	safety_exception(const Cstring&);
	Cstring		msg;
};

#endif
