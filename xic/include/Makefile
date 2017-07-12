#######################################################################
# xic/include/Makefile
#######################################################################
# $Id:$
#######################################################################

LPATH = ../../xt_base
FILES = spmatrix.h hash.h paramsub.h spnumber.h spparse.h

depend: $(FILES) reltag

spmatrix.h: $(LPATH)/sparse/spmatrix.h
	cp -f $(LPATH)/sparse/spmatrix.h .

hash.h: $(LPATH)/spnumber/hash.h
	cp -f $(LPATH)/spnumber/hash.h .

paramsub.h: $(LPATH)/spnumber/paramsub.h
	cp -f $(LPATH)/spnumber/paramsub.h .

spnumber.h: $(LPATH)/spnumber/spnumber.h
	cp -f $(LPATH)/spnumber/spnumber.h .

spparse.h: $(LPATH)/spnumber/spparse.h
	cp -f $(LPATH)/spnumber/spparse.h .

clean:

distclean:
	rm -f $(FILES) Makefile

# Create/update the reltag.h file.
reltag:
	@version=`../version xic`; \
	if [ -z "$$version" ]; then \
	    echo "Can't find version!"; \
	    exit; \
	fi; \
	tifs=$$IFS; \
	IFS="."; \
	set -- $$version; \
	f1=$$1; \
	f2=$$2; \
	IFS=".-"; \
	set -- $$3; \
	IFS=$$tifs; \
	f3=$$1; \
	rev=$$2; \
	if [ -z "$$rev" ]; then \
	    tag="xic-$$f1-$$f2-$$f3"; \
	    num="$$f1$$f2`printf %02d $$f3`0"; \
	else \
	    tag="xic-$$f1-$$f2-$$f3-$$rev"; \
	    num="$$f1$$f2`printf %02d $$f3`$$rev"; \
	fi; \
	echo "#define XIC_RELEASE_TAG \"$$tag\"" > /tmp/reltag.h; \
	echo "#define XIC_RELEASE_NUM $$num" >> /tmp/reltag.h; \
	diff -q /tmp/reltag.h ./reltag.h > /dev/null 2>&1; \
	if [ $$? != 0 ]; then \
	    cp -f /tmp/reltag.h .; \
	fi; \
	unlink /tmp/reltag.h

#######################################################################
