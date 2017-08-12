#! /bin/sh
# $Id: specs.sh,v 1.9 2014/07/20 22:25:16 stevew Exp $

OSNAME=$1
VERSION=$2
SRCDIR=$3

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: XicTools accessory programs'
echo "Name: xt_accs-$OSNAME-gen4"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: commercial'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'Accessory programs for XicTools integrated circuit design software.'
echo

echo '%files'
echo
echo "%attr(0755, root, root) /usr/local/xictools/bin/fcpp"
echo "%attr(0755, root, root) /usr/local/xictools/bin/lstpack"
echo "%attr(0755, root, root) /usr/local/xictools/bin/lstunpack"

echo
cat files/scripts
