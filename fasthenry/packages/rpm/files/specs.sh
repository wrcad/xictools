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
echo "Name: xictools_fasthenry-$OSNAME"
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
echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/examples/README"
echo "%attr(0755, root, root) /usr/local/xictools/fasthenry/examples/cf"
echo "%attr(0755, root, root) /usr/local/xictools/fasthenry/examples/run"
echo
echo "%dir /usr/local/xictools/fasthenry/examples/input"
input=`$files examples_input`
for a in $input; do
    echo "%attr(0755, root, root) /usr/local/xictools/fasthenry/examples/input/$a"
done
echo
echo "%dir /usr/local/xictools/fasthenry/examples/results"
echo "%dir /usr/local/xictools/fasthenry/examples/results/linux_dss"
results=`$files examples_results`
for a in $results; do
    echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/examples/results/linux_dss/$a"
done
echo "%dir /usr/local/xictools/fasthenry/examples/results/linux_klu"
for a in $results; do
    echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/examples/results/linux_klu/$a"
done
echo "%dir /usr/local/xictools/fasthenry/examples/results/linux_sparse"
for a in $results; do
    echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/examples/results/linux_sparse/$a"
done
echo "%dir /usr/local/xictools/fasthenry/examples/torture"
echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/examples/torture/README"
echo "%attr(0644, root, root) /usr/local/xictools/fasthenry/examples/torture/bfh.inp"

echo
cat files/scripts
