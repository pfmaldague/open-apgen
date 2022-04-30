#ifndef _tree_dir_h_
#define _tree_dir_h_

#define MAXDEPTH 40
#include <stdlib.h> // for NULL
#include <limits.h> // for 32- vs. 64-bit long int

class TypedValue;

namespace TreeDir {
typedef enum { LEFT_DIR = 0, RIGHT_DIR = 1, BALANCED, UNUSED } Dir;
typedef enum {
	DO_NOT_UPDATE_CONS,
	UPDATE_CONS } cons_adjust;
}

#define void_to_uint( x ) ( ( ( unsigned long ) x ) >> 2 )

#if ULONG_MAX == 4294967295UL
	// 32-bit CPU
#	define arch_is_32_bit
	typedef long long true_long;
	typedef unsigned long long true_ulong;
#else
	// 64-bit CPU
#	define arch_is_64_bit
	typedef long true_long;
	typedef unsigned long true_ulong;
#endif

// Holds values for the valueHolder class
typedef union _intOrDouble {
	true_long	I;
	double		D; } intOrDouble; 


/* We really want to hold a TypedValue but that class won't be defined
for a while. */
class valueHolder {
	bool	isDouble;
public:
	valueHolder(long i) : isDouble(false) { content.I = (true_long) i; }
	valueHolder(long long i) : isDouble(false) { content.I = (true_long) i; }
	valueHolder(double d) : isDouble(true) { content.D = d; }
	valueHolder(const valueHolder& k) : isDouble(k.isDouble) {
			if(isDouble) content.D = k.content.D;
			else content.I = k.content.I; }
	virtual ~valueHolder() {}

	virtual valueHolder*	copy() { return new valueHolder(*this); }
	virtual valueHolder&	operator +=(const valueHolder& k) {
					if(isDouble) {
						content.D += k.get_double(); }
					else {
						content.I += k.get_content(); }
					return *this; }
	virtual valueHolder&	operator =(const valueHolder &k) {
					if(isDouble) {
						content.D = k.get_double(); }
					else {
						content.I = k.get_content(); }
					return *this; }
	// for use by sophisticated derived classes
	virtual TypedValue*	get_value() { return NULL; }
	true_long		get_content() const {
					if(isDouble) {
						return (true_long) content.D; }
					return content.I; }
	double			get_double() const {
					if(isDouble) {
						return content.D; }
					return (double) content.I; }
	intOrDouble		content; };

#endif /* _tree_dir_h_ */
