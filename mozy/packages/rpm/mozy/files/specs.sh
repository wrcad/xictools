#! /bin/sh
# $Id: specs.sh,v 1.9 2014/07/20 22:25:16 stevew Exp $

files=../../util/mozy_files

OSNAME=$1
VERSION=$2
SRCDIR=$3

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: MOZY html help system and accessories'
echo "Name: mozy-$OSNAME-gen4"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: open-source'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'Help system for XicTools integrated circuit design software.'
echo

echo '%files'
echo
bin=`$files bin`
for a in $bin; do
    echo "%attr(0755, root, root) /usr/local/xictools/bin/$a"
done

echo
echo "%dir /usr/local/xictools/mozy"
echo "%dir /usr/local/xictools/mozy/help"
help=`$files help`
for a in $help; do
    echo "%attr(0644, root, root) /usr/local/xictools/mozy/help/$a"
done
echo "%dir /usr/local/xictools/mozy/help/screenshots"
ss=`$files help_ss`
for a in $ss; do
    echo "%attr(0644, root, root) /usr/local/xictools/mozy/help/screenshots/$a"
done

echo
echo "%dir /usr/local/xictools/mozy/startup"
startup=`$files startup`
for a in $startup; do
    echo "%config /usr/local/xictools/mozy/startup/$a"
done

echo
cat files/scripts
