% #include <unistd.h>
% #include <string.h>

enum apvaltype {AP_INT,
		AP_FLOAT,
		AP_BOOL,
		AP_TIME,
		AP_DURATION,
		AP_STRING,
		AP_INSTANCE,
		AP_ARRAY,
		AP_STRUCT,
		AP_UNDEFINED
		};

/* 
 *	- hyper is not defined on all machines - in particular, the Mac barfs on it.
 *	  Use int64_t instead.
 *
 *	- time can be encoded as a 64-bit integer number of milliseconds; only 1
 *	  integer needed.
 */

enum aptag {
	AP_NONE,
	AP_ARRAY_STYLE,
	AP_STRUCT_STYLE };

union apindex switch(aptag tag) {
	case AP_NONE:
		void;
	case AP_ARRAY_STYLE:
		int	x;
	case AP_STRUCT_STYLE:
		string	s<>;
	};

union apvalue switch(apvaltype type) {
	case AP_INT:
		int64_t	asInt;
	case AP_FLOAT:
		double	asFloat;
	case AP_BOOL:
		bool	asBool;
	case AP_TIME:
		int64_t	asTime;
	case AP_DURATION:
		int64_t asDuration;
	case AP_STRING:
		string	asString<>;
	case AP_INSTANCE:
		string	asInstance<>;
	case AP_ARRAY:
		void;
	case AP_STRUCT:
		void;
	};

/* The xdr specification of a pointer implies that the object
 * that is pointed to should be included in the data
 * that is transferred between client and server; that data is
 * always created when an rpc call is made. There is no way (in
 * xdr) to include a true pointer to an already-defined piece
 * of data.
 *
 * In that case, how do we transfer fields that are meant to be
 * true pointers, without new data associated with them? The
 * answer is that we have to use something else, like an int,
 * to identify elements of an array, and then we can use the
 * int as an index to refer to a particular array element. */
struct aparrayelement {
	apvalue		content;
	apindex		indx;
	/* unique integer associated with each array element */
	int		apid;
	/* use the apid of the parent if it exists, -1 if not: */
	int		parent;
	};

struct aparray {
	aparrayelement	apvector<>;
	};
	

program PRINTVAL {
	version PRINTVAL_V0 {
		void PRINT_NOTHING(void) = 1;
	} = 0;
	version	PRINTVAL_V1 {
		string PRINT_VALUE(apvalue) = 1;
		string PRINT_ARRAY(aparray) = 2;
	} = 1;
} = 200001;
