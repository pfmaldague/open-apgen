#include "apvalue.h"
#include <stdio.h>
#include <string.h>

/* Annoyingly, xdr_mem_create() (defined in /usr/include/rpc/xdr.h) wants to
 * know how long a byte stream is supplied. It does not provide any hint as to
 * what that length should be.
 *
 * The purpose of the utilities in this file is to compute, ahead of time, the
 * size that should be allocated to a byte stream so xdr_mem_create() won't run
 * out of memory.
 *
 * We mimic the structure of the code found in the rpcgen-generated file
 * apvalue_xdr.h, which handles the C structures on the client and server
 * sides (but not the stream structures on the transport side of the RPC
 * protocol.)
 * */

long xdr_apvalue_length(apvalue* val) {
	long		length = 0;

	length += sizeof(int); // apvaltype
	switch(val->type) {
		case AP_INT:
			length += sizeof(int64_t);
			break;
		case AP_FLOAT:
			length += sizeof(double);
			break;
		case AP_BOOL:
			length += sizeof(int);
			break;
		case AP_TIME:
			length += sizeof(int64_t);
			break;
		case AP_DURATION:
			length += sizeof(int64_t);
			break;
		case AP_STRING:
			{
			/* we need to compute the actual length of the string
			 * and round it off to the nearest multiple of 4 */
			char*	s = val->apvalue_u.asString;
			length += sizeof(int) * ((3 + sizeof(int) + strlen(s)) / sizeof(int));
			}
			break;
		case AP_INSTANCE:
			{
			char*	s = val->apvalue_u.asInstance;
			length += sizeof(int) * ((3 + sizeof(int) + strlen(s)) / sizeof(int));
			}
			break;
		case AP_ARRAY:
			// no storage allocated - empty array
			break;
		case AP_STRUCT:
			// no storage allocated - empty struct
			break;
		default:
			break; }
	return length; }

long xdr_apindex_length(apindex* indx) {
	long		length = 0;

	length += sizeof(int); // aptag
	switch(indx->tag) {
		case AP_NONE:
			break;
		case AP_ARRAY_STYLE:
			length += sizeof(int);
			break;
		case AP_STRUCT_STYLE:
			{
			/* we need to compute the actual length of the string
			 * and round it off to the nearest multiple of 4 */
			char*	s = indx->apindex_u.s;
			length += sizeof(int) * ((3 + sizeof(int) + strlen(s)) / sizeof(int));
			}
			break;
		default:
			break; }
	return length; }

long xdr_aparrayelement_length(aparrayelement* el) {
	long		length = 0;

	length += xdr_apvalue_length(&el->content);
	length += xdr_apindex_length(&el->indx);
	length += sizeof(int); // apid
	length += sizeof(int); // parent

	return length; }

long xdr_aparray_length(aparray* objp) {
	u_int		L = objp->apvector.apvector_len;
	aparrayelement*	A = objp->apvector.apvector_val;
	u_int		i = 0;
			// we start with the length of the array, an int:
	long		length = sizeof(u_int);

	while(i < L) {
		long l;
		length += (l = xdr_aparrayelement_length(&A[i]));
		// debug
		printf("element[i = %d] length = %ld\n", i, l);
		i++; }
	return length; }
