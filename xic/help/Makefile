# xic/lib/help/Makefile

XPMS = \
  arc.xpm box.xpm break.xpm deck.xpm desel.xpm devs.xpm donut.xpm \
  erase.xpm iplot.xpm label.xpm layer.xpm logo.xpm lpal.xpm lspec.xpm \
  ltpal.xpm ltsty.xpm ltvis.xpm nodmp.xpm place.xpm plot.xpm polyg.xpm \
  put.xpm rdraw.xpm round.xpm run.xpm selcp.xpm setcl.xpm shapes.xpm \
  sides.xpm spcmd.xpm spin.xpm style.xpm subct.xpm symbl.xpm terms.xpm \
  wire.xpm wr.xpm xform.xpm xor.xpm

BASEFILES = \
  FileSel.hlp Hardcopy.hlp TMsmall.gif wrbannermain.gif

MOZYFILES = \
  HelpSys.hlp TextEditor.hlp accessories.hlp

SPFILES = \
  spAnalysis.hlp spElements.hlp spExpressions.hlp spInput.hlp \
  spModels.hlp spSemicond.hlp spSources.hlp

BITMAPS = ../include/bitmaps
BASE = ../../xt_base/help
SPBASE = ../../xt_base/sphelp
MOZY = ../../mozy/help

depend: $(XPMS) $(BASEFILES) $(MOZYFILES) $(SPFILES)
	cd screenshots; $(MAKE)

$(XPMS): $(BITMAPS)/$@
	cp -fp $(BITMAPS)/$@ .

$(BASEFILES):
	cp -fp $(BASE)/$@ .

$(MOZYFILES):
	cp -fp $(MOZY)/$@ .

$(SPFILES):
	cp -fp $(SPBASE)/$@ .

FileSel.hlp:    $(BASE)/FileSel.hlp
Hardcopy.hlp:   $(BASE)/Hardcopy.hlp
TMsmall.gif:    $(BASE)/TMsmall.gif
wrbannermain.gif: $(BASE)/wrbannermain.gif
HelpSys.hlp:    $(MOZY)/HelpSys.hlp
TextEditor.hlp: $(MOZY)/TextEditor.hlp
accessories.hlp: $(MOZY)/accessories.hlp

spAnalysis.hlp: $(SPBASE)/spAnalysis.hlp
spElements.hlp: $(SPBASE)/spElements.hlp
spExpressions.hlp: $(SPBASE)/spExpressions.hlp
spInput.hlp:    $(SPBASE)/spInput.hlp
spModels.hlp:   $(SPBASE)/spModels.hlp
spSemicond.hlp: $(SPBASE)/spSemicond.hlp
spSources.hlp:  $(SPBASE)/spSources.hlp

clean:

distclean:
	-@rm -f $(XPMS) $(BASEFILES) $(MOZYFILES) $(SPFILES)
	-@cd screenshots; $(MAKE) $@

# Look for forms like <a href="something>, some browsers can't handle
# a missing trailing quote.
check::
	-egrep 'href="[^>"]+>' *.hlp

