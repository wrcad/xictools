# $Id: Makefile,v 1.2 2016/01/18 18:52:12 stevew Exp $

CXX = g++
CFLAGS = -O -g

dummy: keyform licform mklic accform mkacc

keyform: keyform.o backend.o
	$(CXX) $(CFLAGS) -o keyform keyform.o backend.o
	chmod 4755 keyform

licform: licform.o backend.o
	$(CXX) $(CFLAGS) -o licform licform.o backend.o
	chmod 4755 licform

accform: accform.o backend.o
	$(CXX) $(CFLAGS) -o accform accform.o backend.o
	chmod 4755 accform

mklic: mklic.cc
	$(CXX) $(CFLAGS) -o mklic mklic.cc

mkacc: mkacc.cc
	$(CXX) $(CFLAGS) -o mkacc mkacc.cc

.cc.o:
	$(CXX) $(CFLAGS) -c $*.cc

clean:
	rm -f *.o keyform licform mklic
