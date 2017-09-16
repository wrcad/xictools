#! /bin/sh

OSNAME=$1
VERSION=$2
SRCDIR=$3

files=../util/adms_files

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: ADMS translator'
echo "Name: xictools-adms-$OSNAME"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: open-source'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'The admsXml program is used to support Verilog-A in WRspice.'
echo

echo '%files'
echo
echo "%attr(0755, root, root) /usr/local/xictools/bin/admsXml"
echo

echo "%dir /usr/local/xictools/adms"
echo "%attr(0644, root, root) /usr/local/xictools/adms/README"

echo "%dir /usr/local/xictools/adms/doc"
echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/README"

echo "%dir /usr/local/xictools/adms/doc/html"
list=$($files html)
for a in $list; do
  echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/html/$a"
done

echo "%dir /usr/local/xictools/adms/doc/html/doc"
list=$($files html_doc)
for a in $list; do
  echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/html/doc/$a"
done

echo "%dir /usr/local/xictools/adms/doc/html/introduction"
list=$($files html_intro)
for a in $list; do
  echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/html/introduction/$a"
done

echo "%dir /usr/local/xictools/adms/doc/html/scripts"
list=$($files html_scripts)
for a in $list; do
  echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/html/scripts/$a"
done

echo "%dir /usr/local/xictools/adms/doc/html/tutorials"
list=$($files html_tutorials)
for a in $list; do
  echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/html/tutorials/$a"
done

echo "%dir /usr/local/xictools/adms/doc/html/tutorials/Ilya-Lisichkin"
echo "%dir /usr/local/xictools/adms/doc/html/tutorials/Ilya-Lisichkin/MOSlevel1"
list=$($files tmos)
for a in $list; do
  echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/html/tutorials/Ilya-Lisichkin/MOSlevel1/$a"
done
echo "%attr(0644, root, root) \"/usr/local/xictools/adms/doc/html/tutorials/Ilya-Lisichkin/MOSlevel1/MOS1 into ZSPICE.html\""

echo
echo "%dir /usr/local/xictools/adms/doc/man"
echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/man/admsXml.1"

echo
echo "%dir /usr/local/xictools/adms/doc/xml"
list=$($files xml)
for a in $list; do
  echo "%attr(0644, root, root) /usr/local/xictools/adms/doc/xml/$a"
done

echo
echo "%dir /usr/local/xictools/adms/examples"
echo "%attr(0755, root, root) /usr/local/xictools/adms/examples/admsCheck"
echo "%attr(0644, root, root) /usr/local/xictools/adms/examples/admsCheck.1"

echo "%dir /usr/local/xictools/adms/examples/scripts"
list=$($files scripts)
for a in $list; do
  echo "%attr(0644, root, root) /usr/local/xictools/adms/examples/scripts/$a"
done

echo "%dir /usr/local/xictools/adms/examples/testcases"
list=$($files testcases)
for a in $list; do
  echo "%attr(0644, root, root) /usr/local/xictools/adms/examples/testcases/$a"
done

echo
cat files/scripts
