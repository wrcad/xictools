#! /bin/sh

files=../util/fh_files

OSNAME=$1
VERSION=$2
SRCDIR=$3

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: FastHenry inductance extractor'
echo "Name: xictools-fasthenry-$OSNAME"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: open-source'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'The FastHenry program from MIT modified by Whiteley Research Inc, for'
echo 'use in the XicTools tool set.'
echo

echo '%files'
echo
execs=`$files bin`
for a in $execs; do
    echo "%attr(0755, root, root) /usr/local/xictools/bin/$a"
done

echo
echo "%dir /usr/local/xictools/fasthenry"
echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/README"
echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/README.mit"

echo
echo "%dir /usr/local/xictools/fasthenry/doc"
docs=`$files doc`
for a in $docs; do
    echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/doc/$a"
done

echo
echo "%dir /usr/local/xictools/fasthenry/examples"
exfiles=`$files examples`
for a in $exfiles; do
    echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/examples/$a"
done

echo
echo "%dir /usr/local/xictools/fasthenry/examples/work"
echo "%attr(0755, root, root) /usr/local/xictools/fasthenry/examples/work/run"

echo
echo "%dir /usr/local/xictools/fasthenry/examples/work/results"
results=`$files results`
for a in $results; do
    echo "%attr(0755, root, root) /usr/local/xictools/fasthenry/examples/work/results/$a"
done

echo
cat files/scripts
