#ifndef _XMLRPC_INTFC_H_
#define _XMLRPC_INTFC_H_

#include <iostream>
#include <sstream>
#include <stdlib.h>

#include <APdata.H>
#include <XmlRpc.H>
#include <XmlRpcCompress.H>

// normal return is 0
int
getActivityParametersAPI(
		const string		&act_type,
		XmlRpc::XmlRpcValue	&parVec,
		XmlRpc::XmlRpcValue	&attVec,
		string			&errors);

XmlRpc::XmlRpcValue
typedval2xml_API(
		const TypedValue &theVal);

void
xml2typedval_API(
		XmlRpc::XmlRpcValue &x,
		TypedValue &retval);

extern "C" {
#include <concat_util.h>

void
getActInstanceAsXMLstring(
		void *act_ptr,
		buf_struct *result);
void
getAllActAttrParamDefsAsXMLstring(
		buf_struct *result);
} // extern "C"

int
getAllActAttrParamDefsAPI(
		XmlRpc::XmlRpcValue &returnedStruct,
		std::string &errors);
void
getOneInstanceAsXMLAPI(
		ActivityInstance *req,
		XmlRpc::XmlRpcValue &result,
		apgen::act_visibility_state &Vis,
		bool &has_parent,
		bool NoCommands,
		bool Compress);

#endif /* _XMLRPC_INTFC_H_ */
