#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="Sodipodi"

(test -f $srcdir/configure.in \
  && test -d $srcdir/src \
  && test -f $srcdir/src/sodipodi.h) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level sodipodi directory"
    exit 1
}

. $srcdir/macros/autogen.sh

