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
	rm -f utod.exe

utod.exe: utod.c
	gcc -O utod.c -o utod.exe

