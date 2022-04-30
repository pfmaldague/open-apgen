#ifndef IO_APGEN_DATA_H
#define IO_APGEN_DATA_H

#include "UTL_stl.H"
// #include "IO_ApgenEnums.h"

using namespace std;
#include <string>
#include <list>

#include "TypedValue.H"
#include <iostream>
#include <sstream>
#include <APbasic.H>

#define SIGNIFICANT_DIGITS_MAX 13

class IO_TypedValueArray;

class IO_WindowSize
{

public:
	IO_WindowSize();
	void Set(long heightPixels, long widthPixels);
	void Get(long *heightPixels, long *widthPixels) const;
private:
	long HeightPixels;
	long WidthPixels;

};

double roundPlaces(double number, long numPlaces);

class IO_Time;

class IO_TimeEpochRelative;

class IO_Epoch {

public:

	IO_Epoch() {};
	IO_Epoch(const string& name, const CTime_base& time);
  
	// sets
	void SetOrigin(const CTime_base& origin);
	void SetName(const string& name);
 
	// gets
	const CTime_base GetOrigin() const {
		return Origin;
	}
	const string GetName() const {
		return Name;
	}

	// time Relative stuff
	CTime_base GetEvaluatedTime(bool before, const CTime_base& relativeTime) const;
	IO_TimeEpochRelative* GetRelativeTime(const IO_Time* time) const;

private:

	CTime_base Origin;
	string Name;

};

class IO_TimeSystemRelative;

class IO_TimeSystem
{
public:
  
	IO_TimeSystem();

	// Sets
	void SetName(const string& name);
	void SetOrigin(const CTime_base& origin);
	void SetScale(double scale);

	// Gets
	const string GetName() const;
	const CTime_base GetOrigin() const;
	const double GetScale() const;

	// time Relative stuff
	CTime_base GetEvaluatedTime(
			bool before,
			const CTime_base& relativeTime) const;
	IO_TimeSystemRelative* GetRelativeTime(const IO_Time* time) const;

private:

	string Name;
	CTime_base Origin;
	double Scale;
};


class IO_Legend
{
public:
	IO_Legend();
	IO_Legend(const string &name, unsigned long height);

	void SetName(const string& name);
	void SetHeight(unsigned long height);
	void GetName(string* name) const;
	void GetHeight(unsigned long* height) const;
private:

	string Name;
	unsigned long Height;
};

class IO_Time;

class IO_ApgenTypedValDataClient
{
public:

  IO_ApgenTypedValDataClient(){};
  virtual ~IO_ApgenTypedValDataClient(){};

  // forces compile errors if we forget a type or add one
  virtual void UnInitializedReceived() = 0;
  virtual void BoolReceived(bool value) = 0;
  virtual void IntReceived(long value) = 0;
  virtual void DoubleReceived(double value) = 0;
  virtual void StringReceived(const string& value) = 0;
  virtual void TimeReceived(const IO_Time& value) = 0;
  virtual void ArrayReceived(const IO_TypedValueArray& array) = 0;
  virtual void InstanceReceived(const string& instanceString) = 0;
  virtual void DurationReceived(const CTime_base& duration) = 0;

private:

  // block copying and assignment
  IO_ApgenTypedValDataClient(const IO_ApgenTypedValDataClient& client);
  IO_ApgenTypedValDataClient& operator=(const IO_ApgenTypedValDataClient& rhs);

};

class IO_TypedValue {
public:
  
	IO_TypedValue() : Name(), ValueString() {}
	virtual ~IO_TypedValue(){};

	void SetName(const string& name) {
		Name = name;
	}
	// virtual copy constructor
	virtual IO_TypedValue* Clone() const = 0;
	virtual apgen::DATA_TYPE GetType() const = 0;
	string GetName() const {
		return Name;
	}
	void SetAPFValueString(const string& valueString) {
		ValueString = valueString;
	}
	string GetAPFValueString() const;

protected:

	// disallow mixed-type assignments
	IO_TypedValue& operator=(const IO_TypedValue& rhs);
	IO_TypedValue(const IO_TypedValue& rhs)
		: Name(rhs.Name),
			ValueString(rhs.ValueString) {}

private:
	string Name;
	string ValueString;
};

class StringBreak {
public:
	StringBreak()
		: Break(true),
		  Data()
	{
	};
	StringBreak(const string& str)
		: Data(str),
		  Break(false)
	{
	};

	bool IsBreak() const {return Break;};
	string GetString() const {return Data;};

private:
	string Data;
	bool Break; };

typedef std::vector<StringBreak> SBVect;

class IO_TypedValBool : public IO_TypedValue
{
public:

	IO_TypedValBool(bool value)
			: Value(apgen::DATA_TYPE::BOOL_TYPE) {
		Value = value; }
	IO_TypedValBool(const TypedValue& val)
			: Value(val, apgen::DATA_TYPE::BOOL_TYPE) {}

	virtual apgen::DATA_TYPE GetType() const {
		return apgen::DATA_TYPE::BOOL_TYPE;
	};
	TypedValue GetValue() const {return Value;};
	virtual IO_TypedValBool* Clone() const {
		return new IO_TypedValBool(*this);
	};

private:

	TypedValue Value;

};


class IO_Duration : public IO_TypedValue {
public:
	IO_Duration() : Value(0, 0, true) {};
	IO_Duration(const IO_Duration& value) : Value(value.Value) {
		assert(Value.is_duration());
	}
	IO_Duration(CTime_base T) : Value(T) {
		assert(Value.is_duration());
	}

	// gets 
	virtual apgen::DATA_TYPE GetType() const {return apgen::DATA_TYPE::DURATION;};
	virtual IO_Duration* Clone() const {return new IO_Duration(*this);};
	CTime_base GetBase() const {
		return Value;
	}

	bool operator<(const IO_Duration& otherDuration) const;
	bool operator>(const IO_Duration& otherDuration) const;
	bool operator==(const IO_Duration& otherDuration) const {
		return Value == otherDuration.Value;
	}
	string GetAPFDurationString() const {
		return string(*Value.to_string());
	}

	// sets
	void operator+=(const IO_Duration& otherDuration);
	void operator-=(const IO_Duration& otherDuration);
	IO_Duration& operator=(const IO_Duration& rhs);

	friend std::ostream& operator<<(std::ostream& s, const IO_Duration& dur);

private:

	CTime_base Value;
  
};


class IO_ApgenTimeDataClient {
public:
	IO_ApgenTimeDataClient() {};
	virtual ~IO_ApgenTimeDataClient(){};
	virtual void EpochRelativeReceived(const IO_Epoch& epoch, bool beforeEpoch, const CTime_base& relativeTime) = 0;
	virtual void ExactReceived(const CTime_base& timeBase) = 0;

private:

	// block copying and assignment
	IO_ApgenTimeDataClient(const IO_ApgenTimeDataClient& client);
	IO_ApgenTimeDataClient& operator=(const IO_ApgenTimeDataClient& rhs);

};

class IO_TimeEpochRelative;
class IO_TimeExact;

class IO_Time : public IO_TypedValue {
public:

	IO_Time() : IO_TypedValue() {};
	virtual ~IO_Time() {};// virtual destructor
	virtual apgen::DATA_TYPE GetType() const {return apgen::DATA_TYPE::TIME;};
	virtual IO_Time* Clone() const = 0;
	virtual CTime_base GetEvaluatedTime() const = 0;
	virtual string GetAPFTimeString(bool includeMS = true) const = 0;
	IO_Duration GetSubtract(const IO_Time& otherTime) const;
	bool operator<(const IO_Time& otherTime) const;
	bool operator==(const IO_Time& otherTime) const;
	//these are to determine if times are compatible
	//i.e will changes to epochs or timeSystems have no affect their time relative
	//to each other
	virtual bool RelativeCompatible(const IO_Time* otherTime) const = 0; //implement this!
	virtual bool RelativeCompatible(const IO_TimeEpochRelative* otherTime) const {return false;};
	virtual bool RelativeCompatible(const IO_TimeExact* otherTime) const {return false;};


protected:

	// disallow mixed type assignments
	IO_Time& operator=(const IO_Time& rhs);
	IO_Time(const IO_Time& rhs) : IO_TypedValue(rhs) {};

};

class IO_TimeEpochRelative : public IO_Time {

public:

	// maybe hide this constructor
	IO_TimeEpochRelative(
			const IO_Epoch& epoch,
			bool beforeEpoch,
			const CTime_base& relativeTime);
	IO_TimeEpochRelative(
			const IO_TimeEpochRelative& rhs);
	IO_TimeEpochRelative& operator=(const IO_TimeEpochRelative& rhs);

	~IO_TimeEpochRelative(){};
	virtual IO_TimeEpochRelative* Clone() const {
		return new IO_TimeEpochRelative(*this);
	}
	virtual CTime_base GetEvaluatedTime() const;
	virtual CTime_base GetTime() const {
		return RelativeTime;
	}
	virtual string GetAPFTimeString(bool includeMS = true) const;
	// using IO_Time::RelativeCompatible;
	virtual bool RelativeCompatible(const IO_Time* otherTime) const {
		return otherTime->RelativeCompatible(this);
	}
	// using IO_Time::RelativeCompatible;
	virtual bool RelativeCompatible(const IO_TimeEpochRelative* otherTime) const;  
private:
	IO_Epoch Epoch;
	bool BeforeEpoch;
	CTime_base RelativeTime;
};

class IO_TimeSystemRelative : public IO_Time {

public:
	// maybe hide this constructor
	IO_TimeSystemRelative(
			const IO_TimeSystem& system,
			bool beforeSystem,
			const CTime_base& relativeTime);
	IO_TimeSystemRelative(const IO_TimeSystemRelative& rhs);
	IO_TimeSystemRelative& operator=(const IO_TimeSystemRelative& rhs);

	~IO_TimeSystemRelative(){};
	virtual IO_TimeSystemRelative* Clone() const {
		return new IO_TimeSystemRelative(*this);
	}
	virtual CTime_base GetEvaluatedTime() const;
	virtual string GetAPFTimeString(bool includeMS = true) const;

	// relative compatible stuff
	// using IO_Time::RelativeCompatible;
	virtual bool RelativeCompatible(const IO_Time* otherTime) const {
		return otherTime->RelativeCompatible(this);
	}

	// using IO_Time::RelativeCompatible;
	virtual bool RelativeCompatible(const IO_TimeSystemRelative* otherTime) const;
	// adds a duration in the scale of the time system

private:
	IO_TimeSystem TimeSystem;
	bool BeforeSystem;
	CTime_base RelativeTime;

};

class IO_TimeExact : public IO_Time {

public:
	// maybe hide this constructor
	IO_TimeExact() : ExactTime(), IO_Time() {};
	IO_TimeExact(const CTime_base&);
	IO_TimeExact(const IO_TimeExact&);
	~IO_TimeExact(){};

	IO_TimeExact& operator=(CTime_base const& T) {
		ExactTime = T;
		return *this;
	}
	virtual IO_TimeExact* Clone() const {
		return new IO_TimeExact(*this);
	}
	virtual CTime_base GetEvaluatedTime() const;
	virtual string GetAPFTimeString(bool includeMS = true) const;

	// relative compatible stuff
	// using IO_Time::RelativeCompatible;
	virtual bool RelativeCompatible(const IO_Time* otherTime) const  {
		return otherTime->RelativeCompatible(this);
	}

	// using IO_Time::RelativeCompatible;
	virtual bool RelativeCompatible(const IO_TimeExact* otherTime) const {
		return true;
	}
private:
	CTime_base ExactTime;

};

class IO_TypedValUnInitialized : public IO_TypedValue {
public:

	virtual apgen::DATA_TYPE GetType() const {return apgen::DATA_TYPE::UNINITIALIZED;};
	virtual void GetValue(IO_ApgenTypedValDataClient* client) const {client->UnInitializedReceived();};
	virtual IO_TypedValUnInitialized* Clone() const {return new IO_TypedValUnInitialized();};
};

class IO_TypedValInt : public IO_TypedValue
{
public:
  
	IO_TypedValInt(long value) : Value(apgen::DATA_TYPE::INTEGER) {
		Value = value;
	}
	// default copy constructor is fine

	virtual apgen::DATA_TYPE GetType() const {return apgen::DATA_TYPE::INTEGER;};
	virtual void GetValue(IO_ApgenTypedValDataClient* client) const {
		client->IntReceived(Value.get_int());
	}
	TypedValue GetValue() const {return Value;};
	virtual IO_TypedValInt* Clone() const {
		return new IO_TypedValInt(*this);
	}
private:

	TypedValue Value;
};

class IO_TypedValDouble : public IO_TypedValue
{
public:
	IO_TypedValDouble(double value) : Value(apgen::DATA_TYPE::FLOATING) {
		Value = value;
	}

	virtual apgen::DATA_TYPE GetType() const {return apgen::DATA_TYPE::FLOATING;};
	TypedValue GetValue() const {return Value;};
	virtual IO_TypedValDouble* Clone() const {return new IO_TypedValDouble(*this);};

private:
	TypedValue Value;

};

class IO_TypedValString : public IO_TypedValue
{
public:

	IO_TypedValString(string value) : IO_TypedValue(), Value(value) {};
  
	virtual apgen::DATA_TYPE GetType() const {return apgen::DATA_TYPE::STRING;};
	string GetValue() const {return Value;};
	virtual IO_TypedValString* Clone() const {return new IO_TypedValString(*this);};

private:

	string Value;


};

class IO_TypedValInstance : public IO_TypedValue
{
public:

	IO_TypedValInstance(const TypedValue value) : Value(value) {
	}

	virtual apgen::DATA_TYPE GetType() const {
		return apgen::DATA_TYPE::INSTANCE;
	}
	TypedValue GetValue() const {
		return Value;
	}
	virtual IO_TypedValInstance* Clone() const {
		return new IO_TypedValInstance(*this);
	}

private:
	TypedValue Value;
};

typedef vector<IO_TypedValue*> IO_TypedValueVect;

class IO_TypedValueArray : public IO_TypedValue {
public:
	explicit IO_TypedValueArray(TypedValue::arrayType indexType = TypedValue::arrayType::UNDEFINED);
	IO_TypedValueArray(const TypedValue& val) {
		assert(val.is_array());
		Value.recursively_copy(val.get_array());
	}


	IO_TypedValueArray(const IO_TypedValueArray& array) {
		Value.recursively_copy(array.get_array());
	}
	virtual ~IO_TypedValueArray();
	IO_TypedValueArray& operator=(const IO_TypedValueArray& rhs);

	virtual apgen::DATA_TYPE GetType() const = 0; // to be defined by our subclasses
	virtual IO_TypedValueArray* Clone() const = 0;//again by our subclasses

	TypedValue GetElement(const string& name) const;
	TypedValue GetElement(unsigned long index) const;

	// copies
	// bool AddElement(const IO_TypedValue* value);
	// bool AddElement(const IO_TypedValue& value);
	// void SetIndexType(long arrayIndexType);

	TypedValue::arrayType GetIndexType() const {
		return Value.get_array().get_array_type();
	}

	unsigned long GetNumElements() const {
		return Value.get_array().get_length();
	}
	// PFM addition
	const ListOVal& get_array() const {
		return Value.get_array();
	}

private:

	TypedValue Value;
	void UnInit();
};

class IO_TypedValueArraySet : public IO_TypedValueArray
{
public:
	IO_TypedValueArraySet()
		{}
	IO_TypedValueArraySet(const TypedValue& val)
		: IO_TypedValueArray(val) {
		assert(val.is_array());
	}
	virtual apgen::DATA_TYPE GetType() const {
		return apgen::DATA_TYPE::ARRAY;
	}
	virtual IO_TypedValueArraySet* Clone() const {
		return new IO_TypedValueArraySet(*this);
	}
};

class IO_TypedValueArrayAppend : public IO_TypedValueArray
{
public:
	IO_TypedValueArrayAppend() {}
	IO_TypedValueArrayAppend(const TypedValue& val)
		: IO_TypedValueArray(val) {
			assert(val.is_array());
	}
	virtual apgen::DATA_TYPE GetType() const {
		return apgen::DATA_TYPE::ARRAY;
	}
	virtual IO_TypedValueArrayAppend* Clone() const {
		return new IO_TypedValueArrayAppend(*this);
	}
};

class IO_ActivityInstance
{

public:

	IO_ActivityInstance();
	~IO_ActivityInstance(){};

	// generalsets
	void SetName(const string &name){Name = name;};
	void SetType(const string &type){Type = type;};
	void SetID(const string& id){ID = id;};
	void SetIsRequest(long isRequest) {}
	void SetIsChameleon(long isChameleon) {}
	void SetExpansionState(long displayState){ExpansionState=displayState;};
	void SetDecompositionType(long decompositionType){DecompositionType = decompositionType;};
	void AddChildID(const string &childID);
	void SetParentID(const string &parentID);

	void AddAttribute(const string& name, const TypedValue& val) {
		map<string, int>::const_iterator iter = AttributeMap.find(name);
		if(iter != AttributeMap.end()) {
			Cstring errs;
			errs << "AddAttribute: attr. \"" << name
				<< "\" already exists";
			throw(eval_error(errs));
		}
		AttributeMap[name] = Attributes.size();
		Attributes.push_back(val);
	}

	void AddParameter(const string& name, const TypedValue& val) {
		map<string, int>::const_iterator iter = ParameterMap.find(name);
		if(iter != ParameterMap.end()) {
			Cstring errs;
			errs << "AddParameter: attr. \"" << name
				<< "\" already exists";
			throw(eval_error(errs));
		}
		ParameterMap[name] = Parameters.size();
		Parameters.push_back(val);
	}


	//gets

	// generalgets
	const string		GetName() const { return Name; }
	const string		GetType() const { return Type; }
	const string		GetID() const { return ID.size() ? ID : "ID Never Defined"; }
  
	long			GetIsRequest() const;
	long			GetIsChameleon() const { return 0; }
	long			GetExpansionState() const { return ExpansionState; }
	long			GetDecompositionType() const { return DecompositionType; }
	unsigned long		GetNumSubActivities() const;
	const string		GetSubActivityID(unsigned long subActivityIndex) const;
	const string		GetParentActivityID() const { return ParentActivityID; }

	map<string, TypedValue>	GetAttributes() const {
		map<string, TypedValue> reqAttributes;
		map<string, int>::const_iterator iter;
		for(	iter = AttributeMap.begin();
			iter != AttributeMap.end();
		       	iter++) {
		    reqAttributes[iter->first] = Attributes[iter->second];
		}
		return reqAttributes;
	}
	TypedValue		GetAttribute(const string& name) const {
		map<string, int>::const_iterator iter = AttributeMap.find(name);
		if(iter == AttributeMap.end()) {
			return TypedValue();
		}
		return Attributes[iter->second];
	}
	vector<TypedValue>	GetParameters() const {
		vector<TypedValue> reqParameters;
		for(int k = 0; k < Parameters.size(); k++) {
		    reqParameters.push_back(Parameters[k]);
		}
		return reqParameters;
	}

private:

	string			Name;
	string			Type;
	string			ID;
	long			ExpansionState;
	long			DecompositionType;
	vector<string>		SubActivityIDs;
	string			ParentActivityID;
public:
	vector<TypedValue>	Attributes;
	map<string, int>	AttributeMap;
	vector<TypedValue>	Parameters;
	map<string, int>	ParameterMap;
};


class debug_this {
public:
	debug_this() : save_yes_no(yes_no) { yes_no = true; indentation += 2; }
	~debug_this() { yes_no = save_yes_no; indentation -= 2; }
	static bool yes_no;
	static int indentation;
	static void indent();
	bool save_yes_no;
	};

class debug_inhibit {
public:
	debug_inhibit() : save_yes_no(debug_this::yes_no) { debug_this::yes_no = false; }
	~debug_inhibit() { debug_this::yes_no = save_yes_no; }
	bool save_yes_no; };

class debug_indenter {
public:
	debug_indenter() : save_indentation(debug_this::indentation) { debug_this::indentation += 4; }
	~debug_indenter() { debug_this::indentation = save_indentation; }
	int save_indentation; };


#endif
