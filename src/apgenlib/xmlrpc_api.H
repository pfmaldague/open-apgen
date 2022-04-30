#ifndef _PARSER_API_H_
#define _PARSER_API_H_

#include "XmlRpc.H"
#include <RES_exceptions.H>
#include <iostream>


class xmlrpc_api {
public:
	xmlrpc_api(const char* s)
		: enabled(false), id(s) {
	}
	~xmlrpc_api() {
	}

	static xmlrpc_api&	api();

	const char*		id;
	bool			enabled;
	XmlRpc::XmlRpcValue	init_client(
					const char*	host_name,
					int		port_number);
	static void		init_server(
					int		port_number);
	void			report_event(
					const std::string& s);
	void			report_start(
					const XmlRpc::XmlRpcValue& v);
	void			report_end(
					const XmlRpc::XmlRpcValue& v);
};

class xmlrpc_actor {
public:
	virtual XmlRpc::XmlRpcValue	xmlrpc_self() const {
						return id; }
	XmlRpc::XmlRpcValue	id;

	xmlrpc_actor(const std::string& i, long num) { id["seq_id"] = num; id["id"] = i; }
	~xmlrpc_actor() {}
};

class xmlrpc_report {
public:
	XmlRpc::XmlRpcValue	actor;
	XmlRpc::XmlRpcValue	event;
	xmlrpc_report(const xmlrpc_actor& A, const XmlRpc::XmlRpcValue& E)
			: actor(A.xmlrpc_self()),
			event(E) {
		if(xmlrpc_api::api().enabled) {
			XmlRpc::XmlRpcValue	v;
			v["actor"] = actor;
			v["event"] = event;

			// debug:
			// std::cout << "xmlrpc_report: v = " << v << "\n";
			// std::cout.flush();

			xmlrpc_api::api().report_start(v); }
		}
	~xmlrpc_report() {
		if(xmlrpc_api::api().enabled) {
			XmlRpc::XmlRpcValue	v;
			v["actor"] = actor;
			v["event"] = event;
			xmlrpc_api::api().report_end(v); }
		}
};

#endif /* _PARSER_API_H_ */
