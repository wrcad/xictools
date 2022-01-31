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

depend: $(XPMS) $(BASEFILES) $(MOZYFILES) $(SPFILES) cptest
	cd screenshots; $(MAKE)

$(XPMS): $(BITMAPS)/$@
	@cp -fp $(BITMAPS)/$@ .

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

distclean: cptest
	-@rm -f $(XPMS) $(BASEFILES) $(MOZYFILES) $(SPFILES)
	-@cd screenshots; $(MAKE) $@

# Look for forms like <a href="something>, some browsers can't handle
# a missing trailing quote.
check::
	-egrep 'href="[^>"]+>' *.hlp

# Make sure that the copies are the same as the source, avoid local
# updates that won't propagate!
cptest::
	@for a in $(XPMS); do \
	    set -- $$(/usr/bin/sum $(BITMAPS)/$$a); s1=$$1; \
	    set -- $$(/usr/bin/sum $$a); s2=$$1; \
            if [ "$$s1" != "$$s2" ]; then \
	        echo ; \
	        echo  "ERROR!!! $$a differs in $(BITMAPS), FIXME"; \
	        echo ; \
	        exit 1; \
	    fi; \
	done
	@for a in $(BASEFILES); do \
	    set -- $$(/usr/bin/sum $(BASE)/$$a); s1=$$1; \
	    set -- $$(/usr/bin/sum $$a); s2=$$1; \
            if [ "$$s1" != "$$s2" ]; then \
	        echo ; \
	        echo  "ERROR!!! $$a differs in $(BASE), FIXME"; \
	        echo ; \
	        exit 1; \
	    fi; \
	done
	@for a in $(MOZYFILES); do \
	    set -- $$(/usr/bin/sum $(MOZY)/$$a); s1=$$1; \
	    set -- $$(/usr/bin/sum $$a); s2=$$1; \
            if [ "$$s1" != "$$s2" ]; then \
	        echo ; \
	        echo  "ERROR!!! $$a differs in $(MOZY), FIXME"; \
	        echo ; \
	        exit 1; \
	    fi; \
	done
	@for a in $(SPFILES); do \
	    set -- $$(/usr/bin/sum $(SPBASE)/$$a); s1=$$1; \
	    set -- $$(/usr/bin/sum $$a); s2=$$1; \
            if [ "$$s1" != "$$s2" ]; then \
	        echo ; \
	        echo  "ERROR!!! $$a differs in $(SPBASE), FIXME"; \
	        echo ; \
	        exit 1; \
	    fi; \
	done
