#! /bin/sh
# $Id: setup.sh,v 1.10 2014/07/19 23:14:26 stevew Exp $

pkgfiles=../../pkgfiles

inno="/inno-5.5.1"

top=../../root/usr/local
srctree=../../../..
version=`$srctree/version xt_accs`
utod=../../util/utod.exe

if [ ! -f $utod ]; then
    cwd=`pwd`
    cd ../../util
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
