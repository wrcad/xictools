#! /bin/sh
# $Id: specs.sh,v 1.31 2015/09/16 00:13:23 stevew Exp $

with_devkit=yes

files=../../util/wrspice_files

OSNAME=$1
VERSION=$2
SRCDIR=$3

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: Circuit simulation software'
echo "Name: wrspice-$OSNAME-gen4"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: commercial'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'The WRspice program is a general purpose simulator of electronic'
echo 'circuits.'
echo

echo '%files'
echo
echo "%dir /usr/local/xictools/wrspice"
echo "%dir /usr/local/xictools/wrspice/bin"
bin=`$files bin`
for a in $bin; do
    echo "%attr(0755, root, root) /usr/local/xictools/wrspice/bin/$a"
done

echo
echo "%dir /usr/local/xictools/wrspice/docs"
docs=`$files docs`
for a in $docs; do
    echo "%attr(0644, root, root) /usr/local/xictools/wrspice/docs/$a"
done
echo "%attr(0644, root, root) /usr/local/xictools/wrspice/docs/wrs$VERSION"

echo
echo "%dir /usr/local/xictools/wrspice/examples"
examples=`$files examples`
for a in $examples; do
    echo "%attr(0644, root, root) /usr/local/xictools/wrspice/examples/$a"
done

echo
echo "%dir /usr/local/xictools/wrspice/help"
help=`$files help`
for a in $help; do
    echo "%attr(0644, root, root) /usr/local/xictools/wrspice/help/$a"
done
echo "%dir /usr/local/xictools/wrspice/help/screenshots"
help_screenshots=`$files help_screenshots`
for a in $help_screenshots; do
    echo "%attr(0644, root, root) /usr/local/xictools/wrspice/help/screenshots/$a"
done

echo
echo "%dir /usr/local/xictools/wrspice/icons"
icons=`$files icons`
for a in $icons; do
    echo "%attr(0644, root, root) /usr/local/xictools/wrspice/icons/$a"
done

echo
echo "%dir /usr/local/xictools/wrspice/scripts"
scripts=`$files scripts`
for a in $scripts; do
    echo "%attr(0644, root, root) /usr/local/xictools/wrspice/scripts/$a"
done

echo
echo "%dir /usr/local/xictools/wrspice/startup"
startup=`$files startup`
for a in $startup; do
    echo "%config /usr/local/xictools/wrspice/startup/$a"
done
klulib=`$files klu Linux`
for a in $klulib; do
    echo "%attr(0755, root, root) /usr/local/xictools/wrspice/startup/$a"
done
echo "%dir /usr/local/xictools/wrspice/startup/devices"
echo "%attr(0644, root, root) /usr/local/xictools/wrspice/startup/devices/README"
vlmods=`$files vlmods`
for a in $vlmods; do
    echo "%attr(0644, root, root) /usr/local/xictools/wrspice/startup/devices/$a.so"
done

echo
if [ $with_devkit=yes ]; then
  echo "%dir /usr/local/xictools/wrspice/devkit"
  echo "%attr(0644, root, root) /usr/local/xictools/wrspice/devkit/Makefile"
  echo "%attr(0644, root, root) /usr/local/xictools/wrspice/devkit/README"
  echo "%attr(0644, root, root) /usr/local/xictools/wrspice/devkit/README.adms"
  echo "%dir /usr/local/xictools/wrspice/devkit/admst"
  admst=`$files admst`
  for a in $admst; do
      echo "%attr(0644, root, root) /usr/local/xictools/wrspice/devkit/admst/$a"
  done
  echo "%dir /usr/local/xictools/wrspice/devkit/include"
  devincl=`$files devincl`
  for a in $devincl; do
      echo "%attr(0644, root, root) /usr/local/xictools/wrspice/devkit/include/$a"
  done
  echo "%dir /usr/local/xictools/wrspice/devkit/examples"
  dkexdirs=`$files dkexdirs`
  for a in $dkexdirs; do
      echo "%dir /usr/local/xictools/wrspice/devkit/examples/$a"
      echo "%dir /usr/local/xictools/wrspice/devkit/examples/$a/tests"
      echo "%dir /usr/local/xictools/wrspice/devkit/examples/$a/module_dist"
  done
  for a in $dkexdirs; do
      mfile=`$files modname $a`.`$files soext Linux`
      echo "%attr(0755, root, root) /usr/local/xictools/wrspice/devkit/examples/$a/module_dist/$mfile"
  done
  efiles=`cat ../../util/adms_examples`
  for a in $efiles; do
      echo "%attr(0644, root, root) /usr/local/xictools/wrspice/devkit/$a"
  done
  echo
fi
cat files/scripts

