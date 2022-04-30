#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <APdata.H>
#include <xmlrpc_intfc.H>
#include <ActivityInstance.H>

#include <sstream>

//
// utilities for converting from XmlRpcValue to apgen data types
//

//
// v is known to be an array
//
void
fill_struct_API(
		TypedValue &v,
		XmlRpc::XmlRpcValue &x) {
	XmlRpc::XmlRpcValue::ValueStruct::iterator	item;
	XmlRpc::XmlRpcValue::ValueStruct		theStruct = (XmlRpc::XmlRpcValue::ValueStruct) x;
	TypedValue					theValue;

	for(	item = theStruct.begin();
		item != theStruct.end();
		item++) {
		// name, value pair
		Cstring theName(item->first.c_str());
		xml2typedval_API(item->second, theValue);
		try{
			v.get_array().add(theName, theValue);
		} catch(eval_error Err) {
			std::cerr << "Error in fill_struct_API:\n" << Err.msg << std::endl;
		}
	}
}

//
// v is known to be an array
//
void fill_list_API(
		TypedValue &v,
		XmlRpc::XmlRpcValue &x) {
	int i;

	for(i = 0; i < x.size(); i++) {
		TypedValue	val;
		xml2typedval_API(x[i], val);
		try {
			v.get_array().add((long)i, val);
		} catch(eval_error Err) {
			std::cerr << "Error in fill_list_API:\n" << Err.msg << std::endl;
		}
	}
}

void
fill_time_API(
		TypedValue &v,
		XmlRpc::XmlRpcValue &x) {
	struct tm		TM((struct tm) x);
	time_t			int_time = XmlRpc::timegm(&TM);

	v = CTime_base(int_time, 0, false);
}

XmlRpc::XmlRpcValue
typedval2xml_API(
		const TypedValue &theVal) {
	XmlRpc::XmlRpcValue	x;
	extern XmlRpc::XmlRpcValue xmlTime(long seconds);

	switch(theVal.get_type()) {
		case apgen::DATA_TYPE::ARRAY:
			if(theVal.get_array().get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
				ArrayElement*					n;

				for(int k = 0; k < theVal.get_array().get_length(); k++) {
					n = theVal.get_array()[k];
					x[*n->get_key()] = typedval2xml_API(n->Val());
				}
				if(!x.valid()) {
					throw(XmlRpc::XmlRpcException("empty structure", 100));
				}
			} else {
				ArrayElement*					n;

				for(int k = 0; k < theVal.get_array().get_length(); k++) {
					n = theVal.get_array()[k];
					x[k] = typedval2xml_API(n->Val());
				}
				if(!x.valid()) {
					throw(XmlRpc::XmlRpcException("empty array", 100));
				}
			}
			break;
		case apgen::DATA_TYPE::BOOL_TYPE:
			x = (long) theVal.get_bool();
			break;
		case apgen::DATA_TYPE::DURATION:
			x = theVal.get_time_or_duration().convert_to_double_use_with_caution();
			break;
		case apgen::DATA_TYPE::FLOATING:
			x = theVal.get_double();
			break;
		case apgen::DATA_TYPE::INTEGER:
			x = theVal.get_int();
			break;
		case apgen::DATA_TYPE::STRING:
			x = *theVal.get_string();
			break;
		case apgen::DATA_TYPE::TIME:
			// x = xmlTime(theVal.get_time_or_duration().get_seconds());
			x[0] = theVal.get_time_or_duration().get_seconds();
			x[1] = theVal.get_time_or_duration().get_milliseconds();
			break;
		case apgen::DATA_TYPE::INSTANCE:
			// UUUUUU need to handle abstract resource instances here
			x = *(*theVal.get_object())["id"].get_string();
			break;
		case apgen::DATA_TYPE::UNINITIALIZED:
			break;
		default:
			break;
	}
	return x;
}

void
xml2typedval_API(
		XmlRpc::XmlRpcValue&	x,
		TypedValue&		retval) {
	TypedValue*	theVal;
	ListOVal*	lov;

	switch(x.getType()) {
		case XmlRpc::XmlRpcValue::TypeStruct:
			theVal = new TypedValue(apgen::DATA_TYPE::ARRAY);
			lov = new ListOVal;
			*theVal = *lov;
			fill_struct_API(*theVal, x);
			break;
		case XmlRpc::XmlRpcValue::TypeBoolean:
			theVal = new TypedValue(apgen::DATA_TYPE::BOOL_TYPE);
			*theVal = (bool) x;
			break;
		case XmlRpc::XmlRpcValue::TypeInt:
			theVal = new TypedValue(apgen::DATA_TYPE::INTEGER);
			*theVal = (long) x;
			break;
		case XmlRpc::XmlRpcValue::TypeDouble:
			theVal = new TypedValue(apgen::DATA_TYPE::FLOATING);
			*theVal = (double) x;
			break;
		case XmlRpc::XmlRpcValue::TypeString:
			theVal = new TypedValue(apgen::DATA_TYPE::STRING);
			*theVal = (std::string) x;
			break;
		case XmlRpc::XmlRpcValue::TypeDateTime:
			theVal = new TypedValue(apgen::DATA_TYPE::TIME);
			fill_time_API(*theVal, x);
			break;
		case XmlRpc::XmlRpcValue::TypeBase64:
			theVal = new TypedValue;
			// don't do anything here... unused
			break;
		case XmlRpc::XmlRpcValue::TypeArray:
			theVal = new TypedValue(apgen::DATA_TYPE::ARRAY);
			lov = new ListOVal;
			*theVal = *lov;
			fill_list_API(*theVal, x);
			break;
		case XmlRpc::XmlRpcValue::TypeInvalid:
			theVal = new TypedValue;
			break;
	}
	retval = *theVal;
	delete theVal;
}

#ifdef OBSOLETE

// normal return is 0
int
getActivityParametersAPI(
		const string		&act_type,
		XmlRpc::XmlRpcValue	&parVec,
		// XmlRpc::XmlRpcValue	&attVec,
		string			&errors) {
	return 0;
}

extern "C" {
void getActInstanceAsXMLstring(	void *act_ptr,
				buf_struct *result) {
	ActivityInstance *req = (ActivityInstance *) act_ptr;
	XmlRpc::XmlRpcValue V, encodedVal;
	apgen::act_visibility_state Vis;
	bool has_parent;
	XmlRpc::XmlRpcValue::BinaryData	encoded;

	getOneInstanceAsXMLAPI(req, V, Vis, has_parent, false, /* Compress = */ true);
	initialize_buf_struct(result);
	compressAString(V.toXml(), encoded);
	encodedVal = encoded;
	concatenate(result, encodedVal.toXml().c_str()); }

void getAllActAttrParamDefsAsXMLstring(buf_struct *result) {
	string				any_errors;
	XmlRpc::XmlRpcValue		returnedStruct;
	XmlRpc::XmlRpcValue::BinaryData	encoded;
	XmlRpc::XmlRpcValue		encodedVal;

	initialize_buf_struct(result);
	getAllActAttrParamDefsAPI(returnedStruct, any_errors);
	compressAString(returnedStruct.toXml(), encoded);
	encodedVal = encoded;
	concatenate(result, encodedVal.toXml().c_str()); }
	
} // extern "C"

int getAllActAttrParamDefsAPI(XmlRpc::XmlRpcValue &returnedStruct, std::string &errors) {
	return 0;
}

void getOneInstanceAsXMLAPI(
		ActivityInstance *req,
		XmlRpc::XmlRpcValue &result,
		apgen::act_visibility_state &Vis,
		bool &has_parent,
		bool NoCommands,
		bool Compress) {
	Cstring			Id, Type, Name;
	CTime_base		Start;
	tvslist Atts_off;
	tvslist Atts_nick;
	tvslist Params;
	stringslist Children;
	stringslist Parent;
	tvslist::iterator atts(Atts_off), params(Params);
	TaggedValue* tds;
	XmlRpc::XmlRpcValue	V, A, P, C, vectime;
	TypedValue		StartValue;
	bool			anyparameters = false;
	String_node*		N;

	req->export_symbols(Id, Type, Name, Start, Atts_off, Atts_nick, Params, Parent, Children, Vis);
	V["id"] = *Id;
	V["type"] = *Type;
	V["name"] = *Name;
	vectime[0] = Start.get_seconds();
	vectime[1] = Start.get_milliseconds();
	V["start"] = vectime;
	while((tds = atts())) {
		std::string data_type;
		// this makes sure we don't come up with undefined XmlRpcValues,
		// e. g. when translating an empty apgen array
		try {
			if(tds->payload.get_type() == apgen::DATA_TYPE::ARRAY) {
				if(tds->payload.get_array().get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
					data_type = "struct"; }
				else {
					data_type = "array"; } }
			else {
				data_type = *spell(tds->payload.get_type()); }
			A[*tds->get_key()] = typedval2xml_API(tds->payload); }
		catch(XmlRpc::XmlRpcException) {} }
	V["attributes"] = A;
	while((tds = params())) {
		std::string data_type;
		// this makes sure we don't come up with undefined XmlRpcValues,
		// e. g. when translating an empty apgen array
		try {
			if(tds->payload.get_type() == apgen::DATA_TYPE::ARRAY) {
				if(tds->payload.get_array().get_array_type() == TypedValue::arrayType::STRUCT_STYLE) {
					data_type = "struct"; }
				else {
					data_type = "array"; } }
			else {
				data_type = *spell(tds->payload.get_type()); }
			P[*tds->get_key()] = typedval2xml_API(tds->payload);
			anyparameters = true; }
		catch(XmlRpc::XmlRpcException) {} }
	if(anyparameters) {
		V["parameters"] = P; }
	if(Vis == apgen::act_visibility_state::VISIBILITY_REGULAR) {
		V["visible"] = true; }
	else {
		V["visible"] = false;
		if(Vis == apgen::act_visibility_state::VISIBILITY_DECOMPOSED) {
			V["visible_parent"] = false; }
		else {
			V["visible_parent"] = true; } }
	if((N = (String_node*) Parent.first_node())) {
		has_parent = true;
		V["parent"] = *N->get_key(); }
	else {
		has_parent = false; }
	if(Children.get_length()) {
		bool						thereAreSome = false;
		slist<alpha_void, smart_actptr>::iterator	c(req->hierarchy().get_down_iterator());
		smart_actptr*					currentChildNode;
		int						j = 0;

		while((currentChildNode = c())) {
			ActivityInstance* child = currentChildNode->BP;
			if(!(NoCommands && child->is_command())) {
				thereAreSome = true;
				C[j++] = *child->get_unique_id(); } }
		if(thereAreSome) {
			V["children"] = C; } }
	if(Compress) {
		XmlRpc::XmlRpcValue::BinaryData	encoded;	// this is really a vector<char>
		XmlRpc::XmlRpcValue encodedVal;			// XmlRpcValue wrapping around encoded

		compressAString(V.toXml(), encoded);
		encodedVal = encoded;

		/* this defines result as an XmlRpc string which is the XML
		 * serialization of the actual XmlRpc object representing the
		 * activity instance. To recreate the XmlRpc object, all the
		 * client has to do is
		 *	1. extract the BinaryData (a.k.a. vector<char>) from
		 *	   the returned XmlRpc value
		 *	2. uncompress the BinaryData using the zlib inflate()
		 *	   method
		 *	3. invoke the fromXml() method on the resulting string
		 */
		result = encodedVal.toXml(); }
	else {
		/* this defines result as the XmlRpc object representing the
		 * activity instance: */
		result = V; } }
#endif /* OBSOLETE */
