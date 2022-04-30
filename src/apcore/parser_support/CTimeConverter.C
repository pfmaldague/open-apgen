#if HAVE_CONFIG_H
#include <config.h>
#endif
// #define apDEBUG
#include "apDEBUG.H"

// TEST OF DASH C OPTION 100802

#include "C_global.H"
#include <ActivityInstance.H>
#include "fileReader.H"
#include "RES_exec.H"
#include "UTL_time.H"

using namespace std;


// EXTERNS:
			// in UTL_time.C:
extern long		yearadj(long yr);

int	CTime::check_time_string(const char* c) {
	static long s = 0L;
	return CTime_version_of_string_to_pseudo_milliseconds(c, s);
}

// Overrides the base class to add epoch and time system support:
//added return value for success or failure
bool CTime::CTime_version_of_string_to_pseudo_milliseconds(
		const char*	cc,
		long&		pseudo_m) {
				/* NOTE: endp is a char because of strtol;
				 * strtol does not change the contents, so
				 * the cast is OK. */
	char*			endp = (char*) cc;
	unsigned int		acc;
	int			i = 0;
	bool			neg = false;
	long			days;
	unsigned		stack[6];
	CTime_base		dbl;
	CTime_base		offset;
	double			multiplier;
	bool			non_utc = false;

	long int theSeconds = 0L;
	long int theMilliseconds = 0L;
	bool is_a_duration = false;

	if(*endp == '-') {		/* look for leading sign (1) */
		neg = true;
		endp++;
	}
	if(*endp == '\"') {
		//an epoch or time system
		Cstring		S(cc);
		Cstring		epoch_name(cc);
		ListOVal*	tv;

		non_utc = true;
		epoch_name = ("\"" / S) / "\"";
		try {
			TypedValue&	tdv = globalData::get_symbol(epoch_name);
			if(globalData::isAnEpoch(epoch_name)) {
				offset = tdv.get_time_or_duration();
				multiplier = 1.0;
			} else if(globalData::isATimeSystem(epoch_name)) {
				tv = &tdv.get_array();
				offset = tv->find("origin")->Val().get_time_or_duration();
				multiplier = tv->find("scale")->Val().get_double();
			} else {
				return false;
			} //unrecognized system name
			while(*endp && *endp++ != ':');
			if(!*endp) { // wrong format
				return false;
			}
			if(*endp == '-') {		/* look for 'leading' sign (2) */
				neg = true;
				endp++;
			}
		} catch(eval_error Err) {
			return false;
		}
	}
	/* parse for time or duration format */
	/* strtol() ignores leading plus signs and leading spaces */
	do {
		acc = (unsigned) strtol(endp, &endp, 10);
		stack[i++] = acc;
		} while (i < 5 && *endp && *endp != '.' && endp++);
	if(	*endp == '.'
		&& *++endp
	  ) {
				/* Avoid potential name conflicts */
		long		/* multiplier */ int_multipl = 1000L;

		while(*endp && isdigit(*endp) && int_multipl > 1L)  {
			int_multipl /= 10L;
			theMilliseconds += int_multipl * (*endp - '0');
			endp++;
		}
		if(*endp && isdigit(*endp) && *endp >= '5') {
			theMilliseconds += int_multipl;
		}
	}		/* mseconds */
	theSeconds +=  (i > 0) ? (long) stack[--i] : 0L;		/* seconds */
	theSeconds += ((i > 0) ? (long) stack[--i] * 60L : 0L);		/* minutes */
	theSeconds += ((i > 0) ? (long) stack[--i] * 3600L : 0L);	/* hours */
	days	    = ((i > 0) ? (long) stack[--i] : 0);		/* days */
	if(i > 0) {
		long	year_days = (yearadj(stack[--i])-1970)*365;
		long	leap_days = (yearadj(stack[i])-1969)/4;

		is_a_duration = false;
		// adjust for no day zero (i.e. total_days - 1)
		days += ( year_days + leap_days - 1 );
	} else {
		is_a_duration = true;
	}
	theSeconds = theSeconds + ( int ) days * ( int ) SECS_PER_DAY;
	if( neg ) {
		theMilliseconds = -theMilliseconds;
		theSeconds = -theSeconds;
	}
	// NOTE: apply sign to duration if epoch- or time-system-based!!
	if(non_utc) {
		CTime_base	dbl(theSeconds, theMilliseconds, false);

		is_a_duration = false;
		dbl *= multiplier;
		dbl += offset;
		theSeconds = dbl.get_seconds();
		theMilliseconds = dbl.get_milliseconds();
	}
	pseudo_m = to_pseudo_milliseconds(theSeconds, theMilliseconds, is_a_duration);
	return true;
}

	// Emulates CTime_base::zone_time() (in UTL_time.C)
void CTime::zone_time(
			tim& time_structure,
			const TIMEZONE &timezone,
			const bool	keep_ms) const {

	// We need to detect whether timezone is one of the standard time zones,
	// or whether it is an epoch- or time system-based zone.
	if(timezone.zone != TIMEZONE_EPOCH) {
		CTime_base::zone_time(time_structure, timezone, keep_ms);
	} else {
		int		est_length = timezone.epoch.length();
		CTime		delta = (*this - *timezone.theOrigin) / timezone.scale;
		int sec_day = (int) (delta.get_seconds() / SECS_PER_DAY);

		delta -= CTime(sec_day * SECS_PER_DAY, 0, true);
		if( delta < CTime(0, 0, true) ) {
			delta += CTime(SECS_PER_DAY, 0, true); }
		time_structure.tim_year = 0;
		time_structure.tim_yday = 0;
		time_structure.tim_hour = (delta.get_seconds() / 60) / 60;
		time_structure.tim_min = (delta.get_seconds() / 60) % 60;
		time_structure.tim_sec = delta.get_seconds() % 60;
		if(keep_ms) {
			time_structure.tim_ms = delta.get_milliseconds();
		} else {
			time_structure.tim_ms = 0;
		}
	}
}

CTime::CTime(const Cstring& str) {
	CTime_version_of_string_to_pseudo_milliseconds(*str, pseudo_millisec);
}
