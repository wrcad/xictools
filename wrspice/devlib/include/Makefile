#######################################################################
# devlib/include/Makefile
#######################################################################
# $Id:$
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
	rm -f $(SPHFILES)

