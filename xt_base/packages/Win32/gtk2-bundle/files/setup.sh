#! /bin/sh
# $Id: setup.sh,v 1.2 2015/04/25 20:27:55 stevew Exp $

pkgfiles=../../pkgfiles

inno="/inno-5.5.1"

program="gtk2-bundle"
version=$1
utod=../../util/utod.exe

if [ ! -f $utod ]; then
    cwd=`pwd`
    cd ../../util
    make utod.exe
    cd $cwd
fi

sed -e s/VERSION/$version/ < files/$program.iss.in > $program.iss
$utod $program.iss

$inno/iscc $program.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

exfiles="$pkgfiles/gtk2-bundle*.exe"
if [ x"exfiles" != x ]; then
    for a in $exfiles; do
        rm -f $a
    done
fi

mv Output/*.exe $pkgfiles
rmdir Output
rm $program.iss
echo Done

