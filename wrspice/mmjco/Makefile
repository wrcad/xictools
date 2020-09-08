# Makefile for build with local cmpfig-1.4.

mmjco: ./cmpfit-1.4/libmpfit.a libmmjco.a main.cc
	g++ -g -O -o mmjco main.cc -L. -lmmjco -lgsl -lgslcblas -L./cmpfit-1.4 \
 -lmpfit -lm

libmmjco.a: mmjco.cc mmjco.h
	g++ -g -O -I./cmpfit-1.4 -c mmjco.cc
	ar -cr libmmjco.a mmjco.o
	ranlib libmmjco.a

./cmpfit-1.4/libmpfit.a:
	cd cmpfit-1.4; $(MAKE)

clean::
	-@cd cmpfit-1.4; $(MAKE) clean
	-@rm -f libmmjco.a *.o mmjco tca*.fit tca*.data tca*.model
