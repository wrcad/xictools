LPATH = ../../../../xt_base

VERSION = `../../../version wrspice`
PKGDIR = $(LPATH)/packages/pkgfiles
SRCDIR = `pwd`/../../root
DEBIAN = $(SRCDIR)/DEBIAN
USER = `whoami`

PROG = wrspice

osbase = ${@:_64=}

##### WARNING  this is not tested #####

.DEFAULT:
	@if [ ! $@[0-7] != "Debian" ]; then \
	    echo "Unknown OSNAME $@"; \
	    exit; \
	fi
	@if [ ! -d $(DEBIAN) ]; then \
	    mkdir $(DEBIAN); \
	fi
	@cat files/control | sed s/@VERSION@/$(VERSION)/ | \
            sed s/@ARCH@/i386/ > $(DEBIAN)/control
	@chmod 644 $(DEBIAN)/control
	@cp -pf files/preinst $(DEBIAN)
	@cp -pf files/postinst $(DEBIAN)
	@cat files/prerm | sed s/@VERSION@/$(VERSION)/ > $(DEBIAN)/prerm
	@chmod 755 $(DEBIAN)/prerm
	@cat files/postrm | sed s/@VERSION@/$(VERSION)/ > $(DEBIAN)/postrm
	@chmod 755 $(DEBIAN)/postrm
	-@rm -f $(PKGDIR)/$(PROG)-$@*
	@sudo chown -R root:root $(SRCDIR)
	@if [ $(osbase) = $@ ]; then \
	    arch=i386; \
	else \
	    arch=amd_64; \
	fi; \
	dpkg-deb -b $(SRCDIR) $(PKGDIR)/$(PROG)-$@-$(VERSION)-$$arch.deb;
	@sudo chown -R $(USER) $(SRCDIR)

clean::

