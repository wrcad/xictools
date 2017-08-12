#! /bin/sh

inno="/inno-5.5.1"

program=xtlserv
version=`../../../version $program`
top=../../root/usr/local
base=../../../../xt_base
baseutil=$base/packages/util
basefiles=$base/packages/files
pkgfiles=$base/packages/pkgfiles

utod=../../util/utod.exe
if [ ! -f $utod ]; then
    cwd=`pwd`
    cd $baseutil
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
