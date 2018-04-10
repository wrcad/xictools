#! /bin/sh

OSNAME=$1
VERSION=$2
SRCDIR=$3

files=../util/wrspice_files
adms_examples=../util/adms_examples
top=wrspice.current

with_devkit=no
if [ -d ../root/usr/local/xictools/$top/devkit ]; then
    with_devkit=yes
fi

with_cadence=no
if [ -d ../root/usr/local/xictools/$top/cadence-oasis ]; then
    with_cadence=yes
fi

tifs="$IFS"
IFS="."
set -- $VERSION
VERSION="$1.$2"
RELEASE="$3"
IFS="$tifs"

echo 'Summary: Circuit simulation software'
echo "Name: xictools_wrspice-$OSNAME"
echo "Version: $VERSION"
echo "Release: $RELEASE"
echo 'Prefix: /usr/local'
echo "BuildRoot: $SRCDIR"
echo 'Vendor: Whiteley Research Inc.'
echo 'License: open-source'
echo 'Group: Applications/Engineering'
echo 'AutoReqProv: 0'
echo '%description'
echo 'The WRspice program is a general purpose simulator of electronic'
echo 'circuits.'
echo

echo '%files'
echo
echo "%dir /usr/local/xictools/$top"
echo "%dir /usr/local/xictools/$top/bin"
bin=`$files bin`
for a in $bin; do
    echo "%attr(0755, root, root) /usr/local/xictools/$top/bin/$a"
done

echo
echo "%dir /usr/local/xictools/$top/docs"
docs=`$files docs`
for a in $docs; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/docs/$a"
done
echo "%attr(0644, root, root) /usr/local/xictools/$top/docs/wrs$VERSION"

echo
echo "%dir /usr/local/xictools/$top/examples"
examples=`$files examples`
for a in $examples; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/examples/$a"
done
echo "%dir /usr/local/xictools/$top/examples/JJexamples"
jjex=`$files JJexamples`
for a in $jjex; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/examples/JJexamples/$a"
done
echo "%dir /usr/local/xictools/$top/examples/JJexamples_old"
jjex=`$files JJexamples_old`
for a in $jjex; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/examples/JJexamples_old/$a"
done

echo
echo "%dir /usr/local/xictools/$top/help"
help=`$files help`
for a in $help; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/help/$a"
done
echo "%dir /usr/local/xictools/$top/help/screenshots"
help_screenshots=`$files help_screenshots`
for a in $help_screenshots; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/help/screenshots/$a"
done

echo
echo "%dir /usr/local/xictools/$top/icons"
icons=`$files icons`
for a in $icons; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/icons/$a"
done

echo
echo "%dir /usr/local/xictools/$top/scripts"
scripts=`$files scripts`
for a in $scripts; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/scripts/$a"
done

echo
echo "%dir /usr/local/xictools/$top/startup"
startup=`$files startup`
for a in $startup; do
    echo "%config /usr/local/xictools/$top/startup/$a"
done
klulib=`$files klu Linux`
for a in $klulib; do
    echo "%attr(0755, root, root) /usr/local/xictools/$top/startup/$a"
done
echo "%dir /usr/local/xictools/$top/startup/devices"
echo "%attr(0644, root, root) /usr/local/xictools/$top/startup/devices/README"
vlmods=`$files vlmods`
for a in $vlmods; do
    echo "%attr(0644, root, root) /usr/local/xictools/$top/startup/devices/$a.so"
done

echo
if [ $with_devkit = yes ]; then
  echo "%dir /usr/local/xictools/$top/devkit"
  echo "%attr(0644, root, root) /usr/local/xictools/$top/devkit/Makefile"
  echo "%attr(0644, root, root) /usr/local/xictools/$top/devkit/README"
  echo "%attr(0644, root, root) /usr/local/xictools/$top/devkit/README.adms"
  echo "%dir /usr/local/xictools/$top/devkit/admst"
  admst=`$files admst`
  for a in $admst; do
      echo "%attr(0644, root, root) /usr/local/xictools/$top/devkit/admst/$a"
  done
  echo "%dir /usr/local/xictools/$top/devkit/include"
  devincl=`$files devincl`
  for a in $devincl; do
      echo "%attr(0644, root, root) /usr/local/xictools/$top/devkit/include/$a"
  done
  echo "%dir /usr/local/xictools/$top/devkit/examples"
  dkexdirs=`$files dkexdirs`
  for a in $dkexdirs; do
      echo "%dir /usr/local/xictools/$top/devkit/examples/$a"
      echo "%dir /usr/local/xictools/$top/devkit/examples/$a/tests"
      echo "%dir /usr/local/xictools/$top/devkit/examples/$a/module_dist"
  done
  for a in $dkexdirs; do
      mfile=`$files modname $a`.`$files soext Linux`
      echo "%attr(0755, root, root) /usr/local/xictools/$top/devkit/examples/$a/module_dist/$mfile"
  done
  efiles=`cat $adms_examples`
  for a in $efiles; do
      echo "%attr(0644, root, root) /usr/local/xictools/$top/devkit/$a"
  done
  echo
fi

echo
if [ $with_cadence = yes ]; then
  dir=/usr/local/xictools/$top/cadence-oasis
  echo "%dir $dir"
  echo "%attr(0644, root, root) $dir/advTool.il"
  echo "%attr(0644, root, root) $dir/analysis.il"
  echo "%attr(0644, root, root) $dir/ChangeLog"
  echo "%attr(0644, root, root) $dir/classes.il"
  echo "%attr(0644, root, root) $dir/dataAccess.il"
  echo "%attr(0644, root, root) $dir/envOption.il"
  echo "%attr(0644, root, root) $dir/hnl.ile"
  echo "%attr(0644, root, root) $dir/initialize.il"
  echo "%attr(0644, root, root) $dir/labels.il"
  echo "%attr(0755, root, root) $dir/makeAnalog"
  echo "%attr(0755, root, root) $dir/makeWrs"
  echo "%attr(0755, root, root) $dir/mkcdsenv"
  echo "%attr(0755, root, root) $dir/mkcx"
  echo "%attr(0644, root, root) $dir/netlist.il"
  echo "%attr(0644, root, root) $dir/OasisCustomer.pdf"
  echo "%attr(0644, root, root) $dir/params.il"
  echo "%attr(0644, root, root) $dir/README"
  echo "%attr(0644, root, root) $dir/se.ile"
  echo "%attr(0644, root, root) $dir/simControl.il"
  echo "%attr(0644, root, root) $dir/simInfo.il"
  echo "%attr(0644, root, root) $dir/simOption.il"
  echo "%attr(0644, root, root) $dir/startup.il"
  echo "%attr(0644, root, root) $dir/WRspiceBuild.il"
  echo "%attr(0644, root, root) $dir/WRspiceCdsenvFile"
  echo "%attr(0644, root, root) $dir/WRspice.cxt"
  echo "%attr(0644, root, root) $dir/WRspice.il"
  echo "%attr(0644, root, root) $dir/WRspice.ini"
  echo "%attr(0644, root, root) $dir/WRspiceInit.il"
  echo "%attr(0644, root, root) $dir/WRspice.menus"

  echo "%dir $dir/64bit"
  echo "%attr(0644, root, root) $dir/64bit/README"
  echo "%attr(0644, root, root) $dir/64bit/WRspice.cxt"

  echo "%dir $dir/iftest"
  echo "%attr(0644, root, root) $dir/iftest/cdsinfo.tag"
  echo "%attr(0644, root, root) $dir/iftest/data.dm"
  echo "%attr(0644, root, root) $dir/iftest/.oalib"
  echo "%dir $dir/iftest/ift1"
  sch=$dir/iftest/ift1/schematic
  echo "%dir $sch"
  echo "%attr(0644, root, root) $sch/data.dm"
  echo "%attr(0644, root, root) $sch/master.tag"
  echo "%attr(0644, root, root) $sch/sch.oa"
  echo "%attr(0644, root, root) $sch/thumbnail_128x128.png"
  echo "%dir $dir/iftest/ift2"
  sch=$dir/iftest/ift2/schematic
  echo "%dir $sch"
  echo "%attr(0644, root, root) $sch/data.dm"
  echo "%attr(0644, root, root) $sch/master.tag"
  echo "%attr(0644, root, root) $sch/sch.oa"
  echo "%attr(0644, root, root) $sch/thumbnail_128x128.png"
  echo "%dir $dir/iftest/ift3"
  sch=$dir/iftest/ift3/schematic
  echo "%dir $sch"
  echo "%attr(0644, root, root) $sch/data.dm"
  echo "%attr(0644, root, root) $sch/master.tag"
  echo "%attr(0644, root, root) $sch/sch.oa"
  echo "%attr(0644, root, root) $sch/thumbnail_128x128.png"
  echo "%dir $dir/iftest/ift4"
  sch=$dir/iftest/ift4/schematic
  echo "%dir $sch"
  echo "%attr(0644, root, root) $sch/data.dm"
  echo "%attr(0644, root, root) $sch/master.tag"
  echo "%attr(0644, root, root) $sch/sch.oa"
  echo "%attr(0644, root, root) $sch/thumbnail_128x128.png"
  echo "%dir $dir/iftest/ift5"
  sch=$dir/iftest/ift5/schematic
  echo "%dir $sch"
  echo "%attr(0644, root, root) $sch/data.dm"
  echo "%attr(0644, root, root) $sch/master.tag"
  echo "%attr(0644, root, root) $sch/sch.oa"
  echo "%attr(0644, root, root) $sch/thumbnail_128x128.png"
  echo "%dir $dir/iftest/jtl4"
  echo "%attr(0644, root, root) $dir/iftest/jtl4//data.dm"
  sch=$dir/iftest/jtl4/schematic
  echo "%dir $sch"
  echo "%attr(0644, root, root) $sch/data.dm"
  echo "%attr(0644, root, root) $sch/master.tag"
  echo "%attr(0644, root, root) $sch/sch.oa"
  echo "%attr(0644, root, root) $sch/thumbnail_128x128.png"
  sch=$dir/iftest/jtl4/symbol
  echo "%dir $sch"
  echo "%attr(0644, root, root) $sch/master.tag"
  echo "%attr(0644, root, root) $sch/symbol.oa"
  echo "%attr(0644, root, root) $sch/thumbnail_128x128.png"

  echo "%dir $dir/WRspiceDevs"
  echo "%attr(0644, root, root) $dir/WRspiceDevs/cdsinfo.tag"
  echo "%attr(0644, root, root) $dir/WRspiceDevs/data.dm"
  echo "%attr(0644, root, root) $dir/WRspiceDevs/.oalib"
  echo "%dir $dir/WRspiceDevs/jj"
  echo "%attr(0644, root, root) $dir/WRspiceDevs/jj/data.dm"
  sch=$dir/WRspiceDevs/jj/symbol
  echo "%dir $sch"
  echo "%attr(0644, root, root) $sch/master.tag"
  echo "%attr(0644, root, root) $sch/symbol.oa"
  echo "%attr(0644, root, root) $sch/thumbnail_128x128.png"
  sch=$dir/WRspiceDevs/jj/WRspice
  echo "%dir $sch"
  echo "%attr(0644, root, root) $sch/master.tag"
  echo "%attr(0644, root, root) $sch/symbol.oa"
  echo "%attr(0644, root, root) $sch/thumbnail_128x128.png"

  echo "%dir $dir/wrsmods"
  echo "%attr(0644, root, root) $dir/wrsmods/models.scs"
fi
cat files/scripts

