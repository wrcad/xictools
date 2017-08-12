BASE = ../../../../xt_base

VERSION = `../../../version vl`
BRANCH = `../../../version tools`
PKGFILE = vl-$@-$(VERSION)-i386.pkg
PKGDIR = $(BASE)/packages/pkgfiles
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
	-@rm -f $(PKGDIR)/vl-$@*
	-@rm -f contents description preinstall postinstall preremove postremove
	@tifs="$$IFS"; \
	IFS="."; \
	tmp=$(VERSION); \
	set -- $$tmp; \
	version="$$1.$$2"; \
	revision="$$3"; \
	IFS="$$tifs"; \
	files/contents.sh $$version $$revision | sed s%@SRCDIR@%$(SRCDIR)% \
  > contents; \
	cat files/description.sed | sed s/@VERSION@/$(VERSION)/ > description; \
	cp files/preinstall preinstall; \
	cp files/postinstall postinstall; \
	cat files/preremove.sed | sed s/@VERSION@/$$version/ > preremove; \
	cat files/postremove.sed | sed s/@VERSION@/$$version/ > postremove
	@chmod 0755 preinstall postinstall preremove postremove
	pkg create -f contents -c -"The vl Verilog simulator" -d description \
  -o /cad/vl-$(BRANCH) -v -i preinstall -I postinstall -k preremove \
  -K postremove $(PKGDIR)/$(PKGFILE)
	@rm -f contents description preinstall postinstall preremove postremove

clean::
	-@rm -f contents description preinstall postinstall preremove postremove

subdirs::
	@echo files

files::
	@echo \
  files/contents.sh \
  files/description.sed \
  files/postinstall \
  files/postremove.sed \
  files/preinstall \
  files/preremove.sed

