#!/bin/bash

function detect_spaces
	if [ $# -gt "1" ]; then
		echo "Current directory contains one or more space(s); cannot build apgen."
		exit 1
	fi

detect_spaces ${PWD}

# Fails if the return code is non-zero
# parms: $? the return code of the most recent command, $1, lineno calling this
#
assertZeroReturn() {
	if [[ $? -ne 0 ]]
	then
		echo $0: Build failed at line number $1·
		exit 1
	fi
}

# MACH should be set for bamboo builds, otherwise infer based on machine architecture.
if [ -z "$MACH" ]; then
	if [ $(uname) = "Linux" ]; then

		if [[ $(uname -a) =~ .*el7.* ]]; then
			OS="rhel7"
			if [ $(uname -p) = "x86_64" ]; then
				BITS="_64"
			else
				BITS=""
			fi
		elif [[ $(uname -a) =~ .fc30. ]]; then
			OS="fc30"
			BITS=""
		else
			OS="rhel7"			
			BITS="_64"			
		fi

		MACH="${OS}${BITS}"

	elif [ $(uname) = "Darwin" ]; then
		MACH="darwin"
	else
		echo "platform not supported; exiting"
	fi
fi

gtest=" "
debug_events=""
xmloption=""
cxxflags="-O3"
profile_options=""
configureopts="--disable-shared"
keep_generated_files=""

installdir="/opt/local"
bamboo="false"
udeflib=""
build_everything=""
aafcompiler="n"
while [ $# -gt 0 ]; do
	echo "processing arg $1..."
	if [ $1 = "debug" ]; then
		cxxflags="-g"
	elif [ $1 = "full" ]; then
		build_everything="--with-everything"
	elif [ $1 = "compile-aaf" ]; then
		aafcompiler="y"
	elif [ $1 = "gtest" ]; then
		gtest="--enable-gtest"
	elif [ $1 = "verbose" ]; then
		cxxflags="-DapDEBUG -g"
	elif [ $1 = "dynamic" ]; then
		configureopts=""
	elif [ $1 = "profile" ]; then
		profile_options="-pg"
		cxxflags="-pg"
	elif [ $1 = "coverage" ]; then
		profile_options="-g"
		cxxflags="-g -fprofile-arcs -ftest-coverage"
	elif [ $1 = "events" ]; then
		debug_events="-DDBGEVENTS"
	elif [ $1 = "prefix" ]; then
		shift
		if [ $# -gt 0 ]; then
			installdir=$1
		else
			echo "prefix should be followed by a directory name; exiting."
			exit 1
		fi
	elif [ $1 = "udef" ]; then
		shift
		if [ $# -gt 0 ]; then
			udeflib="--with-udef=$1"
		else
			echo "udef should be followed by the directory in which libudef is installed; exiting."
			exit 1
		fi
	elif [ $1 = "bamboo" ]; then
		echo "Detected bamboo option."
		configureopts="--disable-shared"
		rm -rf bamboo
		mkdir -p bamboo/$MACH
		installdir="$PWD/bamboo/$MACH"
		# setup SEQBLDTAG, but only if it does not exist
		if [ -z "$SEQBLDTAG" ]; then
			echo "SEQBLDTAG: $SEQBLDTAG"
			echo "SEQBLDTAG is not set. Using MACH plus time string."
			TIME_STRING=`date +"%y%m%d-%H%M"`
			export SEQBLDTAG="$MACH-$TIME_STRING"
			echo "SEQBLDTAG: $SEQBLDTAG"
		fi
		bamboo="true"
	elif [ $1 = "keep" ]; then
		echo "Detected keep generated files option."
		keep_generated_files="y"
	elif [ $1 = "xml" ]; then
		echo "Detected XML option."
		xmloption="--with-orchestrator"
	else
		echo "argument $1 not understood; exiting. Valid options: debug, full, prefix <...>, compile-aaf, coverage, dynamic, keep, profile, udef <...>, bamboo."
		exit 1
	fi
	shift
done

#
# Confirm to the user that the platform is what we think it is
#
echo "MACH = $MACH, CSEDIR = $CSEDIR, PKG_CONFIG_PATH = $PKG_CONFIG_PATH"

#
# Clean up various files which may have been left over from
# a previous build
#
if [ -L json-c/lib ]; then
	rm json-c/lib
fi
if [ -L json-c/include ]; then
	rm json-c/include
fi

if [ x$keep_generated_files = "x" ]; then
    echo "Removing any leftover generated files..."

    pushd src/apcore/parser
    rm -f grammar.y grammar.C grammar.H tokens.C cmd_grammar.C cmd_grammar.H cmd_tokens.C
    popd
    pushd src/tpf/comparison
    rm -f grammar.y tolcomp_grammar.C tolcomp_grammar.H \
	tolcomp_tokens.C tolcomp_tokens.H \
	gen_tol_exp.C gen_tol_exp.H
    popd
else
    echo "Not removing any leftover generated files (keep option chosen)"
fi

#
# Generate the environment and the specific arguments to
# the configure script, as needed on the various platforms
# that we support
#
linkeropts=
cflags="${cxxflags}"
if [ $MACH = "rhel7_64" ]; then
	export CSEDIR=/ammos/apgen/cse
	export PKG_CONFIG_PATH=${CSEDIR}/lib64/pkgconfig:/usr/lib64/pkgconfig:/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig
	linkeropts="${profile_options} -L${CSEDIR}/lib64 -Wl,-rpath=${CSEDIR}/lib64"
	cxxflags="${cxxflags} -std=c++11"
elif [ $MACH = "rhel5-64" ]; then
	linkeropts="${profile_options}"
	cxxflags="${cxxflags} -DUSE_AUTO_PTR"
elif [ $MACH = "fc30" ]; then
	linkeropts="${profile_options}"
	cxxflags="${cxxflags} -std=c++11"
elif [ $MACH = "darwin" ]; then
	export PATH=/usr/local/opt/bison/bin:/opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X11/bin
	export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig
	configureopts="${configureopts} --with-macos"
	cxxflags="${cxxflags} -std=c++11 -Wno-deprecated-register"
	cflags="${cflags} -Wno-deprecated-register"
	unset DYLD_LIBRARY_PATH
	linkeropts="${profile_options}"
fi

#
# Most of the action takes place in the src directory:
#
pushd src

#
# If the 'AAF compiler' option was selected, handle adaptation files, if any
#
if [ $aafcompiler = "y" ]; then
    adapt_files=""
    if [ -f apcore/parser_support/adapt_abs_res.H ]; then
	adapt_files=${adapt_files}y
    else
	adapt_files=${adapt_files}n
	
    fi
 
    if [ -f apcore/parser_support/adapt_abs_res.C ]; then
	adapt_files=${adapt_files}y
    else
	adapt_files=${adapt_files}n
    fi
 
    if [ -f apcore/parser_support/adapt_abs_res_2.C ]; then
	adapt_files=${adapt_files}y
    else
	adapt_files=${adapt_files}n
    fi
    if [ $adapt_files = "yyy" ]; then
	echo "Adaptation file test: all files found"
	configureopts="${configureopts} --enable-AAF_compiler"
    else
	echo "Test:" $adapt_files "Not all adaptation files found in apcore/parser_support/."
	echo "You must run apbatch or atm with the -generate-cplusplus option,"
	echo "then copy the adapt_abs_res* files to apcore/parser_support/."
	exit 1
    fi
fi

if [ -f Makefile ]; then
	echo "Cleaning up files from previous build"
	make distclean
	bash mrproper
fi

#
# Dennis and I are both desperate for info regarding environment variables, so here we go:
#
echo "MACH = $MACH, CSEDIR = $CSEDIR, PKG_CONFIG_PATH = $PKG_CONFIG_PATH"

#
# check if json-c package exists.
#
# WARNING: detection largely dependent upon the package manager
# installed on the machine
#
json=
pkg-config --exists json-c
if [ $? -eq 0 ]; then
	echo "json-c detected on machine"
	json="--enable-json"
else
	echo "json-c not detected on machine, using local version"

	if [ -L json-c/lib ]; then
		rm json-c/lib
	fi
	if [ -L json-c/include ]; then
		rm json-c/include
	fi
	ln -s ${PWD}/json-c/${MACH}/lib json-c/lib
	ln -s ${PWD}/json-c/${MACH}/include json-c/include
	cp json-c/include/*.h apgenlib
fi

#
# The following commands are redundant - 'make distclean'
# should remove all generated files if the CLEANFILES
# macro is properly defined. However, a special annoyance
# is the possible existence of a leftover gen_tol_exp.H
# file in the apgenlib subdirectory; let's clean it up
# if it exists:
rm -f apgenlib/gen_tol_exp.H


#
# First build step: bootstrap
#
if ! ./bootstrap; then
    popd
    echo bootstrap error
    exit 1
fi

# autoreconf -i -I config


#
# Second build step: configure
#
if ! ./configure --prefix=${installdir} --enable-gtk-editor --enable-xmltol \
	${gtest} ${json} ${configureopts} ${udeflib} ${xmloption} ${build_everything}; then
    popd
    echo configure error
    exit 3
fi

#
# Third build step: make install
#
cxxflags="${cxxflags} ${debug_events}"
if ! make -j 5 install CFLAGS="${cflags}" CXXFLAGS="${cxxflags}" LDFLAGS="${linkeropts}"; then
    popd
    echo make error
    exit 2
fi

#
# if bamboo build, tar up install files
#
if [ $bamboo = "true" ]; then
	# run rpath -d on binaries and executables to remove all hard-coded
	# references to build directories
	pushd bamboo/$MACH
	if [ $MACH = "rhel7_64" ]; then
		echo "Modifiying rpath for all binaries and libraries to ${CSEDIR}/lib64"
		chrpath -r "${CSEDIR}/lib64" bin/* lib/*.so.*.*.*
	else
		echo "Deleting rpath for all binaries and libraries"
		chrpath -d bin/* lib/*.so.*.*.*
	fi
	popd

	# reorganize directory structure; make sure install directory
	# is at the same level as src, where the test script expects it
	rm -rf ../install
	mkdir -p ../install/bin
	mkdir -p ../install/lib
	mkdir -p ../install/etc
	mkdir -p ../install/include
	cp -rp bamboo/$MACH/bin/* ../install/bin/
	assertZeroReturn $LINENO
	cp -rp bamboo/$MACH/lib/* ../install/lib/
	assertZeroReturn $LINENO
	cp -rp bamboo/$MACH/etc/* ../install/etc/
	assertZeroReturn $LINENO
	cp -rp bamboo/$MACH/include/* ../install/include/
	assertZeroReturn $LINENO

	# create tarball
	pushd ../install
	echo "creating tar file apgen-$SEQBLDTAG.tar.gz"
	tar czvf apgen-$SEQBLDTAG.tar.gz *
	popd
fi

#
# Clean up generated files, _again_
#
if [ x$keep_generated_files = "x" ]; then
    echo "Removing any leftover generated files..."

    pushd apcore/parser
    rm -f grammar.y grammar.C grammar.H tokens.C cmd_grammar.C cmd_grammar.H cmd_tokens.C
    popd
    pushd tpf/comparison
    rm -f grammar.y tolcomp_grammar.C tolcomp_grammar.H \
	tolcomp_tokens.C tolcomp_tokens.H \
	gen_tol_exp.C gen_tol_exp.H
    popd
else
    echo "Not removing any leftover generated files (keep option chosen)"
fi

#
# Clean up json headers if necessary
#
if [ x$json = "x" ]; then
	pushd apgenlib
	rm -f arraylist.h json.h json_inttypes.h json_object_private.h linkhash.h \
	  bits.h json_c_version.h json_object.h json_tokener.h printbuf.h \
	  debug.h json_config.h json_object_iterator.h json_util.h random_seed.h
	popd
fi

#
# Most of the action took place in the src directory; now we return home
#
popd
