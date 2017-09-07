BASE = ../../../../xt_base

PROG = adms_wr
VERSION = `../../../version $(PROG)`
PKGDIR = $(BASE)/packages/pkgfiles
SRCDIR = `pwd`/../../root

osbase = ${@:_64=}

.DEFAULT:
	@if [ ! $@[0-6] != "Linux" ]; then \
	    echo "Unknown OSNAME $@"; \
	    exit; \
	fi
	-@rm -f $(PKGDIR)/$(PROG)-Linux*
	-@rm -rf specfile BUILD BUILDROOT SOURCES SPECS SRPMS
	@files/specs.sh $@ $(VERSION) $(SRCDIR) > specfile
	@if [ -f $$HOME/.rpmmacros ]; then \
	    mv -f $$HOME/.rpmmacros $$HOME/.rpmmacros_TMP; \
	fi
	@if [ $(osbase) = $@ ]; then \
	    arch=i686; \
	else \
	    arch=x86_64; \
	fi; \
	echo '%_rpmdir        '`pwd`> $$HOME/.rpmmacros; \
	echo '%_var           /var'>> $$HOME/.rpmmacros; \
	echo '%_topdir        %{_rpmdir}'>> $$HOME/.rpmmacros; \
	echo "%_build_name_fmt \
  $(PROG)-$(osbase)-%%{VERSION}.%%{RELEASE}-$$arch.rpm">> $$HOME/.rpmmacros; \
	rpmbuild --buildroot $(SRCDIR) -bb --target $$arch specfile
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

