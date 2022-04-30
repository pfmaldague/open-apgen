#if HAVE_CONFIG_H
#include <config.h>
#endif
// #define apDEBUG
#include "apDEBUG.H"

#include <iostream>
#include <iomanip>
#include <stdlib.h>

#ifdef DEBUG_BLIST
#define apDEBUG
#endif

#include "v_list.H"
#include "C_string.H"
#include <treeDir.H>

using namespace std;

// v_blist debugging stuff:
#define CERR cout
// #define CERR cerr

// #define MAXDEPTH 100
static v_Node *path[ MAXDEPTH ] ; 	//	mild violation of 'no hard limits':
					//	2**100 =~ 1,000,000,000,000,000,000,000,000,000,000
static int p_direction[ MAXDEPTH ] ;

// end of v_blist debugging stuff; see code at bottom (more or less)

// unsigned long TreeDir::LEFT_DIR = 0L;
// unsigned long TreeDir::RIGHT_DIR = 1L;
// unsigned long TreeDir::BALANCED = 2L;
// unsigned long TreeDir::UNUSED = 3L;

	/*
	 * This constructor copies the list and all of its nodes.  Each node
	 * must have method, copy() defined to create a new like node.
	 */

v_List::v_List( const v_List& list ) {
	v_Node				*node ;

	head = 0 ;
	tail = 0 ;
	length = 0 ;
	for(	node = list.first_node() ;			//for each node on right
		node ;
		node = node->next_node() ) { 
			insert_node( node->copy() ) ; } }


	/*
	 *	This function empties the list.  This method (as well as
	 *	delete_node()) make use of the virtual destructor of class v_Node.  In
	 *	order for this destructor to work properly, ALL DESCENDANTS OF NODE
	 *	SHOULD HAVE A DESTRUCTOR DECLARED, or else their members' destructors
	 *	will not be invoked.
	 */

void v_List::clear() {
	v_Node	*node ;
	v_Node	*next ;

	for( node = head; node; node = next ) {
		next = node->next_node();
		if (node->list != this) {
			cerr << "v_Node( " << (void *) node << " )'s list is not correct:\n";
			cerr << "   this v_List = " << hex << this << "\n";
			cerr << "   node's list = " << hex << (long) node->list << "\n";
			cerr << "   Next = " << hex << (long) node->next << "\n";
			cerr << "   Length = " << hex << length << "\n"; }
		delete node; }
	head = tail = 0;
	length = 0; }

v_List::~v_List() {
	clear() ; }

	/*
	 *	This function inserts the specified node at the end of the list.
	 */

v_Node *v_List::insert_node( v_Node *node ) {
	if( !node ) {
		return 0 ; }
	if( node->list ) {							//if already in a list
		node->list->remove_node( node ) ; }				//remove it
	node->list = this ;
	node->previous = tail ;							//new backward link
	node->next = 0 ;
	if( head ) {
		tail->next = node ; }						//new forward link
	else {
		head = node ; }							//new start of list
	tail = node ;								//new end of list
	length++ ;
	return node ; }


	/*
	 *	This function adds new_node to the v_List immediately after old_node.
	 *	If old_node is 0, then new_node will be inserted at the start of the
	 *	list.
	 */

v_Node *v_List::insert_node( v_Node* new_node , v_Node * old_node ) {
	if( new_node==old_node) {
		return( new_node ) ; }
	if (old_node && old_node->list!=this) return 0 ;
	if (!new_node) return 0;
	if (new_node->list) {					//if already in a list
		new_node->list->remove_node( new_node ) ; }	//remove it
	new_node->list = this;
	if (old_node == 0) {
		if (head) {
			head->previous = new_node; }
		else {
			tail = new_node; }
		new_node->next = head;
		new_node->previous = 0;
		head = new_node; }
	else {
		if (old_node->next) {
			old_node->next->previous = new_node; }
		else {
			tail = new_node; }
		new_node->next = old_node->next;
		new_node->previous = old_node;
		old_node->next = new_node ; }
	length++ ;
	return new_node ; }

	/*
	 *	This function deletes the specified node from the list and fixes the
	 *	links (see v_Node::~v_Node()).  It returns true only if the specified node is on
	 *	the specified list.  See clear().
	 */

bool v_List::delete_node( v_Node *node ) {
	if ( !node || node->list != this ) return false ;
	delete node ;
	return true ; }

	/*
	 *	This function removes a node from the list but does not delete it. 
	 *	It returns the specified node if it is on the specified list.  Otherwise
	 *	it returns 0.
	 */

v_Node *v_List::remove_node( v_Node *node ) {
	if( !node || node->list != this ) {
		cerr << "STRANGE -- attempt to remove foreign node.\n" ;
		return 0 ; }

	//if not the first node:
 	if (node->previous) {
		//fix forward link
		node->previous->next = node->next; }
	else {
		//new head
		head = node->next; }

	//if not last node
	if (node->next) 	{
		//fix backward link
		node->next->previous = node->previous; }
	else {
		//new tail
		tail = node->previous; }
	length-- ;
	node->list = 0 ;
	node->next = 0 ;
	node->previous = 0 ;
	return node ; }

v_List & v_List::operator = (const v_List& list) {
	v_Node	*node ;

	clear() ;
	for (	node = list.first_node();					//for each node on right
		node;
		node = node->next_node()) {
		insert_node(node->copy()); }					//insert a copy
	return *this ; }
	
v_blist & v_blist::operator = ( const v_blist & L ) {
	v_Node	*node ;

	if( cmp != L.cmp ) return * this ;
	clear() ;
	for (	node = L.first_node();					//for each node on right
		node;
		node = node->next_node()) {
		insert_node(node->copy()); }				//insert a copy
	return *this ; }
	
v_List & v_List::operator << ( v_List & from ) {
	v_Node	*node ;

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

	return *this; }

/*This function returns a pointer to the first node with given key (as
defined by virtual method, v_Node::get_key()).  It returns 0 if the key
is not found.*/
v_Node *v_List::find(const Cstring& target) const {
	v_Node	*node ;

	for (node = head; node; node = node->next_node()) 
		if (target == node->get_key())
			break;
	return node ; }

/*This function returns a pointer to the first node with given key (as
defined by virtual method, v_Node::get_key()).  It returns 0 if the key
is not found.*/
v_Node* v_List::find( const char * target ) const {
	v_Node	*node ;

	for (node = head; node; node = node->next_node()) 
		if ( node->get_key() == target )
			break ;
	return node ; }

/* This function returns a pointer to the first node with given key (as
** defined by virtual method, v_Node::get_key()) that follows the list node 
** 'after' of this list.  If `after' is NULL the first node of this list is
** used.  It returns NULL if the key is not found.
*/
v_Node	*v_List::find_after(const Cstring& target, v_Node* after) const {
	v_Node	*node ; 

	// make sure that after is a node of this list
	if (after)
		if (after->list == this)
			node = after->next_node();
		else
			return NULL;
	else
		node = head;

	for ( ; node; node = node->next_node()) 
		if (target == node->get_key())
			break;

	return node;
	}

// PFM: made the following operator const

v_Node * v_List::operator[] (const unsigned long i) const
/*
This operator returns a pointer to the i'th node (i = 0 => first
node).  It returns 0 if i is too large, and the first node if i is zero or negative:
*/
	{
	v_Node		*node;
	unsigned long	count;

	for (	node = head, count = 0L ;
		node && count < i ;
		node = node->next_node(), count++ )
		;
	return node ; }

long v_Node::get_index() const {
	if(!list) return -1L;
	v_Node * N = list->first_node();
	long i = 0L;
	while(N) {
		if(N == this) return i;
		i++;
		N = N->next_node(); }
	return -1L; }

v_Node::~v_Node() {
	if( list ) {
		list->remove_node( this ) ;
		list = NULL ; } }

#ifdef bpdebug
void dump_bp_list( back_pointer * bp ) {
	double_pointer_node	*ptr ;
	int			i = 0 ;

	DBG_INDENT( "BP( " << bp->get_id() << " ).list[" <<  bp->Pointerv_Nodes.get_length() << "]\n" ) ;
	for(	ptr = ( double_pointer_node * ) bp->Pointerv_Nodes.first_node() ;
		ptr ;
		ptr = ( double_pointer_node * ) ptr->next_node() ) {
		DBG_NOINDENT( i++ << ": ->" << ( ( void * ) ptr->get_ptr1() ) << "-> index " ) ;
		if( ptr->get_ptr2() ) {
			DBG_NOPREFIX( ptr->get_ptr2()->index_in_backpointer_list << endl ) ; }
		else {
			DBG_NOPREFIX( "<<NULL>>" << endl ) ; }
		cout.flush() ; }
	DBG_UNINDENT( "...DONE\n" ) ; }
#endif

v_Node *v_List_iterator::next() {
	node = node ? node->next_node() : list->first_node();
	return node; }

v_Node *v_List_iterator::previous() {
	node = node ? node->previous_node() : list->last_node(); 
	return node; }	

// BALANCED LIST (PFM)

v_blist::v_blist( const v_blist & B ) : v_List() , cmp( B.cmp ) ,
		enabled( B.enabled ) , callbacksEnabled( B.callbacksEnabled ) {
	operator = ( B ) ; }

void v_blist::order() {
	v_Node	*N = first_node() , *M ;

	if( ! root ) return ;
	disable_callbacks() ;
	while( N ) {
		M = N->next_node() ;
		v_List::remove_node( N ) ;
		N = M ; }
	order( root ) ;
	enable_callbacks() ; }

void v_blist::order( v_Node *N ) {
	v_Node	*M ;

	if((M = N->links[TreeDir::LEFT_DIR]))
		order( M ) ;
	v_List::insert_node( N ) ;
	if((M = N->links[TreeDir::RIGHT_DIR]))
		order( M ) ; }

v_Node *v_blist::earliest_node() const {
	v_Node	* N = root , * M ;

	if(!root) return NULL ;
	while((M = N->links[TreeDir::LEFT_DIR]))
		N = M ;
	return N ; }

v_Node * v_blist::latest_node() const {
	v_Node	* N = root , * M ;

	if(!root) return NULL ;
	while((M = N->links[TreeDir::RIGHT_DIR]))
		N = M ;
	return N ; }

void v_blist::clear() {
#							ifdef DEBUG_BLIST
							cout << "v_blist::clear( " << ( void * ) this << " ) START...\n" ;
#							endif
#							ifdef TRACE
							cout << ( void * ) this << " clear start\n" ;
							cout.flush() ;
#							endif

	enabled = 0 ;
	// disable_callbacks() ;
	v_List::clear() ;
	root = NULL ;
	enabled = 1 ;
	// enable_callbacks() ;
#							ifdef TRACE
							cout << ( void * ) this << " clear end\n" ;
							cout.flush() ;
#							endif
#							ifdef DEBUG_BLIST
							cout << "v_blist::clear( " << ( void * ) this << " ) DONE.\n" ;
#							endif
	}

v_List & v_blist::operator << ( v_Node * N ) {
	if ( ! insert_node( N ) ) {
		cerr << "APGEN Internal Error:  duplicate v_blist node ignored by v_List&v_blist::operator<<(v_Node*N)"
			<< endl ;
		// exit( -1 ) ;
		}
	return * this ; }

bool v_blist::operator <= (v_blist &rhs) {
	v_Node *N = first_node();

	while(N) {
		if( ! rhs.find( N ) ) return false;
		N = N->next_node(); }
	return true; }

void v_blist::add_without_duplication( v_List & l ) {
	v_Node * N = l.first_node() ;
	v_Node * M ;

	while( N ) {
		M = N->next_node() ;
		if( ! find( N ) ) insert_node( N ) ;
		N = M ; } }

v_List & v_blist::operator << ( v_List & l ) {
	v_Node * N = l.first_node() ;
	v_Node * M ;
#							ifdef DEBUG_BLIST
							cout << "operator v_blist (" << get_length() << ") <- v_List(" <<
								l.get_length() << ") START...\n" ;
#							endif

	while( N ) {
		M = N->next_node() ;
		if ( ! insert_node( N ) ) {
			cerr << "APGEN Internal Error:  duplicate v_blist node ignored by v_List&v_blist::operator<<(v_List&l)"
				<< endl ;
			// a little too drastic:
			// exit( -2 ) ;
			}
		N = M ; }
	return * this ; }

v_List & v_blist::operator << ( v_blist & l ) {
	v_Node * N = l.first_node() ;
	v_Node * M ;

#							ifdef DEBUG_BLIST
							cout << "operator v_blist (" << get_length() << ") <- v_blist(" <<
								l.get_length() << ") START...\n" ;
#							endif
	while( N ) {
		M = N->next_node() ;
		if ( ! insert_node( N ) ) {
			cerr << "APGEN Internal Error:  duplicate v_blist node ignored by v_List&v_blist::operator<<(v_blist&l)"
				<< endl ;
			// a little too drastic:
			// exit( -3 ) ;
			}
		N = M ; }
	return * this ; }

v_Node *v_blist::find(unsigned long I) const {
	v_int_node	Inn(I);

	return find(&Inn); }

v_Node *v_blist::find(void *p) const {
	return NULL; }

static		v_bstringnode std_bs ;

v_Node *v_blist::find( const char * s ) const {
	v_Node			*ret ;
	static const char	*savethis , *savethat ;

	std_bs.string.substitute( s , &savethis ) ;
	ret = find( & std_bs ) ;
	// If you don't do this the Cstring destructor will attempt to free the string:
	std_bs.string.substitute( savethis , &savethat ) ;
	return ret ; }

v_Node *v_blist::find( const Cstring & s ) const {
	v_Node			*ret ;
	static const char	*savethis , *savethat ;

	std_bs.string.substitute( *s , &savethis ) ;
	ret = find( & std_bs ) ;
	std_bs.string.substitute( savethis , &savethat ) ;
	return ret ; }

v_Node * v_Node::preceding_node() {
	v_blist		*L = ( v_blist * ) list ;

	if( ! L ) return NULL ;
	return L->find_latest_before( this ) ; }

// does NOT assume that the list is ordered (links do NOT have to agree with previous/next pointers)
v_Node * v_blist::find_latest_before( v_Node * new_node ) const {
	int	f ;
	v_Node	*current_node = root ;
	v_Node	*last_earlier_node = NULL ;

	if( ! current_node ) return NULL ;
	do {
		if ( ( f = cmp( new_node , current_node ) ) < 0 )
			current_node = current_node->links[TreeDir::LEFT_DIR] ;
		else if( ! f ) {
			if( current_node != new_node )
				return current_node ;
			else
				current_node = current_node->links[TreeDir::LEFT_DIR] ; }
		else {
			last_earlier_node = current_node ;
			current_node = current_node->links[TreeDir::RIGHT_DIR] ; }
		} while( current_node ) ;
	return last_earlier_node ; }

v_Node *v_Node::following_node() {
	v_blist		*L = ( v_blist * ) list ;

	if( ! L ) return NULL ;
	return L->find_earliest_after( this ) ; }

// does NOT assume that the list is ordered (links do NOT have to agree with previous/next pointers)
v_Node *v_blist::find_earliest_after( v_Node * new_node ) const {
	int	f ;
	v_Node	*current_node = root ;
	v_Node	*last_later_node = NULL ;

	if( ! current_node ) return NULL ;
	do {
		if ( ( f = cmp( new_node , current_node ) ) < 0 ) {
			last_later_node = current_node ;
			current_node = current_node->links[TreeDir::LEFT_DIR] ; }
		else if( ! f ) {
			if( current_node != new_node )
				return current_node ;
			else
				current_node = current_node->links[TreeDir::RIGHT_DIR] ; }
		else
			current_node = current_node->links[TreeDir::RIGHT_DIR] ;
		} while( current_node ) ;
	return last_later_node ; }

v_Node *v_blist::find( v_Node *new_node ) const {
	int	f ;
	v_Node	*current_node = root ;

	if( ! current_node ) return NULL ;
	do {
		if ( ( f = cmp( new_node , current_node ) ) < 0 )
			current_node = current_node->links[TreeDir::LEFT_DIR] ;
		else if( ! f )
			break ;
		else
			current_node = current_node->links[TreeDir::RIGHT_DIR] ;
		} while( current_node ) ;
	return current_node ; } // node already exists

v_Node *v_blist::insert_node( v_Node *N , v_Node *before_this_one ) {
	v_Node	*K = N ;

	if( N->list ) N->list->remove_node( N ) ;
	if( insert_binary_node( K ) ) {
		// see "IMPORTANT NOTE" below
		return v_List::insert_node( N , before_this_one ) ; }
	return NULL ; }

v_Node *v_blist::insert_node( v_Node *N ) {
	v_Node	*K = N ;

	if( N->list ) N->list->remove_node( N ) ;
	if( insert_binary_node( K ) ) {
		// see "IMPORTANT NOTE" below
		return v_List::insert_node( N ) ; }
	return NULL ; }

	/*

	IMPORTANT NOTE:
	---------------

	It would seem that the following function should call v_List::insert_node( v_Node * , v_Node * )
	to insert the new node at the proper position.

	The reason this is NOT done is that it often happens that one wants to (i) search a v_blist
	efficiently and (ii) maintain the (unsorted) order in which the nodes were inserted.

	If you want to insert a whole bunch of v_Nodes and end up with a sorted v_blist, call v_blist::order()
	to force agreement between the linked list and the btree.

	*/

v_Node * v_blist::insert_binary_node( v_Node * & N ) {
	int		n , i , f , dir , op_dir ;
	v_Node		* current_node , * new_node = N , * last_earlier_node = NULL ;

	// AVL BALANCED TREE ALGORITHM (see e. g. Knuth)

	if( ! root ) {
		root = new_node ;
		root->indicator = TreeDir::BALANCED ;
		root->links[TreeDir::LEFT_DIR] = NULL ;
		root->links[TreeDir::RIGHT_DIR] = NULL ;
#		ifdef PARENT_NODE
		root->parent = NULL ;
#		endif
		N = last_earlier_node ;
		return root ; }

	i = 0 ;
	current_node = root ;
	do {
		path[i] = current_node ;
		if ( ( f = cmp( new_node , current_node ) ) < 0 ) {
			current_node = current_node->links[TreeDir::LEFT_DIR] ;
			p_direction[ i++ ] = TreeDir::LEFT_DIR ; }
		else if( ! f ) {
			// node already exists
			cerr << "v_blist::insert_binary_node: attempting to insert node " << new_node->get_key()
				<< " @ " << ( ( void * ) new_node ) << " into a spot already taken by "
				<< current_node->get_key() << " @ " << ( ( void * ) current_node ) << "...\n" ;
			
			return NULL ; }
		else {
			last_earlier_node = current_node ;
			current_node = current_node->links[TreeDir::RIGHT_DIR] ;
			p_direction[ i++ ] = TreeDir::RIGHT_DIR ; }
		} while( current_node ) ;

	N = last_earlier_node ;

	path[i] = new_node ;
	p_direction[i--] = TreeDir::BALANCED ;	// wrong but useful for focal point computation
	path[i]->links[ p_direction[i] ] = new_node ;
#	ifdef PARENT_NODE
	new_node->parent = path[i] ;
#	endif
	new_node->links[TreeDir::RIGHT_DIR] = NULL ;
	new_node->links[TreeDir::LEFT_DIR] = NULL ;
	new_node->indicator = TreeDir::BALANCED ;

	// n is the number of items in the path to the new node

	n = i + 2 ;

	// look for a focal point

	f = n - 1 ;
	while( --f >= 0 && path[f]->indicator == TreeDir::BALANCED ) ;

	// path[f] is the focal point

	// CASE 1

	if( f < 0 ) {	// focal point is above the root
		while( ++f < n - 1 )
			path[f]->indicator = p_direction[f] ;
		return new_node ; }

	// CASE 2

	if( path[f]->indicator != p_direction[f] ) {
		path[f]->indicator = TreeDir::BALANCED ;
		while( ++f < n - 1 )
			path[f]->indicator = p_direction[f] ;
		return new_node ; }

	/* CASE 3

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

	dir = p_direction[f] ;
	op_dir = 1 - dir ;

	if( dir == p_direction[f + 1 ] ) {
		path[f]->links[dir] = path[f + 1]->links[op_dir] ;
#		ifdef PARENT_NODE
		if( path[f]->links[dir] != NULL )
			( path[f]->links[dir] )->parent = path[f] ;
#		endif
		path[f + 1]->links[op_dir] = path[f] ;

		for( i = f + 2 ; i < n ; i++ )
			path[i]->indicator = p_direction[i] ;

		path[f]->indicator = TreeDir::BALANCED ;

		if( ! f ) {
			root = path[1] ;
#			ifdef PARENT_NODE
			path[1]->parent = NULL ;
			path[0]->parent = path[1] ;
#			endif
			return new_node ; }
		else {
			path[f - 1]->links[p_direction[f - 1]] = path[f + 1] ;
#			ifdef PARENT_NODE
			path[f + 1]->parent = path[f - 1] ;
			path[f]->parent = path[f + 1] ;
#			endif
			return new_node ; } }

	/* MODIFIED CASES 4 AND 5

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
	if( ( f + 3 ) == n ) {
		path[f]->indicator = TreeDir::BALANCED ;
#		ifdef PARENT_NODE
		path[f]->parent = path[f + 2] ;
		path[f + 1]->parent = path[f + 2];
#		endif
		path[f + 2]->links[dir] = path[f + 1] ;
		path[f + 2]->links[op_dir] = path[f] ;
		path[f]->links[dir] = NULL ;
		path[f + 1]->links[op_dir] = NULL ;
		if( ! f )
			{
			root = path[f + 2] ;
#			ifdef PARENT_NODE
			root->parent = NULL ;
#			endif
			return new_node ;
			}
		path[f - 1]->links[p_direction[f - 1]] = path[f + 2] ;
#		ifdef PARENT_NODE
		path[f + 2]->parent = path[f - 1] ;
#		endif
		return new_node ; }

	/* CASE 4

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

	if( p_direction[f + 2] == dir ) {
		path[f]->indicator = op_dir ;
		path[f]->links[dir] = path[f + 2]->links[ op_dir ] ;
#		ifdef PARENT_NODE
		if( path[f + 2]->links[op_dir] != NULL )
 			( path[f + 2]->links[op_dir] )->parent = path[f] ;
#		endif
		path[f + 2]->links[op_dir] = path[f] ;
		path[f + 2]->links[dir] = path[f + 1] ;
		path[f + 1]->links[op_dir] = path[f + 3] ;
#		ifdef PARENT_NODE
		path[f + 1]->parent = path[f + 2] ;
		path[f + 3]->parent = path[f + 1] ;
#		endif
		for( i = f + 3 ; i < n ; i++ )
			path[i]->indicator = p_direction[i] ;
		if( ! f )
			{
			root = path[f + 2] ;
#			ifdef PARENT_NODE
			root->parent = NULL ;
			path[f]->parent = path[f + 2] ;
#			endif
			return new_node ;
			}
		path[f - 1]->links[p_direction[f - 1]] = path[f + 2] ;
#		ifdef PARENT_NODE
		path[f + 2]->parent = path[f]->parent ;
		path[f]->parent = path[f + 2] ;
#		endif
		return new_node ; }
	/* CASE 5

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

	path[f]->indicator = TreeDir::BALANCED ;
	path[f + 1]->indicator = dir ;
#	ifdef PARENT_NODE
	path[f + 3]->parent = path[f] ;
	path[f + 1]->parent = path[f + 2] ;
#	endif
	path[f]->links[dir] = path[f + 3] ;
	path[f + 2]->links[op_dir] = path[f] ;
	path[f + 1]->links[op_dir] = path[f + 2]->links[dir] ;
#	ifdef PARENT_NODE
	if( path[f + 2]->links[dir] )
		( path[f + 2]->links[dir] )->parent = path[f + 1] ;
#	endif
	path[f + 2]->links[dir] = path[f + 1] ;
	for( i = f + 3 ; i < n ; i++ )
		path[i]->indicator = p_direction[i] ;
	if( ! f ) {
		root = path[f + 2] ;
#		ifdef PARENT_NODE
		root->parent = NULL ;
		path[f]->parent = path[f + 2] ;
#		endif
		return new_node ; }
#	ifdef PARENT_NODE
	path[f + 2]->parent = path[f]->parent ;
#	endif
	path[f - 1]->links[p_direction[f - 1]] = path[f + 2] ;
#	ifdef PARENT_NODE
	path[f]->parent = path[f + 2] ;
#	endif
	return new_node ; }

v_Node * v_blist::remove_node( v_Node * N ) {
	int				i , n , f , i_old ,
						heavy_direction , search_dir , P , Q ;
	v_Node				*old_node = N , *current_node , *end_node , *save , *a , *b , *newnode , *q_link_f , *z ;

	if( ! enabled ) {
		if((current_node = v_List::remove_node(old_node))) {
			current_node->indicator = TreeDir::BALANCED ;
			current_node->links[TreeDir::RIGHT_DIR] = NULL ;
			current_node->links[TreeDir::LEFT_DIR] = NULL ; }
		if( ! get_length() ) root = NULL ;
		return current_node ; }

	// PHASE 1: SEARCH

	if( ! root )
		return NULL ;
	i = 0 ;
	current_node = root ;
	do {
		path[i] = current_node ;
		if( ( f = cmp( old_node , current_node ) ) < 0 ) {
			current_node = current_node->links[ TreeDir::LEFT_DIR ] ;
			p_direction[i++] = TreeDir::LEFT_DIR ; }
		else if( ! f )
			break ;
		else {
			current_node = current_node->links[ TreeDir::RIGHT_DIR ] ;
			p_direction[i++] = TreeDir::RIGHT_DIR ; }
		} while( current_node ) ;

	if( ! current_node )
		return NULL ;

	// DON'T FORGET TO REMOVE THE NODE:

	v_List::remove_node( current_node ) ;

	// PHASE 2: SWAP

	// not quite redundant (old_node might be a copy or something):
	// old_node = current_node ;
	if( old_node != current_node ) {
		cerr << "v_blist \"" << get_id() << "\" remove " << N->get_key() << " fatal error -- node is a copy\n" ;
		cerr << "Here is a re-enactment:\n" ;
#		ifdef apDEBUG
		cout << "v_blist \"" << get_id() << "\" remove " << N->get_key() << " fatal error -- node is a copy\n" ;
		cout << "Here is a re-enactment:\n" ;
#		endif
		i = 0 ;
		current_node = root ;
		cerr << "  root = " << root->get_key() << endl ;
#		ifdef apDEBUG
		cout << "  root = " << root->get_key() << endl ;
#		endif
		do {
			path[i] = current_node ;
			if( ( f = cmp( old_node , current_node ) ) < 0 ) {
				cerr << "  going left from " << current_node->get_key() << endl ;
#				ifdef apDEBUG
				cout << "  going left from " << current_node->get_key() << endl ;
#				endif
				current_node = current_node->links[ TreeDir::LEFT_DIR ] ;
				p_direction[i++] = TreeDir::LEFT_DIR ; }
			else if( ! f ) {
				cerr << "  found match with " << current_node->get_key() << endl ;
#				ifdef apDEBUG
				cout << "  found match with " << current_node->get_key() << endl ;
#				endif
				break ; }
			else {
				cerr << "  going right from " << current_node->get_key() << endl ;
#				ifdef apDEBUG
				cout << "  going right from " << current_node->get_key() << endl ;
#				endif
				current_node = current_node->links[ TreeDir::RIGHT_DIR ] ;
				p_direction[i++] = TreeDir::RIGHT_DIR ; }
			} while( current_node ) ;
		if( current_node ) {
			cerr << "  at the end, left with " << current_node->get_key() << endl ;
#			ifdef apDEBUG
			cout << "  at the end, left with " << current_node->get_key() << endl ;
#			endif
			}
		else {
			cerr << " current node is NULL (nothing here)\n" ;
#			ifdef apDEBUG
			cout << " current node is NULL (nothing here)\n" ;
#			endif
			}
		// a little too drastic:
		// exit( - 1 ) ;
		return NULL ; }

	i_old = i ;
	if( ( ! current_node->links[TreeDir::LEFT_DIR] ) && ( ! current_node->links[TreeDir::RIGHT_DIR] ) ) {
		// we know old_node is a leaf; no swap necessary.
		if( ! i ) {
			root = NULL ;
			return old_node ; }
		// delete the leaf; note that we don't touch the indicator:
		path[i - 1]->links[p_direction[i - 1]] = NULL ;
		n = i - 1 ; }
	else {
		v_Node * other ;
		// swap the node to be deleted with a leaf

		if( ( current_node->indicator == TreeDir::LEFT_DIR ) || ( current_node->indicator == TreeDir::BALANCED ) )
			heavy_direction = TreeDir::LEFT_DIR ;
		else
			heavy_direction = TreeDir::RIGHT_DIR ;
		search_dir = 1 - heavy_direction ;

		// move once in the heavy direction

		current_node = current_node->links[heavy_direction] ;
		p_direction[i] = heavy_direction ;
		path[++i] = current_node ;

		// then move in the direction that's not heavy

		current_node = current_node->links[search_dir] ;

		if( current_node ) {
			do {
				p_direction[i] = search_dir ;
				path[++i] = current_node ;
				current_node = current_node->links[search_dir] ; }
			while( current_node ) ;

			// now see whether there is another node hanging from the last one in the path

			end_node = path[i] ;
			// path[i - 1]->links[p_direction[i - 1]] = end_node->links[heavy_direction] ;
			// disconnect the endnode
			path[i - 1]->links[search_dir] = ( a = end_node->links[heavy_direction] ) ;
			if( ! a )
				n = i - 1 ;
			else {
				n = i ;
				path[n] = a ;
				// path[n - 1]->links[p_direction[n - 1]] = end_node->links[heavy_direction] ;
#				ifdef PARENT_NODE
				a->parent = path[i - 1] ;
#				endif
				p_direction[n] = search_dir ;
				path[n]->indicator = search_dir ; } // that's where the deleted node would have been

			// now we swap path[i_old] with the end_node

			save = path[i_old] ;
			path[i_old] = end_node ;
			end_node->links[TreeDir::LEFT_DIR] = save->links[TreeDir::LEFT_DIR] ;
#			ifdef PARENT_NODE
			if( save->links[TreeDir::LEFT_DIR] )
				save->links[TreeDir::LEFT_DIR]->parent = end_node ;
#			endif
			end_node->links[TreeDir::RIGHT_DIR] = save->links[TreeDir::RIGHT_DIR] ;
#			ifdef PARENT_NODE
			if( save->links[TreeDir::RIGHT_DIR] )
				save->links[TreeDir::RIGHT_DIR]->parent = end_node ;
#			endif
			end_node->indicator = save->indicator ; }
		else if((other = ( a = path[i] )->links[heavy_direction]))
			// could only move once. We cheat a little bit...

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
			{

			// we mimic deleting path[i_old] in its new position

			a->links[search_dir] = ( b = old_node->links[search_dir] ) ;
			a->indicator = old_node->indicator ;
			other->indicator = search_dir ;
#			ifdef PARENT_NODE
			b->parent = a ;
#			endif
			path[i_old] = a ;
			p_direction[i_old] = heavy_direction ;
			path[i] = other ;
			p_direction[i] = search_dir ;
			n = i ;
			end_node = a ;
			}
		else if((b = old_node->links[search_dir]))
			// we cheat a little bit by pretending that old_node was < a ...
			{
			// we mimic deleting path[i_old] in its new position

			a->links[search_dir] = ( b = old_node->links[search_dir] ) ;
#			ifdef PARENT_NODE
			b->parent = a ;
#			endif
			path[i_old] = a ;
			p_direction[i_old] = heavy_direction ;
			n = i_old ;
			end_node = a ; }
		else {
			path[i_old] = a ;
			p_direction[i_old] = search_dir ;
			a->indicator = search_dir ;
			n = i_old ;
			end_node = a ; }
		if( ! i_old ) {
			root = end_node ;
#			ifdef PARENT_NODE
			end_node->parent = NULL ;
#			endif
			}
		else {
			path[i_old - 1]->links[p_direction[i_old - 1]] = end_node ;
#			ifdef PARENT_NODE
			end_node->parent = path[i_old - 1] ;
#			endif
			}
		}

	// now we have to adjust the tree

	// step 1: initial adjustment


	if( path[n]->indicator == TreeDir::BALANCED ) {
		// case 1
		path[n]->indicator = 1 - p_direction[n] ;
		return old_node ; }
	else if( path[n]->indicator == p_direction[n] ) {
		// case 2
		path[n]->indicator = TreeDir::BALANCED ;
		f = n - 1 ;
		if( f < 0 )
			return old_node ;
		// else go to step 2
		}
	else {
		/* case 3: path[n] is heavy in the q_direction. Since the deleted node
		is a leaf, a = path[n]->links[q_direction] has one descendant in the p_direction
		of path[n] (situation A), one descendant in the q-direction of path[n] (situation
		B), or two descendants (situation C).
		*/
		P = p_direction[n] ;
		Q = 1 - P ;
		a = path[n]->links[Q] ;
		if( a->indicator == P ) // situation A
			{
			newnode = a->links[P] ;
			newnode->links[P] = path[n] ;
#			ifdef PARENT_NODE
			path[n]->parent = newnode ;
#			endif
			newnode->links[Q] = a ;
#			ifdef PARENT_NODE
			a->parent = newnode ;
#			endif
			a->links[P] = NULL ;
			a->indicator = TreeDir::BALANCED ;
			path[n]->links[Q] = NULL ;
			path[n]->indicator = TreeDir::BALANCED ;
			}
		else if( a->indicator == Q ) // situation B
			{
			newnode = a ;
			newnode->links[P] = path[n] ;
#			ifdef PARENT_NODE
			path[n]->parent = a ;
#			endif
			path[n]->links[Q] = NULL ;
			newnode->indicator = TreeDir::BALANCED ;
			path[n]->indicator = TreeDir::BALANCED ;
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
			b = a->links[P] ;
			b->links[P] = path[n] ;
#			ifdef PARENT_NODE
			path[n]->parent = b ;
#			endif
			a->links[P] = b ;
#			ifdef PARENT_NODE
			b->parent = a ;
#			endif
			path[n]->links[Q] = NULL ;
			b->indicator = P ;
			a->indicator = P ;
			path[n]->indicator = TreeDir::BALANCED ;
			if( ! n )
				{
				root = a ;
#				ifdef PARENT_NODE
				a->parent = NULL ;
#				endif
				return old_node ;
				}
#			ifdef PARENT_NODE
			a->parent = path[n - 1] ;
#			endif
			path[n - 1]->links[p_direction[n - 1]] = a ;
			return old_node ;
			}
		if( ! n )
			{
			root = newnode ;
#			ifdef PARENT_NODE
			newnode->parent = NULL ;
#			endif
			return old_node ;
			}
		f = n - 1 ;
		path[f]->links[p_direction[n - 1]] = newnode ;
#		ifdef PARENT_NODE
		newnode->parent = path[f] ;
#		endif
		}

	// step 2: search for a prime position

	while( 1 )
		{
		if( f < 0 )
			{
#			ifdef DEBUG_BLIST
			print() ;
#			endif
			return old_node ;
			}
		if( path[f]->indicator == TreeDir::BALANCED ) // case 1
			{
			path[f]->indicator = 1 - p_direction[f] ;
			return old_node ;
			}
		else if( path[f]->indicator == p_direction[f] ) // case 2
			{
			path[f--]->indicator = TreeDir::BALANCED ;
			}
		else		// case 3
			{

			// step 3: prime adjustment

			q_link_f = path[f]->links[1 - p_direction[f]] ;
			if( q_link_f->indicator == p_direction[f] )	// case 1
				{
				v_Node * child ;
				path[f]->links[1 - p_direction[f]] = ( child = q_link_f->step4() ) ;
#				ifdef PARENT_NODE
				child->parent = path[f] ;
#				endif
				}
			else				// case 2
				{
				if( ! f )
					{
					root = q_link_f ;
#					ifdef PARENT_NODE
					q_link_f->parent = NULL ;
#					endif
					}
				else
					{
					path[f - 1]->links[p_direction[f - 1]] = q_link_f ;
#					ifdef PARENT_NODE
					q_link_f->parent = path[f - 1] ;
#					endif
					}
				z = q_link_f->links[p_direction[f]] ;
				q_link_f->links[p_direction[f]] = path[f] ;
#				ifdef PARENT_NODE
				path[f]->parent = q_link_f ;
#				endif
				path[f]->links[1 - p_direction[f]] = z ;
#				ifdef PARENT_NODE
				z->parent = path[f] ;
#				endif
				if( q_link_f->indicator != TreeDir::BALANCED )
					{
					path[f]->indicator = TreeDir::BALANCED ;
					q_link_f->indicator = TreeDir::BALANCED ;
					f-- ;
					}
				else
					{
					path[f]->indicator = 1 - p_direction[f] ;
					q_link_f->indicator = p_direction[f] ;
					return old_node ;
					}
				}
			}
		}
	}

v_Node * v_Node::step4()
	{
	int t_direction , u_direction ;
	v_Node * new_root , * child ;

	t_direction = indicator ;
	u_direction = 1 - t_direction ;

	if( t_direction == TreeDir::BALANCED )
		return NULL ;
	if( links[t_direction]->indicator == u_direction )
		{
		links[t_direction] = ( child = links[t_direction]->step4() ) ;
#		ifdef PARENT_NODE
		child->parent = this ;
#		endif
		}
	new_root = links[t_direction] ;
	if( new_root->indicator == TreeDir::BALANCED )
		indicator = TreeDir::BALANCED ;
	else
		indicator = u_direction ;
	new_root->indicator = u_direction ;
	links[t_direction] = ( child = new_root->links[u_direction] ) ;
#	ifdef PARENT_NODE
	if( child ) child->parent = this ;
#	endif
	new_root->links[u_direction] = this ;
#	ifdef PARENT_NODE
	parent = new_root ;
#	endif
	return new_root ;
	}

#ifdef TROUBLE
static Cstring out[40] ;

void v_Node::print( int w , int start , int level ) {
	int i , M ;
	v_String_node * a ;

	if( level >= 40 ) return ;
	M = start - out[level].length() ;
	for( i = 0 ; i < M ; i++ ) out[level] << " " ;
	out[level] << get_key() ;
	if( indicator == TreeDir::LEFT_DIR ) out[level] << " l" ;
	else if( indicator == TreeDir::RIGHT_DIR ) out[level] << " r" ;
	else out[level] << " 0" ;
	if( a = ( v_String_node * ) links[TreeDir::LEFT_DIR] )
		a->print( w / 2 , start - w / 2 , level + 1 ) ;
	if( a = ( v_String_node * ) links[TreeDir::RIGHT_DIR] )
		a->print( w / 2 , start + w / 2 , level + 1 ) ; }

static int maxlen = 0 ;

void v_blist::print( v_Node * N ) const {
	int	i ;

	if( N == NULL ) N = root ;
	if( ! N ) return ;
	if( N->links[TreeDir::LEFT_DIR] ) {
		maxlen++ ;
		print( N->links[TreeDir::LEFT_DIR] ) ;
		maxlen-- ; }
	for( i = 0 ; i < maxlen ; i++ ) cout << "  " ;
	cout << N->get_key() << endl ;
	if( N->links[TreeDir::RIGHT_DIR] ) {
		maxlen++ ;
		print( N->links[TreeDir::RIGHT_DIR] ) ;
		maxlen-- ; } }

bool v_Node::check_indicator( int & path_length )
	{
	v_Node *b;
	int left = 0 , right = 0 ;

	if (b = links[TreeDir::LEFT_DIR]) {
		if( ! b->check_indicator( left ) ) return false; }
	if (b = links[TreeDir::RIGHT_DIR]) {
		if(!b->check_indicator(right)) return false; }
	if(left == right) {
		if( indicator != TreeDir::BALANCED ) {
			CERR << "node " << get_key() << "'s indicator is wrong: left = " << left
				<< ", right = " << right << ", indicator = ";
			if(indicator == TreeDir::LEFT_DIR)
				CERR << "TreeDir::LEFT\n";
			else
				CERR << "TreeDir::RIGHT\n";
			return false; }
		path_length = 1 + left;
		return true; }
	else if( left > right ) {
		if( indicator != TreeDir::LEFT_DIR ) {
			CERR << "node " << get_key() << "'s indicator is wrong: left = " << left
				<< ", right = " << right << ", indicator = " ;
			if( indicator == TreeDir::RIGHT_DIR )
				CERR << "TreeDir::RIGHT\n" ;
			else
				CERR << "TreeDir::BALANCED\n" ;
			return false; }
		path_length = 1 + left ;
		return true; }
	if( indicator != TreeDir::RIGHT_DIR ) {
		CERR << "node " << get_key() << "'s indicator is wrong: left = " << left
			<< ", right = " << right << ", indicator = " ;
		if( indicator == TreeDir::LEFT_DIR )
			CERR << "TreeDir::LEFT\n" ;
		else
			CERR << "TreeDir::BALANCED\n" ;
		return false; }
	path_length = 1 + right ;
	return true; }

bool v_Node::check_parent() {
#	ifndef PARENT_NODE
	return true;
#	else
	v_Node * b ;

	if ( b = links[TreeDir::LEFT_DIR] )
		{
		if( b->parent != this )
			{
			CERR << "node " << b->get_key() << "'s parent is not " << get_key() << endl ;
			return false;
			}
		if( ! b->check_parent() ) return false;
		}
	if ( b = links[TreeDir::RIGHT_DIR] )
		{
		if( b->parent != this )
			{
			CERR << "node " << b->get_key() << "'s parent is not " << get_key() << endl ;
			return false;
			}
		if( ! b->check_parent() ) retur false;
		}
	return true;
#	endif
	}

bool v_blist::check() {
	int len;

	if(!root) return true;
#	ifdef PARENT_NODE
	if( root->parent ) {
		CERR << "root->parent != 0\n" ;
		return false ; }

	// 1. check parent/child relationships
	if( ! root->check_parent() ) return false ;

#	endif

	// 2. check indicators
	if( ! root->check_indicator( len ) ) return false ;
	return true; }
#endif

int v_compare_v_bstringnodes( const v_Node * A , const v_Node * B ) {
	return strcmp( * A->get_key() , * B->get_key() ) ; }

int v_compare_v_int_nodes( const v_Node * a , const v_Node * b ) {
	static v_int_node		*t1 , *t2 ;

	t1 = ( v_int_node * ) a ;
	t2 = ( v_int_node * ) b ;
	if( t1->_i < t2->_i ) {
		return - 1; }
	else if( t1->_i == t2->_i ) {
		if( t1->secondary_key > t2->secondary_key ) {
			return 1 ; }
		else if( t1->secondary_key < t2->secondary_key ) {
			return - 1 ; }
		return 0 ; }
	return 1 ; }

v_aoString::v_aoString()
	: theBuffer( NULL ) ,
	theCharPointer( 0 ) ,
	theList( new v_List ) ,
	theCharHolder( NULL ) {}

v_aoString::~v_aoString() {
	delete theList ;
	if( theBuffer )
		free( theBuffer ) ;
	if( theCharHolder )
		free( theCharHolder ) ; }

v_aoString & v_aoString::operator << ( const char *S ) {
	if( theCharPointer ) {

		// 11/5/2000: this was missing
		theCharHolder[theCharPointer] = '\0' ;

		(*theList) << new v_String_node( theCharHolder ) ;
		theCharHolder[0] = '\0' ; }
	if( S && strlen( S ) ) {
		(*theList) << new v_String_node( S ) ; }
	return *this ; }

v_aoString & v_aoString::operator << ( const Cstring &S ) {
	if( theCharPointer ) {

		// 11/5/2000: this was missing
		theCharHolder[theCharPointer] = '\0' ;

		(*theList) << new v_String_node( theCharHolder ) ;
		theCharHolder[0] = '\0' ; }
	if( S.length() ) {
		(*theList) << new v_String_node( S ) ; }
	return *this ; }

unsigned long	v_aoString::theCharHolderSize = 128L;

void v_aoString::clear() {
	theList->clear() ;
	// Do NOT clear the buffer; ONLY clear the buffer inside str():
	// if( theBuffer ) {
	// 	free( theBuffer ) ;
	// 	theBuffer = NULL ; }
	if( theCharPointer ) {
		theCharPointer = 0 ;
		theCharHolder[0] = '\0' ; } }

void v_aoString::put( const char c ) {
	if( !theCharHolder ) {
		theCharPointer = 0 ;
		theCharHolder = ( char * ) malloc( theCharHolderSize + 1 ) ; }
	theCharHolder[ theCharPointer++ ] = c ;
	if( theCharPointer == theCharHolderSize ) {
		theCharHolder[ theCharPointer ] = '\0' ;
		theCharPointer = 0 ;
		(*theList) << new v_String_node( theCharHolder ) ;
		theCharHolder[0] = '\0' ; } }

char *v_aoString::str() {
	int		theLength = 0 ;
	v_List_iterator	theNodes( (*theList) ) ;
	v_Node		*N ;
	char		*c ;

	if( theBuffer ) {
		free( theBuffer ) ;
		theBuffer = NULL ; }
	while((N = theNodes())) {
		theLength += N->get_key().length() ; }
	if(theCharPointer) {
		theCharHolder[ theCharPointer ] = '\0' ;
		theLength += theCharPointer ; }
	theBuffer = ( char * ) malloc( theLength + 1 ) ;
	c = theBuffer ;
	*c = '\0' ; // Required if buffers are empty
	while((N = theNodes())) {
		strcpy( c , *N->get_key() ) ;
		c += N->get_key().length() ; }
	if(theCharPointer) {
		strcpy( c , theCharHolder ) ; }
	clear() ;
	return theBuffer ; }

bsymbolicnode::~bsymbolicnode() {
	if( insertion_history ) {
		delete insertion_history ;
		// just in case...
		insertion_history = NULL ; } }
