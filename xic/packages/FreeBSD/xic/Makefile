LPATH = ../../../../xt_base
include $(LPATH)/packages/files/make_config

VERSION = `../../../version xic`
BRANCH = `../../../version tools`
PKGFILE = xic-$@-$(VERSION)-i386.pkg
PKGDIR = $(LPATH)/packages/pkgfiles
SRCDIR = `pwd`/../../root

releases = \
  FreeBSD10

dummy::
	echo FreeBSD targets: $(releases)

# Note:  when using set expansion, make variables seem to be
# implicitly quoted, so that these have to be moved to a shall
# variable for the splitting to work correctly.

$(releases)::
	echo "# Package manifest for xic-$@" > manifest
	echo 'cat << EOF' >> manifest
	echo "name:          Xic" >> manifest
	echo "version:       $(VERSION)" >> manifest
	echo 'origin:        Whiteley Research Inc.' >> manifest
	echo 'comment:       Graphical Editor' >> manifest
	echo 'arch:          freebsd:10:x86:32' >> manifest
	echo 'www:           http://wrcad.com' >> manifest
	echo 'maintainer:    stevew@wrcad.com' >> manifest
	echo 'prefix:        /' >> manifest
	echo 'licenselogic:  single' >> manifest
	echo 'licenses:      proprietary' >> manifest
	echo 'flatsize:      $${DIR_SIZE}' >> manifest
	echo 'desc:          Xic installer for FreeBSD 10' >> manifest
	echo 'categories:    [local]' >> manifest
	echo 'EOF' >> manifest
	./build-pkg.sh manifest /usr/home/stevew/src/xictools/src/packages/root

clean::
	-@rm -f contents description preinstall postinstall preremove postremove

