#if HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef GUI
#include <Xm/Xm.h>

#ifdef DEBUG_CONVERT
#	include <stdio.h>
#endif

extern Boolean CvtStringToStringList(Display *dpy,XrmValuePtr args,
	Cardinal *num_args,XrmValuePtr from,XrmValuePtr to,XtPointer *data);

extern void StringListDestructor(XtAppContext app,XrmValuePtr to,
	XtPointer converter_data,XrmValuePtr args,Cardinal *num_args);

/* Make sure to define function prototyping */
#ifndef FUNCPROTO
#define FUNCPROTO
#endif


Boolean CvtStringToStringList(
		Display *dpy ,
		XrmValuePtr args ,
		Cardinal * num_args ,
		XrmValuePtr from ,
		XrmValuePtr to ,
		XtPointer * data )
	{
	register int i, count = 1 ;
	register char *ch, *start = from->addr ;
	static String *list ;
	int len ;

#	ifdef DEBUG_CONVERT
	fprintf( stderr , "CvtStringToStringList: START\n" ) ;
#	endif
	if( * num_args != 0 )
		{
		XtAppErrorMsg( XtDisplayToApplicationContext( dpy ),
			"cvtStringToStringList", "wrongParameters",
			"XtToolkitError",
			"String to string list conversion needs no extra arguments",
			( String * ) NULL, ( Cardinal * ) NULL ) ;
		}
#	ifdef DEBUG_CONVERT
	fprintf( stderr , "CvtStringToStringList: 1\n" ) ;
#	endif
	if( to->addr != NULL && to->size < sizeof( String * ) )
		{
		to->size = sizeof( String * ) ;
		return FALSE ;
		}
#	ifdef DEBUG_CONVERT
	fprintf( stderr , "CvtStringToStringList: 2\n" ) ;
#	endif
	if( start == NULL || *start == '\0' )
		list = NULL ;
	else
		{
		for( ch = start; *ch != '\0'; ch++ )
			{
			if ( *ch == ';' ) count++ ;
			}
		list = ( String * ) XtCalloc( count+1, sizeof( String ) ) ;

		for( i = 0; i < count; i++ )
			{
			/* for( ch = start; *ch != '\n' && *ch != '\0'; ch++ ) {} */
			for( ch = start; *ch != ';' && *ch != '\0'; ch++ ) {}
			len = ch - start ;
			list[i] = XtMalloc( len+1 ) ;
			( void ) strncpy( list[i],start,len ) ;
			list[i][len] = '\0' ;
			start = ch + 1 ;
			}
		}
#	ifdef DEBUG_CONVERT
	fprintf( stderr , "CvtStringToStringList: 3\n" ) ;
#	endif
	if( to->addr == NULL ) 
		to->addr = ( XtPointer ) &list ;
	else
		*( String ** ) to->addr = list ;
	to->size = sizeof( String * ) ;
#	ifdef DEBUG_CONVERT
	fprintf( stderr , "CvtStringToStringList: 4\n" ) ;
#	endif
	return TRUE ;
	}

void StringListDestructor( XtAppContext app,XrmValuePtr to,
	XtPointer converter_data,XrmValuePtr args,Cardinal *num_args )
	{
	String *list = ( String * )to->addr ;
	register String *entry ;

#	ifdef DEBUG_CONVERT
	fprintf( stderr , "StringListDestructor: START\n" ) ;
#	endif
	if( list == NULL ) return ;

	for( entry = list; entry != NULL; entry++ )
	XtFree( ( XtPointer ) entry ) ;

	XtFree( ( XtPointer ) list ) ;
	}
#endif
