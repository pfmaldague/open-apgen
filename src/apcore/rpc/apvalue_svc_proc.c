#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <string.h>

#include <apvalue.h>
#include <stdio.h>
#include <stdlib.h>

/*	- implemented in TypedValueServer.C
 *	- returns a pointer to a TypedValue object */
extern void* apvalue_to_TypedValue(apvalue*);
extern void* aparray_to_TypedValue(aparray*);
extern char* TypedValue_to_string(void*);
extern void destroy_TypedValue(void*);

void* print_nothing_0_svc(void* apv, struct svc_req* req) {
	return NULL; }

/* NOTE: this function implements the guts of the print_value() method
 * declared in the .x file. In general, the returned value is an xdr_something
 * type and the first thing the function should do is deallocate the return
 * value allocated in the previous call, using the pattern
 * 
 *	static result_type* result = NULL;
 *	...
 *	if(result) {
 *		xdr_free(xdr_result_type, result); }
 *	result = (result_type*) malloc(sizeof(result_type));
 *	...
 * See mini-RPC-tutorial.txt about this.
 *
 * In the present case, the returned value is a char*, i. e., an
 * "RPC-primitive type" which can be freed using free(), and a call to
 * an xdr method is not required. */
char** print_value_1_svc(apvalue* apv, struct svc_req* req) {
	void*		typed_value_obj;
			/* the string that will be returned by TypedValue_to_string: */
	static char*	serialized_obj = NULL;

	/* We want to recreate a TypedValue object from the rpc data, then
	 * serialize it to a string. */

	/* value returned by TypedValue_to_string(), if not NULL,
	 * will have been allocated using strdup(): */
	if(serialized_obj) {
		free(serialized_obj); }

	typed_value_obj = apvalue_to_TypedValue(apv);
	if(!typed_value_obj) {
		static char* errormsg = NULL;
		if(!errormsg) {
			errormsg = (char*) malloc(199); }
		strcpy(errormsg, "print_value_1_svc() error: typed_value_obj returned by apvalue_to_TypedValue() is NULL");
		return &errormsg; }
	/* returned value is allocated using strdup(): */
	serialized_obj = TypedValue_to_string(typed_value_obj);
	if(!serialized_obj) {
		static char* errormsg = NULL;
		if(!errormsg) {
			errormsg = (char*) malloc(199); }
		strcpy(errormsg, "print_value_1_svc() error: serialized_obj returned by TypedValue_to_string() is NULL");
		return &errormsg; }
	destroy_TypedValue(typed_value_obj);
	return &serialized_obj; }

char** print_array_1_svc(aparray* apv, struct svc_req* req) {
	void*		typed_value_obj;
	static char*	serialized_obj = NULL;

	/* We want to recreate a TypedValue object from the rpc data. */
	if(serialized_obj) {
		free(serialized_obj); }
	typed_value_obj = aparray_to_TypedValue(apv);
	if(!typed_value_obj) {
		static char* errormsg = NULL;
		if(!errormsg) {
			errormsg = (char*) malloc(199); }
		strcpy(errormsg, "print_value_1_svc() error: typed_value_obj returned by apvalue_to_TypedValue() is NULL");
		return &errormsg; }
	serialized_obj = TypedValue_to_string(typed_value_obj);
	if(!serialized_obj) {
		static char* errormsg = NULL;
		if(!errormsg) {
			errormsg = (char*) malloc(199); }
		strcpy(errormsg, "print_value_1_svc() error: serialized_obj returned by TypedValue_to_string() is NULL");
		return &errormsg; }
	destroy_TypedValue(typed_value_obj);
	return &serialized_obj; }
