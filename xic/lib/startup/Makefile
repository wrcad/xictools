# lib/xic/startup/Makefile
# $Id: Makefile,v 1.2 2015/09/16 00:13:22 stevew Exp $

FILES = wr_install

dummy: $(FILES)

$(FILES): ../../../xt_base/packages/util/$@
	cp -f ../../../xt_base/packages/util/$@ .

clean:

distclean:
	rm -f $(FILES)
