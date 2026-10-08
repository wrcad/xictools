BASE = ../../../xt_base

APPNAME = xictools_xtlserv
SUBDIRS = bin license

VERSION = `../../release.sh`
PKGFILE = $(APPNAME)-$(OSNAME)-$(VERSION)-$(ARCH).pkg
PKGNAME = $(APPNAME)-$(VERSION)
PKGDIR = $(BASE)/packages/pkgfiles
OSNAME = `$(BASE)/info.sh osname`
ARCH = `$(BASE)/info.sh arch`
pd = `pwd`

package::
	@osname=$(OSNAME); \
	if [ $${osname:0:6} != "Darwin" ]; then \
	    echo "Unknown OSNAME $$osname"; \
	    exit; \
	fi
	sudo $(MAKE) buildpkg

#NOTE: https://community.cisco.com/t5/security-knowledge-base/deploying-the-cisco-secure-client-cm-deployment-on-macos-with/ta-p/5580661/jump-to/first-unread-message
# "During the build, pkgbuild may display four “write: Permission denied”
# messages. These are non-fatal — the package is still written successfully,
# confirmed by the final “Wrote package to...” line. The resulting package
# has also been confirmed to install correctly despite these messages."

buildpkg::
	-@rm -f $(PKGDIR)/$(APPNAME)-Darwin*
	-@rm -rf contents
	mkdir contents contents/xictools
	for a in $(SUBDIRS); do \
	    cp -R ../root/usr/local/xictools/$$a contents/xictools; \
	done
	chown -R root:admin contents
	pkgbld="pkgbuild \
	       --root $(pd)/contents \
	       --install-location /usr/local \
	       --identifier $(PKGNAME) \
	       --version $(VERSION)"; \
	if [ -d $(pd)/resources ]; then \
	    pkgbld="$$pkgbld --scripts $(pd)/resources"; \
        fi; \
        $$pkgbld $(PKGFILE)
	@if [ -f $(PKGFILE) ]; then \
	    mv -f $(PKGFILE) $(PKGDIR); \
	    echo ==================================; \
	    echo Package file $(PKGFILE); \
            echo moved to xt_base/packages/pkgfiles; \
	    echo ==================================; \
	fi
	-@rm -rf contents

clean::
	-@rm -rf contents $(PKGFILE)

