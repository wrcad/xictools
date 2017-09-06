
# This creates the adms_examples used by the packagers.  This must be
# kept in sync with the examples tree.
#
# Do make distclean in devlib/adms/examples first!
#
# Be sure to edit the list in case any crap files sneak in.

adms_examples::
	-@if [ -f adms_examples ]; then \
	    mv adms_examples adms_examples.bak; \
	fi
	-@aex=`pwd`/adms_examples; \
	cd ../../devlib/adms; find examples -type f | \
  sed -e s/Makefile\\.in/Makefile/ | sort > $$aex

