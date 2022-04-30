bin_PROGRAMS = \
	apcmd

# apcmd_LDFLAGS = \
# 	../libAPserver.la \
# 	../../xmlrpc/libAPxml.la \
# 	../../apcore/libAPcore.la \
# 	`pkg-config gdome2 --libs` -ldl

AM_CPPFLAGS = -I../../apgenlib 
apcmd_LDFLAGS = 
AM_LDFLAGS = 

if USE_LIBXMLCXX

apcmd_LDFLAGS += \
	../libAPserver.la \
 	../../xmlrpc/libAPxml.la \
	../../apcore/libAPcore.la  \
	../../apcore/parser/libParser.la 

if JSON_INSTALLED

AM_CPPFLAGS += `pkg-config libxml++-3.0 --cflags json-c`
apcmd_LDFLAGS += `pkg-config libxml++-3.0 --libs json-c`
AM_LDFLAGS += `pkg-config --libs json-c`

else

AM_CPPFLAGS += `pkg-config libxml++-3.0 --cflags` \
				-I../../json-c/include
apcmd_LDFLAGS += `pkg-config libxml++-3.0 --libs` \
	-L../../json-c/lib -ljson-c

endif

else

if JSON_INSTALLED

AM_CPPFLAGS += `pkg-config --cflags json-c`
apcmd_LDFLAGS += `pkg-config --libs json-c`
AM_LDFLAGS += `pkg-config --libs json-c`

else

AM_CPPFLAGS += -I../../json-c/include
apcmd_LDFLAGS += -L../../json-c/lib -ljson-c

endif

endif

apcmd_SOURCES = \
	apcheckplan.C \
	apcmd.C \
	apconsumption.C \
	apcrash.C \
	apcreate.C \
	apcreatemany.C \
	apdeleteact.C \
	apedit.C \
	apgetapf.C \
	apgetattr.C \
	apgetparam.C \
	apgetplan.C \
	apgetresource.C \
	apgetstdatts.C \
	apgetviolations.C \
	apinit.C \
	apintegral.C \
	apjson.C \
	apputaaf.C \
	apsetattr.C \
	apsetparam.C \
	apsitAndWait.C \
	apterminate.C \
	aptranslate.C \
	apsystem.C
