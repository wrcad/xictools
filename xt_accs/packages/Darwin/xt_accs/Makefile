LPATH = ../../../../xt_base
include $(LPATH)/packages/files/make_config

VERSION = `../../../version xt_accs`
RELEASE = `../../../version release`
PKGFILE = xt_accs-$(OSNAME)-$(VERSION)-$(DARWIN_ARCH).pkg
PKGNAME = XicTools.Accs.$(RELEASE)
PKGDIR = $(LPATH)/packages/pkgfiles
pd = `pwd`

dummy::
	echo Darwin targets: Darwin

Darwin Darwin64::
	sudo $(MAKE) OSNAME=$@ mkpkg

mkpkg::
	-@rm -f $(PKGDIR)/xt_accs-Darwin*
	-@rm -rf contents
	mkdir contents contents/xictools
	cp -R ../../root/usr/local/xictools/bin contents/xictools
	cp -R ../../root/usr/local/xictools/mozy contents/xictools
	chown -R root:admin contents
	pkgbuild --identifier $(PKGNAME) --version $(VERSION) \
  --root $(pd)/contents --install-location /usr/local $(PKGDIR)/$(PKGFILE)
	-@rm -rf contents

clean::
	-@rm -rf contents

subdirs::
	@echo

files::
	@echo

