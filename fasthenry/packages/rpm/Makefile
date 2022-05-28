BASE = ../../../xt_base

PROG = xictools_fasthenry
VERSION = `../../release.sh`
PKGDIR = $(BASE)/packages/pkgfiles
SRCDIR = `pwd`/../root
OSNAME = `$(BASE)/info.sh osname`
ARCH = `$(BASE)/info.sh arch`

package:
	@osname=$(OSNAME); \
	if [ $${osname:0:5} != "Linux" ]; then \
	    echo "Unknown OSNAME $$osname"; \
	    exit; \
	fi
	-@rm -f $(PKGDIR)/$(PROG)-Linux*
	-@rm -rf specfile BUILD BUILDROOT SOURCES SPECS SRPMS
	@files/specs.sh $(OSNAME) $(VERSION) $(SRCDIR) > specfile
	@if [ -f $$HOME/.rpmmacros ]; then \
	    mv -f $$HOME/.rpmmacros $$HOME/.rpmmacros_TMP; \
	fi
	@echo '%_rpmdir        '`pwd`> $$HOME/.rpmmacros
	@echo '%_var           /var'>> $$HOME/.rpmmacros
	@echo '%_topdir        %{_rpmdir}'>> $$HOME/.rpmmacros
	@echo "%_build_name_fmt \
  $(PROG)-$(OSNAME)-%%{VERSION}.%%{RELEASE}-$(ARCH).rpm">> $$HOME/.rpmmacros
	rpmbuild --buildroot $(SRCDIR) -bb --target $(ARCH) specfile
	@if [ -f $$HOME/.rpmmacros_TMP ]; then \
	    mv -f $$HOME/.rpmmacros_TMP $$HOME/.rpmmacros; \
	fi
	-@rm -rf specfile BUILD BUILDROOT SOURCES SPECS SRPMS
	@pkg=`ls $(PROG)-Linux*.rpm`; \
	if [ -n "$$pkg" ]; then \
	    mv -f $(PROG)-Linux* $(PKGDIR); \
	    echo ==================================; \
	    echo Package file $$pkg; \
            echo moved to xt_base/packages/pkgfiles; \
	    echo ==================================; \
	fi

clean::
	-@rm -rf specfile BUILD BUILDROOT SOURCES SPECS SRPMS

