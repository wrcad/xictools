LPATH = ../../../../xt_base
include $(LPATH)/packages/files/make_config

VERSION = `../../../version xt_accs`
BRANCH = `../../../version tools`
PKGFILE = xt_accs-$@-$(VERSION)-i386.pkg
PKGDIR = $(LPATH)/packages/pkgfiles
SRCDIR = `pwd`/../../root

releases = \
  FreeBSD \
  FreeBSD7 \
  FreeBSD8 \
  FreeBSD9 \
  FreeBSD10

dummy::
	echo FreeBSD targets: $(releases)

# Note:  when using set expansion, make variables seem to be
# implicitly quoted, so that these have to be moved to a shall
# variable for the splitting to work correctly.

$(releases)::
	-@rm -f $(PKGDIR)/xt_accs-$@*
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
	pkg create -f contents -c -"The XicTools accessories" -d description \
  -o /cad/xt_accs-$(BRANCH) -v -i preinstall -I postinstall -k preremove \
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

