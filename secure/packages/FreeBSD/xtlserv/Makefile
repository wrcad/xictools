LPATH = ../../../../xt_base
include $(LPATH)/packages/files/make_config

VERSION = `../../../version xtlserv`
BRANCH = `../../../version tools`
PKGFILE = xtlserv-$@-$(VERSION)-i386.pkg
PKGDIR = $(LPATH)/packages//pkgfiles
SRCDIR = `pwd`/../../root

releases = \
  FreeBSD \
  FreeBSD7 \
  FreeBSD8 \
  FreeBSD9 \
  FreeBSD10

dummy::
	echo FreeBSD targets: $(releases)

$(releases)::
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

