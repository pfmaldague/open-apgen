bin_PROGRAMS = tolcomp tol2json accumulate

tolcomp_SOURCES = tolcomp.C \
	process_tol.C \
	process_record.C \
	tol_comparator.C \
	tol_interpreter.C

tol2json_SOURCES = tol2json.C \
	process_tol.C \
	process_record.C \
	tol_comparator.C \
	tol_interpreter.C

accumulate_SOURCES = accumulate.C

SUBDIRS= comparison .

if JSON_INSTALLED

AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags json-c` \
	-I./comparison

else

AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags` \
	-I../../json-c/include \
	-I./comparison

endif

tolcomp_LDADD = ../apcore/libAPcore.la \
	comparison/libTOLcompare.la

tol2json_LDADD = ../apcore/libAPcore.la \
	comparison/libTOLcompare.la

CLEANFILES=comparison/tolcomp_grammar.C \
	comparison/tolcomp_grammar.H \
	comparison/tolcomp_tokens.C \
	comparison/tolcomp_tokens.H \
	comparison/tolcomp_grammar.output \
	comparison/gen_tol_exp.C \
	comparison/gen_tol_exp.H \
	comparison/grammar.y
