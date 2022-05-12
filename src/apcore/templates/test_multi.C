#if HAVE_CONFIG_H
#include <config.h>
#endif

#define debug_templates

	/************************************************************************
	 *	Here we test the homogeneous multiterator template. We will	*
	 *	use a random-number generator to generate five lists of		*
	 *	time-based nodes, then define a multiterator that traverses	*
	 *	those five lists simultaneously.				*
	 *									*
	 *	Optionally, a deletion probability can be supplied. In that	*
	 *	case, nodes will be deleted at random times while the		*
	 *	multiterator is scanning the list.				*
	 ***********************************************************************/

#include <stdlib.h>		// for random()
#include <multitemplate.H>
#include <UTL_time_base.H>
#include <iostream>

static int test1();
static int currentPriority = -1;

static double random_float() {
	// between 0 and 1:
	double r = ((double) random()) / (double) RAND_MAX;
	return r; }

static CTime_base random_time() {
	// between 0 and 1:
	double r = random_float();
	/* 1 millionth of 1 year = approx. 30 seconds
	 * hence 1 second = approx. 1 year / 30 million */
	double dsec = r * 30000000.0;
	CTime_base sec = CTime_base(1, 0, true) * dsec;
	CTime_base base_year("2000-001T00:00:00.000");

	// debug
	sec += base_year;
	std::cout << "\t" << sec.to_string() << " - " << currentPriority << "\n";
	return sec; }

typedef Cntnr<alpha_time, Cstring>					Tstring;
typedef slist<alpha_time, Tstring>::iterator				ac_iter;
typedef Cntnr<relptr<ac_iter>, ac_iter*>				t_thread;
typedef slist<relptr<ac_iter>, Cntnr<relptr<ac_iter>, ac_iter*> >	t_threadslist;
typedef tlist<relptr<ac_iter>, Cntnr<relptr<ac_iter>, ac_iter*> >	t_threadtlist;

static void traverse_one(t_thread* n) {
	if(!n) return;
	traverse_one(n->Links[0]);
	std::cout << "\t" << n->get_key() << " - " << n->Key.get_priority() << "\n";
	traverse_one(n->Links[1]); }

static void traverse(t_threadtlist& L) {
	traverse_one(L.root); }

static int usage(char* s) {
	fprintf(stderr, "Usage: %s [<floating-point deletion probability>]\n", s);
	return -1; }

static int test_deletion(double probability);

static const char* labels[] = {	"First list",
				"Second list",
				"Third list",
				"Fourth list",
				"Fifth list"};

int test_multi(int argc, char* argv[]) {
	if(argc == 1) {
		return test1(); }
	else if(argc == 2) {
			// probability that a node will be deleted
		double	Prob = 0.0;
		sscanf(argv[1], "%lf", &Prob);
		printf("Running test with deletion probability = %lf\n", Prob);
		return test_deletion(Prob); }
	return usage(argv[0]); }

	// print each list in sorted order
static void	print_list_in_sorted_order(tlist<alpha_time, Tstring>& theList) {
	tlist<alpha_time, Tstring>::iterator	it1(theList);
	Tstring*				b;

	while((b = it1())) {
		std::cout << "\t" << b->Key.getetime().to_string() << " - " << b->payload << "\n"; } }

static void	fill_list(tlist<alpha_time, Tstring>& theList) {
	int						i;
	char						buf[80];

	for(i = 0; i < 1000; i++) {
		sprintf(buf, "%d", i);
		theList << new Cntnr<alpha_time, Cstring>(random_time(), buf); } }

static int test1() {
	// We start by defining a few tlists of time nodes
	tlist<alpha_time, Tstring>*			events[5];
	int						i;
	Miterator<slist<alpha_time, Tstring>, Tstring>	miter("test1");
	slist<alpha_time, Tstring>::iterator*		tt;

	for(i = 0; i < 5; i++) {
		events[i] = new tlist<alpha_time, Tstring>(true); }

	for(currentPriority = 0; currentPriority < 5; currentPriority++) {
		printf("\nFilling list %d:\n", currentPriority + 1);
		fill_list(*events[currentPriority]);
		miter.add_thread(*events[currentPriority], labels[currentPriority], currentPriority);
		printf("\nList %d in sorted order:\n", currentPriority + 1);
		print_list_in_sorted_order(*events[currentPriority]); }
	// prepare for using cout
	fflush(stdout);

	std::cout << "\nCalling miter.first()...\n";
	miter.first();
	std::cout << "Traversing the list of active threads:\n";
	traverse(miter.active_threads);
	std::cout << "\nLooping through miter.next()...\n";
	Cstring name;
	unsigned long prio;
	while((tt = miter.there_is_a_current_event(name, prio))) {
		std::cout << "\t" << tt->getNode()->Key.getetime().to_string() << " - " << prio << "\n";
		miter.next(); }
	for(i = 0; i < 5; i++) {
		delete events[i]; }
	return 0; }

static int test_deletion(double p) {
	// we start exactly as in the previous test
	tlist<alpha_time, Tstring>*			events[5];
	int						i;
	Miterator<slist<alpha_time, Tstring>, Tstring>	miter("test deletion");
	slist<alpha_time, Tstring>::iterator*		tt;

	for(i = 0; i < 5; i++) {
		events[i] = new tlist<alpha_time, Tstring>(true); }

	for(currentPriority = 0; currentPriority < 5; currentPriority++) {
		printf("\nFilling list %d:\n", currentPriority + 1);
		fill_list(*events[currentPriority]);
		miter.add_thread(*events[currentPriority], labels[currentPriority], currentPriority);
		printf("\nList %d in sorted order:\n", currentPriority + 1);
		print_list_in_sorted_order(*events[currentPriority]); }
	// prepare for using cout
	fflush(stdout);

	std::cout << "\nCalling miter.first()...\n";
	miter.first();
	std::cout << "Traversing the list of active threads:\n";
	traverse(miter.active_threads);
	std::cout << "\nLooping through miter.next()...\n";
	Cstring name;
	unsigned long prio;
	while((tt = miter.there_is_a_current_event(name, prio))) {
		std::cout << "\t" << tt->getNode()->Key.getetime().to_string()
			<< " - " << prio << "\n";
		/* at various random times, we want to delete a node chosen at random
		 * among the various threads still alive */
		if(random_float() < p) {
			// pick node(s) at random
			// insert a pointer to it in a list of pointers
			// call advance_threads_beyond()
			// delete the node(s)
			}
		miter.next(); }
	for(i = 0; i < 5; i++) {
		delete events[i]; }
	return 0; }
