# wrspice/lib/help/Makefile

BASEFILES = FileSel.hlp Hardcopy.hlp TMsmall.gif
MOZYFILES = HelpSys.hlp TextEditor.hlp

SPFILES = \
  spAnalysis.hlp spElements.hlp spExpressions.hlp spInput.hlp spModels.hlp \
  spSemicond.hlp spSources.hlp

BASE = ../../xt_base/help
SPBASE = ../../xt_base/sphelp
MOZY = ../../mozy/help

depend: $(BASEFILES) $(MOZYFILES) $(SPFILES)
	cd screenshots; $(MAKE)

$(BASEFILES):
	cp -fp $(BASE)/$@ .

$(MOZYFILES):
	cp -fp $(MOZY)/$@ .

$(SPFILES):
	cp -fp $(SPBASE)/$@ .

FileSel.hlp:    $(BASE)/FileSel.hlp
Hardcopy.hlp:   $(BASE)/Hardcopy.hlp
TMsmall.gif:    $(BASE)/TMsmall.gif
HelpSys.hlp:    $(MOZY)/HelpSys.hlp
TextEditor.hlp: $(MOZY)/TextEditor.hlp

spAnalysis.hlp: $(SPBASE)/spAnalysis.hlp
spElements.hlp: $(SPBASE)/spElements.hlp
spExpressions.hlp: $(SPBASE)/spExpressions.hlp
spInput.hlp:    $(SPBASE)/spInput.hlp
spModels.hlp:   $(SPBASE)/spModels.hlp
spSemicond.hlp: $(SPBASE)/spSemicond.hlp
spSources.hlp:  $(SPBASE)/spSources.hlp

clean:

distclean:
	-@rm -f $(BASEFILES) $(MOZYFILES) $(SPFILES)
	-@cd screenshots; $(MAKE) distclean

# Look for forms like <a href="something>, some browsers can't handle
# a missing trailing quote.
check::
	-egrep 'href="[^>"]+>' *.hlp

# Make sure that the copies are the same as the source, avoid local
# updates that won't propagate!
cptest::
	@for a in $(BASEFILES); do \
	    s1="`/usr/bin/sum $(BASE)/$$a`"; \
	    s2="`/usr/bin/sum $$a`"; \
            if [ "$$s1" != "$$s2" ]; then \
	        echo ; \
	        echo  "ERROR!!! $$a differs in $(BASE), FIXME"; \
	        echo ; \
	        exit 1; \
	    fi; \
	done
	@for a in $(MOZYFILES); do \
	    s1="`/usr/bin/sum $(MOZY)/$$a`"; \
	    s2="`/usr/bin/sum $$a`"; \
            if [ "$$s1" != "$$s2" ]; then \
	        echo ; \
	        echo  "ERROR!!! $$a differs in $(MOZY), FIXME"; \
	        echo ; \
	        exit 1; \
	    fi; \
	done
	@for a in $(SPFILES); do \
	    s1="`/usr/bin/sum $(SPBASE)/$$a`"; \
	    s2="`/usr/bin/sum $$a`"; \
            if [ "$$s1" != "$$s2" ]; then \
	        echo ; \
	        echo  "ERROR!!! $$a differs in $(SPBASE), FIXME"; \
	        echo ; \
	        exit 1; \
	    fi; \
	done

