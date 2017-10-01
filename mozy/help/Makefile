#######################################################################
LOCATION = mozy/lib/help
#######################################################################

BASEFILES = \
  FileSel.hlp Hardcopy.hlp HelpSys.hlp TextEditor.hlp accessories.hlp \
  TMsmall.gif wrbannermain.gif

SSFILES = \
  filesel.png print.png help.png texted.png email.png

BASEDIR = ../../xt_base/help

dummy: $(BASEFILES) $(SSFILES)

$(BASEFILES): $(BASEDIR)/$@
	cp -fp $(BASEDIR)/$@ .

$(SSFILES): $(BASEDIR)/screenshots/$@
	if [ ! -d screenshots ]; then \
	    mkdir screenshots; \
	fi;
	cp -fp $(BASEDIR)/screenshots/$@ screenshots

clean::

distclean:
	-@rm -rf $(BASEFILES) screenshots

