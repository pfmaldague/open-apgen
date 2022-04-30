#ifndef IO_SASF_WRITE_IMPL_H
#define IO_SASF_WRITE_IMPL_H

#include "IO_SASFWrite.H"
#include "C_string.H"

#include <algorithm>
#include <numeric>
#include <map>
#include <set>

using std::string;
using std::ostream;

class SASF_ActivityInstance;

// define a few typedefs using the stl

// A map String->SASF_ActivityInstance the string is the id of the instance
typedef std::map<string, SASF_ActivityInstance*> StringSASFActivityMap; 

// A vector of SASF_ActivityInstances
typedef std::vector<SASF_ActivityInstance*> SASF_ActivityPtrVect;

// A vector of IO_TypedValStrings
typedef std::vector<IO_TypedValString> TypedStringVect;


//
// An activity instance class for SASF Output.
// This class represents an activity instance in the plan with
// some extra helping functions used to simplify getting SASF
// attributes and navigating the parent child hierarchy.
//
// In addition to representing activity instances in the plan,
// an instance of this class is also used to represent the
// top-level "request" in the output SASF. Such a top-level
// request may not be necessary if all activities to be output
// have a parent activity with an SASF type attribute of "request".
// But activities that represent an SASF step (e. g. a note or
// a command) need a request to hold them. The top-level activity
// (somewhat misleadingly called "Plan") acts as a catchall
// request for such steps.
//
// In IO_Activity Instances, the parent to child pointers are
// represented as id strings (similarly the child to parent pointers).
// In this class, the pointers are real pointers.
//
class SASF_ActivityInstance {

private:
	// the encapsulated activity instance
	IO_ActivityInstance	ActivityInstance;
	
	// the array of children
	SASF_ActivityPtrVect ChildInstances;
	
	/**the potentially null parent pointer
	   Note: there is a top level Plan activity instance that is the parent of
	   all the parentless instances*/
	SASF_ActivityInstance*	ParentInstance;
	
	// generated during creation, this is a useful identifier for each activity instance
	long ID;
	
	// block the calling of this for now (by making it private)
	SASF_ActivityInstance& operator=(const SASF_ActivityInstance& rhs) = delete;

public:
	explicit SASF_ActivityInstance(long id);
	SASF_ActivityInstance(long id, const IO_ActivityInstance& instance);
	SASF_ActivityInstance(const SASF_ActivityInstance& instance);
	~SASF_ActivityInstance();

	// Adds a child pointer
	void AddChildActivityPtr(SASF_ActivityInstance* childInstance);
	// sets a parent pointer
	void SetParentActivityPtr(SASF_ActivityInstance* parentInstance);
  
	// gets the number of children
	unsigned long GetNumChildren() const;

	/* gets the number of children recursively.
	   Internal nodes are counted.  */
	unsigned long GetNumChildrenRecursive() const;

	long GetID() const {
		return ID;
	}
	string GetAPGENID() const {
		return ActivityInstance.GetID();
	}
	string GetAPGENType() const {
		return ActivityInstance.GetType();
	}
	vector<TypedValue> GetAPGENParameters() const {
		return ActivityInstance.GetParameters();
	}

	// returns highest encapsulating request or NULL
	const SASF_ActivityInstance* GetRequestInstance() const; 
  
	string GetFileName() const;
	string GetDescription() const;
	bool IsRequest() const;
  
	/* hierarchy_traversal
	   These methods help traverse the SASF_ActivityInstance heirarchy */
	SASF_ActivityInstance* GetParentInstance() const {
		return ParentInstance;
	}
	SASF_ActivityInstance* GetChildInstance(unsigned long childIndex) const;

	// start time 
	IO_Time* GetStartTime() const;

	/**@name SASFaccessors
	   generic SASF attribute accessors */
	bool HasSASFAttribute() const;
	map<string, TypedValue> GetSASFAttributes() const;

	map<string, TypedValue> GetAttributes() const {
		return ActivityInstance.GetAttributes();
	}
	vector<TypedValue> GetParameters() const {
		return ActivityInstance.GetParameters();
	}
	const TypedValue& GetAttribute(const string& attName) const {
		map<string, int>::const_iterator iter = ActivityInstance.AttributeMap.find(attName);
		if(iter != ActivityInstance.AttributeMap.end()) {
			return ActivityInstance.Attributes[iter->second];
		}
		Cstring errs;
		errs << "GetAttribute: attr. " << attName.c_str() << " not found.";
		throw(eval_error(errs));
	}
};

//
// Interface class to represent an SASF step. Derived classes:
//
//		SASF_GenericStep
//		SASF_ActivityStep
//		SASF_CommandStep
//		SASF_NoteStep
//		SASF_NoteEndStep
//
class SASF_Step {
	// block calling of assignment operator because it is unimplemented and the default might be unappropriate
	SASF_Step& operator=(const SASF_Step& rhs) = delete;

	//
	// don't delete this (it will get freed later) this may be NULL
	// when the step doesn't correspond to an activity
	//
	const SASF_ActivityInstance* InstancePtr;

	//
	// offset from start of enclosing step sequence or request
	//
	IO_Duration 		FromStart;

protected:
	IO_Duration		GetFromStart() const {
		return FromStart;
	}
	long			GetInstanceDuration(IO_Duration* dur) const;
	long			CiGetSASFStringAttribute(const string& attrName, string& attributeVal) const;
	string			GetExtraSASFAttributesString() const;
	bool			GetSASFAttributeString(const string& attrName, string& attributeVal) const;

	vector<TypedValue>	GetParameters() const;

	string			GetType() const;
	string			GetDescription() const;

public:

	SASF_Step(const SASF_ActivityInstance* instanceRef)
		: FromStart(), InstancePtr(instanceRef) {}
	virtual ~SASF_Step() {}

	// static class initializer, initializes the ReservedAttributeNames Vector
	static void		Init();

	// holds the names of the reserved sasf attribute names
	static StringVect	ReservedAttrNames;

	void			SetTimeFromSequenceStart(CTime_base fromStart) {
		FromStart = fromStart;
	}
	IO_Duration		GetTimeFromSequenceStart() const {
		return FromStart;
	}
	// gets SASF string representation of this step
	virtual string		GetStepString(long stepNum) const = 0;
	const SASF_ActivityInstance* get_instance() const {
		return InstancePtr;
	}

	// virtual copy-constructor
	virtual SASF_Step*	Clone() const = 0;
};

//
// This function is used in producing SASF output.
// It generates a string with the extra non sasf attributes in it.
//
// NOTE: this is obsolete, as there is no way for an activity instance
// to have non-standard attribute in APGenX.
//
string GetExtraSASFAttributesString(const TypedStringVect &sasfAttributes, const StringVect& toOmit);

// for ordering steps -- we do this to facilitate the matching of step sequences

struct StepPtrLess : public std::binary_function<SASF_Step*, SASF_Step*, bool> {
        bool operator() (const SASF_Step* step1, const SASF_Step* step2) const;
};

//
// A multiset of SASF_Step pointers
//
typedef std::multiset<SASF_Step*, StepPtrLess> SASF_StepPtrMSet;

//
// Represents a sequence of steps. These can be merged with other Step
// sequences via the function MergeStepSequences
//
class SASF_StepSeq {
private:

	// the start time
	IO_Time*			SequenceStartTime;
	
	//
	// Corresponds to the SASF_ActivityInstance that the request goes with.
	// We only merge step sequences that have the same request ID
	//
	const SASF_ActivityInstance*	Request;

	//
	// the stepSequence
	//
	SASF_StepPtrMSet		Steps;

					/* private function called by MergeStepSequences
					   can only merge sequences later than yourself */
	void				MergeSequenceLater(const SASF_StepSeq& toMerge); 

					// private function used by StepEquivalent and by GetRequestString
	string				requestToStringWithStartTime(const IO_Time* startTime) const;

					// where we get the Request attributes from.
	map<string, TypedValue>		RequestAttributes;

public:
	SASF_StepSeq(
			const SASF_ActivityInstance* request,
			IO_Time* startTime,
			const map<string, TypedValue>& requestAttributes);
	SASF_StepSeq(
			const SASF_StepSeq& sequence);
	SASF_StepSeq& operator=(
			const SASF_StepSeq& rhs);
	~SASF_StepSeq();

	IO_Time* GetStartTime() const {
		return SequenceStartTime->Clone();
	}
	long GetID() const;
	const SASF_ActivityInstance* GetRequestInstance() const {
		return Request;
	}
	bool Empty() const { return Steps.empty(); }
	map<string, TypedValue> GetRequestAttributes() const {
		return RequestAttributes;
	}

	long CiGetSASFStringAttribute(const string& attrName, string& attributeVal) const;

	// returns the string that represents this StepSequence as a request
	string GetRequestString() const;

	// determines if two step sequences are the same (useful for cyclic detection)
	bool StepEquivalent(const SASF_StepSeq& otherSeq) const;

	// calls steps virtual copy-constructor
	void AddStep(const SASF_Step *step); 

	// sets the start time for the step sequence
	void SetStartTime(const IO_Time *startTime);

	// Adds this duration to each of the steps
	void UpdateSteps(const IO_Duration &dur);

	// to use MergeSequence later
	friend void MergeStepSequences(
			const SASF_StepSeq& firstToMerge,
			const SASF_StepSeq& secondToMerge,
			SASF_StepSeq* mergedSequence);

	// for debug output
	friend ostream& operator<<(ostream& s, const SASF_StepSeq& req);
};

// Merges two step sequences together
void MergeStepSequences(
		const SASF_StepSeq& firstToMerge,
		const SASF_StepSeq& secondToMerge,
		SASF_StepSeq* mergedSequence);

//
// For ordering of Step sequences.
// this helps keeps our step sequences in a readily mergable ordering at all times
//
struct SequenceLess : public std::binary_function<SASF_StepSeq, SASF_StepSeq, bool> {
	// operator used by sort or map or set
        bool operator()(const SASF_StepSeq& req1, const SASF_StepSeq& req2) const;
};

// a multiset of step sequences
typedef std::multiset<SASF_StepSeq, SequenceLess> SASF_StepSeqMSet;

// a vector of step sequences
typedef std::vector<SASF_StepSeq> SASFStepSeqVect;

// a map of string->IO_TypedValue* 
typedef std::map<string, IO_TypedValue*> TypedValPtrMap;

// a vector of Epochs
typedef std::vector<IO_Epoch> EpochVect;

//
// The main high-level class in this subsystem.
//
// In Adam Chase's original design, this class was a "client" and
// the rest of APGen acted as a "server" which sent all the activities
// in the plan to the client. A new instance of a client was created
// whenever a file needed to be output.
//
// While there was consistency and a certain kind of elegance to this
// design, the main flaw was that it put too much emphasis on design
// features that were not permanent - for example, cyclics, which have
// been pretty much forgotten since Galileo and Cassini.
//
// Also, the implementation was pretty rigid - large number of typedef
// statements to hide the original definition in terms of STL templates.
// This made the code very hard to read if you didn't use it every
// day.
//
// The refactored design is that IO_SASFWriteImpl is a singleton, whose
// SASFwrite() method is invoked every time it's needed. The singleton
// is cleared before reuse, and since it is invoked via a static method
// it can be accessed from anywhere in the APGenX code.
//
class IO_SASFWriteImpl : public IO_SASFWrite {

private:
	/* userOptions */
	string DesiredOutputFileName;
	StringVect SymbolicFilesToInclude;
	// exclude instances that start before this
	IO_TimeExact StartTime;
	// exclude instances that start after this
	IO_TimeExact EndTime;
	// whether to include requests that start at the endTime or not
	long InclusionFlag; 

	// name to value
	map<string, TypedValue> GlobalMap;

	// the epochs
	EpochVect Epochs;

	// the plan
	SASF_ActivityInstance* Plan;

	// a hash to activity pointers
	StringSASFActivityMap ActivityMap;

	// for use of generating activity instance ids
	long NextActivityInstanceID;

	// a debugging output function
	void VerifyStructures();

	// for getting header values out of the globals easily
	string GetHeaderVal(
			const string& globalName,
			const string& symbolicFileName) const;

	// main function which traverses the instance heirarchy
	void AddSequences(
			SASF_StepSeqMSet* sequenceVect,
			const SASF_ActivityInstance* instance);

	// adds a step sequence to the sequence vect if there is one to add in instance
	void AddInstanceSequence(
			SASF_StepSeqMSet* sequenceVect,
			const SASF_ActivityInstance* instance) const;
  
	// combines sequences that can be merged
	void CombineSequences(
			SASF_StepSeqMSet* sequenceVect);
	// merges sequences
	void MergeSequences(
			SASF_StepSeqMSet::const_iterator begin,
			SASF_StepSeqMSet::const_iterator end,
			SASF_StepSeq* mergedSequence) const;

	// writes the Header
	void WriteHeader(aoString & out) const;

	// filters by starttime criteria
	void FilterRequests(
			SASFStepSeqVect* requests) const;

	/* update request start times.
	   Sets the start of the request to be the earlier of 
	   the activity/request that represents the request 
	   and the first step in the request*/
	void UpdateRequestStartTimes(SASFStepSeqVect& stepSeq) const;

	// writes the sasf file body
	void WriteBody(
			aoString & out,
			const SASFStepSeqVect& stepSeq) const;


public:
	// constructor
	IO_SASFWriteImpl();
	// destructor
	~IO_SASFWriteImpl();

	void clear() {
		DesiredOutputFileName = "";
		SymbolicFilesToInclude.clear();
		Epochs.clear();
		NextActivityInstanceID = 0;
		if(Plan) {
			delete Plan;
			Plan = new SASF_ActivityInstance(
					NextActivityInstanceID++);
		}
		ActivityMap.clear();
		GlobalMap.clear();
	}
	SASF_ActivityInstance* get_plan() {
		return Plan;
	}

	static IO_SASFWriteImpl& SASFwriter();

	long BeginGlobalsSection() {
		return 1;
	}
	long EndGlobalSection() {
		return 1;
	}
	long BeginEpochsSection() {
		return 1;
	}
	long EndEpochsSection() {
		return 1;
	}

	/* IO_ApgenDataClient functions that IO_SASFWrite implements */
	virtual long GetClientName(string* nameString) const {
		*nameString = "SASF";
		return 1;
	}
	virtual long AddOneGlobal(const string& name, const TypedValue& value);
	virtual long AddOneEpoch(const IO_Epoch& value);
	virtual long AddOneActivity(const IO_ActivityInstance& instance);

	//
	// writes the sasf file - throws eval_error if errors
	//
	virtual void WriteSASF(IO_SASFWriteOptions &options);
	// gets some user data from the input data
	virtual long GetAvailableSymbolicFiles(StringVect* vect) const;
};

// an exception class for if there is a problem with the activity instance setup
class SASFException : public std::exception {
private:
	string What;

public:
	SASFException(const string& what) : What(what) {}
	~SASFException() throw() {}
	virtual const char* what() const throw() { return What.c_str();} };

class SASF_StepSeq;

// a generic step type used for debugging and before the other types were implemented yet
class SASF_GenericStep : public SASF_Step {

public:

	// constructor
	SASF_GenericStep(SASF_ActivityInstance* instanceRef) : SASF_Step(instanceRef) {}

	/* SASF_StepMethods
	   must implement pure virtual inherited methods */
	virtual string GetStepString(long stepNum) const;
	virtual SASF_Step* Clone() const { return new SASF_GenericStep(*this); } };


// another of the step implementations, (this one is for activity steps)
class SASF_ActivityStep : public SASF_Step {
public:

	SASF_ActivityStep(const SASF_ActivityInstance* instancePtr)
		: SASF_Step(instancePtr) {};

	/* SASF_StepMethods
	    must implement pure virtual inherited members */
	virtual string GetStepString(long stepNum) const;
	virtual SASF_Step* Clone() const { return new SASF_ActivityStep(*this); }
	long CiGetSASFStringAttribute(const string& attrName, string& attributeVal) const;
	};

// Command strp implementation of the step interface
class SASF_CommandStep : public SASF_Step {

public:

	// constructor
	SASF_CommandStep(const SASF_ActivityInstance* instanceRef)
		: SASF_Step(instanceRef) {};

	/* SASF_StepMethods
	   Must implement pure virtual inherited methods */
	virtual string GetStepString(long stepNum) const;
	virtual SASF_Step* Clone() const { return new SASF_CommandStep(*this); }
	long CiGetSASFStringAttribute(const string& attrName, string& attributeVal) const;
	};

// notes implementation of the step class
class SASF_NoteStep : public SASF_Step {
public:
	// constructor
	SASF_NoteStep(const SASF_ActivityInstance* instancePtr)
		: SASF_Step(instancePtr){};

	/* SASF_StepMethods
	   Must implement pure virtual inherited methods */
	virtual string GetStepString(long stepNum) const;
	virtual SASF_Step* Clone() const { return new SASF_NoteStep(*this); }
	long CiGetSASFStringAttribute(const string& attrName, string& attributeVal) const;
	};

class SASF_NoteEndStep : public SASF_Step {
public:
	// constructor
	SASF_NoteEndStep(const SASF_ActivityInstance* instancePtr)
		: SASF_Step(instancePtr){};

	/* SASF_StepMethods
	   Must implement pure virtual inherited methods */
	virtual string GetStepString(long stepNum) const;
	virtual SASF_Step* Clone() const { return new SASF_NoteEndStep(*this); }
	long CiGetSASFStringAttribute(const string& attrName, string& attributeVal) const;
	};



//
// this is a merger -- it merges a number of step sequences that can be merged by
// calling MergeStepSequences on them
//
class MergeSequence: public std::unary_function<SASF_StepSeq, void> {

private:
	// 
	SASF_StepSeq AccumulatedSequence;

public:

	// constructor
	MergeSequence(const SASF_StepSeq& initialSequence)
		: AccumulatedSequence(initialSequence)
	{}

	// operator that is called in a foreach or accumulate
	void operator()(const SASF_StepSeq& sequence);

	// returns the result
	SASF_StepSeq GetAccumulatedSequence() const { return AccumulatedSequence; }
};

// a set of strings
typedef std::set<string> StringSet;

// for getting the symbolic file names here
class GetSymbolicFileSet : public std::unary_function<StringSASFActivityMap::value_type, void> {
private:

	// 
	StringSet Files;

public:
	// called by foreach to accumulate the symbolic filenames
	void operator()(const StringSASFActivityMap::value_type& mapElem);
	// 
	void GetFiles(StringVect* vect);
};


//
// for ordering step sequences
//
struct RequestLess : public std::binary_function<SASF_StepSeq, SASF_StepSeq, bool> {
	bool operator()(const SASF_StepSeq &req1, const SASF_StepSeq &req2) {
	//ordering is on startTime
	IO_Time *req1Time = req1.GetStartTime();
	CTime_base base1 = req1Time->GetEvaluatedTime();
	delete req1Time;

	IO_Time *req2Time = req2.GetStartTime();
	CTime_base base2 = req2Time->GetEvaluatedTime();
	delete req2Time;

	return (base1 < base2);
	}
};



#endif
