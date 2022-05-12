
#include "ActivityInstance.H"
#include "EventLoop.H"
#include "instruction_node.H"

//
// This file is 1 of 2 places where the execStack copy
// constructor is used; the other is in EventLoop.C. */
//

//
// for signals and resource conditions:
//
instruction_node::instruction_node(
		pEsys::execStack&	C,
		behaving_element&	sym,
		const parsedExp&	cond,
		bool			is_regex)
	: baseC<alpha_int, instruction_node>(EventLoop::CurrentEvent->eventID),
		Calls(C.get_context()),
		Criterion(cond),
		has_max_and_actual(false),
		cond_is_a_reg_exp(is_regex),
		theSymbols(sym) {}

//
// for OKtoSchedule without window feedback:
//
instruction_node::instruction_node(
		pEsys::execStack&	C,
		behaving_element&	sym,
		const parsedExp&	condition,
		const CTime_base&	dur)
	: baseC<alpha_int, instruction_node>(EventLoop::CurrentEvent->eventID),
		Calls(C.get_context()),
		Criterion(condition),
		Duration(dur),
		has_max_and_actual(false),
		cond_is_a_reg_exp(false),
		theSymbols(sym) {}

instruction_node::instruction_node(
		pEsys::execStack&	C,
		behaving_element&	sym,
		const parsedExp&	condition,
		const CTime_base&	dur,
		const CTime_base&	du_max,
		const Cstring&		variableToStoreActualDuration)
	: baseC<alpha_int, instruction_node>(EventLoop::CurrentEvent->eventID),
		Calls(C.get_context()),
		Criterion(condition),
		Duration(dur),
		has_max_and_actual(true),
		cond_is_a_reg_exp(false),
		MaxDuration(du_max),
		name_of_actual_duration_variable(variableToStoreActualDuration),
		theSymbols(sym) {}

instruction_node::instruction_node(const instruction_node& inst)
	: baseC<alpha_int, instruction_node>(inst),
		Calls(inst.Calls.get_context()),
		Criterion(inst.Criterion),
		Duration(inst.Duration),
		theSymbols(inst.theSymbols),
		cond_is_a_reg_exp(inst.cond_is_a_reg_exp),
		MaxDuration(inst.MaxDuration),
		name_of_actual_duration_variable(inst.name_of_actual_duration_variable),
		has_max_and_actual(inst.has_max_and_actual) {
}

			/* blist of instruction_nodes (derived from bpointernode class);
			 * instruction_node constructor uses the stack (one if its args)
			 * to extract the ACT_req, then defines the bpointernode pointer
			 * as the address of the ACT_req: */
tlist<alpha_int, instruction_node>& where_to_go() {
			static tlist<alpha_int, instruction_node> wh;
			return wh;
}




