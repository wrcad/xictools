
#######################################################################
# Makefile for public file retrieval.
# User Configurable.
#######################################################################
#######################################################################
# User configuration
#
# URL of the directory that contains the distribution files.  This MUST
# end with a directory separation character.
# Translation error occurs in 104 and 103.8, 103.1 builds ok.
SRCDIR  = https://www.cea.fr/cea-tech/leti/pspsupport/Documents/Level%20104/
#SRCDIR = https://www.cea.fr/cea-tech/leti/pspsupport/Documents/Level%20103_8_2/
#SRCDIR = https://www.cea.fr/cea-tech/leti/pspsupport/Documents/Level%20103.3.1/

# Name of the distribution file to download.
SRCFILE = PSP104.0_vacode.tar.gz
#SRCFILE = PSP103.8.2_vacode.tar.gz
#SRCFILE = vacode_103.1.1.tar.gz
#######################################################################

# Full URL to the file to download, or empth if none.
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
	curl -L -O --output-dir ./SOURCE $(SRCPATH); \
	if [ "$$?" == "0" ]; then \
	    tar xzf $@ -C SOURCE; \
	else \
	    echo "Download failed for $(SRCFILE)"; \
	    rm SOURCE/$(SRCFILE); \
	    exit 1;\
	fi
	@if [ -d SOURCE/vacode ]; then \
	    cp -f SOURCE/vacode/*.va .; \
	    set -- SOURCE/vacode/*.include; \
	    if [ -f "$$1" ]; then \
	        cp -f SOURCE/vacode/*.include .; \
	    fi; \
	fi

clean_fetch::
	-@rm -f *.include *.va *.txt *.pdf
	-@rm -rf SOURCE
endif

