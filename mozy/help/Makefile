#######################################################################
LOCATION = mozy/lib/help
#######################################################################

MOZYFILES = \
  HelpSys.hlp TextEditor.hlp Accessories.hlp

BASEFILES = \
  FileSel.hlp Hardcopy.hlp TMsmall.gif wrbannermain.gif

MOZYSSFILES = \
  help.png texted.png email.png

BASESSFILES = \
  filesel.png print.png

BASEDIR = ../../xt_base/help

dummy: $(BASEFILES) $(BASESSFILES)

$(BASEFILES): $(BASEDIR)/$@
	cp -fp $(BASEDIR)/$@ .

$(BASESSFILES): $(BASEDIR)/screenshots/$@
	cp -fp $(BASEDIR)/screenshots/$@ screenshots

clean::

distclean:
	-@rm -f $(BASEFILES)
	-@(cd  screenshots; rm -f $(BASESSFILES))

