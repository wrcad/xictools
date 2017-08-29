#######################################################################
# packages/util/Makefile
#######################################################################

install::
	if [ -d ../pkgfiles ]; then \
	    cp -fp wr_install ../pkgfiles; \
	fi

clean::
	if [ -d ../pkgfiles ]; then \
	    rm -f ../pkgfiles/wr_install; \
	fi

distclean:: clean
	rm -f utod.exe

utod.exe: utod.c
	gcc -O utod.c -o utod.exe

