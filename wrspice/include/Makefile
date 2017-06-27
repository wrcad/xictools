#######################################################################
# wrspice/include/Makefile
#######################################################################
# $Id:$
#######################################################################

LPATH = ../../xt_base
FILES = spmatrix.h hash.h paramsub.h spnumber.h spparse.h

depend: $(FILES)

spmatrix.h: $(LPATH)/sparse/spmatrix.h
	cp -fp $(LPATH)/sparse/spmatrix.h .

spparse.h: $(LPATH)/spnumber/spparse.h
	cp -fp $(LPATH)/spnumber/spparse.h .

hash.h: $(LPATH)/spnumber/hash.h
	cp -fp $(LPATH)/spnumber/hash.h .

paramsub.h: $(LPATH)/spnumber/paramsub.h
	cp -fp $(LPATH)/spnumber/paramsub.h .

spnumber.h: $(LPATH)/spnumber/spnumber.h
	cp -fp $(LPATH)/spnumber/spnumber.h .

clean:
	rm -f $(FILES)

distclean:
	rm -f $(FILES)

