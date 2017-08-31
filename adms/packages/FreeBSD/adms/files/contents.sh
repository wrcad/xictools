#! /bin/sh

version=$1
revision=$2
tifs=$IFS
IFS="."
set -- $version
IFS=$tifs
gen=$1

echo "@name xt_accs_$gen-$version.$revision"
echo
echo "@cwd /usr/local"
echo "@srcdir @SRCDIR@/usr/local"
echo
echo "@owner root"
echo "@group bin"
echo
echo "@mode 0755"
echo "xictools/bin/fcpp"
echo "xictools/bin/lstpack"
echo "xictools/bin/lstunpack"
echo
echo "@exec chown root:wheel %D/xictools"
echo "@exec chown root:wheel %D/xictools/bin"
echo
echo "@dirrm xictools/mozy/help/screenshots"
echo "@dirrm xictools/mozy/help"
echo "@dirrm xictools/mozy/startup"

