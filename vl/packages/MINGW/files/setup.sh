#! /bin/sh

inno="/c/inno-5.5.9"

appname=xictools_vl
appdir=vl
version=`../../release.sh`
osname=`../../../xt_base/info.sh osname`
arch=`uname -m`
top=../root/usr/local
base=../../../xt_base
baseutil=$base/packages/util
basefiles=$base/packages/files
pkgfiles=$base/packages/pkgfiles

utod=$baseutil/utod.exe
if [ ! -f $utod ]; then
    cwd=`pwd`
    cd $baseutil
    make utod.exe
    cd $cwd
fi

lib=$top/xictools/$appdir
$utod $lib/README
$utod $lib/ChangeLog
$utod $lib/verilog-manual.html

$utod $lib/examples/ivl/*
$utod $lib/examples/ivl/contrib/*
$utod $lib/examples/ivl/extra/*
$utod $lib/examples/ivl/ivltests/*
$utod $lib/examples/book/*
$utod $lib/examples/book/ch13/eg/*
$utod $lib/examples/book/ch13/mag_comp/*
$utod $lib/examples/book/ch13/vending/*
$utod $lib/examples/book/ch14/eg/*
$utod $lib/examples/book/ch14/mag_comp/*
$utod $lib/examples/book/ch14/vending/*
$utod $lib/examples/vbs/*

sed -e s/OSNAME/$osname/ -e s/VERSION/$version/ -e s/ARCH/$arch/ \
  < files/$appname.iss.in > $appname.iss
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
