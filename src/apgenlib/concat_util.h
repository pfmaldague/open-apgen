#ifndef _CONCAT_UTIL_H
#define _CONCAT_UTIL_H

#include <stdio.h>
#include <string.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
#include <stdlib.h>
#endif


	/* DATA TYPES */

// general text buffer structure (allows efficient concatenation)
typedef struct _buf_struct {
	char	*buf;
	int	bufsize;
	int	stringL; } buf_struct;

extern void	(*activityInstanceSerializationHandle)(void *, buf_struct *);
extern void	(*activityDefsSerializationHandle)(buf_struct *);

	/* TEXT CONCATENATION UTILITIES */

void			concatenate(buf_struct *B, const char *addendum);
			/* 'bfs' = buf_struct - prefix added to avoid clash with XCode on mavericks */
int			bfs_is_empty(buf_struct *buffer);
void			initialize_buf_struct(buf_struct *B);
void			destroy_buf_struct(buf_struct *B);
char			*add_quotes(const char *A);
char			*add_quotes_no_nl(const char *A);
char			*remove_quotes(const char *A);

#endif /* _CONCAT_UTIL_H */
