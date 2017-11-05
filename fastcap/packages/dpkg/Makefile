BASE = ../../../xt_base

PROG = xictools_fastcap
VERSION := $(shell ../../version)
PKGDIR := $(BASE)/packages/pkgfiles
SRCDIR := $(shell pwd)/../root
OSNAME := $(shell $(BASE)/info.sh osname)
ARCH := $(shell $(BASE)/info.sh arch)
DEBIAN := $(SRCDIR)/DEBIAN
USER := $(shell whoami)
PKGFILE := $(PROG)-$(OSNAME)-$(VERSION)-$(ARCH).deb
OSID := $(shell echo $(OSNAME) | sed -e s/Linux// | tr A-Z a-z)

package:
	@if [ ! -d $(DEBIAN) ]; then \
	    mkdir $(DEBIAN); \
	fi
	@if [ $(ARCH) = x86_64 ]; then \
	    arch=amd64; \
	else \
	    arch=$(ARCH); \
	fi; \
	sed -e s/@OSID@/$(OSID)/ -e s/@VERSION@/$(VERSION)/ \
            -e s/@ARCH@/$$arch/ < files/control.sed > $(DEBIAN)/control
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
	-@rm -f $(PKGDIR)/$(PROG)-$(OSNAME)* $(PKGFILE)
	@sudo chown -R root:root $(SRCDIR)
	-dpkg-deb -b $(SRCDIR) $(PKGFILE)
	@sudo chown -R $(USER) $(SRCDIR)
	@if [ -f $(PKGFILE) ]; then \
	    mv -f $(PKGFILE) $(PKGDIR); \
	    echo ==================================; \
	    echo Package file $(PKGFILE); \
            echo moved to xt_base/packages/pkgfiles; \
	    echo ==================================; \
	fi

clean:
	-@rm -rf $(DEBIAN)

