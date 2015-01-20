#!/bin/sh
#
# This file is part of playd.
# playd is licensed under the MIT licence: see COPYING.txt.
#
#
# Configuration script for playd.
# This config script is directed by several environment
# variables, given below.
#
#
# Variables:
#   General:
#     PROGNAME.....................................................program name
#                                                            (default: 'playd')
#     PROGVER...................................................program version
#                                                    (default: acquire via git)
#
#   Directories:
#     SRCDIR..........................................directory of source files
#                                                              (default: ./src)
#     BUILDDIR.......................................directory for object files
#                                                            (default: ./build)
#     PREFIX...........................................root of where to install
#                                                        (default: ./usr/local)
#
#   Programs:
#     MAKE.....................................GNU Make (specifically GNU Make)
#     PKGCONF..........................................pkg-config or equivalent
#
#   Package name overrides:
#     FLACXX_PKG......................................FLAC++ pkg-config package
#     LIBMPG123_PKG................................libmpg123 pkg-config package
#     LIBSNDFILE_PKG..............................libsndfile pkg-config package
#     SDL2_PKG..........................................SDL2 pkg-config package
#     LIBUV_PKG........................................libuv pkg-config package
#
#   File format flags (set to non-empty string to activate):
#     NO_FLAC................................................don't support FLAC
#     NO_MP3..................................................don't support MP3
#     NO_SNDFILE...............................don't support libsndfile formats
#
# Notes:
#   - lack of FLAC++ implies NO_FLAC;
#   - lack of libmpg123 implies NO_MP3;
#   - lack of libsndfile implies NO_SNDFILE;
#   - lack of pkgconf/pkg-config or SDL2 is fatal.

# Runs `make clean`, if a Makefile is present.
clean() {
	if [ -f "Makefile" ]; then
		echo 'running `'$MAKE' clean` first...'
		$MAKE clean >/dev/null
		echo 'removing existing Makefile...'
		rm Makefile
		echo
	fi
}

#
# Misc variable finding
#

# Populates $PROGNAME and $PROGVER if not already set up.
find_name_and_version() {
	if [ -z "$PROGNAME" ]; then PROGNAME="playd"; fi

	if [ -z "$PROGVER" ]; then
		PROGVER=`git describe --tags --always`
		if [ "$?" -ne 0 ]; then
			echo "not a git clone, need to set 'PROGVER'."
			echo "cannot continue"
			exit 1
		fi
	fi

	echo "Configuring build for $PROGNAME-$PROGVER"
}

find_dirs() {
	echo "DIRECTORIES:"

	if [ -z "$SRCDIR" ]; then SRCDIR="`pwd`/src"; fi
	echo "  srcdir:        $SRCDIR"

	if [ -z "$BUILDDIR" ]; then BUILDDIR="`pwd`/build"; fi
	echo "  builddir:      $BUILDDIR"

	if [ -z "$PREFIX" ]; then PREFIX="/usr/local"; fi
	echo "  prefix:        $PREFIX"
}


#
# Package/program finding
#

# $1: name of package variable, sans _PKG
# $2: name of package to attempt to use
try_use_pkg() {
	if [ -z `eval echo '"$'${1}'_PKG"'` ]; then
		if $PKGCONF --exists "$2"
		then
			echo "$2"
			eval "${1}_PKG=${2}"					
		fi
	fi
}

# Sets a NO_xyz flag if a package is unavailable.
#
# $1: name of package variable, sans _PKG
# $2: name of feature to disable if $1 empty
disable_if_no_pkg() {
	if [ -z `eval echo '"$'${1}'_PKG"'` ]; then
		echo "no package; disabling ${2}"
		eval "NO_${2}='1'"
	fi
}

# Tries to find needed programs.
find_progs() {
	echo "PROGRAMS:"
	find_cc
	find_cxx
	find_gnumake
	find_pkgconf
}

# If $2 exists, sets $1 to it.
find_prog() {
	if [ -z `eval echo '"$'${1}'"'` ]; then
		if which "${2}" >/dev/null
		then
			eval "${1}=`which ${2}`"
		fi
	fi
}

# Tries to find a sane C compiler.
#
# Stores the result in $CC if successful.
# Halts the script on failure.
find_cc() {
	echo -n "  C compiler:    "
	find_prog CC clang
	find_prog CC gcc

	if [ -n "$CC" ]; then
		echo "$CC"
	else
		echo "not found"
		exit 1
	fi
}

# Tries to find a sane C++ compiler.
#
# Stores the result in $CXX if successful.
# Halts the script on failure.
find_cxx() {
	echo -n "  C++ compiler:  "
	find_prog CXX clang++
	find_prog CXX g++

	if [ -n "$CXX" ]; then
		echo "$CXX"
	else
		echo "not found"
		exit 1
	fi
}

# Tries to find a sane pkgconf/pkg-config.
#
# Stores the result in $PKGCONF if successful.
# Halts the script on failure.
find_pkgconf() {
	echo -n "  pkgconf:       "
	find_prog PKGCONF pkgconf
	find_prog PKGCONF pkg-config

	if [ -n "$PKGCONF" ]; then
		echo "$PKGCONF"
	else
		echo "not found"
		exit 1
	fi
}

# Tries to find a sane GNU make.
#
# Stores the result in $MAKE if successful.
# Halts the script on failure.
find_gnumake() {
	echo -n "  GNU Make:      "
	find_prog MAKE gmake
	find_prog MAKE gnumake
	find_prog MAKE make

	if [ -n "$MAKE" ]; then
		echo "$MAKE"
	else
		echo "not found"
		exit 1
	fi
}

# Tries to find all dependencies.
find_deps() {
	echo "DEPENDENCIES:"
	find_flac
	find_mp3
	find_sndfile
	find_sdl2
	find_libuv
}

# Finds FLAC++ to provide FLAC support, if requested.
find_flac() {
	echo -n "  FLAC++:        "

	if [ -n "$NO_FLAC" ]; then
		echo "FLAC disabled; skipping"
		return
	fi

	try_use_pkg       FLACXX "FLAC++"
	try_use_pkg       FLACXX "flac++"
	disable_if_no_pkg FLACXX FLAC
}

# Finds libmpg123 to provide MP3 support, if requested.
find_mp3() {
	echo -n "  libmpg123:     "

	if [ -n "$NO_MP3" ]; then
		echo "MP3 disabled; skipping"
		return
	fi

	try_use_pkg       LIBMPG123 "libmpg123"
	disable_if_no_pkg LIBMPG123 MP3
}

# Finds libsndfile, if requested.
find_sndfile() {
	echo -n "  libsndfile:    "

	if [ -n "$NO_SNDFILE" ]; then
		echo "sndfile disabled; skipping"
		return
	fi

	try_use_pkg       LIBSNDFILE "libsndfile"
	try_use_pkg       LIBSNDFILE "sndfile"
	disable_if_no_pkg LIBSNDFILE SNDFILE
}

# Finds SDL2
find_sdl2() {
	echo -n "  SDL2:          "

	try_use_pkg SDL2 "sdl2"

	if [ -z "$SDL2_PKG" ]; then
		echo "not found; cannot continue"
		exit 2
	fi
}

# Finds libuv
find_libuv() {
	echo -n "  libuv:         "

	try_use_pkg LIBUV "libuv"

	if [ -z "$LIBUV_PKG" ]; then
		echo "not found; cannot continue"
		exit 2
	fi
}

#
# Feature listing
#

# If $2 is not set, adds $1 to $FORMATS and $3 as a compile flag to $FCFLAGS.
add_format_to_lists() {
	if [ -z `eval echo '$'"$2"` ]; then
		FORMATS=`printf "%s\n%s" "$FORMATS" "$1"`
		FCFLAGS=`printf "%s\n%s" "$FCFLAGS" "-D$3"`
	fi

}

# Lists features on stdout.
list_features() {
	FORMATS=""
	FCFLAGS=""

	add_format_to_lists flac NO_FLAC    WITH_FLAC
	add_format_to_lists mp3  NO_MP3     WITH_MP3
	add_format_to_lists flac NO_SNDFILE WITH_SNDFILE
	add_format_to_lists ogg  NO_SNDFILE WITH_SNDFILE
	add_format_to_lists wav  NO_SNDFILE WITH_SNDFILE

	# Sort, uniquify, space-delimit and strip formats/flags
	FORMATS=`echo "$FORMATS" | sort | uniq | tr -s "\n" " " | sed -e 's/^ //g' -e 's/ $//g'`
	FCFLAGS=`echo "$FCFLAGS" | sort | uniq | tr -s "\n" " " | sed -e 's/^ //g' -e 's/ $//g'`

	# No point building playd with no file formats!
	if [ -z "$FORMATS" ]; then
		echo "no file formats available; cannot continue"
		exit 3
	fi

	echo "FILE FORMATS:"
	echo "  $FORMATS"
	if [ -n "$FCFLAGS" ]; then echo "  (CFLAGS: $FCFLAGS)"; fi
	
}

# Collates the pkg-config packages into $PACKAGES.
# Also lists on stdout.
list_packages() {
	PACKAGES=`echo "$FLACXX_PKG $LIBMPG123_PKG $LIBSNDFILE_PKG $SDL2_PKG $LIBUV_PKG" | sed 's/  */ /g'`
	echo "PACKAGES USED:"
	echo "  $PACKAGES"
}


#
# Makefile making
#

# If $1 is non-empty, adds to cxx_expr a rule removing filenames containing $2.
disable_feature_files() {
	if [ -n "$1" ]; then
		cxx_expr="${cxx_expr} -a \( \! -name '*"$2"*' \)"
	fi
}

# Finds all relevant sources.
# Outputs to the variables $CXXSOURCES and $CSOURCES.
find_sources() {
	# Start off by looking for all cpp or cxx files.
	cxx_expr="\( -name "*.cpp" -o -name "*.cxx" \)"

	# Disable feature files if those features are disabled.
	disable_feature_files "${NO_FLAC}"    flac
	disable_feature_files "${NO_MP3}"     mp3
	disable_feature_files "${NO_SNDFILE}" sndfile

	CXXSOURCES=`eval find "$SRCDIR" "$cxx_expr"`

	# Need to backslash-escape slashes so the upcoming seds work.
	sd=`echo "$SRCDIR" | sed 's|/|\\\\/|g'`

	# Remove test code.
	CXXSOURCES=`echo "$CXXSOURCES" | sed '/'"$sd"'\/tests/d'`

	# Compared to above, the C sources are easy--they're always there,
	# regardless of features.
	CSOURCES=`find "$SRCDIR" -name '*.c'`
}

write_makefile() {
	echo "Now making the Makefile."

	find_sources

	# Make sure the sources are all on one line.
	CXXSOURCES=`echo "$CXXSOURCES" | tr "\n" " "`
	CSOURCES=`echo "$CSOURCES" | tr "\n" " "`

	# Generate object sets:
	# $SRCDIR/.../foo.cxx => $BUILDDIR/.../foo.o
	# $SRCDIR/.../bar.cpp => $BUILDDIR/.../bar.o
	# $SRCDIR/.../baz.c   => $BUILDDIR/.../baz.o
	CXXOBJECTS=`echo "$CXXSOURCES" | sed -e "s|$SRCDIR/|$BUILDDIR/|g" -e 's|\.[^ ]*|.o|g'`
	COBJECTS=`echo "$CSOURCES" | sed -e "s|$SRCDIR/|$BUILDDIR/|g" -e 's|\.[^ ]*|.o|g'`

	cat Makefile.in |
		sed -e "s|%%COBJECTS%%|$COBJECTS|g"	\
		    -e "s|%%CSOURCES%%|$CSOURCES|g"	\
		    -e "s|%%CXXOBJECTS%%|$CXXOBJECTS|g"	\
		    -e "s|%%CXXSOURCES%%|$CXXSOURCES|g" \
		    -e "s|%%FCFLAGS%%|$FCFLAGS|g"	\
		    -e "s|%%PACKAGES%%|$PACKAGES|g"	\
		    -e "s|%%PROGNAME%%|$PROGNAME|g"	\
		    -e "s|%%PROGVER%%|$PROGVER|g"	\
		    -e "s|%%SRCDIR%%|$SRCDIR|g"		\
		    -e "s|%%BUILDDIR%%|$BUILDDIR|g"	\
		    -e "s|%%CC%%|$CC|g"			\
		    -e "s|%%CXX%%|$CXX|g"		> Makefile
}

#
# Main script
#

find_progs
echo
clean
find_name_and_version
echo
find_dirs
echo
find_deps
echo
list_features
echo
list_packages
echo
write_makefile
echo
echo "If this is what you wanted, now run ${MAKE}."
echo "Else, fix any environment problems and re-run $0."
