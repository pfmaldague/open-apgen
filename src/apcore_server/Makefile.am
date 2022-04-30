bin_PROGRAMS = APcore
lib_LTLIBRARIES = libAPserver.la

if USE_LIBXMLCXX

if JSON_INSTALLED
APcore_LDFLAGS = `pkg-config libxml++-3.0 --libs json-c` \
	-export-dynamic -ldl -lcurl
AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags json-c`
else
APcore_LDFLAGS = `pkg-config libxml++-3.0 --libs` \
	-export-dynamic -ldl -lcurl
AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags` \
	-I../json-c/include
endif

SUBDIRS= . test

else

if JSON_INSTALLED
APcore_LDFLAGS = -export-dynamic -ldl -lcurl

AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags json-c`
else
APcore_LDFLAGS = -export-dynamic -ldl -lcurl
AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags` \
	-I../json-c/include
endif

SUBDIRS= .

endif

AM_LDFLAGS=

APcore_LDADD =	\
	libAPserver.la \
	../apcore/parser/libParser.la \
	../xmlrpc/libAPxml.la \
	../apcore/libAPcore.la \
	@EXTRA_LIBS@

if JSON_INSTALLED

AM_LDFLAGS += `pkg-config --libs json-c`

else

APcore_LDADD+= ../json-c/lib/libjson-c.a

endif


libAPserver_la_SOURCES = \
	xmlrpc_intfc.C

libAPserver_la_LIBADD = \
	../xmlrpc/libAPxml.la \
	../apcore/libAPcore.la

APcore_SOURCES = \
	APcoreServer.C

## CFLAGS are defined in configure.in now, to make testing of X features easier.
AM_CFLAGS =
