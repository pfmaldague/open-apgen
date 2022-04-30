#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "App_concat_util.h"

char *app_add_quotes(const char *A, buf_struct* BS) {
	// static buf_struct	B = {NULL, 0, 0};
	char			b[2] = {'\0','\0'};
	int			i = 0;
	int			l = strlen(A);

	initialize_buf_struct(BS);
	concatenate(BS, "\"");
	i = 0;
	while (i < l) {
		if ( A[i] == '\"' ) {
			b[0] = '\\';
			concatenate(BS, b);
			b[0] = '\"';
			concatenate(BS, b); }
		else if ( A[i] == '\\' ) {
			b[0] = '\\';
			concatenate(BS, b);
			b[0] = '\\';
			concatenate(BS, b); }
		else if( A[i] == '\n' ) {
			b[0] = '\\';
			concatenate(BS, b);
			b[0] = 'n';
			concatenate(BS, b);
			/* add an escaped newline for readability: */
			b[0] = '\\';
			concatenate(BS, b);
			b[0] = '\n';
			concatenate(BS, b); }
		else {
			b[0] = A[i];
			concatenate(BS, b); }
		i++; }
	concatenate(BS, "\"");

	return BS->buf; }

char *app_add_quotes_no_nl(const char *A, buf_struct* BS) {
	// static buf_struct	B = {NULL, 0, 0};
	char			b[2] = {'\0','\0'};
	int			i = 0;
	int			l = strlen(A);

	initialize_buf_struct(BS);
	concatenate(BS, "\"");
	i = 0;
	while (i < l) {
		if ( A[i] == '\"' ) {
			b[0] = '\\';
			concatenate(BS, b);
			b[0] = '\"';
			concatenate(BS, b); }
		else if ( A[i] == '\\' ) {
			b[0] = '\\';
			concatenate(BS, b);
			b[0] = '\\';
			concatenate(BS, b); }
		else if( A[i] == '\n' ) {
			b[0] = '\\';
			concatenate(BS, b);
			b[0] = 'n';
			concatenate(BS, b);
			/* do not add an escaped newline for readability:
			b[0] = '\\';
			concatenate(BS, b);
			b[0] = '\n';
			concatenate(BS, b);
			*/
			}
		else {
			b[0] = A[i];
			concatenate(BS, b); }
		i++; }
	concatenate(BS, "\"");

	return BS->buf; }

char *app_remove_quotes(const char *A, buf_struct* BS) {
	// static buf_struct	B = {NULL, 0, 0};
	char			b[2] = {'\0','\0'};
	int			state = 0;
	int			i = -1;

	initialize_buf_struct(BS);
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
			concatenate(BS, b); } }
		else if(state == 1) {
			if(A[i] == '"') {
				b[0] = '"';
				concatenate(BS, b); }
			else if(A[i] == 'n') {
				b[0] = '\n';
				concatenate(BS, b); }
			else if(A[i] == 't') {
				b[0] = '\t';
				concatenate(BS, b); }
			else if(A[i] == '\n') {
				; }
			else {
				b[0] = A[i];
				concatenate(BS, b); }
			state = 0; }
		} while(A[i] != '\0');
	return BS->buf; }
