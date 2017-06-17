#! /bin/sh
# $Id: contents.sh,v 1.23 2015/09/16 00:13:23 stevew Exp $

files=../../util/wrspice_files

with_devkit=yes

version=$1
revision=$2
docfile=wrs$version
tifs=$IFS
IFS="."
set -- $version
IFS=$tifs
gen=$1

echo "@name wrspice_$gen-$version.$revision"
echo '@cwd /usr/local'
echo '@srcdir @SRCDIR@/usr/local'
echo
echo '@owner root'
echo '@group bin'

echo
echo '@mode 0755'
bin=`$files bin`
for a in $bin; do
    echo "xictools/wrspice/bin/$a"
done

echo
echo '@mode 0644'
docs=`$files docs`
for a in $docs; do
    echo "xictools/wrspice/docs/$a"
done
echo "xictools/wrspice/docs/$docfile"

echo
examples=`$files examples`
for a in $examples; do
    echo "xictools/wrspice/examples/$a"
done

echo
help=`$files help`
for a in $help; do
    echo "xictools/wrspice/help/$a"
done
help_screenshots=`$files help_screenshots`
for a in $help_screenshots; do
    echo "xictools/wrspice/help/screenshots/$a"
done

echo
icons=`$files icons`
for a in $icons; do
    echo "xictools/wrspice/icons/$a"
done

echo
scripts=`$files scripts`
for a in $scripts; do
    echo "xictools/wrspice/scripts/$a"
done

echo
startup=`$files startup`
for a in $startup; do
    echo "xictools/wrspice/startup/$a"
done
echo "xictools/wrspice/startup/devices/README"
vlmods=`$files vlmods`
for a in $vlmods; do
    echo "xictools/wrspice/startup/devices/$a.so"
done
klulib=`$files klu FreeBSD`
for a in $klulib; do
    echo "xictools/wrspice/startup/$a"
done
echo

efiles=""
if [ $with_devkit=yes ]; then
  echo "xictools/wrspice/devkit/Makefile"
  echo "xictools/wrspice/devkit/README"
  admst=`$files admst`
  for a in $admst; do
      echo "xictools/wrspice/devkit/admst/$a"
  done
  devincl=`$files devincl`
  for a in $devincl; do
      echo "xictools/wrspice/devkit/include/$a"
  done
  dkexdirs=`$files dkexdirs`
  for a in $dkexdirs; do
      mfile=`$files modname $a`.`$files soext FreeBSD`
      echo "xictools/wrspice/devkit/examples/$a/module_dist/$mfile"
  done
  efiles=`cat ../../util/adms_examples`
  for a in $efiles; do
      echo "xictools/wrspice/devkit/$a"
  done
  echo
fi

echo "@exec chown root:wheel %D/xictools"
echo "@exec chown root:wheel %D/xictools/wrspice"
echo "@exec chown root:wheel %D/xictools/wrspice/bin"
echo "@exec chown root:wheel %D/xictools/wrspice/docs"
echo "@exec chown root:wheel %D/xictools/wrspice/examples"
echo "@exec chown root:wheel %D/xictools/wrspice/help"
echo "@exec chown root:wheel %D/xictools/wrspice/help/screenshots"
echo "@exec chown root:wheel %D/xictools/wrspice/icons"
echo "@exec chown root:wheel %D/xictools/wrspice/scripts"
echo "@exec chown root:wheel %D/xictools/wrspice/startup"
echo "@exec chown root:wheel %D/xictools/wrspice/startup/devices"

if [ $with_devkit=yes ]; then
  echo "@exec chown root:wheel %D/xictools/wrspice/devkit"
  echo "@exec chown root:wheel %D/xictools/wrspice/devkit/admst"
  echo "@exec chown root:wheel %D/xictools/wrspice/devkit/include"
  echo "@exec chown root:wheel %D/xictools/wrspice/devkit/examples"
  for a in $dkexdirs; do
    echo "@exec chown root:wheel %D/xictools/wrspice/devkit/examples/$a"
    if [ -d %D/xictools/wrspice/devkit/examples/$a/tests ]; then
      echo "@exec chown root:wheel %D/xictools/wrspice/devkit/examples/$a/tests"
    fi
    if [ -d %D/xictools/wrspice/devkit/examples/$a/module_dist ]; then
      echo "@exec chown root:wheel %D/xictools/wrspice/devkit/examples/$a/module_dist"
    fi
  done
fi

echo
echo "@dirrm xictools/wrspice/bin"
echo "@dirrm xictools/wrspice/startup/devices"
echo "@dirrm xictools/wrspice/scripts"
echo "@dirrm xictools/wrspice/help/screenshots"
echo "@dirrm xictools/wrspice/help"
echo "@dirrm xictools/wrspice/icons"
echo "@dirrm xictools/wrspice/examples"
if [ $with_devkit=yes ]; then
  echo "@dirrm xictools/wrspice/devkit/admst"
  echo "@dirrm xictools/wrspice/devkit/include"
  for a in $dkexdirs; do
    echo "@dirrm xictools/wrspice/devkit/examples/$a/module_dist"
    echo "@dirrm xictools/wrspice/devkit/examples/$a/tests"
    echo "@dirrm xictools/wrspice/devkit/examples/$a"
  done
  echo "@dirrm xictools/wrspice/devkit/examples"
  echo "@dirrm xictools/wrspice/devkit"
fi

