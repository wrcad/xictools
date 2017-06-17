#! /bin/sh
# $Id: setup.sh,v 1.24 2015/10/04 04:14:55 stevew Exp $

pkgfiles=../../pkgfiles

inno="/inno-5.5.1"

program=wrspice
top=../../root/usr/local
srctree=../../../..
version=`$srctree/version wrspice`
utod=../../util/utod.exe
tifs="$IFS"
IFS="."
set $version
relnote=wrs$1.$2
IFS="$tifs"

if [ ! -f $utod ]; then
    cwd=`pwd`
    cd ../../util
    make utod.exe
    cd $cwd
fi

# Move binaries to the common bin
if [ ! -d $top/xictools/bin ]; then
    mkdir $top/xictools/bin
fi
cp ../../files/program.bat $top/xictools/bin/$program.bat
chmod 755 $top/xictools/bin/$program.bat
$utod $top/xictools/bin/$program.bat
mv $top/xictools/$program/bin/* $top/xictools/bin
rmdir $top/xictools/$program/bin

license=$top/xictools/license
if [ ! -d $license ]; then
    mkdir $license
fi
cp $srctree/src/secure/README.MSW $license
$utod $license/README.MSW

examples=$top/xictools/$program/examples
$utod $examples/*

help=$top/xictools/$program/help
$utod $help/*.hlp

startup=$top/xictools/$program/startup
$utod $startup/*
$utod $startup/devices/README

scripts=$top/xictools/$program/scripts
$utod $scripts/*

docs=$top/xictools/$program/docs
cp ../../files/MSWINFO.TXT $docs
$utod $docs/$relnote
$utod $docs/README
$utod $docs/MSWINFO.TXT

devkit=$top/xictools/$program/devkit
$utod $devkit/README
$utod $devkit/README.adms
$utod $devkit/Makefile
$utod $devkit/admst/*
$utod $devkit/include/*
foo=`cat ../../util/adms_examples`
for a in $foo; do
    $utod $devkit/$a
done

sed -e s/VERSION/$version/ < files/$program.iss.in > $program.iss
$utod $program.iss

$inno/iscc $program.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

exfiles="$pkgfiles/wrspice-Win32*.exe"
if [ x"exfiles" != x ]; then
    for a in $exfiles; do
        rm -f $a
    done
fi

mv Output/*.exe $pkgfiles
rmdir Output
rm $program.iss
echo Done

