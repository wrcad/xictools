BASE = ../../../../xt_base

VERSION = `../../../version mozy`
RELEASE = `../../../version release`
PKGFILE = mozy-$(OSNAME)-$(VERSION)-x86_64.pkg
PKGNAME = XicTools.Mozy.$(RELEASE)
PKGDIR = $(BASE)/packages/pkgfiles
pd = `pwd`

.DEFAULT:
	@if [ ! $@[0-7] != "Darwin" ]; then \
	    echo "Unknown OSNAME $@"; \
	    exit; \
	fi
	-@rm -f $(PKGDIR)/mozy-Darwin*
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
