# xic/lib/Makefile

SUBDIRS = docs help scripts examples icons startup

# The depend target does general setup required before build.  Do this
# during dependency check.

depend::
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
	
