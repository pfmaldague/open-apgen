# open-apgen

## Origin and Inheritance

open-apgen is the open-source version of the APGenX application developed by the Jet Propulsion Laboratory for NASA. The initial version of open-apgen, version O.1, is identical with version X.9.1 of JPL's APGenX as far as the C++ code is concerned. However, JPL-specific interfaces and documentation files have been eliminated or modified in the open-source version.

## APGenX as part of a system

APGenX is a C++ application which can be run as an interactive, GUI-based program or as an invisible process running in the background. In order to fully understand APGenX, however, it helps to look at it as part of a system. The purpose of the system is to provide a complete simulation and operational environment for a deep-space mission. Deep space is characterized by long interplanetary travel times, typically from 6 months to several years, and also by long round-trip light times, typically from 30 minutes to several hours. As a result, command sequences executed onboard the spacecraft are unusually long and complex. They must also be robust against a wide range of possible faults: memory corruption due to cosmic rays, equipment failures that are inevitable over long periods of time, and undetected bugs in the spacecraft's operating or instrument software. The fault protection subsystem, which continuously monitors the spacecraft's health, responds to such faults by putting the spacecraft into a well-defined "safe state," thus providing the ground with an opportunity to send the necessary commands for the spacecraft to resume operations. Creating command sequences, verifying that they can be safely executed and measuring their effectiveness in meeting mission objectives is a major challenge. APGenX and the system built around it were designed to meet this challenge.

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

