#! /bin/sh

files=../util/xic_files
with_oa=`../../../xt_base/info.sh with_oa`

top=xic.current

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
echo "Name: xictools-xic-$OSNAME"
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
echo "%dir /usr/local/xictools/$top"
echo "%dir /usr/local/xictools/$top/bin"
bin=`$files bin`
for a in $bin; do
    echo "%attr(0755, root, root) /usr/local/xictools/$top/bin/$a"
done

echo "%dir /usr/local/xictools/bin"
echo "%attr(0644, root, root) /usr/local/xictools/$top/bin/xic.sh"

echo
echo "%dir /usr/local/xictools/$top/docs"
docs=`$files docs`
for a in $docs; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/docs/$a"
done
echo "%attr(0644, root, root) /usr/local/xictools/$top/docs/xic$VERSION"

echo
echo "%dir /usr/local/xictools/$top/examples"
examples=`$files examples`
for a in $examples; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/examples/$a"
done
echo "%dir /usr/local/xictools/$top/examples/PCells"
examples_pcells=`$files examples_pcells`
for a in $examples_pcells; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/examples/PCells/$a"
done
echo "%dir /usr/local/xictools/$top/examples/memchip_example"
examples_memchip_example=`$files examples_memchip_example`
for a in $examples_memchip_example; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/examples/memchip_example/$a"
done

echo
echo "%dir /usr/local/xictools/$top/help"
help=`$files help`
for a in $help; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/help/$a"
done
echo "%dir /usr/local/xictools/$top/help/screenshots"
help_screenshots=`$files help_screenshots`
for a in $help_screenshots; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/help/screenshots/$a"
done

echo
echo "%dir /usr/local/xictools/$top/icons"
icons=`$files icons`
for a in $icons; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/icons/$a"
done

echo
echo "%dir /usr/local/xictools/$top/plugins"
plugins=`$files plugins $OSNAME`
for a in $plugins; do
    echo "%attr(0755, root, root) /usr/local/xictools/$top/plugins/$a"
done
if [ x$with_oa = xyes ]; then
    oaplugin=`$files oaplugin $OSNAME`
fi
if [ -n "$oaplugin" ]; then
    echo "%attr(0755, root, root) /usr/local/xictools/$top/plugins/$oaplugin"
fi

echo
echo "%dir /usr/local/xictools/$top/scripts"
scripts=`$files scripts`
for a in $scripts; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/scripts/$a"
done
echo

echo
echo "%dir /usr/local/xictools/$top/scrkit"
scrkit=`$files scrkit`
for a in $scrkit; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/scrkit/$a"
done
echo

echo "%dir /usr/local/xictools/$top/startup"
startup=`$files startup`
for a in $startup; do
    echo "%config /usr/local/xictools/$top/startup/$a"
done
echo

echo
cat files/scripts
