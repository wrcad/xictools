# lib/mozy/Makefile
# $Id: Makefile,v 1.1 2008/08/28 00:05:35 stevew Exp $

SUBDIRS = help startup

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

