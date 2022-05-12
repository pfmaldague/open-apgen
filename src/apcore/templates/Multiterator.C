#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "apDEBUG.H"
#include "Multiterator.H"
#include <assert.h>

int Multiterator_compare(const Node *a, const Node *b) {
	tol_thread	*t1, *t2;
	CTime_base	b1, b2;

	t1 = (tol_thread *) a;
	t2 = (tol_thread *) b;
	t1->get_actual_time(b1) ;
	t2->get_actual_time(b2) ;
	if(b1 < b2) {
		return - 1 ; }
	else if(b1 == b2) {
		if(t1->priority < t2->priority) {
			return -1 ; }
		else if(t1->priority > t2->priority) {
			return 1 ; }
		if((! t1->has_a_list()) && (t2->has_a_list())) {
			return 1 ; }
		else if((! t2->has_a_list()) && (t1->has_a_list())) {
			return - 1 ; }
		else if(t1->secondary_key.getTheKey() > t2->secondary_key.getTheKey()) {
			return 1 ; }
		else if(t1->secondary_key.getTheKey() < t2->secondary_key.getTheKey()) {
			return - 1 ; }
		return 0 ; }
	return 1 ; }

Multiterator::~Multiterator() {}

Multiterator::Multiterator(act_lists *theLists)
		: active_threads(compare_function(Multiterator_compare, true)),
		pointers_to_threads(compare_function(compare_bpointernodes, false)) {
	add(theLists->someActiveInstances, "active instances", 0);
	add(theLists->someDecomposedActivities, "decomposed instances", 0);
	add(theLists->someAbstractedActivities, "abstracted instances", 0); }

Multiterator::Multiterator(act_lists *theLists, int &pri)
		: active_threads(compare_function(Multiterator_compare, true)),
		pointers_to_threads(compare_function(compare_bpointernodes, false)) {
	add(theLists->someActiveInstances, "active instances", pri++);
	add(theLists->someDecomposedActivities, "decomposed instances", pri++);
	add(theLists->someAbstractedActivities, "abstracted instances", pri++); }

Multiterator::Multiterator(const Multiterator &mit)
	: L0(mit.L0),
	active_threads(mit.active_threads),
	dead_threads(mit.dead_threads),
	pointers_to_threads(compare_function(compare_bpointernodes, false)) {
	List_iterator	l(active_threads);
	tol_thread	*tt;
	while((tt = (tol_thread *) l())) {
		pointers_to_threads << new pointer_to_pointer((void *) tt->parent_list, tt); } }

Multiterator::Multiterator()
	: active_threads(compare_function(Multiterator_compare, true)),
	pointers_to_threads(compare_function(compare_bpointernodes, false)) {}

Multiterator::Multiterator(internal_compare_function F)
	: active_threads(compare_function(F, true)),
	pointers_to_threads(compare_function(compare_bpointernodes, false)) {}

Time_node *Multiterator::first() {
	tol_thread	*tt;
	List_iterator	l(L0);

	// Remember that active_threads is a clist, so all threads are in time order!
	active_threads.clear();
	dead_threads.clear();
	while((tt = (tol_thread *) l())) {
		Time_node	*tn = (Time_node *) tt->parent_list->earliest_node();

		// L0 is a simple list, we can change curNode without invalidating a blist.
		tt->curNode = tn;
		if(tn) {
			DBG_NOINDENT("Multiterator::first(): adding " << tt->kind_of_thread << " to active threads\n");
			active_threads << new tol_thread(*tt); }
		else {
			DBG_NOINDENT("Multiterator::first(): adding " << tt->kind_of_thread << " to dead threads\n");
			dead_threads << new tol_thread(*tt); } }
	tt = (tol_thread *) active_threads.first_node();
	if(tt) {
		assert(tt->curNode);
		DBG_NOINDENT("Multiterator::first(): returning " << tt->curNode->get_key() << endl);
		return tt->curNode; }
	return NULL; }

void Multiterator::adopt_node(Time_node *tn) {
	pointer_to_pointer	*pp = (pointer_to_pointer *) pointers_to_threads.find(tn->list);
	tol_thread		*tt;
	internal_compare_function	cmp = tn->list->get_compare_function();

	assert(tn);
	DBG_NOINDENT("Multiterator::adopt_node(): adopting " << tn->get_key() << endl);
	if(pp) {
		tt = (tol_thread *) pp->PTR;
		// we've got to do 2 things. First, tt has a curNode that may now be wrong because
		// tn is earlier. Second, whatever the first thread is may need to be reinserted.

		// Oh, wait... it's all one thing; re-inserting the thread will of course 'redefine' the first node.
		if(tt->list == &active_threads) {
			bool reinsert = false;

			// note: we know ct exists...

			// for RES_timenodes, we need a fine-grained comparison method
			if(tn->node_type() == RES_TIMENODE && cmp(tn, tt->curNode) < 0) {
				reinsert = true; }
			// else this is good enough
			else if(tn->getetime() < tt->curNode->getetime()) {
				reinsert = true; }
			if(reinsert) {
				active_threads.remove_node(tt);
				tt->setetime(tn->getetime());
				tt->curNode = tn;
				active_threads << tt; } }
		else if(tt->list == &dead_threads) {
			dead_threads.remove_node(tt);
			tt->setetime(tn->getetime());
			tt->curNode = tn;
			active_threads << tt; } }
	else {
		return; } }

Time_node *Multiterator::next() {
	tol_thread*	tt = (tol_thread *) active_threads.first_node();
	Time_node*	to_return;
	Time_node*	for_next_time;
	Node*		ptr;

	if(!tt) {
		// We'll be ready next time:
		first();
		// Tell the client we are done:
		return NULL ; }
	assert(tt->curNode);
	assert(tt->curNode->list == tt->parent_list);
	to_return = tt->curNode;
	active_threads.remove_node(tt);
	assert(!tt->list);
	// the blist we added may not be a clist, so use following_node instead of next_node
	for_next_time = (Time_node *) to_return->following_node();
	if(for_next_time) {
		tt->setetime(for_next_time->getetime());
		tt->curNode = for_next_time;
		assert(for_next_time->list == tt->parent_list);
		DBG_NOINDENT("Multiterator::next(): setting future node to " << ((void *) for_next_time)
			<< ", time " << for_next_time->getetime().to_string() << endl);
		DBG_NOINDENT("Multiterator::next(): returning present node (" << to_return->get_key()
			<< ") at time " << to_return->getetime().to_string() << endl);
		active_threads << tt;
		return to_return; }
	DBG_NOINDENT("Multiterator::next(): no future node; returning last node of thread at "
		<< to_return->get_key() << endl);
	tt->curNode = NULL;
	dead_threads << tt;
	return to_return; }

Time_node* Multiterator::remove_nodes(tlist<alpha_void, Cntnr<alpha_void, value_node*> >& sel) {
	tol_thread*	tt;
	Time_node*	u;
	List_iterator	titer(active_threads);
	List		ptrs;
	List_iterator	piter(ptrs);
	Pointer_node*	ptr;

	DBG_INDENT("Multi::remove_nodes START\n");
	while((tt = (tol_thread*) titer())) {
		ptrs << new Pointer_node(tt, NULL); }
	while((ptr = (Pointer_node*) piter())) {
		tt = (tol_thread*) ptr->get_ptr();
		if(sel.find((void*)tt->curNode)) {
#			ifdef apDEBUG
			if(tt->curNode->node_type() == VALUE_NODE) {
				DBG_NOINDENT("node " << tt->curNode->get_key() << " is in the selection...\n"); }
#			endif /* apDEBUG */
			active_threads.remove_node(tt);
			while((u = (Time_node*) tt->curNode->following_node())) {
				tt->curNode = u;
				if(!sel.find((void*) u)) {
					DBG_NOINDENT("successor " << tt->curNode->get_key() << " is NOT in the selection; enough!\n");
					break; }
				else {
					DBG_NOINDENT("successor " << tt->curNode->get_key() << " is in the selection...\n"); } }
			if(!u) {
				DBG_NOINDENT("Thread empty, putting into dead list.\n");
				tt->curNode = NULL;
				dead_threads << tt; }
			else {
				DBG_NOINDENT("Thread alive, adjusting thread time to " << u->getetime().to_string()
					<< "\n");
				assert(tt->curNode);
				assert(tt->curNode->list == tt->parent_list);
				tt->setetime(u->getetime());
				active_threads << tt; } }
		else {
			DBG_NOINDENT("node " << tt->curNode->get_key() << " is NOT in the selection...\n"); }
		}
	tt = (tol_thread*) active_threads.first_node();
	if(tt) {
		assert(tt->curNode->list == tt->parent_list);
		DBG_UNINDENT("Multi::remove_nodes END (OK)\n");
		return tt->curNode; }
	DBG_UNINDENT("Multi::remove_nodes END (empty)\n");
	return NULL; }

void	Multiterator::checkAll() {
	tol_thread*	tt;
	List_iterator	titer(active_threads);

	while((tt = (tol_thread*) titer())) {
		Time_node*	u = tt->curNode;
		while(1) {
			assert(u->list == tt->parent_list);
			CTime_base s(u->getetime());
			if(!(u = (Time_node*) u->following_node())) {
				break; } } } }

Time_node *Multiterator::peek() {
	tol_thread	*tt = (tol_thread *) active_threads.first_node();

	if(!tt) {
		// DBG_NOINDENT("Multiterator::peek(): no thread, returning NULL\n");
		return NULL ; }
	assert(tt->curNode);
	assert(tt->curNode->list == tt->parent_list);
	// tt _must_ have a non-NULL curNode
	DBG_NOINDENT("Multiterator::peek(): returning curNode " << ((void *) tt->curNode)
		<< ", key " << tt->curNode->get_key() << endl);
	return tt->curNode; }

bool Multiterator::isAheadOf(Time_node* n) {
	List*			l = n->list;
	if(!l) {
		throw(eval_error("isInTheFuture: given node has no list")); } 
	pointer_to_pointer*	pp = (pointer_to_pointer*) pointers_to_threads.find((void*) l);
	if(!pp) {
		throw(eval_error("isInTheFuture: given node is in unknown list")); } 
	tol_thread*		tt = (tol_thread*) pp->get_ptr();
	Time_node*		m = tt->curNode;
	if(!m) {
		// thread is dead, ahead of everything
		cerr << "isAheadOf: thread (" << tt->kind_of_thread << ") is dead, returning true\n";
		return true; }
	if(l->get_compare_function()(m,n) >= 0) {
		cerr << "isAheadOf: thread (" << tt->kind_of_thread << ") time " << m->getetime().to_string();
		cerr << ", given node time = " << n->getetime().to_string() << " - ";
		cerr << "the thread is ahead.\n";
		return true; }
	return false; }

void Multiterator::add(const blist &b, const char *s, int which_priority) {
	Time_node	*tn = (Time_node *) b.earliest_node();
	tol_thread	*tt;
	CTime_base	T0;

	DBG_INDENT("Multiterator::add START - adding list " << s << endl);
	if(tn) {
		assert(tn->list == &b);
		T0 = tn->getetime();
		tt = new tol_thread(T0, which_priority, &b);
		tt->curNode = tn;
		DBG_NOINDENT("current node = " << tn->get_key() << endl);
		active_threads << tt; }
	else {
		tt = new tol_thread(T0, which_priority, &b);
		tt->curNode = tn;
		DBG_NOINDENT("list empty, no current node.\n");
		dead_threads << tt; }
	pointers_to_threads << new pointer_to_pointer((void *) &b, tt);
	tt->kind_of_thread = s;

	// Important: no matter what, we keep the threads in a safe place.
	L0 << new tol_thread(*tt);
	DBG_UNINDENT("Multiterator::add DONE\n"); }

void Multiterator::dump_to_TOL(aoString &Stream, CTime_base end_time) {}
