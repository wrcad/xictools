#######################################################################
LOCATION = devlib/adms/examples/Makefile
#######################################################################

# Create the makefiles and build the example modules.

# This just lists the available modules.
ALLMODULES = \
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

# This is a list of the modules to actually build.  Some take a long
# time to build, and you may not need them anyway.  Simply remove the
# names of modules that you want to skip, all lines except the bottom
# should have a trailing backslash ('\').
MODULES = \
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

JJS = jj tjm

dummy: $(MODULES)
jjs: $(JJS)

make::
	@for a in $(MODULES); do \
	    (cd $$a; ./mkmk); \
	done

depend::
	@for a in $(MODULES); do \
	    (cd $$a; ./mkmk; $(MAKE) $@); \
	done

clean realclean::
	@for a in $(MODULES); do \
	    $(MAKE) -C $$a $@ WRSPICE= ; \
	done

distclean::
	@for a in $(MODULES); do \
	    (cd $$a; $(MAKE) $@ WRSPICE= ; rm -f Makefile) \
	done
	-@rm -f examples

$(MODULES)::
	cd $@; $(MAKE)

exlist:
	echo "$(MODULES)" > examples
