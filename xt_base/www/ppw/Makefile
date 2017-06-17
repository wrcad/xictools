
CC = g++
CFLAGS = -O -Wall -W -Wshadow -Wwrite-strings -DPWDIR="\"`pwd`/\""

INCLUDE = -I.

all: ppw

ppw: ppw.o
	$(CC) -o $@ ppw.o

.cc.o:
	$(CC) -c $(CFLAGS) $(INCLUDE) $*.cc

clean::
	-rm -f *.o ppw

