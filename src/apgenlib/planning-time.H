#ifndef _PLANNING_TIME_H_
#define _PLANNING_TIME_H_

#include <assert.h>
#include <string>

// from APGen:
#include "UTL_time_base.H"

// this code is part of the planning framework

class cppTime {
public:
	CTime_base	T;
	cppTime(const CTime_base& t) : T(t) {}
	cppTime(long int S, long int M) : T(S, M, false) {}
	cppTime(const cppTime& t) : T(t.T) {}
	cppTime(const char* s) : T(s) { assert(T.is_duration() == false); }
	cppTime() : T(0, 0, /* is_duration = */ false) {}
	~cppTime() {}

	static bool less(const cppTime& a, const cppTime& b) {
		return a.T < b.T; }
	// converts to SCET time format
	operator std::string() const;
	// converts to milliseconds from Jan 1 1970
	operator long int() const { return T.get_milliseconds() + 1000 * T.get_seconds(); }
};

class cppDuration {
public:
	CTime_base	T;
	cppDuration(const CTime_base& t) : T(t) {}
	cppDuration(const char* s) : T(s) { assert(T.is_duration() == true); }
	cppDuration(long int S, long int M) : T(S, M, true) {}
	cppDuration() : T(0, 0, /* is_duration = */ true) {}
	~cppDuration() {}
	// converts to [dddT-]hh:mm:ss.nnn format
	operator std::string() const;
	// converts to milliseconds
	operator long int() const { return T.get_milliseconds() + 1000 * T.get_seconds(); }
};


cppTime operator+(const cppTime& a, const cppDuration& b);
cppTime operator-(const cppTime& a, const cppDuration& b);
cppTime operator-(const cppTime& a, const cppTime& b);
cppDuration operator+(const cppDuration& a, const cppDuration& b);
cppDuration operator-(const cppDuration& a, const cppDuration& b);
cppDuration operator*(const cppDuration& a, double b);
cppDuration operator*(double b, const cppDuration& a);
cppDuration operator/(const cppDuration& a, double b);
double operator/(const cppDuration& a, const cppDuration& b);
bool operator<(const cppDuration& a, const cppDuration& b);
bool operator<(const cppTime& a, const cppTime& b);
bool operator==(const cppTime&a, const cppTime& b);

class cppTimeLess {
public:
	bool operator()(const cppTime& a, const cppTime& b) const {
		return a.T < b.T;  }
};

#endif /* _PLANNING_TIME_H_ */
