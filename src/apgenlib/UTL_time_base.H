#ifndef UTL_time_base_H
#define UTL_time_base_H

#include <assert.h>

#include <apDEBUG.H>
#include "C_string.H"
#include "C_global.H"	// for RETURN_STATUS
#include "RES_exceptions.H"

#define MS_PER_DAY 86400000L		// milliseconds in a day
#define SECS_PER_DAY 86400L		// seconds in a day
#define SECS_PER_YEAR 31557600L		// seconds in Julian year (365.25 days)


/* Timezone enumeration names, which are actually offsets in seconds from UTC
 *   (offsets are ROUNDED to NEAREST WHOLE MINUTE by CTime_base methods,which allows
 *   multiple distinct timezone names with the same effective offset value).
 *   Positive if zone is East of Greenwich(and west of International Dateline);
 *   negative if zone is West of Greenwich(and east of International Dateline).
 * PLEASE ADD TIMEZONES IN REVERSE NUMERIC ORDER; VALUES MUST BE DISTINCT!!!
 * Also,keep in sync with switch statement in method timezone_to_abbrev_string.
 */

// enum TIMEZONE
enum ZONE {
	TIMEZONE_EPOCH=	     1,		//Epoch INSTEAD OF timezone(all Epochs are UTC)
	TIMEZONE_UTC=	     0,		//informally known as Greenwich Mean Time (GMT)
	TIMEZONE_EDT=	-14401,		//Eastern  Daylight Time
	TIMEZONE_EST=	-18000,		//Eastern  Standard Time
	TIMEZONE_CDT=	-18001,		//Central  Daylight Time
	TIMEZONE_CST=	-21600,		//Central  Standard Time
	TIMEZONE_MDT=	-21601,		//Mountain Daylight Time
	TIMEZONE_MST=	-25200,		//Mountain Standard Time
	TIMEZONE_PDT=	-25201,		//Pacific  Daylight Time
	TIMEZONE_PST=	-28800 };	//Pacific  Standard Time

// PFM 8/18/97: allow for non-Earth-based time systems
class CTime_base;

class TIMEZONE {
public:
	TIMEZONE();
	TIMEZONE(ZONE z);
	TIMEZONE(const TIMEZONE &z);
	~TIMEZONE();

	ZONE		zone;
	double		scale;
	CTime_base	*theOrigin;
	Cstring		epoch;

	TIMEZONE &	operator = (const TIMEZONE &z);
	int		operator == (const TIMEZONE &z);
};

		// convert timezone enum to timezone as abbreviation string
		//   (GLOBAL METHOD since no CTime_base needed to do this)
		// WARNING:  copy the returned string before any destructive usage
char*	timezone_to_abbrev_string(const ZONE timezone = TIMEZONE_UTC);


// structure for time conversion

struct tim {
    int tim_year;
    int tim_yday;
    int tim_hour;
    int tim_min;
    int tim_sec;
    int tim_ms;
};

//
// This is the basic CTime_base class definition.
// Change introduced April 20, 2020: The content
// of the class is just a number of milliseconds.
// If a duration, that's it. If a time, it's the
// number of milliseconds since Jan. 1st, 2020
// plus 2^62.
//
class CTime_base {
private:

			//
			// time to formatted SCET string (default is equivalent to ORIGINAL
			//  time_to_SCET_string).  Can set timezone to enumerated zone, and can
			//  set remaining 3 args FALSE to drop msec digits and round to nearest
			//  second, and/or to drop century digits, and/or to change 'T' to '/')
			//
	void		time_to_SCET_string(
				   Cstring& result,
				   const TIMEZONE &timezone,
				   const bool keep_ms = true,
				   const bool keep_century = true,
				   const bool use_tee = true) const;

			//
			// time to duration string (e.g. 345/12:22:33.333 or 345T12:22:33.333);
			//   optional arg is new for Epoch-relative needs
			//
	void		time_as_duration_string(
				Cstring& result,
				const bool keep_ms = true) const;
protected:
 	long		pseudo_millisec;

	//
	// internal methods get 0-BASED: month, day-of-month, day-of-week(0=Sun)
	//
	static void	compute_month_day (const tim* time, int& month, int& day);
	static void	compute_dayofweek (const tim* time, int& dayofweek);

	static long int	TIME_BIT;	// 2^62 milliseconds. Used to differentiate
					// between time and duration.
public:

	//
	// convenience functions
	//
	static long int	to_pseudo_milliseconds(const Cstring&);
	static long int	to_pseudo_milliseconds(const double& D);
	static long int	to_pseudo_milliseconds(long sec, long millisec, bool is_dur);

	//
	// Default is a time, not a duration
	//
	CTime_base() : pseudo_millisec(TIME_BIT) {}

	CTime_base(const CTime_base& t)
		: pseudo_millisec(t.pseudo_millisec) {}

	CTime_base(long ps)
		: pseudo_millisec(ps) {}

	CTime_base(long a, long b, bool d)
		: pseudo_millisec(to_pseudo_milliseconds(a, b, d)) {
		assert(pseudo_millisec >= 0 || pseudo_millisec & TIME_BIT);
	}

	CTime_base(const Cstring& str)
		: pseudo_millisec(to_pseudo_milliseconds(str)) {
		assert(pseudo_millisec >= 0 || pseudo_millisec & TIME_BIT);
	}

	virtual ~CTime_base() {}

	//
	// 'use with caution' because doubles are not suitable for
	// testing equality. In the 'old days', the CTime class was
	// build on top of a double. Because of the way Solaris worked
	// (at the time), comparing doubles 'did the right thing'.
	// However, early experience with Linux blew away our complacency,
	// and we had to quickly do something better... whence the current
	// class members, int seconds and int milliseconds. Note that time
	// as defined here will run out in 2038 or thereabout, but by then
	// ints will be 64 bits (128 maybe? :-)
	//
	double		convert_to_double_use_with_caution() const;

	static CTime_base convert_from_double_use_with_caution(const double& D, bool is_a_duration);

	const long int	get_seconds() const {
		if(pseudo_millisec < 0) {

			//
			// Must be a duration
			//
			assert(pseudo_millisec & TIME_BIT);
			return pseudo_millisec / 1000;
		} else if(pseudo_millisec & TIME_BIT) {
			return (pseudo_millisec - TIME_BIT) / 1000;
		}
		return pseudo_millisec / 1000;
	}

	const long int	get_milliseconds() const {
		if(pseudo_millisec < 0) {

			//
			// Must be a duration
			//
			assert(pseudo_millisec & TIME_BIT);
			return pseudo_millisec % 1000;
		} else if(pseudo_millisec & TIME_BIT) {
			return (pseudo_millisec - TIME_BIT) % 1000;
		}
		return pseudo_millisec % 1000L;
	}

	long int	to_milliseconds() const {
		if(pseudo_millisec < 0) {

			//
			// Must be a duration
			//
			assert(pseudo_millisec & TIME_BIT);
			return pseudo_millisec;
		} else if(pseudo_millisec & TIME_BIT) {
			return pseudo_millisec - TIME_BIT;
		}
		return pseudo_millisec;
	}

	long int	get_pseudo_millisec() const {
		return pseudo_millisec;
	}

	bool		is_duration() const {

		//
		// Must work for both positive and negative durations:
		//
		return pseudo_millisec < 0 || pseudo_millisec < TIME_BIT;
	}

			//
			// includes 0
			//
	bool		is_positive() const {
		return pseudo_millisec >= 0;
	}

			//
			// excludes 0
			//
	bool		is_negative() const {

		//
		// Note: If negative, the time bit should
		// always be set, because durations must
		// be less than 2^62 in absolute value
		//
		if(pseudo_millisec < 0) {
			assert(pseudo_millisec & TIME_BIT);
			return true;
		}
		return false;
	}


			//
			// test for leap year
			//
	static int	leap_year(int);

	static CTime_base& std_null_time() {
		static CTime_base T;
		return T;
	}

	static CTime_base& std_null_duration() {
		static CTime_base D(0, 0, true);
		return D;
	}

			//
			// convert CTime_base to tim structure for CTime_base to string conversions
			//  (default initializers retains gmt_time 0-argument behavior, which
			//  uses UTC and rounds to msec; 1st arg allows other enumerated time
			//  zones; 2nd arg == FALSE discards msec and rounds to nearest sec)
			//
	virtual void	zone_time(	tim&		returned_structure,
					const TIMEZONE& timezone,
					const bool keep_ms = true) const;

	virtual void	zone_time(	tim&	returned_structure) const {
		zone_time(returned_structure, TIMEZONE(TIMEZONE_UTC));
	}

			//
			// useful for debugging and reporting time - note: won't throw
			//
	Cstring		to_string() const {
		Cstring s;
		if(is_duration()) {
			time_as_duration_string(s);
		} else {
			time_to_SCET_string(s, TIMEZONE(TIMEZONE_UTC));
		}
		return s;
	}

	Cstring		to_string(	const TIMEZONE& timezone,
					const bool keep_ms = true,
					const bool keep_century = true,
					const bool use_tee = true) const {
		Cstring s;
		time_to_SCET_string(s, timezone, keep_ms, keep_century, use_tee);
		return s;
	}

	Cstring		to_string(const bool keep_ms) const {
		Cstring s;
		time_as_duration_string(s, keep_ms);
		return s;
	}

			//
			// time to DATE string (ignore time), in words (e.g. Tue, Feb 29, 2000)
			//
	void		time_to_word_date_SCET_string(Cstring& result, const TIMEZONE &timezone) const;

	void		time_to_word_date_SCET_string(Cstring& result) const {
				time_to_word_date_SCET_string(result, TIMEZONE(TIMEZONE_UTC));
	}

			//
			// time to DATE string in words, concatenated with formatted SCET date+
			//  time string and followed by the timezone abbreviation
			//
	void		time_to_complete_SCET_string(
				   Cstring& result,
				   const TIMEZONE &timezone,
				   const bool keep_ms = true,
				   const bool keep_century = true,
				   const bool use_tee = true) const;

	Cstring		time_to_complete_SCET_string() const {
		Cstring s;
		time_to_complete_SCET_string(s, TIMEZONE(TIMEZONE_UTC));
		return s;
	}

	//
	// CTime_base class operators (i.e. ==, !=, >, <, >=, <=, =, +=, -=, +, -, (unary)- *=, /=, *, /)
	//

	bool			operator ==	(const CTime_base &t) const;

	bool			operator !=	(const CTime_base &t) const;

	bool			operator >	(const CTime_base &t) const;

	bool			operator <	(const CTime_base &t) const;

	bool			operator >=	(const CTime_base &t) const;

	bool			operator <=	(const CTime_base &t) const;

	CTime_base		operator -	() {
		CTime_base T;

		//
		// Must be a duration.
		//
		assert(is_duration());
		T.pseudo_millisec = -pseudo_millisec;
		return T;
	}
				// assignment
	CTime_base&		operator =	(const CTime_base& t) {
		pseudo_millisec = t.pseudo_millisec;
		return *this;
	}
				// similar to the built-in operator
	CTime_base&		operator +=	(const CTime_base &t);

				// similar to the built-in operator
	CTime_base&		operator -=	(const CTime_base &t);

				// similar to the built-in operator
	CTime_base&		operator *=	(double m);

				// similar to the built-in operator
	CTime_base&		operator/=	(double d);

	friend CTime_base	operator  + (const CTime_base &s,	const CTime_base &t);

	friend CTime_base	operator  - (const CTime_base &s,	const CTime_base &t);

	friend CTime_base	operator  * (const CTime_base &s,	double m);

	friend CTime_base	operator  * (double s,			const CTime_base &m);

	friend double	 	operator  / (const CTime_base &s,	const CTime_base &d);

	friend CTime_base	operator  / (const CTime_base &s,	double d);

	friend CTime_base 	operator  % (const CTime_base &s,	const CTime_base &t);

        friend CTime_base	abs(const CTime_base& s);

	friend class prio_time;
};

extern void is_a_valid_time(const Cstring&, CTime_base& theResult);
extern void is_a_valid_duration(const Cstring&, CTime_base& theResult);

#endif /* UTL_time_base_H */
