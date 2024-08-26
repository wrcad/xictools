#! /bin/sh

inno=`../../../xt_base/info.sh innoloc`
toolroot=`../../../xt_base/info.sh toolroot`
appname=${toolroot}_mozy
appdir=mozy
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

program=mozy
cp $base/util/program.bat $top/$toolroot/bin/$program.bat
chmod 755 $top/$toolroot/bin/$program.bat
$utod $top/$toolroot/bin/$program.bat
program=xeditor
cp $base/util/program.bat $top/$toolroot/bin/$program.bat
chmod 755 $top/$toolroot/bin/$program.bat
$utod $top/$toolroot/bin/$program.bat
program=httpget
cp $base/util/program.bat $top/$toolroot/bin/$program.bat
chmod 755 $top/$toolroot/bin/$program.bat
$utod $top/$toolroot/bin/$program.bat

help=$top/$toolroot/$appdir/help
$utod $help/*.hlp

startup=$top/$toolroot/$appdir/startup
$utod $startup/README

sed -e s/APPNAME/$appname/ -e s/OSNAME/$osname/ \
  -e s/VERSION/$version/ -e s/ARCH/$arch/ \
  -e s/TOOLROOT/$toolroot/g < files/xictools_mozy.iss.in > $appname.iss
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
