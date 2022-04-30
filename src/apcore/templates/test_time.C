#if HAVE_CONFIG_H
#include <config.h>
#endif

#define debug_templates

#include <timeptr.H>

static int usage(char*s) {
	cout << "usage: " << s << " <-nocrash | -negative>\n";
	return -1; }

static int test1(bool crash);
static int test_negative();

int test_time(int argc, char* argv[]) {
	bool crash = true;
	bool negative = false;

	if(argc > 1) {
		if(argc != 2) {
			return usage(argv[0]); }
		if(!strncmp(argv[1], "-nocrash", strlen("-nocrash"))) {
			crash = false; }
		else if(!strncmp(argv[1], "-negative", strlen("-negative"))) {
			negative = true; }
		else return usage(argv[0]); }

	if(negative) {
		return test_negative(); }
	return test1(crash); }

void test_print_time(const char* varname, CTime_base& T, bool is_duration = false) {
	if(is_duration) {
		cout << varname << " = " << T.to_string(); }
	else {
		cout << varname << " = " << T.to_string(); }
	cout << " = (" << T.get_seconds() << ", " << T.get_milliseconds() << ")\n"; }

int test_negative() {
			/* test inspired by apgen_test/AdaptSchedule_batch/MULTI.aaf,
			 * which is a regression test for AUTOGEN */
	CTime_base	profile("2001-001T00:00:00");
	CTime_base	current("2006-073T12:34:56.789");
	CTime_base	T, U, V, W, X, Y, Z;

	cout << "profile = ";
	test_print_time("profile", profile);
	test_print_time("current", current);
	T = -current;
	test_print_time("-current", T);
	U = T + profile;
	test_print_time("profile - current", U, true);
	V = -U;
	test_print_time("-(profile - current)", V, true);
	W = current - profile;
	test_print_time("current - profile", W, true);
	X = profile + W;
	test_print_time("profile + (current - profile)", X);
	return 0; }

/* we need to derive the master class of time nodes from Tnode<alpha_time, PLD>
 * since the virtual getetime() method in the base class always returns 1970.
 *
 * We would like to choose a trivial integer payload. However, the TMptr template
 * demands that the master's payload should suppport a get_timespan() method
 * (returning CTime_base). Therefore we start by defining a suitable payload.
 *
 * Note that for the base class of masterNode we use BPnode instead of Tnode since
 * we really want a back-pointer, too. */

class masterPLD {
public:
// DATA
  CTime_base	dur;

// CONSTRUCTORS
	masterPLD(unsigned long k, unsigned long l) : dur(k, l, true) {}
	masterPLD(const masterPLD& s) : dur(s.dur) {}
	~masterPLD() {}

  bool			use_etime() { return true; }
  CTime_base		compute_timeptr_etime() { return CTime_base(0, 0, true); }
  const CTime_base&	get_timespan() const { return dur; }
};

// typedef Tnode<alpha_time, masterPLD> mBase;
typedef BPnode<alpha_time, masterPLD> masterNode;


// note: crash is unused
int test1(bool crash) {
	typedef TMptr<masterNode, Cstring /* , snode_string */ > snode;
	// typedef Tnode<timeptr<masterNode>, Cstring> s_base;
	// We start by defining a master tlist of time nodes
	tlist<alpha_time, masterNode> Masters;

	// Then we define a slave list that contains time pointers to the time nodes
	tlist<timeptr<masterNode>, snode> Slaves;

	try {
		Masters << new masterNode(CTime_base("2000-033T11:22:53.9"), masterPLD(150, 0));
		Masters << new masterNode(CTime_base("2000-042T20:22:17.3"), masterPLD(77, 323));
		Masters << new masterNode(CTime_base("2000-042T21:22:38.445"), masterPLD(112120, 323));
		Masters << new masterNode(CTime_base("2000-032T01:22:33.445"), masterPLD(0, 328));
		Masters << new masterNode(CTime_base("2000-032T01:29:03.600"), masterPLD(10, 93));
		Masters << new masterNode(CTime_base("2000-042T11:22:37.012"), masterPLD(9, 882));
		Masters << new masterNode(CTime_base("2000-033T02:22:19.45"), masterPLD(200, 173));
	    }
	catch(eval_error Err) {
		cout << "Error:\n" << Err.msg << "\n";
		exit(-1); }

	cout << "In insertion order (using next_node()):\n";

	masterNode* N = Masters.first_node();
	while(N) {
		cout << "  " << N->get_key() << ", " << N->payload.dur.to_string() << "\n";
		N = N->next_node(); }

	cout << "In insertion order (using iterator):\n";

	tlist<alpha_time, masterNode>::iterator iter(Masters);
	while((N = iter())) {
		cout << "  " << N->get_key() << ", " << N->payload.dur.to_string() << "\n"; }

	cout << "In tlist order:\n";
	N = Masters.earliest_node();
	while(N) {
		cout << "  " << N->get_key() << ", " << N->payload.dur.to_string() << "\n";
		N = N->following_node(); }

	// now populate the slave list:
	int k = 0;
	while((N = iter())) {
		static char buf[80];
		sprintf(buf, "Hello_%d", k);
		k++;
		Slaves << new snode(N, CTime_base(0, 0, true), new Cstring(buf)); }

	// print the start times:
	slist<timeptr<masterNode>, snode>::iterator iter2(Slaves);
	snode* S;
	cout << "Slave nodes:\n";
	while((S = iter2())) {
		cout << "  " << S->get_key() << ", " << *S->payload << "\n"; }
	Masters.clear();
	return 0; }
