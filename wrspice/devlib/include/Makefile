#######################################################################
# wrspice/devlib/include
#######################################################################

HFILES = gencurrent.h
SPHFILES = circuit.h device.h acdefs.h dctdefs.h distdefs.h noisdefs.h \
    errors.h ifdata.h inpline.h input.h

depend: $(SPHFILES)

$(SPHFILES):
	cp -fp ../../include/$@ .

circuit.h:      ../../include/circuit.h
device.h:       ../../include/device.h
acdefs.h:       ../../include/acdefs.h
dctdefs.h:      ../../include/dctdefs.h
distdefs.h:     ../../include/distdefs.h
noisdefs.h:     ../../include/noisdefs.h
errors.h:       ../../include/errors.h
ifdata.h:       ../../include/ifdata.h
inpline.h:      ../../include/inpline.h
input.h:        ../../include/input.h

clean distclean::
	-@rm -f $(SPHFILES)

# Make sure that the copies are the same as the source, avoid local
# updates that won't propagate!
cptest::
	@for a in $(SPHFILES); do \
	    s1="`/usr/bin/sum ../../include/$$a`"; \
	    s2="`/usr/bin/sum $$a`"; \
            if [ "$$s1" != "$$s2" ]; then \
	        echo ; \
	        echo  "ERROR!!! $$a differs in ../../include, FIXME"; \
	        echo ; \
	        exit 1; \
	    fi; \
	done

