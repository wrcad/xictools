BASE = ../../../../xt_base

APPNAME = wrspice
APPTOK = WRspice
SUBDIRS = $(APPNAME)
VERSION = `../../../version $(APPNAME)`
RELEASE = `../../../version release`
PKGFILE = $(APPNAME)-$(OSNAME)-$(VERSION)-x86_64.pkg
PKGNAME = XicTools.$(APPTOK).$(RELEASE)
PKGDIR = $(BASE)/packages/pkgfiles
pd = `pwd`

.DEFAULT:
	@if [ ! $@[0-7] != "Darwin" ]; then \
	    echo "Unknown OSNAME $@"; \
	    exit; \
	fi
	sudo OSNAME=$@ $(MAKE) package

package::
	-@rm -f $(PKGDIR)/$(APPNAME)-Darwin*
	-@rm -rf contents
	mkdir contents contents/xictools
	for a in $(SUBDIRS); do \
	    cp -R ../../root/usr/local/xictools/$$a contents/xictools; \
	done
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

