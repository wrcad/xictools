#! /bin/sh

files=../../util/fc_files

OSNAME=$1
VERSION=$2
SRCDIR=$3

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: FastCap capacitance extractor'
echo "Name: fastcap_wr-$OSNAME"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: open-source'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'The FastCap program from MIT modified by Whiteley Research Inc, for'
echo 'use in the XicTools tool set.'
echo

echo '%files'
echo
execs="`$files accs` `$files bin`"
for a in $execs; do
    echo "%attr(0755, root, root) /usr/local/xictools/bin/$a"
done

echo
echo "%dir /usr/local/xictools/fastcap"
echo "%attr(0644, root, root) /usr/local/xictools/fastcap/README"
echo "%attr(0644, root, root) /usr/local/xictools/fastcap/README.mit"

echo
echo "%dir /usr/local/xictools/fastcap/doc"
docs=`$files doc`
for a in $docs; do
    echo "%attr(0644, root, root) /usr/local/xictools/fastcap/doc/$a"
done

echo
echo "%dir /usr/local/xictools/fastcap/examples"
exfiles=`$files examples`
for a in $exfiles; do
    echo "%attr(0644, root, root) /usr/local/xictools/fastcap/examples/$a"
done
scripts=`$files examples_scripts`
for a in $scripts; do
    echo "%attr(0755, root, root) /usr/local/xictools/fastcap/examples/$a"
done

echo
echo "%dir /usr/local/xictools/fastcap/examples/work"
echo "%attr(0644, root, root) /usr/local/xictools/fastcap/examples/work/Makefile"
echo "%attr(0755, root, root) /usr/local/xictools/fastcap/examples/work/run"

echo
echo "%dir /usr/local/xictools/fastcap/examples/work/results"
results=`$files results`
for a in $results; do
    echo "%attr(0755, root, root) /usr/local/xictools/fastcap/examples/work/results/$a"
done

echo
cat files/scripts
