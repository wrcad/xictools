LPATH = ../../../../xt_base

VERSION = `../../../version xt_accs`
RELEASE = `../../../version release`
PKGFILE = xt_accs-$(OSNAME)-$(VERSION)-x86_64.pkg
PKGNAME = XicTools.Accs.$(RELEASE)
PKGDIR = $(LPATH)/packages/pkgfiles
pd = `pwd`

.DEFAULT:
	@if [ ! $@[0-7] != "Darwin" ]; then \
	    echo "Unknown OSNAME $@"; \
	    exit; \
	fi
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
