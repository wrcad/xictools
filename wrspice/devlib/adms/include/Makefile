#######################################################################
# wrspice/devlib/adms/include
#######################################################################

SPHFILES = circuit.h device.h acdefs.h dctdefs.h distdefs.h noisdefs.h \
 errors.h ifcx.h ifdata.h inpline.h input.h
INCL = ../../..

depend: $(SPHFILES)

$(SPHFILES):
	cp -fp $(INCL)/include/$@ .

circuit.h:      $(INCL)/include/circuit.h
device.h:       $(INCL)/include/device.h
acdefs.h:       $(INCL)/include/acdefs.h
dctdefs.h:      $(INCL)/include/dctdefs.h
distdefs.h:     $(INCL)/include/distdefs.h
noisdefs.h:     $(INCL)/include/noisdefs.h
errors.h:       $(INCL)/include/errors.h
ifcx.h:         $(INCL)/include/ifcx.h
ifdata.h:       $(INCL)/include/ifdata.h
inpline.h:      $(INCL)/include/inpline.h
input.h:        $(INCL)/include/input.h

clean distclean:
	-@rm -f $(SPHFILES)

# Make sure that the copies are the same as the source, avoid local
# updates that won't propagate!
cptest::
	@for a in $(SPHFILES); do \
	    set -- $$(/usr/bin/sum $(INCL)/include/$$a); s1=$$1; \
	    set -- $$(/usr/bin/sum $$a); s2=$$1; \
            if [ "$$s1" != "$$s2" ]; then \
	        echo ; \
	        echo  "ERROR!!! $$a differs in $(INCL)/include, FIXME"; \
	        echo ; \
	        exit 1; \
	    fi; \
	done


