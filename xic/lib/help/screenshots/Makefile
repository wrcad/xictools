# xic/lib/help/screenshots/Makefile

SSFILES = \
  filesel.png print.png help.png texted.png email.png

SSHOTS = ../../../../xt_base/lib/help/screenshots

dummy: $(SSFILES)

$(SSFILES):
	cp -fp $(SSHOTS)/$@ .

filesel.png:    $(SSHOTS)/filesel.png
print.png:      $(SSHOTS)/print.png
help.png:       $(SSHOTS)/help.png
texted.png:     $(SSHOTS)/texted.png
email.png:      $(SSHOTS)/email.png

distclean:
	-@rm -f $(SSFILES)
