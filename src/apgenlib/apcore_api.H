#ifndef _APCORE_API_H
#define _APCORE_API_H


		// the intent is to try and manage with
		// only this include file, which defines
		// the all-important TypedValue class:
#include	"APdata.H"
#include	"EventRegistry.H"

		// Unfortunately, we need access to instances,
		// which are the responsibility of ACT_exec...
#include	"ACT_exec.H"
#include 	"ActivityInstance.H"
#include	"Multiterator.H"

extern "C" {
#include	"concat_util.h"
} // extern "C"

		// provides access to the XmlRpcValue class:
#include	"XmlRpc.H"
#include	"XmlRpcCompress.H"

extern void fill_struct_API(TypedValue &v, XmlRpc::XmlRpcValue &x);
extern void fill_list_API(TypedValue &v, XmlRpc::XmlRpcValue &x);
extern void fill_time_API(TypedValue &v, XmlRpc::XmlRpcValue &x);
extern void xml2typedval_API(XmlRpc::XmlRpcValue &x, TypedValue &y);
extern XmlRpc::XmlRpcValue typedval2xml_API(const TypedValue &theVal);
extern time_t xmlrpc_timegm(struct tm *);
extern std::string xml2serialized_act_symbol(
			const std::string& id,
			const std::string& symbol_name,
			std::string& correct_symbol_name,
			XmlRpc::XmlRpcValue& V);

extern int createActivityAPI(
		const std::string		&name,
		const std::string		&type,
		const std::string		&legend,
		std::string			&id,
		std::vector<std::pair<std::string,
				std::string> >	&params,
		struct tm			&start_time,
		XmlRpc::XmlRpcValue		&any_events_caught,
		std::string			&errors
		);

#ifdef OBSOLETE
extern int	checkPlanAPI(XmlRpc::XmlRpcValue &intermediateResult, bool checkForChanges, string &errors);
extern void	getPlanIDsAPI(XmlRpc::XmlRpcValue &PlanIDs, bool &isEmpty, bool Full);
extern void	getVersionInfoAPI(XmlRpc::XmlRpcValue &theValue, std::string &errormsg);
extern void	getPlanAPI(	XmlRpc::XmlRpcValue&	aPlan,
				bool&			isEmpty,
				bool			Full,
				bool			Top,
				bool			NoCommands,
				bool			Compress);
extern void	getOneInstanceAsXMLAPI(
				ActivityInstance*			req,
				XmlRpc::XmlRpcValue&		V,
				apgen::act_visibility_state&	Vis,
				bool&				has_parent,
				bool				NoCommands,
				bool				Compress);

#ifdef IN_XMLRPC_COMPRESS_H_NOW
extern int	compressAString(const std::string &inbuf,
				XmlRpc::XmlRpcValue::BinaryData &result);
extern int	dexompressAString(
				XmlRpc::XmlRpcValue::BinaryData &inbuf,
				std::string &result);
#endif /* IN_XMLRPC_COMPRESS_H_NOW */

extern void	getPlanAsXMLStringAPI(
				XmlRpc::XmlRpcValue &aPlan,
				bool &isEmpty,
				bool Full,
				bool Top,
				bool NoCommands,
				bool Compress);

extern void	getEpochsAPI(	XmlRpc::XmlRpcValue &epochs);
		// normal result = 0
extern int	getAllActAttrParamDefsAPI(
				XmlRpc::XmlRpcValue &result,
				std::string &errors);

extern "C" {
void getActInstanceAsXMLstring(void *act_ptr, buf_struct *result);
void getAllActAttrParamDefsAsXMLstring(buf_struct *result);
} // extern "C"

	/* includes param descriptions (if available); time values are
	 * expressed as vectors = [seconds, milliseconds] */
extern int getActivityParametersAPI(
				const std::string	&actTypeName,
				XmlRpc::XmlRpcValue	&params,
				std::string		&any_errors);

extern int isActDurEditableAPI(std::string& theID, XmlRpc::XmlRpcValue& theValue);
#endif /* OBSOLETE */

#endif /* _APCORE_API_H */
