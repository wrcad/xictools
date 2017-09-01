#! /bin/sh
# $Id: specs.sh,v 1.27 2015/10/04 22:05:42 stevew Exp $

files=../../util/xic_files

OSNAME=$1
VERSION=$2
SRCDIR=$3

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: Integrated circuit design software'
echo "Name: xic-$OSNAME-gen4"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: commercial'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'The Xic program is a graphical editor for integrated circuit layout,'
echo 'schematic capture, and other design tasks.'
echo

echo '%files'
echo
echo "%dir /usr/local/xictools/xic"
echo "%dir /usr/local/xictools/xic/bin"
bin=`$files bin`
for a in $bin; do
    echo "%attr(0755, root, root) /usr/local/xictools/xic/bin/$a"
done

echo
echo "%dir /usr/local/xictools/xic/docs"
docs=`$files docs`
for a in $docs; do
    echo "%attr(0644, root, root) /usr/local/xictools/xic/docs/$a"
done
echo "%attr(0644, root, root) /usr/local/xictools/xic/docs/xic$VERSION"

echo
echo "%dir /usr/local/xictools/xic/examples"
examples=`$files examples`
for a in $examples; do
    echo "%attr(0644, root, root) /usr/local/xictools/xic/examples/$a"
done
echo "%dir /usr/local/xictools/xic/examples/PCells"
examples_pcells=`$files examples_pcells`
for a in $examples_pcells; do
    echo "%attr(0644, root, root) /usr/local/xictools/xic/examples/PCells/$a"
done
echo "%dir /usr/local/xictools/xic/examples/memchip_example"
examples_memchip_example=`$files examples_memchip_example`
for a in $examples_memchip_example; do
    echo "%attr(0644, root, root) /usr/local/xictools/xic/examples/memchip_example/$a"
done

echo
echo "%dir /usr/local/xictools/xic/help"
help=`$files help`
for a in $help; do
    echo "%attr(0644, root, root) /usr/local/xictools/xic/help/$a"
done
echo "%dir /usr/local/xictools/xic/help/screenshots"
help_screenshots=`$files help_screenshots`
for a in $help_screenshots; do
    echo "%attr(0644, root, root) /usr/local/xictools/xic/help/screenshots/$a"
done

echo
echo "%dir /usr/local/xictools/xic/icons"
icons=`$files icons`
for a in $icons; do
    echo "%attr(0644, root, root) /usr/local/xictools/xic/icons/$a"
done

echo
echo "%dir /usr/local/xictools/xic/plugins"
plugins=`$files plugins $OSNAME`
for a in $plugins; do
    echo "%attr(0755, root, root) /usr/local/xictools/xic/plugins/$a"
done
oaplugin=`$files oaplugin $OSNAME`
for a in $oaplugin; do
    echo "%attr(0755, root, root) /usr/local/xictools/xic/plugins/$a"
done

echo
echo "%dir /usr/local/xictools/xic/scripts"
scripts=`$files scripts`
for a in $scripts; do
    echo "%attr(0644, root, root) /usr/local/xictools/xic/scripts/$a"
done
echo

echo
echo "%dir /usr/local/xictools/xic/scrkit"
scrkit=`$files scrkit`
for a in $scrkit; do
    echo "%attr(0644, root, root) /usr/local/xictools/xic/scrkit/$a"
done
echo

echo "%dir /usr/local/xictools/xic/startup"
startup=`$files startup`
for a in $startup; do
    echo "%config /usr/local/xictools/xic/startup/$a"
done
echo

echo
cat files/scripts
