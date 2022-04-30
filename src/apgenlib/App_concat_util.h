#ifndef _APP_CONCAT_UTIL_H
#define _APP_CONCAT_UTIL_H

#include <stdio.h>
#include <string.h>
#include <concat_util.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif


	/* TEXT CONCATENATION UTILITIES */

			/* NOTE: added buf_struct to avoid the
			 * need for static storage inside the
			 * function implementation, making them
			 * safe to use in pthreads */
char			*app_add_quotes(const char *A, buf_struct* B);
char			*app_add_quotes_no_nl(const char *A, buf_struct* B);
char			*app_remove_quotes(const char *A, buf_struct* B);

#endif /* _APP_CONCAT_UTIL_H */
