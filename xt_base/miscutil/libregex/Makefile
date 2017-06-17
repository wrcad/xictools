SRCS =	regcomp.c regerror.c regexec.c regfree.c
OBJS =	regcomp.o regerror.o regexec.o regfree.o

prefix = /usr/local/mingw32

TARGET = libregex.a

$(TARGET): $(OBJS)
	ar r $(TARGET) $?
	ranlib $(TARGET)

.c.o:
	gcc -c $*.c

install:
	install libregex.a $(prefix)/lib
	install regex.h $(prefix)/include

clean:
	rm *.a *.o
