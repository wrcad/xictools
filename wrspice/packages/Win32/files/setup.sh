#! /bin/sh

inno="/inno-5.5.1"

appname=xictools_wrspice
version=`../../version`
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

# Move binaries to the common bin
if [ ! -d $top/xictools/bin ]; then
    mkdir $top/xictools/bin
fi

program=wrspice
cp $basefiles/program.bat $top/xictools/bin/$program.bat
chmod 755 $top/xictools/bin/$program.bat
$utod $top/xictools/bin/$program.bat
mv $top/xictools/$appname/bin/* $top/xictools/bin
rmdir $top/xictools/$appname/bin

examples=$top/xictools/$appname/examples
$utod $examples/*

help=$top/xictools/$appname/help
$utod $help/*.hlp

startup=$top/xictools/$appname/startup
$utod $startup/*
$utod $startup/devices/README

scripts=$top/xictools/$appname/scripts
$utod $scripts/*

docs=$top/xictools/$appname/docs
cp $basefiles/MSWINFO.TXT $docs
$utod $docs/$relnote
$utod $docs/README
$utod $docs/MSWINFO.TXT

devkit=$top/xictools/$appname/devkit
if [ -d $devkit ]; then
    $utod $devkit/README
    $utod $devkit/README.adms
    $utod $devkit/Makefile
    $utod $devkit/admst/*
    $utod $devkit/include/*
    foo=`cat ../../util/adms_examples`
    for a in $foo; do
        $utod $devkit/$a
    done
    sed -e s/VERSION/$version/ < files/$appname.iss.in > $appname.iss
else
    sed -e s/VERSION/$version/ < files/wrspice_nodk.iss.in > $appname.iss
fi
$utod $appname.iss

$inno/iscc $appname.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

exfiles="$pkgfiles/$appname-Win32*.exe"
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
