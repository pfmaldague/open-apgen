#ifndef APDEBUG_H
#define APDEBUG_H

#include <iostream>

#ifdef apDEBUG
	extern int theDebugIndentation ;
	extern int enablePrint ;
	static int d_i ;

#	define COUT for( d_i = 0 ; d_i < theDebugIndentation ; d_i++ ) std::cout << " " ; std::cout
#	include <stdlib.h>
#endif

#ifdef apDEBUG
#	define DBG_NOINDENT( x ) if( enablePrint ) { COUT << x ; std::cout.flush() ; }
#	define DBG_NOPREFIX( x ) if( enablePrint ) { std::cout << x ; std::cout.flush() ; }
#	define DBG_INDENT( x ) if( enablePrint ) { if( ( theDebugIndentation = theDebugIndentation + 4 ) > 2000 ) \
		{ std::cerr << "infinite loop, bailing out.\n" ; exit( - 1 ) ; } ; \
       		COUT << "{\n" ; COUT << x ; std::cout.flush() ; }
#	define DBG_UNINDENT( x ) if( enablePrint ) { COUT << x ; COUT << "}\n" ; theDebugIndentation -= 4 ; std::cout.flush() ; }
#	define DBG_UNINDENTNOPREFIX( x ) if( enablePrint ) { std::cout << x ; COUT << "}\n" ; theDebugIndentation -= 4 ; std::cout.flush() ; }
#	define DBG_MOREINDENT() if( enablePrint ) { if( ( theDebugIndentation = theDebugIndentation + 4 ) > 2000 ) \
		{ std::cerr << "infinite loop, bailing out.\n" ; exit( - 1 ) ; } ; \
       		COUT << "{\n" ; std::cout.flush() ; }
#	define DBG_LESSINDENT() if( enablePrint ) { COUT << "}\n" ; theDebugIndentation -= 4 ; std::cout.flush() ; }
#	define DBG_PRINT_SYMBOLS( x ) if(enablePrint) { TypedSymbol     *tds ; \
		Cstring temp; \
                blist_iterator  sym(*x); \
                COUT << "symbol table:\n"; \
                while(tds = sym()) { \
                        COUT << tds->get_key() << " = "; \
                        tds->Val().print(temp); \
                        std::cout << temp << std::endl; } }
#else
#	define DBG_INDENT( x ) ; 
#	define DBG_NOINDENT( x ) ;
#	define DBG_NOPREFIX( x ) ;
#	define DBG_UNINDENTNOPREFIX( x ) ;
#	define DBG_MOREINDENT() ;
#	define DBG_LESSINDENT() ;
#	define DBG_UNINDENT( x ) ;
#	define DBG_PRINT_SYMBOLS( x ) ;
#endif

// #define DBGEVENTS
// #define debug_templates

#ifdef DBGEVENTS

#	include <iostream>
	extern int event_indent;
	extern bool inhibitDBGEVENTS;
	extern void eventindent();
	extern void eventunindent();
	extern void dbgindent();

#	ifdef apDEBUG
#	    define DBGDOC( x ) \
		DBG_NOINDENT( x )
#	    define DBGDOCPLUS( x ) \
		DBG_NOINDENT( x )
#	    define DBGDOC0( x ) \
		DBG_NOPREFIX( x )
#	else 
#	    define DBGDOC( x ) \
		dbgindent(); if(!inhibitDBGEVENTS) cout << x; cout.flush()
#	    define DBGDOC0( x ) \
		if(!inhibitDBGEVENTS) cout << x; cout.flush()
#	    define DBGDOCPLUS( x ) \
		eventindent(); dbgindent(); if(!inhibitDBGEVENTS) cout << x; eventunindent(); cout.flush()
#	endif /* apDEBUG */

#else

#	define DBGDOC( x ) \
		DBG_NOINDENT( x )
#	define DBGDOC0( x ) \
		DBG_NOPREFIX( x )
#	define DBGDOCPLUS( x ) \
		DBG_NOINDENT( x )

#endif /* DBGEVENTS */


class dbgindenter {
public:
	dbgindenter() {
		DBG_MOREINDENT();
#		ifdef DBGEVENTS
		eventindent();
#		else
#		endif /* DBGEVENTS */
		}
	~dbgindenter() {
		DBG_LESSINDENT();
#		ifdef DBGEVENTS
		eventunindent();
#		endif /* DBGEVENTS */
		}
};


#endif
