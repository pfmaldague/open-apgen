#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "APcoreServer.H"
#include <apcoreWaiter.H>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <unistd.h> // for sleep()
#include <string.h>
#include <curl/curl.h>

extern "C" {
			// in ../apcore/APmain.C:
	extern void	(*activityInstanceSerializationHandle)(void *, buf_struct *);
	extern void	(*activityDefsSerializationHandle)(buf_struct *);
} // extern "C"

bool debug_commands = false;

using namespace XmlRpc;

string &theRunTimeDir() {
	static string s;
	return s;
}


//
// Obsolete - keep it as a source of inspiration
// for a possible follow-up
//
int
connectToMPSserverViaCurl(char* url) {
	CURL*		c = curl_easy_init();
	CURLcode	errornum;

	curl_easy_setopt(c, CURLOPT_URL, url);
	curl_easy_setopt(c, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
	if(CURLE_OK != (errornum = curl_easy_perform(c))) {
		fprintf(stderr, "curl error %s\n", curl_easy_strerror(errornum));
		curl_easy_cleanup(c);
		return -1;
	}
	curl_easy_cleanup(c);
	// normal return
	return 0;
}

//
// debug
//
void print_tm(const struct tm &theTime) {
	cout << "tm contents: Y " << theTime.tm_year
		<< " M " << (theTime.tm_mon + 1)
		<< " D " << theTime.tm_mday
		<< " H " << theTime.tm_hour
		<< " M " << theTime.tm_min
		<< " S " << theTime.tm_sec;
}

XmlRpcValue xmlTime(long sec) {
	struct tm	theTime;

	gmtime_r((const time_t *) &sec, &theTime);
	return XmlRpcValue(&theTime);
}

//
// The server
//
XmlRpcServer& theServer() {
	static XmlRpcServer s;
	return s;
}

class APcoreAlwaysFail: public  XmlRpcServerMethod {
public:
	APcoreAlwaysFail(XmlRpcServer *s) : XmlRpcServerMethod("alwaysFail", s) {}
	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		throw XmlRpcException("Testing testing 123", XmlRpcException::TESTING);
	}
	std::string help() { return std::string("Returns error code 0 and error string \"Testing testing 123\". "
			"Used for testing of exception handling.");
	}
} apcore_always_fails__not(&theServer());

#ifdef REMOVE_ALL_METHODS

APcoreInit::APcoreInit(XmlRpcServer *s) : XmlRpcServerMethod("APcoreInit", s) {}

// APcoreInit RESULT TYPE:
//
//   on success: string 'APcore initialized OK'
//
//   on failure: string 'APcore initialization error'
//
void APcoreInit::execute(XmlRpcValue& params, XmlRpcValue& result) {
	int returned_value;
	int argc = 0;
	int k;
	std::vector<std::string> theArgs;

	if(params.getType() != XmlRpcValue::TypeInvalid) {
		argc = params.size();
	}
	if(was_APcore_initialized()) {
		throw XmlRpcException("APcore already initialized", XmlRpcException::OVERINIT);
	}

	for(k = 0; k < argc; k++) {
		if(params[k].getType() != XmlRpcValue::TypeString) {
			char buf[200];
			sprintf(buf, "%d", k + 1);
			result = string("Error: parameter ") + buf + " is not a string";
			return;
		}
		if(params[k] == "-timeout") {
			double timeOutInSeconds;
			k++;
			string s = (string) params[k];
			sscanf(s.c_str(), "%lf", &timeOutInSeconds);
			theServer().set_timeout_value_to(timeOutInSeconds);
		}
		else {
			theArgs.push_back((std::string) params[k]);
		}
	}
	string errors;
	if(initializeAPcore(theArgs, errors)) {
		result = "APcore initialized OK";
	}
	else {
		string errs("APcore initialized with error(s):\n");
		errs = errs + errors;
		result = errs;
	}
}

std::string APcoreInit::help() {
	return std::string("Initializes APcore and returns a status string.\n"
	"Arguments: same as the command-line arguments of apgen/atm. Examples:\n"
	"-libudef /path/to/libudef.so, -noremodel, -restore-id, -read /path/to/APF/\n");
}

APcoreInit apcore_init(&theServer());    // This constructor registers the method with the server

class APcoreTerminate: public XmlRpcServerMethod {
public:
	static bool terminateAPcore();
	APcoreTerminate(XmlRpcServer *s) : XmlRpcServerMethod("APcoreTerminate", s) {}

	/* APcoreTerminate RESULT TYPE:
	 *
	 *   on success: 'APcore terminated OK';
	 *
	 *   on failure: 'APcore termination error'
	 */
	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		if(debug_commands) {
			std::cout << "APcoreTerminate called\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		else if(terminateAPcore()) {
			result = "APcore terminated OK";
		}
		else {
			result = "APcore termination error";
		}
	}

	std::string help() { return std::string("Terminates APcore and returns a status string.\n"
						"Arguments: none\n");
	}
} apcore_terminate(&theServer());    // This constructor registers the method with the server

static bool init_done = false;

bool APcoreInit::was_APcore_initialized() {
	return init_done;
}

bool APcoreInit::initializeAPcore(const std::vector<std::string> &args, string &errors) {
	init_done = true;
	bool no_errors = APcoreIntfc::initialize(args, errors) == 0;
	return no_errors;
}

bool APcoreTerminate::terminateAPcore() {
	init_done = false;
	return APcoreIntfc::terminate() == 0;
}


// No args, returns array of epochs or "NO DATA"
class getEpochs : public XmlRpcServerMethod {
public:
	getEpochs(XmlRpcServer *s) : XmlRpcServerMethod("getEpochs", s) {}

	/* putAAFFile RESULT TYPE:
	 *
	 *	on success: 'OK'
	 *
	 *	on failure: 'ERROR'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		if(debug_commands) {
			std::cout << "getEpochs called\n";
			std::cout.flush();
		}
		getEpochsAPI(result);
		if(result.getType() != XmlRpcValue::TypeArray) {
			result = "NO DATA";
		}
	}
	std::string help() {
		return std::string(
			"Takes no arg; returns array of epochs as keyword/vec\n"
			"pairs (vec[0] = sec, vec[1] = millisec). Returns the\n"
			"string 'NO DATA' if no epochs are defined.\n");
	}
} get_epochs(&theServer());

// One argument is passed (the contents of the AAF), result is "OK" or "ERROR"
class putAAFFile : public XmlRpcServerMethod {
public:
	putAAFFile(XmlRpcServer *s) : XmlRpcServerMethod("putAAFFile", s) {}

	/* putAAFFile RESULT TYPE:
	 *
	 *	on success: 'OK'
	 *
	 *	on failure: 'ERROR'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		std::string	fileContents = params[0];
		std::string	resultString;
		std::string	errorString;

		if(debug_commands) {
			std::cout << "putAAFfile called\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(!APcoreIntfc::FILE_OPEN(fileContents, errorString)) {
			resultString = "OK";
		}
		else {
			throw XmlRpcException(errorString, XmlRpcException::BADSYNTAX);
		}
		result = resultString;
	}
	std::string help() {
		return std::string(
			"Takes 1 string arg; parses supplied string as an AAF");
	}
} put_aaf_file(&theServer());

// One argument is passed (the contents of the file to be read by APcore), result is "OK" or "ERROR"
class putFile : public XmlRpcServerMethod {
public:
	putFile(XmlRpcServer *s) : XmlRpcServerMethod("putFile", s) {}

	/* putFile RESULT TYPE:
	 *
	 *	on success: 'OK'
	 *
	 *	on failure: 'ERROR'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		std::string	fileContents = params[0];
		std::string	resultString;
		std::string	errorString;

		if(debug_commands) {
			std::cout << "putFile called\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(!APcoreIntfc::FILE_OPEN(fileContents, errorString)) {
			resultString = "OK";
		}
		else {
			throw XmlRpcException(errorString, XmlRpcException::BADSYNTAX);
		}
		result = resultString;
	}
	std::string help() {
		return std::string(
			"Takes 1 string arg; parses supplied string as an input file (e. g. AD, AAF, APF)");
	}
} put_file(&theServer());

// Zero argument is passed; result is an array {"APcore", "udef", "mmpat", or "mmpat_config"}, w/version info for each item.
class getVersionInfo : public XmlRpcServerMethod {
public:
	getVersionInfo(XmlRpcServer *s) : XmlRpcServerMethod("getVersionInfo", s) {}

	/* getVersionInfo RESULT TYPE:
	 *
	 *	on success: array containing version info for APcore, udef, mmpat, mmpat_config.
	 *
	 *	on failure: throws exception
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		std::string errorString;

		if(debug_commands) {
			std::cout << "getVersionInfo called\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		getVersionInfoAPI(result, errorString);
		if(errorString.size() > 0) {
			throw XmlRpcException(errorString, XmlRpcException::NOSUBSYSTEM);
		}
	}
	std::string help() {
		return std::string(
			"Takes 1 string arg; parses supplied string as an input file (e. g. AD, AAF, APF)");
	}
} get_version_info2(&theServer());

XmlRpcValue recursively_fill_value(apcoreValue *av, int indent /* declared default = 0 */ ) {
	XmlRpcValue		oneVal;
	int			abc = 0;

	if(av->is_string()) {
		oneVal[0] = av->get_type();
		oneVal[1] = av->get_string();
	}
	else if(av->is_list()) {
		apcoreArray		&A = av->get_array();
		apcoreArray::iterator	k;
		XmlRpcValue		anotherVal;

#		ifdef APcoreDEBUG
		std::cerr << "recursive(array): array size = " << A.size() << std::endl;
#		endif /* APcoreDEBUG */
		for(k = A.begin(); k != A.end(); k++) {
#		ifdef APcoreDEBUG
		std::cerr << "recursive(array): calling self\n";
#		endif /* APcoreDEBUG */
			anotherVal[(*k)->get_key()] = recursively_fill_value((*k), indent + 3);
		}
		oneVal[0] = string("array");
		oneVal[1] = anotherVal;
		}
	else {
		oneVal["value"] = string("Error from recursively_fill_value");
	}
#	ifdef APcoreDEBUG
	for(abc = 0; abc < indent; abc++) {
		std::cerr << " ";
	}
	std::cerr << "recursive: returning " << oneVal << std::endl;
#	endif /* APcoreDEBUG */
	return oneVal;
}

class getActivityInstanceAttributeValue : public XmlRpcServerMethod {
public:
	getActivityInstanceAttributeValue(XmlRpcServer *s) : XmlRpcServerMethod("getActivityInstanceAttributeValue", s) {}

	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		extern void getActivityInstanceAttributeValueAPI(
			XmlRpcValue &anId,
			XmlRpcValue &anAttributeName,
			XmlRpcValue &theValue,
			bool &nosuchid,
			bool &nosuchattribute,
			bool &attrisundefined);
		ostringstream msg;
		bool nosuchid;
		bool nosuchattribute;
		bool attrisundefined;
		XmlRpcValue val;

		if(debug_commands) {
			std::cout << "getActivityInstanceAttributeValue called\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(params.size() != 2) {
			msg << "Method invocation has " << params.size() << " parameters while server expected 2.";
			throw XmlRpcException(msg.str(), XmlRpcException::BADPARNUM);
		}
		getActivityInstanceAttributeValueAPI(params[0], params[1], val, nosuchid, nosuchattribute, attrisundefined);
		if(nosuchid) {
			msg << "Cannot find activity instance with ID " << params[0];
			throw XmlRpcException(msg.str(), XmlRpcException::NOSUCHID);
		}
		if(nosuchattribute) {
			msg << "Attribute " << params[1] << " does not exist in the Activity Dictionary";
			throw XmlRpcException(msg.str(), XmlRpcException::NOSUCHATT);
		}
		if(attrisundefined) {
			msg << "Attribute " << params[1] << " is not defined for activity with ID " << params[0];
			throw XmlRpcException(msg.str(), XmlRpcException::ATTUNDEF);
		}
		result = val;
	}
	std::string help() {
		return std::string(
			"Takes 2 args: <act ID>, <att name>; returns value of requested attribute.\n"
			"Throws NOSUCHID if act does not exist, NOSUCHATT if attribute name is unknown,\n"
			"ATTUNDEF if attribute was not defined for this act.\n");
	}
} get_act_inst_att_val(&theServer());

class setActivityInstanceAttributeValue : public XmlRpcServerMethod {
public:
	setActivityInstanceAttributeValue(XmlRpcServer *s) : XmlRpcServerMethod("setActivityInstanceAttributeValue", s) {}

	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		ostringstream	msg;
		apcoreArray	editvals;
		list<string>	range_list;

		if(debug_commands) {
			std::cout << "setActivityInstanceAttributeValue called; args = " << params << "\n";
			std::cout.flush();
		}

		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}

		if(params.size() != 3) {
			msg << "Method invocation has " << params.size() << " parameters while server expected 3.";
			throw XmlRpcException(msg.str(), XmlRpcException::BADPARNUM);
		}

		/* params[0] = activity ID
		 * params[1] = symbol name
		 * params[2] = symbol value as an XmlRpcValue, with time potentially expressed as an array [sec, msec] */
		if(	params[0].getType() != XmlRpcValue::TypeString
			|| params[1].getType() != XmlRpcValue::TypeString) {
			throw XmlRpcException("Bad parameters: expected string(ID), string(symbolName), value",
				XmlRpcException::BADPARTYPE);
		}

		string	stringvalue;
		string	correctName;
		string	theID = (string) params[0];

		stringvalue = xml2serialized_act_symbol(theID, (string) params[1], correctName, params[2]);
		editvals.push_back(new de_facto_string(
			correctName,
			"",
			range_list,
			stringvalue,
			"string"));

		if(APcoreIntfc::editActivity(theID, editvals)) {
			throw XmlRpcException(string("ERROR trying to edit activity ") + theID, XmlRpcException::NOSUCHID);
		}
		result = string("OK");
	}

	std::string help() {
		return std::string(
			"Takes 3 args: <act ID>, <att name>, <att value>; returns nothing.\n"
			"Throws NOSUCHID if act does not exist, NOSUCHATT if attribute name is unknown,\n"
			"BADPARBAL if an error is found when trying to set this act's parameter to the given value.\n");
	}
} set_act_inst_att_val(&theServer());


class getActivityInstanceParameterValue : public XmlRpcServerMethod {
public:
	getActivityInstanceParameterValue(XmlRpcServer *s) : XmlRpcServerMethod("getActivityInstanceParameterValue", s) {}

	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		extern void getActivityInstanceParamValueAPI(
			XmlRpcValue &anId,
			XmlRpcValue &aParamName,
			XmlRpcValue &theValue,
			bool &nosuchid,
			bool &paramisundefined);
		ostringstream msg;
		bool nosuchid;
		bool paramisundefined;
		XmlRpcValue val;

		if(debug_commands) {
			std::cout << "getActivityInstanceParameterValue called\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(params.size() != 2) {
			msg << "Method invocation has " << params.size() << " parameters while server expected 2.";
			throw XmlRpcException(msg.str(), XmlRpcException::BADPARNUM);
		}
		getActivityInstanceParamValueAPI(params[0], params[1], val, nosuchid, paramisundefined);
		if(nosuchid) {
			msg << "Cannot find activity instance with ID " << params[0];
			throw XmlRpcException(msg.str(), XmlRpcException::NOSUCHID);
		}
		if(paramisundefined) {
			msg << "Parameter " << params[1] << " is not defined for activity with ID " << params[0];
			throw XmlRpcException(msg.str(), XmlRpcException::PARUNDEF);
		}
		result = val;
	}
	std::string help() {
		return std::string(
			"Takes 2 args: <act ID>, <param name>; returns value of requested parameter.\n"
			"Throws NOSUCHID if act does not exist, PARUNDEF if parameter was not defined for this act.\n");
	}
} get_act_inst_param_val(&theServer());

class setActivityInstanceParameterValue : public XmlRpcServerMethod {
public:
	setActivityInstanceParameterValue(XmlRpcServer *s) : XmlRpcServerMethod("setActivityInstanceParameterValue", s) {}

	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		extern void setActivityInstanceParamValueAPI(
			XmlRpcValue &anId,
			XmlRpcValue &aParamName,
			XmlRpcValue &theValue,
			bool &nosuchid,
			bool &paramisundefined,
			bool &errorfound,
			string &errmsg);
		ostringstream msg;
		string errormsg;
		bool nosuchid;
		bool paramisundefined;
		bool errorfound;

		if(debug_commands) {
			std::cout << "setActivityInstanceParameterValue called; args = " << params << "\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(params.size() != 3) {
			msg << "Method invocation has " << params.size() << " parameters while server expected 2.";
			throw XmlRpcException(msg.str(), XmlRpcException::BADPARNUM);
		}
		setActivityInstanceParamValueAPI(params[0], params[1], params[2],
			nosuchid, paramisundefined, errorfound, errormsg);
		if(nosuchid) {
			msg << "Cannot find activity instance with ID " << params[0];
			throw XmlRpcException(msg.str(), XmlRpcException::NOSUCHID);
		}
		if(paramisundefined) {
			msg << "Parameter " << params[1] << " is not defined for activity with ID " << params[0];
			throw XmlRpcException(msg.str(), XmlRpcException::PARUNDEF);
		}
		if(errorfound) {
			msg << "Error trying to set " << params[1] << " to value " << params[2]
				<< " for activity with ID " << params[0] << "; details: " << errormsg;
			throw XmlRpcException(msg.str(), XmlRpcException::BADPARVAL);
		}
	}
	std::string help() {
		return std::string(
			"Takes 3 args: <act ID>, <param name>, <param valu>; returns nothing.\n"
			"Throws NOSUCHID if act does not exist, PARUNDEF if parameter was not defined for this act.\n");
	}
} set_act_inst_param_val(&theServer());

#ifdef OBSOLETE
class updateActivityInstanceAttributes : public XmlRpcServerMethod {
public:
	updateActivityInstanceAttributes(XmlRpcServer *s) : XmlRpcServerMethod("updateActivityInstanceAttributes", s) {}

	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		extern void updateActivityInstanceAttributesAPI(
			XmlRpcValue &anId,
			bool &nosuchid,
			bool &errorfound,
			string &errmsg);
		ostringstream msg;
		string errormsg;
		bool nosuchid;
		bool paramisundefined;
		bool errorfound;

		if(debug_commands) {
			std::cout << "updateActivtiyInstanceAttributes called; params = " << params << "\n";
			std::cout.flush(); }
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT); }
		if(params.size() != 1) {
			msg << "Method invocation has " << params.size() << " parameters while server expected 1.";
			throw XmlRpcException(msg.str(), XmlRpcException::BADPARNUM); }
		updateActivityInstanceAttributesAPI(params[0], nosuchid, errorfound, errormsg);
		if(nosuchid) {
			msg << "Cannot find activity instance with ID " << params[0];
			throw XmlRpcException(msg.str(), XmlRpcException::NOSUCHID); }
		if(errorfound) {
			msg << "Error trying to update activity with ID " << params[0] << "; details: " << errormsg;
			throw XmlRpcException(msg.str(), XmlRpcException::BADPARVAL); } }
	std::string help() {
		return std::string(
			"Takes 1 args: <act ID>; returns nothing.\n"
			"Throws NOSUCHID if act does not exist, BADPARVAL if errors was found while updating activity.\n"); }
	} update_act_inst_att(&theServer());
#endif /* OBSOLETE */

void delete_gtk_elements(apcoreArray &A) {
	apcoreArray::iterator i;

	for(i = A.begin(); i != A.end(); i++) {
		delete (*i); }
	A.erase(A.begin(), A.end());
}

class createActivity : public XmlRpcServerMethod {
public:
	createActivity(XmlRpcServer *s) : XmlRpcServerMethod("createActivity", s) {}

	/* createActivity RESULT TYPE:
	 *
	 *	on success: vector V in which V[0] = the string 'OK'
	 *
	 *	on failure: string 'ERROR'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		if(debug_commands) {
			std::cout << "createActivity called; args = " << params << "\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(	params[0].getType() != XmlRpcValue::TypeStruct
			|| (!params[0].hasMember("name"))
			|| (!params[0].hasMember("type"))
			|| (!params[0].hasMember("start"))
		  ) {
			throw XmlRpcException("ERROR: param 0 is not a struct or does not contain one of "
				"(name, type, start)", XmlRpcException::BADPARVAL);
		}
		std::string any_errors;
		// inefficient, but both constant and non-constant references lead to errors
		XmlRpcValue::ValueStruct Param0 = (XmlRpcValue::ValueStruct) params[0];
		std::string theType(Param0["type"]);
		std::string theName(Param0["name"]);
		std::string theID(theName);
		struct tm theStart(Param0["start"]);
		XmlRpcValue	intermediateResult;

#		ifdef APcoreDEBUG
		std::cout << "createActivity: start time = ";
		print_tm(theStart);
		std::cout << std::endl;
#		endif /* APcoreDEBUG */

		std::string theLegend;
		std::vector<apcoreValue *> officialParams;
		int failure = APcoreIntfc::getActivityParameters(
				theType, officialParams, any_errors);
		std::vector<apcoreValue *>::iterator i;
		std::map<std::string, XmlRpcValue>::iterator k;
		std::map<std::string, int> parameterMap;
		int l, params_found = 0;
		std::vector<std::pair<std::string, std::string> > param_vec;

		if(failure) {
			throw XmlRpcException(string("ERROR: cannot find params for act. type ") + theType,
				XmlRpcException::BADPARVAL);
		}
		for(l = 0; l < officialParams.size(); l++) {
			apcoreValue				*av = officialParams[l];
			XmlRpcValue::ValueStruct::iterator	f = Param0.find(av->Key);

			parameterMap[officialParams[l]->Key] = l;
		}
		for(	k = Param0.begin();
			k != Param0.end();
			k++) {
#			ifdef APcoreDEBUG
			std::cout << "createAct: processing " << k->first << std::endl;
#			endif /* APcoreDEBUG */
			if(mapOfStdActAtts().find(k->first) != mapOfStdActAtts().end()) {
#				ifdef APcoreDEBUG
				std::cout << "createAct: got attr. " << (*k).first;
#				endif /* APcoreDEBUG */
				if(k->first == "legend") {
#					ifdef APcoreDEBUG
					std::cout << "found legend = " << k->second << std::endl;
#					endif /* APcoreDEBUG */
					theLegend = (std::string) k->second;
				}
			} else if(parameterMap.find(k->first) != parameterMap.end()) {
				std::pair<std::string, std::string>	thePair(k->first,
									(std::string) k->second);

				params_found++;
				param_vec.push_back(thePair);
#				ifdef APcoreDEBUG
				std::cout << "createAct: got param. " << (*k).first << std::endl;
#				endif /* APcoreDEBUG */
			}
			else if(k->first == "id") {
				theID = (std::string) k->second;
			}
			else if(k->first != "type" && k->first != "name") {
				result = string("ERROR; unknown parameter/attribute ") + (*k).first;
				delete_gtk_elements(officialParams);
				return;
			}
		}
		if(createActivityAPI(
				theName,
				theType,
				theLegend,
				theID,
				param_vec,
				theStart,
				intermediateResult,
				any_errors)) {
			throw XmlRpcException(any_errors, XmlRpcException::BADPARVAL);
		}
		else {
			result[0] = string("OK");
			result[1] = theID;
			if(intermediateResult.getType() == XmlRpcValue::TypeStruct) {
				result[2] = intermediateResult;
			}
		}
		delete_gtk_elements(officialParams);
	}

	std::string help() {
		return std::string(
			"Takes 1 struct arg containing at least name, type, start;\n"
			"optionally, id and legend can also be specified.\n"
			"returns a vector {'OK', id} if successful.\n");
	}
} create_activity(&theServer());

int handle_one_act_creation_request(
		XmlRpcValue::ValueStruct Param0, // inefficient... neither constant nor non-constant references work
		std::string &theID,
		std::string &any_errors) {
	std::string theType(Param0["type"]);
	std::string theName(Param0["name"]);
	struct tm theStart(Param0["start"]);
	std::string theLegend;
	std::vector<apcoreValue *> officialParams;
	int failure = APcoreIntfc::getActivityParameters(
			theType, officialParams, any_errors);
	std::vector<apcoreValue *>::iterator i;
	std::map<std::string, XmlRpcValue>::iterator k;
	std::map<std::string, int> parameterMap;
	int l, params_found = 0;
	std::vector<std::pair<std::string, std::string> > param_vec;

	// we'll look for "id" later, will supersede this choice:
	theID = theName;

	if(failure) {
		any_errors = string("ERROR: cannot find params for act. type ") + theType;
		return failure; }
	for(l = 0; l < officialParams.size(); l++) {
		apcoreValue				*av = officialParams[l];
		XmlRpcValue::ValueStruct::iterator	f = Param0.find(av->Key);

		parameterMap[officialParams[l]->Key] = l;
	}
	for(	k = Param0.begin();
		k != Param0.end();
		k++) {
#		ifdef APcoreDEBUG
		std::cout << "createAct: processing " << k->first << std::endl;
#		endif /* APcoreDEBUG */
		if(mapOfStdActAtts().find(k->first) != mapOfStdActAtts().end()) {
#			ifdef APcoreDEBUG
			std::cout << "createAct: got attr. " << (*k).first;
#			endif /* APcoreDEBUG */
			if(k->first == "legend") {
#				ifdef APcoreDEBUG
				std::cout << "found legend = " << k->second << std::endl;
#				endif /* APcoreDEBUG */
				theLegend = (std::string) k->second;
			}
		} else if(parameterMap.find(k->first) != parameterMap.end()) {
			std::pair<std::string, std::string>	thePair(k->first,
								(std::string) k->second);

			params_found++;
			param_vec.push_back(thePair);
#			ifdef APcoreDEBUG
			std::cout << "createAct: got param. " << (*k).first << std::endl;
#			endif /* APcoreDEBUG */
		} else if(k->first == "id") {
			theID = (std::string) k->second;
		} else if(k->first != "type" && k->first != "name") {
			any_errors = string("ERROR; unknown parameter/attribute ") + (*k).first;
			delete_gtk_elements(officialParams);
			break;
		}
		XmlRpc::XmlRpcValue result;
		if((failure = createActivityAPI(
				theName,
				theType,
				theLegend,
				theID,
				param_vec,
				theStart,
				result,
				any_errors))) {
			delete_gtk_elements(officialParams);
			break;
		}
		delete_gtk_elements(officialParams);
	}
	// normal return: failure = 0...
	return failure;
}

class createActivityHierarchy : public XmlRpcServerMethod {
public:
	createActivityHierarchy(XmlRpcServer *s) : XmlRpcServerMethod("createActivityHierarchy", s) {}

	/* createActivityHierarchy RESULT TYPE:
	 *
	 *	on success: vector V in which V[0] = the string 'OK'
	 *
	 *	on failure: string 'ERROR'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		std::string any_errors;
		std::string the_id;
		int k;

		if(debug_commands) {
			std::cout << "createActivityHierarchy called; args = " << params << "\n";
			std::cout.flush(); }
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT); }
		if(	params[0].getType() != XmlRpcValue::TypeStruct
			|| (!params[0].hasMember("name"))
			|| (!params[0].hasMember("type"))
			|| (!params[0].hasMember("start"))
		  ) {
			result = "ERROR: param # 0 is not a struct or does not contain one of "
				"(name, type, start) for the parent activity";
			return; }
		if(handle_one_act_creation_request(params[0], the_id, any_errors)) {
			throw XmlRpcException(any_errors, XmlRpcException::MODELERR); }
		if(params.size() == 1) {
#			ifdef APcoreDEBUG
			cout << "createActHier: Only 1 param given; will only create the parent.\n";
#			endif /* APcoreDEBUG */
			}
		for(k = 1; k < params.size(); k++) {
			if(	params[k].getType() != XmlRpcValue::TypeStruct
				|| (!params[0].hasMember("name"))
				|| (!params[0].hasMember("type"))
				|| (!params[0].hasMember("start"))) {
				throw XmlRpcException("ERROR: param # 1 is not an array of child activities",
					XmlRpcException::BADPARVAL); }
			// handle act creation here
			}
		result[0] = string("OK");
		result[1] = the_id; }
		
	std::string help() {
		return std::string(
			"Takes 1 or more struct args containing at least name, type, start.\n"
			"Each struct specifies an activity. The first one is the parent;\n"
			"the following ones (if any) are to be added as children to the parent.\n"
			"Optionally, id and legend can be specified in addition to parameters\n"
			"for each activity.\n"
			"returns a vector {'OK', <id of parent>} if successful.\n"); }
} create_activity_hierarchy(&theServer());

// One argument is passed (the contents of the AAF), result is "OK" or "ERROR"
class getLogMessage : public XmlRpcServerMethod {
public:
	getLogMessage(XmlRpcServer *s) : XmlRpcServerMethod("getLogMessage", s) {}

	/* getLogMessage RESULT TYPE:
	 *
	 *	on success: string containing the log
	 *
	 *	on failure: empty string
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		if(debug_commands) {
			std::cout << "getLogMessage called\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		std::string	resultString = APcoreIntfc::getLogMessage();
		result = resultString;
	}
	std::string help() {
		return std::string(
			"Takes no arg; returns log so far.\n");
	}

} get_log_message(&theServer());

// One argument is passed (the contents of the AAF), result is "OK" or "ERROR"
class checkPlan : public XmlRpcServerMethod {
public:
	checkPlan(XmlRpcServer *s) : XmlRpcServerMethod("checkPlan", s) {}

	/* checkPlan RESULT TYPE:
	 *
	 *	on success: array of 1 or 2 elements;  1st element = string 'OK', 2nd element,
	 *		if it exists, contains ID's and names of changed parameters.
	 *
	 *	on failure: exception thrown
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		string errorsFound;
		XmlRpcValue intermediateResult;
		bool should_check_for_changed_acts = true;

		if(params.getType() == XmlRpcValue::TypeArray) {
			should_check_for_changed_acts = false;
		}
		if(debug_commands) {
			std::cout << "checkPlan called\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		int	k = checkPlanAPI(intermediateResult, should_check_for_changed_acts, errorsFound);

		if(k) {
			throw XmlRpcException(errorsFound, XmlRpcException::MODELERR);
		}
		result[0] = "OK";
		if(intermediateResult.getType() == XmlRpcValue::TypeStruct) {
			result[1] = intermediateResult;
		}
	}

	std::string help() {
		return std::string(
			"Takes no arg; causes APcore to perform a 'remodel'. Throws exception if error;\n"
 			"returns array w/ 1st el. = OK string, optional 2nd el. = struct indexed by act ID.\n"
			"The value of each element of the struct is itself a struct indexed by name of parameter\n"
			"whose value has changed while checking the plan. The value of this leaf struct\n"
			"element is always the integer 1.\n");
	}
} check_plan(&theServer());

class getActivityParameters : public XmlRpcServerMethod {
public:
	getActivityParameters(XmlRpcServer *s) : XmlRpcServerMethod("getActivityParameters", s) {}

	/* getActivityParameters RESULT TYPE:
	 *
	 *	on success: a vector V of N + 1 elements, where N is the number of activity parameters.
	 *		Element V[0] is the string 'OK'. For the n-th parameter, element V[n] is a
	 *		struct W such that
	 *
	 *			W["name"] = name of parameter
	 *			W["value"] = ValueVector (see getPlan above)
	 *
	 *	on failure: a vector V of one element, which is a string containing the message
	 *		'Error: <blah>' where <blah> explains the nature of the error (usually:
	 *		'activity "foo" does not exist')
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		std::string	activityType = params[0];
		apcoreArray	returned_params;
		std::string	any_errors;
		XmlRpcValue	result_parameters, result_attributes;

		if(debug_commands) {
			std::cout << "getActivityParameters called; args = " << params << "\n";
			std::cout.flush();
		}
		int		success_code = APcoreIntfc::getActivityParameters(
							activityType,
							returned_params,
							any_errors);
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		apcoreArray::iterator	k;

		if(success_code) {
			throw XmlRpcException(any_errors, XmlRpcException::BADPARVAL);
		} else {
			int				i = 0;

			result[0] = "OK";
			for(k = returned_params.begin(); k != returned_params.end(); k++) {
				XmlRpcValue		one_param;

#				ifdef APcoreDEBUG
				std::cerr << "getActParam(" << (*k)->get_key() << "): i = " << i << std::endl;
#				endif /* APcoreDEBUG */
				one_param["name"] = (*k)->get_key();
				// one_param["type"] = (*k)->get_type();
				if((*k)->is_string()) {
					XmlRpcValue	theVal;
					theVal[0] = (*k)->get_type();
					theVal[1] = (*k)->get_string();
#					ifdef APcoreDEBUG
					std::cerr << "string case: filled in value = " << (*k)->get_string() << std::endl;
#					endif /* APcoreDEBUG */
					one_param["value"] = theVal;
				} else {
					XmlRpcValue	theVal = recursively_fill_value((*k), 3);
#					ifdef APcoreDEBUG
					std::cerr << "array case: filled in value = " << theVal << std::endl;
#					endif /* APcoreDEBUG */
					one_param["value"] = theVal;
				}
				result_parameters[i++] = one_param;
			}
			if(!i) {
				result_parameters = "NO DATA";
			}
			result[1] = result_parameters;
			if(!i) {
				result_attributes = "NO DATA";
			}
			result[2] = result_attributes;
		}
		for(k = returned_params.begin(); k != returned_params.end(); k++) {
			delete (*k);
		}
	}

	std::string help() {
		return std::string(
			"Takes one arg, the act. type; returns a structure containing the string OK, a vector of params or the string 'NO DATA' if none.\n");
	}
		
} get_act_params(&theServer());

class getActivityData : public XmlRpcServerMethod {
public:
	getActivityData(XmlRpcServer *s) : XmlRpcServerMethod("getActivityData", s) {}

	/* getActivityData RESULT TYPE:
	 *
	 *	on success: a vector V of 3 elements.
	 *		Element V[0] is the string 'OK'. For the n-th parameter, element V[n] is a
	 *		struct W such that
	 *
	 *			W["name"] = name of parameter
	 *			W["value"] = value of the parameter expressed as an XmlRpcValue
	 *			W["description"] = description if available
	 *
	 *	on failure: a vector V of one element, which is a string containing the message
	 *		'Error: <blah>' where <blah> explains the nature of the error (usually:
	 *		'activity "foo" does not exist')
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		std::string		activityType = params[0];
		XmlRpc::XmlRpcValue	returned_params;
		std::string		any_errors;
		if(debug_commands) {
			std::cout << "getActivityData called; args = " << params << "\n";
			std::cout.flush();
		}
		int			success_code = getActivityParametersAPI(
							activityType,
							returned_params,
							any_errors);
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(success_code) {
			throw XmlRpcException(any_errors, XmlRpcException::BADPARVAL);
		}

		result[0] = "OK";
		result[1] = returned_params;
	}

	std::string help() {
		return std::string(
			"Takes one arg, the act. type; returns a structure containing the string OK, "
			"a vector of params or the string 'NO DATA' if none.\n");
	}
} get_act_data(&theServer());

class getStdActivityAttributes : public XmlRpcServerMethod {
public:
	getStdActivityAttributes(XmlRpcServer *s) : XmlRpcServerMethod("getStandardActivityAttributes", s) {}

	/* getStandardActivityAttributes RESULT TYPE:
	 *
	 *	on success: a vector V of N elements where N is the number of standard attributes; currently
	 *		n = 19. For each n, V[n] is a triplet W such that
	 *
	 *		W[0] = attribute name
	 *		W[1] = attribute type (e. g. integer, string, array, time etc.)
	 *		W[2] = description (a string)
	 *
	 *	on failure: N/A (does not happen)
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		std::vector<infotriplet>::iterator k;
		int i = 0;

		if(debug_commands) {
			std::cout << "getStdActivityAttributes called\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		for(i = 0, k = stdActAtts().begin(); k != stdActAtts().end(); k++, i++) {
			XmlRpcValue vec;
			vec[0] = (*k)[0];
			vec[1] = (*k)[1];
			vec[2] = (*k)[2];
			result[i] = vec;
		}
	}

	std::string help() {
		return std::string(
			"Takes no arg; returns a vector w/ all valid activity attributes as {name, type, description} triplets.\n");
	}
		
} get_std_act_atts(&theServer());

	// Use this to push back new items on an apcoreArray.
apcoreValue *xml2apcore(XmlRpcValue &xv) {
	switch(xv.getType()) {
		case XmlRpcValue::TypeBoolean:
			return new de_facto_int((bool) xv);
		case XmlRpcValue::TypeInt:
			return new de_facto_int((int) xv);
		case XmlRpcValue::TypeDouble:
			return new de_facto_double((double) xv);
		case XmlRpcValue::TypeString:
			return new de_facto_string(add_quotes((string) xv));
		case XmlRpcValue::TypeDateTime:
			{
			struct tm T((struct tm) xv);
			return new de_facto_time(std::pair<long, long>(mktime(&T), 0));
			}
		case XmlRpcValue::TypeArray:
			return NULL;
		case XmlRpcValue::TypeStruct:
			return NULL;
		default:
			return new de_facto_string("undefined");
	}
	return NULL;
}

class editActivity : public XmlRpcServerMethod {
public:
	editActivity(XmlRpcServer *s) : XmlRpcServerMethod("editActivity", s) {}

	/* editActivity RESULT TYPE:
	 *
	 *	on success: string 'OK'
	 *
	 *	on failure: string 'ERROR: message'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		string		theID;
		int		i, N;
		list<string>	range_list;
		apcoreArray	editvals;
		XmlRpcValue::ValueStruct::iterator item;

		if(debug_commands) {
			std::cout << "editActivity called; params = " << params << "\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(   params.size() != 2
		   || params[0].getType() != XmlRpcValue::TypeString
		   || params[1].getType() != XmlRpcValue::TypeStruct
		  ) {
			throw XmlRpcException("Bad parameters: expected string, struct", XmlRpcException::BADPARTYPE);
		}
		theID = (string) params[0];
		XmlRpcValue::ValueStruct theStruct = (XmlRpcValue::ValueStruct) params[1];
#		ifdef APcoreDEBUG
		cout << "return param # 1: " << params[1] << endl;
#		endif /* APcoreDEBUG */
		for(	item = theStruct.begin();
			item != theStruct.end();
			item++) {
			std::string	stringvalue;
			std::string	correctName;
			XmlRpcValue& value(item->second);

#			ifdef APcoreDEBUG
			cout << "theStruct[" << item->first << "] = " << item->second << endl;
#			endif /* APcoreDEBUG */
			stringvalue = xml2serialized_act_symbol(theID, item->first, correctName, value);
			editvals.push_back(new de_facto_string(
					correctName,
					"",
					range_list,
					stringvalue,
					"string"));
		}
		if(APcoreIntfc::editActivity(theID, editvals)) {
			throw XmlRpcException(string("ERROR trying to edit activity ") + theID, XmlRpcException::NOSUCHID); }
		result = string("OK");
	}

	std::string help() {
		return std::string(
			"Takes 2 args: act ID (string) and kwd/value pairs (array). Returns 'OK' or 'ERROR: msg'.\n");
	}
} edit_activity(&theServer());

class actDurIsEditable : public XmlRpcServerMethod {
public:
	actDurIsEditable(XmlRpcServer *s) : XmlRpcServerMethod("actDurIsEditable", s) {}

	/* actDurIsEditable RESULT TYPE:
	 *
	 *	on success: boolean true or false
	 *
	 *	on failure: throws exception
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		string			id;
		XmlRpc::XmlRpcValue	theValue;

		if(debug_commands) {
			std::cout << "actDurIsEditable called; params = " << params << "\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(   params.size() != 1
		   || params[0].getType() != XmlRpcValue::TypeString
		  ) {
			throw XmlRpcException("Bad parameters: expected 1 string", XmlRpcException::BADPARTYPE); }
		id = (string) params[0];
		if(isActDurEditableAPI(id, theValue)) {
			throw XmlRpcException(string("ERROR finding whether duration can be edited for act. ") + id, XmlRpcException::NOSUCHID); }
		result = theValue;
	}

	std::string help() {
		return std::string(
			"Takes 1 arg: act ID (string). Returns boolean value or throws exception.\n");
	}
} isActDurEditable(&theServer());

class regenChildren : public XmlRpcServerMethod {
public:
	regenChildren(XmlRpcServer *s) : XmlRpcServerMethod("regenChildren", s) {}

	/* editActivity RESULT TYPE:
	 *
	 *	on success: string 'OK'
	 *
	 *	on failure: string 'ERROR: message'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		string		theID;
		string		any_error;
		int		i, N;
		XmlRpcValue::ValueStruct::iterator item;

		if(debug_commands) {
			std::cout << "regenChildren called; args = " << params << "\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(   params.size() != 1
		   || params[0].getType() != XmlRpcValue::TypeString) {
			throw XmlRpcException("ERROR: expected 1 parameter (<id string>)", XmlRpcException::BADPARNUM);
			return;
		}
		theID = (string) params[0];
		if(APcoreIntfc::regenChildren(theID, any_error)) {
			throw XmlRpcException(string("ERROR trying to regen children for activity ") + theID + ": " + any_error,
				XmlRpcException::MODELERR);
		}
		result = string("OK");
	}

	std::string help() {
		return std::string(
			"Takes 1 arg: act ID (string). Returns 'OK' or 'ERROR: msg'.\n");
	}
} regen_children(&theServer());

class editActivityStringParams : public XmlRpcServerMethod {
public:
	editActivityStringParams(XmlRpcServer *s) : XmlRpcServerMethod("editActivityStringParams", s) {}

	/* editActivity RESULT TYPE:
	 *
	 *	on success: string 'OK'
	 *
	 *	on failure: string 'ERROR: message'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		string		theID;
		int		i, N;
		list<string>	range_list;
		apcoreArray	editvals;
		XmlRpcValue::ValueStruct::iterator item;

		if(debug_commands) {
			std::cout << "editActivityStringParams called; args = " << params << "\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(   params.size() != 2
		   || params[0].getType() != XmlRpcValue::TypeString
		   || params[1].getType() != XmlRpcValue::TypeStruct
		  ) {
			throw XmlRpcException("ERROR: expected 2 parameters (<id string>, <kwd-value struct>)", XmlRpcException::BADPARNUM);
		}
		theID = (string) params[0];
		XmlRpcValue::ValueStruct theStruct = (XmlRpcValue::ValueStruct) params[1];
#		ifdef APcoreDEBUG
		cout << "return param # 1: " << params[1] << endl;
#		endif /* APcoreDEBUG */
		for(	item = theStruct.begin();
			item != theStruct.end();
			item++) {
#			ifdef APcoreDEBUG
			cout << "theStruct[" << item->first << "] = " << item->second << endl;
#			endif /* APcoreDEBUG */
			editvals.push_back(new de_facto_string(
						item->first,
						"",
						range_list,
						(string) item->second,
						"string"));
		}
		if(APcoreIntfc::editActivity(theID, editvals)) {
			throw XmlRpcException(string("ERROR trying to edit activity ") + theID, XmlRpcException::NOSUCHID); }
		result = string("OK");
	}

	std::string help() {
		return std::string(
			"Takes 2 args: act ID (string) and kwd/value pairs (array).\n"
			"The values in the keyword/value pairs should be XMLRPC strings\n"
			"that can be parsed into APGEN data types. Returns 'OK' or 'ERROR: msg'.\n");
	}
} edit_activity_string_params(&theServer());

class getConsumption : public XmlRpcServerMethod {
public:
	getConsumption(XmlRpcServer *s) : XmlRpcServerMethod("getConsumption", s) {}

	/* getConsumption RESULT TYPE:
	 *
	 *	on success: double
	 *
	 *	on failure: string 'ERROR: ...'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		double total = 0.0;
		bool isUsed = false;
		string any_errors;

		if(debug_commands) {
			std::cout << "getConsumption called; args = " << params << "\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(   params.size() != 2
		   || params[0].getType() != XmlRpcValue::TypeString
		   || params[1].getType() != XmlRpcValue::TypeString
		  ) {
			throw XmlRpcException("ERROR: expected 2 string parameters containing act. ID and resource name.",
				XmlRpcException::BADPARNUM);
		}
		if(APcoreIntfc::getConsumption((string) params[0], (string) params[1], total, isUsed, any_errors)) {
			throw XmlRpcException(any_errors, XmlRpcException::MODELERR);
		}
		result = total;
	}

	std::string help() {
		return std::string("Takes 2 string args, the activity ID and the resource name;\n"
				"returns consumption for that activity as a double-precision numeric value.\n");
	}
} get_consumption(&theServer());

class getIntegratedConsumption : public XmlRpcServerMethod {
public:
	getIntegratedConsumption(XmlRpcServer *s) : XmlRpcServerMethod("getIntegratedConsumption", s) {}

	/* getConsumption RESULT TYPE:
	 *
	 *	on success: double
	 *
	 *	on failure: string 'ERROR: ...'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		double total = 0.0;
		bool isUsed = false;
		string any_errors;

		if(debug_commands) {
			std::cout << "getIntegratedConsumption called; args = " << params << "\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(   params.size() != 2
		   || params[0].getType() != XmlRpcValue::TypeString
		   || params[1].getType() != XmlRpcValue::TypeString
		  ) {
			throw XmlRpcException("ERROR: expected 2 string parameters containing act. ID and resource name.",
				XmlRpcException::BADPARNUM);
		}
		if(APcoreIntfc::getIntegratedConsumption((string) params[0], (string) params[1], total, isUsed, any_errors)) {
			throw XmlRpcException(any_errors, XmlRpcException::MODELERR);
		}
		result = total;
	}

	std::string help() {
		return std::string("Takes 2 string args, the activity ID and the resource name;\n"
				"returns integrated consumption for that activity as a double-precision numeric value.\n");
	}
} get_integrated_consumption(&theServer());

class deleteActivity : public XmlRpcServerMethod {
public:
	deleteActivity(XmlRpcServer *s) : XmlRpcServerMethod("deleteActivity", s) {}

	/* deleteActivity RESULT TYPE:
	 *
	 *	on success: string 'OK'
	 *
	 *	on failure: string 'ERROR'
	 */
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		string errors;

		if(debug_commands) {
			std::cout << "deleteActivity called; args = " << params << "\n";
			std::cout.flush();
		}
		if(!APcoreInit::was_APcore_initialized()) {
			throw XmlRpcException("APcore was not initialized", XmlRpcException::UNDERINIT);
		}
		if(   params.size() != 1
		   || params[0].getType() != XmlRpcValue::TypeString
		  ) {
			throw XmlRpcException("ERROR: expected 1 string parameter containing the activity ID.", XmlRpcException::BADPARNUM);
		}

#ifdef OBSOLETE
		if(APcoreIntfc::deleteActivity((string) params[0], errors)) {
			string err("ERROR: could not find activity with ID ");
			err = err + (string) params[0];
			throw XmlRpcException(err, XmlRpcException::NOSUCHID);
		}
#endif /* OBSOLETE */

		result = "OK";
	}

	std::string help() {
		return std::string("Deletes an activity. Takes 1 arg, the activity ID.\n");
	}
} delete_activity(&theServer());


class exitAPcore: public XmlRpcServerMethod {
public:
	exitAPcore(XmlRpcServer *s) : XmlRpcServerMethod("exit", s ) {}
	void execute(XmlRpcValue &, XmlRpcValue &result) {
		if(debug_commands) {
			std::cout << "exitAPcore called\n";
			std::cout.flush();
		}

		result = "OK Bye";
		if(APcoreInit::was_APcore_initialized()) {
	  		APcoreIntfc::terminate();
		}
		theServer().setExitFlag();
	}
	std::string help() {
		return std::string("Causes APcore to exit gracefully.\n");
	}
} exitMethod(&theServer());

class sitAndWait : public XmlRpcServerMethod {
public:
	sitAndWait(XmlRpcServer *s) : XmlRpcServerMethod("sitAndWait", s) {}
	void execute(XmlRpcValue &params, XmlRpcValue &result) {
		char t[100];

		// allow client to use this before APcore initialization -- 
		// useful e. g. for session sever:

		if(debug_commands) {
			std::cout << "sitAndWait called; args = " << params << "\n";
			std::cout.flush();
		}
		if(params.size() == 2) {
			if(params[0].getType() != XmlRpcValue::TypeInt) {
				throw XmlRpcException(
					"ERROR: expected param 1 to be an integer representing seconds.",
					XmlRpcException::BADPARTYPE); }
			if(params[1].getType() != XmlRpcValue::TypeString) {
				throw XmlRpcException(
					"ERROR: expected param 2 to be a string requesting the run time dir.",
					XmlRpcException::BADPARTYPE);
			}
			result = theRunTimeDir();
			return;
		} else if(   params.size() != 1
		   || params[0].getType() != XmlRpcValue::TypeInt
		  ) {
			result = string("ERROR: expected 1 integer parameter representing seconds.");
			return;
		}
		sprintf(t, "Finished sleeping for %ld seconds", (long) params[0]);
		result = t;
		sleep((long) params[0]);
	}
	std::string help() {
		return std::string("Sleeps the requested number of seconds.\n");
	}
} sit_and_wait_handler(&theServer());


extern int	thread_main(int, char**);
static char**	apcore_argv = NULL;
static int	apcore_argc = 0;
static std::thread* apcore_thread = NULL;


int APcoreIntfc::terminate() {

#ifdef PREMATURE

	/*
	 * NOTE: as long as APinit has been called, it is always the case
	 *	 that the waiter().usingAPcore mutex is locked when _any_ interface
	 *	 function is called. After making its request, the interface
	 *	 function can then do a pthread_cond_wait that will reacquire
	 *	 waiter().usingAPcore, paving the way for the next request.
	 */

	waiter().getTheCommand() = new QUITrequest();
	pthread_cond_signal(&waiter().signalRequestReady);
	pthread_cond_wait(&waiter().signalApcoreReady, &waiter().usingAPcore);

	/*
	 * Most interface functions don't do this; they keep the mutex
	 * locked until the next interface function releases it through
	 * its call to pthread_cond_wait.
	 */

	pthread_mutex_unlock(&waiter().usingAPcore);
#endif /* PREMATURE */

	if(apcore_argv) {
		for(int i = 0; i < apcore_argc; i++) {
			free(apcore_argv[i]);
		}
		free((char*)apcore_argv);
	}

	return 0;
}

int APcoreIntfc::initialize(
		const vector<string>&	args,
		string&			errors) {
	apcore_argv = (char**)malloc(sizeof(char*) * args.size());
	apcore_argc = args.size();

	for(int i = 0; i < args.size(); i++) {
		apcore_argv[i] = strdup(args[i].c_str());
	}

	//
	// Here, we launch thread_main as a genuine
	// execution thread, in contrast with apbatch
	// and atm which call it as a regular function.
	//
	apcore_thread = new std::thread(
		thread_main, args.size(), apcore_argv);

	return 0;
}

#endif /* REMOVE_ALL_METHODS */


int usage(const char* a, const char* b) {
    if(b && b[0]) {
	std::cerr << "Did not understand option \"" << b << "\"." << endl
	   << "Usage: " << a
	   << " [-d] [-verbosity <integer>] [-debug-commands] <port>\n";
    
    } else {
	std::cerr << "Usage: " << a
	    << " [-d] [-verbosity <integer>] [-debug-commands] <port>\n";
    }
    return -1;
}

int main(int argc, char* argv[]) {
	int		portIndex = 1;
	int		port = -1;
	int		notifier_port_arg = -1;
	int		verbosity = 0;
	int		i;
	bool		shouldStartNotifier = false;
	bool		daemon_option = false;

			//
			// default: timeout = 12 hours
			//
	double		timeOutInSeconds = 43200.0;
	int		waitTimeInSeconds = -1;

	for(i = 1; i < argc; i++) {

		//
		// -daemon option
		//
		if(!strcmp(argv[i], "-d")) {
			daemon_option = true;
		} else if(!strcmp(argv[i], "-debug-commands")) {
			debug_commands = true;
		} else if((!strcmp(argv[i], "-n")) && (i < argc - 1)) {
			shouldStartNotifier = true;
			notifier_port_arg = i + 1;
			i++;
		} else if((!strcmp(argv[i], "-verbosity")) && (i < argc - 1))  {
			portIndex += 2;
			verbosity = atoi(argv[i + 1]);
			i++;
		} else if((!strcmp(argv[i], "-timeout")) && (i < argc - 1)) {
			sscanf(argv[i+1], "%lf", &timeOutInSeconds);
			i++;
		} else if((!strcmp(argv[i], "-runtimedir")) && (i < argc - 1)) {
			theRunTimeDir() = argv[i + 1];
			i++;
		}

		//
		// suppress leading quotes; PSI adds them if you have complex options:
		//
		else if(((!strcmp(argv[i], "-wait")) || (!strcmp(argv[i], "\"-wait"))) && (i < argc - 1)) {
			char *A = argv[i+1];
			int l = strlen(A);
			char *t = (char *) malloc(l + 1);

			//
			// suppress trailing quotes
			//
			if(A[l-1] == '"') {
				A[l-1] = '\0';
			}
			sscanf(A, "%d", &waitTimeInSeconds);
			free(t);
			i++;
		} else if(i == argc - 1) {
			char* dd = argv[i];
			while(*dd) {
				if(!isdigit(*dd)) {
					return usage(argv[0], argv[i]);
				}
				dd++;
			}
			port = atoi(argv[i]);
		} else {
			return usage(argv[0], argv[i]);
		}
	}

	if(port < 0) {
		return usage(argv[0], "");
	}

	if(waitTimeInSeconds > 0) {

		//
		// debug
		//
		cerr << "waiting for " << waitTimeInSeconds << "...";
		sleep(waitTimeInSeconds);

		//
		// debug
		//
		cerr << " done.\n";
	}

	if(daemon_option) {
			fclose(stdout);
			fclose(stdin);
			fclose(stderr);
	}
	XmlRpc::setVerbosity(verbosity);

	//
	// Create the server socket on the specified port
	//
	theServer().bindAndListen(port);

	//
	// Enable introspection
	//
	theServer().enableIntrospection(true);

	//
	// Wait for requests for indicated no. of seconds (forever if = -1)
	//
	theServer().work(timeOutInSeconds);

#ifdef OBSOLETE
	if(apcore_thread && apcore_thread->joinable()) {
		apcore_thread->join();
	}
#endif /* OBSOLETE */

	return 0;
}
