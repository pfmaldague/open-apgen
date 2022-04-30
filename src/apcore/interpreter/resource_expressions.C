#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "ParsedExpressionSystem.H"
#include "aafReader.H"
#include "grammar.H"
#include "apcoreWaiter.H"
#include "Rsource.H"
#include "action_request.H"
#include "ActivityInstance.H"
#include "ACT_exec.H"
#include "EventLoop.H"

namespace pEsys {
#include <gen-parsedExp.H>
}

extern int yylineno;

using namespace pEsys;

void ArrayList::to_stream(aoString* aos, int ind) const {

	//
	// It is not necessary to figure out exactly what one_array is
	//

	// Array*	anArray = dynamic_cast<Array*>(one_array.object());
	// Symbol*	aSymbol = dynamic_cast<Symbol*>(one_array.object());

	one_array->to_stream(aos, 0);
	for(int i = 0; i < expressions.size(); i++) {
		if(expressions[i]->getData() != "(" && expressions[i]->getData() != ")") {
			(*aos) << ", ";
			expressions[i]->to_stream(aos, 0);
		}
	}
}

void	ArrayList::addExp(
		const parsedExp& pe) {
	if(pe->getData() != ",") {
		pE::addExp(pe);
	}
}

void ModelingSection::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << resource_usage_header->getData();
	if(resource_usage_header->expressions.size()) {
		(*aos) << " ";
		(*aos) << resource_usage_header->expressions[0]->getData();
	}
	(*aos) << "\n";
	program->to_stream(aos, 12);
}

void Resource::to_stream(aoString* aos, int ind) const {
	ResourceInfo*	ri = dynamic_cast<ResourceInfo*>(resource_prefix.object());
	ResourceDef*	rd = dynamic_cast<ResourceDef*>(resource_def.object());
	bool	is_a_declaration = rd->ASCII_123;
	ri->to_stream(aos, ind);
	if(is_a_declaration) {
		indent(aos, ind);
		(*aos) << "{\n";
		if(rd->res_parameter_section_or_null) {
			rd->res_parameter_section_or_null->to_stream(aos, ind);
		}
		(*aos) << "}\n";
	} else {
		indent(aos, ind);
		(*aos) << "    begin\n";
		if(rd->opt_attributes_section_res) {
			(*aos) << "        attributes\n";
			Program* dp
				= dynamic_cast<Program*>(
					rd->opt_attributes_section_res->expressions[0].object());
			LabeledProgram* lp
				= dynamic_cast<LabeledProgram*>(
					rd->opt_attributes_section_res->expressions[0].object());
			if(dp) {
				dp->to_stream(aos, 12);
			} else if(lp) {
				lp->to_stream(aos, 12);
			}
		}
		if(rd->res_parameter_section_or_null) {
			rd->res_parameter_section_or_null->to_stream(aos, ind);
		}
		if(rd->opt_states_section) {
			ExpressionList*	aList = dynamic_cast<ExpressionList*>(
					rd->opt_states_section->expressions[0].object());
			LabeledStates* states = dynamic_cast<LabeledStates*>(
					rd->opt_states_section->expressions[0].object());
			(*aos) << "        states\n";
			if(aList) {
				aList->to_stream(aos, 12);
				(*aos) << ";\n";
			} else if(states) {
				states->to_stream(aos, 12);
				for(int i = 0; i < states->expressions.size(); i++) {
					states->expressions[i]->to_stream(aos, 12);
				}
			}
		}
		if(rd->opt_profile_section) {
			ProfileList* aProfile = dynamic_cast<ProfileList*>(
					rd->opt_profile_section->expressions[0].object());
			LabeledProfiles* profileList = dynamic_cast<LabeledProfiles*>(
					rd->opt_profile_section->expressions[0].object());
			(*aos) << "        profile\n";
			if(aProfile) {
				aProfile->to_stream(aos, 12);
			} else if(profileList) {
				profileList->to_stream(aos, 12);
			}
		}
		if(rd->usage_section_or_null) {
			UsageSection* us = dynamic_cast<UsageSection*>(
				rd->usage_section_or_null.object());
			ModelingSection* ms = dynamic_cast<ModelingSection*>(
				rd->usage_section_or_null.object());
			if(us) {
				indent(aos, 8);
				(*aos) << "usage\n";
				us->Expression->to_stream(aos, 12);
				(*aos) << ";\n";
			} else if(ms) {
				ms->to_stream(aos, 8);
			}
		}
		(*aos) << "    end resource " << ri->tok_id->getData() << "\n";
	}
}

void ResourceInfo::to_stream(aoString* aos, int ind) const {
	(*aos) << "\nresource " << tok_id->getData();
	if(resource_arrays) {
		Array*	anArray = dynamic_cast<Array*>(resource_arrays.object());
		ArrayList* aList = dynamic_cast<ArrayList*>(resource_arrays.object());
		if(anArray) {
			anArray->to_stream(aos, 0);
		} else if(aList) {
			(*aos) << "(";
			aList->to_stream(aos, 0);
			(*aos) << ")";
		}
	}
	(*aos) << ": ";
}

void ResourceType::to_stream(aoString* aos, int) const {
	if(tok_assoc) {
		(*aos) << tok_assoc->getData();
		if(tok_consumable) {
			(*aos) << " " << tok_consumable->getData();
		}
	} else if(tok_integral) {
		(*aos) << tok_integral->getData();
		(*aos) << " " << tok_id->getData();
	} else if(tok_consumable) {
		(*aos) << " " << tok_consumable->getData();
	} else if(tok_nonconsumable) {
		(*aos) << " " << tok_nonconsumable->getData();
	} else if(tok_state) {
		(*aos) << " " << tok_state->getData();
	} else if(tok_extern) {
		(*aos) << " " << tok_extern->getData();
	}
}

//
// SimpleCurrentVal is used inside a resource definition, where currentval()
// can be used in standalone mode:
//
// 	c = currentval();		      // SimpleCurrentVal
//
// here, currentval() means "the current value of the ambient resource". In
// all other cases, the resource must be specified explicitly, as in
//
// 	c = Foo.currentval();		      // ResourceCurrentVal
//
// or
//
// 	c = Foo["some index"].currentval();   // ArrayedResourceCurrentVal
//
SimpleCurrentVal::SimpleCurrentVal(const SimpleCurrentVal& scv)
	: my_res(scv.my_res), pE(scv) {}

SimpleCurrentVal::SimpleCurrentVal(bool, const SimpleCurrentVal& scv)
	: my_res(scv.my_res), pE(true, scv) {}

apgen::DATA_TYPE SimpleCurrentVal::get_result_type() const {
	return my_res->get_datatype();
}

void SimpleCurrentVal::eval_expression(
		behaving_base*	obj,
		TypedValue&	val) {
	res_usage_object* ruo = dynamic_cast<res_usage_object*>(obj);
	assert(ruo);

	ruo->get_concrete_res()->evaluate_present_value(
		val);
}

void SimpleCurrentVal::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << "currentval()";
}

//
// Current value of a resource expressed as a resource method:
//
// 	c = Foo.currentval()
//
ResourceCurrentVal::ResourceCurrentVal(
			const origin_info& o,
			const Cstring& resname)
		: ResourceMethod(o, resname) {
	
	//
	// This constructor is only invoked from consolidate()
	//

	//
	// First thing is to get the resource
	//
	RCsource* container = RCsource::resource_containers().find(resname);
	if(!container) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": applying currentval() to non-concrete resource "
			<< resname;
		throw(eval_error(errs));
	}

	//
	// We know there is only one resource in
	// array_elements. If the resource is an
	// array, look at ArrayedResourceCurrentVal,
	// below.
	//
	my_resource = container->payload->Object->array_elements[0];
}

ResourceCurrentVal::ResourceCurrentVal(const ResourceCurrentVal& rcv)
	: ResourceMethod(rcv),
		my_resource(rcv.my_resource) {
}

ResourceCurrentVal::ResourceCurrentVal(bool, const ResourceCurrentVal& rcv)
	: ResourceMethod(true, rcv),
		my_resource(rcv.my_resource) {
}

void ResourceCurrentVal::eval_expression(
		behaving_base* obj,
		TypedValue& val) {
	my_resource->evaluate_present_value(val);
}

void ResourceCurrentVal::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << theData << get_indices() << "." << get_method();
	(*aos) << "()";
}

apgen::DATA_TYPE ResourceCurrentVal::get_result_type() const {
	return my_resource->get_datatype();
}

ResourceVal::ResourceVal(
			const origin_info& o,
			const Cstring& resname,
			parsedExp& A)
		: ResourceMethod(o, resname), Arg(A) {
	
	//
	// This constructor is only invoked from consolidate()
	//

	//
	// First thing is to get the resource
	//
	RCsource* container = RCsource::resource_containers().find(resname);
	if(!container) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": applying value() to non-concrete resource "
			<< resname;
		throw(eval_error(errs));
	}

	//
	// We know there is only one resource in
	// array_elements. If the resource is an
	// array, look at ArrayedResourceVal,
	// below.
	//
	my_resource = container->payload->Object->array_elements[0];
}

ResourceVal::ResourceVal(const ResourceVal& rcv)
	: ResourceMethod(rcv),
		my_resource(rcv.my_resource) {
	Arg.reference(rcv.Arg->copy());
}

ResourceVal::ResourceVal(bool, const ResourceVal& rcv)
	: ResourceMethod(true, rcv),
		Arg(rcv.Arg),
		my_resource(rcv.my_resource) {
}

apgen::DATA_TYPE ResourceVal::get_result_type() const {
	return my_resource->get_datatype();
}

void ResourceVal::eval_expression(
		behaving_base*	obj,
		TypedValue&	val) {
	TypedValue timeval;
	Arg->eval_expression(obj, timeval);
	my_resource->evaluate_present_value(
		val,
		timeval.get_time_or_duration_ref());
}

void ResourceVal::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << theData << get_indices() << "." << get_method();
	(*aos) << "(";
	Arg->to_stream(aos, 0);
	(*aos) << ")";
}

ResourceInterpVal::ResourceInterpVal(
			const origin_info& o,
			const Cstring& resname)
		: ResourceMethod(o, resname) {
	
	//
	// This constructor is only invoked from consolidate()
	//

	//
	// First thing is to get the resource
	//

	RCsource* container = RCsource::resource_containers().find(resname);
	if(!container) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": applying interpval() to non-concrete resource "
			<< resname;
		throw(eval_error(errs));
	}
	my_resource = container->payload->Object->array_elements[0];
}

ResourceInterpVal::ResourceInterpVal(const ResourceInterpVal& rcv)
	: ResourceMethod(rcv),
		my_resource(rcv.my_resource) {
}

ResourceInterpVal::ResourceInterpVal(bool, const ResourceInterpVal& rcv)
	: ResourceMethod(true, rcv),
		my_resource(rcv.my_resource) {
}

apgen::DATA_TYPE ResourceInterpVal::get_result_type() const {
	return my_resource->get_datatype();
}

void ResourceInterpVal::eval_expression(
		behaving_base* obj,
		TypedValue& val) {
	EvalTiming::strategy_setter setter(
			my_resource, 
			EvalTiming::INTERPOLATED);

	// debug
	// if(my_resource->name == "SpacecraftGanymedeAltitude") {
	// 	cout << "got it!\n";
	// }

	my_resource->evaluate_present_value(val);
}

void ResourceInterpVal::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << theData << get_indices() << "." << get_method();
	(*aos) << "()";
}

ResourceHist::ResourceHist(
			const origin_info& o,
			const Cstring& resname,
			parsedExp& start_arg,
			parsedExp& end_arg)
		: ResourceMethod(o, resname) {
	Args[0] = start_arg;
	Args[1] = end_arg;
	
	//
	// This constructor is only invoked from consolidate()
	//

	//
	// First thing is to get the resource
	//

	RCsource* container = RCsource::resource_containers().find(resname);
	if(!container) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": applying history() to non-concrete resource "
			<< resname;
		throw(eval_error(errs));
	}
	my_resource = container->payload->Object->array_elements[0];
}

ResourceHist::ResourceHist(const ResourceHist& rcv)
	: ResourceMethod(rcv),
		my_resource(rcv.my_resource) {
	Args[0].reference(rcv.Args[0]->copy());
	Args[1].reference(rcv.Args[1]->copy());
}

ResourceHist::ResourceHist(bool, const ResourceHist& rcv)
	: ResourceMethod(true, rcv),
		my_resource(rcv.my_resource) {
	Args[0] = rcv.Args[0];
	Args[1] = rcv.Args[1];
}

apgen::DATA_TYPE ResourceHist::get_result_type() const {
	return apgen::DATA_TYPE::ARRAY;
}

void ResourceHist::eval_expression(
		behaving_base* obj,
		TypedValue& val) {
	eval_array(obj, val);
}

void ResourceHist::eval_array(
		behaving_base* obj,
		TypedValue& val) {
	TypedValue				start;
	TypedValue				end;
	tlist<	alpha_time, 
			unattached_value_node,
			Rsource*>		l;

	Args[0]->eval_expression(obj, start);
	Args[1]->eval_expression(obj, end);

	val = my_resource->get_partial_history(
			start.get_time_or_duration(),
			end.get_time_or_duration() - start.get_time_or_duration());
}

void ResourceHist::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << theData << get_indices() << "." << get_method();
	(*aos) << "()";
}

//
// Current value of a resource expressed as a method applied to a resource
// array element:
//
// 	c = Foo["some index"].currentval()
//
ArrayedResourceCurrentVal::ArrayedResourceCurrentVal(
			const origin_info& o,
			const Cstring& resname,
			smart_ptr<MultidimIndices>& multi
		) : ResourceMethod(o, resname),
			indices(multi) {
	assert(multi);
	my_container = RCsource::resource_containers().find(resname);
	if(!my_container) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": applying currentval to non-concrete resource "
			<< resname;
		throw(eval_error(errs));
	}
}

ArrayedResourceCurrentVal::ArrayedResourceCurrentVal(
			const ArrayedResourceCurrentVal& rcv)
	: ResourceMethod(rcv),
		my_container(rcv.my_container) {
	MultidimIndices* multi
		= dynamic_cast<MultidimIndices*>(
				rcv.indices->copy());
	indices.reference(multi);
}

ArrayedResourceCurrentVal::ArrayedResourceCurrentVal(
			bool,
			const ArrayedResourceCurrentVal& rcv)
	: ResourceMethod(true, rcv),
		my_container(rcv.my_container),
		indices(rcv.indices) {
}

apgen::DATA_TYPE ArrayedResourceCurrentVal::get_result_type() const {
	return my_container->get_datatype();
}

void ArrayedResourceCurrentVal::eval_expression(
			behaving_base*	loc,
			TypedValue&	val) {
	resContPLD&	pld = *my_container->payload;
	int I = indices->evaluate_flat_index_for(
			pld.Type,
			loc);
	if(I >= pld.Object->array_elements.size()) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": evaluating arrayed resource for index "
			<< I << " does not work.";
		throw(eval_error(errs));
	}

	pld.Object->array_elements[I]->evaluate_present_value(
			val);
}

void ArrayedResourceCurrentVal::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << theData << get_indices() << "." << get_method();
	(*aos) << "()";
}

ArrayedResourceVal::ArrayedResourceVal(
			const origin_info& o,
			const Cstring& resname,
			smart_ptr<MultidimIndices>& multi,
			parsedExp& A
		) : ResourceMethod(o, resname),
			indices(multi),
			Arg(A) {
	assert(multi);
	my_container = RCsource::resource_containers().find(resname);
	if(!my_container) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": applying value() to non-concrete resource "
			<< resname;
		throw(eval_error(errs));
	}
}

ArrayedResourceVal::ArrayedResourceVal(const ArrayedResourceVal& rcv)
	: ResourceMethod(rcv),
		my_container(rcv.my_container) {
	MultidimIndices* multi
		= dynamic_cast<MultidimIndices*>(rcv.indices->copy());
	Arg.reference(rcv.Arg->copy()),
	indices.reference(multi);
}

ArrayedResourceVal::ArrayedResourceVal(
			bool,
			const ArrayedResourceVal& rcv)
	: ResourceMethod(true, rcv),
		my_container(rcv.my_container),
		Arg(rcv.Arg),
		indices(rcv.indices) {
}

apgen::DATA_TYPE ArrayedResourceVal::get_result_type() const {
	return my_container->get_datatype();
}

void ArrayedResourceVal::eval_expression(
			behaving_base*	loc,
			TypedValue&	val) {
	resContPLD&	pld = *my_container->payload;
	int I = indices->evaluate_flat_index_for(
			pld.Type,
			loc);

	TypedValue timeval;
	Arg->eval_expression(loc, timeval);
	pld.Object->array_elements[I]->evaluate_present_value(
		val,
		timeval.get_time_or_duration());
}

void ArrayedResourceVal::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << theData << get_indices() << "." << get_method();
	(*aos) << "(";
	Arg->to_stream(aos, 0);
	(*aos) << ")";
}

ArrayedResourceInterpVal::ArrayedResourceInterpVal(
			const origin_info& o,
			const Cstring& resname,
			smart_ptr<MultidimIndices>& multi
		) : ResourceMethod(o, resname),
			indices(multi) {
	assert(multi);
	my_container = RCsource::resource_containers().find(resname);
	if(!my_container) {
		Cstring errs;
		errs << "File " << file << ", line " << line
		    << ": applying interpval() to non-concrete resource "
		    << resname;
		throw(eval_error(errs));
	}
}

ArrayedResourceInterpVal::ArrayedResourceInterpVal(
				const ArrayedResourceInterpVal& rcv)
	: ResourceMethod(rcv),
		my_container(rcv.my_container) {
	MultidimIndices* multi
		= dynamic_cast<MultidimIndices*>(rcv.indices->copy());
	indices.reference(multi);
}

ArrayedResourceInterpVal::ArrayedResourceInterpVal(
				bool,
				const ArrayedResourceInterpVal& rcv)
	: ResourceMethod(true, rcv),
		my_container(rcv.my_container),
		indices(rcv.indices) {
}

apgen::DATA_TYPE ArrayedResourceInterpVal::get_result_type() const {
	return my_container->get_datatype();
}

void ArrayedResourceInterpVal::eval_expression(
			behaving_base*	loc,
			TypedValue&	val) {
	resContPLD&	pld = *my_container->payload;
	int I = indices->evaluate_flat_index_for(
			pld.Type,
			loc);

	Rsource* res = pld.Object->array_elements[I];
	EvalTiming::strategy_setter setter(
			res, 
			EvalTiming::INTERPOLATED);
	res->evaluate_present_value(val);
}

void ArrayedResourceInterpVal::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << theData << get_indices() << "." << get_method();
	(*aos) << "()";
}

ArrayedResourceHist::ArrayedResourceHist(
			const origin_info& o,
			const Cstring& resname,
			smart_ptr<MultidimIndices>& multi,
			parsedExp& start_arg,
			parsedExp& end_arg)
		: ResourceMethod(o, resname),
			indices(multi) {
	assert(multi);
	Args[0] = start_arg;
	Args[1] = end_arg;
	
	//
	// This constructor is only invoked from consolidate()
	//

	//
	// First thing is to get the resource
	//

	my_container = RCsource::resource_containers().find(resname);
	if(!my_container) {
		Cstring errs;
		errs << "File " << file << ", line " << line
			<< ": applying history() to non-concrete resource "
			<< resname;
		throw(eval_error(errs));
	}
}

ArrayedResourceHist::ArrayedResourceHist(const ArrayedResourceHist& rcv)
	: ResourceMethod(rcv),
		my_container(rcv.my_container) {
	Args[0].reference(rcv.Args[0]->copy());
	Args[1].reference(rcv.Args[1]->copy());
	MultidimIndices* multi
		= dynamic_cast<MultidimIndices*>(rcv.indices->copy());
	indices.reference(multi);
}

ArrayedResourceHist::ArrayedResourceHist(bool, const ArrayedResourceHist& rcv)
	: ResourceMethod(true, rcv),
		my_container(rcv.my_container),
		indices(rcv.indices) {
	Args[0] = rcv.Args[0];
	Args[1] = rcv.Args[1];
}

apgen::DATA_TYPE ArrayedResourceHist::get_result_type() const {
	return apgen::DATA_TYPE::ARRAY;
}

void ArrayedResourceHist::eval_expression(
		behaving_base* obj,
		TypedValue& val) {
	eval_array(obj, val);
}

void ArrayedResourceHist::eval_array(
		behaving_base* obj,
		TypedValue& val) {

	TypedValue				start;
	TypedValue				end;
	tlist<	alpha_time, 
			unattached_value_node,
			Rsource*>		l;

	Args[0]->eval_expression(obj, start);
	Args[1]->eval_expression(obj, end);

	resContPLD&	pld = *my_container->payload;
	int I = indices->evaluate_flat_index_for(
			pld.Type,
			obj);

	Rsource* res = pld.Object->array_elements[I];

	val = res->get_partial_history(
			start.get_time_or_duration(),
			end.get_time_or_duration() - start.get_time_or_duration());
}

void ArrayedResourceHist::to_stream(aoString* aos, int ind) const {
	indent(aos, ind);
	(*aos) << theData << get_indices() << "." << get_method();
	(*aos) << "(";
	Args[0]->to_stream(aos, 0);
	(*aos) << ", ";
	Args[1]->to_stream(aos, 0);
	(*aos) << ")";
}
