#! /bin/sh
# $Id: contents.sh,v 1.16 2015/10/03 01:28:26 stevew Exp $

files=../../util/xic_files

version=$1
revision=$2
docfile=xic$version
tifs=$IFS
IFS="."
set -- $version
IFS=$tifs
gen=$1

echo "@name xic_$gen-$version.$revision"
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
    echo "xictools/xic/bin/$a"
done
echo
echo "@group wheel"
echo "@mode 0644"
docs=`$files docs`
for a in $docs; do
    echo "xictools/xic/docs/$a"
done
echo "xictools/xic/docs/$docfile"
echo
examples=`$files examples`
for a in $examples; do
    echo "xictools/xic/examples/$a"
done
examples_pcells=`$files examples_pcells`
for a in $examples_pcells; do
    echo "xictools/xic/examples/PCells/$a"
done
examples_memchip_example=`$files examples_memchip_example`
for a in $examples_memchip_example; do
    echo "xictools/xic/examples/memchip_example/$a"
done
echo
help=`$files help`
for a in $help; do
    echo "xictools/xic/help/$a"
done
help_screenshots=`$files help_screenshots`
for a in $help_screenshots; do
    echo "xictools/xic/help/screenshots/$a"
done
echo
icons=`$files icons`
for a in $icons; do
    echo "xictools/xic/icons/$a"
done
echo
startup=`$files startup`
for a in $startup; do
    echo "xictools/xic/startup/$a"
done
echo
scripts=`$files scripts`
for a in $scripts; do
    echo "xictools/xic/scripts/$a"
done
echo
scrkit=`$files scrkit`
for a in $scrkit; do
    echo "xictools/xic/scrkit/$a"
done
echo
echo "@exec chown root:wheel %D/xictools"
echo "@exec chown root:wheel %D/xictools/xic"
echo "@exec chown root:wheel %D/xictools/xic/bin"
echo "@exec chown root:wheel %D/xictools/xic/docs"
echo "@exec chown root:wheel %D/xictools/xic/examples"
echo "@exec chown root:wheel %D/xictools/xic/examples/PCells"
echo "@exec chown root:wheel %D/xictools/xic/examples/memchip_example"
echo "@exec chown root:wheel %D/xictools/xic/help"
echo "@exec chown root:wheel %D/xictools/xic/help/screenshots"
echo "@exec chown root:wheel %D/xictools/xic/icons"
echo "@exec chown root:wheel %D/xictools/xic/scripts"
echo "@exec chown root:wheel %D/xictools/xic/startup"
echo "@dirrm xictools/xic/bin"
echo "@dirrm xictools/xic/scripts"
echo "@dirrm xictools/xic/icons"
echo "@dirrm xictools/xic/help/screenshots"
echo "@dirrm xictools/xic/help"
echo "@dirrm xictools/xic/examples/memchip_example"
echo "@dirrm xictools/xic/examples/PCells"
echo "@dirrm xictools/xic/examples"
echo
