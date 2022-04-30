#ifndef _GEN_PARSED_EXP_H_
#define _GEN_PARSED_EXP_H_

class Differentiator_0 {
public:
	Differentiator_0(int d) : id(d) {}
	int id;
};

class Differentiator_1 {
public:
	Differentiator_1(int d) : id(d) {}
	int id;
};

class Differentiator_2 {
public:
	Differentiator_2(int d) : id(d) {}
	int id;
};

class Differentiator_3 {
public:
	Differentiator_3(int d) : id(d) {}
	int id;
};

class Differentiator_4 {
public:
	Differentiator_4(int d) : id(d) {}
	int id;
};

class Differentiator_5 {
public:
	Differentiator_5(int d) : id(d) {}
	int id;
};

class Differentiator_6 {
public:
	Differentiator_6(int d) : id(d) {}
	int id;
};

class Differentiator_7 {
public:
	Differentiator_7(int d) : id(d) {}
	int id;
};

class Differentiator_8 {
public:
	Differentiator_8(int d) : id(d) {}
	int id;
};

class Differentiator_9 {
public:
	Differentiator_9(int d) : id(d) {}
	int id;
};

class Differentiator_10 {
public:
	Differentiator_10(int d) : id(d) {}
	int id;
};

class Differentiator_11 {
public:
	Differentiator_11(int d) : id(d) {}
	int id;
};

class Differentiator_12 {
public:
	Differentiator_12(int d) : id(d) {}
	int id;
};

class Differentiator_13 {
public:
	Differentiator_13(int d) : id(d) {}
	int id;
};

class Differentiator_14 {
public:
	Differentiator_14(int d) : id(d) {}
	int id;
};

class Differentiator_15 {
public:
	Differentiator_15(int d) : id(d) {}
	int id;
};

class Differentiator_16 {
public:
	Differentiator_16(int d) : id(d) {}
	int id;
};

class Differentiator_17 {
public:
	Differentiator_17(int d) : id(d) {}
	int id;
};

class Differentiator_18 {
public:
	Differentiator_18(int d) : id(d) {}
	int id;
};

class Differentiator_19 {
public:
	Differentiator_19(int d) : id(d) {}
	int id;
};

class Differentiator_20 {
public:
	Differentiator_20(int d) : id(d) {}
	int id;
};

class Differentiator_21 {
public:
	Differentiator_21(int d) : id(d) {}
	int id;
};

class Differentiator_22 {
public:
	Differentiator_22(int d) : id(d) {}
	int id;
};

class Differentiator_23 {
public:
	Differentiator_23(int d) : id(d) {}
	int id;
};

class Differentiator_24 {
public:
	Differentiator_24(int d) : id(d) {}
	int id;
};

class Differentiator_25 {
public:
	Differentiator_25(int d) : id(d) {}
	int id;
};

class Differentiator_26 {
public:
	Differentiator_26(int d) : id(d) {}
	int id;
};

class ActInstance;
class ActType;
class ActTypeAttributes;
class ActTypeBody;
class ActTypeClassVariables;
class ActTypeDecl;
class ActTypeDecomp;
class ActTypeInitial;
class AdditiveExp;
class Array;
class ArrayList;
class ArrayPrecomp;
class Assignment;
class AssignmentPrecomp;
class ByValueSymbol;
class ClassMember;
class Comparison;
class ConcreteResourceTypeEtc;
class Constant;
class Continue;
class CustomAttributes;
class DataType;
class DataTypeDefaultValue;
class Declaration;
class Declarations;
class Decomp;
class DefaultExpression;
class Directive;
class EqualityTest;
class ExpressionList;
class ExpressionListPrecomp;
class File;
class FunctionCall;
class FunctionDeclaration;
class FunctionDefinition;
class FunctionIdentity;
class FunctionReturnType;
class GetInterpwins;
class GetWindows;
class Global;
class Hierarchy;
class IdList;
class If;
class InputFile;
class LabeledDefault;
class LabeledDefaultList;
class LabeledProfiles;
class LabeledProgram;
class LabeledStates;
class Logical;
class ModelingSection;
class MultidimIndices;
class MultiplicativeExp;
class Null;
class OneNumber;
class Parameters;
class Parentheses;
class PassiveCons;
class PassiveConsItem;
class PassiveConsMessage;
class PassiveConsSeverity;
class ProfileList;
class Program;
class Qualifications;
class QualifiedSymbol;
class RaiseToPower;
class RangeExpression;
class ResUsageWithArgs;
class Resource;
class ResourceDef;
class ResourceInfo;
class ResourceType;
class Return;
class ReverseRangeExpression;
class Signature;
class SingleIndex;
class SinglePrecompResName;
class Symbol;
class TemporalSpec;
class TimeSeriesStart;
class TypeCreationSection;
class Typedef;
class UnaryMinus;
class Usage;
class UsageSection;
class VarDescription;
class VarRange;
class VarUnits;
class WaitFor;
class WaitUntil;
class WaitUntilRegexp;
class While;



class ActInstance: public pE {
	public:
	ActInstance(
		int		main_index,
		parsedExp&	P_act_instance_header,
		parsedExp&	P_tok_begin,
		parsedExp&	P_decomposition_info,
		parsedExp&	P_attributes_section_inst,
		parsedExp&	P_param_section_inst,
		parsedExp&	P_tok_end,
		parsedExp&	P_tok_activity,
		parsedExp&	P_tok_instance,
		parsedExp&	P_activity_id,
		Differentiator_0);
	~ActInstance() {}
	ActInstance(const ActInstance& p);
	ActInstance(bool, const ActInstance& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ActInstance(*this);
	}
	virtual pE* shallow_copy() override {
		return new ActInstance(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void	consolidate(int dent);
	parsedExp	act_instance_header;
	parsedExp	activity_id;
	parsedExp	attributes_section_inst;
	parsedExp	decomposition_info;
	parsedExp	param_section_inst;
	parsedExp	tok_activity;
	parsedExp	tok_begin;
	parsedExp	tok_end;
	parsedExp	tok_instance;
	virtual const char* spell() const override {
		return "ActInstance";
	}
};

class ActType: public pE {
	public:
	ActType(
		int		main_index,
		parsedExp&	P_initial_act_type_section,
		parsedExp&	P_body_of_activity_type_def,
		Differentiator_0);
	~ActType() {}
	ActType(const ActType& p);
	ActType(bool, const ActType& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ActType(*this);
	}
	virtual pE* shallow_copy() override {
		return new ActType(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;

	//
	// We need to preserve original copies of all the programs
	// stored within an activity type, so we can consolidate new
	// copies in the context of activity instances when necessary.
	//
	// The programs in question are:
	//
	//	program		ActType member/class				member/class where stored		
	//	-------		--------------------				-------------------------
	//	class vars	initial_act_type_section/ActTypeInitial		opt_type_class_variables/ActTypeClassVariables
	//	attributes	initial_act_type_section/ActTypeInitial		opt_act_attributes_section/ActTypeAttributes
	//	parameters	initial_act_type_section/ActTypeInitial		opt_act_type_param_section/Parameters
	//	creation	body_of_activity_type_section/ActTypeBody	opt_creation_section/TypeCreationSection
	//	modeling	body_of_activity_type_section/ActTypeBody	opt_res_usage_section/ModelingSection
	//			NOTE: modeling section is also found in
	//			opt_decomposition_section/ActTypeDecomp
	//	decomposition	body_of_activity_type_def/ActTypeBody		opt_decomposition_section/ActTypeDecomp
	//	destruction	body_of_activity_type_def/ActTypeBody		opt_destruction_section/TypeCreationSection
	//									NOTE: not a typo
	//
	// In the "original..." members below, it is understood that each
	// parsedExp holds a Program, Declarations.
	// If the program does not exist, the parsedExp is empty.
	//

	parsedExp	original_class_vars;
	parsedExp	original_attributes;
	parsedExp	original_parameters;
	parsedExp	original_creation;
	parsedExp	original_modeling;
	parsedExp	original_decomposition;
	parsedExp	original_destruction;

	stringtlist	defined_attributes;

	void		consolidate(int dent);
	void		handle_modeling_program(ModelingSection*, Behavior&, int dent);
	void		add_a_wait_statement(parsedExp header, Program* theProgram);
	parsedExp	body_of_activity_type_def;
	parsedExp	initial_act_type_section;
	virtual const char* spell() const override {
		return "ActType";
	}
};

class ActTypeAttributes: public pE {
	public:
	ActTypeAttributes(
		int		main_index,
		parsedExp&	P_declarative_program,
		parsedExp&	P_tok_attributes,
		Differentiator_0);
	~ActTypeAttributes() {}
	ActTypeAttributes(const ActTypeAttributes& p);
	ActTypeAttributes(bool, const ActTypeAttributes& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ActTypeAttributes(*this);
	}
	virtual pE* shallow_copy() override {
		return new ActTypeAttributes(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	declarative_program;
	parsedExp	tok_attributes;
	virtual const char* spell() const override {
		return "ActTypeAttributes";
	}
};

class ActTypeBody: public pE {
	public:
	ActTypeBody(
		int		main_index,
		parsedExp&	P_opt_creation_section,
		parsedExp&	P_opt_res_usage_section,
		parsedExp&	P_opt_decomposition_section,
		parsedExp&	P_opt_destruction_section,
		parsedExp&	P_end_of_activity_type_def,
		Differentiator_0);
	~ActTypeBody() {}
	ActTypeBody(const ActTypeBody& p);
	ActTypeBody(bool, const ActTypeBody& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ActTypeBody(*this);
	}
	virtual pE* shallow_copy() override {
		return new ActTypeBody(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	end_of_activity_type_def;
	parsedExp	opt_creation_section;
	parsedExp	opt_decomposition_section;
	parsedExp	opt_destruction_section;
	parsedExp	opt_res_usage_section;
	virtual const char* spell() const override {
		return "ActTypeBody";
	}
};

class ActTypeClassVariables: public pE {
	public:
	ActTypeClassVariables(
		int		main_index,
		parsedExp&	P_type_class_variables,
		Differentiator_0)
;
	~ActTypeClassVariables() {}
	ActTypeClassVariables(const ActTypeClassVariables& p);
	ActTypeClassVariables(bool, const ActTypeClassVariables& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ActTypeClassVariables(*this);
	}
	virtual pE* shallow_copy() override {
		return new ActTypeClassVariables(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void	consolidate(int dent);
	parsedExp	type_class_variables;
	virtual const char* spell() const override {
		return "ActTypeClassVariables";
	}
};

class ActTypeDecl: public pE {
	public:
	ActTypeDecl(
		int		main_index,
		parsedExp&	P_activity_name,
		parsedExp&	P_activity_type_preface,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~ActTypeDecl() {}
	ActTypeDecl(const ActTypeDecl& p);
	ActTypeDecl(bool, const ActTypeDecl& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ActTypeDecl(*this);
	}
	virtual pE* shallow_copy() override {
		return new ActTypeDecl(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void	to_stream(aoString* aos, int ind) const override {
		indent(aos, ind);
		(*aos) << "activity type " << theData << ";\n";
	}
	parsedExp	ASCII_59;
	parsedExp	activity_name;
	parsedExp	activity_type_preface;
	virtual const char* spell() const override {
		return "ActTypeDecl";
	}
};

class ActTypeDecomp: public pE {
	public:
	ActTypeDecomp(
		int		main_index,
		parsedExp&	P_decomposition_header,
		parsedExp&	P_program,
		parsedExp&	P_opt_res_usage_section,
		Differentiator_0);
	~ActTypeDecomp() {}
	ActTypeDecomp(const ActTypeDecomp& p);
	ActTypeDecomp(bool, const ActTypeDecomp& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ActTypeDecomp(*this);
	}
	virtual pE* shallow_copy() override {
		return new ActTypeDecomp(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	decomposition_header;
	parsedExp	opt_res_usage_section;
	parsedExp	program;
	virtual const char* spell() const override {
		return "ActTypeDecomp";
	}
};

class ActTypeInitial: public pE {
	public:
	ActTypeInitial(
		int		main_index,
		parsedExp&	P_activity_type_header,
		parsedExp&	P_opt_type_class_variables,
		parsedExp&	P_opt_act_attributes_section,
		parsedExp&	P_opt_act_type_param_section,
		Differentiator_0);
	~ActTypeInitial() {}
	ActTypeInitial(const ActTypeInitial& p);
	ActTypeInitial(bool, const ActTypeInitial& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ActTypeInitial(*this);
	}
	virtual pE* shallow_copy() override {
		return new ActTypeInitial(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	activity_type_header;
	parsedExp	opt_act_attributes_section;
	parsedExp	opt_act_type_param_section;
	parsedExp	opt_type_class_variables;
	virtual const char* spell() const override {
		return "ActTypeInitial";
	}
};

class AdditiveExp: public pE {
	public:
	AdditiveExp(
		int		main_index,
		parsedExp&	P_tok_plus,
		parsedExp&	P_maybe_a_sum,
		parsedExp&	P_maybe_added_or_subtracted,
		Differentiator_0);
	AdditiveExp(
		int		main_index,
		parsedExp&	P_tok_minus,
		parsedExp&	P_maybe_a_sum,
		parsedExp&	P_maybe_added_or_subtracted,
		Differentiator_1);
	~AdditiveExp() {}
	AdditiveExp(const AdditiveExp& p);
	AdditiveExp(bool, const AdditiveExp& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new AdditiveExp(*this);
	}
	virtual pE* shallow_copy() override {
		return new AdditiveExp(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void	to_stream(aoString* aos, int ind) const override;
	void	(*binaryFunc)(
			const TypedValue&,
			const TypedValue&,
			TypedValue&);
	static void	func_MINUS(const TypedValue&, const TypedValue&, TypedValue&);
	static void	func_PLUS(const TypedValue&, const TypedValue&, TypedValue&);
	apgen::DATA_TYPE get_result_type() const override;
	void		eval_string(behaving_base* obj, Cstring& L) override {
		Cstring s = Lhs->eval_string_val(obj);
		TypedValue val;
		Rhs->eval_expression(obj, val);
		if(val.is_string()) {
			s << val.get_string();
		} else {
			s << val.to_string();
		}
		L = s;
	}
	Cstring eval_string_val(
			behaving_base* obj) override {
		Cstring s(Lhs->eval_string_val(obj));
		TypedValue val;
		Rhs->eval_expression(obj, val);
		if(val.is_string()) {
			s << val.get_string();
		} else {
			s << val.to_string();
		}
		return s;
	}

	parsedExp Operator;
	parsedExp Lhs;
	parsedExp Rhs;
	parsedExp	maybe_a_sum;
	parsedExp	maybe_a_sum_0;
	parsedExp	maybe_added_or_subtracted;
	parsedExp	maybe_added_or_subtracted_2;
	parsedExp	tok_minus;
	parsedExp	tok_plus;
	virtual const char* spell() const override {
		return "AdditiveExp";
	}
};

class Array: public pE {
	public:
	Array(
		int		main_index,
		parsedExp&	P_rest_of_list,
		parsedExp&	P_ASCII_91,	// [
		parsedExp&	P_ASCII_93,	// ]
		Differentiator_0);
	~Array() {}
	Array(const Array& p);
	Array(bool, const Array& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Array(*this);
	}
	virtual pE* shallow_copy() override {
		return new Array(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void		to_stream(aoString* aos, int ind) const override;
	// map<Cstring, parsedExp>	dictionary;
	vector<parsedExp> actual_values;
	vector<parsedExp> actual_keys;
	bool		is_list;
	bool		is_struct;
	apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::ARRAY;
	}
	void	consolidate(int dent);
	virtual void eval_array(
			behaving_base* obj,
			TypedValue& L) override {
		eval_expression(obj, L);
	}
	virtual TypedValue eval_array_val(
			behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val;
	}
	parsedExp	ASCII_91;
	parsedExp	ASCII_93;
	parsedExp	rest_of_list;
	virtual const char* spell() const override {
		return "Array";
	}
};

class ArrayList: public pE {
	public:
	ArrayList(
		int		main_index,
		parsedExp&	P_one_array,
		Differentiator_0)
;
	~ArrayList() {}
	ArrayList(const ArrayList& p);
	ArrayList(bool, const ArrayList& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ArrayList(*this);
	}
	virtual pE* shallow_copy() override {
		return new ArrayList(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void			to_stream(aoString* aos, int ind) const override;
	virtual void		addExp(const parsedExp& pe) override;
	void			consolidate(int dent,
					const Cstring& the_resource);
	void			consolidate_one_element(
					int dent,
					parsedExp& pe,
					const Cstring& the_resource);
	vector<TypedValue>	evaluated_elements;
	TypedValue		val;
	parsedExp	one_array;
	virtual const char* spell() const override {
		return "ArrayList";
	}
};

class ArrayPrecomp: public pE {
	public:
	ArrayPrecomp(
		int		main_index,
		parsedExp&	P_keyword_value_pairs_precomp,
		Differentiator_0)
;
	~ArrayPrecomp() {}
	ArrayPrecomp(const ArrayPrecomp& p);
	ArrayPrecomp(bool, const ArrayPrecomp& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ArrayPrecomp(*this);
	}
	virtual pE* shallow_copy() override {
		return new ArrayPrecomp(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	keyword_value_pairs_precomp;
	virtual const char* spell() const override {
		return "ArrayPrecomp";
	}
};

class Assignment: public executableExp {
	public:
	Assignment(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_equal_sign,
		parsedExp&	P_Expression_2,
		parsedExp&	P_opt_range,
		parsedExp&	P_opt_descr,
		parsedExp&	P_opt_units,
		Differentiator_0);
	Assignment(
		int		main_index,
		parsedExp&	P_lhs_of_assign,
		parsedExp&	P_assignment_op,
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_1);
	~Assignment() {}
	Assignment(const Assignment& p);
	Assignment(bool, const Assignment& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Assignment(*this);
	}
	virtual pE* shallow_copy() override {
		return new Assignment(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void	to_stream(aoString* aos, int ind) const override;
	bool	can_interrupt() const override {
		// it would make more sense to query the r.h.s.
		// return true;
		return Expression && Expression->can_interrupt();
	}
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	Cstring original_lhs;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	Expression_2;
	parsedExp	assignment_op;
	parsedExp	lhs_of_assign;
	parsedExp	opt_descr;
	parsedExp	opt_range;
	parsedExp	opt_units;
	parsedExp	tok_equal_sign;
	virtual const char* spell() const override {
		return "Assignment";
	}
};

class AssignmentPrecomp: public pE {
	public:
	AssignmentPrecomp(
		int		main_index,
		parsedExp&	P_tok_stringval,
		parsedExp&	P_tok_equal_sign,
		parsedExp&	P_ASCII_91,	// [
		parsedExp&	P_one_number,
		parsedExp&	P_ASCII_44,	// ,
		parsedExp&	P_one_number_5,
		parsedExp&	P_ASCII_93,	// ]
		Differentiator_0);
	~AssignmentPrecomp() {}
	AssignmentPrecomp(const AssignmentPrecomp& p);
	AssignmentPrecomp(bool, const AssignmentPrecomp& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new AssignmentPrecomp(*this);
	}
	virtual pE* shallow_copy() override {
		return new AssignmentPrecomp(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
 // not clear yet whether we need this:
 void consolidate(vector<parsedExp>& args, int dent);

 // in pE_system.C. The rationale is that this function does
 // nothing; the work is done while parsing, there is no need
 // to keep this parse tree around.
 virtual void	addExp(
				const parsedExp& pe) override;
	parsedExp	ASCII_44;
	parsedExp	ASCII_91;
	parsedExp	ASCII_93;
	parsedExp	one_number;
	parsedExp	one_number_5;
	parsedExp	tok_equal_sign;
	parsedExp	tok_stringval;
	virtual const char* spell() const override {
		return "AssignmentPrecomp";
	}
};

class ByValueSymbol: public pE {
	public:
	ByValueSymbol(
		int		main_index,
		parsedExp&	P_tok_mult,
		parsedExp&	P_tok_id,
		Differentiator_0);
	~ByValueSymbol() {}
	ByValueSymbol(const ByValueSymbol& p);
	ByValueSymbol(bool, const ByValueSymbol& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ByValueSymbol(*this);
	}
	virtual pE* shallow_copy() override {
		return new ByValueSymbol(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	task*		my_task;
	int		my_level;
	int		my_index;
	int		my_use_task_index;
	int		my_index_in_use_task;

	//
	// NOTE: this class is not thread-safe
	//

	virtual TypedValue& get_val_ref(
				behaving_base* loc) override;
	void	consolidate(int dent);
	void	to_stream(aoString* aos, int ind) const override {
		indent(aos, ind);
		(*aos) << tok_mult->getData() << tok_id->getData();
	}
	parsedExp	tok_id;
	parsedExp	tok_mult;
	virtual const char* spell() const override {
		return "ByValueSymbol";
	}
};

class ClassMember: public pE {
	public:
	ClassMember(
		int		main_index,
		parsedExp&	P_symbol,
		parsedExp&	P_ASCII_46,	// .
		Differentiator_0);
	~ClassMember() {}
	ClassMember(const ClassMember& p);
	ClassMember(bool, const ClassMember& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ClassMember(*this);
	}
	virtual pE* shallow_copy() override {
		return new ClassMember(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	TypedValue*	eval_expression_special(
					behaving_base* lhs_obj,
					const Cstring&);

		
	// we store the task to check that it agrees with the object's
	TypedValue*		my_value;
	const task*		my_task;
	apgen::DATA_TYPE	get_result_type() const override;

	parsedExp		OptionalIndex;

	void	to_stream(aoString* aos, int ind) const override {
		(*aos) << ".";
		symbol->to_stream(aos, 0);
	}
	virtual void	addExp(const parsedExp& pe) override;
	parsedExp	ASCII_46;
	parsedExp	symbol;
	virtual const char* spell() const override {
		return "ClassMember";
	}
};

class Comparison: public pE {
	public:
	Comparison(
		int		main_index,
		parsedExp&	P_tok_lessthan,
		parsedExp&	P_maybe_compared,
		parsedExp&	P_maybe_compared_2,
		Differentiator_0);
	Comparison(
		int		main_index,
		parsedExp&	P_tok_lessthanoreq,
		parsedExp&	P_maybe_compared,
		parsedExp&	P_maybe_compared_2,
		Differentiator_1);
	Comparison(
		int		main_index,
		parsedExp&	P_tok_gtrthanoreq,
		parsedExp&	P_maybe_compared,
		parsedExp&	P_maybe_compared_2,
		Differentiator_2);
	Comparison(
		int		main_index,
		parsedExp&	P_tok_gtrthan,
		parsedExp&	P_maybe_compared,
		parsedExp&	P_maybe_compared_2,
		Differentiator_3);
	~Comparison() {}
	Comparison(const Comparison& p);
	Comparison(bool, const Comparison& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Comparison(*this);
	}
	virtual pE* shallow_copy() override {
		return new Comparison(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void	to_stream(aoString* aos, int ind) const override;
	void	(*binaryFunc)(
			const TypedValue&,
			const TypedValue&,
			TypedValue&);
	static void	func_GTRTHAN(const TypedValue&, const TypedValue&, TypedValue&);
	static void	func_GTRTHANOREQ(const TypedValue&, const TypedValue&, TypedValue&);
	static void	func_LESSTHAN(const TypedValue&, const TypedValue&, TypedValue&);
	static void	func_LESSTHANOREQ(const TypedValue&, const TypedValue&, TypedValue&);
	bool		eval_bool_val(behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.get_int();
	}
	apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::BOOL_TYPE;
	}
	parsedExp Operator;
	parsedExp	maybe_compared;
	parsedExp	maybe_compared_0;
	parsedExp	maybe_compared_2;
	parsedExp	tok_gtrthan;
	parsedExp	tok_gtrthanoreq;
	parsedExp	tok_lessthan;
	parsedExp	tok_lessthanoreq;
	virtual const char* spell() const override {
		return "Comparison";
	}
};

class ConcreteResourceTypeEtc: public pE {
	public:
	ConcreteResourceTypeEtc(
		int		main_index,
		parsedExp&	P_concrete_resource_type_etc,
		parsedExp&	P_resource_data_type,
		Differentiator_0);
	~ConcreteResourceTypeEtc() {}
	ConcreteResourceTypeEtc(const ConcreteResourceTypeEtc& p);
	ConcreteResourceTypeEtc(bool, const ConcreteResourceTypeEtc& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ConcreteResourceTypeEtc(*this);
	}
	virtual pE* shallow_copy() override {
		return new ConcreteResourceTypeEtc(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	concrete_resource_type_etc;
	parsedExp	resource_data_type;
	virtual const char* spell() const override {
		return "ConcreteResourceTypeEtc";
	}
};

class Constant: public pE {
	public:
	Constant(
		int		main_index,
		parsedExp&	P_tok_number,
		Differentiator_0)
;
	Constant(
		int		main_index,
		parsedExp&	P_tok_false,
		Differentiator_1)
;
	Constant(
		int		main_index,
		parsedExp&	P_tok_true,
		Differentiator_2)
;
	Constant(
		int		main_index,
		parsedExp&	P_tok_stringval,
		Differentiator_3)
;
	Constant(
		int		main_index,
		parsedExp&	P_tok_duration,
		Differentiator_4)
;
	Constant(
		int		main_index,
		parsedExp&	P_tok_pi,
		Differentiator_5)
;
	Constant(
		int		main_index,
		parsedExp&	P_tok_rad,
		Differentiator_6)
;
	Constant(
		int		main_index,
		parsedExp&	P_tok_stringval,
		Differentiator_7)
;
	Constant(
		int		main_index,
		parsedExp&	P_tok_time,
		Differentiator_8)
;
	Constant(
		int		main_index,
		parsedExp&	P_tok_stringval,
		parsedExp&	P_ASCII_58,	// :
		parsedExp&	P_tok_duration,
		Differentiator_9);
	~Constant() {}
	Constant(const Constant& p);
	Constant(bool, const Constant& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Constant(*this);
	}
	virtual pE* shallow_copy() override {
		return new Constant(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	bool		is_timesystem_based;
	CTime_base	timesystem_duration;
	Cstring		timesystem_name;
	TypedValue	val;
	apgen::DATA_TYPE get_result_type() const override {
		return val.get_type();
	}
	parsedExp	ASCII_58;
	parsedExp	tok_duration;
	parsedExp	tok_duration_2;
	parsedExp	tok_false;
	parsedExp	tok_number;
	parsedExp	tok_pi;
	parsedExp	tok_rad;
	parsedExp	tok_stringval;
	parsedExp	tok_time;
	parsedExp	tok_true;
	virtual const char* spell() const override {
		return "Constant";
	}
};

class Continue: public executableExp {
	public:
	Continue(
		int		main_index,
		parsedExp&	P_tok_continue,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~Continue() {}
	Continue(const Continue& p);
	Continue(bool, const Continue& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Continue(*this);
	}
	virtual pE* shallow_copy() override {
		return new Continue(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	parsedExp	ASCII_59;
	parsedExp	tok_continue;
	virtual const char* spell() const override {
		return "Continue";
	}
};

class CustomAttributes: public pE {
	public:
	CustomAttributes(
		int		main_index,
		parsedExp&	P_tok_attributes,
		parsedExp&	P_custom_decls,
		parsedExp&	P_tok_end,
		parsedExp&	P_tok_attributes_3,
		Differentiator_0);
	~CustomAttributes() {}
	CustomAttributes(const CustomAttributes& p);
	CustomAttributes(bool, const CustomAttributes& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new CustomAttributes(*this);
	}
	virtual pE* shallow_copy() override {
		return new CustomAttributes(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void consolidate(int dent);
	parsedExp	custom_decls;
	parsedExp	tok_attributes;
	parsedExp	tok_attributes_3;
	parsedExp	tok_end;
	virtual const char* spell() const override {
		return "CustomAttributes";
	}
};

class DataType: public pE {
	public:
	DataType(
		int		main_index,
		parsedExp&	P_builtin_type,
		parsedExp&	P_tok_local,
		Differentiator_0);
	DataType(
		int		main_index,
		parsedExp&	P_builtin_type,
		Differentiator_1)
;
	DataType(
		int		main_index,
		parsedExp&	P_tok_dyn_type,
		Differentiator_2)
;
	DataType(
		int		main_index,
		parsedExp&	P_builtin_type,
		Differentiator_3)
;
	DataType(
		int		main_index,
		parsedExp&	P_builtin_type,
		Differentiator_4)
;
	~DataType() {}
	DataType(const DataType& p);
	DataType(bool, const DataType& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new DataType(*this);
	}
	virtual pE* shallow_copy() override {
		return new DataType(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void		consolidate(int dent);
	TypedValue	val;
	void		to_stream(aoString* aos, int ind) const override {
		if(builtin_type) {
			builtin_type->to_stream(aos, ind);
		} else if(tok_dyn_type) {
			tok_dyn_type->to_stream(aos, ind);
		}
	}
	parsedExp	builtin_type;
	parsedExp	tok_dyn_type;
	parsedExp	tok_local;
	virtual const char* spell() const override {
		return "DataType";
	}
};

class DataTypeDefaultValue: public pE {
	public:
	DataTypeDefaultValue(
		int		main_index,
		parsedExp&	P_builtin_type,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_ASCII_41,	// )
		Differentiator_0);
	DataTypeDefaultValue(
		int		main_index,
		parsedExp&	P_tok_dyn_type,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_ASCII_41,	// )
		Differentiator_1);
	~DataTypeDefaultValue() {}
	DataTypeDefaultValue(const DataTypeDefaultValue& p);
	DataTypeDefaultValue(bool, const DataTypeDefaultValue& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new DataTypeDefaultValue(*this);
	}
	virtual pE* shallow_copy() override {
		return new DataTypeDefaultValue(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	TypedValue	val;
	parsedExp	ASCII_40;
	parsedExp	ASCII_40_0;
	parsedExp	ASCII_41;
	parsedExp	ASCII_41_2;
	parsedExp	builtin_type;
	parsedExp	tok_dyn_type;
	virtual const char* spell() const override {
		return "DataTypeDefaultValue";
	}
};

class Declaration: public executableExp {
	public:
	Declaration(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_ASCII_58,	// :
		parsedExp&	P_param_scope_and_type,
		parsedExp&	P_tok_default,
		parsedExp&	P_tok_to,
		parsedExp&	P_Expression,
		parsedExp&	P_opt_range,
		parsedExp&	P_opt_descr,
		parsedExp&	P_opt_units,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~Declaration() {}
	Declaration(const Declaration& p);
	Declaration(bool, const Declaration& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Declaration(*this);
	}
	virtual pE* shallow_copy() override {
		return new Declaration(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void consolidate(int dent);

	//
	// Holds type and declared_type:
	//
	TypedValue	val;
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	parsedExp	ASCII_58;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	opt_descr;
	parsedExp	opt_range;
	parsedExp	opt_units;
	parsedExp	param_scope_and_type;
	parsedExp	tok_default;
	parsedExp	tok_id;
	parsedExp	tok_to;
	virtual const char* spell() const override {
		return "Declaration";
	}
};

class Declarations: public pE {
	public:
	Declarations(
		int		main_index,
		parsedExp&	P_declaration,
		Differentiator_0)
;
	~Declarations() {}
	Declarations(const Declarations& p);
	Declarations(bool, const Declarations& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Declarations(*this);
	}
	virtual pE* shallow_copy() override {
		return new Declarations(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override {
 	if(declaration) {
 		declaration->to_stream(aos, ind);
 	}
 	for(int i = 0; i < expressions.size(); i++) {
 		if(expressions[i]) {
 			expressions[i]->to_stream(aos, ind);
 		}
 	}
 }
	parsedExp	declaration;
	virtual const char* spell() const override {
		return "Declarations";
	}
};

class Decomp: public executableExp {
	public:
	Decomp(
		int		main_index,
		parsedExp&	P_tok_act_type,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_optional_expression_list,
		parsedExp&	P_ASCII_41,	// )
		parsedExp&	P_temporalSpecification,
		Differentiator_0);
	Decomp(
		int		main_index,
		parsedExp&	P_tok_call,
		parsedExp&	P_call_or_spawn_arguments,
		parsedExp&	P_temporalSpecification,
		Differentiator_1);
	Decomp(
		int		main_index,
		parsedExp&	P_tok_spawn,
		parsedExp&	P_call_or_spawn_arguments,
		parsedExp&	P_temporalSpecification,
		Differentiator_2);
	~Decomp() {}
	Decomp(const Decomp& p);
	Decomp(bool, const Decomp& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Decomp(*this);
	}
	virtual pE* shallow_copy() override {
		return new Decomp(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	apgen::DECOMP_STATEMENT_TYPE statementType;
	Behavior*		Type;
	vector<parsedExp>	ActualArguments;
	void			consolidate(
					apgen::METHOD_TYPE mt,
					int dent);
	void			consolidate_temporal_spec(
					apgen::METHOD_TYPE mt,
					int dent);
	// parsedExp		conditionalSpec;
	virtual void		execute(
					execution_context*	context,
					execution_context::return_code&	Code,
					pEsys::execStack*	stack_to_use = NULL) override;
	vector<CTime_base>	decomp_times;
	parsedExp		at_expression;
	parsedExp		from_expression;
	parsedExp		to_expression;
	void			get_time_from_start(
					behaving_base* obj);
	void			get_time_from_now(
					behaving_base* obj);
	void			get_time_from_one_expression(
					behaving_base* obj);
	void			(Decomp::*eval_decomp_times)(
					behaving_base*	obj);
	void			execute_one_decomp(
					pEsys::execStack*		stack_to_use,
					behaving_base*		obj,
					const CTime_base&		start_time,
					execution_context::return_code&	Code);
	bool			can_wait() const override {
		return true;
	}
	void			to_stream(
					aoString*	aos,
					int		ind) const override;
	parsedExp	ASCII_40;
	parsedExp	ASCII_41;
	parsedExp	call_or_spawn_arguments;
	parsedExp	call_or_spawn_arguments_1;
	parsedExp	optional_expression_list;
	parsedExp	temporalSpecification;
	parsedExp	temporalSpecification_2;
	parsedExp	tok_act_type;
	parsedExp	tok_call;
	parsedExp	tok_spawn;
	virtual const char* spell() const override {
		return "Decomp";
	}
};

class DefaultExpression: public pE {
	public:
	DefaultExpression(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~DefaultExpression() {}
	DefaultExpression(const DefaultExpression& p);
	DefaultExpression(bool, const DefaultExpression& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new DefaultExpression(*this);
	}
	virtual pE* shallow_copy() override {
		return new DefaultExpression(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	virtual const char* spell() const override {
		return "DefaultExpression";
	}
};

class Directive: public pE {
	public:
	Directive(
		int		main_index,
		parsedExp&	P_one_declarative_assignment,
		parsedExp&	P_tok_directive,
		Differentiator_0);
	Directive(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_directive,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_1);
	~Directive() {}
	Directive(const Directive& p);
	Directive(bool, const Directive& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Directive(*this);
	}
	virtual pE* shallow_copy() override {
		return new Directive(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void	consolidate(int dent);
	smart_ptr<executableExp>	assignment;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	one_declarative_assignment;
	parsedExp	tok_directive;
	parsedExp	tok_directive_0;
	virtual const char* spell() const override {
		return "Directive";
	}
};

class EqualityTest: public pE {
	public:
	EqualityTest(
		int		main_index,
		parsedExp&	P_tok_equal,
		parsedExp&	P_maybe_equal,
		parsedExp&	P_maybe_equal_2,
		Differentiator_0);
	EqualityTest(
		int		main_index,
		parsedExp&	P_tok_notequal,
		parsedExp&	P_maybe_equal,
		parsedExp&	P_maybe_equal_2,
		Differentiator_1);
	~EqualityTest() {}
	EqualityTest(const EqualityTest& p);
	EqualityTest(bool, const EqualityTest& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new EqualityTest(*this);
	}
	virtual pE* shallow_copy() override {
		return new EqualityTest(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void	to_stream(aoString* aos, int ind) const override;
	void	(*binaryFunc)(
			const TypedValue&,
			const TypedValue&,
			TypedValue&);
	static void	func_equal(const TypedValue&, const TypedValue&, TypedValue&);
	static void	func_NOTEQUAL(const TypedValue&, const TypedValue&, TypedValue&);
	bool		eval_bool_val(behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.get_int();
	}
	apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::BOOL_TYPE;
	}
	parsedExp	Operator;
	parsedExp	maybe_equal;
	parsedExp	maybe_equal_0;
	parsedExp	maybe_equal_2;
	parsedExp	tok_equal;
	parsedExp	tok_notequal;
	virtual const char* spell() const override {
		return "EqualityTest";
	}
};

class ExpressionList: public pE {
	public:
	ExpressionList(
		int		main_index,
		parsedExp&	P_Expression,
		Differentiator_0)
;
	~ExpressionList() {}
	ExpressionList(const ExpressionList& p);
	ExpressionList(bool, const ExpressionList& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ExpressionList(*this);
	}
	virtual pE* shallow_copy() override {
		return new ExpressionList(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void consolidate(vector<parsedExp>& args, int dent);
	parsedExp	Expression;
	virtual const char* spell() const override {
		return "ExpressionList";
	}
};

class ExpressionListPrecomp: public pE {
	public:
	ExpressionListPrecomp(
		int		main_index,
		parsedExp&	P_one_series,
		parsedExp&	P_time_series_keyword,
		Differentiator_0);
	~ExpressionListPrecomp() {}
	ExpressionListPrecomp(const ExpressionListPrecomp& p);
	ExpressionListPrecomp(bool, const ExpressionListPrecomp& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ExpressionListPrecomp(*this);
	}
	virtual pE* shallow_copy() override {
		return new ExpressionListPrecomp(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	// not clear yet whether we need this:
	// void consolidate(vector<parsedExp>& args, int dent);

	// in pE_system.C. The main purpose of this override
	// is to prevent the accumulation of nodes in the
	// expressions vector.
	virtual void	addExp(
				const parsedExp& pe) override;
	parsedExp	one_series;
	parsedExp	time_series_keyword;
	virtual const char* spell() const override {
		return "ExpressionListPrecomp";
	}
};

class File: public pE {
	public:
	File(
		int		main_index,
		parsedExp&	P_adaptation_item,
		Differentiator_0)
;
	File(
		int		main_index,
		parsedExp&	P_plan_item,
		Differentiator_1)
;
	~File() {}
	File(const File& p);
	File(bool, const File& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new File(*this);
	}
	virtual pE* shallow_copy() override {
		return new File(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void write_to_stream(aoString* aos, int ind, const Cstring& option) const;
	parsedExp	adaptation_item;
	parsedExp	plan_item;
	virtual const char* spell() const override {
		return "File";
	}
};

class FunctionCall: public executableExp {
	public:
	FunctionCall(
		int		main_index,
		parsedExp&	P_tok_internal_func,
		parsedExp&	P_zero_or_more_args,
		Differentiator_0);
	FunctionCall(
		int		main_index,
		parsedExp&	P_tok_local_function,
		parsedExp&	P_zero_or_more_args,
		Differentiator_1);
	FunctionCall(
		int		main_index,
		parsedExp&	P_symbol,
		parsedExp&	P_zero_or_more_args,
		Differentiator_2);
	FunctionCall(
		int		main_index,
		parsedExp&	P_tok_internal_func,
		parsedExp&	P_zero_or_more_args,
		Differentiator_3);
	FunctionCall(
		int		main_index,
		parsedExp&	P_tok_local_function,
		parsedExp&	P_zero_or_more_args,
		Differentiator_4);
	FunctionCall(
		int		main_index,
		parsedExp&	P_tok_method,
		parsedExp&	P_zero_or_more_args,
		Differentiator_5);
	FunctionCall(
		int		main_index,
		parsedExp&	P_tok_method,
		parsedExp&	P_tok_spawn,
		parsedExp&	P_zero_or_more_args,
		Differentiator_6);
	FunctionCall(
		int		main_index,
		parsedExp&	P_qualified_symbol,
		parsedExp&	P_zero_or_more_args,
		Differentiator_7);
	FunctionCall(
		int		main_index,
		parsedExp&	P_tok_internal_func,
		parsedExp&	P_zero_or_more_args,
		Differentiator_8);
	FunctionCall(
		int		main_index,
		parsedExp&	P_tok_local_function,
		parsedExp&	P_zero_or_more_args,
		Differentiator_9);
	~FunctionCall() {}
	FunctionCall(const FunctionCall& p);
	FunctionCall(bool, const FunctionCall& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new FunctionCall(*this);
	}
	virtual pE* shallow_copy() override {
		return new FunctionCall(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	bfuncnode*		func;		// for internal functions
	task*			TaskInvoked;	// for AAF-defined functions
	void			to_stream(aoString* aos, int ind) const override;
	bool			can_interrupt() const override {
		return true;
	}
	vector<parsedExp>	actual_arguments;
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	apgen::DATA_TYPE get_result_type() const override;
	void consolidate(int dent);

	void	eval_precomputed_expression(
			behaving_base*	local_object,
			TypedValue&	result);

	void	eval_expression(
			behaving_base*	local_object
			) {
		TypedValue val;
		eval_expression(local_object, val);
	}

	//
	// The function call might be qualified by indices and/or members
	//
	smart_ptr<MultidimIndices> indices;
	smart_ptr<ClassMember>	   a_member;

	//
	// For those functions that are AAF scripts:
	//
	static void eval_script(
			const Cstring&		script_name,
			vector<TypedValue*>	actual_arguments,
			TypedValue&	returnedval);
	parsedExp	qualified_symbol;
	parsedExp	symbol;
	parsedExp	tok_internal_func;
	parsedExp	tok_local_function;
	parsedExp	tok_method;
	parsedExp	tok_spawn;
	parsedExp	zero_or_more_args;
	parsedExp	zero_or_more_args_1;
	parsedExp	zero_or_more_args_2;
	virtual const char* spell() const override {
		return "FunctionCall";
	}
};

class FunctionDeclaration: public pE {
	public:
	FunctionDeclaration(
		int		main_index,
		parsedExp&	P_start_function_decl,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_signature,
		parsedExp&	P_ASCII_41,	// )
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~FunctionDeclaration() {}
	FunctionDeclaration(const FunctionDeclaration& p);
	FunctionDeclaration(bool, const FunctionDeclaration& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new FunctionDeclaration(*this);
	}
	virtual pE* shallow_copy() override {
		return new FunctionDeclaration(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void		consolidate(int dent);
	parsedExp	ASCII_40;
	parsedExp	ASCII_41;
	parsedExp	ASCII_59;
	parsedExp	signature;
	parsedExp	start_function_decl;
	virtual const char* spell() const override {
		return "FunctionDeclaration";
	}
};

class FunctionDefinition: public pE {
	public:
	FunctionDefinition(
		int		main_index,
		parsedExp&	P_start_function_def,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_optional_func_params,
		parsedExp&	P_ASCII_123,	// {
		parsedExp&	P_program,
		parsedExp&	P_ASCII_125,	// }
		Differentiator_0);
	~FunctionDefinition() {}
	FunctionDefinition(const FunctionDefinition& p);
	FunctionDefinition(bool, const FunctionDefinition& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new FunctionDefinition(*this);
	}
	virtual pE* shallow_copy() override {
		return new FunctionDefinition(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void write_declaration(aoString* aos, int ind) const;
	void		consolidate(int dent);
	static void	generate_behavior_header(const task&);
	static void	generate_behavior_body(const task&);
	parsedExp	ASCII_123;
	parsedExp	ASCII_125;
	parsedExp	ASCII_40;
	parsedExp	optional_func_params;
	parsedExp	program;
	parsedExp	start_function_def;
	virtual const char* spell() const override {
		return "FunctionDefinition";
	}
};

class FunctionIdentity: public pE {
	public:
	FunctionIdentity(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_tok_func,
		parsedExp&	P_function_return_type,
		Differentiator_0);
	FunctionIdentity(
		int		main_index,
		parsedExp&	P_tok_local_function,
		parsedExp&	P_tok_func,
		parsedExp&	P_function_return_type,
		Differentiator_1);
	FunctionIdentity(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_tok_func,
		Differentiator_2);
	FunctionIdentity(
		int		main_index,
		parsedExp&	P_tok_local_function,
		parsedExp&	P_tok_func,
		Differentiator_3);
	FunctionIdentity(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_tok_model,
		parsedExp&	P_tok_func,
		Differentiator_4);
	FunctionIdentity(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_tok_resource,
		parsedExp&	P_tok_usage,
		parsedExp&	P_tok_func,
		Differentiator_5);
	FunctionIdentity(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_tok_decomposition,
		parsedExp&	P_tok_func,
		Differentiator_6);
	~FunctionIdentity() {}
	FunctionIdentity(const FunctionIdentity& p);
	FunctionIdentity(bool, const FunctionIdentity& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new FunctionIdentity(*this);
	}
	virtual pE* shallow_copy() override {
		return new FunctionIdentity(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	function_return_type;
	parsedExp	function_return_type_1;
	parsedExp	tok_decomposition;
	parsedExp	tok_func;
	parsedExp	tok_func_0;
	parsedExp	tok_func_1;
	parsedExp	tok_func_2;
	parsedExp	tok_id;
	parsedExp	tok_local_function;
	parsedExp	tok_model;
	parsedExp	tok_resource;
	parsedExp	tok_usage;
	virtual const char* spell() const override {
		return "FunctionIdentity";
	}
};

class FunctionReturnType: public pE {
	public:
	FunctionReturnType(
		int		main_index,
		parsedExp&	P_builtin_type,
		Differentiator_0)
;
	FunctionReturnType(
		int		main_index,
		parsedExp&	P_tok_void,
		Differentiator_1)
;
	~FunctionReturnType() {}
	FunctionReturnType(const FunctionReturnType& p);
	FunctionReturnType(bool, const FunctionReturnType& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new FunctionReturnType(*this);
	}
	virtual pE* shallow_copy() override {
		return new FunctionReturnType(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	builtin_type;
	parsedExp	tok_void;
	virtual const char* spell() const override {
		return "FunctionReturnType";
	}
};

class GetInterpwins: public executableExp {
	public:
	GetInterpwins(
		int		main_index,
		parsedExp&	P_expression_list,
		parsedExp&	P_tok_interpwins,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_ASCII_41,	// )
		parsedExp&	P_tok_for,
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~GetInterpwins() {}
	GetInterpwins(const GetInterpwins& p);
	GetInterpwins(bool, const GetInterpwins& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new GetInterpwins(*this);
	}
	virtual pE* shallow_copy() override {
		return new GetInterpwins(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void		to_stream(aoString* aos, int ind) const override;
	void		consolidate(int dent);
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	void		get_windows_of_opportunity(
				pEsys::execStack*	stack_to_use);

	//
	// actual_arguments should only have one element, which is
	// the scheduling condition.
	//

	vector<parsedExp>	actual_arguments;

	//
	// options should be a struct-syle Array
	//

	smart_ptr<Array>	Options;

	//
	// symbol in which the actual duration of the first
	// window found should be stored (optional)
	//

	smart_ptr<Symbol>	ActualSymbol;

	//
	// SchedulingCondition should evaluate to a boolean:
	//

	parsedExp		SchedulingCondition;
	parsedExp	ASCII_40;
	parsedExp	ASCII_41;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	expression_list;
	parsedExp	tok_for;
	parsedExp	tok_interpwins;
	virtual const char* spell() const override {
		return "GetInterpwins";
	}
};

class GetWindows: public executableExp {
	public:
	GetWindows(
		int		main_index,
		parsedExp&	P_expression_list,
		parsedExp&	P_tok_getwindows,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_ASCII_41,	// )
		parsedExp&	P_tok_for,
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~GetWindows() {}
	GetWindows(const GetWindows& p);
	GetWindows(bool, const GetWindows& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new GetWindows(*this);
	}
	virtual pE* shallow_copy() override {
		return new GetWindows(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void		to_stream(aoString* aos, int ind) const override;
	void		consolidate(int dent);
	bool		can_interrupt() const override {
		return true;
	}
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	void		get_windows_of_opportunity(
				pEsys::execStack*	stack_to_use);

	//
	// actual_arguments should only have one element, which is
	// the scheduling condition.
	//
	vector<parsedExp>	actual_arguments;

	//
	// options should be a struct-syle Array
	//
	smart_ptr<Array>	Options;

	//
	// symbol in which the actual duration of the first
	// window found should be stored (optional)
	//
	smart_ptr<Symbol>	ActualSymbol;

	//
	// SchedulingCondition should evaluate to a boolean:
	//
	parsedExp		SchedulingCondition;

	//
	// List of names of resource containers which
	// affect the scheduling condition:
	//
	stringtlist		TriggerNames;
	parsedExp	ASCII_40;
	parsedExp	ASCII_41;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	expression_list;
	parsedExp	tok_for;
	parsedExp	tok_getwindows;
	virtual const char* spell() const override {
		return "GetWindows";
	}
};

class Global: public pE {
	public:
	Global(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_tok_epoch,
		parsedExp&	P_tok_equal_sign,
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	Global(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_tok_timesystem,
		parsedExp&	P_tok_equal_sign,
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_1);
	Global(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_local_or_global,
		parsedExp&	P_any_data_type,
		parsedExp&	P_tok_equal_sign,
		parsedExp&	P_Expression,
		parsedExp&	P_opt_range,
		parsedExp&	P_opt_descr,
		parsedExp&	P_opt_units,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_2);
	~Global() {}
	Global(const Global& p);
	Global(bool, const Global& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Global(*this);
	}
	virtual pE* shallow_copy() override {
		return new Global(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	Cstring		data_type;
	parsedExp	rhs;
	void		to_stream(aoString* aos, int ind) const override;
	void		consolidate(int dent);
	parsedExp	ASCII_59;
	parsedExp	ASCII_59_4;
	parsedExp	ASCII_59_8;
	parsedExp	Expression;
	parsedExp	Expression_3;
	parsedExp	Expression_4;
	parsedExp	any_data_type;
	parsedExp	local_or_global;
	parsedExp	opt_descr;
	parsedExp	opt_range;
	parsedExp	opt_units;
	parsedExp	tok_epoch;
	parsedExp	tok_equal_sign;
	parsedExp	tok_equal_sign_2;
	parsedExp	tok_equal_sign_3;
	parsedExp	tok_id;
	parsedExp	tok_timesystem;
	virtual const char* spell() const override {
		return "Global";
	}
};

class Hierarchy: public pE {
	public:
	Hierarchy(
		int		main_index,
		parsedExp&	P_tok_hierarchy_keyword,
		parsedExp&	P_tok_into,
		parsedExp&	P_id_list,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~Hierarchy() {}
	Hierarchy(const Hierarchy& p);
	Hierarchy(bool, const Hierarchy& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Hierarchy(*this);
	}
	virtual pE* shallow_copy() override {
		return new Hierarchy(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override {
		indent(aos, ind);
		(*aos) << tok_hierarchy_keyword->getData() << " into ";
		id_list->to_stream(aos, 0);
	}
	parsedExp	ASCII_59;
	parsedExp	id_list;
	parsedExp	tok_hierarchy_keyword;
	parsedExp	tok_into;
	virtual const char* spell() const override {
		return "Hierarchy";
	}
};

class IdList: public pE {
	public:
	IdList(
		int		main_index,
		parsedExp&	P_activity_id,
		Differentiator_0)
;
	~IdList() {}
	IdList(const IdList& p);
	IdList(bool, const IdList& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new IdList(*this);
	}
	virtual pE* shallow_copy() override {
		return new IdList(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override {
		indent(aos, ind);
		(*aos) << getData();
		for(int i = 0; i < expressions.size(); i++) {
			(*aos) << ", ";
			expressions[i]->to_stream(aos, 0);
		}
	}
	virtual void	addExp(
				const parsedExp& pe) override {
		if(pe && pe->getData() != ",") {
			expressions.push_back(pe);
		}
	}
	parsedExp	activity_id;
	virtual const char* spell() const override {
		return "IdList";
	}
};

class If: public executableExp {
	public:
	If(
		int		main_index,
		parsedExp&	P_tok_if,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_41,	// )
		Differentiator_0);
	If(
		int		main_index,
		parsedExp&	P_tok_else,
		parsedExp&	P_tok_if,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_41,	// )
		Differentiator_1);
	If(
		int		main_index,
		parsedExp&	P_tok_else,
		Differentiator_2)
;
	~If() {}
	If(const If& p);
	If(bool, const If& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new If(*this);
	}
	virtual pE* shallow_copy() override {
		return new If(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	bool	can_push() const override {
		return true;
	}
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use) override;
	virtual void	execute_if(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use);
	virtual void	execute_elseif(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use);
	virtual void	execute_else(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use);
	void		(If::*execute_method)(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use);
	virtual void	addExp(
				const parsedExp& pe) override;
	parsedExp	ASCII_40;
	parsedExp	ASCII_40_2;
	parsedExp	ASCII_41;
	parsedExp	ASCII_41_4;
	parsedExp	Expression;
	parsedExp	Expression_3;
	parsedExp	tok_else;
	parsedExp	tok_if;
	parsedExp	tok_if_1;
	virtual const char* spell() const override {
		return "If";
	}
};

class InputFile: public pE {
	public:
	InputFile(
		int		main_index,
		parsedExp&	P_file_body,
		parsedExp&	P_tok_apgen,
		parsedExp&	P_tok_version,
		parsedExp&	P_Expression,
		Differentiator_0);
	~InputFile() {}
	InputFile(const InputFile& p);
	InputFile(bool, const InputFile& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new InputFile(*this);
	}
	virtual pE* shallow_copy() override {
		return new InputFile(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override {
		(*aos) << tok_apgen->getData() << " "
			<< tok_version->getData() << " ";
		Expression->to_stream(aos, 0);
		(*aos) << "\n\n";
		file_body->to_stream(aos, ind);
	}

	//
	// to support -write and -declarations options:
	//

	void write_to_stream(
			aoString* aos,
			int ind,
			const Cstring& option) const;
	void write_to_stream_no_header(
			aoString* aos,
			int ind,
			const Cstring& option) const;
	Cstring		InputFileName;
	void		delete_all_instances();
	parsedExp	Expression;
	parsedExp	file_body;
	parsedExp	tok_apgen;
	parsedExp	tok_version;
	virtual const char* spell() const override {
		return "InputFile";
	}
};

class LabeledDefault: public pE {
	public:
	LabeledDefault(
		int		main_index,
		parsedExp&	P_when_case_label,
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~LabeledDefault() {}
	LabeledDefault(const LabeledDefault& p);
	LabeledDefault(bool, const LabeledDefault& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new LabeledDefault(*this);
	}
	virtual pE* shallow_copy() override {
		return new LabeledDefault(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	when_case_label;
	virtual const char* spell() const override {
		return "LabeledDefault";
	}
};

class LabeledDefaultList: public pE {
	public:
	LabeledDefaultList(
		int		main_index,
		parsedExp&	P_labeled_default_list,
		Differentiator_0)
;
	~LabeledDefaultList() {}
	LabeledDefaultList(const LabeledDefaultList& p);
	LabeledDefaultList(bool, const LabeledDefaultList& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new LabeledDefaultList(*this);
	}
	virtual pE* shallow_copy() override {
		return new LabeledDefaultList(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	labeled_default_list;
	virtual const char* spell() const override {
		return "LabeledDefaultList";
	}
};

class LabeledProfiles: public pE {
	public:
	LabeledProfiles(
		int		main_index,
		parsedExp&	P_when_case_label,
		parsedExp&	P_list_of_profile_expressions,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~LabeledProfiles() {}
	LabeledProfiles(const LabeledProfiles& p);
	LabeledProfiles(bool, const LabeledProfiles& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new LabeledProfiles(*this);
	}
	virtual pE* shallow_copy() override {
		return new LabeledProfiles(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	ASCII_59;
	parsedExp	list_of_profile_expressions;
	parsedExp	when_case_label;
	virtual const char* spell() const override {
		return "LabeledProfiles";
	}
};

class LabeledProgram: public pE {
	public:
	LabeledProgram(
		int		main_index,
		parsedExp&	P_when_case_label,
		parsedExp&	P_declarative_program,
		Differentiator_0);
	~LabeledProgram() {}
	LabeledProgram(const LabeledProgram& p);
	LabeledProgram(bool, const LabeledProgram& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new LabeledProgram(*this);
	}
	virtual pE* shallow_copy() override {
		return new LabeledProgram(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	declarative_program;
	parsedExp	when_case_label;
	virtual const char* spell() const override {
		return "LabeledProgram";
	}
};

class LabeledStates: public pE {
	public:
	LabeledStates(
		int		main_index,
		parsedExp&	P_when_case_label,
		parsedExp&	P_expression_list,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~LabeledStates() {}
	LabeledStates(const LabeledStates& p);
	LabeledStates(bool, const LabeledStates& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new LabeledStates(*this);
	}
	virtual pE* shallow_copy() override {
		return new LabeledStates(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	ASCII_59;
	parsedExp	expression_list;
	parsedExp	when_case_label;
	virtual const char* spell() const override {
		return "LabeledStates";
	}
};

class Logical: public pE {
	public:
	Logical(
		int		main_index,
		parsedExp&	P_tok_orop,
		parsedExp&	P_Expression,
		parsedExp&	P_maybe_ORed,
		Differentiator_0);
	Logical(
		int		main_index,
		parsedExp&	P_tok_andop,
		parsedExp&	P_maybe_ORed,
		parsedExp&	P_maybe_ANDed,
		Differentiator_1);
	~Logical() {}
	Logical(const Logical& p);
	Logical(bool, const Logical& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Logical(*this);
	}
	virtual pE* shallow_copy() override {
		return new Logical(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void	(*binaryFunc)(
			parsedExp&,
			parsedExp&,
			behaving_base*,
			TypedValue&);
	static void	func_LAND(parsedExp&, parsedExp&, behaving_base*, TypedValue&);
	static void	func_LOR(parsedExp&, parsedExp&, behaving_base*, TypedValue&);
	bool		eval_bool_val(behaving_base* obj) override {
		TypedValue val;
		eval_expression(obj, val);
		return val.get_int();
	}
	apgen::DATA_TYPE get_result_type() const override {
		return apgen::DATA_TYPE::BOOL_TYPE;
	}
	parsedExp	Operator;
	parsedExp	Lhs;
	parsedExp	Rhs;
	parsedExp	Expression;
	parsedExp	maybe_ANDed;
	parsedExp	maybe_ORed;
	parsedExp	maybe_ORed_0;
	parsedExp	tok_andop;
	parsedExp	tok_orop;
	virtual const char* spell() const override {
		return "Logical";
	}
};

class ModelingSection: public pE {
	public:
	ModelingSection(
		int		main_index,
		parsedExp&	P_resource_usage_header,
		parsedExp&	P_program,
		Differentiator_0);
	~ModelingSection() {}
	ModelingSection(const ModelingSection& p);
	ModelingSection(bool, const ModelingSection& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ModelingSection(*this);
	}
	virtual pE* shallow_copy() override {
		return new ModelingSection(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	program;
	parsedExp	resource_usage_header;
	virtual const char* spell() const override {
		return "ModelingSection";
	}
};

class MultidimIndices: public pE {
	public:
	MultidimIndices(
		int		main_index,
		parsedExp&	P_single_index,
		Differentiator_0)
;
	~MultidimIndices() {}
	MultidimIndices(const MultidimIndices& p);
	MultidimIndices(bool, const MultidimIndices& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new MultidimIndices(*this);
	}
	virtual pE* shallow_copy() override {
		return new MultidimIndices(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	/* Assumption: lhsval points to an array of arrays A
	 * for which the expression
	 *
	 * 		A[a][b]...[z]
	 *
	 * makes sense, where a, b, ..., z are the values of the n indices
	 * stored in the expressions vector. Thus, the depth of A has to
	 * agree with the size of expressions.
	 */
	virtual TypedValue* apply_indices_to(
				TypedValue*	lhsval,
				behaving_base* loc,
				bool tolerant = false);
	/* For fetching a resource from a resource container.
	 * Assumption: the size N of the expressions vector
	 * agrees with the dimension of the container array,
	 * and the evaluated index expressions will be valid
	 * strings in the lists of indices stored in the
	 * container. */
	int		evaluate_flat_index_for(
				Behavior& T,
				behaving_base* loc);
	void to_stream(aoString* aos, int ind) const override;
	vector<parsedExp>	actual_indices;
	parsedExp	single_index;
	virtual const char* spell() const override {
		return "MultidimIndices";
	}
};

class MultiplicativeExp: public pE {
	public:
	MultiplicativeExp(
		int		main_index,
		parsedExp&	P_tok_mult,
		parsedExp&	P_maybe_a_product,
		parsedExp&	P_maybe_a_factor,
		Differentiator_0);
	MultiplicativeExp(
		int		main_index,
		parsedExp&	P_tok_div,
		parsedExp&	P_maybe_a_product,
		parsedExp&	P_maybe_a_factor,
		Differentiator_1);
	MultiplicativeExp(
		int		main_index,
		parsedExp&	P_tok_mod,
		parsedExp&	P_maybe_a_product,
		parsedExp&	P_maybe_a_factor,
		Differentiator_2);
	~MultiplicativeExp() {}
	MultiplicativeExp(const MultiplicativeExp& p);
	MultiplicativeExp(bool, const MultiplicativeExp& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new MultiplicativeExp(*this);
	}
	virtual pE* shallow_copy() override {
		return new MultiplicativeExp(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void	to_stream(aoString* aos, int ind) const override;
	void	(*binaryFunc)(
			const TypedValue&,
			const TypedValue&,
			TypedValue&);
	static void	func_divide(const TypedValue&, const TypedValue&, TypedValue&);
	static void	func_MOD(const TypedValue&, const TypedValue&, TypedValue&);
	static void	func_MULT(const TypedValue&, const TypedValue&, TypedValue&);
	apgen::DATA_TYPE get_result_type() const override;
	parsedExp	Operator;
	parsedExp	maybe_a_factor;
	parsedExp	maybe_a_factor_2;
	parsedExp	maybe_a_product;
	parsedExp	maybe_a_product_0;
	parsedExp	tok_div;
	parsedExp	tok_mod;
	parsedExp	tok_mult;
	virtual const char* spell() const override {
		return "MultiplicativeExp";
	}
};

class Null: public pE {
	public:
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_0)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_1)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_2)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_3)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_4)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_5)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_6)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_7)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_8)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_9)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_10)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_11)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_12)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_13)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_14)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_15)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_16)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_17)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_18)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_19)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_20)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_21)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_22)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_23)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_24)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_25)
;
	Null(
		int		main_index,
		parsedExp&	P_null,
		Differentiator_26)
;
	~Null() {}
	Null(const Null& p);
	Null(bool, const Null& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Null(*this);
	}
	virtual pE* shallow_copy() override {
		return new Null(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	null;
	virtual const char* spell() const override {
		return "Null";
	}
};

class OneNumber: public pE {
	public:
	OneNumber(
		int		main_index,
		parsedExp&	P_tok_number,
		Differentiator_0)
;
	OneNumber(
		int		main_index,
		parsedExp&	P_tok_number,
		parsedExp&	P_tok_minus,
		Differentiator_1);
	~OneNumber() {}
	OneNumber(const OneNumber& p);
	OneNumber(bool, const OneNumber& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new OneNumber(*this);
	}
	virtual pE* shallow_copy() override {
		return new OneNumber(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
 double	get_double() {
	double D = strtod(*tok_number->getData(), NULL);
	if(tok_minus) {
	    D = -D;
	}
	return D;
 }
	parsedExp	tok_minus;
	parsedExp	tok_number;
	virtual const char* spell() const override {
		return "OneNumber";
	}
};

class Parameters: public pE {
	public:
	Parameters(
		int		main_index,
		parsedExp&	P_param_declarations,
		parsedExp&	P_tok_parameters,
		Differentiator_0);
	Parameters(
		int		main_index,
		parsedExp&	P_param_declarations,
		parsedExp&	P_tok_parameters,
		Differentiator_1);
	~Parameters() {}
	Parameters(const Parameters& p);
	Parameters(bool, const Parameters& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Parameters(*this);
	}
	virtual pE* shallow_copy() override {
		return new Parameters(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void consolidate(int dent);
	parsedExp	param_declarations;
	parsedExp	tok_parameters;
	parsedExp	tok_parameters_0;
	virtual const char* spell() const override {
		return "Parameters";
	}
};

class Parentheses: public pE {
	public:
	Parentheses(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_ASCII_41,	// )
		Differentiator_0);
	~Parentheses() {}
	Parentheses(const Parentheses& p);
	Parentheses(bool, const Parentheses& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Parentheses(*this);
	}
	virtual pE* shallow_copy() override {
		return new Parentheses(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void	to_stream(aoString* aos, int ind) const override;
	static void	func_PAREN(const TypedValue&, TypedValue&);
	void	(*unaryFunc)(
			const TypedValue&,
			TypedValue&);
	apgen::DATA_TYPE get_result_type() const override {
		return Expression->get_result_type();
	}
	virtual long int	eval_int_val(
				    behaving_base*	loc) override {
		return Expression->eval_int_val(loc);
	}
	virtual double		eval_double_val(
				    behaving_base*	loc) override {
		return Expression->eval_double_val(loc);
	}
	virtual bool		eval_bool_val(
				    behaving_base*	loc) override {
		return Expression->eval_bool_val(loc);
	}
	virtual CTime_base	eval_time_val(
				    behaving_base*	loc) override {
		return Expression->eval_time_val(loc);
	}
	virtual CTime_base	eval_duration_val(
				    behaving_base*	loc) override {
		return Expression->eval_duration_val(loc);
	}
	virtual Cstring		eval_string_val(
				    behaving_base*	loc) override {
		return Expression->eval_string_val(loc);
	}
	virtual TypedValue	eval_array_val(
				    behaving_base*	loc) override {
		return Expression->eval_array_val(loc);
	}

			/* stores result(s) in the TypedValue
			 * vector provided by the caller: */
	virtual void	eval_expression(
				behaving_base* loc,
				vector<TypedValue>& result) override {
		Expression->eval_expression(loc, result);
	}
			/* does not store any result; used for function calls
			 * that do not return a value */
	virtual void	update_expression(
				behaving_base*	loc) override {
		Expression->update_expression(loc);
	}

			/* for derived classes selected at
			 * consolidation time: */
	virtual void	eval_int(
				behaving_base*	loc,
				long int&		I) override {
		Expression->eval_int(loc, I);
	}
	virtual void	eval_double(
				behaving_base* obj,
				double& L) override {
		Expression->eval_double(obj, L);
	}
			// booleans are implemented as long ints:
	virtual void	eval_bool(
				behaving_base*	loc,
				long int&		I) override {
		Expression->eval_bool(loc, I);
	}

	virtual void	eval_time(
				behaving_base*	loc,
				CTime_base&		T) override {
		Expression->eval_time(loc, T);
	}

	virtual void	eval_duration(
				behaving_base*	loc,
				CTime_base&		T) override {
		Expression->eval_duration(loc, T);
	}

	virtual void	eval_string(
				behaving_base*	loc,
				Cstring&		S) override {
		Expression->eval_string(loc, S);
	}

	virtual void	eval_array(
				behaving_base*	loc,
				TypedValue&	L) override {
		Expression->eval_array(loc, L);
	}
	parsedExp	ASCII_40;
	parsedExp	ASCII_41;
	parsedExp	Expression;
	virtual const char* spell() const override {
		return "Parentheses";
	}
};

class PassiveCons: public pE {
	public:
	PassiveCons(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_tok_constraint,
		parsedExp&	P_tok_id_1,
		parsedExp&	P_ASCII_58,	// :
		parsedExp&	P_tok_begin,
		parsedExp&	P_opt_constraint_class_variables,
		parsedExp&	P_constraint_body,
		parsedExp&	P_constraint_message,
		parsedExp&	P_constraint_severity,
		parsedExp&	P_tok_end,
		parsedExp&	P_tok_constraint_10,
		parsedExp&	P_tok_id_11,
		Differentiator_0);
	~PassiveCons() {}
	PassiveCons(const PassiveCons& p);
	PassiveCons(bool, const PassiveCons& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new PassiveCons(*this);
	}
	virtual pE* shallow_copy() override {
		return new PassiveCons(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	virtual void	consolidate(int dent);
	virtual void	consolidate_round_two(Constraint*);

	Cstring		constraintType;
	Cstring		severity;
	parsedExp	condition;
	CTime_base	maximum_duration;
	parsedExp	theMessage;

	stringtlist	containers_used_in_condition;
	parsedExp	ASCII_58;
	parsedExp	constraint_body;
	parsedExp	constraint_message;
	parsedExp	constraint_severity;
	parsedExp	opt_constraint_class_variables;
	parsedExp	tok_begin;
	parsedExp	tok_constraint;
	parsedExp	tok_constraint_10;
	parsedExp	tok_end;
	parsedExp	tok_id;
	parsedExp	tok_id_1;
	parsedExp	tok_id_11;
	virtual const char* spell() const override {
		return "PassiveCons";
	}
};

class PassiveConsItem: public pE {
	public:
	PassiveConsItem(
		int		main_index,
		parsedExp&	P_constraint_item,
		Differentiator_0)
;
	~PassiveConsItem() {}
	PassiveConsItem(const PassiveConsItem& p);
	PassiveConsItem(bool, const PassiveConsItem& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new PassiveConsItem(*this);
	}
	virtual pE* shallow_copy() override {
		return new PassiveConsItem(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	constraint_item;
	virtual const char* spell() const override {
		return "PassiveConsItem";
	}
};

class PassiveConsMessage: public pE {
	public:
	PassiveConsMessage(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_message,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~PassiveConsMessage() {}
	PassiveConsMessage(const PassiveConsMessage& p);
	PassiveConsMessage(bool, const PassiveConsMessage& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new PassiveConsMessage(*this);
	}
	virtual pE* shallow_copy() override {
		return new PassiveConsMessage(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	tok_message;
	virtual const char* spell() const override {
		return "PassiveConsMessage";
	}
};

class PassiveConsSeverity: public pE {
	public:
	PassiveConsSeverity(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_severity,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~PassiveConsSeverity() {}
	PassiveConsSeverity(const PassiveConsSeverity& p);
	PassiveConsSeverity(bool, const PassiveConsSeverity& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new PassiveConsSeverity(*this);
	}
	virtual pE* shallow_copy() override {
		return new PassiveConsSeverity(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	tok_severity;
	virtual const char* spell() const override {
		return "PassiveConsSeverity";
	}
};

class ProfileList: public pE {
	public:
	ProfileList(
		int		main_index,
		parsedExp&	P_Expression,
		Differentiator_0)
;
	~ProfileList() {}
	ProfileList(const ProfileList& p);
	ProfileList(bool, const ProfileList& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ProfileList(*this);
	}
	virtual pE* shallow_copy() override {
		return new ProfileList(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	Expression;
	virtual const char* spell() const override {
		return "ProfileList";
	}
};

class Program: public pE {
	public:
	Program(
		int		main_index,
		parsedExp&	P_one_declarative_assignment,
		Differentiator_0)
;
	Program(
		int		main_index,
		parsedExp&	P_statement,
		Differentiator_1)
;
	Program(
		int		main_index,
		parsedExp&	P_class_var_declaration,
		Differentiator_2)
;
	~Program() {}
	Program(const Program& p);
	Program(bool, const Program& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Program(*this);
	}
	virtual pE* shallow_copy() override {
		return new Program(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void to_AAF(aoString* aos, int ind) const;
	void to_APF(aoString* aos, int ind) const;
	bool			has_associative_resources() {
		return false;
	}
	apgen::METHOD_TYPE	orig_section;
	apgen::METHOD_TYPE	section_of_origin() {
		return orig_section;
	}
	ProgStyle		get_style() const {
		return compute_type(orig_section);
	}
	static ProgStyle	compute_type(
					apgen::METHOD_TYPE m) {
	    switch(m) {
		case apgen::METHOD_TYPE::NONE:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::FUNCTION:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::ATTRIBUTES:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::PARAMETERS:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::PROFILE:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::STATES:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::USAGE:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::RESUSAGE:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::MODELING:
			return SYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::DECOMPOSITION:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::NONEXCLDECOMP:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::CONCUREXP:
			return SYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::CREATION:
			return ASYNCHRONOUS_STYLE;
		case apgen::METHOD_TYPE::DESTRUCTION:
			return ASYNCHRONOUS_STYLE;
		default:
			return ASYNCHRONOUS_STYLE;
	    }
	}
	vector<smart_ptr<executableExp> >	statements;
	map<Cstring, Assignment*>		symbols;

	//
	// Thanks to this overloaded version of addExp, which is
	// invoked by the syntax, Statement objects get added
	// to the statements vector instead of going into the
	// expressions vector, which is the default behavior.
	//

	virtual void	addExp(
				const parsedExp& pe) override {
		executableExp* ee = dynamic_cast<executableExp*>(pe.object());

		//
		// pe could be "attributes"
		//

		if(ee) {
			statements.push_back(smart_ptr<executableExp>(ee));
		}
	}
	apgen::DATA_TYPE ReturnType;
	apgen::DATA_TYPE get_result_type() const override {
		return ReturnType;
	}
	parsedExp	class_var_declaration;
	parsedExp	one_declarative_assignment;
	parsedExp	statement;
	virtual const char* spell() const override {
		return "Program";
	}
};

class Qualifications: public pE {
	public:
	Qualifications(
		int		main_index,
		parsedExp&	P_qualification,
		Differentiator_0)
;
	Qualifications(
		int		main_index,
		parsedExp&	P_member,
		Differentiator_1)
;
	~Qualifications() {}
	Qualifications(const Qualifications& p);
	Qualifications(bool, const Qualifications& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Qualifications(*this);
	}
	virtual pE* shallow_copy() override {
		return new Qualifications(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	vector<parsedExp>	IndicesOrMembers;
	void	to_stream(aoString* aos, int ind) const override {
		for(int i = 0; i < IndicesOrMembers.size(); i++) {
			IndicesOrMembers[i]->to_stream(aos, ind);
		}
	}
	virtual void	addExp(const parsedExp& pe) override {
		MultidimIndices* mi = dynamic_cast<MultidimIndices*>(pe.object());
		ClassMember* cm = dynamic_cast<ClassMember*>(pe.object());
		assert(mi || cm);
		IndicesOrMembers.push_back(pe);

		if(cm) {
			for(int i = 0; i < cm->expressions.size(); i++) {
				MultidimIndices* mi2
					= dynamic_cast<MultidimIndices*>(cm->expressions[i].object());
				assert(mi2);
				IndicesOrMembers.push_back(cm->expressions[i]);
			}
			cm->expressions.clear();
		}
	}
	parsedExp	member;
	parsedExp	qualification;
	virtual const char* spell() const override {
		return "Qualifications";
	}
};

class QualifiedSymbol: public pE {
	public:
	QualifiedSymbol(
		int		main_index,
		parsedExp&	P_symbol,
		parsedExp&	P_qualifications,
		Differentiator_0);
	~QualifiedSymbol() {}
	QualifiedSymbol(const QualifiedSymbol& p);
	QualifiedSymbol(bool, const QualifiedSymbol& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new QualifiedSymbol(*this);
	}
	virtual pE* shallow_copy() override {
		return new QualifiedSymbol(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;

	int		my_level;
	int		my_index;

	//
	// defaults to true, but should be set
	// to false when evaluating expressions:
	//
	bool		tolerant_array_evaluation;

	smart_ptr<MultidimIndices> indices;
	smart_ptr<ClassMember>	   a_member;

	//
	// Requires that result of evaluation be well-defined;
	// only computes the address of the TypedValue:
	//
	virtual TypedValue& get_val_ref(
				behaving_base* loc) override;

	void		to_stream(aoString* aos, int ind) const override;
	apgen::DATA_TYPE get_result_type() const override;

	//
	// elements of this vector are either MultidimIndices or
	// ClassMember instances:
	//
	parsedExp	expression_list;

 //
 // The purpose of this overloaded version of addExp is as follows:
 //
 //	Example: a qualified symbol of the form
 //
 //		glob_inst.x[a][b][c].w.z[h]
 //
 //	leads to a Symbol named glob_inst whose qualifications element
 //	contains the following items:
 //
 //		item 0:	ClassMember	x
 //		item 1: MultidimIndices	(a, b, c)
 //		item 2: ClassMember	w
 //		item 3: ClassMember	z
 //		item 4: MultidimIndices	(h)
 //
 //	Note that x, w, z must be expressions that evaluate to strings;
 //	each string must be the name of a member of the instance to which
 //	it applies. For example, x must be the name of a member in global_inst,
 //	amd w must be the name of a member in glob_inst.x[a][b][c]. Both
 //	glob_inst and glob_inst.x[a][b][c] must evaluate to instances.
 //
 //	Similarly, a, b, c, h must be expressions that evaluate either to
 //	valid numeric indices in a list or to valid strings in a struct.
 //

	virtual void	addExp(const parsedExp& pe) override {
		Qualifications* q;
		ExpressionList* el;
		if(pe->getData() == "(") {
			// empty argument list
			expression_list = pe;
		} else if((el = dynamic_cast<ExpressionList*>(pe.object()))) {
			expression_list = pe;
		} else if((q = dynamic_cast<Qualifications*>(pe.object()))) {
			qualifications = pe;
		} else {
			assert(false);
		}
	}
	parsedExp	qualifications;
	parsedExp	symbol;
	virtual const char* spell() const override {
		return "QualifiedSymbol";
	}
};

class RaiseToPower: public pE {
	public:
	RaiseToPower(
		int		main_index,
		parsedExp&	P_tok_exponent,
		parsedExp&	P_maybe_a_factor,
		parsedExp&	P_atom,
		Differentiator_0);
	~RaiseToPower() {}
	RaiseToPower(const RaiseToPower& p);
	RaiseToPower(bool, const RaiseToPower& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new RaiseToPower(*this);
	}
	virtual pE* shallow_copy() override {
		return new RaiseToPower(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void	to_stream(aoString* aos, int ind) const override;
	void	(*binaryFunc)(
			const TypedValue&,
			const TypedValue&,
			TypedValue&);
	static void	func_exponent(const TypedValue&, const TypedValue&, TypedValue&);
	apgen::DATA_TYPE get_result_type() const override;
	parsedExp	atom;
	parsedExp	maybe_a_factor;
	parsedExp	tok_exponent;
	virtual const char* spell() const override {
		return "RaiseToPower";
	}
};

class RangeExpression: public pE {
	public:
	RangeExpression(
		int		main_index,
		parsedExp&	P_Expression,
		Differentiator_0)
;
	RangeExpression(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_range_sym,
		Differentiator_1);
	RangeExpression(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_range_sym,
		parsedExp&	P_Expression_2,
		Differentiator_2);
	RangeExpression(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_range_sym,
		parsedExp&	P_tok_multiple_of,
		parsedExp&	P_Expression_3,
		parsedExp&	P_opt_rest_of_range,
		Differentiator_3);
	RangeExpression(
		int		main_index,
		parsedExp&	P_tok_multiple_of,
		parsedExp&	P_Expression,
		parsedExp&	P_opt_rest_of_range,
		Differentiator_4);
	~RangeExpression() {}
	RangeExpression(const RangeExpression& p);
	RangeExpression(bool, const RangeExpression& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new RangeExpression(*this);
	}
	virtual pE* shallow_copy() override {
		return new RangeExpression(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	Expression;
	parsedExp	Expression_1;
	parsedExp	Expression_2;
	parsedExp	Expression_3;
	parsedExp	opt_rest_of_range;
	parsedExp	opt_rest_of_range_2;
	parsedExp	tok_multiple_of;
	parsedExp	tok_range_sym;
	parsedExp	tok_range_sym_1;
	virtual const char* spell() const override {
		return "RangeExpression";
	}
};

class ResUsageWithArgs: public pE {
	public:
	ResUsageWithArgs(
		int		main_index,
		parsedExp&	P_resource_usage_name,
		parsedExp&	P_tok_action,
		parsedExp&	P_ASCII_40,	// (
		parsedExp&	P_optional_expression_list,
		parsedExp&	P_ASCII_41,	// )
		Differentiator_0);
	~ResUsageWithArgs() {}
	ResUsageWithArgs(const ResUsageWithArgs& p);
	ResUsageWithArgs(bool, const ResUsageWithArgs& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ResUsageWithArgs(*this);
	}
	virtual pE* shallow_copy() override {
		return new ResUsageWithArgs(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	ASCII_40;
	parsedExp	ASCII_41;
	parsedExp	optional_expression_list;
	parsedExp	resource_usage_name;
	parsedExp	tok_action;
	virtual const char* spell() const override {
		return "ResUsageWithArgs";
	}
};

class Resource: public pE {
	public:
	Resource(
		int		main_index,
		parsedExp&	P_resource_prefix,
		parsedExp&	P_resource_def,
		Differentiator_0);
	~Resource() {}
	Resource(const Resource& p);
	Resource(bool, const Resource& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Resource(*this);
	}
	virtual pE* shallow_copy() override {
		return new Resource(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	void consolidate_declaration_phase(int dent);
	void consolidate_implementation_phase(int dent);
	void consolidate_one_list_of_states(ExpressionList&, tvslist&, int);
	void consolidate_one_profile(
		RCsource*,
		ProfileList&,
		tlist<alpha_time, Cnode0<alpha_time, parsedExp> >&,
		int);
	parsedExp	resource_def;
	parsedExp	resource_prefix;
	virtual const char* spell() const override {
		return "Resource";
	}
};

class ResourceDef: public pE {
	public:
	ResourceDef(
		int		main_index,
		parsedExp&	P_concrete_resource_def_top_line,
		parsedExp&	P_ASCII_123,	// {
		parsedExp&	P_res_parameter_section_or_null,
		parsedExp&	P_ASCII_125,	// }
		Differentiator_0);
	ResourceDef(
		int		main_index,
		parsedExp&	P_abstract_resource_def_top_line,
		parsedExp&	P_ASCII_123,	// {
		parsedExp&	P_res_parameter_section_or_null,
		parsedExp&	P_ASCII_125,	// }
		Differentiator_1);
	ResourceDef(
		int		main_index,
		parsedExp&	P_abstract_resource_def_top_line,
		parsedExp&	P_tok_begin,
		parsedExp&	P_opt_attributes_section_res,
		parsedExp&	P_res_parameter_section_or_null,
		parsedExp&	P_resource_usage_section,
		parsedExp&	P_tok_end,
		parsedExp&	P_tok_resource,
		parsedExp&	P_tok_id,
		Differentiator_2);
	ResourceDef(
		int		main_index,
		parsedExp&	P_concrete_resource_def_top_line,
		parsedExp&	P_tok_begin,
		parsedExp&	P_opt_attributes_section_res,
		parsedExp&	P_res_parameter_section_or_null,
		parsedExp&	P_opt_states_section,
		parsedExp&	P_opt_profile_section,
		parsedExp&	P_opt_time_series,
		parsedExp&	P_opt_default_section,
		parsedExp&	P_usage_section_or_null,
		Differentiator_3);
	~ResourceDef() {}
	ResourceDef(const ResourceDef& p);
	ResourceDef(bool, const ResourceDef& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ResourceDef(*this);
	}
	virtual pE* shallow_copy() override {
		return new ResourceDef(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;

	//
	// Code for optional generation of adaptation-specific
	// usage classes, which can be used as C++ wrappers
	// and, ultimately, replacements for Usage statements:
	//
	static void	generate_abstract_usage_factory(const Cstring&);
	static void	generate_abstract_usage_mini_factory(const Cstring&);
	static void	generate_abstract_behavior_header(const Behavior&);
	static void	generate_execute_method(const Behavior&);
	parsedExp	ASCII_123;
	parsedExp	ASCII_123_1;
	parsedExp	ASCII_125;
	parsedExp	ASCII_125_3;
	parsedExp	abstract_resource_def_top_line;
	parsedExp	concrete_resource_def_top_line;
	parsedExp	opt_attributes_section_res;
	parsedExp	opt_attributes_section_res_2;
	parsedExp	opt_default_section;
	parsedExp	opt_profile_section;
	parsedExp	opt_states_section;
	parsedExp	opt_time_series;
	parsedExp	res_parameter_section_or_null;
	parsedExp	res_parameter_section_or_null_2;
	parsedExp	res_parameter_section_or_null_3;
	parsedExp	resource_usage_section;
	parsedExp	tok_begin;
	parsedExp	tok_begin_1;
	parsedExp	tok_end;
	parsedExp	tok_id;
	parsedExp	tok_resource;
	parsedExp	usage_section_or_null;
	virtual const char* spell() const override {
		return "ResourceDef";
	}
};

class ResourceInfo: public pE {
	public:
	ResourceInfo(
		int		main_index,
		parsedExp&	P_tok_id,
		parsedExp&	P_tok_resource,
		parsedExp&	P_resource_arrays,
		parsedExp&	P_ASCII_58,	// :
		Differentiator_0);
	~ResourceInfo() {}
	ResourceInfo(const ResourceInfo& p);
	ResourceInfo(bool, const ResourceInfo& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ResourceInfo(*this);
	}
	virtual pE* shallow_copy() override {
		return new ResourceInfo(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	ASCII_58;
	parsedExp	resource_arrays;
	parsedExp	tok_id;
	parsedExp	tok_resource;
	virtual const char* spell() const override {
		return "ResourceInfo";
	}
};

class ResourceType: public pE {
	public:
	ResourceType(
		int		main_index,
		parsedExp&	P_tok_assoc,
		Differentiator_0)
;
	ResourceType(
		int		main_index,
		parsedExp&	P_tok_assoc,
		parsedExp&	P_tok_consumable,
		Differentiator_1);
	ResourceType(
		int		main_index,
		parsedExp&	P_tok_integral,
		parsedExp&	P_tok_id,
		Differentiator_2);
	ResourceType(
		int		main_index,
		parsedExp&	P_tok_consumable,
		Differentiator_3)
;
	ResourceType(
		int		main_index,
		parsedExp&	P_tok_nonconsumable,
		Differentiator_4)
;
	ResourceType(
		int		main_index,
		parsedExp&	P_tok_settable,
		Differentiator_5)
;
	ResourceType(
		int		main_index,
		parsedExp&	P_tok_state,
		Differentiator_6)
;
	ResourceType(
		int		main_index,
		parsedExp&	P_tok_extern,
		Differentiator_7)
;
	~ResourceType() {}
	ResourceType(const ResourceType& p);
	ResourceType(bool, const ResourceType& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ResourceType(*this);
	}
	virtual pE* shallow_copy() override {
		return new ResourceType(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	tok_assoc;
	parsedExp	tok_consumable;
	parsedExp	tok_extern;
	parsedExp	tok_id;
	parsedExp	tok_integral;
	parsedExp	tok_nonconsumable;
	parsedExp	tok_settable;
	parsedExp	tok_state;
	virtual const char* spell() const override {
		return "ResourceType";
	}
};

class Return: public executableExp {
	public:
	Return(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_return,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~Return() {}
	Return(const Return& p);
	Return(bool, const Return& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Return(*this);
	}
	virtual pE* shallow_copy() override {
		return new Return(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	apgen::DATA_TYPE get_result_type() const override {
		return Expression->get_result_type();
	}
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	tok_return;
	virtual const char* spell() const override {
		return "Return";
	}
};

class ReverseRangeExpression: public pE {
	public:
	ReverseRangeExpression(
		int		main_index,
		parsedExp&	P_tok_range_sym,
		parsedExp&	P_Expression,
		Differentiator_0);
	~ReverseRangeExpression() {}
	ReverseRangeExpression(const ReverseRangeExpression& p);
	ReverseRangeExpression(bool, const ReverseRangeExpression& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new ReverseRangeExpression(*this);
	}
	virtual pE* shallow_copy() override {
		return new ReverseRangeExpression(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	Expression;
	parsedExp	tok_range_sym;
	virtual const char* spell() const override {
		return "ReverseRangeExpression";
	}
};

class Signature: public pE {
	public:
	Signature(
		int		main_index,
		parsedExp&	P_signature_stuff,
		parsedExp&	P_tok_id,
		Differentiator_0);
	~Signature() {}
	Signature(const Signature& p);
	Signature(bool, const Signature& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Signature(*this);
	}
	virtual pE* shallow_copy() override {
		return new Signature(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	signature_stuff;
	parsedExp	tok_id;
	virtual const char* spell() const override {
		return "Signature";
	}
};

class SingleIndex: public pE {
	public:
	SingleIndex(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_ASCII_91,	// [
		parsedExp&	P_ASCII_93,	// ]
		Differentiator_0);
	~SingleIndex() {}
	SingleIndex(const SingleIndex& p);
	SingleIndex(bool, const SingleIndex& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new SingleIndex(*this);
	}
	virtual pE* shallow_copy() override {
		return new SingleIndex(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	ASCII_91;
	parsedExp	ASCII_93;
	parsedExp	Expression;
	virtual const char* spell() const override {
		return "SingleIndex";
	}
};

class SinglePrecompResName: public pE {
	public:
	SinglePrecompResName(
		int		main_index,
		parsedExp&	P_tok_stringval,
		Differentiator_0)
;
	~SinglePrecompResName() {}
	SinglePrecompResName(const SinglePrecompResName& p);
	SinglePrecompResName(bool, const SinglePrecompResName& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new SinglePrecompResName(*this);
	}
	virtual pE* shallow_copy() override {
		return new SinglePrecompResName(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	tok_stringval;
	virtual const char* spell() const override {
		return "SinglePrecompResName";
	}
};

class Symbol: public pE {
	public:
	Symbol(
		int		main_index,
		parsedExp&	P_tok_id,
		Differentiator_0)
;
	Symbol(
		int		main_index,
		parsedExp&	P_tok_id,
		Differentiator_1)
;
	Symbol(
		int		main_index,
		parsedExp&	P_tok_id,
		Differentiator_2)
;
	Symbol(
		int		main_index,
		parsedExp&	P_tok_start,
		Differentiator_3)
;
	Symbol(
		int		main_index,
		parsedExp&	P_tok_finish,
		Differentiator_4)
;
	Symbol(
		int		main_index,
		parsedExp&	P_tok_nodeid,
		Differentiator_5)
;
	Symbol(
		int		main_index,
		parsedExp&	P_tok_type,
		Differentiator_6)
;
	~Symbol() {}
	Symbol(const Symbol& p);
	Symbol(bool, const Symbol& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Symbol(*this);
	}
	virtual pE* shallow_copy() override {
		return new Symbol(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;

	int		my_level;
	int		my_index;

	static bool	debug_symbol;

	virtual TypedValue& get_val_ref(
				behaving_base* loc) override;
	void		to_stream(aoString* aos, int ind) const override;

	//
	// elements of this vector are either MultidimIndices or
	// ClassMember instances:
	//

 //
 // The purpose of this overloaded version of addExp is as follows:
 //
 //	Example: a qualified symbol of the form
 //
 //		glob_inst.x[a][b][c].w.z[h]
 //
 //	leads to a Symbol named glob_inst whose qualifications element
 //	contains the following items:
 //
 //		item 0:	ClassMember	x
 //		item 1: MultidimIndices	(a, b, c)
 //		item 2: ClassMember	w
 //		item 3: ClassMember	z
 //		item 4: MultidimIndices	(h)
 //
 //	Note that x, w, z must be expressions that evaluate to strings;
 //	each string must be the name of a member of the instance to which
 //	it applies. For example, x must be the name of a member in global_inst,
 //	amd w must be the name of a member in glob_inst.x[a][b][c]. Both
 //	glob_inst and glob_inst.x[a][b][c] must evaluate to instances.
 //
 //	Similarly, a, b, c, h must be expressions that evaluate either to
 //	valid numeric indices in a list or to valid strings in a struct.
 //

	virtual void	addExp(const parsedExp& pe) override {
		assert(false);
	}
	parsedExp	tok_finish;
	parsedExp	tok_id;
	parsedExp	tok_nodeid;
	parsedExp	tok_start;
	parsedExp	tok_type;
	virtual const char* spell() const override {
		return "Symbol";
	}
};

class TemporalSpec: public pE {
	public:
	TemporalSpec(
		int		main_index,
		parsedExp&	P_tok_from,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_to,
		parsedExp&	P_Expression_3,
		Differentiator_0);
	TemporalSpec(
		int		main_index,
		parsedExp&	P_tok_at,
		parsedExp&	P_Expression,
		Differentiator_1);
	TemporalSpec(
		int		main_index,
		parsedExp&	P_tok_immediately,
		Differentiator_2)
;
	~TemporalSpec() {}
	TemporalSpec(const TemporalSpec& p);
	TemporalSpec(bool, const TemporalSpec& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new TemporalSpec(*this);
	}
	virtual pE* shallow_copy() override {
		return new TemporalSpec(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	Expression;
	parsedExp	Expression_1;
	parsedExp	Expression_3;
	parsedExp	tok_at;
	parsedExp	tok_from;
	parsedExp	tok_immediately;
	parsedExp	tok_to;
	virtual const char* spell() const override {
		return "TemporalSpec";
	}
};

class TimeSeriesStart: public pE {
	public:
	TimeSeriesStart(
		int		main_index,
		parsedExp&	P_tok_time_series,
		Differentiator_0)
;
	~TimeSeriesStart() {}
	TimeSeriesStart(const TimeSeriesStart& p);
	TimeSeriesStart(bool, const TimeSeriesStart& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new TimeSeriesStart(*this);
	}
	virtual pE* shallow_copy() override {
		return new TimeSeriesStart(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	tok_time_series;
	virtual const char* spell() const override {
		return "TimeSeriesStart";
	}
};

class TypeCreationSection: public pE {
	public:
	TypeCreationSection(
		int		main_index,
		parsedExp&	P_tok_creation,
		parsedExp&	P_program,
		Differentiator_0);
	TypeCreationSection(
		int		main_index,
		parsedExp&	P_tok_destruction,
		parsedExp&	P_program,
		Differentiator_1);
	~TypeCreationSection() {}
	TypeCreationSection(const TypeCreationSection& p);
	TypeCreationSection(bool, const TypeCreationSection& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new TypeCreationSection(*this);
	}
	virtual pE* shallow_copy() override {
		return new TypeCreationSection(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	program;
	parsedExp	program_1;
	parsedExp	tok_creation;
	parsedExp	tok_destruction;
	virtual const char* spell() const override {
		return "TypeCreationSection";
	}
};

class Typedef: public pE {
	public:
	Typedef(
		int		main_index,
		parsedExp&	P_typedef_preface,
		parsedExp&	P_tok_equal_sign,
		parsedExp&	P_Expression,
		parsedExp&	P_opt_range,
		parsedExp&	P_opt_descr,
		parsedExp&	P_opt_units,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~Typedef() {}
	Typedef(const Typedef& p);
	Typedef(bool, const Typedef& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Typedef(*this);
	}
	virtual pE* shallow_copy() override {
		return new Typedef(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	opt_descr;
	parsedExp	opt_range;
	parsedExp	opt_units;
	parsedExp	tok_equal_sign;
	parsedExp	typedef_preface;
	virtual const char* spell() const override {
		return "Typedef";
	}
};

class UnaryMinus: public pE {
	public:
	UnaryMinus(
		int		main_index,
		parsedExp&	P_tok_minus,
		parsedExp&	P_exp_modifiable_by_unary_minus,
		Differentiator_0);
	~UnaryMinus() {}
	UnaryMinus(const UnaryMinus& p);
	UnaryMinus(bool, const UnaryMinus& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new UnaryMinus(*this);
	}
	virtual pE* shallow_copy() override {
		return new UnaryMinus(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	static void	func_NEG(const TypedValue&, TypedValue&);
	void		(*unaryFunc)(
				const TypedValue&,
				TypedValue&);
	apgen::DATA_TYPE get_result_type() const override;
	parsedExp	exp_modifiable_by_unary_minus;
	parsedExp	tok_minus;
	virtual const char* spell() const override {
		return "UnaryMinus";
	}
};

class Usage: public executableExp {
	public:
	Usage(
		int		main_index,
		parsedExp&	P_resource_usage_with_arguments,
		parsedExp&	P_temporalSpecification,
		Differentiator_0);
	~Usage() {}
	Usage(const Usage& p);
	Usage(bool, const Usage& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new Usage(*this);
	}
	virtual pE* shallow_copy() override {
		return new Usage(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	apgen::USAGE_TYPE	usageType;
	RCsource*		theContainer;
	Behavior*		abs_type; // the abstract resource used, if it exists
	int			theTaskIndex;
	int			consumptionIndex;
	int			theStorageTaskIndex;
	Cstring			theStorageTaskName;
	smart_ptr<MultidimIndices> indices;
	parsedExp		whenSpec;
	vector<CTime_base>	usage_times;
	vector<parsedExp>	actual_arguments;

	RES_resource*	get_resource_used_by_this(
					behaving_element& loc) const;
	virtual void	refresh_args_and_call_usage_method(
					usage_event&	  event);
	void		to_stream(
					aoString* aos,
					int ind) const override;
	bool		can_wait() const override {
		return true;
	}

	//
	// Actual consolidate methods will be defined in
	// derived classes if the action keyword is
	// anything other than set, and if the resource
	// being used is abstract:
	//
	//	SetUsage
	//	ResetUsage
	//	SignalUsage
	//	AbstractUsage
	//
	virtual void	consolidate(int dent);
	virtual void	consolidate_temporal_spec(
					int dent);

	parsedExp	at_expression;
	parsedExp	from_expression;
	parsedExp	to_expression;
	parsedExp	every_expression;

	virtual void	get_time_from_now(
				behaving_base* obj);
	virtual void	get_time_from_start(
				behaving_base* obj);
	virtual void	get_time_from_start_and_finish(
				behaving_base* obj);
	virtual void	get_time_from_one_expression(
				behaving_base* obj);
	virtual void	get_time_from_two_expressions(
				behaving_base* obj);
	void		(Usage::*eval_usage_times)(
				behaving_base*	obj);
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	parsedExp	resource_usage_with_arguments;
	parsedExp	temporalSpecification;
	virtual const char* spell() const override {
		return "Usage";
	}
};

class UsageSection: public pE {
	public:
	UsageSection(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_usage,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~UsageSection() {}
	UsageSection(const UsageSection& p);
	UsageSection(bool, const UsageSection& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new UsageSection(*this);
	}
	virtual pE* shallow_copy() override {
		return new UsageSection(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	tok_usage;
	virtual const char* spell() const override {
		return "UsageSection";
	}
};

class VarDescription: public pE {
	public:
	VarDescription(
		int		main_index,
		parsedExp&	P_tok_stringval,
		parsedExp&	P_ASCII_63,	// ?
		Differentiator_0);
	~VarDescription() {}
	VarDescription(const VarDescription& p);
	VarDescription(bool, const VarDescription& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new VarDescription(*this);
	}
	virtual pE* shallow_copy() override {
		return new VarDescription(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	ASCII_63;
	parsedExp	tok_stringval;
	virtual const char* spell() const override {
		return "VarDescription";
	}
};

class VarRange: public pE {
	public:
	VarRange(
		int		main_index,
		parsedExp&	P_range_list,
		parsedExp&	P_tok_range,
		Differentiator_0);
	VarRange(
		int		main_index,
		parsedExp&	P_simple_function_call,
		parsedExp&	P_tok_range,
		parsedExp&	P_tok_func,
		Differentiator_1);
	~VarRange() {}
	VarRange(const VarRange& p);
	VarRange(bool, const VarRange& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new VarRange(*this);
	}
	virtual pE* shallow_copy() override {
		return new VarRange(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	range_list;
	parsedExp	simple_function_call;
	parsedExp	tok_func;
	parsedExp	tok_range;
	parsedExp	tok_range_0;
	virtual const char* spell() const override {
		return "VarRange";
	}
};

class VarUnits: public pE {
	public:
	VarUnits(
		int		main_index,
		parsedExp&	P_tok_stringval,
		parsedExp&	P_ASCII_36,	// $
		Differentiator_0);
	~VarUnits() {}
	VarUnits(const VarUnits& p);
	VarUnits(bool, const VarUnits& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new VarUnits(*this);
	}
	virtual pE* shallow_copy() override {
		return new VarUnits(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	parsedExp	ASCII_36;
	parsedExp	tok_stringval;
	virtual const char* spell() const override {
		return "VarUnits";
	}
};

class WaitFor: public executableExp {
	public:
	WaitFor(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_wait,
		parsedExp&	P_tok_for,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~WaitFor() {}
	WaitFor(const WaitFor& p);
	WaitFor(bool, const WaitFor& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new WaitFor(*this);
	}
	virtual pE* shallow_copy() override {
		return new WaitFor(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	bool		can_wait() const override {
		return true;
	}
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	void		consolidate(int dent);
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	tok_for;
	parsedExp	tok_wait;
	virtual const char* spell() const override {
		return "WaitFor";
	}
};

class WaitUntil: public executableExp {
	public:
	WaitUntil(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_wait,
		parsedExp&	P_tok_until,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~WaitUntil() {}
	WaitUntil(const WaitUntil& p);
	WaitUntil(bool, const WaitUntil& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new WaitUntil(*this);
	}
	virtual pE* shallow_copy() override {
		return new WaitUntil(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	bool		can_wait() const override {
		return true;
	}
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	virtual bool	reenter(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL);
	void		consolidate(int dent);
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	tok_until;
	parsedExp	tok_wait;
	virtual const char* spell() const override {
		return "WaitUntil";
	}
};

class WaitUntilRegexp: public executableExp {
	public:
	WaitUntilRegexp(
		int		main_index,
		parsedExp&	P_Expression,
		parsedExp&	P_tok_wait,
		parsedExp&	P_tok_until,
		parsedExp&	P_tok_regexp,
		parsedExp&	P_ASCII_59,	// ;
		Differentiator_0);
	~WaitUntilRegexp() {}
	WaitUntilRegexp(const WaitUntilRegexp& p);
	WaitUntilRegexp(bool, const WaitUntilRegexp& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new WaitUntilRegexp(*this);
	}
	virtual pE* shallow_copy() override {
		return new WaitUntilRegexp(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	bool		can_wait() const override {
		return true;
	}
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	virtual bool	reenter(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL);
	void		consolidate(int dent);
	parsedExp	ASCII_59;
	parsedExp	Expression;
	parsedExp	tok_regexp;
	parsedExp	tok_until;
	parsedExp	tok_wait;
	virtual const char* spell() const override {
		return "WaitUntilRegexp";
	}
};

class While: public executableExp {
	public:
	While(
		int		main_index,
		parsedExp&	P_while_statement_header,
		parsedExp&	P_ASCII_123,	// {
		parsedExp&	P_program,
		parsedExp&	P_ASCII_125,	// }
		Differentiator_0);
	~While() {}
	While(const While& p);
	While(bool, const While& p);
	void recursively_apply(exp_analyzer& EA) override;
	virtual pE* copy() override {
		return new While(*this);
	}
	virtual pE* shallow_copy() override {
		return new While(true, *this);
	}
	virtual void	initExpression(
				const Cstring& nodeData) override;
	virtual void eval_expression(
		behaving_base* loc,
		TypedValue&	result) override;
	void to_stream(aoString* aos, int ind) const override;
	virtual void	execute(
				execution_context*	context,
				execution_context::return_code&	Code,
				pEsys::execStack*	stack_to_use = NULL) override;
	parsedExp	ASCII_123;
	parsedExp	ASCII_125;
	parsedExp	program;
	parsedExp	while_statement_header;
	virtual const char* spell() const override {
		return "While";
	}
};
#endif /* _GEN_PARSED_EXP_H_ */
