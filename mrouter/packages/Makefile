#######################################################################
LOCATION = mrouter/packages
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

package::
	@if [ ! -d root ]; then mkdir root; fi
	@osname=$(OSNAME); \
	case $$osname in \
	Darwin*) if [ ! -h $$osname ]; then ln -s darwin $$osname; fi ;; \
	Linux*) \
	    if [ ! -h $$osname ]; then \
	        dst=`$(MAKE) testrpm`; \
	        if [ -n "$$dst" ]; then \
	            ln -s rpm $$osname; \
	        else \
	            echo "Packager for $$osname unavailable"; \
	            exit; \
	        fi; \
	    fi ;; \
	esac
	@osname=$(OSNAME); \
	cd $$osname; $(MAKE) package;

clean::
	-@for a in darwin/*; do \
	    if [ -d $$a -a -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) clean) \
	    fi \
	done
	-@for a in rpm/*; do \
	    if [ -d $$a -a -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) clean) \
	    fi \
	done
	-@for a in win32/*; do \
	    if [ -d $$a -a -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) clean) \
	    fi \
	done
	-@rm -rf root

distclean:: clean
	-@for a in Darwin* Linux*; do \
	    case $$a in \
	    Darwin*)  rm -f $$a ;; \
	    Linux*)   rm -f $$a ;; \
	    esac \
	done

testrpm::
	@if [ -f /etc/redhat-release ]; then \
	    dst=`rpm -qf /etc/redhat-release`; \
	elif [ -f /etc/centos-release ]; then \
	    dst=`rpm -qf /etc/centos-release`; \
	elif [ -f /etc/SuSE-release ]; then \
	    dst=`rpm -qf /etc/SuSE-release`; \
	fi; \
	echo $$dst

