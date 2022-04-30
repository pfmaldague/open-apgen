#!/bin/bash

if [ -z "$MACH" ]; then
	if [ $(uname) = "Linux" ]; then

		if [[ $(uname -a) =~ .*el7.* ]]; then
			OS="rhel7"
			if [ $(uname -p) = "x86_64" ]; then
				BITS="_64"
			else
				BITS=""
			fi
		# test for linux kernel (used to make it build on Fedora)
		elif [[ $(uname -r) =~ 4.9.*-linuxkit ]]; then
			OS="rhel7"
			if [ $(uname -p) = "x86_64" ]; then
				BITS="_64"
			else
				BITS=""
			fi
		elif [[ $(uname -r) =~ "fc30" ]]; then
			OS="rhel7"
			if [ $(uname -p) = "x86_64" ]; then
				BITS="_64"
			else
				BITS=""
			fi
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

# installdir="/opt/local"
installdir="${HOME}/UDEF"
cxxflags="-O3"
apgendir="/opt/local"
static_flag="no"
while [ $# -gt 0 ]; do
	echo "processing arg $1..."
	if [ $1 = "debug" ]; then
		cxxflags="-g"
	elif [ $1 = "static" ]; then
		static_flag="yes"
	elif [ $1 = "profile" ]; then
		cxxflags="-pg"
		static_flag="yes"
	elif [ $1 = "prefix" ]; then
		shift
		if [ $# -gt 0 ]; then
			installdir=$1
		else
			echo "prefix should be followed by a directory name; exiting."
			exit
		fi
	elif [ $1 = "apgen" ]; then
		shift
		if [ $# -gt 0 ]; then
			apgendir=$1
		else
			echo "apgen should be followed by a directory name; exiting."
			exit 1
		fi
	elif [ $1 = "help" ]; then
		echo "usage:"
		echo "./build.sh [debug | static | profile] [prefix <installdir>] [apgen <apgendir>]"
		echo "(defaults: installdir = $HOME/UDEF, apgendir = /opt/local, compiler flags = -O3, shared libs enabled)"
		exit
	else
		echo "argument $1 not understood; will exit. Correct usage:"
		echo "./build.sh [debug | static | profile] [prefix <installdir>] [apgen <apgendir>]"
		echo "(defaults: installdir = $HOME/UDEF, apgendir = /opt/local, compiler flags = -O3, shared libs enabled)"
		exit
	fi
	shift
done

if [ $MACH = "unknown" ]; then
	echo "platform not specified; exiting"
	exit
fi

cflags=${cxxflags}

if [ $MACH = "rhel7_64" ]; then
	cxxflags="${cxxflags} -std=c++11"
	export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:/usr/lib64/pkgconfig:/usr/local/lib/pkgconfig
	unset LD_LIBRARY_PATH
elif [ $MACH = "rhel5-64" ]; then
	cxxflags="${cxxflags} -DUSE_AUTO_PTR"
	export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:/usr/lib64/pkgconfig:/usr/local/lib/pkgconfig
	unset LD_LIBRARY_PATH
elif [ $MACH = "darwin" ]; then
	cxxflags="${cxxflags} -std=c++11 -Wno-inconsistent-missing-override"
	export PATH=/opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X11/bin
	export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig
	unset DYLD_LIBRARY_PATH
fi

if [ -L cspice/lib ]; then
	rm cspice/lib
fi
if [ -L cspice/include ]; then
	rm cspice/include
fi
if [ -L rosspice/lib ]; then
	rm rosspice/lib
fi
if [ -L rosspice/include ]; then
	rm rosspice/include
fi

if [ -f Makefile ]; then
	make distclean
	bash mrproper
fi

ln -s ${PWD}/cspice/${MACH}/lib cspice/lib
ln -s ${PWD}/cspice/${MACH}/include cspice/include
ln -s ${PWD}/rosspice/${MACH}/lib rosspice/lib
ln -s ${PWD}/rosspice/${MACH}/include rosspice/include

# check if json-c package exists
# WARNING: detection largely dependent upon the package manager installed on the machine
json=
pkg-config --exists json-c
if [ $? -eq 0 ]; then
	echo "json-c detected on machine"
	json="--enable-json"
else
	echo "json-c not detected on machine, using APGen's headers which hopefully include the json-c headers"
fi

./bootstrap

# autoreconf -i -I config
# export LD_LIBRARY_PATH=/opt/local/lib


if [ $static_flag = "yes" ]; then
    ./configure --prefix=${installdir} --disable-shared --with-apgen-include=${apgendir}/include/apgenlib ${json}
else
    ./configure --prefix=${installdir} --with-apgen-include=${apgendir}/include/apgenlib ${json}
fi

make install CFLAGS="${cflags}" CXXFLAGS="${cxxflags}"


if [ $static_flag = "yes" ]; then
    cp cspice/${MACH}/lib/libcspice.a ${installdir}/lib
    cp rosspice/${MACH}/lib/librosspice_c.a ${installdir}/lib
fi

