
APPNAME = gtk2_bundle_x11
APPTOK = gtk2-bundle-x11
VERSION = 4.0.1
PKGFILE = $(APPNAME)-Darwin64-$(VERSION)-x86_64.pkg
RELEASE = `../../../../version release`
PKGNAME = XicTools.$(APPTOK)
PKGDIR = ../../pkgfiles
pd = `pwd`

dummy::
	echo targets: Darwin

pkg::
	sudo $(MAKE) mkpkg

mkpkg::
	-@rm -f $(PKGDIR)/$(APPNAME)*
	-@files/mkgtkx11
	chown -R root:admin contents
	pkgbld="pkgbuild --identifier $(PKGNAME) --version $(VERSION) \
  --root $(pd)/contents --install-location /usr/local"; \
	if [ -d $(pd)/resources ]; then \
	    pkgbld="$$pkgbld --scripts $(pd)/resources"; \
        fi; \
        $$pkgbld $(PKGDIR)/$(PKGFILE)
	-@rm -rf contents

clean::
	-@rm -rf contents

