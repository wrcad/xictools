
#######################################################################
# Makefile for public file retrieval.
# User Configurable.
#######################################################################
#######################################################################
# User configuration
#
# URL of the directory that contains the distribution files.  This MUST
# end with a directory separation character.
#SRCDIR  = https://github.com/ekv26/model/archive/refs/heads/
SRCDIR  = http://github.com/ekv26/model/archive/

# Name of the distribution file to download.
SRCFILE = master.tar.gz
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
	exit 0; \
	else \
	    echo "Download failed for $(SRCFILE)"; \
	    rm SOURCE/$(SRCFILE); \
	    exit 1;\
	fi
	@if [ -d SOURCE/model-master ]; then \
	    cp -f SOURCE/model-master/*.va .; \
	    set -- SOURCE/model-master/*.include; \
	    if [ -f "$$1" ]; then \
	        cp -f SOURCE/model-master/*.include .; \
	    fi; \
	else \
	    cp -f SOURCE/*.va .; \
	    set -- SOURCE/*.include; \
	    if [ -f "$$1" ]; then \
	        cp -f SOURCE/*.include .; \
	    fi \
	fi

clean_fetch::
	-@rm -f *.include *.va *.txt *.pdf
	-@rm -rf SOURCE
endif

