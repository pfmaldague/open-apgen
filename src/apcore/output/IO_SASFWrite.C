#include <unistd.h>

#include "action_request.H"
#include "ActivityInstance.H"
#include "IO_SASFWriteImpl.H"
#include "IO_ApgenDataOptions.H"
#include "aafReader.H"

#include <fstream>
#include <sys/time.h>
#include <memory>
#include <algorithm>
#include <cctype>
#include <stdio.h>
#include <iostream>


#ifdef USE_AUTO_PTR
	#define AUTO_PTR_TEMPL auto_ptr
#else
	#define AUTO_PTR_TEMPL unique_ptr
#endif

using namespace std;

bool debug_this::yes_no = false;
int debug_this::indentation = 0;

void debug_this::indent() {
	if(yes_no) {
		int k = 0;
		for(; k < indentation; k++) {
			cout << " ";
		}
	}
}


//
// SASF write constructor
//
IO_SASFWriteImpl::IO_SASFWriteImpl() {
	NextActivityInstanceID = 0;

	//
	// This creates a Plan with a fake ActivityInstance at the top
	//
	Plan = new SASF_ActivityInstance(NextActivityInstanceID++);
	SASF_Step::Init();
}

IO_SASFWriteImpl& IO_SASFWriteImpl::SASFwriter() {
	static IO_SASFWriteImpl s;
	return s;
}


//
// writes the sasf - throws eval_error if problems found
//
void IO_SASFWriteImpl::WriteSASF(
		IO_SASFWriteOptions& options) {
    CTime_base	evStart(options.GetStartTime());
    CTime_base	evEnd(options.GetEndTime());
    debug_indenter	I;

    if(debug_this::yes_no) {
	debug_this::indent();
	cout << "IO_SASFWriteImpl::WriteSASF start\n";
    }

    //store options in the class
    DesiredOutputFileName = options.GetFileName();
    SymbolicFilesToInclude = options.GetSymbolicFileNames();

    // StartTime = options.GetStartTime();
    // EndTime = options.GetEndTime();
    StartTime = evStart;
    EndTime = evEnd;

    //whether to include requests that start at the end time
    InclusionFlag = options.GetInclusionFlag();

    //now write the file
    try {
	multiset<SASF_StepSeq, SequenceLess>	requests;
	debug_indenter				J;
	
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "calling AddSequences( Plan )\n";
	}

	//
	// this one can throw an exception - WATCH OUT: filters
	// activities based on criteria... (V_7_7 change)
	//
	AddSequences(&requests, Plan);

	aoString fileToOutput;

	//
	// write Header
	//
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "calling WriteHeader()\n";
	}
	WriteHeader(fileToOutput);
		
	//
	// SASFStepSeqVect = vector<SASF_StepSeq>
	//
	SASFStepSeqVect requestVect;

	//
	// initialize vector
	//
	insert_iterator<SASFStepSeqVect> insertIter(requestVect, requestVect.begin());
	copy(requests.begin(), requests.end(), insertIter);

	//make sure the start times are ok
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "calling UpdateRequestStartTimes()\n";
	}
	UpdateRequestStartTimes(requestVect);

	//sort by startTime
	sort(requestVect.begin(), requestVect.end(), RequestLess()); 

	//will take out the bad ones (based on time)
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "calling FilterRequests()\n";
	}
	FilterRequests(&requestVect);

	if(requestVect.empty())
	{
	  SASFException noSteps("The SASF contains no steps\n");
	  throw noSteps;
	}
		
	//write body
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "calling WriteBody()\n";
	}
	WriteBody(fileToOutput, requestVect);

	ofstream fileOut(DesiredOutputFileName.c_str());

	//write it out
	fileOut << fileToOutput.str();
	fileOut.close();
    } catch(SASFException &ex) {
	string errs(ex.what());
	Cstring cerrs(errs);
	throw(eval_error(cerrs));
    } catch(eval_error Err) {
	Cstring errs;
	errs << "An error occurred while writing the SASF; details:\n"
		<< Err.msg;
	throw(eval_error(errs));
    }

    if(debug_this::yes_no) {
	debug_this::indent();
	cout << "IO_SASFWriteImpl::WriteSASF normal end\n";
    }
}

//
// The main driver of this sasf madness -- recurses down the instance tree
// and then back up.
//
// The method should be used after the "Plan" has been constructed.
//
//
void
IO_SASFWriteImpl::AddSequences(
		SASF_StepSeqMSet* sequences,
		const SASF_ActivityInstance* instance) {

	//
	// call AddSequences recursively (for each of our children)
	//
	const AUTO_PTR_TEMPL<multiset<SASF_StepSeq, SequenceLess> >
			localSequences(new multiset<SASF_StepSeq, SequenceLess>());
	debug_indenter	I;

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "IO_SASFWriteImpl::AddSequences("
			<< instance->GetAPGENID() << ", #steps = " << sequences->size() << ") start\n";
	}
	for(unsigned int i = 0; i < instance->GetNumChildren(); i++) {

		//
		// WATCH OUT! This call results in activity instances
		// being rejected if they fail criteria:
		// (V_7_7 change)
		if(debug_this::yes_no) {
			debug_this::indent();
			cout << "Adding sequence for child " << i << "\n";
		}
		AddSequences(localSequences.get(), instance->GetChildInstance(i));
	}

	//
	// see if there are any sequences for this instance
	// - WATCH OUT: filters based on criteria (V_7_7 change)
	//
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "calling AddInstanceSequence\n";
	}
	AddInstanceSequence(localSequences.get(), instance);

	// combine local sequences that should go together
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "calling CombineSequences\n";
	}
	CombineSequences(localSequences.get());

	// add local sequences to sequences
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "inserting " << localSequences->size() << " sequence(s) into parent\n";
	}
	sequences->insert(localSequences->begin(), localSequences->end());
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "AddSequences done; " << sequences->size() << " sequence(s) so far.\n";
	}
}


void upperCase(string *uppify) {
	string::iterator iter = uppify->begin();

	while(iter != uppify->end()) {
		*iter = toupper(*iter);
		iter++; } }

//for case-insensitive compare
bool ciCharLess(char c1, char c2) {
	return tolower(static_cast<unsigned char>(c1)) <
		tolower(static_cast<unsigned char>(c2)); }

bool ciStringLess(const string &s1, const string &s2) {
 return lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), ciCharLess); }

bool ciStringEqual(const string &s1, const string &s2) {
	//if s1 is not less than s2 and s2 is not less than s1 they are the same

	//could also be accomplished by transforming both to upper or lower and then comparing
	return ((!ciStringLess(s1, s2)) && (!ciStringLess(s2, s1))); }

//functor
struct CIStringEqual:
	std::binary_function<string, string, bool> {
	bool operator()(const string &lhs, const string &rhs) const {
		return ciStringEqual(lhs, rhs); }; };

//for ordering activity instances --we want to traverse them in order
bool SASF_ActivityInstanceLess(SASF_ActivityInstance *first, SASF_ActivityInstance *second) {
	//good enough for now
	const AUTO_PTR_TEMPL<IO_Time> firstTime(first->GetStartTime());
	const AUTO_PTR_TEMPL<IO_Time> secondTime(second->GetStartTime());

	CTime_base firstBase = firstTime->GetEvaluatedTime();
	CTime_base secondBase = secondTime->GetEvaluatedTime();

	return firstBase < secondBase; }

bool SequenceLess::operator()(const SASF_StepSeq &req1, const SASF_StepSeq &req2) const {
	//ordering is first on requestID, then on startTime
	if(req1.GetID() < req2.GetID())
		return true;
	
	if(req1.GetID() > req2.GetID())
		return false;
	
	IO_Time *req1Time = req1.GetStartTime();
	CTime_base base1 = req1Time->GetEvaluatedTime();
	delete req1Time;
	
	IO_Time *req2Time = req2.GetStartTime();
	CTime_base base2 = req2Time->GetEvaluatedTime();
	delete req2Time;
	
	return (base1 < base2); }

bool StepPtrLess::operator()(const SASF_Step *step1, const SASF_Step *step2) const {
	
	/*order by duration first, then by step string, this is to make
		matching sequences match and not miss because steps that happen at the same time
		are ordered differently*/
	IO_Duration dur1 = step1->GetTimeFromSequenceStart();
	IO_Duration dur2 = step2->GetTimeFromSequenceStart();
	
	if(dur1 > dur2)
		return false;

	if(dur1 < dur2)
		return true;
	
	return (step1->GetStepString(0) < step2->GetStepString(0)); }

int ciGetStringAttribute(
		const map<string, TypedValue>&	attrVect,
		const string&			attName,
		string&				stringValue) {
	//iterate through the attributes
	debug_indenter	I;

	map<string, TypedValue>::const_iterator iter;

	for(iter = attrVect.begin(); iter != attrVect.end(); iter++) {

		if(ciStringEqual(iter->first, attName)) {

			//
			//see if we have a string value
			//
			if(!iter->second.is_string()) {
				if(debug_this::yes_no) {
					debug_this::indent();
					cout << "ciGetStringAtt: " << attName << " is not a string\n";
				}
				return 0;
			}

			stringValue = *iter->second.get_string();
			if(debug_this::yes_no) {
				debug_this::indent();
				cout << "ciGetStringAtt: " << attName << " = " << stringValue << endl;
			}
			return 1;
		}
	}
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "ciGetStringAtt: " << attName << " not found\n";
	}

	return 0;
}


/*Initializes list of reserved attribute names*/
void
SASF_Step::Init() {}

long SASF_Step::CiGetSASFStringAttribute(
		const string &attrName,
		string& attributeVal) const {
	// debug_inhibit di;
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "CiGetSASFAttr(" << attrName << "): base class, not working!\n"; }
	return 0;
}

long SASF_StepSeq::CiGetSASFStringAttribute(
		const string &attrName,
	       	string& attributeVal) const {
	// debug_inhibit di;
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "CiGetSASFAttr[seq](" << attrName << "): calling ciGetStringAttr\n";
	}
	return ciGetStringAttribute(RequestAttributes, attrName, attributeVal);
}

long SASF_ActivityStep::CiGetSASFStringAttribute(
		const string &attrName,
		string& attributeVal) const {
	// debug_inhibit di;
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "CiGetSASFAttr[activity](" << attrName << "): calling ciGetStringAttr\n";
	}

	map<string, TypedValue> reqAttributes = get_instance()->GetAttributes();
	return ciGetStringAttribute(reqAttributes, attrName, attributeVal);
}

long SASF_CommandStep::CiGetSASFStringAttribute(
		const string &attrName,
		string& attributeVal) const {
	// debug_inhibit di;
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "CiGetSASFAttr[command](" << attrName << "): calling ciGetStringAttr\n"; }
	map<string, TypedValue> reqAttributes = get_instance()->GetSASFAttributes();
	return ciGetStringAttribute(reqAttributes, attrName, attributeVal);
}

long SASF_NoteStep::CiGetSASFStringAttribute(
		const string &attrName,
		string& attributeVal) const {
	// debug_inhibit di;
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "CiGetSASFAttr[note](" << attrName << "): calling ciGetStringAttr\n"; }
	map<string, TypedValue> reqAttributes = get_instance()->GetSASFAttributes();
	return ciGetStringAttribute(reqAttributes, attrName, attributeVal);
}

long SASF_NoteEndStep::CiGetSASFStringAttribute(
		const string &attrName,
		string& attributeVal) const {
	// debug_inhibit di;
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "CiGetSASFAttr[note end](" << attrName << "): calling ciGetStringAttr\n"; }
	map<string, TypedValue> reqAttributes = get_instance()->GetSASFAttributes();
	return ciGetStringAttribute(reqAttributes, attrName, attributeVal);
}


/* function used with ReservedAttrNames that will get you a string of attributes that are not
 * reserved nor understood by apgen. These are output into the sasf */
string GetExtraAttributesString(map<string, TypedValue>& sasfAttributes) {
	aoString	buildString;
	int		numWritten = 0;

			// in ACT_exec.C:
	extern blist&	theStandardSASFelementNames();

	debug_indenter	I;

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "GetExtraAttributesString start\n";
	}
	map<string, TypedValue>::const_iterator iter;
	for(	iter = sasfAttributes.begin();
		iter != sasfAttributes.end();
		iter++) {

		if(debug_this::yes_no) {
			debug_this::indent();
			cout << "examining " << iter->first << "\n";
		}

		//
		// not clear that any attributes could satisfy this...
		// in old APGen, it may be that attributes with unknown
		// names could be introduced at will
		//
		map<Cstring, Cstring>::const_iterator it1 = 
		   aafReader::nickname_to_activity_string().find(iter->first.c_str());
		if(!theStandardSASFelementNames().find(iter->first.c_str())
		   && it1 == aafReader::nickname_to_activity_string().end()
		   && iter->first != "id"
		   && iter->first != "name"
		  ) {

			//
			//make sure that its a string value
			//
			if(!iter->second.is_string()) {
				continue;
			}
			if(numWritten > 0)
				buildString << "," << "\n";
			//upperify this
			string name = iter->first;
			upperCase(&name);

			buildString << "            " << name << ", " << iter->second.to_string();
			numWritten++;
		}
	}

	string attString(buildString.str());
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "GetExtraAttributesString done; final string: " << attString << "\n";
	}
	return attString;
}

bool SASF_Step::GetSASFAttributeString(const string& key, string& value) const {
	string extraString;

	// check that we have an instance
	if(!InstancePtr) {
		return false;
	}

	map<string, TypedValue> all_attributes = InstancePtr->GetAttributes();
	map<string, TypedValue> sasf_attribute;

	//
	// We need to extract the SASF attributes
	//
	map<string, TypedValue>::const_iterator it = all_attributes.find("sasf");

	if(it != all_attributes.end()) {
		ListOVal& array = it->second.get_array();
		ArrayElement* el = array.find(key.c_str());
		if(el) {
			if(el->Val().is_string()) {
				value = *el->Val().get_string();
			} else {
				value = *el->Val().to_string();
			}
			return true;
		}
	}
	return false;
}

string SASF_Step::GetExtraSASFAttributesString() const {
	string extraString;

	// check that we have an instance
	if(!InstancePtr) {
		return extraString;
	}

	map<string, TypedValue> all_attributes = InstancePtr->GetAttributes();
	map<string, TypedValue> sasf_attribute;

	//
	// We need to extract the SASF attributes
	//
	map<string, TypedValue>::const_iterator it = all_attributes.find("sasf");

	if(it != all_attributes.end()) {
		ListOVal& array = it->second.get_array();
		ArrayElement* el;
		for(int z = 0; z < array.get_length(); z++) {
			el = array[z];
			sasf_attribute[*el->get_key()] = el->Val();
		}
	}

	return GetExtraAttributesString(sasf_attribute);
}

vector<TypedValue> SASF_Step::GetParameters() const {
	if(!InstancePtr)
		return vector<TypedValue>();

	return InstancePtr->GetAPGENParameters();
}


long SASF_Step::GetInstanceDuration(IO_Duration *dur) const {
	if(!InstancePtr)
		return 0;

	const TypedValue& val(InstancePtr->GetAttribute("span"));

	IO_Duration duration(val.get_time_or_duration());

	*dur = duration;

	return 1;
}

string SASF_Step::GetType() const {
	string type;

	if(InstancePtr) {
		type = InstancePtr->GetAPGENType();
	}

	return type;
}


string SASF_Step::GetDescription() const {
	string description;

	if(InstancePtr) {
		description = InstancePtr->GetDescription(); }

	return description; }

// for debugging output
ostream &operator<<(ostream &s, const SASF_StepSeq &req) {
	s << "REQUEST:" << req.GetID() << endl;
	string apfTimeString = req.SequenceStartTime->GetAPFTimeString();
	s << "StartTime: " << apfTimeString << endl;
	s << "STEPS:" << endl;
	
	//print out steps
	SASF_StepPtrMSet::const_iterator iter = req.Steps.begin();

	if(iter != req.Steps.end()) {
		int i = 0;

		//call virtual step method
		s << (*iter)->GetStepString(i++);
		iter++;

		while(iter != req.Steps.end()) {
		  //call virtual step method
		  s << (*iter)->GetStepString(i++);
		  iter++; }

		s << endl; }

	return s; }


//Produces a the string representation of this step in a generic way
string SASF_GenericStep::GetStepString(long stepNum) const {
	aoString buildString;
	// debug_inhibit dt;

	buildString << "        " << "Type" << "( ";
	
	//the label would override the default step numbering scheme
	string stepLabel;

	{
	// uncomment for debug
	// debug_this dt;

	if(debug_this::yes_no) {
		cout << "checking the step_label attribute...\n"; }
	if(CiGetSASFStringAttribute("step_label", stepLabel))
		buildString << stepLabel;
	else
		buildString << stepNum;
	}

	//time stuff always from request start
	buildString << ", SCHEDULED_TIME, " << GetFromStart().GetAPFDurationString();
	buildString << ", FROM_REQUEST_START," << "\n";

	//add the unknown attributes to the output
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "GetStepString(" << stepNum << "): about to call GetExtraSASFAttributesString()\n"; }
	string extraAtts = GetExtraSASFAttributesString();

	if(!extraAtts.empty())
		buildString << extraAtts << "," << "\n";

	buildString << "            step(" << "\n";
	buildString << "            )," << "\n";

	string stepString(buildString.str());
	return stepString; }


//This method gets the step string representation for this step
string SASF_ActivityStep::GetStepString(long stepNum) const {
	//for building strings
	aoString buildString;
	string stepName;

	{
	// uncomment for debug
	// debug_this dt;

	if(debug_this::yes_no) {
		cout << "checking the step_name attribute...\n"; }
	if(!CiGetSASFStringAttribute("step_name", stepName)) {
		stepName = "activity"; }
	}

	buildString << "        " << stepName << "(";


	string stepLabel;
	if(CiGetSASFStringAttribute("step_label", stepLabel))
		buildString << stepLabel;
	else
		buildString << stepNum;

	buildString << ", SCHEDULED_TIME, " << GetFromStart().GetAPFDurationString();
	buildString << ", FROM_REQUEST_START," << "\n";

	//extra attributes
	string extraAttsStr = GetExtraSASFAttributesString();

	if(!extraAttsStr.empty())
		buildString << extraAttsStr << "," << "\n";

	string groupName;

	if(!GetSASFAttributeString("group_name", groupName)) {
		groupName = "APGEN";
	}
	
	string activityName;
	if(!GetSASFAttributeString("activity_name", activityName))
		activityName = GetType();

	buildString << "            " << groupName << "(" << activityName;//for activities
	
	vector<TypedValue> parameters(GetParameters());

	//put in the parameters
	for(int k = 0; k < parameters.size(); k++) {
		buildString << "," << "\n";
		const TypedValue& value = parameters[k];
		buildString << "                " << value.to_string();
	}

	buildString << ")" << "\n";
	buildString << "             )," << "\n";

	return string(buildString.str());
}


//generates the step string for a command type of step
string 
SASF_CommandStep::GetStepString(long stepNum) const {
	aoString buildString;

	string stepName;

	{
	// uncomment for debug
	// debug_this dt;

	if(!CiGetSASFStringAttribute("step_name", stepName))
		stepName = "command";
	}

	buildString << "        " << stepName << "(";

	string stepLabel;
	if(CiGetSASFStringAttribute("step_label", stepLabel))
		buildString << stepLabel;
	else
		buildString << stepNum;

	//start time
	buildString << ", SCHEDULED_TIME, " << GetFromStart().GetAPFDurationString();
	buildString << ", FROM_REQUEST_START," << "\n";

	//add any additional unused attributes
	string extraAtts = GetExtraSASFAttributesString();

	if(!extraAtts.empty())
		buildString << extraAtts << "," << "\n";

	string commandName;
	if(!CiGetSASFStringAttribute("command_name", commandName))
		commandName = GetType();
	

	buildString << "            " << commandName << "("; //for commands

	vector<TypedValue> parameters(GetParameters());

	//add in parameters
	for(int k = 0; k < parameters.size(); k++) {
		if(k != 0) {
			buildString << "," << "\n";
			buildString << "                ";
		}

		buildString << parameters[k].to_string();
	}

	buildString << ")" << "\n";
	buildString << "             )," << "\n";

	return string(buildString.str());
}


//gets the string representation of this step
string SASF_NoteStep::GetStepString(long stepNum) const {
	// debug_inhibit dt;
	//for building the string
	aoString buildString;

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "building a note step.\n"; }
	buildString << "        note(";

	string stepLabel;

	if(CiGetSASFStringAttribute("step_label", stepLabel))
		buildString << stepLabel;
	else
		buildString << stepNum;

	//start time
	buildString << ", SCHEDULED_TIME, " << GetFromStart().GetAPFDurationString();
	buildString << ", FROM_REQUEST_START," << "\n";

	//add any additional unused attributes
	string extraAtts = GetExtraSASFAttributesString();

	if(!extraAtts.empty())
		buildString << extraAtts << "," << "\n";

	//what is the text of this note?

	string noteText;

	if(!CiGetSASFStringAttribute("text", noteText)) {
		//build note string
		aoString noteBuild;
		IO_Duration dur;
		GetInstanceDuration(&dur);
		noteBuild << "\"Duration, " << dur.GetAPFDurationString() << ", ";
		noteBuild << "APGEN_activity_type, " << GetType() << ", ";
		noteBuild << "description, " << GetDescription();

		noteBuild << "\"";
		noteText = string(noteBuild.str()); }
	else {
		noteText = "\"" + noteText + "\""; }

	//the text
	buildString << "            TEXT, " << noteText;
	buildString << ")," << "\n";

	return string(buildString.str()); }

//gets the string representation of this step
string SASF_NoteEndStep::GetStepString(long stepNum) const {
	//for building the string
	aoString buildString;

	buildString << "        note(";

	string stepLabel;

	if(CiGetSASFStringAttribute("step_label", stepLabel))
		buildString << stepLabel;
	else
		buildString << stepNum;

	//start time
	buildString << ", SCHEDULED_TIME, " << GetFromStart().GetAPFDurationString();
	buildString << ", FROM_REQUEST_START," << "\n";

	//add any additional unused attributes
	string extraAtts = GetExtraSASFAttributesString();

	if(!extraAtts.empty())
		buildString << extraAtts << "," << "\n";

	//what is the text of this note?

	string noteText = "\"End of activity ";
	noteText += GetType();
	noteText += "\"";

	//the text
	buildString << "            TEXT, " << noteText;
	buildString << ")," << "\n";

	return string(buildString.str()); }

	/* This function is poorly named. It is the top-level function for expressing
	 * a single activity instance in SASF format. (V_7_7 change) */
string SASF_StepSeq::requestToStringWithStartTime(
		const IO_Time* startTime) const {

	//for building strings
	aoString buildString;

	// debug_inhibit dt;
	debug_indenter I;

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "SASF_StepSeq::requestToStringWithStartTime() start\n"; }

	//Request can be NULL and we should use the defaults in that case
	
	//request identifier?
	string identifier;
	if(!ciGetStringAttribute(RequestAttributes, "identifier", identifier))
		identifier = Request->GetAPGENID();
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "setting identifier to " << identifier << "\n"; }

	string processor;
	//get processor
	if(!ciGetStringAttribute(RequestAttributes, "processor", processor))
		processor = "\"NONE\"";
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "setting processor to " << processor << "\n";
	}

	string key;
	//get key
	if(!ciGetStringAttribute(RequestAttributes, "key", key)) {
		key = "\"NOKEY\"";
	}
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "setting key to " << key << "\n";
	}

	string requestor;
	if(!ciGetStringAttribute(RequestAttributes, "requestor", requestor)) {
		requestor = "\"";
		//use logname for requestor
		char *login = getenv("LOGNAME");
		requestor += login;
		requestor += "\"";
	}
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "setting requestor to " << requestor << "\n";
	}

	//display time system relative times as exact times (because sasf doesn't support them)
	string apfTimeString;
	
	//use dynamic cast NULL means failed, otherwise success
	const IO_TimeSystemRelative *systemStart = dynamic_cast<const IO_TimeSystemRelative*>(startTime);
	if(systemStart != NULL) {
		apfTimeString = IO_TimeExact(systemStart->GetEvaluatedTime()).GetAPFTimeString(); }
	else {
		apfTimeString = startTime->GetAPFTimeString(); }

	//put the string together
	buildString << "request(";
	buildString << identifier << ", REQUESTOR, ";
	buildString << requestor << "," << "\n";
	buildString << "            START_TIME, " << apfTimeString << "," << "\n";
	buildString << "            PROCESSOR, ";
	buildString << processor << "," << "\n";
	buildString << "            KEY, " << key;
	
	//other potential attributes
	string attrVal;
	if(ciGetStringAttribute(RequestAttributes, "upper_label", attrVal))
		buildString << "," << "\n" << "            UPPER_LABEL, " << attrVal;

	if(ciGetStringAttribute(RequestAttributes, "lower_label", attrVal))
		buildString << "," << "\n" << "            LOWER_LABEL, " << attrVal;

	if(ciGetStringAttribute(RequestAttributes, "title", attrVal))
		buildString << "," << "\n" << "            TITLE, " << attrVal;

	if(ciGetStringAttribute(RequestAttributes, "genealogy", attrVal))
		buildString << "," << "\n" << "            GENEALOGY, " << attrVal;

	if(ciGetStringAttribute(RequestAttributes, "workgroup", attrVal))
		buildString << "," << "\n" << "            WORKGROUP, " << attrVal;

	if(ciGetStringAttribute(RequestAttributes, "status", attrVal))
		buildString << "," << "\n" << "            STATUS, " << attrVal;

	if(ciGetStringAttribute(RequestAttributes, "editgroup", attrVal))
		buildString << "," << "\n" << "            EDITGROUP, " << attrVal;

	if(ciGetStringAttribute(RequestAttributes, "request_state", attrVal))
		buildString << "," << "\n" << "            REQUEST_STATE, " << attrVal;


	//right now description is repeated for generated request and step -- probably ok
	string description;
	description = Request->GetDescription();

	if(!description.empty())
		buildString << "," << "\n" << "            DESCRIPTION, \"" << description << "\"";

	buildString <<")" << "\n";

	if(debug_this::yes_no) {
		Cstring ss(buildString.str());
		debug_this::indent();
		cout << "buildString so far: " << ss;
		buildString << ss;
		debug_this::indent();
		cout << "About to call GetStepString on each step...\n"; }
	//add the steps, call their respective GetStepStrings
	SASF_StepPtrMSet::const_iterator stepIter = Steps.begin();
	int i = 1;
	while(stepIter != Steps.end()) {
		buildString << (*stepIter)->GetStepString(i);
		i++;
		stepIter++; }

	buildString << "    end;" << "\n";

	return string(buildString.str()); }

//gets the request string for the step sequence
string
SASF_StepSeq::GetRequestString() const {
	//calls the private helper function
	return requestToStringWithStartTime(SequenceStartTime); }

//sets the start time of the step sequence
void
SASF_StepSeq::SetStartTime(const IO_Time *startTime) {
	//SequenceStartTime must be a pointer because IO_Time is an abstract class
	if(SequenceStartTime != NULL) {
		delete SequenceStartTime;
	}

	//use virtual constructor to get the right type
	SequenceStartTime = startTime->Clone();
}

void
SASF_StepSeq::UpdateSteps(const IO_Duration &dur) {
	//iterate through, adding dur
	SASF_StepPtrMSet::iterator step = Steps.begin();
	while(step != Steps.end()) {
		IO_Duration duration = (*step)->GetTimeFromSequenceStart();
		//add any additional duration
		duration += dur;
		(*step)->SetTimeFromSequenceStart(duration.GetBase());
		step++;
	}
}


//gets the id from its request
long
SASF_StepSeq::GetID() const {
	return Request->GetID();
}

//
// Adds a step sequence with one or more steps (two for notes) based
// on the elements inside the SASF attribute in this ActivityInstance
//
void
IO_SASFWriteImpl::AddInstanceSequence(
		SASF_StepSeqMSet* sequences,
		const SASF_ActivityInstance* instance) const {
	const AUTO_PTR_TEMPL<IO_Time> startTime(instance->GetStartTime());
	if(startTime.get() == NULL) // top activity
		return;

	debug_indenter I;

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "AddInstanceSequence(" << instance->GetAPGENID() << ") start\n";
	}

	/* finds the ActivityInstance of the enclosing request,
		it is the id of this activityInstance that we will merge step sequences on,
		and it is the attributes of this instance that we will build the 
		request string from*/
	const SASF_ActivityInstance* requestInstance = instance->GetRequestInstance();

	// if there was an enclosing request, use its attributes for the request
	if(requestInstance == NULL) {
		if(debug_this::yes_no) {
			debug_this::indent();
			cout << "no enclosing request for this sequence; using the lower-level instance instead\n";
		}
		// otherwise use this instance for merging and starttime, but leave the attributes empty
		requestInstance = instance;
	} else if(debug_this::yes_no) {
		debug_this::indent();
		cout << "found the enclosing request\n";
	}

	// not an SASF thing
	if(!instance->HasSASFAttribute()) {
		if(debug_this::yes_no) {
			debug_this::indent();
			cout << "AddInstanceSequence() end (no SASF attrs)\n";
		}
		return;
	}

	// not a step 
	if(instance->IsRequest()) {
		if(debug_this::yes_no) {
			debug_this::indent();
			cout << "AddInstanceSequence() end (instance is a request, not a step)\n"; }
		return;
	}

	string	theFileNameToRemember;
	// check to see if this is a symbolic file we're including in this sasf file
	StringVect::const_iterator found = find(
			SymbolicFilesToInclude.begin(),
			SymbolicFilesToInclude.end(),
			theFileNameToRemember = instance->GetFileName());

	// this file was in our list
	if(found == SymbolicFilesToInclude.end()) {
		if(debug_this::yes_no) {
			debug_this::indent();
			cout << "AddInstanceSequence() end (instance's file attribute " << theFileNameToRemember << " is not on the list to output)\n";
		}
		return;
	}

	// has a type? (default to notes)

	// get the attributes for the steps
	map<string, TypedValue> attributes = instance->GetSASFAttributes();

	string type;
	if(!ciGetStringAttribute(attributes, "type", type)) // no type no request
		type = "notes";

	map<string, TypedValue> reqAttributes;

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "looking for SASF attributes of the enclosing request...\n";
	}
	reqAttributes = requestInstance->GetSASFAttributes();

	SASF_StepSeq newSequence(requestInstance, startTime.get(), reqAttributes);

	// add whichever steps we might have
	if(type == "activity") {
		// create the step and add it to the sequence
		SASF_ActivityStep activity(instance);

		newSequence.AddStep(&activity);
	} else if(type == "command") {
		// create the command and add it
		SASF_CommandStep command(instance);
		newSequence.AddStep(&command);
	} else if((type == "note") || (type == "notes")) {
		// note or notes steps

		// use autoptr for memory cleanup
		TypedValue attribute(instance->GetAttribute("span"));
		
		// dynamic cast returns NULL if the cast fails
		CTime_base T = attribute.get_time_or_duration();
		

		// create and add note step
		SASF_NoteStep noteStep(instance);
		newSequence.AddStep(&noteStep);

		if(type ==  "notes") {
			// create and add second note step
			SASF_NoteEndStep secondStep(instance);
			secondStep.SetTimeFromSequenceStart(T);
			newSequence.AddStep(&secondStep);
		}
	} else {
		// exceptional case
		string errorString = "Unrecognized SASF type attribute:";
		errorString += type;
		errorString += "\n";
		SASFException badAttribute(errorString);
		throw (badAttribute);
	}

	// insert our new request
	sequences->insert(newSequence);
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "AddInstanceSequence() done\n";
	}
}

// step sequence constructor
SASF_StepSeq::SASF_StepSeq(
		const SASF_ActivityInstance* request,
		IO_Time* startTime,
		const map<string, TypedValue>& requestAttributes)
	: Request(request),
		RequestAttributes(requestAttributes) {
	SequenceStartTime = startTime->Clone();
}

// step sequence copy constructor
SASF_StepSeq::SASF_StepSeq(const SASF_StepSeq &sequence)
	: Request(sequence.Request),
		RequestAttributes(sequence.RequestAttributes) {
	SequenceStartTime = sequence.SequenceStartTime->Clone();
	
	for (SASF_StepPtrMSet::iterator iter = sequence.Steps.begin();
	     iter != sequence.Steps.end();
	     iter++) {
		Steps.insert(Steps.begin(), (*iter)->Clone());
	}
}

// used in a foreach for freeing steps
void
DeleteStep(SASF_Step *step) {
	delete step;
}

// assignment operator for Step sequences
SASF_StepSeq &
SASF_StepSeq::operator=(const SASF_StepSeq &rhs) {
	// compare pointers
	if(this == &rhs)
		return *this;

	SetStartTime(rhs.SequenceStartTime);

	Request = rhs.Request;

	//set the requestAttributes
	RequestAttributes = rhs.RequestAttributes;

	//delete the steps
	for_each(Steps.begin(), Steps.end(), DeleteStep);

	//erase the places
	Steps.clear();

	//insert the clones of the other steps
	
	for (SASF_StepPtrMSet::iterator iter = rhs.Steps.begin();
	     iter != rhs.Steps.end();
	     iter++) {
		Steps.insert(Steps.begin(), (*iter)->Clone());
	}

	return *this;
}

//step sequence destructor
SASF_StepSeq::~SASF_StepSeq() {
	if(SequenceStartTime != NULL)
		delete SequenceStartTime;

	//for debugging
	SequenceStartTime = NULL;

	//delete steps
	for_each(Steps.begin(), Steps.end(), DeleteStep);

	//removes space
	Steps.clear();
}

//determines if two step sequences have equivalent steps
bool
SASF_StepSeq::StepEquivalent(const SASF_StepSeq &otherSeq) const {
	//compare the request string of this sequence with the request string of the
	//other sequence if it had the same start time as this sequence

	//cerr << GetRequestString() << " : " << otherSeq.requestToStringWithStartTime(SequenceStartTime) << endl;
	
	return (GetRequestString() == otherSeq.requestToStringWithStartTime(SequenceStartTime)); }

//adds a step to a sequence
void
SASF_StepSeq::AddStep(const SASF_Step* step) {
	// call virtual copy constructor
	Steps.insert(step->Clone());
}

//Merges two step sequences into a new sequence throws exceptions if they cannot be merged
void
MergeStepSequences(
		const SASF_StepSeq &firstToMerge,
		const SASF_StepSeq &secondToMerge,
		SASF_StepSeq *mergedSequence) {
	//base cases if only one of the sequences has any steps
	if(firstToMerge.Empty()) {
		*mergedSequence = secondToMerge;
		return; }

	if(secondToMerge.Empty()) {
		*mergedSequence = firstToMerge;
		return; }

	//general case

	//AUTO_PTR_TEMPLs for memory management with exceptions
	const AUTO_PTR_TEMPL<IO_Time> firstStartTime(firstToMerge.GetStartTime());
	const AUTO_PTR_TEMPL<IO_Time> secondStartTime(secondToMerge.GetStartTime());

	/*determine if the starttimes are compatible:
		In general: ExactTimes are compatible with exact times
		Epoch relative times are compatible if the epochs they refer to are the same
		Time System relative times are compatible if the time systems they refer to are the same
	*/

	bool compatible = firstStartTime->RelativeCompatible(secondStartTime.get());
	bool less = *firstStartTime < *secondStartTime;

	if(!compatible) {
		SASFException badStartingTimes("Starting times of steps in a request must be compatible\n");
		throw(badStartingTimes);
	}

	//call private class method
	if(less) {
		*mergedSequence = firstToMerge;
		mergedSequence->MergeSequenceLater(secondToMerge);
	} else {
		*mergedSequence = secondToMerge;
		mergedSequence->MergeSequenceLater(firstToMerge);
	}
}

//this method merges a later step sequence into this sequence
void
SASF_StepSeq::MergeSequenceLater(const SASF_StepSeq &toMerge) {
	//or we cannot merge
	// assert(toMerge.GetID() == GetID());
	if(toMerge.GetID() == GetID()) { ; }
	else return;
	if(toMerge.SequenceStartTime->RelativeCompatible(SequenceStartTime)) { ; }
	else return;

	if(!Steps.empty()) { ; }
	else return;

	IO_Duration firstDuration = (*(Steps.begin()))->GetTimeFromSequenceStart();

	//assert that first thing starts at the start of the sequence
	IO_Duration startDuration;//default constructor initializes to 0
	// assert(firstDuration == startDuration);
	if(firstDuration == startDuration) { ; }
	else return;

	//will not merge with sequences before us
	// assert(! (*(toMerge.SequenceStartTime) < *SequenceStartTime)) { ; }
	if(! (*(toMerge.SequenceStartTime) < *SequenceStartTime)) { ; }
	else return;

	//update toMerge's steps and put them in this list
	IO_Duration difference = toMerge.SequenceStartTime->GetSubtract(*SequenceStartTime);
	
	/*add the steps from the other sequence into us, updating their 
		sequence start offsets*/
	SASF_StepPtrMSet::const_iterator iter = toMerge.Steps.begin();
	while(iter != toMerge.Steps.end()) {
		//virtual copy constructor (allows us to not modify our arguments)
		SASF_Step *step = (*iter)->Clone();
		IO_Duration duration = step->GetTimeFromSequenceStart();
		//add any additional duration
		duration += difference;
		step->SetTimeFromSequenceStart(duration.GetBase());
		//add the step (which copies)
		AddStep(step);
		//delete the clone
		delete step;
		iter++;
	}
}

//returns the number of children directly beneath us
unsigned long SASF_ActivityInstance::GetNumChildren() const {
	return ChildInstances.size();
}

/*recursively computes the number of children of children etc
	counting non-leaf nodes*/
unsigned long SASF_ActivityInstance::GetNumChildrenRecursive() const {
	SASF_ActivityPtrVect::const_iterator i = ChildInstances.begin();

	unsigned int numChildrenRecursive = ChildInstances.size();

	//add in the numChildren of our children
	while(i != ChildInstances.end()) {
		numChildrenRecursive += (*i)->GetNumChildrenRecursive();
		i++;
	}
	
	return numChildrenRecursive;
}

//returns the childindex child
SASF_ActivityInstance *SASF_ActivityInstance::GetChildInstance(
		unsigned long childIndex) const {
	//make sure that we are in range
	// assert(childIndex < ChildInstances.size());
	if(childIndex < ChildInstances.size()) { ; }
	else return NULL;

	return ChildInstances[childIndex];
}

//checks for the attribute
bool SASF_ActivityInstance::HasSASFAttribute() const {
	TypedValue sasfAtt;

	if(debug_this::yes_no) {
		// debug
		debug_this::indent();
		cout << "  HasSASFAttribute(" << ActivityInstance.GetID() << ")\n";
	}

	try {
		sasfAtt = ActivityInstance.GetAttribute("sasf");
	} catch(eval_error Err) {

		//
		// attribute not found
		//
		return false;
	}

	return sasfAtt.get_type() != apgen::DATA_TYPE::UNINITIALIZED;
}

/*gets all of the sasf attributes (I think we use this to make sure the extra attributes
	get put in the steps and requests) */
map<string, TypedValue> SASF_ActivityInstance::GetSASFAttributes() const {
	debug_indenter	I;

	if(debug_this::yes_no) {
		// debug
		debug_this::indent();
		cout << "GetSASFAttributes(act = " << ActivityInstance.GetID() << ")\n";
	}
	map<string, TypedValue> sasfAttrs;

	TypedValue sasf_attr = ActivityInstance.GetAttribute("sasf");

	//
	// type will be apgen::DATA_TYPE::UNINITIALIZED if the sasf attribute does not exist
	//
	if(sasf_attr.get_type() != apgen::DATA_TYPE::ARRAY) {
		if(debug_this::yes_no) {
			// debug
			debug_this::indent();
			cout << "GetSASFAttributes() done\n";
		}
		return sasfAttrs;
	}
	ListOVal&	lov = sasf_attr.get_array();
	for(int i = 0; i < lov.get_length(); i++) {
		ArrayElement* ae = lov[i];

		TypedValue& elem = ae->Val();
		debug_inhibit dt;

		if(elem.is_string()) {
			if(debug_this::yes_no) {
				// debug
				debug_this::indent();
				cout << "  got attribute " << ae->get_key()
					<< " = " << elem.get_string() << "\n";
			}
			sasfAttrs[*ae->get_key()] = elem;
		} else {
			// debug
			// cout << "  attribute " << (*x)->GetName() << " for act "
			// 	<< GetAPGENID() << " is not a string\n";
		}
	}
	if(debug_this::yes_no) {
		// debug
		debug_this::indent();
		cout << "GetSASFAttributes() done\n";
	}
	return sasfAttrs;
}

//will find the description based on a regular non-SASF attribute
string SASF_ActivityInstance::GetDescription() const {
	string description;
	TypedValue attribute;

	try {
		attribute = GetAttribute("description");
	} catch(eval_error Err) {
		return description;
	}

	
	if(attribute.is_string()) {
		description = *attribute.get_string();
	}

	return description;
}


IO_Time* SASF_ActivityInstance::GetStartTime() const {

	bool get_timing_from_children = false;

	//
	// First, check whether this is the fake top-level activity:
	//
	if(ActivityInstance.GetID() == "ID Never Defined") {

		//
		// Get time from children
		//
		get_timing_from_children = true;
	}
	if(get_timing_from_children) {
	    if(GetNumChildren() > 0) {
		SASF_ActivityPtrVect children(ChildInstances);

		//sort children
		sort(children.begin(), children.end(), SASF_ActivityInstanceLess);

		//get start time of first child
		return (*(children.begin()))->GetStartTime();
	    }
	} else {

		//
		// Regular activity
		//
		TypedValue val;
		val = ActivityInstance.GetAttribute("start");
		if(val.is_time()) {
			return new IO_TimeExact(val.get_time_or_duration());
		}
	}

	//
	// Don't return NULL; that throws exceptions
	//
	throw(eval_error("GetStartTime: cannot make sense of this"));
}

string SASF_ActivityInstance::GetFileName() const {
	string fileName;

	map<string, TypedValue> attributes = GetSASFAttributes();

	//get the fileName
	if(!ciGetStringAttribute(attributes, "file", fileName))
		fileName = "<Default>";
	return fileName;
}


// determines if an activity instance is a request or not
bool SASF_ActivityInstance::IsRequest() const {
	debug_indenter	I;

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "IsRequest(): examining APGen activity instance \""
			<< ActivityInstance.GetID() << "\"\n";
	}

	//
	// There used to be two ways to be a request:
	// 	- the activity was an instance of the request class
	// 	- the SASF attributes have type = request
	// The derived class 'request' has been eliminated, leaving
	// us with only one way to define a request.
	//

	//
	// Note: the top-level 'Plan' activity is treated as a request
	// for any step-like activities that do not have a request
	// parent. The 'Plan' activity can be easily detected because
	// it does not have a real APGenX activity under the hood.
	//
	// However, IsRequest() uses a narrower definition; it wants
	// to know if this object is a 'real' apgen activity that
	// qualifies a request. Therefore, if this object is the
	// Plan activity, the returned value should be 'false'.
	//
	if(this == IO_SASFWriteImpl::SASFwriter().get_plan()) {

		if(debug_this::yes_no) {
			debug_this::indent();
			cout << "IsRequest(): no\n";
		}
		return false;
	}

	map<string, TypedValue> attributes = GetSASFAttributes();

	//
	//search case-insensitively in the attributes
	//
	string type;
	if(ciGetStringAttribute(attributes, "type", type)
	   && type == "request") {
	    if(debug_this::yes_no) {
		debug_this::indent();
		cout << "IsRequest(): APGen activity instance's SASF attribute "
			<< "has type \"request\", so yes\n";
	    }
	    return true;
	}

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "IsRequest(): no\n";
	}

	return false;
}

//finds the top most enclosing request if there is one
const SASF_ActivityInstance* SASF_ActivityInstance::GetRequestInstance() const {
	debug_indenter	I;

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "GetRequestInstance(" << ActivityInstance.GetID()
			<< "): looking for an enclosing request\n";
	}
	// trying to get uppermost request (ignore file attributes)
	SASF_ActivityInstance* parentInstance = GetParentInstance();

	// is recursive
	if(NULL != parentInstance) {
		if(debug_this::yes_no) {
			debug_this::indent();
			cout << "found parent " << parentInstance->GetAPGENID() << "...\n"; }
		const SASF_ActivityInstance* parentRequest
			= parentInstance->GetRequestInstance();

		if(parentRequest != NULL) {
			if(debug_this::yes_no) {
				debug_this::indent();
				cout << "returning parent request "
					<< parentRequest->GetAPGENID() << "...\n";
			}
			return parentRequest;
		}
	}

	if(IsRequest()) {
		if(debug_this::yes_no) {
			debug_this::indent();
			cout << "returning this...\n";
		}
		return this;
	}
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "returning NULL...\n";
	}
	return NULL;
}


void MergeSequence::operator()(const SASF_StepSeq &sequence) {

	//invoke copy-constructor to get a temporary stepSeq
	SASF_StepSeq tempSeq(AccumulatedSequence);
	MergeStepSequences(AccumulatedSequence, sequence, &tempSeq);
	AccumulatedSequence = tempSeq;
}


//calls the merger to merge the sequences
void IO_SASFWriteImpl::MergeSequences(
		SASF_StepSeqMSet::const_iterator begin,
		SASF_StepSeqMSet::const_iterator end,
		SASF_StepSeq *mergedSequence) const {
	IO_TimeExact time;

	//use begin's attributes to initialize the merge
	SASF_StepSeq initSequence(begin->GetRequestInstance(), &time, begin->GetRequestAttributes());
	
#ifdef _Verify

	cerr << "Sequences before merge:" << endl;

	copy(begin, end, ostream_iterator<SASF_StepSeq>(cerr, "\n"));

#endif

	//create a merger with access to the errorClient
	MergeSequence merger(initSequence);

	//merge them
	merger = for_each(begin, end, merger);
	
	//get the merged value
	*mergedSequence = merger.GetAccumulatedSequence();

#ifdef _Verify

	cerr << "Sequence after merge" << endl;

	cerr << *mergedSequence << endl;

#endif

}

//
// we are merging sequences here
//
void
IO_SASFWriteImpl::CombineSequences(SASF_StepSeqMSet* sequences) {
	// find which groups sequences should be merged
	// all of the sequences should be sorted by ID and time

	SASF_StepSeqMSet mergedSequences;

	//  cerr << "sequences before merge:" << endl;
	// copy(sequences->begin(), sequences->end(), ostream_iterator<SASF_StepSeq>(cerr, "\n"));

	SASF_StepSeqMSet::const_iterator first = sequences->begin();
	SASF_StepSeqMSet::const_iterator second = first;

	IO_TimeExact defaultTime;

	while(first != sequences->end()) {
		// find the end of sequences with the same id
		// they should be all in a row, because they are ordered that way
		while((second != sequences->end()) && (second->GetID() == first->GetID()))
			second++;

		SASF_StepSeq combinedSequence(first->GetRequestInstance(), &defaultTime, first->GetRequestAttributes());

		MergeSequences(first, second, &combinedSequence);

		mergedSequences.insert(combinedSequence);

		//move first to the next section
		first = second;
	}

	sequences->swap(mergedSequences);
	//cerr << "sequences after merge:" << endl;
	//copy(sequences->begin(), sequences->end(), ostream_iterator<SASF_StepSeq>(cerr, "\n"));
}

// for verification of tree structure
void
CheckInstance(StringSASFActivityMap::value_type& pair) {
	SASF_ActivityInstance*	thisInstance = pair.second;

	SASF_ActivityInstance* parentInstance = thisInstance->GetParentInstance();
	
	//every instance has a parent
	// assert(parentInstance != NULL);
	if(parentInstance != NULL) { ; }
	else return;

	//this element's parent contains this as a child
	int parentNumChildren = parentInstance->GetNumChildren();

	int i;

	int foundThisInstance = 0;

	for(i = 0; i < parentNumChildren; i++) {
		SASF_ActivityInstance *parentChildInstance = parentInstance->GetChildInstance(i);

		if(parentChildInstance == thisInstance) {
		  foundThisInstance = 1;
		  break; } }

	// assert(foundThisInstance);
	}

//used to cleanup the instances in the map
void
DeleteActivity(StringSASFActivityMap::value_type &pair ) {
	delete pair.second; }

//to clean up the values in the GlobalMap
void
DeleteTypedValue(TypedValPtrMap::value_type &pair) {
	delete pair.second;
}

// another constructor
SASF_ActivityInstance::SASF_ActivityInstance(long id, const IO_ActivityInstance& instance)
	: ActivityInstance(instance),
		ChildInstances(),
		ParentInstance(NULL),
		ID(id) {}

// a copy constructor
SASF_ActivityInstance::SASF_ActivityInstance(const SASF_ActivityInstance &instance)
	: ActivityInstance(instance.ActivityInstance),
		ChildInstances(instance.ChildInstances),
		ParentInstance(instance.ParentInstance),
		ID(instance.ID) {}

SASF_ActivityInstance::~SASF_ActivityInstance()
{
}

//adds a child to an instance
void
SASF_ActivityInstance::AddChildActivityPtr(SASF_ActivityInstance *childInstance)
{
	//some debug output stuff here
#ifdef _Verify
	
	cerr << ActivityInstance.GetID() << " ---> " << childInstance->ActivityInstance.GetID() << endl;

#endif //_Verify

	ChildInstances.push_back(childInstance);
}

//sets a parent to an instance
void
SASF_ActivityInstance::SetParentActivityPtr(SASF_ActivityInstance *parentInstance)
{
	//for now
	// assert(ParentInstance == NULL);
	if(ParentInstance == NULL) { ; }
	else return;

	//some debug output code here
#ifdef _Verify

	cerr << parentInstance->ActivityInstance.GetID() << " <--- " << ActivityInstance.GetID() << endl;
	cerr << "ID:" << GetID() << endl;
	
	if(HasSASFAttribute()) {
		AUTO_PTR_TEMPL<IO_TypedValue> sasf(GetAttribute("sasf"));

		IO_TypedValueArray *sasfArray = dynamic_cast<IO_TypedValueArray*>(sasf.get());

		// assert(sasfArray != NULL);
		if(sasfArray != NULL) { ; }
		else return;

		unsigned int numElts = sasfArray->GetNumElements();
		unsigned int i;
		for(i = 0; i < numElts; i++) {
		    AUTO_PTR_TEMPL<IO_TypedValue> attribute(sasfArray->GetElement(i));

		    cerr << attribute->GetName() << ": " << attribute->GetAPFValueString() << endl;
		}
	}

#endif //_Verify

	ParentInstance = parentInstance;
}

//
// instance constructor taking a integer ID
//
SASF_ActivityInstance::SASF_ActivityInstance(long id)
	: ActivityInstance(), 		// Fake ActivityInstance object
		ChildInstances(),
		ParentInstance(NULL),
		ID(id) {}



void 
GetSymbolicFileSet::operator()(const StringSASFActivityMap::value_type &mapElem)
{
	//first is the key, second is the value
	const SASF_ActivityInstance *instance = mapElem.second;
	
	if(instance->HasSASFAttribute())
	{
		  //ignore files on requests
		if(!instance->IsRequest())
		{
		  Files.insert(instance->GetFileName());
		}
	}
}

void 
GetSymbolicFileSet::GetFiles(StringVect *vect)
{
		vect->clear();

		insert_iterator<StringVect> insertIter(*vect, vect->begin());

		copy(Files.begin(), Files.end(), insertIter);
};


//for getting the available symbolicfilenames
long IO_SASFWriteImpl::GetAvailableSymbolicFiles(StringVect *vect) const {
	GetSymbolicFileSet getFiles;

	//iterate through the ActivityInstances with the file getter
	getFiles = for_each(ActivityMap.begin(), ActivityMap.end(), GetSymbolicFileSet());
	getFiles.GetFiles(vect);
	return 1;
}

//sasf write destructor
IO_SASFWriteImpl::~IO_SASFWriteImpl() {
	for_each(ActivityMap.begin(), ActivityMap.end(), DeleteActivity);
	delete Plan;
}

//IO_ApgenDataClient method --stores globals in a map
long IO_SASFWriteImpl::AddOneGlobal(const string& name, const TypedValue& value) {
	//insert a copy of this global into the global map
	GlobalMap[name] = value;

	return 1;
}

//IO_ApgenDataClient method -- stores Epochs in a vector
long
IO_SASFWriteImpl::AddOneEpoch(const IO_Epoch &epoch) {
	Epochs.push_back(epoch);
	return 1;
}

//IO_ApgenDataClient method -- uses a map to put instances into the SASF_ActivityInstance hierarchy
long
IO_SASFWriteImpl::AddOneActivity(
		const IO_ActivityInstance& instance) {

	//
	// build the instance tree which will contain the given instance
	// and any of its children. Remember that the "Plan" has already
	// been defined as a semantically empty SASF_ActivityInstance
	// at the top level, and any 'real' top-level activities will
	// be defined as its children.
	//
	SASF_ActivityInstance*	sasfInstance
		= new SASF_ActivityInstance(NextActivityInstanceID++, instance);

	StringSASFActivityMap::iterator hashInstancePair
		= ActivityMap.find(instance.GetID());

	debug_indenter I;

	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "AddOneActivity(" << instance.GetID() << ") start\n";
	}
	//make sure that there isn't anything in there already
	// assert(hashInstancePair == ActivityMap.end());
	if(hashInstancePair == ActivityMap.end()) {
		;
	} else {
		cerr << "IO_SASFWriteImpl: activity " << instance.GetID().c_str()
			<< " is already in the global activity map. Skipping it.\n";
		return 1;
	}

	//insert the new node
	ActivityMap.insert(StringSASFActivityMap::value_type(instance.GetID(), sasfInstance));

	//
	// If the given instance has no parent, make it a child of the
	// semantically empty "Plan" instance and update the parent
	// pointer of this sasfInstance accordingly.
	//
	if(instance.GetParentActivityID().empty()) {
		Plan->AddChildActivityPtr(sasfInstance);
		sasfInstance->SetParentActivityPtr(Plan);
	} else {

		//
		// the given instance has a real parent
		//

		//
		//ParentInstancePair is a pair<id, sasfInstance*> iterator
		//
		StringSASFActivityMap::iterator
			parentInstancePair = ActivityMap.find(
					instance.GetParentActivityID());

		if(parentInstancePair != ActivityMap.end()) {

		    //
		    // we found the parent
		    //
		    parentInstancePair->second->AddChildActivityPtr(sasfInstance);
		    sasfInstance->SetParentActivityPtr(parentInstancePair->second);
		}
	}

	//
	// Duplicate the (already duplicated!) children structure of
	// the given instance into that of the sasf instance.
	//
	for(int i = 0; i < instance.GetNumSubActivities(); i++) {
		string subActivityID = instance.GetSubActivityID(i);
		
		StringSASFActivityMap::iterator childInstancePair
			= ActivityMap.find(subActivityID);

		if(childInstancePair != ActivityMap.end()) {
		  //found the child in the hash table

		  childInstancePair->second->SetParentActivityPtr(sasfInstance);
		  sasfInstance->AddChildActivityPtr(childInstancePair->second);
		}
	}
	
	if(debug_this::yes_no) {
		debug_this::indent();
		cout << "                 done\n";
	}
	return 1;
}

//dumps stuff to stderr if _Verify is set
void
IO_SASFWriteImpl::VerifyStructures() {
#ifdef _Verify

	int numElementsInMap = (int) ActivityMap.size();
	cerr << numElementsInMap << "elements in ActivityMap" << endl;

	//this will assert some of the tree structure
	for_each(ActivityMap.begin(), ActivityMap.end(), CheckInstance);

	int numChildrenInTree = Plan->GetNumChildrenRecursive();

	assert(numChildrenInTree == numElementsInMap);

	cerr << "Tree structure verified" << endl;

#endif //_Verify
}

//does the Global map array lookup
string
IO_SASFWriteImpl::GetHeaderVal(
		const string &globalName,
		const string &symbolicFileName) const {
	string value;

	map<string, TypedValue>::const_iterator globalIter = GlobalMap.find(globalName);

	if(globalIter != GlobalMap.end()) {
		const TypedValue& global = globalIter->second;

		if(global.is_array()) {
			//it is an array
			ArrayElement* ae = global.get_array().find(symbolicFileName.c_str());

			if(ae != NULL && ae->Val().get_type() == apgen::DATA_TYPE::STRING) {
				//there is an element for this filename
				value = *ae->Val().get_string();
			}
		}
	}

	return value;
}

//filters out the requests that are not within the time constraints
void
IO_SASFWriteImpl::FilterRequests(SASFStepSeqVect *requests) const
{
	SASFStepSeqVect filtered;

	//iterate through the requests
	SASFStepSeqVect::const_iterator reqIter = requests->begin();
	while(reqIter != requests->end())
	{
		bool includeThisRequest = true;

		const AUTO_PTR_TEMPL<IO_Time> startTime(reqIter->GetStartTime());
		
		if(EndTime < *startTime)
		  includeThisRequest = false;
		
		if(*startTime < StartTime)
		  includeThisRequest = false;
		
		if((*startTime == EndTime) && (InclusionFlag == 0))
		  includeThisRequest = false;

		if(includeThisRequest)
		  filtered.push_back(*reqIter);

		reqIter++;
	}

	//swap em back
	requests->swap(filtered);
}

//writes the body of the sasf file
void
IO_SASFWriteImpl::WriteBody(aoString &out, const SASFStepSeqVect &requestVect) const {
	//iterate through requests
	SASFStepSeqVect::const_iterator reqIter = requestVect.begin();
	while(reqIter != requestVect.end()) {
		out << reqIter->GetRequestString();
		
		reqIter++; }

	//add end of file
	out << "$$EOF\n"; }

//this method will set the start time of the request to the earlier of
//the activity/request that represents the request and the first step in the request
void
IO_SASFWriteImpl::UpdateRequestStartTimes(SASFStepSeqVect &requestVect) const {
	//iterate through the requests updating the start times
	SASFStepSeqVect::iterator reqIter = requestVect.begin();

	while(reqIter != requestVect.end()) {
		AUTO_PTR_TEMPL<IO_Time> seqStart(reqIter->GetStartTime());
		AUTO_PTR_TEMPL<IO_Time> reqStart(reqIter->GetRequestInstance()->GetStartTime());

		//cerr << "sequenceStart:" << seqStart->GetAPFTimeString() << endl;
		//cerr << "requestStart:" << reqStart->GetAPFTimeString() << endl;

		//the request in the plan starts before the first step
		if(*reqStart < *seqStart) {
		  //get the difference
		  IO_Duration difference = seqStart->GetSubtract(*reqStart);

		  //update the steps
		  reqIter->UpdateSteps(difference);

		  //set the new start time
		  reqIter->SetStartTime(reqStart.get());
		}

		reqIter++;
	}
}

//writes the header to the file
void
IO_SASFWriteImpl::WriteHeader(aoString &out) const {
	string firstSymbolic = SymbolicFilesToInclude[0];
	//get Variables and stick em in strings
	string seq_id = GetHeaderVal("SASF_SEQ_ID", firstSymbolic);

	if(seq_id.empty())
		seq_id = "APGEN";
	
	string data_set_id = GetHeaderVal("SASF_DATA_SET_ID", firstSymbolic);

	if(data_set_id.empty())
		data_set_id = "SPACECRAFT_ACTIVITY_SEQUENCE";
	

	//get Variables and stick em in strings
	string mission_name = GetHeaderVal("SASF_MISSION_NAME", firstSymbolic);

	if(mission_name.empty())
		mission_name = "UNKNOWN_MISSION";

	string spacecraft_name = GetHeaderVal("SASF_SPACECRAFT_NAME", firstSymbolic);

	if(spacecraft_name.empty())
		spacecraft_name = "UNKNOWN_SPACECRAFT";

	string producer_id = GetHeaderVal("SASF_PRODUCER_ID", firstSymbolic);

	if(producer_id.empty())
		producer_id = "UNKNOWN_PRODUCER";

	char hostname[1024];//should be large enough
	hostname[0] = 0;
	gethostname(hostname, sizeof(hostname));

	string project_acronym = GetHeaderVal("SASF_PROJECT_ACRONYM", firstSymbolic);
	if(project_acronym.empty())
		project_acronym = "UNKNOWN_PROJECT_ACRONYM";

	string project = GetHeaderVal("SASF_PROJECT", firstSymbolic);
	if(project.empty())
		project = "UNKNOWN_PROJECT";

	string spacecraft_number = GetHeaderVal("SASF_SPACECRAFT_NUMBER", firstSymbolic);
	if(spacecraft_number.empty())
		spacecraft_number = "UNKNOWN_SPACECRAFT_NUMBER";

	struct timeval currentTime;
	struct timezone currentZone;

	gettimeofday(&currentTime, &currentZone);

	struct tm *timestruct = gmtime((time_t *)&currentTime.tv_sec);

	char timeString[128];
	sprintf(timeString, "%d-%03dT%02d:%02d:%02d",
		      timestruct->tm_year + 1900,
		      timestruct->tm_yday + 1,
		      timestruct->tm_hour,
		      timestruct->tm_min,
		      timestruct->tm_sec);

	out  << "CCSD3ZF0000100000001NJPL3KS0L015$$MARK$$;\r\n";
	out << "MISSION_NAME = " << mission_name << ";\r\n";
	out << "SPACECRAFT_NAME = " << spacecraft_name << ";\r\n";
	out << "DATA_SET_ID = " << data_set_id.c_str() << ";\r\n";
	out << "FILE_NAME = " << DesiredOutputFileName << ";\r\n";
	out << "APPLICABLE_START_TIME = " << StartTime.GetAPFTimeString() << ";\r\n";
	out << "APPLICABLE_STOP_TIME = " << EndTime.GetAPFTimeString() << ";\r\n";
	out << "PRODUCT_CREATION_TIME = " << timeString << ";\r\n";

	out << "PRODUCER_ID = " << producer_id << ";\r\n";
	out << "SEQ_ID = " << seq_id.c_str() << ";\r\n";
	out << "HOST_ID = " << hostname << ";\r\n";
	out << "CCSD3RE00000$$MARK$$NJPL3IF0M01300000001;\r\n";
	out << "$$" << project_acronym << "       SPACECRAFT ACTIVITY SEQUENCE FILE\n";
	out << "**********************************************************************\n";
	out << "*PROJECT    " << project << "\n";
	out << "*SPACECRAFT " << spacecraft_number << "\n";
	out << "*OPERATOR   ";
	
	out << getenv("LOGNAME") << "\n";

	char *ascii_time = asctime(timestruct);
	ascii_time[24] = '\0';//terminate this string
	out << "*DATE       " << ascii_time << "\n";
	out << "*APGEN      " << get_apgen_version() << "\n";
	out << "*BEGIN      " << StartTime.GetAPFTimeString() << "\n";
	out << "*CUTOFF     " << EndTime.GetAPFTimeString() << "\n";
	out << "*TITLE      APGEN PLAN\n";

	if( !Epochs.empty() ) 
	{
		out << "*EPOCHS_DEF\n";
		EpochVect::const_iterator iter= Epochs.begin();
		while(iter != Epochs.end())
		{
		  out << "*" << iter->GetName();
		  out << ",       " << iter->GetOrigin().to_string() << "\n";
		  iter++;
		}

		out << "*EPOCHS_END\n"; 
	}

	out << "*Input Files used:\n";
	out << "*File Type  Last Modified                   File Name\n";
	out << "**********************************************************************\n";
	out << "*SC_MODEL\n";
	out << "*CATALOG\n";
	out << "*LEGENDS\n";
	out << "*SEQUENCE\n";
	out << "*RESOLUTION\n";
	out << "*LIGHTTIME\n";
	out << "*RULES\n";
	out << "*DEFINITION\n";
	out << "*CLOCK\n";
	out << "*REQUESTS\n";
	out << "*CONDITIONS\n";
	out << "*SCRIPT\n";
	out << "*MASK\n";
	out << "*ALLOCATION\n";
	out << "*VIEWPERIOD\n";
	out << "*TELEMETRY\n";
	out << "*GEOMETRY\n";
	out << "*BG_SEQUENCE\n";
	out << "*CONTEXT\n";
	out << "*DEP_CONTEXT\n";
	out << "*REDUNDANT\n";
	out << "*OPTG_FD\n";
	out << "*EVENTS\n";
	out << "**********************************************************************\n";
	out << "$$EOH\n";

	//
	// old APGen output cyclics if any were defined. That was ripped out, but
	// we still need to indicate the end of the definitions section:
	//
	out << "$$EOD\n";
}

//instantiates the implementation
IO_SASFWrite *IO_SASFWrite::CreateSASFWrite() {
	return new IO_SASFWriteImpl();
}
