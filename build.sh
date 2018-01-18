#!/usr/bin/env bash
#
# Copyright (c) 2005-2011 Martin Schröder <martin@luatex.org>
# Copyright (c) 2009-2014 Taco Hoekwater <taco@luatex.org>
# Copyright (c) 2012-2014 Luigi Scarso   <luigi@luatex.org>
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
#      --jit       : also build luajittex
#      --nojit     : don't build luajit 
#      --make      : only make, no make distclean; configure
#      --parallel  : make -j 8 -l 8.0
#      --nostrip   : do not strip binary
#      --warnings= : enable compiler warnings
#      --lua52     : build luatex  with luatex 52
#      --nolua52   : don't build luatex  with luatex 52
#      --lua53     : build luatex  with luatex 53
#      --nolua53   : don't build luatex  with luatex 53
#      --mingw     : crosscompile for mingw32 from x86_64linux
#      --mingw32   : crosscompile for mingw32 from x86_64linux
#      --mingw64   : crosscompile for mingw64 from x86_64linux
#      --host=     : target system for mingw32 cross-compilation
#      --build=    : build system for mingw32 cross-compilation
#      --arch=     : crosscompile for ARCH on OS X
#      --clang     : use clang & clang++
#      --debug     : CFLAGS="-g -O0" --warnings=max --nostrip
#      --debugopt  : CFLAGS="-g -O3" --warnings=max --nostrip
$DEBUG
#export CFLAGS="-D_FORTIFY_SOURCE=2 -O3"
#export CXXFLAGS="-D_FORTIFY_SOURCE=2 -O3"

# try to find bash, in case the standard shell is not capable of
# handling the generated configure's += variable assignments
if which bash >/dev/null
then
 CONFIG_SHELL=`which bash`
 export CONFIG_SHELL
fi

# try to find gnu make; we may need it
MAKE="make V=1"
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

BUILDJIT=FALSE
BUILDLUA52=FALSE
BUILDLUA53=TRUE
ONLY_MAKE=FALSE
STRIP_LUATEX=TRUE
WARNINGS=yes
MINGW=FALSE
MINGWCROSS=FALSE
MINGWCROSS64=FALSE
MACCROSS=FALSE
CLANG=FALSE
CONFHOST=
CONFBUILD=
JOBS_IF_PARALLEL=${JOBS_IF_PARALLEL:-8}
MAX_LOAD_IF_PARALLEL=${MAX_LOAD_IF_PARALLEL:-8}
TARGET_CC=gcc
TARGET_TCFLAGS=

CFLAGS="$CFLAGS"
CXXFLAGS="$CXXFLAGS"

# poppler 0.55 needs c++11 
# for the moment we put the flag here
# export CXXFLAGS="-std=c++11 $CXXFLAGS"

until [ -z "$1" ]; do
  case "$1" in
    --jit       ) BUILDJIT=TRUE     ;;
    --nojit     ) BUILDJIT=FALSE     ;;
    --make      ) ONLY_MAKE=TRUE     ;;
    --nostrip   ) STRIP_LUATEX=FALSE ;;
    --debugopt  ) STRIP_LUATEX=FALSE; WARNINGS=max ; CFLAGS="-O3 -g -ggdb3 $CFLAGS" ; CXXFLAGS="-O3 -g -ggdb $CXXFLAGS"  ;;
    --debug     ) STRIP_LUATEX=FALSE; WARNINGS=max ; CFLAGS="-O0 -g -ggdb3 $CFLAGS" ; CXXFLAGS="-O0 -g -ggdb $CXXFLAGS"  ;;
    --clang     ) export CC=clang; export CXX=clang++ ; TARGET_CC=$CC ; CLANG=TRUE ;;
    --warnings=*) WARNINGS=`echo $1 | sed 's/--warnings=\(.*\)/\1/' `        ;;
    --lua52     ) BUILDLUA52=TRUE    ;;
    --nolua52   ) BUILDLUA52=FALSE   ;;
    --lua53     ) BUILDLUA53=TRUE    ;;
    --nolua53   ) BUILDLUA53=FALSE   ;;
    --mingw     ) MINGWCROSS=TRUE    ;;
    --mingw32   ) MINGWCROSS=TRUE    ;;
    --mingw64   ) MINGWCROSS64=TRUE  ;;
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
LUATEXEXEJIT=luajittex
LUATEXEXE=luatex
LUATEXEXE53=luatex53


case `uname` in
    MINGW*   ) LUATEXEXEJIT=luajittex.exe ; LUATEXEXE=luatex.exe ; LUATEXEXE53=luatex53.exe ;;
   MINGW64*   ) LUATEXEXEJIT=luajittex.exe ; LUATEXEXE=luatex.exe ; LUATEXEXE53=luatex53.exe ;;
  MINGW32*   ) LUATEXEXEJIT=luajittex.exe ; LUATEXEXE=luatex.exe ; LUATEXEXE53=luatex53.exe ;;
  CYGWIN*    ) LUATEXEXEJIT=luajittex.exe ; LUATEXEXE=luatex.exe ; LUATEXEXE53=luatex53.exe ;;
  Darwin     ) STRIP="strip -u -r" ;;
esac

WARNINGFLAGS=--enable-compiler-warnings=$WARNINGS

B=build

## Useful for cross-compilation for ARM
if [ "x$CONFHOST" != "x" ]
then
 B="build-$CONFHOST"
 B=`printf "$B"| sed 's/--host=//'`
fi

if [ "$CLANG" = "TRUE" ]
then
  B=build-clang
fi

OLDPATH=$PATH
if [ "$MINGWCROSS64" = "TRUE" ]
then
  B=build-windows64
  LUATEXEXEJIT=luajittex.exe
  LUATEXEXE=luatex.exe
  LUATEXEXE53=luatex53.exe
  PATH=/usr/mingw32/bin:$PATH
  PATH=`pwd`/extrabin/mingw:$PATH
  CFLAGS="-mtune=nocona -g -O3 -fno-lto -fno-use-linker-plugin $CFLAGS"
  CXXFLAGS="-mtune=nocona -g -O3 -fno-lto -fno-use-linker-plugin $CXXFLAGS"
  : ${CONFHOST:=--host=x86_64-w64-mingw32}
  : ${CONFBUILD:=--build=x86_64-unknown-linux-gnu}
  RANLIB="${CONFHOST#--host=}-ranlib"
  STRIP="${CONFHOST#--host=}-strip"
  LDFLAGS="${LDFLAGS} -fno-lto -fno-use-linker-plugin -static-libgcc -static-libstdc++"
  export CFLAGS CXXFLAGS LDFLAGS
fi

if [ "$MINGWCROSS" = "TRUE" ]
then
  B=build-windows
  LUATEXEXEJIT=luajittex.exe
  LUATEXEXE=luatex.exe
  LUATEXEXE53=luatex53.exe
  PATH=/usr/mingw32/bin:$PATH
  PATH=`pwd`/extrabin/mingw:$PATH
  CFLAGS="-mtune=nocona -g -O3 $CFLAGS"
  CXXFLAGS="-mtune=nocona -g -O3 $CXXFLAGS"
  : ${CONFHOST:=--host=i686-w64-mingw32}
  : ${CONFBUILD:=--build=x86_64-unknown-linux-gnu}
  RANLIB="${CONFHOST#--host=}-ranlib"
  STRIP="${CONFHOST#--host=}-strip"
  LDFLAGS="-Wl,--large-address-aware -Wl,--stack,2621440 $CFLAGS"
  export CFLAGS CXXFLAGS LDFLAGS BUILDCXX BUILDCC
fi


if [ "$MACCROSS" = "TRUE" ]
then
  # make sure that architecture parameter is valid
  case $ARCH in
    i386 | x86_64 | ppc | ppc64 ) ;;
    * ) echo "ERROR: architecture $ARCH is not supported"; exit 1;;
  esac
  B=build-$ARCH
  if [ "x$CONFHOST" != "x" ]
    then
    B="build-$CONFHOST"
    B=`printf "$B"| sed 's/--host=//'`
  fi
  CFLAGS="-arch $ARCH -g -O2 $CFLAGS"
  CXXFLAGS="-arch $ARCH -g -O2 $CXXFLAGS"
  LDFLAGS="-arch $ARCH $LDFLAGS" 
  STRIP="${CONFHOST#--host=}-strip -u -r -A -n"
  #STRIP="${CONFHOST#--host=}-strip"
  export CFLAGS CXXFLAGS LDFLAGS
fi


### Dirty trick to check  Darwin X86_64
# TARGET_TESTARCH=$( ($TARGET_CC $TARGET_TCFLAGS -E source/libs/luajit/luajit-2.0.2/src/lj_arch.h -dM|grep -q LJ_TARGET_X64 && echo x64) || echo NO)
# HOST_SYS=$(uname -s)
# echo HOST_SYS=$HOST_SYS
# echo TARGET_TESTARCH=$TARGET_TESTARCH
# if [ $HOST_SYS == "Darwin" ]  
# then
#  if [ $TARGET_TESTARCH == "x64" ] 
#  then
#    export LDFLAGS="-pagezero_size 10000 -image_base 100000000  $LDFLAGS"
#    echo Setting LDFLAGS=$LDFLAGS
#  fi
# fi


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
#if [ "$WARNINGS" = "max" ]
#then
#    rm -f source/texk/web2c/luatexdir/luatex_svnversion.h
#fi
#( cd source  ; ./texk/web2c/luatexdir/getluatexsvnversion.sh )

cd "$B"

JITENABLE=
if [ "$BUILDJIT" = "TRUE" ]
then
  JITENABLE="--enable-luajittex --without-system-luajit "
fi

LUA52ENABLE=
if [ "$BUILDLUA52" = "TRUE" ]
then
  LUA52ENABLE="--enable-luatex"
fi

LUA53ENABLE=--enable-luatex53
if [ "$BUILDLUA53" = "FALSE" ]
then
  LUA53ENABLE=
fi




if [ "$ONLY_MAKE" = "FALSE" ]
then
TL_MAKE=$MAKE ../source/configure  $CONFHOST $CONFBUILD  $WARNINGFLAGS\
    --enable-cxx-runtime-hack \
    --enable-silent-rules \
    --disable-all-pkgs \
    --disable-shared    \
    --disable-ptex \
    --disable-largefile \
    --disable-ipc \
    --enable-dump-share  \
    --enable-coremp  \
    --enable-web2c  \
    --enable-dctdecoder=libjpeg --enable-libopenjpeg=openjpeg2 \
    $LUA52ENABLE  $LUA53ENABLE  $JITENABLE \
    --without-system-cairo  \
    --without-system-pixman \
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

(cd libs; $MAKE all )
(cd libs/zziplib; $MAKE all )
(cd libs/zlib; $MAKE all )
(cd libs/libpng; $MAKE all )
(cd libs/poppler; $MAKE all )
(cd texk; $MAKE web2c/Makefile)
(cd texk/kpathsea; $MAKE )
if [ "$BUILDJIT" = "TRUE" ]
then
  (cd libs/luajit; $MAKE all )
  (cd texk/web2c; $MAKE $LUATEXEXEJIT)
fi

if [ "$BUILDLUA52" = "TRUE" ]
then
  (cd texk/web2c; $MAKE $LUATEXEXE )
fi

if [ "$BUILDLUA53" = "TRUE" ]
then
  (cd texk/web2c; $MAKE $LUATEXEXE53 )
fi


# go back
cd ..

if [ "$STRIP_LUATEX" = "TRUE" ] ;
then
    if [ "$BUILDJIT" = "TRUE" ]
    then
	$STRIP "$B"/texk/web2c/$LUATEXEXEJIT
    fi
    if [ "$BUILDLUA52" = "TRUE" ]
    then
	$STRIP "$B"/texk/web2c/$LUATEXEXE
    fi
    if [ "$BUILDLUA53" = "TRUE" ]
    then
	$STRIP "$B"/texk/web2c/$LUATEXEXE53
    fi
else
  echo "lua(jit)tex binary not stripped"
fi

if [ "$MINGWCROSS" = "TRUE" ]
then
  PATH=$OLDPATH
fi

# show the result
if [ "$BUILDJIT" = "TRUE" ]
then
ls -l "$B"/texk/web2c/$LUATEXEXEJIT
fi
if [[ ("$BUILDLUA52" = "TRUE") && ("$BUILDLUA53" = "TRUE") ]]
then
    ls -l "$B"/texk/web2c/$LUATEXEXE
    ls -l "$B"/texk/web2c/$LUATEXEXE53
fi
if [[ ("$BUILDLUA52" = "TRUE") && ("$BUILDLUA53" = "FALSE") ]]
then
    ls -l "$B"/texk/web2c/$LUATEXEXE
fi
if [[ ("$BUILDLUA52" = "FALSE") && ("$BUILDLUA53" = "TRUE") ]]
then
    mv -v  "$B"/texk/web2c/$LUATEXEXE53 "$B"/texk/web2c/$LUATEXEXE
    ls -l "$B"/texk/web2c/$LUATEXEXE
fi




