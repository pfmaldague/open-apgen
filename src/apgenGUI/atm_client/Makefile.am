noinst_LIBRARIES = libATM_client.a
bin_PROGRAMS = atm

## do not include the udef directories, which are handled automatically.
## For now, don't include huge documentation subdirectories.
##
## NOTE: only one of the udef directories will be included in the tarball.
##	 We should find a better way to handle this, but will do for now
##	 (the customer probably only cares about one of them anyway)
EXTRA_DIST = atm_sources aTM_client


##install app-defaults
appdefaultsdir=${sysconfdir}/app-defaults
appdefaults_DATA = aTM_client

atm_LDADD =	libATM_client.a \
	 	UIutil/libUIutil.a \
		../../apcore/libAPcore.la \
		../../apcore/parser/libParser.la \
		@GTK_ED_LIB_LOC@

atm_LDFLAGS =

if GTK_EDITOR_OPTION

atm_LDFLAGS+= `pkg-config gtkmm-2.4 --libs`

endif

if USE_LIBXMLCXX

atm_LDFLAGS+= \
	`pkg-config libxml++-3.0 --libs` 

endif

atm_LDFLAGS += -export-dynamic -ldl

SUBDIRS = UIutil .

## if GTK_EDITOR_OPTION
## 
## GTK_INCLUDES	= -Iinclude \
## 	-I@GTK_ED_INCL_LOC@ \
## 	`pkg-config gtkmm-@GTKMM_VERSION_TO_USE@ --cflags`
## 
## endif

## In final stage, add doc, test and tutorials:
## EXTRA_DIST = doc test tutorials $EXTRA_DIST

include atm_sources

AM_CPPFLAGS =
AM_LDFLAGS = 

if USE_LIBXMLCXX

if JSON_INSTALLED
AM_CPPFLAGS += `pkg-config libxml++-3.0 --cflags json-c`
atm_LDFLAGS += `pkg-config --libs json-c`
else
AM_CPPFLAGS += `pkg-config libxml++-3.0 --cflags` \
	-I${top_srcdir}/json-c/include
atm_LDFLAGS += -L${top_srcdir}/json-c/lib -ljson-c
endif
else
if JSON_INSTALLED
AM_CPPFLAGS += `pkg-config --cflags json-c`
else
AM_CPPFLAGS += \
	-I${top_srcdir}/json-c/include
atm_LDFLAGS += -L${top_srcdir}/json-c/lib -ljson-c
endif
endif

AM_CPPFLAGS += -DGUI @XPM_ATTR_21@ @XPM_HEAD@ @GTK_EDITOR_FLAG@
