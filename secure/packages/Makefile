#######################################################################
LOCATION = secure/packages
#######################################################################

# To build a package:
# In this directory, give "make package", or just "make".

# The OSNAME (obtained from configuration info) is used here to
# identify the native packaging system, and except under Windows a
# link to the appropriate packaging subdirectory is created.  This is
# intended for Linux, for which various packagers exist, including rpm
# (RedHat) and dpkg (Debian).

BASE = ../../xt_base
OSNAME = `$(BASE)/info.sh osname`
SFX = `$(BASE)/info.sh dist_suffix`

package::
	@if [ ! -d root ]; then mkdir root; fi
	@osname=$(OSNAME); \
	case $$osname in \
	Darwin*) \
	    if [ -e $$osname -a ! -L $$osname ]; then \
	        echo "Error: $$osname exists and is not a symbolic link"; \
	        exit; \
	    fi; \
            if [ ! -L $$osname ]; then ln -s osxpkg $$osname; fi ;; \
	Linux*) \
	    if [ -e $$osname -a ! -L $$osname ]; then \
	        echo "Error: $$osname exists and is not a symbolic link"; \
	        exit; \
	    fi; \
	    if [ ! -L $$osname ]; then \
	        if [ x$(SFX) = xrpm ]; then \
	            ln -s rpm $$osname; \
	        elif [ x$(SFX) = xdeb ]; then \
	            ln -s dpkg $$osname; \
	        else \
	            echo "No current packager support for $$osname"; \
	            exit; \
	        fi; \
	    fi ;; \
	Win32) ;; \
	*) \
	    if [ -z "$$osname" ]; then \
	        echo "Error: Can't determine OSNAME"; \
	    else \
	        echo "Unsupported OSNAME $$osname"; \
	    fi; \
	    exit ;; \
	esac
	@osname=$(OSNAME); \
	cd $$osname; $(MAKE) package;

clean::
	-@if [ -d dpkg ]; then cd dpkg; $(MAKE) clean; fi
	-@if [ -d osxpkg ]; then cd osxpkg; $(MAKE) clean; fi
	-@if [ -d rpm ]; then cd rpm; $(MAKE) clean; fi
	-@if [ -d Win32 ]; then cd Win32; $(MAKE) clean; fi
	-@rm -rf root

distclean:: clean
	-@rm -f Darwin* Linux*

# not used
testrpm::
	@if [ -f /etc/redhat-release ]; then \
	    dst=`rpm -qf /etc/redhat-release`; \
	elif [ -f /etc/centos-release ]; then \
	    dst=`rpm -qf /etc/centos-release`; \
	elif [ -f /etc/SuSE-release ]; then \
	    dst=`rpm -qf /etc/SuSE-release`; \
	fi; \
	echo $$dst

