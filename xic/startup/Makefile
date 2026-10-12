#######################################################################
LOCATION = xic/startup
#######################################################################

BASE = ../../xt_base

depend:: wr_install
	@echo depending in $(LOCATION)

wr_install: $(BASE)/packages/util/wr_install
	cp -f $(BASE)/packages/util/wr_install .

clean distclean:
	-@rm -f wr_install 

