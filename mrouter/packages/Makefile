#######################################################################
LOCATION = mrouter/packages
#######################################################################

# To build a package:
# In this directory, give "make dirname-osname".  The dirname is the
# name of a subdirectory found under the subdirectories of this
# directory, which is usually the application name.  The osname,
# separated from the dirname with a hyphen (no space), is the name
# assigned by the configure script for this machine and environment. 
# This is used here to identify the native packaging system, and a
# link to the appropriate packaging subdirectory is created.  This
# really applies only to Linux, which can have rpm (RedHat) or dpkg
# (Debian) packagers.

# Note:  The subdirectory names are lower-case, to differentiate from
# the osname (the make arg), and thus the symbolic link name.  The
# exception is Windows, where we don't have symbolic links, and in
# this case the packager subdirectory name must match the osname token
# exactly.  The osname is mixed-case with the lead character
# upper-case.

dummy:

.DEFAULT:
	@if [ ! -d root ]; then mkdir root; fi
	@tok=$@; \
	case $$tok in \
	Darwin*) if [ ! -f $$tok ]; then ln -s darwin  $$tok; fi ;; \
	Linux*) \
	    if [ ! -f $$tok ]; then \
	        dst=`$(MAKE) testrpm`; \
	        if [ -n "$$dst" ]; then \
	            ln -s rpm   $$tok; \
	        else \
	            echo "Packager for $$tok unavailable"; \
	            exit; \
	        fi; \
	    fi ;; \
	*) \
	    IFS="-"; \
	    set -- $$tok; \
	    if [ -z $$2 ]; then \
	        echo "Unknown target $$tok"; \
	        exit; \
	    fi; \
	    $(MAKE) $$2; \
	    if [ -d $$2/$$1 ]; then \
	        cd $$2/$$1; $(MAKE) $$2; \
	    else \
	        echo "Unknown package $$2/$$1"; \
	        exit; \
	    fi \
	;; \
	esac

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

distclean: clean
	-@for a in Darwin* Linux*; do \
	    case $$a in \
	    Darwin*)  rm -f $$a ;; \
	    Linux*)   rm -f $$a ;; \
	    esac \
	done

testrpm:
	@if [ -f /etc/redhat-release ]; then \
	    dst=`rpm -qf /etc/redhat-release`; \
	elif [ -f /etc/centos-release ]; then \
	    dst=`rpm -qf /etc/centos-release`; \
	elif [ -f /etc/SuSE-release ]; then \
	    dst=`rpm -qf /etc/SuSE-release`; \
	fi; \
	echo $$dst

