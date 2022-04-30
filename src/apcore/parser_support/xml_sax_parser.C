#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>

#include <ActivityInstance.H>
#include <APsax.H>
#include <stdlib.h>	// for atoi()
#include <fstream>
#include <iostream>

// #define DEBUG_THE_READER

#include <xml_sax_parser.H>
// #include <act_plan_parser.H>

// #include <flexval.H>


extern pthread_mutex_t& builderProcessingRecord();

extern pthread_cond_t& parserGotToEndOfRecord();

extern pthread_mutex_t& parserDoingItsJob();

extern void lock_parserDoingItsJob();

extern void unlock_parserDoingItsJob();

extern pthread_cond_t& builderWantsToIterate();


bool	XmlTolSaxParser::context_includes(const string& s) {
	if(inElement.find(s) != inElement.end() && inElement[s]) {
		return true;
	}
	return false;
}

void	XmlTolSaxParser::main_process_waits_for_thread_to_iterate(bool first_time /* = false */ ) {
	if(!first_time) {
		pthread_cond_signal(&parserGotToEndOfRecord());
	}

	//
	// the following releases both mutexes, allowing the builder thread to
	// start. It also guarantees that we are ready to receive
	// the builder's message the first time it gets to iterate(): */
	//
	if(!error_found) {

#ifdef DEBUG_THE_READER
		cout << "(main) " << indent_by_depth() << "xml_sax_parser::parse(): "
			<< "about to wait for builderWantsToIterate()\n";
		cout.flush();
#endif

		pthread_mutex_unlock(&builderProcessingRecord());
		pthread_cond_wait(&builderWantsToIterate(), &parserDoingItsJob());

#ifdef DEBUG_THE_READER
		cout << "LOCKED (cond_wait)\n";
#endif /* DEBUG_THE_READER */

		pthread_mutex_lock(&builderProcessingRecord());
	} else {

#		ifdef DEBUG_THE_READER
		cout << "(main) " << indent_by_depth() << "xml_sax_parser::parse():"
			<< " errors found. returning from main_process_waits...()\n";
		cout.flush();
#		endif

	}
}


void	XmlTolSaxParser::plan_building_thread_waits_for_main_parser_to_get_to_end_of_record(
			bool last_time /* = false */ ) {

#ifdef DEBUG_THE_READER
	cout << "(thread) iterate(): about to signal builderWantsToIterate()\n";
	cout.flush();
#endif

	//
	// we know we can send the signal, because we were able
	// to get here only when the parser released parserDoingItsJob()
	// and we grabbed it either at the start of plan_builder or below
	//
	pthread_cond_signal(&builderWantsToIterate());

#ifdef DEBUG_THE_READER
	cout << "(thread) iterate(): about to unlock parserDoingItsJob()";
	if(last_time) {
		cout << "\n";
	} else {
		cout << " and wait on parserGotToEndOfRecord()\n";
	}
	cout.flush();
#endif

	unlock_parserDoingItsJob();
	if(last_time) {
		pthread_mutex_unlock(&builderProcessingRecord());

#ifdef DEBUG_THE_READER
		cout << "(thread) iterate(): unlocked builderProcessingRecord()\n";
#endif /* DEBUG_THE_READER */

	} else {
		pthread_cond_wait(&parserGotToEndOfRecord(), &builderProcessingRecord());

#		ifdef DEBUG_THE_READER
		cout << "(thread) iterate(): woke up from parserGotToEndOfRecord...\n";
		cout << "(thread) iterate(): OK, got parserGotToEndOfRecord()...\n";
		cout << "    (thread) about to lock parserDoingItsJob()...\n";
#		endif

		lock_parserDoingItsJob();

#		ifdef DEBUG_THE_READER
		cout << "    (thread) OK, locked parserDoingItsJob(). Exiting iterate(true).\n";
		cout.flush();
#		endif

		}
	}

	/* This method is used by the main sax-parsing process */
void XmlTolSaxParser::at_end_of_tol_record() {

	if(error_found) {

#ifdef DEBUG_THE_READER
		cout << "at_end_of_tol_record: error found - throwing exception\n";
		cout.flush();
#endif

		Cstring	errtext;
		errtext << "at_end_of_tol_record(): error was found in building thread - " << error_message << "\n";
		throw(exception(*errtext, line, col)); }

#ifdef DEBUG_THE_READER
	cout << "(main) at_end_of_tol_record: no error found\n";
	cout.flush();
#endif

	main_process_waits_for_thread_to_iterate();
	}

XmlTolSaxParser::XmlTolSaxParser()
	: sax_parser(),
		error_found(false),
		cur_depth(0),
		index_level(0)
	{
	}

XmlTolSaxParser::~XmlTolSaxParser() {}
	

void XmlTolSaxParser::resize_all(int maxindex) {
	if(compoundType.size() <= maxindex) {
		compoundType.resize(maxindex + 1); }
	if(curindex.size() <= maxindex) {
		curindex.resize(maxindex + 1); }
	if(curval.size() <= maxindex) {
		curval.resize(maxindex + 1); }
	}

	// (main) parser uses this
void XmlTolSaxParser::on_start_document() {
#	ifdef DEBUG_THE_READER
	cout << "(main) " << indent_by_depth() << "on_start_document()" << endl;
#	endif
	resize_all(0);
	reached_the_end = false;
	}

	// (main) parser uses this
void XmlTolSaxParser::on_end_document() {
#	ifdef DEBUG_THE_READER
	cout << "(main) " << indent_by_depth() << "on_end_document()" << endl;
#	endif
	reached_the_end = true;

#	ifdef DEBUG_THE_READER
	cout << "(main) " << indent_by_depth() << "on_end_document: about to call main_process_waits...()\n";
#	endif

	main_process_waits_for_thread_to_iterate(false);
#	ifdef DEBUG_THE_READER
	cout << "(main) " << indent_by_depth() << "on_end_document: returned from main_process_waits...(); returning.\n";
	cout.flush();
#	endif
	}

void XmlTolSaxParser::on_start_element(const string& name,
		const vector<pair<string, string> >& attributes_in_order,
		const map<string, string>& attributes) {
#	ifdef DEBUG_THE_READER
	static int count = 0;
#	endif

	inElement[name] = true;

	cur_depth++;
	resize_all(cur_depth);

	// debug
#	ifdef DEBUG_THE_READER
	cout << "(main) " << indent_by_depth() << "on_start_element(depth = "
		<< cur_depth << " name = " << name << ")\n";
#	endif

	if(name == "TOLrecord") {
		resIndices.clear(); }
	else if(name == "ResourceSpec") {
		resIndices.clear(); }

	if(name == "Index") {
		// look for level attribute
		map<string, string>::const_iterator	iter = attributes.find("level");
		if(iter != attributes.end()) {
				index_level = atoi(iter->second.c_str()); } }
	else if(name.find("Value") != string::npos) {
		inElement["AnyValue"] = true;
		if(name == "BooleanValue") {}
		else if(name == "InstanceValue") {}
		else if(name == "IntegerValue") {}
		else if(name == "StringValue") {}
		else if(name == "DurationValue") {}
		else if(name == "TimeValue") {}
		else if(name == "ListValue" || name == "EmptyListValue") {
			compoundType[cur_depth] = "list"; }
		else if(name == "StructValue") {
			compoundType[cur_depth] = "struct"; }
		else if(name == "EmptyListValue") {}
		else if(name == "UnknownValue") {}
		else if(name == "UnitializedValue") {}
		}
	else if(name == "Element") {
		// look for index attribute
		map<string, string>::const_iterator	iter = attributes.find("index");
		// debug
#		ifdef DEBUG_THE_READER
		cout << "Looking for \"index\" attribute in Element tag...\n";
#		endif
		if(iter != attributes.end()) {
#			ifdef DEBUG_THE_READER
			cout << "found it, value = " << iter->second << "\n";
#			endif
			if(compoundType[cur_depth - 1] == "list") {
				// debug
#				ifdef DEBUG_THE_READER
				cout << "setting curindex[" << cur_depth << "] to "
					<< atol(iter->second.c_str()) << "\n";
#				endif
				curindex[cur_depth] = atol(iter->second.c_str()); }
			else {
				// debug
#				ifdef DEBUG_THE_READER
				cout << "setting curindex[" << cur_depth << "] to "
					<< iter->second << "\n";
#				endif
				curindex[cur_depth] = iter->second; } }
#		ifdef DEBUG_THE_READER
		else {
			//debug
			cout << "did not find it !? All attributes:\n";
			for(iter = attributes.begin(); iter != attributes.end(); iter++) {
				cout << "\t" << iter->first << " = " << iter->second << "\n";
				}
			}
		cout.flush();
#		endif
		}
	else if(name == "Instance") {
		actName = "";
		actID = "";
		actType = "";
		actParent = "";
		actVisibility = ""; }
	else if(name == "TOLrecord") {
		recordType = "";

		// assert(cur_depth == 0);
		// if(curval.size() < 1) {
		// 	curval.resize(1); }

		// look for record type
		map<string, string>::const_iterator	iter = attributes.find("type");
		if(iter != attributes.end()) {
			if(iter->second == "ACT_START") {
				recordType = "activity"; }
			else if(iter->second == "RES_VAL" || iter->second == "RES_FINAL_VAL") {
				recordType = "resource"; }
			else if(iter->second == "ACT_END") {
				recordType = "activity end"; } }
#		ifdef DEBUG_THE_READER
		else {
			cout << "?! type is not defined for TOLrecord!?\n";
			}
#		endif

		// debug
#		ifdef DEBUG_THE_READER
		cout << "(main) " << indent_by_depth() << "Confirming that name = TOLrecord. type = "
			<< recordType << "\n";
		if((count % 1000) == 0) {
			cout << "\rcount: " << count; }
		count++;
#		endif
		}
	if(context_includes("Instance")) {
		if(name == "Attributes") {
			instanceAttributes.clear(); }
		else if(name == "Parameters") {
			parameters.clear(); } }
	}

void XmlTolSaxParser::on_end_element(const string& name) {
	inElement[name] = false;

	// debug
#	ifdef DEBUG_THE_READER
	cout << "(main) " << indent_by_depth() << "on_end_element(depth = " << cur_depth << " name = " << name << ")\n";
	cout.flush();
#	endif

	// if(name == "Attribute") {
	// 	bool caught = true;
	// 	}
	if(name == "ResourceSpec" || name == "Resource") {
		// debug
		// cout << "resName = " << resName << "\n";
		// cout.flush();

		fullResName.str("");
		fullResName << resName;
		for(int i = 0; i < resIndices.size(); i++) {
			fullResName << "[" << addQuotes(resIndices[i]) << "]"; }
#		ifdef DEBUG_THE_READER
		cout << "(main) " << indent_by_depth() << "full resource name = " << fullResName.str() << "\n";
		cout << "(main) " << indent_by_depth() << "\tres data type = " << resDataType << "\n";
		cout << "(main) " << indent_by_depth() << "\tres interpolation = " << resInterpolation << "\n";
#		endif
		resName.clear();
		resDataType.clear();
		resInterpolation.clear();
		}
	else if(name == "TOLrecord") {
		at_end_of_tol_record(); }
	else if(name == "EmptyListValue") {
#		ifdef DEBUG_THE_READER
		cout << "setting empty list value\n";
#		endif
		flexval&	cur(curval[cur_depth]);
		cur.clear();
		cur[0] = "empty";
		cur.get_array().erase(cur.get_array().begin()); }
	else if(name.find("Value") != string::npos) {
		inElement["AnyValue"] = false; }
	else if(name == "Element") {
		flexval&	compound(curval[cur_depth - 1]);

		if(compoundType[cur_depth - 1] == "list") {
#			ifdef DEBUG_THE_READER
			cout << "curindex[" << cur_depth << "] type is "
				<< flexval::type2string(curindex[cur_depth].getType())
				<< " (should be int)\n";
			cout.flush();
#			endif
			assert(curindex[cur_depth].getType() == flexval::TypeInt); }
		else if(compoundType[cur_depth - 1] == "struct") {
#			ifdef DEBUG_THE_READER
			cout << "curindex[" << cur_depth << "] type is "
				<< flexval::type2string(curindex[cur_depth].getType())
				<< " (should be string)\n";
			cout.flush();
#			endif
			assert(curindex[cur_depth].getType() == flexval::TypeString); }

#		ifdef DEBUG_THE_READER
		cout << "    setting curval[" << (cur_depth - 1) << "][" << curindex[cur_depth] << "] to "
			<< curval[cur_depth + 1] << "\n";
		cout.flush();
#		endif

		if(compoundType[cur_depth - 1] == "list") {
#			ifdef DEBUG_THE_READER
			cout << "compound type: " << flexval::type2string(compound.getType()) << "\n";
			cout.flush();
#			endif
			assert(compound.getType() == flexval::TypeArray
				|| compound.getType() == flexval::TypeInvalid);
			long int indexval = curindex[cur_depth];
#			ifdef DEBUG_THE_READER
			cout << "Setting compound[" << indexval << "]...\n";
#			endif
			compound[indexval] = curval[cur_depth + 1]; }
		else {
#			ifdef DEBUG_THE_READER
			cout << "compound type: " << flexval::type2string(compound.getType()) << "\n";
			cout.flush();
#			endif
			assert(compound.getType() == flexval::TypeStruct
				|| compound.getType() == flexval::TypeInvalid);
			compound[(string)curindex[cur_depth]] = curval[cur_depth + 1]; }
		}
	else if(name == "Attribute") {
#		ifdef DEBUG_THE_READER
		cout << "(main) " << indent_by_depth() << "attribute " << attributeName
			<< " = " << curval[cur_depth + 1] << "; cur_depth = " << cur_depth << "\n";
		cout.flush();
#		endif
		instanceAttributes.push_back(pair<string, flexval>(attributeName, curval[cur_depth + 1])); }
	else if(name == "Parameter") {
#		ifdef DEBUG_THE_READER
		cout << "(main) " << indent_by_depth() << "parameter " << parameterName
			<< " = " << curval[cur_depth + 1] << "; cur_depth = " << cur_depth << "\n";
		cout.flush();
#		endif
		parameters.push_back(pair<string, flexval>(parameterName, curval[cur_depth + 1])); }
	cur_depth--;
	}

void XmlTolSaxParser::on_characters(const string& text) {

	// debug
#	ifdef DEBUG_THE_READER
	cout << "(main) " << indent_by_depth() << "on_characters: text.size() = "
		<< text.size() << ", text = \"" << text << "\"\n";
#	endif

	if(text.size() == 0) return;

	if(	context_includes("ResourceSpec")
		|| context_includes("Resource")
	  ) {
		if(context_includes("Index")) {
			// debug
#			ifdef DEBUG_THE_READER
			cout << "(main) " << indent_by_depth() << "grabbing resource Index\n";
#			endif
			resIndices.push_back(text); }
		else if(context_includes("Name")) {
			// debug
#			ifdef DEBUG_THE_READER
			cout << "(main) " << indent_by_depth() << "grabbing resource Name\n";
#			endif
			resName = text; } }
	else if(context_includes("Instance")) {

		// debug
#		ifdef DEBUG_THE_READER
		cout << "(main) " << indent_by_depth() << "on_characters(): we are inside an Instance\n";
#		endif

		if(context_includes("Attribute")) {
			if(context_includes("Name")) {
				// debug
#				ifdef DEBUG_THE_READER
				cout << "(main) " << indent_by_depth() << "grabbing attribute name\n";
#				endif
				attributeName = text; } }
		else if(context_includes("Parameter")) {
			if(context_includes("Name")) {
				// debug
#				ifdef DEBUG_THE_READER
				cout << "(main) " << indent_by_depth() << "grabbing parameter name\n";
#				endif
				parameterName = text; } }
		else if(context_includes("Name")) {
			// debug
#			ifdef DEBUG_THE_READER
			cout << "(main) " << indent_by_depth() << "grabbing Name for sth other than att/param\n";
#			endif
			actName = text; }
		else if(context_includes("Parent")) {
			// debug
#			ifdef DEBUG_THE_READER
			cout << "(main) " << indent_by_depth() << "grabbing Parent\n";
#			endif
			actParent = text; }
		else if(context_includes("Visibility")) {
			// debug
#			ifdef DEBUG_THE_READER
			cout << "(main) " << indent_by_depth() << "grabbing Visibility\n";
#			endif
			actVisibility = text; }
		else if(context_includes("ID")) {
			// debug
#			ifdef DEBUG_THE_READER
			cout << "(main) " << indent_by_depth() << "grabbing ID\n";
#			endif
			actID = text; }
		else if(context_includes("Type")) {
			// debug
#			ifdef DEBUG_THE_READER
			cout << "(main) " << indent_by_depth() << "grabbing Type\n";
#			endif
			actType = text; }
		}
	else if(context_includes("TimeStamp")) {
		record_time = CTime_base(text.c_str());
#		ifdef DEBUG_THE_READER
		cout << "(main) " << indent_by_depth() << "grabbing time stamp = " << text << "\n";
#		endif
		}
	if(context_includes("AnyValue")) {

		// debug
#		ifdef DEBUG_THE_READER
		cout << "(main) " << indent_by_depth() << "(in AnyValue) ";
#		endif

		if(context_includes("BooleanValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting boolean value\n";
#			endif
			if(text == "true" || text == "TRUE") {
				curval[cur_depth] = true; }
			else {
				curval[cur_depth] = false; }
			}
		else if(context_includes("IntegerValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting integer value\n";
#			endif
			long	L;
			sscanf(text.c_str(), "%ld", &L);
			curval[cur_depth] = L; }
		else if(context_includes("DoubleValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting double value\n";
#			endif
			double	D;
			sscanf(text.c_str(), "%lf", &D);
			curval[cur_depth] = D; }
		else if(context_includes("InstanceValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting instance value\n";
#			endif
			/* hopefully setting an instance TypedValue
			 * to this string will be OK */
			curval[cur_depth] = text; }
		else if(context_includes("StringValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting string value\n";
#			endif
			curval[cur_depth] = text; }
		else if(context_includes("DurationValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting duration value\n";
#			endif
			cppDuration	d(text.c_str());
			curval[cur_depth] = d; }
		else if(context_includes("TimeValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting time value\n";
#			endif
			cppTime		t(text.c_str());
			curval[cur_depth] = t; }
		else if(context_includes("ListValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting list value [" << cur_depth << "]\n";
#			endif
			curval[cur_depth].clear();
			}
		else if(context_includes("StructValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting struct value [" << cur_depth << "]\n";
#			endif
			curval[cur_depth].clear();
			}
#ifdef USELESS_BECAUSE_EMPTY_LIST_HAS_NO_CHARACTERS
		else if(context_includes("EmptyListValue")) {
#			ifdef DEBUG_THE_READER
			cout << "setting empty list value (doing nothing)\n";
#			endif
			flexval&	cur(curval[cur_depth]);
			cur.clear();
			cur[0] = "empty";
			cur.get_array().erase(cur.get_array().begin());
			}
#endif /* USELESS_BECAUSE_EMPTY_LIST_HAS_NO_CHARACTERS */
		}
	}
