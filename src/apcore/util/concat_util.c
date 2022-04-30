#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "concat_util.h"

void		(*activityInstanceSerializationHandle)(void *, buf_struct *) = NULL;
void		(*activityDefsSerializationHandle)(buf_struct *) = NULL;

int	bfs_is_empty(buf_struct *buffer) {
	if((!buffer) || (!buffer->buf) || (!buffer->buf[0])) {
		return 1; }
	return 0; }

void initialize_buf_struct(buf_struct *B) {
	if(B->bufsize) {
		B->buf[0] = '\0';
		B->stringL = 0; } }

void destroy_buf_struct(buf_struct *B) {
	if(B->buf) {
		free(B->buf);
		B->buf = NULL; }
	B->bufsize = 0;
	B->stringL = 0; }

void concatenate(buf_struct *B, const char *addendum) {
	char	*buffer = B->buf;
	int	l;

	if(!buffer) {
		B->bufsize = 120;
		B->buf = (char *) malloc(120);
		B->stringL = 0;
		B->buf[0] = '\0'; }
	if(!addendum) {
		return; }
	l = strlen(addendum);
	if(B->bufsize < B->stringL + l + 1) {
		B->bufsize = 2 * B->bufsize + l + 1;
		B->buf = (char *) realloc(B->buf, B->bufsize); }
	strcat(B->buf + B->stringL, addendum);
	B->stringL += l; }

char *add_quotes(const char *A) {
	static buf_struct	B = {NULL, 0, 0};
	static char		b[2] = {'\0','\0'};
	int			i = 0;
	int			l = strlen(A);

	initialize_buf_struct(&B);
	concatenate(&B, "\"");
	i = 0;
	while (i < l) {
		if ( A[i] == '\"' ) {
			b[0] = '\\';
			concatenate(&B, b);
			b[0] = '\"';
			concatenate(&B, b); }
		else if ( A[i] == '\\' ) {
			b[0] = '\\';
			concatenate(&B, b);
			b[0] = '\\';
			concatenate(&B, b); }
		else if( A[i] == '\n' ) {
			b[0] = '\\';
			concatenate(&B, b);
			b[0] = 'n';
			concatenate(&B, b);
			/* add an escaped newline for readability: */
			b[0] = '\\';
			concatenate(&B, b);
			b[0] = '\n';
			concatenate(&B, b); }
		else {
			b[0] = A[i];
			concatenate(&B, b); }
		i++; }
	concatenate(&B, "\"");

	return B.buf; }

char *add_quotes_no_nl(const char *A) {
	static buf_struct	B = {NULL, 0, 0};
	static char		b[2] = {'\0','\0'};
	int			i = 0;
	int			l = strlen(A);

	initialize_buf_struct(&B);
	concatenate(&B, "\"");
	i = 0;
	while (i < l) {
		if ( A[i] == '\"' ) {
			b[0] = '\\';
			concatenate(&B, b);
			b[0] = '\"';
			concatenate(&B, b); }
		else if ( A[i] == '\\' ) {
			b[0] = '\\';
			concatenate(&B, b);
			b[0] = '\\';
			concatenate(&B, b); }
		else if( A[i] == '\n' ) {
			b[0] = '\\';
			concatenate(&B, b);
			b[0] = 'n';
			concatenate(&B, b);
			/* do not add an escaped newline for readability:
			b[0] = '\\';
			concatenate(&B, b);
			b[0] = '\n';
			concatenate(&B, b);
			*/
			}
		else {
			b[0] = A[i];
			concatenate(&B, b); }
		i++; }
	concatenate(&B, "\"");

	return B.buf; }

char *remove_quotes(const char *A) {
	static buf_struct	B = {NULL, 0, 0};
	static char		b[2] = {'\0','\0'};
	int			state = 0;
	int			i = -1;

	initialize_buf_struct(&B);
	if((!A) || A[0] != '"') {
		return (char *) "remove_quotes error"; }
	do {
		i++;
		if(state == 0) {
			if(A[i] == '"') ;
			else if(A[i] == '\\') {
				state = 1; }
			else {
			b[0] = A[i];
			concatenate(&B, b); } }
		else if(state == 1) {
			if(A[i] == '"') {
				b[0] = '"';
				concatenate(&B, b); }
			else if(A[i] == 'n') {
				b[0] = '\n';
				concatenate(&B, b); }
			else if(A[i] == 't') {
				b[0] = '\t';
				concatenate(&B, b); }
			else if(A[i] == '\n') {
				; }
			else {
				b[0] = A[i];
				concatenate(&B, b); }
			state = 0; }
		} while(A[i] != '\0');
	return B.buf; }
