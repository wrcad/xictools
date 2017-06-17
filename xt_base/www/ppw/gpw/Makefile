# Makefile for password generator
# THVV 06/01/94 initial
# THVV 05/29/02 updated to build on Linux
# THVV 12/24/03 updated to build on gcc 3.3 (gets warnings on Panther, FreeBSD, and RH9)
#

DEBUGARGS = -g
COMPILER = g++
# on AIX, use COMPILER = xlC
DICT = /usr/share/dict/words
# on AIX, use /usr/dict/words

all : gpw loadtris
	echo gpw created, can delete loadtris

gpw : gpw.o
	$(COMPILER) $(DEBUGARGS) -o gpw gpw.o

trigram.h : loadtris
	./loadtris $(DICT) | sed "s/, }/}/" > trigram.h

gpw.o : gpw.c trigram.h
	$(COMPILER) $(DEBUGARGS) -o gpw.o -c gpw.c
# gcc produces some warnings about long int vs double, shd fix

loadtris : loadtris.o
	$(COMPILER) $(DEBUGARGS) -o loadtris loadtris.o

loadtris.o : loadtris.c
	$(COMPILER) $(DEBUGARGS) -o loadtris.o -c loadtris.c

clean : 
	rm gpw loadtris loadtris.o gpw.o trigram.h
