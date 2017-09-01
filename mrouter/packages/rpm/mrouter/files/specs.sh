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

echo 'Summary: Mrouter maze router'
echo "Name: mrouter-$OSNAME-gen4"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: open-source'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'Mrouter maze router for XicTools integrated circuit design software.'
echo

echo '%files'
echo
echo "%attr(0755, root, root) /usr/local/xictools/bin/mrouter"
echo
echo "%dir /usr/local/xictools/mrouter"
echo "%dir /usr/local/xictools/mrouter/lib"
echo "%attr(0644, root, root) /usr/local/xictools/mrouter/lib/libmrouter.so"
echo
echo "%dir /usr/local/xictools/mrouter/doc"
docs=`../../../util/mrouter_files doc`
for a in $docs; do 
    echo "%attr(0644, root, root) /usr/local/xictools/mrouter/doc/$a"
done
echo
echo "%dir /usr/local/xictools/mrouter/examples"
examp=`../../../util/mrouter_files examples`
for a in $examp; do 
    echo "%attr(0644, root, root) /usr/local/xictools/mrouter/examples/$a"
done
echo "%dir /usr/local/xictools/mrouter/examples/osu35"
examp=`../../../util/mrouter_files examples_osu35`
for a in $examp; do 
    echo "%attr(0644, root, root) /usr/local/xictools/mrouter/examples/osu35/$a"
done
echo "%dir /usr/local/xictools/mrouter/examples/xic"
examp=`../../../util/mrouter_files examples_xic`
for a in $examp; do 
    echo "%attr(0644, root, root) /usr/local/xictools/mrouter/examples/xic/$a"
done

echo
cat files/scripts
