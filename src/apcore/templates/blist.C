#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define apDEBUG
#include "apDEBUG.H"

#include <iostream>
#include <sstream>
#include <stdlib.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

using namespace std;

#include "v_list.H"
#include "C_list.H"
#include "C_string.H"
#include "RES_exceptions.H"
#include <APbasic.H>

// #define DEBUG_BLIST

#ifdef DEBUG_BLIST
#define apDEBUG
#endif

// debug
aoString *VVVdebug = NULL;
void set_VVVdebug_to(aoString *a) {
	VVVdebug = a; }
bool VVVproblemFound = false;
#include "APdata.H"

// 2 LIST (PFM)

blist &blist::operator = (const blist & L) {
	Node	*node;

	if(cmp != L.cmp) return *this;
	clear();
	for (	node = L.first_node();			//for each node on right
		node;
		node = node->next_node()) {
		insert_node(node->copy()); }		//insert a copy
	return *this; }

clist &clist::operator = (const clist &L) {
	Node	*node;

	myType = BLIST;
	if(cmp != L.cmp) return *this;
	clear();
	for (	node = L.first_node();			//for each node on right
		node;
		node = node->next_node()) {
		insert_node(node->copy()); }		//insert a copy
	return *this; }

blist::blist(const blist &B) : cmp(B.cmp),
		enabled(B.enabled),
		callbacksEnabled(B.callbacksEnabled) {
	myType = BLIST;
	operator = (B);
}

blist::~blist() {}
	
void clist::synchronize_all() {
	Node* M;
	List l;
	CTime_base T, U;

	for(Node* N = first_node(); N; N = M) {
		M = N->next_node();
		Time_node* tn = dynamic_cast<Time_node*>(N);
		assert(tn);
		tn->get_actual_time(T);
		if(T != (U = tn->getetime())) {
			remove_node(tn);
			tn->actual_time.theTime = U;
			l.insert_node(N);
		}
	}
	for(Node* N = l.first_node(); N; N = M) {
		M = N->next_node();
		Time_node* tn = dynamic_cast<Time_node*>(N);
		assert(tn);
		l.remove_node(N);
		insert_node(tn);
	}

	// debug
	// for(Node* N = first_node(); N; N = M) {
	// 	M = N->next_node();
	// 	Time_node* tn = dynamic_cast<Time_node*>(N);
	// 	Time_node* tn2 = dynamic_cast<Time_node*>(M);
	// 	assert(!tn2 || tn2->getetime() >= tn->getetime());
	// }
	
}

void blist::order() {
	Node	*N = first_node(), *M;

	if(!root) return;
	// disable_callbacks();
	callbacksEnabled = false;
	while(N) {
		M = N->next_node();
		List::remove_node(N);
		N = M; }
	order(root);
	// enable_callbacks();
	callbacksEnabled = true; }

void blist::order(Node *N) {
	Node	*M;

	if((M = N->get_left()))
		order(M);
	List::insert_node(N);
	if((M = N->get_right()))
		order(M); }

Node *blist::earliest_node() const {
	Node	*N = root, *M;

	if(! root) return NULL;
	while((M = N->get_left()))
		N = M;
	return N; }

Node *blist::latest_node() const {
	Node	*N = root, *M;

	if(! root) return NULL;
	while((M = N->get_right()))
		N = M;
	return N; }

void blist::clear() {
#		ifdef DEBUG_BLIST
		cout << "blist::clear(" << (void *) this
			<< ") START...\n";
#		endif
#		ifdef TRACE
		cout << (void *) this << " clear start\n";
		cout.flush();
#		endif

	enabled = false;
	List::clear();
	root = NULL;
	enabled = true;

#		ifdef TRACE
		cout << (void *) this << " clear end\n";
		cout.flush();
#		endif
#		ifdef DEBUG_BLIST
		cout << "blist::clear(" << (void *) this
			<< ") DONE.\n";
#		endif
	}

List & blist::operator << (Node *N) {
	if (! insert_node(N)) {
		cerr << "APGEN Internal Error:  duplicate blist node ignored by L"
			"ist&blist::operator<<(Node*N)"
			<< endl;
		// exit(-1);
		}
	return *this; }

bool blist::operator <= (blist & rhs) {
	Node *N = first_node();

	while(N) {
		if(! rhs.find(N)) return false;
		N = N->next_node(); }
	return true; }

void blist::add_without_duplication(List & l) {
	Node *N = l.first_node();
	Node *M;

	while(N) {
		M = N->next_node();
		if(! find(N)) insert_node(N);
		N = M; } }

List & blist::operator << (List & l) {
	Node *N = l.first_node();
	Node *M;
#			ifdef DEBUG_BLIST
			cout << "operator blist (" << get_length() << ") <- List(" <<
				l.get_length() << ") START...\n";
#			endif

	while(N) {
		M = N->next_node();
		if (! insert_node(N)) {
			cerr << "APGEN Internal Error:  duplicate blist node ignored "
				"by List&blist::operator<<(List&l)"
				<< endl;
			// a little too drastic:
			// exit(-2);
			}
		N = M; }
	return *this; }

List & blist::operator << (blist & l) {
	Node *N = l.first_node();
	Node *M;

#			ifdef DEBUG_BLIST
			cout << "operator blist (" << get_length() << ") <- blist(" <<
				l.get_length() << ") START...\n";
#			endif
	while(N) {
		M = N->next_node();
		if (! insert_node(N)) {
			cerr << "APGEN Internal Error:  duplicate blist node ignored "
				"by List&blist::operator<<(blist&l)"
				<< endl;
			// a little too drastic:
			// exit(-3);
			}
		N = M; }
	return *this; }

Node* blist::find(int I) const {
	if(cmp.compfunc == compare_int_nodes) {
		int_node	Inn(I);

		return find(&Inn); }
// 	else if(cmp.compfunc == compare_arrayelements) {
// 		ArrayElement	Inn(I);
// 
// 		return find(&Inn); }
	return NULL; }

Node* blist::find(void*p) const {
	bpointernode	pn(p, NULL);
	return find(&pn); }

bstringnode& std_bs() {
       static bstringnode	theBnode;

       return theBnode; }

Node* blist::find(const char *s) const {
	Node			*ret;
	static const char	*savethis, *savethat;

	std_bs().theString.substitute(s, &savethis);
	ret = find(&std_bs());
	std_bs().theString.substitute(savethis, &savethat);
	return ret; }

Node *blist::find(const Cstring &s) const {
	Node			*ret;
	static const char	*savethis, *savethat;

	std_bs().theString.substitute(*s, &savethis);
	ret = find(&std_bs());
	std_bs().theString.substitute(savethis, &savethat);
	return ret; }

// does NOT assume that the list is ordered (links do NOT
// have to agree with previous/next pointers)
Node *blist::find_latest_before(Node *new_node) const {
	int	f;
	Node	*current_node = root;
	Node	*last_earlier_node = NULL;

	if(!current_node) return NULL;
	do {
		if ((f = cmp(new_node, current_node)) < 0)
			current_node = current_node->get_left();
		else if(!f) {
			if(current_node != new_node)
				return current_node;
			else
				current_node = current_node->get_left(); }
		else {
			last_earlier_node = current_node;
			current_node = current_node->get_right(); }
		} while(current_node);
	return last_earlier_node; }

Node *blist::find_latest_before_w_comp_func(Node *new_node, internal_compare_function F) const {
	int	f;
	Node	*current_node = root;
	Node	*last_earlier_node = NULL;

	if(!current_node) return NULL;
	do {
		if ((f = F(new_node, current_node)) < 0)
			current_node = current_node->get_left();
		else if(! f) {
			if(current_node != new_node)
				return current_node;
			else
				current_node = current_node->get_left(); }
		else {
			last_earlier_node = current_node;
			current_node = current_node->get_right(); }
		} while(current_node);
	return last_earlier_node; }

// does NOT assume that the list is ordered (links do NOT
// have to agree with previous/next pointers)
Node *blist::find_earliest_after(Node *new_node) const {
	int	f;
	Node	*current_node = root;
	Node	*last_later_node = NULL;

	if(! current_node) return NULL;
	do {
		if(!current_node->list) {
			return NULL; }
		if ((f = cmp(new_node, current_node)) < 0) {
			last_later_node = current_node;
			current_node = current_node->get_left(); }
		else if(! f) {
			if(current_node != new_node) {
				return current_node; }
			else {
				current_node = current_node->get_right(); } }
		else {
			current_node = current_node->get_right(); }
		} while(current_node);
	return last_later_node; }

Node *blist::find(Node *new_node) const {
	int	f;
	Node	*current_node = root;

	if(!current_node) return NULL;
	do {
		if(!current_node->list) {
			cerr << "blist::find: node at " << (void *) this
				<< " has NULL list pointer" << endl;
			return NULL; }
		if ((f = cmp(new_node, current_node)) < 0)
			current_node = current_node->get_left();
		else if(! f)
			break;
		else
			current_node = current_node->get_right();
		} while(current_node);
	return current_node; } // node already exists

	// The one place where the following function is used is in
	// UI_ds_timeline, dealing with Legends. You need to
	// search through Legends quickly, whence the blist part;
	// but you want to control the order also and
	// the display uses the linked-list order for that purpose.
Node* blist::insert_node(Node *N, Node *before_this_one) {
	Node* K = N;

	if(insert_binary_node(K)) {
		if(cmp.compfunc == compare_int_nodes) {
			// use K instead; that was the purpose of making
			// the argument of insert_binary_node() a reference:
			// int_node	*M = (int_node *) find_latest_before(N);

			((int_node *) N)->secondary_key = 0;
			if(K) {
				if(((int_node *) K)->_i == ((int_node *) N)->_i) {
					((int_node *) N)->secondary_key =
						((int_node *) K)->secondary_key + 1;
				}
			}
		} else if(cmp.compares_times()) {
			// use K instead; that was the purpose of making the
			// argument of insert_binary_node() a reference:
			// Time_node	*M = (Time_node *) find_latest_before(N);

			((Time_node *) N)->secondary_key.theKey = 0;
			if(K) {
				CTime_base	t1, t2;

				((Time_node *) K)->get_actual_time(t1);
				((Time_node *) N)->get_actual_time(t2);
				if(t1 == t2) {
					((Time_node *) N)->secondary_key.theKey =
					((Time_node *) K)->secondary_key.theKey + 1;
				}
			}
		}
		// see "IMPORTANT NOTE" below
		return List::insert_first_node_after_second(N, before_this_one);
	}
	return NULL;
}

Node* blist::insert_node(Node* N) {
	Node* K = N;

	if(insert_binary_node(K)) {
		if(cmp.compfunc == compare_int_nodes) {
			// use K instead; that was the purpose of making the
			// argument of insert_binary_node() a reference:
			// int_node	*M = (int_node *) find_latest_before(N);

			((int_node *) N)->secondary_key = 0;
			if(K) {
				if(((int_node *) K)->_i == ((int_node *) N)->_i) {
					((int_node *) N)->secondary_key =
					((int_node *) K)->secondary_key + 1;
				}
			}
		} else if(cmp.compares_times()) {
			// use K instead; that was the purpose of making the
			// argument of insert_binary_node() a reference:
			// Time_node	*M = (Time_node *) find_latest_before(N);

			((Time_node *) N)->secondary_key.theKey = 0;
			if(K) {
				CTime_base	t1, t2;

				((Time_node *) K)->get_actual_time(t1);
				((Time_node *) K)->get_actual_time(t2);
				if(t1 == t2) {
					((Time_node *) N)->secondary_key.theKey =
					((Time_node *) K)->secondary_key.theKey + 1;
				}
			}
		}
		return List::insert_node(N);
	}
	return NULL;
}

void Node::reset_links() {
	indicator = 2;
	Links[1] = NULL;
	Links[0] = NULL;
	init_cons();
	}

int Node::get_pathlength(int &length) {
	int	L = 0, R = 0;
	Node	*N;

	if((N = Links[0])) {
		if(!N->get_pathlength(L)) {
			return 0; } }
	if((N = Links[1])) {
		if(!N->get_pathlength(R)) {
			return 0; } }
	if(R > L) {
		if(indicator != 1) {
			cout << "PROBLEM: " << get_key() << " has indicator " << indicator
				<< ", left = " << L << ", right = " << R << endl;
			return 0; }
		// store largest value in L
		L = R; }
	else if(L > R) {
		if(indicator != 0) {
			cout << "PROBLEM: " << get_key() << " has indicator " << indicator
				<< ", left = " << L << ", right = " << R << endl;
			return 0; } }
	else {
		if(indicator != 2) {
			cout << "PROBLEM: " << get_key() << " has indicator " << indicator
				<< ", left = " << L << ", right = " << R << endl;
			return 0; } }
	length = L + 1;
	return 1; }

bool blist::validate() {
	if(root) {
		int	L;

		if(!root->get_pathlength(L)) {
			return false; } }
	return true; }

// debug
Cstring VVVdoc(Node *m) {
	Cstring temp;
	temp << m->get_i_cons().get_content();
	return temp; }

// debug
Node *VVVnext(blist *L, Node *new_node) {
	int	f;
	Node	*current_node = L->root;
	Node	*last_later_node = NULL;

	if(! current_node) return NULL;
	do {
		// if(!current_node->list) {
		// 	return NULL; }
		if ((f = L->cmp(new_node, current_node)) < 0) {
			last_later_node = current_node;
			current_node = current_node->get_left(); }
		else if(! f) {
			if(current_node != new_node)
				return current_node;
			else
				current_node = current_node->get_right(); }
		else
			current_node = current_node->get_right();
		} while(current_node);
	return last_later_node; }

// debug
bool VVVcheckTheTree(blist *L) {
	return true; }

	/*

	IMPORTANT NOTE:
	---------------

	It would seem that the following function should call
	List::insert_node(Node *, Node *) to insert the new node
	at the proper position.

	The reason this is NOT done is that it often happens that
	one wants to (i) search a blist efficiently and (ii) maintain
	the (unsorted) order in which the nodes were inserted.

	If you want to insert a whole bunch of Nodes and end up
	with a sorted blist, call blist::order() to force agreement
	between the linked list and the btree.

	*/

Node *blist::insert_binary_node(Node *& N) {
	long		n, i, f;
	int		dir, op_dir;
	Node		*current_node, *new_node = N,
			*last_earlier_node = NULL;
	valueHolder	*save_new_cons;
	Node		*path[MAXDEPTH];
	int		p_direction[MAXDEPTH];

	// Always do this first because we need to have a clean slate before
	// detecting synchronization problems (Time_nodes only)
	if(N->list) N->list->remove_node(N);

	// AVL 2 TREE ALGORITHM (see e. g. Knuth)

	if(cmp.compares_times()) {
		((Time_node *) N)->actual_time.theTime =
			((Time_node *) N)->getetime();
	}
	if(!root) {
		new_node->make_root_of(this);
		root->reset_links();
		N = last_earlier_node;
		// DONE (nothing to do)
		return root;
	}

	i = 0L;
	current_node = root;

	/* The following is not really appropriate:
		r_cons[0] = 0;
		l_cons[0] = 0;
	What we want to do instead is 2 things:
		- update the left and right consumptions as
		  we change the topology of the btree
		- update the path to the current node at the
		  very end so as to include its contribution
		  to all the relevant consumption values.
	*/

	do {
		if(!current_node->list) {
			cerr << "blist::insert_binary_node: node at "
				<< (void *) this
				<< " has NULL list pointer" << endl;
			// DONE (error)
			return NULL;
		}
		path[i] = current_node;
		if ((f = cmp(new_node, current_node)) < 0) {
			/* See above comment; we'll do things differently.
			r_cons[i] +=	current_node->get_i_cons()
						+ current_node->get_r_cons();
			*/
			current_node = current_node->get_left();
			p_direction[i++] = 0;
		} else if(! f) {
			// node already exists
			Cstring msg;
			msg << "blist::insert_binary_node: attempting to "
				"insert node " << new_node->get_key()
				<< " @ " << ((void *) new_node)
				<< " into a spot already taken by "
				<< current_node->get_key() << " @ "
				<< ((void *) current_node) << "...\n";
			DBG_NOINDENT("blist::insert_binary_node: attempting to "
				"insert node " << new_node->get_key()
				<< " @ " << ((void *) new_node)
				<< " into a spot already taken by "
				<< current_node->get_key() << " @ "
				<< ((void *) current_node) << "...\n");
			
			// DONE (error)
			throw(eval_error(msg));
		} else {
			last_earlier_node = current_node;
			/* See above comment; we'll do things differently.
			l_cons[i] +=	current_node->get_i_cons()
						+ current_node->get_l_cons();
			*/
			current_node = current_node->get_right();
			p_direction[i++] = 1;
		}
	} while(current_node);
	N = last_earlier_node;

	// We know the path ends with a leaf. Attach the new node to it.
	path[i--] = new_node;

	new_node->reset_links();
	/* change in topology, but for the time being we 'pretend' that
	 * the current node consumes 0; we will adjust consumption values
	 * at the very end.  */
	save_new_cons = new_node->get_i_cons().copy();
	new_node->set_i_cons(NULL);
	path[i]->attach(p_direction[i], new_node);

	// n is the number of items in the path to the new node

	n = i + 2;

	// look for a focal point

	f = n - 1;
	while(--f >= 0 && path[f]->indicator == 2);

	// path[f] is the focal point

	// CASE 1

	if(f < 0) {	// focal point is above the root
		// no change in topology, but new node needs to be accounted for
		// in all nodes in the path that leads to it
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		while(++f < n - 1) {
			path[f]->indicator = p_direction[f];
		}
		new_node->add_cons_to_ancestors();
#ifdef VVVdebug
		// debug
		if(VVVdebug) {
		(*VVVdebug) << "CASE 1\n";
		if(!VVVcheckTheTree(this)) {
			VVVproblemFound = true;
			cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
			delete VVVdebug;
			VVVdebug = NULL; }
#endif /* VVVdebug */
		// DONE
		return new_node;
	}

	// CASE 2

	if(path[f]->indicator != p_direction[f]) {
		// no change in topology, but new node needs to be accounted for
		// in all nodes in the path that leads to it

		path[f]->indicator = 2;
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		while(++f < n - 1) {
			path[f]->indicator = p_direction[f];
		}
		new_node->add_cons_to_ancestors();
		// DONE
#ifdef VVVdebug
		// debug
		if(VVVdebug) {
		(*VVVdebug) << "CASE 2\n";
		if(!VVVcheckTheTree(this)) {
			VVVproblemFound = true;
			cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
			delete VVVdebug;
			VVVdebug = NULL;
		}
#endif /* VVVdebug */
		return new_node;
	}

	/*CASE 3

	       f - 1					f - 1 
	      /						  |
	     /						  |
	    /						  |
	    f			===>		    f	  |
	   / \					   /|\	  |
	  /   \ heavy				  / | \	  |
	 /     \				 /  |  \  |
	A	f + 1				A   |   f + 1
		/  \				    |      \
	       /    \				    |       \
	      /      \				    |        \
	     B        f + 2			    B	    f + 2

	*/

	dir = p_direction[f];
	op_dir = 1 - dir;

	if(dir == p_direction[f + 1]) {
		path[f]->attach(dir, path[f + 1]->get_link(op_dir), TreeDir::UPDATE_CONS);
		path[f + 1]->attach(op_dir, path[f], TreeDir::UPDATE_CONS);

		// BUG: n - 1 here used to be n
		for(i = f + 2; i < n - 1; i++) {
			path[i]->indicator = p_direction[i];
		}

		path[f]->indicator = 2;

		if(!f) {
			path[1]->make_root_of(this);
			new_node->set_i_cons(save_new_cons);
			delete save_new_cons;
			new_node->add_cons_to_ancestors();
			// DONE
#ifdef VVVdebug
			// debug
			if(VVVdebug) {
			(*VVVdebug) << "CASE 3(a)\n";
			if(!VVVcheckTheTree(this)) {
				VVVproblemFound = true;
				cout << VVVdebug->str();
			}
			// else {
			// 	dump(VVVdebug);
			// 	cout << VVVdebug->str();
			// 	cout << "  --> OK so far <--\n"; }
			delete VVVdebug;
			VVVdebug = NULL;
			}
#endif /* VVVdebug */
			return new_node;
		} else {
			/* do not update consumption (didn't change -- remember
			 * the current node is assumed to have zero consumption */
			path[f - 1]->attach(p_direction[f - 1], path[f + 1]);
			new_node->set_i_cons(save_new_cons);
			delete save_new_cons;
			new_node->add_cons_to_ancestors();
			// DONE
#ifdef VVVdebug
			// debug
			if(VVVdebug) {
			(*VVVdebug) << "CASE 3(b)\n";
		if(!VVVcheckTheTree(this)) {
			VVVproblemFound = true;
			cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
			delete VVVdebug;
			VVVdebug = NULL; }
#endif /* VVVdebug */
			return new_node;
		}
	}

	/*MODIFIED CASES 4 AND 5

		f - 1				f - 1
		 /				  |
		/				  |
	       /		===>		  |
	      f				f_	  |
	       \ heavy			  \	  |
		\			   \	  |
		 \			    \	  |
		f + 1			     \	  |    ___f + 1
		 /			      \   |   /
		/			       \  |  /
	       /				\ | /
	    f + 2				f + 2

	*/
	if((f + 3) == n) {
		path[f]->indicator = 2;
		path[f]->attach(dir, NULL, TreeDir::UPDATE_CONS);
		path[f + 1]->attach(op_dir, NULL, TreeDir::UPDATE_CONS);
		path[f + 2]->attach(dir, path[f + 1], TreeDir::UPDATE_CONS);
		path[f + 2]->attach(op_dir, path[f], TreeDir::UPDATE_CONS);
		if(!f) {
			path[f + 2]->make_root_of(this);
			new_node->set_i_cons(save_new_cons);
			delete save_new_cons;
			new_node->add_cons_to_ancestors();
			// DONE
#ifdef VVVdebug
			// debug
			if(VVVdebug) {
				(*VVVdebug) << "MOD CASE 4, 5(a)\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str();
				}
				// else {
				// 	dump(VVVdebug);
				// 	cout << VVVdebug->str();
				// 	cout << "  --> OK so far <--\n";
				// }
				delete VVVdebug;
				VVVdebug = NULL;
			}
#endif /* VVVdebug */
			return new_node;
		}
		// no adjustment
		path[f - 1]->attach(p_direction[f - 1], path[f + 2]);
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		new_node->add_cons_to_ancestors();
		// DONE
#ifdef VVVdebug
		// debug
		if(VVVdebug) {
			(*VVVdebug) << "MOD CASE 4, 5(b)\n";
			// dump(VVVdebug);
		if(!VVVcheckTheTree(this)) {
			VVVproblemFound = true;
			cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
			delete VVVdebug;
			VVVdebug = NULL;
		}
#endif /* VVVdebug */
		return new_node;
	}

	/*CASE 4

	      f - 1				    f - 1
	       /				      |
	      /				  	      |
	     /				  	      |
	    f				 f____ 	      |
	   /  \ heavy			/ \   \	      |
	  /    \		===>   /   |   \      |
	 /	\		      /    |	\     |
	A	f + 1 (B)	     A	   |	 \    |    ___f + 1
		 /			   |      \   |   /     /
		/			   |       \  |  /     /
	       /			   |	    \ | /     /
	    f + 2 (B)			   |	    f + 2    /
	   /    \			   \		    /
	  /      \			    \		   /
	 /        \			     \	          /
	B	 f + 3			      B	        f + 3
	*/

	if(p_direction[f + 2] == dir) {
		path[f]->indicator = op_dir;
		path[f]->attach(dir, path[f + 2]->get_link(op_dir), TreeDir::UPDATE_CONS);
#ifdef VVVdebug
		if(VVVdebug) {
			(*VVVdebug) << "CASE 4\n";
		}
#endif /* VVVdebug */
		path[f + 2]->attach(op_dir, path[f], TreeDir::UPDATE_CONS);
#ifdef VVVdebug
		if(VVVdebug) {
			(*VVVdebug) << "  path[f+2] index after attaching f: "
				<< path[f+2]->get_l_cons().get_content() << "\n";
			(*VVVdebug) << "  path[f+3] index before attaching it to f+1: "
				<< path[f+3]->get_i_cons().get_content() << "\n";
		}
#endif /* VVVdebug */
		path[f + 1]->attach(op_dir, path[f + 3], TreeDir::UPDATE_CONS);
#ifdef VVVdebug
		if(VVVdebug) {
			(*VVVdebug) << "  path[f+1] index after attaching f+3: "
				<< path[f+1]->get_l_cons().get_content() << "\n";
			(*VVVdebug) << "  path[f+1] right index after attaching f+3: "
				<< path[f+1]->get_r_cons().get_content() << "\n";
		}
#endif /* VVVdebug */
		path[f + 2]->attach(dir, path[f + 1], TreeDir::UPDATE_CONS);
#ifdef VVVdebug
		if(VVVdebug) {
			(*VVVdebug) << "  path[f+2] index after attaching f+1: "
				<< path[f+2]->get_l_cons().get_content() << "\n";
		}
#endif /* VVVdebug */
		for(i = f + 3; i < n - 1; i++) {
			path[i]->indicator = p_direction[i];
		}
		if(!f) {
			path[f + 2]->make_root_of(this);
			new_node->set_i_cons(save_new_cons);
			delete save_new_cons;
			new_node->add_cons_to_ancestors();
			// DONE
#ifdef VVVdebug
			// debug
			if(VVVdebug) {
				(*VVVdebug) << "CASE 4(a)\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str();
				}
				// else {
				// 	dump(VVVdebug);
				// 	cout << VVVdebug->str();
				// 	cout << "  --> OK so far <--\n";
				// }
				delete VVVdebug;
				VVVdebug = NULL;
			}
#endif /* VVVdebug */
			return new_node;
		}
		// no update necessary
		// hmm... maybe...
		// path[f - 1]->attach(p_direction[f - 1], path[f + 2]);
		path[f - 1]->attach(p_direction[f - 1], path[f + 2], TreeDir::UPDATE_CONS);
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		new_node->add_cons_to_ancestors();
		// DONE
#ifdef VVVdebug
		// debug
		if(VVVdebug) {
			(*VVVdebug) << "CASE 4(b)\n";
			// dump(VVVdebug);
			if(!VVVcheckTheTree(this)) {
				VVVproblemFound = true;
				cout << VVVdebug->str(); }
				// else {
				// 	dump(VVVdebug);
				// 	cout << VVVdebug->str();
				// 	cout << "  --> OK so far <--\n";
				// }
				delete VVVdebug;
				VVVdebug = NULL;
		}
#endif /* VVVdebug */
		return new_node;
	}
	/*CASE 5

	      f - 1				    f - 1
	       /				      |
	      /				  	      |
	     /				  	      |
	    f				 f____ 	      |
	   /  \ heavy			/ \   \	      |
	  /    \		===>   /   |   \      |
	 /	\		      /    |	\     |
	A	f + 1 (B)	     A	   |	 \    |    ___f + 1
		 / \			   |      \   |   /     / \
		/   \			   |       \  |  /     /   \ (heavy)
	       /    ...			   |	    \ | /     /     \
	    f + 2 (B)			   |	    f + 2    /      ...
	   /    \			   \		    /
   op_dir /      \ dir			    \		   /
	 /        \			     \	          /
	f + 3	   B			    f + 3        B
	*/

	path[f]->indicator = 2;
	path[f + 1]->indicator = dir;
	path[f]->attach(dir, path[f + 3], TreeDir::UPDATE_CONS);
	path[f + 2]->attach(op_dir, path[f], TreeDir::UPDATE_CONS);
	path[f + 1]->attach(op_dir, path[f + 2]->get_link(dir), TreeDir::UPDATE_CONS);
	path[f + 2]->attach(dir, path[f + 1], TreeDir::UPDATE_CONS);
	for(i = f + 3; i < n - 1; i++)
		path[i]->indicator = p_direction[i];
	if(!f) {
		path[f + 2]->make_root_of(this);
		new_node->set_i_cons(save_new_cons);
		delete save_new_cons;
		new_node->add_cons_to_ancestors();
		// DONE
#ifdef VVVdebug
		// debug
		if(VVVdebug) {
			(*VVVdebug) << "CASE 5\n";
			if(!VVVcheckTheTree(this)) {
				VVVproblemFound = true;
				cout << VVVdebug->str();
			}
			// else {
			// 	dump(VVVdebug);
			// 	cout << VVVdebug->str();
			// 	cout << "  --> OK so far <--\n";
			// }
			delete VVVdebug;
			VVVdebug = NULL;
		}
#endif /* VVVdebug */
		return new_node;
	}
	// no update necessary
	path[f - 1]->attach(p_direction[f - 1], path[f + 2]);
	new_node->set_i_cons(save_new_cons);
	delete save_new_cons;
	new_node->add_cons_to_ancestors();
	// DONE
#ifdef VVVdebug
	// debug
		if(VVVdebug) {
			(*VVVdebug) << "CASE 6\n";
			if(!VVVcheckTheTree(this)) {
				VVVproblemFound = true;
				cout << VVVdebug->str();
			}
			// else {
			// 	dump(VVVdebug);
			// 	cout << VVVdebug->str();
			// 	cout << "  --> OK so far <--\n";
			// }
			delete VVVdebug;
			VVVdebug = NULL;
		}
#endif /* VVVdebug */
	return new_node;
}

bool	check_verbose = 0;

class node_remover {
public:
	List		*L;
	Node		*N;
	node_remover(List *l, Node *n) : L(l), N(n) {}
	// Last condition inside if() was added because exceptions thrown
	// while executing action_callbacks can leave a Node in a somewhat
	// unstable state.
	~node_remover() { if(N && L && N->list == L) L->List::remove_node(N); }
	Node		*remove() {
			List *l = L;
			L = NULL;
			return l->List::remove_node(N); }
	};

Node *blist::remove_node(Node *N) {
	node_remover	nr(this, N);
	long		i, n, f, i_old;
	unsigned long	heavy_direction, search_dir, P, Q;
	Node		*old_node = N,
			*current_node, *end_node, *save,
			*a, *b, *newnode, *q_link_f, *z;
	Node		*path[MAXDEPTH];
	unsigned long	p_direction[MAXDEPTH];

	if(!enabled) {
		if((current_node = nr.remove())) {
			current_node->reset_links(); }
		if(!get_length()) root = NULL;
		return current_node; }

	// PHASE 1: SEARCH

	if(!root)
		return NULL;
	i = 0;
	current_node = root;
	do {
		if(!current_node->list) {
			cerr << "blist::remove_node: node at " << (void *) this
				<< " has NULL list pointer" << endl;
			return NULL; }
		path[i] = current_node;
		if((f = cmp(old_node, current_node)) < 0) {
			current_node = current_node->get_left();
			p_direction[i++] = 0; }
		else if(!f)
			break;
		else {
			current_node = current_node->get_right();
			p_direction[i++] = 1; }
		} while(current_node);

	/* change sign of incremental consumption and update
	 * ancestors. This will cancel the effect of the node
	 * being removed. */
	old_node->subtract_cons_from_ancestors(NULL);

	if((!current_node) || (current_node != old_node)) {
		int		k = 0;
		Node		*n = first_node();
		stringstream	cc;

		cc << "Bad news: can't seem to remove node from binary tree. "
			"'old' node @" << ((void *) N) << " : " << N->get_key() << "\n";
		cc << "Complete list:\n";
		while(n) {
			cc << k++ << " @" << ((void *) n) << " : " << n->get_key();
			if(n == root) {
				cc << " (root)\n";
			} else if(n == current_node) {
				cc << " (best guess)\n";
			} else {
				cc << "\n";
			}
			if(n->get_left()) {
				cc << "    left: "
				   << n->get_left()->get_key()
				   << "\n";
			} else {
				cc << "    left: NULL\n";
			}
			if(n->get_right()) {
				cc << "    right: "
				   << n->get_right()->get_key()
				   << "\n";
			} else {
				cc << "    right: NULL\n";
			}
			if(N == n) {
				cc << "This one matches!\n";
			}
			n = n->next_node();
		}

		cc << "Path:\n";
		for(k = 0; k < i; k++) {
			cc << "# " << (k + 1) << ": " << path[k]->get_key()
			   << "\n";
		}
		/* this is pretty bad... the node doesn't belong to the list!?
		 * Try to remove it from the dumb underlying list anyway, maybe
		 * we'll learn something...
		 * List::remove_node(N);
		 */

		throw(eval_error(cc.str().c_str()));
	}

	/* DON'T FORGET TO REMOVE THE NODE: handled by the node_remover now.
	 * List::remove_node(current_node);
	 */

	// PHASE 2: SWAP

	i_old = i;
	if((!current_node->get_left()) && (!current_node->get_right())) {
		// the old_node is a leaf; no swap necessary.
		if(!i) {
			root = NULL;
#ifdef VVVdebug
			// debug
			nr.remove();
			if(VVVdebug) {
				(*VVVdebug) << "DONE removing (1)...\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
				delete VVVdebug;
				VVVdebug = NULL; }
#endif /* VVVdebug */
			return old_node; }
		// delete the leaf; note that we don't touch the indicator.
		// No need to update: current node has zero incremental consumption now.
		path[i - 1]->attach(p_direction[i - 1], NULL);
		n = i - 1; }
	else {
		Node *other;

		// swap the node to be deleted with a leaf

		if(	(current_node->indicator == 0)
			|| (current_node->indicator == 2))
			heavy_direction = 0;
		else
			heavy_direction = 1;
		search_dir = 1 - heavy_direction;

		// move once in the heavy direction

		current_node = current_node->get_link(heavy_direction);
		p_direction[i] = heavy_direction;
		path[++i] = current_node;

		// then move in the direction that's not heavy

		current_node = current_node->get_link(search_dir);

		if(current_node) {

			// swap sub-case (1) (nominal: found a branch containing swap candidate)

			do {
				p_direction[i] = search_dir;
				path[++i] = current_node;
				current_node =
					current_node->get_link(search_dir); }
			while(current_node);

			// now see whether there is another node hanging from
			// the last one in the path

			end_node = path[i];
			// disconnect the endnode
			end_node->subtract_cons_from_ancestors(path[i_old]);
			// no need to adjust consumption further; we've done exactly the right thing
			path[i - 1]->attach(
					search_dir,
					(a = end_node->get_link(heavy_direction)));
			if(!a) {
				n = i - 1; }
			else {
				n = i;
				path[n] = a;
				p_direction[n] = search_dir;
				path[n]->indicator = search_dir; }
				// that's where the deleted node would have been

			// now we swap path[i_old] with the end_node

			save = path[i_old];
			path[i_old] = end_node;
			// adjust the end node itself; everything else is OK
			end_node->attach_left(save->get_left(), TreeDir::UPDATE_CONS);
			end_node->attach_right(save->get_right(), TreeDir::UPDATE_CONS);
			end_node->indicator = save->indicator; }
		else if((other = (a = path[i])->get_link(heavy_direction))) {

			/* swap sub-case (2) (branch that was to contain
			 * swap candidate extends in the wrong direction) */

			/* The situation: we were able to move once (in the heavy
			 * direction) but there is no node in the search direction.
			 * We cheat a little bit and rearrange the tree in a way that
			 * does NOT respect the order... This does not matter,
			 * because we will remove the node at i_old shortly
			 * anyway. */

/*
				i_old-1			i_old-1
				   \			   \
				    \			    \
				     \			     \
				    i_old	==>	      a
				    /   \		     / \
			   (heavy) /     \		    /   \ 
				  /       \		   /     \
			    a = path[i]	   b  		other	  b
				/	  / \		  \	 / \
			heavy  /	 ... \		   \	/   \
			      /		     ...	    \  ...   \
			     other			  i_old      ...
*/

			// we mimic deleting path[i_old] in its new position
			// NOTE: only a needs an updated consumption value
			a->attach(	search_dir,
					(b = old_node->get_link(search_dir)),
					TreeDir::UPDATE_CONS);
			a->indicator = old_node->indicator;
			other->indicator = search_dir;
			path[i_old] = a;
			p_direction[i_old] = heavy_direction;
			path[i] = other;
			p_direction[i] = search_dir;
			n = i;
			end_node = a; }
		else if((b = old_node->get_link(search_dir))) {

			// swap sub-case (3) (must swap with short stub)

			// we cheat a little bit by pretending that old_node
			// was < a ...
			// we mimic deleting path[i_old] in its new position

			// NOTE: only a needs an updated consumption value
			a->attach(
				search_dir,
				(b = old_node->get_link(search_dir)),
				TreeDir::UPDATE_CONS);
			path[i_old] = a;
			p_direction[i_old] = heavy_direction;
			n = i_old;
			end_node = a; }
		else {

			/* swap sub-case (4) (old node has single node under
			 * it in the heavy direction). No need to adjust
			 * consumptions: we are not changing topology. */

			path[i_old] = a;
			p_direction[i_old] = search_dir;
			a->indicator = search_dir;
			n = i_old;
			end_node = a; }

		/* DONE so far with consumption adjustment. No need to adjust
		 * path[i_old-1] since we've already cancelled the effect of
		 * the node to be deleted. */

		// attach the leaf we swapped with the to-be-deleted node to the upstream node
		if(! i_old) {
			end_node->make_root_of(this); }
		else {
			path[i_old - 1]->attach(
					p_direction[i_old - 1],
					end_node); } }

	// now we have to adjust the tree

	// step 1: initial adjustment

	if(path[n]->indicator == 2) {
		// case 1
		path[n]->indicator = 1 - p_direction[n];
#ifdef VVVdebug
			// debug
			nr.remove();
			if(VVVdebug) {
				(*VVVdebug) << "DONE removing (2)...\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
				delete VVVdebug;
				VVVdebug = NULL; }
#endif /* VVVdebug */
		// DONE
		return old_node; }
	else if(path[n]->indicator == p_direction[n]) {
		// case 2
		path[n]->indicator = 2;
		f = n - 1;
		if(f < 0) {
#ifdef VVVdebug
			// debug
			nr.remove();
			if(VVVdebug) {
				(*VVVdebug) << "DONE removing (3)...\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
				delete VVVdebug;
				VVVdebug = NULL; }
#endif /* VVVdebug */
			// DONE
			return old_node; }
		// else go to step 2
		}
	else {
		/* case 3: path[n] is heavy in the q_direction. Since the
		 * deleted node is a leaf, a = path[n]->links[q_direction]
		 * has one descendant in the p_direction of path[n] (situation A),
		 * one descendant in the q-direction of path[n] (situation B),
		 * or two descendants (situation C).
		 */
		P = p_direction[n];
		Q = 1 - P;
		a = path[n]->get_link(Q);
		if(a->indicator == P) // situation A
			{
			newnode = a->get_link(P);
			path[n]->attach(Q, NULL, TreeDir::UPDATE_CONS);
			path[n]->indicator = 2;
			a->attach(P, NULL, TreeDir::UPDATE_CONS);
			a->indicator = 2;
			newnode->attach(P, path[n], TreeDir::UPDATE_CONS);
			newnode->attach(Q, a, TreeDir::UPDATE_CONS);
			// DONE so far
			}
		else if(a->indicator == Q) // situation B
			{
			newnode = a;
			path[n]->attach(Q, NULL, TreeDir::UPDATE_CONS);
			path[n]->indicator = 2;
			newnode->attach(P, path[n], TreeDir::UPDATE_CONS);
			newnode->indicator = 2;
			// DONE so far
			}
		else // situation C
			{

/*

			path[n]					a
			/     \				       / \
		   P   /       \ Q (heavy)	     (heavy)  /   \
		      /         \			     /     \
		(deleted)	 a		==>	    b	  other
				/ \			   /
			     P /   \ Q		  (heavy) /
			      /     \			 /
			     b       other	       path[n]

*/
			b = a->get_link(P);
			path[n]->attach(Q, NULL, TreeDir::UPDATE_CONS);
			path[n]->indicator = 2;
			b->attach(P, path[n], TreeDir::UPDATE_CONS);
			b->indicator = P;
			a->attach(P, b, TreeDir::UPDATE_CONS);
			a->indicator = P;
			if(!n) {
				a->make_root_of(this);
#ifdef VVVdebug
			// debug
			nr.remove();
			if(VVVdebug) {
				(*VVVdebug) << "DONE removing (4)...\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
				delete VVVdebug;
				VVVdebug = NULL; }
#endif /* VVVdebug */
				// DONE
				return old_node; }
			// no adjustment needed
			path[n - 1]->attach(p_direction[n - 1], a);
#ifdef VVVdebug
			// debug
			nr.remove();
			if(VVVdebug) {
				(*VVVdebug) << "DONE removing (5)...\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
				delete VVVdebug;
				VVVdebug = NULL; }
#endif /* VVVdebug */
			// DONE
			return old_node; }
		// We only get here in situations A and B, both of which define newnode
		if(!n) {
			newnode->make_root_of(this);
#ifdef VVVdebug
			// debug
			nr.remove();
			if(VVVdebug) {
				(*VVVdebug) << "DONE removing (6)...\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
				delete VVVdebug;
				VVVdebug = NULL; }
#endif /* VVVdebug */
			// DONE
			return old_node; }
		f = n - 1;
		// no adjustment needed
		path[f]->attach(p_direction[n - 1], newnode); }
	// DONE so far

	// step 2: search for a prime position

	while(1) {
		if(f < 0) {
#ifdef VVVdebug
			// debug
			nr.remove();
			if(VVVdebug) {
				(*VVVdebug) << "DONE removing (7)...\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
				delete VVVdebug;
				VVVdebug = NULL; }
#endif /* VVVdebug */
			// DONE
			return old_node; }
		if(path[f]->indicator == 2) {
				// case 1: we are removing the node from
				//	   the branch in the p-direction;
				//	   just change the indicator
			path[f]->indicator = 1 - p_direction[f];
			// We can return, because the weight of the
			// node at position f has not changed w.r.t. the
			// situation prior to deleting the node.
#ifdef VVVdebug
			// debug
			nr.remove();
			if(VVVdebug) {
				(*VVVdebug) << "DONE removing (8)...\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
				delete VVVdebug;
				VVVdebug = NULL; }
#endif /* VVVdebug */
			// DONE
			return old_node; }

		else if(path[f]->indicator == p_direction[f]) {
				// case 2: even better; the p-branch was
				//	   heavy, so now it's balanced!

			// NOTE: we can't return yet, because we are
			// changing the weight of path[f] and the topology
			// of upstream nodes may have to be modified.
			path[f--]->indicator = 2; }

		else {	
			unsigned long temp_dir = 1 - p_direction[f];
				// case 3: darn, not so easy. We want to
				//	   remove a fruit from a light branch
				//	   opposite a heavier one.

			// step 3: prime adjustment

			q_link_f = path[f]->get_link(temp_dir);
			if(q_link_f->indicator == p_direction[f]) {
				// case a: the first node down the heavy (q)
				// 	   branch leans the same way as the
				//	   p-branch. Rotate recursively.

				path[f]->attach(
					temp_dir,
					// subtree consumption is the same; no adjustment
					q_link_f->rotate());
			/* Note that we don't do anything here; we just loop
			 * again with the same [f]. Since we rotated the tree,
			 * we are guaranteed to fall into a different case at
			 * the next iteration. */
				}
			else {
				// case b: the first node down the heavy (q)
				//	   branch leans in the q direction.
				if(!f) {
					q_link_f->make_root_of(this); }
				else {
					// no adjustment necessary
					path[f - 1]->attach(
						p_direction[f - 1],
						q_link_f); }
				z = q_link_f->get_link(p_direction[f]);
				path[f]->attach(1 - p_direction[f], z, TreeDir::UPDATE_CONS);
				q_link_f->attach(p_direction[f], path[f], TreeDir::UPDATE_CONS);
				// DONE with updates
				if(q_link_f->indicator != 2) {
					path[f]->indicator = 2;
					q_link_f->indicator = 2;
					f--; }
				else {
					path[f]->indicator = 1 - p_direction[f];
					q_link_f->indicator = p_direction[f];
#ifdef VVVdebug
			// debug
			nr.remove();
			if(VVVdebug) {
				(*VVVdebug) << "DONE removing (9)...\n";
				if(!VVVcheckTheTree(this)) {
					VVVproblemFound = true;
					cout << VVVdebug->str(); }
		// else {
		// 	dump(VVVdebug);
		// 	cout << VVVdebug->str();
		// 	cout << "  --> OK so far <--\n"; }
				delete VVVdebug;
				VVVdebug = NULL; }
#endif /* VVVdebug */
					return old_node; } } } } }

	/* The purpose of this function is to transform this into a Node
	 * that is either balanced or heavy in the (1 - indicator) direction.
	 * The function returns NULL if it is ever called on a balanced Node,
	 * for which there is nothing to do. */
Node *Node::rotate() {
	unsigned long	t_direction, u_direction;
	Node	*new_root;

	if(indicator == 2) {
		return NULL; }
	t_direction = indicator;
	u_direction = 1 - t_direction;

	new_root = get_link(t_direction);
	if(new_root->indicator == u_direction) {
		new_root = new_root->rotate();
		attach(t_direction, new_root, TreeDir::UPDATE_CONS); }
	if(new_root->indicator == 2)
		indicator = 2;
	else
		indicator = u_direction;
	new_root->indicator = u_direction;
	attach(t_direction, new_root->get_link(u_direction), TreeDir::UPDATE_CONS);
	new_root->attach(u_direction, this, TreeDir::UPDATE_CONS);
	return new_root; }

Node *blist::v_earliest_node(v_blist &bl, int &score) const {
	Node			*N = root, *M;
	unsigned int		_k;

	score = 10;
	if(!root) {
	       	return NULL; }

	_k = void_to_uint(root);
	bl << new v_int_node(_k);
	while((M = N->get_left())) {
		if(bl.find(_k = void_to_uint(M))) {
			score = 0;
			return NULL; }
		bl << new v_int_node(_k);
		N = M; }
	return N; }

// does NOT assume that the list is ordered (links do NOT
// have to agree with previous/next pointers)
Node *blist::v_find_earliest_after(
			Node *new_node,
			v_blist &bl,
			int &score) const {
	int			f;
	Node			*current_node = root;
	Node			*last_later_node = NULL;
	unsigned int		_k;

	score = 10;
	if(! current_node) return NULL;
	do {
		if(!current_node->list) {
			cerr << "v_find_earliest_after: NULL list for node at "
				<< void_to_uint(current_node)
				<< " referenced in list at "
				<< void_to_uint(this) << endl; }
		if(bl.find(_k = void_to_uint(current_node))) {
			score = 0;
			return NULL; }
		bl << new v_int_node(_k);
		if ((f = cmp(new_node, current_node)) < 0) {
			last_later_node = current_node;
			current_node = current_node->get_left(); }
		else if(! f) {
			if(current_node != new_node)
				return current_node;
			else
				current_node = current_node->get_right(); }
		else
			current_node = current_node->get_right();
		} while(current_node);
	return last_later_node; }

bool blist::check() {
	v_blist		L(v_compare_v_int_nodes),
			L1(v_compare_v_int_nodes),
			L2(v_compare_v_int_nodes);
	int		theScore = 0;
	Node		*N = v_earliest_node(L, theScore);
	unsigned int	_k = void_to_uint(N);
	Node		*n = first_node();
	int		null_count = 0;
	int		foreign_count = 0;
	int		k;

	while(n) {
		if(L2.find(void_to_uint(n))) {
			cerr << "checking list at " << void_to_uint(this)
				<< ": link recursion.\n";
			return false; }
		L2 << new v_int_node(void_to_uint(n));
		if(n->list != this) {
			null_count++; }
		n = n->next_node(); }
	if(root && !L2.find(void_to_uint(root))) {
		cerr << "Uh-Oh, root is not referenced in the linked "
			"nodes...\n"; }

	if(null_count) {
		cerr << "checking list at " << void_to_uint(this) <<
			": size " << get_length()
			<< ", null_count = " << null_count << endl; }

	n = first_node();
	while(n) {
		if(n->get_left()) {
			if(!L2.find(void_to_uint(n->get_right()))) {
				foreign_count++; } }
		if(n->get_right()) {
			if(!L2.find(void_to_uint(n->get_right()))) {
				foreign_count++; } }
		n = n->next_node(); }
	if(foreign_count) {
		cerr << "checking list at " << void_to_uint(this)
			<< ": foreign_count = " << foreign_count << endl; }
	if(!theScore) {
		return false; }
	k = 0;
	while(N) { 
		if(N->get_left()) {
			if(cmp(N, N->get_left()) <= 0) {
				cerr << "Inconsistent left link from "
					<< N->get_key() << " to "
					<< N->get_left()->get_key() << endl; } }
		if(N->get_right()) {
			if(cmp(N, N->get_right()) >= 0) {
				cerr << "Inconsistent right link from "
					<< N->get_key() << " to "
					<< N->get_right()->get_key()
					<< endl; } }
		L1 << new v_int_node(void_to_uint(N));
		L.clear();
		if(check_verbose) {
			cerr << "node " << k++ << " = " << N->get_key()
				<< endl; }
		N = v_find_earliest_after(N, L, theScore);
		if(!theScore) {
			return false; }
		if(L1.find(_k = void_to_uint(N))) {
			return false; } }
	return true; }

blist::blist(compare_function c)
	      : root(NULL),
		cmp(c),
		enabled(1),
		callbacksEnabled(true) {
	myType = BLIST;
}

	// Debugging tool:
void blist::dump() {
	dump(root, 0); }

	// Debugging tool:
void blist::dump(aoString *o) {
	dump(root, 0, o); }

	// Debugging tool:
void blist::dump(Node *N, int indentation, aoString *o) {
	}

	// Debugging tool:
void blist::dump(Node *N, int indentation) {
	int	i;

	if(!N) {
		return; }
	for(i = 0; i < indentation; i++) {
		cout << " "; }
	cout << N->get_key() << "[" << N->get_index() << "] ";
	if(N->indicator == 0) {
		cout << "L\n"; }
	else if(N->indicator == 1) {
		cout << "R\n"; }
	else if(N->indicator == 2) {
		cout << "B\n"; }
	else {
		cout << "U\n"; }
	indentation += 2;
	dump(N->Links[0], indentation);
	dump(N->Links[1], indentation); }
