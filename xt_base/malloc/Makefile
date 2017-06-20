# Generated automatically from Makefile.in by configure.
# Makefile.in for malloc
# $Id: Makefile.in,v 1.12 2013/12/05 19:37:42 stevew Exp $

# To enable the monitor, add to CFLAGS: -DENABLE_MONITOR
# If monitor is enabled, to monitor new/delete only: -DCPP_ONLY

MALLOCFILE = malloc-2.8.6.c
CXX = /usr/local/win-builds/bin/g++
CC = /usr/local/win-builds/bin/gcc
CFLAGS = -O -g -Wall -W -Wshadow -Wwrite-strings -pthread 
DEPEND_PROG = $(CC) -MM -O -g -Wall -W -Wshadow -Wwrite-strings -pthread -DMALLOCFILE=\"$(MALLOCFILE)\"
AR = /usr/local/win-builds/bin/ar
RANLIB = /usr/local/win-builds/bin/ranlib
FILTER = |sed -e s/C:// -e s///

CCFILES = local_malloc.cc monitor.cc
INCLUDE = -I../miscutil

LIB_TARGET = malloc.a

$(LIB_TARGET): local_malloc.o monitor.o malloc.o
	@if [ -f $(LIB_TARGET) ]; then \
	    rm -f $(LIB_TARGET); \
	fi
	$(AR) cr $(LIB_TARGET) local_malloc.o monitor.o malloc.o
	$(RANLIB) $(LIB_TARGET)

local_malloc.o: local_malloc.cc Makefile
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o local_malloc.o local_malloc.cc

monitor.o: monitor.cc Makefile
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o monitor.o monitor.cc

malloc.o: malloc.c $(MALLOCFILE) Makefile
	$(CC) $(CFLAGS) $(INCLUDE) -DMALLOCFILE=\"$(MALLOCFILE)\" -c \
 -o malloc.o malloc.c

clean distclean:
	-@rm *.o *.a

mtest: test.cc malloc.a
	$(CXX) $(CFLAGS) $(INCLUDE) -o mtest test.cc malloc.a

depend:
	@echo depending in malloc
	@if [ x$(DEPEND_DONE) = x ]; then \
	    echo DEPEND_DONE = 1 >> Makefile; \
	    $(DEPEND_PROG) $(CCFILES) $(INCLUDE) malloc.c $(FILTER) \
 >> Makefile; \
	fi

####################################################################
DEPEND_DONE = 1
local_malloc.o: local_malloc.cc local_malloc.h
monitor.o: monitor.cc local_malloc.h
malloc.o: malloc.c
