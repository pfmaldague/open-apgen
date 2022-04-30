#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <ctype.h>
#include <mutex>
#include <map>
#include <vector>

#include <templates.H>
#include <backpointer.H>
#include <alphatime.H>
#include <array_key.H>

#include <RES_exceptions.H>
#include <treeDir.H>

char ptr2global_string[] = "ptr2global";
char BPptr2_string[] = "BPptr2";

using namespace std;

int	theDebugIndentation = 0;
int	enablePrint = 1;

bool	Cstring::make_all_strings_permanent = false;

const Cstring*	null_string() {
	static Cstring* S = new Cstring();
	return S;
}

const Cstring&	Cstring::null() {
	static const Cstring& St = *null_string();
	return St;
}

struct char_star_less {
	bool operator()(const char* const & a, const char* const & b) const {
		int k = strcmp(a, b);
		return k < 0;
	}
};

map<const char*, int, char_star_less>&	cstring_map_M() {
	static map<const char*, int, char_star_less>	M;
	return M;
}

vector<const char*>&			cstring_vec_V() {
	static vector<const char*>	V;
	return V;
}

const char* Cstring::create(srep& P, const char* s) {

	if(!s) {
		P.perm = true;
		P.s = "";
		return P.s;
	}
	if(make_all_strings_permanent) {
		map<const char*, int, char_star_less>::iterator	iter = cstring_map_M().find(s);
		const char*					t;

		P.perm = true;
		if(iter == cstring_map_M().end()) {

			//
			// Would be a memory leak, except we never forget it!
			//
			t = strdup(s);
			cstring_map_M()[t] = cstring_vec_V().size();
			cstring_vec_V().push_back(t);
			P.s = t;
		} else {
			t = cstring_vec_V()[iter->second];
			P.s = t;
		}
	} else {
		P.perm = false;
		P.s = strdup(s);
	}
	return P.s;
}

void Cstring::delete_permanent_strings() {
	for(int i = 0; i < cstring_vec_V().size(); i++) {
		free((char*)cstring_vec_V()[i]);
	}
}

const char* Cstring::create(srep& P, const srep& Q) {
	if(Q.perm) {
		P.perm = true;
		P.s = Q.s;
	} else if(make_all_strings_permanent) {
		map<const char*, int, char_star_less>::iterator	iter = cstring_map_M().find(Q.s);
		const char*					t;

		P.perm = true;
		if(iter == cstring_map_M().end()) {

			//
			// Would be a memory leak, except
			// we never forget the pointer!
			//
			t = strdup(Q.s);
			cstring_map_M()[t] = cstring_vec_V().size();
			cstring_vec_V().push_back(t);
			P.s = t;
		} else {
			t = cstring_vec_V()[iter->second];
			P.s = t;
		}
	} else {
		P.perm = false;
		P.s = strdup(Q.s);
	}
	return P.s;
}

Cstring::Cstring() {
	p.s = "";
}

Cstring::Cstring(const Cstring& x) {
	create(p, x.p);
}

// Cstring::Cstring(srep* S) {
// 	S->n++;
// 	p = S;
// }

Cstring::Cstring(const char* s) {
	create(p, s);
}

Cstring::Cstring(const string& str){
	//convert string to const char* then do normal conversion
	const char* s = str.c_str();
	create(p, s);
}


//97-12-04 DSG -- added non-const signature with identical code to above
Cstring::Cstring(char *s) {
	create(p, s);
}

Cstring::Cstring(const int& i) {
	char* Str;
	if (i > 0) {
		Str = (char*) malloc((long)log10((double)i) + 2);
	} else if (i < 0) {
		Str = (char*) malloc((long)log10((double)abs(i)) + 3);
	} else {
		Str = (char*) malloc(2);
	}
	sprintf(Str,"%d",i);
	create(p, Str);
	free(Str);
}

Cstring::Cstring(const long& i) {
	char* Str;
	if(i > 0L) {
		Str = (char*) malloc((long)log10((double)i) + 2);
	} else if(i < 0L) {
		Str = (char*) malloc((long)log10((double)-i) + 3);
	} else {
		Str = (char*) malloc(2);
	}
	sprintf(Str, "%ld", i);
	create(p, Str);
	free(Str);
}

Cstring::Cstring(const long long& i) {
	char* Str;
	if(i > 0LL) {
		Str = (char*) malloc((int)log10((double)i) + 2);
	} else if(i < 0LL) {
		Str = (char*) malloc((int)log10((double)-i) + 3);
	} else {
		Str = (char*) malloc(2);
	}
	sprintf(Str, "%lld", i);
	create(p, Str);
	free(Str);
}

Cstring::Cstring(const unsigned long& i) {
	char* Str;
	if(i > 0L) {
		Str = (char*) malloc((int)log10((double)i) + 2);
	} else {
		Str = (char*) malloc(2);
	}
	sprintf(Str, "%lu", i);
	create(p, Str);
	free(Str);
}

Cstring::Cstring(const unsigned long long& i) {
	char* Str;
	if(i > 0) {
		Str = (char*) malloc((int)log10((double)i) + 2);
	} else {
		Str = (char*) malloc(2);
	}
	sprintf(Str, "%llu", i);
	create(p, Str);
	free(Str);
}


Cstring::Cstring(const double d, const int pr) {
	int	prec;
	int	fractionLength = 0;
	int	integralLength = 0;
	int	theNumberOfUninterruptedNines = 0;
	char*	t;
	char*	there_is_a_decimal_point = NULL;
	char*	theLastCharBeforeTheNines = NULL;

	//
	// DOUBLE_DIG is (approximately) significant digits in a double
	//
	int	max_size = DOUBLE_DIG + 7;
	char*	Str = (char*) malloc(max_size);
	if( pr >= DOUBLE_DIG ) {
		prec = DOUBLE_DIG - 1;
	} else if( pr <= 0 ) {
		prec = 1;
	} else {
		prec = pr;
	}

	//
	// this format yields desired precision, but may leave out the actual
	// decimal point, for both floating and scientific formats:
	//
	int	actual_size = snprintf(Str, max_size, "%.*g", prec, d);
	if(actual_size >= max_size) {
		max_size = actual_size + 1;

		//
		// !!! bug discovered 8/2/2019...
		//
		// delete Str;
		free(Str);
		Str = (char*) malloc(max_size);
		actual_size = snprintf(Str, max_size, "%.*g", prec, d);
		assert(actual_size < max_size);
	}

	//
	// THIS PART ADDED TO AVOID THE "3 -> 2.99999999999999999999999999999997" syndrome:
	//
	t = Str;
	while(*t) {
		if(*t == '.') {

			//
			// t points to the decimal point
			//
			integralLength = t - Str;
			fractionLength = strlen(++t);
			if(fractionLength > 1) {
				int		k = fractionLength - 2;

				while(k >= 0 && t[k] == '9') {
					k--;
				}
				theLastCharBeforeTheNines = t + k;

				//
				// k could be -1, in which case *theLastChar... is '.'
				//
				theNumberOfUninterruptedNines = fractionLength - 2 - k;
			}
			break;
		}
		t++;
	}

	//
	// It's hard to guess what the right precision is. Should check who uses this.
	// if( theNumberOfUninterruptedNines > 3 )
	//
	if(theNumberOfUninterruptedNines > 6) {
		if(*theLastCharBeforeTheNines != '.') {
			// There is a digit other than 9
			(*theLastCharBeforeTheNines++)++;
			*theLastCharBeforeTheNines = '\0';
		} else if( theLastCharBeforeTheNines > Str ) {
			double	F, G;

			F = modf(d, &G);
			if(G < 0. || F < 0.) {
				// account for the minus sign
				integralLength--;
				G -= 1.0;
			} else {
				G += 1.0;
			}

			// fix PFM April 18 2016
			// sprintf(Str, "%.0g", G);
			actual_size = snprintf(Str, max_size, "%.*g", integralLength, G);
			if(actual_size >= max_size) {
				max_size = actual_size + 1;

				//
				// !!! bug discovered 8/2/2019...
				//
				// delete Str;
				free(Str);

				Str = (char*) malloc(max_size);
				actual_size = snprintf(Str, max_size, "%.*g", integralLength, G);
				assert(actual_size < max_size);
			}

			// OLD:
			//
			// theLastCharBeforeTheNines--;
			// WON'T WORK IF = 9!!!
			// (*theLastCharBeforeTheNines++)++;
			// *theLastCharBeforeTheNines = '\0';
		}
	}
	// END OF SYNDROME FIX

	// fix double->int problem
	bool leave_it_alone = false;
	t = Str;
	// skip over leading minus sign
	if(*t == '-') t++;
	while( *t ) {
		if((!isdigit(*t)) && *t != '.') {
			leave_it_alone = true;
			break;
		}
		t++;
	}
	if( !leave_it_alone ) {
		t = Str;
		while(*t) {
			if(*t == '.')
				break;
			t++;
		}
		if(!*t) {
			// no decimal point
			int l = strlen(Str);
			if(l + 2 < max_size + 1) {
				strcat(Str, ".0");
			} else {
				char*	cpy = strdup(Str);

				//
				// !!! bug discovered 8/2/2019...
				//
				// delete Str;
				free(Str);
				Str = (char*) malloc(l + 3);
				strcpy(Str, cpy);
				strcat(Str, ".0");
				free(cpy);
			}
		}
	}
	create(p, Str);
	free(Str);
}

/*This constructor makes an 8-wide, zero-padded hex string with no "0x" or "0X".*/
Cstring::Cstring(const void* pv) {
	char str[15];

	//
	// was:
	// 	sprintf(p->Str, "%08x", pv);
	// is:
	//
	sprintf(str, "%p", pv);

	//
	// format is now consistent with that of aoString::operator <<
	//
	create(p, str);
}

Cstring::Cstring(const char c) {
	char str[2];
	str[0] = c;
	str[1] = '\0';
	create(p, str);
}


Cstring::~Cstring() {
	if (!p.perm) {
		free((char*)p.s);
	}
}


Cstring& Cstring::operator = (const Cstring& x) {
	if (!p.perm) {
		free((char*)p.s);
	}
	create(p, x.p);
	return *this;
}

Cstring& Cstring::operator = (const char* s) {
	if (!p.perm) {
		free((char*)p.s);
	}
	create(p, s);
	return *this;
}

char Cstring::operator [] (long i) const {
	//
	// Returns 0 if i is out of range or if the Cstring is undefined.
	//

	// (PFM)
	// the check on strlen is nice but abominably inefficient; drop it:
	// if (!is_defined() || i < 0 || strlen(p->Str) < i)  return 0;
	//
	if(i < 0L) {
		return '\0';
	} else {
		return p.s[i];
	}
}


char* Cstring::operator () () const {
	return (char*) p.s;
}

long Cstring::OccurrencesOf(const char *C) const {
	const char*	c = p.s;
	long		count = 0L;
	long		l = strlen(C);

	if(!l) return 0L;
	while(1) {
		if(!*c) return count;
		if(!strncmp(c, C, l)) {
			count++;
			c += l;
		} else {
			c++;
		}
	}
	return count;
}

bool operator & (const Cstring& str, const char* c) {
	char	*t = ( char * ) *str , * cend = (char *) *str + str.length() ;
	long	i, k = strlen(c);

	if(str.length() < k) return false;
	if(!k) return true;
	for(; (t + k) <= cend ; t++) {
		for(i = 0; i < k; i++) {
			if(t[i] != c[i]) break;
		}
		if(i == k) return true;
	}
	return false;
}

Cstring operator / (const Cstring &str, const char *c) {
	char *S = (char *) *str, *cend = (char *) S + str.length();
	char *t = S;
	long i, k = strlen(c);


	if(str.length() < k) return Cstring();
	if(!k) return str;
	for( ; (t + k) <= cend ; t++ ) {
		for(i = 0L; i < k; i++) {
			if(t[i] != c[i]) break;
		}
		if(i == k) { // have a match
			char	*temp = (char *) malloc(t - S + 1);
			Cstring	ret;

			strncpy(temp, S, t - *str);
			temp[t - S] = '\0';
			ret = temp;
			free(temp);
			return ret;
		}
	}
	return Cstring();
}

Cstring operator / (const char * c, const Cstring &str) {
	char	*S = (char *) *str, *cend = S + str.length();
	char	*t = S;
	int	i, k = strlen(c);

	if(str.length() < k) return Cstring();
	if(!k) return str;
	for( ; (t + k) <= cend ; t++) {
		for(i = 0; i < k; i++) {
			if(t[i] != c[i]) break;
		}
		if(i == k) {
			int	length_of_returned_string = str.length() - (t + i - S) + 1;
			char	*temp = (char *) malloc(length_of_returned_string + 1);
			Cstring	ret;

			strncpy(temp, t + i, length_of_returned_string);
			temp[ length_of_returned_string ] = '\0';
			ret = temp;
			free(temp);
			return ret;
		}
	}
	return Cstring();
}

Cstring operator - (const Cstring &str, const char *c) {
	const char	*t = *str + str.length();
	int		i, k  = strlen(c), flag = 1;
	char		*ret;
	Cstring	 	returned_string;
 
	for(--t; t >= *str; t--) {
		flag = 0;
		for(i = 0; i < k; i++) {
			if(c[i] == *t) {
				flag = 1;
				break;
			}
		}
		if( ! flag ) break ;
	}
	i = ++t - *str ;
	ret = ( char * ) malloc( i + 1 ) ;
	strncpy( ret , *str , i ) ;
	ret[i] = '\0' ;
	returned_string = ret ;
	free( ret ) ;
	return returned_string ;
}

Cstring operator - ( const char *c , const Cstring &str ) {
	const char	*t = *str ;
	int		i , k  = strlen(c) , l = str.length() , flag = 1 ;
 
	for( ; t - *str < l ; t++ ) {
		// !!
		if( t - *str == l ) break ;
		flag = 0 ;
		for( i = 0 ; i < k ; i++ ) {
			if( c[i] == *t ) flag = 1 ;
		}
		if( ! flag ) break ;
	}
	return Cstring(t);
}

void Cstring::undefine() {
	if(!p.perm) {
		free((char*)p.s);
	}
	p.perm = true;
	p.s = "";
}


/*This function concatenates x onto this (see operator+()).*/
Cstring& Cstring::operator << (const Cstring& x) {

	if(!x.is_defined()) {
		return *this;
	}

	//
	// create new concatenated string
	//
	char* Str = (char*) malloc(length() + x.length() + 1);

	strcpy(Str, p.s);
	strcat(Str, x.p.s);

	//
	// if old string obsolete
	//
	if (!p.perm) {
		free((char*)p.s);
	}

	//
	// attach to new string
	//
	create(p, Str);
	free(Str);
	return *this;
}

/*This function concatenates x and y.*/
Cstring operator + ( const Cstring& x , const Cstring& y ) {
	Cstring result = x;

	result << y;
	return result;
}

static const char	*XMLquote = "&quot;";
static const char	*XMLamper = "&amp;";
static const char	*XMLgreat = "&gt;";
static const char	*XMLsmall = "&lt;";
static int		XMLquoteL = strlen("&quot;");
static int		XMLamperL = strlen("&amp;");
static int		XMLgreatL = strlen("&gt;");
static int		XMLsmallL = strlen("&lt;");

char *fix_for_XML(const Cstring &S) {
	static char	*buf = NULL;
	static int	l = 0;

	static mutex		mtx2;

	lock_guard<mutex> guard(mtx2);

	int		quotes = 0, ampers = 0, greate = 0, smalle = 0;
	int		theLength = S.length();
	const char	*d = *S;
	char		*c;

	if(!theLength) return NULL;

	quotes = S.OccurrencesOf("\"");
	ampers = S.OccurrencesOf("&");
	greate = S.OccurrencesOf(">");
	smalle = S.OccurrencesOf("<");
	theLength += quotes * XMLquoteL;
	theLength += ampers * XMLamperL;
	theLength += greate * XMLgreatL;
	theLength += smalle * XMLsmallL;
	if(l < (theLength + 1)) {
		if(buf) {
			buf = (char *) realloc(buf, theLength * 2 + 20);
		} else {
			buf = (char *) malloc(theLength * 2 + 20);
		}
	}
	c = buf;
	while(*d) {
		if(*d == '\"') {
			strcpy(c, XMLquote);
			c += XMLquoteL;
		} else if(*d == '&') {
			strcpy(c, XMLamper);
			c += XMLamperL;
		} else if(*d == '>') {
			strcpy(c, XMLgreat);
			c += XMLgreatL;
		} else if(*d == '<') {
			strcpy(c, XMLsmall);
			c += XMLsmallL;
		} else {
			*c++ = *d;
		}
		d++;
	}
	*c = '\0';
	return buf;
}

ostream&
operator << (
		ostream& s,
		const Cstring& x) {
	return s << x.p.s;
}

ostream&
operator << (
		ostream& s,
		const alpha_string& x) {
	return s << x.str;
}

ostream&
operator << (
		ostream& s,
		const array_string& x) {
	return s << x.str;
}

ostream&
operator << (
		ostream& s,
		const ualpha_string& x) {
	return s << x.str;
}

ostream&
operator << (
		ostream& s,
		const ualpha_int& x) {
	return s << x.get_key();
}

const Cstring&
ualpha_int::get_key() const {
	static Cstring t;

	static mutex mtx1;

	lock_guard<mutex> guard(mtx1);

	Cstring s(Num);
	t = s;
	return t;
}

void Cstring::to_lower() {
	char* src = (char*) p.s;
	char* Str = (char*) malloc(strlen(src) + 1);
	char* dest = Str;

	while (*src) {
		*dest++ = tolower(*src++);
	}
	*dest = '\0';
	if(!p.perm) {
		free((char*)p.s);
	}
	create(p, Str);
	free(Str);
}


void Cstring::to_upper() {
	char* src = (char*) p.s;
	char* Str = (char*) malloc(strlen(src) + 1);
	char* dest = Str;

	while (*src) {
		*dest++ = toupper(*src++);
	}
	*dest = '\0';
	if(!p.perm) {
		free((char*)p.s);
	}
	create(p, Str);
	free(Str);
}

// return the substring starting at 'strt' for length 'len'
//	'strt' = 0 for first character
Cstring Cstring::substr(long strt, long len) {
	char*		dest;
	const char*	src;
	long		i, L;
	Cstring		result;

	if (p.s != NULL && strt >= 0L && strt < length() && len > 0L) {
		src = p.s + strt;
		if (len > ( L = (int)strlen(src) ) ) {
			len = L;
		}
		dest = (char*) malloc(len + 1);
		for (i=0L; i < len; i++) {
			dest[i] = *src++;
		}
		dest[i] = '\0';
		result = dest;
		free(dest);
	}
	return result;
}

eval_error::eval_error(
		const Cstring& theError)
	: msg(theError) {}

eval_error::eval_error(
		const eval_error &ee)
	: msg(ee.msg)
		{}

namespace listEnum {
// utility
const char* spellEvent(listEvent e) {
	switch(e) {
		case BEFORE_NODE_INSERTION:
			return "BEFORE NODE INSERTION";
		case AFTER_NODE_INSERTION:
			return "AFTER NODE INSERTION";
		case BEFORE_NODE_REMOVAL:
			return "BEFORE NODE REMOVAL";
		case AFTER_NODE_REMOVAL:
			return "AFTER NODE REMOVAL";
	 	default: ; }
	return "UNKNOWN LIST EVENT"; }
} // namespace listEnum

Cstring nonewlines(const Cstring& s) {
	Cstring t;
	if(!s.is_defined()) {
		return t;
	}
	const char*	c = s.p.s;
	char*		u = (char*) malloc(s.length() + 1);
	int		State = 0;
	int		i = 0;

	while(*c) {
		if(State == 0) {
			if(*c == ' ' || *c == '\t' || *c == '\n') {
				State = 1;
			} else {
				u[i++] = *c;
			}
		} else if(State == 1) {
			if(*c != ' ' && *c != '\t' && *c != '\n') {
				State = 0;
				u[i++] = ' ';
				u[i++] = *c;
			}
		}
		c++;
	}
	u[i] = '\0';
	t = u;
	free(u);
	return t;
}
