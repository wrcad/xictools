#! /bin/sh

version=$1
revision=$2
tifs=$IFS
IFS="."
set -- $version
IFS=$tifs
gen=$1

echo "@name vl$gen-$version.$revision"
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

FIXME!
