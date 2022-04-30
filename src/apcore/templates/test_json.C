#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <json.h>

	/* this function is based on the tests subdirectory
	 * of the json-c package */
int test_json(int argc, char* argv[]) {
	json_object*			obj;
	int				i;
	struct json_object_iterator	obj_iter;
	struct json_object_iterator	obj_end;

	/* first build an array */
	obj = json_object_new_array();
	json_object_array_add(obj, json_object_new_int(123));
	json_object_array_add(obj, json_object_new_int(204));
	json_object_array_add(obj, json_object_new_int(209));
	json_object_array_add(obj, json_object_new_string("Hello"));

	/* iterate over the array */
	printf("\nfirst array = (using explicit iteration)\n");
	for(i = 0; i < json_object_array_length(obj); i++) {
		json_object*	element = json_object_array_get_idx(obj, i);
		printf("\t[%d] = %s\n", i, json_object_to_json_string(element)); }
	printf("\nfirst array = (using to_string())\n");
	printf("%s", json_object_to_json_string(obj));
	/* free all memory associated with obj */
	json_object_put(obj);

	/* next build a struct */
	obj = json_object_new_object();
	json_object_object_add(obj, "abc", json_object_new_int(100));
	json_object_object_add(obj, "zzy", json_object_new_boolean(0));
	json_object_object_add(obj, "77FD", json_object_new_string("World"));

	/* iterate over the struct */
	printf("\nsecond object = (using foreach macro)\n");
	/* NOTE: what about multiple iteration loops? Do the iteration
	 * variables have to be different, or can they be the same?
	 * Let's find out
	 */
	json_object_object_foreach(obj, key, val) {
		printf("\t%s: %s\n", key, json_object_to_json_string(val)); }

	/* OK, I tested: the following does not work because of variable duplication
	printf("again:\n");
	json_object_object_foreach(obj, key, val) {
		printf("\t%s: %s\n", key, json_object_to_json_string(val)); }
	 */

	/* get the length of the object */
	printf("Will now iterate explicitly over %d object(s) using an iterator:\n",
		json_object_object_length(obj));
	obj_end = json_object_iter_end(obj);
	obj_iter = json_object_iter_begin(obj);
	while(!json_object_iter_equal(&obj_iter, &obj_end)) {
		printf("%s: %s\n", json_object_iter_peek_name(&obj_iter),
			json_object_to_json_string(json_object_iter_peek_value(&obj_iter)));
		json_object_iter_next(&obj_iter); }
	printf("Finally, using to_string():\n%s\n", json_object_to_json_string(obj));
	json_object_put(obj);
	return 0; }
