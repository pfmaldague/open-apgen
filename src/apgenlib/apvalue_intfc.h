#ifndef _APVALUE_INTFC_H_
#define _APVALUE_INTFC_H_

extern "C" {
/* #include <typed_value.h> */

int make_simpleInt(apvalue* s, int64_t h);
int make_simpleFloat(apvalue* s, double d);
int make_simpleBool(apvalue* s, bool_t b);
int make_simpleTime(apvalue* s, int64_t* t);
int make_simpleDuration(apvalue* s, int64_t* D);
int make_simpleString(apvalue* s, const char* str);
int make_simpleInstance(apvalue* s, const char* inst);
int make_simpleArray(apvalue* s);
int make_simpleStruct(apvalue* s);
void copy_apvalue(apvalue* to, apvalue* from);
void mk_aparrayelement(
		aparrayelement* apval,
		apindex*	indx,
		int		id_to_use,
		int		parent,
		apvalue*	val);

long xdr_apvalue_length(apvalue* val);
long xdr_apindex_length(apindex* indx);
long xdr_aparrayelement_length(aparrayelement* el);
long xdr_aparray_length(aparray* objp);

} // extern "C"

#endif /* _APVALUE_INTFC_H_ */
