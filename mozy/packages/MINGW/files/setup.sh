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

cp -f files/postinstall.bat $top/$toolroot/mozy/bin
cp -f files/preinstall.bat $top/$toolroot/mozy/bin
$utod $top/$toolroot/mozy/bin/postinstall.bat
$utod $top/$toolroot/mozy/bin/preinstall.bat
$utod $top/$toolroot/mozy/bin/mozy.bat
$utod $top/$toolroot/mozy/bin/xeditor.bat
$utod $top/$toolroot/mozy/bin/mozy.sh
$utod $top/$toolroot/mozy/bin/xeditor.sh
chmod 755 $top/$toolroot/mozy/bin/postinstall.bat
chmod 755 $top/$toolroot/mozy/bin/preinstall.bat
chmod 755 $top/$toolroot/mozy/bin/mozy.bat
chmod 755 $top/$toolroot/mozy/bin/xeditor.bat
chmod 644 $top/$toolroot/mozy/bin/mozy.sh
chmod 644 $top/$toolroot/mozy/bin/xeditor.sh

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
