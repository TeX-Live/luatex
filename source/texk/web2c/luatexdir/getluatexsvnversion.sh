#! /bin/sh

# This script should be run within the source directory.

$DEBUG

FILE="texk/web2c/luatexdir/luatex_svnversion.h"

LANG=C
if [ ! -r $FILE ]
then
  echo '#define luatex_svn_revision -1' > $FILE
fi
if ( [ -d ./.svn ] && svnversion > /dev/null )
then
  # svn up > /dev/null
  DEFREV=`cat $FILE`
  SVNREV=`svnversion -c . | sed -ne 's/^[0-9]*:*\([0-9]*\).*/#define luatex_svn_revision \1/p'`
  test "$DEFREV" != "$SVNREV" && echo "$SVNREV" > $FILE
fi
