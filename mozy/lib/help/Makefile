# lib/mozy/help/Makefile
# $Id: Makefile,v 1.3 2014/07/20 22:25:16 stevew Exp $

BASEFILES = \
  FileSel.hlp Hardcopy.hlp HelpSys.hlp TextEditor.hlp accessories.hlp \
  TMsmall.gif wrbannermain.gif

SSFILES = \
  filesel.png print.png help.png texted.png email.png

BASE = ../../../../xt_base/lib/help

dummy: $(BASEFILES) $(SSFILES)

$(BASEFILES): $(BASE)/$@
	cp -fp $(BASE)/$@ .

$(SSFILES): $(BASE)/screenshots/$@
	if [ ! -d screenshots ]; then \
	    mkdir screenshots; \
	fi;
	cp -fp $(BASE)/screenshots/$@ screenshots

clean::

distclean:
	rm -rf $(BASEFILES) screenshots
