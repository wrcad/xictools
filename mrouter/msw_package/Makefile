
# This will create a Windows package in the current directory.  It needs
# 1) the inno packager installed in /inno-5.5.1
# 2) a u2d utility (Cygwin?)
# 
# You can tune mkwinpkg and/or package.iss.in to your needs.


package::
	./mkwinpkg
	rm -rf data package.iss

clean::
	rm -rf data package.iss *.exe *.log

distclean::
	rm -rf data package.iss *.exe *.log
