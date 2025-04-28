
#######################################################################
# Makefile for public file retrieval.
# User Configurable.
#######################################################################
#######################################################################
# User configuration
#
# URL of the directory that contains the distribution files.  This MUST
# end with a directory separation character.
SRCDIR  = 

# Name of the distribution file to download.
SRCFILE = 
#######################################################################

# Full URL to the file to download, or empty if none.
SRCPATH = $(SRCDIR)$(SRCFILE)

ifeq ($(strip $(SRCPATH)),)
# Nothing to download, local files only.  Supply dummy targets.

fetch:

clean_fetch:

else
# Something to download has been specified.

fetch: SOURCE/$(SRCFILE)

SOURCE/$(SRCFILE):
	cd SOURCE; curl -L -O $(SRCPATH); cd ..; \
	if [ "$$?" == "0" ]; then \
	    tar xzf $@ -C SOURCE; \
	else \
	    echo "Download failed for $(SRCFILE)"; \
	    rm SOURCE/$(SRCFILE); \
	    exit 1;\
	fi
	@if [ -d SOURCE/code ]; then \
	    cp -f SOURCE/code/*.va .; \
	    set -- SOURCE/code/*.include; \
	    if [ -f "$$1" ]; then \
	        cp -f SOURCE/code/*.include .; \
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
	-@rm -rf SOURCE/*
endif

