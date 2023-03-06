
AC_INIT([apgen],[0.5])
AC_CONFIG_SRCDIR([apgen.c])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS(apgenlib/config.h)

dnl allows for maintainer mode
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
LT_INIT
AM_PROG_CC_C_O

dnl The default value of CFLAGS is '-g -O2'; we override this because in early
dnl phases we want any core files to be as useful as possible...
AM_CFLAGS="-O2"
AM_CXXFLAGS="-O2"
CPPFLAGS=

AC_DEFINE([have_apgen64], [], [Indicates we are compiling the original 64-bit version of apgen as opposed to the template-based version])

BUILDTAG=
if test "X$APGENBLDTAG" = X; then
    BUILDTAG="Custom Build - `date`"
else
    BUILDTAG=${APGENBLDTAG}
fi

AC_SUBST(BUILDTAG)

dnl 0. Is this a Mac OS X platform?

AC_ARG_WITH(macos,
[  --with-macos
    Use this to specify a build for the Mac OS X Operating System
],
    [AC_DEFINE([HAVE_MACOS], [], [Indicates we are compiling for the Mac OS X target])
],)

AC_ARG_WITH(orchestrator,
[  --with-orchestrator
    Use this to specify a build that includes an external orchestrator
],
    [AC_DEFINE([HAVE_XMLRPC], [], [Indicates we are compiling an orchestrator-enabled version of apgen])
],)

AM_CONDITIONAL(CHECK_HEADER_HIERARCHY,false)
AM_CONDITIONAL(BUILD_EVERYTHING,false)


AC_ARG_WITH(header-test,
[  --with-header-test
	Use this to check that the hierarchy of APGen header files is sane.],
	[ AM_CONDITIONAL(CHECK_HEADER_HIERARCHY,true)
	AC_DEFINE([HAVE_HEADER_HIER_CHECK], [], [Indicates we are including sanity tests of the header file hierarchy])	
	],
)

AC_ARG_WITH(everything,
[  --with-everything
	Use this to build everything (APcore server, S/C library, TPF utilities.
],
	[ AM_CONDITIONAL(BUILD_EVERYTHING,true) ],
)

dnl 1.1 use the SEQ directory structure if enabled - OBSOLETE
dnl AC_SEQ_DIRS

CFLAGS=

UDEF_LIBRARY_FLAG=
UDEF_COMPILE_FLAG=
AC_ARG_WITH(udef,
[  --with-udef
        Link libudef.so statically instead of at run-time.],
        [UDEF_LIBRARY_FLAG="-L${with_udef} -ludef -lmtat -lndtable -lmmpat -lmise -ldiburn -lEphem -lrosspice_c -lcspice -lsalt"
    UDEF_COMPILE_FLAG="-DOLDUDEF"]
        )
AC_SUBST(UDEF_LIBRARY_FLAG)
AC_SUBST(UDEF_COMPILE_FLAG)

dnl        [UDEF_LIBRARY_FLAG="-L${with_udef} -ludef "

dnl AM_CONDITIONAL(COMPILE_XML_READER,false)

dnl AC_ARG_ENABLE(xml-reader,
dnl [  --enable-xml-reader
dnl     Compile utilities for reading multi-mission AD (requires gdome library).],
dnl     [if test "X$enableval" = Xyes; then
dnl     AM_CONDITIONAL(COMPILE_XML_READER,true)
dnl     AC_DEFINE([have_xml_reader], [], [For chunky xml output, use the --enable-xml-reader option])
dnl      fi]
dnl     )

AM_CONDITIONAL(USE_LIBXMLCXX,false)

AC_ARG_ENABLE(xmltol,
[  --enable-xmltol
    Compile utilities for reading and writing XML (requires libxml++ library).],
    [if test "X$enableval" = Xyes; then
    AM_CONDITIONAL(USE_LIBXMLCXX,true)
    AC_DEFINE([have_xmltol], [], [For xmltol output, use the --enable-xmltol option])
     fi]
    )

AC_ARG_ENABLE(AAF_compiler,
[  --enable-AAF_compiler
    Compile abstract resources into C++ code (requires code generated with the -generate-cpluplus option).],
    [if test "X$enableval" = Xyes; then
    AC_DEFINE([have_AAF_compiler], [], [For AAF compilation, use the --enable-AAF_compiler option])
     fi]
    )

AC_ARG_WITH(spice-services,
[  --with-spice-services
    Add an API suitable for spice services.],
    [AC_DEFINE([have_spice_services], [], [For regular APcore services, disable the with-spice option])
    ]
    )

AM_CONDITIONAL(USE_GOOGLETEST, false)

AC_ARG_ENABLE(gtest,
    AS_HELP_STRING([--enable-gtest],[Enable Google Test framework for unit testing]),
    AC_CHECK_FILE(../../libgtest.a, 
        [AM_CONDITIONAL(USE_GOOGLETEST, true) 
        AC_DEFINE([have_gTest], [], [To enable Google Test Framework, build libgtest.a])
        ],
        [AM_CONDITIONAL(USE_GOOGLETEST, false) 
        AC_MSG_ERROR([library for Google Test Framework not found])
        ]
    )
)

AM_CONDITIONAL(JSON_INSTALLED, false)
AC_ARG_ENABLE(json,
    AS_HELP_STRING([--enable-json],[AUTODETECT: detect whether json-c library is locally installed]),
    [AM_CONDITIONAL(JSON_INSTALLED, true) 
    AC_DEFINE([json_installed], [], [json-c library not installed on machine, using local copy])
    ],
    [AM_CONDITIONAL(JSON_INSTALLED, false) 
    ]
)

dnl 2. X AND MOTIF

APGEN_X_INCL=
APGEN_XM_INCL=
XPM_HEAD=
XPM_INCL=
XPM_ATTR_21=

AC_FIND_MYMOTIF
AC_PATH_X
AC_PATH_XTRA

if test "X$ac_x_libraries" = X; then
        AC_MSG_WARN([

Either you are using a platform in which the X libraries need no
special treatment (e. g. Fedore Core 5), or you don't seem to
have access to the X binaries and/or libraries. 

In the latter case, you won't be able to build the apgen graphic
front-end.  On Solaris, try adding /usr/openwin/bin to your
PATH and /usr/openwin/lib to your LD_LIBRARY_PATH.

If desperate you can disable the front-end build by using the
--disable-atm option.
])
fi

dnl Store location of X includes and libraries for use by automake as discussed above

if test "X$ac_x_includes" = X; then
APGEN_X_INCL=
elif test "X$ac_x_includes" = Xno; then
APGEN_X_INCL=
else
APGEN_X_INCL=-I$ac_x_includes
fi

CPPGOOD="$CPPFLAGS $APGEN_X_INCL"

dnl should be a little smarter here:
if test "X$ac_x_libraries" = X; then
APGEN_X_LIBS=
elif test "X$ac_x_libraries" = Xno; then
APGEN_X_LIBS=
else
APGEN_X_LIBS=-L$ac_x_libraries 
fi

dnl LIBSGOOD="-lXt -lX11 -lXext"
LIBSGOOD="$APGEN_X_LIBS -lXt -lX11 -lXext"
LDFLAGS=$LIBSGOOD
AC_MSG_RESULT(libs flag used to test X/motif: $LIBSGOOD)
XP_LIB=

dnl        Allow the installer to omit the Xp library (for some Solaris systems)
AC_ARG_ENABLE(xp,
[  --enable-xp
        Add the Xp library while linking. Use this if you need to.],
        [if test "X$enableval" = Xyes; then
           XP_LIB=-lXp
         fi]
    )

if test -d motif/include ;
then
        APGEN_XM_INCL="-I`pwd`/motif/include"

    case $host in
       *-solaris*)
             APGEN_XM_LD="-L`pwd`/motif/lib -lapgenXm $XP_LIB"
             ;;
           *-linux*)
             APGEN_XM_LD="-L`pwd`/motif/lib -lapgenXm -lXmu $XP_LIB"
             ;;
           *)
             APGEN_XM_LD="-L`pwd`/motif/lib -lapgenXm $XP_LIB"
             ;;
        esac
fi

CPPFLAGS="$CPPGOOD $APGEN_XM_INCL"
LIBS="$APGEN_XM_LD $LIBSGOOD"


AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <Xm/Xm.h>]], [[XmFontList fl ; XFontStruct fs ; XmStringCharSet charset ;
     XmFontListAdd( fl , &fs , charset)]])],[AC_MSG_RESULT(motif dir motif link successful $APGEN_XM_LD)
     ],[
        if test x$with_motif = xyes;
        then
                APGEN_XM_INCL=$include_motif
                APGEN_XM_LD=$link_motif
                CPPFLAGS="$CPPGOOD $APGEN_XM_INCL"
                LIBS="$APGEN_XM_LD $LIBSGOOD"
                AC_MSG_RESULT([Successful link: $CPPFLAGS $LIBS])
        LDFLAGS="$APGEN_XM_LD $LDFLAGS"
        else
                AC_MSG_ERROR([Unsuccessful link: $CPPFLAGS $LIBS
You don't seem to have access to the Motif libraries.  Try defining
the --with-motif-includes=DIR and --with-motif-libraries=DIR options,
or make sure ./motif/lib/(libapgenXm.so or libapgenXm.a) has been
defined and that ./motif/include contains the Xm header directory.
See the README file for more details.
])

        fi
        ])

dnl OK, we made it past here; that means we know how to compile and link with
dnl the basic X11 and Motif libraries. Save the results of all that good work:

config_xpm_type=XpmAttributes
config_xpm_include='"Xm/XpmP.h"'

AC_ARG_WITH(Xpm,
[  --with-Xpm=DIR 
    DIR is full path name of directory containing the XPM header
    file.  On most systems this is MOTIFPATH/Xm/XpmP.h where
    MOTIFPATH is the Motif directory. However, on a few systems
    (e.g., Solaris 2.5), the header file is called xpm.h and it is
    located in a non-standard directory (e.g., /tps/include/X11).
    In that case use this option to specify the directory where
    to find the header (e.g., --with-Xpm=/tps/include/X11).
    [Default: Not used. We assume the header is named XpmP.h and
    is located in the same directory as Xm.h.]],
    [
dnl        So we can do more tests with the correct Xpm include:
        CPPFLAGS="$CPPFLAGS -I$with_Xpm"
dnl        So Makefile.am knows where to fetch the Xpm include:
        XPM_INCL="-I$with_Xpm"
        config_xpm_include="<${with_Xpm}/xpm.h>"
        XPM_HEAD=-DUSES_XPM_H]
  )

AC_ARG_WITH(XpmLib,
    [  --with-XpmLib=DIR
        DIR is the full directory name of the Xpm library. On most
        systems this library is not needed because libXm provides the xpm
        functionality. But on Solaris 2.* you may have to specify this
        (e.g., as /tps/lib/X11).  [Default: Xpm lib is not used.]],
      [XPM_LIBDIR="-L$with_XpmLib -lXpm"
        LDFLAGS="$LDFLAGS -L$with_XpmLib -lXpm"]
      )

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include $config_xpm_include]], [[XpmAttributes_21 attributes]])],[XPM_ATTR_21=-DHAVE_XPMATTR_21
        config_xpm_type=XpmAttributes_21
        ],[])

dnl Now check whether we need a separate Xpm library
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include $config_xpm_include]], [[Display *k;Window w; char **c;Pixmap p1;Pixmap p2;${config_xpm_type} a;
        XpmCreatePixmapFromData(k,w,c,&p1,&p2,&a)]])],[],[AC_MSG_ERROR(
Having trouble finding XpmCreatePixmapFromData.  This should be in the
Motif library. See README-MOTIF for possible insight.
)])

dnl automake will use XPM_ATTR_21
dnl automake will use XPM_HEAD as part of its INCLUDES:
AC_SUBST(APGEN_X_INCL)
AC_SUBST(APGEN_XM_INCL)
AC_SUBST(XPM_HEAD)
AC_SUBST(XPM_INCL)
AC_SUBST(XPM_ATTR_21)

dnl 3. GTK EDITOR

AM_CONDITIONAL(GTK_EDITOR_OPTION,false)
AM_CONDITIONAL(USE_GTKMM_2_4,true)
GTK_EDITOR_FLAG=
GTK_ED_LIB_LOC=
GTK_ED_INCL_LOC=
GTKMM_VERSION_TO_USE=2.4
GTK_SUBDIRECTORY=../APedit-2.4

AC_ARG_WITH(gtkmm-version,
[  --with-gtkmm-version
    Use this to specify a gtkmm version other than the default, 2.4
],
    [GTK_SUBDIRECTORY=../APedit-${with_gtkmm_version}
    GTKMM_VERSION_TO_USE=${with_gtkmm_version}
    AM_CONDITIONAL(USE_GTKMM_2_4,true)
]
dnl     AC_MSG_WARN([gtkmm version 2.4 will be used if not overridden by using
dnl --with-gtkmm-version=NNN.])
  )

AC_ARG_ENABLE(gtk-editor,
[  --enable-gtk-editor
        Add the Xp library while linking. Use this if you need to.],
        [if test "X$enableval" = Xyes; then
    AM_CONDITIONAL(GTK_EDITOR_OPTION,true)
    GTK_ED_LIB_LOC=${GTK_SUBDIRECTORY}/libAPedit.a
    GTK_ED_INCL_LOC=${GTK_SUBDIRECTORY}
    GTK_EDITOR_FLAG="-DGTK_EDITOR"
         fi]
        )
AC_SUBST(GTK_ED_LIB_LOC)
AC_SUBST(GTK_ED_INCL_LOC)
AC_SUBST(GTK_EDITOR_FLAG)
AC_SUBST(GTKMM_VERSION_TO_USE)

dnl 4. OTHER LIBRARIES

dnl Save the release option in case profiling is NOT wanted:
APGEN_NSLLIB="nsl"


AC_LANG_SAVE
AC_LANG([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include "${ac_x_includes}/X11/Xlib.h"]], [[int dummy = 1]])],[],[CFLAGS="${CFLAGS} -fpermissive -w"])
AC_LANG_RESTORE

CXXFLAGS=$CFLAGS
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)

AC_HEADER_DIRENT

# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
# AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(malloc.h sys/time.h unistd.h fcntl.h  sys/stat.h \
    sys/file.h sys/ioctl.h termio.h sys/filio.h libxml++-2.6/schema.h)

dnl CPPFLAGS= 
dnl LIBS=

dnl This check looks for the libsocket.so library; not needed on Linux,
dnl but Solaris needs it. If the library is found, configure adds the
dnl proper '-lsocket' string to the final linker command.
AC_CHECK_LIB( socket, socket, , )
AC_CHECK_LIB( xslt, xsltCleanupGlobals, , )


dnl Required for Solaris 2.8:
dnl NOTE: when profiling on Linux, we need a way to link with nsl_p, not nsl... maybe this:
AC_CHECK_LIB( ${APGEN_NSLLIB}, inet_ntoa, , )

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

AC_STRUCT_TM

dnl Checks for library functions.

AC_CHECK_FUNCS(getcwd gethostname gettimeofday stime strdup strerror strstr strtol)


NEED_STATIC_LIBS=no
EXTRA_LIBS="${UDEF_LIBRARY_FLAG} ${GMSEC_LIBRARY} -ldl -lpthread"
EXTRA_XLIBS="-lXt -lX11 -lXext"
MOTIF_LIBS="${APGEN_XM_LD} ${XPM_LIBDIR} ${XP_LIB}"

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(MOTIF_LIBS)
AC_SUBST(EXTRA_LIBS)
AC_SUBST(EXTRA_XLIBS)

echo "All done checking libraries. Final result:"
echo "    LDFLAGS = " ${LDFLAGS}
echo "    CPPFLAGS = " ${CPPFLAGS}
echo "    CFLAGS = " ${CFLAGS}
echo "    MOTIF_LIBS = " ${MOTIF_LIBS}
echo "    EXTRA_LIBS = " ${EXTRA_LIBS}
echo "    EXTRA_XLIBS = " ${EXTRA_XLIBS}

AC_CONFIG_FILES([Makefile
	xmlrpc/Makefile
	apcore/Makefile
	apcore/parser/Makefile
	apcore/parser_util/Makefile
	apcore/fd/Makefile
	tpf/Makefile
	tpf/comparison/Makefile
	apcore_server/Makefile
	apcore_server/test/Makefile
	apgenGUI/Makefile
	apgenGUI/APedit-2.4/Makefile
	apgenGUI/atm_client/Makefile
	apgenGUI/atm_client/UIutil/Makefile
])
AC_OUTPUT

