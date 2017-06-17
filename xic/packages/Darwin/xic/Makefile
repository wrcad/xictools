LPATH = ../../../../xt_base
include $(LPATH)/packages/files/make_config

VERSION = `../../../version xic`
RELEASE = `../../../version release`
PKGFILE = xic-$(OSNAME)-$(VERSION)-$(DARWIN_ARCH).pkg
PKGNAME = XicTools.Xic.$(RELEASE)
PKGDIR = $(LPATH)/packages/pkgfiles
pd = `pwd`

dummy::
	echo Darwin targets: Darwin $(PKGNAME)

Darwin Darwin64::
	sudo $(MAKE) OSNAME=$@ mkpkg

mkpkg::
	-@rm -f $(PKGDIR)/xic-Darwin*
	-@rm -rf contents
	mkdir contents contents/xictools
	cp -R ../../root/usr/local/xictools/xic contents/xictools
	chown -R root:admin contents
	pkgbuild --identifier $(PKGNAME) --version $(VERSION) \
  --root $(pd)/contents --install-location /usr/local \
  --scripts $(pd)/resources $(PKGDIR)/$(PKGFILE)
	-@rm -rf contents

clean::
	-@rm -rf contents

