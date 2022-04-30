#ifndef _APCORE_SERVER_H_
#define _APCORE_SERVER_H_

#include "XmlRpc.H"
#include "XmlRpcServer.H"
#include "apcore_intfc.H"
#include "apcore_api.H"

class APcoreInit: public XmlRpc::XmlRpcServerMethod {
public:
	static bool initializeAPcore(const vector<string>& args, string& errors);
	static bool was_APcore_initialized();
	APcoreInit(XmlRpc::XmlRpcServer *s);
	void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);
	std::string help();
	};

extern XmlRpc::XmlRpcValue xmlTime(long sec);
extern XmlRpc::XmlRpcServer &theServer();
extern XmlRpc::XmlRpcValue recursively_fill_value(apcoreValue *av, int indent = 0);
extern void getActivityTypesAPI(XmlRpc::XmlRpcValue &result, bool &Empty);
extern void getLegendBasedActivityTypesAPI(XmlRpc::XmlRpcValue &result);

#endif /* _APCORE_SERVER_H_ */
