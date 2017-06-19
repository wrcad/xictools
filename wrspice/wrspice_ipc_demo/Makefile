# Generated automatically from Makefile.in by configure.
#######################################################################
# wrspice_ipc_demo/Makefile.in
#######################################################################
# $Id: Makefile.in,v 1.6 2014/10/05 03:17:48 stevew Exp $
#######################################################################

DATE = Sun Jun 18 13:38:56 PDT 2017
SPICE_VERSION = 4.2.17
DEVLIB_VERSION = 4.2.17

prefix = /usr/local
CXX = /usr/local/win-builds/bin/g++
CC = /usr/local/win-builds/bin/gcc
LINKCC = /usr/local/win-builds/bin/g++ -static-libgcc -static-libstdc++
CFLAGS = -O -g -Wall -W -Wshadow -Wwrite-strings -pthread    -DWRSPICE
LFLAGS =   
PIC_OPT = 
#LSHFLAG = -shared -Wl,--enable-auto-import,--export-all-symbols
LIBS = $(APPLIBS)  -lm -lgdi32 -lwinspool -lcomdlg32 -lole32 -loleaut32  -luuid -lodbc32 -lversion -lcomctl32 -lwsock32 -lws2_32 -liphlpapi -lbfd -liberty -lintl -liconv -lm -ltiff -ljpeg -lpng -lz  -Wl,-luuid -LC:/usr/local/gtk2-bundle/lib -lgtk-win32-2.0 -lgdk-win32-2.0 -limm32 -lshell32 -lole32 -latk-1.0 -lpangocairo-1.0 -lgio-2.0 -lgdk_pixbuf-2.0 -lpangowin32-1.0 -lgdi32 -lpango-1.0 -lm -lcairo -lgobject-2.0 -lglib-2.0 -lintl   
STDCLIB = 
SLIBS = -lgdi32 -lwinspool -lcomdlg32 -lole32 -loleaut32  -luuid -lodbc32 -lversion -lcomctl32 -lwsock32 -lws2_32 -liphlpapi
OSNAME = Win32
ARCH = i386
DIST_SUFFIX = exe
GRPREF = gtk
DEPEND_PROG = $(CC) -MM -O -g -Wall -W -Wshadow -Wwrite-strings -pthread   -DWRSPICE
INSTALL = $(LPATH)/util/install-sh
INSTALL_USER = @INSTALL_USER@
NTSUFFIX = .exe
FILTER = |sed -e s/C:// -e s///
MALLOC = 
SECURE = @SECURE@
NTREGEX = $(LPATH)/miscutil/libregex/libregex.a
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
