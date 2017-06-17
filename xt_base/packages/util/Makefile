#######################################################################
# packages/util/Makefile
#######################################################################
# $Id: Makefile,v 1.11 2015/02/23 17:08:52 stevew Exp $
#######################################################################

FILES = fetch_action fetchdist fetchnew wr_filename wr_install Makefile

install::
	cp -fp wr_install /usr1/stevew/export/xictools

clean::

distclean::

utod.exe: utod.c
	gcc -O utod.c -o utod.exe

# This creates the adms_examples used by the packagers.  This must be
# kept in sync with the examples tree.
#
# Do make distclean in devlib/adms/examples first!
#
# Be sure to edit the list in case any crap files sneak in.  The file
# is kept in CVS to ensure consistency.

adms_examples::
	-@aex=`pwd`/adms_examples; \
	cd ../../devlib/adms; find examples -type f | grep -v CVS | sed -e s/Makefile\\.in/Makefile/ | sort > $$aex
