#ifndef _TOL_PE_SYS_H_
#define _TOL_PE_SYS_H_

#include <map>
#include <list>
#include <vector>
#include <atomic>
#include <thread>

#include <sys/types.h>

#include "templates.H"
#include "APbasic.H"
#include "alphastring.H"
#include "smartptr.H"

//
// The TypedValue class is not used, but for compatibility
// with gramgen we still define an eval_expression() method
// which requires it. Since it is unused, the TypedValue
// class only needs to be declared; it does not have to
// be defined.
//
class TypedValue;


//
// interface to the lex stuff, which we do not want to include here:
//
extern int tol_get_line_number(int the_thread_index);

//
// Used in gen_tol_exp.C:
//
#define tollineno tol_get_line_number(tol::which_parser(thread_index))


namespace tol {

//
// We have two parsers; each one needs 'globals'. To
// make this work, we use the following function,
// defined in tolcomp.C:
//
extern int which_parser(int indx);

class pE;
class Record;
}

typedef smart_ptr<tol::pE>		tolExp;
typedef Cnode0<alpha_int, tolExp>	tolNode;
typedef slist<alpha_int, tolNode>	tolList;

//
// tol = Time-Ordered Listing
//
namespace tol {

#ifdef MIGRATED_TO_TOL_DATA_H

class abstract_type {
public:
	virtual const Cstring		to_string() const = 0;
	virtual apgen::DATA_TYPE	type() const = 0;
};

class double_type: public abstract_type {
public:
	double_type() : val(0.0) {}
	double_type(const double_type& D)
		: val(D.val) {}
	double_type(double d)
		: val(d) {}
	~double_type() {}

	double_type operator=(const double_type& D) {
		val = D.val;
		return *this;
	}
	double_type operator=(double D) {
		val = D;
		return *this;
	}

	virtual const Cstring to_string() const override {
		return Cstring(val);
	}
	virtual apgen::DATA_TYPE	type() const override {
		return apgen::DATA_TYPE::FLOATING;
	}

	double	val;
};

class integer_type: public abstract_type {
public:
	integer_type() : val(0) {}
	integer_type(const integer_type& D)
		: val(D.val) {}
	integer_type(long int I)
		: val(I) {}
	~integer_type() {}

	integer_type operator=(const integer_type& I) {
		val = I.val;
		return *this;
	}
	integer_type operator=(long int I) {
		val = I;
		return *this;
	}

	virtual const Cstring to_string() const override {
		return Cstring(val);
	}
	virtual apgen::DATA_TYPE	type() const override {
		return apgen::DATA_TYPE::INTEGER;
	}

	long int val;
};

class time_type: public abstract_type {
public:
	time_type() : val(0) {}
	time_type(const time_type& D)
		: val(D.val) {}
	time_type(CTime_base T)
		: val(T) {}
	~time_type() {}

	time_type operator=(const time_type& I) {
		val = I.val;
		return *this;
	}
	time_type operator=(CTime_base T) {
		val = T;
		return *this;
	}

	virtual const Cstring to_string() const override {
		return val.to_string();
	}
	virtual apgen::DATA_TYPE	type() const override {
		return apgen::DATA_TYPE::TIME;
	}

	CTime_base val;
};

class duration_type: public abstract_type {
public:
	duration_type() : val(0) {}
	duration_type(const duration_type& D)
		: val(D.val) {}
	duration_type(CTime_base D)
		: val(D) {}
	~duration_type() {}

	duration_type operator=(const duration_type& I) {
		val = I.val;
		return *this;
	}
	duration_type operator=(CTime_base T) {
		val = T;
		return *this;
	}

	virtual const Cstring to_string() const override {
		return val.to_string();
	}
	virtual apgen::DATA_TYPE	type() const override {
		return apgen::DATA_TYPE::DURATION;
	}

	CTime_base val;
};

class bool_type: public abstract_type {
public:
	bool_type() : val(0) {}
	bool_type(const bool_type& D)
		: val(D.val) {}
	bool_type(bool B)
		: val(B) {}
	~bool_type() {}

	bool_type operator=(const bool_type& I) {
		val = I.val;
		return *this;
	}
	bool_type operator=(bool T) {
		val = T;
		return *this;
	}

	virtual const Cstring to_string() const override {
		if(val) {
			return "true";
		} else {
			return "false";
		}
	}
	virtual apgen::DATA_TYPE	type() const override {
		return apgen::DATA_TYPE::BOOL_TYPE;
	}

	bool val;
};

class string_type: public abstract_type {
public:
	string_type() : val(0) {}
	string_type(const string_type& D)
		: val(D.val) {}
	string_type(const Cstring& s)
		: val(s) {}
	~string_type() {}

	string_type operator=(const string_type& I) {
		val = I.val;
		return *this;
	}
	string_type operator=(const Cstring& s) {
		val = s;
		return *this;
	}

	virtual const Cstring to_string() const override {
		return val;
	}
	virtual apgen::DATA_TYPE	type() const override {
		return apgen::DATA_TYPE::STRING;
	}

	Cstring val;
};

class abs_res_data {
public:
	abs_res_data(const Cstring& s)
		: name(s) {}
	abs_res_data(CTime_base T, const Cstring& s)
		: name(s),
       		last_known_time(T) {}
	virtual ~abs_res_data() {}

	virtual const Cstring		get_name() const {
		return name;
	}
	virtual const Cstring		get_value_string() const {
		return get_value().to_string();
	}
	virtual CTime_base		get_time() const {
		return last_known_time;
	}

	Cstring				name;
	CTime_base			last_known_time;
	virtual const abstract_type&	get_value() const = 0;
};

template <typename value_type>
class res_data: public abs_res_data {
public:
	res_data() = default;
	res_data(const res_data&) = default;

	res_data(const Cstring& s)
		: abs_res_data(s) {}
	res_data(CTime_base T, const Cstring& s)
		: abs_res_data(T, s) {}
	res_data(CTime_base T, const Cstring& s, const value_type& V)
		: abs_res_data(T, s),
		value(V) {}
	virtual ~res_data() = default;

	virtual const value_type&	get_value() const override {
		return value;
	}

	value_type	value;
};

map<Cstring, int>&	resource_indices(int);
vector<unique_ptr<abs_res_data> >& resource_data(int);
vector<int>&		repeated_time_tags(int);
vector<int>&		repeated_values(int);
vector<int>&		repeated_tags_and_values(int);

#endif /* MIGRATED_TO_TOL_DATA_H */

//
// Interface (pure abstract class) for recursive search
//
class exp_analyzer {
public:
	exp_analyzer() {}
	exp_analyzer(const exp_analyzer&) = delete;
	virtual ~exp_analyzer() {}
	virtual void pre_analyze(pE*) = 0;
	virtual void post_analyze(pE*) = 0;
	virtual bool inhibit(pE*) { return false; }
};

class origin_info {
public:
	origin_info() : file("unknown"), line(1) {}
	origin_info(int L , const Cstring& F) : file(F), line(L) {}

	origin_info(const origin_info& o) : line(o.line), file(o.file) {}
	~origin_info() {}

	int line;
	Cstring file;
};

class pE {
protected:
	Cstring		theData;
public:

	//
	// Instead of using static data, we have to use one
	// set of 'globals' for each parsing thread. So,
	// we use static accessors taking a parser index
	// argument denoted by P_index. The link between
	// thread_index and P_index is provided by the
	// function which_parser(thread_index).
	//

	//
	// The following statics are controlled and/or used
	// by the parsing threads:
	//
	static tolList&			theRecords(int P_index);
	static tolList&			theActTypes(int P_index);
	static std::atomic<tolNode*>&	LastParsedRecord(int P_index);
	static std::atomic<tolNode*>&	FirstRecordToProcess(int P_index);
	static std::atomic<tolNode*>&	LastProcessedRecord(int P_index);
	static std::atomic<bool>&	file_parsing_complete(int P_index);
	static std::atomic<bool>&	terminate_parsing(int P_index);
	static std::atomic<long int>&	first_time_tag(int P_index);

	//
	// Not atomic because it is not shared between threads:
	//
	static bool&			first_record(int P_index);

	vector<tolExp>	expressions;
	int		mainIndex;

	//
	// parsing support:
	//
	int		lval;
	int		line;
	Cstring		file;

	pE(const origin_info& o)
		: lval(0),
			ref_count(0),
			mainIndex(0),
			file(o.file),
			line(o.line) {
	}
	// deep copy:
	pE(const pE& C)
		: theData(C.theData),
			ref_count(0),
			mainIndex(0),
			lval(C.lval),
			file(C.file),
			line(C.line) {
		for(int i = 0; i < C.expressions.size(); i++) {
			if(C.expressions[i]) {
				expressions.push_back(tolExp(
					C.expressions[i]->copy()));
			} else {
				expressions.push_back(tolExp());
			}
		}
	}
	// shallow copy:
	pE(bool, const pE& C)
		: theData(C.theData),
			ref_count(0),
			lval(C.lval),
			mainIndex(0),
			file(C.file),
			line(C.line),
			expressions(C.expressions) {
	}

	pE(	const origin_info& o,
		const Cstring& nodeData,
		long int semantic_value = 0)
		: lval(semantic_value),
			ref_count(0),
			theData(nodeData),
			mainIndex(0),
			file(o.file),
			line(o.line) {
	}
	virtual ~pE() {}

	virtual void initExpression(const Cstring& nodeData) {}

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
	virtual const char*	spell() const {
	    return "pE";
	}
	virtual void eval_expression(TypedValue& result) {
	}
	void indent(aoString* s, int indentation) const {
		for(int i = 0; i < indentation; i++) {
			*s << " ";
		}
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
	virtual const Cstring&	getData() const {
		return theData;
	}
	virtual vector<tolExp>*	getExpressions() {
		return &expressions;
	}
	virtual void	recursively_apply(exp_analyzer& EA) {
		EA.pre_analyze(this);
		for(int i = 0; i < expressions.size(); i++) {
			expressions[i]->recursively_apply(EA);
		}
		EA.post_analyze(this);
	}
	virtual void	addExp(
				const tolExp& pe) {
		expressions.push_back(pe);
	}
	virtual pE* copy() {
		return new pE(*this);
	}
	virtual pE* shallow_copy() {
		return new pE(true, *this);
	}
	virtual pE* get(long int i) {
		if(i >= expressions.size()) {
			return NULL;
		}
		return expressions[i].object();
	}
	void			to_stream(
			std::ostream*		oS) const {
		aoString	CO;

		to_stream(&CO, 0);
		(*oS) << CO.str();
	}
	Cstring			to_string() const {
		aoString aos;
		to_stream(&aos, 0);
		return aos.str();
	}
};

#include "gen_tol_exp.H"

} // namespace tol

#endif /* _TOL_PE_SYS_H_ */
