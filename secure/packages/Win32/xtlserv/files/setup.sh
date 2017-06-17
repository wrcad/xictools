#! /bin/sh
# $Id: setup.sh,v 1.6 2014/02/06 18:01:56 stevew Exp $

pkgfiles=../../pkgfiles

inno="/inno-5.5.1"

top=../../root/usr/local
srctree=../../../..
version=`$srctree/version xtlserv`
utod=../../util/utod.exe

if [ ! -f $utod ]; then
    cwd=`pwd`
    cd ../../util
    make utod.exe
    cd $cwd
fi

startup=$top/xictools/license
$utod $startup/README

sed -e s/VERSION/$version/ < files/xtlserv.iss.in > xtlserv.iss
$utod xtlserv.iss

$inno/iscc xtlserv.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

mv Output/*.exe $pkgfiles
rmdir Output
rm xtlserv.iss
echo Done
