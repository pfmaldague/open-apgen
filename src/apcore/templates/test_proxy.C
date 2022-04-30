#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <multitemplate.H>
#include <alphastring.H>
#include <UTL_time_base.H>
#include <iostream>
#include <memory>	// for auto_ptr

/* The purpose of this program is to test the variations on the multiterator
 * theme that are used in IO_write.C. */

static int	main1();
static int	main2();
static int	maxN = 10;

int test_proxy(int argc, char* argv[]) {

	if(argc == 2) {
		cerr << "Usage: " << argv[0] << " [-n <maxN>]\n";
		return -1; }
	if(argc > 2) {
		if(!strcmp(argv[1], "-n")) {
			sscanf(argv[2], "%d", &maxN); } }

	/* main1 tests the concept of a proxy list whose nodes refer
	 * to time-indexed nodes with incompatible payloads. This paves
	 * the way for a kind of multiterator for inhomogeneous lists. */
	std::cout << "\n\nResult of test 1: " << main1() << "\n\n\n";
	/* main2 tests a more ambitious concept: a version of Miterator
	 * that is able to handle inhomogeneous lists via proxy lists of
	 * only one element each. */
	return main2(); }

static int currentPriority = -1;

CTime_base random_time2() {
	// between 0 and 1:
	double r = ((double) random()) / (double) RAND_MAX;
	/* 1 millionth of 1 year = approx. 30 seconds
	 * hence 1 second = approx. 1 year / 30 million */
	double dsec = r * 30000000.0;
	CTime_base sec = CTime_base(1, 0) * dsec;
	CTime_base base_year("2000-001T00:00:00.000");
	sec += base_year;
	return sec; }

char random_char2() {
	// between 0 and 1:
	double r = ((double) random()) / (double) RAND_MAX;
	int c = (int) (52.0 * r);
	if(c < 26) {
		return 'A' + c; }
	return 'a' + (c - 26); }

int random_int2() {
	// between 0 and 1:
	double r = ((double) random()) / (double) RAND_MAX;
	int c = (int) (1000000.0 * r);
	return c; }

/* The following two classes are both time-based. They provide the
 * heterogeneous events which the flex iterator will have to deal with to
 * deal with. */
typedef Cnode<alpha_time, int> Tintnode;
typedef slist<alpha_time, Tintnode> Tintslist;
typedef tlist<alpha_time, Tintnode> Tinttlist;

typedef Cnode<alpha_time, Cstring> Tstringnode;
typedef slist<alpha_time, Tstringnode> Tstringslist;
typedef tlist<alpha_time, Tstringnode> Tstringtlist;

/* Now we emulate the classes proxynodePLD, proxyIterator, multiproxy found
 * in IO_write.C. We start by combining the heterogenous iterators into a
 * single union: */
typedef union {
	Tintslist::iterator*		i;
	Tstringslist::iterator*		s;
	} flexIterPtr;

/* We then capture this union as the payload of a flexible iterator class: */
class flxPtrRef2 {
public:

  typedef enum { integer, string, unknown } threadType;

	flxPtrRef2(Tintslist::iterator* a) : n(1), type(integer) { ptr.i = a; }
	flxPtrRef2(Tstringslist::iterator* a) : n(1), type(string) { ptr.s = a; }
	flxPtrRef2() : n(0), type(unknown) { ptr.i = NULL; }
	~flxPtrRef2();

  int n; // reference count
  flexIterPtr	ptr;
  threadType	type;
};

flxPtrRef2::~flxPtrRef2() {
	if(type == integer) delete ptr.i;
	else if(type == string) delete ptr.s; }

/* next we define the payload. SUBTLETY: even though we want to delete iterators
 * passed to the constructors via pointers when deleting a payload, we need a
 * functional copy constructor - keep in mind that the reference passed to the
 * copy constructor will be deleted in typical situations! Since we don't want
 * _that_ deletion to result in the demise of the iterator, we need to define
 * a smart iterator class - or use auto_pointers. */
class proxynodePLD {
public:

	proxynodePLD() : ref(NULL) {}
	proxynodePLD(Tintslist::iterator* A) : ref(new flxPtrRef2(A)) {}
	proxynodePLD(Tstringslist::iterator* A) : ref(new flxPtrRef2(A)) {}
	proxynodePLD(const proxynodePLD& f) : ref(f.ref) { if(ref) ref->n++; }
	~proxynodePLD() { if(ref) { if(ref->n == 0 || --ref->n == 0) delete ref; } }

  void undefine() {  if(ref) { if(ref->n == 0 || --ref->n == 0) delete ref; ref = NULL; } }
  void deep_copy(proxynodePLD&);
  flxPtrRef2*	ref;
  Cstring	str();
};

Cstring proxynodePLD::str() {
    if(ref) {
	Cstring s;
	switch(ref->type) {
		case flxPtrRef2::integer:
			s << Cstring(ref->ptr.i->getNode()->payload);
			s << ", ref = " << ((void*) ref) << ", count " << ref->n;
			return s;
		case flxPtrRef2::string:
			s << Cstring(ref->ptr.s->getNode()->payload);
			s << ", ref = " << ((void*) ref) << ", count " << ref->n;
			return s;
		case flxPtrRef2::unknown:
		default:
			; } }
    return Cstring("NULL"); }

void proxynodePLD::deep_copy(proxynodePLD& E) {
	E.undefine();
	switch((ref->type)) {
		case flxPtrRef2::integer:
			{ Tintslist::iterator* newIter = new Tintslist::iterator(*ref->ptr.i);
			E.ref = new flxPtrRef2(newIter); }
			break;
		case flxPtrRef2::string:
			{ Tstringslist::iterator* newIter = new Tstringslist::iterator(*ref->ptr.s);
			E.ref = new flxPtrRef2(newIter); }
			break;
		default:
			; } }

typedef Cnode<alpha_time, proxynodePLD> proxynode;
typedef slist<alpha_time, proxynode> proxyslist;
typedef tlist<alpha_time, proxynode> proxytlist;

/* if proxy == false, next() falls back to the base class implementation.
 * if proxy == true, next() calls next() on its internal iterator;
 * it then updates its node (which is the only one in the list.) */
class proxyIterator: public proxyslist::iterator {
public:
	proxyIterator(const proxyslist& l, bool proxy = true)
		: proxyslist::iterator(l),
			proxy_enabled(proxy) {}
	proxyIterator(const proxytlist& l, bool proxy = true)
		: proxyslist::iterator(l),
			proxy_enabled(proxy) {}
	proxyIterator(const proxyIterator& f)
		: proxyslist::iterator(f),
			proxy_enabled(f.proxy_enabled) {}
	~proxyIterator() {}

  bool		proxy_enabled;

  void		delete_self();
  void		go_to_beginning(); // same as first() but returns nothing
  proxynode*	next();
  const void*	get_void_list();
};

void proxyIterator::delete_self() {
	if(iterNode) {
		delete iterNode;
		iterNode = NULL; } }

#ifdef NO_NO_NO_WE_DON_T_WANT_TO_DO_THIS_SEE_MULTIFLEX_DESTRUCTOR
proxyIterator::~proxyIterator() {
	if(L) {
		delete L;
		L = NULL; } }
#endif /* NO_NO_NO_WE_DON_T_WANT_TO_DO_THIS_SEE_MULTIFLEX_DESTRUCTOR */

const void* proxyIterator::get_void_list() {
	proxynode* onlyNode = L->first_node();	// NOTE: technically it doesn't have to be the only node
	flxPtrRef2::threadType T = onlyNode->payload.ref->type;
  	flexIterPtr*	p = &onlyNode->payload.ref->ptr;

	switch(T) {
		case flxPtrRef2::integer:
			return p->i->get_void_list();
		case flxPtrRef2::string:
			return p->s->get_void_list();
		case flxPtrRef2::unknown:
		default:
			return NULL; } }

void proxyIterator::go_to_beginning() {
	if(!proxy_enabled) {
		proxyslist::iterator::go_to_beginning();
		return; }

	/* The iterNode will be set to the one and only node in the proxy list.
	 * Its payload is of course a flexible pointer to a real iterator; the
	 * real iterator needs to be initialized, and the time key of the one
	 * and only node needs to be set to the time key of the element pointed
	 * to by the real iterator. */
	iterNode = L->first_node();	// should be the one any only proxynode
	if(iterNode) {
  		flexIterPtr*	p = &iterNode->payload.ref->ptr;
		switch(iterNode->payload.ref->type) {
			case flxPtrRef2::integer:
				if(p->i->first()) {
					iterNode->Key.setetime(p->i->getNode()->Key.getetime()); }
				break;
			case flxPtrRef2::string:
				if(p->s->first()) {
					iterNode->Key.setetime(p->s->getNode()->Key.getetime()); }
				break;
			case flxPtrRef2::unknown:
			default: ; } } }

proxynode* proxyIterator::next() {
	if(!proxy_enabled) {
		return proxyslist::iterator::next(); }
	proxynode*		onlyNode = L->first_node();
	flxPtrRef2::threadType	T = onlyNode->payload.ref->type;
  	flexIterPtr*		p = &onlyNode->payload.ref->ptr;
	DBG_INDENT("proxyIterator::next() START; only node = " << onlyNode->get_key() << ", ref = " << ((void*)onlyNode->payload.ref)
		<< ", n = " << onlyNode->payload.ref->n << "\n");
	switch(T) {
		case flxPtrRef2::integer:
			DBG_NOINDENT("case integer\n");
			if(!iterNode) {
				if(p->i->first()) {
					iterNode = onlyNode;
					DBG_NOINDENT("Starting from empty, advancing node to " << p->i->getNode()->get_key() << "\n");
					iterNode->Key.setetime(p->i->getNode()->Key.getetime()); } }
			else {
				if(p->i->next()) {
					DBG_NOINDENT("Advancing node to " << p->i->getNode()->get_key() << "\n");
					iterNode->Key.setetime(p->i->getNode()->Key.getetime()); }
				else {
					DBG_NOINDENT("No more integers in list; setting iterNode to NULL\n");
					iterNode = NULL; } }
			break;
		case flxPtrRef2::string:
			DBG_NOINDENT("case string\n");
			if(!iterNode) {
				if(p->s->first()) {
					iterNode = onlyNode;
					DBG_NOINDENT("Starting from empty, advancing node to " << p->s->getNode()->get_key() << "\n");
					iterNode->Key.setetime(p->s->getNode()->Key.getetime()); } }
			else {
				if(p->s->next()) {
					DBG_NOINDENT("Advancing node to " << p->s->getNode()->get_key() << "\n");
					iterNode->Key.setetime(p->s->getNode()->Key.getetime()); }
				else {
					DBG_NOINDENT("No more strings in list; setting iterNode to NULL\n");
					iterNode = NULL; } }
			break;
		case flxPtrRef2::unknown:
		default:
			DBG_NOINDENT("case unknown, setting iterNode to NULL\n");
			iterNode = NULL; } 
	DBG_UNINDENT("proxyIterator::next() DONE\n");
	return iterNode; }

// OK, we got enough for a reasonable test of the proxynode concept. Let's do some testing.
int main1() {
	// first we build some lists
	Tinttlist	theIlist(true);
	Tstringtlist	theSlist(true);
	int		i;
	char		tiny[3];
	proxynode*	event;

	tiny[2] = '\0';
	currentPriority = 0;
	for(i = 0; i < maxN; i++) {
		CTime_base t = random_time2();
		int k = random_int2();

		// debug
		std::cout << t.time_to_SCET_string() << " - " << currentPriority << " { " << k << " }\n";

		theIlist << new Tintnode(t, k); }
	currentPriority = 1;
	for(i = 0; i < maxN; i++) {
		CTime_base t = random_time2();

		tiny[0] = random_char2();
		tiny[1] = random_char2();

		// debug
		std::cout << t.time_to_SCET_string() << " - " << currentPriority << " { " << tiny << " }\n";

		theSlist << new Tstringnode(t, tiny); }

	proxytlist*		theTlist = new proxytlist(true);
	Tintslist::iterator	ints(theIlist);
	Tintslist::iterator*	intPtr;
	Tstringslist::iterator	strings(theSlist);
	Tstringslist::iterator*	stringPtr;
	Tintnode*		ti;
	Tstringnode*		ts;

	while(ti = ints()) {
		intPtr = new Tintslist::iterator(ints);
		/* Reminders:
		 *		typedef Cnode<alpha_time, proxynodePLD> proxynode;
		 *		proxynodePLD(Tintslist::iterator* A) : type(integer) { ptr.i = A; }
		 *		proxynodePLD(Tstringslist::iterator* A) : type(string) { ptr.s = A; }
		 */
		(*theTlist) << new proxynode(ti->Key.getetime(), intPtr); }
	while(ts = strings()) {
		stringPtr = new Tstringslist::iterator(strings);
		(*theTlist) << new proxynode(ts->Key.getetime(), stringPtr); }

	/* We now have a proxyslist that refers to 2 incompatible types of nodes.
	 * Let's see whether we can iterate through it with a proxyIterator. */

	proxyIterator	fiter(theTlist, false);
	while(event = fiter.next()) {
		std::cout << event->Key.getetime().time_to_SCET_string() << " { " << event->payload.str() << " }\n"; }
	delete theTlist;

	return 0; }

/* NOTE: it _is_ worth defining a template for this, even if we'll still need a derived class
 *
 * try something like
 *
 * template <class proxynodePLD, class proxyIterator = slist<alpha_time, proxynodePLD>::iterator>
 * multiproxy: public Miterator<slist<alpha_time, proxynodePLD>, proxyIterator> {
 * typedef slist<alpha_time, proxynodePLD>	proxyslist;
 * typedef tlist<alpha_time, proxynodePLD>	proxytlist;
 * typedef Cnode<alpha_time, proxynodePLD>	proxynode;
 * ...
 * };
 *
 * Having defined such a template, it only remains to implement the proxynodePLD and
 * the proxyIterator in applications.
 *
 * NOTE: it's probably not necessary to provide a default for proxyIterator - the
 * default would never work.
 */
class multiproxy: public Multiproxy<proxynodePLD, proxyIterator> {
public:
	multiproxy() {}
	multiproxy(const multiproxy& m) : Multiproxy<proxynodePLD, proxyIterator>(m) {}
	~multiproxy();

  void add_flexthread(const Tintslist& til, const Cstring& s, unsigned long prio);
  void add_flexthread(const Tstringslist& tis, const Cstring& s, unsigned long prio);
  bool flexible_peek(CTime_base&, proxynodePLD&);
  slist<alpha_void, Cnode<alpha_void, proxyslist*> >	Flist;
};

void multiproxy::add_flexthread(const Tintslist& til, const Cstring& s, unsigned long prio) {
	/* we create everything we need: a new  proxylist to hold a single proxynode, which
	 * refers to a brand new specific iterator */
	proxyslist* fl = new proxyslist;
	proxynode* F = new proxynode(CTime_base(), new Tintslist::iterator(til));
	/* we create a new proxy list, put the new proxynode into it,
	 * and put a pointer to the new proxy list in Flist. */
	(*fl) << F;
	Flist << new Cnode<alpha_void, proxyslist*>(fl, fl);
	/* we want the underlying Miterator to use a flexIterator,
	 * not just any dumb one.
	 *
	 * We need to overload add_thread().
	 */
	add_thread(*fl, s, prio); }

void multiproxy::add_flexthread(const Tstringslist& tis, const Cstring& s, unsigned long prio) {
	/* we create everything we need: a new  proxylist to hold a single proxynode, which
	 * refers to a brand new specific iterator */
	proxyslist* fl = new proxyslist;
	proxynode* F = new proxynode(CTime_base(), new Tstringslist::iterator(tis));
	/* we create a new proxy list, put the new proxynode into it,
	 * and put a pointer to the new proxy list in Flist. */
	(*fl) << F;
	Flist << new Cnode<alpha_void, proxyslist*>(fl, fl);
	/* we want the underlying Miterator to use a flexIterator,
	 * not just any dumb one.
	 *
	 * We need to overload add_thread().
	 */
	add_thread(*fl, s, prio); }

multiproxy::~multiproxy() {
	Cnode<alpha_void, proxyslist*>* ptr;
	slist<alpha_void, Cnode<alpha_void, proxyslist*> >::iterator Iter(Flist);

	while(ptr = Iter()) {
		delete ptr->payload; }
	Flist.clear(); }

bool multiproxy::flexible_peek(CTime_base& next_time, proxynodePLD& iter) {
	Tthread*	tt = active_threads.first_node();
	Tthread*	new_tt;
	Tthread*	next_tt;

	DBG_INDENT("multiproxy::flexible_peek START\n");
	if(!tt) {
		DBG_UNINDENT("Miterator::flexible_peek() DONE (no active thread)\n");
		// Tell the client we can't find anything:
		return false; }

	DBG_NOINDENT("removing active thread at " << tt->payload->getNode()->Key.getetime().time_to_SCET_string() << "\n");
	active_threads.remove_node(tt);
	CTime_base origTime = tt->payload->getNode()->Key.getetime();

	/* ALL I have to do now is to save the proxynode pointed to by tt,
	 * then replace it by a fully functional deep copy.
	 *
	 * I certainly don't want to create a new proxyIterator!! I should use the
	 * same, and simply restore it to its pristine condition. */
	proxynode* node_to_save = tt->payload->getNode(); // no need to get the list, we know it's a single-element proxylist
	DBG_NOINDENT("saving the present proxynode " << proxynode2str(node_to_save) << "\n");
	proxynodePLD new_pld;
	node_to_save->payload.deep_copy(new_pld);
	proxyslist* list_to_save = node_to_save->list;
	list_to_save->remove_node(node_to_save);

	/* this node should be just as good as the original - except we
	 * didn't insert it into any list, which shouldn't matter: */
	proxynode* new_proxynode = new proxynode(node_to_save->Key, new_pld);
	DBG_NOINDENT("creating a new proxynode " << proxynode2str(new_proxynode) << "\n");
	(*list_to_save) << new_proxynode;
	tt->payload->iterNode = new_proxynode;

	assert(tt->payload->iterNode->payload.ref == new_pld.ref);

	active_threads << tt;
	move_on(); // note: only new_tt is affected since it's now the current node
	next_tt = active_threads.first_node();
	if(next_tt) {
		DBG_NOINDENT("Miterator::flexible_peek(): returning " << next_tt->payload->getNode()->get_key() << "\n");
		next_time = next_tt->payload->getNode()->Key.getetime();
		next_tt->payload->getNode()->payload.deep_copy(iter); }
	if(tt->list == &active_threads) {
		active_threads.remove_node(tt); }
	else {
		dead_threads.remove_node(tt); }
	tt->payload->iterNode = node_to_save;
	DBG_NOINDENT("restoring the current thread's iterator so it points to\n");
	DBG_NOINDENT("    " << proxynode2str(node_to_save) << "\n");
	delete new_proxynode;
	(*list_to_save) << node_to_save;

	// we know tt was active, originally
	active_threads << tt;

	// make sure we restored things correctly
	assert(active_threads.first_node() == tt);
	assert(origTime == tt->payload->getNode()->Key.getetime());
	assert(tt->payload->getNode() == node_to_save);

	DBG_UNINDENT("Miterator::flexible_peek(): returning " << (next_tt != NULL) << "\n");
	return next_tt != NULL; }

int main2() {
	// first we build some lists
	Tinttlist	theIlist(true);
	Tstringtlist	theSlist(true);
	int		i;
	char		tiny[3];
	proxynode*	event;
	proxyslist::iterator* tt;

	tiny[2] = '\0';
	currentPriority = 0;
	for(i = 0; i < maxN; i++) {
		CTime_base t = random_time2();
		int k = random_int2();

		// debug
		std::cout << t.time_to_SCET_string() << " - " << currentPriority << " { " << k << " }\n";

		theIlist << new Tintnode(t, k); }
	currentPriority = 1;
	for(i = 0; i < maxN; i++) {
		CTime_base t = random_time2();
		tiny[0] = random_char2();
		tiny[1] = random_char2();
		// debug
		std::cout << t.time_to_SCET_string() << " - " << currentPriority << " { " << tiny << " }\n";
		theSlist << new Tstringnode(t, tiny); }

	proxytlist theTlist(true);
	multiproxy multi;
	multi.add_flexthread(	theIlist,
				"Ints",
				1);
	multi.add_flexthread(	theSlist,
				"Strings",
				1);	// try this with '2' to check that priority works
	multi.first();
	Cstring thread_name;
	unsigned long prio;
	cout << "First, go through the multi list without peeking:\n";
	try {
	    // debug
	    // cout << "\n\nwriteTOL start\n";
	    while(tt = multi.there_is_a_current_event(thread_name, prio)) {
		event = tt->getNode();
		cout << event->get_key() << " { " << event->payload.str() << " }\n";
		multi.move_on(); } }
	catch(eval_error Err) {
		cout << "Error: " << Err.message << "\n"; }
	cout << "Next, go through the multi list and peek at each successive node:\n";
	multi.first();
	try {
	    // debug
	    // cout << "\n\nwriteTOL start\n";
	    while(tt = multi.there_is_a_current_event(thread_name, prio)) {
		event = tt->getNode();
		cout << event->get_key() << " { " << event->payload.str() << " }\n";

		// test flexible_peek():
		proxynodePLD	aLook;
		CTime_base	next_time;
		if(multi.flexible_peek(next_time, aLook)) {
			cout << "  peek returns " << next_time.time_to_SCET_string() << ", " << aLook.str() << "\n"; }
		multi.move_on(); } }
	catch(eval_error Err) {
		cout << "Error: " << Err.message << "\n"; }
	return 0; }
