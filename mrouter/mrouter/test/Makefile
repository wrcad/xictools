
# The tests.  Be aware that openMSP430 takes a loooong time to run.
T1 = map9v3
T2 = openMSP430

# Test-specific target words.  Give with '1' or '2' suffix to specify
# which test from the list above.  E.g., "make test1".
#
VALS = test run runsc qrun foo

# Location for qrouter program, for "make qout1/2"
QRBIN = ..
#QRVERSION = 1.3.57-mod3
QRVERSION = 1.3.62

VALS1 = $(VALS:=1)
VALS2 = $(VALS:=2)

$(VALS1): $(T1).def
	echo $(@:1=)
	$(MAKE) TEST=$(T1) $(@:1=)

$(T1).def:
	cp -f ../../examples/$(T1).def .

$(VALS2): $(T2).def
	echo $(@:2=)
	$(MAKE) TEST=$(T2) $(@:2=)

$(T2).def:
	cp -f ../../examples/$(T2).def .

clean::
	-@rm -f $(T1)_route*.def $(T2)_route*.def lef_out_test.lef

distclean::
	$(MAKE) clean
	rm -f $(T1).def $(T2).def

foo:
	echo $(TEST)

######### The remaining targets are not given directly.

test:
	@if [ x$(TEST) = x ]; then \
	    echo Bad target; \
	    exit 1; \
	fi;
	../mrouter -q $(TEST) -v 1 -c $(TEST).cfg -p vdd -g gnd > /dev/null 2>&1
	$(MAKE) test_lef
	@echo
	@VRS=`../../version`; \
	if [ ! -f $(TEST)_route.def ]; then \
            echo "$(TEST)_route.def file missing!  DEF output failed."; \
        elif [ ! -f reference/$(TEST)-mrouter-$$VRS-q.def ]; then \
            echo "reference/$(TEST)-mrouter-$$VRS-q.def file missing."; \
        elif [ x"`diff -q -Z $(TEST)_route.def \
                reference/$(TEST)-mrouter-$$VRS-q.def`" = x ]; then \
            echo "DEF files $(TEST)_route.def and \
reference/$(TEST)-mrouter-$$VRS-q.def match."; \
	    echo "  Routing test PASSED"; \
	else \
            echo "DEF files $(TEST)_route.def and \
reference/$(TEST)-mrouter-$$VRS-q.def differ."; \
	    echo "  Routing test FAILED"; \
	fi
	@echo

test_lef:
	@echo
	@VRS=`../../version`; \
	if [ ! -f lef_out_test.lef ]; then \
            echo "lef_out_test.lef file missing!  LEF output failed."; \
        elif [ ! -f reference/mrouter-$$VRS-out.lef ]; then \
            echo "reference/mrouter-$$VRS-out.lef file missing."; \
        elif [ x"`diff -q -Z lef_out_test.lef \
                reference/mrouter-$$VRS-out.lef`" = x ]; then \
            echo "LEF files lef_out_test.lef and \
reference/mrouter-$$VRS-out.lef match"; \
	    echo "  LEF test PASSED"; \
	else \
            echo "LEF files lef_out_test.lef and \
reference/mrouter-$$VRS-out.lef differ"; \
	    echo "  LEF test FAILED"; \
	fi

# Run the router using the config file as for Qrouter.  The "-d 0x1"
# which sets Qrouter compatibility mode is actually redundant here, as
# it is set implicitly when the -q option is used.
#
run:
	@if [ x$(TEST) = x ]; then \
	    echo Bad target; \
	    exit 1; \
	fi;
	../mrouter -v 1 -c $(TEST).cfg -p vdd -g gnd -q $(TEST) -d 0x1

# Run the router using the script from the examples.  Output routes
# may be a little different since internal ordering differs from
# Qrouter without the compatibility flag (debug = 0x1) set.
#
runsc:
	@if [ x$(TEST) = x ]; then \
	    echo Bad target; \
	    exit 1; \
	fi;
	../mrouter < ../../examples/$(TEST).rsc

# Run a qrouter that happens to be installed.  This assumes that the
# non-Tcl version has been built.  Don't expect the output to match
# MRouter, even in "Qrouter compatibility" mode, as there are a number
# of things which may cause differences.  The only real test of
# success and equivalence is that both layouts pass an LVS and DRC.
#
qrun:
	@if [ x$(TEST) = x ]; then \
	    echo Bad target; \
	    exit 1; \
	fi;
	$(QRBIN)/qrouter-$(QRVERSION)/qrouter -v 1 -c $(TEST).cfg -p vdd \
 -g gnd $(TEST)

