# wrspice/lib/help/Makefile
# $Id: Makefile,v 1.6 2011/08/23 03:50:15 stevew Exp $

BASEFILES = FileSel.hlp Hardcopy.hlp HelpSys.hlp TextEditor.hlp TMsmall.gif

SPFILES = \
  spAnalysis.hlp spElements.hlp spExpressions.hlp spInput.hlp spModels.hlp \
  spSemicond.hlp spSources.hlp

BASE = ../../../xt_base/lib/help
SPBASE = ../../../xt_base/lib/sphelp

dummy: $(BASEFILES) $(SPFILES)
	cd screenshots; $(MAKE)

$(BASEFILES): $(BASE)/$@
	cp -f $(BASE)/$@ .

$(SPFILES): $(SPBASE)/$@
	cp -f $(SPBASE)/$@ .

clean:

distclean:
	rm -f $(BASEFILES) $(SPFILES)
	cd screenshots; $(MAKE) distclean

# Look for forms like <a href="something>, some browsers can't handle
# a missing trailing quote.
check::
	-egrep 'href="[^>"]+>' *.hlp

