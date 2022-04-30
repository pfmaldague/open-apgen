#if HAVE_CONFIG_H
#include <config.h>
#endif
/*	Ancient notes:
 *
 *	Project:	SEQ_GEN
 *	Subsystem:	GLOBAL
 *	File Name:	C_Time.C
 *
 *	Description: This file defines the methods of the CTime_base class.
 *
 *	Date		Revision	Author		Reason for change
 *-----------------------------------------------------------------------
 *	2/13/91		000		W. Lombard	Original Design	
 *	2/24/95		001		R. Valdez	Update for APGEN
 *	4/11/95		002		D. Glicksberg	Update gmt_time; add
 *						      time_to_short_SCET_string
 *      4/20/95         003             D. Glicksberg   Combine above method
 *                                                      & time_to_SCET_string,
 *							adding timezone arg; rm
 *                                                      gmt_time, add zone_time
 *	6/12/95		004		D. Glicksberg	change timezone lookup
 *      6/20/95         005             D. Glicksberg   determineTimemark* here
 *      3/29/96                         D. Glicksberg   time_to_word_date_SCET_
 *                                                      string method added
 *      5/08/96                         D. Glicksberg timezone_to_abbrev_string
 *
 */


#include <assert.h>
#include "UTL_time_base.H"
#include "C_list.H"

// test for leap year (returns 0 for non-leap years, 1 for leap years)

int CTime_base::leap_year(int yr) {
	return ((yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0) ; }

// adjust two character year format to four (e.g. 92 = 1992, 30 = 2030)

long yearadj(long yr) {
	if(70L <= yr && yr < 170L)
		return (yr += 1900L);	// adjust short input
	else if(0L <= yr && yr < 70L)
		return (yr += 2000L);
	else if(yr < 1970L || yr >= 2070L)
		return 1970L;	// error nominal return
	else return yr;
}

long int CTime_base::TIME_BIT = 1L << 62;

//
// Converts from long seconds, long milliseconds, bool is_dur
// to pseudo_millisec. If this is a negative duration, both
// sec and millisec should be negative.
//
long int CTime_base::to_pseudo_milliseconds(long sec, long millisec, bool is_dur) {
	long int pseudo_milliseconds = (1000L * sec) + millisec;
	if(!is_dur) {

		//
		// The time value should be positive and less than 2^62.
		// We don't check.
		//
		pseudo_milliseconds += TIME_BIT;
	}
	return pseudo_milliseconds;
}

//
// Converts from a double D to pseudo_millisec, always a duration;
// if you have reasons to believe the answer should be a time, you
// need to flip the DUR_BIT
//
long int CTime_base::to_pseudo_milliseconds(const double& D) {
	double F, G;
	long int l;

	//
	// Returns the fractional part of D; the integral part is stored in G.
	// Both the fractional part and the integral part have the sign of D.
	//
	F = modf(D, &G);

	//
	// CHECK BOTH!!!! -0. does not qualify as negative...
	//
	if(G < 0. || F < 0.) {
		l = ((long) -G);

		//
		// both sec and millisec are <= 0
		//
		long int millisec = ((long) (0.5 - 1000 * F));
		if(millisec == 1000L) {
			l++;
			millisec = 0L;
		}
		return - millisec - 1000 * l;
	}
	l = ((long) G);

	//
	// both sec and millisec are >= 0
	//
	long int millisec = ((long) (0.5 + 1000 * F));
	if(millisec == 1000L) {
		l++;
		millisec = 0L;
	}
	return millisec + 1000 * l;
}


//
// convert a string to time class
// this function should deal properly with any "valid" input time format
//
long int CTime_base::to_pseudo_milliseconds(
					const Cstring&	S) {

	//
	// The original code (borrowed from SEQGen) used the
	// string stored in S, but that's unsafe
	//
	char*			duplicate = strdup(*S);
	char*			endp = duplicate;
	unsigned		acc;
	int			i = 0;
	bool			neg = false;
	long			days;
	unsigned		stack[6];
	CTime_base		dbl;
	CTime_base		offset;
	double			multiplier;
	bool			is_dur = false;

	long int theSeconds = 0;
	long int theMilliseconds = 0;
	if(*endp == '-') {		/* look for leading sign (1) */
		neg = true;
		endp++;
	}

	//
	// parse for time or duration format by getting all the integers we
	// can find. At most, we expect Y D H M S m, which is 6 ints. But
	// the loop below stops at the optional decimal point, so we have five
	// ints max. NOTE: strtol() ignores leading plus signs and leading spaces
	//
	do {
			acc = (unsigned) strtol(endp, &endp, 10);
			stack[i++] = acc;
		} while (i < 5 && *endp && *endp != '.' && endp++);
	// is is 5 if and only if we have a time, including year
	if(	*endp == '.'
		&& *++endp
	  ) {
		long		multiplier = 1000L;

		while(*endp && isdigit(*endp) && multiplier > 1L)  {
			multiplier /= 10;
			theMilliseconds += multiplier * (*endp - '0');
			endp++;
		}
		if(*endp && isdigit(*endp) && *endp >= '5') {
			theMilliseconds += multiplier;
		}
	}		/* mseconds */
	theSeconds +=  (i > 0) ? (long) stack[--i] : 0L;		/* seconds */
	theSeconds += ((i > 0) ? (long) stack[--i] * 60L : 0L);		/* minutes */
	theSeconds += ((i > 0) ? (long) stack[--i] * 3600L : 0L);	/* hours */
	days	    = ((i > 0) ? (long) stack[--i] : 0L);		/* days */
	if(i > 0) {
		// we only end up here if i was exactly 5 after extracting ints
		long	year_days = (yearadj(stack[--i]) - 1970L) * 365;
		long	leap_days = (yearadj(stack[i]) - 1969L) / 4;

		// adjust for no day zero (i.e. total_days - 1)
		days += (year_days + leap_days - 1L);
		is_dur = false;
	} else {
		is_dur = true;
	}
	theSeconds = theSeconds + (long) days * (long) SECS_PER_DAY;
	if(neg) {
		theMilliseconds = -theMilliseconds;
		theSeconds = -theSeconds;
	}
	free(duplicate);
	return to_pseudo_milliseconds(theSeconds, theMilliseconds, is_dur);
}

// convert a string to time class
// this function should deal properly with any "valid" input time format
CTime_base timezone_and_string_to_time(const TIMEZONE &timezone, const Cstring &T) {
	Cstring			S(T);
	char			*endp;
	unsigned		acc;
	int			i=0;
	int			neg=false;
	long			days;
	unsigned		stack[6];
	long			dbl1 = 0L, dbl2 = 0L;

	if( timezone.zone != TIMEZONE_EPOCH )
		return CTime_base( S );
	S = ( *timezone.epoch ) / S;
	endp = ( char * ) *S;
	if( *endp == '-' ) {		/* look for leading sign */
		neg = true;
		endp++; }
	/* parse for time or duration format */
	/* strtol() ignores leading plus signs and leading spaces */
	do {
			acc = (unsigned) strtol (endp,&endp,10);
			stack[i++] = acc;
		} while (i < 5 && *endp && *endp != '.' && endp++);
	if( *endp == '.' ) dbl1 = atoi( ++endp ) ;		/* mseconds */
	dbl2 += ( i > 0 ) ? ( int ) stack[--i] :  0 ;		/* seconds */
	dbl2 += ( ( i > 0 ) ? ( int ) stack[--i] * 60 : 0 );	/* minutes */
	dbl2 += ( ( i > 0 ) ? ( int ) stack[--i] * 3600 : 0 ) ;	/* hours */
	days = ( ( i > 0 ) ? stack[--i] : 0 ) ;			/* days */
	dbl2 = dbl2 + days * SECS_PER_DAY ;
	if(neg) {
		dbl1 = -dbl1;
		dbl2 = -dbl2;
	}
	return CTime_base(dbl2, dbl1, false);
}			/* return time in seconds */

// convert timezone enum to timezone abbreviation string (default TIMEZONE_UTC)
//   (GLOBAL METHOD since no CTime_base needed to do this)

char* timezone_to_abbrev_string (const ZONE timezone) {
	static char zone_str[30];

	//Keep switch statement case labels in sync with enum TIMEZONE in .H
	switch (timezone) {
	    case (TIMEZONE_EPOCH) :
		strcpy(zone_str, "") ;	//Epochs have no "zone"
		break;
	    case (TIMEZONE_UTC) :
		strcpy(zone_str, "UTC") ;
		break;
	    case (TIMEZONE_EDT) :
		strcpy(zone_str, "EDT") ;
		break;
	    case (TIMEZONE_EST) :
		strcpy(zone_str, "EST") ;
		break;
	    case (TIMEZONE_CDT) :
		strcpy(zone_str, "CDT") ;
		break;
	    case (TIMEZONE_CST) :
		strcpy(zone_str, "CST") ;
		break;
	    case (TIMEZONE_MDT) :
		strcpy(zone_str, "MDT") ;
		break;
	    case (TIMEZONE_MST) :
		strcpy(zone_str, "MST") ;
		break;
	    case (TIMEZONE_PDT) :
		strcpy(zone_str, "PDT") ;
		break;
	    case (TIMEZONE_PST) :
		strcpy(zone_str, "PST") ;
		break;
	    default:	//should NEVER happen if keep switch in sync with enum
		strcpy(zone_str, "UTC") ;
		break;
	}

	return zone_str;
}

// convert CTime_base to tim structure for CTime_base to string conversions (default
//  initializers retain original gmt_time 0-argument behavior, which
//  uses UTC and rounds to msec; 1st arg allows other enumerated time
//  zones; 2nd arg == false discards msec and rounds to nearest sec)

void CTime_base::zone_time(
		tim& tim_structure,
		const TIMEZONE& timezone,
		const bool keep_ms
		) const {

			//
			//adjust for timezone first (enumeration is valid only to nearest
			//  MINUTE, so ROUND the offset to the nearest minute before adding it)
			//
	long		offset = 60L * ( ( abs( timezone.zone ) + 30L ) / 60L );
	long		sec_round = get_seconds() + ( ( timezone.zone < 0 ) ? -offset : offset );

	int		dys = (int) ( sec_round / SECS_PER_DAY );

	sec_round -= dys * SECS_PER_DAY;
	if(keep_ms) {
		tim_structure.tim_ms = get_milliseconds(); // (int)(modf(sec_round,&sec_round)*1000);
	} else {
		tim_structure.tim_ms = 0;
	}
	long sec_day = sec_round;	//truncates any msec
	
	// in the time frame of 1970 - 2069 the leap year calculation may be
	// simplified by as shown here (works, since 2000 IS a leap year).

	int yr = dys/365;
	int lpdays = (yr+1)/4;
	int dayofyr = dys - (yr*365+lpdays);
	if (dayofyr < 0) {
		yr--;
		dayofyr += (365+leap_year(yr+1970));
	}
	tim_structure.tim_year = yr+70;		//offset from 1900 (i.e. 95 for 1995)
	tim_structure.tim_yday = dayofyr;	//0-based DOY is 1 less than customary
	tim_structure.tim_hour = sec_day / 60 / 60;
	tim_structure.tim_min = sec_day / 60 % 60;
	tim_structure.tim_sec = sec_day % 60;
}

// time to formatted SCET string (default initializers retain original 0-arg
//  behavior, e.g. 1990-345T12:22:33.333 (UTC)). Can set timezone to enumerated
//  zone, and can set remaining 3 args false to drop msec digits and round to
//  nearest second, and/or to drop century digits, and/or to change 'T' to '/')

void CTime_base::time_to_SCET_string(
			Cstring&		result,
			const TIMEZONE&		timezone_struct,
			const bool		keep_ms,
			const bool		keep_century,
			const bool		use_tee
			) const {
	assert(!is_duration());
	if(timezone_struct.zone != TIMEZONE_EPOCH) {
		char		tim_str[64];
		char		temp[30];
		struct tim	tmp;
		zone_time(tmp, timezone_struct, keep_ms);

		tim_str[0] = '\0';
		temp[0] = '\0';
		if(keep_century) {
			sprintf(temp,"%02d", (1900 + tmp.tim_year)/100);
			strcat(tim_str, temp);
		}
		sprintf(temp , "%02d-%03d",
			tmp.tim_year % 100, tmp.tim_yday + 1);
		strcat(tim_str, temp);
		sprintf(temp, (use_tee ? "T" : "/"));
		strcat(tim_str, temp);
		sprintf(temp, "%02d:%02d:%02d",
			 tmp.tim_hour, tmp.tim_min, tmp.tim_sec);
		strcat(tim_str , temp);
		if (keep_ms) {
			sprintf(temp, ".%03d", tmp.tim_ms);
			strcat(tim_str, temp);
		}
		result = tim_str;
	} else {
		int		est_length = timezone_struct.epoch.length();
		CTime_base	delta = (*this - *timezone_struct.theOrigin) / timezone_struct.scale;
		Cstring		duration_offset;

		delta.time_as_duration_string(duration_offset, keep_ms);

		int tim_str_length = 64 ;
		char* tim_str = (char*) malloc(tim_str_length);
	
		// if want to keep millisec, but happened to get an even second, and
		//   thus the ".000" was omitted, add it back!
		if(	keep_ms
		  	&& (duration_offset[duration_offset.length() - 4] != '.')) {
			duration_offset << ".000";
		}

		est_length += duration_offset.length() + 1;
		if(delta >= CTime_base(0, 0, true))
			est_length += 1;
		if(est_length > tim_str_length) {
			tim_str_length = 2 * est_length;
			tim_str = (char*) realloc(tim_str, tim_str_length);
		}
		strcpy(tim_str, *timezone_struct.epoch);
		if(delta >= CTime_base(0, 0, true))
			strcat(tim_str, "+");
		strcat(tim_str, *duration_offset);
		result = tim_str;
	}
}

void CTime_base::compute_month_day(const tim* time, int& month, int& day) {
	//modified from function month_day, K&R p. 111
	int daytab[2][12] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};
	int		i, leap, yearday;

	leap = leap_year(1900 + time->tim_year) ;
	yearday = time->tim_yday;
	for (i = 0; yearday >= daytab[leap][i]; i++)
		yearday -= daytab[leap][i];
	month = i;
	day = yearday;
}

void CTime_base::compute_dayofweek (const tim* time, int& dayofweek) {
	//Algorithm need only handle 1970 and later (earlier dates invalid,
	//  because Evaluator cannot handle them).  Basically, just need to
	//  determine day-of-week for Jan  1 (day-of-year==0), and then add
	//  day-of-year modulo 7.  Jan 1 moves 1 day (Sun then Mon then Tue)
	//  as the year increments (365 == 7*52 + 1), except that it moves 2
	//  days between the Jan 1 of a leap year and the next Jan 1.
	int years_since_1970 = time->tim_year - 70;
	int nleap_since_1970 = (years_since_1970 + 1) / 4;

	dayofweek = 4		//Jan  1, 1970 (base year) is a Thursday(==4)
		  + years_since_1970		//+1 day per year since base
		  + nleap_since_1970		//+1 extra day per leap year
		  + time->tim_yday;		//add days since Jan 1 (==0)
	dayofweek %= 7 ; }		//day-of-week cycles from Sun==0 thru Sat==6

// time to SCET DATE string (ignore time), in words (e.g. Tue, Feb 29, 2000)
void CTime_base::time_to_word_date_SCET_string(
		Cstring& result, const TIMEZONE &timezone ) const {
	char tim_str[64];

	static char month_names[][10] = { //use max. name length of 9 for each
				"January  ", "February ", "March    ",
				"April    ", "May      ", "June     ",
				"July     ", "August   ", "September",
				"October  ", "November ", "December "
				     };

	static char   day_names[][10] = {	  //use max. name length of 9 for each
				"Sunday   ", "Monday   ", "Tuesday  ", "Wednesday",
				"Thursday ", "Friday   ", "Saturday "
				   };

	char	temp[30];
	
	struct tim	tmp;
	zone_time(tmp, timezone, false);	//rounds to seconds
	int		month = 0;
	int		day = 0;
	int		dayofweek = 0;

	compute_month_day(&tmp, month, day);	//0-based month, day-of-month
	compute_dayofweek(&tmp, dayofweek);	//0=Sun -based day-of-week

	tim_str[0] = '\0';
	temp[0] = '\0';
	sprintf(temp , "%.3s, ", day_names[dayofweek]);
	strcat(tim_str, temp);
	sprintf( temp , "%.3s %2d, ",
		month_names[month], (day + 1));
	strcat(tim_str, temp);
	sprintf(temp , "%04d", (1900 + tmp.tim_year));
	strcat(tim_str, temp);
	result = tim_str;
}

// time to DATE string in words, concatenated with formatted SCET date+time
//   string and followed by the timezone abbreviation.  Only the SCET date+time
//   portion is modifiable; therefore, this method has the same args and arg
//   defaults as for time_to_SCET_string().  Can set timezone to enumerated
//   zone, and can set remaining 3 args FALSE to drop msec digits and round to
//   nearest second, and/or to drop century digits, and/or to change 'T'->'/').
//   Added 97-10-01 for Epochs:  don't strip year, and don't append timezone.
void CTime_base::time_to_complete_SCET_string(
					   Cstring& result,
					   const TIMEZONE &timezone,
					   const bool keep_ms,
					   const bool keep_century,
					   const bool use_tee) const {
	Cstring	str;

	time_to_word_date_SCET_string(str, timezone);
	char*	tim_str = (char*) malloc(1000);
	strcpy(tim_str, *str);
	if (timezone.zone != TIMEZONE_EPOCH) {
	    tim_str[strlen(tim_str) - 4] = '\0'; //strip trailing 4-digit year
	} else {
	    strcpy((tim_str + strlen(tim_str)), " "); //blank before Epoch time
	}
	Cstring temp;
	time_to_SCET_string(temp, timezone, keep_ms, keep_century, use_tee);
	strcpy((tim_str + strlen(tim_str)), *temp);
	if(timezone.zone != TIMEZONE_EPOCH) {
	    strcpy((tim_str + strlen( tim_str)), " "); //blank before zone name
	    strcpy((tim_str + strlen( tim_str)), timezone_to_abbrev_string(timezone.zone));
	}

	result = tim_str;
	free(tim_str);
}


//
// time to duration string (e.g. 345/12:22:33.333 or 345T12:22:33.333).  New
//    single argument defaults to true (original behavior); use FALSE to never
//    output millisec digits and to round to nearest second.
void CTime_base::time_as_duration_string(Cstring& result, const bool keep_ms) const {
	char		tim_str[30];
	long		sec_round = get_seconds();
	long		millisec_round = get_milliseconds();
	int		minus_sign = 0;

	assert(is_duration());
	if(	sec_round < 0
		|| ( sec_round == 0 && millisec_round < 0 ) ) {
		minus_sign = 1;
		sec_round = -sec_round;
		millisec_round = -millisec_round;
	}

	int		days = (int) (sec_round / SECS_PER_DAY);

	sec_round -= (days * SECS_PER_DAY);

	int		ms = millisec_round;

	if(!keep_ms)
		ms = 0;		//already rounded to sec;ms meaningless
	int secs = sec_round;
	int rsec = secs % 60;
	int rmin = secs / 60 % 60;
	int rhour = secs / 60 / 60 % 24;

	//Note:  ms is output only if request msec AND have non-zero ms value
	if(days && ms)
		sprintf(tim_str,"%.*s%03dT%02d:%02d:%02d.%03d",
		    minus_sign ? 1 : 0, "-", days, rhour, rmin, rsec, ms);
	else if(days)
		sprintf(tim_str, "%.*s%03dT%02d:%02d:%02d", minus_sign ? 1 : 0, "-",
		    days, rhour, rmin, rsec);
	else if(ms)
		sprintf(tim_str,"%.*s%02d:%02d:%02d.%03d" , minus_sign ? 1 : 0 , "-",
		    rhour, rmin, rsec, ms);
	else
		sprintf( tim_str , "%.*s%02d:%02d:%02d" , minus_sign ? 1 : 0 , "-" ,
		    rhour, rmin, rsec);
	result = tim_str;
}

CTime_base operator -(const CTime_base& s, const CTime_base&t) {
	CTime_base rtn(s);

	rtn -= t;
	return (rtn);
}

CTime_base operator +(const CTime_base& s, const CTime_base& t) {
	CTime_base rtn(s);

	rtn += t;
	return rtn;
}

CTime_base operator % (const CTime_base& s, const CTime_base& t) {
	long int c = t.to_milliseconds();
	if(!c) {
		Cstring err;
		err << "second argument in modulo expression is zero";
		throw(eval_error(err));
	}
	long int d = (s.to_milliseconds()) % c;
	return CTime_base(d/1000, d % 1000, true);
}

CTime_base operator * (const CTime_base& s, double m) {
	CTime_base rtn(s);

	rtn *= m;
	return rtn;
}

CTime_base operator * (double m, const CTime_base& s) {
	CTime_base rtn(s);

	rtn *= m;
	return rtn;
}

CTime_base operator / (const CTime_base& s, double d) {
	CTime_base rtn(s);

	rtn /= d;
	return rtn;
}

CTime_base
abs(const CTime_base& s) {
  // this method only makes sense for durations
  return CTime_base(abs(s.get_seconds()), abs(s.get_milliseconds()), true);
}

//
// They must both be durations - we don't check
//
double operator / (const CTime_base& A, const CTime_base& B) {
	double		D = (double) A.pseudo_millisec;
	double		G = (double) B.pseudo_millisec;

	if(G == 0.0) {
		Cstring err;
		err << "Dividing duration by zero";
		throw(eval_error(err));
	}
	return D/G;
}
//
// Converts from a double D to long seconds, milliseconds
//
void internal_conversion(const double &D, long *sec, long *millisec) {
	double F, G;

	//
	// Returns the fractional part of D; the integral part is stored in G.
	// Both the fractional part and the integral part have the sign of D.
	//
	F = modf(D, &G);

	//
	// CHECK BOTH!!!! -0. does not qualify as negative...
	//
	if(G < 0. || F < 0.) {
		*sec = (long) G;
		*millisec = -((long) (0.5 - 1000 * F));

		//
		// both sec and millisec are <= 0
		//
		if(*millisec == -1000L) {
			(*sec)--;
			*millisec = 0L;
		}
	} else {

		//
		// both sec and millisec are >= 0
		//
		*sec = (long) G;
		*millisec = (long) (0.5 + 1000 * F);
		if(*millisec == 1000L) {
			(*sec)++ ;
			*millisec = 0L;
		}
	}
}

CTime_base CTime_base::convert_from_double_use_with_caution(const double& D, bool is_dur) {
	long		a, b;

	internal_conversion(D, &a, &b);
	return CTime_base(a, b, is_dur);
}

CTime_base& CTime_base::operator /= ( double d ) {
	double		D = ( double ) get_seconds();

	D += ( ( double ) get_milliseconds() ) / 1000. ;
	D /= d;
	pseudo_millisec = to_pseudo_milliseconds(D);
	return *this;
}

CTime_base& CTime_base::operator *= (double d) {
	double		D = (double) get_seconds();

	D += ( ( double ) get_milliseconds() ) / 1000.;
	D *= d;
	pseudo_millisec = to_pseudo_milliseconds(D);
	return *this;
}

//
// Simple addition of pseudo-milliseconds is the right thing
// to do; the 2^62 bit of time values is never duplicated
// since you may not add times to times.
//
CTime_base& CTime_base::operator += (const CTime_base& d) {
	pseudo_millisec += d.pseudo_millisec;
	return *this;
}


//
// Simple subtraction of pseudo-milliseconds is the right thing
// to do; the 2^62 bit of two time values cancel out to yield
// a legal duration.
//
CTime_base& CTime_base::operator -= (const CTime_base& d ) {
    bool isDuration = is_duration() == d.is_duration();
    pseudo_millisec -= d.pseudo_millisec;
    if(!isDuration) {

	//
	// Check that we don't inadvertently sail
	// through Jan. 1, 1970
	//
	if(pseudo_millisec < TIME_BIT) {
		throw(eval_error(
			"Subtraction leads to a negative time value."));
	}
    }
    return *this ;
}

bool CTime_base::operator == (const CTime_base& t) const {
	return pseudo_millisec == t.pseudo_millisec;
}

bool CTime_base::operator != (const CTime_base& t) const {
	return pseudo_millisec != t.pseudo_millisec;
}

//
// This has become very simple with the new convention
// that a time is a duration since 1970 plus 2^62.
//
bool CTime_base::operator > (const CTime_base& t) const {
	return pseudo_millisec > t.pseudo_millisec;
}

bool CTime_base::operator < (const CTime_base& t) const {
	return pseudo_millisec < t.pseudo_millisec;
}

bool CTime_base::operator <= ( const CTime_base & t ) const {
	return pseudo_millisec <= t.pseudo_millisec;
}

bool CTime_base::operator >= ( const CTime_base & t ) const {
	return pseudo_millisec >= t.pseudo_millisec;
}

bool time_within_window( CTime_base t , CTime_base s , CTime_base e ) {
	return ( t >= s && t < e );
}

bool window_within_window( CTime_base ts , CTime_base te , CTime_base s , CTime_base e ) {
	return ( ts >= s && te <= e );
}

bool windows_overlap(CTime_base ts, CTime_base te, CTime_base s, CTime_base e) {
	return ((ts >= s && ts < e) || (te > s && te <= e) ||
		(ts < s && te > e));
}

double CTime_base::convert_to_double_use_with_caution() const {
	if(is_duration()) {
		return ((double) pseudo_millisec) / 1000.0;
	}
	return ((double)(pseudo_millisec - TIME_BIT)) / 1000.0;
}

int TIMEZONE::operator == (const TIMEZONE &z) {
	if(zone == z.zone) {
		if(zone == TIMEZONE_EPOCH) {
			if(epoch.length()) {
				if(epoch == z.epoch)
					return 1; }
			else {
				if(!z.epoch.length())
					return 1;
				return 0; } }
		else
			return 1; }
	return 0; }

TIMEZONE::TIMEZONE()
	: zone(TIMEZONE_UTC),
	scale(1.0),
	theOrigin(new CTime_base(0, 0, false)) {}

TIMEZONE::TIMEZONE(ZONE z)
	: zone(z),
	scale(1.0),
	theOrigin(new CTime_base(0, 0, false)) {}

TIMEZONE::TIMEZONE(const TIMEZONE &z)
	: zone(z.zone),
	scale(z.scale),
	epoch(z.epoch),
	theOrigin(new CTime_base(*z.theOrigin)) {}


TIMEZONE::~TIMEZONE() {
	delete theOrigin; }

TIMEZONE &TIMEZONE::operator = (const TIMEZONE &z) {
	zone = z.zone;
	epoch = z.epoch;
	*theOrigin = *z.theOrigin;
	scale = z.scale;
	return *this; }

