#! /bin/sh

files=../../util/mozy_files

version=$1
revision=$2
tifs=$IFS
IFS="."
set -- $version
IFS=$tifs
gen=$1

echo "@name mozy_$gen-$version.$revision"
echo
echo "@cwd /usr/local"
echo "@srcdir @SRCDIR@/usr/local"
echo
echo "@owner root"
echo "@group bin"
echo
echo "@mode 0755"
bin=`$files bin`
for a in $bin; do
    echo "xictools/bin/$a"
done
echo
echo "@group wheel"
echo "@mode 0644"
echo
help=`$files help`
for a in $help; do
    echo "xictools/mozy/help/$a"
done
ss=`$files help_ss`
for a in $ss; do
    echo "xictools/mozy/help/screenshots/$a"
done
echo
startup=`$files startup`
for a in $startup; do
    echo "xictools/mozy/startup/$a"
done
echo
echo "@exec chown root:wheel %D/xictools"
echo "@exec chown root:wheel %D/xictools/mozy"
echo "@exec chown root:wheel %D/xictools/mozy/help"
echo "@exec chown root:wheel %D/xictools/mozy/help/screenshots"
echo "@exec chown root:wheel %D/xictools/mozy/startup"
echo
echo "@dirrm xictools/mozy/help/screenshots"
echo "@dirrm xictools/mozy/help"
echo "@dirrm xictools/mozy/startup"

