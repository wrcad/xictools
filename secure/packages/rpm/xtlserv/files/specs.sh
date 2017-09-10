#! /bin/sh
# $Id: specs.sh,v 1.6 2014/09/29 16:49:14 stevew Exp $

OSNAME=$1
VERSION=$2
SRCDIR=$3

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: License daemon for Xic and WRspice'
echo "Name: xtlserv-$OSNAME"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: commercial'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'This is the license server for Xic and WRspice software.'
echo
echo '%files'
echo
echo '%attr(0755, root, root) /usr/local/xictools/bin/xtlserv'
echo '%attr(0755, root, root) /usr/local/xictools/bin/xtjobs'
echo "%dir /usr/local/xictools/license"
echo '%attr(0644, root, root) /usr/local/xictools/license/README'
echo
cat files/scripts
