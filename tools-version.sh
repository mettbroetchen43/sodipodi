#!/bin/sh
# Report the version of distro and tools building sodipodi
#
# You can get the latest distro command from 
# distro web page: http://distro.pipfield.ca/ 

# Please add a tool you want to check
TOOLS="m4 automake autoheader aclocal autoconf intltoolize gettextize libtoolize "
ENVPATTERN='PATH\|FLAGS\|LANG'

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

echo '============================================================================='
echo 'When you report a trouble about building CVS version of sodipodi,            '
echo 'Please report following information about distro and tools version, too.     '
echo 
(echo '--1. distribution------------------------------------------------------------'
$srcdir/distro -a
echo )

(echo '--2. tools-------------------------------------------------------------------'
for x in $TOOLS; do 
    echo "which $x:"
    $x --version | grep $x 
done 
echo )

(echo '--3. environment variables---------------------------------------------------'
env | grep -e $ENVPATTERN
echo )
echo '============================================================================='
