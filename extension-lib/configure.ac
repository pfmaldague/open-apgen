AC_INIT([libudef],[3.8.3])
AC_CONFIG_SRCDIR([udef/udef_functions.C])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS(config.h)

dnl allows for maintainer mode
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
LT_INIT

dnl The default value of CFLAGS is '-g -O2'; we override this because in early
dnl phases we want any core files to be as useful as possible...
CFLAGS="-g"
CXXFLAGS="-g"
CPPFLAGS=

APGEN_INCLUDE=

AC_ARG_WITH(apgen-include,
[  --with-apgen-include=DIR
        DIR is prefix of the apgen include installation. e.g. /usr/local/include
        or /home/joe/APGEN/include.  Configure will look for the include
	file(s) in directory DIR. [Default: apgen is not used.]],
      [APGEN_INCLUDE="${with_apgen_include}"],
     AC_MSG_ERROR([Option --with-apgen-include=DIR not used; cannot build the library.])
     )

AC_SUBST(APGEN_INCLUDE)

AM_CONDITIONAL(SPICE_SERVICES,false)

AM_CONDITIONAL(COMPILE_XML_READER,false)

AC_DEFINE([have_xml_reader], [], [For chunky xml output, use the --enable-xml-reader option])



AM_CONDITIONAL(LINK_STATICALLY,false)

AC_ARG_WITH(static-linking,
[  --with-static-linking
	Use this option to avoid loading libAPcore dynamically.
	NOTE: this option is mandatory with --disable-shared.
],
	[AC_DEFINE([HAVE_STATICLIBS], [], [Indicates we are linking statically (no udef)])
	AM_CONDITIONAL(LINK_STATICALLY,true)
],)

AM_CONDITIONAL(JSON_INSTALLED, false)
AC_ARG_ENABLE(json,
    AS_HELP_STRING([--enable-json],[AUTODETECT: detect whether json-c library is locally installed]),
    [AM_CONDITIONAL(JSON_INSTALLED, true)
    AC_DEFINE([json_installed], [], [json-c library not installed on machine, using local copy])
    ],
    [AM_CONDITIONAL(JSON_INSTALLED, false)
    ]
)

AC_ARG_WITH(unix,
[  --with-unix
	Use this command if unix is not defined by default (e. g. on Mac OS X).],
	[AC_DEFINE([unix], [], [It is safely assumed that this code will never be compiled under windows.])])

AC_ARG_WITH(apgen64,
[  --with-apgen64
	Use this command if this library will be used with apgen64 (as opposed to apmodular).],
	[AC_DEFINE([have_apgen64], [], [use --with-apgen64 if appropriate.])])

AC_ARG_WITH(apgen2,
[  --with-apgen2
	Use this command if this library will be used with apgen2 (as opposed to apmodular or apgen64).],
	[AC_DEFINE([have_apgen2], [], [use --with-apgen2 if appropriate.])])

SPICEFLAG="-DSPICE"
AC_SUBST(SPICEFLAG)

AC_CHECK_HEADERS(malloc.h sys/time.h unistd.h fcntl.h  dirent.h sys/ioctl.h values.h )

AC_CONFIG_FILES([Makefile add_time/Makefile udef/Makefile])
AC_OUTPUT
