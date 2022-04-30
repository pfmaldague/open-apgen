
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "XmlRpc.H"

using namespace XmlRpc;

// The server
XmlRpcServer &theServer() {
	static XmlRpcServer s;
	return s; }

class apiMethod: public XmlRpcServerMethod {
public:
	apiMethod(XmlRpcServer* s) : XmlRpcServerMethod("api", s) {}
	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		if(
			params.size() != 1
			|| params[0].getType() != XmlRpcValue::TypeString
		  ) {
			std::cerr << "apiMethod: param size = " << params.size() << "; params[0] = " << params[0] << "\n";
			throw XmlRpcException("error: expected 1 string parameter", 10);
		}
		std::string	arg = (std::string) params[0];
		result = 0L;
		std::cout << "api(" << arg << ")\n";
	}
	std::string help() {
		return std::string("allows client to initialize the reporting API.\n");
	}

} api_handler(&theServer());

class exitMethod: public XmlRpcServerMethod {
public:
	exitMethod(XmlRpcServer* s) : XmlRpcServerMethod("exit", s) {}
	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		theServer().setExitFlag();
		result = 0L;
		std::cout << "exit()\n";
	}
	std::string help() {
		return std::string("tells client to exit.\n");
	}

} exit_handler(&theServer());

class event_Method: public XmlRpcServerMethod {
public:
	event_Method(XmlRpcServer* s) : XmlRpcServerMethod("event", s) {}
	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		if(
			params.size() != 1
			|| (params[0].getType() != XmlRpcValue::TypeString && params[0].getType() != XmlRpcValue::TypeArray)
		  ) {
			std::cerr << "event_Method: param size = " << params.size() << "; params[0] = " << params[0] << "\n";
			throw XmlRpcException("error: expected 1 array or string parameter", 10);
		}
		if(params[0].getType() == XmlRpcValue::TypeString) {
			std::string	arg = (std::string) params[0];
			result = 0L;
			std::cout << arg << "\n";
			std::cout.flush();
		} else if(params[0].getType() == XmlRpcValue::TypeArray) {
			std::vector<XmlRpc::XmlRpcValue>&	vec(params[0].get_array());
			std::cout << "event(" << vec.size() << "):\n";
			for(int i = 0; i < vec.size(); i++) {
				XmlRpcValue	arg = vec[i];
				std::cout << "\t[" << i << "] = " << arg;
			}
			std::cout.flush();
			result = 0L;
		}
	}
	std::string help() {
		return std::string("allows client to send event to the API.\n");
	}

} event_handler(&theServer());

class start_Method: public XmlRpcServerMethod {
public:
	start_Method(XmlRpcServer* s) : XmlRpcServerMethod("start", s) {}
	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		if(params.size() != 1) {
			std::cerr << "start_Method: param size = " << params.size() << "\n";
			throw XmlRpcException("error: expected 1 parameter", 10);
		}
		std::cout << "START: " << params[0] << "\n";
		std::cout.flush();
		result = 0L;
	}
	std::string help() {
		return std::string("allows client to send start to the API.\n");
	}
} start_handler(&theServer());

class end_Method: public XmlRpcServerMethod {
public:
	end_Method(XmlRpcServer* s) : XmlRpcServerMethod("end", s) {}
	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		if(params.size() != 1) {
			std::cerr << "end_Method: param size = " << params.size() << "\n";
			throw XmlRpcException("error: expected 1 parameter", 10);
		}
		std::cout << "END: " << params[0] << "\n";
		std::cout.flush();
		result = 0L;
	}
	std::string help() {
		return std::string("allows client to send end to the API.\n");
	}
} end_handler(&theServer());

int usage(const char* a) {
	std::cerr << "Usage: " << a << " <port>\n";
	return -1;
}

int main(int argc, char* argv[]) {
	int		port = -1;
			// default: timeout = 12 hours
	double		timeOutInSeconds = 43200.0;

	for(int i = 1; i < argc; i++) {
		if(i == argc - 1) {
			char* dd = argv[i];
			while(*dd) {
				if(!isdigit(*dd)) {
					return usage(argv[0]);
				}
				dd++;
			}
			port = atoi(argv[i]);
			// debug
			std::cerr << "Setting port to " << port << "\n";
		} else {
			return usage(argv[0]);
		}
	}

	if(port < 0) {
		return usage(argv[0]);
	}

	// debug:
	// XmlRpc::setVerbosity(5);
	
	// Create the server socket on the specified port
	theServer().bindAndListen(port);

	// Enable introspection
	theServer().enableIntrospection(true);

	// Wait for requests for indicated no. of seconds (forever if = -1)
	theServer().work(timeOutInSeconds);

	return 0;
}
