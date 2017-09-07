#######################################################################
# wrspice/devlib/adms/include
#######################################################################

SPHFILES = circuit.h device.h acdefs.h dctdefs.h distdefs.h noisdefs.h \
 errors.h ifdata.h inpline.h input.h
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
ifdata.h:       $(INCL)/include/ifdata.h
inpline.h:      $(INCL)/include/inpline.h
input.h:        $(INCL)/include/input.h

clean distclean:
	-@rm -f $(SPHFILES)


