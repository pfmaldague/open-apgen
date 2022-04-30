/*
 * seqrclient.h
 */
#ifndef SEQRCLIENT_H_
#define SEQRCLIENT_H_
#include <stdlib.h>

/* argument points to the function to invoke to get the password */
extern void seqrclient_init(char * (*)(void));
extern void seqrclient_verbose(int v);

/* All three functions below return 1 if successful, 0 otherwise */

/* URL: "https://<server>:<port number>/tms/v4/scn/<server change number>/timeline/metadata/<schema>/" */
extern int tms_post_metadata(
			const char*	server,		/* e. g.: seqrdev2.jpl.nasa.gov */
			const char*	port,		/* e. g.: 8443 */
			const char*	scn,		/* e. g.: latest */
			const char*	schema,		/* e. g.: timr1, oreilly1, maldague1 */
			const char*	mimetype,	/* e. g.: application/json, application/xml */
			const char*	buffer		/* string to be posted */
			);

/* URL: "https://<server>:<port number>/tms/v4/scn/<server change number>/timeline/data/<schema>/" */
extern int tms_post_data(
			const char*	server,		/* e. g.: seqrdev2.jpl.nasa.gov */
			const char*	port,		/* e. g.: 8443 */
			const char*	scn,		/* e. g.: latest */
			const char*	schema,		/* e. g.: timr1, oreilly1, maldague1 */
			const char*	mimetype,	/* e. g.: application/json, application/xml */
			const char*	buffer		/* string to be posted */
			);

/* URL: "https://<server>:<port number>/tms/v4/scn/<server change number>/timeline/data/<schema>/<namespace>/<timelinename>" */
extern int tms_get(
			const char*	server,		/* e. g.: seqrdev2.jpl.nasa.gov */
			const char*	port,		/* e. g.: 8443 */
			const char*	scn,		/* e. g.: latest */
			const char*	schema,		/* e. g.: timr1, oreilly1, maldague1 */
			const char*	aNnamespace,	/* e. g.: test/apf */
			const char*	timelinename,	/* e. g.: activities */
			const char*	mimetype,	/* e. g.: application/json, application/xml */
			size_t		(*)(void *,size_t,size_t,void *) /* callback func to handle the returned data */
			);

#endif

