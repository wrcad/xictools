#######################################################################
# devlib/adms/examples/Makefile
#######################################################################
# $Id: Makefile,v 1.10 2015/07/30 23:09:33 stevew Exp $
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

depend::
	@for a in $(SUBDIRS); do \
	    (cd $$a; $(MAKE) $@); \
	done

clean realclean distclean::
	@for a in $(SUBDIRS); do \
	    (cd $$a; $(MAKE) $@); \
	done
	rm -f examples

$(SUBDIRS)::
	cd $@; $(MAKE)

exlist:
	echo "$(SUBDIRS)" > examples
