BASE = ../../../xt_base

APPNAME = gtk2_bundle_x11

VERSION = 4.0.1
PKGFILE = $(APPNAME)-$(OSNAME)-$(VERSION)-$(ARCH).pkg
PKGNAME = xictools.$(APPNAME)
PKGDIR = ../../pkgfiles
OSNAME = `$(BASE)/info.sh osname`
ARCH = `$(BASE)/info.sh arch`
pd = `pwd`

package::
	@osname=$(OSNAME); \
	if [ $${osname:0:6} != "Darwin" ]; then \
	    echo "Unknown OSNAME $$osname"; \
	    exit; \
	fi
	sudo $(MAKE) buildpkg

buildpkg::
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

