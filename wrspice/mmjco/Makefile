# Makefile for build with local cmpfig-1.4.

ifeq ($(shell uname),Darwin)
	CFLAGS = -O -g -Wall -W -Wshadow -Wwrite-strings \
  -mmacosx-version-min=11.0 -arch x86_64 -isysroot \
  /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk \
  -I/usr/local/gtk2-bundle-x11/include -DWRSPICE
	GSL = -L/usr/local/gtk2-bundle-x11/lib
else
	CFLAGS = -O -g -Wall -W -Wshadow -Wwrite-strings
endif

mmjco: ./cmpfit-1.4/libmpfit.a libmmjco.a main.cc
	g++ $(CFLAGS) -o mmjco main.cc -L. -lmmjco $(GSL) \
 -lgsl -lgslcblas ../../xt_base/lib/miscutil.a -L./cmpfit-1.4 -lmpfit -lm

libmmjco.a: mmjco.o mmjco_cmds.o
	ar -cr libmmjco.a mmjco.o mmjco_cmds.o
	ranlib libmmjco.a

mmjco.o: mmjco.cc mmjco.h
	g++ $(CFLAGS) -I./cmpfit-1.4 -c mmjco.cc

mmjco_cmds.o: mmjco_cmds.cc mmjco_cmds.h mmjco.h
	g++ $(CFLAGS) -I./cmpfit-1.4 -c mmjco_cmds.cc

./cmpfit-1.4/libmpfit.a:
	cd cmpfit-1.4; $(MAKE)

clean::
	-@cd cmpfit-1.4; $(MAKE) clean
	-@rm -f libmmjco.a *.o mmjco tca*
