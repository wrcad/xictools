LPATH = ../../../../xt_base
include $(LPATH)/packages/files/make_config

VERSION = `../../../version wrspice`
RELEASE = `../../../version release`
PKGFILE = wrspice-$(OSNAME)-$(VERSION)-$(DARWIN_ARCH).pkg
PKGNAME = XicTools.WRspice.$(RELEASE)
PKGDIR = $(LPATH)/packages/pkgfiles
pd = `pwd`

dummy::
	echo Darwin targets: Darwin

Darwin Darwin64::
	sudo $(MAKE) OSNAME=$@ mkpkg

mkpkg::
	-@rm -f $(PKGDIR)/wrspice-Darwin*
	-@rm -rf contents
	mkdir contents contents/xictools
	cp -R ../../root/usr/local/xictools/wrspice contents/xictools
	chown -R root:admin contents
	pkgbuild --identifier $(PKGNAME) --version $(VERSION) \
  --root $(pd)/contents --install-location /usr/local \
  --scripts $(pd)/resources $(PKGDIR)/$(PKGFILE)
	-@rm -rf contents

clean::
	-@rm -rf contents

