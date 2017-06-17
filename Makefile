#######################################################################
# Top level Makefile for XicTools.
#######################################################################
# $Id:
#######################################################################

WRSPICE = KLU vl wrspice/bin
XIC = mrouter xic/bin

SUBDIRS = xt_base secure $(WRSPICE) $(XIC) xt_accs

CFARGS = --enable-oa=/usr/local/cad/OA-22.04 \
         --enable-psf=/home/stevew/cadence/oasis-kit/tools.lnx86

config:
	if [ ! -f xt_base/Makefile ]; then \
	    (cd xt_base; autoconf; ./configure $(CFARGS);) \
	fi

reconfig:
	rm -f xt_base/Makefile
	$(MAKE) config

all: config
	for a in $(SUBDIRS); do \
	    (cd $$a; $(MAKE)) \
	done

depend clean distclean::
	for a in $(SUBDIRS); do \
	    (cd $$a; $(MAKE) $@) \
	done
