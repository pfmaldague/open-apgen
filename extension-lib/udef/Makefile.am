lib_LTLIBRARIES = libudef.la

libudef_la_SOURCES = \
	udef_functions.C \
	User_lib_Title.C \
	horizons.C \
	spice_interface.C \
	spkez_interface.C \
	geom_sys_util.C \
	spice_kernels.C \
	vzfrac.c \
	spice_subsys.C \
	file_subsys.C \
	vector_subsys.C \
	geom_subsys.C

libudef_la_LDFLAGS = -version-info=1.0.1

libudef_la_LIBADD = \
	../rosspice/lib/librosspice_c.a \
	../cspice/lib/libcspice.a \
	-lm

AM_CPPFLAGS = \
	-I@APGEN_INCLUDE@ \
	-I../cspice/include \
	-I../rosspice/include

if JSON_INSTALLED

AM_CPPFLAGS+= `pkg-config --cflags json-c`

endif

EXTRA_DIST = horizons.h constants.h

.PHONY: udef_title

udef_title:
	 echo  ' char * User_lib_build_date = (char *) "' `date` '"; ' > User_lib_Title.C
	 echo  ' char * User_lib_Title = (char *) 0 ; ' >> User_lib_Title.C

User_lib_Title.C User_lib_Title.o: udef_title
