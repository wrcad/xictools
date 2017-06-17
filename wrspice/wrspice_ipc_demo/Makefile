#######################################################################
# wrspice_ipc_demo/Makefile.in
#######################################################################
# $Id: Makefile.in,v 1.6 2014/10/05 03:17:48 stevew Exp $
#######################################################################

DATE = Thu Jun 15 22:08:57 PDT 2017
SPICE_VERSION = 4.2.17
DEVLIB_VERSION = 4.2.17

prefix = /usr/local
CXX = g++
CC = gcc
LINKCC = g++
CFLAGS = -O -g -Wall -W -Wshadow -Wwrite-strings -pthread -DDYNAMIC_LIBS   -DWRSPICE
LFLAGS = -rdynamic  
PIC_OPT = -fPIC
#LSHFLAG = -shared
LIBS = $(APPLIBS) -lncurses -lm   -lpng -ljpeg -ltiff -lz -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfontconfig -lgobject-2.0 -lglib-2.0 -lfreetype   
STDCLIB = 
SLIBS = 
OSNAME = LinuxRHEL7_64
ARCH = x86_64
DIST_SUFFIX = rpm
GRPREF = gtk
DEPEND_PROG = $(CC) -MM -O -g -Wall -W -Wshadow -Wwrite-strings -pthread -DDYNAMIC_LIBS  -DWRSPICE
INSTALL = $(LPATH)/util/install-sh
INSTALL_USER = @INSTALL_USER@
NTSUFFIX = 
FILTER = 
MALLOC = $(LPATH)/malloc/malloc.a
SECURE = @SECURE@
NTREGEX = 
NTDBGOBJ = @NTDBGOBJ@
#DEVLIB_SHARED = libwrsdev.so.2.0
DEVLIB_STATIC = libwrsdev.a
DEVLIB_CALL = ../devlib/$(DEVLIB_STATIC)
TOOLROOT = xictools

GFX_PROG = xic
GFX_EXEC_DIR = $(prefix)/$(TOOLROOT)/bin
SPICE_PROG = wrspice
BUG_ADDR = wrspice@wrcad.com
ANALYSES = op dc tf ac tran pz disto noise sense
SPICE_HOST =
SPICED_LOG = /tmp/$(SPICE_PROG)d.log
DEFAULT_EDITOR = xeditor
ASCII_RAWFILE = 1
OPT_CHAR = -
SPICE_NOTICE =

LPATH = .
MISCUTIL = $(LPATH)/miscutil/miscutil.a

CCFILES = wrspiced.cc spclient.cc sced_spiceipc.cc
INCLUDE = -I. -I$(LPATH)/ginterf -I$(LPATH)/miscutil

all: spclient wrspiced

spclient: spclient.o sced_spiceipc.o $(MISCUTIL)
	$(CXX) -o spclient $(CFLAGS) $(INCLUDE) spclient.o sced_spiceipc.o \
        $(MISCUTIL)

wrspiced: wrspiced.cc $(MISCUTIL)
	$(CXX) -o wrspiced $(CFLAGS) $(INCLUDE) wrspiced.cc $(MISCUTIL)

sced_spiceipc.o:
	$(CXX) $(CFLAGS) -DDEMO_EXPORT $(INCLUDE) -c $*.cc

.cc.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -c $*.cc

$(MISCUTIL)::
	cd $(@D); $(MAKE)

clean::
	cd $(LPATH)/miscutil; $(MAKE) clean
	rm -f spclient wrspiced

distclean::
	cd $(LPATH)/miscutil; $(MAKE) distclean
	rm -f spclient wrspiced Makefile config.*

depend:
	cd $(LPATH)/miscutil; $(MAKE) depend
	@echo depending in wrspice_ipc_demo
	@if [ x$(DEPEND_DONE) = x ]; then \
	    echo DEPEND_DONE = 1 >> Makefile; \
	    $(DEPEND_PROG) $(INCLUDE) $(CCFILES) -DDEMO_EXPORT $(FILTER) \
 >> Makefile; \
	fi

#######################################################################
