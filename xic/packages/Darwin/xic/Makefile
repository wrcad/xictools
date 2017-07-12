LPATH = ../../../../xt_base

VERSION = `../../../version xic`
RELEASE = `../../../version release`
PKGFILE = xic-$(OSNAME)-$(VERSION)-x86_64.pkg
PKGNAME = XicTools.Xic.$(RELEASE)
PKGDIR = $(LPATH)/packages/pkgfiles
pd = `pwd`

.DEFAULT:
	@if [ ! $@[0-7] != "Darwin" ]; then \
	    echo "Unknown OSNAME $@"; \
	    exit; \
	fi
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

