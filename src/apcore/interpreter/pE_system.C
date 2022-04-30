#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "ParsedExpressionSystem.H"
#include "aafReader.H"
#include "grammar.H"
#include "apcoreWaiter.H"
#include "Rsource.H"
#include "ActivityInstance.H"
#include "ACT_exec.H"

namespace pEsys {
#include <gen-parsedExp.H>
}

extern int yylineno;

using namespace pEsys;

bool currentval_finder::debug_currentval_finder = false;

Cstring& eval_error::thread_unsafe_msg() {
	static Cstring** C = NULL;
	static bool initialize = true;
	if(initialize) {
		initialize = false;
		C = (Cstring**) malloc(sizeof(Cstring*) * 10);
		for(int i = 0; i < 10; i++) {
			C[i] = new Cstring;
		}
	}
	return *(C[thread_index]);
}

bool& eval_error::thread_unsafe_error() {
	static bool B[10];
	static bool initialize = true;
	if(initialize) {
		initialize = false;
		for(int i = 0; i < 10; i++) {
			B[i] = false;
		}
	}
	return B[thread_index];
}

void eval_error::safety_exception(const Cstring& s) {
	thread_unsafe_error() = true;
	thread_unsafe_msg() = thread_unsafe_msg() + string("\n") + string(*s);
}

pE::pE(
			const origin_info& o,
			const Cstring& nodeData,
			int semantic_value)
	: ref_count(0),
		mainIndex(0),
		lval(semantic_value),
		file(o.file),
		line(o.line) {
	initExpression(nodeData);
}

void pE::initExpression(
			const Cstring&	nodeData) {
	theData = nodeData;
}

//
// deep copy:
//

pE::pE(
				const pE& C)
	: theData(C.theData),
		lval(C.lval),
		ref_count(0),
		mainIndex(0),
		file(C.file),
		line(C.line)
		/* , val(C.val) */ {
	initExpression(C.getData());
	for(int i = 0; i < C.expressions.size(); i++) {
		if(C.expressions[i]) {
			expressions.push_back(parsedExp(
				C.expressions[i]->copy()));
		} else {
			expressions.push_back(parsedExp());
		}
	}
}

//
// shallow copy:
//

pE::pE(bool, const pE& C)
	: theData(C.theData),
		lval(C.lval),
		ref_count(0),
		mainIndex(0),
		file(C.file),
		line(C.line),
		expressions(C.expressions) {
	initExpression(C.getData());
}

void
pE::indent(
		aoString* outStream,
		int indentation) const {
	if(indentation > 0) {
		for(int i = 0; i < indentation; i++) {
			*outStream << " ";
		}
	}
}

void pE::to_stream(
	ostream*	outStream
	) const {
	aoString	CO;

	to_stream(&CO, 0);
	(*outStream) << CO.str();
}

// simplistic white space strategy for now
void	pE::to_stream(
		aoString*	out,
		int		indentation
		) const {
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

void	pE::print(
		Cstring&	s) const {
	aoString	aos;
	to_stream(&aos, 0);
	s = aos.str();
}

// const Cstring&	pE::get_rhs(
// 		bool		sasf_attr,
// 		const blist*	src_of_sasf_syms,
// 		bool		value_only) const {
	// wrong but lets us link:
// 	return theData;
// }

void ActInstance::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "\nactivity instance " << act_instance_header->getData() << " of type ";
	Cstring the_type = act_instance_header->expressions[4]->getData();
	(*aos) << the_type;
	if(act_instance_header->expressions[5]) {
		(*aos) << " id " << act_instance_header->expressions[5]->getData();
	}
	(*aos) << "\n";
	(*aos) << "    begin\n";
	if(decomposition_info) {
		Hierarchy*	hier = dynamic_cast<Hierarchy*>(decomposition_info.object());
		hier->to_stream(aos, 8);
		(*aos) << ";\n";
		// there will be at most one of these:
		for(int i = 0; i < hier->expressions.size(); i++) {
			Hierarchy* hier2 = dynamic_cast<Hierarchy*>(hier->expressions[i].object());
			hier2->to_stream(aos, 8);
			(*aos) << ";\n";
		}
	}
	if(attributes_section_inst) {
		Program* dp = dynamic_cast<Program*>(attributes_section_inst.object());
		(*aos) << "        attributes\n";
		dp->to_stream(aos, 12);
	}
	if(param_section_inst) {
		(*aos) << "        parameters\n";
		(*aos) << "            (\n";
		ExpressionList* el = dynamic_cast<ExpressionList*>(
				param_section_inst.object());
		if(el) {
			el->Expression->to_stream(aos, 12);
			for(int i = 0; i < el->expressions.size(); i++) {
				if(el->expressions[i]->getData() != "("
				   && el->expressions[i]->getData() != ")"
				   && el->expressions[i]->getData() != ","
				   && el->expressions[i]->getData() != ";"
				   && el->expressions[i]->getData() != "parameters") {
					(*aos) << ",\n";
					el->expressions[i]->to_stream(aos, 12);
				}
			}
		}
		(*aos) << "\n            );\n";
	}
	(*aos) << "    end activity instance " << act_instance_header->getData() << "\n";
}

void ActType::to_stream(aoString* aos, int ind) const {
	ActTypeInitial*	ati = dynamic_cast<ActTypeInitial*>(initial_act_type_section.object());
	(*aos) << "\n";
	indent(aos, ind);
	(*aos) << "activity type " << ati->activity_type_header->getData() << "\n";
	(*aos) << "    begin\n";
	ActTypeClassVariables* atcv = dynamic_cast<ActTypeClassVariables*>(ati->opt_type_class_variables.object());
	if(atcv) {
		Program* pg = dynamic_cast<Program*>(atcv->type_class_variables.object());
		pg->to_stream(aos, 8);
	}
	if(ati->opt_act_attributes_section) {
		ActTypeAttributes* ata = dynamic_cast<ActTypeAttributes*>(ati->opt_act_attributes_section.object());
		Program* attributes_program = dynamic_cast<Program*>(ata->declarative_program.object());
		assert(attributes_program);
		(*aos) << "        attributes\n";
		attributes_program->to_AAF(aos, 12);
	}
	Parameters*	params = dynamic_cast<Parameters*>(ati->opt_act_type_param_section.object());
	if(params) {
		params->to_stream(aos, 12);
	}
	ActTypeBody*	atb = dynamic_cast<ActTypeBody*>(body_of_activity_type_def.object());
	if(atb) {
		if(atb->opt_creation_section) {
			TypeCreationSection*	tcs = dynamic_cast<TypeCreationSection*>(atb->opt_creation_section.object());
			(*aos) << "        " << tcs->tok_creation->getData() << "\n";
			tcs->program->to_stream(aos, 12);
		}
		if(atb->opt_res_usage_section) {
			atb->opt_res_usage_section->to_stream(aos, 8);
		}
		if(atb->opt_decomposition_section) {
			ActTypeDecomp*	atd = dynamic_cast<ActTypeDecomp*>(atb->opt_decomposition_section.object());
			(*aos) << "        " << atd->decomposition_header->getData();
			if(atd->decomposition_header->expressions.size()) {
				(*aos) << " "
					<< atd->decomposition_header->expressions[0]->getData();
			}
			(*aos) << "\n";
			Program*	pg = dynamic_cast<Program*>(atd->program.object());
			assert(pg);
			pg->to_stream(aos, 12);
		}
		if(atb->opt_destruction_section) {
			TypeCreationSection* tds = dynamic_cast<TypeCreationSection*>(atb->opt_destruction_section.object());
			(*aos) << "        destruction\n";
			tds->program->to_stream(aos, 12);
		}
	}
	(*aos) << "    end activity type " << ati->activity_type_header->getData() << "\n";
}

//
// We want an array to come out like this:
//
// The declaration
//
// 	x: array default to [1, 2, 3]:
//
// should be translated into
//
// smartArray x(new class foo_0: public ListOVal {
//   public:
//	foo_0() {
//	    foo_0() {
//		add(0, 1);
//		add(1, 2);
//		add(2, 3);
//	    }
//	}
// } );
//
void Array::to_stream(aoString* aos, int ind) const {
	ExpressionList*	el = dynamic_cast<ExpressionList*>(rest_of_list.object());
	Assignment*	as = dynamic_cast<Assignment*>(rest_of_list.object());
	if(el) {
		// list-style array
		indent(aos, ind);
		(*aos) << "[";
		el->to_stream(aos, 0);
		(*aos) << "]";
	} else if(as) {
		// struct-style array
		indent(aos, ind);
		(*aos) << "[";
		as->to_stream(aos, 0);
		// NOTE: the last expression will print the closing bracket!
		for(int i = 0; i < as->expressions.size(); i++) {
			if(as->expressions[i]->getData() != ",") {
				(*aos) << ", ";
				as->expressions[i]->to_stream(aos, 0);
			}
		}
		(*aos) << "]";
	} else {
		indent(aos, ind);
		(*aos) << "[]";
	}
}

void Assignment::to_stream(aoString* aos, int ind) const {
	if(lhs_of_assign) {
		// potential_lval_or_string
		indent(aos, ind);
		Symbol* sym = dynamic_cast<Symbol*>(lhs_of_assign.object());
		if(sym) {
			// Watch out: if we consolidated, this may be a
			// nickname:
			if(original_lhs.is_defined()) {
				(*aos) << original_lhs;
			} else {
				sym->to_stream(aos, 0);
			}
		} else {
			// must be a string. Watch out: we may never have
			// consolidated...
			if(original_lhs.is_defined()) {
				(*aos) << original_lhs;
			} else {
				lhs_of_assign->to_stream(aos, 0);
			}
		}
		(*aos) << " " << assignment_op->getData() << " ";
		Expression->to_stream(aos, 0);
		(*aos) << ";\n";
	} else if(tok_equal_sign) {
		Expression->to_stream(aos, 0);
		(*aos) << " = ";
		Expression_2->to_stream(aos, 0);
		if(opt_range) {
			opt_range->to_stream(aos, 0);
		}
		if(opt_descr) {
			opt_descr->to_stream(aos, 0);
		}
		if(opt_units) {
			opt_units->to_stream(aos, 0);
		}
	}
}

void		ClassMember::addExp(const parsedExp& pe) {
	MultidimIndices* mi = dynamic_cast<MultidimIndices*>(pe.object());
	assert(mi);
	OptionalIndex = pe;
}

TypedValue*	ClassMember::eval_expression_special(
				behaving_base* lhs_obj,
				const Cstring& rhs) {
    const task& T = lhs_obj->Task;
    map<Cstring, int>::const_iterator iter = T.get_varindex().find(rhs);
    if(iter != T.get_varindex().end()) {
	return &(*lhs_obj)[iter->second];
    }

    //
    // If lhs_obj supports a method, look in the constructor
    // of its object before throwing an exception. Why do this?
    // Because lhs_obj is the object that called us. The call
    // may have taken place in a method (most likely, it did).
    // But the variable we seek is most likely a class variable
    // for that object. Therefore, we need the object associated
    // with the constructor, not the object associated with the
    // method from which we were called.
    //
    if(lhs_obj->level[2] && lhs_obj != lhs_obj->level[1]) {
	behaving_base* object = lhs_obj->level[1];

	//
	// This will be the constructor:
	//
	const task& constr = object->Task;
	iter = constr.get_varindex().find(rhs);

	if(iter != constr.get_varindex().end()) {
	    return &(*object)[iter->second];
	}
	Cstring err;
	err << "File " << file << ", line " << line << ": member "
	    << rhs << " not found in " << T.full_name()
	    << " nor in " << constr.full_name();
	throw(eval_error(err));
    }
    Cstring err;
    err << "File " << file << ", line " << line << ": member "
	<< rhs << " not found in " << T.full_name();
    throw(eval_error(err));
}

apgen::DATA_TYPE ClassMember::get_result_type() const {
	// placeholder until we implement class members
	return apgen::DATA_TYPE::UNINITIALIZED;
}

void DataTypeDefaultValue::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	if(builtin_type) {
		(*aos) << "(" << builtin_type->getData() << ")";
	} else if(tok_dyn_type) {
		(*aos) << "(" << tok_dyn_type->getData() << ")";
	}
}

//
// When converting an AAF program to C++ as an execute() method
// for an adaptation-specific class, gather all class variables,
// parameters and local program variables in a first step. Declare
// these variables as members of the class that implements the
// program.
//
// Note that the complete list of variables is available as the
// varinfo vector of the behavior's task.
//
// Then, the execute method does not need to declare any
// variables. Replace all if blocks by switch statements,
// and while blocks by simple switch statements plus a 'goto'
// statement that iterates.
//
void Declaration::to_stream(aoString* aos, int ind) const {
	// indent(aos, ind);
	// (*aos) << param_scope_and_type->getData() << "\t" << tok_id->getData();
	// (*aos) << ";\n";
	indent(aos, ind);
	(*aos) << tok_id->getData() << " = ";
	Expression->to_stream(aos, 0);
	// if(opt_range) {
	// 	opt_range->to_stream(aos, 0);
	// }
	// if(opt_descr) {
	// 	opt_descr->to_stream(aos, 0);
	// }
	// if(opt_units) {
	// 	opt_units->to_stream(aos, 0);
	// }
	(*aos) << ";\n";
}

void Decomp::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	if(tok_act_type) {
		(*aos) << tok_act_type->getData() << "(";
		if(optional_expression_list) {
			optional_expression_list->to_stream(aos, 0);
		}
		(*aos) << ")";
	} else if(tok_call) {
		(*aos) << tok_call->getData() << "(";
		call_or_spawn_arguments->to_stream(aos, 0);
		(*aos) << ")";
	} else if(tok_spawn) {
		(*aos) << tok_spawn->getData() << "(";
		call_or_spawn_arguments->to_stream(aos, 0);
		(*aos) << ")";
	}
	if(temporalSpecification) {
		(*aos) << " ";
		temporalSpecification->to_stream(aos, 0);
	}
	// if(cond_spec) {
	// 	(*aos) << " ";
	// 	cond_spec->to_stream(aos, 0);
	// }
	(*aos) << ";\n";
}

void Directive::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "directive ";
	if(one_declarative_assignment) {
		one_declarative_assignment->to_stream(aos, 0);
	} else {
		Expression->to_stream(aos, 0);
		(*aos) << ";\n";
	}
}

void ExpressionList::to_stream(aoString* aos, int ind) const {
	Expression->to_stream(aos, ind);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]->getData() != "("
		   && expressions[i]->getData() != ")"
		   && expressions[i]->getData() != ","
		   && expressions[i]->getData() != ";"
		  ) {
			(*aos) << ", ";
			expressions[i]->to_stream(aos, 0);
		}
	}
}

void File::to_stream(aoString* aos, int ind) const {
	if(adaptation_item) {
		// AAF definitions
		adaptation_item->to_stream(aos, ind);
	} else if(plan_item) {
		// directives, activity instances
		plan_item->to_stream(aos, ind);
	}
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			expressions[i]->to_stream(aos, ind);
		}
	}
}

void File::write_to_stream(aoString* aos, int ind, const Cstring& option) const {
    if(option == "declarations") {
	if(adaptation_item) {
		// AAF definitions
		FunctionDefinition* fd = dynamic_cast<FunctionDefinition*>(adaptation_item.object());
		if(fd) {
			fd->write_declaration(aos, ind);
		}
	}
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]) {
			FunctionDefinition* fd = dynamic_cast<FunctionDefinition*>(expressions[i].object());
			if(fd) {
				fd->write_declaration(aos, ind);
			}
		}
	}
    }
}


void AdditiveExp::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	Lhs->to_stream(aos, 0);
	(*aos) << " " << Operator->getData() << " ";
	Rhs->to_stream(aos, 0);
}

apgen::DATA_TYPE AdditiveExp::get_result_type() const {
	switch(Lhs->get_result_type()) {
		case apgen::DATA_TYPE::FLOATING:
			switch(Rhs->get_result_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					return apgen::DATA_TYPE::FLOATING;
				default:
					return apgen::DATA_TYPE::UNINITIALIZED;
			}
			break;
		case apgen::DATA_TYPE::INTEGER:
			switch(Rhs->get_result_type()) {
				case apgen::DATA_TYPE::FLOATING:
					return apgen::DATA_TYPE::FLOATING;
				case apgen::DATA_TYPE::INTEGER:
					return apgen::DATA_TYPE::INTEGER;
				default:
					return apgen::DATA_TYPE::UNINITIALIZED;
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(Rhs->get_result_type()) {
				case apgen::DATA_TYPE::DURATION:
					return apgen::DATA_TYPE::DURATION;
				case apgen::DATA_TYPE::TIME:
					return apgen::DATA_TYPE::TIME;
				default:
					return apgen::DATA_TYPE::UNINITIALIZED;
			}
			break;
		case apgen::DATA_TYPE::TIME:
			switch(Rhs->get_result_type()) {
				case apgen::DATA_TYPE::DURATION:
					return apgen::DATA_TYPE::TIME;
				case apgen::DATA_TYPE::TIME:
					return apgen::DATA_TYPE::DURATION;
				default:
					return apgen::DATA_TYPE::UNINITIALIZED;
			}
			break;
		case apgen::DATA_TYPE::STRING:
			return apgen::DATA_TYPE::STRING;
			break;
		default:
			return apgen::DATA_TYPE::UNINITIALIZED;
			break;
	}
}

void MultiplicativeExp::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	maybe_a_product->to_stream(aos, 0);
	(*aos) << " " << Operator->getData() << " ";
	maybe_a_factor->to_stream(aos, 0);
}

apgen::DATA_TYPE MultiplicativeExp::get_result_type() const {
	switch(maybe_a_product->get_result_type()) {
		case apgen::DATA_TYPE::FLOATING:
			switch(maybe_a_factor->get_result_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					return apgen::DATA_TYPE::FLOATING;
				case apgen::DATA_TYPE::DURATION:
					return apgen::DATA_TYPE::DURATION;
				default:
					return apgen::DATA_TYPE::UNINITIALIZED;
			}
			break;
		case apgen::DATA_TYPE::INTEGER:
			switch(maybe_a_factor->get_result_type()) {
				case apgen::DATA_TYPE::FLOATING:
					return apgen::DATA_TYPE::FLOATING;
				case apgen::DATA_TYPE::INTEGER:
					return apgen::DATA_TYPE::INTEGER;
				case apgen::DATA_TYPE::DURATION:

					//
					// Note: this is nonsense for
					// division, but we'll catch that at
					// run time
					//

					return apgen::DATA_TYPE::DURATION;
				default:
					return apgen::DATA_TYPE::UNINITIALIZED;
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(maybe_a_factor->get_result_type()) {
				case apgen::DATA_TYPE::FLOATING:
					return apgen::DATA_TYPE::DURATION;
				case apgen::DATA_TYPE::INTEGER:
					return apgen::DATA_TYPE::DURATION;
				case apgen::DATA_TYPE::DURATION:

					//
					// Note: this is nonsense for
					// multiplication, but we'll catch
					// that at run time
					//

					return apgen::DATA_TYPE::FLOATING;
				default:
					return apgen::DATA_TYPE::UNINITIALIZED;
			}
			break;
		default:
			return apgen::DATA_TYPE::UNINITIALIZED;
			break;
	}
}

void RaiseToPower::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	maybe_a_factor->to_stream(aos, 0);
	(*aos) << " " << tok_exponent->getData() << " ";
	atom->to_stream(aos, 0);
}

apgen::DATA_TYPE RaiseToPower::get_result_type() const {
	switch(maybe_a_factor->get_result_type()) {
		case apgen::DATA_TYPE::INTEGER:
			switch(atom->get_result_type()) {
				case apgen::DATA_TYPE::INTEGER:
				case apgen::DATA_TYPE::FLOATING:
					return apgen::DATA_TYPE::FLOATING;
				default:
					return apgen::DATA_TYPE::UNINITIALIZED;
			}
		case apgen::DATA_TYPE::FLOATING:
			switch(atom->get_result_type()) {
				case apgen::DATA_TYPE::INTEGER:
				case apgen::DATA_TYPE::FLOATING:
					return apgen::DATA_TYPE::FLOATING;
				default:
					return apgen::DATA_TYPE::UNINITIALIZED;
			}
		default:
			return apgen::DATA_TYPE::UNINITIALIZED;
	}
}

void Comparison::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	maybe_compared->to_stream(aos, 0);
	(*aos) << " " << Operator->getData() << " ";
	maybe_compared_2->to_stream(aos, 0);
}

void EqualityTest::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	maybe_equal->to_stream(aos, 0);
	(*aos) << " " << Operator->getData() << " ";
	maybe_equal_2->to_stream(aos, 0);
}

apgen::DATA_TYPE UnaryMinus::get_result_type() const {
	return exp_modifiable_by_unary_minus->get_result_type();
}

void FunctionCall::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	if(tok_internal_func) {
		(*aos) << tok_internal_func->getData();
	} else if(tok_local_function) {
		(*aos) << tok_local_function->getData();
	} else if(qualified_symbol) {
		qualified_symbol->to_stream(aos, 0);
	} else if(symbol) {
		symbol->to_stream(aos, 0);
	}
	ExpressionList*	el = dynamic_cast<ExpressionList*>(zero_or_more_args.object());
	if(el) {
		(*aos) << "(";
		el->to_stream(aos, 0);
		(*aos) << ")";
	} else {
		(*aos) << "()";
	}

	//
	// Look for qualifications
	//

	if(expressions.size() && expressions[0]->getData() != ";") {
		Qualifications* qual = dynamic_cast<Qualifications*>(expressions[0].object());
		assert(qual);
		qual->to_stream(aos, 0);
	}
}

apgen::DATA_TYPE FunctionCall::get_result_type() const {
	if(TaskInvoked) {
		if(TaskInvoked->prog) {
 			return TaskInvoked->prog->ReturnType;
		} else {
 			return TaskInvoked->return_type;
		}
	} else if(func) {
		return func->payload.returntype;
	}
	return apgen::DATA_TYPE::UNINITIALIZED;
}

//
// static method used when executing AAF scripts (XCMD's)
//
void FunctionCall::eval_script(
			const Cstring&		script_name,
			vector<TypedValue*>	actual_arguments,
			TypedValue&		returnedval) {

	//
	// We need to emulate both FunctionCall::consolidate()
	// and FunctionCall::eval_expression()...
	//

	//
	// from FunctionCall::consolidate():
	//
	task*			TaskInvoked = NULL;
	apgen::DATA_TYPE	returned_type;
	Behavior*		global_type = &Behavior::GlobalType();
	map<Cstring, int>::iterator	iter = global_type->taskindex.find(script_name);
	if(iter != global_type->taskindex.end()) {
		TaskInvoked = global_type->tasks[iter->second];

		//
		// NOTE: if TaskInvoked has not been
		// consolidated, the return type will not be
		// known
		//
		if(TaskInvoked->prog) {
			returned_type = TaskInvoked->prog->ReturnType;
		} else {
			returned_type = TaskInvoked->return_type;
		}
	}

	if(!TaskInvoked) {
		Cstring err;
		err << "Trying to execute AAF function " << script_name << "; cannot find it.";
		throw(eval_error(err));
	}

	if(TaskInvoked->paramindex.size() != actual_arguments.size()) {
		Cstring err;
		err << "Trying to call " << script_name
			<< " with " << actual_arguments.size()
			<< " parameter(s); function requires "
			<< TaskInvoked->paramindex.size();
		throw(eval_error(err));
	}
	for(int k = 0; k < TaskInvoked->paramindex.size(); k++) {
		apgen::DATA_TYPE expected_dt = TaskInvoked->get_varinfo()[TaskInvoked->paramindex[k]].second;
		apgen::DATA_TYPE actual_dt = apgen::DATA_TYPE::UNINITIALIZED;
		if((actual_dt = actual_arguments[k]->get_type()) != apgen::DATA_TYPE::UNINITIALIZED) {
			if(!TypedValue::compatible_types(actual_dt, expected_dt)) {
				Cstring err;
				err << "Trying to call " << script_name
					<< "; parameter " << (k+1) << " has type "
					<< apgen::spell(actual_dt) << " but function "
					<< "expects " << apgen::spell(expected_dt);
				throw(eval_error(err));
			}
		}
	}

	//
	// from FunctionCall::eval_expression():
	//

	// must execute a task

	/* We need to define an object that is suitable
	 * for executing the function, and install the evaluated
	 * results in the appropriate value locations */
	function_object* fo = new function_object(
					returnedval,
					*TaskInvoked);

	behaving_element fbe(fo);
	for(int k = 0; k < actual_arguments.size(); k++) {
		int pk = TaskInvoked->paramindex[k];
		(*fo)[pk] = *actual_arguments[k];
	}

	execution_context* context;
	execution_context::return_code Code = execution_context::REGULAR;
	try {

	    //
	    // invoke Execute - NOTE: at some point, replace the constant
	    // strategy by invoking a global that should be set
	    // appropriately when evaluating resource profiles...
	    //
	    context = new execution_context(
		    fbe,
		    TaskInvoked->prog.object());
	    context->ExCon(Code);
	} catch(eval_error& Err) {
		// let's not delete context in case the exception was thrown
		// by the construtor...
		// delete context;
		Cstring err;
		err << "Error calling AAF script " << script_name << ":\n" << Err.msg;
		throw (eval_error(err));
	}

	delete context;
}

void CustomAttributes::to_stream(aoString* aos, int ind) const {
	(*aos) << "attributes\n";
	Declaration*	dec = dynamic_cast<Declaration*>(custom_decls.object());
	(*aos) << "    directive attribute ";
	dec->to_stream(aos, 0);
	for(int i = 0; i < custom_decls->expressions.size(); i++) {
		Declaration*	dec2 = dynamic_cast<Declaration*>(custom_decls->expressions[i].object());
		if(dec2) {
			(*aos) << "    directive attribute ";
			dec2->to_stream(aos, 0);
		}
	}
	(*aos) << "end attributes\n";
}

void FunctionDeclaration::to_stream(aoString* aos, int ind) const {
	FunctionIdentity* fi = dynamic_cast<FunctionIdentity*>(start_function_decl.object());
	assert(fi);
	Cstring func_name = getData();
	Cstring func_keyword = fi->tok_func->getData();
	(*aos) << "\n" << func_keyword << " " << func_name;
	(*aos) << " " << start_function_decl->getData();
	(*aos) << "(";
	if(signature) {
		Signature* sig = dynamic_cast<Signature*>(signature.object());
		if(sig->signature_stuff) {
			sig->signature_stuff->to_stream(aos, 0);
		} else {
			(*aos) << "void";
		}
		(*aos) << " " << sig->tok_id->getData();
		for(int i = 0; i < sig->expressions.size(); i++) {
			sig->expressions[i]->to_stream(aos, 0);
		}
	}
	(*aos) << ");\n";
}

void FunctionDefinition::to_stream(aoString* aos, int ind) const {
	bool any_parameters = optional_func_params->expressions.size() > 0;
	bool param_declarations = false;
	FunctionIdentity* fi = dynamic_cast<FunctionIdentity*>(start_function_def.object());
	assert(fi);
	Cstring func_name = getData();
	Cstring func_keyword = fi->tok_func->getData();

	(*aos) << "\n" << func_keyword << " " << func_name
		<< "(" << optional_func_params->getData();
	for(int i = 0; i < optional_func_params->expressions.size(); i++) {
		parsedExp item = optional_func_params->expressions[i];
		if(item) {
			if(item->getData() == ")") {
				(*aos) << ")\n";
			} else if(item->getData() == "parameters") {
				param_declarations = true;
				(*aos) << "    parameters\n";
			} else if(param_declarations) {
				item->to_stream(aos, 8);
			} else {
				item->to_stream(aos, 0);
			}
		}
	}
	(*aos) << "    {\n";
	program->to_stream(aos, 4);
	(*aos) << "    }\n";
}

#ifdef OBSOLETE
void EmbeddedFunction::to_stream(aoString* aos, int ind) const {
	bool any_parameters = optional_func_params->expressions.size() > 0;
	bool param_declarations = false;
	Cstring func_name = getData();

	(*aos) << "\nfunction " << func_name
		<< "(" << optional_func_params->getData();
	for(int i = 0; i < optional_func_params->expressions.size(); i++) {
		parsedExp item = optional_func_params->expressions[i];
		if(item) {
			if(item->getData() == ")") {
				(*aos) << ")\n";
			} else if(item->getData() == "parameters") {
				param_declarations = true;
				(*aos) << "    parameters\n";
			} else if(param_declarations) {
				item->to_stream(aos, 8);
			} else {
				item->to_stream(aos, 0);
			}
		}
	}
	(*aos) << "    {\n";
	program->to_stream(aos, 4);
	(*aos) << "    }\n";
}
#endif /* OBSOLETE */

void FunctionDefinition::write_declaration(aoString* aos, int ind) const {
    bool any_parameters = optional_func_params->expressions.size() > 0;
    bool found_parameters = false;
    Cstring func_keyword = start_function_def->expressions[0]->getData();

    (*aos) << "\n" << func_keyword << " void " << start_function_def->getData() << "(";
    vector<Cstring> param_names;
    vector<Cstring> param_types;
    if(optional_func_params->getData() != ")") {
	param_names.push_back(optional_func_params->getData());
    }
    for(int i = 0; i < optional_func_params->expressions.size(); i++) {
	parsedExp item = optional_func_params->expressions[i];
	if(item) {
	    if(item->getData() == ")") {
		;
	    } else if(item->getData() == "parameters") {
		found_parameters = true;
	    } else if(found_parameters) {
		Declarations* decls = dynamic_cast<Declarations*>(item.object());
		assert(decls);
		Declaration* decl = dynamic_cast<Declaration*>(decls->declaration.object());
		assert(decl);
		DataType* dt = dynamic_cast<DataType*>(decl->param_scope_and_type.object());
		assert(dt);
		if(dt->builtin_type) {
		    param_types.push_back(dt->builtin_type->getData());
		} else {
		    param_types.push_back("array");
		}
		for(int j = 0; j < decls->expressions.size(); j++) {
		    decl = dynamic_cast<Declaration*>(decls->expressions[j].object());
		    assert(decl);
		    dt = dynamic_cast<DataType*>(decl->param_scope_and_type.object());
		    assert(dt);
		    if(dt->builtin_type) {
		        param_types.push_back(dt->builtin_type->getData());
		    } else {
		        param_types.push_back("array");
		    }
		}
	    } else if(item->getData() != ",") {
	    	param_names.push_back(item->getData());
	    }
	}
    }
    if(param_names.size() != param_types.size()) {
	Cstring errs;
	errs << "File " << file << ", line " << line
		<< " - FunctionDefinition::write_declaration(): mismatch between parameters and declarations";
	throw(eval_error(errs));
    }
    for(int i = 0; i < param_names.size(); i++) {
	(*aos) << param_types[i] << " " << param_names[i];
	if(i < param_names.size() - 1) {
		(*aos) << ", ";
	}
    }
    (*aos) << ");\n";
}

void GetInterpwins::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "interpolated_windows(";
	expression_list->to_stream(aos, 0);
	(*aos) << ") for ";
	Expression->to_stream(aos, 0);
	(*aos) << ";\n";
}

void GetWindows::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "get_windows(";
	expression_list->to_stream(aos, 0);
	(*aos) << ") for ";
	Expression->to_stream(aos, 0);
	(*aos) << ";\n";
}

void Global::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	if(local_or_global) {
		(*aos) << "global " << any_data_type->getData() << " " << tok_id->getData()
			<< " = ";
		Expression->to_stream(aos, 0);
		if(opt_range) {
			(*aos) << " ";
			opt_range->to_stream(aos, 0);
		}
		if(opt_descr) {
			(*aos) << " ";
			opt_descr->to_stream(aos, 0);
		}
		if(opt_units) {
			(*aos) << " ";
			opt_units->to_stream(aos, 0);
		}
		(*aos) << ";\n";
	} else if(tok_epoch) {
		(*aos) << tok_epoch->getData() << " " << tok_id->getData()
			<< " = ";
		Expression->to_stream(aos, 0);
		(*aos) << ";\n";
	}
}

void If::to_stream(aoString*	aos, int ind) const {
    if(tok_else) {
	if(tok_if) {
	    indent(aos, ind);
	    (*aos) << "else if(";
	    Expression->to_stream(aos, 0);
	    (*aos) << ") {\n";
	    for(int k = 0; k < expressions.size(); k++) {
	    	expressions[k]->to_stream(aos, ind + 4);
	    }
	    indent(aos, ind);
	    (*aos) << "}\n";
	} else {
	    indent(aos, ind);
	    (*aos) << "else {\n";
	    for(int k = 0; k < expressions.size(); k++) {
		expressions[k]->to_stream(aos, ind + 4);
	    }
	    indent(aos, ind);
	    (*aos) << "}\n";
	}
    } else {
	indent(aos, ind);
	(*aos) << "if(";
	Expression->to_stream(aos, 0);
	(*aos) << ") {\n";
	for(int k = 0; k < expressions.size(); k++) {
	    expressions[k]->to_stream(aos, ind + 4);
	}
	indent(aos, ind);
	(*aos) << "}\n";
    }
}

void	If::addExp(
			const parsedExp& pe) {
	if(pe->getData() == "{" || pe->getData() == "}") {
		return;
	}
	executableExp* ee = dynamic_cast<executableExp*>(pe.object());
	Program* pp = dynamic_cast<Program*>(pe.object());
	assert(!expressions.size());
	if(ee) {
		parsedExp pe2(pe);
		expressions.push_back(
			parsedExp(new Program(
					0,
					pe2,
					Differentiator_0(0))));
	} else if(pp) {
		expressions.push_back(pe);
	}
	assert(expressions.size());
}

void	InputFile::write_to_stream(
		aoString* aos,
		int ind,
		const Cstring& option) const {
	if(option == "") {
		to_stream(aos, ind);
	} else {
		(*aos) << tok_apgen->getData() << " "
			<< tok_version->getData() << " ";
		Expression->to_stream(aos, 0);
		(*aos) << "\n\n";
		File* fb = dynamic_cast<File*>(file_body.object());
		fb->write_to_stream(aos, ind, option);
	}
}

void	InputFile::write_to_stream_no_header(
		aoString* aos,
		int ind,
		const Cstring& option) const {
	if(option == "") {
		file_body->to_stream(aos, ind);
	} else {
		File* fb = dynamic_cast<File*>(file_body.object());
		fb->write_to_stream(aos, ind, option);
	}
}

void	InputFile::delete_all_instances() {
	File* f = dynamic_cast<File*>(file_body.object());
	assert(f);
	if(f->plan_item) {
		ActInstance* act = dynamic_cast<ActInstance*>(f->plan_item.object());
		if(act) {
			f->plan_item.dereference();
		}
	}
	for(int m = 0; m < f->expressions.size(); m++) {
		ActInstance* act = dynamic_cast<ActInstance*>(f->expressions[m].object());
		if(act) {
			f->expressions[m].dereference();
		}
	}
}

void LabeledProgram::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "when ";
	for(int i = 0; i < when_case_label->expressions.size(); i++) {
		when_case_label->expressions[i]->to_stream(aos, 0);
	}
	(*aos) << "\n";
	declarative_program->to_stream(aos, ind + 4 + 4);
	// additional labels:
	for(int i = 0; i < expressions.size(); i++) {
		expressions[i]->to_stream(aos, ind);
	}
}

void LabeledProfiles::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "when ";
	for(int i = 0; i < when_case_label->expressions.size(); i++) {
		when_case_label->expressions[i]->to_stream(aos, 0);
	}
	(*aos) << "\n";
	list_of_profile_expressions->to_stream(aos, ind + 4 + 4);
	// additional labels:
	for(int i = 0; i < expressions.size(); i++) {
		expressions[i]->to_stream(aos, ind);
	}
}

void LabeledStates::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "when ";
	for(int i = 0; i < when_case_label->expressions.size(); i++) {
		when_case_label->expressions[i]->to_stream(aos, 0);
	}
	(*aos) << " ";
	expression_list->to_stream(aos, 0);
	(*aos) << ";\n";
}

void Logical::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	Lhs->to_stream(aos, 0);
	(*aos) << " " << Operator->getData() << " ";
	Rhs->to_stream(aos, 0);
}

void MultidimIndices::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	for(int i = 0; i < expressions.size(); i++) {
		SingleIndex*	si2 = dynamic_cast<SingleIndex*>(expressions[i].object());
		(*aos) << "[";
		si2->Expression->to_stream(aos, 0);
		(*aos) << "]";
	}
}

		/* For fetching a resource from a resource container.
		 * Assumption: the size N of the expressions vector
		 * agrees with the dimension of the container array,
		 * and the evaluated index expressions will be valid
		 * strings in the lists of indices stored in the
		 * container. */
int MultidimIndices::evaluate_flat_index_for(
				Behavior& T,
				behaving_base* loc
				) {
	int			I = 0;
	vector<parsedExp>&	vec = actual_indices;
	vector<map<Cstring, int> >& maps = T.SubclassMaps;
	map<Cstring, int>*	currentmap;
	map<Cstring, int>::iterator iter;
	parsedExp*		currentindex;
	TypedValue		currentindexval;
	int			k;
	for(int i = 0; i < vec.size(); i++) {
		currentmap = &maps[i];
		currentindex = &vec[i];
		I *= currentmap->size();
		(*currentindex)->eval_expression(loc, currentindexval);
		if(currentindexval.is_int()) {
			k = currentindexval.get_int();
		} else if(currentindexval.is_string()) {
			iter = currentmap->find(currentindexval.get_string());
			if(iter == currentmap->end()) {
				Cstring err;
				err << "File " << file << ", line " << line
					<< ": error while evaluating "
					<< to_string() << ": index "
					<< currentindexval.to_string()
					<< " is not found in " << (i + 1)
					<< "-th index list for resource "
					<< T.name;
				throw(eval_error(err));
			}
			k = iter->second;
		} else {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": error while evaluating "
				<< to_string() << ": index "
				<< currentindexval.to_string()
				<< " is neither a string nor an integer.";
			throw(eval_error(err));
		}
		I += k;
	}
	return I;
}

void Parameters::to_stream(aoString* aos, int ind) const {
	(*aos) << "        parameters\n";
	param_declarations->to_stream(aos, 12);
}

void Parentheses::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "(";
	Expression->to_stream(aos, ind);
	(*aos) << ")";
}

void PassiveCons::to_stream(aoString* aos, int ind) const {
	(*aos) << "\nconstraint " << tok_id_1->getData() << ": " << tok_id->getData() << "\n";
	(*aos) << "    begin\n";
	PassiveConsItem*	pci = dynamic_cast<PassiveConsItem*>(constraint_body.object());
	parsedExp item = pci->constraint_item;
	(*aos) << "        " << item->getData() << "\n";
	item->expressions[0]->to_stream(aos, 12);
	(*aos) << ";\n";
	for(int i = 0; i < pci->expressions.size(); i++) {
		parsedExp item2 = pci->expressions[i];
		(*aos) << "        " << item2->getData() << "\n";
		item2->expressions[0]->to_stream(aos, 12);
		(*aos) << ";\n";
	}
	(*aos) << "        message\n";
	PassiveConsMessage* pcm = dynamic_cast<PassiveConsMessage*>(constraint_message.object());
	pcm->Expression->to_stream(aos, 12);
	(*aos) << ";\n";
	PassiveConsSeverity* pcs = dynamic_cast<PassiveConsSeverity*>(constraint_severity.object());
	(*aos) << "        severity\n";
	pcs->Expression->to_stream(aos, 12);
	(*aos) << ";\n";
	(*aos) << "    end constraint " << tok_id_1->getData() << "\n";
}

void ProfileList::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	Expression->to_stream(aos, 0);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]->getData() == "until") {
			(*aos) << " until ";
		} else if(expressions[i]->getData() == "then") {
			(*aos) << "\n";
			indent(aos, ind);
			(*aos) << "then ";
		} else if(expressions[i]->getData() != ";") {
			expressions[i]->to_stream(aos, 0);
		}
	}
	(*aos) << ";\n";
}

void Program::to_stream(aoString* aos, int ind) const {
	for(int i = 0; i < statements.size(); i++) {
		FunctionCall* fc = dynamic_cast<FunctionCall*>(statements[i].object());
		statements[i]->to_stream(aos, ind);
		if(fc) {
			(*aos) << ";\n";
		}
	}
}

void Program::to_AAF(aoString* aos, int ind) const {
	Cstring					attribute_nickname;
	tlist<alpha_string, Cnode0<alpha_string, Assignment*> >	correct_order(true);
	slist<alpha_string, Cnode0<alpha_string, Assignment*> >::iterator
					CorrectOrder(correct_order);
	Cnode0<alpha_string, Assignment*>*	tag;
	Cstring					lhs;

	for(int iatt = 0; iatt < statements.size(); iatt++) {
		Assignment* assign
			= dynamic_cast<Assignment*>(statements[iatt].object());
		assert(assign);
		lhs = assign->lhs_of_assign->to_string();
		correct_order << new Cnode0<alpha_string, Assignment*>(
						lhs,
						assign);
	}

	//
	// We need to handle 2 separate cases:
	//    (1) consolidation did not take place
	//    (2) consolidation took place
	// In case (1), we can use assign->lhs_of_assign as it is.
	// In case (2), consolidation replaced lhs_of_assign by a
	// symbol whose name is the nickname of the attribute. Therefore,
	// we need to check whether or not we have a nickname, and act
	// accordingly.
	//

	while((tag = CorrectOrder())) {
		Assignment* assign = tag->payload;
		lhs = tag->get_key();
		bool was_consolidated = false;

		map<Cstring, Cstring>::const_iterator iter
			= aafReader::nickname_to_activity_string().find(lhs);
		if(iter != aafReader::nickname_to_activity_string().end()) {
			was_consolidated = true;
			lhs = iter->second;
		}
		(*aos) << "            " << lhs << " "
			<< assign->assignment_op->to_string() << " ";

		if(was_consolidated && lhs == "\"Status\"") {
			TypedValue val;
			assign->Expression->eval_expression(
				behaving_object::GlobalObject(),
				val);
			if(val.get_int()) {
				(*aos) << "\"scheduled\"";
			} else {
				(*aos) << "\"unscheduled\"";
			}
		} else {
			(*aos) << assign->Expression->to_string();
		}
		(*aos) << ";\n";
	}
}

void execution_context::HandleNewBlock(
		Program* P) {
    execution_context::mini_stack* new_counter;
    execution_context::mini_stack* current_counter = counter;
    if(!(new_counter = counter->next)) {

	//
	// By default, new_counter's state will be set to
	// TRUE_CONDITION_FOUND, which is correct given where we are:
	//
	new_counter = new execution_context::mini_stack(P);
	new_counter->prev = counter;
	counter->next = new_counter;
    } else {

	//
	// We can reuse a previously defined counter stored as
	// counter->next. Note that previously defined
	// counters which are no longer in use get deleted
	// automatically and recursively when the top-level counter
	// gets deleted as a result of its execution_context owner
	// exercising its destructor (see 
	//

	new_counter->Prog = P;
	new_counter->state = execution_context::TRUE_CONDITION_FOUND;
    }

    //
    // We switch context to the new counter, but the current one has been
    // safely stashed away as new_counter->next, so Execute() will be
    // able to retrieve it when it is done processing the body of the 'if':
    //
    counter = new_counter;

    //
    // the first thing Execute() will do is increment new_counter's
    // pointer; we want it to point to the first instruction of the if's
    // body at that point:
    //
    counter->I = -1;

    //
    // when Execute() is done with the new counter, it will fall back on
    // current_counter which should point to the instruction immediately
    // following the if:
    //
    current_counter->I++;
}

//
// The Mother of all execution loops in APGenX - execution of all
// adaptation code modules passes through here.
//
// Executing an adaptation program requires two objects:
//
// 	- a pE::Program object (the "program")
// 	- an execution_context object (the "context")
//
// A program starts life as a Parsed Expression, as created by the
// adaptation grammar. During consolidation, parsed data is converted
// to a vector of executable expressions; see the following line in
// apcore/parser/constructors/constructor-specifics.txt:
//
// 	vector<smart_ptr<executableExp> > statements;
//
// This entry appears in the specifics for the pE::Program class; it
// is processed by gramgen to produce the actual class in the
// gen-parsedExp.H header file.
//
void execution_context::ExCon(
		execution_context::return_code& Code,
		execStack*	stack_to_use) {

    //
    // debug
    //
    // dbgind indenter;
    // int execution_count = 0;

    assert(Code == REGULAR);

    try {
	do {
	    executableExp* ee;
	    int k;
	    while((k = counter->I) < counter->Prog->statements.size()) {

		(ee = counter->Prog->statements[k].object())->execute(this, Code, stack_to_use);

		if(eval_error::thread_unsafe_error()) {
		    Cstring errs;
		    errs << "Thread conflict in statement in file "
				<< ee->file << ", line " << ee->line
				<< ": " << eval_error::thread_unsafe_msg();
		    throw(eval_error(errs));
		}
		switch(Code) {
		    case RETURNING:

			while(counter->prev) {
			    counter = counter->prev;
			}
			return;

		    case WAITING:

			//
			// the EC has been inserted in the
			// stack of a new event because of a
			// CALL; we are done.
			//
			// When the event gets to resume
			// execution through its do_model()
			// method, it will jump right into
			// this method - so, let's make sure
			// that the counter is set to the
			// proper instruction for resuming:
			//

			//
			// Well, not anymore - we now create
			// special events for each type of
			// wait; simple wait events do their
			// own incrementing.
			//

			// counter->I++;

			return;

		    case CONTINUING:

			//
			// We need to unwind the stack until we
			// find a WHILE instruction
			//
			{
			bool found_it = false;
			while(counter->prev) {
			    mini_stack* prev_counter = counter->prev;

			    //
			    // Now we face a thorny issue: what does the
			    // previous stack's instruction pointer point
			    // to? If it is a While, it points to itself;
			    // if it is an If, it points to the next
			    // statement...
			    //
			    While* w = dynamic_cast<While*>(
				prev_counter->Prog->statements[prev_counter->I].object());
			    if(w) {
				found_it = true;
				break;
			    }

			    NonRecursiveIf* nri = dynamic_cast<NonRecursiveIf*>(
				prev_counter->Prog->statements[prev_counter->I - 1].object());

			    NonRecursiveElseIf* nrei = dynamic_cast<NonRecursiveElseIf*>(
				prev_counter->Prog->statements[prev_counter->I - 1].object());

			    NonRecursiveElse* nre = dynamic_cast<NonRecursiveElse*>(
				prev_counter->Prog->statements[prev_counter->I - 1].object());

			    //
			    // With 'while', these are the only statements that
			    // 'sponsor' a child program.  Any other possibility
			    // is ruled out by construction.
			    //
			    assert(nri || nrei || nre);
			    counter = prev_counter;
			}
			if(found_it) {

			    //
			    // We know that counter points to the inside of
			    // a While loop. The following statement will cause
			    // the ++ instruction below to reach the end of the
			    // subprogram.
			    //
			    counter->I = counter->Prog->statements.size() - 1;
			    Code = REGULAR;
			} else {
			    Cstring errs;
			    errs << "Continue statement in file "
				<< ee->file << ", line " << ee->line
				<< " is not inside a while loop";
			    throw(eval_error(errs));
			}
			}
			break;

		    case FINISHED:

			//
			// This particular case looks distressing if
			// you are expecting some kind of stack
			// behavior: how could the stack be different
			// from the current stack?
			//
			// The right way to look at this is to realize
			// that the stack behavior is _entirely_ captured
			// in the execution_context and execStack objects.
			//
			// In a sequence of function calls, the C++ stack
			// reflects the DSL stack because function calls
			// result in a nested invocation of ExCon. But
			// when behavioral methods are invoked, the DSL
			// stack winding and unwinding is interrupted by
			// WAIT statements, which always cause ExCon and
			// Execute to return. When the modeling loop
			// revives these interrupted windings, Execute
			// is called from scratch. When a particular
			// method terminates, Execute unwinds the stack
			// as it goes, then calls ExCon to resume execution.
			//

			if(stack_to_use && stack_to_use->get_context() != this) {

			    assert(!stack_to_use->get_context());

			    return;
			}
			Code = REGULAR;

		    case REGULAR:
		    default:

			//
			// We continue executing
			//
			break;
		}
		counter->I++;
	    }
	} while(counter->prev ? (counter = counter->prev):(mini_stack*)0);
    } catch(eval_error Err) {
	Cstring		msg;

	msg << "context->Execute ERROR:\n" << Err.msg << "\n";
	throw(Err);
    }

    Code = FINISHED;
}

void QualifiedSymbol::to_stream(aoString* aos, int ind) const {
	Symbol* sym = dynamic_cast<Symbol*>(symbol.object());
	indent(aos, ind);
	sym->to_stream(aos, 0);
	qualifications->to_stream(aos, 0);
	if(expression_list) {
		if(expression_list->getData() == ")") {
			(*aos) << "()";
		} else {
			(*aos) << "(";
			expression_list->to_stream(aos, 0);
			(*aos) << ")";
		}
	}
}

apgen::DATA_TYPE QualifiedSymbol::get_result_type() const {

	//
	// we have to figure things out at run time,
	// unless we are dealing with a homogeneous
	// array
	//
	return apgen::DATA_TYPE::UNINITIALIZED;
}

TypedValue& QualifiedSymbol::get_val_ref(
		behaving_base* loc) {

    TypedValue&	indexed_val = (*loc->level[my_level])[my_index];
    Qualifications* qual = dynamic_cast<Qualifications*>(qualifications.object());

    //
    // Tentatively, we set the returned value to the value of the
    // symbol we just looked up.  We know this isn't right, since
    // the symbol is qualified.  But it starts the recursion.
    //
    TypedValue*	to_return = &indexed_val;

    for(int i = 0; i < qual->IndicesOrMembers.size(); i++) {
	ClassMember*	cm
		= dynamic_cast<ClassMember*>(qual->IndicesOrMembers[i].object());
	MultidimIndices*	multi
		= dynamic_cast<MultidimIndices*>(qual->IndicesOrMembers[i].object());
	if(multi) {

	    //
	    // This is obsolete; the syntax no longer allows
	    // one to intermix array indexing and member
	    // dereferencing.  There is no deep reason why
	    // not, except that it makes the code more
	    // complicated.
	    //
	    if(to_return->get_type() != apgen::DATA_TYPE::ARRAY) {
		Cstring err;
		err << "File " << file << ", line " << line << ": ";
		err << " indexed symbol \"" << getData() << "\" is not "
		    << "an array; cannot get array element.";
		throw (eval_error(err));
	    }
		
	    try {
		to_return = multi->apply_indices_to(
					to_return,
					loc,
					tolerant_array_evaluation);
	    } catch(eval_error Err) {
		Cstring err;
		err << "File " << file << ", line " << line << ": ";
		err << "array \"" << getData() << "\" does not "
		    << "have the requested element:\n"
		    << Err.msg;
		throw (eval_error(err));
	    }
	} else if(cm) {
	    if(to_return->get_type() != apgen::DATA_TYPE::INSTANCE) {
		Cstring err;
		err << "File " << file << ", line " << line << ": ";
		err << " indexed symbol \"" << getData() << "\" is not "
		    << "a valid instance; cannot get member "
		    << cm->symbol->getData();
		throw (eval_error(err));
	    }
	    behaving_element& be = to_return->get_object();
	    if(!be) {
		Cstring err;
		err << "File " << file << ", line " << line << ": ";
		err << "Cannot get member "
		    << cm->symbol->getData()
		    << " because it is applied to an undefined instance";
		throw (eval_error(err));
	    }
	    const task&	T = be->Task;
	    map<Cstring, int>::const_iterator iter =
				T.get_varindex().find(cm->getData());
	    if(iter == T.get_varindex().end()) {

		//
		// look for class members
		//
		const task& Constr = *be->Type.tasks[0];
		iter = Constr.get_varindex().find(cm->getData());
		if(iter == Constr.get_varindex().end()) {
		    Cstring err;
		    aoString aos;
		    err << "File " << file << ", line " << line << ": ";
		    err << " \"" << cm->symbol->getData() << "\" is not "
			<< "a member of instance ";
	       	    be->print(aos, "");
		    err << aos.str();
		    throw (eval_error(err));
		}

		//
		// We know it's level 1 because it is an instance variable
		//
		to_return = &((*be->level[1])[iter->second]);
	    } else {
		to_return = &((*be)[iter->second]);
	    }

	    //
	    // Before returning, we must determine whether there
	    // is an optional index, and if there is, we must
	    // apply it.
	    //
	    if(cm->OptionalIndex) {
		MultidimIndices*	multi
		    = dynamic_cast<MultidimIndices*>(cm->OptionalIndex.object());
		if(to_return->get_type() != apgen::DATA_TYPE::ARRAY) {
		    Cstring err;
		    err << "File " << file << ", line " << line << ": ";
		    err << " indexed symbol \"" << getData() << "\" is not "
		        << "an array; cannot get array element.";
		    throw (eval_error(err));
		}
		
		try {
		    to_return = multi->apply_indices_to(
					to_return,
					loc,
					tolerant_array_evaluation);
		} catch(eval_error Err) {
		    Cstring err;
		    err << "File " << file << ", line " << line << ": ";
		    err << "array \"" << getData() << "\" does not "
		        << "have the requested element:\n"
		        << Err.msg;
		    throw (eval_error(err));
		}
	    }
	}
    }

    return *to_return;
}

void ReverseRangeExpression::to_stream(aoString* aos, int ind) const {
	tok_range_sym->to_stream(aos, ind);
	Expression->to_stream(aos, 0);
}

void RangeExpression::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	/* 6 cases:
	 * 	- range is an Expression
	 * 	- range starts with Expression, then ->
	 * 	- range starts with Expression, then ->, then Expression
	 * 	- range starts with Expression, then ->, then ...* Expression, then optional -> Expression
	 * 	- range starts with ...*, then Expression, then optional -> Expression
	 * 	- range starts with ->, then Expression (handled by ReverseRangeExpression)
	 */
	if(!tok_range_sym && !tok_multiple_of) {
		// case 1
		Expression->to_stream(aos, 0);
	} else if(tok_range_sym) {
		// cases 2, 3, 4
		Expression->to_stream(aos, 0);
		tok_range_sym->to_stream(aos, 0);
		if(Expression_2) {
			Expression_2->to_stream(aos, 0);
		} else if(tok_multiple_of) {
			tok_multiple_of->to_stream(aos, 0);
			Expression_3->to_stream(aos, 0);
			if(opt_rest_of_range) {
				opt_rest_of_range->to_stream(aos, 0);
			}
		}
	} else {
		// case 5
		tok_multiple_of->to_stream(aos, 0);
		Expression->to_stream(aos, 0);
		if(opt_rest_of_range) {
			opt_rest_of_range->to_stream(aos, 0);
		}
	}
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]->getData() == ",") {
			(*aos) << ", ";
		} else {
			expressions[i]->to_stream(aos, 0);
		}
	}
}

void Return::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "return ";
	Expression->to_stream(aos, 0);
	(*aos) << ";\n";
}

void Symbol::to_stream(aoString* aos, int ind) const {
	Cstring sym;
	if(debug_symbol) {
		(*aos) << "(" << my_level << "->" << my_index << ") ";
	}
	if(tok_finish) {
		sym = tok_finish->getData();
	} else if(tok_id) {
		sym = tok_id->getData();
	} else if(tok_nodeid) {
		sym = tok_nodeid->getData();
	} else if(tok_start) {
		sym = tok_start->getData();
	} else if(tok_type) {
		sym = tok_type->getData();
	} else {
		sym = "bizarre symbol";
	}
	indent(aos, ind);
	if(!my_level) {
		(*aos) << "global_data::";
	}
	(*aos) << sym;
}

TypedValue& Symbol::get_val_ref(
			behaving_base* loc) {
	return (*loc->level[my_level])[my_index];
}

TypedValue& ByValueSymbol::get_val_ref(
			behaving_base* loc) {
	/* 2 modes:
	 * 	- when creating an event, vars passed by value must
	 * 	  be extracted from the user and stored in the target
	 * 	- when refreshing parameters, vars passed by value
	 * 	  must be taken from the target store
	 */
	if(loc->var_should_be_stored) {
		task* use_task = my_task->Type.tasks[my_use_task_index];
		assert(	my_index_in_use_task >= 0
			&& my_index_in_use_task < use_task->get_varinfo().size());

		if(my_task != &loc->level[my_level]->Task) {
			Cstring err;
			err << "File " << file << ", line " << line << ": "
				<< "ByValueSymbol eval error: my_task = "
				<< my_task->full_name() << ", object(level = "
				<< my_level << ") = "
				<< loc->level[my_level]->Task.full_name();
			throw(eval_error(err));
		}
		TypedValue& result = (*loc->level[my_level])[my_index];
		(*loc->temporary_storage)[my_index_in_use_task] = result;
	}
	return (*loc->temporary_storage)[my_index_in_use_task];
}


void TemporalSpec::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	if(tok_at) {
		(*aos) << tok_at->getData() << " ";
		Expression->to_stream(aos, 0);
	}
	if(tok_from) {
		(*aos) << tok_from->getData() << " ";
		Expression->to_stream(aos, 0);
	}
	if(tok_to) {
		(*aos) << " " << tok_to->getData() << " ";
		Expression_3->to_stream(aos, 0);
	}
	if(tok_immediately) {
		(*aos) << tok_immediately->getData();
	}
}

void Typedef::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "typedef " << typedef_preface->expressions[1]->getData()
		<< " " << typedef_preface->getData();
	(*aos) << " = ";
	Expression->to_stream(aos, 0);
	if(opt_range) {
		(*aos) << " ";
		opt_range->to_stream(aos, 0);
	}
	if(opt_descr) {
		(*aos) << " ";
		opt_descr->to_stream(aos, 0);
	}
	if(opt_units) {
		(*aos) << " ";
		opt_units->to_stream(aos, 0);
	}
	(*aos) << ";\n";
}

void UnaryMinus::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "-";
	exp_modifiable_by_unary_minus->to_stream(aos, 0);
}

void Usage::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	ResUsageWithArgs* resusage
		= dynamic_cast<ResUsageWithArgs*>(
			resource_usage_with_arguments.object());
	(*aos) << resusage->tok_action->getData() << " "
		<< resusage->resource_usage_name->getData();
	if(resusage->resource_usage_name->expressions.size()
	   && resusage->resource_usage_name->expressions[0]) {
		MultidimIndices* multi = dynamic_cast<MultidimIndices*>(
			resusage->resource_usage_name->expressions[0].object());
		multi->to_stream(aos, 0);
	}

	(*aos) << "(";
	ExpressionList* el = dynamic_cast<ExpressionList*>(resusage->optional_expression_list.object());
	if(el) {
		el->to_stream(aos, 0);
	}
	(*aos) << ")";
	if(temporalSpecification) {
		(*aos) << " ";
		temporalSpecification->to_stream(aos, 0);
	}
	// if(cond_spec) {
	// 	(*aos) << " ";
	// 	cond_spec->to_stream(aos, 0);
	// }
	(*aos) << ";\n";
}

void VarDescription::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << " ? " << tok_stringval->getData();
}

void VarRange::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << " range ";
	range_list->to_stream(aos, ind);
}

void VarUnits::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << " $ " << tok_stringval->getData();
}

void WaitFor::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "wait for ";
	Expression->to_stream(aos, 0);
	(*aos) << ";\n";
}

void WaitUntil::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "wait until ";
	Expression->to_stream(aos, 0);
	(*aos) << ";\n";
}

void WaitUntilRegexp::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "wait until regexp ";
	Expression->to_stream(aos, 0);
	(*aos) << ";\n";
}

void While::to_stream(aoString*	aos, int ind) const {
	indent(aos, ind);
	(*aos) << "while(";
	while_statement_header->to_stream(aos, 0);
	(*aos) << ") {\n";
	program->to_stream(aos, ind + 4);
	indent(aos, ind);
	(*aos) << "}\n";
}

void	ConstantTime::to_stream(
		aoString*	out,
		int		indentation
		) const {
    CTime_base	T(val.get_time_or_duration());
    (*out) << "CTime_base(" << addQuotes(T.to_string()) << ")";
}

void	ConstantDuration::to_stream(
		aoString*	out,
		int		indentation
		) const {
    CTime_base	T(val.get_time_or_duration());
    (*out) << "CTime_base(" << addQuotes(T.to_string()) << ")";
}

void	AssignmentPrecomp::addExp(const parsedExp& pe) {
    if(pe && pe->getData() != ",") {
	AssignmentPrecomp* assign = dynamic_cast<AssignmentPrecomp*>(pe.object());
	assert(assign);

	//
	// assign->initExpression() has done all the work.
	// We no longer need this node. We don't need to
	// dereference anything, because we are not inserting
	// pe into the expressions vector and so it will be
	// dereferenced automatically when it falls out of scope.
	//
    }
}

void	ExpressionListPrecomp::addExp(const parsedExp& pe) {
    if(pe && pe->getData() != ",") {

	//
	// The main purpose of this overridden method
	// is that it does not add anything to the
	// expressions vector, so we don't use too much RAM!
	//
    }
}


#include "gen-parsedExp.C"
