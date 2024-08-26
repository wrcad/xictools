#! /bin/sh

inno=`../../../xt_base/info.sh innoloc`
toolroot=`../../../xt_base/info.sh toolroot`
appname=${toolroot}_xic
appdir=xic.current
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
relnote=xic$1.$2
IFS="$tifs"

utod=$baseutil/utod.exe
if [ ! -f $utod ]; then
    cwd=`pwd`
    cd $baseutil
    make utod.exe
    cd $cwd
fi

chmod 755 $top/$toolroot/$appdir/bin/xic.bat
$utod $top/$toolroot/$appdir/bin/xic.bat
cp -f files/postinstall.bat $top/$toolroot/$appdir/bin
chmod 755 $top/$toolroot/$appdir/bin/postinstall.bat
$utod $top/$toolroot/$appdir/bin/postinstall.bat
cp -f files/preinstall.bat $top/$toolroot/$appdir/bin
chmod 755 $top/$toolroot/$appdir/bin/preinstall.bat
$utod $top/$toolroot/$appdir/bin/preinstall.bat

examples=$top/$toolroot/$appdir/examples
$utod $examples/README
$utod $examples/cgdtest.scr
$utod $examples/cgdtest1.scr
$utod $examples/density.scr
$utod $examples/density1.scr
$utod $examples/density2.scr
$utod $examples/diffpair
$utod $examples/fixvia.scr
$utod $examples/ma1
$utod $examples/ma2
$utod $examples/mosamp
$utod $examples/netcgdtest.scr
$utod $examples/preferences.scr
$utod $examples/spiral1.scr
$utod $examples/tkdemo.tk
$utod $examples/vec_examp1
$utod $examples/vec_examp2
$utod $examples/xclient.cc
$utod $examples/xclient.scr
$utod $examples/PCells/*
$utod $examples/memchip_example/README
$utod $examples/memchip_example/xic_tech.demo

help=$top/$toolroot/$appdir/help
$utod $help/*.hlp
$utod $help/*.xpm

startup=$top/$toolroot/$appdir/startup
$utod $startup/README
$utod $startup/device.lib
$utod $startup/model.lib
$utod $startup/wr_install
$utod $startup/xic_format_lib
$utod $startup/xic_mesg
$utod $startup/xic_stipples
if [ -f $startup/xic_tech ]; then
    $utod $startup/xic_tech
fi
$utod $startup/xic_tech.cni
$utod $startup/xic_tech.hyp
$utod $startup/xic_tech.n65
$utod $startup/xic_tech.scmos

scripts=$top/$toolroot/$appdir/scripts
$utod $scripts/blackbg.scr
$utod $scripts/fullcursor.scr
$utod $scripts/paths.scr
$utod $scripts/spiral.scr
$utod $scripts/spiralform.scr
$utod $scripts/whitebg.scr
$utod $scripts/whitebw.scr
$utod $scripts/yank.scr

scrkit=$top/$toolroot/$appdir/scrkit
$utod $scrkit/Makefile
$utod $scrkit/miscmath.h
$utod $scrkit/README
$utod $scrkit/scrkit.h
$utod $scrkit/si_args.h
$utod $scrkit/si_if_variable.h
$utod $scrkit/si_scrfunc.h
$utod $scrkit/template.cc
$utod $scrkit/test.scr

docs=$top/$toolroot/$appdir/docs
cp $basefiles/MSWINFO.TXT $docs
$utod $docs/$relnote
$utod $docs/README
$utod $docs/MSWINFO.TXT

sed -e s/APPNAME/$appname/ -e s/OSNAME/$osname/ \
  -e s/VERSION/$version/ -e s/ARCH/$arch/ \
  -e s/TOOLROOT/$toolroot/g < files/xictools_xic.iss.in > $appname.iss
$utod $appname.iss

$inno/iscc $appname.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

exfiles="$pkgfiles/$appname-$osname*.exe"
if [ x"exfiles" != x ]; then
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
