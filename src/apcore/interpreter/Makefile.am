## At first, using LT (libtool) libraries sounded attractive. However,
## this is not really feasible as long as the library exhibits circular
## dependencies vis-a-vis the core executable, which is (unfortunately)
## the case now. According to libtool designers, a library should be
## self-contained or should rely on external 'sub-libraries', but should
## note rely on symbols defined on the executable which links it.
##
## This is definitely a good design principle, but the interpreter library was
## worked into APGEN 'after the fact' and it is taking time to remove
## all the circularity. As soon as circularity removal is complete, the
## LT library style will be re-instated.

lib_LTLIBRARIES = libinterpreter.la

libinterpreter_la_SOURCES = AP_exp_eval.C \
	RES_eval.C \
	ParsedExpressionSystem.C \
	ParsedExpressionExtensions.C \
	EventRegistry.C \
	BuiltInFunctions.C \
	GLOBdata.C \
	Behavior.C

if USE_LIBXMLCXX

if JSON_INSTALLED
AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags json-c`
else
AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags` \
	-I../../json-c/include
endif
else
if JSON_INSTALLED
AM_CPPFLAGS = `pkg-config --cflags json-c`
else
AM_CPPFLAGS = -I../../json-c/include
endif
endif

AM_LDFLAGS =
if JSON_INSTALLED
AM_LDFLAGS += `pkg-config --libs json-c`
endif
