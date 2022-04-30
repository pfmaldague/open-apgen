#if HAVE_CONFIG_H
#include <config.h>
#endif

// #define apDEBUG
// #define DEBUG_BLIST
// #define REPORT_ERRORS

#include "apDEBUG.H"

#include <iostream>
#include <iomanip>
#include <stdlib.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

using namespace std;

#define bpdebug

#ifdef DEBUG_BLIST
#define apDEBUG
#endif

#include "C_list.H"
#include "C_string.H"
#include "RES_exceptions.H"

// debug
#include "APdata.H"

extern Cstring		find_list_id_of(_validation_tag *);
extern char		*APcore_title;
extern char		*APcore_build_date;
extern char		*APcore_platform;

const char *List::get_short_version() {
	return VERSION;
}

List::List() : myType(LIST_BASE) {
	head = 0; tail = 0; length = 0;
}

	/*
	 *This constructor copies the list and all of its nodes.  Each node
	 *must have method, copy() defined to create a new like node.
	 */

List::List(const List &list) : myType(LIST_BASE) {
	Node				*node;

	head = 0;
	tail = 0;
	length = 0;
	for(	node = list.first_node();			//for each node on right
		node;
		node = node->next_node()) { 
			insert_node(node->copy());
	}
}

	/*
	 *	This function empties the list.  This method (as well as
	 *	delete_node()) make use of the virtual destructor of class Node.  In
	 *	order for this destructor to work properly, ALL DESCENDANTS OF NODE
	 *	SHOULD HAVE A DESTRUCTOR DECLARED, or else their members' destructors
	 *	will not be invoked.
	 */

void List::clear() {
	Node	*node;
	Node	*next;

	for(node = head; node; node = next) {
		next = node->next_node();
		if (node->list != this) {
			cerr << "Node(" << (void *) node << ")'s list is not correct:\n";
			cerr << "   this List = " << hex << this << "\n";
			cerr << "   node's list = " << hex << (long) node->list << "\n";
			cerr << "   Next = " << hex << (long) node->next << "\n";
			cerr << "   Length = " << hex << length << "\n";
		}
		if(list_type() == LISTOVAL && node->node_type() != ARRAYELEMENT) {
			cerr << "bad array list.\n";
			exit(-1);
		}
		delete node;
	}
	head = tail = 0;
	length = 0;
}

List::~List() {
	clear();
	/* Crashes when owner no longer exists:
	 * LO->Lists.remove_list(n_this); */
	}

	/*
	 *	This function inserts the specified node at the end of the list.
	 */

Node *List::insert_node(Node *node) {
	_validation_tag		*theList;
	_validation_tag		*theNode;

	if(!node) {
		return 0;
	}
	if(node->list) {							//if already in a list
		node->list->remove_node(node);
	}				//remove it
	if(list_type() == LISTOVAL && node->node_type() != ARRAYELEMENT) {
		cerr << "List::insert_node: ListOVal contains non-array-element node(s). Exiting.\n";
		exit(-1);
	}
	node->list = this;
	node->previous = tail;			//new backward link
	node->next = 0;
	if(head) {
		tail->next = node;
	} else {				//new forward link
		head = node;
	}					//new start of list
	tail = node;				//new end of list
	length++;
	return node;
}


	/*
	 *	This function adds new_node to the List immediately after old_node.
	 *	If old_node is 0, then new_node will be inserted at the start of the
	 *	list.
	 */

Node* List::insert_first_node_after_second(Node *new_node , Node *old_node) {
	if(new_node == old_node) {
		return(new_node);
	}
	if(old_node && old_node->list != this) return 0;
	if(!new_node) {
		return 0;
	}
	if(list_type() == LISTOVAL && new_node->node_type() != ARRAYELEMENT) {
		cerr << "List::insert_first_node_after_second: ListOVal contains non-array-element node(s). Exiting.\n";
		exit(-1);
	}
	if(new_node->list) {					//if already in a list
		new_node->list->remove_node(new_node);
	}	//remove it
	new_node->list = this;
	if(old_node == 0) {
		if(head) {
			head->previous = new_node;
		} else {
			tail = new_node;
		}
		new_node->next = head;
		new_node->previous = 0;
		head = new_node;
	} else {
		if (old_node->next) {
			old_node->next->previous = new_node;
		} else {
			tail = new_node;
		}
		new_node->next = old_node->next;
		new_node->previous = old_node;
		old_node->next = new_node;
	}
	length++;
	return new_node;
}

	/*
	 *	This function deletes the specified node from the list and fixes the
	 *	links (see Node::~Node()).  It returns TRUE only if the specified node is on
	 *	the specified list.  See clear().
	 */

bool List::delete_node(Node *node) {
	if (!node || node->list != this) return false;
	delete node;
	return true;
}

	/*
	 *	This function removes a node from the list but does not delete it. 
	 *	It returns the specified node if it is on the specified list.  Otherwise
	 *	it returns 0.
	 */

Node *List::remove_node(Node *node) {
	v_blist			*sync;
	v_int_node		*b;

	if(!node) {
		cerr << "APGEN INTERNAL ERROR -- attempt to remove foreign node.\n";
#		ifdef apDEBUG
		cout << "APGEN INTERNAL ERROR -- attempt to remove foreign node.\n";
#		endif
		return 0;
	}
	if(node->list != this) {
		cerr << "APGEN INTERNAL ERROR -- attempt to remove foreign node; it says its list is at "
			<< void_to_uint(node->list) << ", not this at " << void_to_uint(this)
			<< ".\n";
#		ifdef apDEBUG
		cout << "APGEN INTERNAL ERROR -- attempt to remove foreign node; it says its list is at "
			<< void_to_uint(node->list) << ", not this at " << void_to_uint(this)
			<< ".\n";
#		endif
		return 0;
	}
	if(list_type() == LISTOVAL && node->node_type() != ARRAYELEMENT) {
		cerr << "List::remove_node: ListOVal contains non-array-element node(s). Exiting.\n";
		exit(-1);
	}

	//if not the first node:
 	if (node->previous) {
		//fix forward link
		node->previous->next = node->next;
	} else {
		//new head
		head = node->next;
	}

	//if not last node
	if (node->next) 	{
		//fix backward link
		node->next->previous = node->previous;
	} else {
		//new tail
		tail = node->previous;
	}
	length--;
	node->list = 0;
	node->next = 0;
	node->previous = 0;
	return node;
}

List & List::operator = (const List& list) {
	Node	*node;

	clear();
	for (	node = list.first_node();	//for each node on right
		node;
		node = node->next_node()) {
		insert_node(node->copy());
	}					//insert a copy
	return *this;
}

const char *List::spell_Node_type(Node_type nt) {
	switch(nt) {
		case AAF_FUNCTION:
			return "AAF_FUNCTION";
		case ACT_EXECUTION_CONTEXT:
			return "ACT_EXECUTION_CONTEXT";
		case ACT_REQ:
			return "ACT_REQ";
		case ACTION_REQUEST:
			return "ACTION_REQUEST";
		case ARRAYELEMENT:
			return "ARRAYELEMENT";
		case ASSOC_FLOAT:
			return "ASSOC_FLOAT";
		case BATTRIBUTE_TYPE_NODE:
			return "BATTRIBUTE_TYPE_NODE";
		case BLOCK_NODE:
			return "BLOCK_NODE";
		case BPOINTERNODE:
			return "BPOINTERNODE";
		case BREVERSE_TAG:
			return "BREVERSE_TAG";
		case BSTRINGNODE:
			return "BSTRINGNODE";
		case BSYMBOL_NODE:
			return "BSYMBOLNODE";
		case BTAG:
			return "BTAG";
		case CONCRETE_TIME_NODE:
			return "CONCRETE_TIME_NODE";
		case CONSTRAINT:
			return "CONSTRAINT";
		case CULPRIT_NODE:
			return "CULPRIT_NODE";
		case CULPRIT_THREAD:
			return "CULPRIT_THREAD";
		case CULPRIT_OBJECT:
			return "CULPRIT_OBJECT";
		// case DECLPARAM_INSTRUCTION:
		// 	return "DECLPARAM_INSTRUCTION";
		case DOUBLE_POINTER_NODE:
			return "DOUBLE_POINTER_NODE";
		case EDITNODE:
			return "EDITNODE";
		case EXECUTION_CONTEXT:
			return "EXECUTION_CONTEXT";
		case FUNCTION_EXECUTION_CONTEXT:
			return "FUNCTION_EXECUTION_CONTEXT";
		case GLOBALSYMBOL:
			return "GLOBALSYMBOL";
		case GUISTUFF:
			return "GUISTUFF";
		case INSTRUCTION:
			return "INSTRUCTION";
		case INTERNAL_SYMBOL_LIST:
			return "INTERNAL_SYMBOL_LIST";
		case TIME_NODE:
			return "TIME_NODE";
		case INT_NODE:
			return "INT_NODE";
		case LIST_OBJECT:
			return "LIST_OBJECT";
		case LOCALLISTENER:
			return "LOCALLISTENER";
		case PEF_RECORD:
			return "PEF_RECORD";
		case POINTER_NODE:
			return "POINTER_NODE";
		case POINTER_TO_POINTER:
			return "POINTER_TO_POINTER";
		case REF_NODE:
			return "REF_NODE";
		case RES_ABSTRACT:
			return "RES_ABSTRACT";
		case RES_ASSOCIATIVE:
			return "ASSOCIATIVE";
		case RES_EXECUTION_CONTEXT:
			return "RES_EXECUTION_CONTEXT";
		case RES_STATE:
			return "RES_STATE";
		case RES_CONSUMABLE:
			return "RES_CONSUMABLE";
		case RES_CONTAINER:
			return "RES_CONTAINER";
		case RES_HISTORY:
			return "RES_HISTORY";
		case RES_NONCONSUMABLE:
			return "RES_NONCONSUMABLE";
		case RES_NODE:
			return "RES_NODE";
		case SASF_POINTER:
			return "SASF_POINTER";
		case SEVERITY:
			return "SEVERITY";
		case SIBLING:
			return "SIBLING";
		case SIMPLE_EXECUTION_CONTEXT:
			return "SIMPLE_EXECUTION_CONTEXT";
		case SIMPLE_BSYMBOL_NODE:
			return "SIMPLE_BSYMBOLNODE";
		case STRING_NODE:
			return "STRING_NODE";
		case SYMBOL_NODE:
			return "SYMBOL_NODE";
		case TAG:
			return "TAG";
		case TAGGEDLIST:
			return "TAGGEDLIST";
		case TAGLIST:
			return "TAGLIST";
		case TAGLIST_OBJECT:
			return "TAGLIST_OBJECT";
		case TIME_PTRNODE:
			return "TIME_PTRNODE";
		case TYPEDNODE:
			return "TYPEDNODE";
		case TYPEDSYMBOL:
			return "TYPEDSYMBOL";
		case TIME_EXPNODE:
			return "TIME_EXPNODE";
		case UNKNOWN_EXTENSION:
			return "UNKNOWN_EXTENSION";
		case USAGE_CLAUSE_NODE:
			return "USAGE_CLAUSE_NODE";
		case VALUE_NODE:
			return "VALUE_NODE";
		case VIOLATION_INFO:
			return "VIOLATION_INFO";
		case WIN_NODE:
			return "WIN_NODE";
		default:
			return "Never seen this before!!";
	}
}

void Time_node::get_actual_time(CTime_base &t) {
	t = actual_time.getTheTime();
}

void Node::make_root_of(blist *b) {
	b->root = this;
}

void	Node::attach_left(Node *N, TreeDir::cons_adjust CA) {
	attach(0, N);
	}

void	Node::attach_right(Node *N, TreeDir::cons_adjust CA) {
	attach(1, N);
	}

void	Node::attach(int right_or_left, Node *N, TreeDir::cons_adjust CA) {
	Links[right_or_left] = N;
	}

void synchronize_unattached_node(
		Time_node*		node,
		const CTime_base&	U) {
	assert(!node->list);
	node->actual_time.theTime = U;
}

void Time_node::synchronize_self() {
	List*	l;
	CTime_base T, U;

	get_actual_time(T);
	if(T != (U = getetime())) {
		if((l = list)) {
			l->remove_node(this);
		}
		synchronize_unattached_node(this, U);
		if(l) {
			l->insert_node(this);
		}
	}
}

Node *clist::insert_node(Node *N) {
	Node	*K = N;

	if(insert_binary_node(K)) {
		if(cmp.compfunc == compare_int_nodes) {
			// use K instead; that was the purpose of making the argument of insert_binary_node() a reference:
			// int_node	*M = (int_node *) find_latest_before(N);

			((int_node *) N)->secondary_key = 0;
			if(K) {
				if(((int_node *) K)->_i == ((int_node *) N)->_i)
					((int_node *) N)->secondary_key = ((int_node *) K)->secondary_key + 1;
			}
		}
		else if(cmp.compares_times()) {
			// use K instead; that was the purpose of making the argument of insert_binary_node() a reference:
			// Time_node	*M = (Time_node *) find_latest_before(N);

			((Time_node *) N)->secondary_key.theKey = 0;
			if(K) {
				CTime_base		t1 , t2;

				((Time_node *) K)->get_actual_time(t1);
				((Time_node *) N)->get_actual_time(t2);
				if(t1 == t2) {
					// debug
					// cerr << "Time_node at " << void_to_uint(N)
					//	<< "'s key moved to " << (K->secondary_key + 1) << endl; 
					((Time_node *) N)->secondary_key.theKey =
						((Time_node *) K)->secondary_key.theKey + 1;
				}
			}
		}
		return List::insert_first_node_after_second(N , K);
	}
	return NULL;
}

	/*
	 *	This method removes the nodes from the list specified by the argument
	 *	and attaches them to the end of this list.  "x << x;" does nothing.  N.B.,
	 *	one may write, "a << b << c;", where a, b, and c are all List's.
	 */

List & List::operator << (List & from) {
	Node	*node;

	if (&from == this) return *this;			//"x << x;"
	if (!head) {								//if this is empty
		head = from.head;						//point to start of from
		tail = from.tail;						//point to end of new list
		}
	else if (from.head) {						//if from is not empty
		tail->next = from.head;					//link up last old to first new
		from.head->previous = tail;
		tail = from.tail;						//point to end of new list
		}

	for (node = from.head;						//for each new node
		 node;
		 node = node->next_node()) {
		node->list = this;
		}
	length += from.length;

	from.head = 0;
	from.tail = 0;								//declare from empty
	from.length = 0;

	return *this;
}

/*This function returns a pointer to the first node with given key (as
defined by virtual method, Node::get_key()).  It returns 0 if the key
is not found.*/
Node *List::find(const Cstring& target) const {
	Node	*node;

	for (node = head; node; node = node->next_node()) 
		if (target == node->get_key())
			break;
	return node;
}

Node *List::find(void *P) const {
	Node	*node;

	for (node = head; node; node = node->next_node()) 
		if (P == ((Pointer_node *) node)->get_ptr())
			break;
	return node;
}

/*This function returns a pointer to the first node with given key (as
defined by virtual method, Node::get_key()).  It returns 0 if the key
is not found.*/
Node*List::find(const char *target) const {
	Node	*node;

	for (node = head; node; node = node->next_node()) 
		if (node->get_key() == target)
			break;
	return node;
}

/*This function returns a pointer to the first node with given key (as
**defined by virtual method, Node::get_key()) that follows the list node 
**'after' of this list.  If `after' is NULL the first node of this list is
**used.  It returns NULL if the key is not found.
*/
Node	*List::find_after(const Cstring& target, Node*after) const {
	Node	*node; 

	// make sure that after is a node of this list
	if (after)
		if (after->list == this)
			node = after->next_node();
		else
			return NULL;
	else
		node = head;

	for (; node; node = node->next_node()) 
		if (target == node->get_key())
			break;

	return node;
	}

Node *List::operator[] (const unsigned long i) const {
/*
This operator returns a pointer to the i'th node (i = 0 => first
node).  It returns 0 if i is too large, and the first node if i is zero or negative:
*/
	Node		*node;
	unsigned int	count;

	for (	node = head, count = 0;
		node && count < i;
		// efficiency...
		// node = node->next_node()
		node = node->next
		, count++
		)
		;
	return node;
}

Node *blist::operator()(const unsigned long i) const {
	return List::operator[](i);
}

Node *OwnedCList::insert_node(Node *n) {
	Node		*m;

	if(list_type() == LISTOVAL && n->node_type() != ARRAYELEMENT) {
		cerr << "Attempting to stick a non-array-element in an array. Exiting.\n";
		exit(-1);
	}
	if(!n) {
		return NULL;
	}

	if(n->list) {
		// Do this now so removal takes place before pre-insertion callback is invoked:
		n->list->remove_node(n);
	}
	if(callbacksEnabled) {
		n->action_callback(owner, BEFORE_NODE_INSERTION);
	}
	m = clist::insert_node(n);
	if(callbacksEnabled) {
		n->action_callback(owner, AFTER_NODE_INSERTION);
	}
	// enablePrint = save_print;
	return m;
}

Node *OwnedCList::remove_node(Node *n) {
	Node		*m;
	// int save_print = enablePrint;

	// enablePrint = 1;
	if(callbacksEnabled) {
		n->action_callback(owner , BEFORE_NODE_REMOVAL);
	}
	m = clist::remove_node(n);
	if(callbacksEnabled) {
		n->action_callback(owner , AFTER_NODE_REMOVAL);
	}
	// enablePrint = save_print;
	return m;
}

Node *OwnedCList::remove_node_prior_to_deletion(Node *n) {
	Node		*m;
	// int save_print = enablePrint;

	// enablePrint = 1;

	if(callbacksEnabled) {
		n->action_callback(owner , BEFORE_FINAL_NODE_REMOVAL);
	}
	m = clist::remove_node(n);
	if(callbacksEnabled) {
		n->action_callback(owner , AFTER_FINAL_NODE_REMOVAL);
	}
	// enablePrint = save_print;
	return m;
}


Node *OwnedList::insert_node(Node *n) {
	Node		*m;

	if(list_type() == LISTOVAL && n->node_type() != ARRAYELEMENT) {
		cerr << "Attempting to stick a non-array-element in an array. Exiting.\n";
		exit(-1);
	}
	if(!n) {
		return NULL;
	}

	if(n->list) {
		// Do this now so removal takes place before pre-insertion callback is invoked:
		n->list->remove_node(n);
	}
	if(callbacksEnabled) {
		n->action_callback(owner, BEFORE_NODE_INSERTION);
	}
	m = blist::insert_node(n);
	if(callbacksEnabled) {
		n->action_callback(owner, AFTER_NODE_INSERTION);
	}
	// enablePrint = save_print;
	return m;
}

Node *OwnedList::remove_node(Node *n) {
	Node		*m;
	// int save_print = enablePrint;

	// enablePrint = 1;
	if(callbacksEnabled) {
		n->action_callback(owner , BEFORE_NODE_REMOVAL);
	}
	m = blist::remove_node(n);
	if(callbacksEnabled) {
		n->action_callback(owner , AFTER_NODE_REMOVAL);
	}
	// enablePrint = save_print;
	return m;
}

Node *OwnedList::remove_node_prior_to_deletion(Node *n) {
	Node		*m;
	// int save_print = enablePrint;

	// enablePrint = 1;

	if(callbacksEnabled) {
		n->action_callback(owner , BEFORE_FINAL_NODE_REMOVAL);
	}
	m = blist::remove_node(n);
	if(callbacksEnabled) {
		n->action_callback(owner , AFTER_FINAL_NODE_REMOVAL);
	}
	// enablePrint = save_print;
	return m;
}

long Node::get_index_const() const {
	if(!list) return -1;
	Node *N = list->first_node();
	long i = 0;
	while(N) {
		if(N == this) return i;
		i++;
		N = N->next_node();
	}
	return -1L;
}

long Node::get_index() {
	if(!list) return -1;
	Node *N = list->first_node();
	long i = 0;
	while(N) {
		if(N == this) return i;
		i++;
		N = N->next_node();
	}
	return -1L;
}

Node::~Node() {
	if(list) {
		list->remove_node_prior_to_deletion(this);
		list = NULL;
	}
}

back_pointer::back_pointer()
	: PointerNodes(compare_function(compare_bpointernodes, false))
	{}

back_pointer::~back_pointer() {
	delete_pointers_to_this();
}

void back_pointer::delete_pointers_to_this() {
	double_pointer_node		*N;
	pointer_to_backpointer		*P;
	int				i = 0;

	for(	N = (double_pointer_node *) PointerNodes.first_node();
		N;
		N = (double_pointer_node *) N->next_node()) {
		if((P = N->get_ptr2())) {
			Node		*n = (Node *) N->get_ptr();

			if(P->DP != N) {
				cerr << "back_pointer::~back_pointer(): fatal error; Pointer_node " <<
					(void *) P << " has DP pointer = " <<
					(void *) P->DP << " instead of " << (void *) N << "\n";
				cerr.flush();
			}
			P->DP = NULL;
			delete n;
		}
		i++;
	}
	// inactive_count = 0;
	PointerNodes.clear();
}

void	*Pointer_node::get_ptr() const {
	return pointer;
}

// last arg defaults to 1:
Pointer_node::Pointer_node(void *p , back_pointer *bp)
	: pointer(p),
	BP(bp) {

	// NOTE: p always points to the true start of the object;
	//       for objects which are derived from the back_pointer class,
	//	 BP may be offset by the size of members of other classes
	//	 that the object is derived from!!! I LEARNED THIS THE HARD WAY!!
	if(BP) {
		Node			*this_as_a_node = (Node *) this;

		DP = new double_pointer_node(this , (pointer_to_backpointer *) this);
		BP->PointerNodes << DP;
	}
}

Pointer_node::Pointer_node(const Pointer_node & pn)
	: pointer(pn.pointer),
	BP(pn.BP) {
	_validation_tag			*T;

	// NOTE: p always points to the true start of the object;
	//       for objects which are derived from the back_pointer class,
	//	 bp may be offset by the size of members of other classes
	//	 that the object is derived from!!! I LEARNED THIS THE HARD WAY!!
	if(BP) {
		DP = new double_pointer_node((Node *) this , (pointer_to_backpointer *) this);
		BP->PointerNodes << DP;
	}
}

Pointer_node::~Pointer_node() {
	if(BP && DP) {
		double_pointer_node	*dpn = (double_pointer_node *) BP->PointerNodes.find((void *) this);
		if(dpn != DP) {
			cerr << "Pointer_node::~Pointer_node(): fatal error; Pointer_node " <<
				(void *) pointer << " has DP pointer = " <<
				(void *) dpn << " instead of " << (void *) DP << "\n";
		}
		if(dpn) {
			dpn->pointer2 = NULL;
			delete dpn;
		}
	}
	if(list) {
		list->remove_node(this);
		list = NULL;
	}
}

Node *Pointer_node::copy() {
	return new Pointer_node(pointer , BP);
}

pointer_to_backpointer::pointer_to_backpointer()
	: DP(NULL) {}

pointer_to_backpointer::~pointer_to_backpointer() {}

double_pointer_node::double_pointer_node(Node *a , pointer_to_backpointer *b)
	: bpointernode(a, NULL) , pointer2(b) {}

double_pointer_node::double_pointer_node(const double_pointer_node & D)
	: bpointernode(D) , pointer2(D.pointer2) {}

double_pointer_node::~double_pointer_node() {
	// avoid spurious validation messages:
	if(list) {
		list->remove_node(this);
	}
}

// Node *double_pointer_node::get_ptr1() {
// 	return pointer1; }

pointer_to_backpointer *double_pointer_node::get_ptr2() {
	return pointer2;
}

void double_pointer_node::set_ptr2(pointer_to_backpointer *P) {

	pointer2 = P;
}

const Cstring& double_pointer_node::get_key() const {
	return Cstring::null();
}

const Cstring& Pointer_node::get_key() const {
	return Cstring::null();
}

Symbol_node::Symbol_node(
		const Cstring &nm,
		const Cstring & eq)
	: name(nm), equivalent(eq) {}

Tag::Tag(const Cstring& nm,void*pt) : name(nm),pointer(pt) {}

Node *List_iterator::next() {
	node = node ? node->next_node() : list->first_node();
	return node;
}

Node *List_iterator::previous() {
	node = node ? node->previous_node() : list->last_node(); 
	return node;
}	

Node *blist_iterator::next() {
	node = node ? node->following_node() : first();
	return node;
}

Node *blist_iterator::previous() {
	node = node ? node->preceding_node() : last(); 
	return node;
}	

Node *Node::preceding_node() {
	blist		*L = (blist *) list;

	if(! L) return NULL;
	return L->find_latest_before(this);
}

clist::clist(const clist &B) : blist(B.cmp) {
	operator = (B);
}

Node *clist::find_latest_before(Node *new_node) const {
	if(new_node->is_in_list(*this))
		return new_node->previous_node();
	return blist::find_latest_before(new_node);
}

Node *Node::following_node() {
	blist		*L = (blist *) list;

	if(!L) return NULL;
	return L->find_earliest_after(this);
}

Node *clist::find_earliest_after(Node *new_node) const {
	if(new_node->is_in_list(*this))
		return new_node->next_node();
	return blist::find_earliest_after(new_node);
}

int compare_bstringnodes(const Node *A, const Node *B) {
	return strcmp(*A->get_key(), *B->get_key());
}

int compare_bstringnodes_nocase(const Node *A, const Node *B) {
	return strcasecmp(*A->get_key(), *B->get_key());
}

int compare_bpointernodes(const Node *A , const Node *B) {
	Pointer_node	*a = (bpointernode *) A, *b = (bpointernode *) B;
	unsigned long	i = (unsigned long) a->pointer, j = (unsigned long) b->pointer;

	if(i > j)
		return 1;
	else if(i == j)
		return 0;
	return -1;
}

int compare_Time_nodes(const Node *a , const Node *b) {
	Time_node	*t1 , *t2;
	CTime_base	b1 , b2;

	t1 = (Time_node *) a;
	t2 = (Time_node *) b;
	t1->get_actual_time(b1);
	t2->get_actual_time(b2);
	if(b1 < b2) {
		return -1;
	}
	else if(b1 == b2) {
		if((!t1->has_a_list()) && (t2->has_a_list())) {
			return 1;
		} else if((!t2->has_a_list()) && (t1->has_a_list())) {
			return -1;
		} else if(t1->secondary_key.getTheKey() > t2->secondary_key.getTheKey()) {
			return 1;
		} else if(t1->secondary_key.getTheKey() < t2->secondary_key.getTheKey()) {
			return -1;
		}
		return 0;
	}
	return 1;
}

int compare_int_nodes(const Node *a , const Node *b) {
	int_node		*t1 , *t2;

	t1 = (int_node *) a;
	t2 = (int_node *) b;
	if(t1->_i < t2->_i) {
		return -1;
	} else if(t1->_i == t2->_i) {
		if((! t1->has_a_list()) && (t2->has_a_list())) {
			return 1;
		} else if((! t2->has_a_list()) && (t1->has_a_list())) {
			return -1;
		} else if(t1->secondary_key > t2->secondary_key) {
			return 1;
		} else if(t1->secondary_key < t2->secondary_key) {
			return -1;
		}
		return 0;
	}
	return 1;
}

int_node::~int_node() {
		// debug
		// cout << "int_node::~int_node @" << ((void *) this) << "\n";
		}

void int_node::add_cons_to_ancestors() {
	Node	*p = get_parent();
	Node	*t = this;

	while(p) {
		if(t == p->Links[0]) {
			p->add_cons(0, myCount, 0.0);
		} else {
			p->add_cons(1, myCount, 0.0);
		}
		t = p;
		p = p->get_parent();
	}
}

void int_node::subtract_cons_from_ancestors(Node *until_this) {
	Node	*p = get_parent();
	Node	*t = this;

	while(p) {
		if(t == p->Links[0]) {
			p->add_cons(0, valueHolder(-myCount.get_content()), 0.0);
		} else {
			p->add_cons(1, valueHolder(-myCount.get_content()), 0.0);
		}
		t = p;
		p = p->get_parent();
		if(p == until_this) {
			break;
		}
	}
}

void	int_node::attach_left(Node *N, TreeDir::cons_adjust CA) {
	attach(0, N);
	if(CA == TreeDir::UPDATE_CONS) {
		if(N) {
			valueHolder	t(N->get_i_cons());
			t += N->get_l_cons();
			t += N->get_r_cons();
			nCount_left = t;
		}
		else {
			nCount_left = valueHolder(0L);
		}
	}
}

void	int_node::attach_right(Node *N, TreeDir::cons_adjust CA) {
	attach(1, N);
	if(CA == TreeDir::UPDATE_CONS) {
		if(N) {
			valueHolder	t(N->get_i_cons());
			t += N->get_l_cons();
			t += N->get_r_cons();
			nCount_right = t;
		}
			
		else {
			nCount_right = valueHolder(0L);
		}
	}
}

void	int_node::attach(int right_or_left, Node *N, TreeDir::cons_adjust CA) {
	Links[right_or_left] = N;
	if(N) {
		((int_node *) N)->Parent = this;
		if(CA == TreeDir::UPDATE_CONS) {
			valueHolder	t(N->get_i_cons());
			t += N->get_l_cons();
			t += N->get_r_cons();
			if(right_or_left == 1) {
				nCount_right = t;
			} else {
				nCount_left = t;
			}
		}
	} else if(CA == TreeDir::UPDATE_CONS) {
		if(right_or_left == 1) {
			nCount_right = valueHolder(0L);
		} else {
			nCount_left = valueHolder(0L);
		}
	}
}
