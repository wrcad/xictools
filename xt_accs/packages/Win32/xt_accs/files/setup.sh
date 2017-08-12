#! /bin/sh

inno="/inno-5.5.1"

program=xt_accs
version=`../../../version $program`
top=../../root/usr/local
base=../../../../xt_base
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

sed -e s/VERSION/$version/ < files/xt_accs.iss.in > xt_accs.iss
$utod xt_accs.iss

$inno/iscc xt_accs.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

exfiles="$pkgfiles/xt_accs-Win32*.exe"
if [ x"exfiles" != x ]; then
    for a in $exfiles; do
        rm -f $a
    done
fi

mv Output/*.exe $pkgfiles
rmdir Output
rm xt_accs.iss
echo Done
