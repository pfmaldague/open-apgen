#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef OBSOLETE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>		/* for getenv() */
#include <sys/wait.h>		/* for wait() */
#include <unistd.h>		/* for getpid() ; */

#include "APserver.h"

#include <errno.h>		/* for the socket stuff borrowed from glib docs */
#include <sys/socket.h>		/* ditto */
#include <netinet/in.h>		/* ditto */
#include <netdb.h>		/* ditto */


				/* in main.C: */
extern int			ARGC ;
extern char			**ARGV ;
				/* in IO_seqtalk.C (apgen) or apgen_proxy.c (apgen_proxy) */
extern void			usr1_signal_callback() ;

				/* to support the -server option: */
unsigned int			shared_mem_id ;
char				*theSharedData ;
				/* semaphore ID: */
int				sem_id ;
				/* for the command utility that wants to attach itself to us: */
void				*shared_mem_loc = NULL ;
				/*
				 * consistent use of these will allow us to test signal_state
				 * through a simple == test rather than a call to strcmp():
				 */
const char			*undefined_state = "UNDEFINED" ;
const char 			*enabled_state = "ENABLED" ;
const char			*disabled_state = "DISABLED" ;

				/* Can't initialize to undefined_state here (not constant) */
const char			*signal_state ;

static int			received_signal_from_child = 0 ;
static sigset_t			newmask , oldmask ;
static  pid_t			parent_pid , pid_of_server_proxy ;

int wait_for_connection( int *sockarray , int numsock ) ;
 
/* after W. Richard Stevens' "Advanced Programming in the UNIX Environment": */
Sigfunc reliable_version_of_signal( int signo , Sigfunc func ) {
	struct sigaction        act , oact ;

	act.sa_handler = func ;
	sigemptyset( &act.sa_mask ) ;
	act.sa_flags = 0 ;
	if( signo == SIGALRM ) {
#               ifdef SA_INTERRUPT
                        act.sa_flags |= SA_INTERRUPT ;
#               endif
                }
	else {
#               ifdef SA_RESTART
			act.sa_flags |= SA_RESTART ;
#               endif
		}
	if( sigaction( signo , &act , &oact ) < 0 )
		return SIG_ERR ;
	return oact.sa_handler ; }

void sig_usr1( int k ) {
	/* Note: k is the 'signal number' as per 'man signal'. In our case it better be SIGUSR1. */
	received_signal_from_child = 1 ;
	usr1_signal_callback() ; }

int prepare_to_receive_signal_1_from_child() {
	static int	sigusr1_was_never_set = 1 ;

	received_signal_from_child = 0 ;
	if( sigusr1_was_never_set ) {
		sigusr1_was_never_set = 0 ;
		/* debug */
		fprintf( stderr , "APserver (prepare_to_receive_signal_1_from_child) enabling SIGUSR1...\n" ) ;
		if( reliable_version_of_signal( SIGUSR1 , sig_usr1 ) == SIG_ERR ) {
			fprintf( stderr , "signal( SIGUSR1 ) error in server parent\n" ) ;
			return 0 ; } }
	sigemptyset( &newmask ) ;
	sigaddset( &newmask , SIGUSR1 ) ;
	/* debug */
	fprintf( stderr , "prepare: new mask after sigaddset = %d\n" , newmask ) ;

	/*
	 * Let's IMMEDIATELY block this signal, because we are not yet ready to handle it. We WILL be
	 * ready AFTER we fork off the child/grandchild pair (the first time we communicate with
	 * the child), or AFTER we send the child the usr2 signal (when engaged in regular
	 * communications).
	 *
	 * NOTE: the oldmask business is not really needed; it is included here because mabe some day we'll trap
	 * more signals, and we want to preserve watever we were trapping before.
	 */

	if( sigprocmask( SIG_BLOCK , &newmask , &oldmask ) < 0 ) {
		fprintf( stderr , "SIG_BLOCK error in server parent\n" ) ;
		return 0 ; }

	/* debug */
	fprintf( stderr , "prepare: new mask after sigprocmask = %d\n" , newmask ) ;
	fprintf( stderr , "prepare: old mask after sigprocmask (should be 0) = %d\n" , oldmask ) ;
	return 1 ; }

int test_for_signal_1_from_child( int unblock ) {
	sigset_t	theMask ;
	int		got_it = 0 ;

	/* NOTE: oldmask was set by call to sigprocmask( SIG_BLOCK , &newmask , &oldmask )  */
	/* in handle_the_new_participant(), which should always be called first... */
	/* NOW we enable the signal, being careful not to let it fall through cracks: */
	/* if( sigsuspend( &oldmask ) != -1 ) */
	sigemptyset( &theMask ) ;
	if( sigpending( &theMask ) ) {
		/* debug */
		fprintf( stderr , "test (failure): mask after sigpending = %d\n" , theMask ) ;
		return 0 ; }
	if( sigismember( &theMask , SIGUSR1 ) ) {
		sigemptyset( &theMask ) ;
		/* debug
		fprintf( stderr , "There is a pending signal; let's intercept it..." ) ;
		 */
		sigsuspend( &theMask ) ;
		got_it = 1 ;
		/* debug
		fprintf( stderr , " got it.\n" ) ;
		 */
		}
	if( unblock ) {
		/*
		 * NOTE: according to Stevens the signal mask is set to what is was before the
		 * call to sigsuspend(), i. e., the signal is again blocked. If we are not busy crunching,
		 * this is OK; we'll get the socket interrupt soon enough.
		 *
		 * Before crunching, though, we'll have to unblock the signal, and we should block it again
		 * AFTER crunching.
		 */
		/* debug */
		fprintf( stderr , "Unblocking the SIGUSR1 signal... oldmask = (should be 0) %d\n" , oldmask ) ;
		sigprocmask( SIG_SETMASK , &oldmask , NULL ) ; }
	if( got_it ) {
		return 1 ; }
	/* debug */
	fprintf( stderr , "test_for_signal_1_from_child: no signal; theMask = %d\n" , theMask ) ;
	return 0 ; }

void enable_signals() {
	sigemptyset( &newmask ) ;
	/* debug
	fprintf( stderr , "Enabling Signals.\n" ) ;
	 */
	signal_state = enabled_state ;
	sigprocmask( SIG_SETMASK , &newmask , NULL ) ; }

void disable_signals( int k ) {
	sigemptyset( &newmask ) ;
	sigaddset( &newmask , SIGUSR1 ) ;
	/* debug
	fprintf( stderr , "Disabling Signals( %d ).\n" , k ) ;
	 */
	signal_state = disabled_state ;
	if( sigprocmask( SIG_BLOCK , &newmask , NULL ) < 0 ) {
		fprintf( stderr , "disable_signals() failure\n" ) ; } }

int wait_for_signal_1_from_child( int report_error ) {
	/* NOTE: oldmask was set by call to sigprocmask( SIG_BLOCK , &newmask , &oldmask )  */
	/* in handle_the_new_participant(), which should always be called first... */
	/* NOW we enable the signal, being careful not to let it fall through cracks: */
	if( sigsuspend( &oldmask ) != -1 ) {
		fprintf( stderr , "sigsuspend error in server parent\n" ) ;
		return 0 ; }
	/* NOTE: from "Advanced Programming etc.", the mask AFTER the call is reset to the value BEFORE the call,
	 * so we need to set things the way they should be... i. e., we want to unblock the signal...
	 *
	 * if( sigprocmask( SIG_SETMASK , &oldmask , NULL ) < 0 ) {
	 * 	fprintf( stderr , "sigprocmask error in server parent\n" ) ;
	 *	return 0 ; }
	 *
	 * Well, no... in our case we DO want to block until we get a socket message.
	 */

	/* OK, we got the SIGUSR1 signal from the child. It must be ready to receive stuff... */
	if( received_signal_from_child ) {
		/* debug */
		fprintf( stderr , "parent received usr1 signal OK\n" ) ; }
	else {
		if( report_error ) {
			/* protocol error */
			fprintf( stderr , "sig_usr1 not activated in server parent\n" ) ; }
		return 0 ; }
	return 1 ; }

/*
 * It's the client's responsibility to free the memory pointer to by *C:
 */
void get_the_full_pathname_of_the_executable( char **C ) {
        char            *path_variable = getenv( "PATH" ) ;
        /* Cstring         thePath( path_variable ) ; */
        char      	*t = path_variable ;
	char		oldc = 'A' , *s = t ;
        FILE            *tryThisFile ;
        /* Cstring         temp ; */
	const char	*u = ARGV[0] + strlen( ARGV[0] ) - 1 ;
	char		*l = NULL ;
	const char	*P = "apgen_proxy" ;
	const char	*SLASH_P = "/apgen_proxy" ;
	int		size ;

	while( u >= ARGV[0] ) {
		if( *u == '/' ) {
			break ; }
		u-- ; }
	if( *u == '/' ) {
		size = (u - ARGV[0]) + 2 + strlen( P ) ;
		l = ( char * ) malloc( size ) ;
		strncpy( l , ARGV[0] , (u - ARGV[0]) + 1 ) ;
		l[1 + (u - ARGV[0])] = '\0' ; }
	else {
		size = 1 + strlen( P ) ;
		l = ( char * ) malloc( size ) ;
		l[0] = '\0' ; }
	strcat( l , P ) ;
        while( 1 ) {
		/* debug
		fprintf( stderr , "Trying proxy \"%s\"...\n" , l ) ;
		 */
		tryThisFile = fopen( l , "r" ) ;
		if( tryThisFile ) {
			fclose( tryThisFile ) ;
			*C = l ;
			/* free( ( char * ) l ) ; */
			return ; }
		if( !path_variable ) {
			*C = NULL ;
			free( l ) ;
			return ; }
		if( !oldc )
			break ;

		/* better make sure that t is a valid pointer; that's why we checked on path_variable... */
                while( *t && *t != ':' ) {
                        t++ ; }
		oldc = *t ;
		*t = '\0' ;
		if( ( strlen( SLASH_P ) + 1 + (t - s) ) > size ) {
			size = strlen( SLASH_P ) + 1 + (t - s ) ;
			l = ( char * ) realloc( l , size ) ; }
		strcpy( l , s ) ;
		strcat( l , SLASH_P ) ;
		*t = oldc ;
		s = ++t ; }
	if( l ) free( ( char * ) l ) ;
	*C = NULL ;
	return ; }

int wait_and_lock_the_semaphore( void ) {
	static struct sembuf	theSemaphoreBuffer ;

	theSemaphoreBuffer.sem_num = 0 ;
	theSemaphoreBuffer.sem_flg = 0 ;
	theSemaphoreBuffer.sem_op = -1 ;
	if( semop( sem_id , &theSemaphoreBuffer , 1 ) == -1 ) {
		fprintf( stderr , "Semaphore operation error.\n" ) ;
		return 0 ; }
	return 1 ; }

int release_the_semaphore( void ) {
	static struct sembuf	theSemaphoreBuffer ;

	theSemaphoreBuffer.sem_num = 0 ;
	theSemaphoreBuffer.sem_flg = 0 ;
	theSemaphoreBuffer.sem_op = 1 ;
	if( semop( sem_id , &theSemaphoreBuffer , 1 ) == -1 ) {
		fprintf( stderr , "Semaphore operation error.\n" ) ;
		return 0 ; }
	return 1 ; }

int initiate_server_proxy( char *thePortToUse , int theSocketFd , int *theFileDescrForTheConnection ) {

			/* for shared memory: */
	key_t		shmem_key = ftok( "." , 'D' ) ;
	int		shmflg = IPC_CREAT | 0660 ;

			/* for the semaphore: */
	key_t		sem_key = ftok( "." , 'E' ) ;
	int		semflg = IPC_CREAT | 0660 ;
	union semun	semarg ;

	pid_t		child_pid ;
	char		buffer[80] , buf2[80] , buf3[80] ;
	char		*SERVER_PROXY ;

	/* We should do this in the definition but "the initializer element is not constant..." */
	signal_state = undefined_state ;

	/* 1. Ask for a shared memory area */
	if( ( shared_mem_id = shmget( shmem_key , SIZE_OF_SHARED_DATA , shmflg ) ) == -1 ) {
		fprintf( stderr , "initiate_server_proxy: shared memory allocation error.\n" ) ;
		return 0 ; }
	if( ( int ) ( shared_mem_loc = shmat( shared_mem_id , 0 , 0 ) ) == -1 ) {
		fprintf( stderr , "initiate_server_proxy: shared memory allocation error.\n" ) ;
		return 0 ; }

	/* 2. Ask for a semaphore */
	if( ( sem_id = semget( sem_key , 1 , semflg ) ) == -1 ) {
		fprintf( stderr , "initiate_server_proxy: semaphore allocation error.\n" ) ;
		return 0 ; }
	semarg.val = 1 ;
	semctl( sem_id , 0 , SETVAL , semarg ) ;

	parent_pid = getpid() ;

	/* let's assign strings before forking; we want to call exec() ASAP after forking, */
	/* minimizing the chances that the kernel will duplicate our address space... */
	sprintf( buffer , "%d" , shared_mem_id ) ;
	sprintf( buf2 , "%d" , parent_pid ) ;
	sprintf( buf3 , "%d" , sem_id ) ;
	get_the_full_pathname_of_the_executable( &SERVER_PROXY ) ;
	if( !SERVER_PROXY ) {
		fprintf( stderr , "initiate_server_proxy: cannot locate apgen_proxy executable.\n" ) ;
		return 0 ; }

	/* actually we don't need this right away because we need to accept the socket connection anyway;
	 * however this will be useful later.
	 */
	prepare_to_receive_signal_1_from_child() ;

	/* NOW we fork: */
	if( ( child_pid = fork() ) < 0 ) {
		fprintf( stderr , "fork error in top-level initiate_server_proxy()\n" ) ;
		return 0 ; }
	else if( !child_pid ) {	/* this is the child */
		pid_t		grandchild_pid ;

		/* NOTE: for the "double fork" to work correctly, we must avoid zombie children.
			 This REQUIRES that the parent should request the child's pid. Failure to
			to so results in zombies, which actually go away on SUN when a second
			fork takes place (to service a new participant!!!)
		*/
		if( ( grandchild_pid = fork() ) < 0 ) {
			fprintf( stderr , "fork error in child-level initiate_server_proxy()\n" ) ;
			exit( - 7 ) ; }
		else if( !grandchild_pid ) {	/* this is the grandchild */

			/* debug */
			fprintf( stderr , "    (APserver grandchild) will execl %s %s %s %s %s %s\n" ,
				SERVER_PROXY , "-proxy" , buffer , buf2 , thePortToUse , buf3 ) ;

			if( execl( SERVER_PROXY , SERVER_PROXY , "-proxy" ,
					buffer , buf2 , thePortToUse , buf3 , NULL ) ) {
				fprintf( stderr , "child-level error in spawning a new proxy; exiting\n" ) ;
				exit( 1 ) ; }
			exit( 0 ) ; }
		else {		/* this is the child */
			exit( 0 ) ; } }
	else {				/* this is the parent */
		int child_status ;

		wait( &child_status ) ;	/* wait till the CHILD terminates (exits) */
			/* debug */
			fprintf( stderr , "APserver returned from \"wait( &status )\" call.\n" ) ;
		*theFileDescrForTheConnection = wait_for_connection( &theSocketFd , 1 ) ;
			/* debug */
			fprintf( stderr , "APserver returned from \"wait_for_connection()\" call.\n" ) ;
		/* the proxy sticks its pid in the first shared memory location; let's get it: */
		pid_of_server_proxy = *( ( int * ) shared_mem_loc ) ;
			/* debug */
			fprintf( stderr , "Parent finished forking, got child acknowledgment.\n" ) ;
			fprintf( stderr , "Received pid of proxy as %d\n" , * ( int * ) shared_mem_loc ) ;
		/* we initialize the shared memory flag */
		theSharedData = GET_SHARED_DATA( shared_mem_loc ) ;
		*theSharedData = 0 ; }

	return 1 ; }


/*
 * Based on info at
 *
 * http://www.gnu.org/manual/glibc-2.2.3/html_chapter
 */

	/*
	 * The original example had an "infinite server loop". Here, we want the X event
	 * loop be interrupted for servicing the messages dealt with here. This requires
	 * some changes...
	 */
int wait_for_connection( int *array_of_sock , int numsock ) {
	fd_set			read_fd_set ;
	struct sockaddr_in 	clientname ;
	size_t			size ;
	int			new ;
	int			sock , i ;

	/* Initialize the set of active sockets. */
	FD_ZERO (&read_fd_set);
	for( i = 0 ; i < numsock ; i++ ) {
		/* debug */
		fprintf( stderr , "wait_for_connection(): select on sock %d\n" , array_of_sock[i] ) ;
		FD_SET( array_of_sock[i] , &read_fd_set ) ; }

	/* Block until input arrives on one or more active sockets. */
	if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
		  perror ("select");
		  return -1 ; }
	size = sizeof (clientname);
	for (i = 0; i < FD_SETSIZE; ++i) {
		if (FD_ISSET (i, &read_fd_set)) {
			sock = i ;
			new = accept( sock , (struct sockaddr *) &clientname , &size ) ;
			if (new < 0) {
				perror ("accept");
				return -1 ; }
			break ; } }
	/* debug */
	fprintf( stderr , "Server: connect from host %s, port %hd.\n",
		inet_ntoa (clientname.sin_addr) , ntohs (clientname.sin_port ) ) ;
	return new ; }

int wait_for_a_message( int *array_of_sock , int numsock ) {
	fd_set			read_fd_set ;
	int			i ;

	/* Initialize the set of active sockets. */
	FD_ZERO (&read_fd_set);
	for( i = 0 ; i < numsock ; i++ ) {
		/* debug */
		fprintf( stderr , "wait_for_connection(): select on sock %d\n" , array_of_sock[i] ) ;
		FD_SET( array_of_sock[i] , &read_fd_set ) ; }

	/* Block until input arrives on one or more active sockets. */
	if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
		perror ("select");
		return -1 ; }
	for (i = 0; i < FD_SETSIZE; ++i) {
		if (FD_ISSET (i, &read_fd_set)) {
			return i ; } }
	/* shouldn't get here */
	return -1 ; }
#endif /* OBSOLETE */
