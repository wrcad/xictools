
# This creates the adms_examples used by the packagers.  This must be
# kept in sync with the examples tree.
#
# Do this first: 
#   cd devlib/adms/examples
#   make distclean
#   make make
#
# Be sure to edit the list in case any crap files sneak in.

adms_examples::
	-@if [ -f adms_examples ]; then \
	    mv adms_examples adms_examples.bak; \
	fi
	-@aex=`pwd`/adms_examples; \
	cd ../../devlib/adms; find examples -type f \! -name ".gitignore"| \
  sed -e s/Makefile\\.in/Makefile/ | sort > $$aex

