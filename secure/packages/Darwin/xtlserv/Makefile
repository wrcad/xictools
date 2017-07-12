LPATH = ../../../../xt_base

VERSION = `../../../version xtlserv`
RELEASE = `../../../version release`
PKGFILE = xtlserv-$(OSNAME)-$(VERSION)-x86_64.pkg
PKGNAME = XicTools.Xtlserv.$(RELEASE)
PKGDIR = $(LPATH)/packages//pkgfiles
pd = `pwd`

.DEFAULT:
	@if [ ! $@[0-7] != "Darwin" ]; then \
	    echo "Unknown OSNAME $@"; \
	    exit; \
	fi
	-@rm -f $(PKGDIR)/xtlserv-Darwin*
	-@rm -rf contents
	mkdir contents contents/xictools
	cp -R ../../root/usr/local/xictools/bin contents/xictools
	cp -R ../../root/usr/local/xictools/license contents/xictools
	chown -R root:admin contents
	pkgbuild --identifier $(PKGNAME) --version $(VERSION) \
  --root $(pd)/contents --install-location /usr/local \
  --scripts $(pd)/resources $(PKGDIR)/$(PKGFILE)
	-@rm -rf contents

clean::
	-@rm -rf contents

