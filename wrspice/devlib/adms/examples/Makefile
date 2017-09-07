#######################################################################
LOCATION = devlib/adms/examples/Makefile
#######################################################################

# Create the makefiles and build the example modules.


SUBDIRS = \
  bsim6 \
  bsim6.1 \
  bsimcmg \
  bsoi44 \
  bsoi45 \
  cap \
  ekv \
  hicum0 \
  hicum2 \
  ind \
  jj \
  mextram \
  psp102 \
  psp103

dummy: $(SUBDIRS)

make::
	@for a in $(SUBDIRS); do \
	    (cd $$a; ./mkmk); \
	done

depend::
	@for a in $(SUBDIRS); do \
	    (cd $$a; ./mkmk; $(MAKE) $@); \
	done

clean realclean::
	@for a in $(SUBDIRS); do \
	    (cd $$a; $(MAKE) $@); \
	done

distclean::
	@for a in $(SUBDIRS); do \
	    (cd $$a; $(MAKE) $@; rm -f Makefile) \
	done
	-@rm -f examples

$(SUBDIRS)::
	cd $@; $(MAKE)

exlist:
	echo "$(SUBDIRS)" > examples
