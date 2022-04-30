#include <iostream>
#include <iomanip>
#include "IO_ApgenData.H"
#include <sys/time.h>
#include "C_list.H"
#include "UTL_time_base.H"
#include <math.h>
#include <time.h>

using namespace std;

// #define min(a,b)            ((a) < (b) ? (a):(b) )


//not sure why I can't find this function in stdlib.h -- (grrr)
namespace poor_policy {
long long int
llabs(long long int val) {
	if(val < 0LL)
		return val * -1LL;

	return val; }

} // namespace poor_policy

void Assert(bool B) {
	int i;
	if(!B) {
		// debug
		// int Zer = 0;
		// i = 41 / Zer;
		cerr << "APGEN internal error: "
			"incorrect SASF conversion in IO_ApgenData.C\n"; } }

//why isn't this defined for me?

/*this forces a double into a long long (which truncates) and then puts it back*/
double
roundPlaces(double number, int numPlaces) {
	//lets put our number in a range
	int places = std::min(numPlaces, 16);

	places = std::max(numPlaces, 1);

	bool negative = (number < 0.0);

	number = fabs(number);

	int exponent = (int) log10(number);
	long long int integralNum = (long long int) floor(number * pow(10.0, places - 1) + 0.5);
	double roundedValue = integralNum / pow(10.0, places - 1);
	roundedValue *= pow(10.0, exponent);

	if(negative)
		roundedValue *= -1.0;

	return roundedValue; }

IO_ActivityInstance::IO_ActivityInstance()
	// : ExpansionState(APGENExpansionActive),
	  // IsRequest(0),
	  // IsChameleon(0),
	  // DecompositionType(APGENDecompNoDetail),
	  // Attributes(APGENArrayIndexString),
	  // Parameters(APGENArrayIndexString)
{ }

void IO_ActivityInstance::AddChildID(const string& childID) {
	SubActivityIDs.push_back(childID);
}

void IO_ActivityInstance::SetParentID(const string &parentActivityID) {
	ParentActivityID = parentActivityID;
}

unsigned long IO_ActivityInstance::GetNumSubActivities() const {
	return SubActivityIDs.size();
}

const string IO_ActivityInstance::GetSubActivityID(unsigned long subActivityIndex) const {
	Assert(subActivityIndex < SubActivityIDs.size());
	
	return SubActivityIDs[subActivityIndex];
}

long IO_ActivityInstance::GetIsRequest() const {
	TypedValue atts = GetAttribute("sasf");
	if(atts.is_array()) {
		ArrayElement* ae = atts.get_array().find("type");
		if(ae) {
			if(ae->Val().is_string() && ae->Val().get_string() == "request") {
				return 1;
			}
		}
	}
	return 0;
}

IO_TypedValue& IO_TypedValue::operator=(const IO_TypedValue& rhs) {
	if(&rhs == this)
	  return *this; 
	Name = rhs.Name;

	ValueString = rhs.ValueString;

	return *this;
}

TypedValue IO_TypedValueArray::GetElement(unsigned long index) const {
	Assert(index < Value.get_array().get_length());

	ArrayElement* ae = Value.get_array()[index];
	if(ae) {
		return ae->Val();
	}
	return TypedValue();
}

TypedValue IO_TypedValueArray::GetElement(const string& name) const {
	ArrayElement* ae = Value.get_array()[name];
	if(ae) {
		return ae->Val();
	}
	return TypedValue();
}

void IO_TypedValueArray::UnInit() {
	Value.undefine();
}

IO_TypedValueArray::~IO_TypedValueArray() {
	UnInit();
}

IO_Legend::IO_Legend()
	: Name(),
	  Height(0) {
}

void IO_Legend::SetName(const string &name) {
	Name = name; }

void IO_Legend::SetHeight(unsigned long height) {
	Height = height; }

void IO_Legend::GetName(string* name) const {
	*name = Name; }

void IO_Legend::GetHeight(unsigned long *height) const {
	*height = Height; }

//epochs
IO_Epoch::IO_Epoch(const string& name, const CTime_base& origin)
	: Origin(origin),
	  Name(name)
{
}

void
IO_Epoch::SetOrigin(const CTime_base& origin)
{
	Origin = origin;
}

void
IO_Epoch::SetName(const string& name)
{
	Name = name;
}

CTime_base
IO_Epoch::GetEvaluatedTime(bool beforeEpoch, const CTime_base& relativeTime) const
{
	if(beforeEpoch)
	  return Origin - relativeTime;
	else
	  return Origin + relativeTime;
}

IO_TimeEpochRelative*
IO_Epoch::GetRelativeTime(const IO_Time* time) const
{
	CTime_base exactTime = time->GetEvaluatedTime();

	if(exactTime < Origin)
	  return new IO_TimeEpochRelative(*this, 1, Origin - exactTime);
	else
	  return new IO_TimeEpochRelative(*this, 0, exactTime - Origin);
}

bool
IO_Duration::operator<(const IO_Duration& otherDuration) const
{
	return Value < otherDuration.Value;
}

bool
IO_Duration::operator>(const IO_Duration& otherDuration) const
{
	return otherDuration.Value < Value;
}

void
IO_Duration::operator+=(const IO_Duration& otherDuration)
{
	Value += otherDuration.Value;
}

IO_Duration IO_Time::GetSubtract(const IO_Time& otherTime) const {
	IO_Duration dur(GetEvaluatedTime() - otherTime.GetEvaluatedTime());
	return dur;
}

void
IO_Duration::operator-=(const IO_Duration& otherDuration)
{
	Value -= otherDuration.Value;
}

IO_Duration&
IO_Duration::operator=(const IO_Duration& rhs)
{
	if(rhs == *this)
	  return *this;

	IO_TypedValue::operator=(rhs);

	Value = rhs.Value;
	return *this;
}



std::ostream& 
operator<<(std::ostream& s, const IO_Duration& dur) {
	s << *dur.Value.to_string();
	return s;
}

bool
IO_Time::operator<(const IO_Time& otherTime) const
{
	CTime_base thisBase = GetEvaluatedTime();
	CTime_base otherBase = otherTime.GetEvaluatedTime();

	return (thisBase < otherBase);
}

bool
IO_Time::operator==(const IO_Time& otherTime) const
{
	CTime_base thisBase = GetEvaluatedTime();
	CTime_base otherBase = otherTime.GetEvaluatedTime();

	return (thisBase == otherBase);
}

IO_Time&
IO_Time::operator=(const IO_Time& rhs)
{
	if(this == &rhs)
	  return *this;

	//call base class' assign
	IO_TypedValue::operator=(rhs);
	return *this;
}

IO_TimeEpochRelative::IO_TimeEpochRelative(
		const IO_Epoch& epoch,
		bool beforeEpoch,
		const CTime_base& relativeTime)
	: IO_Time(),
	  Epoch(epoch),
	  BeforeEpoch(beforeEpoch),
	  RelativeTime(relativeTime)
{
}

IO_TimeEpochRelative&
IO_TimeEpochRelative::operator=(const IO_TimeEpochRelative& rhs)
{
	if(this == &rhs)
	  return *this;

	IO_Time::operator=(rhs);
	
	Epoch = rhs.Epoch;
	BeforeEpoch = rhs.BeforeEpoch;
	RelativeTime = rhs.RelativeTime;

	return *this;
}

IO_TimeEpochRelative::IO_TimeEpochRelative(const IO_TimeEpochRelative& rhs)
	: IO_Time(rhs),
	  Epoch(rhs.Epoch),
	  BeforeEpoch(rhs.BeforeEpoch),
	  RelativeTime(rhs.RelativeTime)
{
}

CTime_base
IO_TimeEpochRelative::GetEvaluatedTime() const
{
	return Epoch.GetEvaluatedTime(BeforeEpoch, RelativeTime);
}

string
IO_TimeEpochRelative::GetAPFTimeString(bool includeMS) const
{
	//do apf time string stuff here
	string apfString = Epoch.GetName();
	
	if(BeforeEpoch)
	  apfString += " - ";
	else
	  apfString += " + ";

	apfString += *RelativeTime.to_string();

	return apfString;
}

bool
IO_TimeEpochRelative::RelativeCompatible(const IO_TimeEpochRelative* otherTime) const
{
	return (otherTime->Epoch.GetName() == Epoch.GetName());
}

IO_TimeSystemRelative::IO_TimeSystemRelative(
		const IO_TimeSystem& system,
		bool beforeSystem,
		const CTime_base& relativeTime)
	: IO_Time(),
	  TimeSystem(system),
	  BeforeSystem(beforeSystem),
	  RelativeTime(relativeTime)
{
}

IO_TimeSystemRelative&
IO_TimeSystemRelative::operator=(const IO_TimeSystemRelative& rhs)
{
	if(this == &rhs)
	  return *this;

	//call base class assign
	IO_Time::operator=(rhs);
	
	TimeSystem = rhs.TimeSystem;
	BeforeSystem = rhs.BeforeSystem;
	RelativeTime = rhs.RelativeTime;

	return *this;
}


IO_TimeSystemRelative::IO_TimeSystemRelative(const IO_TimeSystemRelative& rhs)
	: IO_Time(rhs),
	  TimeSystem(rhs.TimeSystem),
	  BeforeSystem(rhs.BeforeSystem),
	  RelativeTime(rhs.RelativeTime)
{
}

CTime_base
IO_TimeSystemRelative::GetEvaluatedTime() const
{
	return TimeSystem.GetEvaluatedTime(BeforeSystem, RelativeTime);
}

string
IO_TimeSystemRelative::GetAPFTimeString(bool includeMS) const
{
	string apfString;

	if(BeforeSystem)
	  apfString += "-";

	apfString += "\"";
	apfString += TimeSystem.GetName();
	apfString += "\":";

	apfString += *RelativeTime.to_string();

	return apfString;
}

bool
IO_TimeSystemRelative::RelativeCompatible(const IO_TimeSystemRelative* otherTime) const
{
	return (TimeSystem.GetName() == otherTime->TimeSystem.GetName());
}

IO_TimeExact::IO_TimeExact(const CTime_base& time)
	: IO_Time(),
	  ExactTime(time)
{
}

IO_TimeExact::IO_TimeExact(const IO_TimeExact& rhs)
	: IO_Time(rhs),
	  ExactTime(rhs.ExactTime)
{
}

CTime_base
IO_TimeExact::GetEvaluatedTime() const
{
	return ExactTime;
}

string
IO_TimeExact::GetAPFTimeString(bool includeMS) const
{
	return *ExactTime.to_string();
}

IO_TimeSystem::IO_TimeSystem()
	: Scale(1.0)
{
}

void
IO_TimeSystem::SetName(const string& name)
{
	Name = name;
}

void
IO_TimeSystem::SetOrigin(const CTime_base& origin)
{
	Origin = origin;
}

void
IO_TimeSystem::SetScale(double scale)
{
	Scale = scale;
}

const string
IO_TimeSystem::GetName() const
{
	return Name;
}

const CTime_base
IO_TimeSystem::GetOrigin() const
{
	return Origin;
}

const double
IO_TimeSystem::GetScale() const
{
	return Scale;
}

IO_TimeSystemRelative*
IO_TimeSystem::GetRelativeTime(const IO_Time* time) const
{
	CTime_base exactTime = time->GetEvaluatedTime();

	if(exactTime < Origin)
	  return new IO_TimeSystemRelative(*this, 1, (Origin - exactTime) / Scale);
	else
	  return new IO_TimeSystemRelative(*this, 0, (exactTime - Origin) / Scale);
}

CTime_base
IO_TimeSystem::GetEvaluatedTime(bool before, const CTime_base& relativeTime) const
{
	 if(before)
	  return CTime_base(Origin - (relativeTime * Scale));
	else
	  return CTime_base(Origin - (relativeTime * Scale));
}

IO_WindowSize::IO_WindowSize()
	: HeightPixels(0),
	  WidthPixels(0) {}

void IO_WindowSize::Set(long heightPixels, long widthPixels) {
	HeightPixels = heightPixels;
	WidthPixels = widthPixels; }

void IO_WindowSize::Get(long *heightPixels, long *widthPixels) const {
	*heightPixels = HeightPixels;
	*widthPixels = WidthPixels; }
