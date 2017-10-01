# wrspice/lib/help/Makefile

BASEFILES = FileSel.hlp Hardcopy.hlp HelpSys.hlp TextEditor.hlp TMsmall.gif

SPFILES = \
  spAnalysis.hlp spElements.hlp spExpressions.hlp spInput.hlp spModels.hlp \
  spSemicond.hlp spSources.hlp

BASE = ../../xt_base/help
SPBASE = ../../xt_base/sphelp

depend: $(BASEFILES) $(SPFILES)
	cd screenshots; $(MAKE)

$(BASEFILES):
	cp -fp $(BASE)/$@ .

$(SPFILES):
	cp -fp $(SPBASE)/$@ .

FileSel.hlp:    $(BASE)/FileSel.hlp
Hardcopy.hlp:   $(BASE)/Hardcopy.hlp
HelpSys.hlp:    $(BASE)/HelpSys.hlp
TextEditor.hlp: $(BASE)/TextEditor.hlp
TMsmall.gif:    $(BASE)/TMsmall.gif

spAnalysis.hlp: $(SPBASE)/spAnalysis.hlp
spElements.hlp: $(SPBASE)/spElements.hlp
spExpressions.hlp: $(SPBASE)/spExpressions.hlp
spInput.hlp:    $(SPBASE)/spInput.hlp
spModels.hlp:   $(SPBASE)/spModels.hlp
spSemicond.hlp: $(SPBASE)/spSemicond.hlp
spSources.hlp:  $(SPBASE)/spSources.hlp

clean:

distclean:
	-@rm -f $(BASEFILES) $(SPFILES)
	-@cd screenshots; $(MAKE) distclean

# Look for forms like <a href="something>, some browsers can't handle
# a missing trailing quote.
check::
	-egrep 'href="[^>"]+>' *.hlp

