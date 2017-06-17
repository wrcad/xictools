# lib/xic/Makefile
# $Id: Makefile,v 1.1 2009/02/04 19:41:41 stevew Exp $

SUBDIRS = docs help scripts examples startup

dummy::
	-@for a in $(SUBDIRS); do \
	    if [ -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE)); \
	    fi; \
	done

distclean::
	-@for a in $(SUBDIRS); do \
	    if [ -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) distclean); \
	    fi; \
	done

