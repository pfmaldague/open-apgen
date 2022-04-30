#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* TypedValueClient.C - main program and C++ utilities for processing APGen TypedValue
 * objects on the client side of the RPC interface */


#include <string.h>
#include <stdio.h> // without this the xdr.h include will fail to define xdrstdio_create()

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <rpc/types.h>
#include <rpc/xdr.h>

extern "C" {
#include <apvalue.h>
#include <apvalue_intfc.h>
// #include <typed_value.h>
}



int main(int argc, char* argv[]) {
	aparray		new_aparray;
	apindex		new_index;
	apvalue		new_val;
	aparrayelement*	el;
	CLIENT*		cl;
	char**		result;
	int		el_count = 0;
	int		parent = 0;

	if (argc < 2) {
		fprintf(stderr, "%s: need host argument\n", argv[0]);
		return -1; }
/* test TypedValue: 
 * ["first" = true, "second" = ["one" = "foo", "two" = 3.141592, "three" = 1.414, "four" = 2.001] ]
 * 
 * aparrayelement count:
 * 	  1 for the top-level struct
 * 	  1 for item 1 of the top-level struct
 * 	  1 for the 2nd-level struct (item 2)
 * 	  4 for the 4 items in the 2nd-level struct
 * 
 * total  7
 */

	/* we will create a struct of 7 elements */
	new_aparray.apvector.apvector_val = (aparrayelement*) malloc(sizeof(aparrayelement) * 7);
	new_aparray.apvector.apvector_len = 7;

	/* make the root of the array */
	new_index.tag = AP_NONE;
	// make_simpleInt(&new_val, 137);
	make_simpleStruct(&new_val);
	mk_aparrayelement(new_aparray.apvector.apvector_val, &new_index, el_count, -1, &new_val);
	parent = el_count;
	el_count++;

	/* make element 0 of the top array */
	new_index.tag = AP_STRUCT_STYLE;
	new_index.apindex_u.s = strdup("first");
	make_simpleBool(&new_val, 1);
	mk_aparrayelement(new_aparray.apvector.apvector_val + el_count, &new_index, el_count, parent, &new_val);
	el_count++;

	/* make element 1 of the top array */
	new_index.tag = AP_STRUCT_STYLE;
	new_index.apindex_u.s = strdup("second");
	make_simpleStruct(&new_val);
	mk_aparrayelement(new_aparray.apvector.apvector_val + el_count, &new_index, el_count, parent, &new_val);
	parent = el_count;
	el_count++;

	/* make element 0 of the lower array */
	new_index.tag = AP_STRUCT_STYLE;
	new_index.apindex_u.s = strdup("one");
	make_simpleString(&new_val, "foo");
	mk_aparrayelement(new_aparray.apvector.apvector_val + el_count, &new_index, el_count, parent, &new_val);
	el_count++;

	/* make element 1 of the array */
	new_index.tag = AP_STRUCT_STYLE;
	new_index.apindex_u.s = strdup("two");
	make_simpleFloat(&new_val, 3.141592);
	mk_aparrayelement(new_aparray.apvector.apvector_val + el_count, &new_index, el_count, parent, &new_val);
	el_count++;

	/* make element 2 of the array */
	new_index.tag = AP_STRUCT_STYLE;
	new_index.apindex_u.s = strdup("three");
	make_simpleFloat(&new_val, 1.414);
	mk_aparrayelement(new_aparray.apvector.apvector_val + el_count, &new_index, el_count, parent, &new_val);
	el_count++;

	/* make element 3 of the array */
	new_index.tag = AP_STRUCT_STYLE;
	new_index.apindex_u.s = strdup("four");
	make_simpleFloat(&new_val, 2.001);
	mk_aparrayelement(new_aparray.apvector.apvector_val + el_count, &new_index, el_count, parent, &new_val);
	el_count++;

	/* At this point we have a valid structure, new_aparray. Let's
	 * serialize it to see what it looks like... */
	const char*	filename = "test.raw";
	FILE*		testfile = fopen(filename, "w");
	struct stat	FileStats;
	XDR		xdr_data;

	xdrstdio_create(&xdr_data, testfile, XDR_ENCODE);

	if (!xdr_aparray(&xdr_data, &new_aparray)) {
		fprintf (stderr, "apvalue test: could not encode\n"); }
	else {
		xdr_destroy (&xdr_data);
		fclose (testfile);
		stat(filename, &FileStats);
		long bytes_written = FileStats.st_size;
		long est_bytes = xdr_aparray_length(&new_aparray);
		printf("client: raw file size = %ld, predicted size = %ld\n",
			bytes_written, est_bytes); }

	cl = clnt_create(argv[1], PRINTVAL, PRINTVAL_V1, (char*) "tcp");
	if (cl == NULL) {
		printf("error: could not connect to server.\n");
		return -1; }
	result = print_array_1(&new_aparray, cl);
	if (result == NULL) {
		printf("error: RPC failed!\n");
		return -1; }
	printf("client: server returns %s\n", *result);
	/* Note that for this to work we must have constructed the aparray in a
	 * manner consistent with the xdr implementation, i. e., using strdup()
	 * etc. */
#ifdef HAVE_MACOS
	xdr_free( (xdrproc_t) xdr_aparray, &new_aparray);
#else
	xdr_free( (xdrproc_t) xdr_aparray, (char*) &new_aparray);
#endif /* HAVE_MACOS */

	return 0; }
