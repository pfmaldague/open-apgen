#if	HAVE_CONFIG_H
#include <config.h>
#endif

#include <XmlRpc.H>		// XmlRpc::XmlRpcClient
#include <stdlib.h>		// getenv()
#include <xmlrpc_api.H>
#include <iostream>

using namespace XmlRpc;

static const char* getXmlRpcId() {
	char*	XmlRpcId = getenv("AUTOID");
	if(XmlRpcId) {
		return XmlRpcId;
	}
	return "default XmlRpcId";
}

static const char*	theClientName = NULL;
static int		thePortNumber = -1;

static XmlRpcClient& theClient() {
	static XmlRpcClient a_client(theClientName, thePortNumber);
	return a_client;
}

static XmlRpcServer& theServer() {
	static XmlRpcServer s;
	return s;
}

xmlrpc_api&	xmlrpc_api::api() {
	static xmlrpc_api	pa(getXmlRpcId());
	return pa;
}

void xmlrpc_api::init_server(
		/* later on we may change these default values
		 * through arguments of this method: */
	int	port_number) {
	int	verbosity = 0;
				// 12 hours:
	double	timeOutInSeconds = 43200.0;

	if(port_number < 0) {
		throw(eval_error("xmlrpc_api::init_server(): negative port"));
	}
	XmlRpc::setVerbosity(verbosity);
	if(!theServer().bindAndListen(port_number)) {
		Cstring err("xmlrpc_api::init_server(): cannot bind to port ");
		err << port_number;
		throw(eval_error(err));
	}
	theServer().enableIntrospection(true);

	// Wait for requests for indicated no. of seconds (forever if = -1)
	theServer().work(timeOutInSeconds);
}

XmlRpcValue xmlrpc_api::init_client(
		const char* host_info,
		int port_number) {
	theClientName = host_info;
	thePortNumber = port_number;

	XmlRpcValue	Args, oneArg, result, returned;

	Args = "initialize";
	oneArg[0] = Args;
	// debug
	// std::cerr << "about to call execute with arg " << oneArg << "...\n";
	bool it_worked = theClient().execute("api", oneArg, result);
	// debug
	// std::cerr << "after the call to execute...\n";
	if(!it_worked) {
		std::cout << "xmlrpc_api::init(): error creating client(host = "
			<< host_info << ", port = " << port_number << ")\n";
		enabled = false;
		returned = std::string("error");
		return returned;
	}
	if(theClient().isFault()) {
		std::cout << "xmlrpc_api::init(): error sending cmd:\n"
			<< result << "\n";
		returned = std::string("error");
		enabled = false;
		return returned;
	} else {
		enabled = true;
		// debug
		// std::cout << "xmlrpc_api::init() success - result = " << result << "\n";
	}
	// debug
	// std::cerr << "init(): successful return\n";
	return result;
}

void xmlrpc_api::report_event(const std::string& s) {
	XmlRpcValue	Args, oneArg, result;

	Args = s;
	oneArg[0] = Args;
	// debug
	// std::cerr << "about to call execute with arg " << oneArg << "...\n";
	bool it_worked = theClient().execute("event", oneArg, result);
	// debug
	// std::cerr << "after the call to execute...\n";
	if(!it_worked) {
		std::cout << "xmlrpc_api::report_event(): error creating client(host = "
			<< theClientName << ", port = " << thePortNumber << ")\n";
		enabled = false;
	}
	if(theClient().isFault()) {
		std::cout << "xmlrpc_api::report_event(): error sending message:\n" << result << "\n";
		enabled = false;
	} else {
		// debug
		// std::cout << "xmlrpc_api::report_event() success - result = " << result << "\n";
	}
}

void xmlrpc_api::report_start(const XmlRpcValue& s) {
	XmlRpcValue	oneArg, result;

	oneArg[0] = s;
	bool it_worked = theClient().execute("start", oneArg, result);
	if(!it_worked) {
		std::cout << "xmlrpc_api::report_start(): error creating client(host = "
			<< theClientName << ", port = " << thePortNumber << ")\n";
		enabled = false;
	}
	if(theClient().isFault()) {
		std::cout << "xmlrpc_api::report_start(): error sending message:\n"
			<< result << "\n";
		enabled = false;
	} else {
		// std::cout << "xmlrpc_api::report_start() success - result = "
		// 	<< result << "\n";
	}
}

void xmlrpc_api::report_end(const XmlRpcValue& s) {
	XmlRpcValue	oneArg, result;

	oneArg[0] = s;
	bool it_worked = theClient().execute("end", oneArg, result);
	if(!it_worked) {
		std::cout << "xmlrpc_api::report_end(): error creating client(host = "
			<< theClientName << ", port = " << thePortNumber << ")\n";
		enabled = false;
	}
	if(theClient().isFault()) {
		std::cout << "xmlrpc_api::report_end(): error sending message:\n"
			<< result << "\n";
		enabled = false;
	} else {
		// std::cout << "xmlrpc_api::report_end() success - result = "
		// 	<< result << "\n";
	}
}
