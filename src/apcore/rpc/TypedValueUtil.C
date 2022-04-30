#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* TypedValueUtil.C - C++ utilities for processing APGen TypedValue
 * objects on the server side of the RPC interface */

#include <assert.h>

// contains code that will automatically include the extern "C" qualifier:
#include <apvalue.h>

extern "C" {
#include <apvalue_intfc.h>
// #include <typed_value.h>
}

#include <string.h>
#include <stdio.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <APdata.H>	// for TypedValue

/* returns the total number of aparrayelements needed to model the TypedValue */
int	get_n_el(const TypedValue* val) {
	int		count = 1;
	ListOVal*	lov = NULL;
	ArrayElement*	ae = NULL;
	switch(val->get_type()) {
		case apgen::INTEGER:
		case apgen::FLOATING:
		case apgen::BOOL_TYPE:
		case apgen::TIME:
		case apgen::DURATION:
		case apgen::STRING:
		case apgen::INSTANCE:
			break;
		case apgen::ARRAY:
			{
			ListOVal&	lov(val->get_array());
			count++;
			ae = lov.first_node();
			while(ae) {
				count += get_n_el(&ae->Val());
				ae = ae->next_node(); } }
			break;
		default:
			break; }
	return count; }

// below:
extern void TypedValue_to_aparrayelement(aparrayelement** arrayptr, TypedValue* tv);

extern "C" {

void destroy_TypedValue(void* av) {
	delete (TypedValue*) av; }

apvalue*	TypedValue_to_apvalue(void* val_as_void_ptr) {
	TypedValue*	val = (TypedValue*) val_as_void_ptr;
	apvalue*	av = (apvalue*) malloc(sizeof(apvalue));
	behaving_element act;

	switch(val->get_type()) {
		case apgen::INTEGER:
			av->type = AP_INT;
			av->apvalue_u.asInt = val->get_int();
			break;
		case apgen::FLOATING:
			av->type = AP_FLOAT;
			av->apvalue_u.asFloat = val->get_double();
			break;
		case apgen::BOOL_TYPE:
			av->type = AP_BOOL;
			av->apvalue_u.asBool = val->get_bool();
			break;
		case apgen::TIME:
			av->type = AP_TIME;
			av->apvalue_u.asTime = val->get_time_or_duration().get_seconds() * 1000
					+ val->get_time_or_duration().get_milliseconds();
			break;
		case apgen::DURATION:
			av->type = AP_DURATION;
			av->apvalue_u.asDuration = val->get_time_or_duration().get_seconds() * 1000
					+ val->get_time_or_duration().get_milliseconds();
			break;
		case apgen::STRING:
			av->type = AP_STRING;
			av->apvalue_u.asString = strdup(*val->get_string());
			break;
		case apgen::INSTANCE:
			av->type = AP_INSTANCE;
			act = val->get_object();
			av->apvalue_u.asInstance = strdup(*(*act)["id"].get_string());
			break;
		case apgen::ARRAY:
			// only makes sense for empty apgen arrays
			assert(val->get_array().get_array_type() == 0);
			// by convention we use AP_ARRAY
			av->type = AP_ARRAY;
			// avoid uninitialized memory
			av->apvalue_u.asInt = 0;
			break;
		case apgen::UNINITIALIZED:
		default:
			av->type = AP_UNDEFINED;
			av->apvalue_u.asInt = 0;
			break; }
	return av; }

// returns a pointer to a TypedValue object that conforms to the supplied apvalue
void* apvalue_to_TypedValue(apvalue* av) {
	// need to create a TypedValue based on RPC apvalue
	TypedValue*	val = NULL;
	ListOVal*	lov = NULL;
	switch(av->type) {
		case AP_INT:
			val = new TypedValue(apgen::INTEGER);
			*val = av->apvalue_u.asInt;
			break;
		case AP_FLOAT:
			val = new TypedValue(apgen::FLOATING);
			*val = av->apvalue_u.asFloat;
			break;
		case AP_BOOL:
			val = new TypedValue(apgen::BOOL_TYPE);
			*val = (bool) av->apvalue_u.asBool;
			break;
		case AP_TIME:
			val = new TypedValue(apgen::TIME);
			*val = CTime_base(
					av->apvalue_u.asTime / 1000,
					av->apvalue_u.asTime % 1000,
					false);
			break;
		case AP_DURATION:
			val = new TypedValue(apgen::DURATION);
			*val = CTime_base(
					av->apvalue_u.asTime / 1000,
					av->apvalue_u.asTime % 1000,
					true);
			break;
		case AP_STRING:
			val = new TypedValue(apgen::STRING);
			*val = av->apvalue_u.asString;
			break;
		case AP_INSTANCE:
			// not obvious; is an instance supposed to be available?
			val = new TypedValue;
			break;
		case AP_ARRAY:
			val = new TypedValue(apgen::ARRAY);
			lov = new ListOVal;
			*val = *lov;
			break;
		case AP_STRUCT:
			// should never happen
			break;
		case AP_UNDEFINED:
		default:
			val = new TypedValue;
			break; }
	return (void*) val; }

// C wrapper for the TypedValue::print() method
char* TypedValue_to_string(void* void_obj) {
	TypedValue*	obj = (TypedValue*) void_obj;
	Cstring		s;
	obj->print(s);
	char* t = strdup(*s);
	return t; }


	/* There are only 2 possibilities:
	 * 	- the given TypedValue is an apgen array
	 *	- the given TypedValue is a scalar
	 * Here we assume that the given TypedValue is an apgen array.
	 * Note that the function is not recursive. What needs to be
	 * recursive, however, is the method that creates aparrayelements
	 * within the aparray. */

aparray*	TypedValue_to_aparray(
			void*	typed_val_as_void_ptr,
			int	parent,			/* default -1 */
			int	start_id		/* default -1 */
			) {
	aparray*	aa = (aparray*) malloc(sizeof(aparray));
	TypedValue*	tv = (TypedValue*) typed_val_as_void_ptr;
	int		number_of_elements = get_n_el(tv);
	aparrayelement*	element;

	aa->apvector.apvector_len = number_of_elements;
	aa->apvector.apvector_val = (aparrayelement*) malloc(
			sizeof(aparrayelement) * number_of_elements
			);
	element = aa->apvector.apvector_val;
	element->indx.tag = AP_NONE;
	element->apid = 0;
	element->parent = -1;
	TypedValue_to_aparrayelement(&element, tv);
	return aa; }

void*	aparray_to_TypedValue(aparray* aa) {
	ListOVal*	lov;
	/* n is assumed > 0; even an empty array leads to n = 1 */
	int		n = aa->apvector.apvector_len;
	int		i;
	TypedValue**	vals = (TypedValue**) malloc(sizeof(TypedValue*) * n);
	TypedValue*	val;
	ArrayElement*	ae;
	Cstring		Err;

	/* we need to build an array of TypedValues indexed by
	 * the apid in each aparrayelement */
	for(i = 0; i < n; i++) {
		aparrayelement*		element = &aa->apvector.apvector_val[i];
		apvalue&		theContent(element->content);
		apindex&		theIndex(element->indx);
		apgen::DATA_TYPE	theType = apgen::UNINITIALIZED;
		TypedValue		V;

		lov = NULL;
		switch(theContent.type) {
			case AP_INT:
				V = theContent.apvalue_u.asInt;
				theType =apgen::INTEGER;
				break;
			case AP_FLOAT:
				V = theContent.apvalue_u.asFloat;
				theType = apgen::FLOATING;
				break;
			case AP_BOOL:
				V = (bool) theContent.apvalue_u.asBool;
				theType = apgen::BOOL_TYPE;
				break;
			case AP_TIME:
				V = CTime_base(
					theContent.apvalue_u.asTime / 1000,
					theContent.apvalue_u.asTime % 1000,
					false);
				theType = apgen::TIME;
				break;
			case AP_DURATION:
				V = CTime_base(
					theContent.apvalue_u.asDuration / 1000,
					theContent.apvalue_u.asDuration % 1000,
					true);
				theType = apgen::DURATION;
				break;
			case AP_STRING:
				V = theContent.apvalue_u.asString;
				theType = apgen::STRING;
				break;
			case AP_INSTANCE:
				/* this is not quite right but the correct version
				 * requires solving philosophical problems */
				V = theContent.apvalue_u.asInstance;
				theType = apgen::INSTANCE;
				break;
			case AP_ARRAY:
			case AP_STRUCT:
				lov = new ListOVal;
				/* Note: cannot yet determine the type of array */
				V = *lov;
				theType = apgen::ARRAY;
				break;
			default:
				break;
			}
		vals[element->apid] = new TypedValue(theType);
		*vals[element->apid] = V; }
	// now we connect the arrays and structs to their children
	for(i = 0; i < n; i++) {
		aparrayelement*		element = &aa->apvector.apvector_val[i];
		apvalue&		theContent(element->content);
		apindex&		theIndex(element->indx);
		char*			label = NULL;
		int			el_count = 0;

		if(theIndex.tag == AP_ARRAY_STYLE) {
			if(!(element->parent >= 0 && element->parent < n)) {
				delete vals[0];
				vals[0] = new TypedValue(apgen::STRING);
				(*vals[0]) = "Error: parent index out of bounds";
				break; }
			if(!vals[element->parent]->is_array()) {
				delete vals[0];
				vals[0] = new TypedValue(apgen::STRING);
				(*vals[0]) = "Error: array element has non-array parent";
				break; }
			lov = &vals[element->parent]->get_array();
			el_count = theIndex.apindex_u.x;
			ae = new ArrayElement(el_count);
			// TypedSymbol::set_typed_symbol(ae, *vals[i], Err);
			ae->SetVal(*vals[i]);
			// can throw in principle:
			ae->get_ptr_to_val()->cast();
			(*lov) << ae; }
		else if(theIndex.tag == AP_STRUCT_STYLE) {
			if(!(element->parent >= 0 && element->parent < n)) {
				delete vals[0];
				vals[0] = new TypedValue(apgen::STRING);
				(*vals[0]) = "Error: parent index out of bounds";
				break; }
			if(!vals[element->parent]->is_array()) {
				delete vals[0];
				vals[0] = new TypedValue(apgen::STRING);
				(*vals[0]) = "Error: array element has non-array parent";
				break; }
			lov = &vals[element->parent]->get_array();
			label = theIndex.apindex_u.s;
			ae = new ArrayElement(label, vals[i]->get_type());
			// TypedSymbol::set_typed_symbol(ae, *vals[i], Err);
			ae->SetVal(*vals[i]);
			// can throw in principle:
			ae->get_ptr_to_val()->cast();
			(*lov) << ae; }
		else {
			// the only indx with a tag of AP_NONE is the root
			if(i != 0) {
				delete vals[0];
				vals[0] = new TypedValue(apgen::STRING);
				(*vals[0]) = "Error: array element has AP_NONE index";
				break; } } }
	val = vals[0];
	// we keep val = vals[0], which we return; everything else can be deleted
	for(i = 1; i < n; i++) {
		delete vals[i]; }
	free((char*) vals);
	return val; }

} // extern "C"

/* Context: *arrayptr points to the current element, whose content needs to
 * be filled out. The indx, apid and parent members of the current element
 * have already been set to the correct values.
 *
 * When the function returns, *arrayptr points to the last element whose
 * content was filled out.
 */
void TypedValue_to_aparrayelement(aparrayelement** arrayptr, TypedValue* tv) {
  aparrayelement*	element = *arrayptr;
  behaving_element	act;
    switch(tv->get_type()) {
	case apgen::INTEGER:
		element->content.type = AP_INT;
		element->content.apvalue_u.asInt = tv->get_int();
		break;
	case apgen::FLOATING:
		element->content.type = AP_FLOAT;
		element->content.apvalue_u.asFloat = tv->get_double();
		break;
	case apgen::BOOL_TYPE:
		element->content.type = AP_BOOL;
		element->content.apvalue_u.asBool = tv->get_bool();
		break;
	case apgen::TIME:
		element->content.type = AP_TIME;
		element->content.apvalue_u.asTime = tv->get_time_or_duration().get_seconds() * 1000
						+ tv->get_time_or_duration().get_milliseconds();
		break;
	case apgen::DURATION:
		element->content.type = AP_DURATION;
		element->content.apvalue_u.asDuration =
				tv->get_time_or_duration().get_seconds() * 1000
				+ tv->get_time_or_duration().get_milliseconds();
		break;
	case apgen::STRING:
		element->content.type = AP_STRING;
		element->content.apvalue_u.asString = strdup(*tv->get_string());
		break;
	case apgen::INSTANCE:
		/* Instance values bring in some philosophical issues - within an
		 * APGen server, there is no question that instances have a meaning,
		 * but in order to exchange instance variables between servers, we
		 * need to know how to access the referenced instance... we need
		 * a set of reasonable assumptions. */
		element->content.type = AP_INSTANCE;
		act = tv->get_object();
		element->content.apvalue_u.asInstance = strdup(*(*act)["id"].get_string());
		break;
	case apgen::ARRAY:
		{
		ListOVal&		lov(tv->get_array());
		InternalKey::arrayType	array_type = lov.get_array_type();
		int			n = lov.get_length();
		int			parent_id = element->apid;
		int			new_id = element->apid + 1;
		ArrayElement*		ae;
		int			k = 0;

		if(array_type == InternalKey::STRUCT_STYLE) {
			element->content.type = AP_STRUCT; }
		else {
			/* NOTE: this includes the case of an empty array.
			 * Be careful when unmarshalling. */
			element->content.type = AP_ARRAY; }
		/* avoid uninitialized memory */
		element->content.apvalue_u.asInt = 0;
		ae = lov.first_node();
		while(ae) {
			(*arrayptr)++;
			element = *arrayptr;
			if(array_type == InternalKey::STRUCT_STYLE) {
				element->indx.tag = AP_STRUCT_STYLE;
				element->indx.apindex_u.s = strdup(*ae->get_key()); }
			else {
				element->indx.tag = AP_ARRAY_STYLE;
				element->indx.apindex_u.x = k++; }
			element->apid = new_id++;
			element->parent = parent_id;
			TypedValue_to_aparrayelement(&element, ae->get_ptr_to_val());
			ae = ae->next_node(); }
		}
		break;
	default:
		// "should never happen"
		assert(false);
		break; } }

