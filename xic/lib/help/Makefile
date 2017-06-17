# xic/lib/help/Makefile
# $Id: Makefile,v 1.9 2015/02/07 06:02:35 stevew Exp $

XPMS = \
  arc.xpm box.xpm break.xpm deck.xpm desel.xpm devs.xpm donut.xpm \
  erase.xpm iplot.xpm label.xpm layer.xpm logo.xpm lpal.xpm lspec.xpm \
  ltpal.xpm ltsty.xpm ltvis.xpm nodmp.xpm place.xpm plot.xpm polyg.xpm \
  put.xpm rdraw.xpm round.xpm run.xpm selcp.xpm setcl.xpm shapes.xpm \
  sides.xpm spcmd.xpm spin.xpm style.xpm subct.xpm symbl.xpm terms.xpm \
  wire.xpm wr.xpm xform.xpm xor.xpm

BASEFILES = \
  FileSel.hlp Hardcopy.hlp HelpSys.hlp TextEditor.hlp accessories.hlp \
  TMsmall.gif wrbannermain.gif

SPFILES = \
  spAnalysis.hlp spElements.hlp spExpressions.hlp spInput.hlp \
  spModels.hlp spSemicond.hlp spSources.hlp

BITMAPS = ../../include/bitmaps
BASE = ../../../xt_base/lib/help
SPBASE = ../../../xt_base/lib/sphelp

dummy: $(XPMS) $(BASEFILES) $(SPFILES)
	cd screenshots; $(MAKE)

$(XPMS): $(BITMAPS)/$@
	cp -f $(BITMAPS)/$@ .

$(BASEFILES): $(BASE)/$@
	cp -f $(BASE)/$@ .

$(SPFILES): $(SPBASE)/$@
	cp -f $(SPBASE)/$@ .

clean:

distclean:
	rm -f $(XPMS) $(BASEFILES) $(SPFILES)
	cd screenshots; $(MAKE) distclean

# Look for forms like <a href="something>, some browsers can't handle
# a missing trailing quote.
check::
	-egrep 'href="[^>"]+>' *.hlp

