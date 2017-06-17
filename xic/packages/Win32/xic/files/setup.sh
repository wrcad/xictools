#! /bin/sh
# $Id: setup.sh,v 1.24 2017/04/10 02:39:17 stevew Exp $

pkgfiles=../../pkgfiles

inno="/inno-5.5.1"

program=xic
top=../../root/usr/local
srctree=../../../..
version=`$srctree/version xic`
utod=../../util/utod.exe
tifs="$IFS"
IFS="."
set $version
relnote=xic$1.$2
IFS="$tifs"

if [ ! -f $utod ]; then
    cwd=`pwd`
    cd ../../util
    make utod.exe
    cd $cwd
fi

# Move binaries to the common bin
if [ ! -d $top/xictools/bin ]; then
    mkdir $top/xictools/bin
fi
cp ../../files/program.bat $top/xictools/bin/$program.bat
chmod 755 $top/xictools/bin/$program.bat
$utod $top/xictools/bin/$program.bat
mv $top/xictools/$program/bin/* $top/xictools/bin
rmdir $top/xictools/$program/bin

license=$top/xictools/license
if [ ! -d $license ]; then
    mkdir $license
fi
cp $srctree/src/secure/README.MSW $license
$utod $license/README.MSW

examples=$top/xictools/$program/examples
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

help=$top/xictools/$program/help
$utod $help/*.hlp
$utod $help/*.xpm

startup=$top/xictools/$program/startup
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

scripts=$top/xictools/$program/scripts
$utod $scripts/blackbg.scr
$utod $scripts/fullcursor.scr
$utod $scripts/paths.scr
$utod $scripts/spiral.scr
$utod $scripts/spiralform.scr
$utod $scripts/whitebg.scr
$utod $scripts/yank.scr

scrkit=$top/xictools/$program/scrkit
$utod $scrkit/Makefile
$utod $scrkit/miscmath.h
$utod $scrkit/README
$utod $scrkit/scrkit.h
$utod $scrkit/si_args.h
$utod $scrkit/si_if_variable.h
$utod $scrkit/si_scrfunc.h
$utod $scrkit/template.cc
$utod $scrkit/test.scr

docs=$top/xictools/$program/docs
cp ../../files/MSWINFO.TXT $docs
$utod $docs/$relnote
$utod $docs/README
$utod $docs/MSWINFO.TXT

sed -e s/VERSION/$version/ < files/$program.iss.in > $program.iss
$utod $program.iss

$inno/iscc $program.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

exfiles="$pkgfiles/xic-Win32*.exe"
if [ x"exfiles" != x ]; then
    for a in $exfiles; do
        rm -f $a
    done
fi

mv Output/*.exe $pkgfiles
rmdir Output
rm $program.iss
echo Done

