

#ifndef _C_LIST_H_
#define _C_LIST_H_

#include <new>
#include "v_list.H"

//declares several enums:
#include "C_global.H"
#include "C_string.H"
#include "RES_exceptions.H"
#include "UTL_time_base.H"


#ifndef DISABLE_ASSOC_RES
// the following function adds cpu time but may be useful for some applications:
#define PARENT_NODE

#endif

class List;
class blist;
class Node;
class pointer_to_backpointer;
class double_pointer_node;
class Multiterator;

#define MAXDEPTH 40

typedef int (* internal_compare_function) (const Node *, const Node *);

// Encapsulation of the function used to compare nodes in ordered lists
class compare_function {
private:
		// To keep track of functions that compare times
	bool	time_flag;
public:
	compare_function(int (*C)(const Node *, const Node *), bool T)
		: compfunc(C),
		time_flag(T) {}
	compare_function(const compare_function& c)
		: compfunc(c.compfunc),
		time_flag(c.time_flag) {}
	~compare_function() {}
					// the function that actually does the work
	internal_compare_function	compfunc;

		/* used to figure out whether we should set the secondary key when
		 * dealing with equal-time nodes */
	bool	compares_times() const { return time_flag; }
		// for compatibility with old definition
	int	operator () (const Node *a, const Node *b) const {
			return compfunc(a, b); }
	bool	operator != (const compare_function &C) const {
			return compfunc != C.compfunc; }
	bool	operator == (const compare_function &C) const {
			// shouldn't have to compare flags if client knows what he/she is doing.
			return compfunc == C.compfunc; }
	};

class Pointer_node;

// #define LEFT_DIR	0
// #define RIGHT_DIR	1
// #define BALANCED	2
// #define UNUSED	3

typedef enum {
	BEFORE_NODE_INSERTION,
	AFTER_NODE_INSERTION,
	BEFORE_NODE_REMOVAL,
	AFTER_NODE_REMOVAL,
	BEFORE_FINAL_NODE_REMOVAL,
	AFTER_FINAL_NODE_REMOVAL } list_event;

typedef enum {
	AAF_FUNCTION,
	ACT_EXECUTION_CONTEXT,
	ACT_REQ,
	ACTION_REQUEST,
	ARRAYELEMENT,
	ASSOC_FLOAT,
	BATTRIBUTE_TYPE_NODE,
	BLOCK_NODE,
	BPOINTERNODE,
	BREVERSE_TAG,
	BSTRINGNODE,
	BSYMBOL_NODE,
	BTAG,
	CONCRETE_TIME_NODE,
	CONSTRAINT,
	CULPRIT_NODE,
	CULPRIT_THREAD,
	CULPRIT_OBJECT,
	// DECLPARAM_INSTRUCTION,	- unused
	DOUBLE_POINTER_NODE,
	EDITNODE,
	EXECUTION_CONTEXT,
	FUNCTION_EXECUTION_CONTEXT,
	GLOBALSYMBOL,
	GUISTUFF,
	INSTRUCTION,
	INTERNAL_SYMBOL_LIST,
	TIME_NODE,
	INT_NODE,
	LIST_OBJECT,
	LOCALLISTENER,
	PEF_RECORD,
	POINTER_NODE,
	POINTER_TO_POINTER,
	REF_NODE,
	RES_ABSTRACT,
	RES_ASSOCIATIVE,
	RES_EXECUTION_CONTEXT,
	RES_STATE,
	RES_CONSUMABLE,
	RES_CONTAINER,
	RES_HISTORY,
	RES_NONCONSUMABLE,
	RES_NODE,
	RES_TIMENODE,
	SASF_POINTER,
	SEVERITY,
	SIBLING,
	SIMPLE_EXECUTION_CONTEXT,
	SIMPLE_BSYMBOL_NODE,
	STRING_NODE,
	SYMBOL_NODE,
	TAG,
	TAGGEDLIST,
	TAGLIST,
	TAGLIST_OBJECT,
	TIME_PTRNODE,
	TYPEDNODE,
	TYPEDSYMBOL,
	TIME_EXPNODE,
	TOL_THREAD,
	UNATTACHED_VALUE_NODE,
	UNKNOWN_EXTENSION,
	USAGE_CLAUSE_NODE,
	VALUE_NODE,
	NC_VALUE_NODE,
	VIOLATION_INFO,
	WIN_NODE
	} Node_type;

typedef enum {
	LIST_BASE,
	BLIST,
	ILIST,
	OWNEDLIST,
	OWNEDCLIST,
	LISTOVAL } List_type;

class TypedValue;

/*
Generic linked list:  Your nodes should be derived from class Node
and created from the heap (using operator 'new').  A previously-defined
class may be made into a node by using that class
and Node as mix-ins.  Whenever List methods return Node*, it must be
cast to a pointer to the derived class, or methods may be defined for
the derived class that do the conversion.  In general, each node should
have the following methods defined:

	copy()         		(see comments for List::operator=())
	get_key()    		(see comments for List::find(),
				Ordered_list, Node::compare_keys())
	destructor		(see comments for List::clear())
*/
class Node {
private:
	/* Private data section
	This section contains the nuts and bolts needed to support
	three structures: a linked list, a Btree, and a Tree of
	Cumulative Values. */
				// next node in linked list
	Node			*next;
				// previous node in linked list
	Node			*previous;
				// AVL links
	Node			*Links[2];
public:
	/* Friends
	Classes that need access to our private data members. Blist
	contains the AVL algorithms for node insertion and deletion.
	*/
	friend			class List;
 	friend			class blist;
				// desperate for efficiency
	friend class back_pointer;
				// Solaris doesn't require this but HP does
	friend class clist;


	/* Public methods used by private algorithms
	This section contains the basic methods that support
	three structures: a linked list, a Btree, and a Tree of
	Cumulative Values. */
	virtual void		make_root_of(blist *b);
	Node*			get_left() const { return Links[0]; }
	Node*			get_right() const { return Links[1]; }
	Node*			get_link(int which) const { return Links[which]; }
	virtual Node*		get_parent() const { return NULL; }
	int		get_indicator() const { return indicator; }
	Node			*me_as_node() { return this; }

	/* Public methods that support consumption modeling 
	This section contains the basic methods that support
	modeling the cumulative consumption of nodes in the list.
	*/
	virtual void		init_cons() {}
	virtual valueHolder	get_l_cons() const { return valueHolder(0L); }
	virtual valueHolder	get_r_cons() const { return valueHolder(0L); }
	virtual valueHolder	get_i_cons() const { return valueHolder(0L); }
	virtual void		set_i_cons(const valueHolder *) {}
	virtual void		add_cons(int which_dir, const valueHolder &, const valueHolder &) {}
	virtual void		add_cons_to_ancestors() {}
	virtual void		subtract_cons_from_ancestors(Node *stop_here = NULL) {}

	/* Public methods and data that support insertion and deletion methods
	This section contains the basic methods and data that support
	maintenance of linked lists and btrees.
	*/
				// Initialize all (linked list and Btree) links
	void			reset_links();
				// changes connectivity
	virtual void		attach_left(Node *N, TreeDir::cons_adjust CA = TreeDir::DO_NOT_UPDATE_CONS);
				// changes connectivity
	virtual void		attach_right(Node *N, TreeDir::cons_adjust CA = TreeDir::DO_NOT_UPDATE_CONS);
				// changes connectivity
	virtual void		attach(int right_or_left, Node *N, TreeDir::cons_adjust CA = TreeDir::DO_NOT_UPDATE_CONS);
				// does validation; returns 1 if subtree is valid, 0 if not
	int			get_pathlength(int &length);
				// parent list (moved here from 'private' for derived destructors)
	List			*list;
				// Used by the AVL balanced tree algorithm
	int		indicator;

public:

	/* Constructor section -- This section contains the basic constructors and destructors */

	/* void constructor
	  NOTE:	derived classes that want to take advantage of balanced
		tree features must initialize members that are specific
		to the avl algorithm: links[] and indicator
	*/
	Node() : next(0), previous(0), list(0),
		indicator(TreeDir::BALANCED) {
		Links[1] = NULL;
		Links[0] = NULL; }
	Node(const Node &) {
		next = 0;
		previous = 0;
		list = 0;
		indicator = TreeDir::BALANCED ;
		Links[1] = NULL;
		Links[0] = NULL; }

				/* The destructor MUST be virtual (PFM). This is to make sure that
				* the list is able to destroy its component Nodes with the proper
				* high-level destructor, e. g. when clearing. Note that unlike
				* other functions, making a base class destructor virtual does
				* NOT mean that the base class destructor is superseded by the
				* derived destructors. It just guarantees that all of them are
				* called as they should be.
				* see List::clear() */
	virtual ~Node();

	/* Virtual functions
	Some of these methods are pure virtual (get_key() and copy() mostly)
	*/
				// pure virtual function: required in all derived classes
	virtual Node		*copy() = 0;
				// for compare_keys() and List::find()
	virtual const Cstring	&get_key() const = 0;
	virtual Node_type	node_type() const = 0; // { return UNKNOWN_EXTENSION; }

				// assignment of a Node does not change Node-part members
	Node			&operator = (const Node &) { return *this; }

				/* returns i for the i-th Node in the list, - 1 if no list
				    (PFM) made virtual so nodes that are inserted once and for all
				    can store their index, making retrieval much faster: */
	virtual long		get_index_const() const;
	virtual long		get_index();
				// allows a client to check whether the Node belongs to a given list
	bool			is_in_list(const List &l) const { return list == &l; }

	/* Navigational methods -- These methods can navigate both the linked list and the Btree */
				// return next node
	Node			*next_node() const {return next;}
				// return previous node
	Node			*previous_node() const {return previous;}

				/* WARNING:

				   THE FOLLOWING TWO METHODS ASSUME THAT list POINTS
				   TO A blist; USER BEWARE IF IT DOES NOT... */
	Node			*following_node();
	Node			*preceding_node();

	/* General-purpose information and action methods */
	// const Cstring		&whoOwnsMe() const;
				// for node insertion into blist
	Node			*rotate();
				/* Linked node support (e. g. when changing the
				time tag of a bunch of linked nodes)
				*/
	virtual void		temporarily_remove_from_list() {}
				/* Linked node support (e. g. when changing the
				time tag of a bunch of linked nodes)
				*/
	virtual void		re_insert_in_list() {}
				/*
				 * can be overridden; called when list -> an OwnedList,
				 * which will invoke its Nodes' action_callback whenever
				 * inserting/deleting nodes (see list_event enum and
				 * OwnedList class definition below.)
				 */
	virtual void		action_callback(void *my_owner, list_event what_happened) {}
	bool			is_a_node() { return 1; }
	virtual int		reacts_to_time_events() { return 0; }
	virtual List		*get_saved_list() { return NULL; }
	virtual void		set_saved_list(List *) {}
	friend class ArrayElement;
	// friend class AssociativeFloat;
	friend class nc_value_node_f;
	friend class nc_value_node_i;
	friend class int_node;
	};


// linked list
class List {
private:
				// the first Node in the list
	Node			*head;
				// the last Node in the list
	Node			*tail;
				// # of Nodes in the list
	long			length;

protected:
				// identifies type
	List_type		myType;
public:
	List();
	List(const List &);
public:
	virtual ~List();

	static const char	*spell_Node_type(const Node_type);
	virtual internal_compare_function get_compare_function() const { return NULL; }

				// makes list empty and deletes nodes -- made virtual to support blist
	virtual void		clear();
	virtual List_type	list_type() { return myType; }
	virtual void		enable_callbacks() {}
	virtual bool		is_enabled() const { return false; }
	virtual void		disable_callbacks() {}

				// to support list_with_max_length
	virtual long		getMaxLength() const { return -1; }
	virtual void		setMaxLength(long) {}
	virtual long		isAListWithMaxLength() const { return 0; }
	virtual long		isAListOfValueNodes() const { return 0; }
				/* Overridden in OwnedList */
	virtual Node		*remove_node_prior_to_deletion(Node *n)
					{ return remove_node(n); }

				// return # of Nodes
	long			get_length() const { return length; }
				// add node to end of list -- made virtual to support blist
	virtual Node		*insert_node(Node *);

				// add node to end of list
	virtual List&		operator << (Node * node)
					{ insert_node(node); return *this; }

				// add 1st node after 2nd
	Node			*insert_first_node_after_second(Node *, Node *);
				// removes and deletes the node
	bool			delete_node(Node *);
				// remove node from list -- made virtual to support blist
	virtual Node		*remove_node(Node *);
	Node			*first_node() const {return head;}
	Node			*last_node() const {return tail;}
				// copies all nodes
	List			&operator = (const List &);
				// move nodes from argument to this
	virtual List &		operator << (List &);
			 	// find the node with given key
	virtual Node		*find(const Cstring & key) const;
				// same, avoids a call to malloc
	virtual Node		*find(const char *key) const;
				// find node with given ptr (assumes List contains Pointer_nodes)
	virtual Node		*find(void *P) const;
				// uses the key of the given node
	virtual Node		*find(Node * N) const { return find(N->get_key()); }
				// meant to be overridden
	virtual void		*get_owner() { return NULL; }
			 	// find the node, following the given node, with given key
	Node			*find_after(const Cstring & key, Node *) const;
				/* return ith node (0-relative). This function used to have
					a significant performance problem; problem was fixed
					with the ListOVal and ArrayElement classes */
	Node			*operator [] (const unsigned long) const;

public:

#	ifdef ENABLE_ALLOCATION_STATS
	static void *operator new(size_t);
	static void operator delete(void *);
	static double report_total_bytes();
	static int report_total_objects();
#	endif /* ENABLE_ALLOCATION_STATS */
	// static const char *get_apcore_version();
	static const char *get_short_version();

	// Classes that need the ability to create Lists on the heap:
	friend class TypedValue;
	friend class ArrayHandler;
	friend class sasf_pointer;
	friend class list_buf;
	friend class blist;
	};

// node containing a string
class String_node : public Node {
public:
	String_node(const char		*st) : theString(st) {}
	String_node(const Cstring	&st) : theString(st) {}
	String_node() {}
	~String_node() { if(list) list->remove_node(this); list = NULL; }

	Cstring			theString;

	Node			*copy() { return new String_node(theString); }
	Node_type		node_type() const { return STRING_NODE; }
	const Cstring &		get_key() const { return theString; } };

// node containing two strings
class Symbol_node : public Node {
public:
	Cstring			name;
	Cstring			equivalent;

	Symbol_node(const Cstring & nm, const Cstring & eq);
	~Symbol_node() { if(list) list->remove_node(this); list = NULL; }
	Node_type		node_type() const { return SYMBOL_NODE; }
	Node			*copy() { return new Symbol_node(name, equivalent); }
	const Cstring		&get_key() const { return name; } };

// node containing a pointer and a string
class Tag : public Node {
	void		*pointer;
public:
	Cstring		name;

	Tag(const Cstring& nm, void* pt);
	~Tag() { if(list) list->remove_node(this); list = NULL; }

	void		*get_pointer() { return pointer; }
	Node		*copy() { return new Tag(name, pointer); }
	Node_type	node_type() const { return TAG; }
	const Cstring	&get_key() const { return name; } };

// STANDARD NODE COMPARISON FUNCTIONS:

	// compares based on key
int	compare_bstringnodes(const Node *A, const Node *B);
	// compares based on key but considers uppercase to be the same as lowercase
int	compare_bstringnodes_nocase(const Node *A, const Node *B);
	// compares based on void *
int	compare_bpointernodes(const Node *A, const Node *B);
	// compares based on event times; node not yet inserted has greater value when times are equal
int	compare_Time_nodes(const Node *A, const Node *B);
	// compares based on integer key; node not yet inserted has greater value when times are equal
int	compare_int_nodes(const Node *A, const Node *B);

//  version of String_node suitable for insertion into balanced trees:
class bstringnode: public String_node {
public:
	bstringnode() : String_node()
		{}
	bstringnode(const Cstring & a) : String_node(a)
		{}
	bstringnode(const bstringnode & B) : String_node(B)
		{}
	~bstringnode() { if(list) list->remove_node(this); list = NULL; }

	Node*		copy() { return new bstringnode(*this); }
	Node_type		node_type() const { return BSTRINGNODE; }

	bstringnode*	next_node() { return (bstringnode*) Node::next_node(); }
	bstringnode*	previous_node() { return (bstringnode*) Node::previous_node(); }
	static compare_function	compfunc() { return compare_function(compare_bstringnodes, false); }
};

class bSymbol_node: public bstringnode {
public:
	Cstring			equivalent;

	bSymbol_node(const Cstring &nm, const Cstring &eq) : bstringnode(nm), equivalent(eq) {}
	~bSymbol_node() { if(list) list->remove_node(this); list = NULL; }
	Node_type		node_type() const { return SIMPLE_BSYMBOL_NODE; }
	Node			*copy() { return new bSymbol_node(get_key(), equivalent); } };

// BALANCED LIST (PFM)

#define MAXSEARCHDEPTH 40

#define EMPTY_TREE 1
#define NONEXISTENT_NODE 2
#define TREE_TOO_BIG	3
#define ERROR_IN_STEP4	4


class aoString;

// blist is the mother of all complex lists in APGEN.
class blist: public List {
protected:
				// Basic insertion method
	Node			*insert_binary_node(Node * &);
public:
				// The root of the shadow Btree that parallels the linked list
	Node			*root;
				// Used to disable btree logic when clearing the blist
	bool			enabled;
	virtual bool		is_enabled() const { return enabled; }
	virtual void		enable_callbacks() { callbacksEnabled = true; }
	virtual void		disable_callbacks() { callbacksEnabled = false; }
				/* Used to disable callbacks when ordering a blist
				 *  (all nodes will be reinserted) */
	bool			callbacksEnabled;

				// for debugging; use NULL for root
	void			dump(Node *, int indentation);
	void			dump(Node *, int indentation, aoString *);
	void			dump();
	void			dump(aoString *);
	bool			validate();

	blist(compare_function c);
	blist(const blist &B);
public:
	/* Note: the base class destructor invokes virtual clear() which
		  will disable binary features to save time */
	~blist();

	internal_compare_function get_compare_function() const { return cmp.compfunc; }
	blist&			operator = (const blist &);

				// order the whole list
	virtual void		order();

				// insert a node and its subnodes in order
	void			order(Node *);

				/* insert a node (returns NULL if a node with
				 * the same key exists, EXCEPT special case for
				 * (cmp == compare_Time_nodes) ONLY, which
				 * allows duplicates via a unique secondary key) */
	Node			*insert_node(Node *);

				// adds 1st node after 2nd, as in List::insert_node(Node*,Node*)
	Node			*insert_node(Node *, Node *);
	void			add_without_duplication(List &);
	Node			*remove_node(Node *);
	Node			*find(Node *) const;
	Node			*find(const Cstring &) const;
	Node			*find(int) const;
	Node			*find(const char *) const;
	Node			*find(void *) const;
	virtual Node		*find_latest_before(Node *) const;
	virtual Node		*find_latest_before_w_comp_func(Node *, internal_compare_function) const;
	virtual Node		*find_earliest_after(Node *) const;

	virtual Node		*earliest_node() const;
	virtual Node		*latest_node() const;
	void			clear();
	virtual const Cstring&	get_id() const {
		return Cstring::null();
	}

				/* insertion operators (SILENTLY) discard duplicates,
				 * so check first!  Actually, a msg to stderr is issued */
	List			&operator << (Node * node);
	List			&operator << (List &);
	List			&operator << (blist &);

				/* inclusion operator (returns 1 if every Node
				 * of this is found in r.h.s., 0 otherwise) */
	bool			operator <= (blist &);
	compare_function	cmp;

				// for verification
	Node			*v_earliest_node(v_blist &keep_track, int &score) const;
	Node			*v_find_earliest_after(
					Node	*,
					v_blist	&keep_track,
					int	&score
					) const;
	virtual Node		*operator() (const unsigned long) const;
	bool			check();
	friend class TimeKey;
};

/*
The following class allows creating "smart" Pointer_nodes.
A smart pointer is a pointer that destroys itself when the
object it points to disappears.

  Needless to say, for this to work it is necessary to enlist
  the cooperation of the object pointed to -- the "Pointee". This
  cooperation is obtained by making the Pointee inherit from the
  "back_pointer" class.

  Before we explain this in detail, let us introduce some careful
  terminology. There are three objects under consideration:

	- the Pointor; this is an object that contains a List of zero
	  or more Nodes based on the Pointer_node class

	- the Pointee; this is an object based on the back_pointer class.
	  It knows how many Pointer_nodes are pointing to it, and it does
	  appropriate "smart" things, for example delete all those pointers
	  before destroying itself. This ensures that there will be no
	  "dangling pointers" left.

	- the Pointicle (!) -- terminology introduced to avoid confusion
	  with pointer, Pointer_node, smart pointer etc.

  When a Pointicle is created, it is told (via the second argument to its
  constructor) whether it should be smart or not. If it is to be smart, the
  second argument is non-null and is a pointer to the Pointee viewed as a
  back_pointer.

  This latter point requires some explanation. Suppose the Pointee is based on
  two classes, foo and bar. Since we want to Pointee to be pointed at by a
  Pointicle, we need to also base the Pointee on the back_pointer class. Therefore,
  the Pointee's class declaration will look something like

	class Pointee_object: public foo, public bar, public back_pointer
		{
		...
		};

  Now when we use pointer nodes to point to things, we typically provide the node
  with a pointer to the pointee, cast as a void *. Thus, if object A is a Pointee_object,
  we pass (void *) &A as the first argument to a pointer node. If we want to retrieve
  A later on, all we need to do is to call the get_ptr() method of the Pointer_node
  and cast it as a Pointee_object.

  However, the generic methods in the back_pointer and List classes don't know exactly what
  class the Pointee belongs to; all they care about is that Pointee should be based
  (possibly among other things) on the back_pointer class. But casting the void pointer
  stored in the Pointer_node will NOT work. This is because an object is really a C
  structure, in which data members belonging to base classes are organized in the order
  in which the base classes are inherited by the object. In the present case, a Pointee_object
  inherits from back_pointer last, and therefore the data corresponding to the "back_pointerness"
  of the object will be found at the END of the structure, not at the beginning.

  For this reason, a Pointicle requires not only a pointer to the "fully derived" object
  cast as a void *, but also a pointer to that same object cast as a back_pointer. This
  second pointer points to precisely the right part of the object's structure for the
  node logic to work properly.

  Now let's think about what happens when a pointicle is created. We just discussed how the
  pointee information is passed to the pointicle. When the pointicle constructs itself, it
  will store a new pointer (actually, a double_pointer_node) in the PointerNodes list of the
  pointee; this List is a member of the back_pointer class on which the pointee is based. The
  purpose of this List is of course to provide the pointee with a list of all the pointicles
  pointing to it, so that the pointee can (among other things) delete all these pointicles
  before destroying itself.
  
  This double_pointer_node points to the pointicle. The reason why a double_pointer_node is
  used, instead of a simpler Pointer_node, is similar to the reason why 2 pointers are passed
  to the pointicle constructor instead of 1: the actual pointicle class could be based on multiple
  classes. The back_pointer and List classes need to know where the relevant information is
  stored inside the pointicle. For this purpose, pointicles must inherit from the
  "pointer_to_backpointer" class, and the second argument of the double_pointer_node
  constructor is -- you guessed it -- a pointer to the pointicle itself, cast as a
  pointer_to_backpointer *.

  (...)

  Later in life, I decided that this mechanism, although really nifty, was not complex enough.
  Let's look at an example: the ACT_req class, which contains a List called "TypeObjects". This
  List is empty if the activity type for the request has not been defined. If the type has been
  defined, then the TypeObjects List contains exactly one pointicle pointing to a pointee of the
  ACT_type class.

  Thanks to the back_pointer mechanism, if anybody deletes the ACT_type object, the pointicle will
  automatically disappear, so that the ACT_req object will "know" that its type is no longer defined.
  This did wonderful things like enable "Purge" to work without leaks, etc. But it is not enough!
  In reality, there are other things that should take place when an activity request sees its type
  disappear. For instance, it should probably execute its destruction code, since it no longer
  exists as an instance of the disappearing type (some philosophical questions here, admittedly;
  however I believe this is the right thing to do). Why can't we automate this task, by allowing
  smart pointers to automatically go through a "list of things to do"? Actually and while we are
  at it, how about TWO lists of things to do, one on the pointor and one on the pointee?

  I believe that such lists of things to do would greatly enhance code intelligibility, simplicity
  and maintainability. So much for theory; now we'll look at applications...
*/
class back_pointer {
public:
	back_pointer();
	back_pointer(const back_pointer &)
		: PointerNodes(compare_function(compare_bpointernodes, false)) {}
	virtual ~back_pointer();

				/* only used in derived class; forces offset between a back_pointer
				 * and its PointerNodes (convenient, if not very object-oriented) */
	void			*theData;
				/* List of double_pointer_nodes WITHOUT back_pointers...
				* The pointer of each Node in the List points to the
				* Pointer_node referencing this object */
	blist			PointerNodes;

				// Used when Validation is turned on
	void			create_doc_record_for_self_and_PointerNodes(
					void	*address_to_use,
					v_blist	*optional_fields);

				// for identifying this when doing Validation
	virtual Cstring		get_id() { return "base back_pointer object"; }
				/* callback function, used when a node is inserted into/removed
				 * from a List. */
	virtual void		theAction(	void *,
						Pointer_node *,
						list_event message_to_the_owner)
					{}
				/* note that destruction becomes a carefully orchestrated ballet
				 * when forward- and back-pointing. This function is used to make
				 * life a little simpler. */
	void			delete_pointers_to_this();
	};

	/*
	 * We make the following class pure virtual and use derived classes to specify theAction.
	 * Note that this type of node is only useful when pointers to them are inserted into an
	 * OwnedList.
	 *
	 * Note also that TWO actions are invoked whenever such pointers are inserted, removed,
	 * and deleted: first, the action specified by the OwnedList itself (if any) is performed;
	 * then, the action associated with the back_pointer is carried out.
	 *			
	 * This class is MEANT TO BE USED AS 'pointees' for Pointer_nodes in an OwnedList.
	 * If the container list of the pointers is not an OwnedList, the action mechanism
	 * will not be invoked at all.
	 *
	 * theAction() will be called every time the POINTER node (not THIS node)
	 * is inserted (message_to_the_owner = BEFORE/AFTER_NODE_INSERTION),
	 * removed (message_to_the_owner = BEFORE/AFTER_NODE_REMOVAL), or
	 * removed just prior to deletion (message_to_the_owner = BEFORE/AFTER_FINAL_NODE_REMOVAL).
	 * The call is made with the following arguments: the
	 * owner of the list containing the pointer, the pointer itself, and an enum
	 * 'message' as indicated above.
	 *
	 * Note that theAction() is NOT defined here; it already is defined as a virtual
	 * method in the base back_pointer class, where it does nothing. To activate the
	 * mechanism, define a class based on back_pointer_with_action and override theAction
	 * with your own method.
	 *
	 * migrated theData to base class to provide offset (convenient for reporting debug info):
	 *	void			*theData;
	 */
class back_pointer_with_action: public back_pointer {
public:
	back_pointer_with_action(void *d)
		{ theData = d; }
	~back_pointer_with_action() {} };

class pointer_to_backpointer {
public:
	pointer_to_backpointer();
	~pointer_to_backpointer();

	double_pointer_node	*DP;
	};

// node containing a pointer -- MODIFIED (PFM)
class Pointer_node: public Node, public pointer_to_backpointer {
public:
	void		*pointer;
			/* Need this because (void *) ObjectPtr and (back_pointer *) ObjectPtr
			    are NOT necessarily the same address (learned through bitter experience!) */
	back_pointer	*BP;
	/*
	 * The following index is 0 for a DUMB Node, >= 0 for
	 * a SMART Node. Its value should agree with the List
	 * in the object pointed to; i.e., if p is a Pointer_node *
	 * whose pointer is a SMART object s, then
	 * s->PointerNodes[ p->index_in_backpointer_list ] should be
	 * a DUMB Pointer_node whose pointer is equal to p
	 * 
	 * Use the first argument to create either DUMB or SMART Nodes
	 * For a DUMB node, set the second arg. to NULL
	 * Set the second argument to a TRUE BACK_POINTER to create a
	 * SMART Node and set the first arg. to the SAME OBJECT cast as
	 * a void * if necessary)
	 */
	Pointer_node(void *p, back_pointer *bp); // PFM {pointer = p;}
	Pointer_node(const Pointer_node & pn);


			/* The destructor will delete the backpointer in the object pointed to
			    provided that index_in_backpointer_list is >= 0 */
	~Pointer_node();

	Node		*copy();
	Node_type	node_type() const { return POINTER_NODE; }
	const Cstring	&get_key() const;
	void		*get_ptr() const;
	};

//  version of Pointer_node suitable for insertion into balanced trees
class bpointernode: public Pointer_node {
public:
	bpointernode() : Pointer_node(NULL, NULL) {}
	bpointernode(void * a, back_pointer * b)
		: Pointer_node(a, b) {}
	bpointernode(const bpointernode & B) : Pointer_node(B) {}
	~bpointernode() {
		if(list) list->remove_node(this);
		list = NULL; }
	Node_type		node_type() const { return BPOINTERNODE; }
	Node		*copy() { return new bpointernode(* this); }

	bpointernode*	next_node() { return (bpointernode*) Node::next_node(); }
	bpointernode*	previous_node() { return (bpointernode*) Node::previous_node(); }
	static compare_function	compfunc() { return compare_function(compare_bpointernodes, false); }
	};


template <class PLD>
class bptrnode: public Node {
public:
	bptrnode() : payload(NULL) {}
	bptrnode(PLD* a) : payload(a) {}
	bptrnode(const bptrnode& B) : Node(B), payload(B.payload) {}
	~bptrnode() {
		if(list) list->remove_node(this);
		list = NULL; }

	PLD*		get_ptr() const { return payload; }
	PLD*		payload;

	bptrnode*	next_node() { return (bptrnode*) Node::next_node(); }
	bptrnode*	previous_node() { return (bptrnode*) Node::previous_node(); }
	static compare_function	compfunc() { return compare_function(compare_bpointernodes, false); }
};

// Class to support smart pointers.
class double_pointer_node : public bpointernode {
protected:
	pointer_to_backpointer	*pointer2;
public:
	/* The two pointers in this constructor refer to the same
	 * object, viewed from two different perspective. The first
	 * pointer looks at the object as a Node, which is the base
	 * class of the object. The second pointer points to the
	 * backpointer implementation of the object; this is where the
	 * object keeps track of all references to itself through a
	 * set of pointers - the "back pointers". The double_pointer_node
	 * has access to both incarnation of the object. */
	double_pointer_node(Node *a, pointer_to_backpointer *b);
	double_pointer_node(const double_pointer_node & D);
	~double_pointer_node();

	Node			*copy() { return new double_pointer_node(*this); }
	Node_type		node_type() const { return DOUBLE_POINTER_NODE; }
	const Cstring		&get_key() const;
	Node			*get_ptr1() { return (Node *) pointer; }
	pointer_to_backpointer	*get_ptr2();
	void			set_ptr2(pointer_to_backpointer *);
	// damages the purity of our design, but efficiency comes first...
	friend class back_pointer;
	friend class Pointer_node;
};


/* This class supports the culprit identification mechanism. Note that each
* execution thread in APGEN is reflected in a culprit_thread structure (when
* APGEN is invoked with the -culprit option). Events in the event queue (which
* are RES_timenode objects) have a pointer to this thread structure. But one
* thread can generate many events, and each event needs to store and retrieve
* culprit info that is specific to it. The solution is that the culprit_thread
* contains a list of pointer_to_pointer objects, indexed by the address of each
* RES_timenode it generates. The RES_timenode can use this pointer_to_pointer to
* store information that is specific to it. In particular: the culprit_object
* (line of adaptation code) that gave rise to it, and the amount of resource that
* was consumed (or restored) as a result of creating the event.
*/
class pointer_to_pointer: public bpointernode {
public:
	// Used by culprit-related structures to store info about threads and objects.
	pointer_to_pointer(void *w, void *d) : bpointernode(w, NULL), PTR(d), consumption(0.) {}
	// used by RES_history::get_culpritInfo().
	pointer_to_pointer(void *w, double D) : bpointernode(w, NULL), PTR(NULL), consumption(D) {}
	// copy constructor.
	pointer_to_pointer(const pointer_to_pointer &PTP)
		: bpointernode(PTP), PTR(PTP.PTR), consumption(PTP.consumption)
		{}
	~pointer_to_pointer() {}
	Node_type	node_type() const { return POINTER_TO_POINTER; }	
	Node		*copy() { return new pointer_to_pointer(*this); }
	// Used to store the amount of resource that was consumed/restored by the event (only used by get_culpritInfo)
	double		consumption;
	// Normally a pointer to a culprit_object (when culprit_option is in effect)
	void		*PTR; };

// see Stroustrop, section 6.8, for use
class List_iterator {
private:
			// list which is iterated
	const List	*list;
			// current node;
	Node		*node;
public:
	List_iterator(const List &l) { list = &l; node = NULL; }
	List_iterator(const List_iterator &li) { list = li.list; node = NULL; }

	Node		*operator()() { return next(); }
			// returns first node, then other nodes, then 0, then start from top again
	Node		*next();
			// returns last node, then other nodes, then 0, then start from end again
	Node		*previous();
			// current node = first node
	Node		*first() { node = list->first_node(); return node; }
	Node		*const_first() const { return list->first_node(); }
			// current node = last node
	Node		*last() {node = list->last_node(); return node; } };


template <class node_class>
class Alist {
public:
	List	L;
	Alist() {}
	Alist(const Alist& a) : L(a.L) {}
	~Alist() {}

	class iterator {
	    public:
		List_iterator l;
		iterator(const Alist& a) : l(a.L) {}
		~iterator() {}
		node_class*	next() { return (node_class*) l.next(); }
		node_class*	operator()() { return next(); }
	};

	Alist&	operator << (node_class* N) { L << N; return *this; }
	Alist&	operator << (Alist& l) { L << l.L; return *this; }
	void	clear() { L.clear(); }
	node_class* first_node() { return (node_class*) L.first_node(); }
	node_class* last_node() { return (node_class*) L.last_node(); }
	node_class* find(const Cstring& S) const { return (node_class*) L.find(S); }
	node_class* find(const char* S) const { return (node_class*) L.find(S); }
	int	get_length() { return L.get_length(); }
};


class breverse_tag: public bpointernode {
public:
	breverse_tag(void *a, const Cstring &s)
		: bpointernode(a, NULL),
		theTag(s) {}
	breverse_tag(const breverse_tag &B)
		: bpointernode(B),
		theTag(B.theTag) {}
	~breverse_tag() {}

	Node		*copy() { return new breverse_tag(*this); }
	Node_type	node_type() const { return BREVERSE_TAG; }
	Cstring		theTag; };

class OwnedList: public blist {
public:
	Cstring			id;
	const Cstring		&get_id() const { return id; }

	OwnedList(	const Cstring &the_name,
			void *O,
			compare_function c)
		: id(the_name), owner(O), blist(c) {
			myType = OWNEDLIST; }
private:
	OwnedList(const OwnedList &L) : blist(L) {}
public:
	~OwnedList() {}

	void			*get_owner() { return owner; }
	void			*owner;
	Node			*insert_node(Node *);
	Node			*remove_node(Node *);
	Node			*remove_node_prior_to_deletion(Node *);
				// use rmessage.is_defined() for error check
	Cstring			rmessage;
	Cstring			&get_message() { return rmessage; } };

class list_with_max_length: public blist {
private:
	long			theMaximumLength;
public:
	list_with_max_length(compare_function c) : blist(c), theMaximumLength(-1) {}
	list_with_max_length(const list_with_max_length &B)
		: blist(B), theMaximumLength(B.theMaximumLength) {}
	~list_with_max_length() {}

	long		getMaxLength() const { return theMaximumLength; }
	void		setMaxLength(long s) { theMaximumLength = s; };
	long		isAListWithMaxLength() const { return 1; } };

class clist: public blist {
public:
	clist(compare_function c) : blist(c) {}
	clist(const clist &C);
	/* Note: the base class destructor invokes virtual clear() which
		 will disable binary features to save time */
	~clist() {}

	void			order()
		{
		// already ordered!!
		}
	void			synchronize_all();
	Node			*insert_node(Node *);
	Node			*find_latest_before(Node *) const;
	Node			*find_earliest_after(Node *) const;
	Node			*earliest_node() const { return first_node(); }
	Node			*latest_node() const { return last_node(); }
	clist			&operator = (const clist &);
};

class OwnedCList: public clist {
public:
	Cstring			id;
	const Cstring&		get_id() const { return id; }

	OwnedCList(	const Cstring& the_name,
			void *O,
			compare_function c)
		: id(the_name), owner(O), clist(c) {
			myType = OWNEDCLIST;
	}
	OwnedCList(const OwnedCList &L) : clist(L) {}
	~OwnedCList() {}

	Node*			insert_node(Node*);
	Node*			remove_node(Node*);
	Node*			remove_node_prior_to_deletion(Node*);

	void			*get_owner() { return owner; }
	void			*owner;
};

// same as List_iterator, but traverses blist using compare order:
class blist_iterator {
			// list which is iterated
	const blist	*list;
			// current node;
	Node		*node;
public:
	blist_iterator(const blist &l) { list = &l; node = 0; }
	Node		*operator()() { return next(); }
	// returns first node, then other nodes, then 0, then start from top again
	Node		*next();
	// returns last node, then other nodes, then 0, then start from end again
	Node		*previous();
	// current node = first node
	Node		*first() {node = list->earliest_node(); return node;}
	// current node = last node
	Node		*last() {node = list->latest_node(); return node; } };


template <class node_class>
class Blist {
public:
	blist		L;

	Blist() : L(node_class::compfunc()) {}
	Blist(const Blist<node_class>& b) : L(b.L) {}
	~Blist() {}

	class iterator {
	    public:
		List_iterator	l;

		iterator(const Blist& a) : l(a.L) {}
		~iterator() {}

		node_class*	next() { return (node_class*) l.next(); }
		node_class*	operator()() { return next(); }
	};

	Blist&		operator << (node_class* N) { L << N; return *this; }
	void		clear() { L.clear(); }
	node_class*	first_node() { return (node_class*) L.first_node(); }
	node_class*	last_node() { return (node_class*) L.last_node(); }
	node_class*	find(const Cstring& S) const { return (node_class*) L.find(S); }
	node_class*	find(const char* S) const { return (node_class*) L.find(S); }
	int		get_length() { return L.get_length(); }
};

// To serve as auxiliary index when two Time_nodes have the same time index
class SecondaryKey {
private:
	long			theKey;
public:
	SecondaryKey(long I) : theKey(I) {}
	~SecondaryKey() {}
	long			getTheKey() const { return theKey; }
	// only these three can change theKey: one
	friend Node * blist::insert_node(Node *);
	// two
	friend Node * blist::insert_node(Node *, Node *);
	// three
	friend Node * clist::insert_node(Node *);
};

class Time_node;

// To support Time_nodes
class TimeKey {
private:
	CTime_base		theTime;
				// only works if list == NULL in the Time_node
	void			setTheTime(const CTime_base& t) {
		theTime = t;
	}
public:
	TimeKey() {}
	~TimeKey() {}
	const CTime_base&	getTheTime() const {
		return theTime;
	}
	friend void synchronize_unattached_node(
		Time_node*,
		const CTime_base& new_time);
	friend void clist::synchronize_all();

	//
	// Why does insert_binary_node need to be a friend?
	//

	friend Node* blist::insert_binary_node(Node*&);
};

extern void synchronize_unattached_node(
		Time_node*,
		const CTime_base& new_time);

class Time_node: public Node {
public:
	Time_node() : secondary_key(0L) {}
	Time_node(const Time_node &) : secondary_key(0L) {}
	// !! I forgot to do this at first:
	~Time_node() {
		if(list) {
			list->remove_node(this);
		}
	}

	//
	// these two methods set and get the "official" start time, i. e., the
	// start time as encoded in the value of the "start" symbol in the
	// object attached to the Dsource.
	//

	virtual void		setetime(
					const CTime_base & new_time) = 0;
	virtual const CTime_base& getetime() const = 0;

				// This method should be called by all blists with unsynchronized nodes:
	virtual void		synchronize_self();

			/* to support lookahead-style resource histories: */

	/* Note: no int version of integral provided, although one could
	 * conceivably define an analog based on millisecond-based "dt": */

				/* detects and reports discrepancies between the data
				* in theTime and what's reported by getetime() */
	void			get_actual_time(CTime_base &); // { return actual_time.getTheTime(); }
	bool			has_a_list() {
		return list != NULL;
	}
	unsigned long		get_secondary_key() const {
		return secondary_key.getTheKey();
	}

				//
				// These two members are public, but only
				// their friends can change their data.
				// They capture the actual time tag of the
				// Dsource objects as elements stored in a
				// time-ordered blist. Only the
				// synchronization methods are able to modify
				// them.
				//
	TimeKey			actual_time;
	SecondaryKey		secondary_key;
};

#ifdef OBSOLETE
class concrete_Time_node: public Time_node {
private:
	CTime_base		event_time;
public:
	concrete_Time_node();
	concrete_Time_node(const CTime_base &T);
	concrete_Time_node(const concrete_Time_node & ctn);
	~concrete_Time_node();

				// virtual
	void			setetime(const CTime_base & new_time);
				// virtual
	const CTime_base&	getetime() const;
				// virtual
	const Cstring&		get_key() const {
		return Cstring::null();
	}
				// virtual
	Node_type		node_type() const {
		return CONCRETE_TIME_NODE;
	}
				// virtual
	Node*			copy() {
		return new concrete_Time_node(*this);
	}
};
#endif /* OBSOLETE */

/* Node indexed by an integer key.
* Classes derived from this class are used to maintain windows of activity display real
* estate that need to be refreshed.
*/
class int_node: public Node {
protected:
				// Link to AVL parent node
	Node			*Parent;
				/* Cumulative amounts held in the 2 branches under us
				 (to be made more general later) */
	valueHolder		nCount_left;
	valueHolder		nCount_right;
				// Cumulative amount we hold
	valueHolder		myCount;

	// for disambiguating Nodes that have the same integer key
	unsigned long		secondary_key;
	void			*parentNode;
public:
	void		init_cons() {
				Parent = NULL;
				nCount_left = valueHolder(0L);
				nCount_right = valueHolder(0L);
				myCount = valueHolder(1L); }

	int_node(long i, void *parent_node = NULL)
			:_i(i),
			secondary_key(0),
			parentNode(parent_node),
			nCount_left(0L),
			nCount_right(0L),
			myCount(1L),
			Parent(NULL) {}
	int_node(const int_node &i_n)
			: _i(i_n._i),
			secondary_key(0),
			parentNode(i_n.parentNode),
			nCount_left(0L),
			nCount_right(0L),
			myCount(1L),
			Parent(NULL) {}

	~int_node();

	Node			*get_parent() const {
					return Parent; }
	void			make_root_of(blist *b) { b->root = this; Parent = NULL; }
				// consumption support
	valueHolder		get_l_cons() const {
					return nCount_left; }
	valueHolder		get_r_cons() const {
					return nCount_right; }
	valueHolder		get_i_cons() const {
					return myCount; }
	void			set_i_cons(const valueHolder *k) {
					if(k) myCount = *k;
					else myCount = valueHolder(0L); }
	void			add_cons(int which_dir, const valueHolder &the_amount, const valueHolder &) {
					if(which_dir == 0) {
						nCount_left += the_amount; }
					else if(which_dir == 1) {
						nCount_right += the_amount; } }
	void			add_cons_to_ancestors();
	void			subtract_cons_from_ancestors(Node *stop_here = NULL);
	void			attach_left(Node *N, TreeDir::cons_adjust CA = TreeDir::DO_NOT_UPDATE_CONS);
				// changes connectivity
	void			attach_right(Node *N, TreeDir::cons_adjust CA = TreeDir::DO_NOT_UPDATE_CONS);
				// changes connectivity
	void			attach(int right_or_left, Node *N, TreeDir::cons_adjust CA = TreeDir::DO_NOT_UPDATE_CONS);

	const Cstring&		get_key() const {
		return Cstring::null();
	}
	void			*get_parent_node() const {
					return parentNode; }
	Node_type		node_type() const { return INT_NODE; }
				// mandatory virtual function
	Node			*copy() {
					return new int_node(*this); }
				// The main 'data' member of this class
	long			_i;
				// Convenience function
	bool			has_a_list() {
					return list != NULL; }
	friend int compare_int_nodes(const Node *, const Node *);
	friend int compare_arrayelements(const Node *, const Node *);
	friend class blist;
	friend class clist; };

// Used to maintain windows of activity display real estate that need to be refreshed.
class win_node: public int_node {
public:
	win_node(long i, char which, int top, int bottom) : int_node(i), end_type(which), t(top), b (bottom) {}
	win_node(const win_node & wn) : int_node(wn), end_type(wn.end_type), t(wn.t), b (wn.b) {}
	~win_node() {}
	Node_type		node_type() const { return WIN_NODE; }
	Node			*copy() { return new win_node(*this); }
	char			end_type;
				// top, bottom
	int			t, b; };

// Node containing a blist
class List_object: public Node {
public:
	List_object() : object(compare_function(compare_bpointernodes, false)) {}
	List_object(const List_object & L) : object(L.object) {}
	~List_object() {}

	Node_type		node_type() const { return LIST_OBJECT; }
	const Cstring&		get_key() const {
		return Cstring::null();
	}
	Node			*copy() { return new List_object(*this); }
				// ordinarily a blist of smart bpointernodes
	blist			object; };

class Taglist: public bstringnode {
public:
	Taglist(const Cstring &s) : bstringnode(s), object() {}
	Taglist(const Taglist &L) : bstringnode(L), object(L.object) {}
	Taglist(const Cstring &n, const List &L) : bstringnode(n), object(L) {}
	~Taglist() {}

	Node_type		node_type() const { return TAGLIST; }
	Node			*copy() { return new Taglist(*this); }
				// ordinarily a blist of smart bpointernodes
	List			object; };

class Taglist_object: public bstringnode {
public:
	Taglist_object(const Cstring &s, compare_function &CF) : bstringnode(s), object(CF) {}
	Taglist_object(const Taglist_object &L) : bstringnode(L), object(L.object) {}
	Taglist_object(const Cstring &n, const blist &L) : bstringnode(n), object(L) {}
	~Taglist_object() {}

	Node_type		node_type() const { return TAGLIST_OBJECT; }
	Node			*copy() { return new Taglist_object(*this); }
				// ordinarily a blist of smart bpointernodes
	blist			object; };

// Node containing a simple List; used to parse curly brace-delimited blocks in AAF/APF files.
class block_node: public Node {
public:
	block_node() {}
	block_node(const block_node &L) : object(L.object) {}
	~block_node() {}

	Node_type	node_type() const { return BLOCK_NODE; }
	Node		*copy() { return new block_node(*this); }
	const Cstring&	get_key() const {
		return Cstring::null();
	}
			// ordinarily a List of Pointer_nodes pointing to expNodes
	List		object;
};

// Node with a string key and a pointer data member; useful for retrieving tagged data.
class btag: public bstringnode {
public:
	void			*ptr;
	btag(const Cstring & name, void *P) : bstringnode(name), ptr(P) {}
	btag(const btag & B) : bstringnode(B), ptr(B.ptr) {}
	~btag() {}

	Node_type		node_type() const { return BTAG; }
	Node			*copy() { return new btag(*this); }
	void			*get_pointer() { return ptr; } };

// Special class for handling Attributes:  has data type and attribute type:
class battribute_type_node: public bstringnode {
public:
	apgen::DATA_TYPE	data_type;
	apgen::ATTR_TYPE	attr_type;
	battribute_type_node(const Cstring & name, apgen::DATA_TYPE D, apgen::ATTR_TYPE A) : bstringnode(name), data_type(D), attr_type(A) {}
	battribute_type_node(const battribute_type_node & B) : bstringnode(B), data_type(B.data_type), attr_type(B.attr_type) {}
	~battribute_type_node() {}

	Node_type		node_type() const { return BATTRIBUTE_TYPE_NODE; }
	Node		*copy() { return new battribute_type_node(* this); }
	apgen::DATA_TYPE	get_data_type() { return data_type; }
	apgen::ATTR_TYPE	get_attr_type() { return attr_type; } };


#ifdef OBSOLETE
// to support the sasf_popup class (also used by action_request)
class sasf_pointer: public bstringnode {
public:
        sasf_pointer(const Cstring &orig_file_name) : bstringnode(orig_file_name), new_list() {}
        sasf_pointer(const sasf_pointer &SP)
                : bstringnode(SP.get_key()), new_list(SP.new_list) {}
        ~sasf_pointer() {}

	Node_type		node_type() const { return SASF_POINTER; }
	List			new_list;
        List			&get_list() { return new_list; }; };
#endif /* OBSOLETE */

// ref_node is a Node with 1 Cstring and 1 int
class ref_node: public Node {
public:
	ref_node(const char *s) : Substring(s), ID(-1L) {}
	ref_node(const char *s, long i) : Substring(s), ID(i) {}
	// note that the Substring->st is NOT NECESSARILY a valid C string
	ref_node(const Cstring & s) : Substring(s), ID(- 1) {}

	// note that the Substring->st is NOT NECESSARILY a valid C string
	ref_node(const Cstring & s, long i) : Substring(s), ID(i) {}
	ref_node(const ref_node & s) : Substring(s.Substring), ID(s.ID) {}
	ref_node() {}

	// Note: Substring automatically destroyed through Cstring destructor
	virtual ~ref_node() {}

	virtual int	get_node_type() const { return REF_NODE; }
	Node_type	node_type() const { return REF_NODE; }
	Node		*copy() { return new ref_node(Substring, ID); }
	const Cstring	&get_ref() const { return Substring; }
	const Cstring	&get_key() const { return Substring; }
	const long	&get_id() const { return ID; }
	Cstring		Substring;
	long		ID; };


#endif /* _C_LIST_H_ */
