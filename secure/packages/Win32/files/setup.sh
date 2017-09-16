#! /bin/sh

inno="/inno-5.5.1"

appname=xictools_xtlserv
version=`../../version`
top=../root/usr/local
base=../../../xt_base
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

sed -e s/VERSION/$version/ < files/$appname.iss.in > $appname.iss
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
