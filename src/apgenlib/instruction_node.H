#include "templates.H"
#include "ParsedExpressionSystem.H"
#include "RES_eval.H"
#include "ExecStack.H"

class instruction_node: public Dnode0<alpha_int, instruction_node> {
public:
	// for signals and resource conditions:
	instruction_node(	pEsys::execStack& C,
				behaving_element& sym,
				const parsedExp& cond,
				bool is_regex);
	// for get_windows without window feedback:
	instruction_node(	pEsys::execStack& C,
				behaving_element& sym,
				const parsedExp& cond,
				const CTime_base& dur);
	// for get_windows with window feedback:
	instruction_node(	pEsys::execStack& C,
				behaving_element& sym,
				const parsedExp& cond,
				const CTime_base& dur,
				const CTime_base& du_Max,
				const Cstring& valueName);
	// duh
	instruction_node(const instruction_node& inst);
	~instruction_node() {
		if(list) {
			list->remove_node(this);
		}
	}

	instruction_node*			copy() {
		return new instruction_node(*this);
	}
	instruction_node*			get_this() {
		return this;
	}
	const instruction_node*			get_this() const {
		return this;
	}
	void					unwind() {
		while(Calls.pop_context());
	}

	pEsys::execStack	Calls;		// where to resume execution
	behaving_element	theSymbols;

				// NOTE: when using this node to put a "WAIT_UNTIL" activity
				//	 on hold, Duration will be a null expression and
				//	 Criterion will contain the Condition.
	const parsedExp&	Criterion;	// Boolean expression which needs to be true
	CTime_base		Duration;	// The minimum duration during which the condition must be true
	CTime_base		MaxDuration;
	bool			has_max_and_actual;
	bool			cond_is_a_reg_exp;
	Cstring			name_of_actual_duration_variable;
};

tlist<alpha_int, instruction_node>& where_to_go();
