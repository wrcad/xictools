# wrspice/lib/help/screenshots/Makefile
# $Id:$

SSFILES = \
  filesel.png print.png help.png texted.png email.png

SSHOTS = ../../../../xt_base/lib/help/screenshots

dummy: $(SSFILES)

$(SSFILES): $(SSHOTS)/$@
	cp -fp $(SSHOTS)/$@ .

distclean:
	rm -f $(SSFILES)
