#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <doublelist.H>
#include <alphastring.H>

static int usage(char*s) {
	cout << "usage: " << s << " <-nocrash | -negative>\n";
	return -1; }

extern int test12();
typedef BPnode<alpha_string, long int> foo;

int test_double_list(int argc, char* argv[]) {
	if(argc > 1) {
		return usage(argv[0]); }

	return test12(); }

static void print_dlist(double_list<alpha_string, foo>& dl) {

#ifdef NO_BRACKET_OPERATOR_ANYMORE
	for(int i = 0; i < dl.get_length(); i++) {
		foo* obj = dl[i];
		cout << "dl[" << i << "] = {" << obj->get_key() << ": " << obj->payload << "}; ptr key = " <<
			dl[i]->get_key() << "; index = " << dl.get_index(obj) << "\n";
	}
#endif /* NO_BRACKET_OPERATOR_ANYMORE */
}

int test12() {
	double_list<alpha_string, foo>	dList;

	dList << new foo("hello", 1000);
	dList << new foo("alpha", 2000);
	dList << new foo("omega", 400);
	dList << new foo("nu", 5000);
	dList << new foo("epsilon", -1000);
	dList << new foo("what", 10);
	dList << new foo("zebra", 1200);
	dList << new foo("antilope", 800);

	slist<alpha_string, foo>::iterator	iter(dList.get_iterator());
	foo*					obj;

	while((obj = iter())) {
		cout << obj->get_key() << ": " << obj->payload << "\n"; }
	print_dlist(dList);

	obj = dList.find("zebra");
	assert(obj);
	cout << "dList[\"zebra\"] = {" << obj->get_key() << ": " << obj->payload << "}\n";
	obj = dList.find("antilope");
	assert(obj);
	cout << "dList[\"antilope\"] = {" << obj->get_key() << ": " << obj->payload << "}\n";
	obj = dList.find("Aaargh");
	assert(!obj);

	cout << "deleting nu...\n";
	delete dList.find("nu");
	print_dlist(dList);

	cout << "adding nu...\n";
	dList << new foo("nu", 5000);
	print_dlist(dList);
	return 0; }
