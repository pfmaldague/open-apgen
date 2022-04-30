lib_LTLIBRARIES = libTOLcompare.la

SUBDIRS = .

#
# Don't add more generated files; that would
# result in bison being called at least twice...
# Well, on the other hand, removing tolcomp_grammar.H
# causes errors. So never mind multiple invocations,
# as long as the make process succeeds.
#
BUILT_SOURCES = grammar.y tolcomp_grammar.C tolcomp_grammar.H

#
# NOTE: tol_system.C includes gen_tol_exp.C
#
libTOLcompare_la_SOURCES = tolcomp_grammar.C \
	tolcomp_tokens.C \
	tol_lexer_support.C \
	tol_system.C


if JSON_INSTALLED

AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags json-c` -I.

else

AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags` \
	-I../../json-c/include -I.

endif

.NOTPARALLEL:

#
# we want a reentrant scanner. Options are listed in the
# flex preamble instead of the command line now. Old version:
#       flex -Ptol -otol_tokens.C tol_tokens.l
#
# The -L option removes #line directives, which make it difficult to debug
#
tolcomp_tokens.C tolcomp_tokens.H: tol_tokens.l
	flex -L tol_tokens.l

#
# see above comments for absence of command-line options
# in the new version. Old version:
#       bison -y -b tol_grammar -ptol -otol_grammar.C -t -d -v tol_grammar.y
#
# The -l option removes #line directives, which make it difficult to debug
#
tolcomp_grammar.C tolcomp_grammar.H: grammar.y
	bison -v -l grammar.y

grammar.y: tol_language.txt constructor-specifics.txt
	../../apcore/parser_util/gramgen -indent 3 -dsl tol -f tol_language.txt -c constructor-specifics.txt -reentrant -prefix tolcomp
	mv gen-parsedExp.H gen_tol_exp.H
	mv gen-parsedExp.C gen_tol_exp.C
