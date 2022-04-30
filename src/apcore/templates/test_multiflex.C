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

int main1();
int main2();
static int	maxN = 10;

int test_multiflex(int argc, char* argv[]) {

	if(argc == 2) {
		cerr << "Usage: " << argv[0] << " [-n <maxN>]\n";
		return -1; }
	if(argc > 2) {
		if(!strcmp(argv[1], "-n")) {
			sscanf(argv[2], "%d", &maxN); } }

	/* main1 tests the concept of a proxy2 list whose nodes refer
	 * to time-indexed nodes with incompatible payloads. This paves
	 * the way for a kind of multiterator for inhomogeneous lists. */
	std::cout << "\n\nResult of test 1: " << main1() << "\n\n\n";
	/* main2 tests a more ambitious concept: a version of Miterator
	 * that is able to handle inhomogeneous lists via proxy2 lists of
	 * only one element each. */
	return main2(); }

static int currentPriority = -1;

static CTime_base random_time() {
	// between 0 and 1:
	double r = ((double) random()) / (double) RAND_MAX;
	/* 1 millionth of 1 year = approx. 30 seconds
	 * hence 1 second = approx. 1 year / 30 million */
	double dsec = r * 30000000.0;
	CTime_base sec = CTime_base(1, 0) * dsec;
	CTime_base base_year("2000-001T00:00:00.000");
	sec += base_year;
	return sec; }

static char random_char() {
	// between 0 and 1:
	double r = ((double) random()) / (double) RAND_MAX;
	int c = (int) (52.0 * r);
	if(c < 26) {
		return 'A' + c; }
	return 'a' + (c - 26); }

static int random_int() {
	// between 0 and 1:
	double r = ((double) random()) / (double) RAND_MAX;
	int c = (int) (1000000.0 * r);
	return c; }

typedef Cnode<alpha_time, int> Tintnode;
typedef slist<alpha_time, Tintnode> Tintslist;
typedef tlist<alpha_time, Tintnode> Tinttlist;
typedef Cnode<alpha_time, Cstring> Tstringnode;
typedef slist<alpha_time, Tstringnode> Tstringslist;
typedef tlist<alpha_time, Tstringnode> Tstringtlist;

// Now we emulate the classes flexthreadPLD, proxy2Iterator, multiflex found in IO_write.C.
typedef union {
	Tintslist::iterator*		i;
	Tstringslist::iterator*		s;
	} flexIterPtr;


class flxPtrRef {
public:

  typedef enum { integer, string, unknown } threadType;

	flxPtrRef(Tintslist::iterator* a) : n(1), type(integer) { ptr.i = a; }
	flxPtrRef(Tstringslist::iterator* a) : n(1), type(string) { ptr.s = a; }
	flxPtrRef() : n(0), type(unknown) { ptr.i = NULL; }
	~flxPtrRef();

  int n; // reference count
  flexIterPtr	ptr;
  threadType	type;
};

flxPtrRef::~flxPtrRef() {
	if(type == integer) delete ptr.i;
	else if(type == string) delete ptr.s; }

/* next we define the payload. SUBTLETY: even though we want to delete iterators
 * passed to the constructors via pointers when deleting a payload, we need a
 * functional copy constructor - keep in mind that the reference passed to the
 * copy constructor will be deleted in typical situations! Since we don't want
 * _that_ deletion to result in the demise of the iterator, we need to define
 * a smart iterator class - or use auto_pointers. */
class flexthreadPLD {
public:

	flexthreadPLD() : ref(NULL) {}
	flexthreadPLD(Tintslist::iterator* A) : ref(new flxPtrRef(A)) {}
	flexthreadPLD(Tstringslist::iterator* A) : ref(new flxPtrRef(A)) {}
	flexthreadPLD(const flexthreadPLD& f) : ref(f.ref) { if(ref) ref->n++; }
	~flexthreadPLD() { if(ref) { if(--ref->n == 0) delete ref; } }

  flxPtrRef*	ref;
  Cstring	str();
};

Cstring flexthreadPLD::str() {
    if(ref) {
	switch(ref->type) {
		case flxPtrRef::integer:
			return Cstring(ref->ptr.i->getNode()->payload);
		case flxPtrRef::string:
			return Cstring(ref->ptr.s->getNode()->payload);
		case flxPtrRef::unknown:
		default:
			; } }
    return Cstring("NULL"); }
		

typedef Cnode<alpha_time, flexthreadPLD> proxy2node;
typedef slist<alpha_time, proxy2node> proxy2slist;
typedef tlist<alpha_time, proxy2node> proxy2tlist;

/* if proxy2 == false, next() falls back to the base class implementation.
 * if proxy2 == true, next() calls next() on its internal iterator;
 * it then updates its node (which is the only one in the list.) */
class proxy2Iterator: public proxy2slist::iterator {
public:
	proxy2Iterator(const proxy2slist& l, bool proxy2 = true)
		: proxy2slist::iterator(l),
			proxy2_enabled(proxy2) {}
	proxy2Iterator(const proxy2tlist& l, bool proxy2 = true)
		: proxy2slist::iterator(l),
			proxy2_enabled(proxy2) {}
	proxy2Iterator(const proxy2Iterator& f)
		: proxy2slist::iterator(f),
			proxy2_enabled(f.proxy2_enabled) {}
	~proxy2Iterator() {}

  bool		proxy2_enabled;

  void		go_to_beginning(); // same as first() but returns nothing
  proxy2node*	next();
  const void*	get_void_list();
};

#ifdef NO_NO_NO_WE_DON_T_WANT_TO_DO_THIS_SEE_MULTIFLEX_DESTRUCTOR
proxy2Iterator::~proxy2Iterator() {
	if(L) {
		delete L;
		L = NULL; } }
#endif /* NO_NO_NO_WE_DON_T_WANT_TO_DO_THIS_SEE_MULTIFLEX_DESTRUCTOR */

const void* proxy2Iterator::get_void_list() {
	proxy2node* onlyNode = L->first_node();	// NOTE: technically it doesn't have to be the only node
	flxPtrRef::threadType T = onlyNode->payload.ref->type;
  	flexIterPtr*	p = &onlyNode->payload.ref->ptr;

	switch(T) {
		case flxPtrRef::integer:
			return p->i->get_void_list();
		case flxPtrRef::string:
			return p->s->get_void_list();
		case flxPtrRef::unknown:
		default:
			return NULL; } }

void proxy2Iterator::go_to_beginning() {
	if(!proxy2_enabled) {
		proxy2slist::iterator::go_to_beginning();
		return; }

	/* The iterNode will be set to the one and only node in the proxy2 list.
	 * Its payload is of course a flexible pointer to a real iterator; the
	 * real iterator needs to be initialized, and the time key of the one
	 * and only node needs to be set to the time key of the element pointed
	 * to by the real iterator. */
	iterNode = L->first_node();	// should be the one any only proxy2node
	if(iterNode) {
  		flexIterPtr*	p = &iterNode->payload.ref->ptr;
		switch(iterNode->payload.ref->type) {
			case flxPtrRef::integer:
				if(p->i->first()) {
					iterNode->Key.setetime(p->i->getNode()->Key.getetime()); }
				break;
			case flxPtrRef::string:
				if(p->s->first()) {
					iterNode->Key.setetime(p->s->getNode()->Key.getetime()); }
				break;
			case flxPtrRef::unknown:
			default: ; } } }

proxy2node* proxy2Iterator::next() {
	if(!proxy2_enabled) {
		return proxy2slist::iterator::next(); }
	proxy2node* onlyNode = L->first_node();
	flxPtrRef::threadType T = onlyNode->payload.ref->type;
  	flexIterPtr*	p = &onlyNode->payload.ref->ptr;
	switch(T) {
		case flxPtrRef::integer:
			if(!iterNode) {
				if(p->i->first()) {
					iterNode = onlyNode;
					iterNode->Key.setetime(p->i->getNode()->Key.getetime()); } }
			else {
				if(p->i->next()) {
					iterNode->Key.setetime(p->i->getNode()->Key.getetime()); }
				else {
					iterNode = NULL; } }
			break;
		case flxPtrRef::string:
			if(!iterNode) {
				if(p->s->first()) {
					iterNode = onlyNode;
					iterNode->Key.setetime(p->s->getNode()->Key.getetime()); } }
			else {
				if(p->s->next()) {
					iterNode->Key.setetime(p->s->getNode()->Key.getetime()); }
				else {
					iterNode = NULL; } }
			break;
		case flxPtrRef::unknown:
		default:
			iterNode = NULL; } 
	return iterNode; }

// OK, we got enough for a reasonable test of the proxy2node concept. Let's do some testing.
int main1() {
	// first we build some lists
	Tinttlist	theIlist(true);
	Tstringtlist	theSlist(true);
	int		i;
	char		tiny[3];
	proxy2node*	event;

	tiny[2] = '\0';
	currentPriority = 0;
	for(i = 0; i < maxN; i++) {
		CTime_base t = random_time();
		int k = random_int();

		// debug
		std::cout << t.time_to_SCET_string() << " - " << currentPriority << " { " << k << " }\n";

		theIlist << new Tintnode(t, k); }
	currentPriority = 1;
	for(i = 0; i < maxN; i++) {
		CTime_base t = random_time();

		tiny[0] = random_char();
		tiny[1] = random_char();

		// debug
		std::cout << t.time_to_SCET_string() << " - " << currentPriority << " { " << tiny << " }\n";

		theSlist << new Tstringnode(t, tiny); }

	proxy2tlist*		theTlist = new proxy2tlist(true);
	Tintslist::iterator	ints(theIlist);
	Tintslist::iterator*	intPtr;
	Tstringslist::iterator	strings(theSlist);
	Tstringslist::iterator*	stringPtr;
	Tintnode*		ti;
	Tstringnode*		ts;

	while(ti = ints()) {
		intPtr = new Tintslist::iterator(ints);
		/* Reminders:
		 *		typedef Tnode<alpha_time, flexthreadPLD> proxy2node;
		 *		flexthreadPLD(Tintslist::iterator* A) : type(integer) { ptr.i = A; }
		 *		flexthreadPLD(Tstringslist::iterator* A) : type(string) { ptr.s = A; }
		 */
		(*theTlist) << new proxy2node(ti->Key.getetime(), intPtr); }
	while(ts = strings()) {
		stringPtr = new Tstringslist::iterator(strings);
		(*theTlist) << new proxy2node(ts->Key.getetime(), stringPtr); }

	/* We now have a proxy2slist that refers to 2 incompatible types of nodes.
	 * Let's see whether we can iterate through it with a proxy2Iterator. */

	proxy2Iterator	fiter(theTlist, false);
	while(event = fiter.next()) {
		std::cout << event->Key.getetime().time_to_SCET_string() << " { " << event->payload.str() << " }\n"; }
	delete theTlist;

	return 0; }

class multiflex: public Miterator<proxy2slist, proxy2node> {
public:
	multiflex() {}
	multiflex(const multiflex& m) : Miterator<proxy2slist, proxy2node>(m) {}
	~multiflex();

  void add_flexthread(const Tintslist& til, const Cstring& s, unsigned long prio);
  void add_flexthread(const Tstringslist& tis, const Cstring& s, unsigned long prio);
  slist<alpha_void, Cnode<alpha_void, proxy2slist*> >	Flist;
};

// typedef Tnode<alpha_time, flexthreadPLD> proxy2node;
// void multiflex::add_flexthread(proxy2node* F, const Cstring& s, unsigned long prio)
void multiflex::add_flexthread(const Tintslist& til, const Cstring& s, unsigned long prio) {
	/* we create everything we need: a new  proxy2list to hold a single proxy2node, which
	 * refers to a brand new specific iterator */
	proxy2slist* fl = new proxy2slist;
	proxy2node* F = new proxy2node(CTime_base(), new Tintslist::iterator(til));
	/* we create a new proxy2 list, put the new proxy2node into it,
	 * and put a pointer to the new proxy2 list in Flist. */
	(*fl) << F;
	Flist << new Cnode<alpha_void, proxy2slist*>(fl, fl);
	/* we want the underlying Miterator to use a flexIterator,
	 * not just any dumb one.
	 *
	 * We need to overload add_thread().
	 */
	add_thread(*fl, s, prio); }

void multiflex::add_flexthread(const Tstringslist& tis, const Cstring& s, unsigned long prio) {
	/* we create everything we need: a new  proxy2list to hold a single proxy2node, which
	 * refers to a brand new specific iterator */
	proxy2slist* fl = new proxy2slist;
	proxy2node* F = new proxy2node(CTime_base(), new Tstringslist::iterator(tis));
	/* we create a new proxy2 list, put the new proxy2node into it,
	 * and put a pointer to the new proxy2 list in Flist. */
	(*fl) << F;
	Flist << new Cnode<alpha_void, proxy2slist*>(fl, fl);
	/* we want the underlying Miterator to use a flexIterator,
	 * not just any dumb one.
	 *
	 * We need to overload add_thread().
	 */
	add_thread(*fl, s, prio); }

multiflex::~multiflex() {
	Cnode<alpha_void, proxy2slist*>* ptr;
	slist<alpha_void, Cnode<alpha_void, proxy2slist*> >::iterator Iter(Flist);

	/* Now let's think about this. The Miterator<proxy2slist> already has an
	 * slist<alpha_void, Tnode<relptr<proxy2Iterator>, proxy2Iterator*> > which contains
	 * pointers to what it believes are standard iterators for external lists.
	 *
	 * When adding a thread to the Miterator<proxy2slist>, what is actually passed
	 * to the add_thread() method is a pointer to a new proxy2Iterator, which the
	 * Miterator owns and will delete at the end. The new proxy2Iterator is tucked
	 * away inside a Tthread, which is a Tnode<relptr<proxy2Iterator>, proxy2Iterator*>
	 * which gets its time stamp from the proxy2Iterator and has a pointer to the
	 * proxy2Iterator so it can delete it at the end.
	 */

	while(ptr = Iter()) {
		delete ptr->payload; }
	Flist.clear(); }

int main2() {
	// first we build some lists
	Tinttlist	theIlist(true);
	Tstringtlist	theSlist(true);
	int		i;
	char		tiny[3];
	proxy2node*	event;
	proxy2slist::iterator* tt;

	tiny[2] = '\0';
	currentPriority = 0;
	for(i = 0; i < maxN; i++) {
		theIlist << new Tintnode(random_time(), random_int()); }
	currentPriority = 1;
	for(i = 0; i < maxN; i++) {
		tiny[0] = random_char();
		tiny[1] = random_char();
		theSlist << new Tstringnode(random_time(), tiny); }

	proxy2tlist theTlist(true);
	multiflex multi;
	multi.add_flexthread(	theIlist,
				"Ints",
				1);
	multi.add_flexthread(	theSlist,
				"Strings",
				1);	// try this with '2' to check that priority works
	multi.first();
	Cstring thread_name;
	unsigned long prio;
	try {
	    // debug
	    // cout << "\n\nwriteTOL start\n";
	    while(tt = multi.there_is_a_current_event(thread_name, prio)) {
		event = tt->getNode();
		cout << event->get_key() << " { " << event->payload.str() << " }\n";
		multi.move_on(); } }
	catch(eval_error Err) {
		cout << "Error: " << Err.message << "\n"; }
	return 0; }
