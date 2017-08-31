#######################################################################
LOCATION = vl/packages
#######################################################################

dummy:

.DEFAULT:
	@if [ ! -d root ]; then mkdir root; fi
	@tok=$@; \
	case $$tok in \
	Darwin64) if [ ! -f $$tok ]; then ln -s Darwin  $$tok; fi ;; \
	Debian*)  if [ ! -f $$tok ]; then ln -s Debian  $$tok; fi ;; \
	FreeBSD*) if [ ! -f $$tok ]; then ln -s FreeBSD $$tok; fi ;; \
	Linux*)   if [ ! -f $$tok ]; then ln -s Linux   $$tok; fi ;; \
	Solaris*) if [ ! -f $$tok ]; then ln -s Solaris $$tok; fi ;; \
	*) \
	    IFS="-"; \
	    set -- $$tok; \
	    if [ -z $$2 ]; then \
	        echo "Unknown target $$tok"; \
	        exit; \
	    fi; \
	    $(MAKE) $$2; \
	    cd $$2/$$1; $(MAKE) $$2 \
	;; \
	esac

Darwin FreeBSD Linux Solaris Win32:
	if [ ! -d root ]; then \
	    mkdir root; \
	fi

clean::
	-@for a in Darwin/*; do \
	    if [ -d $$a -a -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) clean) \
	    fi \
	done
	-@for a in Debian/*; do \
	    if [ -d $$a -a -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) clean) \
	    fi \
	done
	-@for a in FreeBSD/*; do \
	    if [ -d $$a -a -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) clean) \
	    fi \
	done
	-@for a in Linux/*; do \
	    if [ -d $$a -a -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) clean) \
	    fi \
	done
	-@for a in Solaris/*; do \
	    if [ -d $$a -a -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) clean) \
	    fi \
	done
	-@for a in Win32/*; do \
	    if [ -d $$a -a -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) clean) \
	    fi \
	done
	-@rm -rf root

distclean: clean
	-@for a in Darwin* Debian* FreeBSD* Linux* Solaris*; do \
	    case $$a in \
	    Darwin*)  if [ $$a != Darwin ];  then rm -f $$a; fi ;; \
	    Debian*)  if [ $$a != Debian ];  then rm -f $$a; fi ;; \
	    FreeBSD*) if [ $$a != FreeBSD ]; then rm -f $$a; fi ;; \
	    Linux*)   if [ $$a != Linux ];   then rm -f $$a; fi ;; \
	    Solaris*) if [ $$a != Solaris ]; then rm -f $$a; fi ;; \
	    esac \
	done

