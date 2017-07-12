LPATH = ../../../../xt_base

VERSION = `../../../version xtlserv`
BRANCH = `../../../version tools`
PKGFILE = xtlserv-$@-$(VERSION)-i386.pkg
PKGDIR = $(LPATH)/packages//pkgfiles
SRCDIR = `pwd`/../../root

osbase = ${@:_64=}

##### WARNING This is designed for a packaging system that is no
##### longer in use.  This script hasn't been tested.

# Note:  when using set expansion, make variables seem to be
# implicitly quoted, so that these have to be moved to a shell
# variable for the splitting to work correctly.

.DEFAULT:
	@if [ ! $@[0-8] != "FreeBSD" ]; then \
	    echo "Unknown OSNAME $@"; \
	    exit; \
	fi
	-@rm -f $(PKGDIR)/xtlserv-$@*
	-@rm -f contents description postinstall
	@tifs="$$IFS"; \
	IFS="."; \
	tmp=$(VERSION); \
	set -- $$tmp; \
	version="$$1.$$2"; \
	revision="$$3"; \
	IFS="$$tifs"; \
	files/contents.sh $$version $$revision | sed s%@SRCDIR@%$(SRCDIR)% \
  > contents; \
	@cat files/description.sed | sed s/@VERSION@/$(VERSION)/ > description
	@cp files/postinstall postinstall
	@chmod 0755 postinstall
	pkg_create -f contents -c -"The XicTools license server" \
  -d description -v -I postinstall -o cad/xtlserv-$(BRANCH) \
  $(PKGDIR)/$(PKGFILE)
	@rm -f contents description postinstall

clean::
	-@rm -f contents description postinstall

