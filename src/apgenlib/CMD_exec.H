#ifndef _CMD_EXEC_H
#define _CMD_EXEC_H

#include "action_request.H"
#include "smartptr.H"
namespace cmdSys {
class cE;
}

typedef smart_ptr<cmdSys::cE> cmdExp;

namespace cmdSys {

class cE {
public:
	cE(const Cstring& txt) : theData(txt), ref_count(0) {}

	cE(const cE& ce) : theData(ce.theData),
		ref_count(0),
		expressions(ce.expressions) {}
	virtual ~cE() {}

	Cstring	theData;

	// smart_ptr support:
	int			ref_count;
	int			ref() {
		ref_count++;
		return ref_count;
	}
	int			deref() {
		ref_count--;
		return ref_count;
	}
	vector<cmdExp>		expressions;
	void			addExp(const cmdExp& ce) {
		expressions.push_back(ce);
	}
};

class Assignment: public cE {
public:
	Assignment(const cmdExp& l_h_s,
			const cmdExp& r_h_s)
		: cE("Assignment"),
			lhs(l_h_s),
			rhs(r_h_s) {}
	Assignment(const Assignment& a)
		: cE(a),
			lhs(a.lhs),
			rhs(a.rhs) {}

	cmdExp		lhs;
	cmdExp		rhs;
};

class EditActivity: public cE {
public:
	EditActivity(const cmdExp& id,
			const cmdExp& flag)
		: cE("EditActivity"),
			act_id(id),
			carry_flag(flag) {}
	EditActivity(const EditActivity& a)
		: cE(a),
			act_id(a.act_id),
			carry_flag(a.carry_flag) {}

	cmdExp		act_id;
	cmdExp		carry_flag;
};

} // namespace cmdSys


class CMD_exec {
	stringslist			strlist;
	slist<alpha_void, dumb_actptr>	ptrlist;
	// pEsys::expNodeStack		stack;

public:
	CMD_exec();
	~CMD_exec();

	commandslist*				cmdlist;
	void					set_list_of_commands_to(commandslist& C) {
		cmdlist = &C;
	}
	const stringslist&			get_strlist() const { return strlist; }
	const slist<alpha_void, dumb_actptr>&	get_ptrlist() const { return ptrlist; }
	void					clear();
	void					add_string_node_to_list(const Cstring& str);
	void					add_act_pointer_to_list(const Cstring& act_name);
	// void					stack_push(pEsys::expNodeType nt, const Cstring& nodeData);
	// const Cstring&			stack_pop();
	static CMD_exec&			cmd_subsystem();
	void					clear_lists();
};

#endif
