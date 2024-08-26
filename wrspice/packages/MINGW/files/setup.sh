#! /bin/sh

inno=`../../../xt_base/info.sh innoloc`
toolroot=`../../../xt_base/info.sh toolroot`
appname=${toolroot}_wrspice
appdir=wrspice.current
version=`../../release.sh`
osname=`../../../xt_base/info.sh osname`
arch=`uname -m`
top=../root/usr/local
base=../../../xt_base
baseutil=$base/packages/util
basefiles=$base/packages/files
pkgfiles=$base/packages/pkgfiles

tifs="$IFS"
IFS="."
set $version
relnote=wrs$1.$2
IFS="$tifs"

utod=$baseutil/utod.exe
if [ ! -f $utod ]; then
    cwd=`pwd`
    cd $baseutil
    make utod.exe
    cd $cwd
fi

chmod 755 $top/$toolroot/$appdir/bin/wrspice.bat
$utod $top/$toolroot/$appdir/bin/wrspice.bat
cp -f files/postinstall.bat $top/$toolroot/$appdir/bin
chmod 755 $top/$toolroot/$appdir/bin/postinstall.bat
$utod $top/$toolroot/$appdir/bin/postinstall.bat
cp -f files/preinstall.bat $top/$toolroot/$appdir/bin
chmod 755 $top/$toolroot/$appdir/bin/preinstall.bat
$utod $top/$toolroot/$appdir/bin/preinstall.bat

examples=$top/$toolroot/$appdir/examples
$utod $examples/*
$utod $examples/JJexamples/*
$utod $examples/JJexamples_old/*

help=$top/$toolroot/$appdir/help
$utod $help/*.hlp

startup=$top/$toolroot/$appdir/startup
$utod $startup/*
$utod $startup/devices/README

scripts=$top/$toolroot/$appdir/scripts
$utod $scripts/*

docs=$top/$toolroot/$appdir/docs
cp $basefiles/MSWINFO.TXT $docs
$utod $docs/$relnote
$utod $docs/README
$utod $docs/MSWINFO.TXT

devkit=$top/$toolroot/$appdir/devkit
if [ -d $devkit ]; then
    $utod $devkit/README
    $utod $devkit/README.adms
    $utod $devkit/Makefile
    $utod $devkit/admst/*
    $utod $devkit/include/*
    foo=`cat ../util/adms_examples`
    for a in $foo; do
        $utod $devkit/$a
    done
    sed -e s/APPNAME/$appname/ -e s/OSNAME/$osname/ \
      -e s/VERSION/$version/ -e s/ARCH/$arch/ \
      -e s/TOOLROOT/$toolroot/g < files/xictools_wrspice.iss.in \
      > $appname.iss
else
    sed -e s/APPNAME/$appname/ -e s/OSNAME/$osname/ \
      -e s/VERSION/$version/ -e s/ARCH/$arch/ \
      -e s/TOOLROOT/$toolroot/g < files/xictools_wrspice_nodk.iss.in \
      > $appname.iss
fi
$utod $appname.iss

$inno/iscc $appname.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

exfiles="$pkgfiles/$appname-$osname*.exe"
if [ -n "$exfiles" ]; then
    for a in $exfiles; do
        rm -f $a
    done
fi

pkg=Output/*.exe
if [ -f $pkg ]; then
    fn=$(basename $pkg)
    mv -f $pkg $pkgfiles
    echo ==================================
    echo Package file $fn
    echo moved to xt_base/packages/pkgfiles
    echo ==================================
fi
rmdir Output
rm $appname.iss
echo Done
