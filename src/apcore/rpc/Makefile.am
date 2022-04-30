bin_PROGRAMS = apvalue_client apvalue_server

# main program is in TypedValueClient.C
apvalue_client_SOURCES = \
	TypedValueClient.C \
	apvalue_util.c \
	apvalue_xdr.c \
	apvalue_clnt.c \
	apvalue_stream.c

# main program is in apvalue_svc.c
apvalue_server_SOURCES = \
	apvalue_svc.c \
	apvalue_svc_proc.c \
	apvalue_xdr.c \
	TypedValueUtil.C

# this used to be INCLUDES
AM_CPPFLAGS = -I../../apgenlib

if JSON_INSTALLED

AM_CPPFLAGS += `pkg-config --cflags json-c`
AM_LDFLAGS = `pkg-config --libs json-c`

else

AM_CPPFLAGS += -I../../json-c/include

endif

apvalue_client_LDFLAGS = `pkg-config libxml++-2.6 --libs`

apvalue_client_LDADD = \
		../libAPcore.la \
		../../xmlrpc/libAPxml.la \
		../util/libUtils.la

apvalue_server_LDFLAGS = `pkg-config libxml++-2.6 --libs`

apvalue_server_LDADD = \
		../libAPcore.la \
		../../xmlrpc/libAPxml.la \
		../util/libUtils.la

# Automatically re-generate the generated files by
# running rpcgen every time APGen is compiled
APVALUE_GEN = apvalue_xdr.c ../../apgenlib/apvalue.h apvalue_svc.c apvalue_clnt.c \
	TypedValueClient.C TypedValueUtil.C apvalue_stream.c apvalue_svc_proc.c

$(APVALUE_GEN): apvalue-regen

.NOTPARALLEL:

.PHONY:	apvalue-regen
apvalue-regen:
	rpcgen apvalue.x
	cp apvalue.h ../../apgenlib

EXTRA_DIST = apvalue.x
