#ifndef _SC_PE_SYS_H_
#define _SC_PE_SYS_H_

#include <map>
#include <list>

#include <sys/types.h>

#include <APdata.H>
#include <smartptr.H>

namespace SC {
class pE_base;
}

typedef smart_ptr<SC::pE_base>		SCExp;
class Action;

//
// SC = Spacecraft
//
namespace SC {

class pE_base;


//
// Interface (pure abstract class) for recursive search
//
class exp_analyzer {
public:
	exp_analyzer() {}
	exp_analyzer(const exp_analyzer&) = delete;
	virtual ~exp_analyzer() {}
	virtual void pre_analyze(pE_base*) = 0;
	virtual void post_analyze(pE_base*) = 0;
	virtual bool inhibit(pE_base*) { return false; }
};

class origin_info {
public:
	origin_info() : file("unknown"), line(1) {}
	origin_info(int L, const Cstring& F) : file(F), line(L) {}
	origin_info(const origin_info& o) : line(o.line), file(o.file) {}
	~origin_info() {}

	int line;
	Cstring file;
};

class pE_base {
protected:
	Cstring			theData;

public:

	vector<SCExp>		expressions;
	int			mainIndex;

	virtual void	recursively_apply(exp_analyzer& EA) {
		EA.pre_analyze(this);
		for(int i = 0; i < expressions.size(); i++) {
			expressions[i]->recursively_apply(EA);
		}
		EA.post_analyze(this);
	}
	virtual void	addExp(
				const SCExp& pe) {
		expressions.push_back(pe);
	}

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

	// parsing support:
	int			lval;
	Cstring			file;
	int			line;

	virtual void initExpression(const Cstring& nodeData) {}

	// deep copy:
	virtual pE_base* copy() {
		return new pE_base(*this);
	}
	// shallow copy:
	virtual pE_base* shallow_copy() {
		return new pE_base(true, *this);
	}
	pE_base() = delete;
	pE_base(const origin_info& o)
		: ref_count(0),
			lval(0),
			mainIndex(0),
			file(o.file),
			line(o.line) {
	}
	// deep copy:
	pE_base(const pE_base& C)
		: theData(C.theData),
			ref_count(0),
			mainIndex(0),
			file(C.file),
			line(C.line) {
		for(int i = 0; i < C.expressions.size(); i++) {
			if(C.expressions[i]) {
				expressions.push_back(SCExp(
					C.expressions[i]->copy()));
			} else {
				expressions.push_back(SCExp());
			}
		}
	}
	// shallow copy:
	pE_base(bool, const pE_base& C)
		: theData(C.theData),
			ref_count(0),
			mainIndex(0),
			file(C.file),
			line(C.line),
			expressions(C.expressions) {
	}

	pE_base(const origin_info& o,
		const Cstring& nodeData)
		: ref_count(0),
			mainIndex(0),
			file(o.file),
			line(o.line) {
	}


	virtual ~pE_base() {}

	virtual const char*	spell() const {
		return "pE_base";
	}

	void indent(aoString* s, int indentation) const {
		for(int i = 0; i < indentation; i++) {
			*s << " ";
		}
	}

	virtual void eval_expression(TypedValue& result) {
	}

	const pE_base* This() const {
		return this;
	}

	const Cstring&	getData() const {
		return theData;
	}
	void			to_stream(
					std::ostream*		oS) const {
		aoString	CO;

		to_stream(&CO, 0);
		(*oS) << CO.str();
	}


	virtual void		to_stream(
					aoString*		out,
					int			indentation) const {
		int	current_index = 0;
		indent(out, indentation);
		if(mainIndex == current_index) {
			(*out) << theData;
			current_index++;
		}
		for(int i = 0; i < expressions.size(); i++) {
			expressions[i]->to_stream(out, indentation);
			current_index++;
			if(mainIndex == current_index) {
				(*out) << " " << theData;
				current_index++;
			}
		}
		if(theData == ";") {
			(*out) << "\n";
		}
	}
	Cstring			to_string() const {
		aoString aos;
		to_stream(&aos, 0);
		return aos.str();
	}
};

class pE: public pE_base {
private:
	TypedValue	val;
public:
	TypedValue&	get_val() {
		return val;
	}
	const TypedValue& get_val() const {
		return val;
	}
	pE(const origin_info& o) : pE_base(o) {}
	pE(const pE& C) : pE_base(C) {}
	pE(bool, const pE& C) : pE_base(true, C) {
	}
	pE(	const origin_info& o,
		const Cstring& nodeData,
		long int semantic_value = 0)
		: pE_base(o, nodeData) {
		val = semantic_value;
	}
	virtual pE* copy() override { return new pE(*this); }
	virtual pE* shallow_copy() override { return new pE(true, *this); }
};

#include "gen_SC_exp.H"

} // namespace SC

#endif /* _SC_PE_SYS_H_ */
