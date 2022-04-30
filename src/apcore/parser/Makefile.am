lib_LTLIBRARIES = libParser.la

# MAKE SURE SUBDIRS APPEARS WITH NOTHING IN FRONT!!! Damn Perl or whatever

SUBDIRS = .

libParser_la_SOURCES = grammar.C tokens.C cmd_grammar.C cmd_tokens.C

if JSON_INSTALLED

AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags json-c`

else

AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags` \
	-I../../json-c/include

endif

.NOTPARALLEL:

# tokens's yy variables are NOT renamed
tokens.C: tokens.l grammar.H
	flex -otokens.C tokens.l

# grammar's yy variables are NOT renamed. However, yylex is redefined
# in grammar.y so the lexer support code can intercept calls to lex.
grammar.C grammar.H: grammar.y
	bison -y -b grammar -pab -ogrammar.C -l -t -d -v grammar.y

grammar.y: syntax/skeleton.txt constructors/constructor-specifics.txt
	../parser_util/gramgen -f syntax/skeleton.txt -c constructors/constructor-specifics.txt
	mv gen-parsedExp.H ../../apgenlib
	mv gen-parsedExp.C ../interpreter

cmd_tokens.C: cmd_tokens.l cmd_grammar.H
	flex -Pww -ocmd_tokens.C cmd_tokens.l

cmd_grammar.C cmd_grammar.H: cmd_grammar.y
	bison -y -b cmd_grammar -ocmd_grammar.C -l -pzz -t -d -v cmd_grammar.y

EXTRA_DIST = skeleton.txt constructor-specifics.txt tokens.l \
	cmd_grammar.y cmd_tokens.l
