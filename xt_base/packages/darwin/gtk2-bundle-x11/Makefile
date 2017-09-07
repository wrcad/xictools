
APPNAME = gtk2_bundle_x11
APPTOK = gtk2-bundle-x11
VERSION = 4.0.1
RELEASE = `../../../../version release`
PKGFILE = $(APPNAME)-$(OSNAME)-$(VERSION)-x86_64.pkg
PKGNAME = XicTools.$(APPTOK)
PKGDIR = ../../pkgfiles
pd = `pwd`

.DEFAULT:
	@if [ ! $@[0-7] != "Darwin" ]; then \
	    echo "Unknown OSNAME $@"; \
	    exit; \
	fi
	sudo OSNAME=$@ $(MAKE) package

package::
	-@rm -f $(PKGDIR)/$(APPNAME)*
	-@files/mkgtkx11
	chown -R root:admin contents
	pkgbld="pkgbuild --identifier $(PKGNAME) --version $(VERSION) \
  --root $(pd)/contents --install-location /usr/local"; \
	if [ -d $(pd)/resources ]; then \
	    pkgbld="$$pkgbld --scripts $(pd)/resources"; \
        fi; \
        $$pkgbld $(PKGFILE)
	@if [ -f $(PKGFILE) ]; then \
	    mv -f $(PKGFILE) $(PKGDIR); \
	    echo ==================================; \
	    echo Package file $(PKGFILE); \
            echo moved to xt_base/packages/pkgfiles; \
	    echo ==================================; \
	fi
	-@rm -rf contents

clean::
	-@rm -rf contents

