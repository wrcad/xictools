#! /bin/sh

OSNAME=$1
VERSION=$2
SRCDIR=$3

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: ADMS translator'
echo "Name: adms_wr-$OSNAME"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: open-source'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'The admsXml program is used to support Verilog-A in WRspice.'
echo

echo '%files'
echo
echo "%attr(0755, root, root) /usr/local/xictools/bin/admsXml"

echo
echo "%dir /usr/local/xictools/adms"
echo "%attr(0644, root, root) /usr/local/xictools/adms/README"
echo "%attr(0644, root, root) /usr/local/xictools/adms/README-WR"

echo
echo "%dir /usr/local/xictools/adms/doc"
echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/admsXml.1"

echo
echo "%dir /usr/local/xictools/adms/examples"
echo "%attr(0755, root, root) /usr/local/xictools/adms/examples/admsCheck"

echo
cat files/scripts
