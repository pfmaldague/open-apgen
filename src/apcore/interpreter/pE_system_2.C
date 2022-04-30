#if HAVE_CONFIG_H
#include <config.h>
#endif

extern "C" {
#include "concat_util.h"
} // extern "C"

// extern int abdebug;

#include <assert.h>

// #define apDEBUG
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "apDEBUG.H"
#include "ActivityInstance.H"
#include "AP_exp_eval.H"
#include "act_plan_parser.H"
#include "apcoreWaiter.H"
#include "ParsedExpressionSystem.H"
#include "RES_def.H"

#define RETRIEVE2( XX , YY , ZZ ) \
		if( args.size() > 2 ) \
				{ rmessage = Cstring( "too many args. in " ) + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		else if( args.size() < 2 ) \
				{ rmessage = Cstring( "too few args. in " ) + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		TypedValue	*YY = args.pop_front() , \
				*XX = args.pop_front() ;

#define RETRIEVE1( XX , ZZ ) \
		if( args.size() > 1 ) \
				{ rmessage = Cstring( "too many args. in " ) + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		else if( args.size() < 1 ) \
				{ rmessage = Cstring("too few args. in ") + ZZ ; \
				return apgen::RETURN_STATUS::FAIL ; } \
		TypedValue	*XX = args.pop_front() ;


TypedValue (*pEsys::process_an_AAF_request_handler)(const Cstring&, ListOVal&);

using namespace std;
using namespace pEsys;

TypedValue* MultidimIndices::apply_indices_to(
			TypedValue*		lhsval,
			behaving_base*	loc,
			bool			tolerant) {
	vector<TypedValue>	currentindexvals;
	TypedValue		currentindexval;
	ArrayElement*		ae;
	for(int i = 0; i < actual_indices.size(); i++) {
		if(lhsval->get_type() != apgen::DATA_TYPE::ARRAY) {
			Cstring err;
			err << "File " << file << ", line " << line
			    << ": array evaluation error";
			for(int z = 0; z < i; z++) {
			    err << " for index [" << currentindexvals[z].to_string() << "]";
			}
			if(!i) {
			    err << "; variable is not an array";
			}
			throw(eval_error(err));
		}
		actual_indices[i]->eval_expression(loc, currentindexval);
		currentindexvals.push_back(currentindexval);
		if(currentindexval.is_string()) {
			ae = lhsval->get_array()[currentindexval.get_string()];
			if(!ae) {
				TypedValue V;
				lhsval->get_array().add(currentindexval.get_string(), V, &ae);
			}
		} else {
			ae = lhsval->get_array()[currentindexval.get_int()];
			if(!ae) {
				TypedValue V;
				lhsval->get_array().add(currentindexval.get_int(), V, &ae);
			}
		}
		lhsval = &ae->Val();
	}
	return lhsval;
}

TypedValue* MultidimIntegers::apply_indices_to(
			TypedValue*	lhsval,
			behaving_base* loc,
			bool tolerant /* = false */ ) {
	long int		currentindexval;
	ArrayElement*		ae;
	for(int i = 0; i < actual_indices.size(); i++) {
		if(lhsval->get_type() != apgen::DATA_TYPE::ARRAY) {
			Cstring err;
			err << "File " << file << ", line " << line
				<< ": trying to get element of non-array type "
				<< apgen::spell(lhsval->get_type());
			throw(eval_error(err));
		}
		ListOVal&	lov = *lhsval->value.LI;
		actual_indices[i]->eval_int(loc, currentindexval);
		if(!(ae = lov[currentindexval])) {
			if(tolerant) {
				while(currentindexval >= lov.get_length()) {
					lov.add(ae = new ArrayElement(lov.get_length()));
				}
			} else {
				Cstring errs;
				errs << "File " << file << ", line " << line
					<< ": out-of-bound index "
					<< actual_indices[i]->to_string()
					<< "; array only has " << lov.get_length()
					<< " elements.";
				throw(eval_error(errs));
			}
		}
		lhsval = &ae->Val();
	}
	return lhsval;
}

void RaiseToPower::initExpression(
		const Cstring&	nodeData) {
	// nodeType = nt;
	theData = nodeData;
	binaryFunc = func_exponent;
}

void Parentheses::initExpression(
		const Cstring& nodeData) {
	// nodeType = nt;
	theData = nodeData;
	unaryFunc = func_PAREN;
}

apgen::RETURN_STATUS exp_divide(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_divide")
	try {
		Comparison::func_LESSTHAN(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void MultiplicativeExp::func_divide(const TypedValue& a, const TypedValue& b, TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					{
					double D = b.get_double();
					if(D == 0.0) {
						Cstring rmessage = "trying to divide by zero";
						throw(eval_error(rmessage));
					}
					res = a.get_double() / D;
					return;
					}
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in divide()";
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					{
					double D = b.get_double();
					if(D == 0.0) {
						Cstring rmessage = "trying to divide by zero";
						throw(eval_error(rmessage));
					}
					res = a.get_time_or_duration() / D;
					return;
					}
				case apgen::DATA_TYPE::DURATION:
					{
					CTime_base D = b.get_time_or_duration();
					if(D == CTime_base(0, 0, true)) {
						Cstring rmessage = "trying to divide by zero";
						throw(eval_error(rmessage));
					}
					res = a.get_time_or_duration() / D;
					return;
					}
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in divide()";
					throw(eval_error(err));
					}
			}
			break;
		default:
			{
			Cstring err;
			err << "incompatible args. " << a.to_string()
				<< " and " << b.to_string() << " in divide()";
			throw(eval_error(err));
			}
	}
}

apgen::RETURN_STATUS exp_LESSTHAN(
		Cstring &rmessage, 
		TypedValue *res, 
		slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_LESSTHAN")
	try {
		Comparison::func_LESSTHAN(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	Comparison::func_LESSTHAN(
		const TypedValue& a,
		const TypedValue& b,
		TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_double() < b.get_double();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in less-than()";
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() < b.get_time_or_duration();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in less-than()";
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::TIME:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::TIME:
					res = a.get_time_or_duration() < b.get_time_or_duration();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in less-than()";
					throw(eval_error(err));
					}
			}
			break;
		default:
			{
			Cstring err;
			err << "incompatible args. " << a.to_string() << " and "
				<< b.to_string() << " in less-than()";
			throw(eval_error(err));
			}
	}
}

apgen::RETURN_STATUS exp_LESSTHANOREQ(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_LESSTHANOREQ")
	try {
		Comparison::func_LESSTHANOREQ(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	Comparison::func_LESSTHANOREQ(
		const TypedValue& a,
		const TypedValue& b,
		TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_double() <= b.get_double();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in less-than-or-equal()";
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() <= b.get_time_or_duration();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in less-than-or-equal()";
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::TIME:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::TIME:
					res = a.get_time_or_duration() <= b.get_time_or_duration();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in less-than-or-equal()";
					throw(eval_error(err));
					}
			}
			break;
		default:
			{
			Cstring err;
			err << "incompatible args. " << a.to_string()
				<< " and " << b.to_string() << " in less-than-or-equal()";
			throw(eval_error(err));
			}
	}
}

apgen::RETURN_STATUS exp_equal(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_equal")
	try {
		EqualityTest::func_equal(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	EqualityTest::func_equal(
		const TypedValue& a,
		const TypedValue& b,
		TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_int() == b.get_int();
					break;
				default:
					{
					Cstring err = "equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::BOOL_TYPE:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::BOOL_TYPE:
					res = a.get_int() == b.get_int();
					break;
				default:
					{
					Cstring err = "equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::TIME:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::TIME:
					res = a.get_time_or_duration() == b.get_time_or_duration();
					break;
				default:
					{
					Cstring err = "equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() == b.get_time_or_duration();
					break;
				default:
					{
					Cstring err = "equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::STRING:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::STRING:
					res = a.get_string() == b.get_string();
					break;
				default:
					{
					Cstring err = "equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::INSTANCE:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::INSTANCE:
					res = (*a.get_object())["id"].get_string() == (*b.get_object())["id"].get_string();
					break;
				default:
					{
					Cstring err = "equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::ARRAY:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::ARRAY:
					res = a.is_equal_to(b);
					break;
				default:
					{
					Cstring err = "equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		default:
			{
			Cstring err;
			err << "equal: testing " << apgen::spell(a.get_type()) << " args. for equality is not supported; "
				"for floating-point values, please use inequalities for testing within some tolerance.";
			throw(eval_error(err));
			}
			break;
	}
}

apgen::RETURN_STATUS exp_GTRTHAN(Cstring& rmessage, TypedValue* res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_GTRTHAN")
	try {
		Comparison::func_GTRTHAN(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	Comparison::func_GTRTHAN(
		const TypedValue& a,
		const TypedValue& b,
		TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_double() > b.get_double();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in greater-than()";
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() > b.get_time_or_duration();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in greater-than()";
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::TIME:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::TIME:
					res = a.get_time_or_duration() > b.get_time_or_duration();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in greater-than()";
					throw(eval_error(err));
					}
			}
			break;
		default:
			{
			Cstring err;
			err << "incompatible args. " << a.to_string()
				<< " and " << b.to_string() << " in greater-than()";
			throw(eval_error(err));
			}
	}
}

apgen::RETURN_STATUS exp_GTRTHANOREQ(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_GTRTHANOREQ")
	try {
		Comparison::func_GTRTHANOREQ(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	Comparison::func_GTRTHANOREQ(
		const TypedValue& a,
		const TypedValue& b,
		TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::FLOATING:
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_double() >= b.get_double();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in greater-than-or-equal()";
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() >= b.get_time_or_duration();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string() << " and "
						<< b.to_string() << " in greater-than-or-equal()";
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::TIME:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::TIME:
					res = a.get_time_or_duration() >= b.get_time_or_duration();
					return;
				default:
					{
					Cstring err;
					err << "incompatible args. " << a.to_string()
						<< " and " << b.to_string() << " in greater-than-or-equal()";
					throw(eval_error(err));
					}
			}
			break;
		default:
			{
			Cstring err;
			err << "incompatible args. " << a.to_string()
				<< " and " << b.to_string() << " in greater-than-or-equal()";
			throw(eval_error(err));
			}
	}
}

void	Logical::func_LAND(
		parsedExp& a,
		parsedExp& b,
		behaving_base* o,
		TypedValue& res) {
	a->eval_expression(o, res);
	switch(res.get_type()) {
		case apgen::DATA_TYPE::BOOL_TYPE:
			break;
		default:
			Cstring err = "Logical AND: expression ";
			aoString aos;
			a->to_stream(&aos, 0);
			err << aos.str() << " evaluates to non-boolean value "
				<< apgen::spell(res.get_type());
			throw(eval_error(err));
	}
	if(!res.get_int()) {
		//
		// No need to evaluate second argument:
		//
		return;
	}
	b->eval_expression(o, res);
	switch(res.get_type()) {
		case apgen::DATA_TYPE::BOOL_TYPE:
			break;
		default:
			Cstring err = "Logical AND: expression ";
			aoString aos;
			b->to_stream(&aos, 0);
			err << aos.str() << " evaluates to non-boolean value "
				<< apgen::spell(res.get_type());
			throw(eval_error(err));
	}
	//
	// we already know that a is true - so we are done
	//
}

void	Logical::func_LOR(
		parsedExp& a,
		parsedExp& b,
		behaving_base* o,
		TypedValue& res) {
	a->eval_expression(o, res);
	switch(res.get_type()) {
		case apgen::DATA_TYPE::BOOL_TYPE:
			break;
		default:
			Cstring err = "Logical OR: expression ";
			aoString aos;
			a->to_stream(&aos, 0);
			err << aos.str() << " evaluates to non-boolean value "
				<< apgen::spell(res.get_type());
			throw(eval_error(err));
	}
	if(res.get_int()) {
		//
		// No need to evaluate second argument:
		//
		return;
	}
	b->eval_expression(o, res);
	switch(res.get_type()) {
		case apgen::DATA_TYPE::BOOL_TYPE:
			break;
		default:
			Cstring err = "Logical OR: expression ";
			aoString aos;
			b->to_stream(&aos, 0);
			err << aos.str() << " evaluates to non-boolean value "
				<< apgen::spell(res.get_type());
			throw(eval_error(err));
	}
	//
	// we already know that a is false - so we are done
	//
}

apgen::RETURN_STATUS exp_MINUS(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_MINUS")
	try {
		AdditiveExp::func_MINUS(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	AdditiveExp::func_MINUS(
		const TypedValue& a,
		const TypedValue& b,
		TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_int() - b.get_int();
					return;
				case apgen::DATA_TYPE::FLOATING:
					res = a.get_double() - b.get_double();
					return;
				default:
					{
					Cstring err = "trying to subtract non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::FLOATING:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_double() - b.get_double();
					return;
				default:
					{
					Cstring err = "trying to subtract non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::TIME:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::TIME:
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() - b.get_time_or_duration();
					return;
				default:
					{
					Cstring err = "trying to subtract non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() - b.get_time_or_duration();
					return;
				default:
					{
					Cstring err = "trying to subtract non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		default:
			{
			Cstring err = "trying to subtract non-comparable args ";
			err << apgen::spell(a.get_type()) << " and "
				<< apgen::spell(b.get_type());
			throw(eval_error(err));
			}
	}
}

apgen::RETURN_STATUS exp_MOD(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_MOD")
	try {
		MultiplicativeExp::func_MOD(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	MultiplicativeExp::func_MOD(const TypedValue& a, const TypedValue& b, TypedValue& res) {
	switch(a.get_type()) {
	    case apgen::DATA_TYPE::INTEGER:
		switch(b.get_type()) {
		    case apgen::DATA_TYPE::INTEGER:
			{
			long int c = b.get_int();
			if(!c) {
				Cstring err;
				err << "second argument in modulo expression is zero";
				throw(eval_error(err));
			}
			res = a.get_int() % c;
			return;
		    }
		    default:
			{
			Cstring err = "modulo expression contains non-integer quantities ";
			a.print(err, MAX_SIGNIFICANT_DIGITS);
			err << " and ";
			b.print(err, MAX_SIGNIFICANT_DIGITS);
			throw(eval_error(err));
		    }
		}
	    case apgen::DATA_TYPE::DURATION:
		switch(b.get_type()) {
		    case apgen::DATA_TYPE::DURATION:
			{
			long int c = b.get_time_or_duration().to_milliseconds();
			if(!c) {
				Cstring err;
				err << "second argument in modulo expression is zero";
				throw(eval_error(err));
			}
			long int d = (a.get_time_or_duration().to_milliseconds()) % c;
			res = CTime_base(d/1000, d % 1000, true);
			return;
		    }
	    	    default:
			{
			Cstring err = "modulo expression contains non-integer quantities ";
			a.print(err, MAX_SIGNIFICANT_DIGITS);
			err << " and ";
			b.print(err, MAX_SIGNIFICANT_DIGITS);
			throw(eval_error(err));
		    }
		}
	    default:
		{
		Cstring err = "modulo expression contains non-integer quantities ";
		a.print(err, MAX_SIGNIFICANT_DIGITS);
		err << " and ";
		b.print(err, MAX_SIGNIFICANT_DIGITS);
		throw(eval_error(err));
	    }
	}
}

apgen::RETURN_STATUS exp_MULT(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_MULT")
	try {
		MultiplicativeExp::func_MULT(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	MultiplicativeExp::func_MULT(const TypedValue& a, const TypedValue& b, TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_int() * b.get_int();
					return;
				case apgen::DATA_TYPE::FLOATING:
					res = a.get_double() * b.get_double();
					return;
				case apgen::DATA_TYPE::DURATION:
					res = a.get_double() * b.get_time_or_duration();
					return;
				default:
					{
					Cstring err = "trying to multiply non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::FLOATING:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_double() * b.get_double();
					return;
				case apgen::DATA_TYPE::DURATION:
					res = a.get_double() * b.get_time_or_duration();
					return;
				default:
					{
					Cstring err = "trying to multiply non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::INTEGER:
				case apgen::DATA_TYPE::FLOATING:
					res = a.get_time_or_duration() * b.get_double();
					return;
				default:
					{
					Cstring err = "trying to multiply non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		default:
			{
			Cstring err = "trying to multiply non-comparable args ";
			err << apgen::spell(a.get_type()) << " and "
				<< apgen::spell(b.get_type());
			throw(eval_error(err));
			}
	}
}

apgen::RETURN_STATUS exp_PLUS(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_PLUS")
	try {
		AdditiveExp::func_PLUS(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	AdditiveExp::func_PLUS(
		const TypedValue& a,
		const TypedValue& b,
		TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_int() + b.get_int();
					return;
				case apgen::DATA_TYPE::FLOATING:
					res = a.get_double() + b.get_double();
					return;
				default:
					{
					Cstring err = "trying to add non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::FLOATING:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::FLOATING:
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_double() + b.get_double();
					return;
				default:
					{
					Cstring err = "trying to add non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::TIME:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() + b.get_time_or_duration();
					return;
				default:
					{
					Cstring err = "trying to add non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::TIME:
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() + b.get_time_or_duration();
					return;
				default:
					{
					Cstring err = "trying to add non-comparable args ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::STRING:
			{
			if(b.is_string()) {
				Cstring theResult(a.get_string());

				theResult << b.get_string();
				res = theResult;
			} else {
				Cstring theResult(a.get_string());

				theResult << b.to_string();
				res = theResult;
			}
			return;
			}
		default:
			{
			Cstring err = "trying to add non-comparable args ";
			err << apgen::spell(a.get_type()) << " and "
				<< apgen::spell(b.get_type());
			throw(eval_error(err));
			}
	}
}

apgen::RETURN_STATUS exp_exponent(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_exponent")
	try {
		RaiseToPower::func_exponent(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void RaiseToPower::func_exponent(
			const TypedValue& a,
			const TypedValue& b,
			TypedValue& res) {
	if((!a.is_numeric()) || (!b.is_numeric())) {
		Cstring rmessage = "non-float arg. in exponent()";
		throw(eval_error(rmessage));
	}
	res = pow(a.get_double(), b.get_double());
}

apgen::RETURN_STATUS exp_NEG(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_PLUS")
	try {
		UnaryMinus::func_NEG(*a, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void UnaryMinus::initExpression(
		const Cstring&	nodeData) {
	// nodeType = nt;
	theData = nodeData;
	if(tok_minus->getData() == "-") {
		unaryFunc = func_NEG;
	} else {
		unaryFunc = NULL;
	}
}

void UnaryMinus::func_NEG(const TypedValue& a, TypedValue& res) {
	if(a.is_duration()) {
		res = - a.get_time_or_duration();
		res.cast();
	} else if(!a.is_numeric()) {
		Cstring rmessage = "func_NEG: trying to negate non-numeric quantity ";
		rmessage << a.to_string();
		throw(rmessage);
	}
	/* We used to always cast as a double; Steve Wissler was expecting an int,
	 * was surprised... Hopefully he'll like this better. */
	else if(a.is_int()) {
		res = - a.get_int();
		res.cast();
	} else {
		res = - a.get_double();
		res.cast();
	}
}


apgen::RETURN_STATUS exp_NOTEQUAL(Cstring &rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE2(a, b, "exp_NOTEQUAL")
	try {
		EqualityTest::func_NOTEQUAL(*a, *b, *res);
	}
	catch(eval_error Err) {
		rmessage = Err.msg;
		return apgen::RETURN_STATUS::FAIL;
	}
	return apgen::RETURN_STATUS::SUCCESS;
}

void	EqualityTest::func_NOTEQUAL(
		const TypedValue& a,
		const TypedValue& b,
		TypedValue& res) {
	switch(a.get_type()) {
		case apgen::DATA_TYPE::INTEGER:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::INTEGER:
					res = a.get_int() != b.get_int();
					break;
				default:
					{
					Cstring err;
					err << "Not equal: trying to compare non-comparable args. "
						<< apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::BOOL_TYPE:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::BOOL_TYPE:
					res = a.get_int() != b.get_int();
					break;
				default:
					{
					Cstring err = "Not equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::TIME:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::TIME:
					res = a.get_time_or_duration() != b.get_time_or_duration();
					break;
				default:
					{
					Cstring err = "Not equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::DURATION:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::DURATION:
					res = a.get_time_or_duration() != b.get_time_or_duration();
					break;
				default:
					{
					Cstring err = "Not equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::STRING:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::STRING:
					res = a.get_string() != b.get_string();
					break;
				default:
					{
					Cstring err = "Not equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::INSTANCE:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::INSTANCE:
					res = (*a.get_object())["id"].get_string() != (*b.get_object())["id"].get_string();
					break;
				default:
					{
					Cstring err = "Not equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		case apgen::DATA_TYPE::ARRAY:
			switch(b.get_type()) {
				case apgen::DATA_TYPE::ARRAY:
					res = !a.is_equal_to(b);
					break;
				default:
					{
					Cstring err = "Not equal: trying to compare non-comparable args. ";
					err << apgen::spell(a.get_type()) << " and "
						<< apgen::spell(b.get_type());
					throw(eval_error(err));
					}
			}
			break;
		default:
			{
			Cstring err;
			err << "Not equal: testing " << apgen::spell(a.get_type())
				<< " args. for equality is not supported; "
				"for floating-point values, please use inequalities for testing within some tolerance.";
			throw(eval_error(err));
			}
			break;
	}
}

apgen::RETURN_STATUS exp_PAREN(Cstring & rmessage, TypedValue *res, slst<TypedValue*>& args) {
	RETRIEVE1(a, "exp_PAREN")
	*res = *a;
	return apgen::RETURN_STATUS::SUCCESS;
}

void Parentheses::func_PAREN(const TypedValue& a, TypedValue& res) {
	res = a;
}

const char* pE::serial() const {
	static Cstring s;
	s.undefine();
	print (s);
	return *s;
}

// bool decomp_finder::inhibit(pE* exp) {
// 	return avoid == exp;
// }

void decomp_finder::pre_analyze(pE* exp) {
    switch(WhichObject) {
	case Activities:
	{
	    ActType* the_type = dynamic_cast<ActType*>(exp);
	    if(the_type) {
		ActTypeInitial* initial_section
				= dynamic_cast<ActTypeInitial*>(
					the_type->initial_act_type_section.object());
		assert(initial_section);
    		current_object = initial_section->activity_type_header->getData();
		we_are_inside_an_act_type = true;

		//
		// debug
		//
		cout << "Documenting act. " << current_object << "\n";
	    }
	}
	break;
	case Functions:
	{
	    FunctionDefinition* the_fun = dynamic_cast<FunctionDefinition*>(exp);
	    if(the_fun) {
		FunctionIdentity* fi = dynamic_cast<FunctionIdentity*>(the_fun->start_function_def.object());
		assert(fi);
		Cstring func_keyword = fi->tok_func->getData();
		bool is_a_script = func_keyword == "script";
		if(!is_a_script) {
			current_object = the_fun->getData();
			we_are_inside_a_function = true;

			//
			// debug
			//
			cout << "Documenting function " << current_object << "\n";
		}
	    }
	}
	break;
	case AbstractResources:
	{
	    Resource* res = dynamic_cast<Resource*>(exp);
	    if(res) {
		bool abstract = false;
		Cstring name;
		handle_a_resource(res, abstract, name);

		if(abstract) {
			current_object = name;
			we_are_inside_an_abs_res = true;

			//
			// debug
			//
			cout << "Documenting abs. res. " << current_object << "\n";
		}
	    }
	}
	break;
	case Resources:
	{
	    Resource* res = dynamic_cast<Resource*>(exp);
	    if(res) {
		bool abstract = false;
		Cstring name;
		handle_a_resource(res, abstract, name);
		if(!abstract) {
			current_object = name;
			we_are_inside_a_resource = true;

			//
			// debug
			//
			cout << "Documenting res. " << current_object << "\n";
		}
	    }
	}
	break;
	default:
	break;
    }
}

void decomp_finder::handle_a_resource(
		Resource*	res,
		bool&		is_abstract,
		Cstring&	name) {
	ResourceDef* rd = dynamic_cast<ResourceDef*>(res->resource_def.object());
	if(rd->abstract_resource_def_top_line) {
		is_abstract = true;
	} else if(rd->concrete_resource_def_top_line) {
		is_abstract = false;
	}
	ResourceInfo* ri = dynamic_cast<ResourceInfo*>(res->resource_prefix.object());
	assert(ri);
	name = ri->tok_id->getData();
}

void decomp_finder::post_analyze(pE* exp) {
    switch(WhichObject) {
	case Activities:
	{
	    ActType* the_type = dynamic_cast<ActType*>(exp);
	    if(the_type) {
		we_are_inside_an_act_type = false;
		return;
	    }
	}
	break;
	case Functions:
	{
	    FunctionDefinition* the_fun = dynamic_cast<FunctionDefinition*>(exp);
	    if(the_fun) {
		we_are_inside_a_function = false;
		return;
	    }
	}
	break;
	case AbstractResources:
	{
	    Resource* res = dynamic_cast<Resource*>(exp);
	    if(res) {
		we_are_inside_an_abs_res = false;
	    }
	}
	default:
	case Resources:
	{
	    Resource* res = dynamic_cast<Resource*>(exp);
	    if(res) {
		we_are_inside_a_resource = false;
	    }
	}
	break;
    }
    if(we_are_inside_an_act_type) {
	Decomp*			decomp = dynamic_cast<Decomp*>(exp);
	Usage*			usage = dynamic_cast<Usage*>(exp);
	ResourceMethod*		res_method = dynamic_cast<ResourceMethod*>(exp);
	Assignment*		assign = dynamic_cast<Assignment*>(exp);
	Symbol*			sym = dynamic_cast<Symbol*>(exp);
	QualifiedSymbol*	qual_sym = dynamic_cast<QualifiedSymbol*>(exp);
	FunctionCall*		fun_call = dynamic_cast<FunctionCall*>(exp);

	if(    decomp
	    || usage
	    || res_method
	    || assign
	    || sym
	    || qual_sym
	    || fun_call
	  ) {
	    map<Cstring, vector<set<Cstring> > >::iterator iter
				= properties.find(current_object);
	    if(iter == properties.end()) {

		//
		// IMPORTANT: update the size of the vector if you
		// add any items to the vec_type enum in the decomp_finder
		// class definition (in ParsedExpressionSystem.H)
		//
		properties[current_object] = vector<set<Cstring> >(8);
		iter = properties.find(current_object);
	    }
	    vector<set<Cstring> >& vec = iter->second;

	    if(decomp) {
		handle(vec, decomp);
	    } else if(usage) {
		handle(vec, usage);
	    } else if(res_method) {
		handle(vec, res_method);
	    } else if(assign) {
		handle(vec, assign);
	    } else if(sym) {
		handle(vec, sym);
	    } else if(qual_sym) {
		handle(vec, qual_sym);
	    } else if(fun_call) {
		handle(vec, fun_call);
	    }
	}
    } else if(we_are_inside_a_function) {
	ResourceMethod*		res_method = dynamic_cast<ResourceMethod*>(exp);
	Assignment*		assign = dynamic_cast<Assignment*>(exp);
	Symbol*			sym = dynamic_cast<Symbol*>(exp);
	QualifiedSymbol*	qual_sym = dynamic_cast<QualifiedSymbol*>(exp);
	FunctionCall*		fun_call = dynamic_cast<FunctionCall*>(exp);

	if( res_method
	    || assign
	    || sym
	    || qual_sym
	    || fun_call
	  ) {
	    map<Cstring, vector<set<Cstring> > >::iterator iter
				= properties.find(current_object);
	    if(iter == properties.end()) {

		//
		// IMPORTANT: update the size of the vector if you
		// add any items to the vec_type enum in the decomp_finder
		// class definition (in ParsedExpressionSystem.H)
		//
		properties[current_object] = vector<set<Cstring> >(8);
		iter = properties.find(current_object);
	    }
	    vector<set<Cstring> >& vec = iter->second;

	    if(res_method) {
		handle(vec, res_method);
	    } else if(assign) {
		handle(vec, assign);
	    } else if(sym) {
		handle(vec, sym);
	    } else if(qual_sym) {
		handle(vec, qual_sym);
	    } else if(fun_call) {
		handle(vec, fun_call);
	    }
	}
    } else if(we_are_inside_an_abs_res) {
	Usage*			usage = dynamic_cast<Usage*>(exp);
	ResourceMethod*		res_method = dynamic_cast<ResourceMethod*>(exp);
	Assignment*		assign = dynamic_cast<Assignment*>(exp);
	Symbol*			sym = dynamic_cast<Symbol*>(exp);
	QualifiedSymbol*	qual_sym = dynamic_cast<QualifiedSymbol*>(exp);
	FunctionCall*		fun_call = dynamic_cast<FunctionCall*>(exp);

	if( usage
	    || res_method
	    || assign
	    || sym
	    || qual_sym
	    || fun_call
	  ) {
	    map<Cstring, vector<set<Cstring> > >::iterator iter
				= properties.find(current_object);
	    if(iter == properties.end()) {

		//
		// IMPORTANT: update the size of the vector if you
		// add any items to the vec_type enum in the decomp_finder
		// class definition (in ParsedExpressionSystem.H)
		//
		properties[current_object] = vector<set<Cstring> >(8);
		iter = properties.find(current_object);
	    }
	    vector<set<Cstring> >& vec = iter->second;

	    if(usage) {
		handle(vec, usage);
	    } else if(res_method) {
		handle(vec, res_method);
	    } else if(assign) {
		handle(vec, assign);
	    } else if(sym) {
		handle(vec, sym);
	    } else if(qual_sym) {
		handle(vec, qual_sym);
	    } else if(fun_call) {
		handle(vec, fun_call);
	    }
	}
    } else if(we_are_inside_a_resource) {
	ResourceMethod*		res_method = dynamic_cast<ResourceMethod*>(exp);
	Symbol*			sym = dynamic_cast<Symbol*>(exp);
	QualifiedSymbol*	qual_sym = dynamic_cast<QualifiedSymbol*>(exp);
	FunctionCall*		fun_call = dynamic_cast<FunctionCall*>(exp);

	if( res_method
	    || sym
	    || qual_sym
	    || fun_call
	  ) {
	    map<Cstring, vector<set<Cstring> > >::iterator iter
				= properties.find(current_object);
	    if(iter == properties.end()) {

		//
		// IMPORTANT: update the size of the vector if you
		// add any items to the vec_type enum in the decomp_finder
		// class definition (in ParsedExpressionSystem.H)
		//
		properties[current_object] = vector<set<Cstring> >(8);
		iter = properties.find(current_object);
	    }
	    vector<set<Cstring> >& vec = iter->second;

	    if(res_method) {
		handle(vec, res_method);
	    } else if(sym) {
		handle(vec, sym);
	    } else if(qual_sym) {
		handle(vec, qual_sym);
	    } else if(fun_call) {
		handle(vec, fun_call);
	    }
	}
    }
}

void decomp_finder::handle(
		vector<set<Cstring> >& vec,
		Decomp* decomp) {

	//
	// This is appropriate for Decomp statements that have
	// been consolidated:
	//
	Cstring child_type;
	if(decomp->call_or_spawn_arguments) {
		parsedExp& childTypeExp = decomp->ActualArguments[0];
		try {
		    TypedValue child_val;
		    childTypeExp->eval_expression(
				behaving_object::GlobalObject(), child_val);
		    if(!child_val.is_string()) {
			Cstring errs;
			errs << "File " << decomp->file << ", line " << decomp->line
				<< ": first argument of call/spawn is not a string";
			throw(eval_error(errs));
		    }
		    child_type = child_val.get_string();
		} catch(eval_error Err) {
		    child_type = "cannot evaluate";
		}
	} else {
		assert(decomp->Type);
		child_type = decomp->Type->name;
	}

	//
	// Won't mind if child_type already exists (could
	// examine returned pair<iterator, bool> to find out)
	//
	vec[CHILDREN].insert(child_type);
}

void decomp_finder::handle(
		vector<set<Cstring> >& vec,
		Usage* usage) {
	if(usage->theContainer) {
	    vec[RESOURCES_SET].insert(usage->theContainer->get_key());
	} else if(usage->abs_type) {
	    vec[ABS_RESOURCES_SET].insert(usage->abs_type->name);
	}
}

void decomp_finder::handle(
		vector<set<Cstring> >& vec,
		ResourceMethod* res_method) {
	ResourceCurrentVal*		curval
		    = dynamic_cast<ResourceCurrentVal*>(res_method);

	ResourceVal*		val
		    = dynamic_cast<ResourceVal*>(res_method);

	ResourceInterpVal*		interpval
		    = dynamic_cast<ResourceInterpVal*>(res_method);

	ArrayedResourceCurrentVal*	a_curval
		    = dynamic_cast<ArrayedResourceCurrentVal*>(res_method);

	ArrayedResourceVal*		a_val
		    = dynamic_cast<ArrayedResourceVal*>(res_method);

	ArrayedResourceInterpVal* a_interpval
		    = dynamic_cast<ArrayedResourceInterpVal*>(res_method);

	if(curval) {

	    	vec[RESOURCES_READ].insert(curval->my_resource->name);
	} else if(val) {

	    	vec[RESOURCES_READ].insert(val->my_resource->name);
	} else if(interpval) {

	    	vec[RESOURCES_READ].insert(interpval->my_resource->name);
	} else if(a_curval) {

	    	vec[RESOURCES_READ].insert(a_curval->my_container->get_key());
	} else if(a_val) {

	    	vec[RESOURCES_READ].insert(a_val->my_container->get_key());
	} else if(a_interpval) {

	    	vec[RESOURCES_READ].insert(a_interpval->my_container->get_key());
	}
}

void decomp_finder::handle(
		vector<set<Cstring> >& vec,
		Assignment* assign) {
	Cstring	name;
	bool	is_global = false;

	//
	// assign->lhs_of_assign is either a Symbol
	// or a QualifiedSymbol; see consolidate.C,
	// function consolidate_assignment()
	//
	Symbol* lhs = dynamic_cast<Symbol*>(
			    assign->lhs_of_assign.object());
	QualifiedSymbol* qual_lhs
		    = dynamic_cast<QualifiedSymbol*>(
			    assign->lhs_of_assign.object());
	if(lhs) {
		name = lhs->getData();

		//
		// is it a global?
		//
		is_global = lhs->my_level == 0;
	} else if(qual_lhs) {
		name = qual_lhs->symbol->getData();

		//
		// is it a global?
		//
		is_global = qual_lhs->my_level == 0;
	}
	if(is_global) {

		vec[GLOBALS_SET].insert(name);
	}
}

void decomp_finder::handle(
		vector<set<Cstring> >& vec,
		Symbol* sym) {
	Cstring name = sym->getData();
	if(sym->my_level == 0) {

	    vec[GLOBALS_READ].insert(name);
	}
}

void decomp_finder::handle(
		vector<set<Cstring> >& vec,
		QualifiedSymbol* qual_sym) {
	Cstring name = qual_sym->symbol->getData();
	if(qual_sym->my_level == 0) {
	    vec[GLOBALS_READ].insert(name);
	}
}

void decomp_finder::handle(
		vector<set<Cstring> >& vec,
		FunctionCall* fun_call) {
	if(fun_call->func) {
	    vec[UDEF_FUNCTIONS_CALLED].insert(fun_call->getData());
	} else {
	    vec[AAF_FUNCTIONS_CALLED].insert(fun_call->getData());
	}
}
