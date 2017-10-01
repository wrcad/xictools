# wrspice/lib/startup/Makefile

BASE = ../../xt_base

depend:: wr_install

wr_install: $(BASE)/packages/util/wr_install
	cp -f $(BASE)/packages/util/wr_install .

clean distclean:
	-@rm -f wr_install klu_wr.$(SOEXT)

