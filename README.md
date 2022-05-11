# open-apgen

## Origin and Inheritance

open-apgen is the open-source version of the APGenX application developed by the Jet Propulsion Laboratory for NASA. The initial version of open-apgen, version O.1, is identical with version X.9.1 of JPL's APGenX as far as the C++ code is concerned. However, JPL-specific interfaces and documentation files have been eliminated or modified in the open-source version.

## Installation

The [build script](./build.sh) can be used to build open-apgen on a Mac or a Linux machine, provided that a number of prerequisite packages have been installed first. The script assumes that the platform on which it is running is one of the few that it knows - mostly recent versions of Max OS X, Red Hat Enterprise Linux or Fedora Core. Other versions of Linux could probably be used with small modifications of the script.

If one or more prerequisites are missing, the build script will fail with error messages that may or may not be helpful. The list below shows the packages that should be installed on Fedora Core. A similar list of packages (the names may be slightly different) can be installed with MacPort or brew on the Mac platform.

  - motif-devel
  - gtkmm24-devel
  - libxml++-30-devel
  - libsxlt-devel
  - libcurl-devel

On the Mac, it is also necessary to install the autotools (autoconf, automake, libtool) which are not provided by the Mac OS.

The build script takes a few options; the most useful is

	./build.sh prefix $HOME/my-apgen-install-dir

which will install executables, libraries and header files in the bin, lib and include subdirectories of the specified directory. If the prefix option is not specified, the build script will attempt to install in the /opt/local directory, which may not be writable by the user.

## Documentation

Over the years, a number of papers about APGen have been presented at SpaceOps meetings. The papers are available from the spaceops archive at the following web site:

	arc.aiaa.org/seqies/6.spaceops

Some of these papers are available in the ./doc subdirectory of the open-apen repository; see [the doc README](./doc/README.md) for more information.

