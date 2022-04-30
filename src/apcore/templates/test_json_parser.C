#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <json.h>

extern "C" {
#include <concat_util.h>
} // extern "C"

void recursively_explore(json_object* obj, int indentation) {
	enum json_type			theType = json_object_get_type(obj);
	int				i;
	struct json_object_iterator	obj_iter;
	struct json_object_iterator	obj_end;

	for(i = 0; i < indentation; i++) {
		printf(" "); }
	printf("  (Type: %s) Value: ", json_type_to_name(theType));
	switch(theType) {
		case json_type_null:
			printf("NULL\n");
			break;
		case json_type_boolean:
			printf("%d\n", json_object_get_boolean(obj));
			break;
		case json_type_double:
			printf("%lf\n", json_object_get_double(obj));
			break;
		case json_type_int:
			printf("%ld\n", (long) json_object_get_int64(obj));
			break;
		case json_type_object:
			printf("struct\n");
			obj_end = json_object_iter_end(obj);
			obj_iter = json_object_iter_begin(obj);
			indentation += 4;
			while(!json_object_iter_equal(&obj_iter, &obj_end)) {
				for(i = 0; i < indentation; i++) {
					printf(" "); }
				printf("%s:\n",
					json_object_iter_peek_name(&obj_iter));
				recursively_explore(
					json_object_iter_peek_value(&obj_iter),
					indentation);
				json_object_iter_next(&obj_iter); }
			indentation -= 4;
			break;
		case json_type_array:
			printf("array\n");
			indentation += 4;
			for(int j = 0; j < json_object_array_length(obj); j++) {
				for(i = 0; i < indentation; i++) {
					printf(" "); }
				printf("%d:\n", j);
				recursively_explore(
					json_object_array_get_idx(obj, j),
					indentation); }
			indentation -= 4;
			break;
		case json_type_string:
			printf("%s\n", json_object_get_string(obj));
			break;
		default:
			printf("UNKNOWN\n");
			break; } }

static int usage(char* prog) {
	fprintf(stderr, "Usage: %s <input-file>\n", prog);
	return -1; }

	/* this function is based on the tests subdirectory
	 * of the json-c package and in particular on the
	 * source file json-c/tests/test_parse.c */
int test_json_parser(int argc, char* argv[]) {
	json_object*			obj;
	int				i;
	FILE*				input_file;
	buf_struct			bs = {NULL, 0, 0};
	char				buf[2];
	enum json_tokener_error		jsonError;

	if(argc != 2) {
		return usage(argv[0]); }
	input_file = fopen(argv[1], "r");
	if(!input_file) {
		fprintf(stderr, "File %s cannot be opened\n", argv[1]);
		return -1; }
	buf[1] = '\0';
	while(fread(buf, 1, 1, input_file)) {
		concatenate(&bs, buf); }
	obj = json_tokener_parse_verbose(bs.buf, &jsonError);
	if(!obj) {
		fprintf(stderr, "%s: parsing error %s\n", argv[0],
			json_tokener_error_desc(jsonError));
		return -1; }
	recursively_explore(obj, /* indent = */ 0);
	destroy_buf_struct(&bs);
	json_object_put(obj);
	return 0; }
