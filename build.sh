#!/usr/bin/env bash
# $Id: build.sh 4734 2014-01-07 12:33:14Z luigi $
#
# Copyright (c) 2005-2011 Martin Schröder <martin@luatex.org>
# Copyright (c) 2009-2011 Taco Hoekwater <taco@luatex.org>
# Copyright (c) 2013-2013 Luigi Scarso and Hans Hagen (mingw stuff)
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# new script to build luatex binaries
# ----------
# Options:
#      --make      : only make, no make distclean; configure
#      --parallel  : make -j 4 -l 5.0
#      --nostrip   : do not strip binary
#      --warnings= : enable compiler warnings
#      --mingw     : crosscompile for mingw32 from i-386linux
#      --mingw32   : crosscompile 32 binary on unix
#      --mingw64   : crosscompile 64 binary on unix
#      --msys      : crosscompile 32 binary on windows
#      --msys32    : crosscompile 32 binary on windows
#      --msys64    : crosscompile 64 binary on windows
#      --host=     : target system for mingw32 cross-compilation
#      --build=    : build system for mingw32 cross-compilation
#      --arch=     : crosscompile for ARCH on OS X
#      --debug     : CFLAGS="-g -O0" CXXFLAGS="-g -O0"--warnings=max --nostrip

# L/H: we assume a 64 bit build system .. it took us two days of experimenting with all
# kind of permutations to figure it out .. partly due to tangle/tie dependencies and
# we're not there yes as there are messages with respect to popen and so
#
# todo:
#
# - generate ctangle and tie
# - get rid of otangle and tangle dependencies
# - don't generate bins we don't need
# - maybe make cairo in mplib optional
#
# Patch suggested  by Fabrice Popineau in texk/web2c/lib/lib.h: #define eof weof

$DEBUG

# try to find bash, in case the standard shell is not capable of
# handling the generated configure's += variable assignments
if which bash >/dev/null
then
 CONFIG_SHELL=`which bash`
 export CONFIG_SHELL
fi

# try to find gnu make; we may need it
MAKE=make;
if make -v 2>&1| grep "GNU Make" >/dev/null
then
  echo "Your make is a GNU-make; I will use that"
elif gmake -v >/dev/null 2>&1
then
  MAKE=gmake;
  export MAKE;
  echo "You have a GNU-make installed as gmake; I will use that"
else
  echo "I can't find a GNU-make; I'll try to use make and hope that works."
  echo "If it doesn't, please install GNU-make."
fi

ONLY_MAKE=FALSE
STRIP_LUATEX=TRUE
WARNINGS=yes
MINGWCROSS=FALSE
MINGWCROSS64=FALSE
MINGWMSYS=FALSE
MINGWMSYS64=FALSE
CONFHOST=
CONFBUILD=
MACCROSS=FALSE
JOBS_IF_PARALLEL=${JOBS_IF_PARALLEL:-4}
MAX_LOAD_IF_PARALLEL=${MAX_LOAD_IF_PARALLEL:-5.0}

CFLAGS="$CFLAGS"
CXXFLAGS="$CXXFLAGS"

until [ -z "$1" ]; do
  case "$1" in
    --make      ) ONLY_MAKE=TRUE     ;;
    --nostrip   ) STRIP_LUATEX=FALSE ;;
    --debug     ) STRIP_LUATEX=FALSE; WARNINGS=max ; CFLAGS="-g3 -g -O0 $CFLAGS" ; CXXFLAGS="-g3 -g -O0 $CXXFLAGS" ;;
    --warnings=*) WARNINGS=`echo $1 | sed 's/--warnings=\(.*\)/\1/' `        ;;
    --mingw     ) MINGWCROSS=TRUE    ;;
    --mingw32   ) MINGWCROSS=TRUE    ;;
    --mingw64   ) MINGWCROSS64=TRUE  ;;
    --msys      ) MINGWMSYS=TRUE     ;;
    --msys32    ) MINGWMSYS=TRUE     ;;
    --msys64    ) MINGWMSYS64=TRUE   ;;
    --host=*    ) CONFHOST="$1"      ;;
    --build=*   ) CONFBUILD="$1"     ;;
    --parallel  ) MAKE="$MAKE -j $JOBS_IF_PARALLEL -l $MAX_LOAD_IF_PARALLEL" ;;
    --arch=*    ) MACCROSS=TRUE; ARCH=`echo $1 | sed 's/--arch=\(.*\)/\1/' ` ;;
    *           ) echo "ERROR: invalid build.sh parameter: $1"; exit 1       ;;
  esac
  shift
done

#
STRIP=strip
LUATEXEXE=luatex

case `uname` in
  MINGW64*    ) LUATEXEXE=luatex.exe ;;
  MINGW32*    ) LUATEXEXE=luatex.exe ;;
  CYGWIN*     ) LUATEXEXE=luatex.exe ;;
esac

WARNINGFLAGS=--enable-compiler-warnings=$WARNINGS

B=build

OLDPATH=$PATH

if [ "$MINGWCROSS" = "TRUE" ]
then
  B=build-windows
  PATH=`pwd`/extrabin/mingw:$PATH
  LUATEXEXE=luatex.exe
  CFLAGS="-mtune=nocona -g -O3 $CFLAGS"
  CXXFLAGS="-mtune=nocona -g -O3 $CXXFLAGS"
  CONFHOST="--host=i586-mingw32msvc"
  CONFBUILD="--build=x86_64-unknown-linux-gnu"
  STRIP="${CONFHOST#--host=}-strip"
  LDFLAGS="-Wl,--large-address-aware $CFLAGS"
  export CFLAGS CXXFLAGS LDFLAGS
fi

if [ "$MINGWCROSS64" = "TRUE" ]
then
  B=build-windows64
  PATH=`pwd`/extrabin/mingw:$PATH
  LUATEXEXE=luatex.exe
  CFLAGS="-mtune=nocona -g -O3 -static-libgcc -static-libstdc++ $CFLAGS"
  CXXFLAGS="-mtune=nocona -g -O3 $CXXFLAGS"
  CONFHOST="--host=x86_64-w64-mingw32"
  CONFBUILD="--build=x86_64-unknown-linux-gnu"
  STRIP="${CONFHOST#--host=}-strip"
  LDFLAGS="$CFLAGS"
  export CFLAGS CXXFLAGS LDFLAGS
fi

if [ "$MINGWMSYS" = "TRUE" ]
then
  B=build-win32
  PATH=`pwd`/extrabin/msys:$PATH
  LUATEXEXE=luatex.exe
  STRIP=strip.exe
  CFLAGS="-mtune=nocona -m32 -g -O3 $CFLAGS"
  CXXFLAGS="-mtune=nocona -m32 -g -O3 $CXXFLAGS"
  CONFHOST="--host=i586-mingw32"
  CONFBUILD="--build=x86_64-w64-mingw32"
  LDFLAGS="-m32"
  CPPFLAGS="-m32"
  export CFLAGS CXXFLAGS LDFLAGS CPPFLAGS
fi

if [ "$MINGWMSYS64" = "TRUE" ]
then
  B=build-win64
  PATH=`pwd`/extrabin/mingw:$PATH
  LUATEXEXE=luatex.exe
  STRIP=strip.exe
  CFLAGS="-mtune=nocona -g -O3 $CFLAGS"
  CXXFLAGS="-mtune=nocona -g -O3 $CXXFLAGS"
  CONFHOST="--host=x86_64-w64-mingw32"
  CONFBUILD="--build=x86_64-w64-mingw32"
  LDFLAGS=""
  CPPFLAGS=""
  export CFLAGS CXXFLAGS LDFLAGS CPPFLAGS
fi

if [ "$MACCROSS" = "TRUE" ]
then
  # make sure that architecture parameter is valid
  case $ARCH in
    i386 | x86_64 | ppc | ppc64 ) ;;
    * ) echo "ERROR: architecture $ARCH is not supported"; exit 1;;
  esac
  B=build-$ARCH
  CFLAGS="-arch $ARCH -g -O2 $CFLAGS"
  CXXFLAGS="-arch $ARCH -g -O2 $CXXFLAGS"
  LDFLAGS="-arch $ARCH $LDFLAGS"
  export CFLAGS CXXFLAGS LDFLAGS
fi

if [ "$STRIP_LUATEX" = "FALSE" ]
then
    export CFLAGS
    export CXXFLAGS
fi

# ----------
# clean up, if needed
if [ -r "$B"/Makefile -a $ONLY_MAKE = "FALSE" ]
then
  rm -rf "$B"
elif [ ! -r "$B"/Makefile ]
then
    ONLY_MAKE=FALSE
fi
if [ ! -r "$B" ]
then
  mkdir "$B"
fi
#
# get a new svn version header
if [ "$WARNINGS" = "max" ]
then
   rm -f source/texk/web2c/luatexdir/luatex_svnversion.h
fi
( cd source  ; ./texk/web2c/luatexdir/getluatexsvnversion.sh )

cd "$B"

if [ "$ONLY_MAKE" = "FALSE" ]
then
TL_MAKE=$MAKE ../source/configure  $CONFHOST $CONFBUILD  $WARNINGFLAGS\
    --enable-cxx-runtime-hack \
    --enable-silent-rules \
    --disable-all-pkgs \
    --disable-shared    \
    --disable-largefile \
    --disable-ptex \
    --disable-ipc \
    --disable-linked-scripts \
    --enable-dump-share  \
    --enable-mp  \
    --enable-luatex  \
    --without-system-harfbuzz \
    --without-system-ptexenc \
    --without-system-kpathsea \
    --without-system-poppler \
    --without-system-xpdf \
    --without-system-freetype \
    --without-system-freetype2 \
    --without-system-gd \
    --without-system-libpng \
    --without-system-teckit \
    --without-system-zlib \
    --without-system-t1lib \
    --without-system-icu \
    --without-system-graphite \
    --without-system-zziplib \
    --without-mf-x-toolkit --without-x \
   || exit 1
fi

$MAKE

# the fact that these makes inside libs/ have to be done manually for the cross
# compiler hints that something is wrong in the --enable/--disable switches above,
# but I am too lazy to look up what is wrong exactly.
# (perhaps more files needed to be copied from TL?)

(cd libs; $MAKE )
(cd libs/zziplib; $MAKE all )
(cd libs/zlib; $MAKE all )
(cd libs/libpng; $MAKE all )
(cd libs/poppler; $MAKE all )

(cd texk/kpathsea; $MAKE )
(cd texk; $MAKE )
(cd texk/web2c; $MAKE $LUATEXEXE )

# go back
cd ..

if [ "$STRIP_LUATEX" = "TRUE" ] ;
then
  $STRIP "$B"/texk/web2c/$LUATEXEXE
else
  echo "luatex binary not stripped"
fi

PATH=$OLDPATH

# show the results
ls -l "$B"/texk/web2c/$LUATEXEXE
