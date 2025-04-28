
#######################################################################
# Makefile for public file retrieval.
# User Configurable.
#######################################################################
#######################################################################
# User configuration
#
# URL of the directory that contains the distribution files.  This MUST
# end with a directory separation character.
SRCDIR  = https://www.eng.auburn.edu/~niuguof/mextram/_downloads/5a2636005b4ee2c183c3783dc90179a2/

# Name of the distribution file to download.
SRCFILE = 505p2p0_vacode.zip
#######################################################################

# Full URL to the file to download, or empty if none.
SRCPATH = $(SRCDIR)$(SRCFILE)

ifeq ($(strip $(SRCPATH)),)
# Nothing to download, local files only.  Supply dummy targets.

fetch:

clean_fetch:

else
# Something to download has been specified.

fetch: chkSOURCE SOURCE/$(SRCFILE)

chkSOURCE:
	@if [ ! -d SOURCE ]; then \
	    mkdir SOURCE; \
	fi

SOURCE/$(SRCFILE):
	cd SOURCE; curl -L -O $(SRCPATH); cd ..; \
	if [ "$$?" == "0" ]; then \
	    set -- SOURCE/*.zip; \
	    if [ -f "$$1" ]; then \
	        unzip -d SOURCE $@; \
	    fi \
	else \
	    echo "Download failed for $(SRCFILE)"; \
	    rm SOURCE/$(SRCFILE); \
	    exit 1;\
	fi
	@if [ -d SOURCE/`basename $(SRCFILE) .zip` ]; then \
	    cp -f SOURCE/`basename $(SRCFILE) .zip`/*.va .; \
	    cp -f SOURCE/`basename $(SRCFILE) .zip`/*.inc .; \
	fi

clean_fetch::
	-@rm -f *.inc *.va *.txt *.pdf
	-@rm -rf SOURCE
endif

