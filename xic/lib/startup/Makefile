# lib/xic/startup/Makefile
# $Id: Makefile,v 1.2 2015/09/16 00:13:22 stevew Exp $

FILES = wr_install
LPATH = ../../../xt_base

dummy: $(FILES)

$(FILES):
	cp -f $(LPATH)/packages/util/$@ .

wr_install:     $(LPATH)/packages/util/wr_install

clean:
	rm -f $(FILES)

distclean:
	rm -f $(FILES)
