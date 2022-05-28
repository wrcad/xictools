BASE = ../../../xt_base

APPNAME = xictools_fastcap
SUBDIRS = bin fastcap

VERSION = `../../release.sh`
PKGFILE = $(APPNAME)-$(OSNAME)-$(VERSION)-$(ARCH).pkg
PKGNAME = $(APPNAME)-$(VERSION)
PKGDIR = $(BASE)/packages/pkgfiles
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
	-@rm -f $(PKGDIR)/$(APPNAME)-Darwin*
	-@rm -rf contents
	mkdir contents contents/xictools
	for a in $(SUBDIRS); do \
	    cp -R ../root/usr/local/xictools/$$a contents/xictools; \
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

