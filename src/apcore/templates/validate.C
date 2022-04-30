#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "v_list.H"

using namespace std;

		/*
		 * v_blist of _validation_tags:
		 */
static v_blist		&theDocumentation() {
			static v_blist d(v_compare_v_int_nodes);
			return d; }

static v_blist		&theInterestingLists() {
			static v_blist i(v_compare_v_int_nodes);
			return i; }

long			Validating = 0;
long			validation_depth = 1;
long			v_indentation = 0;
long			list_operations = VALID_DISABLED;

class self : public v_int_node {
public:
	self( unsigned int j , int k ) : v_int_node( j ) , no_details_please( k ) {}
	~self() {}
	v_Node		*copy() { return new self( _i , no_details_please ); }
	int		no_details_please;
};

_validation_tag *doc_finder(doc_list l, void *what) {
	static unsigned int	_I;
	static v_blist		*which_list;

	_I = ((unsigned long) what) >> 2;
	switch(l) {
		case DOCUM_LIST:
			which_list = &theInterestingLists();
			break;
		case DOCUM_DOC:
		default:
			which_list = &theDocumentation();
			break; }
	return ( _validation_tag * ) which_list->find( _I ); }

Cstring anIDfor( _validation_tag *T ) {
	bsymbolicnode		*bs;

	// For an ACT_req, C_list imposes get_key() as the ID; this is less informative than unique_id.
	if((bs = T->find( "Unique ID" ))) {
		return Cstring( "(Unique ID = " ) + bs->equivalent + " )"; }
	else if((bs = T->find( "ID" ))) {
		return Cstring( "(ID = " ) + bs->equivalent + " )"; }
	else if((bs = T->find( "OWNER" ))) {
		return Cstring( "(OWNER = " ) + bs->equivalent + " )"; }
	else if((bs = T->find( "owner" ))) {
		return Cstring( "(owner = " ) + bs->equivalent + " )"; }
	return Cstring( "(hard-to-identify object at " ) + T->_i + " )" ; }

void clear_the_documentation_lists() {
	theDocumentation().clear() ;
	theInterestingLists().clear() ; }

void document_interesting_lists(v_aoString &a, unsigned long theAddress, const v_blist &theObjectDescription, long Indentation, v_blist &alreadyDocumented);
void document_list_and_nodes(v_aoString &a, unsigned long theAddress, const v_blist &theObjectDescription, long Indentation, v_blist &alreadyDocumented);

_validation_tag::_validation_tag(void *theObjectToDocument, docFunc theF, const v_blist &theDocData) :
	v_int_node(((unsigned long) theObjectToDocument) >> 2),
	listForDoc(theDocData),
	whoami(theF),
	original_address(((unsigned long) theObjectToDocument) >> 2),
	next_tag(NULL),
	usage_count(0L) {
	unsigned long		_I = ((unsigned long) theObjectToDocument ) >> 2;
	_validation_tag		*T;
	v_blist			*list_to_use;

	if(	theF == document_interesting_lists
		|| theF == document_list_and_nodes ) {
		list_to_use = &theInterestingLists() ;
		T = ( _validation_tag * ) theInterestingLists().find( _I ) ; }
	else {
		list_to_use = &theDocumentation() ;
		T = ( _validation_tag * ) theDocumentation().find( _I ) ; }
	if( T ) {
		bsymbolicnode	*N = T->find( "DELETED" ) ;

		if( !N ) {
			static v_aoString	S ;
			static v_blist		L1( v_compare_v_int_nodes ) ;

			S.clear() ;
			L1.clear() ;
			T->whoami( S , T->_i , T->get_the_list() , 4 , L1 ) ;
			if( list_to_use == &theInterestingLists() ) {
				cerr << "while inserting info into theInterestingLists:\n" ; }
			else if( list_to_use == &theDocumentation() ) {
				cerr << "while inserting info into theDocumentation:\n" ; }
			cerr << "STRANGE: address " << void_to_string( theObjectToDocument )
				<< " is being re-used, but there is no trace of deletion in the following object:\n"
				<< S.str() ;
			/*
			 *  stop this nonsense:
			 */
			if( Validating ) {
				Validating = 0 ;
				exit( -1 ) ; } }
		usage_count = T->usage_count + 1 ;
		/*
		 * Should make the tag unique:
		 */
		list_to_use->remove_node( T ) ;
		T->secondary_key = ( unsigned int ) ( usage_count + 1 ) ;
		T->key = Cstring( _I ) + "_" + Cstring( T->secondary_key ) ;
		next_tag = T ;
		(*list_to_use) << T ; }
	key = Cstring( _I ) + "_" + Cstring( secondary_key ) ;
	(*list_to_use) << this ; }

_validation_tag::_validation_tag( const _validation_tag &T ) :
	v_int_node( T ) ,
	listForDoc( T.listForDoc ) ,
	whoami( T.whoami ) ,
	original_address( T.original_address ) ,
	next_tag( NULL ) ,
       	usage_count( 0 ) 	{
	_validation_tag	*T1 ;
	v_blist		*list_to_use ;

	if( whoami == document_interesting_lists ) {
		list_to_use = &theInterestingLists() ;
		T1 = ( _validation_tag * ) theInterestingLists().find( _i ) ; }
	else {
		list_to_use = &theDocumentation() ;
		T1 = ( _validation_tag * ) theDocumentation().find( _i ) ; }

	if( T1 ) {
		usage_count = T1->usage_count + 1 ;
		/*
		 * Should make the tag unique:
		 */
		list_to_use->remove_node( T1 ) ;
		T1->secondary_key = ( unsigned int ) ( usage_count + 1 ) ;
		T1->key = Cstring( _i ) + "_" + Cstring( T1->secondary_key ) ;
		next_tag = T1 ;
		secondary_key = 0 ;
		(*list_to_use) << T1 ; }
	key = Cstring( _i ) + "_" + Cstring( secondary_key ) ;
	(*list_to_use) << this ; }

void indent_cerr() {
	int i ;
	for( i = 0 ; i < v_indentation ; i++ ) {
		cerr << " " ; } }

void include_leading_blanks( v_aoString &b , int k ) {
	int i ;
	for( i = 0 ; i < k + v_indentation ; i++ ) {
		b << " " ; } }

unsigned long string_to_uint(const Cstring &S) {
	unsigned long		i;

	sscanf(*S, "%ld", &i);
	return i; }

Cstring void_to_string(void *v) {
	unsigned long	i = (unsigned long) v;

	return Cstring(i >> 2); }

Cstring uint_to_string(unsigned long i) {
	return Cstring(i); }

Cstring find_list_id_of( _validation_tag *T1 ) {
	bsymbolicnode		*list_id = T1->find( "ID" ) ;

	if( !list_id ) {
		list_id = T1->find( "RELATION" ) ; }
	if( !list_id ) {
		list_id = T1->find( "PointerNodes List" ) ;
		if( list_id ) {
			_validation_tag	*T2 = ( _validation_tag * ) theDocumentation().find( list_id->address ) ; 

			if( T2 ) {
				if((list_id = T2->find("ID"))) {
					return Cstring( "PointerNodes list of " ) + list_id->equivalent ; } } } }
	if( list_id ) {
		return list_id->equivalent ; }
	return Cstring( "UNDOCUMENTED LIST" ) ; }

void document_a_Pointer_node(v_aoString &a, unsigned long theAddress, const v_blist &theObjectDescription, long Indentation, v_blist &alreadyDocumented) {
       	_validation_tag	*T;
	bsymbolicnode	*N;
	v_List_iterator	items(theObjectDescription);
	v_int_node	*me;

	if(alreadyDocumented.get_length() > 2 + validation_depth) {
		return; }
	include_leading_blanks( a , Indentation ) ;
	if( alreadyDocumented.find( theAddress ) ) {
		T = ( _validation_tag * ) theDocumentation().find( theAddress ) ;
		if( !T ) {
			T = ( _validation_tag * ) theInterestingLists().find( theAddress ) ; }

		if( T ) {
			a << "(see " << anIDfor( T ) << ")\n" ; }
		else {
			a << "(see " << theAddress << " above; a little strange: can't find info anywhere...)\n" ; }
		return ; }
	a << theAddress << " (document_a_Pointer_node):\n" ;
	alreadyDocumented << ( me = new self( theAddress , 1 ) ) ;
	if((N = (bsymbolicnode*) theObjectDescription.find("pointer as void"))) {
		include_leading_blanks( a , Indentation ) ;
		if((T = ( _validation_tag * ) theDocumentation().find( N->address ))) {
			a << "Pointer info:\n" ;
			T->whoami( a , T->_i , T->get_the_list() , Indentation + 4 , alreadyDocumented ) ; }
		else {
			a << "Undocumented Pointer at " << N->equivalent << ". Sigh...\n" ; } }
	if((N = ( bsymbolicnode * ) theObjectDescription.find("pointer as back pointer"))) {
		include_leading_blanks( a , Indentation ) ;
		if((T = (_validation_tag*) theDocumentation().find(N->address))) {
			a << "Back Pointer info:\n" ;
			T->whoami( a , T->_i , T->get_the_list() , Indentation + 4 , alreadyDocumented ) ; }
		else {
			a << "Undocumented Back Pointer at " << N->equivalent << ". Sigh...\n" ; } }
	if((N = (bsymbolicnode*) theObjectDescription.find("LIST"))) {
		include_leading_blanks(a, Indentation);
		if((T = (_validation_tag*) theInterestingLists().find(N->address))) {
			a << "LIST:\n" ;
			T->whoami( a , T->_i , T->get_the_list() , Indentation + 4 , alreadyDocumented ) ; }
		else {
			a << "Undocumented LIST at " << N->equivalent << ". Sigh...\n" ; } }
	// any remaining information:
	while((N = (bsymbolicnode*) items())) {
		if(	N->get_key() != "pointer as void"
			&& N->get_key() != "LIST"
			&& N->get_key() != "pointer as back pointer" ) {
			include_leading_blanks( a , Indentation ) ;
			a << N->get_key() << " " << N->equivalent << " " << N->address << "\n" ; } }
	delete me ; }

void document_list_and_nodes(v_aoString &a, unsigned long theAddress, const v_blist &theObjectDescription, long Indentation, v_blist &alreadyDocumented) {
	long		node_count = 0L;
       	_validation_tag	*T;
	bsymbolicnode	*N;
	v_List_iterator	items(theObjectDescription);
	v_int_node	*me;

	if(alreadyDocumented.get_length() > 2 + validation_depth) {
		return; }
	include_leading_blanks(a, Indentation);
	a << theAddress << " (document_list_and_nodes):\n";
	if(alreadyDocumented.find(theAddress)) {
		include_leading_blanks(a, Indentation);
		if((T = ( _validation_tag * ) theDocumentation().find(theAddress))) {
			a << "found address in documentation (see " << anIDfor( T ) << ")\n" ; }
		else if((T = ( _validation_tag * ) theInterestingLists().find(theAddress))) {
			a << "found address in interesting lists (see " << anIDfor( T ) << ")\n" ; }
		else {
			a << "(see " << theAddress << " above; a little strange: can't find info anywhere...)\n" ; }
		return ; }
	alreadyDocumented << ( me = new self( theAddress , 1 ) ) ;
	if((N = (bsymbolicnode*) theObjectDescription.find("OWNER"))) {
		T = (_validation_tag*) theDocumentation().find( N->address ) ;

		if( !T ) {
			T = ( _validation_tag * ) theInterestingLists().find( N->address ) ; }
		if( T ) {
			include_leading_blanks( a , Indentation ) ;
			a << "OWNER " << N->address << ":\n" ;
			T->whoami( a , T->_i , T->get_the_list() , Indentation + 4 , alreadyDocumented ) ; }
		else {
			include_leading_blanks( a , Indentation ) ;
			a << "UNDOCUMENTED OWNER at " << N->equivalent << "\n" ; } }
	while((N = ( bsymbolicnode*) items())) {
		if( N->get_key() != "OWNER" ) {
			include_leading_blanks( a , Indentation ) ;
			if( !strncmp( *N->equivalent , "NODE" , 4 ) ) {
				static _validation_tag	*T2 ;

				if((T2 = (_validation_tag*) theDocumentation().find(string_to_uint(N->get_key())))) {
					a << "Info for NODE # " << Cstring( node_count++ ) << ":\n" ;
					T2->whoami( a , T2->_i , T2->get_the_list() , Indentation + 4 , alreadyDocumented ) ; }
				else {
					a << "Undocumented NODE # " << Cstring( node_count++ ) << " at " << N->get_key() << ". Sigh...\n" ; } }
			else {
				a << N->get_key() << " " << N->equivalent << " " << N->address << "\n" ; } } }
	delete me ; }

void document_interesting_lists(v_aoString &a, unsigned long theAddress, const v_blist &theObjectDescription, long Indentation, v_blist &alreadyDocumented) {
	int		node_count = 0;
       	_validation_tag	*T;
	bsymbolicnode	*N;
	v_List_iterator	items(theObjectDescription);
	v_int_node	*me;

	if(alreadyDocumented.get_length() > 2 + validation_depth) {
		return; }
	include_leading_blanks(a, Indentation);
	a << theAddress << " (document_interesting_lists):\n";
	if( alreadyDocumented.find(theAddress) ) {
		include_leading_blanks(a, Indentation) ;
		if((T = (_validation_tag *) theDocumentation().find(theAddress))) {
			a << "found address in documentation (see " << anIDfor( T ) << ")\n"; }
		else if((T = (_validation_tag*) theInterestingLists().find(theAddress))) {
			a << "found address in interesting lists (see " << anIDfor( T ) << ")\n" ; }
		else {
			a << "(see " << theAddress << " above; a little strange: can't find info anywhere...)\n" ; }
		return ; }
	alreadyDocumented << ( me = new self( theAddress , 0 ) ) ;
	if((N = (bsymbolicnode*) theObjectDescription.find("OWNER"))) {
		T = ( _validation_tag * ) theDocumentation().find( N->address ) ;

		if( !T ) {
			T = ( _validation_tag * ) theInterestingLists().find( N->address ) ; }
		if( T ) {
			include_leading_blanks( a , Indentation ) ;
			a << "OWNER " << N->address << ":\n" ;
			T->whoami( a , T->_i , T->get_the_list() , Indentation + 4 , alreadyDocumented ) ; }
		else {
			include_leading_blanks( a , Indentation ) ;
			a << "UNDOCUMENTED OWNER at " << N->equivalent << "\n" ; } }
	while((N = ( bsymbolicnode * ) items())) {
		if( N->get_key() != "OWNER" ) {
			if( strncmp( *N->equivalent , "NODE" , 4 ) ) {
				include_leading_blanks( a , Indentation ) ;
				a << N->get_key() << " " << N->equivalent << " " << N->address << "\n" ; } } }
	delete me ; }

void document_GenericObjects(v_aoString &a, unsigned long theAddress, const v_blist &theObjectDescription, long Indentation, v_blist &alreadyDocumented) {
	bsymbolicnode	*N;
	v_List_iterator	items(theObjectDescription);
	self		*me , *him = (self *) alreadyDocumented.last_node();
       	_validation_tag	*T1;
	bsymbolicnode	*bs;

	if( alreadyDocumented.get_length() > 2 + validation_depth ) {
		return ; }
	include_leading_blanks( a , Indentation ) ;
	if( alreadyDocumented.find( theAddress ) ) {
		_validation_tag		*T = ( _validation_tag * ) theDocumentation().find( theAddress ) ;

		if( !T ) {
			T = ( _validation_tag * ) theInterestingLists().find( theAddress ) ; }

		if( T ) {
			a << "(see " << anIDfor( T ) << ")\n" ; }
		else {
			a << "(see " << theAddress << " above; a little strange: can't find info anywhere...)\n" ; }
		return ; }
	a << theAddress << " (document_GenericObjects):\n" ;
	alreadyDocumented << ( me = new self( theAddress , 1 ) ) ;
	while((N = (bsymbolicnode*) items())) {
		if(N->equivalent == "SUBLIST") {
			if((!him) || (!him->no_details_please)) {
				include_leading_blanks( a , Indentation ) ;
       				T1 = ( _validation_tag * ) theInterestingLists().find( string_to_uint( N->get_key() ) ) ;
				if( T1 ) {
					a << "SUBLIST:\n" ;
					T1->whoami( a , T1->_i , T1->get_the_list() , Indentation + 4 , alreadyDocumented ) ; }
				else {
					a << "SUBLIST at " << N->get_key() << " is NOT documented!! Damn.\n" ; } } }
		else if( N->get_key() == "LIST" ) {
			if( (!him) || (!him->no_details_please) ) {
				v_Node		*n ;

				include_leading_blanks( a , Indentation ) ;
       				T1 = ( _validation_tag * ) theInterestingLists().find( N->address ) ;
				if( T1 ) {
					a << "INSERTION HISTORY in list " << find_list_id_of( T1 ) << ":\n" ; }
				else {
					a << "INSERTION HISTORY in Undocumented list at " << N->address << ":\n" ; }
				for(	n = N->insertion_history->first_node() ;
					n ;
					n = n->next_node() ) {
					include_leading_blanks( a , Indentation + 4 ) ;
					a << n->get_key() << "\n" ; } } }
		else {
			include_leading_blanks( a , Indentation ) ;
			if( N->get_key() == "pointer as void" ) {
				if((T1 = ( _validation_tag * ) theDocumentation().find(N->address))) {
					a << "Pointer info:\n" ;
					T1->whoami( a , T1->_i , T1->get_the_list() , Indentation + 4 , alreadyDocumented ) ; }
				else {
					a << "Undocumented Pointer at " << N->equivalent << ". Sigh...\n" ; } }
			else {
				a << N->get_key() << " " << N->equivalent << " " << N->address << "\n" ; } } }
		delete me ; }
