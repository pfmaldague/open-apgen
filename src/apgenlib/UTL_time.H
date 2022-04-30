#ifndef C_time_H
#define C_time_H

#include "UTL_time_base.H"
#include "RES_exceptions.H"

class CTime: public CTime_base {
public:
	CTime() : CTime_base() {}
	CTime(const CTime_base& t) : CTime_base(t) {}
	CTime(int a, int b, bool c) : CTime_base(a, b, c) {}
	// explicit should be here, but there is a lot to do right now 
	CTime(const Cstring& str);
	// destructor
	~CTime() {}

			//
			// Returns false if the string is no good
			//
	static bool	CTime_version_of_string_to_pseudo_milliseconds(
			const char*,
			long& pseudo_m);

			// CTime version of this virtual function
	virtual void	zone_time(tim& result, const TIMEZONE& timezone, const bool keep_ms = true) const;

			// CTime version of this virtual function
	virtual void	zone_time(tim& result) const {
				zone_time(result, TIMEZONE(TIMEZONE_UTC));
	}

	static int	check_time_string(const char* y);

	/* CTime class operators
	 * (i.e. ==, !=, >, <, >=, <=, =, +=, -=, +, -, (unary)- *=, /=, *, /) */
	bool	operator == (const CTime &t) const { return CTime_base::operator == (t); }
	bool	operator != (const CTime &t) const { return CTime_base::operator != (t); }
	bool	operator >  (const CTime &t) const { return CTime_base::operator >  (t); }
	bool	operator <  (const CTime &t) const { return CTime_base::operator <  (t); }
	bool	operator >= (const CTime &t) const { return CTime_base::operator >= (t); }
	bool	operator <= (const CTime &t ) const { return CTime_base::operator <= ( t ) ; }
	CTime	operator -  () { return CTime_base::operator -(); }
	CTime&	operator =  (const CTime& t) { CTime_base::operator = (t); return *this; }
	CTime&	operator += (const CTime& t) { CTime_base::operator += (t); return *this; }
	CTime&	operator -= (const CTime& t) { CTime_base::operator -= (t); return *this; }
	CTime&	operator *= (double m) { CTime_base::operator *= (m); return *this; }
	CTime&	operator /= (double d) { CTime_base::operator /= (d); return *this; }

	friend apgen::RETURN_STATUS	determineTimemarkLayout(
						CTime_base startTime,
						CTime_base duration,
						const TIMEZONE &timezone,
						CTime_base& majors,
						CTime_base& minors,
						CTime_base& offset);

	friend CTime_base		determineTimemarkOffset(
						CTime_base startTime,
						CTime_base majors,
						const TIMEZONE &timezone);
};

#endif
