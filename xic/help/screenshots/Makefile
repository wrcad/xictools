# xic/lib/help/screenshots/Makefile

BASESSFILES = \
  filesel.png print.png

MOZYSSFILES = \
  help.png texted.png email.png

BASESSHOTS = ../../../xt_base/help/screenshots
MOZYSSHOTS = ../../../mozy/help/screenshots

depend: $(BASESSFILES) $(MOZYSSFILES)

$(BASESSFILES):
	cp -fp $(BASESSHOTS)/$@ .

$(MOZYSSFILES):
	cp -fp $(MOZYSSHOTS)/$@ .

filesel.png:    $(BASESSHOTS)/filesel.png
print.png:      $(BASESSHOTS)/print.png
help.png:       $(MOZYSSHOTS)/help.png
texted.png:     $(MOZYSSHOTS)/texted.png
email.png:      $(MOZYSSHOTS)/email.png

distclean:
	-@rm -f $(BASESSFILES) $(MOZYSSFILES)
