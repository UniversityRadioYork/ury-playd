#!/bin/sh
#
# This file is part of playd.
# playd is licensed under the MIT licence: see COPYING.txt.
#
#
# Configuration script for playd.
#
#
# Variables:
#   Programs:
#     PKGCONF........................pkg-config or equivalent
#
#   Package name overrides:
#     FLACXX_PKG....................FLAC++ pkg-config package
#     LIBMPG123_PKG..............libmpg123 pkg-config package
#     LIBVORBISFILE_PKG......libvorbisfile pkg-config package
#     PORTAUDIO_PKG..............PortAudio pkg-config package
#     PORTAUDIOCXX_PKG.........PortAudio++ pkg-config package
#
#   File format flags (set to non-empty string to activate):
#     NO_FLAC..............................don't support FLAC
#     NO_MP3................................don't support MP3
#     NO_OGG.........................don't support Ogg Vorbis
#
#   Other flags (set to non-empty string to activate):
#     NO_SYS_PORTAUDIOCXX....use bundled PortAudio++ bindings
#                                         instead of system's
#
# Notes:
#   - lack of FLAC++ implies NO_FLAC;
#   - lack of libmpg123 implies NO_MP3;
#   - lack of libvorbisfile implies NO_OGG;
#   - lack of portaudio++ implies NO_SYS_PORTAUDIOCXX.
#   - lack of pkgconf/pkg-config or portaudio is fatal.


#
# Package/program finding
#

# $1: name of package variable, sans _PKG
# $2: name of package to attempt to use
try_use_pkg()
{
	if [ -z `eval echo '"$'${1}'_PKG"'` ]
	then
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
disable_if_no_pkg()
{
	if [ -z `eval echo '"$'${1}'_PKG"'` ]
	then
		echo "no package; disabling ${2}"
		eval "NO_${2}='1'"
	fi
}

# Tries to find all dependencies.
find_deps()
{
	echo "DEPENDENCIES:"
	find_pkgconf
	find_flac
	find_mp3
	find_ogg
	find_portaudio
	find_portaudiocxx
}

# Tries to find a sane pkgconf/pkg-config.
#
# Stores the result in $PKGCONF if successful.
# Halts the script on failure.
find_pkgconf()
{
	echo -n "  pkgconf:       "

	if [ -n "$PKGCONF" ]
	then
		echo "$PKGCONF"
	elif which pkgconf
	then
		PKGCONF=`which pkgconf`
	elif which pkg-config
	then
		PKGCONF=`which pkg-config`
	else
		echo "not found"
		exit 1
	fi
}

# Finds FLAC++ to provide FLAC support, if requested.
find_flac()
{
	echo -n "  FLAC++:        "

	if [ -n "$NO_FLAC" ]
	then
		echo "FLAC disabled; skipping"
		return
	fi

	try_use_pkg FLACXX "FLAC++"
	try_use_pkg FLACXX "flac++"
	disable_if_no_pkg FLACXX FLAC
}

# Finds libmpg123 to provide MP3 support, if requested.
find_mp3()
{
	echo -n "  libmpg123:     "

	if [ -n "$NO_MP3" ]
	then
		echo "MP3 disabled; skipping"
		return
	fi

	try_use_pkg LIBMPG123 "libmpg123"
	disable_if_no_pkg LIBMPG123 MP3
}

# Finds libvorbisfile to provide Ogg support, if requested.
find_ogg()
{
	echo -n "  libvorbisfile: "

	if [ -n "$NO_OGG" ]
	then
		echo "ogg disabled; skipping"
		return
	fi

	try_use_pkg LIBVORBISFILE "libvorbisfile"
	try_use_pkg LIBVORBISFILE "vorbisfile"
	disable_if_no_pkg LIBVORBISFILE OGG
}

# Finds PortAudio.
find_portaudio()
{
	echo -n "  PortAudio:     "

	try_use_pkg PORTAUDIO "portaudio2"
	try_use_pkg PORTAUDIO "portaudio-2.0"

	if [ -z "$PORTAUDIO_PKG" ]
	then
		echo "not found; cannot continue"
		exit 2
	fi
}

# Finds any available C++ bindings for PortAudio.
find_portaudiocxx()
{
	echo -n "  PortAudioC++:  "

	try_use_pkg PORTAUDIOCXX "portaudiocpp"
	disable_if_no_pkg PORTAUDIOCXX SYS_PORTAUDIOCXX
}


#
# Feature listing
#

# If $2 is not set, adds $1 to $FORMATS.
# Else, adds $2 as a compile flag to $FCFLAGS.
add_format_to_lists()
{
	if [ -z `eval echo '$'"$2"` ]
	then
		FORMATS="$FORMATS $1"
	else
		FCFLAGS="$FCFLAGS -D$2"
	fi

}

# Lists features on stdout.
list_features()
{
	echo -n "Using "
	if [ -z "$NO_SYS_PORTAUDIOCXX"]
	then
		echo -n "system "
	else
		echo -n "bundled "
	fi
	echo "PortAudio C++ bindings."

	echo

	FORMATS=""
	FCFLAGS=""

	add_format_to_lists flac NO_FLAC
	add_format_to_lists mp3 NO_MP3
	add_format_to_lists ogg NO_OGG

	# Strip off trailing spaces, if any.
	FORMATS=`echo "$FORMATS" | sed 's/^ //g'`
	FCFLAGS=`echo "$FCFLAGS" | sed 's/^ //g'`

	# No point building playd with no file formats!
	if [ -z "$FORMATS" ]
	then
		echo "no file formats available; cannot continue"
		exit 3
	fi

	echo "FILE FORMATS:"
	echo "  $FORMATS"
	if [ -n "$FCFLAGS" ]; then echo "  (CFLAGS: $FCFLAGS)"; fi
	
}

# Collates the pkg-config packages into $PACKAGES.
# Also lists on stdout.
list_packages()
{
	PACKAGES=`echo "$FLACXX_PKG $LIBMPG123_PKG $LIBVORBISFILE_PKG $PORTAUDIO_PKG $PORTAUDIOCXX_PKG" | sed 's/  */ /g'`
	echo "PACKAGES USED:"
	echo "  $PACKAGES"
}


#
# Makefile making
#

write_makefile()
{
	echo "Now making the Makefile."
	cat Makefile.in |
		sed "s/%%PACKAGES%%/$PACKAGES/g" |
		sed "s/%%FCFLAGS%%/$FCFLAGS/g" |
		sed "s/%%NO_SYS_PORTAUDIOCXX%%/$NO_SYS_PORTAUDIOCXX/g" > Makefile
}

#
# Main script
#

find_deps

echo

list_features

echo

list_packages

echo

write_makefile

echo
echo "If this is what you wanted, now run GNU make."
echo "Else, fix any environment problems and re-run $0."
