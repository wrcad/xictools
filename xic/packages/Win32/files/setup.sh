#! /bin/sh

inno="/inno-5.5.9"

appname=xictools_xic
appdir=xic.current
version=`../../version`
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

chmod 755 $top/xictools/$appdir/bin/xic.bat
$utod $top/xictools/$appdir/bin/xic.bat
cp -f files/postinstall.bat $top/xictools/$appdir/bin
chmod 755 $top/xictools/$appdir/bin/postinstall.bat
$utod $top/xictools/$appdir/bin/postinstall.bat
cp -f files/preinstall.bat $top/xictools/$appdir/bin
chmod 755 $top/xictools/$appdir/bin/preinstall.bat
$utod $top/xictools/$appdir/bin/preinstall.bat

examples=$top/xictools/$appdir/examples
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

help=$top/xictools/$appdir/help
$utod $help/*.hlp
$utod $help/*.xpm

startup=$top/xictools/$appdir/startup
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

scripts=$top/xictools/$appdir/scripts
$utod $scripts/blackbg.scr
$utod $scripts/fullcursor.scr
$utod $scripts/paths.scr
$utod $scripts/spiral.scr
$utod $scripts/spiralform.scr
$utod $scripts/whitebg.scr
$utod $scripts/yank.scr

scrkit=$top/xictools/$appdir/scrkit
$utod $scrkit/Makefile
$utod $scrkit/miscmath.h
$utod $scrkit/README
$utod $scrkit/scrkit.h
$utod $scrkit/si_args.h
$utod $scrkit/si_if_variable.h
$utod $scrkit/si_scrfunc.h
$utod $scrkit/template.cc
$utod $scrkit/test.scr

docs=$top/xictools/$appdir/docs
cp $basefiles/MSWINFO.TXT $docs
$utod $docs/$relnote
$utod $docs/README
$utod $docs/MSWINFO.TXT

sed -e s/VERSION/$version/ < files/$appname.iss.in > $appname.iss
$utod $appname.iss

$inno/iscc $appname.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

exfiles="$pkgfiles/$appname-Win32*.exe"
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
