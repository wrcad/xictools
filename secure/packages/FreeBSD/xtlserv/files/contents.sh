#! /bin/sh
# $Id: contents.sh,v 1.2 2014/03/01 22:33:57 stevew Exp $

version=$1
revision=$2
tifs=$IFS
IFS="."
set -- $version
IFS=$tifs
gen=$1

echo "@name xtlserv_$gen-$version.$revision"
echo 
echo "@cwd /usr/local"
echo "@srcdir @SRCDIR@/usr/local"
echo 
echo "@owner root"
echo "@group bin"
echo 
echo "@mode 0755"
echo "xictools/bin/xtlserv"
echo "xictools/bin/xtjobs"
echo 
echo "@group wheel"
echo "@mode 0644"
echo "xictools/license/README"
echo 
echo "@exec chown root:wheel %D/xictools"
echo "@exec chown root:wheel %D/xictools/license"
echo "@exec chown root:wheel %D/xictools/bin"
