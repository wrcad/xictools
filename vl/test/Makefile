# examples_ivl/Makefile

#  make test
#  This is like the run script, creates test.out which is compared to
#  run.out.  There should be no differences.
#
#  make clean
#  Delete the junk files produced by running ivltests.

ivltests:
	if [ ! -L ivltests ]; then \
	    ln -s ../examples_ivl/ivltests; \
	fi;

test: ivltests
	(for a in ivltests/*.v; do ../vl $$a; done) 2>&1 |\
 tee test.out
	-@echo ;echo; echo "Running diff test.out run.out..."
	-@diff test.out ../examples_ivl/run.out; \
 if [ $$? == 0 ]; then echo "output matches reference"; fi

clean::
	-@rm -f test.out fopen1.out multdiv.vcd out.dump test.vcd verilog.dump\
  ivltests
