
#
#	Example Makefile to run tests and check results.
#
#	This is an example makefile for running QA tests on a
#	model and then checking the simulated results against
#	reference results. A separate target is defined for each
#	variant of the model. The program runQaTests.pl runs the
#	tests, and that program expects a perl module SIMULATOR.pm
#	to be provided for each simulator that is tested.
#	Examples of these are provided.
#

include		../../files/make.config

#########################################
# CONFIGURE ME

# Only one of these lines should be un-commented.  As-is, the QA is
# for an internal device model.  To use with an external loadable
# module, comment the first line, uncomment the second, and change the
# XXX's to to point to the module file.
#
module		= -mnone
#module		= -m $(vabuild)/XXX/XXX.$(soext)

# Uncomment one of these:  -nw suppresses warnings and removes temporary
# circuit file, -d retains temporary circuit file for debugging.
#
testProgramFlags	=	-nw
#testProgramFlags	=	-d

# End of configuration
#########################################

testProgramName	= ../../lib/runQaTests.pl $($@_simCmd)
wrspice_simCmd	= -sc "$(wrspice) $(module)"

qaSpecFile		=	qaSpec
qaResultsDirectory	=	results

help:
	@echo "" ; \
	echo "Valid targets are:" ; \
	echo "" ; \
	echo "all                run tests and compare results for all simulators" ; \
	echo "" ; \
	echo "spice              run tests and compare results spice" ; \
	echo "wrspice            run tests and compare results wrspice" ; \
	echo "hspice             run tests and compare results hspice" ; \
	echo "spectre            run tests and compare results spectre" ; \
	echo "ads                run tests and compare results ads" ; \
	echo "adsSiMKit          run tests and compare results ads with SiMKit" ; \
	echo "eldo               run tests and compare results eldo" ; \
	echo "nexxim             run tests and compare results nexxim" ; \
	echo "smartspice         run tests and compare results smartspice" ; \
	echo "" ; \
	echo "clean              remove all previously generated simulation results"; \
	echo "" ; \
	echo "NOTE: if test results exist they are not resimulated" ; \
	echo "NOTE: to force resimulation run \"make clean\" first" ; \
	echo ""

all:	spice hspice spectre spectreSiMKit ads adsSiMKit eldo nexxim smartspice

test:
	$(wrspice) $(module) < wrspiceCkt

test1:
	$(wrspice) $(module) wrspiceCkt

#####
##### common target for all simulators
#####

% :
	@-echo ""; \
	localPlatform=`$(testProgramName) -platform` ; \
	localVersion=`$(testProgramName) -sv -s $@ $(qaSpecFile) | awk '{print $$1}'` ; \
	localVersionAndPlatform=$$localVersion._.$$localPlatform ; \
	echo "******"; \
	echo "****** $(qaSpecFile) tests for $@"; \
	echo "****** (for version $$localVersion on platform $$localPlatform)"; \
	echo "******"; \
	for test in `$(testProgramName) -lt -s $@ $(qaSpecFile)` ; \
	do \
	    echo ""; \
	    echo "****** Checking test ($@): $$test" ; \
	    for variant in `$(testProgramName) -lv -s $@ $(qaSpecFile)` ; \
	    do \
	        $(testProgramName) $(testProgramFlags) -s $@ -r -t $$test -var $$variant $(qaSpecFile) ; \
	    done ; \
	done ; \
	for version in `ls -C1 $(qaResultsDirectory)/$@` ; \
	do \
	    for platform in `ls -C1 $(qaResultsDirectory)/$@/$$version` ; \
	    do \
	        versionAndPlatform=$$version._.$$platform ; \
	        if [ $$versionAndPlatform = $$localVersionAndPlatform ] ; \
	        then \
	            break ; \
	        fi ; \
	        echo "" ; \
	        echo "******"; \
	        echo "****** Comparing previously run $(qaSpecFile) tests for $@"; \
	        echo "****** (for version $$version on platform $$platform)"; \
	        echo "****** (and local Version and Platform: $$localVersionAndPlatform)"; \
	        echo "******"; \
	        for test in `$(testProgramName) -lt -s $@ $(qaSpecFile)` ; \
	        do \
	            echo ""; \
	            echo "****** Checking test ($@): $$test" ; \
	            for variant in `$(testProgramName) -lv -s $@ $(qaSpecFile)` ; \
	            do \
	                $(testProgramName) -c $$version $$platform -s $@ -t $$test -var $$variant $(qaSpecFile) ; \
	            done ; \
	        done ; \
	    done ; \
	done

clean:
	@/bin/rm -rf $(qaResultsDirectory)/spice spiceCkt*
	@/bin/rm -rf $(qaResultsDirectory)/wrspice wrspiceCkt*
	@/bin/rm -rf $(qaResultsDirectory)/hspice hspiceCkt*
	@/bin/rm -rf $(qaResultsDirectory)/spectre spectreCkt* *ahdlcmi
	@/bin/rm -rf $(qaResultsDirectory)/ads adsCkt* spectra.raw
	@/bin/rm -rf $(qaResultsDirectory)/adsSiMKit adsSiMKitCkt* spectra.raw
	@/bin/rm -rf $(qaResultsDirectory)/eldo eldoCkt*
	@/bin/rm -rf $(qaResultsDirectory)/nexxim *.sdf *.csv *.csv.*
	@/bin/rm -rf $(qaResultsDirectory)/smartspice smartspiceCkt*
