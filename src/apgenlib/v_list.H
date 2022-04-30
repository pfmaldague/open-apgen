#ifndef v_LIST_H
#define v_LIST_H

//declares several enums:
#include <C_global.H>
#include <treeDir.H>
#include <C_string.H>

// the following function adds cpu time but may be useful for some applications:
// #define PARENT_NODE

extern long	Validating;
extern long	v_indentation;
extern void	indent_cerr();
class _validation_tag;

typedef enum {
	DOCUM_LIST, DOCUM_DOC} doc_list;

typedef enum {
	VALID_ACT = 1,
	VALID_CALLBACK = 2,
	VALID_LIST = 4,
	VALID_POINTER = 8,
	VALID_TIMEPTR = 16,
	VALID_GRAPH = 32,
	VALID_GOBJ = 64}
 doc_option;

extern _validation_tag *doc_finder(doc_list, void *);

class v_List;
class v_Node;
class pointer_to_backpointer;

typedef int (* v_compare_function)(const v_Node*, const v_Node *);

Cstring void_to_string(void *);

// #define LEFT_DIR	0
// #define RIGHT_DIR	1
// #define BALANCED	2
// #define UNUSED		3

#define VALID_ENABLED	1
#define VALID_DISABLED	2

extern long		list_operations;

class v_Node {
private:
	v_Node			*next;
	v_Node			*previous;

private:
	friend			class v_List;
	friend			class v_blist;

public:
				// parent list (moved here from 'private' for derived destructors)
	v_List			*list;
	unsigned int		indicator;
	v_Node			*links[2];
public:
	//constructors
	// NOTE: derived classes that want to take advantage of balanced
	//	 tree features must initialize members that are specific
	//	 to the avl algorithm: links[] and indicator
	v_Node() : next(NULL) , previous(NULL) , list(NULL) , indicator(TreeDir::UNUSED)
		{}
	v_Node(const v_Node &) { next = NULL; previous = NULL; list = NULL; }

				//assignment of a v_Node does not change v_Node-part members
	v_Node			&operator = (const v_Node &) { return * this; }

				// returns i for the i-th v_Node in the list, - 1 if no list
				// (PFM) made virtual so nodes that are inserted once and for all
				// can store their index, making retrieval much faster:
	virtual long		get_index() const;
				// allows a client to check whether the v_Node belongs to a given list
	bool			is_in_list(const v_List & l) const { return list == &l; }

				// Destructor MUST be virtual (PFM). This is to make sure that
				// the list is able to destroy its component v_Nodes with the proper
				// high-level destructor, e. g. when clearing. Note that unlike
				// other functions, making a base class destructor virtual does
				// NOT mean that the base class destructor is superseded by the
				// derived destructors. It just guarantees that all of them are
				// called as they should be.
				// see v_List::clear()
	virtual ~v_Node();
				//pure virtual function: required in all derived classes
	virtual v_Node		*copy() = 0;
				//return next node
	v_Node			*next_node() const {return next;}
				//return previous node
	v_Node			*previous_node() const {return previous;}

				// WARNING:

				// THE FOLLOWING TWO METHODS ASSUME THAT list POINTS
				// TO A v_blist; USER BEWARE IF IT DOES NOT...

	v_Node			*following_node();
	v_Node			*preceding_node();

	void			add_list_ownership(const Cstring & , v_List *);

				//for compare_keys() and v_List::find()
	virtual const Cstring&	get_key() const {
		return Cstring::null();
	}

	const Cstring		&whoOwnsMe() const { return get_key(); }
				// for node insertion into v_blist:
	v_Node			*step4();
				// v_blist debugging:
	bool			check_parent();
	bool			check_indicator(int &path_length);

	bool			is_a_node() { return true; } };

//linked list
class v_List {
				// the first v_Node in the list:
	v_Node			*head;
				// the last v_Node in the list:
	v_Node			*tail;
				// # of v_Nodes in the list:
	unsigned long		length;
public:
	v_List() { head = NULL; tail = NULL; length = 0L; }
	v_List(const v_List &);
	~v_List();
	
				//makes list empty and deletes nodes -- made virtual to support v_blist
	virtual void		clear();

				// return # of v_Nodes:
	long			get_length() const { return length; }

				//add node to end of list -- made virtual to support v_blist
	virtual v_Node		*insert_node(v_Node *);

				//add node to end of list
	virtual v_List&		operator << (v_Node *node) { insert_node(node); return *this; }

				//add 1st node after 2nd
	v_Node			*insert_node(v_Node*,v_Node*);

	bool			delete_node(v_Node *);

				//remove node from list -- made virtual to support v_blist
	virtual v_Node		*remove_node(v_Node *);
	v_Node			*first_node() const { return head; }
	v_Node			*last_node() const { return tail; }

				//copies all nodes
	v_List &		operator = (const v_List &);

				//move nodes from argument to this
	virtual v_List &	operator << (v_List &);

			 	//find the node with given key
	virtual v_Node		*find(const Cstring &key) const;
	virtual v_Node		*find(const char *key) const;
	virtual v_Node		*find(v_Node *N) const { return find(N->get_key()); }

	virtual void		*get_owner() { return NULL; }

			 	//find the node, following the given node, with given key
	v_Node			*find_after(const Cstring &key, v_Node *) const;

				// return ith node (0-relative)
	v_Node			*operator [] (const unsigned long) const; };

//node containing a string
class v_String_node : public v_Node {
public:

	v_String_node(const char		*st) : string(st) {}
	v_String_node(const Cstring	&st) : string(st) {}
	v_String_node() {}
	~v_String_node() { if(list) list->remove_node(this); list = NULL; }

	Cstring			string;

	v_Node			*copy() { return new v_String_node(string); }
	const Cstring &		get_key() const { return string; } };

class v_bstringnode: public v_String_node {
public:
	v_bstringnode() : v_String_node()
		{ indicator = TreeDir::BALANCED; links[TreeDir::RIGHT_DIR] = NULL; links[TreeDir::LEFT_DIR] = NULL; }
	v_bstringnode(const Cstring & a) : v_String_node(a)
		{ indicator = TreeDir::BALANCED; links[TreeDir::RIGHT_DIR] = NULL; links[TreeDir::LEFT_DIR] = NULL; }
	v_bstringnode(const v_bstringnode & B) : v_String_node(B)
		{ indicator = TreeDir::BALANCED; links[TreeDir::RIGHT_DIR] = NULL; links[TreeDir::LEFT_DIR] = NULL; }
	~v_bstringnode() { if(list) list->remove_node(this); list = NULL; }
	v_Node		*copy() { return new v_bstringnode(*this); } };

class v_List_iterator {
			//list which is iterated
	const v_List	*list;
			//current node;
	v_Node		*node;
public:
	v_List_iterator(const v_List& l) {list = &l; node = NULL;}
	v_Node		*operator()() {return next();}
			//returns first node, then other nodes, then 0, then start from top again
	v_Node		*next();
			//returns last node, then other nodes, then 0, then start from end again
	v_Node		*previous();
			//current node = first node
	v_Node		*first() {node = list->first_node(); return node;}
			//current node = last node
	v_Node		*last() {node = list->last_node(); return node; } };


// BALANCED LIST (PFM)

#define MAXSEARCHDEPTH 40

#define EMPTY_TREE 1
#define NONEXISTENT_NODE 2
#define TREE_TOO_BIG	3
#define ERROR_IN_STEP4	4

class v_blist: public v_List {
public:
	v_Node			*root;
	bool			enabled;
	bool			callbacksEnabled;
	void			enable_callbacks() { callbacksEnabled = true; }
	void			disable_callbacks() { callbacksEnabled = false; }
	v_blist(v_compare_function c) : root(NULL), cmp(c), enabled(true), callbacksEnabled(true) {}
	v_blist(const v_blist &B);
	// Note: the base class destructor invokes virtual clear() which
	//	 will disable binary features to save time
	~v_blist() {}

	v_compare_function	get_compare_function() const { return cmp; }
	v_blist			&operator = (const v_blist &);
				// order the whole list
	virtual void		order();
				// insert a node and its subnodes in order
	void			order(v_Node *);
				// insert a node (returns NULL if a node with
				// the same key exists, EXCEPT special case for
				// (cmp == compare_Time_nodes) ONLY, which
				// allows duplicates via a unique secondary key)
	v_Node			*insert_node(v_Node *);
				// adds 1st node after 2nd, as in v_List::insert_node(v_Node*,v_Node*):
	v_Node			*insert_node(v_Node * , v_Node *);
	v_Node			*insert_binary_node(v_Node * &);
	void			add_without_duplication(v_List &);
	v_Node			*remove_node(v_Node *);
	v_Node			*find(v_Node *) const;
	v_Node			*find(const Cstring &) const;
	v_Node			*find(unsigned long) const;
	v_Node			*find(const char *) const;
	v_Node			*find(void *) const;
	virtual v_Node		*find_latest_before(v_Node *) const;
	virtual v_Node		*find_earliest_after(v_Node *) const;

	virtual v_Node		*earliest_node() const;
	virtual v_Node		*latest_node() const;
	void			clear();
	virtual const Cstring&	get_id() const {
		return Cstring::null();
	}

				// insertion operators (SILENTLY discard duplicates, so check first!):
	v_List &			operator << (v_Node * node);
	v_List &			operator << (v_List &);
	v_List &			operator << (v_blist &);

				// inclusion operator (returns 1 if every v_Node of this is found in r.h.s., 0 otherwise)
	bool			operator <= (v_blist &);

	v_compare_function	cmp;
	bool			check(); };

// STANDARD NODE COMPARISON FUNCTIONS:


	// compares based on key:
int	v_compare_v_bstringnodes(const v_Node *A, const v_Node *B);
	// compares based on integer key; node not yet inserted has greater value when times are equal:
int	v_compare_v_int_nodes(const v_Node *A, const v_Node *B);

class v_int_node: public v_Node {
protected:
	unsigned long		secondary_key;
public:
	v_int_node(unsigned long i) :_i(i), secondary_key(0L)
		{ indicator = TreeDir::BALANCED; links[TreeDir::RIGHT_DIR] = NULL; links[TreeDir::LEFT_DIR] = NULL; }
	v_int_node(const v_int_node &i_n) : _i(i_n._i), secondary_key(0L) {}

	~v_int_node() {}
	const Cstring&		get_key() const {
		return Cstring::null();
	}

	v_Node			*copy() { return new v_int_node(*this); }
	unsigned long		_i;
	bool			has_a_list() { return list != NULL; }
	friend int v_compare_v_int_nodes(const v_Node *, const v_Node *);
	friend class v_blist; };

class bsymbolicnode: public v_bstringnode { 
public:
	bsymbolicnode(const Cstring &a, const Cstring &b)
		: v_bstringnode(a),
			equivalent(b),
			insertion_history(NULL),
			address(0L) {}
	bsymbolicnode(const Cstring &a, unsigned long b)
		: v_bstringnode(a),
			insertion_history(NULL),
			address(b) {}
	bsymbolicnode(const bsymbolicnode &BS)
		: v_bstringnode(BS),
			equivalent(BS.equivalent),
			insertion_history(NULL),
			address(BS.address) {}
	~bsymbolicnode();
	v_Node			*copy() { return new bsymbolicnode(*this); }
	v_List			*insertion_history;
	unsigned long		address;
	Cstring			equivalent;
};

class v_aoString {
	v_List			*theList;
	char			*theBuffer;
	char			*theCharHolder;
	unsigned long		theCharPointer;
	static unsigned long	theCharHolderSize;
public:
	v_aoString();
	~v_aoString();

	v_aoString &	operator << (const char *);
	v_aoString &	operator << (const Cstring &);
	char		*str();
	void		put(const char);
	void		clear(); };

	/**
	 * 'Interesting' objects such as ACT_req, Pointer_node, double_pointer_node, Time_ptrnode
	 * will have to have a 'documentation method' available. This method should NOT reference
	 * the object itself, because the object may have been deleted; the idea is that a client
	 * with nothing but a pointer to the object should be able to inquire as to the object's
	 * status. The only way to do this in case the object is no longer 'alive' is to reference
	 * data (a validation_tag indexed by a string whose value is the pointer to the object)
	 * that is NEVER deleted, and TO THE BEST OF OUR KNOWLEDGE valid.
	 *
	 * This will of course take some discipline. For instance, whenever the pointer inside a
	 * Pointer_node is modified, that should be recorded dutifully...
	 *
	 * Initially I included a pointer to the tag holding the data, but that is unnecessary because
	 * the only thing an address is good for is retrieving the validation tag (the REAL address
	 * may point to corrupted/obsolete data), and the only data in the tag is the list we already
	 * have!
	 *
	 * The function returns a (hopefully descriptive and meaningful) text string. The indentation
	 * argument will allow us to describe nested structures etc. Finally, the last argument allows
	 * us to avoid recursive loops when describing objects with back-pointer to their parents. The
	 * object being documenting should insert itself as a btag into that list so that it never
	 * documents itself again in the documentation process...
	 *
	 * Well, hold on: the first thing docFunc should do is see whether it's already documented itself!!
	 * So, I guess it does need the pointer to the original object (as a string).
	 */
typedef void (*docFunc)(v_aoString &a, unsigned long theObj, const v_blist &itsDescription, long Indentation, v_blist &myFriends);

class _validation_tag: public v_int_node {
private:
				/**
				 * This list can contain any bsymbolicnodes as long as theDocMethod understands it. We
				 * could make it a string, but I'd hate to do parsing inside validation methods...
				 */
	v_blist			listForDoc;
	Cstring			key;
public:
	_validation_tag(void *theObjectToDocument , docFunc theF , const v_blist &theDocData);
	_validation_tag(const _validation_tag &T);

				/**
				 * We don't do anything on destruction; the idea is that
				 * these tags 'live' longer than any of the objects they
				 * describe, and they are intended to document problems that
				 * will occur well before the final Hiroshima anyway...
				 */
	~_validation_tag() {}

	v_Node			*copy() { return new _validation_tag(*this); }

				/**
				 * Used to document the object:
				 */
	docFunc			whoami;

	void			add(bsymbolicnode *N) { listForDoc << N; }
	void			remove(bsymbolicnode *N) { listForDoc.remove_node(N); }
	bsymbolicnode		*find(const char *s) { return (bsymbolicnode *) listForDoc.find(s); }
	const v_blist		&get_the_list() const { return listForDoc; }
	const Cstring		&get_key() const { return key; }
	Cstring			get_uint() const { return Cstring(_i); }

				/**
				 * Keeps track of how many objects have been using this address. This is necessary
				 * because malloc() re-uses memory locations, so we need to update our pointers...
				 */
	long			usage_count;
				/**
				 * Points to the previous object that was allocated at this address:
				 */
	_validation_tag		*next_tag;
	unsigned long		original_address; };

class v_list_node: public v_int_node {
public:
	v_list_node(unsigned long k)
	: v_int_node(k),
	  L(v_compare_v_int_nodes) {}
	v_list_node(const v_list_node &V)
	: v_int_node(V),
	  L(v_compare_v_int_nodes) {}
	~v_list_node() {}
	v_blist		L;
	};

extern void		document_a_Pointer_node(		v_aoString &a, unsigned long, const v_blist &, long Indentation, v_blist &);
extern void		document_interesting_lists(		v_aoString &a, unsigned long, const v_blist &, long Indentation, v_blist &);
extern void		document_list_and_nodes(		v_aoString &a, unsigned long, const v_blist &, long Indentation, v_blist &);
extern void		document_GenericObjects(		v_aoString &a, unsigned long, const v_blist &, long Indentation, v_blist &);


#endif
