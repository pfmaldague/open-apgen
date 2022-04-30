#ifndef _AP_PE_SYS_H_
#define _AP_PE_SYS_H_

#include <map>
#include <set>
#include <list>

#include <sys/types.h>

#include <APdata.H>
#include <ExecStack.H>
#include <Rsource.H>
#include <flexval.H>
#include <smartptr.H>

typedef smart_ptr<pEsys::pE>		parsedExp;
typedef Cnode0<alpha_string, parsedExp>	bsymbolnode;

#include <ExecutionContext.H>

class ActivityInstance;
class res_info_for_constraintPLD;
typedef Cnode0<alpha_string, res_info_for_constraintPLD> Res_node;
// class AbstractResource;
class usage_event;
class exec_status;
class Cstack;
class Constraint;

//
// pE = ParsedExpression
//
namespace pEsys {

class pE;

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

Cstring print_func(generic_function func);

	//
	// Static method for handling event creation in the past; depending on
	// command-line options, this may result in nothing, in a warning to
	// stdout, or in a thrown exception.
	//
bool CreatingEventsInThePast(pE* here);

class origin_info {
public:
	origin_info() : file("unknown"), line(1) {}
	origin_info(int L, const Cstring& F) : file(F), line(L) {}
	origin_info(const origin_info& o) : line(o.line), file(o.file) {}
	~origin_info() {}

	int line;
	Cstring file;
};

/* a pE comes directly out of yacc and lex, and it contains
 * the parse tree where each node can be evaluated by a program. */
class pE {
protected:
	Cstring			theData;
	//constructor helper because C++ doesn't support constructors calling each other
	virtual void initExpression(
			const Cstring&	nodeData);

public:

	typedef enum {
		SYNCHRONOUS_STYLE,
		ASYNCHRONOUS_STYLE } ProgStyle;

	vector<parsedExp>	expressions;
	int			mainIndex;

	virtual void	recursively_apply(exp_analyzer& EA) {
		EA.pre_analyze(this);
		for(int i = 0; i < expressions.size(); i++) {
			expressions[i]->recursively_apply(EA);
		}
		EA.post_analyze(this);
	}
	virtual void	addExp(
				const parsedExp& pe) {
		expressions.push_back(pe);
	}
	// execution support:
	virtual bool	can_wait() const {
		return false;
	}
	virtual bool	can_interrupt() const {
		return false;
	}
	virtual bool	can_push() const {
		return false;
	}
	virtual bool	can_return() const {
		return false;
	}

	//
	// smart_ptr support:
	//
	int			ref_count;
	int			ref() {
		ref_count++;
		return ref_count;
	}
	int			deref() {
		ref_count--;
		return ref_count;
	}

	//
	// parsing support:
	//
	int			lval;
	Cstring			file;
	int			line;

	//
	// deep copy:
	//
	virtual pE* copy() { assert(false); }

	//
	// shallow copy:
	//
	virtual pE* shallow_copy() { assert(false); }

	//
	// Method to help implement efficient copying.
	// It creates a copy of oldobject, but only if
	// it can't find a previously created copy in
	// new_objects:
	//
	pE*	smart_copy(
		    tlist<alpha_void, Cnode0<alpha_void, pE*> >& new_objects) {
		Cnode0<alpha_void, pE*>* newobjptr
				= new_objects.find(this);
		pE* newobj;

		//
		// This is the essential step; it guarantees
		// that we don't copy an object more than once
		//
		if(newobjptr) {
			newobj = newobjptr->payload;
		} else {
			new_objects << new Cnode0<alpha_void, pE*>(
				this,
				(newobj = copy())
				);
		}
		return newobj;
	}
				
	pE() = delete;


	//
	// Vanilla constructor:
	//
	pE(const origin_info& o)
		: ref_count(0),
			lval(0),
			mainIndex(0),
			file(o.file),
			line(o.line) {
	}

	//
	// deep copy constructor:
	//
	pE(const pE& C);

	//
	// shallow copy constructor:
	//
	pE(bool, const pE& C);

	//
	// Vanilla constructor:
	//
	pE(	const origin_info& o,
		const Cstring& nodeData,
		int	semantic_value = 0);

	virtual ~pE() {}

	virtual TypedValue* apply_indices_to(
				TypedValue*	lhsval,
				behaving_element& loc,
				bool tolerant = false) {
		return NULL;
	}
	
	virtual const char*	spell() const {
		return "pE";
	}

	void indent(aoString* s, int indentation) const;

	//
	// The following data member is an invitation to disaster:
	// it is not thread-safe. Multiple threads will collide
	// when evaluating the same expression.
	//
	// It needs to be removed.
	//
	// TypedValue		val;

	void			extract_epoch_if_applicable(
					Dsource*		req,
					bool			extend_to_children);

	//
	// get_val() is only used in a few places:
	// left-hand side of assignment expressions,
	// and resource values (e. g. currentval()).
	// The real semantics is "find where the value
	// is located". Unfortunately, for resource
	// values the stored object is not necessarily
	// a TypedValue; some translation is necessary.
	//
	// It would be nice if this could be eliminated
	// and if a clear distinction could be made
	// between _computing_ and _fetching_ a value.
	//
	// Actually, there are three things the API
	// client may want to do:
	//
	// 	- fetching the value
	// 	- fetching the address of the value
	// 	- updating the value (wherever it is)
	//
	// updating and fetching is of course a frequently
	// used combination. So is updating and storing
	// at a specified location.
	//

	//
	// Tentative dictionary:
	//
	// current		"correct"		remark
	// =======		=========		======
	//
	// eval_lval()		get_computed_ref()	Implied: stored internally
	// 						Note: many variations are
	// 						available for getting refs
	// 						to specific value types
	//
	// eval_val()		get_computed_val()	Does not really exist;
	// 						however, variations that
	// 						return builtin objects do,
	// 						like get_computed_int() and
	// 						get_computed_duration()
	//
	// eval_expression()	compute_and_store()	Client supplies a value ref.
	// 						Note: many variations exit,
	// 						for example compute_and_store_int() and
	// 						compute_and_store_duration().
	//
	// eval_expression()	compute_internal()	Stores the result in the
	// 						location allocated to the value
	// 						(e. g. a symbol table in the
	// 						object passed as an argument)
	//
	// get_lval()		get_ref()		Does not evaluate - but this
	// 						is misleading since computing
	// 						an address requires a real
	// 						computation in the case of
	// 						class members and array
	// 						elements
	//

	//
	// Additional note:
	//
	// Letting a TypedValue local to the pE play the role of a reference
	// invites big problems with multithreading: it is equivalent to a global.
	//



	//
	// to support easy initialization of smart pointers
	// to this, which should never delete their object:
	//
	const pE* This() const {
		return this;
	}

	virtual apgen::DATA_TYPE get_result_type() const {
		return apgen::DATA_TYPE::UNINITIALIZED;
	}

	//
	// RETURN REFERENCE TO EVALUATED VALUE
	// ===================================
	//
	virtual long int&	get_int_ref(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::get_int_ref(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"storage location.";
		throw(eval_error(errs));
	}
	virtual double&		get_double_ref(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::get_double_ref(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"storage location.";
		throw(eval_error(errs));
	}
	virtual long int&	get_bool_ref(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::get_bool_ref(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"storage location.";
		throw(eval_error(errs));
	}
	virtual CTime_base&	get_time_ref(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::get_time_ref(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"storage location.";
		throw(eval_error(errs));
	}
	virtual CTime_base&	get_duration_ref(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::get_duration_ref(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"storage location.";
		throw(eval_error(errs));
	}
	virtual Cstring&		get_string_ref(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::get_string_ref(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"storage location.";
		throw(eval_error(errs));
	}
	virtual behaving_element&	get_instance_ref(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::get_instance_ref(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"storage location.";
		throw(eval_error(errs));
	}
	virtual TypedValue&		get_array_ref(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::get_array_ref(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"storage location.";
		throw(eval_error(errs));
	}

	//
	// RETURN EVALUATED OBJECT
	// =======================
	//
	virtual long int	eval_int_val(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual double		eval_double_val(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual bool		eval_bool_val(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual CTime_base	eval_time_val(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual CTime_base	eval_duration_val(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual Cstring		eval_string_val(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual TypedValue	eval_array_val(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual behaving_element eval_instance_val(
				    behaving_base*	loc) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}

	//
	// stores result(s) in the TypedValue provided by the caller.
	// All derived classes must override this method. However,
	// we do not want to make this class pure virtual, so we
	// just throw an exception.
	//
	virtual void		eval_expression(
				    behaving_base*	loc,
				    TypedValue&		result) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"storage location.";
		throw(eval_error(errs));
	}

	//
	// stores result(s) in the TypedValue
	// vector provided by the caller:
	//
	virtual void		eval_expression(
				    behaving_base* loc,
				    vector<TypedValue>& result) {
		Cstring errs;
		errs << spell() << "::eval_expression(obj, vector<>): INTERNAL APGENX ERROR - "
			"this type of expression does not support evaluation of vectors.";
		throw(eval_error(errs));
	}

	//
	// does not store any result; used for function calls
	// that do not return a value
	//
	virtual void		update_expression(
				    behaving_base*	loc) {
		TypedValue val;
		eval_expression(loc, val);
	}

	//
	// RETURN REFERENCE TO STORED VALUE
	// ================================
	//
	virtual TypedValue&	get_val_ref(
				    behaving_base* loc) {
		Cstring errs;
		errs << spell() << "::get_val_ref(obj, vector<>): INTERNAL APGENX ERROR - "
			"this type of expression does not support evaluation "
			"of stored value references.";
		throw(eval_error(errs));
	}

	//
	// for derived classes selected at
	// consolidation time:
	//
	virtual void	eval_int(
				behaving_base*	loc,
				long int&) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual void	eval_double(
				behaving_base* obj,
				double& L) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual void	eval_bool(
				behaving_base*	loc,
				long int&) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}

	virtual void	eval_time(
				behaving_base*	loc,
				CTime_base&) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}

	virtual void	eval_duration(
				behaving_base*	loc,
				CTime_base&) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}

	virtual void	eval_string(
				behaving_base*	loc,
				Cstring&) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}

	virtual void	eval_array(
				behaving_base*	loc,
				TypedValue&) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}
	virtual void	eval_instance(
				behaving_base*	loc,
				behaving_element&) {
		Cstring errs;
		errs << spell() << "::eval_lval(): INTERNAL APGENX ERROR - "
			"this type of expression does not have a standard "
			"value type";
		throw(eval_error(errs));
	}

	const Cstring&	getData() const {
		return theData;
	}

	bool			is_a_simple_duration() const;

	bool			is_epoch_relative(
					Cstring&		epoch_name) const;

	void			print(
					Cstring&		S) const;
	const char*		serial() const;

	void			to_stream(
					std::ostream*		oS) const;

	virtual void		to_stream(
					aoString*		outStream,
					int			indentation) const;
	Cstring			to_string() const {
		aoString aos;
		to_stream(&aos, 0);
		return aos.str();
	}
};

class pE_w_string: public pE {
public:
	pE_w_string(const origin_info& o) : pE(o) {}
	pE_w_string(const pE_w_string& C) : pE(C) {}
	pE_w_string(bool, const pE_w_string& C) : pE(true, C) {
	}
	pE_w_string(	const origin_info& o,
		const Cstring& nodeData,
		int	semantic_value = 0) : pE(o, nodeData, semantic_value) {}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	result) override {
		result = getData();
	}
	virtual pE* copy() override { return new pE_w_string(*this); }
	virtual pE* shallow_copy() override { return new pE_w_string(true, *this); }
};

class executableExp: public pE {
public:
	executableExp(const origin_info& o)
		: pE(o) {}
	executableExp(bool, const origin_info& o)
		: pE(true, o) {}
	executableExp(const origin_info& o,
			const Cstring& s)
		: pE(o, s) {}

	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				execStack*		stack_to_use = NULL) = 0;

	//
	// Only used by Program - maybe we should take this out?
	//
	virtual void	to_AAF(aoString* aos, int dent) const {
		to_stream(aos, dent);
	}
};

#include "gen-parsedExp.H"


class Assign2Attr: public Assignment {
public:
	Assign2Attr() = delete;
	Assign2Attr(Assignment& assign) : Assignment(assign) {
		symbol = dynamic_cast<Symbol*>(lhs_of_assign.object());
	}
	Assign2Attr(bool, Assignment& assign) : Assignment(true, assign) {
		symbol = dynamic_cast<Symbol*>(lhs_of_assign.object());
	}
	virtual pE*	copy() override {
		return new Assign2Attr(*this);
	}
	virtual pE*	shallow_copy() override {
		return new Assign2Attr(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();
		TypedValue val;

		Expression->eval_expression(obj, val);

		//
		// use this instead of operator[] because
		// it is overridden for activity instances:
		//
		obj->level[symbol->my_level]->assign_w_side_effects(
				symbol->my_index,
				val);
	}
	virtual const char* spell() const override {
		return "Assign2Attr";
	}
	Symbol* symbol;
};

class AssignInt2Int: public Assignment {
public:
	AssignInt2Int(Assignment& assign) : Assignment(assign) {}
	AssignInt2Int(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignInt2Int(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignInt2Int(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_int(obj, lhs_of_assign->get_int_ref(obj));
	}
	virtual const char* spell() const override {
		return "AssignInt2Int";
	}
};

class AssignInt2Double: public Assignment {
public:
	AssignInt2Double(Assignment& assign) : Assignment(assign) {}
	AssignInt2Double(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignInt2Double(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignInt2Double(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_double(obj, lhs_of_assign->get_double_ref(obj));
	}
	virtual const char* spell() const override {
		return "AssignInt2Double";
	}
};

class AssignDouble2Int: public Assignment {
public:
	AssignDouble2Int(Assignment& assign) : Assignment(assign) {}
	AssignDouble2Int(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignDouble2Int(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignDouble2Int(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_int(obj, lhs_of_assign->get_int_ref(obj));
	}
	virtual const char* spell() const override {
		return "AssignDouble2Int";
	}
};

class AssignDouble2Double: public Assignment {
public:
	AssignDouble2Double(Assignment& assign) : Assignment(assign) {}
	AssignDouble2Double(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignDouble2Double(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignDouble2Double(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_double(obj, lhs_of_assign->get_double_ref(obj));
	}
	virtual const char* spell() const override {
		return "AssignDouble2Double";
	}
};

class AssignBool2Bool: public Assignment {
public:
	AssignBool2Bool(Assignment& assign) : Assignment(assign) {}
	AssignBool2Bool(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignBool2Bool(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignBool2Bool(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_bool(obj, lhs_of_assign->get_bool_ref(obj));
	}
	virtual const char* spell() const override {
		return "AssignBool2Bool";
	}
};

class AssignTime2Time: public Assignment {
public:
	AssignTime2Time(Assignment& assign) : Assignment(assign) {}
	AssignTime2Time(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignTime2Time(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignTime2Time(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_time(obj, lhs_of_assign->get_time_ref(obj));
	}
	virtual const char* spell() const override {
		return "AssignTime2Time";
	}
};

class AssignDuration2Duration: public Assignment {
public:
	AssignDuration2Duration(Assignment& assign) : Assignment(assign) {}
	AssignDuration2Duration(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignDuration2Duration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignDuration2Duration(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_duration(obj, lhs_of_assign->get_duration_ref(obj));

	}
	virtual const char* spell() const override {
		return "AssignDuration2Duration";
	}
};

class AssignString2String: public Assignment {
public:
	AssignString2String(Assignment& assign) : Assignment(assign) {}
	AssignString2String(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignString2String(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignString2String(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_string(obj, lhs_of_assign->get_string_ref(obj));
	}
	virtual const char* spell() const override {
		return "AssignString2String";
	}
};

class AssignArray2Array: public Assignment {
public:
	AssignArray2Array(Assignment& assign) : Assignment(assign) {}
	AssignArray2Array(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignArray2Array(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignArray2Array(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_array(obj, lhs_of_assign->get_array_ref(obj));
	}
	virtual const char* spell() const override {
		return "AssignArray2Array";
	}
};

class AppendArray2Array: public Assignment {
public:
	AppendArray2Array(Assignment& assign) : Assignment(assign) {}
	AppendArray2Array(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AppendArray2Array(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AppendArray2Array(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		lhs_of_assign->get_array_ref(obj).merge(
			&Expression->eval_array_val(obj).get_array());
	}
	virtual const char* spell() const override {
		return "AppendArray2Array";
	}
};

class AssignInstance2Instance: public Assignment {
public:
	AssignInstance2Instance(Assignment& assign) : Assignment(assign) {}
	AssignInstance2Instance(bool, Assignment& assign) : Assignment(true, assign) {}
	virtual pE*	copy() override {
		return new AssignInstance2Instance(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AssignInstance2Instance(true, *this);
	}
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override {
		behaving_base* obj = context->AmbientObject.object();

		Expression->eval_instance(obj, lhs_of_assign->get_instance_ref(obj));
	}
	virtual const char* spell() const override {
		return "AssignInstance2Instance";
	}
};

class MultidimIntegers: public MultidimIndices {
public:
	MultidimIntegers(MultidimIndices& mi) : MultidimIndices(mi) {
	}
	MultidimIntegers(bool, MultidimIndices& mi) : MultidimIndices(true, mi) {
	}
	MultidimIntegers(MultidimIntegers& mi) : MultidimIndices(mi) {
	}
	MultidimIntegers(bool, MultidimIntegers& mi) : MultidimIndices(true, mi) {
	}
	virtual pE*	copy() override {
		return new MultidimIntegers(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MultidimIntegers(true, *this);
	}
	virtual TypedValue* apply_indices_to(
				TypedValue* lhsval,
				behaving_base* loc,
				bool tolerant = false) override;
	virtual const char* spell() const override {
		return "MultidimIntegers";
	}
};

class SymbolicInt: public Symbol {
public:
	SymbolicInt(Symbol& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicInt(bool, Symbol& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicInt(SymbolicInt& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicInt(bool, SymbolicInt& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	virtual pE*	copy() override {
		return new SymbolicInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SymbolicInt(true, *this);
	}
	virtual long int& get_int_ref(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index].value.INTEGER;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L)  override {
		L = (*obj->level[my_level])[my_index].value.INTEGER;
	}
	virtual void eval_double(
			behaving_base* obj,
			double& D) override {
		D = (*obj->level[my_level])[my_index].value.INTEGER;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index].value.INTEGER;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index].value.INTEGER;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::INTEGER;
	}
	virtual const char* spell() const override {
		return "SymbolicInt";
	}
};

class SymbolicDouble: public Symbol {
public:
	SymbolicDouble(Symbol& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicDouble(bool, Symbol& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicDouble(SymbolicDouble& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicDouble(bool, SymbolicDouble& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	virtual pE*	copy() override {
		return new SymbolicDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SymbolicDouble(true, *this);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = (*obj->level[my_level])[my_index].value.FLOATING;
	}
	virtual void eval_double(
			behaving_base* obj,
			double& D) override {
		D = (*obj->level[my_level])[my_index].value.FLOATING;
	}
	virtual double& get_double_ref(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index].value.FLOATING;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index].value.FLOATING;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index].value.FLOATING;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::FLOATING;
	}
	virtual const char* spell() const override {
		return "SymbolicDouble";
	}
};

class SymbolicBool: public Symbol {
public:
	SymbolicBool(Symbol& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicBool(bool, Symbol& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicBool(SymbolicBool& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicBool(bool, SymbolicBool& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	virtual pE*	copy() override {
		return new SymbolicBool(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SymbolicBool(true, *this);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L) override {
		L = (*obj->level[my_level])[my_index].value.INTEGER;
	}
	virtual long int& get_bool_ref(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index].value.INTEGER;
	}
	virtual bool eval_bool_val(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index].value.INTEGER;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::BOOL_TYPE;
	}
	virtual const char* spell() const override {
		return "SymbolicBool";
	}
};

class SymbolicTime: public Symbol {
public:
	SymbolicTime(Symbol& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicTime(bool, Symbol& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicTime(SymbolicTime& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicTime(bool, SymbolicTime& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	virtual pE*	copy() override {
		return new SymbolicTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SymbolicTime(true, *this);
	}
	virtual void eval_time(
			behaving_base* obj,
			CTime_base& L) override {
		L = *(*obj->level[my_level])[my_index].value.TM;
	}
	virtual CTime_base& get_time_ref(
			behaving_base* obj) override {
		return *(*obj->level[my_level])[my_index].value.TM;
	}
	virtual CTime_base eval_time_val(
			behaving_base* obj) override {
		return *(*obj->level[my_level])[my_index].value.TM;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::TIME;
	}
	virtual const char* spell() const override {
		return "SymbolicTime";
	}
};

class SymbolicDuration: public Symbol {
public:
	SymbolicDuration(Symbol& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicDuration(bool, Symbol& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicDuration(SymbolicDuration& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicDuration(bool, SymbolicDuration& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	virtual pE*	copy() override {
		return new SymbolicDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SymbolicDuration(true, *this);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = *(*obj->level[my_level])[my_index].value.TM;
	}
	virtual CTime_base& get_duration_ref(
			behaving_base* obj) override {
		return *(*obj->level[my_level])[my_index].value.TM;
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return *(*obj->level[my_level])[my_index].value.TM;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::DURATION;
	}
	virtual const char* spell() const override {
		return "SymbolicDuration";
	}
};

class SymbolicString: public Symbol {
public:
	SymbolicString(Symbol& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicString(bool, Symbol& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicString(SymbolicString& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicString(bool, SymbolicString& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	virtual pE*	copy() override {
		return new SymbolicString(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SymbolicString(true, *this);
	}
	virtual void eval_string(
			behaving_base* obj,
			Cstring& L) override {
		L = *(*obj->level[my_level])[my_index].value.CST;
	}
	virtual Cstring& get_string_ref(
			behaving_base* obj)
				 override {
		return *(*obj->level[my_level])[my_index].value.CST;
	}
	virtual Cstring eval_string_val(
			behaving_base* obj)
				 override {
		return *(*obj->level[my_level])[my_index].value.CST;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::STRING;
	}
	virtual const char* spell() const override {
		return "SymbolicString";
	}
};

class SymbolicArray: public Symbol {
public:
	SymbolicArray(Symbol& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicArray(bool, Symbol& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicArray(SymbolicArray& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicArray(bool, SymbolicArray& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	virtual pE*	copy() override {
		return new SymbolicArray(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SymbolicArray(true, *this);
	}
	virtual void eval_array(
			behaving_base* obj,
			TypedValue& L) override {
		L = *(*obj->level[my_level])[my_index].value.LI;
	}
	virtual TypedValue& get_array_ref(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index];
	}
	virtual TypedValue eval_array_val(
			behaving_base* obj) override {
		return (*obj->level[my_level])[my_index];
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::ARRAY;
	}
	virtual const char* spell() const override {
		return "SymbolicArray";
	}
};

class SymbolicInstance: public Symbol {
public:
	SymbolicInstance(Symbol& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicInstance(bool, Symbol& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicInstance(SymbolicInstance& s) : Symbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	SymbolicInstance(bool, SymbolicInstance& s) : Symbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	virtual pE*	copy() override {
		return new SymbolicInstance(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SymbolicInstance(true, *this);
	}
	virtual void eval_instance(
			behaving_base* obj,
			behaving_element& L) override {
		L = *(*obj->level[my_level])[my_index].value.INST;
	}
	virtual behaving_element& get_instance_ref(
			behaving_base* obj) override {
		return *(*obj->level[my_level])[my_index].value.INST;
	}
	virtual behaving_element eval_instance_val(
			behaving_base* obj) override {
		return *(*obj->level[my_level])[my_index].value.INST;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::INSTANCE;
	}
	virtual const char* spell() const override {
		return "SymbolicInstance";
	}
};

class IndexedSymbolicDouble: public QualifiedSymbol {
public:
	IndexedSymbolicDouble(QualifiedSymbol& s) : QualifiedSymbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	IndexedSymbolicDouble(bool, QualifiedSymbol& s) : QualifiedSymbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	IndexedSymbolicDouble(IndexedSymbolicDouble& s) : QualifiedSymbol(s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	IndexedSymbolicDouble(bool, IndexedSymbolicDouble& s) : QualifiedSymbol(true, s) {
		my_level = s.my_level;
		my_index = s.my_index;
	}
	virtual pE*	copy() override {
		return new IndexedSymbolicDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new IndexedSymbolicDouble(true, *this);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L)  override {
		L = indices->apply_indices_to(
			&(*obj->level[my_level])[my_index],
			obj)->value.FLOATING;
	}
	virtual void eval_double(
			behaving_base* obj,
			double& D) override {
		D = indices->apply_indices_to(
			&(*obj->level[my_level])[my_index],
			obj)->value.FLOATING;
	}
	virtual double& get_double_ref(
			behaving_base* obj) override {
		return indices->apply_indices_to(
			&(*obj->level[my_level])[my_index],
			obj)->value.FLOATING;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return indices->apply_indices_to(
			&(*obj->level[my_level])[my_index],
			obj)->value.FLOATING;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return indices->apply_indices_to(
			&(*obj->level[my_level])[my_index],
			obj)->value.FLOATING;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::FLOATING;
	}
	virtual const char* spell() const override {
		return "IndexedSymbolicDouble";
	}
};

class ConstantInt: public Constant {
public:
	ConstantInt(Constant& s) : Constant(s) {
		val = s.val;
	}
	ConstantInt(bool, Constant& s) : Constant(true, s) {
		val = s.val;
	}
	ConstantInt(ConstantInt& s) : Constant(s) {
		val = s.val;
	}
	ConstantInt(bool, ConstantInt& s) : Constant(true, s) {
		val = s.val;
	}
	virtual pE*	copy() override {
		return new ConstantInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ConstantInt(true, *this);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L)  override {
		L = val.value.INTEGER;
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = val.value.INTEGER;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return val.value.INTEGER;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return val.value.INTEGER;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::INTEGER;
	}
	virtual const char* spell() const override {
		return "ConstantInt";
	}
};

class ConstantDouble: public Constant {
public:
	ConstantDouble(Constant& s) : Constant(s) {
		val = s.val;
	}
	ConstantDouble(bool, Constant& s) : Constant(true, s) {
		val = s.val;
	}
	ConstantDouble(ConstantDouble& s) : Constant(s) {
		val = s.val;
	}
	ConstantDouble(bool, ConstantDouble& s) : Constant(true, s) {
		val = s.val;
	}
	virtual pE*	copy() override {
		return new ConstantDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ConstantDouble(true, *this);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L)  override {
		L = val.value.FLOATING;
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = val.value.FLOATING;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return val.value.FLOATING;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return val.value.FLOATING;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::FLOATING;
	}
	virtual const char* spell() const override {
		return "ConstantDouble";
	}
};

class ConstantBool: public Constant {
public:
	ConstantBool(Constant& s) : Constant(s) {
		val = s.val;
	}
	ConstantBool(bool, Constant& s) : Constant(true, s) {
		val = s.val;
	}
	ConstantBool(ConstantBool& s) : Constant(s) {
		val = s.val;
	}
	ConstantBool(bool, ConstantBool& s) : Constant(true, s) {
		val = s.val;
	}
	virtual pE*	copy() override {
		return new ConstantBool(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ConstantBool(true, *this);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L) override {
		L = val.value.INTEGER;
	}
	virtual bool eval_bool_val(
			behaving_base* obj) override {
		return val.value.INTEGER;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::BOOL_TYPE;
	}
	virtual const char* spell() const override {
		return "ConstantBool";
	}
};

class ConstantTime: public Constant {
public:
	ConstantTime(Constant& s) : Constant(s) {
		val = s.val;
	}
	ConstantTime(bool, Constant& s) : Constant(true, s) {
		val = s.val;
	}
	ConstantTime(ConstantTime& s) : Constant(s) {
		val = s.val;
	}
	ConstantTime(bool, ConstantTime& s) : Constant(true, s) {
		val = s.val;
	}
	virtual pE*	copy() override {
		return new ConstantTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ConstantTime(true, *this);
	}
	virtual void eval_time(
			behaving_base* obj,
			CTime_base& L) override {
		L = *val.value.TM;
	}
	virtual CTime_base eval_time_val(
			behaving_base* obj) override {
		return *val.value.TM;
	}
	virtual void to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::TIME;
	}
	virtual const char* spell() const override {
		return "ConstantTime";
	}
};

class ConstantDuration: public Constant {
public:
	ConstantDuration(Constant& s) : Constant(s) {
		val = s.val;
	}
	ConstantDuration(bool, Constant& s) : Constant(true, s) {
		val = s.val;
	}
	ConstantDuration(ConstantDuration& s) : Constant(s) {
		val = s.val;
	}
	ConstantDuration(bool, ConstantDuration& s) : Constant(true, s) {
		val = s.val;
	}
	virtual pE*	copy() override {
		return new ConstantDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ConstantDuration(true, *this);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = *val.value.TM;
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return *val.value.TM;
	}
	virtual void to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::DURATION;
	}
	virtual const char* spell() const override {
		return "ConstantDuration";
	}
};

class ConstantString: public Constant {
public:
	ConstantString(Constant& s) : Constant(s) {
		val = s.val;
	}
	ConstantString(bool, Constant& s) : Constant(true, s) {
		val = s.val;
	}
	ConstantString(ConstantString& s) : Constant(s) {
		val = s.val;
	}
	ConstantString(bool, ConstantString& s) : Constant(true, s) {
		val = s.val;
	}
	virtual pE*	copy() override {
		return new ConstantString(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ConstantString(true, *this);
	}
	virtual void eval_string(
			behaving_base* obj,
			Cstring& L)  override {
		L = *val.value.CST;
	}
	virtual Cstring eval_string_val(
			behaving_base* obj)
				 override {
		return *val.value.CST;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::STRING;
	}
	virtual const char* spell() const override {
		return "ConstantString";
	}
};

class ConstantArray: public Constant {
public:
	ConstantArray(Constant& s) : Constant(s) {
		val = s.val;
	}
	ConstantArray(bool, Constant& s) : Constant(true, s) {
		val = s.val;
	}
	ConstantArray(ConstantArray& s) : Constant(s) {
		val = s.val;
	}
	ConstantArray(bool, ConstantArray& s) : Constant(true, s) {
		val = s.val;
	}
	virtual pE*	copy() override {
		return new ConstantArray(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ConstantArray(true, *this);
	}
	virtual void eval_array(
			behaving_base* obj,
			TypedValue& L) override {
		L = val;
	}
	virtual TypedValue eval_array_val(
			behaving_base* obj) override {
		return val;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::ARRAY;
	}
	virtual const char* spell() const override {
		return "ConstantArray";
	}
};

class ConstantInstance: public Constant {
public:
	ConstantInstance(Constant& s) : Constant(s) {
		val = s.val;
	}
	ConstantInstance(bool, Constant& s) : Constant(true, s) {
		val = s.val;
	}
	ConstantInstance(ConstantInstance& s) : Constant(s) {
		val = s.val;
	}
	ConstantInstance(bool, ConstantInstance& s) : Constant(true, s) {
		val = s.val;
	}
	virtual pE*	copy() override {
		return new ConstantInstance(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ConstantInstance(true, *this);
	}
	virtual void eval_instance(
			behaving_base* obj,
			behaving_element& L) override {
		L = *val.value.INST;
	}
	virtual behaving_element eval_instance_val(
			behaving_base* obj) override {
		return *val.value.INST;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::INSTANCE;
	}
	virtual const char* spell() const override {
		return "ConstantInstance";
	}
};

template < class OpClass >
class IntOp: public OpClass {
public:
	IntOp(OpClass& ae) : OpClass(ae) {
	}
	IntOp(bool, OpClass& ae) : OpClass(true, ae) {
	}
	IntOp(IntOp& io) : OpClass(io) {
	}
	IntOp(bool, IntOp& io) : OpClass(true, io) {
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override = 0;
	virtual void eval_double(
			behaving_base* obj,
			double& L) override = 0;
	virtual double eval_double_val(
			behaving_base* obj) override = 0;
	virtual long int eval_int_val(
			behaving_base* obj) override = 0;
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::INTEGER;
	}
};

template < class OpClass >
class DoubleOp: public OpClass {
public:
	DoubleOp(OpClass& ae) : OpClass(ae) {
	}
	DoubleOp(bool, OpClass& ae) : OpClass(true, ae) {
	}
	DoubleOp(DoubleOp& io) : OpClass(io) {
	}
	DoubleOp(bool, DoubleOp& io) : OpClass(true, io) {
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override = 0;
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override = 0;
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::FLOATING;
	}
};

template < class OpClass >
class BoolOp: public OpClass {
public:
	BoolOp(OpClass& ae) : OpClass(ae) {
	}
	BoolOp(bool, OpClass& ae) : OpClass(true, ae) {
	}
	BoolOp(BoolOp& io) : OpClass(io) {
	}
	BoolOp(bool, BoolOp& io) : OpClass(true, io) {
	}
	apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::BOOL_TYPE;
	}
};

template < class OpClass >
class TimeOp: public OpClass {
public:
	TimeOp(OpClass& ae) : OpClass(ae) {
	}
	TimeOp(bool, OpClass& ae) : OpClass(true, ae) {
	}
	TimeOp(TimeOp& io) : OpClass(io) {
	}
	TimeOp(bool, TimeOp& io) : OpClass(true, io) {
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = OpClass::eval_time_val(loc);
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::TIME;
	}
};

template < class OpClass >
class ArrayOp: public OpClass {
public:
	ArrayOp(OpClass& ae) : OpClass(ae) {
	}
	ArrayOp(bool, OpClass& ae) : OpClass(true, ae) {
	}
	ArrayOp(ArrayOp& io) : OpClass(io) {
	}
	ArrayOp(bool, ArrayOp& io) : OpClass(true, io) {
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::ARRAY;
	}
};

template < class OpClass >
class DurationOp: public OpClass {
public:
	DurationOp(OpClass& ae) : OpClass(ae) {
	}
	DurationOp(bool, OpClass& ae) : OpClass(true, ae) {
	}
	DurationOp(DurationOp& io) : OpClass(io) {
	}
	DurationOp(bool, DurationOp& io) : OpClass(true, io) {
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::DURATION;
	}
};

template < class OpClass >
class StringOp: public OpClass {
public:
	StringOp(OpClass& ae) : OpClass(ae) {
	}
	StringOp(bool, OpClass& ae) : OpClass(true, ae) {
	}
	StringOp(StringOp& io) : OpClass(io) {
	}
	StringOp(bool, StringOp& io) : OpClass(true, io) {
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::STRING;
	}
};

class IntFuncCall: public FunctionCall {
public:
	IntFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	IntFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	IntFuncCall(IntFuncCall& fc) : FunctionCall(fc) {}
	IntFuncCall(bool, IntFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new IntFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new IntFuncCall(true, *this);
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.value.INTEGER;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		TypedValue val;
		eval_expression(obj, val);
		L = val.get_int();
	}
	virtual void eval_double(
			behaving_base* obj,
			double &L) override {
		TypedValue val;
		eval_expression(obj, val);
		L = val.get_int();
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::INTEGER;
	}
	virtual const char* spell() const override {
		return "IntFuncCall";
	}
};

class BoolFuncCall: public FunctionCall {
public:
	BoolFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	BoolFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	BoolFuncCall(BoolFuncCall& fc) : FunctionCall(fc) {}
	BoolFuncCall(bool, BoolFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new BoolFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new BoolFuncCall(true, *this);
	}
	virtual bool eval_bool_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.value.INTEGER;
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L) override {
		TypedValue val;
		eval_expression(obj, val);
		L = val.get_int();
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::BOOL_TYPE;
	}
	virtual const char* spell() const override {
		return "BoolFuncCall";
	}
};

class StringFuncCall: public FunctionCall {
public:
	StringFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	StringFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	StringFuncCall(StringFuncCall& fc) : FunctionCall(fc) {}
	StringFuncCall(bool, StringFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new StringFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new StringFuncCall(true, *this);
	}
	virtual Cstring eval_string_val(
			behaving_base* obj)
				 override {
		TypedValue val;
		eval_expression(obj, val);
		return *val.value.CST;
	}
	virtual void eval_string(
			behaving_base* obj,
			Cstring&	L)
				 override {
		TypedValue val;
		eval_expression(obj, val);
		L = *val.value.CST;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::STRING;
	}
	virtual const char* spell() const override {
		return "StringFuncCall";
	}
};

class DoubleFuncCall: public FunctionCall {
public:
	DoubleFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	DoubleFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	DoubleFuncCall(DoubleFuncCall& fc) : FunctionCall(fc) {}
	DoubleFuncCall(bool, DoubleFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new DoubleFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DoubleFuncCall(true, *this);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.value.FLOATING;
	}
	virtual void eval_double(
			behaving_base* obj,
			double&	D) override {
		TypedValue val;
		eval_expression(obj, val);
		D = val.value.FLOATING;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& D) override {
		TypedValue val;
		eval_expression(obj, val);
		D = val.value.FLOATING;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::FLOATING;
	}
	virtual const char* spell() const override {
		return "DoubleFuncCall";
	}
};

class TimeFuncCall: public FunctionCall {
public:
	TimeFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	TimeFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	TimeFuncCall(TimeFuncCall& fc) : FunctionCall(fc) {}
	TimeFuncCall(bool, TimeFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new TimeFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new TimeFuncCall(true, *this);
	}
	virtual CTime_base eval_time_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return *val.value.TM;
	}
	virtual void eval_time(
			behaving_base* obj,
			CTime_base&	T) override {
		TypedValue val;
		eval_expression(obj, val);
		T = *val.value.TM;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::TIME;
	}
	virtual const char* spell() const override {
		return "TimeFuncCall";
	}
};

class DurationFuncCall: public FunctionCall {
public:
	DurationFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	DurationFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	DurationFuncCall(DurationFuncCall& fc) : FunctionCall(fc) {}
	DurationFuncCall(bool, DurationFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new DurationFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DurationFuncCall(true, *this);
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return *val.value.TM;
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base&	T) override {
		TypedValue val;
		eval_expression(obj, val);
		T = *val.value.TM;
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::DURATION;
	}
	virtual const char* spell() const override {
		return "DurationFuncCall";
	}
};

class ArrayFuncCall: public FunctionCall {
public:
	ArrayFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	ArrayFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	ArrayFuncCall(ArrayFuncCall& fc) : FunctionCall(fc) {}
	ArrayFuncCall(bool, ArrayFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new ArrayFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ArrayFuncCall(true, *this);
	}
	virtual TypedValue eval_array_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val;
	}
	virtual void eval_array(
			behaving_base* obj,
			TypedValue&	L) override {
		eval_expression(obj, L);
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::ARRAY;
	}
	virtual const char* spell() const override {
		return "ArrayFuncCall";
	}
};

class QualifiedArrayFuncCall: public FunctionCall {
public:
	QualifiedArrayFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	QualifiedArrayFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	QualifiedArrayFuncCall(QualifiedArrayFuncCall& fc) : FunctionCall(fc) {}
	QualifiedArrayFuncCall(bool, QualifiedArrayFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new QualifiedArrayFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new QualifiedArrayFuncCall(true, *this);
	}
	virtual TypedValue eval_array_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val;
	}
	virtual void eval_array(
			behaving_base* obj,
			TypedValue&	L) override {
		eval_expression(obj, L);
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::UNINITIALIZED;
	}
	virtual const char* spell() const override {
		return "QualifiedArrayFuncCall";
	}
};

class InstanceFuncCall: public FunctionCall {
public:
	InstanceFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	InstanceFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	InstanceFuncCall(InstanceFuncCall& fc) : FunctionCall(fc) {}
	InstanceFuncCall(bool, InstanceFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new InstanceFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new InstanceFuncCall(true, *this);
	}
	virtual behaving_element eval_instance_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return *val.value.INST;
	}
	virtual void eval_instance(
			behaving_base* obj,
			behaving_element& L) override {
		TypedValue val;
		eval_expression(obj, val);
		L = val.get_object();
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::INSTANCE;
	}
	virtual const char* spell() const override {
		return "InstanceFuncCall";
	}
};

class QualifiedInstanceFuncCall: public FunctionCall {
public:
	QualifiedInstanceFuncCall(FunctionCall& fc) : FunctionCall(fc) {}
	QualifiedInstanceFuncCall(bool, FunctionCall& fc) : FunctionCall(true, fc) {}
	QualifiedInstanceFuncCall(QualifiedInstanceFuncCall& fc) : FunctionCall(fc) {}
	QualifiedInstanceFuncCall(bool, QualifiedInstanceFuncCall& fc) : FunctionCall(true, fc) {}
	virtual pE*	copy() override {
		return new QualifiedInstanceFuncCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new QualifiedInstanceFuncCall(true, *this);
	}
	virtual behaving_element eval_instance_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return *val.value.INST;
	}
	virtual void eval_instance(
			behaving_base* obj,
			behaving_element& L) override {
		TypedValue val;
		eval_expression(obj, val);
		L = val.get_object();
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::UNINITIALIZED;
	}
	virtual const char* spell() const override {
		return "QualifiedInstanceFuncCall";
	}
};

#ifdef MAYBE_NOT
class IntResMethodCall: public FunctionCall {
public:
	IntResMethodCall(FunctionCall& fc)
			: FunctionCall(fc), container(NULL) {}
	IntResMethodCall(IntResMethodCall& fc)
			: FunctionCall(fc), container(fc.container) {}
	virtual pE*	copy() override {
		return new IntResMethodCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new IntResMethodCall(true, *this);
	}
	virtual long int& eval_int_lval(
			behaving_base* obj) override {
		return val.value.INTEGER;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.value.INTEGER;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.value.INTEGER;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		TypedValue val;
		eval_expression(obj, val);
		L = val.get_int();
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		TypedValue val;
		eval_expression(obj, val);
		L = val.get_int();
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::INTEGER;
	}
	virtual const char* spell() const override {
		return "IntResMethodCall";
	}
	RCsource* container;
};

class DoubleResMethodCall: public FunctionCall {
public:
	DoubleResMethodCall(FunctionCall& fc)
		: FunctionCall(fc), container(NULL) {}
	DoubleResMethodCall(DoubleResMethodCall& fc)
		: FunctionCall(fc), container(fc.container) {}
	virtual pE*	copy() override {
		return new DoubleResMethodCall(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DoubleResMethodCall(true, *this);
	}
	virtual long int& eval_int_lval(
			behaving_base* obj) override {
		return val.value.INTEGER;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.value.INTEGER;
	}
	virtual double eval_double_val(
			behaving_base* obj) override ;
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		TypedValue val;
		eval_expression(obj, val);
		L = val.get_int();
	}
	virtual apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::INTEGER;
	}
	virtual const char* spell() const override {
		return "DoubleResMethodCall";
	}
	RCsource* container;
};
#endif /* MAYBE_NOT */

class PlusIntInt: public IntOp<AdditiveExp> {
public:
	PlusIntInt(AdditiveExp& ae) : IntOp<AdditiveExp>(ae) {}
	PlusIntInt(bool, AdditiveExp& ae) : IntOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new PlusIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new PlusIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_int_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = Lhs->eval_int_val(obj)
			+ Rhs->eval_int_val(obj);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = Lhs->eval_int_val(obj)
			+ Rhs->eval_int_val(obj);
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return Lhs->eval_int_val(obj)
			+ Rhs->eval_int_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return Lhs->eval_int_val(obj)
			+ Rhs->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "PlusIntInt";
	}
};

class PlusIntDouble: public DoubleOp<AdditiveExp> {
public:
	PlusIntDouble(AdditiveExp& ae) : DoubleOp<AdditiveExp>(ae) {}
	PlusIntDouble(bool, AdditiveExp& ae) : DoubleOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new PlusIntDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new PlusIntDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = Lhs->eval_int_val(obj)
			+ Rhs->eval_double_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = Lhs->eval_int_val(obj)
			+ Rhs->eval_double_val(obj);
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return Lhs->eval_int_val(obj)
			+ Rhs->eval_double_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return Lhs->eval_int_val(obj)
			+ Rhs->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "PlusIntDouble";
	}
};

class PlusDoubleInt: public DoubleOp<AdditiveExp> {
public:
	PlusDoubleInt(AdditiveExp& ae) : DoubleOp<AdditiveExp>(ae) {}
	PlusDoubleInt(bool, AdditiveExp& ae) : DoubleOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new PlusDoubleInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new PlusDoubleInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = Lhs->eval_double_val(obj)
			+ Rhs->eval_int_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = Lhs->eval_double_val(obj)
			+ Rhs->eval_int_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return Lhs->eval_double_val(obj)
			+ Rhs->eval_int_val(obj);
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return Lhs->eval_double_val(obj)
			+ Rhs->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "PlusDoubleInt";
	}
};

class PlusDoubleDouble: public DoubleOp<AdditiveExp> {
public:
	PlusDoubleDouble(AdditiveExp& ae) : DoubleOp<AdditiveExp>(ae) {}
	PlusDoubleDouble(bool, AdditiveExp& ae) : DoubleOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new PlusDoubleDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new PlusDoubleDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = Lhs->eval_double_val(obj)
			+ Rhs->eval_double_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = Lhs->eval_double_val(obj)
			+ Rhs->eval_double_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return Lhs->eval_double_val(obj)
			+ Rhs->eval_double_val(obj);
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return Lhs->eval_double_val(obj)
			+ Rhs->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "PlusDoubleDouble";
	}
};

class PlusTimeDuration: public TimeOp<AdditiveExp> {
public:
	PlusTimeDuration(AdditiveExp& ae) : TimeOp<AdditiveExp>(ae) {}
	PlusTimeDuration(bool, AdditiveExp& ae) : TimeOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new PlusTimeDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new PlusTimeDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_time_val(loc);
	}
	virtual void eval_time(
			behaving_base* obj,
			CTime_base& L) override {
		L = Lhs->eval_time_val(obj)
			+ Rhs->eval_duration_val(obj);
	}
	virtual CTime_base eval_time_val(
			behaving_base* obj) override {
		return Lhs->eval_time_val(obj)
			+ Rhs->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "PlusTimeDuration";
	}
};

class PlusDurationTime: public TimeOp<AdditiveExp> {
public:
	PlusDurationTime(AdditiveExp& ae) : TimeOp<AdditiveExp>(ae) {}
	PlusDurationTime(bool, AdditiveExp& ae) : TimeOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new PlusDurationTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new PlusDurationTime(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_time_val(loc);
	}
	virtual void eval_time(
			behaving_base* obj,
			CTime_base& L) override {
		L = Lhs->eval_duration_val(obj)
			+ Rhs->eval_time_val(obj);
	}
	virtual CTime_base eval_time_val(
			behaving_base* obj) override {
		return Lhs->eval_duration_val(obj)
			+ Rhs->eval_time_val(obj);
	}
	virtual const char* spell() const override {
		return "PlusDurationTime";
	}
};

class PlusDurationDuration: public DurationOp<AdditiveExp> {
public:
	PlusDurationDuration(AdditiveExp& ae) : DurationOp<AdditiveExp>(ae) {}
	PlusDurationDuration(bool, AdditiveExp& ae) : DurationOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new PlusDurationDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new PlusDurationDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = Lhs->eval_duration_val(obj)
			+ Rhs->eval_duration_val(obj);
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return Lhs->eval_duration_val(obj)
			+ Rhs->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "PlusDurationDuration";
	}
};

class PlusArrayArray: public ArrayOp<AdditiveExp> {
public:
	PlusArrayArray(AdditiveExp& ae) : ArrayOp<AdditiveExp>(ae) {}
	PlusArrayArray(bool, AdditiveExp& ae) : ArrayOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new PlusArrayArray(*this);
	}
	virtual pE*	shallow_copy() override {
		return new PlusArrayArray(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_array_val(loc);
	}
	virtual void eval_array(
			behaving_base* obj,
			TypedValue& L) override {
		L = eval_array_val(obj);
	}
	virtual TypedValue eval_array_val(
			behaving_base* obj) override {
		TypedValue V;
		V.add(Lhs->eval_array_val(obj).get_array());
		V.add(Rhs->eval_array_val(obj).get_array());
		return V;
	}
	virtual const char* spell() const override {
		return "PlusArrayArray";
	}
};

class PlusStringString: public StringOp<AdditiveExp> {
public:
	PlusStringString(AdditiveExp& ae) : StringOp<AdditiveExp>(ae) {}
	PlusStringString(bool, AdditiveExp& ae) : StringOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new PlusStringString(*this);
	}
	virtual pE*	shallow_copy() override {
		return new PlusStringString(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_string_val(loc);
	}
	virtual void eval_string(
			behaving_base* obj,
			Cstring& L) override {
		L = Lhs->eval_string_val(obj)
			+ Rhs->eval_string_val(obj);
	}
	virtual Cstring eval_string_val(
			behaving_base* obj)
				 override {
		Cstring s(Lhs->eval_string_val(obj));
		s << Rhs->eval_string_val(obj);
		return s;
	}
	virtual const char* spell() const override {
		return "PlusStringString";
	}
};

class MinusIntInt: public IntOp<AdditiveExp> {
public:
	MinusIntInt(AdditiveExp& ae) : IntOp<AdditiveExp>(ae) {}
	MinusIntInt(bool, AdditiveExp& ae) : IntOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new MinusIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_int_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = Lhs->eval_int_val(obj)
			- Rhs->eval_int_val(obj);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = Lhs->eval_int_val(obj)
			- Rhs->eval_int_val(obj);
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return Lhs->eval_int_val(obj)
			- Rhs->eval_int_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return Lhs->eval_int_val(obj)
			- Rhs->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusIntInt";
	}
};

class MinusIntDouble: public DoubleOp<AdditiveExp> {
public:
	MinusIntDouble(AdditiveExp& ae) : DoubleOp<AdditiveExp>(ae) {}
	MinusIntDouble(bool, AdditiveExp& ae) : DoubleOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new MinusIntDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusIntDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = Lhs->eval_int_val(obj)
			- Rhs->eval_double_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = Lhs->eval_int_val(obj)
			- Rhs->eval_double_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return Lhs->eval_int_val(obj)
			- Rhs->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusIntDouble";
	}
};

class MinusDoubleInt: public DoubleOp<AdditiveExp> {
public:
	MinusDoubleInt(AdditiveExp& ae) : DoubleOp<AdditiveExp>(ae) {}
	MinusDoubleInt(bool, AdditiveExp& ae) : DoubleOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new MinusDoubleInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusDoubleInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = Lhs->eval_double_val(obj)
			- Rhs->eval_int_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = Lhs->eval_double_val(obj)
			- Rhs->eval_int_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return Lhs->eval_double_val(obj)
			- Rhs->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusDoubleInt";
	}
};

class MinusDoubleDouble: public DoubleOp<AdditiveExp> {
public:
	MinusDoubleDouble(AdditiveExp& ae) : DoubleOp<AdditiveExp>(ae) {}
	MinusDoubleDouble(bool, AdditiveExp& ae) : DoubleOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new MinusDoubleDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusDoubleDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = Lhs->eval_double_val(obj)
			- Rhs->eval_double_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = Lhs->eval_double_val(obj)
			- Rhs->eval_double_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return Lhs->eval_double_val(obj)
			- Rhs->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusDoubleDouble";
	}
};

class MinusTimeTime: public DurationOp<AdditiveExp> {
public:
	MinusTimeTime(AdditiveExp& ae) : DurationOp<AdditiveExp>(ae) {}
	MinusTimeTime(bool, AdditiveExp& ae) : DurationOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new MinusTimeTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusTimeTime(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = Lhs->eval_time_val(obj)
			- Rhs->eval_time_val(obj);
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return Lhs->eval_time_val(obj)
			- Rhs->eval_time_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusTimeTime";
	}
};

class MinusTimeDuration: public TimeOp<AdditiveExp> {
public:
	MinusTimeDuration(AdditiveExp& ae) : TimeOp<AdditiveExp>(ae) {}
	MinusTimeDuration(bool, AdditiveExp& ae) : TimeOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new MinusTimeDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusTimeDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_time_val(loc);
	}
	virtual void eval_time(
			behaving_base* obj,
			CTime_base& L) override {
		L = Lhs->eval_time_val(obj)
			- Rhs->eval_duration_val(obj);
	}
	virtual CTime_base eval_time_val(
			behaving_base* obj) override {
		return Lhs->eval_time_val(obj)
			- Rhs->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusTimeDuration";
	}
};

class MinusDurationDuration: public DurationOp<AdditiveExp> {
public:
	MinusDurationDuration(AdditiveExp& ae) : DurationOp<AdditiveExp>(ae) {}
	MinusDurationDuration(bool, AdditiveExp& ae) : DurationOp<AdditiveExp>(true, ae) {}
	virtual pE*	copy() override {
		return new MinusDurationDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusDurationDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = Lhs->eval_duration_val(obj)
			- Rhs->eval_duration_val(obj);
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return Lhs->eval_duration_val(obj)
			- Rhs->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusDurationDuration";
	}
};

class RaiseToPowerIntInt: public DoubleOp<RaiseToPower> {
public:
	RaiseToPowerIntInt(RaiseToPower& me) : DoubleOp<RaiseToPower>(me) {}
	RaiseToPowerIntInt(bool, RaiseToPower& me) : DoubleOp<RaiseToPower>(true, me) {}
	virtual pE*	copy() override {
		return new RaiseToPowerIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new RaiseToPowerIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_double_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = pow(maybe_a_factor->eval_double_val(obj),
			atom->eval_double_val(obj));
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = pow(maybe_a_factor->eval_double_val(obj),
			atom->eval_double_val(obj));
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return pow(maybe_a_factor->eval_double_val(obj),
			atom->eval_double_val(obj));
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return pow(maybe_a_factor->eval_double_val(obj),
			atom->eval_double_val(obj));
	}
	virtual const char* spell() const override {
		return "RaiseToPowerIntInt";
	}
};

class RaiseToPowerDoubleInt: public DoubleOp<RaiseToPower> {
public:
	RaiseToPowerDoubleInt(RaiseToPower& me) : DoubleOp<RaiseToPower>(me) {}
	RaiseToPowerDoubleInt(bool, RaiseToPower& me) : DoubleOp<RaiseToPower>(true, me) {}
	virtual pE*	copy() override {
		return new RaiseToPowerDoubleInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new RaiseToPowerDoubleInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override{
		V = eval_double_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = pow(maybe_a_factor->eval_double_val(obj),
			(double)atom->eval_int_val(obj));
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = pow(maybe_a_factor->eval_double_val(obj),
			(double)atom->eval_int_val(obj));
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return pow(maybe_a_factor->eval_double_val(obj),
			(double)atom->eval_int_val(obj));
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return pow(maybe_a_factor->eval_double_val(obj),
			(double)atom->eval_int_val(obj));
	}
	virtual const char* spell() const override {
		return "RaiseToPowerDoubleInt";
	}
};

class RaiseToPowerIntDouble: public DoubleOp<RaiseToPower> {
public:
	RaiseToPowerIntDouble(RaiseToPower& me) : DoubleOp<RaiseToPower>(me) {}
	RaiseToPowerIntDouble(bool, RaiseToPower& me) : DoubleOp<RaiseToPower>(true, me) {}
	virtual pE*	copy() override {
		return new RaiseToPowerIntDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new RaiseToPowerIntDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override{
		V = eval_double_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = pow((double)maybe_a_factor->eval_int_val(obj),
			atom->eval_double_val(obj));
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = pow((double)maybe_a_factor->eval_int_val(obj),
			atom->eval_double_val(obj));
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return pow((double)maybe_a_factor->eval_int_val(obj),
			atom->eval_double_val(obj));
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return pow((double)maybe_a_factor->eval_int_val(obj),
			atom->eval_double_val(obj));
	}
	virtual const char* spell() const override {
		return "RaiseToPowerIntDouble";
	}
};

class RaiseToPowerDoubleDouble: public DoubleOp<RaiseToPower> {
public:
	RaiseToPowerDoubleDouble(RaiseToPower& me) : DoubleOp<RaiseToPower>(me) {}
	RaiseToPowerDoubleDouble(bool, RaiseToPower& me) : DoubleOp<RaiseToPower>(true, me) {}
	virtual pE*	copy() override {
		return new RaiseToPowerDoubleDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new RaiseToPowerDoubleDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override{
		V = eval_double_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = pow(maybe_a_factor->eval_double_val(obj),
			atom->eval_double_val(obj));
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = pow(maybe_a_factor->eval_double_val(obj),
			atom->eval_double_val(obj));
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return pow(maybe_a_factor->eval_double_val(obj),
			atom->eval_double_val(obj));
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return pow(maybe_a_factor->eval_double_val(obj),
			atom->eval_double_val(obj));
	}
	virtual const char* spell() const override {
		return "RaiseToPowerDoubleDouble";
	}
};

class TimesIntInt: public IntOp<MultiplicativeExp> {
public:
	TimesIntInt(MultiplicativeExp& me) : IntOp<MultiplicativeExp>(me) {}
	TimesIntInt(bool, MultiplicativeExp& me) : IntOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new TimesIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new TimesIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_int_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = maybe_a_product->eval_int_val(obj)
			* maybe_a_factor->eval_int_val(obj);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = maybe_a_product->eval_int_val(obj)
			* maybe_a_factor->eval_int_val(obj);
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return maybe_a_product->eval_int_val(obj)
			* maybe_a_factor->eval_int_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return maybe_a_product->eval_int_val(obj)
			* maybe_a_factor->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "TimesIntInt";
	}
};

class TimesDurationInt: public DurationOp<MultiplicativeExp> {
public:
	TimesDurationInt(MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(me) {}
	TimesDurationInt(bool, MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new TimesDurationInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new TimesDurationInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = maybe_a_product->eval_duration_val(obj)
			* maybe_a_factor->eval_int_val(obj);
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return maybe_a_product->eval_duration_val(obj)
			* maybe_a_factor->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "TimesDurationInt";
	}
};


class TimesDurationDouble: public DurationOp<MultiplicativeExp> {
public:
	TimesDurationDouble(MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(me) {}
	TimesDurationDouble(bool, MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new TimesDurationDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new TimesDurationDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = maybe_a_product->eval_duration_val(obj)
			* maybe_a_factor->eval_double_val(obj);
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return maybe_a_product->eval_duration_val(obj)
			* maybe_a_factor->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "TimesDurationDouble";
	}
};


class TimesIntDuration: public DurationOp<MultiplicativeExp> {
public:
	TimesIntDuration(MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(me) {}
	TimesIntDuration(bool, MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new TimesIntDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new TimesIntDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = maybe_a_product->eval_int_val(obj)
			* maybe_a_factor->eval_duration_val(obj);
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return maybe_a_product->eval_int_val(obj)
			* maybe_a_factor->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "TimesIntDuration";
	}
};

class TimesDoubleDuration: public DurationOp<MultiplicativeExp> {
public:
	TimesDoubleDuration(MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(me) {}
	TimesDoubleDuration(bool, MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new TimesDoubleDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new TimesDoubleDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = maybe_a_product->eval_double_val(obj)
			* maybe_a_factor->eval_duration_val(obj);
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return maybe_a_product->eval_double_val(obj)
			* maybe_a_factor->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "TimesDoubleDuration";
	}
};


class TimesIntDouble: public DoubleOp<MultiplicativeExp> {
public:
	TimesIntDouble(MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(me) {}
	TimesIntDouble(bool, MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new TimesIntDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new TimesIntDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = maybe_a_product->eval_int_val(obj)
			* maybe_a_factor->eval_double_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = maybe_a_product->eval_int_val(obj)
			* maybe_a_factor->eval_double_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return maybe_a_product->eval_int_val(obj)
			* maybe_a_factor->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "TimesIntDouble";
	}
};

class TimesDoubleInt: public DoubleOp<MultiplicativeExp> {
public:
	TimesDoubleInt(MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(me) {}
	TimesDoubleInt(bool, MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new TimesDoubleInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new TimesDoubleInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = maybe_a_product->eval_double_val(obj)
			* maybe_a_factor->eval_int_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = maybe_a_product->eval_double_val(obj)
			* maybe_a_factor->eval_int_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return maybe_a_product->eval_double_val(obj)
			* maybe_a_factor->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "TimesDoubleInt";
	}
};

class TimesDoubleDouble: public DoubleOp<MultiplicativeExp> {
public:
	TimesDoubleDouble(MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(me) {}
	TimesDoubleDouble(bool, MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new TimesDoubleDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new TimesDoubleDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = maybe_a_product->eval_double_val(obj)
			* maybe_a_factor->eval_double_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = maybe_a_product->eval_double_val(obj)
			* maybe_a_factor->eval_double_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return maybe_a_product->eval_double_val(obj)
			* maybe_a_factor->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "TimesDoubleDouble";
	}
};

class DivideIntInt: public IntOp<MultiplicativeExp> {
public:
	DivideIntInt(MultiplicativeExp& me) : IntOp<MultiplicativeExp>(me) {}
	DivideIntInt(bool, MultiplicativeExp& me) : IntOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new DivideIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DivideIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_int_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = maybe_a_product->eval_int_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L /= M;
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = maybe_a_product->eval_int_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L /= M;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		long int L = maybe_a_product->eval_int_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		long int L = maybe_a_product->eval_int_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual const char* spell() const override {
		return "DivideIntInt";
	}
};

class DivideIntDouble: public DoubleOp<MultiplicativeExp> {
public:
	DivideIntDouble(MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(me) {}
	DivideIntDouble(bool, MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new DivideIntDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DivideIntDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = maybe_a_product->eval_int_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L /= M;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = maybe_a_product->eval_int_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L /= M;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		long int L = maybe_a_product->eval_int_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		long int L = maybe_a_product->eval_int_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual const char* spell() const override {
		return "DivideIntDouble";
	}
};

class DivideDoubleInt: public DoubleOp<MultiplicativeExp> {
public:
	DivideDoubleInt(MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(me) {}
	DivideDoubleInt(bool, MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new DivideDoubleInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DivideDoubleInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = maybe_a_product->eval_double_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L /= M;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		double N = maybe_a_product->eval_double_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L = N / M;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		double L = maybe_a_product->eval_double_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		double L = maybe_a_product->eval_double_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual const char* spell() const override {
		return "DivideDoubleInt";
	}
};

class DivideDoubleDouble: public DoubleOp<MultiplicativeExp> {
public:
	DivideDoubleDouble(MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(me) {}
	DivideDoubleDouble(bool, MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new DivideDoubleDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DivideDoubleDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = maybe_a_product->eval_double_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L /= M;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		double N = maybe_a_product->eval_double_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L = N / M;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		double L = maybe_a_product->eval_double_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		double L = maybe_a_product->eval_double_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual const char* spell() const override {
		return "DivideDoubleDouble";
	}
};

class DivideDurationDuration: public DoubleOp<MultiplicativeExp> {
public:
	DivideDurationDuration(MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(me) {}
	DivideDurationDuration(bool, MultiplicativeExp& me) : DoubleOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new DivideDurationDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DivideDurationDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		CTime_base K = maybe_a_product->eval_duration_val(obj);
		CTime_base M = maybe_a_factor->eval_duration_val(obj);
		if(M == CTime_base(0,0,true)) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L = K / M;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		CTime_base K = maybe_a_product->eval_duration_val(obj);
		CTime_base M = maybe_a_factor->eval_duration_val(obj);
		if(M == CTime_base(0,0,true)) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L = K / M;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		CTime_base L = maybe_a_product->eval_duration_val(obj);
		CTime_base M = maybe_a_factor->eval_duration_val(obj);
		if(M == CTime_base(0,0,true)) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual const char* spell() const override {
		return "DivideDurationDuration";
	}
};

class DivideDurationDouble: public DurationOp<MultiplicativeExp> {
public:
	DivideDurationDouble(MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(me) {}
	DivideDurationDouble(bool, MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new DivideDurationDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DivideDurationDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		CTime_base K = maybe_a_product->eval_duration_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(M == 0.0) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L = K / M;
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		CTime_base L = maybe_a_product->eval_duration_val(obj);
		double M = maybe_a_factor->eval_double_val(obj);
		if(M == 0.0) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual const char* spell() const override {
		return "DivideDurationDouble";
	}
};

class DivideDurationInt: public DurationOp<MultiplicativeExp> {
public:
	DivideDurationInt(MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(me) {}
	DivideDurationInt(bool, MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new DivideDurationInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new DivideDurationInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		CTime_base K = maybe_a_product->eval_duration_val(obj);
		long M = maybe_a_factor->eval_int_val(obj);
		if(M == 0) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		L = K / M;
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		CTime_base L = maybe_a_product->eval_duration_val(obj);
		long M = maybe_a_factor->eval_int_val(obj);
		if(M == 0) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": divide by zero";
			throw(eval_error(err));
		}
		return L / M;
	}
	virtual const char* spell() const override {
		return "DivideDurationInt";
	}
};

class ModIntInt: public IntOp<MultiplicativeExp> {
public:
	ModIntInt(MultiplicativeExp& me) : IntOp<MultiplicativeExp>(me) {}
	ModIntInt(bool, MultiplicativeExp& me) : IntOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new ModIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ModIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_int_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = maybe_a_product->eval_int_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": modulo zero";
			throw(eval_error(err));
		}
		L = L % M;
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		long int K = maybe_a_product->eval_int_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": modulo zero";
			throw(eval_error(err));
		}
		L = K % M;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		long int L = maybe_a_product->eval_int_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": modulo zero";
			throw(eval_error(err));
		}
		return L % M;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		long int L = maybe_a_product->eval_int_val(obj);
		long int M = maybe_a_factor->eval_int_val(obj);
		if(!M) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": modulo zero";
			throw(eval_error(err));
		}
		return L % M;
	}
	virtual const char* spell() const override {
		return "ModIntInt";
	}
};

class ModDurationDuration: public DurationOp<MultiplicativeExp> {
public:
	ModDurationDuration(MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(me) {}
	ModDurationDuration(bool, MultiplicativeExp& me) : DurationOp<MultiplicativeExp>(true, me) {}
	virtual pE*	copy() override {
		return new ModDurationDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ModDurationDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		CTime_base K = maybe_a_product->eval_duration_val(obj);
		CTime_base M = maybe_a_factor->eval_duration_val(obj);
		if(M == CTime_base(0,0,true)) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": modulo zero";
			throw(eval_error(err));
		}
		L = K % M;
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		CTime_base L = maybe_a_product->eval_duration_val(obj);
		CTime_base M = maybe_a_factor->eval_duration_val(obj);
		if(M == CTime_base(0,0,true)) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": modulo zero";
			throw(eval_error(err));
		}
		return L % M;
	}
	virtual const char* spell() const override {
		return "ModDurationDuration";
	}
};

class MinusInt: public IntOp<UnaryMinus> {
public:
	MinusInt(UnaryMinus& um) : IntOp(um) {}
	MinusInt(bool, UnaryMinus& um) : IntOp(true, um) {}
	virtual pE*	copy() override {
		return new MinusInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_int_val(loc);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = -exp_modifiable_by_unary_minus->eval_int_val(obj);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = -exp_modifiable_by_unary_minus->eval_int_val(obj);
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return -exp_modifiable_by_unary_minus->eval_int_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return -exp_modifiable_by_unary_minus->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusInt";
	}
};

class MinusDouble: public DoubleOp<UnaryMinus> {
public:
	MinusDouble(UnaryMinus& um) : DoubleOp(um) {}
	MinusDouble(bool, UnaryMinus& um) : DoubleOp(true, um) {}
	virtual pE*	copy() override {
		return new MinusDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_double_val(loc);
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		L = -exp_modifiable_by_unary_minus->eval_double_val(obj);
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		L = -exp_modifiable_by_unary_minus->eval_double_val(obj);
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		return -exp_modifiable_by_unary_minus->eval_double_val(obj);
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		return -exp_modifiable_by_unary_minus->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusDouble";
	}
};

class MinusDuration: public DurationOp<UnaryMinus> {
public:
	MinusDuration(UnaryMinus& um) : DurationOp(um) {}
	MinusDuration(bool, UnaryMinus& um) : DurationOp(true, um) {}
	virtual pE*	copy() override {
		return new MinusDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new MinusDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_duration_val(loc);
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		L = -exp_modifiable_by_unary_minus->eval_duration_val(obj);
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		return -exp_modifiable_by_unary_minus->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "MinusDuration";
	}
};

class AndBool: public BoolOp<Logical> {
public:
	AndBool(Logical& l) : BoolOp<Logical>(l) {}
	AndBool(bool, Logical& l) : BoolOp<Logical>(true, l) {}
	virtual pE*	copy() override {
		return new AndBool(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AndBool(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = Lhs->eval_bool_val(obj)
			&& Rhs->eval_bool_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return Lhs->eval_bool_val(obj)
			&& Rhs->eval_bool_val(obj);
	}
	virtual const char* spell() const override {
		return "AndBool";
	}
};

class OrBool: public BoolOp<Logical> {
public:
	OrBool(Logical& l) : BoolOp<Logical>(l) {}
	OrBool(bool, Logical& l) : BoolOp<Logical>(true, l) {}
	virtual pE*	copy() override {
		return new OrBool(*this);
	}
	virtual pE*	shallow_copy() override {
		return new OrBool(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = Lhs->eval_bool_val(obj)
			|| Rhs->eval_bool_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return Lhs->eval_bool_val(obj)
			|| Rhs->eval_bool_val(obj);
	}
	virtual const char* spell() const override {
		return "OrBool";
	}
};

class EqualsBool: public BoolOp<EqualityTest> {
public:
	EqualsBool(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	EqualsBool(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new EqualsBool(*this);
	}
	virtual pE*	shallow_copy() override {
		return new EqualsBool(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_equal->eval_bool_val(obj)
			== maybe_equal_2->eval_bool_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_bool_val(obj)
			== maybe_equal_2->eval_bool_val(obj);
	}
	virtual const char* spell() const override {
		return "EqualsBool";
	}
};

class UnequalsBool: public BoolOp<EqualityTest> {
public:
	UnequalsBool(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	UnequalsBool(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new UnequalsBool(*this);
	}
	virtual pE*	shallow_copy() override {
		return new UnequalsBool(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_equal->eval_bool_val(obj)
			!= maybe_equal_2->eval_bool_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_bool_val(obj)
			!= maybe_equal_2->eval_bool_val(obj);
	}
	virtual const char* spell() const override {
		return "UnequalsBool";
	}
};

class EqualsInt: public BoolOp<EqualityTest> {
public:
	EqualsInt(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	EqualsInt(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new EqualsInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new EqualsInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_equal->eval_int_val(obj)
			== maybe_equal_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_int_val(obj)
			== maybe_equal_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "EqualsInt";
	}
};

class UnequalsInt: public BoolOp<EqualityTest> {
public:
	UnequalsInt(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	UnequalsInt(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new UnequalsInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new UnequalsInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_equal->eval_int_val(obj)
			!= maybe_equal_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_int_val(obj)
			!= maybe_equal_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "UnequalsInt";
	}
};

class EqualsString: public BoolOp<EqualityTest> {
public:
	EqualsString(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	EqualsString(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new EqualsString(*this);
	}
	virtual pE*	shallow_copy() override {
		return new EqualsString(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_equal->eval_string_val(obj)
			== maybe_equal_2->eval_string_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_string_val(obj)
			== maybe_equal_2->eval_string_val(obj);
	}
	virtual const char* spell() const override {
		return "EqualsString";
	}
};

class UnequalsString: public BoolOp<EqualityTest> {
public:
	UnequalsString(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	UnequalsString(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new UnequalsString(*this);
	}
	virtual pE*	shallow_copy() override {
		return new UnequalsString(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_equal->eval_string_val(obj)
			!= maybe_equal_2->eval_string_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_string_val(obj)
			!= maybe_equal_2->eval_string_val(obj);
	}
	virtual const char* spell() const override {
		return "UnequalsString";
	}
};

class EqualsDuration: public BoolOp<EqualityTest> {
public:
	EqualsDuration(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	EqualsDuration(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new EqualsDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new EqualsDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_equal->eval_duration_val(obj)
			== maybe_equal_2->eval_duration_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_duration_val(obj)
			== maybe_equal_2->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "EqualsDuration";
	}
};

class UnequalsDuration: public BoolOp<EqualityTest> {
public:
	UnequalsDuration(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	UnequalsDuration(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new UnequalsDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new UnequalsDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
			override 	{
		L = maybe_equal->eval_duration_val(obj)
			!= maybe_equal_2->eval_duration_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_duration_val(obj)
			!= maybe_equal_2->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "UnequalsDuration";
	}
};

class EqualsTime: public BoolOp<EqualityTest> {
public:
	EqualsTime(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	EqualsTime(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new EqualsTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new EqualsTime(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_equal->eval_time_val(obj)
			== maybe_equal_2->eval_time_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_time_val(obj)
			== maybe_equal_2->eval_time_val(obj);
	}
	virtual const char* spell() const override {
		return "EqualsTime";
	}
};

class UnequalsTime: public BoolOp<EqualityTest> {
public:
	UnequalsTime(EqualityTest& et) : BoolOp<EqualityTest>(et) {}
	UnequalsTime(bool, EqualityTest& et) : BoolOp<EqualityTest>(true, et) {}
	virtual pE*	copy() override {
		return new UnequalsTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new UnequalsTime(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_equal->eval_time_val(obj)
			!= maybe_equal_2->eval_time_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_equal->eval_time_val(obj)
			!= maybe_equal_2->eval_time_val(obj);
	}
	virtual const char* spell() const override {
		return "UnequalsTime";
	}
};

class LessEqualIntInt: public BoolOp<Comparison> {
public:
	LessEqualIntInt(Comparison& et) : BoolOp<Comparison>(et) {}
	LessEqualIntInt(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessEqualIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessEqualIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_int_val(obj)
			<= maybe_compared_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_int_val(obj)
			<= maybe_compared_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "LessEqualIntInt";
	}
};

class LessEqualTimeTime: public BoolOp<Comparison> {
public:
	LessEqualTimeTime(Comparison& et) : BoolOp<Comparison>(et) {}
	LessEqualTimeTime(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessEqualTimeTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessEqualTimeTime(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_time_val(obj)
			<= maybe_compared_2->eval_time_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_time_val(obj)
			<= maybe_compared_2->eval_time_val(obj);
	}
	virtual const char* spell() const override {
		return "LessEqualTimeTime";
	}
};

class LessEqualDurationDuration: public BoolOp<Comparison> {
public:
	LessEqualDurationDuration(Comparison& et) : BoolOp<Comparison>(et) {}
	LessEqualDurationDuration(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessEqualDurationDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessEqualDurationDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_duration_val(obj)
			<= maybe_compared_2->eval_duration_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_duration_val(obj)
			<= maybe_compared_2->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "LessEqualDurationDuration";
	}
};

class LessIntInt: public BoolOp<Comparison> {
public:
	LessIntInt(Comparison& et) : BoolOp<Comparison>(et) {}
	LessIntInt(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_int_val(obj)
			< maybe_compared_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_int_val(obj)
			< maybe_compared_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "LessIntInt";
	}
};

class LessIntDouble: public BoolOp<Comparison> {
public:
	LessIntDouble(Comparison& et) : BoolOp<Comparison>(et) {}
	LessIntDouble(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessIntDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessIntDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_int_val(obj)
			< maybe_compared_2->eval_double_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_int_val(obj)
			< maybe_compared_2->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "LessIntDouble";
	}
};

class LessEqualIntDouble: public BoolOp<Comparison> {
public:
	LessEqualIntDouble(Comparison& et) : BoolOp<Comparison>(et) {}
	LessEqualIntDouble(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessEqualIntDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessEqualIntDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_int_val(obj)
			<= maybe_compared_2->eval_double_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_int_val(obj)
			<= maybe_compared_2->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "LessEqualIntDouble";
	}
};

class LessDoubleInt: public BoolOp<Comparison> {
public:
	LessDoubleInt(Comparison& et) : BoolOp<Comparison>(et) {}
	LessDoubleInt(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessDoubleInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessDoubleInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_double_val(obj)
			< maybe_compared_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_double_val(obj)
			< maybe_compared_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "LessDoubleInt";
	}
};

class LessEqualDoubleInt: public BoolOp<Comparison> {
public:
	LessEqualDoubleInt(Comparison& et) : BoolOp<Comparison>(et) {}
	LessEqualDoubleInt(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessEqualDoubleInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessEqualDoubleInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_double_val(obj)
			<= maybe_compared_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_double_val(obj)
			<= maybe_compared_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "LessEqualDoubleInt";
	}
};

class LessDoubleDouble: public BoolOp<Comparison> {
public:
	LessDoubleDouble(Comparison& et) : BoolOp<Comparison>(et) {}
	LessDoubleDouble(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessDoubleDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessDoubleDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_double_val(obj)
			< maybe_compared_2->eval_double_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_double_val(obj)
			< maybe_compared_2->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "LessDoubleDouble";
	}
};

class LessEqualDoubleDouble: public BoolOp<Comparison> {
public:
	LessEqualDoubleDouble(Comparison& et) : BoolOp<Comparison>(et) {}
	LessEqualDoubleDouble(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessEqualDoubleDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessEqualDoubleDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_double_val(obj)
			<= maybe_compared_2->eval_double_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_double_val(obj)
			<= maybe_compared_2->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "LessEqualDoubleDouble";
	}
};

class LessTimeTime: public BoolOp<Comparison> {
public:
	LessTimeTime(Comparison& et) : BoolOp<Comparison>(et) {}
	LessTimeTime(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessTimeTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessTimeTime(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_time_val(obj)
			< maybe_compared_2->eval_time_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_time_val(obj)
			< maybe_compared_2->eval_time_val(obj);
	}
	virtual const char* spell() const override {
		return "LessTimeTime";
	}
};

class LessDurationDuration: public BoolOp<Comparison> {
public:
	LessDurationDuration(Comparison& et) : BoolOp<Comparison>(et) {}
	LessDurationDuration(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new LessDurationDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new LessDurationDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_duration_val(obj)
			< maybe_compared_2->eval_duration_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_duration_val(obj)
			< maybe_compared_2->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "LessDurationDuration";
	}
};

class GreaterEqualIntInt: public BoolOp<Comparison> {
public:
	GreaterEqualIntInt(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterEqualIntInt(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterEqualIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterEqualIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_int_val(obj)
			>= maybe_compared_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_int_val(obj)
			>= maybe_compared_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterEqualIntInt";
	}
};

class GreaterEqualTimeTime: public BoolOp<Comparison> {
public:
	GreaterEqualTimeTime(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterEqualTimeTime(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterEqualTimeTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterEqualTimeTime(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_time_val(obj)
			>= maybe_compared_2->eval_time_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_time_val(obj)
			>= maybe_compared_2->eval_time_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterEqualTimeTime";
	}
};

class GreaterEqualDurationDuration: public BoolOp<Comparison> {
public:
	GreaterEqualDurationDuration(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterEqualDurationDuration(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterEqualDurationDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterEqualDurationDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_duration_val(obj)
			>= maybe_compared_2->eval_duration_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_duration_val(obj)
			>= maybe_compared_2->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterEqualDurationDuration";
	}
};

class GreaterIntInt: public BoolOp<Comparison> {
public:
	GreaterIntInt(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterIntInt(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterIntInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterIntInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_int_val(obj)
			> maybe_compared_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_int_val(obj)
			> maybe_compared_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterIntInt";
	}
};

class GreaterIntDouble: public BoolOp<Comparison> {
public:
	GreaterIntDouble(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterIntDouble(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterIntDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterIntDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_int_val(obj)
			> maybe_compared_2->eval_double_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_int_val(obj)
			> maybe_compared_2->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterIntDouble";
	}
};

class GreaterEqualIntDouble: public BoolOp<Comparison> {
public:
	GreaterEqualIntDouble(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterEqualIntDouble(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterEqualIntDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterEqualIntDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_int_val(obj)
			>= maybe_compared_2->eval_double_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_int_val(obj)
			>= maybe_compared_2->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterEqualIntDouble";
	}
};

class GreaterDoubleInt: public BoolOp<Comparison> {
public:
	GreaterDoubleInt(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterDoubleInt(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterDoubleInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterDoubleInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_double_val(obj)
			> maybe_compared_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_double_val(obj)
			> maybe_compared_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterDoubleInt";
	}
};

class GreaterEqualDoubleInt: public BoolOp<Comparison> {
public:
	GreaterEqualDoubleInt(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterEqualDoubleInt(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterEqualDoubleInt(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterEqualDoubleInt(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_double_val(obj)
			>= maybe_compared_2->eval_int_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_double_val(obj)
			>= maybe_compared_2->eval_int_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterEqualDoubleInt";
	}
};

class GreaterDoubleDouble: public BoolOp<Comparison> {
public:
	GreaterDoubleDouble(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterDoubleDouble(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterDoubleDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterDoubleDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_double_val(obj)
			> maybe_compared_2->eval_double_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_double_val(obj)
			> maybe_compared_2->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterDoubleDouble";
	}
};

class GreaterEqualDoubleDouble: public BoolOp<Comparison> {
public:
	GreaterEqualDoubleDouble(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterEqualDoubleDouble(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterEqualDoubleDouble(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterEqualDoubleDouble(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_double_val(obj)
			>= maybe_compared_2->eval_double_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_double_val(obj)
			>= maybe_compared_2->eval_double_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterEqualDoubleDouble";
	}
};

class GreaterTimeTime: public BoolOp<Comparison> {
public:
	GreaterTimeTime(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterTimeTime(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterTimeTime(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterTimeTime(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V)
					override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_time_val(obj)
			> maybe_compared_2->eval_time_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_time_val(obj)
			> maybe_compared_2->eval_time_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterTimeTime";
	}
};

class GreaterDurationDuration: public BoolOp<Comparison> {
public:
	GreaterDurationDuration(Comparison& et) : BoolOp<Comparison>(et) {}
	GreaterDurationDuration(bool, Comparison& et) : BoolOp<Comparison>(true, et) {}
	virtual pE*	copy() override {
		return new GreaterDurationDuration(*this);
	}
	virtual pE*	shallow_copy() override {
		return new GreaterDurationDuration(true, *this);
	}
	virtual void	eval_expression(
				behaving_base*	loc,
				TypedValue&	V) override {
		V = eval_bool_val(loc);
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)
				override {
		L = maybe_compared->eval_duration_val(obj)
			> maybe_compared_2->eval_duration_val(obj);
	}
	virtual bool eval_bool_val(
			behaving_base* obj)
				override {
		return maybe_compared->eval_duration_val(obj)
			> maybe_compared_2->eval_duration_val(obj);
	}
	virtual const char* spell() const override {
		return "GreaterDurationDuration";
	}
};

class SimpleCurrentVal: public pE {
public:
	SimpleCurrentVal(
			const origin_info& o,
			Rsource* res
		) : pE(o, "currentval"), my_res(res) {}
	SimpleCurrentVal(const SimpleCurrentVal& scv);
	SimpleCurrentVal(bool, const SimpleCurrentVal& scv);
	virtual pE* copy() override { return new SimpleCurrentVal(*this); }
	virtual pE* shallow_copy() override { return new SimpleCurrentVal(true, *this); }
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override;
	virtual void to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override;
	virtual const char* spell() const override {
		return "SimpleCurrentVal";
	}
	Rsource*	my_res;

	virtual void recursively_apply(exp_analyzer& EA)  override{
		EA.pre_analyze(this);
		EA.post_analyze(this);
	}

	//
	// Because get_result_type() returns the actual resource data type, we
	// now have to cover all possible bases:
	//

	virtual void eval_string(
			behaving_base* obj,
			Cstring& L)  override{
		TypedValue tv;
		eval_expression(obj, tv);
		L = *tv.value.CST;
	}
	virtual Cstring eval_string_val(
			behaving_base* obj)  override{
		TypedValue tv;
		eval_expression(obj, tv);
		return *tv.value.CST;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L)  override{
		TypedValue tv;
		eval_expression(obj, tv);
		L = tv.value.INTEGER;
	}
	virtual long int eval_int_val(
			behaving_base* obj)  override{
		TypedValue tv;
		eval_expression(obj, tv);
		return tv.value.INTEGER;
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L)  override{
		TypedValue tv;
		eval_expression(obj, tv);
		L = tv.value.FLOATING;
	}
	virtual double eval_double_val(
			behaving_base* obj)  override{
		TypedValue tv;
		eval_expression(obj, tv);
		return tv.value.FLOATING;
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L)  override{
		TypedValue tv;
		eval_expression(obj, tv);
		L = tv.value.INTEGER;
	}
	virtual bool eval_bool_val(
			behaving_base* obj)  override{
		TypedValue tv;
		eval_expression(obj, tv);
		return tv.value.INTEGER != 0;
	}
	virtual void eval_time(
			behaving_base* obj,
			CTime_base& L)  override{
		TypedValue tv;
		eval_expression(obj, tv);
		L = *tv.value.TM;
	}
	virtual CTime_base eval_time_val(
			behaving_base* obj) override {
		TypedValue tv;
		eval_expression(obj, tv);
		return *tv.value.TM;
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		TypedValue tv;
		eval_expression(obj, tv);
		L = *tv.value.TM;
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		TypedValue tv;
		eval_expression(obj, tv);
		return *tv.value.TM;
	}
};

class InstanceMethod: public pE {
public:
	InstanceMethod() = delete;
	InstanceMethod(
			const origin_info& o,
			parsedExp& the_symbol,
			const Cstring& meth_name)
		: pE(o, the_symbol->getData()),
			theMethod(meth_name) {
		theSymbol.reference(dynamic_cast<Symbol*>(the_symbol.object()));
	}
	InstanceMethod(const InstanceMethod& rm)
		: pE(rm) {}
	InstanceMethod(bool, const InstanceMethod& rm)
		: pE(true, rm) {}
	pE*	copy() override { return new InstanceMethod(*this); }
	pE*	shallow_copy() override { return new InstanceMethod(true, *this); }
	virtual Cstring get_method() const { return theMethod; }
	Cstring theMethod;
	smart_ptr<Symbol> theSymbol;
	virtual apgen::DATA_TYPE get_result_type() const override {
		// we may need more complex logic if when we implement more methods:
		return apgen::DATA_TYPE::BOOL_TYPE;
	}
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override {
		V = ((void*)theSymbol->get_instance_ref(obj)) != NULL;
	}
};

class InstanceExists: public InstanceMethod {
public:
	InstanceExists(
			const origin_info& o,
			parsedExp& the_symbol)
		: InstanceMethod(o, the_symbol, "exists") {
	}
	InstanceExists(const InstanceExists& ie)
		: InstanceMethod(ie) {
	}
	InstanceExists(bool, const InstanceExists& ie)
		: InstanceMethod(true, ie) {
	}
	pE*	copy() override { return new InstanceExists(*this); }
	pE*	shallow_copy() override { return new InstanceExists(true, *this); }
	// virtual long int&	eval_bool_lval(
	// 			    behaving_base*	obj) override {
	// 	eval_expression(obj, val);
	// 	return val.value.INTEGER;
	// }
	virtual bool		eval_bool_val(
				    behaving_base*	obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.value.INTEGER;
	}
	virtual Cstring	get_method() const override {
		return "exists";
	}
	virtual void	to_stream(aoString* aos, int ind) const override {
		theSymbol->to_stream(aos, ind);
		(*aos) << ".exists()";
	}

	virtual const char* spell() const override {
		return "InstanceExists";
	}
};

class ResourceMethod: public pE {
public:
	ResourceMethod() = delete;
	ResourceMethod(
			const origin_info& o,
			const Cstring& resname)
		: pE(o, resname) {}
	ResourceMethod(const ResourceMethod& rm)
		: pE(rm) {}
	ResourceMethod(bool, const ResourceMethod& rm)
		: pE(true, rm) {}
	virtual pE* copy() override = 0;
	virtual pE* shallow_copy() override = 0;
	virtual Cstring get_method() const = 0;
	virtual Cstring get_indices() const = 0;
	virtual apgen::DATA_TYPE get_result_type() const override = 0;

	//
	// Because get_result_type() returns the actual resource data type, we
	// now have to cover all possible bases:
	//

	virtual void eval_string(
			behaving_base* obj,
			Cstring& L) override {
		TypedValue tv;
		eval_expression(obj, tv);
		L = *tv.value.CST;
	}
	virtual Cstring eval_string_val(
			behaving_base* obj) override {
		TypedValue tv;
		eval_expression(obj, tv);
		return *tv.value.CST;
	}
	virtual void eval_int(
			behaving_base* obj,
			long int& L) override {
		TypedValue tv;
		eval_expression(obj, tv);
		L = tv.value.INTEGER;
	}
	virtual long int eval_int_val(
			behaving_base* obj) override {
		TypedValue tv;
		eval_expression(obj, tv);
		return tv.value.INTEGER;
	}
	virtual void eval_double(
			behaving_base* obj,
			double& L) override {
		TypedValue tv;
		eval_expression(obj, tv);
		L = tv.value.FLOATING;
	}
	virtual double eval_double_val(
			behaving_base* obj) override {
		TypedValue tv;
		eval_expression(obj, tv);
		return tv.value.FLOATING;
	}
	virtual void eval_bool(
			behaving_base* obj,
			long int& L) override {
		TypedValue tv;
		eval_expression(obj, tv);
		L = tv.value.INTEGER;
	}
	virtual bool eval_bool_val(
			behaving_base* obj) override {
		TypedValue tv;
		eval_expression(obj, tv);
		return tv.value.INTEGER != 0;
	}
	virtual void eval_time(
			behaving_base* obj,
			CTime_base& L) override {
		TypedValue tv;
		eval_expression(obj, tv);
		L = *tv.value.TM;
	}
	virtual CTime_base eval_time_val(
			behaving_base* obj) override {
		TypedValue tv;
		eval_expression(obj, tv);
		return *tv.value.TM;
	}
	virtual void eval_duration(
			behaving_base* obj,
			CTime_base& L) override {
		TypedValue tv;
		eval_expression(obj, tv);
		L = *tv.value.TM;
	}
	virtual CTime_base eval_duration_val(
			behaving_base* obj) override {
		TypedValue tv;
		eval_expression(obj, tv);
		return *tv.value.TM;
	}
};

class ResourceCurrentVal: public ResourceMethod {
public:
	ResourceCurrentVal(
			const origin_info& o,
			const Cstring& resname);
	ResourceCurrentVal(const ResourceCurrentVal&);
	ResourceCurrentVal(bool, const ResourceCurrentVal&);
	pE*	copy() override { return new ResourceCurrentVal(*this); }
	pE*	shallow_copy() override { return new ResourceCurrentVal(true, *this); }
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override;
	virtual void recursively_apply(exp_analyzer& EA) override {
		EA.pre_analyze(this);
		EA.post_analyze(this);
	}
	virtual Cstring	get_method() const override {
		return "currentval";
	}
	virtual Cstring get_indices() const override { return ""; }
	virtual void	to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override;

	Rsource*	my_resource;

	virtual const char* spell() const override {
		return "ResourceCurrentVal";
	}
};

class ResourceVal: public ResourceMethod {
public:
	ResourceVal(
			const origin_info& o,
			const Cstring& resname,
			parsedExp& A);
	ResourceVal(const ResourceVal&);
	ResourceVal(bool, const ResourceVal&);
	pE*	copy() override { return new ResourceVal(*this); }
	pE*	shallow_copy() override { return new ResourceVal(true, *this); }
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override;
	virtual void recursively_apply(exp_analyzer& EA) override {
		Arg->recursively_apply(EA);
		EA.pre_analyze(this);
		EA.post_analyze(this);
	}
	virtual Cstring	get_method() const override {
		return "value";
	}
	virtual Cstring get_indices() const override { return ""; }
	virtual void	to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override;

	Rsource*	my_resource;
	parsedExp	Arg;

	virtual const char* spell() const override {
		return "ResourceVal";
	}
};

class ResourceInterpVal: public ResourceMethod {
public:
	ResourceInterpVal(
			const origin_info& o,
			const Cstring& resname);
	ResourceInterpVal(const ResourceInterpVal&);
	ResourceInterpVal(bool, const ResourceInterpVal&);
	pE*	copy() override { return new ResourceInterpVal(*this); }
	pE*	shallow_copy() override { return new ResourceInterpVal(true, *this); }
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override;
	virtual void recursively_apply(exp_analyzer& EA) override {
		EA.pre_analyze(this);
		EA.post_analyze(this);
	}
	virtual Cstring	get_method() const override {
		return "interpval";
	}
	virtual Cstring get_indices() const override { return ""; }
	virtual void	to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override;

	Rsource*	my_resource;

	virtual const char* spell() const override {
		return "ResourceInterpVal";
	}
};

class ResourceHist: public ResourceMethod {
public:
	ResourceHist(
			const origin_info& o,
			const Cstring& resname,
			parsedExp& start_arg,
			parsedExp& end_arg);
	ResourceHist(const ResourceHist&);
	ResourceHist(bool, const ResourceHist&);
	pE*	copy() override { return new ResourceHist(*this); }
	pE*	shallow_copy() override { return new ResourceHist(true, *this); }
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override;
	virtual void	eval_array(behaving_base* obj, TypedValue& V) override;
	virtual void recursively_apply(exp_analyzer& EA) override {
		EA.pre_analyze(this);
		EA.post_analyze(this);
	}
	virtual Cstring	get_method() const override {
		return "history";
	}
	virtual Cstring get_indices() const override { return ""; }
	virtual void	to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override;

	Rsource*	my_resource;
	parsedExp	Args[2];

	virtual const char* spell() const override {
		return "ResourceHistory";
	}
};

class ArrayedResourceCurrentVal: public ResourceMethod {
public:
	ArrayedResourceCurrentVal(
			const origin_info& o,
			const Cstring& resname,
			smart_ptr<MultidimIndices>& multi);
	ArrayedResourceCurrentVal(const ArrayedResourceCurrentVal&);
	ArrayedResourceCurrentVal(bool, const ArrayedResourceCurrentVal&);
	pE*	copy() override { return new ArrayedResourceCurrentVal(*this); }
	pE*	shallow_copy() override { return new ArrayedResourceCurrentVal(true, *this); }
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override;
	virtual Cstring	get_method() const override {
		return "currentval";
	}
	virtual Cstring get_indices() const override { return indices->to_string(); }
	smart_ptr<MultidimIndices>	indices;
	virtual void recursively_apply(exp_analyzer& EA) override {
		indices->recursively_apply(EA);
		EA.pre_analyze(this);
		EA.post_analyze(this);
	}
	virtual void	to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override;

	RCsource*	my_container;
	parsedExp	Arg;

	virtual const char* spell() const override {
		return "ArrayedResourceCurrentVal";
	}
};

class ArrayedResourceVal: public ResourceMethod {
public:
	ArrayedResourceVal(
			const origin_info& o,
			const Cstring& resname,
			smart_ptr<MultidimIndices>& multi,
			parsedExp& A);
	ArrayedResourceVal(const ArrayedResourceVal&);
	ArrayedResourceVal(bool, const ArrayedResourceVal&);
	pE*	copy() override { return new ArrayedResourceVal(*this); }
	pE*	shallow_copy() override { return new ArrayedResourceVal(true, *this); }
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override;
	virtual void recursively_apply(exp_analyzer& EA) override {
		indices->recursively_apply(EA);
		Arg->recursively_apply(EA);
		EA.pre_analyze(this);
		EA.post_analyze(this);
	}
	virtual Cstring	get_method() const override {
		return "value";
	}
	virtual Cstring get_indices() const override { return indices->to_string(); }
	smart_ptr<MultidimIndices>	indices;
	virtual void	to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override;

	RCsource*	my_container;
	parsedExp	Arg;

	virtual const char* spell() const override {
		return "ArrayedResourceVal";
	}
};

class ArrayedResourceInterpVal: public ResourceMethod {
public:
	ArrayedResourceInterpVal(
			const origin_info& o,
			const Cstring& resname,
			smart_ptr<MultidimIndices>& multi);
	ArrayedResourceInterpVal(const ArrayedResourceInterpVal&);
	ArrayedResourceInterpVal(bool, const ArrayedResourceInterpVal&);
	pE*	copy() override { return new ArrayedResourceInterpVal(*this); }
	pE*	shallow_copy() override { return new ArrayedResourceInterpVal(true, *this); }
	virtual void recursively_apply(exp_analyzer& EA) override {
		indices->recursively_apply(EA);
		EA.pre_analyze(this);
		EA.post_analyze(this);
	}
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override;
	virtual Cstring	get_method() const override {
		return "interpval";
	}
	virtual Cstring get_indices() const override { return indices->to_string(); }
	smart_ptr<MultidimIndices>	indices;
	virtual void	to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override;

	RCsource*	my_container;
	parsedExp	Arg;

	virtual const char* spell() const override {
		return "ArrayedResourceInterpval";
	}
};

class ArrayedResourceHist: public ResourceMethod {
public:
	ArrayedResourceHist(
			const origin_info& o,
			const Cstring& resname,
			smart_ptr<MultidimIndices>& multi,
			parsedExp&	start_exp,
			parsedExp&	end_exp);
	ArrayedResourceHist(const ArrayedResourceHist&);
	ArrayedResourceHist(bool, const ArrayedResourceHist&);
	pE*	copy() override { return new ArrayedResourceHist(*this); }
	pE*	shallow_copy() override { return new ArrayedResourceHist(true, *this); }
	virtual void recursively_apply(exp_analyzer& EA) override {
		indices->recursively_apply(EA);
		EA.pre_analyze(this);
		EA.post_analyze(this);
	}
	virtual void eval_expression(behaving_base* obj, TypedValue& V) override;
	virtual void	eval_array(behaving_base* obj, TypedValue& V) override;
	virtual Cstring	get_method() const override {
		return "history";
	}
	virtual Cstring get_indices() const override { return indices->to_string(); }
	smart_ptr<MultidimIndices>	indices;
	virtual void	to_stream(aoString* aos, int ind) const override;
	virtual apgen::DATA_TYPE get_result_type() const override;

	RCsource*	my_container;
	parsedExp	Args[2];

	virtual const char* spell() const override {
		return "ArrayedResourceHistory";
	}
};

class NonRecursiveIf: public If {
public:
	NonRecursiveIf(const If& i)
		: If(i) {
	P = dynamic_cast<Program*>(expressions[0].object());
	}
	NonRecursiveIf(bool, const If& i)
		: If(true, i) {
	    P = dynamic_cast<Program*>(expressions[0].object());
	}
	pE*	copy() override { return new NonRecursiveIf(*this); }
	pE*	shallow_copy() override { return new NonRecursiveIf(true, *this); }
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override;
	Program* P;
	virtual const char* spell() const override {
		return "NonRecursiveIf";
	}
	virtual void recursively_apply(exp_analyzer& EA) override {
		If::recursively_apply(EA);
		P->recursively_apply(EA);
	}
};

class NonRecursiveElse: public If {
public:
	NonRecursiveElse(const If& i)
		: If(i) {
	P = dynamic_cast<Program*>(expressions[0].object());
	}
	NonRecursiveElse(bool, const If& i)
		: If(true, i) {
	    P = dynamic_cast<Program*>(expressions[0].object());
	}
	pE*	copy() override { return new NonRecursiveElse(*this); }
	pE*	shallow_copy() override { return new NonRecursiveElse(true, *this); }
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override;
	Program* P;
	virtual const char* spell() const override {
		return "NonRecursiveElse";
	}
	virtual void recursively_apply(exp_analyzer& EA) override {
		If::recursively_apply(EA);
		P->recursively_apply(EA);
	}
};

class NonRecursiveElseIf: public If {
public:
	NonRecursiveElseIf(const If& i)
		: If(i) {
	P = dynamic_cast<Program*>(expressions[0].object());
	}
	NonRecursiveElseIf(bool, const If& i)
		: If(true, i) {
	    P = dynamic_cast<Program*>(expressions[0].object());
	}
	pE*	copy() override { return new NonRecursiveElseIf(*this); }
	pE*	shallow_copy() override { return new NonRecursiveElseIf(true, *this); }
	virtual void	execute(
		execution_context*	context,
		execution_context::return_code&	Code,
		execStack*		stack_to_use = NULL) override;
	Program* P;
	virtual const char* spell() const override {
		return "NonRecursiveElseIf";
	}
	virtual void recursively_apply(exp_analyzer& EA) override {
		If::recursively_apply(EA);
		P->recursively_apply(EA);
	}
};

class AbstractUsage: public Usage {
public:
	AbstractUsage(const Usage& u) : Usage(u) {}
	AbstractUsage(bool, const Usage& u) : Usage(true, u) {}
	~AbstractUsage() {}

	virtual pE*	copy() override {
		return new AbstractUsage(*this);
	}
	virtual pE*	shallow_copy() override {
		return new AbstractUsage(true, *this);
	}
	virtual void	consolidate(int dent) override;
	virtual void	consolidate_temporal_spec(int dent) override;
	
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code& Code,
				Cstack*		stack_to_use = NULL) override;
	virtual void	refresh_args_and_call_usage_method(
				usage_event&	event) override;
	virtual const char* spell() const override {
		return "AbstractUsage";
	}

	//
	// To support resource-specific derived classes:
	//
	static AbstractUsage* AbstractUsageFactory(Usage*);
};

class ImmediateUsage: public Usage {
public:
	ImmediateUsage(const Usage& u) : Usage(u) {}
	ImmediateUsage(bool, const Usage& u) : Usage(true, u) {}
	~ImmediateUsage() {}

	virtual pE*	copy() override {
		return new ImmediateUsage(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ImmediateUsage(true, *this);
	}
	virtual void	consolidate(int dent) override;
	
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code& Code,
				Cstack*		stack_to_use = NULL) override;
	virtual const char* spell() const override {
		return "ImmediateUsage";
	}
};

class SetUsage: public Usage {
public:
	SetUsage(const Usage& u) : Usage(u) {}
	SetUsage(bool, const Usage& u) : Usage(true, u) {}
	~SetUsage() {}

	virtual pE*	copy() override {
		return new SetUsage(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SetUsage(true, *this);
	}
	virtual void	consolidate(int dent) override;
	
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code& Code,
				Cstack*		stack_to_use = NULL) override;
	virtual void	refresh_args_and_call_usage_method(
				usage_event&	event) override;
	virtual const char* spell() const override {
		return "SetUsage";
	}
};

class SetSignal: public Usage {
public:
	SetSignal(const Usage& u) : Usage(u) {}
	SetSignal(bool, const Usage& u) : Usage(true, u) {}
	~SetSignal() {}

	virtual pE*	copy() override {
		return new SetSignal(*this);
	}
	virtual pE*	shallow_copy() override {
		return new SetSignal(true, *this);
	}
	virtual void	consolidate(int dent) override;
	
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code& Code,
				Cstack*		stack_to_use = NULL) override;
	virtual void	refresh_args_and_call_usage_method(
				usage_event&	event) override;
	virtual const char* spell() const override {
		return "SetSignal";
	}
};

class ImmediateSetUsage: public Usage {
public:
	ImmediateSetUsage(const Usage& u) : Usage(u) {}
	ImmediateSetUsage(bool, const Usage& u) : Usage(true, u) {}
	~ImmediateSetUsage() {}

	virtual pE*	copy() override {
		return new ImmediateSetUsage(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ImmediateSetUsage(true, *this);
	}
	virtual void	consolidate(int dent) override;
	
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code& Code,
				Cstack*		stack_to_use = NULL) override;
	virtual const char* spell() const override {
		return "ImmediateSetUsage";
	}
};

class ResetUsage: public Usage {
public:
	ResetUsage(const Usage& u) : Usage(u) {}
	ResetUsage(bool, const Usage& u) : Usage(true, u) {}
	~ResetUsage() {}

	virtual pE*	copy() override {
		return new ResetUsage(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ResetUsage(true, *this);
	}
	virtual void	consolidate(int dent) override;
	
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code& Code,
				Cstack*		stack_to_use = NULL) override;
	virtual void	refresh_args_and_call_usage_method(
				usage_event&	event) override;
	virtual const char* spell() const override {
		return "ResetUsage";
	}
};

class ImmediateResetUsage: public Usage {
public:
	ImmediateResetUsage(const Usage& u) : Usage(u) {}
	ImmediateResetUsage(bool, const Usage& u) : Usage(true, u) {}
	~ImmediateResetUsage() {}

	virtual pE*	copy() override {
		return new ImmediateResetUsage(*this);
	}
	virtual pE*	shallow_copy() override {
		return new ImmediateResetUsage(true, *this);
	}
	virtual void	consolidate(int dent) override;
	
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code& Code,
				Cstack*		stack_to_use = NULL) override;
	virtual const char* spell() const override {
		return "ImmediateResetUsage";
	}
};

extern TypedValue	(*process_an_AAF_request_handler)(const Cstring&, ListOVal&);

class currentval_finder: public exp_analyzer {
public:
	static bool	debug_currentval_finder;
	stringtlist	container_names;
	currentval_finder() {}

	virtual void pre_analyze(pE* exp) {}
	virtual void post_analyze(pE* exp) {
		ResourceMethod* rm = dynamic_cast<ResourceMethod*>(exp);

		if(rm) {

			if(debug_currentval_finder) {
				cerr << "finder: got a(n) " << exp->spell() << ", "
					<< exp->to_string() << "\n";
			}

			if(rm->get_method() == "currentval"
			   || rm->get_method() == "interpval") {
				if(!container_names.find(rm->getData())) {
					container_names << new emptySymbol(rm->getData());
				}
			}
		}
	}
};

//
// Initially, this analyzer was used to document decomposition
// hierarchies in activity types. But it quickly expanded to
// provide other information such as which resources are set
// or used inside a given abstract resource. This analyzer is
// only used in exec_agent::WriteActInteractionsToJsonStrings()
// in ACT_exec_json.C.
//
class decomp_finder: public exp_analyzer {
public:

    //
    // Defines the high-level object which this analyzer should
    // focus on:
    //
    typedef enum {
    		Activities,
		Functions,
		AbstractResources,
		Resources
    } map_type;
	
    //
    // Defines the items which we want to document
    // within a high-level object:
    //
    enum {	CHILDREN			= 0,
	        ABS_RESOURCES_SET		= 1,
	    	RESOURCES_SET			= 2,
		RESOURCES_READ			= 3,
		GLOBALS_SET			= 4,
		GLOBALS_READ			= 5,
		UDEF_FUNCTIONS_CALLED		= 6,
		AAF_FUNCTIONS_CALLED		= 7

		//
		// IMPORTANT: change the size of the array in
		// decomp_finder::post_analyze() (in file
		// pE_system_2.C) whenever you modify this
		// enum
		//
    } vec_type;
	    	

    //
    // Define lists of things we want to grab for
    // a given activity type. Ultimately, we will want
    //
    //	- activity types that this activity decomposes into
    //	- resources whose currentval is consulted by this activity
    //	- resources which are used/set/reset by this activity
    //	- globals which are set by this activity
    //
    // The map below is organized as follows:
    //
    //  properties[<obj-name>][0] = set of children this object decomposes into
    //
    //	properties[<obj-name>][1] = set of abstract resources this object calls
    //
    //	properties[<obj-name>][2] = set of resources this object sets
    //
    //	properties[<obj-name>][3] = set of resources this object reads
    //
    //	properties[<obj-name>][4] = set of globals this object sets
    //
    //	properties[<obj-name>][5] = set of globals this object reads
    //
    //	properties[<obj-name>][6] = set of udef functions this object calls
    //
    //	properties[<obj-name>][7] = set of AAF functions this object calls
    //
    map<Cstring, vector<set<Cstring> > > properties;
    Cstring			current_object;
    bool			we_are_inside_an_act_type;
    bool			we_are_inside_an_abs_res;
    bool			we_are_inside_a_function;
    bool			we_are_inside_a_resource;
    map_type			WhichObject;

    				//
    				// for use by inhibit():
				//
    // pE*				avoid;

    decomp_finder() = delete;
    decomp_finder(map_type MT)
	    : WhichObject(MT),
		we_are_inside_an_act_type(false),
    		we_are_inside_an_abs_res(false),
    		we_are_inside_a_function(false),
    		we_are_inside_a_resource(false) // ,
       		/* avoid(NULL) */ {}

    void handle(vector<set<Cstring> >&, Decomp*);
    void handle(vector<set<Cstring> >&, Usage*);
    void handle(vector<set<Cstring> >&, ResourceMethod*);
    void handle(vector<set<Cstring> >&, Assignment*);
    void handle(vector<set<Cstring> >&, Symbol*);
    void handle(vector<set<Cstring> >&, QualifiedSymbol*);
    void handle(vector<set<Cstring> >&, FunctionCall*);
    void handle_a_resource(Resource*, bool&, Cstring&);

    virtual void pre_analyze(pE* exp) override;
    virtual void post_analyze(pE* exp) override;
    // virtual bool inhibit(pE* exp) override;
};

class constraint_analyzer: public exp_analyzer {
public:
    set<Cstring>	functions_already_checked;
    bool		debug_constraint_analyzer;
    stringtlist		constraints;
    stringtlist		global_symbols;
    stringtlist		global_qualified_symbols;
    PassiveCons*	current_constraint;
    stringtlist		functions;
    slist<alpha_string, Cnode0<alpha_string, FunctionCall*> > function_stack;

    constraint_analyzer()
		: current_constraint(NULL),
		debug_constraint_analyzer(false) {}

    //
    // Do the work in pre_analyze, because we want to catch
    // the constraint itself before its components
    //
    virtual void pre_analyze(pE* exp) {
	FunctionCall*	fc = dynamic_cast<FunctionCall*>(exp);
	PassiveCons*	pc = dynamic_cast<PassiveCons*>(exp);
	Symbol*		sym = dynamic_cast<Symbol*>(exp);
	QualifiedSymbol* qual_sym = dynamic_cast<QualifiedSymbol*>(exp);
	Cstring		name;

	if(pc) {
	    current_constraint = pc;
	    if(debug_constraint_analyzer) {
		cerr << "analyzer: " << pc->constraintType
		     << " " << pc->tok_id->getData()
		     << " " << pc->tok_id_1->getData() << "\n";
	    }
	} else if(fc) {
	    name = fc->getData();
	    if(!functions.find(name)) {
		if(debug_constraint_analyzer) {
		    for(int w = 0; w < function_stack.get_length(); w++) {
			cerr << "\t";
		    }
		    cerr << "\tfunction " << name << "\n";
		}
		functions << new emptySymbol(name);
	    }
	    function_stack << new Cnode0<alpha_string, FunctionCall*>(name, fc);
	} else if(sym && (name = sym->getData()) != "error" && name != "warning") {
	    if(sym->my_level == 0 && !global_symbols.find(name)) {
		global_symbols << new emptySymbol(name);
		if(debug_constraint_analyzer) {
		    cerr << "\t";
		    for(int w = 0; w < function_stack.get_length(); w++) {
			cerr << "\t";
		    }
		    cerr << "glob. sym. " << name << "\n";
		}

		//
		// Make sure the symbol is constant or de facto constant
		//
		set<Cstring>::const_iterator	iter
			= global_behaving_object::constants().find(name);
		if(iter == global_behaving_object::constants().end()) {
		    if(name != "now") {

			//
			// forbidden_access 
			//
			Cstring err;
			err << "Constraint "
			    << current_constraint->tok_id_1->getData()
			    << " relies on the value of global \""
			    << name << "\" which is not a constant.\n";
			throw(eval_error(err));
		    }
		}
	    }
	} else if(qual_sym) {
	    Symbol*	qual_sym_sym = dynamic_cast<Symbol*>(qual_sym->symbol.object());

	    name = qual_sym->symbol->getData();

	    if(	function_stack.last_node()
		&& function_stack.last_node()->get_key() == name) {

		//
		// Nothing to do; the qualification is just 'currentval'.
		// NOTE: this would be a good spot to identify faulty
		// uses of 'value' instead of 'currentval'
		//
		;
	    } else if(   qual_sym_sym->my_level == 0
		  && !global_qualified_symbols.find(name)) {
		global_qualified_symbols << new emptySymbol(name);
		if(debug_constraint_analyzer) {
		    cerr << "\t";
		    for(int w = 0; w < function_stack.get_length(); w++) {
			cerr << "\t";
		    }
		    cerr << "glob. qual. sym. " << name << "\n";
		}
		set<Cstring>::const_iterator iter2
			= global_behaving_object::constants().find(name);

		if(iter2 == global_behaving_object::constants().end()) {

			//
			// forbidden_access 
			//
			Cstring err;
			err << "Constraint "
			    << current_constraint->tok_id_1->getData()
			    << " relies on the value of global \""
			    << name << "\" which is not a constant.\n";
			throw(eval_error(err));
		}
	    }
	}
    }

    virtual void post_analyze(pE* exp) {
	FunctionCall*	fc = dynamic_cast<FunctionCall*>(exp);
	if(fc) {
	    delete function_stack.last_node();
	}
    }
};

} // namespace pEsys

#endif /* _AP_PE_SYS_H_ */
