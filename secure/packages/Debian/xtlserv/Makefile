LPATH = ../../../../xt_base

VERSION = `../../../version xtlserv`
PKGDIR = $(LPATH)/packages//pkgfiles
SRCDIR = `pwd`/../../root
DEBIAN = $(SRCDIR)/DEBIAN
USER = `whoami`

PROG = xtlserv

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
#	@cp -pf files/preinst $(DEBIAN)
	@cp -pf files/postinst $(DEBIAN)
#	@cat files/prerm | sed s/@VERSION@/$(VERSION)/ > $(DEBIAN)/prerm
#	@chmod 755 $(DEBIAN)/prerm
#	@cat files/postrm | sed s/@VERSION@/$(VERSION)/ > $(DEBIAN)/postrm
#	@chmod 755 $(DEBIAN)/postrm
	-@rm -f $(PKGDIR)/$(PROG)-Debian*
	@sudo chown -R root:root $(SRCDIR)
	dpkg-deb -b $(SRCDIR) $(PKGDIR)/$(PROG)-Debian-$(VERSION)-i386.deb;
	@sudo chown -R $(USER) $(SRCDIR)

Debian64::
	@if [ ! -d $(DEBIAN) ]; then \
	    mkdir $(DEBIAN); \
	fi
	@cat files/control | sed s/@VERSION@/$(VERSION)/ | \
            sed s/@ARCH@/amd64/ > $(DEBIAN)/control
	@chmod 644 $(DEBIAN)/control
#	@cp -pf files/preinst $(DEBIAN)
	@cp -pf files/postinst $(DEBIAN)
#	@cat files/prerm | sed s/@VERSION@/$(VERSION)/ > $(DEBIAN)/prerm
#	@chmod 755 $(DEBIAN)/prerm
#	@cat files/postrm | sed s/@VERSION@/$(VERSION)/ > $(DEBIAN)/postrm
#	@chmod 755 $(DEBIAN)/postrm
	-@rm -f $(PKGDIR)/$(PROG)-Debian64*
	@sudo chown -R root:root $(SRCDIR)
	dpkg-deb -b $(SRCDIR) $(PKGDIR)/$(PROG)-Debian64-$(VERSION)-amd_64.deb;
	@sudo chown -R $(USER) $(SRCDIR)

clean::

