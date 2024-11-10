#######################################################################
LOCATION = devlib/adms/examples/Makefile
#######################################################################

# Create the makefiles and build the example modules.


SUBDIRS = \
  bsim-bulk \
  bsim-cmg \
  bsim-img \
  bsim-soi \
  cap \
  ekv \
  hicum \
  ind \
  jj \
  mextram \
  psp \
  tjm

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
	    $(MAKE) -C $$a $@ WRSPICE= ; \
	done

distclean::
	@for a in $(SUBDIRS); do \
	    (cd $$a; $(MAKE) $@ WRSPICE= ; rm -f Makefile) \
	done
	-@rm -f examples

$(SUBDIRS)::
	cd $@; $(MAKE)

exlist:
	echo "$(SUBDIRS)" > examples
