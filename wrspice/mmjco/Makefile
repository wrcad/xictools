# Makefile for build with local cmpfig-1.4.

mmjco: ./cmpfit-1.4/libmpfit.a libmmjco.a main.cc
	g++ -g -O -DWRSPICE -o mmjco main.cc -L. -lmmjco -lgsl -lgslcblas \
 ../../xt_base/lib/miscutil.a -L./cmpfit-1.4 -lmpfit -lm

libmmjco.a: mmjco.o mmjco_cmds.o
	ar -cr libmmjco.a mmjco.o mmjco_cmds.o
	ranlib libmmjco.a

mmjco.o: mmjco.cc mmjco.h
	g++ -g -O -I./cmpfit-1.4 -DWRSPICE -c mmjco.cc

mmjco_cmds.o: mmjco_cmds.cc mmjco_cmds.h mmjco.h
	g++ -g -O -I./cmpfit-1.4 -DWRSPICE -c mmjco_cmds.cc

./cmpfit-1.4/libmpfit.a:
	cd cmpfit-1.4; $(MAKE)

clean::
	-@cd cmpfit-1.4; $(MAKE) clean
	-@rm -f libmmjco.a *.o mmjco tca*
