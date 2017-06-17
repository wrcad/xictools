LPATH = ../../../../xt_base
include $(LPATH)/packages/files/make_config

VERSION = `../../../version xtlserv`
PKGDIR = $(LPATH)/packages//pkgfiles
SRCDIR = `pwd`/../../root

releases = \
   Linux \
   Linux2 \
   LinuxRHEL3 \
   LinuxRHEL4 \
   LinuxRHEL5 \
   LinuxRHEL6 \
   LinuxRHEL7 \
   LinuxRHEL3_64 \
   LinuxRHEL4_64 \
   LinuxRHEL5_64 \
   LinuxRHEL6_64 \
   LinuxRHEL7_64 \
   LinuxOSuSE \
   LinuxOSuSE_64

osbase = ${@:_64=}

dummy::
	echo RPM targets: $(releases)

$(releases)::
	-@rm -f $(PKGDIR)/xtlserv-Linux*
	-@rm -rf specfile BUILD BUILDROOT SOURCES SPECS SRPMS
	@files/specs.sh $@ $(VERSION) $(SRCDIR) > specfile
	if [ -f $$HOME/.rpmmacros ]; then \
	    mv -f $$HOME/.rpmmacros $$HOME/.rpmmacros_TMP; \
	fi; \
	if [ $(osbase) = $@ ]; then \
	    if [ $(osbase) = Linux2 -o $(osbase) = Linux ]; then \
	        arch=i386; \
	    else \
	        arch=i686; \
	    fi; \
	else \
	    arch=x86_64; \
	fi; \
	echo '%_rpmdir        '`pwd`> $$HOME/.rpmmacros; \
	echo '%_var           /var'>> $$HOME/.rpmmacro; \
	echo '%_topdir        %{_rpmdir}'>> $$HOME/.rpmmacro; \
	echo "%_build_name_fmt \
  xtlserv-$(osbase)-%%{VERSION}.%%{RELEASE}-$$arch.rpm">> $$HOME/.rpmmacros; \
	rpmbuild --buildroot $(SRCDIR) -bb --target $$arch specfile
	@if [ -f $$HOME/.rpmmacros_TMP ]; then \
	    mv -f $$HOME/.rpmmacros_TMP $$HOME/.rpmmacros; \
	fi
	-@rm -rf specfile BUILD BUILDROOT SOURCES SPECS SRPMS
	@mv -f xtlserv-Linux* $(PKGDIR)

clean::
	-@rm -rf specfile BUILD BUILDROOT SOURCES SPECS SRPMS
