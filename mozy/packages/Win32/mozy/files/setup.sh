#! /bin/sh

inno="/inno-5.5.1"

appname=mozy
version=`../../../version $appname`
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

program=mozy
cp ../../files/program.bat $top/xictools/bin/$program.bat
chmod 755 $top/xictools/bin/$program.bat
$utod $top/xictools/bin/$program.bat
program=xeditor
cp ../../files/program.bat $top/xictools/bin/$program.bat
chmod 755 $top/xictools/bin/$program.bat
$utod $top/xictools/bin/$program.bat
program=httpget
cp ../../files/program.bat $top/xictools/bin/$program.bat
chmod 755 $top/xictools/bin/$program.bat
$utod $top/xictools/bin/$program.bat

help=$top/xictools/mozy/help
$utod $help/*.hlp

startup=$top/xictools/mozy/startup
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

mv Output/*.exe $pkgfiles
rmdir Output
rm $appname.iss
echo Done