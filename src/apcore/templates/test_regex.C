#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <regex.h>
#include <stdio.h>

extern "C" {
#include <concat_util.h>
}

int Usage(char* s) {
	fprintf(stderr, "Usage: %s <regular expression>\n", s);
	return -1; }

int test_regex(int argc, char* argv[]) {
	if(argc != 2) {
		return Usage(argv[0]); }

	regex_t		pattern_buffer;
	size_t		num_matched;
	int		ret_val = regcomp(&pattern_buffer, argv[1], REG_EXTENDED);
	buf_struct	buf = {NULL, 0, 0};
	char		a[2];
	regmatch_t	matches[1];

	if(ret_val) {
		char	errb[1001];
		int L = regerror(ret_val, &pattern_buffer, errb, 1000);
		errb[1000] = '\0';
		fprintf(stderr, "Reg exp compilation error: %s\n", errb);
		return -1; }
	a[1] = '\0';
	while((a[0] = getchar()) != EOF) {
		if(a[0] == '\n') {
			break; }
		concatenate(&buf, a); }
	printf("string: %s pattern to match: %s\n", buf.buf, argv[1]);
	ret_val = regexec(&pattern_buffer, buf.buf, 1, matches, 0);
	if(ret_val) {
		char	errb[1001];
		int L = regerror(ret_val, &pattern_buffer, errb, 1000);
		errb[1000] = '\0';
		fprintf(stderr, "Reg exp matching error: %s\n", errb);
		return -1; }
	printf("pattern matched: ");
	for(int i = matches[0].rm_so; i < matches[0].rm_eo; i++) {
		printf("%c", buf.buf[i]); }
	printf("\n");
	regfree(&pattern_buffer);
	return 0; }
