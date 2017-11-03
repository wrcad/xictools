BASE = ../../../xt_base

PROG = xictools_wrspice
VERSION = `../../version`
PKGDIR = $(BASE)/packages/pkgfiles
SRCDIR = `pwd`/../root
OSNAME = `$(BASE)/info.sh osname`
ARCH = `$(BASE)/info.sh arch`
DEBIAN = $(SRCDIR)/DEBIAN
USER = `whoami`

package:
	@if [ ! -d $(DEBIAN) ]; then \
	    mkdir $(DEBIAN); \
	fi
	@cat files/control.sed | sed -e s/@VERSION@/$(VERSION)/ \
            -e s/@ARCH@/i386/ > $(DEBIAN)/control
	@chmod 644 $(DEBIAN)/control
	@if [ -f files/preinst ]; then \
	    cp -pf files/preinst $(DEBIAN); \
	fi
	@if [ -f files/postinst ]; then \
	    cp -pf files/postinst $(DEBIAN); \
	fi
	@if [ -f files/postrm ]; then \
	    cp -pf files/postrm $(DEBIAN); \
	fi
	-@rm -f $(PKGDIR)/$(PROG)-$(OSNAME)*
	@sudo chown -R root:root $(SRCDIR)
	dpkg-deb -b $(SRCDIR) $(PKGDIR)/$(PROG)-$(OSNAME)-$(VERSION)-$(ARCH).deb
	@sudo chown -R $(USER) $(SRCDIR)

clean:
	-@rm -rf $(DEBIAN)

