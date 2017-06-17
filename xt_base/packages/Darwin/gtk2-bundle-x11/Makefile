include ../../files/make_config
VERSION = 4.0.1
PKGFILE = gtk2_bundle_x11-Darwin64-$(VERSION)-$(DARWIN_ARCH).pkg
RELEASE = `../../../../version release`
PKGNAME = XicTools.gtk2-bundle-x11
PKGDIR = ../../pkgfiles
pd = `pwd`

dummy::
	echo targets: Darwin

pkg::
	sudo $(MAKE) mkpkg

mkpkg::
	-@rm -f $(PKGDIR)/gtk2-bundle-x11*
	-@files/mkgtkx11
	chown -R root:admin contents
	pkgbuild --identifier $(PKGNAME) --version $(VERSION) \
  --root $(pd)/contents --install-location /usr/local $(PKGDIR)/$(PKGFILE)
	-@rm -rf contents

clean::
	-@rm -rf contents

