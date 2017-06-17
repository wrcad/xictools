# lib/xic/Makefile
# $Id: Makefile,v 1.1 2008/05/08 20:03:19 stevew Exp $

SUBDIRS = docs help scripts examples icons startup

dummy::
	-@for a in $(SUBDIRS); do \
	    if [ -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE)); \
	    fi; \
	done

clean:

distclean:
	-@for a in $(SUBDIRS); do \
	    if [ -f $$a/Makefile ]; then \
	        (cd $$a; $(MAKE) distclean); \
	    fi; \
	done
	
