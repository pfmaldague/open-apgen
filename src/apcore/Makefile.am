bin_PROGRAMS = apbatch
lib_LTLIBRARIES = libAPcore.la

EXTRA_DIST = apbatch_sources

apbatch_LDADD =	libAPcore.la \
	parser/libParser.la

if USE_LIBXMLCXX

if JSON_INSTALLED
libAPcore_la_LDFLAGS = \
	@UDEF_LIBRARY_FLAG@ \
	`pkg-config libxml++-3.0 --libs json-c` \
	`pkg-config glibmm-2.4 --libs` \
	-lcurl -lz

apbatch_LDFLAGS = \
	@UDEF_LIBRARY_FLAG@ \
	`pkg-config libxml++-3.0 --libs json-c` \
	`pkg-config glibmm-2.4 --libs` \
	-export-dynamic -ldl -lcurl -lz

AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags json-c` \
	`pkg-config glibmm-2.4 --cflags` \
		-I./parser \
	      -I${top_srcdir}/json-c/include

else
libAPcore_la_LDFLAGS = \
	@UDEF_LIBRARY_FLAG@ \
	`pkg-config libxml++-3.0 --libs` \
	-L${top_srcdir}/json-c/lib -ljson-c \
	`pkg-config glibmm-2.4 --libs` \
	-lcurl -lz

apbatch_LDFLAGS = \
	@UDEF_LIBRARY_FLAG@ \
	`pkg-config libxml++-3.0 --libs` \
	-L${top_srcdir}/json-c/lib -ljson-c \
	`pkg-config glibmm-2.4 --libs` \
	-export-dynamic -ldl -lcurl -lz

AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags` \
			-I./parser \
			-I${top_srcdir}/json-c/include

endif

else

apbatch_LDFLAGS = \
	@UDEF_LIBRARY_FLAG@ \
	-export-dynamic -ldl -lcurl


if JSON_INSTALLED

AM_CPPFLAGS = `pkg-config --cflags json-c`

else

AM_CPPFLAGS = -I${top_srcdir}/json-c/include

endif

endif

if JSON_INSTALLED
AM_LDFLAGS = `pkg-config --libs json-c`
endif

libAPcore_la_LIBADD = \
	../xmlrpc/libAPxml.la \
	parser/libParser.la \
	@EXTRA_LIBS@

libAPcore_includedir = $(includedir)

SUBDIRS= parser_util parser fd .

## In final stage, add doc, test and tutorials:
## EXTRA_DIST = doc test tutorials $EXTRA_DIST

include apbatch_sources

## CFLAGS are defined in configure.in now, to make testing of X features easier.
AM_CFLAGS = @UDEF_COMPILE_FLAG@
AM_CXXFLAGS = @UDEF_COMPILE_FLAG@
.PHONY: apgen_title

apgen_title:
	echo  ' char APGEN_build_date[] = "' `date` '"; ' >> $@.tmp
	echo  ' char APGEN_build_id[] = "'@BUILDTAG@'"; ' >> $@.tmp
	mv $@.tmp APGEN_Title.C

APGEN_Title.C APGEN_Title.o: apgen_title

# -B option creates a "batch" parser, i. e., one that doesn't listen to stdin...
# -L option prevents output of #line macros which are OK when debugging parsing logic
#   but very confusing when dealing with other bugs.
#
# NOTE: do NOT use -P<new prefix> which confuses the logic of ylwrap. Use
#       an include file with the "#define yy... myname..." trick in it.
# AM_LFLAGS = leXprefiX -B -L
# AM_YFLAGS = leXprefiX
# AM_LFLAGS = -B -L
# AM_YFLAGS = -l -d -v

CLEANFILES=parser/cmd_grammar.C parser/grammar.C parser/grammar.y \
	parser/cmd_tokens.C parser/tokens.C parser/grammar.H \
	parser/cmd_grammar.H \
	parser/cmd_grammar.C parser/grammar.C \
	parser/tokens.C parser/cmd_grammar.H parser/grammar.H \
	parser/grammar.output parser/cmd_grammar.output

