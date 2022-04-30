#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <unistd.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <memory>
#include <stdlib.h>


#include "AP_exp_eval.H"
#include "APdata.H"
#include "C_string.H"
#include "C_list.H"
#include "RES_eval.H"
#include "apDEBUG.H"

#define ARG_POINTER args.size()
#define ARG_POP args.pop_front()
#define ARG_STACK slst<TypedValue*>& args
#define WHERE_TO_STOP 0

#define list_array_type 1
#define struct_array_type 2
#define SET_SYMBOL( x , y , z ) x->set_typed_val(y);

using namespace apgen;

extern void	removeQuotes( Cstring & s ) ;
extern void	add_to_Func_list( const char * , generic_function P ) ;

#define MaxNumReportFiles 100

ofstream*	ReportFile[MaxNumReportFiles];
char		ReportFileName[MaxNumReportFiles][1024];

// @@@ START FILE_IO

RETURN_STATUS	open_ReportFile( Cstring & errs , TypedValue * , ARG_STACK ) {
    TypedValue*	n1; // "ReportFileName"
    int		i=0;
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
	errs << "  wrong number of parameters passed to user-defined function "
	    << "open_ReportFile; expected 1, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }
    n1 = ARG_POP ;
    //cout << "Got here " << *n1->get_string() << "\n";
    if( ! n1->is_string() ) {
	errs << "open_ReportFile: first argument is not a string\n" ;
	return RETURN_STATUS::FAIL ;
    }

    for( i=0; i<MaxNumReportFiles; i++){

	if ( strcmp(ReportFileName[i],(const char *)*n1->get_string()) == 0 ){
	    errs << "Error opening " << n1->get_string() << " File already open\n";
	    return RETURN_STATUS::FAIL;
	}
    }
    //cout << "Got here 1 " << *n1->get_string() << "\n";

    for( i=0; i<MaxNumReportFiles; i++){
      //cout << "Got here 2" << " " << i << " "<< *n1->get_string() << "\n";

	if ( strcmp(ReportFileName[i],"") == 0 ){

	    //cout << "Got here 3" << " " << i << " "<< ReportFileName[i] << "\n";
	    strcpy( ReportFileName[i],( char *)*n1->get_string());
	    //cout << "Got here 4" << " " << i << " "<< ReportFileName[i] << "\n";
	    ReportFile[i] = new ofstream(ReportFileName[i]);
	    //cout << "Opening report..." << i <<" " << ReportFile[i] <<"\n";
	    return RETURN_STATUS::SUCCESS ;
	}
    }
    errs << "Error opening " << n1->get_string() << " too many report files open\n";
    return RETURN_STATUS::FAIL ;
}

RETURN_STATUS	write_to_ReportFile( Cstring & errs , TypedValue *res , ARG_STACK ) {
   TypedValue	*n1; // "string"
    Cstring		temp ;
    // const char	*t ;
    // char		*s ;
    List		fragments;
    String_node	* N ;
    int i,j;
    errs.undefine() ;

    // debug
    //cout << "Entered write_to_ReportFile\n";

    if( ARG_POINTER < ( WHERE_TO_STOP + 2 ) ) {
	errs << "  wrong number of parameters passed to user-defined "
	     << "function write_to_ReportFile; expected at least 2, got "
	     << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return apgen::RETURN_STATUS::FAIL;
    }

    //
    // Get the name of the report file
    //
    n1 = ARG_POP ;
    if(n1->get_type() != apgen::DATA_TYPE::STRING) {

	//
	// Incorrect - arguments are popped in LIFO order,
	// so we start with the last argument, which is
	// the name of the file to write the report to
	//
	// errs << "write_to_ReportFile: first argument is not a string\n" ;
	errs << "write_to_ReportFile: last argument (file name) is not a string\n" ;
	return apgen::RETURN_STATUS::FAIL;
    }

    //
    // Get the ofstream pointer for this file name
    //
    j = -1;
    for( i=0; i<MaxNumReportFiles; i++){
	if ( strcmp(ReportFileName[i], (const char *)*n1->get_string()) == 0 ){
	    j = i;
	}
    }

    if (j == -1) {
	errs << "Could not find an open report file named '" << *n1->get_string() << "'\n";
	return RETURN_STATUS::FAIL ;
    }

    // debug
    //cout << "Found " << *n1->get_string() << " Index = " << j << "\n";

    if ( ReportFile[j] == NULL ) {
	errs << "write_to_ReportFile: file \"" << n1->get_string() << "\" was not opened\n ";
	return apgen::RETURN_STATUS::FAIL ;
    }

    while(ARG_POINTER > WHERE_TO_STOP) {

	//
	// Remember that arguments are popped in LIFO order
	//
	n1 = ARG_POP;
	temp.undefine();

	//
	// Serialize this argument using the
	// TypedValue::to_string() method
	//
	temp = n1->to_string();


	//
	// to_string() always encloses string content
	// in double quotes, so it's ready to use
	// in adaptation code. But here, we _only_
	// want the string content, not a quoted
	// string suitable for insertion into an
	// adaptation. So, we unquote the string.
	//
	if(n1->get_type() == DATA_TYPE::STRING) {
		removeQuotes(temp);
	}

	//
	// The old code had a filtering loop here, to
	// eliminate escaped characters. That's probably
	// going back to the days before the TypedValue
	// to_string() method was available; this method
	// does everything that's necessary.
	//

	fragments << new String_node(temp);
    }
    //cout << "Writing " << ReportFileName[j] << " " <<ReportFile[j] <<"\n";
    for(  N = (String_node*) fragments.last_node();
	  N;
	  N = (String_node*) N->previous_node()) {
	*ReportFile[j] << N->get_key();
    }

    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	close_ReportFile( Cstring & errs , TypedValue * , ARG_STACK ) {
    TypedValue* n1; // "ReportFileName"
    int		i=0;
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
	errs << "  wrong number of parameters passed to user-defined "
	    << "function close_ReportFile; expected 1, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }
    n1 = ARG_POP ;
    //cout << "Got here " << *n1->get_string() << "\n";
    if( ! n1->is_string() ) {
	errs << "close_ReportFile: first argument is not a string\n" ;
	return RETURN_STATUS::FAIL ;
    }

    for( i=0; i<MaxNumReportFiles; i++){

	if ( strcmp(ReportFileName[i],(const char *)*n1->get_string()) == 0 ){
	    //cout << "Closing " << ReportFileName[i] << ReportFile[i] <<"\n";
	    ReportFile[i]->close();
	    strcpy( ReportFileName[i],"");
	    return RETURN_STATUS::SUCCESS;
	}
    }

    //cout << "Error closing " << n1->get_string() << " report file not open\n";
    //return RETURN_STATUS::FAIL ;
    return RETURN_STATUS::SUCCESS;
}

ofstream *slewReportFile;
RETURN_STATUS	open_slewReportFile( Cstring & errs , TypedValue * , ARG_STACK ) {
    //	  cout << "Opening viewperiod report...\n";
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
		errs << "  wrong number of parameters passed to user-defined function open_slewReportFile; expected 0, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
    }

    slewReportFile = new ofstream("slewReportFile");

    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	close_slewReportFile( Cstring & errs , TypedValue * , ARG_STACK ) {
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
	errs << "  wrong number of parameters passed to user-defined "
	    << "function close_slewReportFile; expected 0, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }

    slewReportFile->close();
    return RETURN_STATUS::SUCCESS ;
}
RETURN_STATUS	write_to_slewReportFile( Cstring & errs , TypedValue *res , ARG_STACK ) {
	TypedValue	* n1 ; // "string"
	Cstring		temp ;
	char		* C ;
	int		C_length = 20 ;
	const char	* t ;
	char		* s ;
	int		state = 0 ;
	List		fragments;
	String_node	* N ;

	errs.undefine() ;
	if( ARG_POINTER < ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function write_to_slewReportFile; expected at least 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if ( slewReportFile == NULL ){
	  return RETURN_STATUS::SUCCESS ;
	}
	C = ( char * ) malloc( C_length + 1 ) ;
	while( ARG_POINTER > WHERE_TO_STOP )
		{
		n1 = ARG_POP ;
		temp.undefine() ;
		temp = n1->to_string() ;
		if( n1->get_type() == DATA_TYPE::STRING ) removeQuotes( temp ) ;
		if( C_length < temp.length() )
			{
			C_length = 2 * temp.length() ;
			C = ( char * ) realloc( C , C_length + 1 ) ;
			}
		s = C ;
		state = 0 ;
		for( t = *temp ; *t ; t++ )
			{
			if( ! state )
				{
				if( *t == '\\' )
					state = 1 ;
				else
					*s++ = *t ;
				}
			else
				{
				if( *t == 'n' )
					*s++ = '\n' ;
				else if( *t == 't' )
					*s++ = '\t' ;
				else if( *t == '\"' )
					*s++ = '\"' ;
				else if( *t == '\\' )
					*s++ = '\\' ;
				else
					*s++ = *t ;
				state = 0 ;
				}
			}
		*s = '\0' ;
		fragments << new String_node( C ) ;
		}
	for(	N = ( String_node * ) fragments.last_node() ;
		N ;
		N = ( String_node * ) N->previous_node() )
		*slewReportFile << N->get_key() ;
	free( C ) ;

	return RETURN_STATUS::SUCCESS ;
}

ofstream *specDirectionsFile;
RETURN_STATUS	open_specDirectionsFile( Cstring & errs , TypedValue * , ARG_STACK ) {
	  //	  cout << "Opening viewperiod report...\n";
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function open_specDirectionsFile; expected 0, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}

	    specDirectionsFile = new ofstream("specDirectionsFile");

	    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	close_specDirectionsFile( Cstring & errs , TypedValue * , ARG_STACK ) {
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function close_specDirectionsFile; expected 0, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}

	specDirectionsFile->close();
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	write_to_specDirectionsFile( Cstring & errs , TypedValue *res , ARG_STACK ) {
	TypedValue	* n1 ; // "string"
	Cstring		temp ;
	char		* C ;
	int		C_length = 20 ;
	const char	* t ;
	char		* s ;
	int		state = 0 ;
	List		fragments;
	String_node	* N ;

	errs.undefine() ;
	if( ARG_POINTER < ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function write_to_specDirectionsFile; expected at least 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	C = ( char * ) malloc( C_length + 1 ) ;
	while( ARG_POINTER > WHERE_TO_STOP )
		{
		n1 = ARG_POP ;
		temp.undefine() ;
		temp = n1->to_string() ;
		if( n1->get_type() == DATA_TYPE::STRING ) removeQuotes( temp ) ;
		if( C_length < temp.length() )
			{
			C_length = 2 * temp.length() ;
			C = ( char * ) realloc( C , C_length + 1 ) ;
			}
		s = C ;
		state = 0 ;
		for( t = *temp ; *t ; t++ )
			{
			if( ! state )
				{
				if( *t == '\\' )
					state = 1 ;
				else
					*s++ = *t ;
				}
			else
				{
				if( *t == 'n' )
					*s++ = '\n' ;
				else if( *t == 't' )
					*s++ = '\t' ;
				else if( *t == '\"' )
					*s++ = '\"' ;
				else if( *t == '\\' )
					*s++ = '\\' ;
				else
					*s++ = *t ;
				state = 0 ;
				}
			}
		*s = '\0' ;
		fragments << new String_node( C ) ;
		}
	for(	N = ( String_node * ) fragments.last_node() ;
		N ;
		N = ( String_node * ) N->previous_node() )
		*specDirectionsFile << N->get_key() ;
	free( C ) ;

	return RETURN_STATUS::SUCCESS ;
	}

ofstream *specSequenceFile;

RETURN_STATUS	open_specSequenceFile( Cstring & errs , TypedValue * , ARG_STACK ) {
	  //	  cout << "Opening viewperiod report...\n";
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function open_specSequenceFile; expected 0, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}

	    specSequenceFile = new ofstream("specSequenceFile");

	    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	close_specSequenceFile( Cstring & errs , TypedValue * , ARG_STACK ) {
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
	errs << "  wrong number of parameters passed to user-defined function close_specSequenceFile; expected 0, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }

    specSequenceFile->close();
    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	write_to_specSequenceFile( Cstring & errs , TypedValue *res , ARG_STACK ) {
    TypedValue	* n1 ; // "string"
    Cstring		temp ;
    char		* C ;
    int		C_length = 20 ;
    const char	* t ;
    char		* s ;
    int		state = 0 ;
    List		fragments;
    String_node	* N ;

    errs.undefine() ;
    if( ARG_POINTER < ( WHERE_TO_STOP + 1 ) ) {
	errs << "  wrong number of parameters passed to user-defined "
	    << "function write_to_specSequenceFile; expected at least 1, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }
    C = ( char * ) malloc( C_length + 1 ) ;
    while( ARG_POINTER > WHERE_TO_STOP ) {
	n1 = ARG_POP ;
	temp.undefine() ;
	temp = n1->to_string();
	if( n1->get_type() == DATA_TYPE::STRING ) removeQuotes( temp ) ;
	if( C_length < temp.length() ) {
	    C_length = 2 * temp.length() ;
	    C = ( char * ) realloc( C , C_length + 1 ) ;
	}
	s = C ;
	state = 0 ;
	for( t = *temp ; *t ; t++ ) {
	    if( ! state ) {
		if( *t == '\\' )
		    state = 1 ;
		else
		    *s++ = *t ;
	    } else {
		if( *t == 'n' )
		    *s++ = '\n' ;
		else if( *t == 't' )
		    *s++ = '\t' ;
		else if( *t == '\"' )
		    *s++ = '\"' ;
		else if( *t == '\\' )
		    *s++ = '\\' ;
		else
		    *s++ = *t ;
		state = 0 ;
	    }
	}
	*s = '\0' ;
	fragments << new String_node( C ) ;
    }
    for(N = ( String_node * ) fragments.last_node() ;
	N ;
	N = ( String_node * ) N->previous_node() )
	    *specSequenceFile << N->get_key() ;
    free( C );

    return RETURN_STATUS::SUCCESS ;
}

ofstream *viewperiodReportFile;

RETURN_STATUS	open_viewperiodReport( Cstring & errs , TypedValue * , ARG_STACK ) {
    //	  cout << "Opening viewperiod report...\n";
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
	errs << "  wrong number of parameters passed to user-defined function "
	    << "open_viewperiodReport; expected 0, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }

    viewperiodReportFile = new ofstream("viewperiodReport");

    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	close_viewperiodReport( Cstring & errs , TypedValue * , ARG_STACK ) {
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
	errs << "  wrong number of parameters passed to user-defined "
	    << "function close_viewperiodReport; expected 0, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }

    viewperiodReportFile->close();
    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	write_to_viewperiodReport( Cstring & errs , TypedValue *res , ARG_STACK ) {
	TypedValue	* n1 ; // "string"
	Cstring		temp ;
	char		* C ;
	int		C_length = 20 ;
	const char	* t ;
	char		* s ;
	int		state = 0 ;
	List		fragments;
	String_node	* N ;

	errs.undefine() ;
	if( ARG_POINTER < ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function write_to_viewperiodReport; expected at least 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	C = ( char * ) malloc( C_length + 1 ) ;
	while( ARG_POINTER > WHERE_TO_STOP )
		{
		n1 = ARG_POP ;
		temp.undefine() ;
		temp = n1->to_string();
		if( n1->get_type() == DATA_TYPE::STRING ) removeQuotes( temp ) ;
		if( C_length < temp.length() )
			{
			C_length = 2 * temp.length() ;
			C = ( char * ) realloc( C , C_length + 1 ) ;
			}
		s = C ;
		state = 0 ;
		for( t = *temp ; *t ; t++ )
			{
			if( ! state )
				{
				if( *t == '\\' )
					state = 1 ;
				else
					*s++ = *t ;
				}
			else
				{
				if( *t == 'n' )
					*s++ = '\n' ;
				else if( *t == 't' )
					*s++ = '\t' ;
				else if( *t == '\"' )
					*s++ = '\"' ;
				else if( *t == '\\' )
					*s++ = '\\' ;
				else
					*s++ = *t ;
				state = 0 ;
				}
			}
		*s = '\0' ;
		fragments << new String_node( C ) ;
		}
	for(	N = ( String_node * ) fragments.last_node() ;
		N ;
		N = ( String_node * ) N->previous_node() )
		*viewperiodReportFile << N->get_key() ;
	free( C ) ;

	return RETURN_STATUS::SUCCESS ;
}
ofstream* imageReportFile = NULL;

RETURN_STATUS	open_imageReport( Cstring & errs , TypedValue * , ARG_STACK ) {
    errs.undefine();
    if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
	errs << "  wrong number of parameters passed to user-defined "
	    << "function open_imageReport; expected 0, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }

    imageReportFile = new ofstream("imageReport");

    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	close_imageReport( Cstring & errs , TypedValue * , ARG_STACK ) {
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
	errs << "  wrong number of parameters passed to user-defined "
	    << "function close_imageReport; expected 0, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }

    imageReportFile->close();
    return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS	write_to_imageReport( Cstring & errs , TypedValue *res , ARG_STACK ) {
    TypedValue	* n1 ; // "string"
    Cstring		temp ;
    char		* C ;
    int		C_length = 20 ;
    const char	* t ;
    char		* s ;
    int		state = 0 ;
    List		fragments;
    String_node	* N ;

    if(!imageReportFile) {
	return RETURN_STATUS::SUCCESS;
    }

    errs.undefine() ;
    if( ARG_POINTER < ( WHERE_TO_STOP + 1 ) ) {
	errs << "  wrong number of parameters passed to user-defined "
	    << "function write_to_imageReport; expected at least 1, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }
    C = ( char * ) malloc( C_length + 1 ) ;
    while( ARG_POINTER > WHERE_TO_STOP ) {
	n1 = ARG_POP ;
	temp.undefine() ;
	temp = n1->to_string();
	if( n1->get_type() == DATA_TYPE::STRING )
	    removeQuotes( temp ) ;
	if( C_length < temp.length() ) {
	    C_length = 2 * temp.length() ;
	    C = ( char * ) realloc( C , C_length + 1 ) ;
	}
	s = C ;
	state = 0 ;
	for( t = *temp ; *t ; t++ ) {
	    if( ! state ) {
		if( *t == '\\' )
			state = 1 ;
		else
			*s++ = *t ;
	    } else {
		if( *t == 'n' )
			*s++ = '\n' ;
		else if( *t == 't' )
			*s++ = '\t' ;
		else if( *t == '\"' )
			*s++ = '\"' ;
		else if( *t == '\\' )
			*s++ = '\\' ;
		else
			*s++ = *t ;
		state = 0 ;
	    }
	}
	*s = '\0' ;
	fragments << new String_node( C ) ;
    }
    for(	N = ( String_node * ) fragments.last_node() ;
		N ;
		N = ( String_node * ) N->previous_node() ) {
	*imageReportFile << N->get_key() ;
    }
    free( C ) ;

    return RETURN_STATUS::SUCCESS ;
}

// @@@ END   FILE_IO

void	register_file_subsystem_functions() {
	udef_intfc::add_to_Func_list( "open_imageReport" ,		open_imageReport,		apgen::DATA_TYPE::UNINITIALIZED);
	udef_intfc::add_to_Func_list( "close_imageReport" ,		close_imageReport,		apgen::DATA_TYPE::UNINITIALIZED);
	udef_intfc::add_to_Func_list( "write_to_imageReport" ,		write_to_imageReport,		apgen::DATA_TYPE::UNINITIALIZED);
	udef_intfc::add_to_Func_list( "write_to_viewperiodReport",	write_to_viewperiodReport,	apgen::DATA_TYPE::UNINITIALIZED);
	udef_intfc::add_to_Func_list( "open_viewperiodReport" ,		open_viewperiodReport,		apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "close_viewperiodReport" ,	close_viewperiodReport,		apgen::DATA_TYPE::UNINITIALIZED) ;

	udef_intfc::add_to_Func_list( "open_ReportFile" ,		open_ReportFile,		apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "write_to_ReportFile" ,		write_to_ReportFile,		apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "close_ReportFile" ,		close_ReportFile,		apgen::DATA_TYPE::UNINITIALIZED) ;

	udef_intfc::add_to_Func_list( "open_slewReportFile" ,		open_slewReportFile,		apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "close_slewReportFile" ,		close_slewReportFile,		apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "write_to_slewReportFile" ,	write_to_slewReportFile,	apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "write_to_specSequenceFile" ,	write_to_specSequenceFile,	apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "open_specSequenceFile" ,		open_specSequenceFile,		apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "close_specSequenceFile" ,	close_specSequenceFile,		apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "write_to_specDirectionsFile" ,	write_to_specDirectionsFile,	apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "open_specDirectionsFile" ,	open_specDirectionsFile,	apgen::DATA_TYPE::UNINITIALIZED) ;
	udef_intfc::add_to_Func_list( "close_specDirectionsFile" ,	close_specDirectionsFile,	apgen::DATA_TYPE::UNINITIALIZED) ;
}
